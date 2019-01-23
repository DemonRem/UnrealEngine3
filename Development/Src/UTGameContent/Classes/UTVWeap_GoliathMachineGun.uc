/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVWeap_GoliathMachineGun extends UTVehicleWeapon
	HideDropDown;

defaultproperties
{
 	WeaponFireTypes(0)=EWFT_InstantHit
 	WeaponFireTypes(1)=EWFT_None
	bInstantHit=true
	InstantHitDamageTypes(0)=class'UTDmgType_GoliathMachineGun'

	BulletWhip=SoundCue'A_Weapon.Enforcers.Cue.A_Weapon_Enforcers_BulletWhizz_Cue'

	FireInterval(0)=+0.1
	FireInterval(1)=+0.1
	Spread(0)=0.01
	Spread(1)=0.01

	InstantHitDamage(0)=16
	InstantHitDamage(1)=16

	ShotCost(0)=0
	ShotCost(1)=0

	FireShake(0)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)
	FireShake(1)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)

	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'VH_Goliath.EffectS.PS_Goliath_Gun_Impact',Sound=SoundCue'A_Weapon.Enforcers.Cue.A_Weapon_Enforcers_BulletImpact_Cue',DecalMaterial=Material'WP_Enforcers.M_WP_Enforcers_ImpactDecal',DecalWidth=16.0,DecalHeight=16.0)

	FireTriggerTags=(GoliathMachineGun)

	bZoomedFireMode(0)=0
	bZoomedFireMode(1)=1

	ZoomedTargetFOV=33.0
	ZoomedRate=60.0
}
