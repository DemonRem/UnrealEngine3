/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicle_Manta extends UTHoverVehicle
	native(Vehicle)
	abstract;

var(Movement)	float	JumpForceMag;

/** How far down to trace to check if we can jump */
var(Movement)   float   JumpCheckTraceDist;

var     float	JumpDelay, LastJumpTime;

var(Movement)   float   DuckForceMag;

var repnotify bool bDoBikeJump;
var repnotify bool bHoldingDuck;
var		bool							bPressingAltFire;

var soundcue JumpSound;
var soundcue DuckSound;

var float BladeBlur, DesiredBladeBlur;
/** if >= 0, index in VehicleEffects array for fan effect that gets its MantaFanSpin parameter set to BladeBlur */
var int FanEffectIndex;
/** parameter name for the fan blur, set to BladeBlur */
var name FanEffectParameterName;

/** Suspension height when manta is being driven around normally */
var(Movement) protected float FullWheelSuspensionTravel;

/** Suspension height when manta is crouching */
var(Movement) protected float CrouchedWheelSuspensionTravel;

/** controls how fast to interpolate between various suspension heights */
var(Movement) protected float SuspensionTravelAdjustSpeed;

/** Suspension stiffness when manta is being driven around normally */
var(Movement) protected float FullWheelSuspensionStiffness;

/** Suspension stiffness when manta is crouching */
var(Movement) protected float CrouchedWheelSuspensionStiffness;

/** Adjustment for bone offset when changing suspension */
var  protected float BoneOffsetZAdjust;

/** max speed while crouched */
var(Movement) float CrouchedAirSpeed;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

replication
{
	if (!bNetOwner && Role == ROLE_Authority)
		bDoBikeJump, bHoldingDuck;
}

simulated function bool OverrideBeginFire(byte FireModeNum)
{
	if (FireModeNum == 1)
	{
		bPressingAltFire = true;
		return true;
	}

	return false;
}

simulated function bool OverrideEndFire(byte FireModeNum)
{
	if (FireModeNum == 1)
	{
		bPressingAltFire = false;
		return true;
	}

	return false;
}

function PossessedBy(Controller C, bool bVehicleTransition)
{
	super.PossessedBy(C, bVehicleTransition);

	// reset jump/duck properties
	bHoldingDuck = false;
	LastJumpTime = 0;
	bDoBikeJump = false;
	bPressingAltFire = false;
}

simulated event MantaJumpEffect();

simulated event MantaDuckEffect();

//========================================
// AI Interface

function byte ChooseFireMode()
{
	if (Pawn(Controller.Focus) != None
		&& Vehicle(Controller.Focus) == None
		&& Controller.MoveTarget == Controller.Focus
		&& Controller.InLatentExecution(Controller.LATENT_MOVETOWARD)
		&& VSize(Controller.FocalPoint - Location) < 800
		&& Controller.LineOfSightTo(Controller.Focus) )
	{
		return 1;
	}

	return 0;
}

function bool Dodge(eDoubleClickDir DoubleClickMove)
{
	Rise = 1;
	return true;
}

function bool TooCloseToAttack(Actor Other)
{
	if ( UTPawn(Other) != None )
		return false;
	return super.TooCloseToAttack(Other);
}

function IncomingMissile(Projectile P)
{
	local UTBot B;

	B = UTBot(Controller);
	if (B != None && B.Skill > 4.0 + 4.0 * FRand() && VSize(P.Location - Location) < VSize(P.Velocity))
	{
		DriverLeave(false);
	}
	else
	{
		Super.IncomingMissile(P);
	}
}

// AI hint
function bool FastVehicle()
{
	return true;
}

