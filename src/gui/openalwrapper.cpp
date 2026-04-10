/** @file openalwrapper.cpp
 *  @brief RAII OpenAL wrapper implementation: alCall/alcCall helpers for error checking,
 *         a minimal WAV loader, and constructors/destructors for every wrapper class.
 */
#include "openalwrapper.h"

#include <al.h>
#include <alc.h>
#include <alext.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace AL;

#define _DEBUG

#ifdef _DEBUG
#define validate() Object::checkContextActive()
#else
#define validate() void()
#endif

struct Device::Functions
{
	/* Thread local context functions */
	PFNALCSETTHREADCONTEXTPROC alcSetThreadContext;
	PFNALCGETTHREADCONTEXTPROC alcGetThreadContext;

	/* Filter object functions */
	LPALGENFILTERS alGenFilters;
	LPALDELETEFILTERS alDeleteFilters;
	LPALISFILTER alIsFilter;
	LPALFILTERI alFilteri;
	LPALFILTERIV alFilteriv;
	LPALFILTERF alFilterf;
	LPALFILTERFV alFilterfv;
	LPALGETFILTERI alGetFilteri;
	LPALGETFILTERIV alGetFilteriv;
	LPALGETFILTERF alGetFilterf;
	LPALGETFILTERFV alGetFilterfv;

	/* Effect object functions */
	LPALGENEFFECTS alGenEffects;
	LPALDELETEEFFECTS alDeleteEffects;
	LPALISEFFECT alIsEffect;
	LPALEFFECTI alEffecti;
	LPALEFFECTIV alEffectiv;
	LPALEFFECTF alEffectf;
	LPALEFFECTFV alEffectfv;
	LPALGETEFFECTI alGetEffecti;
	LPALGETEFFECTIV alGetEffectiv;
	LPALGETEFFECTF alGetEffectf;
	LPALGETEFFECTFV alGetEffectfv;

	/* Auxiliary Effect Slot object functions */
	LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
	LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
	LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
	LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
	LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
	LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
	LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
	LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
	LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
	LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
	LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;

	Functions( const Device& device )
	{
		/* Define a macro to help load the function pointers. */
#define LOAD_PROC( x ) ( ( x ) = (decltype( x ))alcGetProcAddress( device.device(), #x ) )
		LOAD_PROC( alcSetThreadContext );
		LOAD_PROC( alcGetThreadContext );

		LOAD_PROC( alGenFilters );
		LOAD_PROC( alDeleteFilters );
		LOAD_PROC( alIsFilter );
		LOAD_PROC( alFilteri );
		LOAD_PROC( alFilteriv );
		LOAD_PROC( alFilterf );
		LOAD_PROC( alFilterfv );
		LOAD_PROC( alGetFilteri );
		LOAD_PROC( alGetFilteriv );
		LOAD_PROC( alGetFilterf );
		LOAD_PROC( alGetFilterfv );

		LOAD_PROC( alGenEffects );
		LOAD_PROC( alDeleteEffects );
		LOAD_PROC( alIsEffect );
		LOAD_PROC( alEffecti );
		LOAD_PROC( alEffectiv );
		LOAD_PROC( alEffectf );
		LOAD_PROC( alEffectfv );
		LOAD_PROC( alGetEffecti );
		LOAD_PROC( alGetEffectiv );
		LOAD_PROC( alGetEffectf );
		LOAD_PROC( alGetEffectfv );

		LOAD_PROC( alGenAuxiliaryEffectSlots );
		LOAD_PROC( alDeleteAuxiliaryEffectSlots );
		LOAD_PROC( alIsAuxiliaryEffectSlot );
		LOAD_PROC( alAuxiliaryEffectSloti );
		LOAD_PROC( alAuxiliaryEffectSlotiv );
		LOAD_PROC( alAuxiliaryEffectSlotf );
		LOAD_PROC( alAuxiliaryEffectSlotfv );
		LOAD_PROC( alGetAuxiliaryEffectSloti );
		LOAD_PROC( alGetAuxiliaryEffectSlotiv );
		LOAD_PROC( alGetAuxiliaryEffectSlotf );
		LOAD_PROC( alGetAuxiliaryEffectSlotfv );
#undef LOAD_PROC
	}
};

