/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_ShockPrimary extends UTDamageType
	abstract;

defaultproperties
{
	DamageWeaponClass=class'UTWeap_ShockRifle'
	DamageWeaponFireMode=0

	DamageBodyMatColor=(R=40,B=50)
	DamageOverlayTime=0.3
	DeathOverlayTime=0.6
	GibPerterbation=0.75
	VehicleMomentumScaling=2.0
	VehicleDamageScaling=0.77
	NodeDamageScaling=0.77
	KDamageImpulse=1500.0

	DamageCameraAnim=CameraAnim'Camera_FX.ShockRifle.C_WP_ShockRifle_Hit_Shake'
}
