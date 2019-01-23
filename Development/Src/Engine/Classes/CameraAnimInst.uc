
/**
 *	CameraAnim: defines a pre-packaged animation to be played on a camera.
 * 	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class CameraAnimInst extends Object
	notplaceable
	native;

/** which CameraAnim this is an instance of */
var CameraAnim					CamAnim;

/** the InterpGroupInst used to do the interpolation */
var protected instanced InterpGroupInst	InterpGroupInst;

/** Current time for the animation */
var protected transient float	CurTime;
/** True if the animation should loop, false otherwise. */
var protected transient bool	bLooping;
/** True if the animation has finished, false otherwise. */
var transient bool				bFinished;

/** Time to interpolate in from zero, for smooth starts. */
var protected float				BlendInTime;
/** Time to interpolate out to zero, for smooth finishes. */
var protected float				BlendOutTime;
/** True if currently blending in. */
var protected transient bool	bBlendingIn;
/** True if currently blending out. */
var protected transient bool	bBlendingOut;
/** Current time for the blend-in.  I.e. how long we have been blending. */
var protected transient float	CurBlendInTime;
/** Current time for the blend-out.  I.e. how long we have been blending. */
var protected transient float	CurBlendOutTime;

/** Multiplier for playback rate.  1.0 = normal. */
var protected float				PlayRate;

/** "Intensity" scalar. */
var protected float				PlayScale;

/* Number in range [0..1], controlling how much this influence this instance should have. */
var float						CurrentBlendWeight;

/** cached movement track from the currently playing anim so we don't have to go find it every frame */
var InterpTrackMove MoveTrack;
var InterpTrackInstMove MoveInst;

/**
 * Starts this instance playing the specified CameraAnim.
 *
 * CamAnim:		The animation that should play on this instance.
 * CamActor:	The Actor that will be modified by this animation.
 * InRate:		How fast to play the animation.  1.f is normal.
 * InScale:		How intense to play the animation.  1.f is normal.
 * InBlendInTime:	Time over which to linearly ramp in.
 * InBlendInTime:	Time over which to linearly ramp out.
 * bInLoop:			Whether or not to loop the animation.
 * bRandomStartTime:	Whether or not to choose a random time to start playing.  Only really makes sense for bLoop = TRUE;
 */
native final function Play(CameraAnim Anim, Actor CamActor, float InRate, float InScale, float InBlendInTime, float InBlendOutTime, bool bInLoop, bool bRandomStartTime);

/** advances the animation by the specified time - updates any modified interp properties, moves the group actor, etc */
native function AdvanceAnim(float DeltaTime, bool bJump);

/** Stops this instance playing whatever animation it is playing. */
native final function Stop(optional bool bImmediate);

defaultproperties
{
	bFinished=true
	PlayRate=1.f

	Begin Object Class=InterpGroupInst Name=InterpGroupInst0
	End Object
	InterpGroupInst=InterpGroupInst0
}
