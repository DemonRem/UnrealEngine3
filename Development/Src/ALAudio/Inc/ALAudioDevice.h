/*=============================================================================
	ALAudioDevice.h: Unreal OpenAL audio interface object.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_ALAUDIODEVICE
#define _INC_ALAUDIODEVICE

/*------------------------------------------------------------------------------------
	Dependencies, helpers & forward declarations.
------------------------------------------------------------------------------------*/

class UALAudioDevice;

#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,8)
#endif
#define AL_NO_PROTOTYPES 1
#define ALC_NO_PROTOTYPES 1
#include "al.h"
#include "alc.h"
#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

#define MAX_EFFECT_SLOTS	8

#if __WIN32__
#define AL_DLL TEXT("OpenAL32.dll")
#else
#define AL_DLL TEXT("libopenal.so")
#endif

/**
 * OpenAL implementation of FSoundBuffer, containing the wave data and format information.
 */
class FALSoundBuffer
{
public:
	/** 
	 * Constructor
	 *
	 * @param AudioDevice	audio device this sound buffer is going to be attached to.
	 */
	FALSoundBuffer( UALAudioDevice* AudioDevice );
	
	/**
	 * Destructor 
	 * 
	 * Frees wave data and detaches itself from audio device.
 	 */
	~FALSoundBuffer( void );

	/**
	 * Static function used to create a buffer.
	 *
	 * @param	InWave		USoundNodeWave to use as template and wave source
	 * @param	AudioDevice	Audio device to attach created buffer to
	 * @return	FALSoundBuffer pointer if buffer creation succeeded, NULL otherwise
	 */
	static FALSoundBuffer* Init( USoundNodeWave* InWave, UALAudioDevice* AudioDevice );

	/**
	 * Locate and precache if necessary the ogg vorbis data. Decompress and validate the header.
	 *
	 * @param	AudioDevice	Owning audio device
	 * @param	InWave		USoundNodeWave to use as template and wave source
	 */
	void PrepareDecompression( UALAudioDevice* AudioDevice, USoundNodeWave* Wave );

	/**
	 * Decompress the next chunk of sound into CurrentBuffer
	 *
	 * @param	bLoop		Whether to loop the sound or pad with 0s
	 * @return	UBOOL		TRUE if the sound looped, FALSE otherwise
	 */
	UBOOL DecodeCompressed( UBOOL bLoop );

	/**
	 * Returns the size of this buffer in bytes.
	 *
	 * @return Size in bytes
	 */
	INT GetSize( void ) { return( BufferSize ); }

	/** 
	 * Returns the number of channels for this buffer
	 */
	INT GetNumChannels( void ) { return( NumChannels ); }
		
	/** Audio device this buffer is attached to */
	UALAudioDevice*			AudioDevice;
	/** Index for the current buffer id - used for double buffering packets. Set to zero for resident sounds. */
	INT						CurrentBuffer;
	/** Array of buffer ids used to reference the data stored in AL. */
	ALuint					BufferIds[2];
	/** Resource ID of associated USoundNodeWave */
	INT						ResourceID;
	/** Pointer to decompression state */
	class FCompressedAudioInfo*	DecompressionState;
	/** Human readable name of resource, most likely name of UObject associated during caching. */
	FString					ResourceName;
	/** Format of the data internal to OpenAL */
	ALuint					InternalFormat;

	/** Number of bytes stored in OpenAL, or the size of the ogg vorbis data */
	INT						BufferSize;
	/** The number of channels in this sound buffer - should be directly related to InternalFormat */
	INT						NumChannels;
	/** Sample rate of the ogg vorbis data - typically 44100 or 22050 */
	INT						SampleRate;

private:
	/**
	 * Static function used to create a double buffered buffer for dynamic decompression of comrpessed sound
	 *
	 * @param	InWave		USoundNodeWave to use as template and wave source
	 * @param	AudioDevice	Audio device to attach created buffer to
	 * @return	FALSoundBuffer pointer if buffer creation succeeded, NULL otherwise
	 */
	static FALSoundBuffer* CreateQueuedBuffer( USoundNodeWave* Wave, UALAudioDevice* AudioDevice );

	/**
	 * Static function used to create a buffer to a sound that is completely uploaded to OpenAL
	 *
	 * @param	InWave		USoundNodeWave to use as template and wave source
	 * @param	AudioDevice	Audio device to attach created buffer to
	 * @return	FALSoundBuffer pointer if buffer creation succeeded, NULL otherwise
	 */
	static FALSoundBuffer* CreateNativeBuffer( USoundNodeWave* Wave, UALAudioDevice* AudioDevice );

	/**
	 * Static function used to create a buffer that can be used for listening to raw PCM data
	 *
	 * @param	Buffer		Existing buffer that needs new sound data
	 * @param	InWave		USoundNodeWave to use as template and wave source
	 * @param	AudioDevice	Audio device to attach created buffer to
	 * @return	FALSoundBuffer pointer if buffer creation succeeded, NULL otherwise
	 */
	static FALSoundBuffer* CreatePreviewBuffer( FALSoundBuffer* Buffer, USoundNodeWave* Wave, UALAudioDevice* AudioDevice );
};

