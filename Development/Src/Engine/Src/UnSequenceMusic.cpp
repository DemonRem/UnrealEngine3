/*=============================================================================
   UnSequenceMusic.cpp: Gameplay Music Sequence native code
   Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineSequenceClasses.h"
#include "EngineAudioDeviceClasses.h"

// Engine level variables
IMPLEMENT_CLASS(UMusicTrackDataStructures);
IMPLEMENT_CLASS(USeqVar_MusicTrack);
IMPLEMENT_CLASS(USeqVar_MusicTrackBank);

// Engine level conditions.

// Engine level Events

// Engine level actions
IMPLEMENT_CLASS(USeqAct_CrossFadeMusicTracks);
IMPLEMENT_CLASS(USeqAct_SetMusicTrack);


namespace // our anon namespace
{
	/** This will check to see if the passed in track is the same as the Currently Playing track. **/
	UBOOL IsSameTrackAsCurrentlyPlaying( USeqAct_CrossFadeMusicTracks* TheCrossFadeMusicAction, const FMusicTrackStruct& TrackToPlay )
	{
		UBOOL Retval = FALSE;

		if( ( TheCrossFadeMusicAction->CurrPlayingTrack != NULL )
			&& ( TheCrossFadeMusicAction->CurrPlayingTrack->SoundCue != NULL )
			&& ( TrackToPlay.TheSoundCue != NULL ) 
			)
		{
			//warnf( TEXT( "CrossFadeTrack0 %s" ), *CurrPlayingTrack->SoundCue->GetFullName() );

			//warnf( TEXT( "CrossFadeTrack1 %s" ), *TrackToPlay.TheSoundCue->GetFullName() );

			if( TrackToPlay.TheSoundCue->GetFullName() == TheCrossFadeMusicAction->CurrPlayingTrack->SoundCue->GetFullName() )
			{
				//warnf( TEXT( "TRACKS ARE THE SAME" ) );
				Retval = TRUE;
			}	
		}

		return Retval;
	}


	/**
	 * This will look for the FIRST item in the LinkedVariables list and return it.
	 *
	 * Basically for the MusicManager we deal with only cases where there
	 * can be one object attached to our VariableLinks and we know ahead of time
	 * what that type will be.  So instead of using the slick TArray<TYPE**> we 
	 * just return the pointer of that type and avoid the extra casts iterating
	 * a list of one
	 **/
	template< typename RETURN_TYPE, typename CAST_TO_TYPE >
		RETURN_TYPE FindSingleInstance( const USequenceAction* TheSeqAct, const FString& VarNameToLookFor )
	{
		RETURN_TYPE Retval = NULL;

		// new loading of levels makes this possible to have null!
		if( TheSeqAct != NULL )
		{
			// we are doing this inline so we don't incur multiple castings  (need a template really for this prob)
			for( INT Idx = 0; Idx < TheSeqAct->VariableLinks.Num(); Idx++ )
			{
				if( ( TheSeqAct->VariableLinks(Idx).LinkedVariables.Num() > 0 )
					&& ( TheSeqAct->VariableLinks(Idx).LinkDesc == VarNameToLookFor ) 
					)
				{
					Retval = Cast<CAST_TO_TYPE>( TheSeqAct->VariableLinks(Idx).LinkedVariables(0) ); // only one element can ever be attached
					break; // we only will ever take the first object found
				}
			}
		}

		return Retval;
	}


	/**
	 * This will Find a MusicTrackBank based off any SeqAct_ passed in.
	 **/
	template< typename ACTION_TYPE >
		USeqVar_MusicTrackBank* FindMusicTrackBank( const ACTION_TYPE* TheSeqAct, const FString& VarName )
	{
		USeqVar_MusicTrackBank* Retval = NULL;

		USeqVar_MusicTrackBank* MTB = FindSingleInstance<USeqVar_MusicTrackBank*, USeqVar_MusicTrackBank>( TheSeqAct, VarName );

		// if we don't have a MTB
		// look on the action and get the TrackBankName and find it in the list of all variables
		if( MTB == NULL )
		{
			MTB = FindTheTrackBankObject( TheSeqAct->TrackBankName );
		}

		Retval = MTB;

		return Retval;
	}


	/**
 	 * This is our Copy struct function.
	 *
	 * @todo: move this to the struct's assignment operator
	 **/
	void MusicTrackStructCopyStruct( FMusicTrackStruct& MTB, const FMusicTrackStruct& MT )
	{
		MTB.Params.FadeInTime = MT.Params.FadeInTime;
		MTB.Params.FadeInVolumeLevel = MT.Params.FadeInVolumeLevel;
		MTB.Params.DelayBetweenOldAndNewTrack = MT.Params.DelayBetweenOldAndNewTrack;
		MTB.Params.FadeOutTime = MT.Params.FadeOutTime;
		MTB.Params.FadeOutVolumeLevel = MT.Params.FadeOutVolumeLevel;

		MTB.TrackType = MT.TrackType;
		MTB.TheSoundCue = MT.TheSoundCue;
	}

	/**
	 * This will find the TrackBankObject who's name matches the name passed in.
	 * It will look through all of the objects until it finds the object
	 **/
	USeqVar_MusicTrackBank* FindTheTrackBankObject( const FName& TrackBankName )
	{
		USeqVar_MusicTrackBank* Retval = NULL;

		if( GWorld->PersistentLevel->GameSequences.Num() > 0 )
		{
			USequence* TmpSeq = GWorld->PersistentLevel->GameSequences(0);
			check(TmpSeq);

			USequence* RootSeq = TmpSeq->GetRootSequence();
			check(RootSeq);

			TArray<USequenceVariable*> Vars;
			RootSeq->FindNamedVariables( TrackBankName, FALSE, Vars);

			// there should be only one.  So just return the first one
			if( Vars.Num() > 0 )
			{
				Retval = Cast<USeqVar_MusicTrackBank>(Vars(0));
			}
		}

		return Retval;
	}


	/**
	 * This will actually play the Sound.  
	 *
	 * It will create an AudioComponent and then set all of the needed fade variables on it.
	 * Additionally it will fadeout the currently playing AudioComponent if needed.
	 **/
	void CrossFadeMusicTracks_SoundPlayer( USeqAct_CrossFadeMusicTracks* TheCrossFadeMusicAction, const FMusicTrackStruct& MusicTrackToFadeIn )
	{
		// preconditions
		// we don't want to ever fade to the same track as that will cause a stop and start of the track.  This can occur when you use
		// the AdjustVolume as that sets the Input on the Kismet action to be "hot" which then makes UpdateOps be called which then more than
		// likely will try to CrossFade as the NextTrackToPlayAt is still at a previous crossfade action's time!
		if( ( MusicTrackToFadeIn.TheSoundCue == NULL ) || ( IsSameTrackAsCurrentlyPlaying( TheCrossFadeMusicAction, MusicTrackToFadeIn ) == TRUE ) )
		{
			return;
		}

		// we can make this not create a component for version 2
		UAudioComponent* AudioComp = UAudioDevice::CreateComponent( MusicTrackToFadeIn.TheSoundCue, GWorld->Scene, NULL, FALSE );
		if( AudioComp != NULL )
		{
			//// update the Fading In AudioComponent
			AudioComp->bAutoDestroy = TRUE;
			AudioComp->bShouldRemainActiveIfDropped = TRUE;
			AudioComp->bIsMusic = TRUE;

			//@todo: implement DelayBetweenOldAndNewTrack
			AudioComp->FadeIn( MusicTrackToFadeIn.Params.FadeInTime, MusicTrackToFadeIn.Params.FadeInVolumeLevel ); // Will call Play.

			//// update the Fading Out AudioComponent
			if( TheCrossFadeMusicAction->CurrPlayingTrack != NULL )
			{
				TheCrossFadeMusicAction->CurrPlayingTrack->FadeOut( MusicTrackToFadeIn.Params.FadeOutTime, MusicTrackToFadeIn.Params.FadeOutVolumeLevel );	
			}

			// this is the component we are now currently using  the previous component should finish playing and then be GC'd
			TheCrossFadeMusicAction->CurrTrackType = MusicTrackToFadeIn.TrackType;
			TheCrossFadeMusicAction->CurrPlayingTrack = AudioComp;
		}
	}


	/**
 	 * look on the TrackType and get the sound cue we are going to be playing based off the type
	 **/
	void CrossFadeMusicTracks_FindTrackType( const FName& TrackTypeToLookFor, const USeqAct_CrossFadeMusicTracks* TheSeqAct, FMusicTrackStruct& out_MusicTrackToFadeIn )
	{
		// find the music that is attached.

		// kk for the SetMusicTrack object we will do the following:
		//  - set it on the attached music var if there is one
		//   - if there is not one, we will look on the "tag" that is writ upon the set object (this will allow us to have binding at run time for music objects aka when a new seamless level loads up we can have it set what ever music values we want)

		// this will correct look at the Variable and if it can't find it look for the TrackBank object
		USeqVar_MusicTrackBank* MTB = FindMusicTrackBank<USeqAct_CrossFadeMusicTracks>( TheSeqAct, TEXT("MusicTrackBank") );


		// even after looking we might have not found the MTB
		if( MTB != NULL )
		{
			// look for matching TrackType names
			for( INT Idx = 0; Idx < MTB->MusicTrackBank.Num(); ++Idx )
			{
				// if the Track name matches the TrackBank's name
				if( MTB->MusicTrackBank(Idx).TrackType == TrackTypeToLookFor )
				{
					MusicTrackStructCopyStruct( out_MusicTrackToFadeIn, MTB->MusicTrackBank(Idx) );
					break;
				}
			}
		}
	}


	/**
	 * This will copy the data from a MusicTrack over to the MusicTrackBank based on the TrackType
	 * It will copy the first match only.
	 **/
	void SetMusicTrack_Activated_CopyNamedTrackIntoTrackBank( USeqVar_MusicTrackBank* MTB, const USeqVar_MusicTrack* MT )
	{
		UBOOL bFoundTrack = FALSE;

		// look for matching TrackType names
		for( INT Idx = 0; Idx < MTB->MusicTrackBank.Num(); ++Idx )
		{
			// if the Track name matches the TrackBank's name
			if( MTB->MusicTrackBank(Idx).TrackType == MT->MusicTrack.TrackType )
			{
				MusicTrackStructCopyStruct( MTB->MusicTrackBank(Idx), MT->MusicTrack );
				bFoundTrack = TRUE;
				break;
			}
		}

		// check for the case where the trackType was not found and we need to insert it instead of replacing
		if( bFoundTrack == FALSE )
		{
			MTB->MusicTrackBank.AddItem( MT->MusicTrack );
		}
	}

} // ecapseman // our anon namespace


