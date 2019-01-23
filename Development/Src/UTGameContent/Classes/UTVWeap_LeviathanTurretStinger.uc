/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_LeviathanTurretStinger extends UTVWeap_LeviathanTurretBase;

defaultproperties
{
	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_LeviathanShard'
	FireInterval(0)=+0.1
	WeaponFireSnd[0]=SoundCue'A_Weapon.Enforcers.Cue.A_Weapon_Enforcers_Fire01_Cue'
	FireTriggerTags=(TurretStingerMF)	
	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_PrimeAltImpact',Sound=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_AltFireImpact_Cue')
}
