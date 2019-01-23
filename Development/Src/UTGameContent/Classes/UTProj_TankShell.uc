/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_TankShell extends UTProjectile;

defaultproperties
{
	ProjFlightTemplate=ParticleSystem'VH_Goliath.Effects.PS_Goliath_Cannon_Trail'
	ProjExplosionTemplate=ParticleSystem'VH_Goliath.Effects.PS_Goliath_Cannon_Impact'
	MaxExplosionLightDistance=+7000.0
	speed=15000.0
	MaxSpeed=15000.0
	Damage=300
	DamageRadius=660
	MomentumTransfer=150000
	MyDamageType=class'UTDmgType_TankShell'
	LifeSpan=1.2
	AmbientSound=SoundCue'A_Weapon.RocketLauncher.Cue.A_Weapon_RL_Travel_Cue'
	ExplosionSound=SoundCue'A_Weapon.RocketLauncher.Cue.A_Weapon_RL_Impact_Cue'
	RotationRate=(Roll=50000)
	DesiredRotation=(Roll=30000)
	bCollideWorld=true
	ExplosionLightClass=class'UTGame.UTTankShellExplosionLight'
	ExplosionDecal=MaterialInstance'VH_Goliath.Materials.DM_Goliath_Cannon_Decal'
	DecalWidth=350
	DecalHeight=350
	Begin Object Name=ProjectileMesh
		StaticMesh=StaticMesh'WP_RocketLauncher.Mesh.S_WP_Rocketlauncher_Rocket'
		Scale=1.5
	End Object

	bWaitForEffects=true
}
