/*=============================================================================
	ALAudioDevice.cpp: Unreal OpenAL Audio interface object.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.

	Unreal is RHS with Y and Z swapped (or technically LHS with flipped axis)

=============================================================================*/

/*------------------------------------------------------------------------------------
	Audio includes.
------------------------------------------------------------------------------------*/

#include "ALAudioPrivate.h"
#include "UnNet.h"

//     2 UU == 1"
// <=> 1 UU == 0.0127 m
#define AUDIO_DISTANCE_FACTOR ( 0.0127f )

// OpenAL function pointers
#define AL_EXT( name, strname ) UBOOL SUPPORTS##name;
#define AL_PROC( name, strname, ret, func, parms ) ret ( CDECL * func ) parms;
#include "ALFuncs.h"
#undef AL_EXT
#undef AL_PROC

#if SUPPORTS_PRAGMA_PACK
#pragma pack( push, 8 )
#endif

#include <vorbis/vorbisfile.h>
#include "ALAudioDecompress.h"
#include <efx.h>

#if SUPPORTS_PRAGMA_PACK
#pragma pack( pop )
#endif

/*------------------------------------------------------------------------------------
	UALAudioDevice constructor and UObject interface.
------------------------------------------------------------------------------------*/

IMPLEMENT_CLASS( UALAudioDevice );

/**
 * Static constructor, used to associate .ini options with member variables.	
 */
void UALAudioDevice::StaticConstructor( void )
{
	new( GetClass(), TEXT( "DeviceName" ), RF_Public ) UStrProperty( CPP_PROPERTY( DeviceName ), TEXT( "ALAudio" ), CPF_Config );
	new( GetClass(), TEXT( "TimeBetweenHWUpdates" ), RF_Public ) UFloatProperty( CPP_PROPERTY( TimeBetweenHWUpdates ), TEXT( "ALAudio" ), CPF_Config );
	new( GetClass(), TEXT( "MinOggVorbisDuration" ), RF_Public ) UFloatProperty( CPP_PROPERTY( MinOggVorbisDuration ), TEXT( "ALAudio" ), CPF_Config );
}

/**
 * Tears down audio device by stopping all sounds, removing all buffers, 
 * destroying all sources, ... Called by both Destroy and ShutdownAfterError
 * to perform the actual tear down.
 */
void UALAudioDevice::Teardown( void )
{
	// Flush stops all sources and deletes all buffers so sources can be safely deleted below.
	Flush();
	
	if( alcProcessContext )
	{	
		alcProcessContext( SoundContext );
	}

	if( Effects )
	{
		delete Effects;
		Effects = NULL;
	}

	if( alcMakeContextCurrent )
	{	
		alcMakeContextCurrent( NULL );
	}

	if( alcDestroyContext )
	{	
		alcDestroyContext( SoundContext );
		SoundContext = NULL;
	}

	if( CaptureDevice )
	{
		delete CaptureDevice;
		CaptureDevice = NULL;
	}

	if( alcCloseDevice )
	{	
		alcCloseDevice( HardwareDevice );
		HardwareDevice = NULL;
	}

	if( DLLHandle )
	{
		appFreeDllHandle( DLLHandle );
		DLLHandle = NULL;
	}
}

//
//	UALAudioDevice::Serialize
//
void UALAudioDevice::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	if( Ar.IsCountingMemory() )
	{
		Ar.CountBytes( Buffers.Num() * sizeof( FALSoundBuffer ), Buffers.Num() * sizeof( FALSoundBuffer ) );
		Buffers.CountBytes( Ar );
		WaveBufferMap.CountBytes( Ar );
	}
}

/**
 * Shuts down audio device. This will never be called with the memory image codepath.
 */
void UALAudioDevice::FinishDestroy( void )
{
	if( !HasAnyFlags( RF_ClassDefaultObject ) )
	{
		Teardown();
		debugf( NAME_Exit, TEXT( "OpenAL Audio Device shut down." ) );
	}

	Super::FinishDestroy();
}

/**
 * Special variant of Destroy that gets called on fatal exit. Doesn't really
 * matter on the console so for now is just the same as Destroy so we can
 * verify that the code correctly cleans up everything.
 */
void UALAudioDevice::ShutdownAfterError( void )
{
	if( !HasAnyFlags( RF_ClassDefaultObject ) )
	{
		Teardown();
		debugf( NAME_Exit, TEXT( "UALAudioDevice::ShutdownAfterError" ) );
	}

	Super::ShutdownAfterError();
}

/*------------------------------------------------------------------------------------
	UAudioDevice Interface.
------------------------------------------------------------------------------------*/

/**
 * Initializes the audio device and creates sources.
 *
 * @warning: 
 *
 * @return TRUE if initialization was successful, FALSE otherwise
 */
