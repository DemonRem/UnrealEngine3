/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDmgType_AvrilRocket extends UTDamageType
	abstract;

defaultproperties
{
	DamageWeaponClass=class'UTWeap_Avril'
	DamageWeaponFireMode=0

	VehicleDamageScaling=1.6
	VehicleMomentumScaling=5.0
	bKRadialImpulse=true
    KDamageImpulse=3000
	KImpulseRadius=100.0
}
