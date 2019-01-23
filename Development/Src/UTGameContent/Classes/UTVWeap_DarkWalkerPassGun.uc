/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_DarkWalkerPassGun extends UTVehicleWeapon
	HideDropDown;

defaultproperties
{
	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_DarkWalkerBolt'
	WeaponFireTypes(1)=EWFT_None

	WeaponFireSnd[0]=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FireCue'

	FireInterval(0)=+0.15
	ShotCost(0)=0
	ShotCost(1)=0
	FireTriggerTags=(TurretWeapon00,TurretWeapon01,TurretWeapon02,TurretWeapon03)
}
