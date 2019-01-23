/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_PlayCameraAnim extends SeqAct_Latent
	native(Sequence);

/** Reference to CameraAnim to play.  Note that specified Pawn/Controller must be using an AnimatedCamera. */
var()	CameraAnim		CameraAnim;

/** True to loop the animation, false otherwise. */
var()	bool			bLoop;

/** Time to interpolate in from zero, for smooth starts. */
var()	float			BlendInTime;

/** Time to interpolate out to zero, for smooth finishes. */
var()	float			BlendOutTime;

/** Rate to play.  1.0 is normal. */
var()	float			Rate;

/** Scalar for intensity.  1.0 is normal. */
var()	float			IntensityScale;

/** True to start the animation at a random time (good for things like looping shakes) */
var()	bool			bRandomStartTime;

/** Internal.  True if this action was stopped via the stop input, false if still playing or it stopped naturally. */
var protected transient bool		bStopped;

/** Internal.  Time remaining in the animation, in seconds.  Used to fire the Finished output at the appropriate time. */
var protected transient float		AnimTimeRemaining;

cpptext
{
	void Activated();
	UBOOL UpdateOp(FLOAT deltaTime);
	void DeActivated();
};

defaultproperties
{
	ObjClassVersion=1

	ObjName="Play CameraAnim"
	ObjCategory="Camera"

	InputLinks(0)=(LinkDesc="Play")
	InputLInks(1)=(LinkDesc="Stop")

	OutputLinks(0)=(LinkDesc="Out")
	OutputLinks(1)=(LinkDesc="Finished")
	OutputLinks(2)=(LinkDesc="Stopped")

	BlendInTime=0.2f
	BlendOutTime=0.2f
	Rate=1.f
	IntensityScale=1.f
}



