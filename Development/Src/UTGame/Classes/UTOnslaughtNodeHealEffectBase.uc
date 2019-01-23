/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTOnslaughtNodeHealEffectBase extends UTReplicatedEmitter;

simulated function ShutDown()
{
	bTearOff = true;
	ParticleSystemComponent.DeactivateSystem();
	bDestroyOnSystemFinish = true;
	LifeSpan = 1.0;
}

simulated event TornOff()
{
	ShutDown();
}

defaultproperties
{
	LifeSpan=0.0
	ServerLifeSpan=0.0
	bNetTemporary=false
}
