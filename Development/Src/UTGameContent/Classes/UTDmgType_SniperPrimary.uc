/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDmgType_SniperPrimary extends UTDamageType
	abstract;

static function float VehicleDamageScalingFor(Vehicle V)
{
	if ( (UTVehicle(V) != None) && UTVehicle(V).bLightArmor )
		return 1.5 * Default.VehicleDamageScaling;

	return Default.VehicleDamageScaling;
}

defaultproperties
{
	DamageWeaponClass=class'UTWeap_SniperRifle'
	DamageWeaponFireMode=0
	GibPerterbation=0.25
	VehicleDamageScaling=0.5
	bNeverGibs=true
}
