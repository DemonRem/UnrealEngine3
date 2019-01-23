/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Scorpion_Content extends UTVehicle_Scorpion;

var ParticleSystem TeamMuzzleFlash[2];
const MUZZLE_FLASH_EFFECT_INDEX=3;
simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if (WorldInfo.NetMode != NM_DedicatedServer && !bDeleteMe && DamageMaterialInstance != none)
	{
		// turn off headlights
		DamageMaterialInstance.SetScalarParameterValue('Green_Glows_Headlights', 0.f );
	}

	BladeBlend = UTAnimBlendByWeapon(Mesh.Animations.FindAnimNode('BladeNode'));
	`Warn("Could not find BladeNode for mesh (" $ Mesh $ ")",BladeBlend == None);

	HatchSequence = AnimNodeSequence(Mesh.Animations.FindAnimNode('HatchNode'));
	`Warn("Could not find HatchNode for mesh (" $ Mesh $ ")",HatchSequence == None);

	BoosterBlend = UTAnimBlendByWeapon(Mesh.Animations.FindAnimNode('BoosterNode'));
	`Warn("Could not find BoosterNode for mesh (" $ Mesh $ ")",BoosterBlend == None);
}

simulated function TeamChanged()
{
	super.TeamChanged();
	VehicleEffects[MUZZLE_FLASH_EFFECT_INDEX].EffectRef.SetTemplate(TeamMuzzleFlash[(Team==1?1:0)]);
}
simulated function DrivingStatusChanged()
{
	Super.DrivingStatusChanged();

	HatchSequence.SetAnim(bDriving ? 'hatch_close' : 'hatch_open');
	HatchSequence.PlayAnim(, HatchSequence.Rate);
}

simulated function SetBladesExtended(bool bExtended)
{
	local int i;

	Super.SetBladesExtended(bExtended);
	if (bBladesExtended)
	{
		BladeBlend.AnimFire('Blades_out', true,,, 'Blades_out_idle');
		PlaySound(BladeExtendSound, true);
	}
	else
	{
		for (i = 0; i < 2; i++)
		{
			if (BladeVictimConstraint[i] != None)
			{
				BladeVictimConstraint[i].Destroy();
				BladeVictimConstraint[i] = None;
			}
		}
		BladeBlend.AnimStopFire();
		PlaySound(BladeRetractSound, true);
	}
}

simulated function BreakOffBlade(bool bLeftBlade)
{
	local SkelControlBase SkelControl;
	local vector BoneLoc;
	local UTBrokenCarPart Part;

	PlaySound(BladeBreakSound, true);
	// FIXME: spawn broken blade emitter
	SkelControl = Mesh.FindSkelControl(bLeftBlade ? 'LeftBlade' : 'RightBlade');
	BoneLoc = Mesh.GetBoneLocation(bLeftBlade? 'Blade_L2' : 'Blade_R2');
	if (SkelControl != None)
	{
		SkelControl.BoneScale = 0.0;
	}
	else
	{
		`warn("Failed to find skeletal controller named" @ (bLeftBlade ? 'LeftBlade' : 'RightBlade') @ "for mesh" @ Mesh);
	}
	if(BoneLoc != Vect(0,0,0))
	{
		part = Spawn(class'UTBrokenCarPart',,,BoneLoc,rotation);
		`log("Spawning at "@BoneLoc);
		Part.Mesh.SetStaticMesh(StaticMesh'VH_Scorpion.Mesh.S_VH_Scorpion_Broken_Blade');
	}
	else
	{
		`log("NO BONE LOC for Scorpion blade break");
	}
}

