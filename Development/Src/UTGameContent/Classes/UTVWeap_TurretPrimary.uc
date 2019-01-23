/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_TurretPrimary extends UTVehicleWeapon;

var float ZoomStep;
var float ZoomMinFOV;

simulated function ProcessInstantHit( byte FiringMode, ImpactInfo Impact )
{
	if (Impact.HitActor != none &&
			UTGameObjective(Impact.HitActor) != none &&
			WorldInfo.GRI.OnSameTeam(Instigator, Impact.HitActor) )
	{
		if (Instigator.Controller != none && PlayerController(Instigator.Controller) != none )
		{
			PlayerController(Instigator.Controller).ReceiveLocalizedMessage(class'UTOnslaughtMessage', 5);
		}
		return;
	}
	super.ProcessInstantHit(FiringMode, Impact);
}

defaultproperties
{
 	WeaponFireTypes(0)=EWFT_InstantHit
 	WeaponFireTypes(1)=EWFT_None
	bInstantHit=true
	InstantHitDamageTypes(0)=class'UTDmgType_TurretPrimary'
	WeaponFireSnd[0]=SoundCue'A_Vehicle_Turret.Cue.AxonTurret_FireCue'
	FireInterval(0)=+0.3
	Spread(0)=0.0
	InstantHitDamage(0)=55
	InstantHitMomentum(0)=50000.0
	ShotCost(0)=0
	FireShake(0)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)

	ShotCost(1)=0
	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'VH_Turret.Effects.P_VH_Turret_Impact',Sound=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_AltFireImpact_Cue')
	BulletWhip=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_BulletWhip_Cue'

	FireTriggerTags=(FireUL,FireUR,FireLL,FireLR)

	bZoomedFireMode(1)=1

	ZoomedTargetFOV=12.0
	ZoomedRate=60.0

	bPlaySoundFromSocket=true
}