static std::uint32_t read_uint32( const char* buffer )
{
	uint32_t tmp = 0;
	std::memcpy( &tmp, buffer, 4 );
	return tmp;
}

static std::uint32_t read_uint16( const char* buffer )
{
	uint32_t tmp = 0;
	std::memcpy( &tmp, buffer, 2 );
	return tmp;
}

static void load_wav_file_header( std::ifstream& file, std::uint8_t& channels, std::uint32_t& sampleRate, std::uint8_t& bitsPerSample, ALsizei& size )
{
	char buffer[4];
	if ( !file.is_open() )
		throw std::logic_error( "File not open" );

	// the RIFF
	if ( !file.read( buffer, 4 ) )
	{
		throw std::logic_error( "ERROR: could not read RIFF" );
	}
	if ( std::strncmp( buffer, "RIFF", 4 ) != 0 )
	{
		throw std::logic_error( "ERROR: file is not a valid WAVE file (header doesn't begin with RIFF)" );
	}

	// the size of the file
	if ( !file.read( buffer, 4 ) )
	{
		throw std::logic_error( "ERROR: could not read size of file" );
	}

	// the WAVE
	if ( !file.read( buffer, 4 ) )
	{
		throw std::logic_error( "ERROR: could not read WAVE" );
	}
	if ( std::strncmp( buffer, "WAVE", 4 ) != 0 )
	{
		throw std::logic_error( "ERROR: file is not a valid WAVE file (header doesn't contain WAVE)" );
	}

	// "fmt/0"
	if ( !file.read( buffer, 4 ) )
	{
		throw std::logic_error( "ERROR: could not read fmt/0" );
	}

	// this is always 16, the size of the fmt data chunk
	if ( !file.read( buffer, 4 ) )
	{
		throw std::logic_error( "ERROR: could not read the 16" );
	}

	// PCM should be 1?
	if ( !file.read( buffer, 2 ) )
	{
		throw std::logic_error( "ERROR: could not read PCM" );
	}

	// the number of channels
	if ( !file.read( buffer, 2 ) )
	{
		throw std::logic_error( "ERROR: could not read number of channels" );
	}
	channels = read_uint16( buffer );

	// sample rate
	if ( !file.read( buffer, 4 ) )
	{
		throw std::logic_error( "ERROR: could not read sample rate" );
	}
	sampleRate = read_uint32( buffer );

	// (sampleRate * bitsPerSample * channels) / 8
	if ( !file.read( buffer, 4 ) )
	{
		throw std::logic_error( "ERROR: could not read (sampleRate * bitsPerSample * channels) / 8" );
	}

	// ?? dafaq
	if ( !file.read( buffer, 2 ) )
	{
		throw std::logic_error( "ERROR: could not read dafaq" );
	}

	// bitsPerSample
	if ( !file.read( buffer, 2 ) )
	{
		throw std::logic_error( "ERROR: could not read bits per sample" );
	}
	bitsPerSample = read_uint16( buffer );

	// data chunk header "data"
	if ( !file.read( buffer, 4 ) )
	{
		throw std::logic_error( "ERROR: could not read data chunk header" );
	}
	if ( std::memcmp( buffer, "data", 4 ) != 0 )
	{
		throw std::logic_error( "ERROR: file is not a valid WAVE file (doesn't have 'data' tag)" );
	}

	// size of data
	if ( !file.read( buffer, 4 ) )
	{
		throw std::logic_error( "ERROR: could not read data size" );
	}
	size = read_uint32( buffer );

	/* cannot be at the end of file */
	if ( file.eof() )
	{
		throw std::logic_error( "ERROR: reached EOF on the file" );
	}
	if ( file.fail() )
	{
		throw std::logic_error( "ERROR: fail state set on the file" );
	}
}