/*
 * UpdateOp
 *
 * Update parameters over time
 */
UBOOL USeqAct_CrossFadeMusicTracks::UpdateOp( FLOAT DeltaTime )
{
	if( GWorld->GetWorldInfo()->TimeSeconds > NextTrackToPlayAt )
	{
		// replicate to clients
		for (AController* C = GWorld->GetWorldInfo()->ControllerList; C != NULL; C = C->NextController)
		{
			APlayerController* PC = C->GetAPlayerController();
			if (PC != NULL)
			{
				PC->eventClientCrossFadeMusicTrack_PlayTrack(this, NextTrackToPlay);
			}
		}

		// Set the output to hot
		OutputLinks( 0 ).bHasImpulse = TRUE;

		// Stop it ticking
		bLatentExecution = FALSE;
		return TRUE ;
	}

	// keep on ticking
	return FALSE ;
}

/**
 * Updates this sequence action to the current version.
 */
void USeqAct_CrossFadeMusicTracks::UpdateObject()
{
	Super::UpdateObject();

	if( ObjClassVersion == 2 )
	{
		// Remove variable links to the Targets property.
		for( INT LinkIndex=0; LinkIndex<VariableLinks.Num(); LinkIndex++ )
		{
			if( VariableLinks(LinkIndex).PropertyName == FName(TEXT("Targets")) )
			{
				VariableLinks.Remove( LinkIndex-- );
			}
		}
	}
}

