/**
 * UTDmgType_Rocket
 *
 *
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_Rocket extends UTDmgType_Burning
	abstract;

defaultproperties
{
	DamageWeaponClass=class'UTWeap_RocketLauncher'
	DamageWeaponFireMode=0
	KDamageImpulse=1000
	KDeathUpKick=200
	bKRadialImpulse=true
	VehicleMomentumScaling=4.0
	VehicleDamageScaling=0.9
	NodeDamageScaling=1.15
	bThrowRagdoll=true
	GibPerterbation=0.15
    AlwaysGibDamageThreshold=99
}
