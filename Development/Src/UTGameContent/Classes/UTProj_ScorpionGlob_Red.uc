/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_ScorpionGlob_Red extends UTProj_ScorpionGlob;

defaultproperties
{
	ProjFlightTemplate = ParticleSystem'VH_Scorpion.Effects.P_Scorpion_Bounce_Projectile_Red'
	ProjExplosionTemplate= ParticleSystem'VH_Scorpion.Effects.PS_Scorpion_Gun_Impact_Red'
	BounceTemplate=ParticleSystem'VH_Scorpion.Effects.P_VH_Scorpion_Fireimpact'
	MyDamageType=class'UTDmgType_ScorpionGlobRed'
}
