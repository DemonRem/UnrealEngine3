	/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicle_Viper extends UTHoverVehicle
	native(Vehicle)
	abstract;

var(Movement)   float   JumpDuration;
var(Movement)	float	JumpForceMag;

/** How far down to trace to check if we can jump */
var(Movement)   float   JumpCheckTraceDist;

var     float   JumpCountdown;
var     float	JumpDelay, LastJumpTime;

var repnotify bool bDoBikeJump;
var repnotify bool bHoldingDuck;
var		bool							bPressingAltFire;

var soundcue JumpSound;
var soundcue DuckSound;

// TEMP for Phil testing
var(Movement) bool bStopWhenGlide;
var(Movement) float GlideAirSpeed;
var(Movement) float GlideSpeedReductionRate;

/** CustomGravityScaling setting when wings not extended */
var(Movement) float NormalGravity;

/** CustomGravityScaling setting when wings are extended */
var(Movement) float GlidingGravity;

/** Self destruct effect properties */
var class<UTDamageType> SelfDestructDamageType;
var SoundCue SelfDestructSoundCue;
var SoundCue EjectSoundCue;

/** name of skel control to perform the self destruct spin move*/
var name SelfDestructSpinName;

/** Whether or not the self destruct sequence is in progress*/
var bool bSelfDestructInProgress;

/** who gets credit for self destruct damage caused */
var Controller SelfDestructInstigator;

/** when self destruct was engaged */
var float DestructStartTime;

/** how long self destruct lasts before blowing up if no targets found */
var float MaxDestructDuration;

/** How long Rise must be > 0 to self destruct*/
var float TimeToRiseForSelfDestruct;

/** flag for whether or not self destruct will go off if the vehicle is left */
var bool bSelfDestructReady;

/** sound to be played when Self Destruct is armed */
var SoundCue SelfDestructReadySnd;

/** replicated property identifies that self destruct is engaged */
var repnotify bool bSelfDestructArmed;

/** replicated vector specifies force to apply to self-destructing viper (in direction ejecting driver was last looking when he left) */
var vector BoostDir;

/** magnitude to apply when determining BoostDir */
var float BoostForce;

/** This is the index of the effect that controls the jet exhause */
var int ExhaustIndex;

/** effect played when self destructing */
var ParticleSystem SelfDestructEffectTemplate;
/** the actual effect, var-ed so we can end it more appropriately*/
var ParticleSystemComponent SelfDestructEffect;

var name ExhaustParamName;

/** anim node that controls the gliding animation. Child 0 should be the normal anim, Child 1 the gliding anim. */
var AnimNodeBlend GlideBlend;
/** how long it takes to switch between gliding animation states */
var float GlideBlendTime;

/** sound played on sudden curves */
var AudioComponent  CurveSound;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		BoostDir;
	if (bNetDirty && !bNetOwner && Role == ROLE_Authority)
		bSelfDestructArmed;
	if (!bNetOwner && Role == ROLE_Authority)
		bDoBikeJump, bHoldingDuck;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	GlideBlend = AnimNodeBlend(Mesh.FindAnimNode('GlideNode'));
}

simulated function SetVehicleEffectParms(name TriggerName, ParticleSystemComponent PSC)
{
	if (TriggerName == 'MantaOnFire')
	{
		PSC.SetFloatParameter('smokeamount', 0.95);
		PSC.SetFloatParameter('fireamount', 0.95);
	}
	else
	{
		Super.SetVehicleEffectParms(TriggerName, PSC);
	}
}

simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	// viper is fragile so it takes momentum even from weapons that don't usually impart it
	if ( (DamageType == class'UTDmgType_Enforcer') && !IsZero(HitLocation) )
	{
		Momentum = (Location - HitLocation) * float(Damage) * 20.0;
	}
	Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
}

function bool Died(Controller Killer, class<DamageType> DamageType, vector HitLocation)
{
	VehicleEvent('MantaNormal');
	return Super.Died(Killer,DamageType,HitLocation);
}

simulated function DrivingStatusChanged()
{
	if ( !bDriving )
	{
		VehicleEvent('CrushStop');
	}
	Super.DrivingStatusChanged();
}

// The pawn Driver has tried to take control of this vehicle
function bool TryToDrive(Pawn P)
{
	return (SelfDestructInstigator == None) && Super.TryToDrive(P);
}

/** If exit while boosting, boost out of the vehicle
Try to exit above
*/
function bool FindAutoExit(Pawn ExitingDriver)
{
	local vector X,Y,Z;
	local float PlaceDist;

	if ( bSelfDestructReady )
	{
		GetAxes(Rotation, X,Y,Z);
		Y *= -1;

		PlaceDist = 150 + 2*ExitingDriver.GetCollisionHeight();

		if ( TryExitPos(ExitingDriver, GetTargetLocation() + PlaceDist * Z) )
			return true;
	}
	return Super.FindAutoExit(ExitingDriver);
}

