/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTEmitCameraEffect extends Emitter
	abstract
	native;

/** How far in front of the camera this emitter should live. */
var() protected float DistFromCamera;

/** Camera this emitter is attached to, will be notified when emitter is destroyed */
var private UTPlayerController Cam;


simulated event PostBeginPlay()
{
	ParticleSystemComponent.SetDepthPriorityGroup(SDPG_Foreground);
	super.PostBeginPlay();
}

function Destroyed()
{
	Cam.RemoveCameraBlood(self);
}

/** Tell the emitter what camera it is attached to. */
function RegisterCamera( UTPlayerController inCam )
{
	Cam = inCam;
}

/** Given updated camera information, adjust this effect to display appropriately. */
native function UpdateLocation( const out vector CamLoc, const out rotator CamRot, float CamFOVDeg );

defaultproperties
{
	Begin Object Name=ParticleSystemComponent0
		//bOnlyOwnerSee=true
	End Object

//CameraBlood.FX.P_CameraBlood
//CameraBlood.FX.P_FX_BioDarkGoop_Camera
//CameraBlood.FX.P_FX_BioGoop_Camera

//CameraBlood.FX.P_FX_PurpleIchor_Camera
//CameraBlood.FX.P_FX_WaterRun_Camera
//CameraBlood.FX.P_ShotgunBlood


	// makes sure I tick after the camera
	TickGroup=TG_DuringAsyncWork

	DistFromCamera=90

	// just using this to help and debug 1-frame-lag issues
	//Begin Object Class=SkeletalMeshComponent Name=WeaponMesh
	//	CollideActors=FALSE
	//	bCastDynamicShadow=TRUE
	//	MotionBlurScale=0.2
	//	SkeletalMesh=SkeletalMesh'Locust_Hammerburst.Locust_Hammerburst'
	//	PhysicsAsset=PhysicsAsset'Locust_Hammerburst.Locust_Hammerburst_Physics'
	//	AnimTreeTemplate=AnimTree'Locust_Hammerburst.AT_HammerBurst'
	//	AnimSets(0)=AnimSet'Locust_Hammerburst.Locust_Hammerburst_Anims'
	//End Object
	//Components.Add(WeaponMesh);

	LifeSpan=10.0f;

	bDestroyOnSystemFinish=true
	bNetInitialRotation=true
	bNoDelete=false

}



