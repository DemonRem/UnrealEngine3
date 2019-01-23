/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleFactory_StealthBender extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_StealthBender.Mesh.SK_VH_StealthBender'
		HiddenGame=true
		Translation=(X=0.0,Y=0.0,Z=-70.0)
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionHeight=100.0
		CollisionRadius=140.0
	End Object

	VehicleClass=class'UTVehicle_StealthBender_Content'
	DrawScale=1.4
}
