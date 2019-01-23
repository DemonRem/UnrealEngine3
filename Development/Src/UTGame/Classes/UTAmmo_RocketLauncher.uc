/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAmmo_RocketLauncher extends UTAmmoPickupFactory;

defaultproperties
{
	AmmoAmount=9
	TargetWeapon=class'UTWeap_RocketLauncher'
	PickupSound=SoundCue'A_Pickups.Ammo.Cue.A_Pickup_Ammo_Rocket_Cue'
	MaxDesireability=0.3

	Begin Object Name=AmmoMeshComp
		StaticMesh=StaticMesh'Pickups.Ammo_Rockets.Mesh.S_Ammo_RocketLauncher'
		Rotation=(Roll=16384)
		Translation=(X=0.0,Y=0.0,Z=1.0)
	End Object
}
