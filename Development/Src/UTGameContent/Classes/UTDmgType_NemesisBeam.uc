/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_NemesisBeam extends UTDamageType
	abstract;

/** SpawnHitEffect()
 * Possibly spawn a custom hit effect
 */
static function SpawnHitEffect(Pawn P, float Damage, vector Momentum, name BoneName, vector HitLocation)
{
	local UTEmit_VehicleHit BF;

	if ( Vehicle(P) != None )
	{
		BF = P.spawn(class'UTEmit_VehicleHit',P,, HitLocation, rotator(Momentum));
		BF.AttachTo(P, BoneName);
	}
}

defaultproperties
{
	DamageWeaponClass=class'UTVWeap_NemesisTurret'
	DamageWeaponFireMode=0

	DamageBodyMatColor=(R=40,B=50)
	DamageOverlayTime=0.3
	DeathOverlayTime=0.6
	GibPerterbation=0.75
	VehicleMomentumScaling=2.0
	VehicleDamageScaling=1.0
	KDamageImpulse=1500
	bCausesBlood=false
}
