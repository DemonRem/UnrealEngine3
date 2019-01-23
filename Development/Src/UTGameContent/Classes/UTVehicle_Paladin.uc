/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicle_Paladin extends UTVehicle;

/** actor used for the shield */
var UTPaladinShield ShockShield;
/** indicates whether or not the shield is currently active */
var repnotify bool bShieldActive;
/** used for replicating shield hits to make it flash on clients */
var repnotify byte ShieldHitCount;

replication
{
	if (Role == ROLE_Authority)
		bShieldActive, ShieldHitCount;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	// @warning: the shield MUST NOT be owned by us, or projectiles we shoot will not collide with it and the combo proximity blast won't work
	ShockShield = Spawn(class'UTPaladinShield');
	ShockShield.SetBase(self,, Mesh, 'Shield_Pitch');
}

function bool ImportantVehicle()
{
	return true;
}

function IncomingMissile(Projectile P)
{
	local AIController C;

	C = AIController(Controller);
	if (C != None && C.Skill >= 2.0)
	{
		UTVWeap_PaladinGun(Weapon).ShieldAgainstIncoming(P);
	}
}

function bool Dodge(eDoubleClickDir DoubleClickMove)
{
	UTVWeap_PaladinGun(Weapon).ShieldAgainstIncoming();
	return false;
}

simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local vector ShieldHitLocation, ShieldHitNormal;

	if ( Role < ROLE_Authority )
		return;

	// don't take damage if the shield is active and it hit the shield component or skipped it for some reason but should have hit it
	if ( ShockShield == None || !(bShieldActive && ShockShield.bFullyActive ) ||
		( HitInfo.HitComponent != ShockShield.CollisionComponent && ( IsZero(Momentum) || HitLocation == Location || DamageType == None
								|| !ClassIsChildOf(DamageType, class'UTDamageType')
								|| !TraceComponent(ShieldHitLocation, ShieldHitNormal, ShockShield.CollisionComponent, HitLocation, HitLocation - 2000.f * Normal(Momentum)) ) ) )
	{
		// Don't take self inflicated damage from proximity explosion
		if (DamageType != class'UTDmgType_PaladinProximityExplosion' || EventInstigator != Controller)
		{
			Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
		}
	}
	else if ( !WorldInfo.GRI.OnSameTeam(self, EventInstigator) )
	{
		UTVWeap_PaladinGun(Weapon).NotifyShieldHit(Damage);
		ShieldHit();
	}
}

simulated function SetShieldActive(int SeatIndex, bool bNowActive)
{
	if (SeatIndex == 0)
	{
		bShieldActive = bNowActive;
		if (ShockShield != None)
		{
			ShockShield.SetActive(bNowActive);
		}
	}
}

