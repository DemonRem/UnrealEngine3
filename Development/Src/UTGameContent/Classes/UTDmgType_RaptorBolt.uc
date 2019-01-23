/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDmgType_RaptorBolt extends UTDamageType
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

static function ScoreKill(PlayerReplicationInfo KillerPRI, PlayerReplicationInfo KilledPRI, Pawn KilledPawn)
{
	if (KilledPRI != None && KillerPRI != KilledPRI && Vehicle(KilledPawn) != None && Vehicle(KilledPawn).bCanFly)
	{
		//Maybe add to game stats?
		if (UTPlayerController(KillerPRI.Owner) != None)
			UTPlayerController(KillerPRI.Owner).ReceiveLocalizedMessage(class'UTVehicleKillMessage', 6);
	}
}

defaultproperties
{
	DamageWeaponClass=class'UTVWeap_RaptorGun'
	DamageWeaponFireMode=2
	KDamageImpulse=1000
	bCausesBlood=false
}
