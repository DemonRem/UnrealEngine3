/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleFactory_Leviathan extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Leviathan.Mesh.SK_VH_Leviathan'
		HiddenGame=true
		Translation=(X=0.0,Y=0.0,Z=-100.0)
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionRadius=450.000000
		CollisionHeight=150.0
	End Object

	VehicleClass=class'UTVehicle_Leviathan_Content'
	DrawScale=1.5
}
