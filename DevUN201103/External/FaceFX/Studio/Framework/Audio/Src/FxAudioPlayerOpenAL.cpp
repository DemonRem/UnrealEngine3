//------------------------------------------------------------------------------
// An OpenAL based audio player.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include "FxAudioPlayerOpenAL.h"
#include "FxConsole.h"

#if !defined(__UNREAL__)

#pragma comment(lib, "OpenAL32.lib")
#pragma message("Linking OpenAL...")

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxAudioPlayerOpenALVersion 0

FX_IMPLEMENT_CLASS(FxAudioPlayerOpenAL,kCurrentFxAudioPlayerOpenALVersion,FxAudioPlayer);

FxAudioPlayerOpenAL::FxAudioPlayerOpenAL()
	: _pDevice(NULL)
	, _pContext(NULL)
	, _alBuffer(0)
	, _alBufferIsValid(FxFalse)
	, _alSource(0)
	, _alSourceIsValid(FxFalse)
	, _bufferStart(0)
	, _bufferEnd(0)
	, _isOK(FxFalse)
{
	// Initialize the OpenAL function pointers to NULL.
	Fx_alcMakeContextCurrent = NULL;
	Fx_alDeleteSources       = NULL;
	Fx_alcDestroyContext     = NULL;
	Fx_alcCloseDevice        = NULL;
	Fx_alcOpenDevice         = NULL;
	Fx_alcCreateContext      = NULL;
	Fx_alGetError            = NULL;
	Fx_alcGetError           = NULL;
	Fx_alcGetIntegerv        = NULL;
	Fx_alListenerfv          = NULL;
	Fx_alGenSources          = NULL;
	Fx_alSourcef             = NULL;
	Fx_alBufferData          = NULL;
	Fx_alSourcei             = NULL;
	Fx_alSourcePlay          = NULL;
	Fx_alSourcePause         = NULL;
	Fx_alSourceStop          = NULL;
	Fx_alGetSourcei          = NULL;
	Fx_alGetSourcef          = NULL;
	Fx_alDeleteBuffers       = NULL;
	Fx_alcGetString          = NULL;
	Fx_alGenBuffers          = NULL;

	// Load the OpenAL function pointers.
	Fx_alcMakeContextCurrent = &alcMakeContextCurrent;
	Fx_alDeleteSources       = &alDeleteSources;
	Fx_alcDestroyContext     = &alcDestroyContext;
	Fx_alcCloseDevice        = &alcCloseDevice;
	Fx_alcOpenDevice         = &alcOpenDevice;
	Fx_alcCreateContext      = &alcCreateContext;
	Fx_alGetError            = &alGetError;
	Fx_alcGetError           = &alcGetError;
	Fx_alcGetIntegerv        = &alcGetIntegerv;
	Fx_alListenerfv          = &alListenerfv;
	Fx_alGenSources          = &alGenSources;
	Fx_alSourcef             = &alSourcef;
	Fx_alBufferData          = &alBufferData;
	Fx_alSourcei             = &alSourcei;
	Fx_alSourcePlay          = &alSourcePlay;
	Fx_alSourcePause         = &alSourcePause;
	Fx_alSourceStop          = &alSourceStop;
	Fx_alGetSourcei          = &alGetSourcei;
	Fx_alGetSourcef          = &alGetSourcef;
	Fx_alDeleteBuffers       = &alDeleteBuffers;
	Fx_alcGetString          = &alcGetString;
	Fx_alGenBuffers          = &alGenBuffers;
	
	if( Fx_alcMakeContextCurrent && 
		Fx_alDeleteSources       && 
		Fx_alcDestroyContext     && 
		Fx_alcCloseDevice        && 
		Fx_alcOpenDevice         && 
		Fx_alcCreateContext      && 
		Fx_alGetError            && 
		Fx_alcGetError           && 
		Fx_alcGetIntegerv        && 
		Fx_alListenerfv          && 
		Fx_alGenSources          && 
		Fx_alSourcef             && 
		Fx_alBufferData          && 
		Fx_alSourcei             && 
		Fx_alSourcePlay          && 
		Fx_alSourcePause         && 
		Fx_alSourceStop          && 
		Fx_alGetSourcei          && 
		Fx_alGetSourcef          && 
		Fx_alDeleteBuffers       && 
		Fx_alcGetString          && 
		Fx_alGenBuffers )
	{
		_isOK = FxTrue;
	}

	if( !_isOK )
	{
		FxWarn("WARNING: The OpenAL state is invalid.");
	}
}