static std::vector<char> load_wav( const std::string& filename, std::uint8_t& channels, std::uint32_t& sampleRate, std::uint8_t& bitsPerSample )
{
	std::ifstream in( filename, std::ios::binary );
	if ( !in.is_open() )
	{
		throw std::logic_error( "ERROR: Could not open \"" + filename + "\"" );
	}
	ALsizei size;
	load_wav_file_header( in, channels, sampleRate, bitsPerSample, size );

	std::vector<char> data;
	data.resize( size );
	in.read( data.data(), data.size() );

	return data;
}

static void check_al_errors()
{
	ALenum error = alGetError();
	if ( error != AL_NO_ERROR )
	{
		std::stringstream ss;
		switch ( error )
		{
			case AL_INVALID_NAME:
				ss << "AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function";
				break;
			case AL_INVALID_ENUM:
				ss << "AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function";
				break;
			case AL_INVALID_VALUE:
				ss << "AL_INVALID_VALUE: an invalid value was passed to an OpenAL function";
				break;
			case AL_INVALID_OPERATION:
				ss << "AL_INVALID_OPERATION: the requested operation is not valid";
				break;
			case AL_OUT_OF_MEMORY:
				ss << "AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out of memory";
				break;
			default:
				ss << "UNKNOWN AL ERROR: " << error;
		}
		throw std::invalid_argument( ss.str() );
	}
}

template <typename alFunction, typename... Params>
auto alCall( alFunction function, Params... params )
	-> typename std::enable_if_t<std::is_same_v<void, decltype( function( params... ) )>, void>
{
	function( std::forward<Params>( params )... );
	check_al_errors();
}

template <typename alFunction, typename... Params>
auto alCall( alFunction function, Params... params )
	-> typename std::enable_if_t<!std::is_same_v<void, decltype( function( params... ) )>, decltype( function( params... ) )>
{
	auto ret = function( std::forward<Params>( params )... );
	check_al_errors();
	return ret;
}

static void check_alc_errors( ALCdevice* device )
{
	ALCenum error = alcGetError( device );
	if ( error != ALC_NO_ERROR )
	{
		std::stringstream ss;
		switch ( error )
		{
			case ALC_INVALID_VALUE:
				ss << "ALC_INVALID_VALUE: an invalid value was passed to an OpenAL function";
				break;
			case ALC_INVALID_DEVICE:
				ss << "ALC_INVALID_DEVICE: a bad device was passed to an OpenAL function";
				break;
			case ALC_INVALID_CONTEXT:
				ss << "ALC_INVALID_CONTEXT: a bad context was passed to an OpenAL function";
				break;
			case ALC_INVALID_ENUM:
				ss << "ALC_INVALID_ENUM: an unknown enum value was passed to an OpenAL function";
				break;
			case ALC_OUT_OF_MEMORY:
				ss << "ALC_OUT_OF_MEMORY: an unknown enum value was passed to an OpenAL function";
				break;
			default:
				ss << "UNKNOWN ALC ERROR: " << error;
		}
		throw std::invalid_argument( ss.str() );
	}
}

template <typename alcFunction, typename... Params>
auto alcCall( alcFunction function, ALCdevice* device, Params... params )
	-> typename std::enable_if_t<std::is_same_v<void, decltype( function( params... ) )>, void>
{
	function( std::forward<Params>( params )... );
	check_alc_errors( device );
}

template <typename alcFunction, typename... Params>
auto alcCall( alcFunction function, ALCdevice* device, Params... params )
	-> typename std::enable_if_t<!std::is_same_v<void, decltype( function( params... ) )>, decltype( function( params... ) )>
{
	auto returnValue = function( std::forward<Params>( params )... );
	check_alc_errors( device );
	return returnValue;
}

