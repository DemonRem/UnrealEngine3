/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAttachment_Avril extends UTWeaponAttachment;

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=StaticMeshComponent1
		SkeletalMesh=SkeletalMesh'WP_AVRiL.Mesh.SK_WP_Avril_3P'
		bOwnerNoSee=true
		CollideActors=false
		Translation=(Y=15)
		Rotation=(Roll=32768,Yaw=16384)
		scale=0.045
		CullDistance=5000
		bUseAsOccluder=FALSE
	End Object
	Mesh=StaticMeshComponent1

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=ParticleSystem'WP_AVRiL.Particles.P_WP_Avril_MuzzleFlash'
	MuzzleFlashDuration=0.33
	MuzzleFlashLightClass=class'UTGame.UTRocketMuzzleFlashLight'
	WeaponClass=class'UTWeap_Avril'
}
