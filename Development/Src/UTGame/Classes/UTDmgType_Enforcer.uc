/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_Enforcer extends UTDamageType
	abstract;

static function SpawnHitEffect(Pawn P, float Damage, vector Momentum, name BoneName, vector HitLocation)
{
	Super.SpawnHitEffect(P,Damage,Momentum,BoneName,HitLocation);
	if(UTPawn(P) != none)
	{
		UTPawn(P).SoundGroupClass.Static.PlayBulletImpact(P);
	}
}

defaultproperties
{
	DamageWeaponClass=class'UTWeap_Enforcer'
	DamageWeaponFireMode=2
    VehicleDamageScaling=0.33
	NodeDamageScaling=0.5
	VehicleMomentumScaling=0.0
    bBulletHit=True
	KDamageImpulse=200
	bNeverGibs=true
	bCausesBloodSplatterDecals=TRUE
}
