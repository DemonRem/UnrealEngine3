/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Scavenger extends UTVehicle_Walker
	native(Vehicle)
	abstract;

var()	float	JumpForceMag;
var     float	JumpDelay, LastJumpTime;

var		bool							bDoBikeJump;
var		bool							bOldDoBikeJump;

var float				SpinStartTime;
var float				SpinTime;

/** Area of effect for spin attack */
var	float				SpinAttackRadius;

var bool				bSpinAttackActive;

/** How much momentum to impart on a non-UTPawn with the arms per sec*/
var float				ArmMomentum;

var repnotify bool bIsInBallMode;

var soundcue JumpSound;
var soundcue DuckSound;
var soundcue BounceSound;

var soundcue ArmRetractSound;
var soundcue ArmExtendSound;
var audiocomponent BallAudio;

/** Sound to play from the ball rolling */
var(Sounds) editconst const AudioComponent RollAudioComp;
var(Sounds) array<MaterialSoundEffect> RollSoundList;
var name CurrentRollMaterial;

var() float FullHover, ReducedHover;

/** Last Turret Height setting transition start time */
var float LastBallTransitionTime;

/** PhysicalMaterial to use while rolling */
var transient PhysicalMaterial RollingPhysicalMaterial;

var float MaxThrustForce, MaxBallForce, MaxBoostForce;

var particlesystemcomponent ImpactParticle;

/** Damage type when collide with something in ball mode */
var class<UTDamageType>		BallCollisionDamageType;

/** Ball mode boost timer and effects */
var repnotify bool	bBallBoostActivated;

/** The visual effect when boosting*/
var ParticleSystemComponent BallBoostEffect;

/** How long you can boost in ball mode */
var float MaxBoostDuration;

/** used to track boost duration */
var float BoostStartTime;

/** How long it takes to recharge between boosts */
var float BoostChargeDuration;

/** used to track boost recharging duration */
var float BoostChargeTime;

/** max allowed speed while boosting */
var float MaxBoostSpeed;

/** max allowed speed while in ball mode (and not boosting) */
var float MaxBallSpeed;

/** currently active seeker ball */
var UTProj_ScavengerBoltBase ActiveSeeker;

/** Team templates for the thruster effects */
var ParticleSystem ThrusterLeftTemplate[2];
var ParticleSystem ThrusterRightTemplate[2];
cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
	virtual FVector GetWalkerBodyLoc();
}

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		bIsInBallMode;
	if (bNetDirty && !bNetOwner && Role==ROLE_Authority)
		bBallBoostActivated;
	if (bNetDirty && bNetOwner )
		ActiveSeeker;
}

simulated function PostBeginPlay()
{
	local vector X, Y, Z;

	Super.PostBeginPlay();
	GetAxes(Rotation, X,Y,Z);
}

/** Ask weapon to spawn a seeker */
event SpawnSeeker()
{
	ActiveSeeker = UTProj_ScavengerBoltBase(Seats[0].Gun.ProjectileFire());
}

simulated native function ImpactEffect(Vector HitPos);

simulated event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
					const out CollisionImpactData Collision, int ContactIndex )
{
	local int Damage;
	local Actor DamagedActor;
	local controller InstigatorController;

	if(health > 0)
	{
		if ( !bIsInBallMode )
		{
			Super.RigidBodyCollision(HitComponent, OtherComponent, Collision, ContactIndex);
			return;
		}
		if (WorldInfo.TimeSeconds - LastCollisionSoundTime > CollisionIntervalSecs)
		{
			PlaySound(BounceSound);
			if(ImpactParticle != none)
			{
				ImpactEffect(Collision.ContactInfos[0].ContactPosition);
			}
			LastCollisionSoundTime = WorldInfo.TimeSeconds;
		}
		if (LastCollisionDamageTime != WorldInfo.TimeSeconds)
		{
			DamagedActor = (OtherComponent != None) ? OtherComponent.Owner : None;

			if ( (DamagedActor != None) && !DamagedActor.bStatic )
			{
				// give impact damage
				Damage = int(VSize(Mesh.GetRootBodyInstance().PreviousVelocity) * 0.05);
				if (Damage > 20)
				{
					if (Controller != None)
					{
						InstigatorController = Controller;
					}
					else if (Instigator != None)
					{
						InstigatorController = Instigator.Controller;
					}
					DamagedActor.TakeDamage(Damage, InstigatorController, Collision.ContactInfos[0].ContactPosition, vect(0,0,0), BallCollisionDamageType);
					LastCollisionDamageTime = WorldInfo.TimeSeconds;
				}
			}
		}
	}
}

