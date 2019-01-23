/*=============================================================================
	UnAudio.cpp: Unreal base audio.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h" 
#include "EngineAudioDeviceClasses.h"
#include "EngineSoundNodeClasses.h"
#include "EngineSoundClasses.h"
#include "UnAudioEffect.h"
#include "UnSubtitleManager.h"

IMPLEMENT_CLASS(AAmbientSound);

/** Audio stats */
DECLARE_STATS_GROUP( TEXT( "Audio" ), STATGROUP_Audio );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Audio Components" ), STAT_AudioComponents, STATGROUP_Audio );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Audio Sources" ), STAT_AudioSources, STATGROUP_Audio );
DECLARE_DWORD_COUNTER_STAT( TEXT( "Wave Instances" ), STAT_WaveInstances, STATGROUP_Audio );
DECLARE_DWORD_COUNTER_STAT(TEXT( "Wave Instances Dropped" ), STAT_WavesDroppedDueToPriority, STATGROUP_Audio );
DECLARE_MEMORY_STAT( TEXT( "Audio Memory Used" ), STAT_AudioMemorySize, STATGROUP_Audio );
DECLARE_FLOAT_ACCUMULATOR_STAT( TEXT( "Audio Buffer Time" ), STAT_AudioBufferTime, STATGROUP_Audio );
DECLARE_FLOAT_ACCUMULATOR_STAT( TEXT( "Audio Buffer Time (w/ Channels)" ), STAT_AudioBufferTimeChannels, STATGROUP_Audio );

#if !CONSOLE
DECLARE_DWORD_COUNTER_STAT( TEXT( "CPU Decompressed Wave Instances" ), STAT_OggWaveInstances, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Decompress Vorbis" ), STAT_VorbisDecompressTime, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Prepare Audio Decompression" ), STAT_VorbisPrepareDecompressionTime, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Decompress Audio" ), STAT_AudioDecompressTime, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Prepare Vorbis Decompression" ), STAT_AudioPrepareDecompressionTime, STATGROUP_Audio );
#endif

DECLARE_CYCLE_STAT( TEXT( "Updating Effects" ), STAT_AudioUpdateEffects, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Updating Sources" ), STAT_AudioUpdateSources, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Buffer Creation" ), STAT_AudioResourceCreationTime, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Source Init" ), STAT_AudioSourceInitTime, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Processing Sources" ), STAT_AudioStartSources, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Gathering WaveInstances" ), STAT_AudioGatherWaveInstances, STATGROUP_Audio );
DECLARE_CYCLE_STAT( TEXT( "Audio Update Time" ), STAT_AudioUpdateTime, STATGROUP_Audio );

//@HACK!!!
FLOAT GGlobalAudioMultiplier = 1.f;

/*-----------------------------------------------------------------------------
	UAudioDevice implementation.
-----------------------------------------------------------------------------*/

// Helper function for "Sort" (higher priority sorts last).
IMPLEMENT_COMPARE_POINTER( FWaveInstance, UnAudio, { return ( B->PlayPriority - A->PlayPriority >= 0 ) ? -1 : 1; } )

void UAudioDevice::PostEditChange( UProperty* PropertyThatChanged )
{
	InitSoundGroups();
	ParseSoundGroups();
	Super::PostEditChange( PropertyThatChanged );
}

UBOOL UAudioDevice::Init( void )
{
	bGameWasTicking = TRUE;
	bTestLowPassFilter = FALSE;
	bTestEQFilter = FALSE;
	CurrentTick = 0;
	TextToSpeech = NULL;
	
	// Make sure the Listeners array has at least one entry, so we don't have to check for Listeners.Num() == 0 all the time
	Listeners.AddZeroed( 1 );

	// Parses sound groups.
	InitSoundGroups();
	ParseSoundGroups();

	// Iterate over all already loaded sounds and precache them. This relies on Super::Init in derived classes
	// to be called last.
	if( !GIsEditor )
	{
		for( TObjectIterator<USoundNodeWave> It; It; ++It )
		{
			USoundNodeWave* SoundNodeWave = *It;
			Precache( SoundNodeWave );
		}
	}

#if WITH_TTS
	// Always init TTS, even if it's only a stub
	TextToSpeech = new FTextToSpeech();
	TextToSpeech->Init();
#endif

	debugf( NAME_Init, TEXT( "%s initialized." ), *GetClass()->GetName() );

	return( TRUE );
}

/** 
 * Lists a summary of loaded sound collated by group
 */
struct FAudioGroupInfo 
{
	INT		NumResident;
	INT		SizeResident;
	INT		NumRealTime;
	INT		SizeRealTime;
};

void UAudioDevice::ListSoundGroups( FOutputDevice& Ar ) 
{
	// Iterate over all sound cues to get a unique map of sound node waves to group names
	TMap<USoundNodeWave*, FName> SoundNodeWaveGroups;

	for( TObjectIterator<USoundCue> CueIt; CueIt; ++CueIt )
	{
		TArray<USoundNodeWave*> Waves;

		USoundCue* SoundCue = *CueIt;
		SoundCue->RecursiveFindWaves( SoundCue->FirstNode, Waves );

		for( INT WaveIndex = 0; WaveIndex < Waves.Num(); ++WaveIndex )
		{
			// Presume one group per sound node wave
			SoundNodeWaveGroups.Set( Waves( WaveIndex ), SoundCue->SoundGroup ); 
		}
	}

	// Add any sound node waves that are not referenced by sound cues
	for( TObjectIterator<USoundNodeWave> WaveIt; WaveIt; ++WaveIt )
	{
		USoundNodeWave* SoundNodeWave = *WaveIt;
		if( SoundNodeWaveGroups.Find( SoundNodeWave ) == NULL )
		{
			SoundNodeWaveGroups.Set( SoundNodeWave, FName( TEXT( "Ungrouped" ) ) );
		}
	}

	// Collate the data into something useful
	TMap<FName, FAudioGroupInfo> AudioGroupInfos;

	for( TMap<USoundNodeWave*, FName>::TIterator MapIter( SoundNodeWaveGroups ); MapIter; ++MapIter )
	{
		USoundNodeWave* SoundNodeWave = MapIter.Key();
		FName GroupName = MapIter.Value();

		FAudioGroupInfo* AudioGroupInfo = AudioGroupInfos.Find( GroupName );
		if( AudioGroupInfo == NULL )
		{
			FAudioGroupInfo NewAudioGroupInfo = { 0 };
			AudioGroupInfos.Set( GroupName, NewAudioGroupInfo );
	
			AudioGroupInfo = AudioGroupInfos.Find( GroupName );
			check( AudioGroupInfo );
		}

		if( SoundNodeWave->ResourceID )
		{
			AudioGroupInfo->SizeResident += SoundNodeWave->SampleDataSize / 1024;
			AudioGroupInfo->NumResident++;
		}
		else if( SoundNodeWave->ResourceSize )
		{
			AudioGroupInfo->SizeRealTime += SoundNodeWave->ResourceSize / 1024;
			AudioGroupInfo->NumRealTime++;
		}
	}

	// Display the collated data
	for( TMap<FName, FAudioGroupInfo>::TIterator AGIIter( AudioGroupInfos ); AGIIter; ++AGIIter )
	{
		FName GroupName = AGIIter.Key();
		FAudioGroupInfo* AGI = AudioGroupInfos.Find( GroupName );

		FString Line = FString::Printf( TEXT( "Group '%s' has %d resident sounds taking %dkb" ), GroupName.GetName(), AGI->NumResident, AGI->SizeResident );
		if( AGI->NumRealTime > 0 )
		{
			Line += FString::Printf( TEXT( ", and %d real time sounds taking %dkb " ), AGI->NumRealTime, AGI->SizeRealTime );
		}

		Ar.Logf( *Line );
	}
}

/**
 * Exec handler used to parse console commands.
 *
 * @param	Cmd		Command to parse
 * @param	Ar		Output device to use in case the handler prints anything
 * @return	TRUE if command was handled, FALSE otherwise
 */
UBOOL UAudioDevice::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	if( ParseCommand( &Cmd, TEXT( "ListSounds" ) ) )
	{
		ListSounds( Ar );
		ListSoundGroups( Ar );
		return( TRUE );
	}
	else if( ParseCommand( &Cmd, TEXT( "ListSoundDurations" ) ) )
	{
		debugf( TEXT( ",Sound,Duration,Channels" ) );
		for( TObjectIterator<USoundNodeWave> It; It; ++It )
		{
			USoundNodeWave* SoundNodeWave = *It;
			debugf( TEXT( ",%s,%f,%i" ), *SoundNodeWave->GetPathName(), SoundNodeWave->Duration, SoundNodeWave->NumChannels );
		}
		return( TRUE );
	}
	else if( ParseCommand( &Cmd, TEXT( "TestLowPassFilter" ) ) )
	{
		if( Effects )
		{
			Ar.Logf( TEXT( "Enabling low pass filter for all sources" ) );
			bTestLowPassFilter = TRUE;
		}
		return( TRUE );
	}
	else if( ParseCommand( &Cmd, TEXT( "TestEQFilter" ) ) )
	{
		if( Effects )
		{
			Ar.Logf( TEXT( "Enabling EQ filter for all sources" ) );
			bTestEQFilter = TRUE;
		}
		return( TRUE );
	}
	else if( ParseCommand( &Cmd, TEXT( "SetSoundMode" ) ) )
	{
		if( Effects )
		{
			ESoundMode SoundMode = ( ESoundMode )Clamp<INT>( appAtoi( Cmd ), SOUNDMODE_NORMAL, SOUNDMODE_MAX );
			// Ar.Logf( TEXT( "Setting sound mode %d" ), ( INT )SoundMode );
			SetSoundMode( SoundMode );
		}
		return( TRUE );
	}
	else if( ParseCommand( &Cmd, TEXT( "ResetSoundState" ) ) )
	{
		if( Effects )
		{
			Ar.Logf( TEXT( "All volumes reset to their defaults; all test filters removed" ) );
			Effects->SetMixDebugState( DEBUGSTATE_None );
			bTestLowPassFilter = FALSE;
			bTestEQFilter = FALSE;
		}
		return( TRUE );
	}
	else if( ParseCommand( &Cmd, TEXT( "IsolateDryAudio" ) ) )
	{
		if( Effects )
		{
			Ar.Logf( TEXT( "Dry audio isolated" ) );
			Effects->SetMixDebugState( DEBUGSTATE_IsolateDryAudio );
		}
		return( TRUE );
	}
	else if( ParseCommand( &Cmd, TEXT( "IsolateReverb" ) ) )
	{
		if( Effects )
		{
			Ar.Logf( TEXT( "Reverb audio isolated" ) );
			Effects->SetMixDebugState( DEBUGSTATE_IsolateReverb );
		}
		return( TRUE );
	}
	else if( ParseCommand( &Cmd, TEXT( "IsolateEQ" ) ) )
	{
		if( Effects )
		{
			Ar.Logf( TEXT( "EQ audio isolated" ) );
			Effects->SetMixDebugState( DEBUGSTATE_IsolateEQ );
		}
		return( TRUE );
	}

	return( FALSE );
}

/**
 * Set up the sound group hierarchy
 */
void UAudioDevice::InitSoundGroups( void )
{
	// Empty cached state.
	NameToSoundGroupIndexMap.Empty();

	// Parse sound groups into TMaps, filling the propagated version with the base values.
	for( INT GroupIndex = 0; GroupIndex < SoundGroups.Num(); GroupIndex++ )
	{
		FSoundGroup& SoundGroup = SoundGroups( GroupIndex );
		//@warning: this relies on SoundGroups to not be modified without reparsing
		NameToSoundGroupIndexMap.Set( SoundGroup.GroupName, GroupIndex ); 
	}

	SourceSoundGroups = SoundGroups;
	CurrentSoundGroups = SoundGroups;
	DestinationSoundGroups = SoundGroups;
}

/**
 * Internal helper function used by ParseSoundGroups to traverse the tree.
 *
 * @param CurrentGroup			Subtree to deal with
 * @param ParentProperties		Propagated properties of parent node
 */
