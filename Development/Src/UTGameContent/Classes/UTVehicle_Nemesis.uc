/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Nemesis extends UTVehicle;

/** animation for entering/exiting the vehicle */
var AnimNodeSequence EnterExitSequence;

/** skeletal controllers that should only be enabled when someone is driving the vehicle */
var array<SkelControlBase> DrivingControllers;

/** skeletal controllers that control the height of the turret */
var array<SkelControlBase> TurretHeightControllers;

/** indicates whether the turret is raised or lowered */
enum ETurretHeightSetting
{
	THS_Lowered,
	THS_Normal,
	THS_Raised,
};
var repnotify ETurretHeightSetting TurretHeightSetting;
var ETurretHeightSetting LastTurretHeightSetting;

/** The Template of the Beam to use */
var ParticleSystem BeamTemplate;

/** The particle system to use when in the raised position */
var ParticleSystem OrbPoweredUpParticle;

/** The particle system to use otherwise */
var ParticleSystem RegularOrb;

/** sounds for raising and lowering the turret */
var SoundCue TurretRaiseSound, TurretLowerSound;

/** audio component to play raise/lower sounds on */
var AudioComponent TurretAudioComponent;

/** Normal engine force */
var(Movement) float NormalEngineForce;

/** Engine force when raised */
var(Movement) float RaisedEngineForce;

/** Engine force when lowered */
var(Movement) float LoweredEngineForce;

/** Normal max speed */
var(Movement) float NormalMaxSpeed;

/** max speed when raised */
var(Movement) float RaisedMaxSpeed;

/** max speed when lowered */
var(Movement) float LoweredMaxSpeed;

/** Firing rate increase when raised */
var(Movement) float RaisedFiringRate;

/** Last Turret Height setting transition start time */
var float LastTurretHeightTransitionTime;

/** CameraScale to use when lowered */
var(Movement) float LoweredCameraScale;

/** BaseCameraOffset to use when lowered */
var(Movement) vector LoweredCameraOffset;

/** time to transition to lowered camera mode */
var(Movement) float FastCamTransitionTime;

/** true when want to transition camera back out of lowered camera mode */
var bool bTransitionCameraScale;

/** bRiseReset becomes true when rise becomes zero again after a posture transition, allowing toggling of postures with same key */
var bool bRiseReset;

var AnimNodeBlend CrouchBlend;

