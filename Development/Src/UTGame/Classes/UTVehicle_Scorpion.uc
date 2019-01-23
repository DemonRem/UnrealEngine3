/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicle_Scorpion extends UTVehicle
	native(Vehicle)
	abstract;

/** animation for the Scorpion's extendable blades */
var UTAnimBlendByWeapon BladeBlend;

/** animation for the Scorpion's hatch */
var AnimNodeSequence HatchSequence;

/** Internal variable.  Maintains brake light state to avoid extraMatInst calls.	*/
var bool bBrakeLightOn;

/** Internal variable.  Maintains reverse light state to avoid extra MatInst calls.	*/
var bool bReverseLightOn;

/** Internal variable.  Maintains headlight state to avoid extra MatInst calls.	*/
var bool bHeadlightsOn;

/** whether or not the blades are currently extended */
var repnotify bool bBladesExtended;

/** whether or not the blade on each side has been broken off */
var repnotify bool bLeftBladeBroken, bRightBladeBroken;

/** how far along blades a hit against world geometry will break them */
var float BladeBreakPoint;

/** material parameter that should be modified to turn the brake lights on and off */
var name BrakeLightParameterName;

/** material parameter that should be modified to turn the reverse lights on and off */
var name ReverseLightParameterName;

/** material parameter that should be modified to turn the headlights on and off */
var name HeadLightParameterName;

/** socket names for the start and end of the blade traces
	if the corresponding blade is not broken anything that gets in between the start and end bone triggers a BladeHit() event */
var name RightBladeStartSocket, RightBladeEndSocket, LeftBladeStartSocket, LeftBladeEndSocket;

/** damage type for blade kills */
var class<DamageType> BladeDamageType;

/** blade sounds */
var SoundCue BladeBreakSound, BladeExtendSound, BladeRetractSound;

/** rocket booster properties */
var float BoosterForceMagnitude;

var repnotify bool	bBoostersActivated;
var		bool	bSteeringLimited;		/** If true, steering is very limited (enabled while boosting) */
var Controller SelfDestructInstigator;
var float BoosterCheckRadius;	/** Radius to auto-check for targets when in self-destruct mode */
var float MaxBoostDuration;
var float BoostStartTime;
var float BoostChargeDuration;
var float BoostChargeTime;
var AudioComponent BoosterSound;
var class<UTDamageType> SelfDestructDamageType;
var float BoostPowerSpeed;
var float BoostReleaseTime;
var float BoostReleaseDelay;
var SoundCue SelfDestructSoundCue;
var SoundCue SelfDestructReadyCue;
var SoundCue SelfDestructWarningSound;
var SoundCue SelfDestructEnabledSound;
var SoundCue SelfDestructEnabledLoop;

/** Sound played whenever Suspension moves suddenly */
var SoundCue SuspensionShiftSound;
var AudioComponent SelfDestructEnabledComponent;
var AudioComponent SelfDestructWarningComponent;
var AudioComponent SelfDestructReadyComponent;
var SoundCue EjectSoundCue;

/** desired camera FOV while using booster */
var float BoosterFOVAngle;

/** animation for the boosters */
var UTAnimBlendByWeapon BoosterBlend;

/** set when boosters activated by Kismet script, so keep them active regardless of input */
var bool bScriptedBoosters;

/** replicated flag indicating when self destruct is activated */
var repnotify bool bSelfDestructArmed;

/** double tap forward to start rocket boosters */
var bool bTryToBoost;
var bool bWasThrottle;
var float ThrottleStartTime;

var	PointLightComponent		LeftBoosterLight, RightBoosterLight;			/** dynamic light */

var RB_ConstraintActor BladeVictimConstraint[2];

var StaticMesh ScorpionHood;

/** Rocket speed is the (clamped) max speed while boosting */
var float RocketSpeed;

/** Square of minimum speed needed to engage self destruct */
var float SelfDestructSpeedSquared;

/** How long the springs should be when the wheels need to be locked to the ground */
var() float LockSuspensionTravel;

/** How stiff the suspension should be when the wheels need to be locked to the ground */
var() float LockSuspensionStiffness;

/** How much the steering should be restricted while boosting */
var() float BoostSteerFactors[3];

/** swap BigExplosionTemplate for this when self-destructing */
var ParticleSystem SelfDestructExplosionTemplate;
var class<UTGib> HatchGibClass;

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		bBladesExtended, bLeftBladeBroken, bRightBladeBroken;
	if (bNetDirty && !bNetOwner && Role==ROLE_Authority)
		bBoostersActivated, bSelfDestructArmed;
}

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

