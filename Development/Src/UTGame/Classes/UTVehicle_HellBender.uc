/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_HellBender extends UTVehicle
	native(Vehicle)
	abstract;

var repnotify vector TurretFlashLocation;
var	repnotify byte TurretFlashCount;
var repnotify rotator TurretWeaponRotation;
var byte TurretFiringMode;
/** Sound played whenever the suspension shifts suddenly */
var AudioComponent SuspensionShiftSound;
/** Time the last SuspensionShift was played.*/
var float LastSuspensionShiftTime;
var name ExhaustEffectName;
/** The Template of the Beam to use */

/** Internal variable.  Maintains brake light state to avoid extraMatInst calls.	*/
var bool bBrakeLightOn;

/** Internal variable.  Maintains reverse light state to avoid extra MatInst calls.	*/
var bool bReverseLightOn;

/** material parameter that should be modified to turn the brake lights on and off */
var name BrakeLightParameterName;
/** material parameter that should be modified to turn the reverse lights on and off */
var name ReverseLightParameterName;

var ParticleSystem BeamTemplate;

replication
{
	if (bNetDirty)
		TurretFlashLocation;
	if (!IsSeatControllerReplicationViewer(1))
		TurretFlashCount, TurretWeaponRotation;
}

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

simulated event SuspensionHeavyShift(float delta)
{
	if(LastSuspensionShiftTime + 0.4f < WorldInfo.TimeSeconds && delta>0 && WorldInfo.NetMode != NM_DEDICATEDSERVER)
	{
		SuspensionShiftSound.VolumeMultiplier = FMin((delta-HeavySuspensionShiftPercent)/(1.0-HeavySuspensionShiftPercent),1.0);
		SuspensionShiftSound.Play();
		LastSuspensionShiftTime=WorldInfo.TimeSeconds;
	}
}

simulated event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
					const out CollisionImpactData RigidCollisionData, int ContactIndex )
{
	// only process rigid body collision if not hitting ground, or hitting at an angle
	if ( (Abs(RigidCollisionData.ContactInfos[0].ContactNormal.Z) < WalkableFloorZ)
		|| (Abs(RigidCollisionData.ContactInfos[0].ContactNormal dot vector(Rotation)) > 0.8)
		|| (VSizeSq(Mesh.GetRootBodyInstance().PreviousVelocity) * GetCollisionDamageModifier(RigidCollisionData.ContactInfos) > 5) )
	{
		super.RigidBodyCollision(HitComponent, OtherComponent, RigidCollisionData, ContactIndex);
	}
}

defaultproperties
{

	Health=600
	StolenAnnouncementIndex=5

	COMOffset=(x=0.0,y=0.0,z=-55.0)
	UprightLiftStrength=500.0
	UprightTorqueStrength=400.0
	bCanFlip=true
	bSeparateTurretFocus=true
	bHasHandbrake=true
	GroundSpeed=800
	AirSpeed=950
	MaxSpeed=1050
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
		LongSlipFactor=2.0
		LatSlipFactor=3.0
		HandbrakeLongSlipFactor=0.8
		HandbrakeLatSlipFactor=0.8
	End Object
	Wheels(2)=RFWheel

	Begin Object Class=UTVehicleHellbenderWheel Name=LFWheel
		BoneName="Lt_Front_Tire"
		BoneOffset=(X=0.0,Y=-42.0,Z=0.0)
		SteerFactor=1.0
		SkelControlName="Lt_Front_Control"
		LongSlipFactor=2.0
		LatSlipFactor=3.0
		HandbrakeLongSlipFactor=0.8
		HandbrakeLatSlipFactor=0.8
	End Object
	Wheels(3)=LFWheel

	CameraScaleMax=1.0

	SpawnRadius=125.0

	bReducedFallingCollisionDamage=true
	ViewPitchMin=-13000
	BaseEyeheight=0
	Eyeheight=0
	bStickDeflectionThrottle=true
	HeavySuspensionShiftPercent=0.2
}