simulated function ShieldHit()
{
	// FIXME: play effects
	ShieldHitCount++;
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bShieldActive')
	{
		SetShieldActive(0, bShieldActive);
	}
	else if (VarName == 'ShieldHitCount')
	{
		ShieldHit();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function BlowupVehicle()
{
	if (ShockShield != None)
	{
		ShockShield.Destroy();
	}

	Super.BlowupVehicle();
}

simulated event Destroyed()
{
	Super.Destroyed();

	if (ShockShield != None)
	{
		ShockShield.Destroy();
	}
}

/**
 * No muzzle flashlight on shield
 */
simulated function CauseMuzzleFlashLight(int SeatIndex)
{
	if ( bShieldActive )
		return;

	Super.CauseMuzzleFlashLight(SeatIndex);
}


/** Don't have vehicle follow where you are looking when using shield. */
simulated function bool AllowLookSteer()
{
	return (!bShieldActive && Super.AllowLookSteer());
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=100.0
		CollisionRadius=260.0
		Translation=(Z=100.0)
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Paladin.Mesh.SK_VH_Paladin'
		AnimTreeTemplate=AnimTree'VH_Paladin.Anims.AT_VH_Paladin'
		PhysicsAsset=PhysicsAsset'VH_Paladin.Mesh.SK_VH_Paladin_Physics'
		AnimSets.Add(AnimSet'VH_Paladin.Anims.VH_Paladin_Anims')
	End Object

	DrawScale=1.35

	Seats(0)={(	GunClass=class'UTVWeap_PaladinGun',
				CameraTag=ViewSocket,
				CameraOffset=-400,
				TurretControls=(Turret_Pitch,Turret_Yaw),
				GunSocket=(GunSocket),
				GunPivotPoints=(Cannon_Pitch),
				MuzzleFlashLightClass=class'UTGame.UTShockComboExplosionLight')}


	VehicleEffects(0)=(EffectStartTag=PrimanyFire,EffectTemplate=ParticleSystem'VH_Paladin.Effects.P_VH_Paladin_Muzzleflash',EffectSocket=GunSocket)
	VehicleEffects(1)=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Vehicle_Damage_1',EffectSocket=DamageSmoke01)

	Health=800

	DriverDamageMult=0.0
	MomentumMult=0.8

	GroundSpeed=700.0
	AirSpeed=1000.0
	MaxSpeed=1200
	BaseEyeheight=40
	Eyeheight=40
	MaxDesireability=0.6
	ObjectiveGetOutDist=1500.0
	bDriverHoldsFlag=false
	bSeparateTurretFocus=true

	bCanFlip=false
	bUseLookSteer=true

	RagdollMesh=SkeletalMesh'VH_Paladin.Mesh.SK_VH_PaladinDestroyed'
	RagdollAsset=PhysicsAsset'VH_Paladin.Mesh.SK_VH_PaladinDestroyed_Physics'
	BigExplosionTemplate=ParticleSystem'VH_Paladin.Effects.P_VH_Paladin_DeathExplosion'

	COMOffset=(x=0.0,y=0.0,z=-100.0)

	Begin Object Class=UTVehicleSimCar Name=SimObject
		WheelSuspensionStiffness=500.0
		WheelSuspensionDamping=6.0
		WheelSuspensionBias=0.0
		ChassisTorqueScale=0.1
    	WheelInertia=0.75
		LSDFactor=1.0
		MaxSteerAngleCurve=(Points=((InVal=0,OutVal=20.0),(InVal=700.0,OutVal=15.0)))
		SteerSpeed=90
    	MaxBrakeTorque=75.0
		StopThreshold=100
		TorqueVSpeedCurve=(Points=((InVal=-300.0,OutVal=0.0),(InVal=0.0,OutVal=100.0),(InVal=1000.0,OutVal=0.0)))
		EngineRPMCurve=(Points=((InVal=-500.0,OutVal=2500.0),(InVal=0.0,OutVal=500.0),(InVal=599.0,OutVal=5000.0),(InVal=600.0,OutVal=3000.0),(InVal=949.0,OutVal=5000.0),(InVal=950.0,OutVal=3000.0),(InVal=1100.0,OutVal=5000.0)))
		EngineBrakeFactor=0.1
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	// Engine sound.
	Begin Object Class=AudioComponent Name=PaladinEngineSound
		SoundCue=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_EngineLoop'
	End Object
	EngineSound=PaladinEngineSound
	Components.Add(PaladinEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=PaladinCollideSound
		SoundCue=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_Collide'
	End Object
	CollideSound=PaladinCollideSound
	Components.Add(PaladinCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_Start'
	ExitVehicleSound=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_Stop'

	Begin Object Class=UTVehicleWheel Name=RRWheel
		BoneName="RtTire04"
		BoneOffset=(X=0.0,Y=35,Z=0.0)
		WheelRadius=48.0
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=-1.0
		SkelControlName="RtTire04"
		Side=SIDE_Right
	End Object
	Wheels(0)=RRWheel

	Begin Object Class=UTVehicleWheel Name=RMRWheel
		BoneName="RtTire03"
		BoneOffset=(X=0.0,Y=35,Z=0.0)
		WheelRadius=48.0
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=-0.5
		SkelControlName="RtTire03"
		Side=SIDE_Right
	End Object
	Wheels(1)=RMRWheel

	Begin Object Class=UTVehicleWheel Name=RMFWheel
		BoneName="RtTire02"
		BoneOffset=(X=0.0,Y=35,Z=0.0)
		WheelRadius=48.0
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=0.5
		SkelControlName="RtTire02"
		Side=SIDE_Right
	End Object
	Wheels(2)=RMFWheel

	Begin Object Class=UTVehicleWheel Name=RFWheel
		BoneName="RtTire01"
		BoneOffset=(X=0.0,Y=35,Z=0.0)
		WheelRadius=48.0
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=1.0
		SkelControlName="RtTire01"
		Side=SIDE_Right
	End Object
	Wheels(3)=RFWheel

	Begin Object Class=UTVehicleWheel Name=LRWheel
		BoneName="LtTire04"
		BoneOffset=(X=0.0,Y=-35,Z=0.0)
		WheelRadius=48.0
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=-1.0
		SkelControlName="LtTire04"
		Side=SIDE_Left
	End Object
	Wheels(4)=LRWheel

	Begin Object Class=UTVehicleWheel Name=LMRWheel
		BoneName="LtTire03"
		BoneOffset=(X=0.0,Y=-35,Z=0.0)
		WheelRadius=48.0
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=-0.5
		SkelControlName="LtTire03"
		Side=SIDE_Left
	End Object
	Wheels(5)=LMRWheel

	Begin Object Class=UTVehicleWheel Name=LMFWheel
		BoneName="LtTire02"
		BoneOffset=(X=0.0,Y=-35,Z=0.0)
		WheelRadius=48.0
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=0.5
		SkelControlName="LtTire02"
		Side=SIDE_Left
	End Object
	Wheels(6)=LMFWheel

	Begin Object Class=UTVehicleWheel Name=LFWheel
		BoneName="LtTire01"
		BoneOffset=(X=0.0,Y=-35,Z=0.0)
		WheelRadius=48.0
		SuspensionTravel=60
		bPoweredWheel=true
		SteerFactor=1.0
		SkelControlName="LtTire01"
		Side=SIDE_Left
	End Object
	Wheels(7)=LFWheel

	SpawnInTemplates[0]=ParticleSystem'VH_Paladin.Effects.P_VH_Paladin_Spawn_Red'
	SpawnInTemplates[1]=ParticleSystem'VH_Paladin.Effects.P_VH_Paladin_Spawn_Blue'
	SpawnInTime=5.00
	RespawnTime=30.0
	FlagBone=RtAntenna03
	FlagOffset=(Z=30.0)
	bStickDeflectionThrottle=true

	ExplosionSound=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_Explode'
	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)

	PawnHudScene=VHud_Paladin'UI_Scenes_HUD.VehicleHUDs.VH_Paladin'

}