/// @brief Opens the system default OpenAL device and loads the EFX function table.
///        Throws std::logic_error if no device is available.
Device::Device() :
	m_openALDevice( alcOpenDevice( nullptr ) )
{
	if ( !m_openALDevice )
		throw std::logic_error( "Could not open device" );
	m_functions = std::make_unique<Functions>( *this );
}

/// @brief Closes the OpenAL device.
Device::~Device()
{
	ALCboolean closed = alcCall( alcCloseDevice, m_openALDevice, m_openALDevice );
}

/// @brief Creates an ALCcontext on the given device and probes ALC_EXT_EFX /
///        ALC_EXT_thread_local_context availability.
/// @param device Owning Device (must be non-null).
Context::Context( const std::shared_ptr<Device>& device ) :
	m_device( device ),
	m_openACLContext( alcCall( alcCreateContext, m_device->device(), m_device->device(), nullptr ) )
{
	if ( !m_openACLContext )
	{
		throw std::logic_error( "Could not create context" );
	}
	m_hasEFX         = alcCall( alcIsExtensionPresent, m_device->device(), m_device->device(), ALC_EXT_EFX_NAME );
	m_hasThreadLocal = alcCall( alcIsExtensionPresent, m_device->device(), m_device->device(), "ALC_EXT_thread_local_context" );
}

/// @brief Destroys the ALCcontext.
Context::~Context()
{
	alcCall( alcDestroyContext, m_device->device(), m_openACLContext );
}

/// @brief Makes @p context current on the calling thread using either the thread-local
///        extension or the standard alcMakeContextCurrent call. Throws if another context
///        is already active (debug builds only) or if activation fails.
/// @param context Context to make current.
Context::Lock::Lock( const std::shared_ptr<Context>& context ) :
	m_context( context )
{
#ifdef _DEBUG
	auto activeContext = m_context->hasThreadLocal() ? m_context->device()->fn()->alcGetThreadContext() : alcGetCurrentContext();
	if ( activeContext )
	{
		throw std::logic_error( "Another OpenAL context is already active" );
	}
#endif // _DEBUG
	if ( m_context->hasThreadLocal() )
	{
		ALCboolean contextMadeCurrent = alcCall( m_context->device()->fn()->alcSetThreadContext, m_context->device()->device(), m_context->context() );
		if ( !contextMadeCurrent )
		{
			throw std::logic_error( "Failed to activate OpenAL context" );
		}
	}
	else
	{
		ALCboolean contextMadeCurrent = alcCall( alcMakeContextCurrent, m_context->device()->device(), m_context->context() );
		if ( !contextMadeCurrent )
		{
			throw std::logic_error( "Failed to activate OpenAL context" );
		}
	}
}

/// @brief Clears the current context on the calling thread, releasing the lock.
Context::Lock::~Lock()
{
	if ( m_context->hasThreadLocal() )
	{
		alcCall( m_context->device()->fn()->alcSetThreadContext, m_context->device()->device(), nullptr );
	}
	else
	{
		alcCall( alcMakeContextCurrent, m_context->device()->device(), nullptr );
	}
}

/// @brief Loads a WAV file from disk, allocates an AL buffer, and uploads the decoded
///        samples. Throws if the channel/bit-depth combination is unsupported.
/// @param context  Owning Context (must be current during the call).
/// @param fileName Path to the .wav file.
Buffer::Buffer( const std::shared_ptr<Context>& context, const std::string& fileName ) :
	Object( context )
{
	validate();
	alCall( alGenBuffers, 1, &m_buffer );
	std::uint8_t channels;
	std::uint32_t sampleRate;
	std::uint8_t bitsPerSample;
	auto soundData = load_wav( fileName, channels, sampleRate, bitsPerSample );
	ALenum format;
	if ( channels == 1 && bitsPerSample == 8 )
		format = AL_FORMAT_MONO8;
	else if ( channels == 1 && bitsPerSample == 16 )
		format = AL_FORMAT_MONO16;
	else if ( channels == 2 && bitsPerSample == 8 )
		format = AL_FORMAT_STEREO8;
	else if ( channels == 2 && bitsPerSample == 16 )
		format = AL_FORMAT_STEREO16;
	else
	{
		std::stringstream ss;
		ss << "Unrecognised wave format: " << channels << " channels, " << bitsPerSample << " bps";
		throw std::invalid_argument( ss.str() );
	}
	alCall( alBufferData, m_buffer, format, soundData.data(), static_cast<ALsizei>( soundData.size() ), sampleRate );
}

