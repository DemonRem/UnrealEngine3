/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_HellBenderPrimary extends UTVehicleWeapon
	HideDropDown;

var float	LastFireTime;
var float   ReChargeTime;


/**
  * returns true if should pass trace through this hitactor
  * turret ignores shock balls fired by hellbender driver
  */
simulated function bool PassThroughDamage(Actor HitActor)
{
	return HitActor.IsA('UTProj_VehicleShockBall') || HitActor.IsA('Trigger') || HitActor.IsA('TriggerVolume');
}

simulated function InstantFire()
{
	Super.InstantFire();
	LastFireTime = WorldInfo.TimeSeconds;
}

simulated function ProcessInstantHit( byte FiringMode, ImpactInfo Impact )
{
	local float DamageMod;

	DamageMod = FClamp( (WorldInfo.TimeSeconds - LastFireTime), 0, RechargeTime);
	DamageMod = FClamp( (DamageMod / 3), 0.05, 1.0);

	// cause damage to locally authoritative actors
	if (Impact.HitActor != None && Impact.HitActor.Role == ROLE_Authority)
	{
		Impact.HitActor.TakeDamage(	InstantHitDamage[CurrentFireMode] * DamageMod,
									Instigator.Controller,
									Impact.HitLocation,
									InstantHitMomentum[FiringMode] * Impact.RayDir,
									InstantHitDamageTypes[FiringMode],
									Impact.HitInfo,
									self );
	}
}

simulated function DrawAmmoCount( Hud H )
{
	local UTHud UTH;
	local float x,y;
	local float Power;

	Power = FClamp( (WorldInfo.TimeSeconds - LastFireTime), 0, RechargeTime);
	Power = FClamp( (Power / 3), 0.0, 1.0);

	UTH = UTHud(H);

	if ( UTH != None )
	{
		x = UTH.Canvas.ClipX - (5 * (UTH.Canvas.ClipX / 1024));
		y = UTH.Canvas.ClipY - (5 * (UTH.Canvas.ClipX / 1024));
		UTH.DrawChargeWidget(X, Y, IconX, IconY, IconWidth, IconHeight, Power, true);
	}
}

defaultproperties
{
 	WeaponFireTypes(0)=EWFT_InstantHit
 	WeaponFireTypes(1)=EWFT_None
	bInstantHit=true
	InstantHitDamageTypes(0)=class'UTDmgType_HellBenderPrimary'
	WeaponFireSnd[0]=SoundCue'A_Vehicle_Hellbender.SoundCues.A_Vehicle_Hellbender_TurretFire'
	FireInterval(0)=+0.4
	Spread(0)=0.00
	InstantHitDamage(0)=200
	ShotCost(0)=0
	FireShake(0)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)

	ShotCost(1)=0

	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'VH_Hellbender.Effects.P_VH_Hellbender_SecondImpact',Sound=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_AltFireImpact_Cue')

	FireTriggerTags=(BackTurretFire)

	bZoomedFireMode(1)=1

	ZoomedTargetFOV=33.0
	ZoomedRate=60.0

	RechargeTime = 3.0;
}

