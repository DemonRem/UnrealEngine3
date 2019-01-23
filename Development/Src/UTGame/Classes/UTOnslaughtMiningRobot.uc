/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtMiningRobot extends GamePawn
	notplaceable
	abstract;

var UTOnslaughtTarydiumProcessor Home;
var staticmeshcomponent CarriedOre;
var repnotify bool bIsCarryingOre;
var soundcue ExplosionSound;
var particlesystem ExplosionTemplate;

var float OreQuantity;

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		bIsCarryingOre;
}

simulated function PostBeginPlay()
{
	super.PostBeginPlay();

	CarriedOre.SetHidden(true);
	SpawnDefaultController();
}

simulated function byte GetTeamNum()
{
	return Home.ControllingNode.DefenderTeamIndex;
}

simulated event ReplicatedEvent(name VarName)
{
	if ( VarName == 'bIsCarryingOre' )
	{
		SetCarryingOre(bIsCarryingOre);
    }
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function SetCarryingOre(bool bNewValue)
{
	bIsCarryingOre = bNewValue;
	CarriedOre.SetHidden(!bIsCarryingOre);
}

simulated function Destroyed()
{
	Super.Destroyed();

	Home.MinerDestroyed();

	// blow up
	if ( (WorldInfo.NetMode != NM_DedicatedServer) && (ExplosionTemplate != None) && EffectIsRelevant(Location,false,5000) )
	{
		WorldInfo.MyEmitterPool.SpawnEmitter(ExplosionTemplate, Location, Rotation);
	}

	if (ExplosionSound != None)
	{
		PlaySound(ExplosionSound, true);
	}
}

simulated function PlayDying(class<DamageType> DamageType, vector HitLoc)
{
	if ( Controller != None )
		Controller.Destroy();
	Destroy();
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}
