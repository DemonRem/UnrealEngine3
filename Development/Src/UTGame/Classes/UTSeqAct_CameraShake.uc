/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTSeqAct_CameraShake extends SequenceAction;

var() UTPlayerController.ViewShakeInfo CameraShake;

defaultproperties
{
	ObjCategory="Camera"
	ObjName="Camera Shake"

	CameraShake=(RotMag=(Z=250),RotRate=(Z=2500),RotTime=6,OffsetMag=(Z=15),OffsetRate=(Z=200),OffsetTime=10)
}