/// @brief Releases the AL buffer handle.
Buffer::~Buffer()
{
	validate();
	alCall( alDeleteBuffers, 1, &m_buffer );
}

/// @brief Creates an AL source, attaches @p buffer, and sets reasonable defaults (rolloff,
///        reference/max distance, gain 1.0).
/// @param context Owning Context.
/// @param buffer  Buffer to attach to the source.
Source::Source( const std::shared_ptr<Context>& context, std::shared_ptr<Buffer> buffer ) :
	Object( context ),
	m_buffer( buffer )
{
	validate();
	alCall( alGenSources, 1, &m_source );

	alCall( alSourcef, m_source, AL_PITCH, 1.0f );
	alCall( alSourcef, m_source, AL_GAIN, 1.0f );
	alCall( alSource3f, m_source, AL_POSITION, 0, 0, 0 );
	alCall( alSource3f, m_source, AL_VELOCITY, 0, 0, 0 );
	alCall( alSourcei, m_source, AL_LOOPING, AL_FALSE );
	alCall( alSourcei, m_source, AL_BUFFER, m_buffer->buffer() );
}

/// @brief Stops playback and releases the AL source handle.
Source::~Source()
{
	validate();
	alCall( alDeleteSources, 1, &m_source );
}

/// @brief Toggles whether the source position is specified relative to the listener (HUD
///        sounds) or in absolute world coordinates.
/// @param relative True for listener-relative, false for absolute.
void Source::setRelativeToListener( bool relative )
{
	validate();
	alCall( alSourcei, m_source, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE );
}

/// @brief Sets the source gain (volume).
/// @param volume Linear gain (0.0 = silent, 1.0 = unity).
void Source::setVolume( float volume )
{
	validate();
	alCall( alSourcef, m_source, AL_GAIN, volume );
}

/// @brief Sets the 3D position of the source.
/// @param x X coordinate.
/// @param y Y coordinate.
/// @param z Z coordinate.
void Source::setPosition( float x, float y, float z )
{
	validate();
	alCall( alSource3f, m_source, AL_POSITION, x, y, z );
}

/// @brief Sets the source pitch (playback rate multiplier).
/// @param pitch 1.0 = original pitch; values above/below shift the pitch.
void Source::setPitch( float pitch )
{
	validate();
	alCall( alSourcef, m_source, AL_PITCH, pitch );
}

/// @brief Rewinds the source to the start of its buffer.
void AL::Source::rewind()
{
	validate();
	alCall( alSourceRewind, m_source );
}

/// @brief Starts playing the source from its current position.
void Source::play()
{
	validate();
	alCall( alSourcePlay, m_source );
}

/// @brief Stops playback immediately.
void Source::stop()
{
	validate();
	alCall( alSourceStop, m_source );
}

/// @brief Attaches (or clears, if null) a Filter on the source's dry signal path.
/// @param filter Filter to apply, or nullptr to remove.
void Source::setDryPathFilter( const std::shared_ptr<Filter>& filter )
{
	validate();
	alCall( alSourcei, m_source, AL_DIRECT_FILTER, filter ? filter->filter() : AL_FILTER_NULL );
	m_dryPath = filter;
}

