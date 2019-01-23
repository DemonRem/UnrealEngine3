/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_HellBender_Content extends UTVehicle_HellBender;

simulated function PostBeginPlay()
{
	super.PostBeginPlay();
	PlayVehicleAnimation('Inactive');
}
simulated function TakeRadiusDamage( Controller InstigatedBy, float BaseDamage, float DamageRadius, class<DamageType> DamageType,
				float Momentum, vector HurtOrigin, bool bFullDamage, Actor DamageCauser )
{
	if ( Role < ROLE_Authority )
		return;

	// don't take damage from own combos
	if (DamageType != class'UTDmgType_VehicleShockChain' || InstigatedBy != Controller)
	{
		Super.TakeRadiusDamage(InstigatedBy, BaseDamage, DamageRadius, DamageType, Momentum, HurtOrigin, bFullDamage, DamageCauser);
	}
}

simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local UTEmitter ShockBeam;
	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	// Handle Beam Effects for the shock beam

	if (!IsZero(HitLocation))
	{
		ShockBeam = Spawn( class'UTEmitter',self,,GetEffectLocation(SeatIndex) );
		if (ShockBeam != none)
		{
			ShockBeam.SetTemplate(BeamTemplate);
			ShockBeam.ParticleSystemComponent.SetVectorParameter('ShotEnd', HitLocation );
		}
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=+65.0
		CollisionRadius=+170.0
		Translation=(Z=-15.0)
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Hellbender.Mesh.SK_VH_Hellbender'
		PhysicsAsset=PhysicsAsset'VH_Hellbender.Mesh.SK_VH_Hellbender_Physics'
		AnimTreeTemplate=AnimTree'VH_Hellbender.Anims.AT_VH_Hellbender'
		AnimSets(0)=AnimSet'VH_Hellbender.Anims.K_VH_Hellbender'
		MorphSets[0]=MorphTargetSet'VH_Hellbender.Mesh.VH_Hellbender_MorphTargets'
	End Object

	DrawScale=1.2

	FlagOffset=(X=9.0,Y=-44,Z=80)

	RagdollMesh=SkeletalMesh'VH_Hellbender.Mesh.SK_VH_HellbenderDestroyed'
	RagdollAsset=PhysicsAsset'VH_Hellbender.Mesh.SK_VH_HellbenderDestroyed_Physics'
	RagdollRotationOffset=(Yaw=-20480)
	RagdollOffset=(Y=-70.0,Z=20.0)
	BigExplosionTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_DeathExplosion'

	Seats(0)={(	GunClass=class'UTVWeap_ShockTurret',
				GunSocket=(GunnerFireSocket),
				TurretControls=(GunnerConstraint),
				CameraTag=DriverViewSocket,
				CameraOffset=-400,
				DriverDamageMult=0.0,
				MuzzleFlashLightClass=class'UTGame.UTShockMuzzleFlashLight')}
	Seats(1)={(	GunClass=class'UTVWeap_HellBenderPrimary',
				GunSocket=(TurretFireSocket),
				GunPivotPoints=(MainTurretYaw),
				TurretVarPrefix="Turret",
				TurretControls=(TurretConstraintPitch,TurretConstraintYaw),
				CameraEyeHeight=20,
				CameraOffset=-256,
				CameraTag=TurretViewSocket,
				DriverDamageMult=0.2,
				ImpactFlashLightClass=class'UTGame.UTShockMuzzleFlashLight',
				MuzzleFlashLightClass=class'UTGame.UTTurretMuzzleFlashLight',
				bSeatVisible=true,
				SeatBone=MainTurretYaw,
				SeatOffset=(X=37,Y=0,Z=-12))}

	// Sounds

	Begin Object Class=AudioComponent Name=ScorpionTireSound
		SoundCue=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireDirt01Cue'
	End Object
	TireAudioComp=ScorpionTireSound
	Components.Add(ScorpionTireSound);

	TireSoundList(0)=(MaterialType=Dirt,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireDirt01Cue')
	TireSoundList(1)=(MaterialType=Foliage,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireFoliage01Cue')
	TireSoundList(2)=(MaterialType=Grass,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireGrass01Cue')
	TireSoundList(3)=(MaterialType=Metal,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireMetal01Cue')
	TireSoundList(4)=(MaterialType=Mud,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireMud01Cue')
	TireSoundList(5)=(MaterialType=Snow,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireSnow01Cue')
	TireSoundList(6)=(MaterialType=Stone,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireStone01Cue')
	TireSoundList(7)=(MaterialType=Wood,Sound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleSurface_TireWood01Cue')

	WheelParticleEffects[0]=(MaterialType=Dirt,ParticleTemplate=ParticleSystem'VH_Scorpion.Effects.PS_Wheel_Rocks')
	WheelParticleEffects[1]=(MaterialType=Water,ParticleTemplate=ParticleSystem'Envy_Level_Effects_2.Vehicle_Water_Effects.P_Hellbender_Water_Splash')

	// Engine sound.
	Begin Object Class=AudioComponent Name=HellBenderEngineSound
		SoundCue=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_EngineIdle'
	End Object
	EngineSound=HellBenderEngineSound
	Components.Add(HellBenderEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=HellBenderCollideSound
		SoundCue=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_Collide'
	End Object
	CollideSound=HellBenderCollideSound
	Components.Add(HellBenderCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_EngineStart'
	ExitVehicleSound=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_EngineStop'

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	SquealThreshold=250.0
	EngineStartOffsetSecs=0.5
	EngineStopOffsetSecs=1.0

	VehicleEffects(0)=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Vehicle_Damage.P_Vehicle_Damage_1_Hellbender',EffectSocket=DamageSmoke01)
	VehicleEffects(1)=(EffectStartTag=ShockTurretFire,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_DriverPrimMuzzleFlash',EffectSocket=GunnerFireSocket)
	VehicleEffects(2)=(EffectStartTag=ShockTurretAltFire,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_DriverAltMuzzleFlash',EffectSocket=GunnerFireSocket)

	VehicleEffects(3)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_GenericExhaust',EffectSocket=ExhaustLeft)
	VehicleEffects(4)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_GenericExhaust',EffectSocket=ExhaustRight)

	VehicleEffects(5)=(EffectStartTag=BackTurretFire,EffectTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_SecondMuzzleFlash',EffectSocket=TurretFireSocket)

	VehicleAnims(0)=(AnimTag=EngineStart,AnimSeqs=(GetIn),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=AnimPlayer)
	VehicleAnims(1)=(AnimTag=Idle,AnimSeqs=(Idle),AnimRate=1.0,bAnimLoopLastSeq=true,AnimPlayerName=AnimPlayer)
	VehicleAnims(2)=(AnimTag=EngineStop,AnimSeqs=(GetOut),AnimRate=1.0,bAnimLoopLastSeq=false,AnimPlayerName=AnimPlayer)
	VehicleAnims(3)=(AnimTag=Inactive,AnimSeqs=(InactiveIdle),AnimRate=1.0,bAnimLoopLastSeq=true,AnimPlayerName=AnimPlayer)

	SpawnInTemplates[0]=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_Spawn_Red'
	SpawnInTemplates[1]=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_Spawn_Blue'
	SpawnInTime=5.00

	DamageMorphTargets(0)=(InfluenceBone=FrontBumper,MorphNodeName=MorphNodeW_Front,Health=200,DamagePropNames=(Damage2))
	DamageMorphTargets(1)=(InfluenceBone=Lt_Rear_Suspension,MorphNodeName=MorphNodeW_RearLt,Health=100,DamagePropNames=(Damage3))
	DamageMorphTargets(2)=(InfluenceBone=Rt_Rear_Suspension,MorphNodeName=MorphNodeW_RearRt,Health=100,DamagePropNames=(Damage3))
	DamageMorphTargets(3)=(InfluenceBone=Lt_Door,MorphNodeName=MorphNodeW_Left,Health=150,DamagePropNames=(Damage1))
	DamageMorphTargets(4)=(InfluenceBone=Rt_Door,MorphNodeName=MorphNodeW_Right,Health=150,DamagePropNames=(Damage1))
	DamageMorphTargets(5)=(InfluenceBone=Antenna1,MorphNodeName=MorphNodeW_Top,Health=200,DamagePropNames=(Damage6))
	DamageMorphTargets(6)=(InfluenceBone=Lt_Front_Suspension,MorphNodeName=MorphNodeW_LtFrontFender,Health=75,DamagePropNames=())
	DamageMorphTargets(7)=(InfluenceBone=Rt_Front_Suspension,MorphNodeName=MorphNodeW_RtFrontFender,Health=75,DamagePropNames=())

	TeamBeaconOffset=(z=60.0)

	BeamTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_Prim_Altfire'
	ExplosionSound=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_Explode'
	ExhaustEffectName=ExhaustVel
	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)

	TeamMaterials[0]=MaterialInstanceConstant'VH_Hellbender.Materials.MI_VH_Hellbender_Red'
	TeamMaterials[1]=MaterialInstanceConstant'VH_Hellbender.Materials.MI_VH_Hellbender_Blue'
	SuspensionShiftSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleCompressC_Cue'

	PawnHudScene=VHud_Hellbender'UI_Scenes_HUD.VehicleHUDs.VH_Hellbender'

	BrakeLightParameterName=Brake_Light
	ReverseLightParameterName=Reverse_Light
	DrivingPhysicalMaterial=PhysicalMaterial'vh_hellbender.materials.physmat_hellbenderdriving'
	DefaultPhysicalMaterial=PhysicalMaterial'vh_hellbender.materials.physmat_hellbender'

	BurnOutMaterial[0]=MaterialInstance'VH_Hellbender.Materials.MI_VH_Hellbender_Red_BO'
	BurnOutMaterial[1]=MaterialInstance'VH_Hellbender.Materials.MI_VH_Hellbender_Blue_BO'
	PassengerTeamBeaconOffset=(X=-125.0f,Y=0.0f,Z=50.0f);
}
