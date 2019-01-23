/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTDmgType_StingerBullet extends UTDamageType
	abstract;


defaultproperties
{
	DamageWeaponClass=class'UTWeap_Stinger'
	DamageWeaponFireMode=0
	KDamageImpulse=200
	VehicleDamageScaling=+0.6
	VehicleMomentumScaling=0.75
	bBulletHit=True
	bNeverGibs=true
}