/// @brief Routes the source's wet signal on auxiliary send index @p slot through
///        @p auxSlot and an optional @p filter. Passing null clears the send.
/// @param slot    Auxiliary send index.
/// @param auxSlot Target EffectSlot (or nullptr to disconnect).
/// @param filter  Optional filter on the send.
void Source::setWetPathEffect( ALuint slot, const std::shared_ptr<EffectSlot>& auxSlot, const std::shared_ptr<Filter>& filter )
{
	validate();
	alCall( alSource3i, m_source, AL_AUXILIARY_SEND_FILTER, auxSlot ? auxSlot->slot() : AL_EFFECTSLOT_NULL, slot, filter ? filter->filter() : AL_FILTER_NULL );
	if ( auxSlot || filter )
	{
		m_wetPath[slot] = std::make_pair( auxSlot, filter );
	}
	else
	{
		m_wetPath.erase( slot );
	}
}

/// @brief Queries the current playback state of the source.
/// @return PlayState enum (INITIAL/PLAYING/PAUSED/STOPPED).
Source::PlayState Source::getPlayState() const
{
	validate();
	ALint state = AL_PLAYING;
	alCall( alGetSourcei, m_source, AL_SOURCE_STATE, &state );
	switch ( state )
	{
		case AL_INITIAL:
			return INITIAL;
		case AL_PLAYING:
			return PLAYING;
		case AL_PAUSED:
			return PAUSED;
		case AL_STOPPED:
			return STOPPED;
		default:
			throw std::logic_error( "Unknown playstate " + std::to_string( state ) );
	}
}

/// @brief Allocates an AL effect object. Throws std::invalid_argument if the context lacks
///        EFX support.
/// @param context Owning Context.
Effect::Effect( const std::shared_ptr<Context>& context ) :
	Object( context )
{
	validate();
	if ( !Object::context()->hasEFX() )
	{
		throw std::invalid_argument( "No EFX support" );
	}
	alCall( fn()->alGenEffects, 1, &m_effect );
}

/// @brief Releases the AL effect handle.
Effect::~Effect()
{
	validate();
	alCall( fn()->alDeleteEffects, 1, &m_effect );
}

/// @brief Allocates an AL filter object, initialised as AL_FILTER_NULL (no-op).
/// @param context Owning Context.
Filter::Filter( const std::shared_ptr<Context>& context ) :
	Object( context )
{
	validate();
	alCall( fn()->alGenFilters, 1, &m_filter );
	alCall( fn()->alFilteri, m_filter, AL_FILTER_TYPE, AL_FILTER_NULL );
}

/// @brief Releases the AL filter handle.
Filter::~Filter()
{
	validate();
	alCall( fn()->alDeleteFilters, 1, &m_filter );
}

/// @brief Configures the filter as the cheapest AL filter type that captures the requested
///        gain curve: null (all 1.0), band-pass (three distinct gains), low-pass (mid=hf),
///        or high-pass (hf=mid).
/// @param lfGain  Low-frequency gain.
/// @param midGain Mid-frequency gain.
/// @param hfGain  High-frequency gain.
void Filter::setGain( float lfGain, float midGain, float hfGain )
{
	validate();
	if ( lfGain == 1.0f && midGain == 1.0f && hfGain == 1.0f )
	{
		fn()->alFilteri( m_filter, AL_FILTER_TYPE, AL_FILTER_NULL );
	}
	else if ( lfGain != midGain && midGain != hfGain )
	{
		alCall( fn()->alFilteri, filter(), AL_FILTER_TYPE, AL_FILTER_BANDPASS );
		alCall( fn()->alFilterf, filter(), AL_BANDPASS_GAINLF, lfGain );
		alCall( fn()->alFilterf, filter(), AL_BANDPASS_GAIN, midGain );
		alCall( fn()->alFilterf, filter(), AL_BANDPASS_GAINHF, hfGain );
	}
	else if ( midGain != hfGain )
	{
		alCall( fn()->alFilteri, filter(), AL_FILTER_TYPE, AL_FILTER_LOWPASS );
		alCall( fn()->alFilterf, filter(), AL_LOWPASS_GAIN, midGain );
		alCall( fn()->alFilterf, filter(), AL_LOWPASS_GAINHF, hfGain );
	}
	else
	{
		alCall( fn()->alFilteri, filter(), AL_FILTER_TYPE, AL_FILTER_HIGHPASS );
		alCall( fn()->alFilterf, filter(), AL_HIGHPASS_GAINLF, lfGain );
		alCall( fn()->alFilterf, filter(), AL_HIGHPASS_GAIN, midGain );
	}
}

