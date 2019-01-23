/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAmmo_LinkGun extends UTAmmoPickupFactory;

defaultproperties
{
	AmmoAmount=50
	TargetWeapon=class'UTWeap_LinkGun_Content'
	PickupSound=SoundCue'A_Pickups.Ammo.Cue.A_Pickup_Ammo_Link_Cue'
	MaxDesireability=0.24

	Begin Object Name=MeshComponent0
		StaticMesh=StaticMesh'Pickups.Ammo_Link.Mesh.S_Ammo_Link'
	End Object

	Begin Object Name=CollisionCylinder
		CollisionHeight=7.2
	End Object
}
