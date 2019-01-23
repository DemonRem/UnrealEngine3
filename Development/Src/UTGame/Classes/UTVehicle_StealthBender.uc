/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_StealthBender extends UTStealthVehicle
	native(Vehicle)
	abstract;

/** Override set inputs so get direct information when deployed. */
simulated function SetInputs(float InForward, float InStrafe, float InUp)
{
	if(IsDeployed())
	{
		Super(Vehicle).SetInputs(InForward,InStrafe,InUp);
	}
	else
	{
		Super(UTVehicle).SetInputs(InForward,InStrafe,InUp);
	}
}
defaultproperties
{
	Health=600
	StolenAnnouncementIndex=5
	RespawnTime=45.0

	COMOffset=(x=0.0,y=0.0,z=-55.0)
	UprightLiftStrength=500.0
	UprightTorqueStrength=400.0
	bCanFlip=true
	bSeparateTurretFocus=true
	bHasHandbrake=true
	GroundSpeed=900
	AirSpeed=1000
	MaxSpeed=1300
	ObjectiveGetOutDist=1500.0
	bUseLookSteer=true

	Begin Object Class=UTVehicleSimHellbender Name=SimObject
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	Begin Object Class=UTVehicleHellbenderWheel Name=RRWheel
		BoneName="Rt_Rear_Tire"
		BoneOffset=(X=0.0,Y=42.0,Z=0.0)
		SkelControlName="Rt_Rear_Control"
	End Object
	Wheels(0)=RRWheel

	Begin Object Class=UTVehicleHellbenderWheel Name=LRWheel
		BoneName="Lt_Rear_Tire"
		BoneOffset=(X=0.0,Y=-42.0,Z=0.0)
		SkelControlName="Lt_Rear_Control"
	End Object
	Wheels(1)=LRWheel

	Begin Object Class=UTVehicleHellbenderWheel Name=RFWheel
		BoneName="Rt_Front_Tire"
		BoneOffset=(X=0.0,Y=42.0,Z=0.0)
		SteerFactor=1.0
		SkelControlName="RT_Front_Control"
		HandbrakeLatSlipFactor=1000
	End Object
	Wheels(2)=RFWheel

	Begin Object Class=UTVehicleHellbenderWheel Name=LFWheel
		BoneName="Lt_Front_Tire"
		BoneOffset=(X=0.0,Y=-42.0,Z=0.0)
		SteerFactor=1.0
		SkelControlName="Lt_Front_Control"
		HandbrakeLatSlipFactor=1000
	End Object
	Wheels(3)=LFWheel

	CameraScaleMax=1.0

	TeamBeaconOffset=(z=60.0)
	SpawnRadius=125.0

	bReducedFallingCollisionDamage=true
	ViewPitchMin=-13000
	BaseEyeheight=0
	Eyeheight=0
	bStickDeflectionThrottle=true
}