/**
 * OpenAL implementation of FSoundSource, the interface used to play, stop and update sources
 */
class FALSoundSource : public FSoundSource
{
public:
	/**
	 * Constructor
	 *
	 * @param	InAudioDevice	audio device this source is attached to
	 */
	FALSoundSource( UAudioDevice* InAudioDevice )
	:	FSoundSource( InAudioDevice ),
		BuffersToFlush( FALSE ),
		SourceId( 0 ),
		Buffer( NULL )
	{}

	/**
	 * Initializes a source with a given wave instance and prepares it for playback.
	 *
	 * @param	WaveInstance	wave instance being primed for playback
	 * @return	TRUE			if initialization was successful, FALSE otherwise
	 */
	virtual UBOOL Init( FWaveInstance* WaveInstance );

	/**
	 * Updates the source specific parameter like e.g. volume and pitch based on the associated
	 * wave instance.	
	 */
	virtual void Update( void );

	/**
	 * Plays the current wave instance.	
	 */
	virtual void Play( void );

	/**
	 * Stops the current wave instance and detaches it from the source.	
	 */
	virtual void Stop( void );

	/**
	 * Pauses playback of current wave instance.
	 */
	virtual void Pause( void );

	/**
	 * Returns whether the buffer associated with this source is using CPU decompression.
	 *
	 * @return TRUE if decompressed on the CPU, FALSE otherwise
	 */
	virtual UBOOL UsesCPUDecompression( void );

	/**
	 * Queries the status of the currently associated wave instance.
	 *
	 * @return	TRUE if the wave instance/ source has finished playback and FALSE if it is 
	 *			currently playing or paused.
	 */
	virtual UBOOL IsFinished( void );

	/**
	 * Handles feeding new data to a real time decompressed sound and waits for the sound to finish if necessary.
	 */
	void HandleRealTimeSource( void );

	/** 
	 * Access function for the source id
	 */
	ALuint GetSourceId( void ) const { return( SourceId ); }

protected:
	/** There are queued buffers that need to play before the sound is finished. */
	UBOOL				BuffersToFlush;
	/** OpenAL source voice associated with this source/ channel. */
	ALuint				SourceId;
	/** Cached sound buffer associated with currently bound wave instance. */
	FALSoundBuffer *	Buffer;

	friend class UALAudioDevice;
};

/** 
 * A class to handle sound being recorded from a microphone
 */
class FALCaptureDevice
{
public:
	FALCaptureDevice( void );
	~FALCaptureDevice( void );

	/** 
	 * Attempts to start recording input from the microphone
	 *
	 * @return	TRUE if recording started successfully
	 */
	virtual UBOOL CaptureStart( void );

	/** 
	 * Stops recording input from the microphone
	 *
	 * @return	TRUE if the device stopped correctly
	 */
	virtual UBOOL CaptureStop( void );

	/**
	 * Returns a pointer the start of the captured data
	 *
	 * @return SWORD* to captured samples
	 * @return INT number of captured samples
	 */
	virtual INT CaptureGetSamples( SWORD*& Samples );

	/** 
	 * Sets the input level of the microphone.
	 */
	virtual void SetMicInputLevel( FLOAT Level );

private:
	/** Find and initialise the control used to set the microphone volume */
	void InitMicControl( void );
	/** Sets the mic input control to a value */
	void SetMicInputControl( DWORD InputLevel );

	/** Device used to input data from the microphone. */
	ALCdevice *									Device;
	/** Sample frequency of captured mic data */
	INT											Frequency;
	/** Number of seconds of sound to capture */
	INT											BufferLength;
	/** Enum required to retrieve the number of samples */
	ALenum										SamplesEnum;
	/** Buffer to store the captured data */
	SWORD*										Buffer;
	/** Current mic input level */
	FLOAT										MicLevel;
	/** The control identifier used to set the mic level */
	DWORD										ControlID;
	DWORD										Minimum;
	DWORD										Maximum;
};

/** 
 * The high level code to manage any sound effect processing
 */
class FALAudioEffectsManager : public FAudioEffectsManager
{
public:
	FALAudioEffectsManager( UAudioDevice* InDevice );
	~FALAudioEffectsManager( void );

	/** 
	 * Calls the platform specific code to set the parameters that define reverb
	 */
	virtual void SetReverbEffectParameters( const FAudioReverbEffect& ReverbEffectParameters );

	/** 
	 * Calls the platform specific code to set the parameters that define reverb
	 */
	virtual void SetEQEffectParameters( const FAudioEQEffect& EQEffectParameters );

	/** 
	 * Sets the output of the source to send to the effect (reverb)
	 */
	virtual void* LinkEffect( FSoundSource* Source );

private:
	ALint				MaxSends;
	ALint				MaxEffectSlots;
	ALuint				EffectSlots[MAX_EFFECT_SLOTS];
	ALuint				ReverbEffect;
	ALuint				EQEffect;
	ALuint				LowPassFilter;
};

/**
 * OpenAL implementation of an Unreal audio device.
 */
