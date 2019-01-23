/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTOnslaughtGodBeam_Content extends UTOnslaughtGodBeam
	notplaceable;

defaultproperties
{
	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EngineResources.S_Actor'
		HiddenGame=True
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(Sprite)

	bIgnoreBaseRotation=true

	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent9
		StaticMesh=StaticMesh'Onslaught_Effects.Meshes.S_Onslaught_FX_GodBeam'
		bAcceptsLights=false
		CollideActors=false
		BlockActors=false
		BlockRigidBody=false
		CastShadow=false
		HiddenGame=false
		Translation=(X=0.0,Y=0.0,Z=-400.0)
		Scale3D=(X=2.0,Y=2.0,Z=2.0)
		bUseAsOccluder=FALSE
	End Object
	NodeBeamEffect=StaticMeshComponent9
	Components.Add(StaticMeshComponent9)
}
