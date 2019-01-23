/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_DarkWalker extends UTVehicle_Walker
	native(Vehicle)
	abstract;

var repnotify byte TurretFlashCount;
var repnotify rotator TurretWeaponRotation;
var byte TurretFiringMode;

var particleSystem BeamTemplate;

/** Holds the Emitter for the Beam */
var ParticleSystemComponent BeamEmitter[2];

/** Where to attach the Beam */
var name BeamSockets[2];

/** The name of the EndPoint parameter */
var name EndPointParamName;

var protected AudioComponent BeamAmbientSound;
var SoundCue BeamFireSound;

var float WarningConeMaxRadius;
var float LengthDarkWalkerWarningCone;
var AudioComponent WarningConeSound;
var name ConeParam;

var Emitter EffectEmitter;


var actor LastHitActor;

var bool bIsBeamActive;

/** radius to allow players under this darkwalker to gain entry */
var float CustomEntryRadius;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );

	//@fixme FIXME: E3 hack!!! adjust test position for Dark Walker so bots realize they've reached points they're floating over
	//need a good general solution post E3
	virtual UBOOL ReachThresholdTest(const FVector& TestPosition, const FVector& Dest, AActor* GoalActor, FLOAT UpThresholdAdjust, FLOAT DownThresholdAdjust, FLOAT ThresholdAdjust)
	{
		return Super::ReachThresholdTest(TestPosition, Dest, GoalActor, UpThresholdAdjust, DownThresholdAdjust + 450.f, ThresholdAdjust);
	}
}

replication
{
	if (!IsSeatControllerReplicationViewer(1))
		TurretFlashCount, TurretWeaponRotation;
}

native simulated final function PlayWarningSoundIfInCone(Pawn Target);

simulated function PostBeginPlay()
{
	super.PostBeginPlay();
	AddBeamEmitter();
}

simulated event Destroyed()
{
	super.Destroyed();
	KillBeamEmitter();
}

simulated function AddBeamEmitter()
{
	local int i;
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		for (i=0;i<2;i++)
		{
			if (BeamEmitter[i] == None)
			{
				if (BeamTemplate != None)
				{
					BeamEmitter[i] = new(Outer) class'UTParticleSystemComponent';
					BeamEmitter[i].SetTemplate(BeamTemplate);
					BeamEmitter[i].SecondsBeforeInactive=1.0f;
					BeamEmitter[i].SetHidden(true);
					Mesh.AttachComponentToSocket( BeamEmitter[i],BeamSockets[i] );
				}
			}
			else
			{
				BeamEmitter[i].ActivateSystem();
			}
		}
	}
}

simulated function KillBeamEmitter()
{
	local int i;
	for (i=0;i<2;i++)
	{
		if (BeamEmitter[i] != none)
		{
			//BeamEmitter[i].SetHidden(true);
			BeamEmitter[i].DeactivateSystem();
		}
	}
}

simulated function SetBeamEmitterHidden(bool bHide)
{
	local int i;

	if ( bHide )
		EffectEmitter = None;
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		if (bIsBeamActive != !bHide )
		{
			for (i=0; i<2; i++)
			{
					if (BeamEmitter[i] != none)
					{
						if(!bHide)
							BeamEmitter[i].SetHidden(bHide);
						else
							BeamEmitter[i].DeactivateSystem();
					}

					if (!bHide)
					{
						BeamAmbientSound.SoundCue = BeamFireSound;
						BeamAmbientSound.Play();
						BeamEmitter[i].ActivateSystem();
					}
					else
					{
						BeamAmbientSound.FadeOut(0.3f, 0.f);
					}
			}
		}
		bIsBeamActive = !bHide;
	}
}

/**
 * Detect the transition from vehicle to ground and vice versus and handle it
 */

simulated function actor FindWeaponHitNormal(out vector HitLocation, out Vector HitNormal, vector End, vector Start, out TraceHitInfo HitInfo)
{
	local Actor NewHitActor;

	NewHitActor = Super.FindWeaponHitNormal(HitLocation, HitNormal, End, Start, HitInfo);
	if (NewHitActor != LastHitActor)
	{
		EffectEmitter = none;
	}
	LastHitActor = NewHitActor;
	return NewHitActor;
}


simulated function Emitter SpawnImpactEmitter(vector HitLocation, vector HitNormal, MaterialImpactEffect ImpactEffect)
{
	local rotator TmpRot;

	TmpRot = rotator(HitNormal);
	TmpRot.Pitch = NormalizeRotAxis(TmpRot.Pitch - 16384);

	if ( (EffectEmitter == None) || EffectEmitter.bDeleteMe )
	{
		EffectEmitter = Super.SpawnImpactEmitter(HitLocation, vector(TmpRot), ImpactEffect);
		EffectEmitter.SetDrawScale(0.7);
	}
	else
	{
		EffectEmitter.SetLocation(HitLocation);
		EffectEmitter.SetRotation(TmpRot);
	}
	return EffectEmitter;
}

simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local int i;

	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	if ( SeatIndex == 0 )
	{
		SetBeamEmitterHidden(false);
		for(i=0;i<2;i++)
		{
			BeamEmitter[i].SetVectorParameter(EndPointParamName, HitLocation);
		}
	}
}

simulated function VehicleWeaponStoppedFiring( bool bViaReplication, int SeatIndex )
{
	if (SeatIndex == 0)
	{
		SetBeamEmitterHidden(true);
	}
}

function PassengerLeave(int SeatIndex)
{
	Super.PassengerLeave(SeatIndex);

	SetDriving(NumPassengers() > 0);
}

function bool PassengerEnter(Pawn P, int SeatIndex)
{
	local bool b;

	b = Super.PassengerEnter(P, SeatIndex);
	SetDriving(NumPassengers() > 0);
	return b;
}

simulated function VehicleCalcCamera(float DeltaTime, int SeatIndex, out vector out_CamLoc, out rotator out_CamRot, out vector CamStart, optional bool bPivotOnly)
{
	local UTPawn P;

	if (SeatIndex == 1)
	{
		// Handle the fixed view
		P = UTPawn(Seats[SeatIndex].SeatPawn.Driver);
		if (P != None && P.bFixedView)
		{
			out_CamLoc = P.FixedViewLoc;
			out_CamRot = P.FixedViewRot;
			return;
		}

		out_CamLoc = GetCameraStart(SeatIndex);
	CamStart = out_CamLoc;
		out_CamRot = Seats[SeatIndex].SeatPawn.GetViewRotation();
		return;
	}

	Super.VehicleCalcCamera(DeltaTime, SeatIndex, out_CamLoc, out_CamRot, CamStart, bPivotOnly);
}


/**
*  Overloading this from SVehicle to avoid torquing the walker head.
*/
function AddVelocity( vector NewVelocity, vector HitLocation, class<DamageType> DamageType, optional TraceHitInfo HitInfo )
{
	// apply hit at location, not hitlocation
	Super.AddVelocity(NewVelocity, Location, DamageType, HitInfo);
}

/**
  * Let pawns standing under me get in, if I have a driver.
  */
function bool InCustomEntryRadius(Pawn P)
{
	return ( (P.Location.Z < Location.Z) && (VSize2D(P.Location - Location) < CustomEntryRadius)
		&& FastTrace(P.Location, Location) );
}

event WalkerDuckEffect();

defaultproperties
{
	Begin Object Name=RB_BodyHandle
		LinearDamping=100.0
		LinearStiffness=99000.0
		AngularDamping=100.0
		AngularStiffness=99000.0
	End Object

	Health=1000
	MeleeRange=-100.0

	COMOffset=(x=0,y=0.0,z=0)
	bCanFlip=false

	AirSpeed=350.0
	GroundSpeed=350.0

	bCanCarryFlag=true
	bFollowLookDir=true
	bCanStrafe=true
	bTurnInPlace=true
	ObjectiveGetOutDist=750.0
	MaxDesireability=0.6
	SpawnRadius=125.0
	bNoZSmoothing=true
	BaseBodyOffset=(Z=0.0)
	BaseEyeheight=20
	Eyeheight=0
	LookForwardDist=40.0
	TeamBeaconOffset=(z=350.0)

	bUseSuspensionAxis=true

	bStayUpright=true
	StayUprightRollResistAngle=0.0			// will be "locked"
	StayUprightPitchResistAngle=0.0
	//StayUprightStiffness=10
	//StayUprightDamping=100

	WheelSuspensionTravel(WalkerStance_Standing)=440
	WheelSuspensionTravel(WalkerStance_Parked)=0
	WheelSuspensionTravel(WalkerStance_Crouched)=110
	SuspensionTravelAdjustSpeed=250
	HoverAdjust(WalkerStance_Standing)=-100.0
	HoverAdjust(WalkerStance_Parked)=0.0
	HoverAdjust(WalkerStance_Crouched)=-20.0

	Begin Object Class=UTVehicleSimHover Name=SimObject
		WheelSuspensionStiffness=400.0
		WheelSuspensionDamping=200.0
		WheelSuspensionBias=0.0
		MaxThrustForce=600.0
		MaxReverseForce=600.0
		LongDamping=0.3
		MaxStrafeForce=600.0
		LatDamping=0.3
		MaxRiseForce=0.0
		UpDamping=0.0
		TurnTorqueFactor=5000.0
		TurnTorqueMax=9000.0
		TurnDamping=1.75
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
		bDisableWheelsWhenOff=false
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	Begin Object Class=UTHoverWheel Name=RThruster
		BoneName="BodyRoot"
		BoneOffset=(X=0,Y=0,Z=-100)
		WheelRadius=70
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

	RespawnTime=45.0

	LengthDarkWalkerWarningCone=7500

	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)

	bHasCustomEntryRadius=true
	CustomEntryRadius=300.0

	bIgnoreStallZ=TRUE
	HUDExtent=250.0
}
