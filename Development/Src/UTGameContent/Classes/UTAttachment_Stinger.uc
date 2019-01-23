/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAttachment_Stinger extends UTWeaponAttachment;

simulated function SpawnTracer(vector EffectLocation,vector HitLocation)
{
	local UTEmitter E;
	local rotator HitDir;

	if (UTPawn(Owner).FiringMode == 0)
	{
		HitDir = rotator(HitLocation - EffectLocation);
		E = Spawn(class'UTEmitter',,, EffectLocation, HitDir);
		E.SetTemplate(ParticleSystem'WP_Stinger.Particles.P_WP_Stinger_AltFire_tracer');
		E.SetExtColorParameter('TracerColor',200,200,255,255);
		E.SetVectorParameter('BeamEndPoint',HitLocation);
		E.LifeSpan = VSize(HitLocation - EffectLocation) / 1200;
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

	Begin Object Class=UTSkeletalMeshComponent Name=SkelMeshComponent0
		SkeletalMesh=SkeletalMesh'WP_Stinger.Mesh.SK_WP_Stinger_3P'
		bOwnerNoSee=true
		bOnlyOwnerSee=false
		CollideActors=false
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		Translation=(X=0,Y=0)
		Scale=1.0
		CullDistance=4000
		bUseAsOccluder=FALSE
	End Object
    Mesh=SkelMeshComponent0

	WeapAnimType=EWAT_Stinger

	DefaultImpactEffect=(ParticleTemplate=ParticleSystem'WP_Stinger.Particles.P_WP_Stinger_Surface_Impact',Sound=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireImpactCue')
	DefaultAltImpactEffect=(ParticleTemplate=ParticleSystem'WP_Stinger.Particles.P_WP_Stinger_AltFire_Surface_Impact',Sound=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireAltImpactCue')
//	bAlignToSurfaceNormal=false

	BulletWhip=SoundCue'A_Weapon.Enforcers.Cue.A_Weapon_Enforcers_BulletWhizz_Cue'
	WeaponClass=class'UTWeap_Stinger_Content'
	MuzzleFlashLightClass=class'UTGameContent.UTStingerMuzzleFlashLight'
	MuzzleFlashSocket=MF
	MuzzleFlashPSCTemplate=ParticleSystem'WP_Stinger.Particles.P_Stinger_3P_MF_Primary'
	MuzzleFlashAltPSCTemplate=ParticleSystem'WP_Stinger.Particles.P_Stinger_3P_MF_Alt_Fire'

}



