/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtPowernode_Content extends UTOnslaughtPowernode;


defaultproperties
{
	InvulnerableRadius=1000.0
	CaptureReturnRadius=500.0
	InvEffectZOffset=16.0
	OrbHealingPerSecond=100
	OrbCaptureInvulnerabilityDuration=10.0

	YawRotationRate=20000
	LinkHealMult=1.0
	DamageCapacity=2000
	Score=5
	bDestinationOnly=false
	bPathColliding=false
	PanelHealthMax=60
	PanelBonePrefix="NodePanel"
	PanelSphereOffset=(X=0.0,Y=0.0,Z=370.0)
	DestructionMessageIndex=16

	//ActiveSound=soundcue'ONSVehicleSounds-S.PwrNodeActive02'

	DestroyedEvent(0)="red_powernode_destroyed"
	DestroyedEvent(1)="blue_powernode_destroyed"
	DestroyedEvent(2)="red_constructing_powernode_destroyed"
	DestroyedEvent(3)="blue_constructing_powernode_destroyed"
	ConstructedEvent(0)="red_powernode_constructed"
	ConstructedEvent(1)="blue_powernode_constructed"


 	Begin Object Name=CollisionCylinder
		CollisionRadius=160.0
		CollisionHeight=30.0
	End Object

	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent0
		StaticMesh=StaticMesh'GP_Onslaught.Mesh.S_GP_Ons_Power_Node_Base'
		CollideActors=true
		BlockActors=true
		CastShadow=true
		bCastDynamicShadow=false
		Translation=(X=0.0,Y=0.0,Z=-34.0)
		bUseAsOccluder=FALSE
	End Object
	NodeBase=StaticMeshComponent0
 	Components.Add(StaticMeshComponent0)

 	Begin Object Class=StaticMeshComponent Name=StaticMeshSpinner
 		StaticMesh=StaticMesh'GP_Onslaught.Mesh.S_GP_Ons_Power_Node_spinners'
 		CollideActors=false
 		BlockActors=false
 		CastShadow=false
		bCastDynamicShadow=false
 		Translation=(Z=-34.0)
		bUseAsOccluder=FALSE
 	End Object
 	NodeBaseSpinner=StaticMeshSpinner
 	Components.Add(StaticMeshSpinner)

	Begin Object Class=SkeletalMeshComponent Name=SkeletalMeshComponent1
		SkeletalMesh=SkeletalMesh'GP_Onslaught.Mesh.SK_GP_Ons_Power_Node_Panels'
		AnimTreeTemplate=AnimTree'GP_Onslaught.Anims.AT_GP_Ons_Power_Node_Panels'
		CollideActors=false
		BlockActors=false
		CastShadow=false
		bCastDynamicShadow=false
		//BlockRigidBody=true
		//bHasPhysicsAssetInstance=true
		//bSkelCompFixed=true
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		Translation=(X=0.0,Y=0.0,Z=240.0)
		Scale3D=(X=1.5,Y=1.5,Z=1.25)
		bUseAsOccluder=FALSE
	End Object
	EnergySphere=SkeletalMeshComponent1
	PanelMesh=SkeletalMeshComponent1
 	Components.Add(SkeletalMeshComponent1)

	Begin Object Class=CylinderComponent Name=CollisionCylinder2
		CollideActors=true
		BlockActors=true
		BlockZeroExtent=true
		BlockNonZeroExtent=true
		BlockRigidBody=false
		Translation=(X=0.0,Y=0.0,Z=400.0)
		CollisionRadius=90
		CollisionHeight=70
	End Object
	EnergySphereCollision=CollisionCylinder2
 	Components.Add(CollisionCylinder2)

	Begin Object Class=UTParticleSystemComponent Name=AmbientEffectComponent
		Translation=(X=0.0,Y=0.0,Z=128.0)
	End Object
	AmbientEffect=AmbientEffectComponent
	Components.Add(AmbientEffectComponent)

	//@fixme FIXME: should be UTParticleSystemComponent, but changing that breaks copies already saved in maps
	// because the old component is getting incorrectly saved even though it hasn't been edited
	Begin Object Class=ParticleSystemComponent Name=ParticleSystemComponent1
		Translation=(X=0.0,Y=0.0,Z=370.0)
		bAcceptsLights=false
		bOverrideLODMethod=true
		LODMethod=PARTICLESYSTEMLODMETHOD_DirectSet
		bAutoActivate=false
	End Object
	ShieldedEffect=ParticleSystemComponent1
	Components.Add(ParticleSystemComponent1)

	HealEffectClasses[0]=class'UTOnslaughtNodeHealEffectRed'
	HealEffectClasses[1]=class'UTOnslaughtNodeHealEffectBlue'

	ActiveEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Center_Red'
	ActiveEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Center_Blue'
	ShieldedActiveEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Center_Red_Shielded'
	ShieldedActiveEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Center_Blue_Shielded'
	NeutralEffectTemplate=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Neutral'
	ConstructingEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Constructing_Red'
	ConstructingEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Constructing_Blue'
	ShieldedEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Shielded_Red'
	ShieldedEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Shielded_Blue'
	VulnerableEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Vulnerable_Red'
	VulnerableEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Vulnerable_Blue'
	DamagedEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Damaged_Red'
	DamagedEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Damaged_Blue'
	DestroyedEffectTemplate=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Destroyed'
	PanelHealEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Panel_Red'
	PanelHealEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Node_Panel_Blue'

	NeutralGlowColor=(R=7.0,G=7.0,B=4.5,A=1.0)
	TeamGlowColors[0]=(R=30.0,G=2.25,B=0.95,A=1.0)
	TeamGlowColors[1]=(R=0.9,G=3.75,B=40.0,A=1.0)

	AttackAnnouncement=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_AttackPowernode')
	DefendAnnouncement=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_DefendPowernode')

	// A Panels currently take 3.0 seconds to go from spawn to mark.  I've set this travel
	// time to 4 seconds to ensure it completes before the node is done
	PanelTravelTime=4.0


	DestroyedSound=SoundCue'A_Gameplay.A_Gameplay_Onslaught_PowerCoreExplode01Cue'
	ConstructedSound=SoundCue'A_Gameplay.A_Gameplay_Onslaught_PowerNodeBuilt01Cue'
	StartConstructionSound=SoundCue'A_Gameplay.A_Gameplay_Onslaught_PowerNodeBuild01Cue'
	NeutralSound=SoundCue'A_Gameplay.A_Gameplay_Onslaught_PowerNodeNotActive01Cue'
	HealingSound=SoundCue'A_Gameplay.A_Gameplay_Onslaught_PowerNodeStartBuild01Cue'
	HealedSound=SoundCue'A_Gameplay.A_Gameplay_Onslaught_PowerNodeBuilt01Cue'
	ShieldHitSound=SoundCue'A_Gameplay.ONS.A_GamePlay_ONS_CoreImpactShieldedCue'

	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent9
	StaticMesh=StaticMesh'Onslaught_Effects.Meshes.S_Onslaught_FX_GodBeam'
	CollideActors=false
	BlockActors=false
	BlockRigidBody=false
	CastShadow=false
	HiddenGame=false
	bAcceptsLights=false
	Translation=(X=0.0,Y=0.0,Z=0.0)
	Scale3D=(X=2.0,Y=2.0,Z=2.0)
	bUseAsOccluder=FALSE
	End Object
	NodeBeamEffect=StaticMeshComponent9
	Components.Add(StaticMeshComponent9)

	PanelGibClass=class'UTPowerNodePanel'

	PanelExplosionTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Expl_Red01'
	PanelExplosionTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Expl_Blue01'

	Begin Object Class=AudioComponent name=OrbNearbySoundComponent
		bShouldRemainActiveIfDropped=true
		bStopWhenOwnerDestroyed=true
		SoundCue=SoundCue'A_Gameplay.ONS.OrbNearConduitCue'
	End Object
	OrbNearbySound=OrbNearbySoundComponent
	Components.Add(OrbNearbySoundComponent)
}