UBOOL UALAudioDevice::Init( void )
{
	// Make sure no interface classes contain any garbage
	Effects = NULL;
	HardwareDevice = NULL;
	SoundContext = NULL;
	CaptureDevice = NULL;
	DLLHandle = NULL;

	// No channels, no sound.
	if( MaxChannels < 1 )
	{
		return( FALSE );
	}

	// Find DLL's.
	if( !DLLHandle )
	{
		DLLHandle = appGetDllHandle( AL_DLL );
		if( !DLLHandle )
		{
			debugf( NAME_Init, TEXT( "Couldn't locate %s - giving up." ), AL_DLL );
			return( FALSE );
		}
	}
	
	// Find functions.
	SUPPORTS_AL = TRUE;
	FindProcs( FALSE );
	if( !SUPPORTS_AL )
	{
		return( FALSE );
	}

	// Open device
	//TODO: Enumerate and select
#if __WIN32__
	HardwareDevice = alcOpenDevice( TCHAR_TO_ANSI( *DeviceName ) );
#else
	HardwareDevice = alcOpenDevice( NULL );
#endif
	if( !HardwareDevice )
	{
		debugf( NAME_Init, TEXT( "ALAudio: no OpenAL devices found." ) );
		return( FALSE );
	}

	// Display the audio device that was actually opened
	const ALCchar* DeviceName = alcGetString( HardwareDevice, ALC_DEVICE_SPECIFIER );
	debugf( NAME_Init, TEXT( "ALAudio device opened: %s" ), ANSI_TO_TCHAR( DeviceName ) );

	// Create a context
	INT Caps[] = { ALC_FREQUENCY, 44100, ALC_MAX_AUXILIARY_SENDS, 5, ALC_STEREO_SOURCES, 4, 0, 0 };
	SoundContext = alcCreateContext( HardwareDevice, Caps );
	if( !SoundContext )
	{
		debugf( NAME_Init, TEXT( "ALAudio: context creation failed." ) );
		return( FALSE );
	}

	alcMakeContextCurrent( SoundContext );
	
	// Make sure everything happened correctly
	if( alError( TEXT( "Init" ) ) )
	{
		debugf( NAME_Init, TEXT( "ALAudio: alcMakeContextCurrent failed." ) );
		return( FALSE );
	}

	debugf( NAME_Init, TEXT( "AL_VENDOR      : %s" ), ANSI_TO_TCHAR( ( ANSICHAR* )alGetString( AL_VENDOR ) ) );
	debugf( NAME_Init, TEXT( "AL_RENDERER    : %s" ), ANSI_TO_TCHAR( ( ANSICHAR* )alGetString( AL_RENDERER ) ) );
	debugf( NAME_Init, TEXT( "AL_VERSION     : %s" ), ANSI_TO_TCHAR( ( ANSICHAR* )alGetString( AL_VERSION ) ) );
	debugf( NAME_Init, TEXT( "AL_EXTENSIONS  : %s" ), ANSI_TO_TCHAR( ( ANSICHAR* )alGetString( AL_EXTENSIONS ) ) );
 
	// Get the enums for multichannel support
	Surround40Format = alGetEnumValue( "AL_FORMAT_QUAD16" );
	Surround51Format = alGetEnumValue( "AL_FORMAT_51CHN16" );
	Surround61Format = alGetEnumValue( "AL_FORMAT_61CHN16" );
	Surround71Format = alGetEnumValue( "AL_FORMAT_71CHN16" );

	// Initialize channels.
	alError( TEXT( "Emptying error stack" ), 0 );
	for( INT i = 0; i < Min( MaxChannels, MAX_AUDIOCHANNELS ); i++ )
	{
		ALuint SourceId;
		alGenSources( 1, &SourceId );
		if( !alError( TEXT( "Init (creating sources)" ), 0 ) )
		{
			FALSoundSource* Source = new FALSoundSource( this );
			Source->SourceId = SourceId;
			Sources.AddItem( Source );
			FreeSources.AddItem( Source );
		}
		else
		{
			break;
		}
	}

	if( Sources.Num() < 1 )
	{
		debugf( NAME_Error,TEXT( "ALAudio: couldn't allocate any sources" ) );
		return( FALSE );
	}

	// Update MaxChannels in case we couldn't create enough sources.
	MaxChannels = Sources.Num();

	// Use our own distance model.
	alDistanceModel( AL_NONE );
	
	// Open up the capture device
	CaptureDevice = new FALCaptureDevice();

	if( UseEffectsProcessing )
	{
		// Check for and init EFX extensions
		Effects = new FALAudioEffectsManager( this );
	}

	// Initialized.
	LastHWUpdate = 0;
	NextResourceID = 1;

	// Initialize base class last as it's going to precache already loaded audio.
	Super::Init();

	return( TRUE );
}

/**
 * Update the audio device and calculates the cached inverse transform later
 * on used for spatialization.
 *
 * @param	Realtime	whether we are paused or not
 */
void UALAudioDevice::Update( UBOOL Realtime )
{
	Super::Update( Realtime );

	// Set Player position
	FVector Location;

	// See file header for coordinate system explanation.
	Location.X = Listeners( 0 ).Location.X;
	Location.Y = Listeners( 0 ).Location.Z; // Z/Y swapped on purpose, see file header
	Location.Z = Listeners( 0 ).Location.Y; // Z/Y swapped on purpose, see file header
	Location *= AUDIO_DISTANCE_FACTOR;
	
	// Set Player orientation.
	FVector Orientation[2];

	// See file header for coordinate system explanation.
	Orientation[0].X = Listeners( 0 ).Front.X;
	Orientation[0].Y = Listeners( 0 ).Front.Z; // Z/Y swapped on purpose, see file header	
	Orientation[0].Z = Listeners( 0 ).Front.Y; // Z/Y swapped on purpose, see file header
	
	// See file header for coordinate system explanation.
	Orientation[1].X = Listeners( 0 ).Up.X;
	Orientation[1].Y = Listeners( 0 ).Up.Z; // Z/Y swapped on purpose, see file header
	Orientation[1].Z = Listeners( 0 ).Up.Y; // Z/Y swapped on purpose, see file header

	// Make the listener still and the sounds move relatively -- this allows 
	// us to scale the doppler effect on a per-sound basis.
	FVector Velocity = FVector( 0.0f, 0.0f, 0.0f );
	
	alListenerfv( AL_POSITION, ( ALfloat * )&Location );
	alListenerfv( AL_ORIENTATION, ( ALfloat * )&Orientation[0] );
	alListenerfv( AL_VELOCITY, ( ALfloat * )&Velocity );

	// Deferred commit (enforce min time between updates).
	if( GCurrentTime < LastHWUpdate )
	{
		LastHWUpdate = GCurrentTime;
	}

	if( ( GCurrentTime - LastHWUpdate ) >= ( TimeBetweenHWUpdates / 1000.f ) )
	{
		LastHWUpdate = GCurrentTime;
		alcProcessContext( SoundContext );
#ifdef WIN32
		alcSuspendContext( SoundContext );
#endif
	}

	alError( TEXT( "UALAudioDevice::Update" ) );
}

/**
 * Precaches the passed in sound node wave object.
 *
 * @param	SoundNodeWave	Resource to be precached.
 */
void UALAudioDevice::Precache( USoundNodeWave* SoundNodeWave )
{
	FALSoundBuffer::Init( SoundNodeWave, this );
}

/**
 * Frees the bulk resource data associated with this SoundNodeWave.
 *
 * @param	SoundNodeWave	wave object to free associated bulk data
 */
void UALAudioDevice::FreeResource( USoundNodeWave* SoundNodeWave )
{
	// Find buffer for resident wavs
	if( SoundNodeWave->ResourceID )
	{
		// Find buffer associated with resource id.
		FALSoundBuffer* Buffer = WaveBufferMap.FindRef( SoundNodeWave->ResourceID );
		if( Buffer )
		{
			// Remove from buffers array.
			Buffers.RemoveItem( Buffer );

			// Delete it. This will automatically remove itself from the WaveBufferMap.
			delete Buffer;
		}

		SoundNodeWave->ResourceID = 0;
	}

	// .. or reference to compressed data
	SoundNodeWave->RemoveAudioResource();
}

