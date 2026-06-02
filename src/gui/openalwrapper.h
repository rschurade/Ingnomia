/** @file openalwrapper.h
 *  @brief Thin RAII C++ wrapper around OpenAL Soft primitives used by AggregatorSound.
 *         Wraps Device, Context, Listener, Buffer, Effect, EffectSlot, Filter, and Source
 *         with non-copyable, non-movable handle classes that manage lifetime via shared_ptr.
 */
#pragma once

#include <memory>
#include <cstdint>
#include <alc.h>
#include <efx-presets.h>

namespace AL
{
/// @brief Owns an ALCdevice handle and a dynamically loaded function table (for EFX support).
class Device
{
public:
	struct Functions;

	Device();
	~Device();
	Device( Device&& ) = delete;
	Device( const Device& ) = delete;

	/// @brief Returns the raw ALCdevice handle.
	ALCdevice* device() const
	{
		return m_openALDevice;
	}

	/// @brief Returns the EFX function-pointer table (may be null if EFX is unsupported).
	const Functions* fn() const
	{
		return m_functions.get();
	}

private:
	ALCdevice* const m_openALDevice = nullptr;  ///< Open OpenAL device handle.
	std::unique_ptr<Functions> m_functions;     ///< EFX entry point table.
};

/// @brief Owns an ALCcontext for a given Device. Also exposes EFX and thread-local support
///        flags and provides a scoped Lock helper that makes the context current.
class Context
{
public:
	Context( const std::shared_ptr<Device>& device );
	~Context();
	Context( Context&& ) = delete;
	Context( const Context& ) = delete;

	/// @brief Returns the owning Device.
	std::shared_ptr<const Device> device() const
	{
		return std::const_pointer_cast<const Device>( m_device );
	}

	/// @brief Returns the raw ALCcontext handle.
	ALCcontext* context() const
	{
		return m_openACLContext;
	}

	/// @brief Returns true if the driver exposes the ALC_EXT_EFX extension.
	bool hasEFX() const
	{
		return m_hasEFX;
	}
	/// @brief Returns true if the driver supports per-thread current contexts.
	bool hasThreadLocal() const
	{
		return m_hasThreadLocal;
	}

	/// @brief RAII helper that makes the enclosing context current for the scope of the
	///        Lock object. Needed for any AL API call outside the main audio thread.
	class Lock
	{
	public:
		Lock( const std::shared_ptr<Context>& context );
		~Lock();

	private:
		std::shared_ptr<Context> m_context; ///< Context held by this lock.
	};

private:
	std::shared_ptr<Device> m_device;           ///< Owning Device.
	ALCcontext* m_openACLContext = nullptr;     ///< Raw ALCcontext handle.
	bool m_hasEFX                = false;       ///< Extension support flag.
	bool m_hasThreadLocal        = false;       ///< Extension support flag.
};

/// @brief Base class for every AL resource wrapper (Listener, Buffer, Source, …). Holds a
///        shared pointer to the Context and provides a checkContextActive() helper that
///        traps accidental cross-context usage.
class Object
{
public:
	Object( const std::shared_ptr<Context>& context );
	~Object();
	Object()                  = delete;
	Object( Object&& )      = delete;
	Object( const Object& ) = delete;

	/// @brief Returns the owning Context.
	std::shared_ptr<const Context> context() const
	{
		return std::const_pointer_cast<const Context>( m_context );
	}