void UAudioDevice::RecurseIntoSoundGroups( FSoundGroup* CurrentGroup, FSoundGroupProperties* ParentProperties )
{
	// Iterate over all child nodes and recurse.
	for( INT ChildIndex = 0; ChildIndex < CurrentGroup->ChildGroupNames.Num(); ChildIndex++ )
	{
		// Look up group and propagated properties.
		FName ChildGroupName = CurrentGroup->ChildGroupNames( ChildIndex );
		INT GroupIndex = NameToSoundGroupIndexMap.FindRef( ChildGroupName );
		FSoundGroup* ChildGroup = &DestinationSoundGroups( GroupIndex );
		FSoundGroupProperties* PropagatedChildGroupProperties = &ChildGroup->Properties;
		
		// Should never be NULL for a properly set up tree.
		if( ChildGroup && PropagatedChildGroupProperties )
		{
			// Propagate parent values...
			PropagatedChildGroupProperties->Volume *= ParentProperties->Volume;
			PropagatedChildGroupProperties->bIsUISound |= ParentProperties->bIsUISound;
			PropagatedChildGroupProperties->bIsMusic |= ParentProperties->bIsMusic;

			// Not all values propagate equally...
			// VoiceCenterChannelVolume, VoiceRadioVolume, bApplyEffects, and bNoReverb do not propagate (sub-groups can be non-zero even if parent group is zero)

			// ... and recurse into child nodes.
			RecurseIntoSoundGroups( ChildGroup, PropagatedChildGroupProperties );
		}
		else
		{
			debugfSuppressed( NAME_DevAudio, TEXT( "Couldn't find child group %s - sound group functionality will not work correctly!" ), *ChildGroupName.ToString() );
		}
	}
}

/**
 * Parses the sound groups and propagates multiplicative properties down the tree.
 */
void UAudioDevice::ParseSoundGroups( void )
{
	// Reset to known state - preadjusted by set group volume calls
	DestinationSoundGroups = SoundGroups;

	// We are bound to have a Master volume.
	FName MasterGroupName = FName( TEXT( "Master" ) );
	INT GroupIndex = NameToSoundGroupIndexMap.FindRef( MasterGroupName );
	FSoundGroup* MasterGroup = &DestinationSoundGroups( GroupIndex );
	if( MasterGroup )
	{
		// Find the propagated master group properties as the root for tree traversal.
		FSoundGroupProperties* PropagatedMasterGroupProperties = &MasterGroup->Properties;
		check( PropagatedMasterGroupProperties );

		// Follow the tree.
		RecurseIntoSoundGroups( MasterGroup, PropagatedMasterGroupProperties );
	}
	else
	{
		debugfSuppressed( NAME_DevAudio, TEXT( "Couldn't find Master sound group! This group is required for the system to function!" ) );
	}
}

/** 
 * Handle mode setting with respect to sound group volumes
 */
void UAudioDevice::SetSoundGroupMode( FModeSettings& ModeSettings )
{
	if( ModeSettings.Mode != CurrentMode )
	{
		debugfSuppressed( NAME_DevAudio, TEXT( "UAudioDevice::SetModeSettings(): Old - %i  New - %i:%f" ),
			( INT )CurrentMode, ModeSettings.Mode, ModeSettings.FadeTime );

		SoundModeStartTime = appSeconds();

		// Copy the current sound group state
		SourceSoundGroups = CurrentSoundGroups;

		SoundModeEndTime = SoundModeStartTime + ModeSettings.FadeTime;
		CurrentMode = ModeSettings.Mode;

		ParseSoundGroups();

		if( CurrentMode != SOUNDMODE_NORMAL && CurrentMode < SoundGroupEffects.Num() )
		{
			// Adjust the sound group properties non recursively
			TArray<FSoundGroupAdjuster>& Adjusters = SoundGroupEffects( CurrentMode ).GroupEffect;
			for( INT i = 0; i < Adjusters.Num(); i++ )
			{
				INT GroupIndex = NameToSoundGroupIndexMap.FindRef( Adjusters( i ).GroupName );
				FSoundGroup* GroupToAdjust = &DestinationSoundGroups( GroupIndex );
				GroupToAdjust->Properties.Volume *= Adjusters( i ).VolumeAdjuster;
			}
		}
	}
}

/** 
 * Interpolate the data in sound groups
 */
void FSoundGroupProperties::Interpolate( FLOAT InterpValue, FSoundGroupProperties& Start, FSoundGroupProperties& End )
{
	FLOAT InvInterpValue = 1.0f - InterpValue;

	Volume = ( Start.Volume * InvInterpValue ) + ( End.Volume * InterpValue );
	VoiceCenterChannelVolume = ( Start.VoiceCenterChannelVolume * InvInterpValue ) + ( End.VoiceCenterChannelVolume * InterpValue );
	VoiceRadioVolume = ( Start.VoiceRadioVolume * InvInterpValue ) + ( End.VoiceRadioVolume * InterpValue );

	// Don't interpolate the on/off toggles - e.g. bNoReverb
}

/** 
 * Gets the parameters for EQ based on settings and time
 */
void UAudioDevice::Interpolate( FLOAT InterpValue, FSoundGroupProperties& Current, FSoundGroupProperties& Start, FSoundGroupProperties& End )
{
	if( InterpValue > 1.0f )
	{
		Current = End;
		return;
	}

	if( InterpValue < 0.0f )
	{
		Current = Start;
		return;
	}

	Current.Interpolate( InterpValue, Start, End );
}

/** 
 * Construct the CurrentSoundGroupProperties map
 *
 * This contains the original sound group properties propagated properly, and all adjustments due to the sound mode
 */
void UAudioDevice::GetCurrentSoundGroupState( void )
{
	FLOAT InterpValue = 1.0f;
	if( SoundModeEndTime - SoundModeStartTime > 0.0 )
	{
		InterpValue = ( FLOAT )( ( appSeconds() - SoundModeEndTime ) / ( SoundModeEndTime - SoundModeStartTime ) );
	}

	for( INT i = 0; i < CurrentSoundGroups.Num(); i++ )
	{
		Interpolate( InterpValue, CurrentSoundGroups( i ).Properties, SourceSoundGroups( i ).Properties, DestinationSoundGroups( i ).Properties );
	}
}

/** 
 * UObject functions
 */
void UAudioDevice::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	if( Ar.IsCountingMemory() )
	{
		Sources.CountBytes( Ar );
		FreeSources.CountBytes( Ar );
		WaveInstanceSourceMap.CountBytes( Ar );
		Ar.CountBytes( sizeof( FWaveInstance ) * WaveInstanceSourceMap.Num(), sizeof( FWaveInstance ) * WaveInstanceSourceMap.Num() );
		NameToSoundGroupIndexMap.CountBytes( Ar );
		SourceSoundGroups.CountBytes( Ar );
		CurrentSoundGroups.CountBytes( Ar );
		DestinationSoundGroups.CountBytes( Ar );
		SoundGroups.CountBytes( Ar );
	}
}

/** 
 * Complete the destruction of this class
 */
void UAudioDevice::FinishDestroy( void )
{
#if WITH_TTS
	if( TextToSpeech )
	{
		delete TextToSpeech;
		TextToSpeech = NULL;
	}
#endif

	Super::FinishDestroy();
}

void UAudioDevice::SetListener( INT ViewportIndex, const FVector& Location, const FVector& Up, const FVector& Right, const FVector& Front )
{
	if( Listeners.Num() <= ViewportIndex )
	{
		Listeners.Add( ViewportIndex - Listeners.Num() + 1 );
	}
	Listeners( ViewportIndex ).Location = Location;
	Listeners( ViewportIndex ).Up = Up;
	Listeners( ViewportIndex ).Right = Right;
	Listeners( ViewportIndex ).Front = Front;
}

/** 
 * Set up the sound EQ effect
 */
void UAudioDevice::SetSoundMode( ESoundMode NewSoundMode )
{
	FModeSettings ModeSettings = { NewSoundMode, 0.1f };

	if( Effects )
	{
		Effects->SetModeSettings( ModeSettings );
	}

	// Modify the sound group volumes for this mode (handled separately to EQ as it doesn't require audio effects to work)
	SetSoundGroupMode( ModeSettings );
}

/**
 * Pushes the specified reverb settings onto the reverb settings stack.
 *
 * @param	ReverbSettings		The reverb settings to use.
 */
void UAudioDevice::SetReverbSettings( const FReverbSettings& ReverbSettings )
{
	if( Effects )
	{
		Effects->SetReverbSettings( ReverbSettings );
	}
}

/** 
 * Platform dependent call to init effect data on a sound source
 */
void* UAudioDevice::InitEffect( FSoundSource* Source )
{
	if( Effects )
	{
		return( Effects->InitEffect( Source ) );
	}

	return( NULL );
}

/** 
 * Platform dependent call to update the sound output with new parameters
 */
void* UAudioDevice::UpdateEffect( FSoundSource* Source )
{
	if( Effects )
	{
		SCOPE_CYCLE_COUNTER( STAT_AudioUpdateEffects );

		return( Effects->UpdateEffect( Source ) );
	}

	return( NULL );
}

/** 
 * Handle pausing/unpausing of sources when entering or leaving pause mode
 */
void UAudioDevice::HandlePause( UBOOL bGameTicking )
{
	if( !GIsEditor )
	{
		// Pause all sounds if transitioning to pause mode.
		if( !bGameTicking && bGameWasTicking )
		{
			for( INT i = 0; i < Sources.Num(); i++ )
			{
				FSoundSource* Source = Sources( i );
				if( Source->IsGameOnly() )
				{
					Source->Pause();
				}
			}
		}
		// Unpause all sounds if transitioning back to game.
		else if( bGameTicking && !bGameWasTicking )
		{
			for( INT i = 0; i < Sources.Num(); i++ )
			{
				FSoundSource* Source = Sources( i );
				if( Source->IsGameOnly() )
				{
					Source->Play();
				}
			}
		}
	}
	else
	{
		// Stop all sounds if transitioning out of realtime.
		if( !bGameTicking && bGameWasTicking )
		{
			for( INT i = 0; i < Sources.Num(); i++ )
			{
				FSoundSource* Source = Sources( i );
				if( Source->IsGameOnly() )
				{
					Source->Stop();
				}
			}
		}
	}

	bGameWasTicking = bGameTicking;
}

/** 
 * Iterate over the active AudioComponents for wave instances that could be playing.
 * 
 * @return Index of first wave instance that can have a source attached
 */
INT UAudioDevice::GetSortedActiveWaveInstances( TArray<FWaveInstance*>& WaveInstances, UBOOL bGameTicking )
{
	SCOPE_CYCLE_COUNTER( STAT_AudioGatherWaveInstances );

	static DOUBLE LastTime;

	const DOUBLE CurrentTime = appSeconds();
	const FLOAT DeltaTime = CurrentTime - LastTime;
	LastTime = CurrentTime;

	// Check and exit early if the delta time is too large to be reasonable
	if( DeltaTime > 1.0f )
	{
		return( 0 );
	}

	// Update the portal volumes
	for( INT i = 0; i < Listeners.Num(); i++ )
	{
		Listeners( i ).PortalVolume = GWorld->GetWorldInfo()->GetPortalVolume( Listeners( i ).Location );
	}

	// Tick all the active audio components
	for( INT i = AudioComponents.Num() - 1; i >= 0; i-- )
	{
		UAudioComponent* AudioComponent = AudioComponents( i );

		if( !AudioComponent )
		{
			// Remove empty slot.
			AudioComponents.Remove( i );
		}
		else if( !AudioComponent->SoundCue )
		{
			// No sound cue - cleanup and remove
			AudioComponent->Stop();
		}
		// If the component's scene allows audio - tick wave instances.
		else if( !AudioComponent->GetScene() || AudioComponent->GetScene()->AllowAudioPlayback() )
		{
			FLOAT Duration = AudioComponent->SoundCue->GetCueDuration();
			if( Duration != 0.0f && AudioComponent->PlaybackTime > Duration )
			{
				AudioComponent->Stop();
			}
			else
			{
				// If not in game, do not advance sounds unless they are UI sounds.
				FLOAT UsedDeltaTime = DeltaTime;
				if( !bGameTicking && !AudioComponent->bIsUISound )
				{
					UsedDeltaTime = 0.0f;
				}

				// UpdateWaveInstances might cause AudioComponent to remove itself from AudioComponents TArray which is why why iterate in reverse order!
				AudioComponent->UpdateWaveInstances( this, WaveInstances, Listeners, UsedDeltaTime );
			}
		}
	}

	// Sort by priority (lowest priority first).
	Sort<USE_COMPARE_POINTER( FWaveInstance, UnAudio )>( &WaveInstances( 0 ), WaveInstances.Num() );

	// Return the first audible waveinstance
	INT FirstActiveIndex = Max( WaveInstances.Num() - MaxChannels, 0 );
	for( ; FirstActiveIndex < WaveInstances.Num(); FirstActiveIndex++ )
	{
		if( WaveInstances( FirstActiveIndex )->Volume > KINDA_SMALL_NUMBER )
		{
			break;
		}
	}
	return( FirstActiveIndex );
}

/** 
 * Stop sources that need to be stopped, and touch the ones that need to be kept alive
 * Stop sounds that are too low in priority to be played
 */
