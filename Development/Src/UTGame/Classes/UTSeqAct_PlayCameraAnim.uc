/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * UT-specific version of SeqAct_PlayCameraAnim to work with UT-specific camera method.
 */

class UTSeqAct_PlayCameraAnim extends SequenceAction;

/** The anim to play */
var() CameraAnim	AnimToPlay;

/** Time to interpolate in from zero, for smooth starts. */
var()	float		BlendInTime;

/** Time to interpolate out to zero, for smooth finishes. */
var()	float		BlendOutTime;

/** Rate to play.  1.0 is normal. */
var()	float		Rate;

/** Scalar for intensity.  1.0 is normal. */
var()	float		IntensityScale;

defaultproperties
{
	ObjClassVersion=2

	ObjName="Play Camera Animation"
	ObjCategory="Camera"

	BlendInTime=0.f
	BlendOutTime=0.f
	Rate=1.f
	IntensityScale=1.f
}
