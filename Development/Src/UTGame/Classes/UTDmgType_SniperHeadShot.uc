/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDmgType_SniperHeadShot extends UTDamageType
	abstract;

static function SpawnHitEffect(Pawn P, float Damage, vector Momentum, name BoneName, vector HitLocation)
{
	local UTPawn UTP;
	local name HeadBone;
	local UTEmit_HitEffect HitEffect;

	UTP = UTPawn(P);
	if (UTP != None && UTP.Mesh != None)
	{
		HeadBone = UTP.HeadBone;
	}
	HitEffect = P.Spawn(class'UTEmit_HeadShotBloodSpray',,, HitLocation, rotator(-Momentum));
	if (HitEffect != None)
	{
		HitEffect.AttachTo(P, HeadBone);
	}
}

static function IncrementKills(PlayerReplicationInfo KillerPRI)
{
	local UTPlayerReplicationInfo xPRI;

	if ( PlayerController(KillerPRI.Owner) != None )
	{
		PlayerController(KillerPRI.Owner).ReceiveLocalizedMessage( class'UTWeaponRewardMessage', 0 );
	}
	xPRI = UTPlayerReplicationInfo(KillerPRI);
	if ( xPRI != None )
	{
		xPRI.HeadCount++;
		if ( (xPRI.HeadCount == 15) && (UTPlayerController(KillerPRI.Owner) != None) )
			UTPlayerController(KillerPRI.Owner).ReceiveLocalizedMessage( class'UTWeaponRewardMessage', 2 );
	}
}

defaultproperties
{
	DamageWeaponClass=class'UTWeap_SniperRifle'
	DamageWeaponFireMode=0
	bSeversHead=true
	bNeverGibs=true
	bIgnoreDriverDamageMult=true
	VehicleDamageScaling=0.5
	NodeDamageScaling=0.5
	DeathAnim=Death_Headshot
}
