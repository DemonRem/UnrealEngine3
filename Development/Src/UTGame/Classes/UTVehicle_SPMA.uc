/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_SPMA extends UTVehicle_Deployable
	native(Vehicle)
	abstract;

/** Treads */
var protected private transient	MaterialInstanceConstant TreadMaterialInstance;
/** material parameter controlling tread panner speed */
var name TreadSpeedParameterName;

/** These values are used in positioning the weapons */
var repnotify	rotator	GunnerWeaponRotation;
var	repnotify	vector	GunnerFlashLocation;
var	repnotify	byte	GunnerFlashCount;
var repnotify	byte	GunnerFiringMode;

/** Helpers for quick access to the constraint system */
var UTSkelControl_TurretConstrained GunnerConstraint;

var name LeftBigWheel, LeftSmallWheels[3];
var name RightBigWheel, RightSmallWheels[3];

/** Used to pan the texture on the treads */
var float TreadPan;

var(DC) float DeployedCameraScale;
var(DC) vector DeployedCameraOffset;
var bool bTransitionCameraScale;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
	virtual void ApplyWeaponRotation(INT SeatIndex, FRotator NewRotation);
}

replication
{
	if (bNetDirty)
		GunnerFlashLocation;
	if (!IsSeatControllerReplicationViewer(1))
		GunnerFlashCount, GunnerFiringMode, GunnerWeaponRotation;
}

/**
 * Setup the helpers
 */
simulated function PostBeginPlay()
{
	local MaterialInterface TreadMaterial;

	Super.PostBeginPlay();

	Seats[1].TurretControllers[0].LagDegreesPerSecond = 300;

	if ( bDeleteMe )
		return;

	if (WorldInfo.NetMode != NM_DedicatedServer && Mesh != None)
	{
		// set up material instance (for overlay effects)
		TreadMaterial = Mesh.GetMaterial(1);
		if ( TreadMaterial != None )
		{
			TreadMaterialInstance = new(Outer) class'MaterialInstanceConstant';
			Mesh.SetMaterial(1, TreadMaterialInstance);
			TreadMaterialInstance.SetParent(TreadMaterial);
		}
	}
}

simulated function VehicleCalcCamera(float DeltaTime, int SeatIndex, out vector out_CamLoc, out rotator out_CamRot, out vector CamStart, optional bool bPivotOnly)
{
	local float RealSeatCameraScale;
	local float TimeSinceTransition;

	RealSeatCameraScale = SeatCameraScale;
	if ( DeployedState == EDS_Deployed || DeployedState == EDS_Deploying )
	{
		bTransitionCameraScale = true;
		TimeSinceTransition = WorldInfo.TimeSeconds - LastDeployStartTime;
		if ( TimeSinceTransition < DeployTime )
		{
			SeatCameraScale = (DeployedCameraScale*TimeSinceTransition + SeatCameraScale*(DeployTime-TimeSinceTransition))/DeployTime;
			Seats[0].CameraBaseOffset = DeployedCameraOffset * TimeSinceTransition/DeployTime;
		}
		else
		{
			Seats[0].CameraBaseOffset = DeployedCameraOffset;
			SeatCameraScale = DeployedCameraScale;
		}
	}
	else if ( bTransitionCameraScale )
	{
		TimeSinceTransition = WorldInfo.TimeSeconds- LastDeployStartTime;
		if ( TimeSinceTransition < UnDeployTime )
		{
			SeatCameraScale = (SeatCameraScale*TimeSinceTransition + DeployedCameraScale*(UnDeployTime-TimeSinceTransition))/UnDeployTime;
			Seats[0].CameraBaseOffset = DeployedCameraOffset * (UnDeployTime - TimeSinceTransition)/UnDeployTime;
		}
		else
		{
			bTransitionCameraScale = false;
		}
	}
	super.VehicleCalcCamera(DeltaTime, SeatIndex, out_CamLoc, out_CamRot, CamStart, bPivotOnly);
	SeatCameraScale = RealSeatCameraScale;
	Seats[0].CameraBaseOffset = vect(0,0,0);
}

/**
 * In a network game, this processing the weapon rotations when they come in.  Make sure you update
 * the native version in ApplyWeaponRotation().
 */
simulated function WeaponRotationChanged(int SeatIndex)
{
	local UTVWeap_SPMACannon Gun;

	if ( SeatIndex == 0 )
	{
		if (DeployedState != EDS_Undeployed)
		{
			Gun = UTVWeap_SPMACannon(Seats[0].Gun);
			if (Gun != none )
			{
				if ( Gun.RemoteCamera != none )
				{
					WeaponRotation = Rotator(Gun.RemoteCamera.GetCurrentTargetLocation(Controller) - Location );
				}
				Gun.CalcTargetVelocity();
				WeaponRotation.Pitch = Rotator(Gun.TargetVelocity).Pitch;
			}
		}
	}
	Super.WeaponRotationChanged(SeatIndex);
}

function Actor GetAlternateLockTarget()
{
	local UTVWeap_SPMACannon Gun;

	Gun = UTVWeap_SPMACannon(Seats[0].Gun);
	if (Gun != None)
	{
		return Gun.RemoteCamera;
	}
}

