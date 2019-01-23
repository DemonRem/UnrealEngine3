/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicleFactory_Goliath extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Goliath.Mesh.SK_VH_Goliath'
		HiddenGame=true
		Translation=(X=0.0,Y=0.0,Z=-70.0)
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionRadius=260.000000
		CollisionHeight=90.0
	End Object

	VehicleClass=class'UTVehicle_Goliath_Content'
	DrawScale=1.3
}
