/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTOnslaughtNodeTeleporter_Content extends UTOnslaughtNodeTeleporter;


defaultproperties
{
	Components.Remove(Sprite)
	Components.Remove(Sprite2)
	GoodSprite=None
	BadSprite=None

	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent0
		StaticMesh=StaticMesh'GP_Onslaught.Mesh.S_GP_Ons_Power_Node_Base'
		CollideActors=true
		BlockActors=true
		CastShadow=true
		bCastDynamicShadow=false
		Translation=(X=0.0,Y=0.0,Z=-34.0)
		Scale=0.5
		bUseAsOccluder=false
	End Object
 	Components.Add(StaticMeshComponent0)

	Begin Object Class=ParticleSystemComponent Name=ParticleSystemComponent0
		Translation=(Z=-70.0)
	End Object
	Components.Add(ParticleSystemComponent0)
	AmbientEffect=ParticleSystemComponent0

 	bStatic=false
 	bCollideActors=true
 	bBlockActors=true

 	Begin Object Name=CollisionCylinder
		CollisionRadius=50.0
		CollisionHeight=30.0
	End Object

	RemoteRole=ROLE_SimulatedProxy
	NetUpdateFrequency=1.0
	bAlwaysRelevant=true

	TeamNum=255
	TeamEffectTemplates[0]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Node_Teleporter_Red'
	TeamEffectTemplates[1]=ParticleSystem'GP_Onslaught.Effects.P_Ons_Node_Teleporter_Blue'
}
