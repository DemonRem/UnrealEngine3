/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDmgType_TankShell extends UTDmgType_Burning
	abstract;

static function ScoreKill(PlayerReplicationInfo KillerPRI, PlayerReplicationInfo KilledPRI, Pawn KilledPawn)
{
	if ( KilledPRI != None && KillerPRI != KilledPRI && UTVehicle(KilledPawn) != None && UTVehicle(KilledPawn).EagleEyeTarget() )
	{
		//Maybe add to game stats?
		if (UTPlayerController(KillerPRI.Owner) != None)
			UTPlayerController(KillerPRI.Owner).ReceiveLocalizedMessage(class'UTVehicleKillMessage', 5);
	}
}

static function float VehicleDamageScalingFor(Vehicle V)
{
	if ( (UTVehicle(V) != None) && UTVehicle(V).bLightArmor )
		return 1.0;

	return Default.VehicleDamageScaling;
}


defaultproperties
{
	DamageWeaponClass=class'UTVWeap_GoliathTurret'
	DamageWeaponFireMode=0
	KDamageImpulse=8000
	KImpulseRadius=500.0
	bKRadialImpulse=true
    AlwaysGibDamageThreshold=99

	VehicleMomentumScaling=1.5
	bThrowRagdoll=true
	GibPerterbation=0.15
}