/// @brief Allocates an auxiliary effect slot and wires the given Effect into it.
/// @param context Owning Context.
/// @param effect  Effect to host (kept alive via shared_ptr).
EffectSlot::EffectSlot( const std::shared_ptr<Context>& context, const std::shared_ptr<Effect>& effect ) :
	Object( context ),
	m_effect( effect )
{
	validate();
	alCall( fn()->alGenAuxiliaryEffectSlots, 1, &m_slot );
	alCall( fn()->alAuxiliaryEffectSloti, m_slot, AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, AL_TRUE );
	alCall( fn()->alAuxiliaryEffectSloti, m_slot, AL_EFFECTSLOT_EFFECT, effect->effect() );
}

/// @brief Releases the auxiliary effect slot handle.
EffectSlot::~EffectSlot()
{
	validate();
	alCall( fn()->alDeleteAuxiliaryEffectSlots, 1, &m_slot );
}

/// @brief Constructs a reverb effect. Prefers AL_EFFECT_EAXREVERB if available, otherwise
///        falls back to AL_EFFECT_REVERB.
/// @param context Owning Context.
ReverbEffect::ReverbEffect( const std::shared_ptr<Context>& context ) :
	Effect( context ),
	m_hasEAX( alCall( alGetEnumValue, "AL_EFFECT_EAXREVERB" ) != 0 )
{
	validate();
	if ( m_hasEAX )
	{
		alCall( fn()->alEffecti, effect(), AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB );
	}
	else
	{
		alCall( fn()->alEffecti, effect(), AL_EFFECT_TYPE, AL_EFFECT_REVERB );
	}
}

ReverbEffect::~ReverbEffect() = default;

