/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/** when an enemy player gets within range, spawns spider mines to attack that player */
class UTSpiderMineTrap extends UTDeployedActor
	abstract;

/** number of spider mines available */
var int AvailableMines;

/** currently deployed spidermine count */
var int DeployedMines;

/** range for detecting enemies */
var float DetectionRange;
/** class of spider mine to spawn */
var class<UTProj_SpiderMineBase> MineClass;

function PostBeginPlay()
{
	Super.PostBeginPlay();
	SkeletalMeshComponent(Mesh).PlayAnim('Deploy');

	SetTimer(0.5, false, 'CheckForEnemies');
}

/** spawn a mine to attack the given target */
function UTProj_SpiderMineBase SpawnMine(Pawn Target, vector TargetDir)
{
	local UTProj_SpiderMineBase Mine;

	if ( AvailableMines > 0 )
	{
		Mine = Spawn(MineClass,,, Location + vect(0,0,25));
		if (Mine != None)
		{
			Mine.Lifeline = self;
			Mine.InstigatorController = InstigatorController;
			Mine.TeamNum = TeamNum;
			Mine.TargetPawn = Target;
			Mine.KeepTargetExtraRange = FMax(0.f, DetectionRange - Mine.DetectionRange);
			Mine.TossZ = 300.0;
			Mine.Init(TargetDir);
			AvailableMines--;
			DeployedMines++;
		}
	}
	return Mine;
}

/** called on a timer to check for enemies to target with mines */
function CheckForEnemies()
{
	local Controller C;
	local float NextInterval;

	// make sure we have an Instigator
	if (Instigator == None)
	{
		if (InstigatorController == None)
		{
			// no one to take credit, so destroy
			Destroy();
			return;
		}
		else
		{
			Instigator = InstigatorController.Pawn;
		}
	}

	if (InstigatorController == None || !WorldInfo.GRI.OnSameTeam(self, InstigatorController) )
	{
		Destroy();
		return;
	}

	if ( AvailableMines + DeployedMines <= 0 )
	{
		// out of mines
		Destroy();
		return;
	}

	if (!bDeleteMe)
	{
		NextInterval = 0.5;
		foreach WorldInfo.AllControllers(class'Controller', C)
		{
			if ( C.Pawn != None && !WorldInfo.GRI.OnSameTeam(self, C) && !C.Pawn.IsA('UTVehicle_DarkWalker')
				&& VSize(C.Pawn.Location - Location) < DetectionRange && FastTrace(C.Pawn.Location, Location) )
			{
				SpawnMine(C.Pawn, Normal(C.Pawn.Location - Location));
				NextInterval = 1.5;
			}
		}
		SetTimer(NextInterval, false, 'CheckForEnemies');
	}
}

defaultproperties
{
	AvailableMines=15
	DetectionRange=1500.0

		Begin Object Class=CylinderComponent Name=ColComp
	End Object
	Components.Add(ColComp)
	CollisionComponent=ColComp
}
