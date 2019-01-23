/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Cicada_Content extends UTVehicle_Cicada;

var array<UTDecoy> Decoys;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if (Mesh != None)
	{
		JetControl = UTSkelControl_JetThruster(Mesh.FindSkelControl('CicadaJet'));
	}
}

function IncomingMissile(Projectile P)
{
	local UTVWeap_CicadaTurret Turret;

	// notify the turret weapon
	if (Seats.length > 1)
	{
		Turret = UTVWeap_CicadaTurret(Seats[1].Gun);
		if (Turret != None)
		{
			Turret.IncomingMissile(P);
		}
	}

	Super.IncomingMissile(P);
}

/** Check all of the active decoys and see if any take effect. */
function Actor GetHomingTarget(UTProjectile Seeker, Controller InstigatedBy)
{
	local int i;

	for (i = 0; i < Decoys.Length; i++)
	{
		if (Decoys[i].CheckRange(Seeker))
		{
			return Decoys[i];
		}
	}

	return Super.GetHomingTarget(Seeker, InstigatedBy);
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=+70.0
		CollisionRadius=+240.0
		Translation=(X=-40.0,Y=0.0,Z=40.0)
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Cicada.Mesh.SK_VH_Cicada'
		AnimTreeTemplate=AnimTree'VH_Cicada.Anims.AT_VH_Cicada'
		PhysicsAsset=PhysicsAsset'VH_Cicada.Mesh.SK_VH_Cicada_Physics'
		AnimSets.Add(AnimSet'VH_Cicada.Anims.VH_Cicada_Anims')
	End Object

	DrawScale=1.3

	Health=500
	RagdollMesh=SkeletalMesh'VH_Cicada.Mesh.SK_VH_CicadaDestroyed'
	RagdollAsset=PhysicsAsset'VH_Cicada.Mesh.SK_VH_CicadaDestroyed_Physics'
	BigExplosionTemplate=ParticleSystem'VH_Cicada.effects.P_VH_Cicada_DeathEffect'
	RagdollRotationOffset=(Yaw=-16384)

	Seats.Empty
	Seats(0)={(	GunClass=class'UTVWeap_CicadaMissileLauncher',
				GunSocket=(Gun_Socket_01,Gun_Socket_02),
				CameraTag=ViewSocket,
				TurretControls=(LauncherA,LauncherB),
				CameraOffset=-400,
				GunPivotPoints=(Main)) }

	Seats(1)={(	GunClass=class'UTVWeap_CicadaTurret',
				GunSocket=(Turret_Gun_Socket_01,Turret_Gun_Socket_02,Turret_Gun_Socket_03,Turret_Gun_Socket_04),
				TurretVarPrefix="Turret",
				TurretControls=(Turret_Rotate),
				CameraTag=Turret_ViewSocket,
				CameraOffset=0,
				GunPivotPoints=(MainTurret_Pitch),
				CameraEyeHeight=0,
				ViewPitchMin=-14000.0,
				ViewPitchMax=1.0)}


	TurretBeamTemplate=ParticleSystem'WP_ShockRifle.Particles.P_WP_ShockRifle_Beam'

	VehicleEffects.Empty
	VehicleEffects(0)=(EffectStartTag=CicadaWeapon01,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_MissileFlash',EffectSocket=Gun_Socket_01)
	VehicleEffects(1)=(EffectStartTag=CicadaWeapon02,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_MissileFlash',EffectSocket=Gun_Socket_02)
	VehicleEffects(2)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Contrail',EffectSocket=LeftWingtip)
	VehicleEffects(3)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Contrail',EffectSocket=RightWingtip)
	VehicleEffects(4)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Contrail',EffectSocket=LeftFrtTrail)
	VehicleEffects(5)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Contrail',EffectSocket=RightFrtTrail)

//	VehicleEffects(6)=(EffectStartTag=TurretWeapon01,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Muzzleflash',EffectSocket=Turret_Gun_Socket_01)
//	VehicleEffects(7)=(EffectStartTag=TurretWeapon02,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Muzzleflash',EffectSocket=Turret_Gun_Socket_02)
	VehicleEffects(6)=(EffectStartTag=TurretWeapon01,EffectTemplate=ParticleSystem'VH_Manta.Effects.PS_Manta_Gun_MuzzleFlash',EffectSocket=Turret_Gun_Socket_01)
	VehicleEffects(7)=(EffectStartTag=TurretWeapon02,EffectTemplate=ParticleSystem'VH_Manta.Effects.PS_Manta_Gun_MuzzleFlash',EffectSocket=Turret_Gun_Socket_02)
	VehicleEffects(8)=(EffectStartTag=TurretWeapon03,EffectTemplate=ParticleSystem'VH_Manta.Effects.PS_Manta_Gun_MuzzleFlash',EffectSocket=Turret_Gun_Socket_03)
	VehicleEffects(9)=(EffectStartTag=TurretWeapon04,EffectTemplate=ParticleSystem'VH_Manta.Effects.PS_Manta_Gun_MuzzleFlash',EffectSocket=Turret_Gun_Socket_04)

	VehicleEffects(10)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_GroundEffect',EffectSocket=GroundEffectBase)
	VehicleEffects(11)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Exhaust',EffectSocket=LeftExhaust)
	VehicleEffects(12)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Exhaust',EffectSocket=RightExhaust)
	VehicleEffects(13)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Contrail',EffectSocket=LeftWingtipb)
	VehicleEffects(14)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Contrail',EffectSocket=RightWingtipb)

	VehicleAnims(0)=(AnimTag=Created,AnimSeqs=(InActiveStill),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=CicadaPlayer)
	VehicleAnims(1)=(AnimTag=EngineStart,AnimSeqs=(GetIn),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=CicadaPlayer)
	VehicleAnims(2)=(AnimTag=Idle,AnimSeqs=(Idle),AnimRate=1.0,bAnimLoopLastSeq=true,AnimPlayerName=CicadaPlayer)
	VehicleAnims(3)=(AnimTag=EngineStop,AnimSeqs=(GetOut),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=CicadaPlayer)

	JetEffectIndices=(9,10)
	ContrailEffectIndices=(2,3,4,5,11,12)
	GroundEffectIndices=(8)

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=RaptorEngineSound
		SoundCue=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_EngineLoop'
	End Object
	EngineSound=RaptorEngineSound
	Components.Add(RaptorEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=RaptorCollideSound
		SoundCue=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_Collide'
	End Object
	CollideSound=RaptorCollideSound
	Components.Add(RaptorCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_Start'

	ExitVehicleSound=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_Stop'

	// Scrape sound.
	Begin Object Class=AudioComponent Name=BaseScrapeSound
		SoundCue=SoundCue'A_Gameplay.A_Gameplay_Onslaught_MetalScrape01Cue'
	End Object
	ScrapeSound=BaseScrapeSound
	Components.Add(BaseScrapeSound);

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	EngineStartOffsetSecs=2.0
	EngineStopOffsetSecs=1.0

	IconXStart=0.5
	IconYStart=0.5
	IconXWidth=0.25
	IconYWidth=0.25

	SpawnInTemplates[0]=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Spawn_Red'
	SpawnInTemplates[1]=ParticleSystem'VH_Cicada.Effects.P_VH_Cicada_Spawn_Blue'
	SpawnInTime=5.00
	ExplosionSound=SoundCue'A_Vehicle_Cicada.SoundCues.A_Vehicle_Cicada_Explode'
	JetScalingParam=JetScale

	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)

	PawnHudScene=VHud_Cicada'UI_Scenes_HUD.VehicleHUDs.VH_Cicada'
	PassengerTeamBeaconOffset=(X=-125.0f,Y=0.0f,Z=-105.0f);	
}
