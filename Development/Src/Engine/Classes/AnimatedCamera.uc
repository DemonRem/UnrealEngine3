
/**
* 	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
*/
class AnimatedCamera extends Camera
	notplaceable
	native;

const MAX_ACTIVE_CAMERA_ANIMS = 8;

/** Pool of anim instance objects available with which to play camera animations */
var protected CameraAnimInst			AnimInstPool[MAX_ACTIVE_CAMERA_ANIMS];

/** Array of anim instances that are currently playing and in-use */
var protected array<CameraAnimInst>		ActiveAnims;
/** Array of anim instances that are not playing and available */
var protected array<CameraAnimInst>		FreeAnims;

/** Internal.  Receives the output of individual camera animations. */
var protected transient DynamicCameraActor AnimCameraActor;
/** Internal.  Collects and blends the output of all of the camera animations, before being applied to the camera */
var protected transient DynamicCameraActor AccumulatorCameraActor;

cpptext
{
protected:
	UCameraAnimInst* AllocCameraAnimInst();
	void ReleaseCameraAnimInst(UCameraAnimInst* Inst);
	
	static void AddScaledInterpProperties(class ACameraActor const* SrcCam, class ACameraActor* DestCam, FLOAT Scale);
	static void ResetInterpProperties(class ACameraActor *CamActor);
	void ApplyInterpPropertiesFromCameraActor(class ACameraActor const* CamActor, FTPOV& OutPOV);

	void ResetTempCameraActor(class ACameraActor* CamActor) const;
public:

	virtual void ModifyPostProcessSettings(FPostProcessSettings& PPSettings) const;
};


function PostBeginPlay()
{
	local int Idx;

	super.PostBeginPlay();

	// create CameraAnimInsts in pool
	for (Idx=0; Idx<MAX_ACTIVE_CAMERA_ANIMS; ++Idx)
	{
		AnimInstPool[Idx] = new(Self) class'CameraAnimInst';
		
		// add everything to the free list initially
		FreeAnims[Idx] = AnimInstPool[Idx];			
	}

	// spawn the two temp CameraActors
	AnimCameraActor = Spawn(class'DynamicCameraActor', self,,,,, TRUE);
	AccumulatorCameraActor = Spawn(class'DynamicCameraActor', self,,,,, TRUE);
}

event Destroyed()
{
	// clean up the temp camera actors
	AnimCameraActor.Destroy();
	AccumulatorCameraActor.Destroy();
}

event ApplyCameraModifiers(float DeltaTime, out TPOV OutPOV)
{
	super.ApplyCameraModifiers(DeltaTime, OutPOV);

	// will handle this in native code
	ApplyCameraModifiersNative(DeltaTime, OutPOV);
}

/** Native version of ApplyCameraModifiers */
simulated private native function ApplyCameraModifiersNative(float DeltaTime, out TPOV OutPOV);

// @fixme: need to return linkage to a particular instance?
/** Play the indicated CameraAnim on this camera. */
simulated native function bool PlayCameraAnim(CameraAnim Anim, optional float Rate=1.f, optional float Scale=1.f, optional float BlendInTime, optional float BlendOutTime, optional bool bLoop, optional bool bRandomStartTime);

/** 
 * Stop playing all instances of the indicated CameraAnim. 
 * bImmediate: TRUE to stop it right now, FALSE to blend it out over BlendOutTime.
 */
simulated native function StopCameraAnim(CameraAnim Anim, optional bool bImmediate);


defaultproperties
{
}
