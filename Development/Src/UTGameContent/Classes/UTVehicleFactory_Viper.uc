/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicleFactory_Viper extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_NecrisManta.Mesh.SK_VH_NecrisManta'
		Translation=(X=0.0,Y=0.0,Z=-64.0)
		HiddenGame=true
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionRadius=100.000000
		CollisionHeight=40.0
	End Object

	VehicleClass=class'UTVehicle_Viper_Content'
}