simulated function TeamChanged()
{
	local MaterialInterface NewMaterial;
// NO SUPER! We want team on Mat 1 not 0.
	if (TeamMaterials[1] != none && TeamMaterials[0] != none)
	{
		NewMaterial = TeamMaterials[(Team == 1) ? 1 : 0];
		Mesh.SetMaterial(1, NewMaterial);
	}
	VehicleEffects[0].EffectTemplate = ThrusterLeftTemplate[(Team==1)?1:0];
	VehicleEffects[0].EffectRef.SetTemplate(VehicleEffects[0].EffectTemplate);
	VehicleEffects[1].EffectTemplate = ThrusterRightTemplate[(Team==1)?1:0];
	VehicleEffects[1].EffectRef.SetTemplate(VehicleEffects[1].EffectTemplate);
	BodyActor.TeamChanged();
	UpdateDamageMaterial();

}

simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	super.TakeDamage(Damage,EventInstigator,HitLocation,Momentum,DamageType,HitInfo,DamageCauser);
	if(WorldInfo.Netmode != NM_DEDICATEDSERVER && ImpactParticle != none && !ImpactParticle.bIsActive && Damage >= 5)
	{
		ImpactEffect(HitLocation);
	}
}

simulated function bool OverrideBeginFire(byte FireModeNum)
{
	if (FireModeNum == 1)
	{
		// toggle ball mode
		if (Role == ROLE_Authority)
		{
			if ( !bIsInBallMode && (WorldInfo.TimeSeconds - BoostStartTime < BoostChargeTime) )
			{
				// @TODO FIXMESTEVE maybe play buzzer sound back to client?
				return true;
			}
			bIsInBallMode = !bIsInBallMode;
			BallModeTransition();
		}
		return true;
	}
	if(bIsInBallMode)
	{
		if ( !bSpinAttackActive )
		{
			bSpinAttackActive = true;
			SpinStartTime = WorldInfo.TimeSeconds;
			UTWalkerBody_Scavenger(BodyActor).Cloak(false);
		}
		return true;
	}

	return false;
}

simulated function DriverLeft()
{
	if ( (Health > 0) && bIsInBallMode && (Role == ROLE_Authority) )
	{
		bIsInBallMode = false;
		BallModeTransition();
	}

	Super.DriverLeft();
}

simulated event EndBallMode()
{
	bIsInBallMode = false;
	BallModeTransition();
}

function PossessedBy(Controller C, bool bVehicleTransition)
{
	super.PossessedBy(C, bVehicleTransition);

	// reset jump/duck properties
	bHoldingDuck = false;
	LastJumpTime = 0;
	bDoBikeJump = false;
}

/**
 * When an icon for this vehicle is needed on the hud, this function is called
 */
simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, LinearColor FinalColor)
{
	local Rotator VehicleRotation;

	if ( HUDMaterialInstance == None )
	{
		HUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		HUDMaterialInstance.SetParent(MP.HUDIcons);
	}
	HUDMaterialInstance.SetVectorParameterValue('HUDColor', FinalColor);
	VehicleRotation = (bIsInBallMode && (Controller != None)) ? Controller.Rotation : Rotation;
	MP.DrawRotatedMaterialTile(Canvas,HUDMaterialInstance, HUDLocation, VehicleRotation.Yaw + 32757, MapSize, MapSize*Canvas.ClipY/Canvas.ClipX,IconXStart, IconYStart, IconXWidth, IconYWidth);
}

simulated function float GetChargePower()
{
	return FClamp( (WorldInfo.TimeSeconds - LastJumpTime), 0, JumpDelay)/JumpDelay;
}

event ScavengerJumpEffect();

event ScavengerDuckEffect();

