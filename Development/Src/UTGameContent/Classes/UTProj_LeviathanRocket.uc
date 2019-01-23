/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_LeviathanRocket extends UTProj_Rocket;

defaultproperties
{
	Speed=6000.0
	MaxSpeed=6000.0
	MomentumTransfer=50000
	Damage=100
	DamageRadius=220.0
	MyDamageType=class'UTDmgType_LeviathanRocket'
	
	ProjFlightTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_MissileTrailIgnited'
	ProjExplosionTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_MissileExplosion'
}