/// @brief Applies an EFX reverb properties preset to this effect. Uses the full EAX
///        parameter set if AL_EFFECT_EAXREVERB is available, otherwise falls back to the
///        standard AL_EFFECT_REVERB subset.
/// @param preset EFXEAXREVERBPROPERTIES structure (e.g. EFX_REVERB_PRESET_HALLWAY).
void ReverbEffect::setPreset( const EFXEAXREVERBPROPERTIES& preset )
{
	validate();
	if ( m_hasEAX )
	{
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_DENSITY, preset.flDensity );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_DIFFUSION, preset.flDiffusion );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_GAIN, preset.flGain );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_GAINHF, preset.flGainHF );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_GAINLF, preset.flGainLF );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_DECAY_TIME, preset.flDecayTime );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_DECAY_HFRATIO, preset.flDecayHFRatio );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_DECAY_LFRATIO, preset.flDecayLFRatio );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_REFLECTIONS_GAIN, preset.flReflectionsGain );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_REFLECTIONS_DELAY, preset.flReflectionsDelay );
		alCall( fn()->alEffectfv, effect(), AL_EAXREVERB_REFLECTIONS_PAN, preset.flReflectionsPan );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_LATE_REVERB_GAIN, preset.flLateReverbGain );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_LATE_REVERB_DELAY, preset.flLateReverbDelay );
		alCall( fn()->alEffectfv, effect(), AL_EAXREVERB_LATE_REVERB_PAN, preset.flLateReverbPan );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_ECHO_TIME, preset.flEchoTime );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_ECHO_DEPTH, preset.flEchoDepth );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_MODULATION_TIME, preset.flModulationTime );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_MODULATION_DEPTH, preset.flModulationDepth );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_AIR_ABSORPTION_GAINHF, preset.flAirAbsorptionGainHF );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_HFREFERENCE, preset.flHFReference );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_LFREFERENCE, preset.flLFReference );
		alCall( fn()->alEffectf, effect(), AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, preset.flRoomRolloffFactor );
		alCall( fn()->alEffecti, effect(), AL_EAXREVERB_DECAY_HFLIMIT, preset.iDecayHFLimit );
	}
	else
	{
		alCall( fn()->alEffectf, effect(), AL_REVERB_DENSITY, preset.flDensity );
		alCall( fn()->alEffectf, effect(), AL_REVERB_DIFFUSION, preset.flDiffusion );
		alCall( fn()->alEffectf, effect(), AL_REVERB_GAIN, preset.flGain );
		alCall( fn()->alEffectf, effect(), AL_REVERB_GAINHF, preset.flGainHF );
		alCall( fn()->alEffectf, effect(), AL_REVERB_DECAY_TIME, preset.flDecayTime );
		alCall( fn()->alEffectf, effect(), AL_REVERB_DECAY_HFRATIO, preset.flDecayHFRatio );
		alCall( fn()->alEffectf, effect(), AL_REVERB_REFLECTIONS_GAIN, preset.flReflectionsGain );
		alCall( fn()->alEffectf, effect(), AL_REVERB_REFLECTIONS_DELAY, preset.flReflectionsDelay );
		alCall( fn()->alEffectf, effect(), AL_REVERB_LATE_REVERB_GAIN, preset.flLateReverbGain );
		alCall( fn()->alEffectf, effect(), AL_REVERB_LATE_REVERB_DELAY, preset.flLateReverbDelay );
		alCall( fn()->alEffectf, effect(), AL_REVERB_AIR_ABSORPTION_GAINHF, preset.flAirAbsorptionGainHF );
		alCall( fn()->alEffectf, effect(), AL_REVERB_ROOM_ROLLOFF_FACTOR, preset.flRoomRolloffFactor );
		alCall( fn()->alEffecti, effect(), AL_REVERB_DECAY_HFLIMIT, preset.iDecayHFLimit );
	}
}

/// @brief Constructs an Object with an associated Context. Throws std::invalid_argument
///        if the context pointer is null.
/// @param context Owning Context.
Object::Object( const std::shared_ptr<Context>& context ) :
	m_context( context )
{
	if ( !m_context )
		throw std::invalid_argument( "Missing OpenAL context" );
}

Object::~Object() = default;

/// @brief Debug-only sanity check that the owning Context is the current one on the calling
///        thread. Throws std::logic_error if not, to catch accidental cross-context use.
void Object::checkContextActive() const
{
	auto activeContext = m_context->hasThreadLocal() ? fn()->alcGetThreadContext() : alcGetCurrentContext();
	if ( activeContext != m_context->context() )
	{
		throw std::logic_error( "Another OpenAL context is active" );
	}
}

/// @brief Constructs a Listener wrapper. OpenAL has only one listener per context, so this
///        just stores the context reference.
/// @param context Owning Context.
Listener::Listener( const std::shared_ptr<Context>& context ) :
	Object( context )
{
}

Listener::~Listener() = default;

/// @brief Sets the listener's orientation (forward and up vectors combined into the
///        AL_ORIENTATION 6-float tuple).
/// @param forward Forward-facing direction.
/// @param up      Up direction.
void Listener::setOrientation( const float forward[3], const float up[3] )
{
	ALfloat orientation[6] = {
		forward[0],
		forward[1],
		forward[2],
		up[0],
		up[1],
		up[2],
	};
	alCall( alListenerfv, AL_ORIENTATION, orientation );
}

/// @brief Sets the listener's 3D position.
/// @param position Three-float XYZ array.
void Listener::setPosition( const float position[3] )
{
	alCall( alListenerfv, AL_POSITION, position );
}

/// @brief Sets the listener's master gain (volume).
/// @param volume Linear gain (0.0 = silent, 1.0 = unity).
void AL::Listener::setVolume( float volume )
{
	alCall( alListenerf, AL_GAIN, volume );
}
