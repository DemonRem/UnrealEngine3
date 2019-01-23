/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTAmmo_Stinger extends UTAmmoPickupFactory;

defaultproperties
{
	AmmoAmount=50
	TargetWeapon=class'UTWeap_Stinger_Content'
	PickupSound=SoundCue'A_Pickups.Ammo.Cue.A_Pickup_Ammo_Stinger_Cue'
	MaxDesireability=0.25

	Begin Object Name=MeshComponent0
		StaticMesh=StaticMesh'Pickups.Ammo_Stinger.Mesh.S_Pickups_Ammo_Stinger'
	End Object
}