void UAudioDevice::StopSources( TArray<FWaveInstance*>& WaveInstances, INT FirstActiveIndex )
{
	// Touch sources that are high enough priority to play
	for( INT InstanceIndex = FirstActiveIndex; InstanceIndex < WaveInstances.Num(); InstanceIndex++ )
	{
		FWaveInstance* WaveInstance = WaveInstances( InstanceIndex );
		FSoundSource* Source = WaveInstanceSourceMap.FindRef( WaveInstance );
		if( Source )
		{
			Source->LastUpdate = CurrentTick;

			// If they are still audible, mark them as such
			if( WaveInstance->Volume > KINDA_SMALL_NUMBER )
			{
				Source->LastHeardUpdate = CurrentTick;
			}
		}
	}

	// Stop inactive sources, sources that no longer have a WaveInstance associated 
	// or sources that need to be reset because Stop & Play were called in the same frame.
	for( INT SourceIndex = 0; SourceIndex < Sources.Num(); SourceIndex++ )
	{
		FSoundSource* Source = Sources( SourceIndex );

#if STATS && !CONSOLE
		if( Source->UsesCPUDecompression() )
		{
			INC_DWORD_STAT( STAT_OggWaveInstances );
		}
#endif
		// Source was not high enough priority to play this tick
		if( Source->LastUpdate != CurrentTick )
		{
			Source->Stop();
		}

		// Source has been inaudible for several ticks
		if( Source->LastHeardUpdate + AUDIOSOURCE_TICK_LONGEVITY < CurrentTick )
		{
			Source->Stop();
		}

		if( Source->WaveInstance && Source->WaveInstance->bIsRequestingRestart )
		{
			Source->Stop();
		}
	}

	// Stop wave instances that are no longer playing due to priority reasons. This needs to happen AFTER
	// stopping sources as calling Stop on a sound source in turn notifies the wave instance of a buffer
	// being finished which might reset it being finished.
	for( INT InstanceIndex = 0; InstanceIndex < FirstActiveIndex; InstanceIndex++ )
	{
		FWaveInstance* WaveInstance = WaveInstances( InstanceIndex );	
		WaveInstance->StopWithoutNotification();
		//debugf( NAME_DevAudio, TEXT( "SoundStoppedWithoutNotification due to priority reasons: %s" ), *WaveInstance->WaveData->GetName() );
	}
}

/** 
 * Start and/or update any sources that have a high enough priority to play
 */
void UAudioDevice::StartSources( TArray<FWaveInstance*>& WaveInstances, INT FirstActiveIndex, UBOOL bGameTicking )
{
	SCOPE_CYCLE_COUNTER( STAT_AudioStartSources );

	// Start sources as needed.
	for( INT InstanceIndex = FirstActiveIndex; InstanceIndex < WaveInstances.Num(); InstanceIndex++ )
	{
		FWaveInstance* WaveInstance = WaveInstances( InstanceIndex );

		// Editor uses bIsUISound for sounds played in the browser.
		if(	bGameTicking || WaveInstance->AudioComponent->bIsUISound )
		{
			FSoundSource* Source = WaveInstanceSourceMap.FindRef( WaveInstance );
			if( !Source )
			{
				check( FreeSources.Num() );
				Source = FreeSources.Pop();
				check( Source);

				// Try to initialize source.
				if( Source->Init( WaveInstance ) )
				{
					// Associate wave instance with it which is used earlier in this function.
					WaveInstanceSourceMap.Set( WaveInstance, Source );
					// Playback might be deferred till the end of the update function on certain implementations.
					Source->Play();
					//debugf( NAME_DevAudio, TEXT( "Playing: %s" ), *WaveInstance->WaveData->GetName() );
				}
				else
				{
					// This can happen if e.g. the USoundNodeWave pointed to by the WaveInstance is not a valid sound file.
					// If we don't stop the wave file, it will continue to try initializing the file every frame, which is a perf hit
					WaveInstance->StopWithoutNotification();
					FreeSources.AddItem( Source );
				}
			}
			else
			{
				Source->Update();
			}
		}
	}
}

/** 
 * The audio system's main "Tick" function
 */
void UAudioDevice::Update( UBOOL bGameTicking )
{
	SCOPE_CYCLE_COUNTER( STAT_AudioUpdateTime );

	// Start a new frame
	CurrentTick++;

	// Handle pause/unpause for the game and editor.
	HandlePause( bGameTicking );

	// Update the audio effects - reverb, EQ etc
	if( Effects )
	{
		Effects->Update();
	}

	// Gets the current state of the sound groups accounting for sound mode
	GetCurrentSoundGroupState();

	// Kill any sources that have finished
	for( INT SourceIndex = 0; SourceIndex < Sources.Num(); SourceIndex++ )
	{
		// Source has finished playing (it's one shot)
		if( Sources( SourceIndex )->IsFinished() )
		{
			Sources( SourceIndex )->Stop();
		}
	}

	// Poll audio components for active wave instances (== paths in node tree that end in a USoundNodeWave)
	TArray<FWaveInstance*> WaveInstances;
	INT FirstActiveIndex = GetSortedActiveWaveInstances( WaveInstances, bGameTicking );

	// Stop sources that need to be stopped, and touch the ones that need to be kept alive
	StopSources( WaveInstances, FirstActiveIndex );

	// Start and/or update any sources that have a high enough priority to play
	StartSources( WaveInstances, FirstActiveIndex, bGameTicking );

	INC_DWORD_STAT_BY( STAT_WaveInstances, WaveInstances.Num() );
	INC_DWORD_STAT_BY( STAT_AudioSources, MaxChannels - FreeSources.Num() );
	INC_DWORD_STAT_BY( STAT_WavesDroppedDueToPriority, Max( WaveInstances.Num() - MaxChannels, 0 ) );
	INC_DWORD_STAT_BY( STAT_AudioComponents, AudioComponents.Num() );
}

/**
 * Stops all game (and possibly UI) sounds
 *
 * @param bShouldStopUISounds If TRUE, this function will stop UI sounds as well
 */
void UAudioDevice::StopAllSounds( UBOOL bShouldStopUISounds )
{
	// go over all sound sources
	for( INT i = 0; i < Sources.Num(); i++ )
	{
		FSoundSource* Source = Sources( i );

		// stop game sounds, and UI also if desired
		if( Source->IsGameOnly() || bShouldStopUISounds )
		{
			// Stop audio component first.
			UAudioComponent* AudioComponent = Source->WaveInstance ? Source->WaveInstance->AudioComponent : NULL;
			if( AudioComponent )
			{
				AudioComponent->Stop();
			}

			// Stop source.
			Source->Stop();
		}
	}
}

void UAudioDevice::AddComponent( UAudioComponent* AudioComponent )
{
	check( AudioComponent );
	AudioComponents.AddUniqueItem( AudioComponent );
}

void UAudioDevice::RemoveComponent( UAudioComponent* AudioComponent )
{
	check( AudioComponent );

	for( INT i = 0; i < AudioComponent->WaveInstances.Num(); i++ )
	{	
		FWaveInstance* WaveInstance = AudioComponent->WaveInstances( i );

		// Stop the owning sound source
		FSoundSource* Source = WaveInstanceSourceMap.FindRef( WaveInstance );
		if( Source )
		{
			Source->Stop();
		}

		// Free up the actual buffer data if requested
		if( WaveInstance->WaveData->bOneTimeUse )
		{
			WaveInstance->WaveData->FreeResources();
		}
	}

	AudioComponents.RemoveItem( AudioComponent );
}

void UAudioDevice::SetGroupVolume( FName GroupName, FLOAT Volume )
{
	INT GroupIndex = NameToSoundGroupIndexMap.FindRef( GroupName );
	// Set the volume in the original sound group
	FSoundGroup* Group = &SoundGroups( GroupIndex );
	if( Group )
	{
		// Find the propagated master group properties as the root for tree traversal.
		Group->Properties.Volume = Volume;
		ParseSoundGroups();
	}
	else
	{
		debugfSuppressed( NAME_DevAudio, TEXT( "Couldn't find specified sound group in UAudioDevice::SetGroupVolume!" ) );
	}
}

UBOOL UAudioDevice::LocationIsAudible( FVector Location, FLOAT MaxDistance )
{
	if( MaxDistance >= WORLD_MAX )
	{
		return( TRUE );
	}

	MaxDistance *= MaxDistance;
	for( INT i = 0; i < Listeners.Num(); i++ )
	{
		if( ( Listeners( i ).Location - Location ).SizeSquared() < MaxDistance )
		{
			return( TRUE );
		}
	}

	return( FALSE );
}


UAudioComponent* UAudioDevice::CreateComponent( USoundCue* SoundCue, FSceneInterface* Scene, AActor* Actor, UBOOL bPlay, UBOOL bStopWhenOwnerDestroyed, FVector* Location )
{
	UAudioComponent* AudioComponent = NULL;

	if( SoundCue 
	&&	( !SoundCue->MaxConcurrentPlayCount || ( SoundCue->CurrentPlayCount < SoundCue->MaxConcurrentPlayCount ) ) 
	&&	GEngine 
	&&	GEngine->bUseSound )
	{
		// Avoid creating component if we're trying to play a sound on an already destroyed actor.
		if( Actor && Actor->ActorIsPendingKill() )
		{
			// Don't create component on destroyed actor.
		}
		// Either no actor or actor is still alive.
		else if( !SoundCue->IsAudibleSimple( Location ) )
		{
			// Don't create a sound component for short sounds that start out of range of any listener
			debugfSuppressed( NAME_DevAudio, TEXT( "AudioComponent not created for out of range SoundCue %s" ), *SoundCue->GetName() );
		}
		else
		{
			// Use actor as outer if we have one.
			if( Actor )
			{
				AudioComponent = ConstructObject<UAudioComponent>( UAudioComponent::StaticClass(), Actor );
			}
			// Let engine pick the outer (transient package).
			else
			{
				AudioComponent = ConstructObject<UAudioComponent>( UAudioComponent::StaticClass() );
			}
			
			AudioComponent->SoundCue = SoundCue;
			AudioComponent->bUseOwnerLocation = Actor ? 1 : 0;
			AudioComponent->bAutoPlay = 0;
			AudioComponent->bAutoDestroy = bPlay;
			AudioComponent->bStopWhenOwnerDestroyed = bStopWhenOwnerDestroyed;

			if( Actor )
			{
				// AActor::UpdateComponents calls this as well though we need an initial location as we manually create the component.
				AudioComponent->ConditionalAttach( Scene, Actor, Actor->LocalToWorld() );
				// Add this audio component to the actor's components array.
				Actor->Components.AddItem( AudioComponent );
			}
			else
			{
				AudioComponent->ConditionalAttach( Scene, NULL, FMatrix::Identity );
			}

			if( bPlay )
			{
				AudioComponent->Play();
			}
		}
	}
	
	return( AudioComponent );
}

/** 
 * Stop all the audio components and sources
 */
void UAudioDevice::Flush( void )
{
	// Stop all audio components attached to the scene
	for( INT ComponentIndex = 0; ComponentIndex < AudioComponents.Num(); ComponentIndex++ )
	{
		UAudioComponent* AudioComponent = AudioComponents( ComponentIndex );
		if( AudioComponent )
		{
			AudioComponent->Stop();
		}
	}
	AudioComponents.Empty();
	
	// Make sure sounds are fully stopped.
	for( INT i = 0; i < Sources.Num(); i++ )
	{
		Sources( i )->Stop();
	}

	WaveInstanceSourceMap.Empty();
}

/**
 * Returns the sound group properties associates with a sound group taking into account
 * the group tree.
 *
 * @param	SoundGroupName	name of sound group to retrieve properties from
 * @return	sound group properties if a sound group with name SoundGroupName exists, NULL otherwise
 */
FSoundGroupProperties* UAudioDevice::GetSoundGroupProperties( FName SoundGroupName )
{
	INT GroupIndex = NameToSoundGroupIndexMap.FindRef( SoundGroupName );
	return( &CurrentSoundGroups( GroupIndex ).Properties );
}

/**
 * Returns an array of existing sound group names.
 *
 * @return array of sound group names
 */
TArray<FName> UAudioDevice::GetSoundGroupNames( void )
{
	TArray<FName> SoundGroupNames;

	for( INT GroupIndex = 0; GroupIndex < SoundGroups.Num(); GroupIndex++ )
	{
		const FSoundGroup& SoundGroup = SoundGroups( GroupIndex );
		if( SoundGroup.GroupName != NAME_None )
		{
			SoundGroupNames.AddUniqueItem( SoundGroup.GroupName );
		}
	}

	return( SoundGroupNames );
}

IMPLEMENT_CLASS( UAudioDevice );

/*-----------------------------------------------------------------------------
	FSoundSource implementation.
-----------------------------------------------------------------------------*/

void FSoundSource::Stop( void )
{
	if( WaveInstance )
	{
		check( AudioDevice );
		AudioDevice->FreeSources.AddUniqueItem( this );
		AudioDevice->WaveInstanceSourceMap.Remove( WaveInstance );
		WaveInstance->NotifyFinished();
		WaveInstance->bIsRequestingRestart = FALSE;
		WaveInstance = NULL;
	}
	else
	{
		check( AudioDevice->FreeSources.FindItemIndex( this ) != INDEX_NONE );
	}
}

