/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Viper_Content extends UTVehicle_Viper;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	GlideBlend = AnimNodeBlend(Mesh.FindAnimNode('GlideNode'));
}

simulated event ViperJumpEffect()
{
	PlaySound(JumpSound, true);
	VehicleEvent('BoostStart');
	if (Role == ROLE_Authority || IsLocallyControlled())
	{
		bSelfDestructReady = false;
		SetTimer(TimeToRiseForSelfDestruct, false, 'ArmSelfDestruct');
		bSelfDestructInProgress = true;
		if (IsLocallyControlled() && IsHumanControlled())
		{
			SelfDestructArmingSnd.Play();
		}
	}
}

simulated function SetVehicleEffectParms(name TriggerName, ParticleSystemComponent PSC)
{
	if (TriggerName == 'MantaOnFire')
	{
		PSC.SetFloatParameter('smokeamount', 0.95);
		PSC.SetFloatParameter('fireamount', 0.95);
	}
	else
	{
		Super.SetVehicleEffectParms(TriggerName, PSC);
	}
}

simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	// viper is fragile so it takes momentum even from weapons that don't usually impart it
	if ( (DamageType == class'UTDmgType_Enforcer') && !IsZero(HitLocation) )
	{
		Momentum = (Location - HitLocation) * float(Damage) * 20.0;
	}
	Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
}

function bool Died(Controller Killer, class<DamageType> DamageType, vector HitLocation)
{
	VehicleEvent('MantaNormal');
	return Super.Died(Killer,DamageType,HitLocation);
}

simulated function DrivingStatusChanged()
{
	if ( !bDriving )
	{
		VehicleEvent('CrushStop');
	}
	Super.DrivingStatusChanged();
}

defaultproperties
{
	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	EngineStartOffsetSecs=2.0
	EngineStopOffsetSecs=1.0

	IconXStart=0.25
	IconYStart=0.5
	IconXWidth=0.25
	IconYWidth=0.25

	SelfDestructDamageType=class'UTDmgType_ScorpionSelfDestruct'
	SelfDestructSoundCue=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_SelfDestructCue'
	EjectSoundCue=SoundCue'A_Weapon.CanisterGun.Cue.A_Weapon_CGGren_Fire_Cue'

	RagdollMesh=SkeletalMesh'VH_NecrisManta.Mesh.SK_VH_NecrisManta_Destroyed'
	RagdollAsset=PhysicsAsset'VH_NecrisManta.Mesh.SK_VH_NecrisManta_Destroyed_Physics'
	RagdollOffset=(X=100,Z=50)
	RagdollRotationOffset=(Pitch=8192,Yaw=18452,Roll=4096)

	VehicleEffects[0]=(EffectStartTag=MantaWeapon01,EffectTemplate=ParticleSystem'VH_NecrisManta.Effects.PS_Viper_Gun_MuzzleFlash',EffectSocket=Gun_Socket_02)
	VehicleEffects[1]=(EffectStartTag=MantaWeapon02,EffectTemplate=ParticleSystem'VH_NecrisManta.Effects.PS_Viper_Gun_MuzzleFlash',EffectSocket=Gun_Socket_01)
	VehicleEffects[2]=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Vehicle_Damage.P_Vehicle_Damage_1_NecrisManta',EffectSocket=DamageSmoke01)
	VehicleEffects[3]=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_NecrisManta.EffectS.PS_Viper_Ground_FX',EffectSocket=GroundEffectSocket)
	VehicleEffects[4]=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_NecrisManta.EffectS.P_VH_Viper_PowerBall',EffectSocket=PowerBallSocket)

	ExhaustIndex=4

	GroundEffectIndices=(3)

	SpawnInTemplates[0]=ParticleSystem'VH_NecrisManta.Effects.P_VH_Viper_Spawn_Red'
	SpawnInTemplates[1]=ParticleSystem'VH_NecrisManta.Effects.P_VH_Viper_Spawn_Blue'
	SpawnInTime=5.00

	Seats(0)={( GunClass=class'UTVWeap_ViperGun',
				GunSocket=(Gun_Socket_01,Gun_Socket_02),
				CameraTag=ViewSocket,
				CameraOffset=-200,
				DriverDamageMult=0.75,
				bSeatVisible=true,
				SeatBone=characterattach,
				SeatOffset=(X=0,Y=0,Z=50) )}

	Begin Object Name=CollisionCylinder
		CollisionHeight=40.0
		CollisionRadius=100.0
		Translation=(X=-40.0,Y=0.0,Z=40.0)
	End Object

	Begin Object Class=CylinderComponent Name=RagdollCollisionCylinder
		CollisionHeight=40.0
		CollisionRadius=100.0
		Translation=(Z=20.0)
	End Object
	RagdollCylinder=RagdollCollisionCylinder

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_NecrisManta.Mesh.SK_VH_NecrisManta'
		AnimTreeTemplate=AnimTree'VH_NecrisManta.Anims.AT_VH_NecrisManta'
		PhysicsAsset=PhysicsAsset'VH_NecrisManta.Mesh.SK_VH_NecrisManta_Physics'
		AnimSets.Add(AnimSet'VH_NecrisManta.Anims.K_VH_NecrisManta')
	End Object

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=MantaEngineSound
		SoundCue=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_EngineLoopCue'
	End Object
	EngineSound=MantaEngineSound
	Components.Add(MantaEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=MantaCollideSound
		SoundCue=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_CollisionCue'
	End Object
	CollideSound=MantaCollideSound
	Components.Add(MantaCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_StartCue'
	ExitVehicleSound=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_StopCue'

	// Scrape sound.
	Begin Object Class=AudioComponent Name=BaseScrapeSound
		SoundCue=SoundCue'A_Gameplay.A_Gameplay_Onslaught_MetalScrape01Cue'
	End Object
	ScrapeSound=BaseScrapeSound
	Components.Add(BaseScrapeSound);

	Begin Object Class=AudioComponent Name=CarveSound
		SoundCue=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_SlideCue'
	End Object
	CurveSound=CarveSound
	Components.Add(CarveSound);

	JumpSound=SoundCue'A_Vehicle_Manta.Sounds.A_Vehicle_Manta_JumpCue'
	DuckSound=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_SquishAttackCue'
	ExplosionSound=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_ExplosionCue'

	DrivingAnim=viper_idle_sitting

	SelfDestructEffectTemplate=ParticleSystem'VH_NecrisManta.Effects.P_VH_Viper_SelfDestruct_FlareUp'

	SpawnInSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeInNecris01Cue'
	SpawnOutSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeOutNecris01Cue'
	ExhaustParamName=ViperExhaust

	HoverBoardAttachSockets=(HoverAttach00)

	PawnHudScene=VHud_Viper'UI_Scenes_HUD.VehicleHUDs.VH_Viper'

	GlideBlendTime=0.5
	SelfDestructSpinName=SuicideSpin

	TimeToRiseForSelfDestruct=1.1
	SelfDestructReadySnd=SoundCue'A_Vehicle_Viper.SoundCues.A_Vehicle_Viper_SelfDestructReady_Cue'

	Begin Object Class=AudioComponent Name=ArmingSnd
		SoundCue=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_SelfDestructCue'
	End Object
	SelfDestructArmingSnd=ArmingSnd
	Components.Add(ArmingSnd)

	DrivingPhysicalMaterial=PhysicalMaterial'VH_NecrisManta.Material.PhysMat_VH_NecrisMantadriving'
	DefaultPhysicalMaterial=PhysicalMaterial'VH_NecrisManta.Material.PhysMat_VH_NecrisManta'
}
