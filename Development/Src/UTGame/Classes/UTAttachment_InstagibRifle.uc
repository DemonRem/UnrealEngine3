/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAttachment_InstagibRifle extends UTWeaponAttachment;

var Color	InstaBeamColors[2];
var ParticleSystem BeamTemplate;
var MaterialImpactEffect TeamImpactEffects[2];
var MaterialInterface	TeamSkins[4];


simulated function color GetBeamColor()
{
	if (Instigator.GetTeamNum() != 1)
	{
		return InstaBeamColors[0];
	}
	else
	{
		return InstaBeamColors[1];
	}
}

simulated function SpawnBeam(vector Start, vector End)
{
	local ParticleSystemComponent E;

	if ( End == Vect(0,0,0) )
	{
	    	return;
	}

	E = WorldInfo.MyEmitterPool.SpawnEmitter(BeamTemplate, Start);
	if (WorldInfo.GRI != None && WorldInfo.GRI.GameClass != None && WorldInfo.GRI.GameClass.default.bTeamGame)
	{
		E.SetColorParameter('ShockBeamColor', GetBeamColor());
	}
	E.SetVectorParameter('ShockBeamEnd', End);
}

simulated function FirstPersonFireEffects(Weapon PawnWeapon, vector HitLocation)	// Should be subclassed
{
	super.FirstPersonFireEffects(PawnWeapon , HitLocation);
	SpawnBeam( UTWeapon(PawnWeapon).GetEffectLocation(), HitLocation);
}

simulated function ThirdPersonFireEffects(vector HitLocation)
{
	Super.ThirdPersonFireEffects(HitLocation);

	SpawnBeam(GetEffectLocation(), HitLocation);
}

simulated function SetSkin(Material NewMaterial)
{
	local int TeamIndex;

	if ( NewMaterial == None ) 	// Clear the materials
	{
		TeamIndex = Instigator.GetTeamNum();
		if ( TeamIndex == 255 )
			TeamIndex = 0;
		Mesh.SetMaterial(0,TeamSkins[TeamIndex]);
	}
	else
	{
		Super.SetSkin(NewMaterial);
	}
}

simulated function PostBeginPlay()
{
	if(Instigator.GetTeamNum()==1)
	{
		DefaultImpactEffect=TeamImpactEffects[1];
		MuzzleFlashAltPSCTemplate=ParticleSystem'WP_ShockRifle.Particles.P_Shockrifle_Instagib_3P_MF_Blue';
		MuzzleFlashPSCTemplate=ParticleSystem'WP_ShockRifle.Particles.P_Shockrifle_Instagib_3P_MF_Blue';
		BeamTemplate=ParticleSystem'WP_Shockrifle.Particles.P_Shockrifle_Instagib_Beam_Blue';
	}
	else
	{
		DefaultImpactEffect=TeamImpactEffects[0];
		MuzzleFlashAltPSCTemplate=ParticleSystem'WP_ShockRifle.Particles.P_Shockrifle_Instagib_3P_MF_Red';
		MuzzleFlashPSCTemplate=ParticleSystem'WP_ShockRifle.Particles.P_Shockrifle_Instagib_3P_MF_Red';
		BeamTemplate=ParticleSystem'WP_Shockrifle.Particles.P_Shockrifle_Instagib_Beam_Red';
	}
	Super.PostBeginPlay();
}

defaultproperties
{

	// Weapon SkeletalMesh
	Begin Object Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'WP_ShockRifle.Mesh.SK_WP_ShockRifle_3P'
	End Object

	//DefaultImpactEffect=(ParticleTemplate=ParticleSystem'WP_ShockRifle.Particles.P_WP_ShockRifle_Beam_Impact',Sound=SoundCue'A_Weapon.ShockRifle.Cue.A_Weapon_SR_AltFireImpact_Cue')
	TeamImpactEffects[0]=(ParticleTemplate=ParticleSystem'WP_Shockrifle.Particles.P_Shockrifle_Instagib_Impact_Red',Sound=SoundCue'A_Weapon_ShockRifle.Cue.A_Weapon_SR_InstagibImpactCue')
	TeamImpactEffects[1]=(ParticleTemplate=ParticleSystem'WP_Shockrifle.Particles.P_Shockrifle_Instagib_Impact_Blue',Sound=SoundCue'A_Weapon_ShockRifle.Cue.A_Weapon_SR_InstagibImpactCue')
	BulletWhip=SoundCue'A_Weapon_ShockRifle.Cue.A_Weapon_SR_WhipCue'

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashColor=(R=255,G=120,B=255,A=255)
	MuzzleFlashDuration=0.33;
	MuzzleFlashLightClass=class'UTGame.UTShockMuzzleFlashLight'

	InstaBeamColors(0)=(R=255,G=64,B=64,A=180);
	InstaBeamColors(1)=(R=64,G=64,B=255,A=180);
	BeamTemplate=particlesystem'WP_ShockRifle.Particles.P_WP_ShockRifle_Beam'

	WeaponClass=class'UTWeap_InstagibRifle'

	TeamSkins(0)=MaterialInterface'WP_ShockRifle.Materials.M_WP_ShockRifle_Instagib_Red'
	TeamSkins(1)=MaterialInterface'WP_ShockRifle.Materials.M_WP_ShockRifle_Instagib_Blue'
	TeamSkins(2)=none
	TeamSkins(3)=none

}