FxAudioPlayerOpenAL::~FxAudioPlayerOpenAL()
{
	if( _isOK )
	{
		Fx_alcMakeContextCurrent(_pContext);
		// Delete the source.
		if( _alSourceIsValid )
		{
			Fx_alDeleteSources(1, &_alSource);
		}

		// Delete the buffer.
		_clearOpenALBuffer();

		if( _pDevice && _pContext )
		{
			// Release the context.
			Fx_alcDestroyContext(_pContext);
			// Close the device.
			Fx_alcCloseDevice(_pDevice);
		}
	}
}

void FxAudioPlayerOpenAL::Initialize( wxWindow* window )
{
	if( !_isOK )
	{
		FxWarn("WARNING: The OpenAL state is invalid.");
		return;
	}

	_window = window;

	// Open the default device.
	_pDevice = Fx_alcOpenDevice(NULL);
	if( _pDevice )
	{
		// Create the context.
		_pContext = Fx_alcCreateContext(_pDevice, NULL);
		if( _pContext )
		{
			// Set the active context.
			Fx_alcGetError(_pDevice);
			Fx_alcMakeContextCurrent(_pContext);
			if( ALC_NO_ERROR == Fx_alcGetError(_pDevice) )
			{
				// Clear the error code.
				Fx_alGetError();
				Fx_alcGetError(_pDevice);

				// Check which device and version we are using.
				const FxChar* deviceName = reinterpret_cast<const FxChar*>(Fx_alcGetString(_pDevice, ALC_DEVICE_SPECIFIER));
				ALint majorVersion, minorVersion;
				Fx_alcGetIntegerv(_pDevice, ALC_MAJOR_VERSION, 1, &majorVersion);
				Fx_alcGetIntegerv(_pDevice, ALC_MINOR_VERSION, 1, &minorVersion);
				FxMsg("OpenAL Device: %s - Spec. Version %d.%d", deviceName, majorVersion, minorVersion);

				if( 1 != majorVersion || 1 != minorVersion )
				{
					FxError("OpenAL returned an invalid version number.  Please update your sound drivers and OpenAL installation or you may experience audio related crashes.");
#ifdef IS_FACEFX_STUDIO
					FxMessageBox(_("OpenAL returned an invalid version number.  Please update your sound drivers and OpenAL installation or you may experience audio related crashes."), _("FaceFX OpenAL Error"));
#endif
				}

				// Set up the listener at the origin and facing into the screen.
				ALfloat listenerPosition[]    = {0.0,0.0,0.0};
				ALfloat listenerVelocity[]    = {0.0,0.0,0.0};
				ALfloat	listenerOrientation[] = {0.0,0.0,-1.0, 0.0,1.0,0.0};

				Fx_alListenerfv(AL_POSITION, listenerPosition);
				if( AL_NO_ERROR != Fx_alGetError() )
				{
					FxWarn("FxAudioPlayerOpenAL::Initialize -> alListenerfv AL_POSITION failed!");
				}

				Fx_alListenerfv(AL_VELOCITY, listenerVelocity);
				if( AL_NO_ERROR != Fx_alGetError() )
				{
					FxWarn("FxAudioPlayerOpenAL::Initialize -> alListenerfv AL_VELOCITY failed!");
				}

				Fx_alListenerfv(AL_ORIENTATION, listenerOrientation);
				if( AL_NO_ERROR != Fx_alGetError() )
				{
					FxWarn("FxAudioPlayerOpenAL::Initialize -> alListenerfv AL_ORIENTATION failed!");
				}

				// Create the source.
				Fx_alGenSources(1, &_alSource);
				if( AL_NO_ERROR != Fx_alGetError() )
				{
					_alSourceIsValid = FxFalse;
					FxWarn("FxAudioPlayerOpenAL::Initialize -> alGenSources failed!");
				}
				else
				{
					_alSourceIsValid = FxTrue;
					// The default OpenAL volume is a little loud, so scale it
					// back a bit.
					Fx_alSourcef(_alSource, AL_GAIN, 0.75f);
				}
			}
			else
			{
				FxWarn("FxAudioPlayerOpenAL::Initialize -> alcMakeContextCurrent failed!");
			}
		}
		else
		{
			FxWarn("FxAudioPlayerOpenAL::Initialize -> alcCreateContext failed!");
		}
	}
	else
	{
		FxWarn("FxAudioPlayerOpenAL::Initialize -> alcOpenDevice failed!");
		FxWarn("Make sure your sound card and drivers are installed properly and that no other application has the audio focus.");
	}
}

