#pragma once

#include <memory>
#include <cstdint>
#include <alc.h>
#include <efx-presets.h>
#include <unordered_map>

namespace AL
{
class Device
{
public:
	struct Functions;

	Device();
	~Device();
	Device( Device&& ) = delete;
	Device( const Device& ) = delete;

	ALCdevice* device() const
	{
		return m_openALDevice;
	}

	const Functions* fn() const
	{
		return m_functions.get();
	}

private:
	ALCdevice* const m_openALDevice = nullptr;
	std::unique_ptr<Functions> m_functions;
};

class Context
{
public:
	Context( const std::shared_ptr<Device>& device );
	~Context();
	Context( Context&& ) = delete;
	Context( const Context& ) = delete;

	std::shared_ptr<const Device> device() const
	{
		return std::const_pointer_cast<const Device>( m_device );
	}

	ALCcontext* context() const
	{
		return m_openACLContext;
	}

	bool hasEFX() const
	{
		return m_hasEFX;
	}
	bool hasThreadLocal() const
	{
		return m_hasThreadLocal;
	}

	class Lock
	{
	public:
		Lock( const std::shared_ptr<Context>& context );
		~Lock();

	private:
		std::shared_ptr<Context> m_context;
	};

private:
	std::shared_ptr<Device> m_device;
	ALCcontext* m_openACLContext = nullptr;
	bool m_hasEFX                = false;
	bool m_hasThreadLocal        = false;
};

class Object
{
public:
	Object( const std::shared_ptr<Context>& context );
	~Object();
	Object()                  = delete;
	Object( Object&& )      = delete;
	Object( const Object& ) = delete;
	std::shared_ptr<const Context> context() const
	{
		return std::const_pointer_cast<const Context>( m_context );
	}

	const Device::Functions* fn() const
	{
		return m_context->device()->fn();
	}

protected:
	void checkContextActive() const;

private:
	std::shared_ptr<Context> m_context;
};

class Listener : public Object
{
public:
	Listener( const std::shared_ptr<Context>& context );
	~Listener();
	void setOrientation( const float forward[3], const float up[3] );
	void setPosition( const float position[3] );
	void setVolume( float volume );
};

class Buffer : public Object
{
public:
	Buffer( const std::shared_ptr<Context>& context, const std::string& fileName );
	~Buffer();
	uint32_t buffer() const
	{
		return m_buffer;
	}

private:
	uint32_t m_buffer = 0;
};

class Effect : public Object
{
public:
	Effect( const std::shared_ptr<Context>& context );
	~Effect();
	uint32_t effect() const
	{
		return m_effect;
	}

private:
	uint32_t m_effect = 0;
};

class ReverbEffect : public Effect
{
public:
	ReverbEffect( const std::shared_ptr<Context>& context );
	~ReverbEffect();
	void setPreset( const EFXEAXREVERBPROPERTIES& preset );

private:
	const bool m_hasEAX;
};

class EffectSlot : public Object
{
public:
	EffectSlot( const std::shared_ptr<Context>& context, const std::shared_ptr<Effect>& effect );
	~EffectSlot();
	uint32_t slot() const
	{
		return m_slot;
	}

private:
	std::shared_ptr<Effect> m_effect;
	uint32_t m_slot = 0;
};

class Filter : public Object
{
public:
	Filter( const std::shared_ptr<Context>& context );
	~Filter();
	void setGain( float lfGain, float midGain, float hfGain );
	uint32_t filter() const
	{
		return m_filter;
	}

private:
	uint32_t m_filter = 0;
};

class Source : public Object
{
public:
	Source( const std::shared_ptr<Context>& context, std::shared_ptr<Buffer> buffer );
	~Source();

	enum PlayState
	{
		INITIAL = 0,
		PLAYING,
		PAUSED,
		STOPPED
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
	std::shared_ptr<Buffer> m_buffer;
	std::shared_ptr<Filter> m_dryPath;
	std::unordered_map<uint32_t, std::pair<std::shared_ptr<EffectSlot>, std::shared_ptr<Filter>>> m_wetPath;
	uint32_t m_source = 0;
};
} // namespace AL
