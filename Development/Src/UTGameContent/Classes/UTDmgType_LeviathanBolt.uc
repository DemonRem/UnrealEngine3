/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_LeviathanBolt extends UTDamageType
	abstract;

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
	DamageWeaponClass=class'UTVWeap_LeviathanPrimary'
	DamageWeaponFireMode=2
	KDamageImpulse=1000
	VehicleMomentumScaling=1.5
}
