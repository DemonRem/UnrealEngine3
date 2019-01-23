/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDmgType_EMP extends UTDamageType
	abstract;

defaultproperties
{
	DamageWeaponClass=class'UTWeap_ImpactHammer'
	DamageWeaponFireMode=2
	VehicleDamageScaling=1.0
    VehicleMomentumScaling=+1.0
	KDamageImpulse=7000
}
