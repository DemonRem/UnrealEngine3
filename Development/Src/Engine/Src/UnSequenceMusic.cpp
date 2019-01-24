/*=============================================================================
   UnSequenceMusic.cpp: Gameplay Music Sequence native code
   Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineSequenceClasses.h"
#include "EngineAudioDeviceClasses.h"

IMPLEMENT_CLASS(UMusicTrackDataStructures);
IMPLEMENT_CLASS(USeqAct_PlayMusicTrack);

void USeqAct_PlayMusicTrack::Activated()
{
	AWorldInfo *WorldInfo = GWorld->GetWorldInfo();
	if (WorldInfo != NULL)
	{
		WorldInfo->UpdateMusicTrack(MusicTrack);
	}
}

void USeqAct_PlayMusicTrack::PreSave()
{
	if (GIsCooking && (GCookingTarget & UE3::PLATFORM_Mobile))
	{
		if (!MusicTrack.MP3Filename.IsEmpty())
		{
			// Remove reference so it does not get cooked to save some space.
			MusicTrack.TheSoundCue = NULL;
		}
	}
}