/**
 * This will cross fade to the music track listed in the variable: TrackTypeToFadeTo
 **/
void USeqAct_CrossFadeMusicTracks::Activated()
{
	// InputLinks(0)=(LinkDesc="CrossFade")
	if( InputLinks.Num() > 0 && InputLinks(0).bHasImpulse == TRUE )
	{
		const USeqVar_String* TrackType = FindSingleInstance<USeqVar_String*, USeqVar_String>( this, TEXT("TrackTypeToFadeTo") );

		USeqVar_MusicTrackBank* MTB = FindMusicTrackBank<USeqAct_CrossFadeMusicTracks>( this, TEXT("MusicTrackBank") );


		if( ( TrackType != NULL )
			&& ( MTB != NULL )
			)
		{
			// look for matching TrackType names
			for( INT Idx = 0; Idx < MTB->MusicTrackBank.Num(); ++Idx )
			{
				debugf( TEXT( "%s | %s" ), *MTB->MusicTrackBank(Idx).TrackType.ToString(), *TrackType->StrValue );
				// if the Track name matches the TrackBank's name
				if( MTB->MusicTrackBank(Idx).TrackType == *TrackType->StrValue )
				{
					if( IsSameTrackAsCurrentlyPlaying( this, MTB->MusicTrackBank(Idx)) == FALSE )
					{
						NextTrackToPlay = MTB->MusicTrackBank(Idx);

						NextTrackToPlayAt = GWorld->GetWorldInfo()->TimeSeconds;
						if( CurrPlayingTrack && CurrPlayingTrack->SoundCue )
						{
							NextTrackToPlayAt += NextTrackToPlay.Params.DelayBetweenOldAndNewTrack;
						}

						// store last used info in WorldInfo to inform newly joining clients
						// and so we can make sure to clean it up on level transitions
						AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
						WorldInfo->LastMusicAction = this;
						WorldInfo->LastMusicTrack = MTB->MusicTrackBank(Idx);

						// Start it ticking
						bLatentExecution = TRUE;
					}
					break;
				}
			}
		}
	}
	// InputLinks(1)=(LinkDesc="CrossFadeTo_Custom")
	else if( InputLinks.Num() > 1 && InputLinks(1).bHasImpulse == TRUE )
	{
		const USeqVar_MusicTrack* MT = FindSingleInstance<USeqVar_MusicTrack*, USeqVar_MusicTrack>( this, TEXT("CustomMusicTrack") );

		// if we have a MusicTrack play it
		if( MT != NULL )
		{
			if( IsSameTrackAsCurrentlyPlaying( this, MT->MusicTrack) == FALSE )
			{
				NextTrackToPlay = MT->MusicTrack;

				NextTrackToPlayAt = GWorld->GetWorldInfo()->TimeSeconds;
				if( CurrPlayingTrack && CurrPlayingTrack->SoundCue )
				{
					NextTrackToPlayAt += NextTrackToPlay.Params.DelayBetweenOldAndNewTrack;
				}

				// store last used info in WorldInfo to inform newly joining clients
				// and so we can make sure to clean it up on level transitions
				AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
				WorldInfo->LastMusicAction = this;
				WorldInfo->LastMusicTrack = MT->MusicTrack;

				// Start it ticking
				bLatentExecution = TRUE;
			}
		}
	}
	// InputLinks(2)=(LinkDesc="CrossFade To Track's FadeOutVolumeLevel")
	else if( InputLinks.Num() > 2 && InputLinks(2).bHasImpulse == TRUE )
	{
		FMusicTrackStruct TrackStruct;
		CrossFadeMusicTracks_FindTrackType( CurrTrackType, this, TrackStruct );

		// replicate to clients
		for (AController* C = GWorld->GetWorldInfo()->ControllerList; C != NULL; C = C->NextController)
		{
			APlayerController* PC = C->GetAPlayerController();
			if (PC != NULL)
			{
				PC->eventClientFadeOutMusicTrack(this, TrackStruct.Params.FadeOutTime, TrackStruct.Params.FadeOutVolumeLevel);
			}
		}
	}
	// 	InputLinks(3)=(LinkDesc="Adjust Volume of Currently Playing Track")
	else if( InputLinks.Num() > 3 && InputLinks(3).bHasImpulse == TRUE )
	{
		AdjustVolumeDuration = 0;
		TArray<FLOAT*> DurationVars;
		GetFloatVars( DurationVars, TEXT("AdjustVolumeDuration") );
		for( INT Idx = 0; Idx < DurationVars.Num(); ++Idx )
		{
			AdjustVolumeDuration = *(DurationVars(Idx));
		}

		AdjustVolumeLevel = 0;
		TArray<FLOAT*> LevelVars;
		GetFloatVars( LevelVars, TEXT("AdjustVolumeLevel") );
		for( INT Idx = 0; Idx < LevelVars.Num(); ++Idx )
		{
			AdjustVolumeLevel = *(LevelVars(Idx)); 
		}

		// replicate to clients
		for (AController* C = GWorld->GetWorldInfo()->ControllerList; C != NULL; C = C->NextController)
		{
			APlayerController* PC = C->GetAPlayerController();
			if (PC != NULL)
			{
				PC->eventClientAdjustMusicTrackVolume(this, AdjustVolumeDuration, AdjustVolumeLevel);
			}
		}
	}
	// bad inputLink
	else
	{
		warnf( TEXT( "USeqAct_CrossFadeMusicTracks::Activated() Bad inputLink" ) );
	}

	Super::Activated();
}

