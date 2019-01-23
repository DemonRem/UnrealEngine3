/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtFlag_Content extends UTOnslaughtFlag;


defaultproperties
{
	Physics=PHYS_None
	bHome=True
	bStatic=False
	NetPriority=+00003.000000
	bCollideWorld=True
	bCollideActors=true
	bAlwaysRelevant=true
	GraceDist=1024.0
	ProximityLinkDist=1000.0
	ProximityLinkCheckInterval=2.0
	MaxDropTime=30.0

	MessageClass=class'UTOnslaughtOrbMessage'
	GodBeamClass=class'UTOnslaughtGodBeam_Content'

	Begin Object class=PointLightComponent name=FlagLightComponent
		Brightness=5.0
		LightColor=(R=255,G=255,B=64)
		Radius=250.0
		CastShadows=true
		bEnabled=true
	End Object
	FlagLight=FlagLightComponent
	Components.Add(FlagLightComponent)

	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent0
		StaticMesh=StaticMesh'Pickups.PowerCell.Mesh.S_Pickups_PowerCell_Cell01'
		CollideActors=false
		BlockRigidBody=false
		CastShadow=false
		bAcceptsLights=true
		scale=1.35
		bUseAsOccluder=FALSE
	End Object
	Mesh=StaticMeshComponent0
 	Components.Add(StaticMeshComponent0)

	Begin Object Class=AudioComponent Name=AmbientSoundComponent
		SoundCue=SoundCue'A_Gameplay.ONS.A_Gameplay_ONS_OrbAmbient'
		bAutoPlay=true
		bShouldRemainActiveIfDropped=true
	End Object
	Components.Add(AmbientSoundComponent)

//	Begin Object Class=UTParticleSystemComponent Name=TetherComponent
//		bAutoActivate=false
//		HiddenGame=true
//	End Object
//	Components.Add(TetherComponent)
//	TetherEffect=TetherComponent

	PickupSound=SoundCue'A_Gameplay.ONS.A_Gameplay_ONS_OrbPickedUp'
	DroppedSound=SoundCue'A_Gameplay.ONS.A_Gameplay_ONS_OrbDropped'
	ReturnedSound=SoundCue'A_Gameplay.ONS.A_Gameplay_ONS_OrbDischarged'

	ReturnedEffectClasses[0]=class'UTEmit_OnslaughtOrbExplosion_Red'
	ReturnedEffectClasses[1]=class'UTEmit_OnslaughtOrbExplosion_Blue'

	ProximityLinkTemplates[0]=ParticleSystem'Pickups.PowerCell.Effects.P_Pickups_PowerCell_Beam_Red'
	ProximityLinkTemplates[1]=ParticleSystem'Pickups.PowerCell.Effects.P_Pickups_PowerCell_Beam_Blue'

	FlagMaterials(0)=Material'Pickups.PowerCell.Materials.M_Pickups_Orb_Red'
	FlagMaterials(1)=Material'Pickups.PowerCell.Materials.M_Pickups_Orb_Blue'

	FlagEffect(0)=ParticleSystem'GP_Onslaught.Effects.P_Orb_Red'
	FlagEffect(1)=ParticleSystem'GP_Onslaught.Effects.P_Orb_Blue'

	Begin Object Class=ParticleSystemComponent Name=EffectComp
	    bAutoActivate=FALSE
	End Object
	FlagEffectComp=EffectComp
	Components.Add(EffectComp)

//	TetherEffectTemplates[0]=ParticleSystem'Pickups.PowerCell.Effects.P_Pickups_PowerCell_Tether_Red'
//	TetherEffectTemplates[1]=ParticleSystem'Pickups.PowerCell.Effects.P_Pickups_PowerCell_Tether_Blue'
//	TetherEffectEndPointName=TetherBeamEnd

	LightColors(0)=(R=255,G=64,B=0)
	LightColors(1)=(R=64,G=128,B=255)

	GameObjOffset3P=(X=-35,Y=30,Z=25)
	GameObjOffset1P=(X=-35,Y=30,Z=25)
	bHardAttach=true
	RotationRate=(Pitch=12000,Yaw=14000,Roll=10000)

	GameObjBone3P=None
	MaxSpringDistance=15.0
	NormalOrbScale=1.35
	HoverboardOrbScale=0.9

	PrebuildTime=0.025
	BuildTime=5.5

	NeedToPickUpAnnouncements[0]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_GrabTheRedOrb')
	NeedToPickUpAnnouncements[1]=(AnnouncementSound=SoundNodeWave'A_Announcer_FemaleUT.Announcements.Announcer_FemaleUT_GrabTheBlueOrb')

	MiniMapStatusStyleName=CellDispenserObjective

}