/**
 * Returns whether associated audio component is an ingame only component, aka one that will
 * not play unless we're in game mode (not paused in the UI)
 *
 * @return FALSE if associated component has bIsUISound set, TRUE otherwise
 */
UBOOL FSoundSource::IsGameOnly( void )
{
	if( WaveInstance
		&& WaveInstance->AudioComponent
		&& WaveInstance->AudioComponent->bIsUISound )
	{
		return( FALSE );
	}

	return( TRUE );
}

/*-----------------------------------------------------------------------------
	FWaveInstance implementation.
-----------------------------------------------------------------------------*/

/** Helper to create good unique type hashs for FWaveInstance instances */
DWORD FWaveInstance::TypeHashCounter = 0;

/**
 * Constructor, initializing all member variables.
 *
 * @param InAudioComponent	Audio component this wave instance belongs to.
 */
FWaveInstance::FWaveInstance( UAudioComponent* InAudioComponent )
:	WaveData( NULL )
,	NotifyBufferFinishedHook( NULL )
,	AudioComponent( InAudioComponent )
,	Volume( 0.0f )
,	PlayPriority( 0.0f )
,	VoiceCenterChannelVolume( 0.0f )
,	VoiceRadioVolume( 0.0f )
,	LoopingMode( LOOP_Never )
,	bApplyEffects( FALSE )
,	bAlwaysPlay( FALSE )
,	bIsUISound( FALSE )
,	bIsMusic( FALSE )
,	bNoReverb( FALSE )
,	bIsFinished( FALSE )
,	bAlreadyNotifiedHook( FALSE )
,	bUseSpatialization( FALSE )
,	bIsRequestingRestart( FALSE )
,	HighFrequencyGain( 1.0f )
,	Pitch( 0.0f )
,	Velocity( FVector( 0.0f, 0.0f, 0.0f ) )
,	Location( FVector( 0.0f, 0.0f, 0.0f ) )
{
	TypeHash = ++TypeHashCounter;
}

/**
 * Notifies the wave instance that it has finished.
 */
void FWaveInstance::NotifyFinished( void )
{
	if( !bAlreadyNotifiedHook )
	{
		// We are finished.
		bIsFinished = TRUE;

		// Avoid double notifications.
		bAlreadyNotifiedHook = TRUE;

		if( NotifyBufferFinishedHook && AudioComponent )
		{
			// Notify NotifyBufferFinishedHook that the current playback buffer has finished...		
			NotifyBufferFinishedHook->NotifyWaveInstanceFinished( this );
		}
	}
}

/**
 * Returns whether wave instance has finished playback.
 *
 * @return TRUE if finished, FALSE otherwise.
 */
UBOOL FWaveInstance::IsFinished( void )
{
	return( bIsFinished );
}

/**
 * Stops the wave instance without notifying NotifyWaveInstanceFinishedHook. This will NOT stop wave instance
 * if it is set up to loop indefinitely or set to remain active.
 */
void FWaveInstance::StopWithoutNotification( void )
{
	if( LoopingMode == LOOP_Forever || ( AudioComponent && AudioComponent->bShouldRemainActiveIfDropped ) )
	{
		// We don't finish if we're either indefinitely looping or the audio component explicitly mandates that we should 
		// remain active which is e.g. used for engine sounds and such.
		bIsFinished = FALSE;
	}
	else
	{
		// We're finished.
		bIsFinished = TRUE;
	}
}

FArchive& operator<<( FArchive& Ar, FWaveInstance* WaveInstance )
{
	if( !Ar.IsLoading() && !Ar.IsSaving() )
	{
		Ar << WaveInstance->WaveData << WaveInstance->NotifyBufferFinishedHook << WaveInstance->AudioComponent;
	}
	return( Ar );
}

/*-----------------------------------------------------------------------------
	FAudioComponentSavedState implementation.
-----------------------------------------------------------------------------*/

void FAudioComponentSavedState::Set( UAudioComponent* AudioComponent )
{
	CurrentNotifyBufferFinishedHook	= AudioComponent->CurrentNotifyBufferFinishedHook;
	CurrentLocation = AudioComponent->CurrentLocation;
	CurrentVolume = AudioComponent->CurrentVolume;
	CurrentPitch = AudioComponent->CurrentPitch;
	CurrentHighFrequencyGain = AudioComponent->CurrentHighFrequencyGain;
	CurrentUseSpatialization = AudioComponent->CurrentUseSpatialization;
	CurrentUseSeamlessLooping = AudioComponent->CurrentUseSeamlessLooping;
}

void FAudioComponentSavedState::Restore( UAudioComponent* AudioComponent )
{
	AudioComponent->CurrentNotifyBufferFinishedHook	= CurrentNotifyBufferFinishedHook;
	AudioComponent->CurrentLocation = CurrentLocation;
	AudioComponent->CurrentVolume = CurrentVolume;
	AudioComponent->CurrentPitch = CurrentPitch;
	AudioComponent->CurrentHighFrequencyGain = CurrentHighFrequencyGain;
	AudioComponent->CurrentUseSpatialization = CurrentUseSpatialization;
	AudioComponent->CurrentUseSeamlessLooping = CurrentUseSeamlessLooping;
}

void FAudioComponentSavedState::Reset( UAudioComponent* AudioComponent )
{
	AudioComponent->CurrentNotifyBufferFinishedHook	= NULL;
	AudioComponent->CurrentVolume = 1.0f,
	AudioComponent->CurrentPitch = 1.0f;
	AudioComponent->CurrentHighFrequencyGain = 1.0f;
	AudioComponent->CurrentUseSpatialization = 0;
	AudioComponent->CurrentLocation = AudioComponent->bUseOwnerLocation ? AudioComponent->ComponentLocation : AudioComponent->Location;
	AudioComponent->CurrentUseSeamlessLooping = FALSE;
}

/*-----------------------------------------------------------------------------
	UAudioComponent implementation.
-----------------------------------------------------------------------------*/

void UAudioComponent::Attach( void )
{
	Super::Attach();
	if( bAutoPlay )
	{
		Play();
	}
}

void UAudioComponent::FinishDestroy( void )
{
	// Delete wave instances and remove from audio device.
	Cleanup();

	// Route Destroy event.
	Super::FinishDestroy();
}

void UAudioComponent::Detach( void )
{
	// Route Detach event.
	Super::Detach();

	// Don't stop audio and clean up component if owner has been destroyed (default behaviour). This function gets 
	// called from AActor::ClearComponents when an actor gets destroyed which is not usually what we want for one- 
	// shot sounds.
	if( !Owner || bStopWhenOwnerDestroyed )
	{		
		Cleanup();
	}
	else
	{
		// clear pending kill flag so GC doesn't kill off the sound early either
		ClearFlags( RF_PendingKill );
	}
}

/**
 * Dissociates component from audio device and deletes wave instances. Also causes sound to be stopped.
 */
void UAudioComponent::Cleanup( void )
{
	if( bWasPlaying && !GExitPurge )
	{
		// @see UAudioComponent::Stop()  we set this to null there.  so if we bypass stop and just call 
		// ::Cleanup (for things like emitters which are destroyed) we need to decrement CurrentPlayCount
		if( CueFirstNode && SoundCue )
		{		
			SoundCue->CurrentPlayCount = Max( SoundCue->CurrentPlayCount - 1, 0 );
		}

		// Removes component from the audio device's component list, implicitly also stopping the sound.
		UAudioDevice* AudioDevice = GEngine && GEngine->Client ? GEngine->Client->GetAudioDevice() : NULL;
		if( AudioDevice )
		{
			AudioDevice->RemoveComponent( this );
		}

		// Delete wave instances.
		for( INT InstanceIndex = 0; InstanceIndex < WaveInstances.Num(); InstanceIndex++ )
		{
			FWaveInstance* WaveInstance = WaveInstances( InstanceIndex );

			// Dequeue subtitles for this sounds
			FSubtitleManager::GetSubtitleManager()->KillSubtitles( ( PTRINT )WaveInstance );

			delete WaveInstance;
		}

		// Reset hook so it can be GCed if other code changes soundcue.
		CurrentNotifyBufferFinishedHook = NULL;

		// Don't clear instance parameters in the editor.
		// Necessary to support defining parameters via the property window, where PostEditChanges
		// routes component reattach, which would immediately clear the just-edited paramaters.
		if ( !GIsEditor || GIsPlayInEditorWorld )
		{
			InstanceParameters.Empty();
		}

		// Force re-initialization on next play.
		SoundNodeData.Empty();
		SoundNodeOffsetMap.Empty();
		SoundNodeResetWaveMap.Empty();
		WaveInstances.Empty();

		PlaybackTime = 0.0f;

		// Reset fade in variables
		FadeInStartTime = 0.0f;
		FadeInStopTime = -1.0f;
		FadeInTargetVolume = 1.0f;
		
		// Reset fade out variables
		FadeOutStartTime = 0.0f;	
		FadeOutStopTime = -1.0f;
		FadeOutTargetVolume = 1.0f;

		// Reset adjust variables
		AdjustVolumeStartTime = 0.0f;	
		AdjustVolumeStopTime = -1.0f;
		AdjustVolumeTargetVolume = 1.0f;
		CurrAdjustVolumeTargetVolume = 1.0f;

		// We cleaned up everything.
		bWasPlaying = FALSE;
	}
}

void UAudioComponent::SetParentToWorld( const FMatrix& ParentToWorld )
{
	Super::SetParentToWorld( ParentToWorld );
	ComponentLocation = ParentToWorld.GetOrigin();
}

void UAudioComponent::SetSoundCue( USoundCue* NewSoundCue )
{
	// We shouldn't really have any references to auto-destroy audiocomponents, and also its kind of undefined what changing the sound cue on
	// one with this flag set should do.
	check( !bAutoDestroy );

	Stop();
	SoundCue = NewSoundCue;
}

/** 
 * Start a sound cue playing on an audio component
 */
void UAudioComponent::Play( void )
{
	// Cache root node of sound container to avoid subsequent dereferencing.
	if( SoundCue )
	{	
		CueFirstNode = SoundCue->FirstNode;
	}

	// Reset variables if we were already playing.
	if( bWasPlaying )
	{
		for( INT InstanceIndex = 0; InstanceIndex < WaveInstances.Num(); InstanceIndex++ )
		{
			FWaveInstance* WaveInstance = WaveInstances( InstanceIndex );
			if( WaveInstance )
			{
				WaveInstance->bIsFinished = FALSE;
				WaveInstance->bIsRequestingRestart = TRUE;
			}
		}
	}
	// Increase the cue's current play count if we're starting up.
	else if( SoundCue )
	{
		SoundCue->CurrentPlayCount++;
	}

	PlaybackTime = 0.0f;
	bFinished = FALSE;
	bWasPlaying = TRUE;

	// Adds component from the audio device's component list.
	UAudioDevice* AudioDevice = GEngine && GEngine->Client ? GEngine->Client->GetAudioDevice() : NULL;
	if( AudioDevice )
	{
		AudioDevice->AddComponent( this );
	}
}

/** 
 * Stop an audio component playing its sound cue, issue any delegates if needed
 */
void UAudioComponent::Stop( void )
{
	// Decrease the cue's current play count on the first call to Stop.
	if( CueFirstNode && SoundCue )
	{
		SoundCue->CurrentPlayCount = Max( SoundCue->CurrentPlayCount - 1, 0 );
	}

	// For safety, clear out the cached root sound node pointer.
	CueFirstNode = NULL;

	// We're done.
	bFinished = TRUE;

	UBOOL bOldWasPlaying = bWasPlaying;

	// Clean up intermediate arrays which also dissociates from the audio device, hence stopping the sound.
	// This leaves the component in a state that is suited for having Play called on it again to restart
	// playback from the beginning.
	Cleanup();

	if( bOldWasPlaying && GWorld != NULL && DELEGATE_IS_SET( OnAudioFinished ) )
	{
		delegateOnAudioFinished( this );
	}

	// Auto destruction is handled via marking object for deletion.
	if( bAutoDestroy )
	{
		// Detach from potential owner.
		if( Owner )
		{
			DetachFromAny();
		}

		// Mark object for deletion and reference removal.
		MarkPendingKill();
	}
}

void UAudioComponent::PostEditChange( UProperty* PropertyThatChanged )
{
	// Reset variables.
	if( bWasPlaying )
	{
		for( INT InstanceIndex = 0; InstanceIndex < WaveInstances.Num(); InstanceIndex++ )
		{
			FWaveInstance* WaveInstance = WaveInstances( InstanceIndex );
			if( WaveInstance )
			{
				WaveInstance->bIsFinished = FALSE;
				WaveInstance->bIsRequestingRestart = TRUE;
			}
		}
	}

	PlaybackTime = 0.0f;
	bFinished = FALSE;

	// Clear node offset associations and data so dynamic data gets re-initialized.
	SoundNodeData.Empty();
	SoundNodeOffsetMap.Empty();

	Super::PostEditChange( PropertyThatChanged );
}