/** 
 * Attempts to start recording input from the microphone
 *
 * @return	TRUE if recording started successfully
 */
UBOOL UALAudioDevice::CaptureStart( void )
{
	if( CaptureDevice )
	{
		return( CaptureDevice->CaptureStart() );
	}

	return( FALSE );
}

/** 
 * Stops recording input from the microphone
 *
 * @return	Number of samples recorded. 0 is returned if the the capture was not or failed to start.
 */
UBOOL UALAudioDevice::CaptureStop( void )
{
	if( CaptureDevice )
	{
		return( CaptureDevice->CaptureStop() );
	}

	return( FALSE );
}

/**
 * Returns a pointer the start of the captured data
 *
 * @return SWORD * to captured samples
 */
INT UALAudioDevice::CaptureGetSamples( SWORD*& Samples )
{
	if( CaptureDevice )
	{
		return( CaptureDevice->CaptureGetSamples( Samples ) );
	}

	return( 0 );
}

/** 
 * Sets the input level of the microphone and returns the level.
 * Pass in a negative number to just return the current volume.
 */
void UALAudioDevice::SetMicInputLevel( FLOAT Level )
{
	if( CaptureDevice )
	{
		CaptureDevice->SetMicInputLevel( Level );
	}
}

/** 
 * Displays debug information about the loaded sounds
 */
void UALAudioDevice::ListSounds( FOutputDevice& Ar )
{
	INT	TotalResident;
	INT	ResidentCount;
	INT	TotalRealTime;
	INT	RealTimeCount;

	Ar.Logf( TEXT( "Sound resources:" ) );
	TotalResident = 0;
	ResidentCount = 0;
	TotalRealTime = 0;
	RealTimeCount = 0;
	for( TObjectIterator<USoundNodeWave> It; It; ++It )
	{
		USoundNodeWave* Sound = *It;
		if( Sound->ResourceID )
		{
			Ar.Logf( TEXT( "Resident: %6i Kb (%d channels at %d Hz) in sound %s" ), Sound->SampleDataSize / 1024, Sound->NumChannels, Sound->SampleRate, *Sound->GetName() );
			TotalResident += Sound->SampleDataSize / 1024;
			ResidentCount++;
		}
		else if( Sound->ResourceSize )
		{
			Ar.Logf( TEXT( "Real time: %6i Kb (%d channels at %d Hz) in sound %s" ), Sound->ResourceSize / 1024, Sound->NumChannels, Sound->SampleRate, *Sound->GetName() );
			TotalRealTime += Sound->ResourceSize / 1024;
			RealTimeCount++;
		}
	}

	Ar.Logf( TEXT( "%8i Kb for %d resident sounds" ), TotalResident, ResidentCount );
	Ar.Logf( TEXT( "%8i Kb for %d real time decompressed sounds" ), TotalRealTime, RealTimeCount );
}

/** Test decompress a vorbis file */
void UALAudioDevice::TestDecompressOggVorbis( USoundNodeWave* Wave )
{
	FVorbisAudioInfo	OggInfo;
	TArray<BYTE>		PCMData;

	// Parse the ogg vorbis header for the relevant information
	if( OggInfo.ReadCompressedInfo( Wave ) )
	{
		// Decompress all the sample data (and preallocate memory)
		OggInfo.ExpandFile( Wave, &PCMData );
	}
}

/** Decompress a wav a number of times for profiling purposes */
void UALAudioDevice::TimeTest( FOutputDevice& Ar, TCHAR* WaveAssetName )
{
	USoundNodeWave* Wave = LoadObject<USoundNodeWave>( NULL, WaveAssetName, NULL, LOAD_NoWarn, NULL );
	if( Wave )
	{
		Wave->InitAudioResource( Wave->CompressedPCData );

		DOUBLE Start = appSeconds();

		for( INT i = 0; i < 1000; i++ )
		{
			TestDecompressOggVorbis( Wave );
		} 

		DOUBLE Duration = appSeconds() - Start;
		Ar.Logf( TEXT( "%s: %g ms - %g ms per second per channel" ), WaveAssetName, Duration, Duration / ( Wave->Duration * Wave->NumChannels ) );

		Wave->RemoveAudioResource();
	}
	else
	{
		Ar.Logf( TEXT( "Failed to find test file '%s' to decompress" ), WaveAssetName );
	}
}

/**
 * Exec handler used to parse console commands.
 *
 * @param	Cmd		Command to parse
 * @param	Ar		Output device to use in case the handler prints anything
 * @return	TRUE if command was handled, FALSE otherwise
 */
UBOOL UALAudioDevice::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	if( Super::Exec( Cmd, Ar ) )
	{
		return( TRUE );
	}
	else if( ParseCommand( &Cmd, TEXT( "TestVorbisDecompressionSpeed" ) ) )
	{
		TimeTest( Ar, TEXT( "TestSounds.AmbientWind" ) );

		TimeTest( Ar, TEXT( "TestSounds.44Mono_TestWeaponSynthetic" ) );
		TimeTest( Ar, TEXT( "TestSounds.44Mono_TestDialogFemale" ) );
		TimeTest( Ar, TEXT( "TestSounds.44Mono_TestDialogMale" ) );

		TimeTest( Ar, TEXT( "TestSounds.22Mono_TestWeaponSynthetic" ) );
		TimeTest( Ar, TEXT( "TestSounds.22Mono_TestDialogFemale" ) );
		TimeTest( Ar, TEXT( "TestSounds.22Mono_TestDialogMale" ) );

		TimeTest( Ar, TEXT( "TestSounds.22Stereo_TestMusicAcoustic" ) );
		TimeTest( Ar, TEXT( "TestSounds.44Stereo_TestMusicAcoustic" ) );
	}
