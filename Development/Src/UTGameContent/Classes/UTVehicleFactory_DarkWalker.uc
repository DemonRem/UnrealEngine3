/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleFactory_DarkWalker extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_DarkWalker.Mesh.SK_VH_DarkWalker_Torso'
		HiddenGame=true
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionHeight=+100.0
		CollisionRadius=+100.0
	End Object

	VehicleClass=class'UTVehicle_DarkWalker_Content'


}
