/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAttachment_Stinger extends UTWeaponAttachment;

var ParticleSystem TracerTemplate;
var color TracerColor;

simulated function SpawnTracer(vector EffectLocation,vector HitLocation)
{
	local ParticleSystemComponent E;
	local rotator HitDir;

	if (UTPawn(Owner).FiringMode == 0)
	{
		HitDir = rotator(HitLocation - EffectLocation);
		E = WorldInfo.MyEmitterPool.SpawnEmitter(TracerTemplate, EffectLocation, HitDir);
		E.SetColorParameter('TracerColor', TracerColor);
		E.SetVectorParameter('BeamEndPoint',HitLocation);
	}
}

simulated function FirstPersonFireEffects(Weapon PawnWeapon, vector HitLocation)
{
	super.FirstPersonFireEffects(PawnWeapon, HitLocation);
	SpawnTracer(UTWeapon(PawnWeapon).GetEffectLocation(),HitLocation);
}

simulated function ThirdPersonFireEffects(vector HitLocation)
{
	Super.ThirdPersonFireEffects(HitLocation);
	SpawnTracer(GetEffectLocation(), HitLocation);
}


simulated event StopThirdPersonFireEffects()
{
	super.StopThirdPersonFireEffects();
}


defaultproperties
{

	// Weapon SkeletalMesh

	Begin Object Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'WP_Stinger.Mesh.SK_WP_Stinger_3P'
		Translation=(X=0,Y=0)
	End Object

	WeapAnimType=EWAT_Stinger

	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'WP_Stinger.Particles.P_WP_Stinger_Surface_Impact',Sound=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireImpactCue')
	DefaultAltImpactEffect=(ParticleTemplate=ParticleSystem'WP_Stinger.Particles.P_WP_Stinger_AltFire_Surface_Impact',Sound=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireAltImpactCue')
//	bAlignToSurfaceNormal=false

	BulletWhip=SoundCue'A_Weapon.Enforcers.Cue.A_Weapon_Enforcers_BulletWhizz_Cue'
	WeaponClass=class'UTWeap_Stinger'
	MuzzleFlashLightClass=class'UTStingerMuzzleFlashLight'
	MuzzleFlashSocket=MF
	MuzzleFlashPSCTemplate=ParticleSystem'WP_Stinger.Particles.P_Stinger_3P_MF_Primary'
	MuzzleFlashAltPSCTemplate=ParticleSystem'WP_Stinger.Particles.P_Stinger_3P_MF_Alt_Fire'

	TracerTemplate=ParticleSystem'WP_Stinger.Particles.P_WP_Stinger_AltFire_tracer'
	TracerColor=(R=200,G=200,B=255,A=255)
}