//=================================
// AI Interface

function bool ImportantVehicle()
{
	return true;
}

function bool RecommendLongRangedAttack()
{
	return true;
}

simulated function DriverLeft()
{
	local UTVWeap_SPMACannon Gun;

	Super.DriverLeft();

	Gun = UTVWeap_SPMACannon(Seats[0].Gun);

	if ( Gun != none && Gun.RemoteCamera != none && Gun.RemoteCamera.bDeployed )
	{
		Gun.RemoteCamera.Disconnect();
	}
}

simulated state UnDeploying
{
	simulated function BeginState(name PreviousStateName)
	{
		local UTVWeap_SPMACannon Gun;

		Super.BeginState(PreviousStateName);

		Gun = UTVWeap_SPMACannon(Seats[0].Gun);
		if (Gun != none && Gun.RemoteCamera != none)
		{
			Gun.RemoteCamera.Disconnect();
		}
	}
}

defaultproperties
{
	Health=800
	MaxDesireability=0.6
	MomentumMult=0.3
	bCanFlip=false
	bTurnInPlace=false
	bCanStrafe=false
	bSeparateTurretFocus=false
	GroundSpeed=650
	MaxSpeed=1000
	BaseEyeheight=0
	Eyeheight=0
	bUseLookSteer=true

	COMOffset=(x=0.0,y=0.0,z=-100.0)

	Begin Object Class=UTVehicleSimCar Name=SimObject
		WheelSuspensionStiffness=300.0
		WheelSuspensionDamping=7.0
		WheelSuspensionBias=0.0
		ChassisTorqueScale=0.1
	WheelInertia=0.9
	MaxSteerAngleCurve=(Points=((InVal=0,OutVal=20.0),(InVal=700.0,OutVal=15.0)))
	SteerSpeed=40
	MaxBrakeTorque=50.0
		StopThreshold=500
		TorqueVSpeedCurve=(Points=((InVal=-300.0,OutVal=0.0),(InVal=0.0,OutVal=120.0),(InVal=700.0,OutVal=0.0)))
		EngineRPMCurve=(Points=((InVal=-500.0,OutVal=2500.0),(InVal=0.0,OutVal=500.0),(InVal=599.0,OutVal=5000.0),(InVal=600.0,OutVal=3000.0),(InVal=949.0,OutVal=5000.0),(InVal=950.0,OutVal=3000.0),(InVal=1100.0,OutVal=5000.0)))
	EngineBrakeFactor=0.1
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	Begin Object Class=UTVehicleWheel Name=RFWheel
		BoneName="RtFrontTire"
		BoneOffset=(X=0.0,Y=33,Z=5.0)
		WheelRadius=60
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=1.0
		SkelControlName="Rt_Ft_Tire"
		Side=SIDE_Right
	End Object
	Wheels(0)=RFWheel

	Begin Object Class=UTVehicleWheel Name=LFWheel
		BoneName="LtFrontTire"
		BoneOffset=(X=0.0,Y=-33,Z=5.0)
		WheelRadius=60
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=1.0
		SkelControlName="Lt_Ft_Tire"
		Side=SIDE_Left
	End Object
	Wheels(1)=LFWheel

	Begin Object Class=UTVehicleWheel Name=RRWheel
		BoneName="RtTread_Wheel3"
		BoneOffset=(X=25.0,Y=0,Z=15.0)
		WheelRadius=60
		SuspensionTravel=45
		bPoweredWheel=true
		SteerFactor=0.0
		SkelControlName="Rt_Tread_Wheels_1_4"
		Side=SIDE_Right
	End Object
	Wheels(2)=RRWheel

	Begin Object Class=UTVehicleWheel Name=LRWheel
		BoneName="LtTread_Wheel3"
		BoneOffset=(X=25.0,Y=0,Z=15.0)
		WheelRadius=60
		SuspensionTravel=45
		bPoweredWheel=true
		SteerFactor=0.0
		SkelControlName="Lt_Tread_Wheels_1_4"
		Side=SIDE_Left
	End Object
	Wheels(3)=LRWheel

	LeftBigWheel="LfFrontTire"
	LeftSmallWheels(0)="Lt_Tread_Wheels_1_4"
	LeftSmallWheels(1)="Lt_Tread_Wheel2"
	LeftSmallWheels(2)="Lt_Tread_Wheel3"

	RightBigWheel="RtFrontTire"
	RightSmallWheels(0)="Rt_Tread_Wheels_1_4"
	RightSmallWheels(1)="Rt_Tread_Wheel2"
	RightSmallWheels(2)="Rt_Tread_Wheel3"

	ProximityShakeRadius=300.0
	ProximityShake=(OffsetMag=(X=2.5,Y=0.0,Z=10.0),OffsetRate=(X=35.0,Y=35.0,Z=35.0),OffsetTime=4.0)
	TurnTime=2.0

	RespawnTime=45.0

	bStickDeflectionThrottle=true

	DeployedCameraScale=2.5
	DeployedCameraOffset=(Z=100)
	HUDExtent=140.0
}
