/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtPowerCore_Content extends UTOnslaughtPowerCore;


DefaultProperties
{
	Begin Object Name=CollisionCylinder
		CollisionRadius=+0240.000000
		CollisionHeight=+0200.000000
	End Object

	Begin Object Class=SkeletalMeshComponent Name=CoreBaseMesh
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=true
		CollideActors=true
		RBChannel=RBCC_Nothing
		RBCollideWithChannels=(Default=true,Pawn=true,Vehicle=true,GameplayPhysics=true,EffectPhysics=true)
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		CastShadow=false
		BlockRigidBody=true
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		bCastDynamicShadow=false
		bHasPhysicsAssetInstance=true
		bSkelCompFixed=true
		SkeletalMesh=SkeletalMesh'GP_Onslaught.Mesh.SK_GP_Ons_Power_Core'
		AnimSets(0)=AnimSet'GP_Onslaught.Anims.K_GP_ONS_Power_Core'
		Translation=(X=0.0,Y=0.0,Z=-325.0)
		PhysicsAsset=PhysicsAsset'GP_Onslaught.Mesh.SK_GP_Ons_Power_Core_Physics'
		AnimTreeTemplate=AnimTree'GP_Onslaught.Anims.AT_GP_Ons_Power_Core'
		bUseAsOccluder=FALSE
	End Object
	Components.Add(CoreBaseMesh)
	CollisionComponent=CoreBaseMesh
	BaseMesh=CoreBaseMesh
	PanelMesh=CoreBaseMesh

	Begin Object Class=UTParticleSystemComponent Name=ParticleComponent3
		Template=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Center_Blue'
		Translation=(X=0.0,Y=0.0,Z=110.0)
	End Object
 	Components.Add(ParticleComponent3)
	InnerCoreEffect=ParticleComponent3

	DrawScale=+0.6
	bDestinationOnly=true
	IconPosX=0
	IconExtentY=0.25
	MaxSensorRange=4000.0
	bMustTouchToReach=false
	CameraViewDistance=800.0
	bAllowOnlyShootable=true
	bAllowRemoteUse=true
	DamageCapacity=5000
	DefensePriority=10

	BaseMaterialColors[0]=(R=50.0,G=3.0,B=1.5,A=1.0)
	BaseMaterialColors[1]=(R=4.0,G=12.0,B=50.0,A=1.0)

	EnergyLightColors[0]=(R=247,G=64,B=32,A=255)
	EnergyLightColors[1]=(R=64,G=96,B=247,A=255)
	BaseBrightness=20.0

	Begin Object Class=PointLightComponent Name=LightComponent0
		CastShadows=false
		bEnabled=true
		Brightness=20.0
		LightColor=(R=247,G=64,B=32,A=255)
		LightShadowMode=LightShadow_Modulate
		Radius=1024
		LightingChannels=(Dynamic=FALSE,CompositeDynamic=FALSE)
	End Object
	EnergyEffectLight=LightComponent0
	Components.Add(LightComponent0)

	InnerCoreEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Center_Red'
	InnerCoreEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Center_Blue'

	DestructionEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Death_Red01'
	DestructionEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Death_Blue01'
	DestroyedPhysicsAsset=PhysicsAsset'GP_Onslaught.Mesh.SK_GP_Ons_Power_Core_Destroyed_Physics'

	EnergyEndPointParameterNames[0]=Elec_End
	EnergyEndPointParameterNames[1]=Elec_End2
	MaxEnergyEffectDist=150.0
	EnergyEndPointBonePrefix="Column"
	EnergyEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Electricity_Red01'
	EnergyEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Electricity01'

	ShieldEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Shielded_Red'
	ShieldEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Shielded_Blue'

	DestroyedSound=SoundCue'A_Gameplay.ONS.A_GamePlay_ONS_CoreExplodeCue'
	ShieldOffSound=SoundCue'A_Gameplay.ONS.A_GamePlay_ONS_CoreShieldToUnshieldCue'
	ShieldOnSound=SoundCue'A_Gameplay.ONS.A_GamePlay_ONS_CoreUnshieldToShieldCue'
	ShieldedAmbientSound=SoundCue'A_Gameplay.ONS.A_GamePlay_ONS_CoreAmbientShieldedCue'
	UnshieldedAmbientSound=SoundCue'A_Gameplay.ONS.A_GamePlay_ONS_CoreAmbientUnshieldedCue'
	DamageWarningSound=SoundCue'A_Gameplay.ONS.Cue.A_Gameplay_ONS_OnsCoreDamage_Cue'

	DefendAnnouncement=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_DefendYourPowercore')

	//DestroyedSound=SoundCue'A_Gameplay.A_Gameplay_Onslaught_PowerCoreExplode01Cue'
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

	PanelGibClass=class'UTPowerCorePanel_Content'

	PanelExplosionTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Expl_Red01'
	PanelExplosionTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Power_Core_Expl_Blue01'

	bNoCoreSwitch=true
}
