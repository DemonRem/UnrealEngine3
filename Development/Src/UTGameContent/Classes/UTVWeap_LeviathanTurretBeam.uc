/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVWeap_LeviathanTurretBeam extends UTVWeap_LeviathanTurretBase;
defaultproperties
{
 	WeaponFireTypes(0)=EWFT_InstantHit

	BulletWhip=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_BulletWhip_Cue'
	WeaponFireSnd[0]=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_Fire_Cue'
	FireInterval(0)=+0.30
	Spread(0)=0.0
	bInstantHit=true
	InstantHitDamageTypes(0)=class'UTDmgType_LeviathanBeam'

	InstantHitDamage(0)=35
	InstantHitMomentum(0)=60000.0
	ShotCost(0)=0
	FireShake(0)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)

	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_PrimeAltImpact',Sound=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_AltFireImpact_Cue')
	
	FireTriggerTags=(TurretBeamMF_L,TurretBeamMF_R)
	
}
