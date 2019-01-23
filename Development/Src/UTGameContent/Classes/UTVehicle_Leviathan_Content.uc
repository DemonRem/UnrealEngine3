/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Leviathan_Content extends UTVehicle_Leviathan;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	Shield[0] = SpawnShield( vect(170,-180,175), 'LT_Front_TurretPitch');
	Shield[1] = SpawnShield( vect(170,180,175), 'RT_Front_TurretPitch');
	Shield[2] = SpawnShield( vect(-125,-180,175), 'LT_Rear_TurretPitch');
	Shield[3] = SpawnShield( vect(-125,180,175), 'RT_Rear_TurretPitch');
	Mesh.AttachComponent(TurretCollision[0],'LT_Front_TurretYaw');
	Mesh.AttachComponent(TurretCollision[1],'RT_Front_TurretYaw');
	Mesh.AttachComponent(TurretCollision[2],'LT_Rear_TurretYaw');
	Mesh.AttachComponent(TurretCollision[3],'RT_Rear_TurretYaw');

	CachedTurrets[0] = UTSkelControl_TurretConstrained(mesh.FindSkelControl('DriverTurret_Yaw'));
	CachedTurrets[1] = UTSkelControl_TurretConstrained(mesh.FindSkelControl('Driverturret_Pitch'));
	CachedTurrets[2] = UTSkelControl_TurretConstrained(mesh.FindSkelControl('MainTurret_Yaw'));
	CachedTurrets[3] = UTSkelControl_TurretConstrained(mesh.FindSkelControl('MainTurret_Pitch'));
	CachedTurrets[2].DesiredBoneRotation.Yaw = 32767;
}

simulated function SetVehicleDeployed()
{
	Super.SetVehicleDeployed();
	Seats[0].CameraTag = 'BigGunCamera';
	Seats[0].GunSocket.Length = 1;
	Seats[0].GunSocket[0] = BigBeamSocket;
	Seats[0].MuzzleFlashLightClass = class'UTGame.UTLeviathanMuzzleFlashLight';
}

simulated function SetVehicleUndeployed()
{

	Super.SetVehicleUndeployed();
	Seats[0].CameraTag = 'DriverCamera';
	Seats[0].GunSocket.Length = 2;
	Seats[0].GunSocket[0] = 'Lt_DriverBarrel';
	Seats[0].GunSocket[1] = 'Rt_DriverBarrel';
	Seats[0].MuzzleFlashLightClass = class'UTGame.UTRocketMuzzleFlashLight';
}

