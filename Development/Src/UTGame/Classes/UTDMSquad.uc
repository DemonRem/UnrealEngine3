/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// DMSquad.
// operational AI control for DeathMatch
// each bot is on its own squad
//=============================================================================

class UTDMSquad extends UTSquadAI;

simulated function DisplayDebug(HUD HUD, out float YL, out float YPos)
{
	local string EnemyList;
	local int i;
	local Canvas Canvas;

	Canvas = HUD.Canvas;
	Canvas.SetDrawColor(255,255,255);
	EnemyList = "     Enemies: ";
	for ( i=0; i<ArrayCount(Enemies); i++ )
		if ( Enemies[i] != None )
			EnemyList = EnemyList@Enemies[i].GetHumanReadableName();
	Canvas.DrawText(EnemyList, false);

	YPos += YL;
	Canvas.SetPos(4,YPos);
}

function bool IsDefending(UTBot B)
{
	return false;
}

function AddBot(UTBot B)
{
	Super.AddBot(B);
	SquadLeader = B;
}

function RemoveBot(UTBot B)
{
	if ( B.Squad != self )
		return;
	Destroy();
}

/*
Return true if squad should defer to C
*/
function bool ShouldDeferTo(Controller C)
{
	return false;
}

function bool CheckSquadObjectives(UTBot B)
{
	return false;
}

function bool WaitAtThisPosition(Pawn P)
{
	return false;
}

function bool NearFormationCenter(Pawn P)
{
	return true;
}

/* BeDevious()
return true if bot should use guile in hunting opponent (more expensive)
*/
function bool BeDevious()
{
	return ( (SquadMembers.Skill >= 4)
		&& (FRand() < 0.65 - 0.15 * WorldInfo.Game.NumBots) );
}

function name GetOrders()
{
	return CurrentOrders;
}

function bool SetEnemy( UTBot B, Pawn NewEnemy )
{
	local bool bResult;

	if ( (NewEnemy == None) || (NewEnemy.Health <= 0) || (NewEnemy.Controller == None)
		|| ((UTBot(NewEnemy.Controller) != None) && (UTBot(NewEnemy.Controller).Squad == self)) )
		return false;

	// add new enemy to enemy list - return if already there
	if ( !AddEnemy(NewEnemy) )
		return false;

	// reassess squad member enemy
	bResult = FindNewEnemyFor(B,(B.Enemy !=None) && B.LineOfSightTo(SquadMembers.Enemy));
	if ( bResult && (B.Enemy == NewEnemy) )
		B.AcquireTime = WorldInfo.TimeSeconds;
	return bResult;
}

function bool FriendlyToward(Pawn Other)
{
	return false;
}

function bool AssignSquadResponsibility(UTBot B)
{
	local Pawn PlayerPawn;
	local PlayerController PC;
	local actor MoveTarget;

	if ( B.Enemy == None )
	{
		// suggest inventory hunt
		if ( (B.Skill > 5) && (WorldInfo.Game.NumBots == 1) && ((B.Pawn.Weapon.AIRating > 0.7) || UTWeapon(B.Pawn.Weapon).bSniping) )
		{
			// maybe hunt player - only if have a fix on player location from sounds he's made
			foreach WorldInfo.AllControllers(class'PlayerController', PC)
			{
				if (PC.Pawn != None)
				{
					PlayerPawn = PC.Pawn;
					if ( (WorldInfo.TimeSeconds - PC.Pawn.Noise1Time < 5) || (WorldInfo.TimeSeconds - PC.Pawn.Noise2Time < 5) )
					{
						B.bHuntPlayer = true;
						if ( (WorldInfo.TimeSeconds - PC.Pawn.Noise1Time < 2) || (WorldInfo.TimeSeconds - PC.Pawn.Noise2Time < 2) )
						{
							B.LastKnownPosition = PC.Pawn.Location;
						}
						break;
					}
					else if ( (VSize(B.LastKnownPosition - PC.Pawn.Location) < 800)
								|| (VSize(B.LastKillerPosition - PC.Pawn.Location) < 800) )
					{
						B.bHuntPlayer = true;
						break;
					}
				}
			}
		}
		if ( B.FindInventoryGoal(0) )
		{
			B.GoalString = "Get pickup";
			B.bHuntPlayer = false;
			B.SetAttractionState();
			return true;
		}
		if ( B.bHuntPlayer )
		{
			B.bHuntPlayer = false;
			B.GoalString = "Hunt Player";
			if ( B.ActorReachable(PlayerPawn) )
				MoveTarget = PlayerPawn;
			else
				MoveTarget = B.FindPathToward(PlayerPawn,B.Pawn.bCanPickupInventory);
			if ( MoveTarget != None )
			{
				B.MoveTarget = MoveTarget;
				if ( B.CanSee(PlayerPawn) )
					SetEnemy(B,PlayerPawn);
				B.SetAttractionState();
				return true;
			}
		}

		// roam around level?
		return B.FindRoamDest();
	}
	return false;
}

defaultproperties
{
	CurrentOrders=Freelance
}