simulated function CauseMuzzleFlashLight(int SeatIndex)
{
	Super.CauseMuzzleFlashLight(SeatIndex);

	//@FIXME: should have general code for this in UTVehicle
	if (SeatIndex == 0)
	{
		VehicleEvent('MuzzleFlash');
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=40.0
		CollisionRadius=100.0
		Translation=(X=-25.0)
	End Object

	Begin Object Class=CylinderComponent Name=RagdollCollisionCylinder
		CollisionHeight=40.0
		CollisionRadius=100.0
		Translation=(X=25.0,Z=40.0)
	End Object
	RagdollCylinder=RagdollCollisionCylinder

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Scorpion.Mesh.SK_VH_Scorpion_001'
		AnimTreeTemplate=AnimTree'VH_Scorpion.Anims.AT_VH_Scorpion_001'
		PhysicsAsset=PhysicsAsset'VH_Scorpion.Mesh.SK_VH_Scorpion_001_Physics'
		MorphSets[0]=MorphTargetSet'VH_Scorpion.Mesh.VH_Scorpion_MorphTargets'
		AnimSets.Add(AnimSet'VH_Scorpion.Anims.K_VH_Scorpion')
	End Object

	DrawScale=1.2

	BrakeLightParameterName=Brake_Light
	ReverseLightParameterName=Reverse_Light
	HeadLightParameterName=Green_Glows_Headlights

	Seats(0)={(	GunClass=class'UTVWeap_ScorpionTurret',
				GunSocket=(TurretFireSocket),
				GunPivotPoints=(gun_rotate),
				TurretVarPrefix="",
				TurretControls=(TurretRotate),
				CameraTag=GunViewSocket,
				CameraOffset=-215)}

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=ScorpionEngineSound
		SoundCue=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_EngineLoop'
	End Object
	EngineSound=ScorpionEngineSound
	Components.Add(ScorpionEngineSound);

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
	WheelParticleEffects[1]=(MaterialType=Water,ParticleTemplate=ParticleSystem'Envy_Level_Effects_2.Vehicle_Water_Effects.P_Scorpion_Water_Splash')

	// Wheel squeeling sound.
	Begin Object Class=AudioComponent Name=ScorpionSquealSound
		SoundCue=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_Slide'
	End Object
	SquealSound=ScorpionSquealSound
	Components.Add(ScorpionSquealSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=ScorpionCollideSound
		SoundCue=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_Collide'
	End Object
	CollideSound=ScorpionCollideSound
	Components.Add(ScorpionCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_Start'
	ExitVehicleSound=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_Stop'

	// Rocket booster sound.
	Begin Object Class=AudioComponent Name=ScorpionBoosterSound
		SoundCue=SoundCue'A_Vehicle_Raptor.SoundCues.A_Vehicle_Raptor_EngineLoop'
	End Object
	BoosterSound=ScorpionBoosterSound
	Components.Add(ScorpionBoosterSound);

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	SquealThreshold=0.1
	SquealLatThreshold=0.02
	LatAngleVolumeMult = 30.0
	EngineStartOffsetSecs=2.0
	EngineStopOffsetSecs=1.0

	VehicleEffects(0)=(EffectStartTag=BoostStart,EffectEndTag=BoostStop,EffectTemplate=ParticleSystem'VH_Scorpion.Effects.PS_Scorpion_Booster',EffectSocket=Booster01)
	VehicleEffects(1)=(EffectStartTag=BoostStart,EffectEndTag=BoostStop,EffectTemplate=ParticleSystem'VH_Scorpion.Effects.PS_Scorpion_Booster',EffectSocket=Booster02)
	VehicleEffects(2)=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Vehicle_Damage.P_Vehicle_Damage_1_Scorpion',EffectSocket=DamageSmoke01)
	VehicleEffects(MUZZLE_FLASH_EFFECT_INDEX)=(EffectStartTag=MuzzleFlash,EffectTemplate=ParticleSystem'VH_Scorpion.Effects.PS_Scorpion_Gun_MuzzleFlash',EffectSocket=TurretFireSocket)

	
	TeamMuzzleFlash[0]=ParticleSystem'VH_Scorpion.Effects.PS_Scorpion_Gun_MuzzleFlash_Red'
	//TeamMuzzleFlash[1]=ParticleSystem'VH_Scorpion.Effects.PS_Scorpion_Gun_MuzzleFlash_Red'
	TeamMuzzleFlash[1]=ParticleSystem'VH_Scorpion.Effects.PS_Scorpion_Gun_MuzzleFlash'

	RightBladeStartSocket=Blade_R_Start
	RightBladeEndSocket=Blade_R_End
	LeftBladeStartSocket=Blade_L_Start
	LeftBladeEndSocket=Blade_L_End
	BladeDamageType=class'UTDmgType_ScorpionBlade'
	BladeBreakSound=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_BladeBreakOff'
	BladeExtendSound=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_BladeExtend'
	BladeRetractSound=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_BladeRetract'

	SelfDestructDamageType=class'UTDmgType_ScorpionSelfDestruct'

	SelfDestructSoundCue=SoundCue'A_Vehicle_Goliath.SoundCues.A_Vehicle_Goliath_Fire'
	EjectSoundCue=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_Eject_Cue'

	RagdollMesh=SkeletalMesh'VH_Scorpion.Mesh.SK_VH_ScorpionRagdoll'
	RagdollAsset=PhysicsAsset'VH_Scorpion.Mesh.PA_VH_ScorpionRagdoll'
	RagdollOffset=(Z=70.0)

	Begin Object class=PointLightComponent name=LeftRocketLight
		Brightness=3.0
		LightColor=(R=96,G=64,B=255)
		Radius=100.0
		CastShadows=false
		bEnabled=false
    	Translation=(X=20,Z=0)
	End Object
	LeftBoosterLight=LeftRocketLight

	Begin Object class=PointLightComponent name=RightRocketLight
		Brightness=3.0
		LightColor=(R=96,G=64,B=255)
		Radius=100.0
		CastShadows=false
		bEnabled=false
    	Translation=(X=20,Z=0)
	End Object
	RightBoosterLight=RightRocketLight

	DamageMorphTargets(0)=(InfluenceBone=LtFront_Fender,AssociatedSkelControl=Damage_LtFrontFender,MorphNodeName=MorphNodeW_LtFrontFender,LinkedMorphNodeName=MorphNodeW_Hood,Health=30,DamagePropNames=(Damage1,Damage7))
	DamageMorphTargets(1)=(InfluenceBone=RtFront_Fender,AssociatedSkelControl=Damage_RtFrontFender,MorphNodeName=MorphNodeW_RtFrontFender,LinkedMorphNodeName=MorphNodeW_Hood,Health=30,DamagePropNames=(Damage1,Damage7))
	DamageMorphTargets(2)=(InfluenceBone=LtRear_Fender,AssociatedSkelControl=Damage_LtRearFender,MorphNodeName=MorphNodeW_LtRearFender,LinkedMorphNodeName=MorphNodeW_Hatch,Health=40,DamagePropNames=(Damage,Damage7,Damage6))
	DamageMorphTargets(3)=(InfluenceBone=RtRear_Fender,AssociatedSkelControl=Damage_RtRearFender,MorphNodeName=MorphNodeW_RtRearFender,LinkedMorphNodeName=MorphNodeW_Hatch,Health=40,DamagePropNames=(Damage,Damage7,Damage6))
	DamageMorphTargets(4)=(InfluenceBone=Hood,MorphNodeName=MorphNodeW_Hood,LinkedMorphNodeName=MorphNodeW_Hatch,Health=100,DamagePropNames=(Damage3))
	DamageMorphTargets(5)=(InfluenceBone=Hatch_Slide,MorphNodeName=MorphNodeW_Hatch,LinkedMorphNodeName=MorphNodeW_Body,Health=125,DamagePropNames=(Damage5))
	DamageMorphTargets(6)=(InfluenceBone=Main_Root,MorphNodeName=MorphNodeW_Body,Health=175,DamagePropNames=(Damage7,Damage6))

	ScorpionHood=StaticMesh'VH_Scorpion.Mesh.S_VH_Scorpion_Hood_Damaged'

	SpawnInTemplates[0]=ParticleSystem'VH_Scorpion.Effects.P_VH_Scorpion_Spawn_Red'
	SpawnInTemplates[1]=ParticleSystem'VH_Scorpion.Effects.P_VH_Scorpion_Spawn_Blue'
	SpawnInTime=5.00
	FlagOffset=(X=-60.0,Y=25,Z=40)

	SelfDestructWarningSound=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_DestructionWarning_Cue'
	SelfDestructReadyCue=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_EjectReadyBeep_Cue'
	SelfDestructEnabledSound=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_EngineThrustStart_Cue'
	SelfDestructEnabledLoop=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_EngineThrustLoop_Cue'

	ExplosionSound=SoundCue'A_Vehicle_Scorpion.SoundCues.A_Vehicle_Scorpion_Explode'
	HoverBoardAttachSockets=(HoverAttach00)

	TeamMaterials[0]=MaterialInstanceConstant'VH_Scorpion.Materials.MI_VH_Scorpion_Red'
	TeamMaterials[1]=MaterialInstanceConstant'VH_Scorpion.Materials.MI_VH_Scorpion_Blue'

	SuspensionShiftSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleCompressD_Cue'

	PawnHudScene=VHud_Scorpion'UI_Scenes_HUD.VehicleHUDs.VH_Scorpion'

	DrivingPhysicalMaterial=PhysicalMaterial'vh_scorpion.materials.physmat_scorpiondriving'
	DefaultPhysicalMaterial=PhysicalMaterial'vh_scorpion.materials.physmat_scorpion'

	BurnOutMaterial[0]=MaterialInstance'VH_Scorpion.Materials.MI_VH_Scorpion_Red_BO'
	BurnOutMaterial[1]=MaterialInstance'VH_Scorpion.Materials.MI_VH_Scorpion_Blue_BO'

	SelfDestructExplosionTemplate=ParticleSystem'VH_Scorpion.Effects.P_VH_Scorpion_SelfDestruct'
	HatchGibClass=class'UTGib_ScorpionHatch'
}
