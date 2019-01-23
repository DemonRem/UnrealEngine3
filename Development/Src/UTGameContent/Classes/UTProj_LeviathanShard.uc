/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTProj_LeviathanShard extends UTProj_StingerShard;

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionRadius=13
		CollisionHeight=13
	End Object

	Begin Object Name=ProjectileMesh
		Scale=2.5
	End Object

	MyDamageType=class'UTDmgType_LeviathanShard'
	bCollideComplex=false

	Components.Remove(ProjectileMesh)

	Speed=4500
	MaxSpeed=6000
	AccelRate=3000.0

	Damage=30
	DamageRadius=0
	MomentumTransfer=70000
	
	ProjExplosionTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_ShardImpact'
	ProjFlightTemplate=ParticleSystem'VH_Leviathan.Effects.PS_VH_Leviathan_Shard'	
}