#if 0
	else if( ParseCommand(&Cmd,TEXT("TOGGLEDRYAUDIO")) )
	{
		//if (DryVolume == 0.0f)
		//{
		//	Ar.Logf(TEXT("Dry audio on"));
		//	DryVolume = 1.0f;
		//}
		//else
		//{
		//	Ar.Logf(TEXT("Dry audio off"));
		//	DryVolume = 0.0f;
		//}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLEREVERB")) )
	{
		//if (ReverbVolume == 0.0f)
		//{
		//	Ar.Logf(TEXT("Reverb audio on"));
		//	ReverbVolume = 1.0f;
		//}
		//else
		//{
		//	Ar.Logf(TEXT("Reverb audio off"));
		//	ReverbVolume = 0.0f;
		//}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLERADIO")) )
	{
		//if (RadioVolume == 0.0f)
		//{
		//	Ar.Logf(TEXT("Radio voice on"));
		//	RadioVolume = 1.0f;
		//}
		//else
		//{
		//	Ar.Logf(TEXT("Radio voice off"));
		//	RadioVolume = 0.0f;
		//}
	}
	else if( ParseCommand(&Cmd,TEXT("ISOLATERADIO")) )
	{
		//{
		//	Ar.Logf(TEXT("Radio voice audio isolated"));
		//	DryVolume = 0.0f;
		//	ReverbVolume = 0.0f;
		//	RadioVolume = 1.0f;
		//	CenterVolume = 0.0f;
		//}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLECENTERVOICE")) )
	{
		//if (CenterVolume == 0.0f)
		//{
		//	Ar.Logf(TEXT("Center channel voice on"));
		//	CenterVolume = 1.0f;
		//}
		//else
		//{
		//	Ar.Logf(TEXT("Center channel voice off"));
		//	CenterVolume = 0.0f;
		//}
	}
	else if( ParseCommand(&Cmd,TEXT("ISOLATECENTERVOICE")) )
	{
		//{
		//	Ar.Logf(TEXT("Center channel voice audio isolated"));
		//	DryVolume = 0.0f;
		//	ReverbVolume = 0.0f;
		//	RadioVolume = 0.0f;
		//	CenterVolume = 1.0f;
		//}
		return TRUE;
	}
#endif
	return FALSE;
}

ALuint UALAudioDevice::GetInternalFormat( INT NumChannels )
{
	ALuint InternalFormat = 0;

	switch( NumChannels )
	{
	case 0:
	case 3:
	case 5:
		break;
	case 1:
		InternalFormat = AL_FORMAT_MONO16;
		break;
	case 2:
		InternalFormat = AL_FORMAT_STEREO16;
		break;
	case 4:
		InternalFormat = Surround40Format;
		break;
	case 6:
		InternalFormat = Surround51Format;
		break;	
	case 7:
		InternalFormat = Surround61Format;
		break;	
	case 8:
		InternalFormat = Surround71Format;
		break;
	}

	return( InternalFormat );
}

/*------------------------------------------------------------------------------------
OpenAL utility functions
------------------------------------------------------------------------------------*/
//
//	FindExt
//
UBOOL UALAudioDevice::FindExt( const TCHAR* Name )
{
	if( alIsExtensionPresent( TCHAR_TO_ANSI( Name ) ) || alcIsExtensionPresent( HardwareDevice, TCHAR_TO_ANSI( Name ) ) )
	{
		debugf( NAME_Init, TEXT( "Device supports: %s" ), Name );
		return( TRUE );
	}

	return( FALSE );
}

//
//	FindProc
//
void UALAudioDevice::FindProc( void*& ProcAddress, char* Name, char* SupportName, UBOOL& Supports, UBOOL AllowExt )
{
	ProcAddress = NULL;
	if( !ProcAddress )
	{
		ProcAddress = appGetDllExport( DLLHandle, ANSI_TO_TCHAR( Name ) );
	}
	if( !ProcAddress && Supports && AllowExt )
	{
		ProcAddress = alGetProcAddress( ( ALchar * ) Name );
	}
	if( !ProcAddress )
	{
		if( Supports )
		{
			debugf( TEXT("   Missing function '%s' for '%s' support"), ANSI_TO_TCHAR( Name ), ANSI_TO_TCHAR( SupportName ) );
		}
		Supports = FALSE;
	}
}

//
//	FindProcs
//
void UALAudioDevice::FindProcs( UBOOL AllowExt )
{
#define AL_EXT( name, strname ) if( AllowExt ) SUPPORTS##name = FindExt( TEXT( #strname ) );
#define AL_PROC( name, strname, ret, func, parms ) FindProc( *( void ** )&func, #func, #strname, SUPPORTS##name, AllowExt );
#include "ALFuncs.h"
#undef AL_EXT
#undef AL_PROC
}

//
//	alError
//
UBOOL UALAudioDevice::alError( TCHAR* Text, UBOOL Log )
{
	ALenum Error = alGetError();
	if( Error != AL_NO_ERROR )
	{
		do 
		{		
			if ( Log )
			{
				switch ( Error )
				{
				case AL_INVALID_NAME:
					debugf( TEXT( "ALAudio: AL_INVALID_NAME in %s" ), Text );
					break;
				case AL_INVALID_VALUE:
					debugf( TEXT( "ALAudio: AL_INVALID_VALUE in %s" ), Text );
					break;
				case AL_OUT_OF_MEMORY:
					debugf( TEXT( "ALAudio: AL_OUT_OF_MEMORY in %s" ), Text );
					break;
				case AL_INVALID_ENUM:
					debugf( TEXT( "ALAudio: AL_INVALID_ENUM in %s" ), Text );
					break;
				case AL_INVALID_OPERATION:
					debugf( TEXT( "ALAudio: AL_INVALID_OPERATION in %s" ), Text );
					break;
				default:
					debugf( TEXT( "ALAudio: Unknown error in %s" ), Text );
					break;
				}
			}
		}
		while( ( Error = alGetError() ) != AL_NO_ERROR );

		return TRUE;
	}

	return FALSE;
}

/*------------------------------------------------------------------------------------
	FALCaptureDevice.
------------------------------------------------------------------------------------*/

/** Find and initialise the control used to set the microphone volume */
void FALCaptureDevice::InitMicControl( void )
{
	HRESULT	Error;

	ControlID = 0;

	// Do we have any microphones attached?
	INT NumDevices = waveInGetNumDevs();
	if( !NumDevices )
	{
		return;
	}

	// Open the mixer device
	HMIXER	Mixer;
		
	Error = mixerOpen( &Mixer, 0, NULL, NULL, 0 );
	if( Error != MMSYSERR_NOERROR )
	{
		return;
	}

	// Get the line of this component
	MIXERLINE MixerLine = { 0 };
	MixerLine.cbStruct = sizeof( MIXERLINE );
	MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;

	Error = mixerGetLineInfo( ( HMIXEROBJ )Mixer, &MixerLine, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE );
	if( Error != MMSYSERR_NOERROR )
	{
		mixerClose( Mixer );
		return;
	}

	MIXERCONTROL MixerControl = { 0 };
	MixerControl.cbStruct = sizeof( MIXERCONTROL );

	MIXERLINECONTROLS MixerLineControl = { 0 };
	MixerLineControl.cbStruct = sizeof( MIXERLINECONTROLS );
	MixerLineControl.dwLineID = MixerLine.dwLineID;
	MixerLineControl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	MixerLineControl.cControls = 1;
	MixerLineControl.cbmxctrl = sizeof( MIXERCONTROL );
	MixerLineControl.pamxctrl = &MixerControl;

	Error = mixerGetLineControls( ( HMIXEROBJ )Mixer, &MixerLineControl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE );
	if( Error != MMSYSERR_NOERROR )
	{
		mixerClose( Mixer );
		return;
	}

	ControlID = MixerControl.dwControlID;
	Minimum = MixerControl.Bounds.dwMinimum;
	Maximum = MixerControl.Bounds.dwMaximum;
	mixerClose( Mixer );
}

/** Sets the mic input control to a value */
void FALCaptureDevice::SetMicInputControl( DWORD InputLevel )
{
	HRESULT	Error;

	if( !ControlID )
	{
		return;
	}

	HMIXER	Mixer;

	Error = mixerOpen( &Mixer, 0, NULL, NULL, 0 );
	if( Error != MMSYSERR_NOERROR )
	{
		return;
	}

	MIXERCONTROLDETAILS_UNSIGNED MixerControlDetailsUnsigned = { 0 };
	MixerControlDetailsUnsigned.dwValue = InputLevel;

	MIXERCONTROLDETAILS MixerControlDetails = { 0 };
	MixerControlDetails.cbStruct = sizeof( MIXERCONTROLDETAILS );
	MixerControlDetails.dwControlID = ControlID;
	MixerControlDetails.cChannels = 1;
	MixerControlDetails.cbDetails = sizeof( MIXERCONTROLDETAILS_UNSIGNED );
	MixerControlDetails.paDetails = &MixerControlDetailsUnsigned;

	mixerSetControlDetails( ( HMIXEROBJ )Mixer, &MixerControlDetails, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE );
	
	mixerClose( Mixer );
}

/** 
 * Create a capture device to record data from the microphone
 */
FALCaptureDevice::FALCaptureDevice( void )
{
	Frequency = 16000;
	BufferLength = 5;

	Device = alcCaptureOpenDevice( NULL, Frequency, AL_FORMAT_MONO16, Frequency * sizeof( SWORD ) * BufferLength );
	if( Device != NULL )
	{
		const ALCchar* DeviceName = alcGetString( Device, ALC_CAPTURE_DEVICE_SPECIFIER );
		debugf( NAME_Init, TEXT( "ALAudio capture device opened: %s" ), ANSI_TO_TCHAR( DeviceName ) );

		SamplesEnum = alcGetEnumValue( Device, ( ALCchar * )"ALC_CAPTURE_SAMPLES" );
		Buffer = new SWORD [Frequency * BufferLength];

		InitMicControl();
	}
}

/** 
 * Clean up any potential assets allocated 
 */
FALCaptureDevice::~FALCaptureDevice( void )
{
	if( Device )
	{
		alcCaptureStop( Device );

		alcCaptureCloseDevice( Device );

		if( Buffer )
		{
			delete Buffer;
			Buffer = NULL;
		}
	}
}

/** 
 * Attempts to start recording input from the microphone
 *
 * @return	TRUE if recording started successfully
 */
UBOOL FALCaptureDevice::CaptureStart( void )
{
	if( Device )
	{
		alcCaptureStart( Device );
		return( TRUE );
	}

	return( FALSE );
}

/** 
 * Stops recording input from the microphone
 *
 * @return	Number of samples recorded. 0 is returned if the the capture was not or failed to start.
 */
UBOOL FALCaptureDevice::CaptureStop( void )
{
	if( Device )
	{
		alcCaptureStop( Device );
		return( TRUE );
	}

	return( FALSE );
}

/**
 * Returns a pointer the start of the captured data
 *
 * @return SWORD* to captured samples
 * @return INT number of captured samples
 */
INT FALCaptureDevice::CaptureGetSamples( SWORD*& Samples )
{
	INT	NumSamples = 0;
	Samples = NULL;
	if( Device )
	{
		alcGetIntegerv( Device, SamplesEnum, sizeof( NumSamples ), &NumSamples );

		alcCaptureSamples( Device, Buffer, NumSamples );

		Samples = Buffer;
	}

	return( NumSamples );
}

/** 
 * Sets the input level of the microphone and returns the level.
 * Pass in a negative number to just return the current volume.
 */
void FALCaptureDevice::SetMicInputLevel( FLOAT Level )
{
	SetMicInputControl( DWORD( Level * ( Maximum - Minimum ) ) );
}

/*------------------------------------------------------------------------------------
	FALSoundSource.
------------------------------------------------------------------------------------*/

/**
 * Initializes a source with a given wave instance and prepares it for playback.
 *
 * @param	WaveInstance	wave instace being primed for playback
 * @return	TRUE if initialization was successful, FALSE otherwise
 */
UBOOL FALSoundSource::Init( FWaveInstance* InWaveInstance )
{
	// Find matching buffer.
	Buffer = FALSoundBuffer::Init( InWaveInstance->WaveData, ( UALAudioDevice * )AudioDevice );
	if( Buffer )
	{
		SCOPE_CYCLE_COUNTER( STAT_AudioSourceInitTime );

		WaveInstance = InWaveInstance;

		// Prime the buffers if necessary
		if( Buffer->CurrentBuffer )
		{
			// NOTE: loop notifications will not be sent from here. Should only happen for really short sounds. All real
			// time decompressed sounds are long.
			Buffer->DecodeCompressed( WaveInstance->LoopingMode != LOOP_Never );
			Buffer->DecodeCompressed( WaveInstance->LoopingMode != LOOP_Never );
		}

		// Enable/disable spatialisation of sounds
		alSourcei( SourceId, AL_SOURCE_RELATIVE, WaveInstance->bUseSpatialization ? AL_FALSE : AL_TRUE );

		// Setting looping on a real time decompressed source suppresses the buffers processed message
		alSourcei( SourceId, AL_LOOPING, ( !Buffer->CurrentBuffer && WaveInstance->LoopingMode == LOOP_Forever ) ? AL_TRUE : AL_FALSE );

		// Always queue up the first buffer
		alSourceQueueBuffers( SourceId, 1, Buffer->BufferIds );	
		if( Buffer->CurrentBuffer )
		{
			// Queue up the second buffer if we are double buffered
			alSourceQueueBuffers( SourceId, 1, Buffer->BufferIds + 1 );	
		}
		else if( WaveInstance->LoopingMode == LOOP_WithNotification )
		{		
			// We queue the sound twice for wave instances that use seamless looping so we can have smooth 
			// loop transitions. The downside is that we might play at most one frame worth of audio from the 
			// beginning of the wave after the wave stops looping.
			alSourceQueueBuffers( SourceId, 1, Buffer->BufferIds );
		}

		// Whether to apply the EQ effect to this source
		EQApplied = ( WaveInstance->SoundModeVoice > 0 );

		Update();

		// Initialization was successful.
		return( TRUE );
	}

	// Failed to initialize source.
	return( FALSE );
}

/**
 * Updates the source specific parameter like e.g. volume and pitch based on the associated
 * wave instance.	
 */
void FALSoundSource::Update( void )
{
	SCOPE_CYCLE_COUNTER( STAT_AudioUpdateSources );

	if( !WaveInstance || Paused )
	{
		return;
	}

	FLOAT Volume = Clamp<FLOAT>( WaveInstance->Volume, 0.0f, 1.0f );
	FLOAT Pitch = Clamp<FLOAT>( WaveInstance->Pitch, 0.5f, 2.0f );

	// Apply reverb on voices that are spatialised or single-channel (eg Marcus' dialog) and not specifically excluded from reverb.
	ReverbApplied = ( ( WaveInstance->bUseSpatialization ) || ( Buffer->NumChannels == 1 ) ) && ( !WaveInstance->bNoReverb );
	LowPassFilterApplied = ( WaveInstance->LowPassFilterApplied || AudioDevice->IsTestingLowPassFilter() );

	FVector Location;
	FVector	Velocity;

	// See file header for coordinate system explanation.
	Location.X = WaveInstance->Location.X;
	Location.Y = WaveInstance->Location.Z; // Z/Y swapped on purpose, see file header
	Location.Z = WaveInstance->Location.Y; // Z/Y swapped on purpose, see file header
	
	Velocity.X = WaveInstance->Velocity.X;
	Velocity.Y = WaveInstance->Velocity.Z; // Z/Y swapped on purpose, see file header
	Velocity.Z = WaveInstance->Velocity.Y; // Z/Y swapped on purpose, see file header

	// Convert to meters.
	Location *= AUDIO_DISTANCE_FACTOR;
	Velocity *= AUDIO_DISTANCE_FACTOR;

	// We're using a relative coordinate system for un- spatialized sounds.
	if( !WaveInstance->bUseSpatialization )
	{
		Location = FVector( 0.f, 0.f, 0.f );
	}

	alSourcef( SourceId, AL_GAIN, Volume );	
	alSourcef( SourceId, AL_PITCH, Pitch );		

	alSourcefv( SourceId, AL_POSITION, ( ALfloat * )&Location );
	alSourcefv( SourceId, AL_VELOCITY, ( ALfloat * )&Velocity );

	// Set the source to stream to the effects system if it exists
	AudioDevice->LinkEffect( this );
}

/**
 * Plays the current wave instance.	
 */
void FALSoundSource::Play( void )
{
	if( WaveInstance )
	{
		alSourcePlay( SourceId );
		Paused = FALSE;
		Playing = TRUE;
		// FIXME: Is this valid coming out of pause?
		BuffersToFlush = FALSE;
	}
}

/**
 * Stops the current wave instance and detaches it from the source.	
 */
void FALSoundSource::Stop( void )
{
	if( WaveInstance )
	{	
		alSourceStop( SourceId );
		// This clears out any pending buffers that may or may not be queued or played
		alSourcei( SourceId, AL_BUFFER, 0 );
		Paused = FALSE;
		Playing = FALSE;

		// Always delete real time decoding buffers as they are always created 
		if( Buffer->CurrentBuffer )
		{
			delete Buffer;
		}

		Buffer = NULL;
		BuffersToFlush = FALSE;
	}

	FSoundSource::Stop();
}

/**
 * Pauses playback of current wave instance.
 */
void FALSoundSource::Pause( void )
{
	if( WaveInstance )
	{
		alSourcePause( SourceId );
		Paused = TRUE;
	}
}

/**
 * Returns whether the buffer associated with this source is using CPU decompression.
 *
 * @return TRUE if decompressed on the CPU, FALSE otherwise
 */
UBOOL FALSoundSource::UsesCPUDecompression( void )
{
	if( Buffer && Buffer->CurrentBuffer )
	{
		return( TRUE );
	}

	return( FALSE );
}

/**
 * Handles feeding new data to a real time decompressed sound
 */
void FALSoundSource::HandleRealTimeSource( void )
{
	// Get the next bit of streaming data
	UBOOL bLooped = Buffer->DecodeCompressed( WaveInstance->LoopingMode != LOOP_Never );

	// Queue the next chunk of sound data
	alSourceQueueBuffers( SourceId, 1, Buffer->BufferIds + ( Buffer->CurrentBuffer & 1 ) );

	// Have we reached the end of the compressed sound?
	if( bLooped )
	{
		switch( WaveInstance->LoopingMode )
		{
		case LOOP_Never:
			// Play out any queued buffers - once there are no buffers left, the state check at the beginning of IsFinished will fire
			BuffersToFlush = TRUE;
			break;

		case LOOP_WithNotification:
			// If we have just looped, and we are programmatically looping, send notification
			WaveInstance->NotifyFinished();
			break;

		case LOOP_Forever:
			// Let the sound loop indefinitely
			break;
		}
	}
}

/**
 * Queries the status of the currently associated wave instance.
 *
 * @return	TRUE if the wave instance/ source has finished playback and FALSE if it is 
 *			currently playing or paused.
 */
UBOOL FALSoundSource::IsFinished( void )
{
	if( WaveInstance )
	{
		ALint State = AL_STOPPED;

		// Check the state of the source
		alGetSourcei( SourceId, AL_SOURCE_STATE, &State );
		if( State != AL_PLAYING && State != AL_PAUSED )
		{
			// Notify the wave instance that it has finished playing.
			WaveInstance->NotifyFinished();
			return( TRUE );
		}
		else 
		{
			// Check to see if any complete buffers have been processed
			ALint BuffersProcessed;
			alGetSourcei( SourceId, AL_BUFFERS_PROCESSED, &BuffersProcessed );

			if( BuffersProcessed )
			{
				ALuint	DequeuedBuffer;

				// Unqueue the processed buffers
				alSourceUnqueueBuffers( SourceId, BuffersProcessed, &DequeuedBuffer );

				// Check for real time decompressed sounds
				if( Buffer->CurrentBuffer )
				{
					if( !BuffersToFlush )
					{
						// Continue feeding new sound data unless we are waiting for the sound to finish
						HandleRealTimeSource();
					}
				}
				else
				{
					// Notify the wave instance that the current (native) buffer has finished playing.
					WaveInstance->NotifyFinished();

					// Queue the same packet again for looping
					alSourceQueueBuffers( SourceId, 1, Buffer->BufferIds );
				}
			}
		}

		return( FALSE );
	}

	return( TRUE );
}

/*------------------------------------------------------------------------------------
	FALSoundBuffer.
------------------------------------------------------------------------------------*/

/** 
 * Constructor
 *
 * @param AudioDevice	audio device this sound buffer is going to be attached to.
 */
FALSoundBuffer::FALSoundBuffer( UALAudioDevice* InAudioDevice )
{
	AudioDevice	= InAudioDevice;
	DecompressionState = NULL;
}

/**
 * Destructor 
 * 
 * Frees wave data and detaches itself from audio device.
 */
FALSoundBuffer::~FALSoundBuffer( void )
{
	DEC_DWORD_STAT_BY( STAT_AudioMemorySize, GetSize() );
	DEC_DWORD_STAT_BY( STAT_AudioMemory, GetSize() );

	if( ResourceID )
	{
		AudioDevice->WaveBufferMap.Remove( ResourceID );
	}

	// Delete AL buffers.
	alDeleteBuffers( 1, BufferIds );
	if( CurrentBuffer )
	{
		alDeleteBuffers( 1, BufferIds + 1 );
	}

	FCompressedAudioInfo* CompressedAudioInfo = ( FCompressedAudioInfo * )DecompressionState;
	delete CompressedAudioInfo;
}

/**
 * Locate and precache if necessary the ogg vorbis data. Decompress and validate the header.
 *
 * @param	InWave		USoundNodeWave to use as template and wave source
 */
void FALSoundBuffer::PrepareDecompression( UALAudioDevice* AudioDevice, USoundNodeWave* Wave )
{
	SCOPE_CYCLE_COUNTER( STAT_AudioPrepareDecompressionTime );

	check( !Wave->UseTTS );

	DecompressionState = new FVorbisAudioInfo();

	Wave->InitAudioResource( Wave->CompressedPCData );

	DecompressionState->ReadCompressedInfo( Wave );

	InternalFormat = AudioDevice->GetInternalFormat( Wave->NumChannels );

	NumChannels = Wave->NumChannels;
	BufferSize = Wave->SampleDataSize;
	SampleRate = Wave->SampleRate;
}

/**
 * Decompress the next chunk of sound into CurrentBuffer
 */
UBOOL FALSoundBuffer::DecodeCompressed( UBOOL bLooping )
{
	SCOPE_CYCLE_COUNTER( STAT_AudioDecompressTime );

	TArray<BYTE>	PCMData;

	FCompressedAudioInfo* CompressedAudioInfo = ( FCompressedAudioInfo* )DecompressionState;
	UBOOL bLooped = CompressedAudioInfo->ReadCompressedData( &PCMData, NumChannels, bLooping, MONO_PCM_BUFFER_SIZE );

	CurrentBuffer++;
	alBufferData( BufferIds[CurrentBuffer & 1], InternalFormat, PCMData.GetTypedData(), PCMData.Num(), SampleRate );

	return( bLooped );
}

FALSoundBuffer* FALSoundBuffer::CreateQueuedBuffer( USoundNodeWave* Wave, UALAudioDevice* AudioDevice )
{
	FALSoundBuffer* Buffer = NULL;

	// Create new buffer.
	Buffer = new FALSoundBuffer( AudioDevice );

	Buffer->CurrentBuffer = 1;

	// Generate 2 buffers to double buffer data in to
	alGenBuffers( 2, Buffer->BufferIds );
	AudioDevice->alError( TEXT( "RegisterSound (generating queued buffer)" ) );		

	// Keep track of associated resource name.
	Buffer->ResourceName = Wave->GetPathName();
	Buffer->ResourceID = 0;

	// Prime the first two buffers
	Buffer->PrepareDecompression( AudioDevice, Wave );
	
	INC_DWORD_STAT_BY( STAT_AudioMemorySize, Buffer->GetSize() );
	INC_DWORD_STAT_BY( STAT_AudioMemory, Buffer->GetSize() );

	if( Buffer->InternalFormat == 0 )
	{
		debugf( NAME_Warning, TEXT( "Audio: sound format not supported." ) );
		delete Buffer;
		Buffer = NULL;
	}

	return( Buffer );
}

/**
 * Static function used to create an OpenAL buffer and upload decompressed ogg vorbis data to.
 *
 * @param InWave		USoundNodeWave to use as template and wave source
 * @param AudioDevice	audio device to attach created buffer to
 * @return FALSoundBuffer pointer if buffer creation succeeded, NULL otherwise
 */
FALSoundBuffer* FALSoundBuffer::CreateNativeBuffer( USoundNodeWave* Wave, UALAudioDevice* AudioDevice )
{
	FALSoundBuffer*		Buffer = NULL;
	FVorbisAudioInfo	OggInfo;
	TArray<BYTE>		PCMData;

	// Grab the data from the bulk data
	Wave->InitAudioResource( Wave->CompressedPCData );

	// Parse the ogg vorbis header for the relevant information
	if( !OggInfo.ReadCompressedInfo( Wave ) )
	{
		return( NULL );
	}

	// Decompress all the sample data
	OggInfo.ExpandFile( Wave, &PCMData );

	// Create new buffer.
	Buffer = new FALSoundBuffer( AudioDevice );

	Buffer->CurrentBuffer = 0;
	Buffer->InternalFormat = AudioDevice->GetInternalFormat( Wave->NumChannels );		
	Buffer->NumChannels = Wave->NumChannels;
	Buffer->SampleRate = Wave->SampleRate;

	alGenBuffers( 1, Buffer->BufferIds );
	AudioDevice->alError( TEXT( "RegisterSound (generating native buffer)" ) );

	alBufferData( Buffer->BufferIds[0], Buffer->InternalFormat, PCMData.GetTypedData(), PCMData.Num(), Buffer->SampleRate );
	if( AudioDevice->alError( TEXT( "RegisterSound (creating buffer)" ) ) )
	{
		Buffer->InternalFormat = 0;
	}

	// Allocate new resource ID and assign to USoundNodeWave. A value of 0 (default) means not yet registered.
	INT ResourceID = AudioDevice->NextResourceID++;
	Buffer->ResourceID = ResourceID;
	Wave->ResourceID = ResourceID;

	Buffer->BufferSize = PCMData.Num();

	AudioDevice->Buffers.AddItem( Buffer );
	AudioDevice->WaveBufferMap.Set( ResourceID, Buffer );

	// Keep track of associated resource name.
	Buffer->ResourceName = Wave->GetPathName();

	Wave->RemoveAudioResource();

	INC_DWORD_STAT_BY( STAT_AudioMemorySize, Buffer->GetSize() );
	INC_DWORD_STAT_BY( STAT_AudioMemory, Buffer->GetSize() );

	if( Buffer->InternalFormat == 0 )
	{
		debugf( NAME_Warning, TEXT( "Audio: sound format not supported." ) );
		delete Buffer;
		Buffer = NULL;
	}

	return( Buffer );
}

/**
 * Static function used to create an OpenAL buffer and upload raw PCM data to.
 *
 * @param InWave		USoundNodeWave to use as template and wave source
 * @param AudioDevice	audio device to attach created buffer to
 * @return FALSoundBuffer pointer if buffer creation succeeded, NULL otherwise
 */
FALSoundBuffer* FALSoundBuffer::CreatePreviewBuffer( FALSoundBuffer* Buffer, USoundNodeWave* Wave, UALAudioDevice* AudioDevice )
{
	if( Buffer == NULL )
	{
		// Create new buffer.
		Buffer = new FALSoundBuffer( AudioDevice );

		alGenBuffers( 1, Buffer->BufferIds );
		AudioDevice->alError( TEXT( "RegisterSound (generating preview buffer)" ) );

		// Allocate new resource ID and assign to USoundNodeWave. A value of 0 (default) means not yet registered.
		INT ResourceID = AudioDevice->NextResourceID++;
		Buffer->ResourceID = ResourceID;
		Wave->ResourceID = ResourceID;

		AudioDevice->Buffers.AddItem( Buffer );
		AudioDevice->WaveBufferMap.Set( ResourceID, Buffer );

		// Keep track of associated resource name.
		Buffer->ResourceName = Wave->GetPathName();
		Buffer->BufferSize = Wave->SampleDataSize;
	}

	Buffer->CurrentBuffer = 0;
	Buffer->InternalFormat = AudioDevice->GetInternalFormat( Wave->NumChannels );		
	Buffer->NumChannels = Wave->NumChannels;
	Buffer->SampleRate = Wave->SampleRate;

	alBufferData( Buffer->BufferIds[0], Buffer->InternalFormat, Wave->RawPCMData, Wave->SampleDataSize, Buffer->SampleRate );
	if( AudioDevice->alError( TEXT( "RegisterSound (creating preview buffer)" ) ) )
	{
		Buffer->InternalFormat = 0;
	}

	// Free up the data if necessary
	if( Wave->DynamicResource )
	{
		appFree( Wave->RawPCMData );
		Wave->RawPCMData = NULL;
		Wave->DynamicResource = FALSE;
	}

	INC_DWORD_STAT_BY( STAT_AudioMemorySize, Buffer->GetSize() );
	INC_DWORD_STAT_BY( STAT_AudioMemory, Buffer->GetSize() );

	if( Buffer->InternalFormat == 0 )
	{
		debugf( NAME_Warning, TEXT( "Audio: sound format not supported." ) );
		delete Buffer;
		Buffer = NULL;
	}

	return( Buffer );
}

/**
 * Static function used to create a buffer.
 *
 * @param InWave		USoundNodeWave to use as template and wave source
 * @param AudioDevice	audio device to attach created buffer to
 * @return FALSoundBuffer pointer if buffer creation succeeded, NULL otherwise
 */
FALSoundBuffer* FALSoundBuffer::Init( USoundNodeWave* Wave, UALAudioDevice* AudioDevice )
{
	SCOPE_CYCLE_COUNTER( STAT_AudioResourceCreationTime );

	// Can't create a buffer without any source data
	if( Wave == NULL || Wave->NumChannels == 0 )
	{
		return( NULL );
	}

	FALSoundBuffer* Buffer = NULL;

	if( Wave->UseTTS || Wave->RawPCMData )
	{
		// Find the existing buffer if any
		if( Wave->ResourceID )
		{
			Buffer = AudioDevice->WaveBufferMap.FindRef( Wave->ResourceID );
		}

		// Override with any new PCM data even if some already exists
		if( Wave->RawPCMData )
		{
			// Upload the preview PCM data to it
			Buffer = CreatePreviewBuffer( Buffer, Wave, AudioDevice );
		}
	}
	else if( ( Wave->Duration > ( AudioDevice->MinOggVorbisDuration * Wave->NumChannels ) ) )
	{
		// Always create a new buffer for streaming ogg vorbis data
		Buffer = CreateQueuedBuffer( Wave, AudioDevice );
	}
	else
	{
		// Decompress ogg vorbis to wav and upload to OpenAL
		if( Wave->ResourceID )
		{
			Buffer = AudioDevice->WaveBufferMap.FindRef( Wave->ResourceID );
		}
		
		if( Buffer == NULL )
		{
			Buffer = CreateNativeBuffer( Wave, AudioDevice );
		}
	}

	return( Buffer );
}

// end
