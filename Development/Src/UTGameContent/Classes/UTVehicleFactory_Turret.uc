/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleFactory_Turret extends UTVehicleFactory_TrackTurretBase;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Turret.Mesh.SK_VH_Turret'
		HiddenGame=true
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
		Translation=(Z=-90.0)
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionHeight=80.0
		CollisionRadius=100.0
	End Object

	VehicleClass=class'UTVehicle_Turret'
}
