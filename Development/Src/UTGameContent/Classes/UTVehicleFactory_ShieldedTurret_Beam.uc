/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicleFactory_ShieldedTurret_Beam extends UTVehicleFactory_TrackTurretBase;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		SkeletalMesh=SkeletalMesh'VH_TurretSmall.Mesh.SK_VH_TurretSmall_Beam'
		HiddenGame=true
		Translation=(X=0.0,Y=0.0,Z=-40.0)
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
	End Object
	Components.Add(SVehicleMesh)
	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionRadius=80.000000
		CollisionHeight=40.0
	End Object

	VehicleClass=class'UTVehicle_ShieldedTurret_Beam'
}