event bool DriverLeave(bool bForceLeave)
{
	local vector AimPoint, HitLocation, HitNormal, CameraLocation;
	local rotator CameraRotation;
	local Actor HitActor;

	if (bSelfDestructReady)
	{
		// we need to calculate the aim point BEFORE getting out of the vehicle
		// because the driver's camera uses different code and therefore may be pointing in a different direction
		if ( PlayerController(Controller) != None )
		{
			PlayerController(Controller).ClientPlaySound(EjectSoundCue);
			PlayerController(Controller).GetPlayerViewPoint(CameraLocation, CameraRotation);
			AimPoint = CameraLocation + 8000*Vector(CameraRotation);
			HitActor = Trace(HitLocation, HitNormal, AimPoint, CameraLocation, true,,,TRACEFLAG_Blocking);
			if ( HitActor != None)
			{
				AimPoint = HitLocation;
			}
			BoostDir = BoostForce * Normal(AimPoint - Location);
		}
		else if (Controller != None)
		{
			BoostDir = BoostForce * Normal(Controller.FocalPoint - Location);
		}
		else
		{
			BoostDir = BoostForce * vector(Rotation);
		}
	}

	return Super.DriverLeave(bForceLeave);
}

function bool EagleEyeTarget()
{
	return bSelfDestructReady;
}

simulated function StopVehicleSounds()
{
	super.StopVehicleSounds();
	CurveSound.Stop();
}

simulated function PlaySelfDestruct()
{
	local int i, CurrentRoll;

	// Terminate stay-upright constraint
	if ( StayUprightConstraintInstance != None)
	{
		StayUprightConstraintInstance.TermConstraint();
	}

	Mesh.SetActorCollision(false, false);

	bNoEncroachCheck = true;
	Mesh.CastShadow = false;
	Mesh.bCastDynamicShadow = false;
	bBlockActors = false;

	CurrentRoll = Rotation.Roll & 65535;
	bFlipRight = (CurrentRoll == Clamp(CurrentRoll, 0, 32768));

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		// kill ground effects
		for (i = 0; i < GroundEffectIndices.length; i++)
		{
			VehicleEffects[GroundEffectIndices[i]].EffectRef.SetHidden(true);
			VehicleEffects[GroundEffectIndices[i]].EffectRef.DeactivateSystem();
		}
		GroundEffectIndices.length = 0;

		if(SelfDestructEffect == none)
		{
			SelfDestructEffect = new(self) class'UTParticleSystemComponent';
			SelfDestructEffect.SetTemplate(SelfDestructEffectTemplate);
			AttachComponent(SelfDestructEffect);
		}
	}
}

simulated function DriverLeft()
{
	if (Role == ROLE_Authority && bSelfDestructReady)
	{
		// arm self destruct if exit vehicle while pushing rise or crouch
		bSelfDestructArmed = true;

		SelfDestructInstigator = Driver.Controller;
		EjectDriver();
		DestructStartTime = WorldInfo.TimeSeconds;

		PlaySelfDestruct();
	}

	bPressingAltFire = false;
	Super.DriverLeft();
}

