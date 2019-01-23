/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicleFactory_Manta extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Manta.Mesh.SK_VH_Manta'
		Translation=(X=0.0,Y=0.0,Z=-70.0)
		HiddenGame=true
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionHeight=+40.0
		CollisionRadius=+100.0
	End Object

	VehicleClass=class'UTVehicle_Manta_Content'
}