/** Returns true if self destruct conditions (boosting, going fast enough) are met */
native function bool ReadyToSelfDestruct();

function bool EagleEyeTarget()
{
	return ReadyToSelfDestruct();
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if(SimObj.bAutoDrive)
	{
		SetDriving(true);
	}
}

simulated function SetInputs(float InForward, float InStrafe, float InUp)
{
	Super.SetInputs(InForward, InStrafe, InUp);

	if (bScriptedBoosters)
	{
		bTryToBoost = true;
	}
	else if (IsLocallyControlled())
	{
		if (Throttle > 0.0)
		{
			if (Rise > 0.0)
			{
				ServerBoost();
				bTryToBoost = true;
			}
			else if (!bWasThrottle)
			{
				if (WorldInfo.TimeSeconds - ThrottleStartTime < class'PlayerInput'.default.DoubleClickTime)
				{
					ServerBoost();
					bTryToBoost = true;
				}
				ThrottleStartTime = WorldInfo.TimeSeconds;
			}
			bWasThrottle = true;
		}
		else if (Throttle <= 0)
		{
			bWasThrottle = false;
		}
	}
}

simulated function StopVehicleSounds()
{
	super.StopVehicleSounds();
	BoosterSound.Stop();
}

simulated event SuspensionHeavyShift(float Delta)
{
	if(Delta>0)
	{
		PlaySound(SuspensionShiftSound);
	}
}

/**
when called makes the wheels stick to the ground more
*/
simulated function LockWheels()
{
	local SVehicleSimCar SimCar;

	bSteeringLimited = true;
	SimCar = SVehicleSimCar(SimObj);

	Wheels[0].SuspensionTravel = LockSuspensionTravel;
	Wheels[1].SuspensionTravel = LockSuspensionTravel;
	SimCar.WheelSuspensionStiffness= LockSuspensionStiffness;
	SimCar.MaxSteerAngleCurve.Points[0].OutVal = BoostSteerFactors[0]; //10.0;
	SimCar.MaxSteerAngleCurve.Points[1].OutVal = BoostSteerFactors[1]; //4.0;
	SimCar.MaxSteerAngleCurve.Points[2].OutVal = BoostSteerFactors[2]; //1.2;

}

/**
Resets the variables that are changed in the LockWheels call
*/
simulated function UnlockWheels()
{
	local SVehicleSimCar SimCar;

	bSteeringLimited = false;
	SimCar = SVehicleSimCar(SimObj);

	Wheels[0].SuspensionTravel = Default.Wheels[0].SuspensionTravel;
	Wheels[1].SuspensionTravel = Default.Wheels[1].SuspensionTravel;
	SimCar.WheelSuspensionStiffness = SVehicleSimCar(Default.SimObj).WheelSuspensionStiffness;
}
/** ActivateRocketBoosters()
called when player activates rocket boosters
*/
simulated event ActivateRocketBoosters()
{
	bSteeringLimited = true;

	AirSpeed = Default.RocketSpeed;

	if ( WorldInfo.NetMode == NM_DedicatedServer )
		return;

	// Play any animations/etc here

	if ( UTPlayerController(Controller) != none )
	{
		UTPlayerController(Controller).StartZoom(BoosterFOVAngle,60);
	}

	// play animation
	BoosterBlend.AnimFire('boosters_out', true,,, 'boosters_out_idle');
	// activate booster sound and effects
	BoosterSound.Play();
	VehicleEvent( 'BoostStart' );

	if ( PlayerController(Controller) != None )
	{
		Mesh.AttachComponentToSocket(LeftBoosterLight, VehicleEffects[0].EffectSocket);
		Mesh.AttachComponentToSocket(RightBoosterLight, VehicleEffects[1].EffectSocket);
		LeftBoosterLight.SetEnabled(TRUE);
		RightBoosterLight.SetEnabled(TRUE);
	}
	LockWheels();
}

/** DeactivateHandbrake()
called (usually by a timer) to deactivate the handbrake
*/
simulated function DeactivateHandbrake()
{
    bOutputHandbrake = FALSE;
    bHoldingDownHandbrake = FALSE;
}