event SelfDestruct(Actor ImpactedActor)
{
	Health = -100000;
	Mesh.SetActorCollision(false, false);
	BlowUpVehicle();
	if ( ImpactedActor != None )
	{
		ImpactedActor.TakeDamage(600, SelfDestructInstigator, GetTargetLocation(), 200000 * Normal(Velocity), SelfDestructDamageType,, self);
	}
	HurtRadius(600,600, SelfDestructDamageType, 200000, GetTargetLocation(),,SelfDestructInstigator);
	PlaySound(SelfDestructSoundCue);
	DestructStartTime = WorldInfo.TimeSeconds;
	bSelfDestructArmed = false;
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bSelfDestructArmed')
	{
		PlaySelfDestruct();
	}
	else if (VarName == 'bDoBikeJump')
	{
		JumpCountdown = JumpDuration;
		ViperJumpEffect();
	}
	else if (VarName == 'bHoldingDuck')
	{
		JumpCountdown = 0.0;
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function BlowupVehicle()
{
	if (SelfDestructEffect != None)
	{
		SelfDestructEffect.DeactivateSystem();
		SelfDestructEffect.SetHidden(true);
	}
	Super.BlowupVehicle();
}

simulated function ArmSelfDestruct()
{
	if (bSelfDestructInProgress) // sanity check
	{
		bSelfDestructReady = true;
		if (IsLocallyControlled() && IsHumanControlled())
		{
			Playsound(SelfDestructReadySnd, true, true, true);
		}
		bSelfDestructInProgress = false;
	}
}

function OnSelfDestruct(UTSeqAct_SelfDestruct Action)
{
	Rise = 1;
	SetTimer(1.0, false, 'ScriptedSelfDestruct');
}

function ScriptedSelfDestruct()
{
	Rise = 1;
	bSelfDestructReady=true;
	DriverLeave(true);
}

simulated function SetInputs(float InForward, float InStrafe, float InUp)
{
	super.SetInputs(InForward,InStrafe,InUp);
	if(bPressingAltFire)
	{
		Rise = 1.0f;
	}
}
simulated function bool OverrideBeginFire(byte FireModeNum)
{
	if (FireModeNum == 1)
	{
		bPressingAltFire = true;
		Rise=1.0f;
		return true;
	}

	return false;
}

simulated function bool OverrideEndFire(byte FireModeNum)
{
	local PlayerController PC;
	if (FireModeNum == 1)
	{
		Rise=0.0f;
		if(bSelfDestructReady)
		{
			DriverLeave(true);
		}
		else
		{
			PC=PlayerController(Seats[0].SeatPawn.Controller);
			if(PC != none)
			{
				PC.ReceiveLocalizedMessage(class'UTVehicleMessage', 0);
			}
		}
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
	JumpCountDown = 0;
	LastJumpTime = 0;
	bDoBikeJump = false;
	bPressingAltFire = false;
}

simulated event ViperJumpEffect()
{
	PlaySound(JumpSound, true);
	VehicleEvent('BoostStart');
	if (Role == ROLE_Authority || IsLocallyControlled())
	{
		bSelfDestructReady = false;
		SetTimer(TimeToRiseForSelfDestruct, false, 'ArmSelfDestruct');
		bSelfDestructInProgress = true;
	}
}

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
	bAttachDriver=true
	bDriverIsVisible=true

	bHomingTarget=true

	bLightArmor=true

	Health=200
	MeleeRange=-100.0
	ExitRadius=160.0

	UprightLiftStrength=30.0
	UprightTorqueStrength=30.0
	bCanFlip=true
	JumpDelay=3.0
	JumpCheckTraceDist=175.0

	MaxDestructDuration=2.2
	NormalGravity=0.9
	GlidingGravity=0.3
	CustomGravityScaling=0.9
	StallZGravityScaling=4.0
	JumpForceMag=600.0
	JumpDuration=0.5
	BoostForce=500.0

	bStayUpright=true
	StayUprightRollResistAngle=35.0
	StayUprightPitchResistAngle=30.0
	StayUprightStiffness=800
	StayUprightDamping=20

	COMOffset=(x=-50.0,y=0.0,z=-70.0)

	Begin Object Class=UTHoverWheel Name=RThruster
		BoneName="Base"
		BoneOffset=(X=-150.0,Y=65.0,Z=-140.0)
		WheelRadius=30
		SuspensionTravel=110
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
		BoneName="Base"
		BoneOffset=(X=-150.0,Y=-65.0,Z=-140.0)
		WheelRadius=30
		SuspensionTravel=110
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
		BoneName="Base"
		BoneOffset=(X=60.0,Y=0.0,Z=-140.0)
		WheelRadius=38
		SuspensionTravel=110
		bPoweredWheel=false
		LongSlipFactor=0.0
		LatSlipFactor=0.0
		HandbrakeLongSlipFactor=0.0
		HandbrakeLatSlipFactor=0.0
		SteerFactor=1.0
		bHoverWheel=true
	End Object
	Wheels(2)=FThruster

	Begin Object Class=UTVehicleSimHover Name=SimObject
		WheelSuspensionStiffness=45.0
		WheelSuspensionDamping=6.0
		WheelSuspensionBias=-0.5
		PitchTorqueMax=35.0
		PitchDamping=0.25
		MaxThrustForce=500.0
		MaxReverseForce=350.0
		LongDamping=0.3
		MaxStrafeForce=350.0
		LatDamping=0.3
		MaxRiseForce=0.0
		UpDamping=0.0
		TurnTorqueFactor=4000.0
		TurnTorqueMax=400.0
		StrafeTurnDamping=0.1
		bStrafeAffectsTurnDamping=true
		TurnDamping=0.25
		MaxYawRate=100000.0
		RollTorqueTurnFactor=250.0
		RollTorqueStrafeFactor=220.0
		RollTorqueMax=220.0
		RollDamping=0.25
		MaxRandForce=3.0
		RandForceInterval=0.75
		bAllowZThrust=true
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	FullAirSpeed=1800.0
	GlideAirSpeed=1.0
	bCanCarryFlag=false
	bFollowLookDir=True
	bTurnInPlace=True
	bScriptedRise=True
	bCanStrafe=True
	ObjectiveGetOutDist=750.0
	MaxDesireability=0.6
	SpawnRadius=125.0
	MomentumMult=3.0
	GlideSpeedReductionRate=1.0

	BaseEyeheight=40
	Eyeheight=40
	CameraLag=0.04
	CameraScaleMin=0.2
	CameraScaleMax=1.5
	LookForwardDist=100.0

	AirSpeed=1800.0
	GroundSpeed=1500.0
	DefaultFOV=90
	bStopWhenGlide=true
}
