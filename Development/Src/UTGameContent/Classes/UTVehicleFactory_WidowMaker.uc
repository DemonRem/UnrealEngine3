/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleFactory_WidowMaker extends UTVehicleFactory
	deprecated;

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
	End Object

	DrawScale=1.3
}
