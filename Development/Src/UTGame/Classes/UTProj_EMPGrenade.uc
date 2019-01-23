/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_EMPGrenade extends UTProjectile;

var float EMPDamage;

simulated function Explode(vector HitLocation, vector HitNormal) // EMP Grenades disables shield belts, vehicles, and most deployables.
{
	class'UTEMPExplosion'.static.Explode(self,DamageRadius,EMPDamage);
	Super.Explode(HitLocation, HitNormal);
}


defaultproperties
{
	DamageRadius=+200.0
	speed=1200.000000
	Damage=0.000000
	MomentumTransfer=0
	MyDamageType=none;
	LifeSpan=6.0
	bCollideWorld=true
	TossZ=+305.0
	CheckRadius=40.0
	EMPDamage=300.0

	ProjFlightTemplate=ParticleSystem'WP_ImpactHammer.Effects.P_WP_EMP_Trail'
	ProjExplosionTemplate=ParticleSystem'WP_ImpactHammer.Particles.P_WP_EMPExplosion'
	ExplosionLightClass=class'UTGame.UTRocketExplosionLight'

	AmbientSound=SoundCue'A_Weapon.ImpactHammer.Cue.A_Weapon_HC_EMPAmbient_Cue'
	ExplosionSound=SoundCue'A_Weapon.ImpactHammer.Cue.A_Weapon_IH_EMPImpact_Cue'

	Physics=PHYS_Falling

	DrawScale=1.5

	// Pickup mesh Transform
	Begin Object Name=ProjectileMesh
		StaticMesh=StaticMesh'WP_FlakCannon.Mesh.S_WP_Flak_Shell'
		scale=1.5
		Rotation=(Pitch=-16384)
		CullDistance=4000
	End Object

	bWaitForEffects=true
	bRotationFollowsVelocity=true
}
