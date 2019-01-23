/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_ShockBall extends UTDamageType
	abstract;



defaultproperties
{
	DamageWeaponClass=class'UTWeap_ShockRifle'
	DamageWeaponFireMode=1

	DamageBodyMatColor=(R=40,B=50)
	DamageOverlayTime=0.3
	DeathOverlayTime=0.6
	VehicleDamageScaling=0.9
	VehicleMomentumScaling=2.75
	KDamageImpulse=1500.0
}
