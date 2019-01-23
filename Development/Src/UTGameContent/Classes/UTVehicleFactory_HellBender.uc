/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleFactory_HellBender extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Hellbender.Mesh.SK_VH_Hellbender'
		HiddenGame=true
		Translation=(X=40.0,Y=0.0,Z=-50.0)
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionHeight=100.0
		CollisionRadius=140.0
		Translation=(X=20.0,Y=0.0,Z=25.0)
	End Object

	VehicleClass=class'UTVehicle_HellBender_Content'
	DrawScale=1.4
}