/** crossfades the specified track immediately without any delay*/
void USeqAct_CrossFadeMusicTracks::ClientSideCrossFadeTrackImmediately(const struct FMusicTrackStruct& TrackToPlay)
{
	CrossFadeMusicTracks_SoundPlayer( this, TrackToPlay );
	// finally set the curr track type
	CurrTrackType = TrackToPlay.TrackType;
}

/** crossfades to the specified track */
void USeqAct_CrossFadeMusicTracks::CrossFadeTrack(const FMusicTrackStruct& TrackToPlay)
{
	// we don't ever want to cross fade into the same track (e.g. track foo is playing.  And a request comes in to crossfade to foo. we just want to ignore that request and continue playing track foo)

	// if we have a new track to play!
	if( IsSameTrackAsCurrentlyPlaying( this, TrackToPlay) == FALSE )
	{
		NextTrackToPlay = TrackToPlay;

		NextTrackToPlayAt = GWorld->GetWorldInfo()->TimeSeconds;
		if( CurrPlayingTrack && CurrPlayingTrack->SoundCue )
		{
			NextTrackToPlayAt += NextTrackToPlay.Params.DelayBetweenOldAndNewTrack;
		}

		// Start it ticking
		bLatentExecution = TRUE;
	}
}


/**
 * This will attempt to find a MusicTrackBank that is attached to this action.  If one can not be found
 * then it will look in the PersistentLevel for the named MusicTrackBank.  If it can't find one there
 * then it will do nothing.
 *
 * If it does find one then it will set the attached TrackVar's data on the TrackBank.
 *
 * Basically this allows you to have seamless levels hold all of the data for sounds and set them
 * dynamically on the PersistentLevel.
 **/
