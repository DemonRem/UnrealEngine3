/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_Redeemer extends UTProj_RedeemerBase;

defaultproperties
{
	ProjFlightTemplate=ParticleSystem'WP_RocketLauncher.Effects.P_WP_RocketLauncher_RocketTrail'
	ProjExplosionTemplate=ParticleSystem'WP_Redeemer.Particles.P_WP_Redeemer_Explo'
	speed=1000.0
	MaxSpeed=1000.0
	Damage=250.0
	DamageRadius=2000.0
	MomentumTransfer=250000
	MyDamageType=class'UTDmgType_Redeemer'
	LifeSpan=20.00

	bCollideWorld=true
	bCollideActors=true
	bProjTarget=true
	bCollideComplex=false
	bNetTemporary=false

	Begin Object Name=CollisionCylinder
		CollisionRadius=24
		CollisionHeight=12
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		CollideActors=true
	End Object

	Begin Object Name=ProjectileMesh
		StaticMesh=StaticMesh'WP_AVRiL.Mesh.S_WP_AVRiL_Missile'
		CullDistance=20000
		Scale=1.5
	End Object

	AmbientSound=SoundCue'A_Weapon.Redeemer.Cue.A_Weapon_Redeemer_Travel_Cue'
	ExplosionSound=SoundCue'A_Weapon.Redeemer.Cue.A_Weapon_Redeemer_Impact_Cue'
	bWaitForEffects=false
	bImportantAmbientSound=true

	BaseExplosionShake=(RotMag=(Z=250),RotRate=(Z=2500),RotTime=6,OffsetMag=(Z=15),OffsetRate=(Z=200),OffsetTime=10)

	ExplosionClass=class'UTEmit_SmallRedeemerExplosion'
}
