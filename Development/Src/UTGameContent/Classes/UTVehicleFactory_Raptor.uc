/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicleFactory_Raptor extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Raptor.Mesh.SK_VH_Raptor'
		HiddenGame=true
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionHeight=+80.0
		CollisionRadius=+100.0
	End Object

	VehicleClass=class'UTVehicle_Raptor_Content'
	DrawScale=1.3
}