simulated event EnableFullSteering()
{
	local SVehicleSimCar SimCar;

	bSteeringLimited = false;
	SimCar = SVehicleSimCar(SimObj);
	SimCar.MaxSteerAngleCurve.Points[0].OutVal = SVehicleSimCar(Default.SimObj).MaxSteerAngleCurve.Points[0].OutVal;
	SimCar.MaxSteerAngleCurve.Points[1].OutVal = SVehicleSimCar(Default.SimObj).MaxSteerAngleCurve.Points[1].OutVal;
	SimCar.MaxSteerAngleCurve.Points[2].OutVal = SVehicleSimCar(Default.SimObj).MaxSteerAngleCurve.Points[2].OutVal;
}

/** DeactivateRocketBoosters()
called when player deactivates rocket boosters or they run out
*/
simulated event DeactivateRocketBoosters()
{
	local UTPlayerController PC;

	// Set handbrake to decrease the possibility of a rollover
//	bOutputHandbrake = TRUE;
//	bHoldingDownHandbrake = TRUE;
//	SetTimer(3.0f, FALSE, 'DeactivateHandbrake');
	AirSpeed = Default.AirSpeed;
	EnableFullSteering();

	if ( WorldInfo.NetMode == NM_DedicatedServer )
		return;

	PC = UTPlayerController(Controller);
	if ( PC != none )
	{
		PC.StartZoom(PC.DefaultFOV,120);
	}

	// play animation
	BoosterBlend.AnimStopFire();
	// deactivate booster sound and effects
	BoosterSound.Stop();
	VehicleEvent( 'BoostStop' );

	LeftBoosterLight.SetEnabled(FALSE);
	RightBoosterLight.SetEnabled(FALSE);
	Mesh.DetachComponent(LeftBoosterLight);
	Mesh.DetachComponent(RightBoosterLight);
	UnlockWheels();
}

function OnActivateRocketBoosters(UTSeqAct_ActivateRocketBoosters BoosterAction)
{
	bScriptedBoosters = true;
}

reliable server function ServerBoost()
{
    bTryToBoost = true;
}

simulated function float AdjustFOVAngle(float FOVAngle)
{
	if (bBoostersActivated)
	{
		return Lerp( FOVAngle, BoosterFOVAngle, FMin(WorldInfo.TimeSeconds - BoostStartTime, 1.0) );
	}
	else
	{
		return Lerp( BoosterFOVAngle, FOVAngle, FMin(WorldInfo.TimeSeconds - BoostChargeTime, 1.0) );
	}
}

event SelfDestruct(Actor ImpactedActor)
{
	Health = -100000;
	if(SelfDestructWarningComponent != none)
	{
		SelfDestructWarningComponent.Stop();
	}
	if(SelfDestructEnabledComponent != none)
	{
		SelfDestructEnabledComponent.Stop();
	}
	BlowUpVehicle();
	if ( ImpactedActor != None )
	{
		ImpactedActor.TakeDamage(600, SelfDestructInstigator, GetTargetLocation(), 200000 * Normal(Velocity), SelfDestructDamageType,, self);
	}
	HurtRadius(600,600, SelfDestructDamageType, 200000, GetTargetLocation(), ImpactedActor, SelfDestructInstigator);
	PlaySound(SelfDestructSoundCue);
	BoostStartTime = WorldInfo.TimeSeconds;
}