simulated function VehicleWeaponFired(bool bViaReplication, vector HitLocation, int SeatIndex)
{
	local UTSkelControl_Rotate SKR;

	Super.VehicleWeaponFired(bViaReplication, HitLocation, SeatIndex);

	if (SeatIndex == 3)	// Minigun Turret
	{
		SKR = UTSkelControl_Rotate(Mesh.FindSkelControl('StingerRotate'));
		if (SKR != None)
		{
			SKR.DesiredBoneRotation.Roll += StingerTurretTurnRate;
		}
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		// not even close to big enough, but this is as big as the path network supports
		CollisionHeight=+100.0
		CollisionRadius=+260.0
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Leviathan.Mesh.SK_VH_Leviathan'
		PhysicsAsset=PhysicsAsset'VH_Leviathan.Mesh.SK_VH_Leviathan_Physics'
		AnimTreeTemplate=AnimTree'VH_Leviathan.Anims.AT_VH_Leviathan'
		AnimSets.Add(AnimSet'VH_Leviathan.Anims.K_VH_Leviathan')
	End Object

	RagdollMesh=SkeletalMesh'VH_Leviathan.Mesh.SK_VH_Leviathan'
	RagdollAsset=PhysicsAsset'VH_Leviathan.Mesh.SK_VH_Leviathan_PhysicsDestroyed'

	Begin Object Class=CylinderComponent Name=TurretCollisionCylinderLF
	    CollisionRadius=+0068.000000
		CollisionHeight=+0074.000000
		Translation=(Z=48)
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=true
		CollideActors=true
	End Object
	TurretCollision(0)=TurretCollisionCylinderLF

	Begin Object Class=CylinderComponent Name=TurretCollisionCylinderRF
	    CollisionRadius=+0068.000000
		CollisionHeight=+0074.000000
		Translation=(Z=48)
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=true
		CollideActors=true
	End Object
	TurretCollision(1)=TurretCollisionCylinderRF

	Begin Object Class=CylinderComponent Name=TurretCollisionCylinderLR
	    CollisionRadius=+0068.000000
		CollisionHeight=+0074.000000
		Translation=(Z=48)
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=true
		CollideActors=true
	End Object
	TurretCollision(2)=TurretCollisionCylinderLR

	Begin Object Class=CylinderComponent Name=TurretCollisionCylinderRR
	    CollisionRadius=+0068.000000
		CollisionHeight=+0074.000000
		Translation=(Z=48)
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=true
		CollideActors=true
	End Object
	TurretCollision(3)=TurretCollisionCylinderRR

	DrawScale=1.25

	Seats(0)={(	GunClass=class'UTVWeap_LeviathanPrimary',
				GunSocket=(Lt_DriverBarrel,Rt_DriverBarrel),
				TurretVarPrefix="",
				TurretControls=(DriverTurret_Yaw,DriverTurret_Pitch),
				GunPivotPoints=(DriverTurretYaw,MainTurretYew),
				MuzzleFlashLightClass=class'UTGame.UTRocketMuzzleFlashLight',
				CameraTag=DriverCamera,
				CameraOffset=-800)}

	Seats(1)={( GunClass=class'UTVWeap_LeviathanTurretBeam',
				GunSocket=(LF_TurretBarrel_L,LF_TurretBarrel_R),
				CameraTag=LF_TurretCamera,
				TurretVarPrefix="LFTurret",
				CameraEyeHeight=50,
				CameraOffset=-100,
				bSeatVisible=true,
				SeatOffset=(X=4,Z=72),
				SeatBone=LT_Front_TurretPitch,
				TurretControls=(LT_Front_TurretYaw,LT_Front_TurretPitch),
				GunPivotPoints=(Lt_Front_TurretYaw,Lt_Front_TurretYaw),
				MuzzleFlashLightClass=class'UTGame.UTTurretMuzzleFlashLight',
				ViewPitchMin=-10402,
				DriverDamageMult=0.0)}

	Seats(2)={( GunClass=class'UTVWeap_LeviathanTurretRocket',
				GunSocket=(RF_TurretBarrel,RF_TurretBarrel,RF_TurretBarrel),
				TurretVarPrefix="RFTurret",
				SeatOffset=(X=4,Z=72),
				CameraEyeHeight=50,
				CameraOffset=-100,
				bSeatVisible=true,
				SeatBone=RT_Front_TurretPitch,
				TurretControls=(RT_Front_TurretYaw,RT_Front_TurretPitch),
				GunPivotPoints=(Rt_Front_TurretYaw,Rt_Front_TurretYaw),
				CameraTag=RF_TurretCamera,
				MuzzleFlashLightClass=class'UTGame.UTRocketMuzzleFlashLight',
				ViewPitchMin=-10402,
				DriverDamageMult=0.0)}

	Seats(3)={( GunClass=class'UTVWeap_LeviathanTurretStinger',
				GunSocket=(LR_TurretBarrel),
				TurretVarPrefix="LRTurret",
				CameraEyeHeight=50,
				CameraOffset=-100,
				bSeatVisible=true,
				SeatBone=LT_Rear_TurretPitch,
				SeatOffset=(X=4,Z=72),
				TurretControls=(LT_Rear_TurretYaw,LT_Rear_TurretPitch),
				GunPivotPoints=(Lt_Rear_TurretYaw,Lt_Rear_TurretYaw),
				CameraTag=LR_TurretCamera,
				MuzzleFlashLightClass=class'UTGameContent.UTStingerTurretMuzzleFlashLight',
				ViewPitchMin=-10402,
				DriverDamageMult=0.0)}

	Seats(4)={( GunClass=class'UTVWeap_LeviathanTurretShock',
				GunSocket=(RR_TurretBarrel),
				TurretVarPrefix="RRTurret",
				TurretControls=(RT_Rear_TurretYaw,RT_Rear_TurretPitch),
				CameraEyeHeight=50,
				CameraOffset=-100,
				bSeatVisible=true,
				SeatOffset=(X=4,Z=72),
				SeatBone=RT_Rear_TurretPitch,
				GunPivotPoints=(Rt_Rear_TurretPitch,Rt_Rear_TurretPitch),
				CameraTag=RR_TurretCamera,
				ViewPitchMin=-10402,
				DriverDamageMult=0.0)}

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=EngineIdleSound
		SoundCue=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_EngineIdle'
	End Object
	EngineSound=EngineIdleSound
	Components.Add(EngineIdleSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=CollideSound
		SoundCue=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_Collide'
	End Object
	CollideSound=CollideSound
	Components.Add(CollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_EngineStart'
	ExitVehicleSound=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_EngineStop'

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	SquealThreshold=0.05
	EngineStartOffsetSecs=2.0
	EngineStopOffsetSecs=1.0

	VehicleEffects(0)=(EffectStartTag=Damage0Smoke,EffectEndTag=NoDamage0Smoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Vehicle_Damage_1',EffectSocket=LF_TurretDamageSmoke)
	VehicleEffects(1)=(EffectStartTag=Damage1Smoke,EffectEndTag=NoDamage1Smoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Vehicle_Damage_1',EffectSocket=RF_TurretDamageSmoke)
	VehicleEffects(2)=(EffectStartTag=Damage2Smoke,EffectEndTag=NoDamage2Smoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Vehicle_Damage_1',EffectSocket=LR_TurretDamageSmoke)
	VehicleEffects(3)=(EffectStartTag=Damage3Smoke,EffectEndTag=NoDamage3Smoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Tests.Effects.P_Vehicle_Damage_1',EffectSocket=RR_TurretDamageSmoke)

	VehicleEffects(4)=(EffectStartTag=StartDeploy,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_Deploy',EffectSocket=DeployEffectSocket)

	VehicleEffects(5)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.PS_Leviathan_Exhaust_Smoke',EffectSocket=ExhaustSocket)
	VehicleEffects(6)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.PS_Leviathan_Exhaust_Smoke',EffectSocket=ExhaustSocketB)

	// Weapon Muzzle Flashes

	VehicleEffects(7)=(EffectStartTag=TurretBeamMF_L,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_TurretBeamMF',EffectSocket=LF_TurretBarrel_L)
	VehicleEffects(8)=(EffectStartTag=TurretBeamMF_R,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_TurretBeamMF',EffectSocket=LF_TurretBarrel_R)
	VehicleEffects(9)=(EffectStartTag=TurretRocketMF,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_TurretRocketMF',EffectSocket=RF_TurretBarrel)
	VehicleEffects(10)=(EffectStartTag=TurretStingerMF,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_TurretStingerMF',EffectSocket=LR_TurretBarrel)
	VehicleEffects(11)=(EffectStartTag=TurretShockMF,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_TurretShockMF',EffectSocket=RR_TurretBarrel)

	VehicleEffects(12)=(EffectStartTag=DriverMF_L,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.PS_VH_Leviathan_DriverMF',EffectSocket=Lt_DriverBarrel)
	VehicleEffects(13)=(EffectStartTag=DriverMF_R,EffectTemplate=ParticleSystem'VH_Leviathan.Effects.PS_VH_Leviathan_DriverMF',EffectSocket=Rt_DriverBarrel)

	TeamBeaconOffset=(z=460.0)

	IdleAnim(0)=InActiveStill
	IdleAnim(1)=ActiveStill
	GetInAnim(0)=none
	GetOutAnim(0)=none
	DeployAnim(0)=Deploying
	DeployAnim(1)=UnDeploying

	DeployTime = 8.3667;
	UnDeployTime = 6.5;

	BeamTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_LaserBeam'
	BeamEndpointVarName=ShockBeamEnd

	BigBeamTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_BigBeam'
	BigBeamSocket=BigGunBarrel
	BigBeamEndpointVarName=BigBeamDest

	DeploySound=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_Deploy'
	UndeploySound=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_Deploy'

	// Vehicle exit sound.
	Begin Object Class=AudioComponent Name=BigBeamFireAC
		SoundCue=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_CannonFire'
	End Object
	BigBeamFiresound=BigBeamFireAC
	Components.Add(BigBeamFireAC);

	MainTurretPivot=MainTurretPitch
	DriverTurretPivot=DriverTurretYaw

	TurretExplosionSound=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_BlowSection'
	TurretActivate=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_TurretActivate'
	TurretDeactivate=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_TurretDeactivate'
	DrivingAnim=Leviathan_idle_sitting

	ExplosionSound=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_Explode'

	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)

	PawnHudScene=VHud_Leviathan'UI_Scenes_HUD.VehicleHUDs.VH_Leviathan'

	ShieldClass=class'UTLeviathanShield'
}