const FxAudio* FxAudioPlayerOpenAL::GetSource( void ) const
{
	return _source;
}

void FxAudioPlayerOpenAL::SetSource( FxAudio* source )
{
	if( !_isOK )
	{
		FxWarn("WARNING: The OpenAL state is invalid.");
		return;
	}

	_source = source;

	Fx_alcMakeContextCurrent(_pContext);

	// Clear the buffer.
	_clearOpenALBuffer();
	// Create the buffer.
	_createOpenALBuffer();

	if( _alSourceIsValid && _alBufferIsValid )
	{
		if( _source )
		{
			// Get the raw sample buffer from the audio source.  Note that FxAudio
			// always uses mono, so _source->GetNumChannels() should always be 1.  This
			// code should handle stereo though just in case that changes in the future.
			FxRawSampleBuffer rawSampleBuffer = _source->ExportRawSampleBuffer();

			if( rawSampleBuffer.GetSamples() )
			{
				// Copy raw sample buffer into the OpenAL buffer.
				ALsizei size = ((_source->GetNumSamples() * _source->GetBitsPerSample()) / FX_CHAR_BIT);
				ALenum format = AL_FORMAT_MONO16;
				if( 1 == _source->GetNumChannels() )
				{
					format = (8 == _source->GetBitsPerSample()) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
				}
				else if( 2 == _source->GetNumChannels() )
				{
					format = (8 == _source->GetBitsPerSample()) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
				}
				else
				{
					FxWarn("Unsupported channel count in FxAudioPlayerOpenAL::SetSource()!");
					FxAssert(!"Unsupported channel count in FxAudioPlayerOpenAL::SetSource()!");
				}
				Fx_alBufferData(_alBuffer, format, rawSampleBuffer.GetSamples(), size, _source->GetSampleRate());

				// Set the buffer in the source.
				Fx_alSourcei(_alSource, AL_BUFFER, _alBuffer);
			}
		}
	}
}

void FxAudioPlayerOpenAL::
GetPlayRange( FxReal& rangeStart, FxReal& rangeEnd ) const
{
	rangeStart = _rangeStart;
	rangeEnd   = _rangeEnd;
}

