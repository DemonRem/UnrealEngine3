/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_LinkPlasma extends UTDamageType
	abstract;

defaultproperties
{
	DamageWeaponClass=class'UTWeap_LinkGun'
	DamageWeaponFireMode=0

	DamageBodyMatColor=(R=100,G=100,B=100)
	DamageOverlayTime=0.5
	DeathOverlayTime=1.0
	VehicleDamageScaling=0.6
	VehicleMomentumScaling=0.25
}