void USeqAct_SetMusicTrack::Activated()
{
	USeqVar_MusicTrackBank* MTB = FindMusicTrackBank<USeqAct_SetMusicTrack>( this, TEXT("MusicTrackBank") );

	// no MTB so we need to return
	if( MTB != NULL )
	{
		const USeqVar_MusicTrack* MT = FindSingleInstance<USeqVar_MusicTrack*, USeqVar_MusicTrack>( this, TEXT("MusicTrack") );

		// no MTB so we need to return
		if( MT != NULL )
		{
			SetMusicTrack_Activated_CopyNamedTrackIntoTrackBank( MTB, MT );
		}
		else
		{
		}
	}
	else
	{
	}
}


/** This will find all of the SeqAct_CrossFadeMusicTracks (music managers in the currently loaded levels **/
void USeqAct_CrossFadeMusicTracks::FindAllMusicManagers( USequence* SequenceToLookIn, TArray<USeqAct_CrossFadeMusicTracks*>& OutputVars ) const
{
	// Iterate over Objects in this Sequence
	for( INT i = 0; i < SequenceToLookIn->SequenceObjects.Num(); ++i )
	{
		// Check any USeqAct_CrossFadeMusicTracks
		USeqAct_CrossFadeMusicTracks* CrossFader = Cast<USeqAct_CrossFadeMusicTracks>( SequenceToLookIn->SequenceObjects(i) );
		if( CrossFader != NULL )
		{
			OutputVars.AddUniqueItem(CrossFader);
		}
	}
}


/** This will stop all of the music that is currently playing via SeqAct_CrossFadeMusicTracks SeqActs **/
void USeqAct_CrossFadeMusicTracks::StopAllMusicManagerSounds()
{
	USequence* TmpSeq = GWorld->PersistentLevel->GameSequences(0);
	check(TmpSeq);

	USequence* RootSeq = TmpSeq->GetRootSequence();
	check(RootSeq);

	TArray<USeqAct_CrossFadeMusicTracks*> Vars;
	FindAllMusicManagers( RootSeq, Vars);

	// for each one we found, fadeout the CurrPlayingTrack
	for( INT i = 0; i < Vars.Num(); ++i )
	{
		USeqAct_CrossFadeMusicTracks* CMT = Vars(i);

		if( CMT->CurrPlayingTrack != NULL )
		{
			CMT->CurrPlayingTrack->FadeOut( 0.1f, 0.0f );
			CMT->CurrPlayingTrack = NULL;
		}
	}
}


