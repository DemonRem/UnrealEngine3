/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAttachment_RocketLauncher extends UTWeaponAttachment;

defaultproperties
{
	Begin Object Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'WP_RocketLauncher.Mesh.SK_WP_RocketLauncher_3P'
    End Object

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=WP_RocketLauncher.Effects.P_WP_RockerLauncher_3P_Muzzle_Flash
	MuzzleFlashDuration=0.33;
	MuzzleFlashLightClass=class'UTGame.UTRocketMuzzleFlashLight'
	WeaponClass=class'UTWeap_RocketLauncher'
}