void UAudioComponent::CheckOcclusion( const FVector& ListenerLocation )
{
	if( OcclusionCheckInterval > 0.0f && GWorld->GetTimeSeconds() - LastOcclusionCheckTime > OcclusionCheckInterval && SoundCue->MaxAudibleDistance != WORLD_MAX )
	{
		LastOcclusionCheckTime = GWorld->GetTimeSeconds();
		FCheckResult Hit( 1.0f );
		UBOOL bNowOccluded = !GWorld->SingleLineCheck( Hit, GetOwner(), ListenerLocation, CurrentLocation, TRACE_World | TRACE_StopAtAnyHit );
		if( bNowOccluded != bWasOccluded )
		{
			bWasOccluded = bNowOccluded;
			eventOcclusionChanged( bNowOccluded );
		}
	}
}

void UAudioComponent::UpdateWaveInstances( UAudioDevice* AudioDevice, TArray<FWaveInstance*> &InWaveInstances, const TArray<FListener>& InListeners, FLOAT DeltaTime )
{
	check( AudioDevice );

	// Early outs.
	if( CueFirstNode == NULL || SoundCue == NULL )
	{	
		return;
	}

	//@todo audio: Need to handle pausing and not getting out of sync by using the mixer's time.
	//@todo audio: Fading in and out is also dependent on the DeltaTime
	PlaybackTime += DeltaTime;

	// Reset temporary variables used for node traversal.
	FAudioComponentSavedState::Reset( this );

	// splitscreen support:
	// we always pass the 'primary' listener (viewport 0) to the sound nodes and the underlying audio system
	// then move the AudioComponent's CurrentLocation so that its position relative to that Listener is the same as its real position is relative to the closest Listener
	Listener = &InListeners( 0 );
	const FListener* ClosestListener = Listener;

	FVector ModifiedLocation = GWorld->GetWorldInfo()->RemapLocationThroughPortals( CurrentLocation, Listener->Location );
	FVector ClosestLocation = ModifiedLocation;
	FLOAT ClosestDistSq = ( ModifiedLocation - Listener->Location ).SizeSquared();

	for( INT i = 1; i < InListeners.Num(); i++ )
	{
		const FListener* TestListener = &InListeners( i );

		ModifiedLocation = GWorld->GetWorldInfo()->RemapLocationThroughPortals( CurrentLocation, TestListener->Location );
		FLOAT DistSq = ( ModifiedLocation - TestListener->Location ).SizeSquared();
		if( DistSq < ClosestDistSq )
		{
			ClosestListener = TestListener;
			ClosestDistSq = DistSq;
			ClosestLocation = ModifiedLocation;
		}
	}

	//@note: we don't currently handle checking occlusion for sounds remapped through portals
	if( CurrentLocation == ClosestLocation )
	{
		CheckOcclusion( ClosestListener->Location );
	}

	CurrentLocation = ClosestLocation;
		
	// if the closest listener is not the primary one, transform CurrentLocation
	if( ClosestListener != Listener )
	{
		// get the world space delta between the sound source and the viewer
		FVector Delta = CurrentLocation - ClosestListener->Location;
		// transform Delta to the viewer's local space
		FVector ViewActorLocal = FRotationMatrix( ClosestListener->Front.Rotation() ).TransformFVector( Delta );
		// transform again to a world space delta to the 'real' listener
		FVector ListenerWorld = FInverseRotationMatrix( Listener->Front.Rotation() ).TransformFVector( ViewActorLocal );
		// add 'real' listener's location to get the final position to use
		CurrentLocation = ListenerWorld + Listener->Location;
	}

	// Default values.
	// It's all Multiplicative!  So now people are all modifying the multiplier values via various means 
	// (even after the Sound has started playing, and this line takes them all into account and gives us
	// final value that is correct
	CurrentVolumeMultiplier = VolumeMultiplier * SoundCue->VolumeMultiplier * GetFadeInMultiplier() * GetFadeOutMultiplier() * GetAdjustVolumeOnFlyMultiplier();
	CurrentPitchMultiplier = PitchMultiplier * SoundCue->PitchMultiplier;
/*
	// Useful debug block for catching ungrouped sound cues
	static const TCHAR* TempName = DebugFName( SoundCue->SoundGroup );
	if( wcscmp( TempName, TEXT("None") ) == 0 )
	{
		OutputDebugString( L"WARNING - Sound cue " );
		OutputDebugString( DebugFName( const_cast<FName&>( SoundCue->GetPureName() ) ) );
		OutputDebugString( L" is not associated with a sound group - This is a bug!\n" );
		check( false );
	}
*/
	// Set multipliers to allow propagation of sound group properties to wave instances.
	FSoundGroupProperties* SoundGroupProperties = AudioDevice->GetSoundGroupProperties( SoundCue->SoundGroup );
	if( SoundGroupProperties )
	{
		// Use values from "parsed/ propagated" sound group properties
		CurrentVolumeMultiplier *= SoundGroupProperties->Volume * GGlobalAudioMultiplier;

		// Not all values propagate multiplicatively
		CurrentVoiceCenterChannelVolume = SoundGroupProperties->VoiceCenterChannelVolume;
		CurrentVoiceRadioVolume = SoundGroupProperties->VoiceRadioVolume * CurrentVolumeMultiplier;

		bApplyEffects = SoundGroupProperties->bApplyEffects;
		bAlwaysPlay = SoundGroupProperties->bAlwaysPlay;
		bIsUISound |= SoundGroupProperties->bIsUISound;	// Yes, that's |= because all children of a UI group should be UI sounds
		bIsMusic |= SoundGroupProperties->bIsMusic;	// Yes, that's |= because all children of a music group should be music
		bNoReverb = SoundGroupProperties->bNoReverb;	// A group with reverb applied may have children with no reverb
	}

	// Recurse nodes, have SoundNodeWave's create new wave instances and update bFinished unless we finished fading out. 
	bFinished = TRUE;
	if( FadeOutStopTime == -1 || ( PlaybackTime <= FadeOutStopTime ) )
	{
		CueFirstNode->ParseNodes( AudioDevice, NULL, 0, this, InWaveInstances );
	}

	// Stop playback, handles bAutoDestroy in Stop.
	if( bFinished == TRUE )
	{
		Stop();
	}
}

/** stops the audio (if playing), detaches the component, and resets the component's properties to the values of its template */
void UAudioComponent::ResetToDefaults( void )
{
	if( !IsTemplate() )
	{
		// make sure we're fully stopped and detached
		Stop();
		DetachFromAny();

		UAudioComponent* Default = GetArchetype<UAudioComponent>();

		// copy all non-native, non-duplicatetransient, non-Component properties we have from all classes up to and including UActorComponent
		for( UProperty* Property = GetClass()->PropertyLink; Property != NULL; Property = Property->PropertyLinkNext )
		{
			if( !( Property->PropertyFlags & CPF_Native ) && !( Property->PropertyFlags & CPF_DuplicateTransient ) && !( Property->PropertyFlags & CPF_Component ) &&
				Property->GetOwnerClass()->IsChildOf( UActorComponent::StaticClass() ) )
			{
				Property->CopyCompleteValue( ( BYTE* )this + Property->Offset, ( BYTE* )Default + Property->Offset, NULL, this );
			}
		}
	}
}

void UAudioComponent::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	if( !Ar.IsLoading() && !Ar.IsSaving() )
	{
		Ar << CueFirstNode; 
		Ar << WaveInstances;
		Ar << SoundNodeOffsetMap;
		Ar << SoundNodeResetWaveMap;
		Ar << CurrentNotifyBufferFinishedHook;
	}
}

void UAudioComponent::SetFloatParameter( FName InName, FLOAT InFloat )
{
	if( InName != NAME_None )
	{
		// First see if an entry for this name already exists
		for( INT i = 0; i < InstanceParameters.Num(); i++ )
		{
			FAudioComponentParam& P = InstanceParameters( i );
			if( P.ParamName == InName )
			{
				P.FloatParam = InFloat;
				return;
			}
		}

		// We didn't find one, so create a new one.
		const INT NewParamIndex = InstanceParameters.AddZeroed();
		InstanceParameters( NewParamIndex ).ParamName = InName;
		InstanceParameters( NewParamIndex ).FloatParam = InFloat;
	}
}

void UAudioComponent::SetWaveParameter( FName InName, USoundNodeWave* InWave )
{
	if( InName != NAME_None )
	{
		// First see if an entry for this name already exists
		for( INT i = 0; i < InstanceParameters.Num(); i++ )
		{
			FAudioComponentParam& P = InstanceParameters( i );
			if( P.ParamName == InName )
			{
				P.WaveParam = InWave;
				return;
			}
		}

		// We didn't find one, so create a new one.
		const INT NewParamIndex = InstanceParameters.AddZeroed();
		InstanceParameters( NewParamIndex ).ParamName = InName;
		InstanceParameters( NewParamIndex ).WaveParam = InWave;
	}
}

/** 
 *	Try and find an Instance Parameter with the given name in this AudioComponent, and if we find it return the float value. 
 *	If we fail to find the parameter, return false.
 */
UBOOL UAudioComponent::GetFloatParameter( FName InName, FLOAT& OutFloat )
{
	// Always fail if we pass in no name.
	if( InName != NAME_None )
	{
		for( INT i = 0; i < InstanceParameters.Num(); i++ )
		{
			const FAudioComponentParam& P = InstanceParameters( i );
			if( P.ParamName == InName )
			{
				OutFloat = P.FloatParam;
				return( TRUE );
			}
		}
	}

	return( FALSE );
}

/** 
 *	Try and find an Instance Parameter with the given name in this AudioComponent, and if we find it return the USoundNodeWave value. 
 *	If we fail to find the parameter, return false.
 */
UBOOL UAudioComponent::GetWaveParameter( FName InName, USoundNodeWave*& OutWave )
{
	// Always fail if we pass in no name.
	if( InName != NAME_None )
	{
		for( INT i = 0; i < InstanceParameters.Num(); i++ )
		{
			const FAudioComponentParam& P = InstanceParameters( i );
			if( P.ParamName == InName )
			{
				OutWave = P.WaveParam;
				return( TRUE );
			}
		}
	}

	return( FALSE );
}

/**
 * This is called in place of "play".  So you will say AudioComponent->FadeIn().  
 * This is useful for fading in music or some constant playing sound.
 *
 * If FadeTime is 0.0, this is the same as calling Play() but just modifying the volume by
 * FadeVolumeLevel. (e.g. you will play instantly but the FadeVolumeLevel will affect the AudioComponent
 *
 * If FadeTime is > 0.0, this will call Play(), and then increase the volume level of this 
 * AudioCompoenent to the passed in FadeVolumeLevel over FadeInTime seconds.
 *
 * The VolumeLevel is MODIFYING the AudioComponent's "base" volume.  (e.g.  if you have an 
 * AudioComponent that is volume 1000 and you pass in .5 as your VolumeLevel then you will fade to 500 )
 *
 * @param FadeInDuration how long it should take to reach the FadeVolumeLevel
 * @param FadeVolumeLevel the percentage of the AudioComponents's calculated volume in which to fade to
 **/
void UAudioComponent::FadeIn( FLOAT FadeInDuration, FLOAT FadeVolumeLevel )
{
	if( FadeInDuration >= 0.0f )
	{
		FadeInStartTime = 0.0f;
		FadeInStopTime = FadeInStartTime + FadeInDuration;
		FadeInTargetVolume = FadeVolumeLevel;
	}

	// @todo msew:  should this restarting playing?  No probably not.  It should AdjustVolume to the FadeVolumeLevel
	Play();
}

/**
 * This is called in place of "stop".  So you will say AudioComponent->FadeOut().  
 * This is useful for fading out music or some constant playing sound.
 *
 * If FadeTime is 0.0, this is the same as calling Stop().
 *
 * If FadeTime is > 0.0, this will decrease the volume level of this 
 * AudioCompoenent to the passed in FadeVolumeLevel over FadeInTime seconds. 
 *
 * The VolumeLevel is MODIFYING the AudioComponent's "base" volume.  (e.g.  if you have an 
 * AudioComponent that is volume 1000 and you pass in .5 as your VolumeLevel then you will fade to 500 )
 *
 * @param FadeOutDuration how long it should take to reach the FadeVolumeLevel
 * @param FadeVolumeLevel the percentage of the AudioComponents's calculated volume in which to fade to
 **/
void UAudioComponent::FadeOut( FLOAT FadeOutDuration, FLOAT FadeVolumeLevel )
{
	if( FadeOutDuration > 0.0f )
	{
		FadeOutStartTime = PlaybackTime;
		FadeOutStopTime = FadeOutStartTime + FadeOutDuration;
		FadeOutTargetVolume = FadeVolumeLevel;
	}
	else
	{
		Stop();
	}
}

