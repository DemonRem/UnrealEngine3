/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVWeap_NemesisTurret extends UTVehicleWeapon;

defaultproperties
{
 	WeaponFireTypes(0)=EWFT_InstantHit
 	WeaponFireTypes(1)=EWFT_None

	InstantHitDamageTypes(0)=class'UTDmgType_NemesisBeam'
	WeaponFireSnd[0]=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_TurretFireCue'
	FireInterval(0)=+0.36
	Spread(0)=0.0
	InstantHitDamage(0)=50
	InstantHitMomentum(0)=75000.0
	ShotCost(0)=0
	FireShake(0)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)
	bInstantHit=true
	ShotCost(1)=0
	bSniping=true

	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'VH_Nemesis.Effects.PS_Nemesis_Gun_Impact',Sound=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_TurretFireImpactCue')

	bZoomedFireMode(1)=1
	ZoomInSound=SoundCue'A_Vehicle_Nemesis.Cue.A_Vehicle_Nemesis_TurretZoomCue'
	ZoomOutSound=None

	ZoomedTargetFOV=33.0
	ZoomedRate=60.0
	BulletWhip=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_BulletWhip_Cue'
}