	/// @brief Returns the EFX function table from the owning Device.
	const Device::Functions* fn() const
	{
		return m_context->device()->fn();
	}

protected:
	void checkContextActive() const;

private:
	std::shared_ptr<Context> m_context;  ///< Owning Context.
};

/// @brief The single listener associated with a Context. Setters forward to alListener*.
class Listener : public Object
{
public:
	Listener( const std::shared_ptr<Context>& context );
	~Listener();
	void setOrientation( const float forward[3], const float up[3] );
	void setPosition( const float position[3] );
	void setVolume( float volume );
};

/// @brief An OpenAL buffer loaded from an audio file (WAV/OGG) via libsndfile or similar.
class Buffer : public Object
{
public:
	Buffer( const std::shared_ptr<Context>& context, const std::string& fileName );
	~Buffer();
	/// @brief Returns the raw OpenAL buffer handle.
	uint32_t buffer() const
	{
		return m_buffer;
	}

private:
	uint32_t m_buffer = 0;  ///< Raw OpenAL buffer handle.
};

/// @brief An EFX effect object (base class; concrete effect type is set by subclasses).
class Effect : public Object
{
public:
	Effect( const std::shared_ptr<Context>& context );
	~Effect();
	/// @brief Returns the raw OpenAL effect handle.
	uint32_t effect() const
	{
		return m_effect;
	}

private:
	uint32_t m_effect = 0;  ///< Raw OpenAL effect handle.
};

/// @brief An EFX reverb effect. Falls back to a normal reverb if the driver lacks EAX support.
class ReverbEffect : public Effect
{
public:
	ReverbEffect( const std::shared_ptr<Context>& context );
	~ReverbEffect();
	void setPreset( const EFXEAXREVERBPROPERTIES& preset );

private:
	const bool m_hasEAX;    ///< True if the EAX reverb extension is available.
};

/// @brief An EFX auxiliary effect slot that hosts an Effect. Sources send their wet path into this.
class EffectSlot : public Object
{
public:
	EffectSlot( const std::shared_ptr<Context>& context, const std::shared_ptr<Effect>& effect );
	~EffectSlot();
	/// @brief Returns the raw OpenAL slot handle.
	uint32_t slot() const
	{
		return m_slot;
	}

private:
	std::shared_ptr<Effect> m_effect;  ///< Effect hosted by this slot.
	uint32_t m_slot = 0;                ///< Raw OpenAL effect slot handle.
};

/// @brief An EFX low-pass/band-pass/high-pass filter applied to a source's dry or wet path.
class Filter : public Object
{
public:
	Filter( const std::shared_ptr<Context>& context );
	~Filter();
	void setGain( float lfGain, float midGain, float hfGain );
	/// @brief Returns the raw OpenAL filter handle.
	uint32_t filter() const
	{
		return m_filter;
	}

private:
	uint32_t m_filter = 0;  ///< Raw OpenAL filter handle.
};

/// @brief A playable OpenAL source attached to a Buffer. Supports 3D positioning, pitch,
///        volume, relative listener mode, dry-path filter, and multiple wet-path effect sends.
class Source : public Object
{
public:
	Source( const std::shared_ptr<Context>& context, std::shared_ptr<Buffer> buffer );
	~Source();

	/// @brief AL source play state, mirroring AL_SOURCE_STATE enum values.
	enum PlayState
	{
		INITIAL = 0, ///< Newly created, never played.
		PLAYING,     ///< Currently playing.
		PAUSED,      ///< Paused mid-playback.
		STOPPED      ///< Finished or explicitly stopped.
	};

	void setRelativeToListener( bool relative );
	void setVolume( float volume );
	void setPosition( float x, float y, float z );
	void setPitch( float pitch );
	void rewind();
	void play();
	void stop();
	void setDryPathFilter( const std::shared_ptr<Filter>& filter );
	void setWetPathEffect( uint32_t slot, const std::shared_ptr<EffectSlot>& effectSlot, const std::shared_ptr<Filter>& filter );
	PlayState getPlayState() const;

private:
	std::shared_ptr<Buffer> m_buffer;           ///< Buffer attached to this source.
	std::shared_ptr<Filter> m_dryPath;          ///< Active dry-path filter (nullable).
	std::unordered_map<uint32_t, std::pair<std::shared_ptr<EffectSlot>, std::shared_ptr<Filter>>> m_wetPath; ///< Active wet-path slot/filter pairs keyed by send index.
	uint32_t m_source = 0;                      ///< Raw OpenAL source handle.
};
} // namespace AL
