/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPickupFactory_Invisibility extends UTPowerupPickupFactory;

defaultproperties
{
	InventoryType=class'UTGameContent.UTInvisibility'
	bRotatingPickup=true
	YawRotationRate=32768
	BaseBrightEmissive=(R=4.0,G=4.0,B=3.0)
	BaseDimEmissive=(R=0.5,G=0.5,B=0.25)

	Begin Object Class=StaticMeshComponent Name=MeshComponentA
		StaticMesh=StaticMesh'Pickups.UDamage.Mesh.S_UDamage'
		HiddenGame=true
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		bAcceptsLights=false
		CollideActors=false
		AlwaysLoadOnServer=false
		AlwaysLoadOnClient=false
		Scale3D=(X=0.7,Y=0.7,Z=-0.7)
		CullDistance=8000
		LightEnvironment=MyLightEnvironment
		bUseAsOccluder=FALSE
	End Object
 	Components.Add(MeshComponentA)
 	PickupMesh=MeshComponentA
	RespawnSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Invisibility_SpawnCue'

	
	Begin Object Class=AudioComponent Name=InvisReady
		SoundCue=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Invisibility_GroundLoopCue'
	End Object
	PickupReadySound=InvisReady
	Components.Add(InvisReady)
}


