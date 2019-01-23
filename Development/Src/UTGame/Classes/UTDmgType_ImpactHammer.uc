/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDmgType_ImpactHammer extends UTDamageType
	abstract;

defaultproperties
{
	bAlwaysGibs=true
	GibPerterbation=+0.5
	DamageWeaponClass=class'UTWeap_ImpactHammer'
	DamageWeaponFireMode=2
	VehicleDamageScaling=0.2
	VehicleMomentumScaling=+1.0
	KDamageImpulse=10000

	DamageCameraAnim=CameraAnim'Camera_FX.ImpactHammer.C_WP_ImpactHammer_Primary_Fire_GetHit_Shake'
	DeathCameraEffectInstigator=class'UTEmitCameraEffect_BloodSplatter'
}
