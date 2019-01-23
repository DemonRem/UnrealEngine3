/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAttachment_FlakCannon extends UTWeaponAttachment;

defaultproperties
{

	Begin Object Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'WP_FlakCannon.Mesh.SK_WP_FlakCannon_3P'
	End Object

	WeaponClass=class'UTWeap_FlakCannon'

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=ParticleSystem'WP_FlakCannon.Effects.P_WP_FlakCannon_3P_Muzzle_Flash'
	MuzzleFlashAltPSCTemplate=ParticleSystem'WP_FlakCannon.Effects.P_WP_FlakCannon_3P_Muzzle_Flash'
	MuzzleFlashLightClass=class'UTGame.UTRocketMuzzleFlashLight'
	MuzzleFlashDuration=0.33
}
