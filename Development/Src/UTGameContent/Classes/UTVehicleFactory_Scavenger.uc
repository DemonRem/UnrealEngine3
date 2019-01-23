/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleFactory_Scavenger extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Scavenger.Mesh.SK_VH_Scavenger_Torso'
		HiddenGame=true
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	DrawScale=0.5

	Begin Object Name=CollisionCylinder
		CollisionHeight=+40.0
		CollisionRadius=+100.0
	End Object

	VehicleClass=class'UTVehicle_Scavenger_Content'

}