void FxAudioPlayerOpenAL::
SetPlayRange( FxReal rangeStart, FxReal rangeEnd )
{
	if( !_isOK )
	{
		return;
	}

	_rangeStart = rangeStart;
	_rangeEnd   = rangeEnd;

	// Swap the range end and range start if they don't make sense.
	if( _rangeEnd < _rangeStart )
	{
		FxReal temp = _rangeStart;
		_rangeStart = _rangeEnd;
		_rangeEnd   = temp;
	}

	if( !_paused )
	{
		Fx_alcMakeContextCurrent(_pContext);

		// Clear the buffer.
		_clearOpenALBuffer();
		// Create the buffer.
		_createOpenALBuffer();

		if( _alSourceIsValid && _alBufferIsValid )
		{
			if( _source )
			{
				//@todo At some point we'll want to do smarter range checking here.
				if( _rangeStart < 0.0f )
				{
					_rangeStart = 0.0f;
				}
				if( _rangeEnd < 0.0f )
				{
					_rangeEnd = 0.0f;
				}
				if( _rangeStart > _source->GetDuration() )
				{
					_rangeStart = _source->GetDuration();
				}
				if( _rangeEnd > _source->GetDuration() )
				{
					_rangeEnd = _source->GetDuration();
				}

				_bufferStart = ((_rangeStart * _source->GetSampleRate() * _source->GetBitsPerSample()) / FX_CHAR_BIT);
				
				// If there is no selection, just copy the entire rest of the buffer.
				if( _rangeStart == _rangeEnd )
				{
					_bufferEnd = ((_source->GetNumSamples() * _source->GetBitsPerSample()) / FX_CHAR_BIT);
				}
				else
				{
					_bufferEnd = ((_rangeEnd * _source->GetSampleRate() * _source->GetBitsPerSample()) / FX_CHAR_BIT);
				}

				// Force the correct "block alignment."
				ALsizei blockAlign = ((_source->GetBitsPerSample() * _source->GetNumChannels()) / FX_CHAR_BIT);
				if( _bufferStart % blockAlign )
				{
					--_bufferStart;
				}
				if( _bufferEnd % blockAlign )
				{
					++_bufferEnd;
				}
				ALsizei size = _bufferEnd - _bufferStart;

				// Get the raw sample buffer from the audio source.  Note that FxAudio
				// always uses mono, so _source->GetNumChannels() should always be 1.  This
				// code should handle stereo though just in case that changes in the future.
				FxRawSampleBuffer rawSampleBuffer = _source->ExportRawSampleBuffer();

				if( rawSampleBuffer.GetSamples() )
				{
					// Copy raw sample buffer into the OpenAL buffer.
					ALenum format = AL_FORMAT_MONO16;
					if( 1 == _source->GetNumChannels() )
					{
						format = (8 == _source->GetBitsPerSample()) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
					}
					else if( 2 == _source->GetNumChannels() )
					{
						format = (8 == _source->GetBitsPerSample()) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
					}
					else
					{
						FxWarn("Unsupported channel count in FxAudioPlayerOpenAL::SetPlayRange()!");
						FxAssert(!"Unsupported channel count in FxAudioPlayerOpenAL::SetPlayRange()!");
					}
					Fx_alBufferData(_alBuffer, format, rawSampleBuffer.GetSamples() + _bufferStart, size, _source->GetSampleRate());

					// Set the buffer in the source.
					Fx_alSourcei(_alSource, AL_BUFFER, _alBuffer);
				}
			}
		}
	}
}

FxReal FxAudioPlayerOpenAL::GetVolume( void ) const
{
	return _volume;
}

void FxAudioPlayerOpenAL::SetVolume( FxReal volume )
{
	if( _isOK )
	{
		_volume = volume;
		if( _alSourceIsValid )
		{
			Fx_alcMakeContextCurrent(_pContext);
			Fx_alSourcef(_alSource, AL_GAIN, _volume);
		}
	}
}

FxReal FxAudioPlayerOpenAL::GetPitch( void ) const
{
	return _pitch;
}

void FxAudioPlayerOpenAL::SetPitch( FxReal pitch )
{
	if( _isOK )
	{
		_pitch = pitch;
		if( _alSourceIsValid )
		{
			Fx_alcMakeContextCurrent(_pContext);
			Fx_alSourcef(_alSource, AL_PITCH, _pitch);
		}
	}
}

