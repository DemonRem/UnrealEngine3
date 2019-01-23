/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_Rocket extends UTProjectile;

defaultproperties
{
	Begin Object Class=StaticMeshComponent Name=ProjectileMesh
		StaticMesh=StaticMesh'WP_RocketLauncher.Mesh.S_WP_Rocketlauncher_Rocket'
		Scale=1.0
		CollideActors=false
		CastShadow=false
		bAcceptsLights=false
		CullDistance=7500
		BlockRigidBody=false
		BlockActors=false
		bUseAsOccluder=FALSE
	End Object
	Components.Add(ProjectileMesh)

	ProjFlightTemplate=ParticleSystem'WP_RocketLauncher.Effects.P_WP_RocketLauncher_RocketTrail'
	ProjExplosionTemplate=ParticleSystem'WP_RocketLauncher.Effects.P_WP_RocketLauncher_RocketExplosion'
	ExplosionDecal=Material'WP_RocketLauncher.M_WP_RocketLauncher_ImpactDecal'
	DecalWidth=128.0
	DecalHeight=128.0
	speed=1350.0
	MaxSpeed=1350.0
	Damage=100.0
	DamageRadius=220.0
	MomentumTransfer=85000
	MyDamageType=class'UTDmgType_Rocket'
	LifeSpan=8.0
	AmbientSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Travel_Cue'
	ExplosionSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Impact_Cue'
	RotationRate=(Roll=50000)
	DesiredRotation=(Roll=30000)
	bCollideWorld=true
	CheckRadius=40.0
	bCheckProjectileLight=true
	ProjectileLightClass=class'UTGame.UTRocketLight'
	ExplosionLightClass=class'UTGame.UTRocketExplosionLight'

	bWaitForEffects=true
}
