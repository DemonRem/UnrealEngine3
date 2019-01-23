/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTTeleportBeacon extends UTDeployedActor
	abstract;

var ParticleSystem	DeathEmitterTemplate;
var soundcue ConstructedSound;
var MaterialInstanceConstant FloorMaterialInstance;
var LinearColor TeamFloorColor;

var private transient   MaterialInstanceConstant HUDMaterialInstance;

var float IconPosX, IconPosY;
var float IconExtentX, IconExtentY;

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		FloorMaterialInstance = Mesh.CreateAndSetMaterialInstanceConstant(1);
		FloorMaterialInstance.SetVectorParameterValue('Team_Color', TeamFloorColor);
		PlaySound(ConstructedSound);
	}

	// Add me to the global game objectives list

	if ( UTGameReplicationInfo(WorldInfo.GRI) != none )
	{
		UTGameReplicationInfo(WorldInfo.GRI).AddGameObjective(self);
	}


}

simulated function Destroyed()
{
	Super.Destroyed();

	WorldInfo.MyEmitterPool.SpawnEmitter(DeathEmitterTemplate, Location, Rotation);

	// Remove me from the global game objectives list
	if ( UTGameReplicationInfo(WorldInfo.GRI) != none )
	{
		UTGameReplicationInfo(WorldInfo.GRI).RemoveGameObjective(self);
	}


}

simulated function HitByEMP()
{
//	Destroy();
}

function AttemptTranslocation(UTPawn TransPawn)
{
	local vector PrevLocation;
	PrevLocation = TransPawn.Location;

	if ( TranslocSucceeded(TransPawn) )
	{
		// Cause any effects to occur
		TransPawn.DoTranslocate(PrevLocation);
	}
}

function bool TranslocSucceeded(UTPawn TransPawn)
{
	local vector newdest;

	if ( TransPawn.SetLocation(Location) )
	{
		return true;
	}

	newdest = Location + TransPawn.GetCollisionRadius()  * vect(1,1,0);
	if ( TransPawn.SetLocation(newdest) )
	{
		return true;
	}

	newdest = Location + TransPawn.GetCollisionRadius()  * vect(1,-1,0);
	if ( TransPawn.SetLocation(newdest) )
	{
		return true;
	}
	newdest = Location + TransPawn.GetCollisionRadius() * vect(-1,1,0);
	if ( TransPawn.SetLocation(newdest) )
	{
		return true;
	}

	newdest = Location + TransPawn.GetCollisionRadius() * vect(-1,-1,0);
	if ( TransPawn.SetLocation(newdest) )
	{
		return true;
	}

	return false;
}


defaultproperties
{
	LifeSpan=60.0

	bAlwaysRelevant=true
	bIgnoreRigidBodyPawns=true
	bBlockActors=true

	IconPosY=0
	IconExtentX=0.25
	IconPosX=0.25
	IconExtentY=0.125

}
