/*=============================================================================
	AnimationCompressionAlgorithm_RemoveEverySecondKey.cpp: Keyframe reduction algorithm that simply removes every second key.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "AnimationUtils.h"

IMPLEMENT_CLASS(UAnimationCompressionAlgorithm_RemoveEverySecondKey);

/**
 * Keyframe reduction algorithm that simply removes every second key.
 *
 * @return		TRUE if the keyframe reduction was successful.
 */
void UAnimationCompressionAlgorithm_RemoveEverySecondKey::DoReduction(UAnimSequence* AnimSeq, USkeletalMesh* SkelMesh, const TArray<FBoneData>& BoneData)
{
	const INT NumTracks = AnimSeq->RawAnimData.Num();

	AnimSeq->TranslationData.Empty( NumTracks );
	AnimSeq->RotationData.Empty( NumTracks );
	AnimSeq->TranslationData.AddZeroed( NumTracks );
	AnimSeq->RotationData.AddZeroed( NumTracks );

	// If bStartAtSecondKey is TRUE, remove keys 1,3,5,etc.
	// If bStartAtSecondKey is FALSE, remove keys 0,2,4,etc.
	const INT StartIndex = bStartAtSecondKey ? 1 : 0;

	for ( INT TrackIndex = 0 ; TrackIndex < NumTracks ; ++TrackIndex )
	{
		const FRawAnimSequenceTrack& RawTrack	= AnimSeq->RawAnimData(TrackIndex);
		FTranslationTrack&	TranslationTrack	= AnimSeq->TranslationData(TrackIndex);
		FRotationTrack&		RotationTrack		= AnimSeq->RotationData(TrackIndex);

		if ( RawTrack.PosKeys.Num() >= MinKeys )
		{
			// Remove every second key.
			for ( INT PosIndex = StartIndex ; PosIndex < RawTrack.PosKeys.Num() ; PosIndex += 2 )
			{
				TranslationTrack.PosKeys.AddItem( RawTrack.PosKeys(PosIndex) );
			}
		}
		else
		{
			// There are too few keys, so simply copy the data.
			for ( INT PosIndex = 0 ; PosIndex < RawTrack.PosKeys.Num() ; ++PosIndex )
			{
				TranslationTrack.PosKeys.AddItem( RawTrack.PosKeys(PosIndex) );
			}
		}

		if ( RawTrack.RotKeys.Num() >= MinKeys )
		{
			// Remove every second key.
			for ( INT RotIndex = StartIndex ; RotIndex < RawTrack.RotKeys.Num() ; RotIndex += 2 )
			{
				RotationTrack.RotKeys.AddItem( RawTrack.RotKeys(RotIndex) );
			}
		}
		else
		{
			// There are too few keys, so simply copy the data.
			for ( INT RotIndex = 0 ; RotIndex < RawTrack.RotKeys.Num() ; ++RotIndex )
			{
				RotationTrack.RotKeys.AddItem( RawTrack.RotKeys(RotIndex) );
			}
		}

		// Set times for the translation track.
		if ( TranslationTrack.PosKeys.Num() > 1 )
		{
			const FLOAT PosFrameInterval = AnimSeq->SequenceLength / static_cast<FLOAT>(TranslationTrack.PosKeys.Num()-1);
			for ( INT PosIndex = 0 ; PosIndex < TranslationTrack.PosKeys.Num() ; ++PosIndex )
			{
				TranslationTrack.Times.AddItem( PosIndex * PosFrameInterval );
			}
		}
		else
		{
			TranslationTrack.Times.AddItem( 0.f );
		}

		// Set times for the rotation track.
		if ( RotationTrack.RotKeys.Num() > 1 )
		{
			const FLOAT RotFrameInterval = AnimSeq->SequenceLength / static_cast<FLOAT>(RotationTrack.RotKeys.Num()-1);
			for ( INT RotIndex = 0 ; RotIndex < RotationTrack.RotKeys.Num() ; ++RotIndex )
			{
				RotationTrack.Times.AddItem( RotIndex * RotFrameInterval );
			}
		}
		else
		{
			RotationTrack.Times.AddItem( 0.f );
		}

		// Trim unused memory.
		TranslationTrack.PosKeys.Shrink();
		TranslationTrack.Times.Shrink();
		RotationTrack.RotKeys.Shrink();
		RotationTrack.Times.Shrink();
	}
}
