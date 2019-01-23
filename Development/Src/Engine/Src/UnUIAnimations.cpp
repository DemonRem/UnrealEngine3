//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================
#include "EnginePrivate.h"
#include "CanvasScene.h"

#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"

#include "EngineSequenceClasses.h"
#include "EngineUISequenceClasses.h"

IMPLEMENT_CLASS(UUIAnimation);
IMPLEMENT_CLASS(UUIAnimationSeq);


/*=========================================================================================
  UUIAnimationSeq - The actual animation sequence
  ========================================================================================= */
void UUIAnimationSeq::ApplyAnimation(UUIObject* TargetWidget, INT TrackIndex, FLOAT Position, INT LFI, INT NFI, struct FUIAnimSeqRef AnimRefInst)
{

	switch ( Tracks(TrackIndex).TrackType )
	{
		case EAT_Opacity:
		{
			FLOAT NewOpacity = Lerp<FLOAT>( Tracks(TrackIndex).KeyFrames(LFI).Data.DestAsFloat, Tracks(TrackIndex).KeyFrames(NFI).Data.DestAsFloat, Position);
			TargetWidget->AnimSetOpacity(NewOpacity);
			break;
		}

		case EAT_Visibility:
		{
			TargetWidget->AnimSetVisibility( Tracks(TrackIndex).KeyFrames(LFI).Data.DestAsFloat>0 );
			break;
		}

		case EAT_Color:
		{
			FLinearColor NewColor = Lerp<FLinearColor>( Tracks(TrackIndex).KeyFrames(LFI).Data.DestAsColor, Tracks(TrackIndex).KeyFrames(NFI).Data.DestAsColor, Position );
			TargetWidget->AnimSetColor(NewColor);
			break;
		}

		case EAT_Rotation:
		{
			FVector Start = Tracks(TrackIndex).KeyFrames(LFI).Data.DestAsRotator.Vector();
			FVector End = Tracks(TrackIndex).KeyFrames(NFI).Data.DestAsRotator.Vector();
			FVector RotVec = Lerp<FVector>(Start, End, Position);
			TargetWidget->AnimSetRotation( RotVec.Rotation() );
			break;
		}

		case EAT_RelRotation:
		{
			FVector Start = Tracks(TrackIndex).KeyFrames(LFI).Data.DestAsRotator.Vector();
			FVector End = Tracks(TrackIndex).KeyFrames(NFI).Data.DestAsRotator.Vector();
			FVector RotVec = Lerp<FVector>(Start, End, Position);
			FRotator FinalRotation = RotVec.Rotation() + AnimRefInst.InitialRotation;

			TargetWidget->AnimSetRotation( FinalRotation );
			break;
		}

		case EAT_Position:
		{
			FVector NewPos = Lerp<FVector>( Tracks(TrackIndex).KeyFrames(LFI).Data.DestAsVector, Tracks(TrackIndex).KeyFrames(NFI).Data.DestAsVector, Position );
			debugf(TEXT("#### %f %f %f %f"),NewPos.X, Tracks(TrackIndex).KeyFrames(LFI).Data.DestAsVector.X, Tracks(TrackIndex).KeyFrames(NFI).Data.DestAsVector.X, Position);
			TargetWidget->AnimSetPosition(NewPos);
			break;
		}

		case EAT_RelPosition:
		{
			FVector NewPos = Lerp<FVector>( Tracks(TrackIndex).KeyFrames(LFI).Data.DestAsVector, Tracks(TrackIndex).KeyFrames(NFI).Data.DestAsVector, Position );
			TargetWidget->AnimSetRelPosition(NewPos,AnimRefInst.InitialRenderOffset);
			break;
		}
		// TODO - Add suport for a notify track

	}
}


