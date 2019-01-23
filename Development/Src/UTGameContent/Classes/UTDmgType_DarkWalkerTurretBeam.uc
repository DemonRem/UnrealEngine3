/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_DarkWalkerTurretBeam extends UTDmgType_Burning
      abstract;

defaultproperties
{
	DamageWeaponClass=class'UTVWeap_DarkWalkerTurret'
	DamageWeaponFireMode=0
	bKRadialImpulse=true
    KDamageImpulse=3000
	KImpulseRadius=100.0
	VehicleDamageScaling=0.5
	bAlwaysGibs=true

}