/**
 * This will allow one to adjust the volume of an AudioComponent on the fly
 **/
void UAudioComponent::AdjustVolume( FLOAT AdjustVolumeDuration, FLOAT AdjustVolumeLevel )
{
	if( AdjustVolumeDuration >= 0.0f )
	{
		AdjustVolumeStartTime = PlaybackTime;
		AdjustVolumeStopTime = AdjustVolumeStartTime + AdjustVolumeDuration;
		AdjustVolumeTargetVolume = AdjustVolumeLevel;
	}
}

/** Helper function to do determine the fade volume value based on start, stop, target volume levels **/
FLOAT UAudioComponent::FadeMultiplierHelper( FLOAT FadeStartTime, FLOAT FadeStopTime, FLOAT FadeTargetValue ) const
{
	// we calculate the total amount of the duration used up and then use that percentage of the TargetVol
	const FLOAT PercentOfVolumeToUse = ( PlaybackTime - FadeStartTime ) / ( FadeStopTime - FadeStartTime );

	// If the clamp fires, the source data is incorrect. 
	FLOAT Retval = Clamp<FLOAT>( PercentOfVolumeToUse * FadeTargetValue, 0.0f, 1.0f );

	return Retval;
}

FLOAT UAudioComponent::GetFadeInMultiplier( void ) const
{
	FLOAT Retval = 1.0f;

	// keep stepping towards our target until we hit our stop time
	if( PlaybackTime <= FadeInStopTime )
	{
		Retval = FadeMultiplierHelper( FadeInStartTime, FadeInStopTime, FadeInTargetVolume );
	}
	else if( PlaybackTime > FadeInStopTime )
	{
		Retval = FadeInTargetVolume;
	}

	return( Retval );
}


FLOAT UAudioComponent::GetFadeOutMultiplier( void ) const
{
	FLOAT Retval = 1.0f;

	// keep stepping towards our target until we hit our stop time
	if( PlaybackTime <= FadeOutStopTime )
	{
		// deal with people wanting to crescendo fade out!
		FLOAT VolAmt = 1.0f;
		if( FadeOutTargetVolume < 1.0f )
		{
			VolAmt = FadeMultiplierHelper( FadeOutStartTime, FadeOutStopTime, ( 1.0f - FadeOutTargetVolume ) );
			Retval = 1.0f - VolAmt; // in FadeOut() we check for negative values
		}
		else if( FadeOutTargetVolume > 1.0f )
		{
			VolAmt = FadeMultiplierHelper( FadeOutStartTime, FadeOutStopTime, ( FadeOutTargetVolume  - 1.0f ) );
			Retval = 1.0f + VolAmt;
		}
	}
	else if( PlaybackTime > FadeOutStopTime )
	{
		Retval =  FadeOutTargetVolume;
	}

	return( Retval );
}

FLOAT UAudioComponent::GetAdjustVolumeOnFlyMultiplier( void )
{
	FLOAT Retval = 1.0f;

	// keep stepping towards our target until we hit our stop time
	if( PlaybackTime <= AdjustVolumeStopTime )
	{
		// deal with people wanting to crescendo fade out!
		FLOAT VolAmt = 1.0f;
		if( AdjustVolumeTargetVolume < CurrAdjustVolumeTargetVolume )
		{
			VolAmt = FadeMultiplierHelper( AdjustVolumeStartTime, AdjustVolumeStopTime, (CurrAdjustVolumeTargetVolume - AdjustVolumeTargetVolume) );
			Retval = CurrAdjustVolumeTargetVolume - VolAmt; 
		}
		else if( AdjustVolumeTargetVolume > CurrAdjustVolumeTargetVolume )
		{
			VolAmt = FadeMultiplierHelper( AdjustVolumeStartTime, AdjustVolumeStopTime, (AdjustVolumeTargetVolume  - CurrAdjustVolumeTargetVolume) );
			Retval = CurrAdjustVolumeTargetVolume + VolAmt;
		}

		//debugf( TEXT( "VolAmt: %f CurrentVolume: %f Retval: %f" ), VolAmt, CurrentVolume, Retval );
	}
	else if( PlaybackTime > AdjustVolumeStopTime )
	{
		CurrAdjustVolumeTargetVolume = AdjustVolumeTargetVolume;
		Retval = AdjustVolumeTargetVolume;
	}

	return( Retval );
}

//
// Script interface
//

void UAudioComponent::execPlay( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	Play();	
}
IMPLEMENT_FUNCTION( UAudioComponent, INDEX_NONE, execPlay );

void UAudioComponent::execStop( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	Stop();
}
IMPLEMENT_FUNCTION( UAudioComponent, INDEX_NONE, execStop );

void UAudioComponent::execSetFloatParameter( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME( InName );
	P_GET_FLOAT( InFloat );
	P_FINISH;

	SetFloatParameter( InName, InFloat );
}
IMPLEMENT_FUNCTION( UAudioComponent, INDEX_NONE, execSetFloatParameter );

void UAudioComponent::execSetWaveParameter( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME( InName );
	P_GET_OBJECT( USoundNodeWave, InWave );
	P_FINISH;

	SetWaveParameter( InName, InWave );
}
IMPLEMENT_FUNCTION( UAudioComponent, INDEX_NONE, execSetWaveParameter );

void UAudioComponent::execFadeIn( FFrame& Stack, RESULT_DECL )
{
	P_GET_FLOAT( FadeInDuration );
	P_GET_FLOAT( FadeVolumeLevel );
	P_FINISH;

	FadeIn( FadeInDuration, FadeVolumeLevel );
}
IMPLEMENT_FUNCTION( UAudioComponent, INDEX_NONE, execFadeIn );

void UAudioComponent::execFadeOut( FFrame& Stack, RESULT_DECL )
{
	P_GET_FLOAT( FadeOutDuration );
	P_GET_FLOAT( FadeVolumeLevel );
	P_FINISH;

	FadeOut( FadeOutDuration, FadeVolumeLevel );
}
IMPLEMENT_FUNCTION( UAudioComponent, INDEX_NONE, execFadeOut );


void UAudioComponent::execAdjustVolume( FFrame& Stack, RESULT_DECL )
{
	P_GET_FLOAT( AdjustVolumeDuration );
	P_GET_FLOAT( AdjustVolumeLevel );
	P_FINISH;

	AdjustVolume( AdjustVolumeDuration, AdjustVolumeLevel );
}
IMPLEMENT_FUNCTION( UAudioComponent, INDEX_NONE, execAdjustVolume );

/*-----------------------------------------------------------------------------
	AAmbientSound implementation.
-----------------------------------------------------------------------------*/

void AAmbientSound::CheckForErrors()
{
	Super::CheckForErrors();
	if ( AudioComponent == NULL )
	{
		GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s : Ambient sound actor has NULL AudioComponent property - please delete!"), *GetName() ), MCACTION_DELETE,  TEXT("AudioComponentNull"));
	}
	else if (AudioComponent->SoundCue == NULL)
	{
		GWarn->MapCheck_Add(MCTYPE_WARNING, this, TEXT("Ambient sound actor's AudioComponent has a NULL SoundCue property!"), MCACTION_NONE, TEXT("SoundCueNull"));
	}
}

/**
 * Starts audio playback if wanted.
 */
void AAmbientSound::UpdateComponentsInternal( UBOOL bCollisionUpdate )
{
	Super::UpdateComponentsInternal(bCollisionUpdate);

	if( bAutoPlay && AudioComponent && !AudioComponent->bWasPlaying )
	{
		AudioComponent->Play();
		bIsPlaying = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	AAmbientSoundNonLoop implementation.
-----------------------------------------------------------------------------*/

void AAmbientSoundNonLoop::CheckForErrors()
{
	Super::CheckForErrors();
	if ( SoundNodeInstance ) // GWarn'd by AAmbientSoundSimple::CheckForErrors().
	{
		FString OwnerName = Owner ? Owner->GetName() : TEXT("");
		USoundNodeAmbientNonLoop* SoundNodeNonLoop = Cast<USoundNodeAmbientNonLoop>( SoundNodeInstance );

		if ( !SoundNodeNonLoop )
		{
			// Warn that the SoundNodeInstance is not of the expected type (USoundNodeAmbientNonLoop).
			GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s::%s : AmbientSoundNonLoop has a non-USoundNodeAmbientNonLoop SoundNodeInstance"), *GetName(), *OwnerName ), MCACTION_NONE, TEXT("NonAmbientSoundNonLoopInstance") );
		}
		else
		{
			// Search sound slots for at least one wave.
			UBOOL bFoundNonNULLWave = FALSE;

			for ( INT SlotIndex = 0 ; SlotIndex < SoundNodeNonLoop->SoundSlots.Num() ; ++SlotIndex )
			{
				if( SoundNodeNonLoop->SoundSlots(SlotIndex).Wave != NULL )
				{
					bFoundNonNULLWave = TRUE;
					break;
				}
			}

			// Warn if no wave was found.
			if ( !bFoundNonNULLWave )
			{
				GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s::%s : AmbientSoundNonLoop has no Slots containing a Wave"), *GetName(), *OwnerName ), MCACTION_NONE, TEXT("NoSlotsWithWave") );
			}
		}
	}
}

IMPLEMENT_CLASS(AAmbientSoundNonLoop);

/*-----------------------------------------------------------------------------
	AAmbientSoundSimple implementation.
-----------------------------------------------------------------------------*/

/**
 * Helper function used to sync up instantiated objects.
 */
void AAmbientSoundSimple::SyncUpInstantiatedObjects()
{
	if( AudioComponent )
	{
		// Propagate instanced objects.
		SoundCueInstance->FirstNode	= SoundNodeInstance;
		AudioComponent->SoundCue	= SoundCueInstance;
		AmbientProperties			= SoundNodeInstance;

		// Make sure neither sound cue nor node are shareable or show up in generic browser.
		check( SoundNodeInstance );
		check( SoundCueInstance );
		SoundNodeInstance->ClearFlags( RF_Public );
		SoundCueInstance->ClearFlags( RF_Public );
	}
}

/**
 * Called from within SpawnActor, calling SyncUpInstantiatedObjects.
 */
void AAmbientSoundSimple::Spawned()
{
	Super::Spawned();
	SyncUpInstantiatedObjects();
}

/**
 * Called when after .t3d import of this actor (paste, duplicate or .t3d import),
 * calling SyncUpInstantiatedObjects.
 */
void AAmbientSoundSimple::PostEditImport()
{
	Super::PostEditImport();
	SyncUpInstantiatedObjects();
}

/**
 * Used to temporarily clear references for duplication.
 *
 * @param PropertyThatWillChange	property that will change
 */
void AAmbientSoundSimple::PreEditChange(UProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);
	if( AudioComponent )
	{
		// Clear references for duplication/ import.
		AudioComponent->SoundCue	= NULL;
		AmbientProperties			= NULL;
	}
}

/**
 * Used to reset audio component when AmbientProperties change
 *
 * @param PropertyThatChanged	property that changed
 */
void AAmbientSoundSimple::PostEditChange(UProperty* PropertyThatChanged)
{
	if( AudioComponent )
	{
		// Sync up properties.
		SyncUpInstantiatedObjects();

		// Reset audio component.
		if( AudioComponent->bWasPlaying )
		{
			AudioComponent->Play();
		}
	}
	Super::PostEditChange(PropertyThatChanged);
}

static void ApplyScaleToFloat(FLOAT& Dst, const FVector& DeltaScale)
{
	const FLOAT Multiplier = ( DeltaScale.X > 0.0f || DeltaScale.Y > 0.0f || DeltaScale.Z > 0.0f ) ? 1.0f : -1.0f;
	Dst += Multiplier * DeltaScale.Size();
	Dst = Max( 0.f, Dst );
}

void AAmbientSoundSimple::EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * 500.0f;

	if ( AmbientProperties )
	{
		UBOOL bMadeChange = FALSE;
		if ( bCtrlDown )
		{
			UDistributionFloat* APMinRadius = AmbientProperties->MinRadius.Distribution;

			if ( APMinRadius->IsA( UDistributionFloatUniform::StaticClass() ) )
			{
				UDistributionFloatUniform* MinRadiusDFU = static_cast<UDistributionFloatUniform*>( APMinRadius );
				ApplyScaleToFloat( MinRadiusDFU->Max, ModifiedScale );
				ApplyScaleToFloat( MinRadiusDFU->Min, ModifiedScale );
				// Clamp Min to Max.
				MinRadiusDFU->Min = Min( MinRadiusDFU->Min, MinRadiusDFU->Max );
				bMadeChange = TRUE;
			}
			else if ( APMinRadius->IsA( UDistributionFloatConstant::StaticClass() ) )
			{
				UDistributionFloatConstant* MinRadiusDFC = static_cast<UDistributionFloatConstant*>( APMinRadius );
				ApplyScaleToFloat( MinRadiusDFC->Constant, ModifiedScale );
				bMadeChange = TRUE;
			}
		}
		else
		{
			UDistributionFloat* APMaxRadius = AmbientProperties->MaxRadius.Distribution;
			
			if ( APMaxRadius->IsA( UDistributionFloatUniform::StaticClass() ) )
			{
				UDistributionFloatUniform* MaxRadiusDFU = static_cast<UDistributionFloatUniform*>( APMaxRadius );
				ApplyScaleToFloat( MaxRadiusDFU->Max, ModifiedScale );
				ApplyScaleToFloat( MaxRadiusDFU->Min, ModifiedScale );
				// Clamp Min to Max.
				MaxRadiusDFU->Min = Min( MaxRadiusDFU->Min, MaxRadiusDFU->Max );
				bMadeChange = TRUE;
			}
			else if ( APMaxRadius->IsA( UDistributionFloatConstant::StaticClass() ) )
			{
				UDistributionFloatConstant* MaxRadiusDFC = static_cast<UDistributionFloatConstant*>( APMaxRadius );
				ApplyScaleToFloat( MaxRadiusDFC->Constant, ModifiedScale );
				bMadeChange = TRUE;
			}
		}

		if ( bMadeChange )
		{
			PostEditChange( NULL );
		}
	}
}

