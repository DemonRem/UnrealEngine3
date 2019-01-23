/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTAttachment_Redeemer extends UTWeaponAttachment;

defaultproperties
{
	// Pickup mesh Transform
	Begin Object Class=SkeletalMeshComponent Name=ThirdPersonMesh
//		SkeletalMesh=SkeletalMesh'WP_Redeemer.Mesh.S_WP_Redeemer_3P'
		SkeletalMesh=SkeletalMesh'WP_Redeemer.Mesh.SK_WP_Redeemer_3P_Mk2'
		AnimTreeTemplate=WP_Redeemer.Anims.K_WP_Redeemer_3_Base
		bOwnerNoSee=true
		CollideActors=false
		Translation=(X=-4,Y=0,Z=12)
		Scale=0.75
		Rotation=(Yaw=16384)
 		bUseAsOccluder=FALSE
   End Object
	Mesh=ThirdPersonMesh

	WeapAnimType=EWAT_ShoulderRocket

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=Envy_Effects.Tests.Effects.P_FX_MuzzleFlash
	MuzzleFlashColor=(R=200,G=64,B=64,A=255)
	MuzzleFlashDuration=0.33;
	MuzzleFlashLightClass=class'UTGame.UTRocketMuzzleFlashLight'

	WeaponClass=class'UTWeap_Redeemer_Content'

}
