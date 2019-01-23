/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_LeviathanShockBall extends UTProj_ShockBall;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	bWideCheck = true;
}

/**
 * Explode this Projectile
 */
simulated function Explode(vector HitLocation, vector HitNormal)
{
	ComboExplosion();
}

defaultproperties
{
	Speed=1500
	MaxSpeed=1500
	Damage=45
	DamageRadius=128
	MomentumTransfer=70000
	ComboDamageType=class'UTDmgType_LeviathanShockBall'
	MyDamageType=class'UTDmgType_LeviathanShockBall'
	CheckRadius=300.0
	ComboDamage=120

	ProjFlightTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_ShockBall'
	ProjExplosionTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_ShockBallImpact'
	ComboTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_ShockballCombo'
}