void AAmbientSoundSimple::CheckForErrors()
{
	Super::CheckForErrors();
	FString OwnerName = Owner ? Owner->GetName() : TEXT("");
	if ( AmbientProperties == NULL )
	{
		GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s::%s : Ambient sound has NULL AmbientProperties"), *GetName(), *OwnerName ), MCACTION_NONE, TEXT("NullAmbientProperties") );
	}
	else if ( AmbientProperties->Wave == NULL
		// Don't warn on AmbientSoundNonLoops, which use sound slots rather than directly referencing a wave.
			&& !IsA(AAmbientSoundNonLoop::StaticClass()) )
	{
		GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s::%s : Ambient sound's AmbientProperties has a NULL Wave"), *GetName(), *OwnerName ), MCACTION_NONE, TEXT("NullAmbientWave") );
	}

	if ( SoundNodeInstance == NULL )
	{
		GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s::%s : Ambient sound has NULL SoundNodeInstance"), *GetName(), *OwnerName ), MCACTION_NONE, TEXT("NullSoundNodeInstance") );
	}
}

IMPLEMENT_CLASS(AAmbientSoundSimple);

/*-----------------------------------------------------------------------------
	WaveModInfo implementation - downsampling of wave files.
-----------------------------------------------------------------------------*/

//  Macros to convert 4 bytes to a Riff-style ID DWORD.
//  Todo: make these endian independent !!!

#undef MAKEFOURCC

#define MAKEFOURCC(ch0, ch1, ch2, ch3)\
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |\
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#define mmioFOURCC(ch0, ch1, ch2, ch3)\
    MAKEFOURCC(ch0, ch1, ch2, ch3)

// Main Riff-Wave header.
struct FRiffWaveHeader
{ 
	DWORD	rID;			// Contains 'RIFF'
	DWORD	ChunkLen;		// Remaining length of the entire riff chunk (= file).
	DWORD	wID;			// Form type. Contains 'WAVE' for .wav files.
};

// General chunk header format.
struct FRiffChunkOld
{
	DWORD	ChunkID;		  // General data chunk ID like 'data', or 'fmt ' 
	DWORD	ChunkLen;		  // Length of the rest of this chunk in bytes.
};

// ChunkID: 'fmt ' ("WaveFormatEx" structure ) 
struct FFormatChunk
{
    WORD   wFormatTag;        // Format type: 1 = PCM
    WORD   nChannels;         // Number of channels (i.e. mono, stereo...).
    DWORD   nSamplesPerSec;    // Sample rate. 44100 or 22050 or 11025  Hz.
    DWORD   nAvgBytesPerSec;   // For buffer estimation  = sample rate * BlockAlign.
    WORD   nBlockAlign;       // Block size of data = Channels times BYTES per sample.
    WORD   wBitsPerSample;    // Number of bits per sample of mono data.
    WORD   cbSize;            // The count in bytes of the size of extra information (after cbSize).
};

// ChunkID: 'smpl'
struct FSampleChunk
{
	DWORD   dwManufacturer;
	DWORD   dwProduct;
	DWORD   dwSamplePeriod;
	DWORD   dwMIDIUnityNote;
	DWORD   dwMIDIPitchFraction;
	DWORD	dwSMPTEFormat;		
	DWORD   dwSMPTEOffset;		//
	DWORD   cSampleLoops;		// Number of tSampleLoop structures following this chunk
	DWORD   cbSamplerData;		// 
};
 
struct FSampleLoop				// Immediately following cbSamplerData in the SMPL chunk.
{
	DWORD	dwIdentifier;		//
	DWORD	dwType;				//
	DWORD	dwStart;			// Startpoint of the loop in samples
	DWORD	dwEnd;				// Endpoint of the loop in samples
	DWORD	dwFraction;			// Fractional sample adjustment
	DWORD	dwPlayCount;		// Play count
};

//
//	Figure out the WAVE file layout.
//
UBOOL FWaveModInfo::ReadWaveInfo( BYTE* WaveData, INT WaveDataSize )
{
	FFormatChunk*		FmtChunk;
	FRiffWaveHeader*	RiffHdr = (FRiffWaveHeader*) WaveData;
	WaveDataEnd					= WaveData + WaveDataSize;	

	if( WaveDataSize == 0 )
	{
		return( FALSE );
	}
		
	// Verify we've got a real 'WAVE' header.
#if __INTEL_BYTE_ORDER__
	if ( RiffHdr->wID != mmioFOURCC('W','A','V','E') )
	{
		return( FALSE );
	}
#else
	if ( (RiffHdr->wID != (mmioFOURCC('W','A','V','E'))) &&
	     (RiffHdr->wID != (mmioFOURCC('E','V','A','W'))) )
	{
		return( FALSE );
	}

	UBOOL AlreadySwapped = (RiffHdr->wID == (mmioFOURCC('W','A','V','E')));
	if( !AlreadySwapped )
	{
		RiffHdr->rID		= INTEL_ORDER32(RiffHdr->rID);
		RiffHdr->ChunkLen	= INTEL_ORDER32(RiffHdr->ChunkLen);
		RiffHdr->wID		= INTEL_ORDER32(RiffHdr->wID);
	}
#endif

	FRiffChunkOld* RiffChunk	= (FRiffChunkOld*)&WaveData[3*4];
	pMasterSize = &RiffHdr->ChunkLen;
	
	// Look for the 'fmt ' chunk.
	while( ( ((BYTE*)RiffChunk + 8) < WaveDataEnd)  && ( INTEL_ORDER32(RiffChunk->ChunkID) != mmioFOURCC('f','m','t',' ') ) )
	{
		RiffChunk = (FRiffChunkOld*) ( (BYTE*)RiffChunk + Pad16Bit(INTEL_ORDER32(RiffChunk->ChunkLen)) + 8);
	}

	if( INTEL_ORDER32(RiffChunk->ChunkID) != mmioFOURCC('f','m','t',' ') )
	{
		#if !__INTEL_BYTE_ORDER__  // swap them back just in case.
			if(! AlreadySwapped )
			{
				RiffHdr->rID = INTEL_ORDER32(RiffHdr->rID);
				RiffHdr->ChunkLen = INTEL_ORDER32(RiffHdr->ChunkLen);
				RiffHdr->wID = INTEL_ORDER32(RiffHdr->wID);
            }
		#endif
		return( FALSE );
	}

	FmtChunk 		= (FFormatChunk*)((BYTE*)RiffChunk + 8);
#if !__INTEL_BYTE_ORDER__
	if( !AlreadySwapped )
	{
		FmtChunk->wFormatTag		= INTEL_ORDER16(FmtChunk->wFormatTag);
		FmtChunk->nChannels			= INTEL_ORDER16(FmtChunk->nChannels);
		FmtChunk->nSamplesPerSec	= INTEL_ORDER32(FmtChunk->nSamplesPerSec);
		FmtChunk->nAvgBytesPerSec	= INTEL_ORDER32(FmtChunk->nAvgBytesPerSec);
		FmtChunk->nBlockAlign		= INTEL_ORDER16(FmtChunk->nBlockAlign);
		FmtChunk->wBitsPerSample	= INTEL_ORDER16(FmtChunk->wBitsPerSample);
	}
#endif
	pBitsPerSample  = &FmtChunk->wBitsPerSample;
	pSamplesPerSec  = &FmtChunk->nSamplesPerSec;
	pAvgBytesPerSec = &FmtChunk->nAvgBytesPerSec;
	pBlockAlign		= &FmtChunk->nBlockAlign;
	pChannels       = &FmtChunk->nChannels;

	// re-initalize the RiffChunk pointer
	RiffChunk = (FRiffChunkOld*)&WaveData[3*4];
	
	// Look for the 'data' chunk.
	while( ( ((BYTE*)RiffChunk + 8) < WaveDataEnd) && ( INTEL_ORDER32(RiffChunk->ChunkID) != mmioFOURCC('d','a','t','a') ) )
	{
		RiffChunk = (FRiffChunkOld*) ( (BYTE*)RiffChunk + Pad16Bit(INTEL_ORDER32(RiffChunk->ChunkLen)) + 8);
	}

	if( INTEL_ORDER32(RiffChunk->ChunkID) != mmioFOURCC('d','a','t','a') )
	{
		#if !__INTEL_BYTE_ORDER__  // swap them back just in case.
			if( !AlreadySwapped )
			{
				RiffHdr->rID				= INTEL_ORDER32(RiffHdr->rID);
				RiffHdr->ChunkLen			= INTEL_ORDER32(RiffHdr->ChunkLen);
				RiffHdr->wID				= INTEL_ORDER32(RiffHdr->wID);
				FmtChunk->wFormatTag		= INTEL_ORDER16(FmtChunk->wFormatTag);
				FmtChunk->nChannels			= INTEL_ORDER16(FmtChunk->nChannels);
				FmtChunk->nSamplesPerSec	= INTEL_ORDER32(FmtChunk->nSamplesPerSec);
				FmtChunk->nAvgBytesPerSec	= INTEL_ORDER32(FmtChunk->nAvgBytesPerSec);
				FmtChunk->nBlockAlign		= INTEL_ORDER16(FmtChunk->nBlockAlign);
				FmtChunk->wBitsPerSample	= INTEL_ORDER16(FmtChunk->wBitsPerSample);
			}
		#endif
		return( FALSE );
	}

	#if !__INTEL_BYTE_ORDER__  // swap them back just in case.
		if( AlreadySwapped ) // swap back into Intel order for chunk search...
		{
			RiffChunk->ChunkLen = INTEL_ORDER32(RiffChunk->ChunkLen);
		}
	#endif

	SampleDataStart 	= (BYTE*)RiffChunk + 8;
	pWaveDataSize   	= &RiffChunk->ChunkLen;
	SampleDataSize  	=  INTEL_ORDER32(RiffChunk->ChunkLen);
	OldBitsPerSample 	= FmtChunk->wBitsPerSample;
	SampleDataEnd   	=  SampleDataStart+SampleDataSize;

	if ((BYTE *) SampleDataEnd > (BYTE *) WaveDataEnd)
	{
		debugf(NAME_Warning, TEXT("Wave data chunk is too big!"));

		// Fix it up by clamping data chunk.
		SampleDataEnd = (BYTE *) WaveDataEnd;
		SampleDataSize = SampleDataEnd - SampleDataStart;
		RiffChunk->ChunkLen = INTEL_ORDER32(SampleDataSize);
	}

	NewDataSize	= SampleDataSize;

	if( FmtChunk->wFormatTag != 1 )
	{
		return( FALSE );
	}

	// PS3 expects the data to be in Intel order, but not the header values
#if !__INTEL_BYTE_ORDER__ && !PS3
	if( !AlreadySwapped )
	{
		if (FmtChunk->wBitsPerSample == 16)
		{
			for (WORD *i = (WORD *) SampleDataStart; i < (WORD *) SampleDataEnd; i++)
			{
				*i = INTEL_ORDER16(*i);
			}
		}
		else if (FmtChunk->wBitsPerSample == 32)
		{
			for (DWORD *i = (DWORD *) SampleDataStart; i < (DWORD *) SampleDataEnd; i++)
			{
				*i = INTEL_ORDER32(*i);
			}
		}
	}
#endif

	// Re-initalize the RiffChunk pointer
	RiffChunk = (FRiffChunkOld*)&WaveData[3*4];
	// Look for a 'smpl' chunk.
	while( ( (((BYTE*)RiffChunk) + 8) < WaveDataEnd) && ( INTEL_ORDER32(RiffChunk->ChunkID) != mmioFOURCC('s','m','p','l') ) )
	{
		RiffChunk = (FRiffChunkOld*) ( (BYTE*)RiffChunk + Pad16Bit(INTEL_ORDER32(RiffChunk->ChunkLen)) + 8);
	}
	
	// Chunk found ? smpl chunk is optional.
	// Find the first sample-loop structure, and the total number of them.
	if( (PTRINT)RiffChunk+4<(PTRINT)WaveDataEnd && INTEL_ORDER32(RiffChunk->ChunkID) == mmioFOURCC('s','m','p','l') )
	{
		FSampleChunk *pSampleChunk = (FSampleChunk *) ((BYTE*)RiffChunk + 8);
		pSampleLoop = (FSampleLoop*) ((BYTE*)RiffChunk + 8 + sizeof(FSampleChunk));

		if (((BYTE *)pSampleChunk + sizeof (FSampleChunk)) > (BYTE *)WaveDataEnd)
		{
			pSampleChunk = NULL;
		}

		if (((BYTE *)pSampleLoop + sizeof (FSampleLoop)) > (BYTE *)WaveDataEnd)
		{
			pSampleLoop = NULL;
		}

#if !__INTEL_BYTE_ORDER__
		if( !AlreadySwapped )
		{
			if( pSampleChunk != NULL )
			{
				pSampleChunk->dwManufacturer		= INTEL_ORDER32(pSampleChunk->dwManufacturer);
				pSampleChunk->dwProduct				= INTEL_ORDER32(pSampleChunk->dwProduct);
				pSampleChunk->dwSamplePeriod		= INTEL_ORDER32(pSampleChunk->dwSamplePeriod);
				pSampleChunk->dwMIDIUnityNote		= INTEL_ORDER32(pSampleChunk->dwMIDIUnityNote);
				pSampleChunk->dwMIDIPitchFraction	= INTEL_ORDER32(pSampleChunk->dwMIDIPitchFraction);
				pSampleChunk->dwSMPTEFormat			= INTEL_ORDER32(pSampleChunk->dwSMPTEFormat);
				pSampleChunk->dwSMPTEOffset			= INTEL_ORDER32(pSampleChunk->dwSMPTEOffset);
				pSampleChunk->cSampleLoops			= INTEL_ORDER32(pSampleChunk->cSampleLoops);
				pSampleChunk->cbSamplerData			= INTEL_ORDER32(pSampleChunk->cbSamplerData);
			}

			if (pSampleLoop != NULL)
			{
				pSampleLoop->dwIdentifier			= INTEL_ORDER32(pSampleLoop->dwIdentifier);
				pSampleLoop->dwType					= INTEL_ORDER32(pSampleLoop->dwType);
				pSampleLoop->dwStart				= INTEL_ORDER32(pSampleLoop->dwStart);
				pSampleLoop->dwEnd					= INTEL_ORDER32(pSampleLoop->dwEnd);
				pSampleLoop->dwFraction				= INTEL_ORDER32(pSampleLoop->dwFraction);
				pSampleLoop->dwPlayCount			= INTEL_ORDER32(pSampleLoop->dwPlayCount);
            }
		}
#endif
		SampleLoopsNum = pSampleChunk->cSampleLoops;
	}

	// Couldn't byte swap this before, since it'd throw off the chunk search.
#if !__INTEL_BYTE_ORDER__
		*pWaveDataSize = INTEL_ORDER32(*pWaveDataSize);
#endif
	
	return( TRUE );
}

