/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_FlakShard extends UTDamageType
	abstract;

static function IncrementKills(PlayerReplicationInfo KillerPRI)
{
	local UTPlayerReplicationInfo xPRI;

	xPRI = UTPlayerReplicationInfo(KillerPRI);
	if ( xPRI != None )
	{
		xPRI.FlakCount++;
		if ( (xPRI.FlakCount == 15) && (UTPlayerController(KillerPRI.Owner) != None) )
			UTPlayerController(KillerPRI.Owner).ReceiveLocalizedMessage( class'UTWeaponRewardMessage', 1 );
	}
}

defaultproperties
{
	DamageWeaponClass=class'UTWeap_FlakCannon'
	DamageWeaponFireMode=0
	KDamageImpulse=600
	VehicleMomentumScaling=0.65
	VehicleDamageScaling=0.9
	bBulletHit=True
	GibThreshold=-15
	MinAccumulateDamageThreshold=55
	AlwaysGibDamageThreshold=80

	//DeathCameraEffectVictim=class'UTEmitCameraEffect_BloodSplatter'
}
