/**
 * UTDmgType_Instagib
 *
 *
 *
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_Instagib extends UTDamageType
	abstract;

static function SpawnHitEffect(Pawn P, float Damage, vector Momentum, name BoneName, vector HitLocation)
{
	Super.SpawnHitEffect(P,Damage,Momentum,BoneName,HitLocation);
	if(UTPawn(P) != none)
	{
		UTPawn(P).SoundGroupClass.Static.PlayInstagibSound(P);
	}
}

defaultproperties
{
	DamageWeaponClass=class'UTWeap_InstagibRifle'
	DamageWeaponFireMode=2
}