/*-----------------------------------------------------------------------------
	UDrawSoundRadiusComponent implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UDrawSoundRadiusComponent);

/**
 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
 * @return The proxy object.
 */
FPrimitiveSceneProxy* UDrawSoundRadiusComponent::CreateSceneProxy()
{
	/** Represents a UDrawSoundRadiusComponent to the scene manager. */
	class FDrawSoundRadiusSceneProxy : public FPrimitiveSceneProxy
	{
	public:
		FDrawSoundRadiusSceneProxy(const UDrawSoundRadiusComponent* InComponent)
			:	FPrimitiveSceneProxy( InComponent )
			,	bDrawWireSphere( InComponent->bDrawWireSphere )
			,	bDrawLitSphere( InComponent->bDrawLitSphere )
			,	bShouldDraw( FALSE )
			,	SphereColor( InComponent->SphereColor )
			,	SphereRadius( 0.f )
			,	SphereSides( InComponent->SphereSides )
		{
			AAmbientSoundSimple* ASS = static_cast<AAmbientSoundSimple*>( InComponent->GetOwner() );
			UDistributionFloat* APMaxRadius = ASS->AmbientProperties->MaxRadius.Distribution;

			if ( APMaxRadius->IsA( UDistributionFloatUniform::StaticClass() ) )
			{
				UDistributionFloatUniform* MaxRadiusDFU = static_cast<UDistributionFloatUniform*>( APMaxRadius );
				SphereRadius = MaxRadiusDFU->Max;
				bShouldDraw = TRUE;
			}
			else if ( APMaxRadius->IsA( UDistributionFloatConstant::StaticClass() ) )
			{
				UDistributionFloatConstant* MaxRadiusDFC = static_cast<UDistributionFloatConstant*>( APMaxRadius );
				SphereRadius = MaxRadiusDFC->Constant;
				bShouldDraw = TRUE;
			}
		}

		/** 
		* Draw the scene proxy as a dynamic element
		*
		* @param	PDI - draw interface to render to
		* @param	View - current view
		* @param	DPGIndex - current depth priority 
		* @param	Flags - optional set of flags from EDrawDynamicElementFlags
		*/
		virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags)
		{
			if( bDrawWireSphere )
			{
				DrawCircle( PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetAxis(0), LocalToWorld.GetAxis(1), SphereColor, SphereRadius, SphereSides, SDPG_World );
				DrawCircle( PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetAxis(0), LocalToWorld.GetAxis(2), SphereColor, SphereRadius, SphereSides, SDPG_World );
				DrawCircle( PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetAxis(1), LocalToWorld.GetAxis(2), SphereColor, SphereRadius, SphereSides, SDPG_World );
			}
#if GEMINI_TODO
			if(bDrawLitSphere && SphereMaterial)
			{
				DrawSphere(PDI,GetOrigin(), FVector(SphereRadius), SphereSides, SphereSides/2, Context.GetRenderMaterial(SphereMaterial,FColor(255,0,0),IsOwnerSelected(),NULL,NULL) );
			}
#endif
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
		{
			const UBOOL bVisible = bShouldDraw && (View->Family->ShowFlags & SHOW_AudioRadius) && (bDrawWireSphere || bDrawLitSphere);
			FPrimitiveViewRelevance Result;
			Result.bDynamicRelevance = IsShown(View) && bVisible;
			Result.SetDPG( SDPG_World,TRUE );
			if (IsShadowCast(View))
			{
				Result.bShadowRelevance = TRUE;
			}
			return Result;
		}
		virtual EMemoryStats GetMemoryStatType( void ) const { return( STAT_GameToRendererMallocOther ); }
		virtual DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
		DWORD GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

	private:
		BITFIELD	bDrawWireSphere;
		BITFIELD	bDrawLitSphere;
		BITFIELD	bShouldDraw;
		FColor		SphereColor;
		FLOAT		SphereRadius;
		INT			SphereSides;
	};

	FPrimitiveSceneProxy* Proxy = NULL;
	if ( IsOwnerSelected() && Owner->IsA( AAmbientSoundSimple::StaticClass() ) )
	{
		Proxy = new FDrawSoundRadiusSceneProxy(this);
	}

	return Proxy;
}

void UDrawSoundRadiusComponent::UpdateBounds()
{
	Bounds = FBoxSphereBounds( FVector(0,0,0), FVector(SphereRadius), SphereRadius ).TransformBy(LocalToWorld);
}

#if WITH_TTS

/*------------------------------------------------------------------------------------
	FTTSAudioInfo.
------------------------------------------------------------------------------------*/

#if SUPPORTS_PRAGMA_PACK
#pragma pack( push, 8 )
#endif

#undef WAVE_FORMAT_1M16

#include "../../../External/DECtalk464/include/FonixTtsDtSimple.h"

#if _WINDOWS
#pragma comment( lib, "..\\..\\External\\DECtalk464\\lib\\Win32\\FonixTtsDtSimple.lib" )
#elif _XBOX
#pragma comment( lib, "..\\..\\External\\DECtalk464\\lib\\Xenon\\FonixTtsDtSimple.lib" )
#elif PS3
#else
#error "Undefined platform"
#endif

#if SUPPORTS_PRAGMA_PACK
#pragma pack( pop )
#endif

/** 
 * Callback from TTS processing
 */
static FTextToSpeech* CallbackState = NULL;

SWORD* FTextToSpeech::StaticCallback( SWORD* AudioData, long Flags )
{
	if( CallbackState )
	{	
		CallbackState->WriteChunk( AudioData );
	}

	check(CallbackState);
	return( CallbackState->TTSBuffer );
}

/** 
 * Convert UE3 language to TTS language index
 */
TCHAR* FTextToSpeech::LanguageConvert[] =
{
	TEXT( "INT" ),		// US_English,
	TEXT( "FRA" ),		// French,
	TEXT( "DEU" ),		// German,
	TEXT( "ESN" ),		// Castilian_Spanish,
	TEXT( "XXX" ),		// Reserved,
	TEXT( "XXX" ),		// UK_English,
	TEXT( "ESM" ),		// Latin_American_Spanish,
	TEXT( "ITA" ),		// Italian
	NULL
};

/** 
 * Look up the language to use for TTS
 */
INT FTextToSpeech::GetLanguageIndex( FString& Language )
{
	INT LanguageIndex = 0;
	while( LanguageConvert[LanguageIndex] )
	{
		if( !appStricmp( LanguageConvert[LanguageIndex], *Language ) )
		{
			return( LanguageIndex );
		}

		LanguageIndex++;
	}

	// None found; default to US English
	return( 0 );
}

/** 
 * Initialise the TTS system
 */
void FTextToSpeech::Init( void )
{
#if _WINDOWS || _XBOX
	INT	Error;

	Error = FnxTTSDtSimpleOpen( StaticCallback, NULL );
	if( Error )
	{
		debugf( NAME_Init, TEXT( "Failed to init TTS" ) );
		return;
	}

	FString Language = appGetLanguageExt();
	INT LanguageIndex = GetLanguageIndex( Language );

	Error = FnxTTSDtSimpleSetLanguage( LanguageIndex, NULL );
	if( Error )
	{
		debugf( NAME_Init, TEXT( "Failed to set language for TTS" ) );
		return;
	}

	CurrentSpeaker = 0;
	FnxTTSDtSimpleChangeVoice( ( FnxDECtalkVoiceId )CurrentSpeaker, WAVE_FORMAT_1M16 );

	bInit = TRUE;
#endif
}

/** 
 * Free up any resources used by TTS
 */
FTextToSpeech::~FTextToSpeech( void )
{
#if _WINDOWS || _XBOX
	if( bInit )
	{
		FnxTTSDtSimpleClose();
	}
#endif
}

/** 
 * Write a chunk of data to a buffer
 */
void FTextToSpeech::WriteChunk( SWORD* AudioData )
{
	INT Index = PCMData.Num();

	PCMData.Add( TTS_CHUNK_SIZE );

	SWORD* Destination = &PCMData( Index );
	appMemcpy( Destination, AudioData, TTS_CHUNK_SIZE * sizeof( SWORD ) );
}
	
/**
 * Speak a line of text using the speaker
 */
void FTextToSpeech::CreatePCMData( USoundNodeWave* Wave )
{
#if _WINDOWS || _XBOX
	if( bInit )
	{
		if( Wave->TTSSpeaker != CurrentSpeaker )
		{
			FnxTTSDtSimpleChangeVoice( ( FnxDECtalkVoiceId )Wave->TTSSpeaker, WAVE_FORMAT_1M16 );
			CurrentSpeaker = Wave->TTSSpeaker;
		}

		// Best guess at size of generated PCM data
		PCMData.Empty( Wave->SpokenText.Len() * 1024 );

		CallbackState = this;
		FnxTTSDtSimpleStart( TCHAR_TO_ANSI( *Wave->SpokenText ), TTSBuffer, TTS_CHUNK_SIZE, WAVE_FORMAT_1M16 );
		CallbackState = NULL;

		// Copy out the generated PCM data
		Wave->SampleDataSize = PCMData.Num() * sizeof( SWORD );
		Wave->RawPCMData = ( SWORD* )appMalloc( Wave->SampleDataSize );
		appMemcpy( Wave->RawPCMData, PCMData.GetTypedData(), Wave->SampleDataSize );

		Wave->NumChannels = 1;
		Wave->SampleRate = 11025;
		Wave->Duration = ( FLOAT )Wave->SampleDataSize / ( Wave->SampleRate * sizeof( SWORD ) );
		Wave->bDynamicResource = TRUE;

		PCMData.Empty();
	}
#endif
}

#endif // WITH_TTS
