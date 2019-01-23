/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTDmgType_ScavengerBolt extends UTDamageType
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
	DamageWeaponClass=class'UTVWeap_ScavengerGun'
	DamageWeaponFireMode=0
	bCausesBlood=false
	VehicleMomentumScaling=1.0

}
