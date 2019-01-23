/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/** when an enemy player gets within range, spawns spider mines to attack that player */
class UTSpiderMineTrap extends UTDeployedActor
	abstract;

/** number of spider mines available */
var int NumMines;

/** range for detecting enemies */
var float DetectionRange;

function PostBeginPlay()
{
	Super.PostBeginPlay();

	if (Instigator == None && InstigatorController == None)
	{
		Destroy();
	}
	else
	{
		if (Instigator != None)
		{
			InstigatorController = Instigator.Controller;
		}
		SetTimer(0.5, true, 'CheckForEnemies');
	}
}

/** spawn a mine to attack the given target */
function SpawnMine(Pawn Target)
{
	local UTProj_SpiderMine Mine;

	Mine = spawn(class'UTProj_SpiderMine', self);
	if (Mine != None)
	{
		Mine.InstigatorController = InstigatorController;
		Mine.TeamNum = TeamNum;
		Mine.TargetPawn = Target;
		Mine.KeepTargetExtraRange = FMax(0.f, DetectionRange - Mine.DetectionRange);
		Mine.TossZ = 300.0;
		Mine.Init(Normal(Target.Location - Location));
		if (--NumMines <= 0)
		{
			// out of mines
			Destroy();
		}
	}
}

/** called on a timer to check for enemies to target with mines */
function CheckForEnemies()
{
	local Controller C;


	// make sure we have an Instigator
	if (Instigator == None)
	{
		if (InstigatorController == None)
		{
			// no one to take credit, so destroy
			Destroy();
		}
		else
		{
			Instigator = InstigatorController.Pawn;
		}
	}

	if ( InstigatorController == none || !WorldInfo.GRI.OnSameTeam(self, InstigatorController) )
	{
		Destroy();
		return;
	}


	if (!bDeleteMe)
	{
		foreach WorldInfo.AllControllers(class'Controller', C)
		{
			if ( C.Pawn != None && !WorldInfo.GRI.OnSameTeam(self, C) &&
				VSize(C.Pawn.Location - Location) < DetectionRange && FastTrace(C.Pawn.Location, Location) )
			{
				SpawnMine(C.Pawn);
			}
		}
	}
}

defaultproperties
{
	Components.Remove(CollisionCylinder)

	Begin Object Class=StaticMeshComponent Name=ThirdPersonMesh
		CollideActors=true
		BlockActors=true
		BlockRigidBody=true
		bUseAsOccluder=FALSE
	End Object
	Components.Add(ThirdPersonMesh)
	Mesh=ThirdPersonMesh
	CollisionComponent=ThirdPersonMesh
	bIgnoreRigidBodyPawns=true

	NumMines=15
	DetectionRange=1500.0
}
