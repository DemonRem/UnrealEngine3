/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleFactory_SPMA extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_SPMA.Mesh.SK_VH_SPMA'
		HiddenGame=true
		Translation=(X=40.0,Y=0.0,Z=-90.0)
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionHeight=100.0
		CollisionRadius=260.0
		Translation=(X=-10.0,Y=0.0,Z=-20.0)
	End Object

	VehicleClass=class'UTVehicle_SPMA_Content'
	DrawScale=1.3
}