event SpinAttackVictim(Pawn HitPawn, float DeltaSeconds)
{
	local vector NewMomentum;
	if(HitPawn != none)
	{
		if(UTPawn(HitPawn) != none) // just kill UTPawn's
		{
			HitPawn.Died(Controller, class'UTDmgType_ScavengerStabbed',Location);
			if(WorldInfo.NetMode != NM_DedicatedServer)
			{
				UTWalkerBody_Scavenger(BodyActor).PawnGrabber[0].GrabComponent(HitPawn.Mesh, UTPawn(HitPawn).TorsoBoneName, HitPawn.Location, FALSE);
				//(UTWalkerBody_Scavenger(BodyActor).SkeletalMeshComponent).AttachComponentToSocket((HitPawn.Mesh),'LegOneRag');
			}
		}
		else // non-UT Pawns get pushed away
		{
			NewMomentum = Normal(HitPawn.Location - Location)*ArmMomentum*DeltaSeconds;
			HitPawn.TakeDamage(0, Controller, Location, NewMomentum, class'UTDmgType_ScavengerStabbed',,self);
		}
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if ( VarName == 'bIsInBallMode' )
	{
		BallModeTransition();
	}
	else if (VarName == 'bBallBoostActivated')
	{
		if ( bBallBoostActivated )
		{
			ActivateBallBooster();
		}
		else
		{
			DeactivateBallBooster();
		}
	}
	else
		Super.ReplicatedEvent(VarName);
}


simulated function ActivateBallBooster()
{
	bBallBoostActivated = TRUE;
	BoostStartTime = WorldInfo.TimeSeconds;
	BallBoostEffect.ActivateSystem();
}

simulated event DeactivateBallBooster()
{
	bBallBoostActivated = false;
	bSpinAttackActive = false;
	BoostStartTime = WorldInfo.TimeSeconds;
	BallBoostEffect.DeactivateSystem();
}

// FIXME play anim
simulated function BallModeTransition()
{
	local AnimNodeBlendList RetractionBlend;
	local UTPlayerReplicationInfo PRI;

	StopFiringWeapon();
	UTVehicleSimHover(SimObj).bUnPoweredDriving = bIsInBallMode;
	if ( BodyActor != None )
	{
		UTWalkerBody_Scavenger(BodyActor).bIsInBallMode = bIsInBallMode;
		UTWalkerBody_Scavenger(BodyActor).bStartedBallMode = false;
		RetractionBlend = AnimNodeBlendList(UTWalkerBody_Scavenger(BodyActor).SkeletalMeshComponent.FindAnimNode(UTWalkerBody_Scavenger(BodyActor).RetractionBlend));
		if(RetractionBlend != none)
		{
			RetractionBlend.SetActiveChild(bIsInBallMode?1.0:2.0, 1.0);
			if(bIsInBallMode)
			{
				RetractionBlend.Children[1].Anim.PlayAnim(false, 0.5);
				PlaySound(ArmRetractSound);
				BallAudio.FadeIn(0.1f,1.0f);
				if(ScrapeSound != none)
				{
					ScrapeSound.stop();
					ScrapeSound = none;
				}
			}
			else
			{
				UTWalkerBody_Scavenger(BodyActor).Cloak(false);
				RetractionBlend.Children[2].Anim.PlayAnim(false, 0.5);
				PlaySound(ArmExtendSound);
				BallAudio.FadeOut(0.1f,0.0f);
				if(ScrapeSound == none)
				{
					ScrapeSound = createaudiocomponent(default.ScrapeSound.soundcue, false, true);
				}
			}
		}
	}

	PRI = UTPlayerReplicationInfo(PlayerReplicationInfo);
	if ( (PRI != None) && (PRI.GetFlag() != None) )
	{
		HoldGameObject(PRI.GetFlag());
	}
	if ( bIsInBallMode )
	{
		BaseEyeheight = 0;
		StayUprightConstraintInstance.TermConstraint();
		Mesh.SetPhysMaterialOverride(RollingPhysicalMaterial);

		if ( Role == ROLE_Authority )
		{
			ActivateBallBooster();
		}
	}
	else
	{
		BaseEyeheight = Default.BaseEyeheight;
		InitStayUpright();
		Mesh.SetPhysMaterialOverride(bDriving ? DrivingPhysicalMaterial : DefaultPhysicalMaterial);

		// turn off ball boost mode
		if ( bBallBoostActivated )
		{
			DeactivateBallBooster();
		}
	}
}

/**
 * HoldGameObject() Attach GameObject to mesh.
 * @param 	GameObj 	Game object to hold
 */
simulated event HoldGameObject(UTCarriedObject GameObj)
{
	super.HoldGameObject(GameObj);

	if ( bIsInBallMode )
	{
		GameObj.SetHardAttach(false);
		GameObj.bIgnoreBaseRotation = true;
	}
}

native function InitStayUpright();

simulated function bool GetPowerLevel(out float PowerLevel)
{
	if (bIsInBallMode)
	{
		if (bBallBoostActivated)
		{
			PowerLevel = 1.0 - (WorldInfo.TimeSeconds - BoostStartTime) / MaxBoostDuration;
		}
		else
		{
			PowerLevel = (WorldInfo.TimeSeconds - BoostChargeTime) / BoostChargeDuration;
		}
		PowerLevel = FClamp(PowerLevel, 0.0, 1.0);
		return true;
	}
	else
	{
		return Super.GetPowerLevel(PowerLevel);
	}
}

defaultproperties
{
	Begin Object Name=RB_BodyHandle
		LinearDamping=100.0
		LinearStiffness=99000.0
		AngularDamping=100.0
		AngularStiffness=99000.0
	End Object

	Health=200
	MeleeRange=-100.0

	COMOffset=(X=0.f,Y=0.f,Z=0.f)
	bCanFlip=true
	bEjectPassengersWhenFlipped=false

	AirSpeed=700.0
	GroundSpeed=700.0

	bCanCarryFlag=true
	bFollowLookDir=true
	bCanStrafe=true
	bTurnInPlace=true
	ObjectiveGetOutDist=750.0
	MaxDesireability=0.6
	SpawnRadius=125.0
	BaseBodyOffset=(Z=-45.f)
	BaseEyeHeight=120
	EyeHeight=120
	LookForwardDist=40.0
	WalkableFloorZ=0.85

	bUseSuspensionAxis=true

	bStayUpright=true
	StayUprightRollResistAngle=0.0			// will be "locked"
	StayUprightPitchResistAngle=0.0

	WheelSuspensionTravel(WalkerStance_Standing)=70
	WheelSuspensionTravel(WalkerStance_Parked)=20
	SuspensionTravelAdjustSpeed=150
	MaxBallForce=50.0
	MaxThrustForce=600.0
	MaxBoostForce=250.0
	MaxBoostDuration=2.0
	BoostChargeDuration=5.0
	BoostChargeTime=-10.0
	MaxBoostSpeed=1500.0
	MaxBallSpeed=1000.0

	Begin Object Class=UTVehicleSimHover Name=SimObject
		WheelSuspensionStiffness=800.f
		WheelSuspensionDamping=200.f
		WheelSuspensionBias=0.0
		MaxThrustForce=600.0
		MaxReverseForce=600.0
		LongDamping=0.3
		MaxStrafeForce=600.0
		LatDamping=0.3
		MaxRiseForce=0.0
		UpDamping=0.0
		TurnTorqueFactor=10000.0
		TurnTorqueMax=100000.0
		TurnDamping=0.3
		MaxYawRate=1.6
		PitchTorqueMax=35.0
		PitchDamping=0.1
		RollTorqueMax=50.0
		RollDamping=0.1
		MaxRandForce=0.0
		RandForceInterval=1000.0
		bCanClimbSlopes=true
		PitchTorqueFactor=0.0
		RollTorqueTurnFactor=0.0
		RollTorqueStrafeFactor=0.0
		bAllowZThrust=false
		bStabilizeStops=true
		StabilizationForceMultiplier=2.0
		CurrentStabilizationMultiplier=2.0
		bFullThrustOnDirectionChange=true
		bDisableWheelsWhenOff=true
	End Object

	SimObj=SimObject
	Components.Add(SimObject)


	// from darkwalker
	Begin Object Class=UTHoverWheel Name=RThruster
		BoneName="BodyRoot"
		BoneOffset=(X=0,Y=0,Z=-100)
		WheelRadius=30
		SuspensionTravel=20
		bPoweredWheel=false
		SteerFactor=1.0
		LongSlipFactor=0.0
		LatSlipFactor=0.0
		HandbrakeLongSlipFactor=0.0
		HandbrakeLatSlipFactor=0.0
	End Object
	Wheels(0)=RThruster

	bHomingTarget=true

	JumpForceMag=9000.0
	JumpDelay=2.0

	bRotateCameraUnderVehicle=true
	BallCollisionDamageType=class'UTDmgType_VehicleCollision'
	ArmMomentum=1.0
	SpinAttackRadius=250.0
	bAnimateDeadLegs=true;
}
