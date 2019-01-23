/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Scavenger_Content extends UTVehicle_Scavenger;

event ScavengerJumpEffect()
{
	PlaySound(JumpSound, true);
	VehicleEvent('BoostStart');
}

event ScavengerDuckEffect()
{
	if (bHoldingDuck)
	{
		if (DuckSound != None)
		{
			PlaySound(DuckSound);
		}
		VehicleEvent('CrushStart');
	}
	else
	{
		VehicleEvent('CrushStop');
	}

	// FIXMESTEVE - make this client side like jump effect
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=+40.0
		CollisionRadius=+100.0
		Translation=(X=-40.0,Y=0.0,Z=40.0)
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Scavenger.Mesh.SK_VH_Scavenger_Torso'
		//AnimSets(0)=AnimSet'VH_Scavenger.Anim.K_VH_Scavenger'
		//AnimTreeTemplate=AnimTree'VH_Scavenger.Anim.AT_VH_Scavenger'
		PhysicsAsset=PhysicsAsset'VH_Scavenger.Mesh.SK_VH_Scavenger_Torso_Physics_Final'
	End Object

	DrawScale=0.5
	DrivingAnim=viper_idle_sitting

	Seats(0)={( GunClass=class'UTVWeap_ScavengerGun',
				GunSocket=(Gun_Socket_01,Gun_Socket_02),
				CameraTag=None,
				CameraOffset=-400,
				TurretVarPrefix="",
				DriverDamageMult=0.0,
				bSeatVisible=true,
				SeatOffset=(X=-40,Y=0,Z=30),
				SeatRotation=(Yaw=0))}

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=MantaEngineSound
		SoundCue=SoundCue'A_Vehicle_Manta.SoundCues.A_Vehicle_Manta_EngineLoop'
	End Object
	EngineSound=MantaEngineSound
	Components.Add(MantaEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=MantaCollideSound
		SoundCue=SoundCue'A_Vehicle_Manta.SoundCues.A_Vehicle_Manta_Collide'
	End Object
	CollideSound=MantaCollideSound
	Components.Add(MantaCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Manta.SoundCues.A_Vehicle_Manta_Start'
	ExitVehicleSound=SoundCue'A_Vehicle_Manta.SoundCues.A_Vehicle_Manta_Stop'

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

	BodyAttachSocketName=LegsAttach

	IconXStart=0
	IconYStart=0.5
	IconXWidth=0.25
	IconYWidth=0.25

	SpawnInSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeInNecris01Cue'
	SpawnOutSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeOutNecris01Cue'

	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)

	PawnHudScene=VHud_Scavenger'UI_Scenes_HUD.VehicleHUDs.VH_Scavenger'

	BodyType=class'UTWalkerBody_Scavenger_Content'
	bCameraNeverHidesVehicle=true
	
	VehicleEffects(0)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_Thruster_Left',EffectSocket=LeftThruster)
	VehicleEffects(1)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_Thruster_Right',EffectSocket=RightThruster)
	VehicleEffects(2)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_Powerball',EffectSocket=Powerball)

	JumpSound=SoundCue'A_Vehicle_Manta.Sounds.A_Vehicle_Manta_JumpCue'
	DuckSound=SoundCue'A_Vehicle_Viper.Cue.A_Vehicle_Viper_SquishAttackCue'
	BounceSound=SoundCue'A_Interface.Menu.UT3MenuAcceptCue'

	DrivingPhysicalMaterial=PhysicalMaterial'VH_Scavenger.mesh.physmat_ScavengerDriving'
	DefaultPhysicalMaterial=PhysicalMaterial'VH_Scavenger.mesh.physmat_Scavenger'
	RollingPhysicalMaterial=PhysicalMaterial'VH_Scavenger.mesh.physmat_ScavengerRolling'
}
