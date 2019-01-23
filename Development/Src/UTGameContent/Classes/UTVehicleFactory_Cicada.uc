/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicleFactory_Cicada extends UTVehicleFactory;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_Cicada.Mesh.SK_VH_Cicada'
		HiddenGame=true
		Translation=(X=-40.0,Y=0.0,Z=-70.0) // -60 seems about perfect for exact alignment, -70 for some 'lee way'
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionHeight=+120.0
		CollisionRadius=+200.0
		Translation=(X=0.0,Y=0.0,Z=-40.0)
	End Object

	VehicleClass=class'UTVehicle_Cicada_Content'
	DrawScale=1.3
}
