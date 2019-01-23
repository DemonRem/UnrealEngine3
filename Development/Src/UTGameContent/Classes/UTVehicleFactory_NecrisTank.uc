/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicleFactory_NecrisTank extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Nemesis.Mesh.SK_VH_Nemesis'
		HiddenGame=true
		Translation=(X=-64.0,Y=0.0,Z=-100.0)
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionRadius=260.000000
		CollisionHeight=100.0
	End Object

	VehicleClass=class'UTVehicle_Nemesis'
}
