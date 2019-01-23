/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicleFactory_Paladin extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Paladin.Mesh.SK_VH_Paladin'
		HiddenGame=true
		Translation=(Z=-70.0)
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionHeight=125.0
		CollisionRadius=260.0
	End Object

	VehicleClass=class'UTVehicle_Paladin'
	DrawScale=1.5
}
