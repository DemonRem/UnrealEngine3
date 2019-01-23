/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_ShockCombo extends UTDamageType
	abstract;


static function IncrementKills(PlayerReplicationInfo KillerPRI)
{
	local UTPlayerReplicationInfo xPRI;

	xPRI = UTPlayerReplicationInfo(KillerPRI);
	if ( xPRI != None )
	{
		xPRI.ComboCount++;
		if ( (xPRI.ComboCount == 15) && (UTPlayerController(KillerPRI.Owner) != None) )
			UTPlayerController(KillerPRI.Owner).ReceiveLocalizedMessage( class'UTWeaponRewardMessage', 3 );
	}
}

defaultproperties
{
	DamageWeaponClass=class'UTWeap_ShockRifle'
	DamageWeaponFireMode=2

	DamageBodyMatColor=(R=40,B=50)
	DamageOverlayTime=0.3
	DeathOverlayTime=0.9
	bThrowRagdoll=true

	KDamageImpulse=6500
	KImpulseRadius=300.0
	bKRadialImpulse=true
	VehicleDamageScaling=0.9
	VehicleMomentumScaling=2.25

	bNeverGibs=true

	DamageCameraAnim=CameraAnim'Camera_FX.ShockRifle.C_WP_ShockRifle_Combo_Shake'
}
