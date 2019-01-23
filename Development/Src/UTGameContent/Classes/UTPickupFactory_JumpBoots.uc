/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPickupFactory_JumpBoots extends UTPickupFactory;

simulated function SetPickupMesh()
{
	Super.SetPickupMesh();
	if ( PickupMesh != none )
	{
		PickupMesh.SetTranslation(PickupMesh.Translation + vect(0,0,10));
	}
}
defaultproperties
{
	InventoryType=class'UTGameContent.UTJumpBoots'

	Begin Object Class=StaticMeshComponent Name=BaseMeshComp
		StaticMesh=StaticMesh'Pickups.Base_Armor.Mesh.S_Pickups_Base_Armor'
		CollideActors=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		Translation=(X=0.0,Y=0.0,Z=-20.0)
		CullDistance=7000
		LightEnvironment=MyLightEnvironment
		bUseAsOccluder=FALSE
	End Object
	BaseMesh=BaseMeshComp
 	Components.Add(BaseMeshComp)

	BaseBrightEmissive=(R=25.0,G=25.0,B=1.0)
	BaseDimEmissive=(R=1.0,G=1.0,B=0.01)

	Begin Object Name=CollisionCylinder
		CollisionRadius=+00030.000000
		CollisionHeight=+00020.000000
		CollideActors=true
	End Object

	
	Begin Object Class=AudioComponent Name=BootsReady
		SoundCue=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_JumpBoots_GroundLoopCue'
	End Object
	PickupReadySound=BootsReady
	Components.Add(BootsReady)
	RespawnSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_JumpBoots_SpawnCue'
}
