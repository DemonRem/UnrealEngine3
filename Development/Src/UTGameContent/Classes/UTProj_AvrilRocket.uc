/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_AvrilRocket extends UTProj_AvrilRocketBase;


defaultproperties
{
	ProjFlightTemplate=ParticleSystem'WP_AVRiL.Particles.P_WP_Avril_Smoke_Trail'
	ProjExplosionTemplate=ParticleSystem'WP_AVRiL.Particles.P_WP_Avril_Explo'
	ExplosionLightClass=class'UTGame.UTRocketExplosionLight'
	speed=550.0
	MaxSpeed=2800.0
	Damage=125.0
	DamageRadius=150.0
	MomentumTransfer=150000
	MyDamageType=class'UTDmgType_AvrilRocket'
	LifeSpan=7.0
	bProjTarget=True

	AmbientSound=SoundCue'A_Weapon.RocketLauncher.Cue.A_Weapon_RL_Travel_Cue'
	ExplosionSound=SoundCue'A_Weapon.AVRiL.Cue.A_Weapon_AVRiL_ImpactCue'

	RotationRate=(Roll=50000)
	DesiredRotation=(Roll=30000)
	bCollideWorld=true

	Begin Object Name=ProjectileMesh
		StaticMesh=StaticMesh'WP_AVRiL.Mesh.S_WP_AVRiL_Missile'
		Scale=0.75
	End Object

	Begin Object Name=CollisionCylinder
		CollisionRadius=15
		CollisionHeight=10
		AlwaysLoadOnClient=True
		AlwaysLoadOnServer=True
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=true
		CollideActors=true
	End Object

	bUpdateSimulatedPosition=true
	AccelRate=750.0
	bCollideComplex=false

	bNetTemporary=false
	bWaitForEffects=true
	bRotationFollowsVelocity=true

	LockWarningInterval=1.0
}
