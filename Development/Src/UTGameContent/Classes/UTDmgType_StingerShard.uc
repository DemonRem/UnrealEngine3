/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDmgType_StingerShard extends UTDamageType;

static function PawnTornOff(UTPawn DeadPawn)
{
	class'UTProj_StingerShard'.static.CreateSpike(DeadPawn, DeadPawn.TakeHitLocation, DeadPawn.Normal(DeadPawn.TearOffMomentum));
}

defaultproperties
{
	DamageWeaponClass=class'UTWeap_Stinger'
	DamageWeaponFireMode=1
	KDamageImpulse=1000
	KDeathUpKick=200
	bKRadialImpulse=true
	VehicleMomentumScaling=2.0
	bThrowRagdoll=true
	bNeverGibs=true
}