class UALAudioDevice : public UAudioDevice
{
	DECLARE_CLASS(UALAudioDevice,UAudioDevice,CLASS_Config|CLASS_Intrinsic,ALAudio)

	/**
	 * Static constructor, used to associate .ini options with member variables.	
	 */
	void StaticConstructor( void );

	/**
	 * Initializes the audio device and creates sources.
	 *
	 * @return TRUE if initialization was successful, FALSE otherwise
	 */
	virtual UBOOL Init( void );

	/**
	 * Update the audio device and calculates the cached inverse transform later
	 * on used for spatialization.
	 *
	 * @param	Realtime	whether we are paused or not
	 */
	virtual void Update( UBOOL bGameTicking );

	/**
	 * Precaches the passed in sound node wave object.
	 *
	 * @param	SoundNodeWave	Resource to be precached.
	 */
	virtual void Precache( USoundNodeWave* SoundNodeWave );

	/** 
	 * Lists all the loaded sounds and their memory footprint
	 */
	virtual void ListSounds( FOutputDevice& Ar );

	/** 
	 * Attempts to start recording input from the microphone
	 *
	 * @return	TRUE if recording started successfully
	 */
	virtual UBOOL CaptureStart( void );

	/** 
	 * Stops recording input from the microphone
	 *
	 * @return	Number of samples recorded. 0 is returned if the the capture was not or failed to start.
	 */
	virtual UBOOL CaptureStop( void );

	/**
	 * Returns a pointer the start of the captured data
	 *
	 * @return SWORD* to captured samples
	 * @return INT number of captured samples
	 */
	virtual INT CaptureGetSamples( SWORD*& Samples );

	/** 
	 * Sets the input level of the microphone.
	 */
	virtual void SetMicInputLevel( FLOAT Level );

	/**
	 * Frees the bulk resource data associated with this SoundNodeWave.
	 *
	 * @param	SoundNodeWave	wave object to free associated bulk data
	 */
	virtual void FreeResource( USoundNodeWave* SoundNodeWave );

	// UObject interface.
	virtual void Serialize( FArchive& Ar );

	/**
	 * Shuts down audio device. This will never be called with the memory image codepath.
	 */
	virtual void FinishDestroy( void );
	
	/**
	 * Special variant of Destroy that gets called on fatal exit. 
	 */
	virtual void ShutdownAfterError( void );

	/**
	 * Exec handler used to parse console commands.
	 *
	 * @param	Cmd		Command to parse
	 * @param	Ar		Output device to use in case the handler prints anything
	 * @return	TRUE if command was handled, FALSE otherwise
	 */
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar );

	// Needed for OpenAL sound playback in FaceFX Studio.
	//@todo Is there a better way to do this?
	void* GetDLLHandle( void ) { return DLLHandle; }
	ALCdevice* GetDevice( void ) { return HardwareDevice; }
	ALCcontext* GetContext( void ) { return SoundContext; }

	void FindProcs( UBOOL AllowExt );

	// Error checking.
	UBOOL alError( TCHAR* Text, UBOOL Log = 1 );

protected:
	/** Returns the enum for the internal format for playing a sound with this number of channels. */
	ALuint GetInternalFormat( INT NumChannels );

	/** Test decompress a vorbis file */
	void TestDecompressOggVorbis( USoundNodeWave* Wave );
	/** Decompress a wav a number of times for profiling purposes */
	void TimeTest( FOutputDevice& Ar, TCHAR* WaveAssetName );

	// Dynamic binding.
	void FindProc( void*& ProcAddress, char* Name, char* SupportName, UBOOL& Supports, UBOOL AllowExt );
	UBOOL FindExt( const TCHAR* Name );

	/**
	 * Tears down audio device by stopping all sounds, removing all buffers,  destroying all sources, ... Called by both Destroy and ShutdownAfterError
	 * to perform the actual tear down.
	 */
	void Teardown( void );

	// Variables.

	/** The name of the OpenAL Device to open - defaults to "Generic Software" */
	FStringNoInit								DeviceName;
	/** All loaded resident buffers */
	TArray<FALSoundBuffer*>						Buffers;
	/** Map from resource ID to sound buffer */
	TDynamicMap<INT, FALSoundBuffer*>			WaveBufferMap;
	DOUBLE										LastHWUpdate;
	/** Next resource ID value used for registering USoundNodeWave objects */
	INT											NextResourceID;

	// AL specific

	/** Device used to play back sounds */
	ALCdevice *									HardwareDevice;
	ALCcontext *								SoundContext;
	void *										DLLHandle;
	/** Formats for multichannel sounds */
	ALenum										Surround40Format;
	ALenum										Surround51Format;
	ALenum										Surround61Format;
	ALenum										Surround71Format;

	// Capture device
	FALCaptureDevice*							CaptureDevice;

	// Configuration.

	/** Minimum number of milliseconds between alProcessContext calls (sound hardware updates) */
	FLOAT										TimeBetweenHWUpdates;
	/** Sound duration in seconds below which sounds are entirely expanded to PCM at load time */
	FLOAT										MinOggVorbisDuration;

	friend class FALSoundBuffer;
};

#endif

