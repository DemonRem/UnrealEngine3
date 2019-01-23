/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTOnslaughtMiningRobotController extends AIController;

var float PauseTime;
var NavigationPoint OldMoveTarget;


event bool NotifyBump(Actor Other, Vector HitNormal)
{
	local Pawn P;

	Disable('NotifyBump');
	SetTimer(1.0, false, 'BumpTimer');

	P = Pawn(Other);
	if ( (P == None) || (P.Controller == None) )
		return false;

	AdjustAround(P);
	return false;
}

function AdjustAround(Pawn Other)
{
	local vector VelDir, OtherDir, SideDir;

	if ( !InLatentExecution(LATENT_MOVETOWARD) )
		return;

	VelDir = Normal(MoveTarget.Location - Pawn.Location);
	VelDir.Z = 0;
	OtherDir = Other.Location - Pawn.Location;
	OtherDir.Z = 0;
	OtherDir = Normal(OtherDir);
	if ( (VelDir Dot OtherDir) > 0.7 )
	{
		bAdjusting = true;
		SideDir.X = VelDir.Y;
		SideDir.Y = -1 * VelDir.X;
		if ( (SideDir Dot OtherDir) > 0 )
			SideDir *= -1;
		AdjustLoc = Pawn.Location + 3 * Other.GetCollisionRadius() * (0.5 * VelDir + SideDir);
	}
}

function BumpTimer()
{
	enable('NotifyBump');
}

auto state Mining
{
	ignores SeePlayer, SeeMonster;

	function FindPath()
	{
		local UTOnslaughtMiningRobot P;
		local NavigationPoint PreviousMoveTarget;

		P = UTOnslaughtMiningRobot(Pawn);
		RouteGoal = P.bIsCarryingOre ? P.Home : P.Home.Mine;

		if ( RouteGoal == None )
		{
			P.Destroy();
			return;
		}
		if ( MoveTarget != RouteGoal )
			PreviousMoveTarget = NavigationPoint(MoveTarget);

		if ( P.ReachedDestination(RouteGoal) )
		{
			PauseTime = P.bIsCarryingOre ? 2 : 6;
			GotoState('Mining', 'Pause');
			return;
		}

		MoveTarget = FindPathToward(RouteGoal,false);

		if ( MoveTarget == None )
		{
			if ( OldMoveTarget != None )
			{
				MoveTarget = OldMoveTarget;
				OldMoveTarget = None;
			}
			else
				P.Destroy();
			return;
		}
		if ( PreviousMoveTarget != None )
			OldMoveTarget = PreviousMoveTarget;
	}

Begin:
	FindPath();
	MoveToward(MoveTarget);
	Goto('Begin');
Pause:
	Sleep(PauseTime);
	if ( UTOnslaughtMiningRobot(Pawn).bIsCarryingOre )
	{
		UTOnslaughtMiningRobot(Pawn).Home.ReceiveOre(UTOnslaughtMiningRobot(Pawn).OreQuantity);
	}
	UTOnslaughtMiningRobot(Pawn).SetCarryingOre(!UTOnslaughtMiningRobot(Pawn).bIsCarryingOre);
	Goto('Begin');
}