// The pawn Driver has tried to take control of this vehicle
function bool TryToDrive(Pawn P)
{
	return (SelfDestructInstigator == None) && Super.TryToDrive(P);
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bBladesExtended')
	{
		SetBladesExtended(bBladesExtended);
	}
	else if (VarName == 'bLeftBladeBroken')
	{
		BreakOffBlade(true);
	}
	else if (VarName == 'bRightBladeBroken')
	{
		BreakOffBlade(false);
	}
	else if (VarName == 'bBoostersActivated')
	{
		if ( bBoostersActivated )
		{
			ActivateRocketBoosters();
		}
		else
		{
			DeActivateRocketBoosters();
		}
	}
	else if (VarName == 'bSelfDestructArmed')
	{
		PlaySelfDestruct();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function bool OverrideBeginFire(byte FireModeNum)
{
	if (FireModeNum == 1)
	{
		if (Role == ROLE_Authority && !bBladesExtended)
		{
			SetBladesExtended(true);
		}
		// note: the blade hit checks are in native tick
		return true;
	}

	return false;
}

simulated function bool OverrideEndFire(byte FireModeNum)
{
	if (FireModeNum == 1)
	{
		if (Role == ROLE_Authority && bBladesExtended)
		{
			SetBladesExtended(false);
		}
		return true;
	}

	return false;
}

/** extends and retracts the blades */
simulated function SetBladesExtended(bool bExtended)
{
	bBladesExtended = bExtended;
}

simulated function PlaySelfDestruct()
{
	local UTGib HatchGib;
	local SkelControlBase SkelControl;

	UTVehicleSimCar(SimObj).bDriverlessBraking = false;
	// play sound
	PlaySound(SelfDestructEnabledSound);
	if(SelfDestructWarningComponent == none)
	{
		SelfDestructWarningComponent = CreateAudioComponent(SelfDestructWarningSound, FALSE, TRUE);
		SelfDestructWarningComponent.Location = Location;
		SelfDestructWarningComponent.bUseOwnerLocation = true;
		AttachComponent(SelfDestructWarningComponent);
	}
	SelfDestructWarningComponent.Play();
	if(SelfDestructEnabledComponent == none)
	{
		SelfDestructEnabledComponent = CreateAudioComponent(SelfDestructEnabledLoop, FALSE, TRUE);
		SelfDestructEnabledComponent.Location = Location;
		SelfDestructEnabledComponent.bUseOwnerLocation = true;
		AttachComponent(SelfDestructEnabledComponent);
	}
	SelfDestructEnabledComponent.FadeIn(1.0f,1.0f);
	// blow off the hatch
	SkelControl = Mesh.FindSkelControl('Hatch');
	if (SkelControl != None)
	{
		SkelControl.BoneScale = 0.0;
		HatchGib = Spawn(HatchGibClass, self,, Mesh.GetBoneLocation('Hatch_Slide'), rot(0,0,0));
		if(HatchGib != none)
		{
			HatchGib.Velocity = 0.25*Velocity;
			HatchGib.Velocity.Z = 400.0;
			HatchGib.GibMeshComp.WakeRigidBody();
			HatchGib.GibMeshComp.SetRBLinearVelocity(HatchGib.Velocity, false);
		}
	}
	BigExplosionTemplates.length = 1;
	BigExplosionTemplates[0].Template = SelfDestructExplosionTemplate;
	BigExplosionTemplates[0].MinDistance = 0.0;
}

simulated function DriverLeft()
{
	if ( ReadyToSelfDestruct() )
	{
		SelfDestructInstigator = (Driver != none?Driver.Controller:none);

		bShouldEject = true;
		if ( PlayerController(Driver.Controller) != None )
		{
			PlayerController(Driver.Controller).ClientPlaySound(EjectSoundCue);
		}

		BoostStartTime = WorldInfo.TimeSeconds - MaxBoostDuration + 1.0;
		bSelfDestructArmed = true;
		PlaySelfDestruct();
	}
	else if (bBladesExtended)
	{
		SetBladesExtended(false);
	}

	Super.DriverLeft();
}

/**
 * Extra damage if hit while boosting
 */
simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	if (Role == ROLE_Authority)
	{
		if ( SelfDestructInstigator != None )
			Damage *= 2.0;
		else if ( bBoostersActivated )
			Damage *= 1.5;
	}

	Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
}

/** breaks off the given blade by scaling the bone to zero and spawning effects */
simulated function BreakOffBlade(bool bLeftBlade);

/** called when the blades hit something (not called for broken blades) */
simulated event BladeHit(Actor HitActor, vector HitLocation, bool bLeftBlade)
{
	local TraceHitInfo HitInfo;
	local vector VelDir;
	local Pawn P;
	local int index;

	if (HitActor.bBlockActors)
	{
		if (Vehicle(HitActor) != None)
		{
			if (HitActor.IsA('UTVehicle_Hoverboard'))
			{
				P = Vehicle(HitActor).Driver;
			}
		}
		else
		{
			P = Pawn(HitActor);
		}

		// if we hit a vehicle or a non-pawn, break off the blade
		if (P == None)
		{
			if (Role == ROLE_Authority)
			{
				if (bLeftBlade)
				{
					bLeftBladeBroken = true;
				}
				else
				{
					bRightBladeBroken = true;
				}
				BreakOffBlade(bLeftBlade);
			}
		}
		else
		{
			if (Role == ROLE_Authority)
			{
				P.TakeDamage(1000, Controller, HitLocation, Velocity * 100.f, BladeDamageType);
			}
			if ( P.Health <= 0 && !P.bDeleteMe && P.Physics == PHYS_RigidBody
				&& P.Mesh != None && P.Mesh.PhysicsAssetInstance != None )
			{
				// grab ragdoll
				VelDir = Normal(Velocity);
				P.CheckHitInfo( HitInfo, P.Mesh, VelDir, HitLocation );
				if ( HitInfo.BoneName == '' )
				{
					P.CheckHitInfo( HitInfo, P.Mesh, Normal(P.Location - HitLocation), HitLocation );
				}
				if ( HitInfo.BoneName != '' )
				{
					index = 0;
					if ( BladeVictimConstraint[index] != None )
					{
						index = 1;
						if ( BladeVictimConstraint[index] != None )
						{
							index = 0;
							BladeVictimConstraint[index].Destroy();
							BladeVictimConstraint[index] = None;
						}
					}
					BladeVictimConstraint[index] = Spawn(class'RB_ConstraintActorSpawnable',,,HitLocation);
					BladeVictimConstraint[index].InitConstraint( self, P, '', HitInfo.BoneName, 200.f);
					BladeVictimConstraint[index].LifeSpan = 1 + 4*FRand();
				}
			}
		}
	}
}


/** If exit while boosting, boost out of the vehicle
Try to exit above
*/
function bool FindAutoExit(Pawn ExitingDriver)
{
	local vector X,Y,Z;
	local float PlaceDist;

	if ( bBoostersActivated )
	{
		GetAxes(Rotation, X,Y,Z);
		Y *= -1;

		PlaceDist = 150 + 2*ExitingDriver.GetCollisionHeight();

		if ( TryExitPos(ExitingDriver, GetTargetLocation() + PlaceDist * Z) )
			return true;
	}
	return Super.FindAutoExit(ExitingDriver);
}

//========================================
// AI Interface

function byte ChooseFireMode()
{
	if (Pawn(Controller.Focus) != None
		&& Vehicle(Controller.Focus) == None
		&& Controller.MoveTarget == Controller.Focus
		&& Controller.InLatentExecution(Controller.LATENT_MOVETOWARD)
		&& VSize(Controller.FocalPoint - Location) < 1200
		&& Controller.LineOfSightTo(Controller.Focus) )
	{
		return 1;
	}
	return 0;
}

function IncomingMissile(Projectile P)
{
	local UTBot B;

	B = UTBot(Controller);
	if (Health < 200 && B != None && B.Skill > 4.0 + 4.0 * FRand() && VSize(P.Location - Location) < VSize(P.Velocity))
	{
		DriverLeave(false);
	}
}

simulated function TeamChanged()
{
	super.TeamChanged();
	// clear out the flags since we have a new material:
	bBrakeLightOn = false;
	bReverseLightOn = false;
	bHeadlightsOn=false;
}

function OnSelfDestruct(UTSeqAct_SelfDestruct Action)
{
	bScriptedBoosters = true;
	SetTimer(0.5, true, 'CheckScriptedSelfDestruct');
}

function CheckScriptedSelfDestruct()
{
	if ( ReadyToSelfDestruct() )
	{
		DriverLeave(true);
		ClearTimer('CheckScriptedSelfDestruct');
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
	Health=300
	StolenAnnouncementIndex=5

	COMOffset=(x=-40.0,y=0.0,z=-36.0)
	UprightLiftStrength=280.0
	UprightTime=1.25
	UprightTorqueStrength=500.0
	bCanFlip=true
	bSeparateTurretFocus=true
	bHasHandbrake=true
	bStickDeflectionThrottle=true
	GroundSpeed=950
	AirSpeed=1100
	RocketSpeed=2000
	ObjectiveGetOutDist=1500.0
	MaxDesireability=0.4
	HeavySuspensionShiftPercent=0.75f;
	bUseLookSteer=true

	Begin Object Class=UTVehicleSimCar Name=SimObject
		WheelSuspensionStiffness=100.0
		WheelSuspensionDamping=3.0
		WheelSuspensionBias=0.1
		ChassisTorqueScale=0.5
		MaxBrakeTorque=5.0
		StopThreshold=100

		MaxSteerAngleCurve=(Points=((InVal=0,OutVal=45),(InVal=600.0,OutVal=15.0),(InVal=1100.0,OutVal=10.0),(InVal=1300.0,OutVal=6.0),(InVal=1600.0,OutVal=1.0)))
		SteerSpeed=110

		LSDFactor=0.0
		TorqueVSpeedCurve=(Points=((InVal=-600.0,OutVal=0.0),(InVal=-300.0,OutVal=80.0),(InVal=0.0,OutVal=120.0),(InVal=950.0,OutVal=120.0),(InVal=1050.0,OutVal=10.0),(InVal=1150.0,OutVal=0.0)))
		EngineRPMCurve=(Points=((InVal=-500.0,OutVal=2500.0),(InVal=0.0,OutVal=500.0),(InVal=549.0,OutVal=3500.0),(InVal=550.0,OutVal=1000.0),(InVal=849.0,OutVal=4500.0),(InVal=850.0,OutVal=1500.0),(InVal=1100.0,OutVal=5000.0)))
		EngineBrakeFactor=0.025
		ThrottleSpeed=0.1
		WheelInertia=0.2
		NumWheelsForFullSteering=4
		SteeringReductionFactor=0.0
		SteeringReductionMinSpeed=1100.0
		SteeringReductionSpeed=1400.0
		bAutoHandbrake=true
		bClampedFrictionModel=true
		FrontalCollisionGripFactor=0.18
		ConsoleHardTurnGripFactor=0.7

		// Longitudinal tire model based on 10% slip ratio peak
		WheelLongExtremumSlip=0.1
		WheelLongExtremumValue=1.0
		WheelLongAsymptoteSlip=2.0
		WheelLongAsymptoteValue=0.6

		// Lateral tire model based on slip angle (radians)
   		WheelLatExtremumSlip=0.35     // 20 degrees
		WheelLatExtremumValue=0.85
		WheelLatAsymptoteSlip=1.4     // 80 degrees
		WheelLatAsymptoteValue=0.7

		bAutoDrive=false
		AutoDriveSteer=0.3
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	BoostSteerFactors[0] = 10.0
	BoostSteerFactors[1] = 4.0
	BoostSteerFactors[2] = 1.2

	Begin Object Class=UTVehicleScorpionWheel Name=RRWheel
		BoneName="B_R_Tire"
		BoneOffset=(X=0.0,Y=20.0,Z=0.0)
		SkelControlName="B_R_Tire_Cont"
	End Object
	Wheels(0)=RRWheel

	Begin Object Class=UTVehicleScorpionWheel Name=LRWheel
		BoneName="B_L_Tire"
		BoneOffset=(X=0.0,Y=-20.0,Z=0.0)
		SkelControlName="B_L_Tire_Cont"
	End Object
	Wheels(1)=LRWheel

	Begin Object Class=UTVehicleScorpionWheel Name=RFWheel
		BoneName="F_R_Tire"
		BoneOffset=(X=0.0,Y=20.0,Z=0.0)
		SteerFactor=1.0
		LongSlipFactor=2.0
		LatSlipFactor=3.0
		HandbrakeLongSlipFactor=0.8
		HandbrakeLatSlipFactor=0.8
		SkelControlName="F_R_Tire_Cont"
	End Object
	Wheels(2)=RFWheel

	Begin Object Class=UTVehicleScorpionWheel Name=LFWheel
		BoneName="F_L_Tire"
		BoneOffset=(X=0.0,Y=-20.0,Z=0.0)
		SteerFactor=1.0
		LongSlipFactor=2.0
		LatSlipFactor=3.0
		HandbrakeLongSlipFactor=0.8
		HandbrakeLatSlipFactor=0.8
		SkelControlName="F_L_Tire_Cont"
	End Object
	Wheels(3)=LFWheel

	lockSuspensionTravel=37;
	lockSuspensionStiffness=62.5;

	BoosterForceMagnitude=450.0
	MaxBoostDuration=2.0
	BoostChargeDuration=5.0
	BoosterCheckRadius=150.0
	BoostChargeTime=-10.0
	BoostPowerSpeed=1800.0
	BoosterFOVAngle=105.0

	TeamBeaconOffset=(z=60.0)
	SpawnRadius=125.0

	BaseEyeheight=30
	Eyeheight=30
	DefaultFOV=80
	bLightArmor=true
	CameraLag=0.07

	bReducedFallingCollisionDamage=true

	BladeBreakPoint=0.8
	BoostReleaseDelay=0.15

	SelfDestructSpeedSquared=1210000.0
}