replication
{
	if (Role == ROLE_Authority)
		TurretHeightSetting;
}

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	EnterExitSequence = AnimNodeSequence(Mesh.Animations.FindAnimNode('EnterExitNode'));
	`Warn("Could not find EnterExitNode for mesh (" $ Mesh $ ")",EnterExitSequence == None);

	DrivingControllers[DrivingControllers.length] = Mesh.FindSkelControl('GunYawLock');
	DrivingControllers[DrivingControllers.length] = Mesh.FindSkelControl('GunPitchLock');
	DrivingControllers[DrivingControllers.length] = Mesh.FindSkelControl('TurretBaseLock');
	DrivingControllers[DrivingControllers.length] = Mesh.FindSkelControl('TurretBaseLock');

	SetDrivingControllersActive(false);

	TurretHeightControllers[TurretHeightControllers.length] = Mesh.FindSkelControl('HeightControl1');
	TurretHeightControllers[TurretHeightControllers.length] = Mesh.FindSkelControl('HeightControl2');

	CrouchBlend = AnimNodeBlend(Mesh.FindAnimNode('CrouchNode'));
	OrbPower(false);
}

simulated function SetDrivingControllersActive(bool bNowActive)
{
	local int i;

	for (i = 0; i < DrivingControllers.length; i++)
	{
		if (DrivingControllers[i] != None)
		{
			DrivingControllers[i].SetSkelControlActive(bNowActive);
		}
	}
}

simulated function DrivingStatusChanged()
{
	Super.DrivingStatusChanged();

	SetDrivingControllersActive(bDriving);

	SetTurretHeight(bDriving ? THS_Normal : THS_Lowered);
	if (CrouchBlend != None)
	{
		CrouchBlend.SetBlendTarget(0.0, EnterExitSequence.GetAnimPlaybackLength());
	}
}


simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local UTEmitter Beam;

	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	// Handle Beam Effects for the shock beam
	if ( !IsZero(HitLocation) )
	{
		Beam = Spawn( class'UTEmitter',self,,GetEffectLocation(SeatIndex) );
		if (Beam != none)
		{
			Beam.SetTemplate(BeamTemplate);
			Beam.ParticleSystemComponent.SetVectorParameter('ShockBeamEnd', HitLocation );
		}
	}
}

simulated function VehicleWeaponFireEffects(vector HitLocation, int SeatIndex)
{
	if (GetBarrelIndex(SeatIndex) == 0)
	{
		VehicleEvent('TankWeapon01');
	}
	else
	{
		VehicleEvent('TankWeapon02');
	}
}

function SetTurretHeight(ETurretHeightSetting NewHeightSetting)
{
	if (TurretHeightSetting != NewHeightSetting)
	{
		TurretHeightSetting = NewHeightSetting;
		ApplyTurretHeight();
	}
}

simulated function ApplyTurretHeight()
{
	local int i;
	local bool bTurretRaised;
	local name NewAnim;
	local float OldFiringRate;

	LastTurretHeightTransitionTime = WorldInfo.TimeSeconds;
	bTurretRaised = (TurretHeightSetting == THS_Raised);
	for (i = 0; i < TurretHeightControllers.length; i++)
	{
		if (TurretHeightControllers[i] != None)
		{
			TurretHeightControllers[i].SetSkelControlActive(bTurretRaised);
		}
	}

	NewAnim = (TurretHeightSetting == THS_Lowered) ? 'GetOut' : 'GetIn';
	if (EnterExitSequence.AnimSeqName != NewAnim)
	{
		EnterExitSequence.SetAnim(NewAnim);
		EnterExitSequence.PlayAnim(, EnterExitSequence.Rate);
	}
	if (CrouchBlend != None)
	{
		CrouchBlend.SetBlendTarget((TurretHeightSetting == THS_Lowered && bDriving) ? 1.0 : 0.0, EnterExitSequence.GetAnimPlaybackLength());
	}

	TurretAudioComponent.Stop();
	if (TurretHeightSetting < LastTurretHeightSetting)
	{
		TurretAudioComponent.SoundCue = TurretLowerSound;
		if (VehicleEffects.length > 2 && VehicleEffects[2].EffectRef != None)
		{
			VehicleEffects[2].EffectRef.SetTemplate(RegularOrb);
			if (!bDriving) // if the Nemesis is resetting itself after an exit, we don't want this on
			{
				VehicleEffects[2].EffectRef.DeactivateSystem();
			}
		}
	}
	else
	{
		TurretAudioComponent.SoundCue = TurretRaiseSound;
	}
	TurretAudioComponent.Play();

	LastTurretHeightSetting = TurretHeightSetting;

	// change engine torque and firing rate based on posture
	if ( TurretHeightSetting == THS_Raised )
	{
		UTVehicleSimHoverTank(SimObj).MaxThrustForce = RaisedEngineForce;
		AirSpeed = RaisedMaxSpeed;
		VehicleEffects[2].EffectRef.SetTemplate(OrbPoweredUpParticle);
	}
	else if ( TurretHeightSetting == THS_Lowered )
	{
		UTVehicleSimHoverTank(SimObj).MaxThrustForce = LoweredEngineForce;
		AirSpeed = LoweredMaxSpeed;
		bTransitionCameraScale = true; // for when leave lowered posture
	}
	else
	{
		UTVehicleSimHoverTank(SimObj).MaxThrustForce = NormalEngineForce;
		AirSpeed = NormalMaxSpeed;
	}
	UTVehicleSimHoverTank(SimObj).MaxReverseForce = 0.67 * UTVehicleSimHoverTank(SimObj).MaxThrustForce;

	if ( Seats[0].Gun != None )
	{
		OldFiringRate = Seats[0].Gun.FireInterval[0];
		Seats[0].Gun.FireInterval[0] = Seats[0].Gun.Default.FireInterval[0];
		if ( TurretHeightSetting == THS_Raised )
		{
			Seats[0].Gun.FireInterval[0] *= RaisedFiringRate;
		}

		if ( (Seats[0].Gun.FireInterval[0] != OldFiringRate)
			&& Seats[0].Gun.IsTimerActive('RefireCheckTimer')
			&& (Seats[0].Gun.CurrentFireMode == 0) )
		{
			// make currently firing weapon change firing rate
			Seats[0].Gun.ClearTimer('RefireCheckTimer');
			Seats[0].Gun.TimeWeaponFiring(0);
		}
	}
}
function bool DriverEnter(Pawn P)
{

	if(Super.DriverEnter(P))
	{
		OrbPower(true);
		return true;
	}
	else
	{
		return false;
	}
}

simulated function DriverLeft()
{
	OrbPower(false);
	super.DriverLeft();
}

simulated function OrbPower(bool bIsActive)
{
	local MaterialInstanceConstant MIC;
	if(MaterialInstanceConstant(Mesh.Materials[1]) != none)
	{
		MIC = MaterialInstanceConstant(Mesh.Materials[1]);
	}
	else
	{
		MIC = Mesh.CreateMaterialInstance(1);
	}
	if(MIC != none)
	{
		MIC.SetScalarParameterValue('EnergyCorePower', bIsActive?1:0);
	}
}

simulated function int PartialTurn(int original, int desired, float PctTurn)
{
	local float result;

	original = original & 65535;
	desired = desired & 65535;

	if ( abs(original - desired) > 32768 )
	{
		if ( desired > original )
		{
			original += 65536;
		}
		else
		{
			desired += 65536;
		}
	}
	result = original*(1-PctTurn) + desired*PctTurn;
	return (int(result) & 65535);
}

/**
  *  Force fixed view if in lowered posture
  */
simulated function Rotator GetViewRotation()
{
	local rotator FixedRotation, ControllerRotation;
	local float TimeSinceTransition, PctTurn;

	if ( TurretHeightSetting != THS_Lowered )
	{
		if ( bTransitionCameraScale )
		{
			TimeSinceTransition = WorldInfo.TimeSeconds- LastTurretHeightTransitionTime;
			if ( TimeSinceTransition < FastCamTransitionTime )
			{
				FixedRotation = super.GetViewRotation();
				PctTurn = TimeSinceTransition/FastCamTransitionTime;
				FixedRotation.Yaw = PartialTurn(Rotation.Yaw, FixedRotation.Yaw, PctTurn);
				FixedRotation.Pitch = PartialTurn(Rotation.Pitch, FixedRotation.Pitch, PctTurn);
				FixedRotation.Roll = PartialTurn(Rotation.Roll, FixedRotation.Roll, PctTurn);
				return FixedRotation;
			}
		}
		return super.GetViewRotation();
	}

	// swing smoothly around to vehicle rotation
	TimeSinceTransition = WorldInfo.TimeSeconds- LastTurretHeightTransitionTime;
	if ( TimeSinceTransition < FastCamTransitionTime )
	{
		FixedRotation = super.GetViewRotation();
		PctTurn = TimeSinceTransition/FastCamTransitionTime;
		FixedRotation.Yaw = PartialTurn(FixedRotation.Yaw, Rotation.Yaw, PctTurn);
		FixedRotation.Pitch = PartialTurn(FixedRotation.Pitch, Rotation.Pitch, PctTurn);
		FixedRotation.Roll = PartialTurn(FixedRotation.Roll, Rotation.Roll, PctTurn);
		return FixedRotation;
	}
	else
	{
		if ( Controller != None )
		{
	    ControllerRotation = Rotation;
	    ControllerRotation.Roll = 0;
			Controller.SetRotation(ControllerRotation);
		}
		return Rotation;
	}
}

simulated function VehicleCalcCamera(float DeltaTime, int SeatIndex, out vector out_CamLoc, out rotator out_CamRot, out vector CamStart, optional bool bPivotOnly)
{
	local float RealSeatCameraScale;
	local float TimeSinceTransition;

	RealSeatCameraScale = SeatCameraScale;
	if ( TurretHeightSetting == THS_Lowered )
	{
		TimeSinceTransition = WorldInfo.TimeSeconds- LastTurretHeightTransitionTime;
		if ( TimeSinceTransition < FastCamTransitionTime )
		{
			SeatCameraScale = (LoweredCameraScale*TimeSinceTransition + SeatCameraScale*(FastCamTransitionTime-TimeSinceTransition))/FastCamTransitionTime;
			Seats[0].CameraBaseOffset = LoweredCameraOffset * TimeSinceTransition/FastCamTransitionTime;
		}
		else
		{
			Seats[0].CameraBaseOffset = LoweredCameraOffset;
			SeatCameraScale = LoweredCameraScale;
		}
	}
	else if ( bTransitionCameraScale )
	{
		TimeSinceTransition = WorldInfo.TimeSeconds- LastTurretHeightTransitionTime;
		if ( TimeSinceTransition < FastCamTransitionTime )
		{
			SeatCameraScale = (SeatCameraScale*TimeSinceTransition + LoweredCameraScale*(FastCamTransitionTime-TimeSinceTransition))/FastCamTransitionTime;
			Seats[0].CameraBaseOffset = LoweredCameraOffset * (FastCamTransitionTime - TimeSinceTransition)/FastCamTransitionTime;
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

simulated function SetInputs(float InForward, float InStrafe, float InUp)
{
	local ETurretHeightSetting NewHeightSetting;

	Super.SetInputs(InForward, InStrafe, InUp);

	if (Role == ROLE_Authority)
	{
		if ( !bRiseReset )
		{
			bRiseReset = (Rise == 0.f);
		}
		if ( !bRiseReset || (WorldInfo.TimeSeconds - LastTurretHeightTransitionTime < 0.25) || (Rise == 0.f) )
		{
			return;
		}
		bRiseReset = false;

		if ( TurretHeightSetting == THS_Normal )
		{
			//@fixme FIXME: if this ends up being useful in gameplay at all, make bots do it
			NewHeightSetting = (Rise < 0.f || bWantsToCrouch) ? THS_Lowered : THS_Raised;
		}
		else
		{
			NewHeightSetting = THS_Normal;
		}
		SetTurretHeight(NewHeightSetting);
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'TurretHeightSetting')
	{
		ApplyTurretHeight();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/** Don't have vehicle follow where you are looking when zoomed in. */
simulated function bool AllowLookSteer()
{
	local UTVehicleWeapon VWeap;

	VWeap = Seats[0].Gun;
	return (VWeap != None && VWeap.GetZoomedState() != ZST_NotZoomed) ? false : Super.AllowLookSteer();
}

defaultproperties
{
	Seats(0)={(	GunClass=class'UTVWeap_NemesisTurret',
				GunSocket=(GunSocketL,GunSocketR),
				GunPivotPoints=(BarrelOffset),
				TurretControls=(GunPitch,GunYaw),
				CameraTag=ViewSocket,
				CameraOffset=-310,
				ImpactFlashLightClass=class'UTNemesisBeamLight',
				MuzzleFlashLightClass=class'UTNemesisMuzzleFlashLight'))}

	bCanFlip=false
	bTurnInPlace=true
	bCanStrafe=true
	bSeparateTurretFocus=true
	Health=600
	BeamTemplate=VH_Nemesis.Effects.P_VH_Nemesis_NewBeam
	RagdollMesh=SkeletalMesh'VH_Nemesis.Mesh.SK_VH_NemesisDestroyed'
	RagdollAsset=PhysicsAsset'VH_Nemesis.Mesh.SK_VH_NemesisDestroyed_Physics'
	RagdollOffset=(X=200.0,Y=-250.0,Z=80.0)
	RagdollRotationOffset=(Yaw=-20480)
	BigExplosionTemplate=ParticleSystem'VH_Nemesis.Effects.P_VH_Nemesis_Explosion'
	bUseLookSteer=true

	VehicleEffects(0)=(EffectStartTag=TankWeapon01,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_MF_Nemesis',EffectSocket=GunSocketL)
	VehicleEffects(1)=(EffectStartTag=TankWeapon02,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_MF_Nemesis',EffectSocket=GunSocketR)
	VehicleEffects(2)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_Power_Ball',EffectSocket=CoreSocket)
	VehicleEffects(3)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_Nano_Jets',EffectSocket=L_Jet1)
	VehicleEffects(4)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_Nano_Jets',EffectSocket=L_Jet2)
	VehicleEffects(5)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_Nano_Jets',EffectSocket=L_Jet3)
	VehicleEffects(6)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_Nano_Jets',EffectSocket=R_Jet1)
	VehicleEffects(7)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_Nano_Jets',EffectSocket=R_Jet2)
	VehicleEffects(8)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_Nano_Jets',EffectSocket=R_Jet3)
	VehicleEffects(9)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_MF_Nemesis_Constant',EffectSocket=GunSocketL)
	VehicleEffects(10)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Nemesis.Effects.P_MF_Nemesis_Constant',EffectSocket=GunSocketR)
	VehicleEffects(11)=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Vehicle_Damage.P_Vehicle_Damage_1_Nemisis',EffectSocket=DamageSmoke)

	Begin Object Name=CollisionCylinder
		CollisionHeight=100.0
		CollisionRadius=260.000000
		Translation=(X=-50.0,Y=0.0,Z=80.0)
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Nemesis.Mesh.SK_VH_Nemesis'
		AnimTreeTemplate=AnimTree'VH_Nemesis.Anims.AT_VH_Nemesis'
		PhysicsAsset=PhysicsAsset'VH_Nemesis.Mesh.SK_VH_Nemesis_Physics'
		AnimSets.Add(AnimSet'VH_Nemesis.Anims.K_VH_Nemesis')
		Materials(0)=MaterialInstance'VH_Nemesis.Materials.M_VH_Nemesis'
		Materials(1)=MaterialInstance'VH_Nemesis.Materials.M_VH_Nemesis_EnergyCore_Glow'
	End Object

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=NemesisEngineSound
		SoundCue=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_EngineCue'
	End Object
	EngineSound=NemesisEngineSound
	Components.Add(NemesisEngineSound)

	// Collision sound.
	Begin Object Class=AudioComponent Name=NemesisCollideSound
		SoundCue=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_CollideCue'
	End Object
	CollideSound=NemesisCollideSound
	Components.Add(NemesisCollideSound)

	EnterVehicleSound=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_EngineStartCue'
	ExitVehicleSound=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_EngineStopCue'

	Begin Object Class=AudioComponent Name=TurretSound
		bStopWhenOwnerDestroyed=true
	End Object
	TurretAudioComponent=TurretSound
	Components.Add(TurretSound);

	ExplosionSound=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_ExplodeCue'
	TurretLowerSound=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_TurretCrouchCue'
	TurretRaiseSound=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_TurretExtendCue'

	COMOffset=(x=-110.0,y=0.0,z=-50.0)

	Begin Object Class=UTVehicleSimHoverTank Name=SimObject
		WheelSuspensionStiffness=75.0
		WheelSuspensionDamping=4.0
		WheelSuspensionBias=0.0
		MaxThrustForce=900.0
		MaxReverseForce=600.0
		LongDamping=0.85
		LatDamping=0.7
		TurnTorqueMax=7000.0
		TurnDamping=0.8
		StopThreshold=100

		DrivingGroundDist=90.0
		ParkedGroundDist=30.0
		CurrentGroundDist=31.0
		GroundDistAdjustSpeed=30.0
		WheelAdjustFactor=10.0
		bStabilizeStops=true
		StabilizationForceMultiplier=2.0
		CurrentStabilizationMultiplier=2.0
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	Begin Object Class=UTHoverWheel Name=LFWheel
		BoneName="LtTail2"
		BoneOffset=(X=70.0,Y=0.0,Z=-55.0)
		WheelRadius=40
		SuspensionTravel=55
		bPoweredWheel=false
		SteerFactor=1.0
		LongSlipFactor=250.0
		LatSlipFactor=500.0
		HandbrakeLongSlipFactor=250.0
		HandbrakeLatSlipFactor=500.0
	End Object
	Wheels(0)=LFWheel

	Begin Object Class=UTHoverWheel Name=LMWheel
		BoneName="LtTail3"
		BoneOffset=(X=0.0,Y=0.0,Z=-55.0)
		WheelRadius=30
		SuspensionTravel=55
		bPoweredWheel=false
		SteerFactor=0.0
		LongSlipFactor=0.0
		LatSlipFactor=0.0
		HandbrakeLongSlipFactor=0.0
		HandbrakeLatSlipFactor=0.0
	End Object
	Wheels(1)=LMWheel

	Begin Object Class=UTHoverWheel Name=LRWheel
		BoneName="LtTail4"
		BoneOffset=(X=-35.0,Y=0.0,Z=-55.0)
		WheelRadius=30
		SuspensionTravel=55
		bPoweredWheel=false
		SteerFactor=1.0
		LongSlipFactor=0.0
		LatSlipFactor=0.0
		HandbrakeLongSlipFactor=250.0
		HandbrakeLatSlipFactor=250.0
		bUseMaterialSpecificEffects=true
	End Object
	Wheels(2)=LRWheel

	Begin Object Class=UTHoverWheel Name=RFWheel
		BoneName="RtTail2"
		BoneOffset=(X=70.0,Y=0.0,Z=-55.0)
		WheelRadius=40
		SuspensionTravel=55
		bPoweredWheel=false
		SteerFactor=1.0
		LongSlipFactor=250.0
		LatSlipFactor=500.0
		HandbrakeLongSlipFactor=250.0
		HandbrakeLatSlipFactor=500.0
	End Object
	Wheels(3)=RFWheel

	Begin Object Class=UTHoverWheel Name=RMWheel
		BoneName="RtTail3"
		BoneOffset=(X=0.0,Y=0.0,Z=-55.0)
		WheelRadius=30
		SuspensionTravel=55
		bPoweredWheel=false
		SteerFactor=0.0
		LongSlipFactor=0.0
		LatSlipFactor=0.0
		HandbrakeLongSlipFactor=0.0
		HandbrakeLatSlipFactor=0.0
	End Object
	Wheels(4)=RMWheel

	Begin Object Class=UTHoverWheel Name=RRWheel
		BoneName="RtTail4"
		BoneOffset=(X=-35.0,Y=0.0,Z=-55.0)
		WheelRadius=30
		SuspensionTravel=55
		bPoweredWheel=false
		SteerFactor=1.0
		LongSlipFactor=0.0
		LatSlipFactor=0.0
		HandbrakeLongSlipFactor=250.0
		HandbrakeLatSlipFactor=250.0
		bUseMaterialSpecificEffects=true
	End Object
	Wheels(5)=RRWheel

	IconXStart=0
	IconYStart=0.5
	IconXWidth=0.25
	IconYWidth=0.25

	FlagOffset=(Z=160.0)
	bCanCrouch=true
	bStickDeflectionThrottle=true

	NormalEngineForce=2000
	RaisedEngineForce=400
	LoweredEngineForce=2000
	AirSpeed=500.0
	NormalMaxSpeed=500.0
	RaisedMaxSpeed=200.0
	LoweredMaxSpeed=1100.0
	RaisedFiringRate=0.75
	LoweredCameraScale=0.35
	LoweredCameraOffset=(X=-50,Y=0,Z=55)
	FastCamTransitionTime=0.5
	bCameraNeverHidesVehicle=true

	SpawnInSound = SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeInNecris01Cue'
	SpawnOutSound = SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeOutNecris01Cue'
	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)
	OrbPoweredUpParticle=ParticleSystem'VH_Nemesis.Effects.P_Power_Ball_Raised'
	RegularOrb=ParticleSystem'VH_Nemesis.Effects.P_Power_Ball'

	DrivingPhysicalMaterial=PhysicalMaterial'VH_Nemesis.physmat_necristankdriving'
	DefaultPhysicalMaterial=PhysicalMaterial'VH_Nemesis.physmat_necristank'

	PawnHudScene=VHud_Nemesis'UI_Scenes_HUD.VehicleHUDs.VH_Nemesis'

	WheelParticleEffects[0]=(MaterialType=Dirt,ParticleTemplate=None)
	WheelParticleEffects[1]=(MaterialType=Water,ParticleTemplate=ParticleSystem'Envy_Level_Effects_2.Vehicle_Water_Effects.P_Nemesis_Water_Splash')
}
