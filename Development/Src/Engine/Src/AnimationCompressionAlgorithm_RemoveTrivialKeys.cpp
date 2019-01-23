/*=============================================================================
	AnimationCompressionAlgorithm_RemoveTrivialKeys.cpp: Removes trivial frames from the raw animation data.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "AnimationUtils.h"

IMPLEMENT_CLASS(UAnimationCompressionAlgorithm_RemoveTrivialKeys);

/**
 * Removes trivial frames -- frames of tracks when position or orientation is constant
 * over the entire animation -- from the raw animation data.  If both position and rotation
 * go down to a single frame, the time is stripped out as well.
 *
 * @return		TRUE if the keyframe reduction was successful.
 */
void UAnimationCompressionAlgorithm_RemoveTrivialKeys::DoReduction(UAnimSequence* AnimSeq, USkeletalMesh* SkelMesh, const TArray<FBoneData>& BoneData)
{
	for( INT TrackIndex = 0 ; TrackIndex < AnimSeq->RawAnimData.Num() ; ++TrackIndex )
	{
		FRawAnimSequenceTrack& RawTrack = AnimSeq->RawAnimData(TrackIndex);

		// Only bother doing anything if we have some keys!
		if(RawTrack.KeyTimes.Num() > 0)
		{
			check( RawTrack.PosKeys.Num() == 1 || RawTrack.PosKeys.Num() == RawTrack.KeyTimes.Num() );
			check( RawTrack.RotKeys.Num() == 1 || RawTrack.RotKeys.Num() == RawTrack.KeyTimes.Num() );

			// Check variation of position keys
			if( RawTrack.PosKeys.Num() > 1 )
			{
				const FVector& FirstPos = RawTrack.PosKeys(0);
				UBOOL bFramesIdentical = TRUE;
				for(INT j=1; j<RawTrack.PosKeys.Num() ; j++)
				{
					if( (FirstPos - RawTrack.PosKeys(j)).Size() > MaxPosDiff )
					{
						bFramesIdentical = FALSE;
						break;
					}
				}

				// If all keys are the same, remove all but first frame
				if(bFramesIdentical)
				{
					RawTrack.PosKeys.Remove(1, RawTrack.PosKeys.Num()- 1);
					RawTrack.PosKeys.Shrink();
				}
			}

			// Check variation of rotational keys
			if(RawTrack.RotKeys.Num() > 1)
			{
				const FQuat& FirstRot = RawTrack.RotKeys(0);
				UBOOL bFramesIdentical = TRUE;
				for(INT j=1; j<RawTrack.RotKeys.Num() ; j++)
				{
					if( FQuatError(FirstRot, RawTrack.RotKeys(j)) > MaxAngleDiff )
					{
						bFramesIdentical = FALSE;
						break;
					}
				}

				if(bFramesIdentical)
				{
					RawTrack.RotKeys.Remove(1, RawTrack.RotKeys.Num()- 1);
					RawTrack.RotKeys.Shrink();
				}			
			}

			// If both pos and rot keys are down to just 1 key - clear out the time keys
			if(RawTrack.PosKeys.Num() == 1 && RawTrack.RotKeys.Num() == 1)
			{
				RawTrack.KeyTimes.Remove(1, RawTrack.KeyTimes.Num() - 1);
				RawTrack.KeyTimes.Shrink();
			}
		}
	}

	AnimSeq->SeparateRawDataToTracks( AnimSeq->RawAnimData, AnimSeq->SequenceLength, AnimSeq->TranslationData, AnimSeq->RotationData );
}