simulated function DriverLeft()
{
	bPressingAltFire = false;

	Super.DriverLeft();
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bDoBikeJump')
	{
		MantaJumpEffect();
	}
	else if (VarName == 'bHoldingDuck')
	{
		MantaDuckEffect();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function float GetChargePower()
{
	return FClamp( (WorldInfo.TimeSeconds - LastJumpTime), 0, JumpDelay)/JumpDelay;
}

simulated event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
				const out CollisionImpactData RigidCollisionData, int ContactIndex )
{
	// only process rigid body collision if not hitting ground
	if ( Abs(RigidCollisionData.ContactInfos[0].ContactNormal.Z) < WalkableFloorZ )
	{
		super.RigidBodyCollision(HitComponent, OtherComponent, RigidCollisionData, ContactIndex);
	}
}


defaultproperties
{
	Health=200
	MeleeRange=-100.0
	ExitRadius=160.0

	COMOffset=(x=0.0,y=0.0,z=0.0)
	UprightLiftStrength=30.0
	UprightTorqueStrength=30.0
	bCanFlip=true
	JumpForceMag=7000.0
	JumpDelay=3.0
	DuckForceMag=-350.0
	JumpCheckTraceDist=175.0
	FullWheelSuspensionTravel=145
	CrouchedWheelSuspensionTravel=100
	FullWheelSuspensionStiffness=20.0
	CrouchedWheelSuspensionStiffness=40.0
	SuspensionTravelAdjustSpeed=100
	BoneOffsetZAdjust=45.0
	CustomGravityScaling=0.8

	AirSpeed=1800.0
	GroundSpeed=1500.0
	CrouchedAirSpeed=1200.0
	FullAirSpeed=1800.0
	bCanCarryFlag=false
	bFollowLookDir=True
	bTurnInPlace=True
	bScriptedRise=True
	bCanStrafe=True
	ObjectiveGetOutDist=750.0
	MaxDesireability=0.6
	SpawnRadius=125.0
	MomentumMult=3.0

	bStayUpright=true
	StayUprightRollResistAngle=5.0
	StayUprightPitchResistAngle=5.0
	StayUprightStiffness=450
	StayUprightDamping=20

	Begin Object Class=UTVehicleSimHover Name=SimObject
		WheelSuspensionStiffness=20.0
		WheelSuspensionDamping=1.0
		WheelSuspensionBias=0.0
		MaxThrustForce=325.0
		MaxReverseForce=325.0
		LongDamping=0.3
		MaxStrafeForce=400.0
		LatDamping=0.3
		MaxRiseForce=0.0
		UpDamping=0.0
		TurnTorqueFactor=2500.0
		TurnTorqueMax=1000.0
		TurnDamping=0.25
		MaxYawRate=100000.0
		PitchTorqueFactor=200.0
		PitchTorqueMax=18.0
		PitchDamping=0.1
		RollTorqueTurnFactor=1000.0
		RollTorqueStrafeFactor=110.0
		RollTorqueMax=500.0
		RollDamping=0.2
		MaxRandForce=3.0
		RandForceInterval=0.75
		bAllowZThrust=true
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	Begin Object Class=UTHoverWheel Name=RThruster
		BoneName="Engine"
		BoneOffset=(X=-50.0,Y=100.0,Z=-200.0)
		WheelRadius=10
		SuspensionTravel=145
		bPoweredWheel=false
		LongSlipFactor=0.0
		LatSlipFactor=0.0
		HandbrakeLongSlipFactor=0.0
		HandbrakeLatSlipFactor=0.0
		SteerFactor=1.0
		bHoverWheel=true
	End Object
	Wheels(0)=RThruster

	Begin Object Class=UTHoverWheel Name=LThruster
		BoneName="Engine"
		BoneOffset=(X=-50.0,Y=-100.0,Z=-200.0)
		WheelRadius=10
		SuspensionTravel=145
		bPoweredWheel=false
		LongSlipFactor=0.0
		LatSlipFactor=0.0
		HandbrakeLongSlipFactor=0.0
		HandbrakeLatSlipFactor=0.0
		SteerFactor=1.0
		bHoverWheel=true
	End Object
	Wheels(1)=LThruster

	Begin Object Class=UTHoverWheel Name=FThruster
		BoneName="Engine"
		BoneOffset=(X=80.0,Y=0.0,Z=-200.0)
		WheelRadius=10
		SuspensionTravel=145
		bPoweredWheel=false
		LongSlipFactor=0.0
		LatSlipFactor=0.0
		HandbrakeLongSlipFactor=0.0
		HandbrakeLatSlipFactor=0.0
		SteerFactor=1.0
		bHoverWheel=true
	End Object
	Wheels(2)=FThruster

	bAttachDriver=true
	bDriverIsVisible=true

	bHomingTarget=true

	BaseEyeheight=110
	Eyeheight=110

	bLightArmor=true
	DefaultFOV=90
	CameraLag=0.02
}
