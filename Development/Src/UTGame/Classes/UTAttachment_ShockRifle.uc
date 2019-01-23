/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAttachment_ShockRifle extends UTWeaponAttachment;

var ParticleSystem BeamTemplate;
var class<UTExplosionLight> ImpactLightClass;

var int CurrentPath;

simulated function SpawnBeam(vector Start, vector End, bool bFirstPerson)
{
	local ParticleSystemComponent E;

	E = WorldInfo.MyEmitterPool.SpawnEmitter(BeamTemplate, Start);
	E.SetVectorParameter('ShockBeamEnd', End);
	if (bFirstPerson)
	{
		E.SetDepthPriorityGroup(SDPG_Foreground);
	}
}

simulated function FirstPersonFireEffects(Weapon PawnWeapon, vector HitLocation)
{
	local vector EffectLocation;

	Super.FirstPersonFireEffects(PawnWeapon, HitLocation);

	if (Instigator.FiringMode == 0 || Instigator.FiringMode == 3)
	{
		EffectLocation = UTWeapon(PawnWeapon).GetEffectLocation();
		SpawnBeam(EffectLocation, HitLocation, true);

		if (!WorldInfo.bDropDetail && Instigator.Controller != None)
		{
			UTEmitterPool(WorldInfo.MyEmitterPool).SpawnExplosionLight(ImpactLightClass, HitLocation);
		}
	}
}

simulated function ThirdPersonFireEffects(vector HitLocation)
{
	Super.ThirdPersonFireEffects(HitLocation);

	if ((Instigator.FiringMode == 0 || Instigator.FiringMode == 3))
	{
		SpawnBeam(GetEffectLocation(), HitLocation, false);
	}
}

simulated function bool AllowImpactEffects(Actor HitActor, vector HitLocation, vector HitNormal)
{
	return (HitActor != None && UTProj_ShockBall(HitActor) == None && Super.AllowImpactEffects(HitActor, HitLocation, HitNormal));
}

simulated function SetMuzzleFlashParams(ParticleSystemComponent PSC)
{
	local float PathValues[3];
	local int NewPath;
	Super.SetMuzzleFlashparams(PSC);
	if (Instigator.FiringMode == 0)
	{
		NewPath = Rand(3);
		if (NewPath == CurrentPath)
		{
			NewPath++;
		}
		CurrentPath = NewPath % 3;

		PathValues[CurrentPath % 3] = 1.0;
		PSC.SetFloatParameter('Path1',PathValues[0]);
		PSC.SetFloatParameter('Path2',PathValues[1]);
		PSC.SetFloatParameter('Path3',PathValues[2]);
//			CurrentPath++;
	}
	else if (Instigator.FiringMode == 3)
	{
		PSC.SetFloatParameter('Path1',1.0);
		PSC.SetFloatParameter('Path2',1.0);
		PSC.SetFloatParameter('Path3',1.0);
	}
	else
	{
		PSC.SetFloatParameter('Path1',0.0);
		PSC.SetFloatParameter('Path2',0.0);
		PSC.SetFloatParameter('Path3',0.0);
	}

}


defaultproperties
{
	// Weapon SkeletalMesh
	Begin Object Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'WP_ShockRifle.Mesh.SK_WP_ShockRifle_3P'
	End Object

	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'WP_ShockRifle.Particles.P_WP_ShockRifle_Beam_Impact',Sound=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_AltFireImpact_Cue')
	BulletWhip=SoundCue'A_Weapon_ShockRifle.Cue.A_Weapon_SR_WhipCue'

	MuzzleFlashSocket=MuzzleFlashSocket
//	MuzzleFlashPSCTemplate=WP_ShockRifle.Particles.P_ShockRifle_MF_Primary
	MuzzleFlashPSCTemplate=WP_ShockRifle.Particles.P_ShockRifle_3P_MF
	MuzzleFlashAltPSCTemplate=WP_ShockRifle.Particles.P_ShockRifle_3P_MF
	MuzzleFlashColor=(R=255,G=120,B=255,A=255)
	MuzzleFlashDuration=0.33;
	MuzzleFlashLightClass=class'UTGame.UTShockMuzzleFlashLight'
	BeamTemplate=particlesystem'WP_ShockRifle.Particles.P_WP_ShockRifle_Beam'
	WeaponClass=class'UTWeap_ShockRifle'
	ImpactLightClass=class'UTShockImpactLight'
}
