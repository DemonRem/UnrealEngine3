/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAmmo_SniperRifle extends UTAmmoPickupFactory;

defaultproperties
{
	AmmoAmount=10
	TargetWeapon=class'UTWeap_SniperRifle_Content'
	PickupSound=SoundCue'A_Pickups.Ammo.Cue.A_Pickup_Ammo_Sniper_Cue'

	Begin Object Name=MeshComponent0
		StaticMesh=StaticMesh'Pickups.Ammo_Sniper.Mesh.S_Ammo_Sniper'
	End Object

	Begin Object Name=CollisionCylinder
		CollisionHeight=14.4
	End Object
}