void FxAudioPlayerOpenAL::Play( FxBool loop )
{
	if( _isOK )
	{
		if( _alSourceIsValid )
		{
			Fx_alcMakeContextCurrent(_pContext);
			if( _paused )
			{
				_paused = FxFalse;
			}
			_loop = loop;
			if( _loop )
			{
				Fx_alSourcei(_alSource, AL_LOOPING, AL_TRUE);
			}
			else
			{
				Fx_alSourcei(_alSource, AL_LOOPING, AL_FALSE);
			}
			Fx_alSourcePlay(_alSource);
		}
	}
}

void FxAudioPlayerOpenAL::Pause( void )
{
	if( _isOK )
	{
		if( _alSourceIsValid )
		{
			Fx_alcMakeContextCurrent(_pContext);
			_paused = !_paused;
			if( _paused )
			{
				Fx_alSourcePause(_alSource);
			}
			else
			{
				Fx_alSourcePlay(_alSource);
			}
		}
	}
}

void FxAudioPlayerOpenAL::Stop( void )
{
	if( _isOK )
	{
		if( _alSourceIsValid )
		{
			Fx_alcMakeContextCurrent(_pContext);
			Fx_alSourceStop(_alSource);
			_paused = FxFalse;
			_loop   = FxFalse;
		}
	}
}

FxBool FxAudioPlayerOpenAL::IsPlaying( void ) const
{
	if( _isOK )
	{
		if( _alSourceIsValid )
		{
			Fx_alcMakeContextCurrent(_pContext);
			ALint state;
			Fx_alGetSourcei(_alSource, AL_SOURCE_STATE, &state);
			if( AL_PLAYING == state )
			{
				return FxTrue;
			}
		}
	}
	return FxFalse;
}

FxBool FxAudioPlayerOpenAL::IsLooping( void ) const
{
	return _loop;
}

FxReal FxAudioPlayerOpenAL::GetPlayCursorPosition( void ) const
{
	// Note: this is relative to the entire audio clip, not just the current 
	// range.
	ALfloat offset = 0.0f;
	if( _isOK )
	{
		if( _alSourceIsValid )
		{
			Fx_alcMakeContextCurrent(_pContext);
			Fx_alGetSourcef(_alSource, AL_SEC_OFFSET, &offset);
			offset += _rangeStart;
		}
	}
	return offset;
}

FxBool FxAudioPlayerOpenAL::SetPlayCursorPosition( FxReal position )
{
	// Note: this is relative to the entire audio clip, not just the current 
	// range.
	if( _isOK )
	{
		if( _alSourceIsValid )
		{
			Fx_alcMakeContextCurrent(_pContext);
			Fx_alSourcef(_alSource, AL_SEC_OFFSET, position - _rangeStart);
			return FxTrue;
		}
	}
	return FxFalse;
}

void FxAudioPlayerOpenAL::_clearOpenALBuffer( void )
{
	if( _isOK )
	{
		if( IsPlaying() )
		{
			Stop();
		}
		// Remove the buffer from the source.
		if( _alSourceIsValid )
		{
			Fx_alSourcei(_alSource, AL_BUFFER, 0);
		}
		// Delete the buffer.
		if( _alBufferIsValid )
		{
			Fx_alDeleteBuffers(1, &_alBuffer);
			_alBuffer = 0;
			_alBufferIsValid = FxFalse;
		}
	}
}

void FxAudioPlayerOpenAL::_createOpenALBuffer( void )
{
	if( _isOK )
	{
		// Create the buffer.
		Fx_alGenBuffers(1, &_alBuffer);
		if( AL_NO_ERROR != Fx_alGetError() )
		{
			_alBufferIsValid = FxFalse;
			FxWarn("FxAudioPlayerOpenAL::_createOpenALBuffer -> alGenBuffers failed!");
		}
		else
		{
			_alBufferIsValid = FxTrue;
		}
	}
}

} // namespace Face

} // namespace OC3Ent

#endif // !defined(__UNREAL__)
