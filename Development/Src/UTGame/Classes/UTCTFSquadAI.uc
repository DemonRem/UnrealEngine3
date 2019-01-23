/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCTFSquadAI extends UTSquadAI;

var float LastSeeFlagCarrier;
var UTCTFFlag FriendlyFlag, EnemyFlag;
var NavigationPoint HidePath;

function bool AllowDetourTo(UTBot B,NavigationPoint N)
{
	if ( !B.PlayerReplicationInfo.bHasFlag )
		return true;

	if ( (B.RouteGoal != FriendlyFlag.HomeBase) || !FriendlyFlag.bHome )
		return true;
	return ( N.LastDetourWeight * B.RouteDist > 2 );
}

function bool ShouldUseAlternatePaths()
{
	return true;
}

/* FindPathToObjective()
Returns path a bot should use moving toward a base
*/
function bool FindPathToObjective(UTBot B, Actor O)
{
	if ( (UTVehicle(B.Pawn) != None) && ((UTCTFFlag(O) != None) || (UTCTFBase(O) != None))
		&& (VSize(B.Pawn.Location - O.Location) < 1000) && B.LineOfSightTo(O) )
	{
		B.FormerVehicle = UTVehicle(B.Pawn);
		B.NoVehicleGoal = O;
		B.RouteGoal = O;
		B.LeaveVehicle(true);
		return true;
	}

	if ( !B.PlayerReplicationInfo.bHasFlag || (O != FriendlyFlag.HomeBase) )
		return Super.FindPathToObjective(B, O);

	if ( O != RouteObjective )
		return B.SetRouteToGoal(O);

	B.MoveTarget = B.FindPathToSquadRoute(B.Pawn.bCanPickupInventory && (Vehicle(B.Pawn) == None));
	return B.StartMoveToward(O);
}

function bool AllowTranslocationBy(UTBot B)
{
	return ( B.Pawn != EnemyFlag.Holder );
}

/* GoPickupFlag()
have bot go pickup dropped friendly flag
*/
function bool GoPickupFlag(UTBot B)
{
	if ( FindPathToObjective(B,FriendlyFlag) )
	{
		if ( WorldInfo.TimeSeconds - UTCTFTeamAI(Team.AI).LastGotFlag > 6 )
		{
			UTCTFTeamAI(Team.AI).LastGotFlag = WorldInfo.TimeSeconds;
			B.SendMessage(None, 'OTHER', B.GetMessageIndex('GOTOURFLAG'), 20, 'TEAM');
		}
		B.GoalString = "Pickup friendly flag";
		return true;
	}
	return false;
}

function actor FormationCenter()
{
	if ( (SquadObjective != None) && (SquadObjective.DefenderTeamIndex == Team.TeamIndex) )
		return SquadObjective;
	if ( (EnemyFlag.Holder != None) && (GetOrders() != 'Defend') && !SquadLeader.IsA('PlayerController') )
		return EnemyFlag.Holder;
	return SquadLeader.Pawn;
}

function bool VisibleToEnemiesOf(Actor A, UTBot B)
{
	if ( (B.Enemy != None) && FastTrace(A.Location, B.Enemy.Location + B.Enemy.GetCollisionHeight() * vect(0,0,1)) )
		return true;
	return false;
}

function NavigationPoint FindHidePathFor(UTBot B)
{
	//local PickupFactory P;
	local PickupFactory Best;
/* //@todo steve
	//local float NewD, BestD;
	local int MyTeamNum, EnemyTeamNum;

	MyTeamNum = Team.TeamIndex;
	if ( MyTeamNum == 0 )
		EnemyTeamNum = 1;

	// look for nearby inventory
	// stay away from enemies, and enemy base
	// don't go too far
	foreach WorldInfo.AllNavigationPoints(class'PickupFactory', P)
		if ( (P.BaseDist[MyTeamNum] < FMin(2400,0.7*P.BaseDist[EnemyTeamNum]))
			&& !FastTrace(P.Location + (88 - 2*P.CollisionHeight)*Vect(0,0,1), Location + (88 - 2*P.CollisionHeight)*Vect(0,0,1)) )
		{
			// FIXME - at start of match, clear NearestBase if visible to other base, instead of tracing here
			if ( Best == None )
			{
				if ( !VisibleToEnemiesOf(P,B) )
				{
					Best = P;
					if ( Best.ReadyToPickup(3) )
						BestD = Best.BotDesireability(B.Pawn);
				}
			}
			else if ( !Best.ReadyToPickup(3) )
			{
				if ( (P.ReadyToPickup(3) || (FRand() < 0.5))
					&& !VisibleToEnemiesOf(P,B)  )
				{
					Best = P;
					BestD = Best.BotDesireability(B.Pawn);
				}
			}
			else if ( P.ReadyToPickup(3) )
			{
				NewD = P.BotDesireability(B.Pawn);
				if ( (NewD > BestD) && !VisibleToEnemiesOf(P,B) )
				{
					Best = P;
					BestD = NewD;
				}
			}
		}
*/
	Best = None;
	return Best;
}

function bool CheckVehicle(UTBot B)
{
	if ( (EnemyFlag.Holder == None) && (VSize(B.Pawn.Location - EnemyFlag.Position().Location) < 1600) )
		return false;
	if ( B.PlayerReplicationInfo.bHasFlag && (VSize(B.Pawn.Location - FriendlyFlag.HomeBase.Location) < 1600) )
		return false;

	return Super.CheckVehicle(B);
}

/* OrdersForFlagCarrier()
Tell bot what to do if he's carrying the flag
*/
function bool OrdersForFlagCarrier(UTBot B)
{
	local UTCTFBase FlagBase;

	if ( CheckVehicle(B) )
	{
		return true;
	}

	// pickup dropped flag if see it nearby
	// FIXME - don't use pure distance - also check distance returned from pathfinding
	if ( !FriendlyFlag.bHome )
	{
		// if one-on-one ctf, then get flag back
		if ( Team.Size == 1 )
		{
			// make sure healthed/armored/ammoed up
			if ( B.NeedWeapon() && B.FindInventoryGoal(0) )
			{
				B.SetAttractionState();
				return true;
			}

			if ( FriendlyFlag.Holder == None )
			{
				if ( GoPickupFlag(B) )
					return true;
				return false;
			}
			else
			{
				if ( (B.Enemy != None) && (B.Enemy.PlayerReplicationInfo != None) && !B.Enemy.PlayerReplicationInfo.bHasFlag )
					FindNewEnemyFor(B,(B.Enemy != None) && B.LineOfSightTo(B.Enemy));
				if ( WorldInfo.TimeSeconds - LastSeeFlagCarrier > 6 )
					LastSeeFlagCarrier = WorldInfo.TimeSeconds;
				B.GoalString = "Attack enemy flag carrier";
				if ( B.IsSniping() )
					return false;
				B.bPursuingFlag = true;
				return ( TryToIntercept(B,FriendlyFlag.Holder,EnemyFlag.Homebase) );
			}
		}
		// otherwise, only get if convenient
		if ( (FriendlyFlag.Holder == None) && B.LineOfSightTo(FriendlyFlag.Position())
			&& (VSize(B.Pawn.Location - FriendlyFlag.Location) < 1500.f)
			&& GoPickupFlag(B) )
			return true;

		// otherwise, go hide
		if ( HidePath != None )
		{
			if ( B.Pawn.ReachedDestination(HidePath) )
			{
				if ( ((B.Enemy == None) || (WorldInfo.TimeSeconds - B.LastSeenTime > 7)) && (FRand() < 0.7) )
				{
					HidePath = None;
					if ( B.Enemy == None )
						B.WanderOrCamp();
					else
						B.DoStakeOut();
					return true;
				}
			}
			else if ( B.SetRouteToGoal(HidePath) )
				return true;
		}
	}
	else
		B.bPursuingFlag = false;
	HidePath = None;

	// super pickups if nearby
	// see if should get superweapon/ pickup
	if ( (B.Skill > 2) && (Vehicle(B.Pawn) == None) )
	{
		if ( (!FriendlyFlag.bHome || (VSize(FriendlyFlag.HomeBase.Location - B.Pawn.Location) > 2000))
				&& Team.AI.SuperPickupAvailable(B)
				&& (B.Pawn.Anchor != None) && B.Pawn.ReachedDestination(B.Pawn.Anchor)
				&& B.FindSuperPickup(800) )
		{
			B.GoalString = "Get super pickup";
			B.SetAttractionState();
			return true;
		}
	}

	if ( (B.Enemy != None) && (B.Pawn.Health < 60 ))
		B.SendMessage(None, 'OTHER', B.GetMessageIndex('NEEDBACKUP'), 25, 'TEAM');
	B.GoalString = "Return to Base with enemy flag!";
	if ( !FindPathToObjective(B,FriendlyFlag.HomeBase) )
	{
		B.GoalString = "No path to home base for flag carrier";
		// FIXME - suicide after a while
		return false;
	}
	if ( B.MoveTarget == FriendlyFlag.HomeBase )
	{
		B.GoalString = "Near my Base with enemy flag!";
		if ( !FriendlyFlag.bHome )
		{
			B.SendMessage(None, 'OTHER', B.GetMessageIndex('NEEDOURFLAG'), 25, 'TEAM');
			B.GoalString = "NEED OUR FLAG BACK!";
			if ( B.Skill > 1 )
				HidePath = FindHidePathFor(B);
			if ( (HidePath != None) && B.SetRouteToGoal(HidePath) )
				return true;
			return false;
		}
		ForEach TouchingActors(class'UTCTFBase', FlagBase)
		{
			if ( FlagBase == FriendlyFlag.HomeBase )
			{
				FriendlyFlag.Touch(B.Pawn, None, B.Pawn.Location, vect(0,0,1));
				break;
			}
		}
	}
	return true;
}

function bool MustKeepEnemy(Pawn E)
{
	if ( (E != None) && (E.PlayerReplicationInfo != None) && E.PlayerReplicationInfo.bHasFlag && (E.Health > 0) )
		return true;
	return false;
}

function bool NearEnemyBase(UTBot B)
{
	return EnemyFlag.Homebase.BotNearObjective(B);
}

function bool NearHomeBase(UTBot B)
{
	return FriendlyFlag.Homebase.BotNearObjective(B);
}

function bool FlagNearBase()
{
	if ( WorldInfo.TimeSeconds - FriendlyFlag.TakenTime < UTCTFBase(FriendlyFlag.HomeBase).BaseExitTime )
		return true;

	return ( VSize(FriendlyFlag.Position().Location - FriendlyFlag.HomeBase.Location) < FriendlyFlag.HomeBase.BaseRadius );
}

function bool OverrideFollowPlayer(UTBot B)
{
	if ( !EnemyFlag.bHome )
		return false;

	if ( EnemyFlag.HomeBase.BotNearObjective(B) )
		return EnemyFlag.HomeBase.TellBotHowToDisable(B);
	return false;
}

function bool CheckSquadObjectives(UTBot B)
{
	local bool bSeeFlag;
	local actor FlagCarrierTarget;
	local controller FlagCarrier;

	if ( B.PlayerReplicationInfo.bHasFlag )
		return OrdersForFlagCarrier(B);

	AddTransientCosts(B,1);
	if ( !FriendlyFlag.bHome  )
	{
		bSeeFlag = B.LineOfSightTo(FriendlyFlag.Position());
		if ( Team.Size == 1 )
		{
			if ( B.NeedWeapon() && B.FindInventoryGoal(0) )
			{
				B.SetAttractionState();
				return true;
			}

			// keep attacking if 1-0n-1
			if ( (FriendlyFlag.Holder != None) || (VSize(B.Pawn.Location - FriendlyFlag.Position().Location) > VSize(B.Pawn.Location - EnemyFlag.Position().Location)) )
				return FindPathToObjective(B,EnemyFlag.Position());
		}
		if ( bSeeFlag )
		{
			if ( FriendlyFlag.Holder == None )
			{
				if ( GoPickupFlag(B) )
					return true;
			}
			else
			{
				if ( (B.Enemy == None) || ((B.Enemy.PlayerReplicationInfo != None) && !B.Enemy.PlayerReplicationInfo.bHasFlag) )
					FindNewEnemyFor(B,(B.Enemy != None) && B.LineOfSightTo(B.Enemy));
				if ( WorldInfo.TimeSeconds - LastSeeFlagCarrier > 6 )
				{
					LastSeeFlagCarrier = WorldInfo.TimeSeconds;
					B.SendMessage(None, 'OTHER', B.GetMessageIndex('ENEMYFLAGCARRIERHERE'), 10, 'TEAM');
				}
				B.GoalString = "Attack enemy flag carrier";
				if ( B.IsSniping() )
					return false;
				B.bPursuingFlag = true;
				return ( TryToIntercept(B,FriendlyFlag.Holder,EnemyFlag.Homebase) );
			}
		}

		if ( GetOrders() == 'Attack' )
		{
			// break off attack only if needed
			if ( B.bPursuingFlag || bSeeFlag || (B.LastRespawnTime > FriendlyFlag.TakenTime) || NearHomeBase(B)
				|| ((WorldInfo.TimeSeconds - FriendlyFlag.TakenTime > UTCTFBase(FriendlyFlag.HomeBase).BaseExitTime) && !NearEnemyBase(B)) )
			{
				B.bPursuingFlag = true;
				B.GoalString = "Go after enemy holding flag rather than attacking";
				if ( FriendlyFlag.Holder != None )
					return TryToIntercept(B,FriendlyFlag.Holder,EnemyFlag.Homebase);
				else if ( GoPickupFlag(B) )
					return true;

			}
			else if ( B.bReachedGatherPoint )
				B.GatherTime = WorldInfo.TimeSeconds - 10;
		}
		else if ( (PlayerController(SquadLeader) == None) && !B.IsSniping()
			&& ((CurrentOrders != 'Defend') || bSeeFlag || B.bPursuingFlag || FlagNearBase()) )
		{
			// FIXME - try to leave one defender at base
			B.bPursuingFlag = true;
			B.GoalString = "Go find my flag";
			if ( FriendlyFlag.Holder != None )
				return TryToIntercept(B,FriendlyFlag.Holder,EnemyFlag.Homebase);
			else if ( GoPickupFlag(B) )
				return true;
		}
	}
	B.bPursuingFlag = false;

	if ( (SquadObjective == EnemyFlag.Homebase) && (B.Enemy != None) && FriendlyFlag.Homebase.BotNearObjective(B)
		&& (WorldInfo.TimeSeconds - B.LastSeenTime < 3) )
	{
		if ( !EnemyFlag.bHome && (EnemyFlag.Holder == None ) && B.LineOfSightTo(EnemyFlag.Position()) )
			return FindPathToObjective(B,EnemyFlag.Position());

		B.SendMessage(None, 'OTHER', B.GetMessageIndex('INCOMING'), 15, 'TEAM');
		B.GoalString = "Intercept incoming enemy!";
		return false;
	}

	if ( EnemyFlag.Holder == None )
	{
		if ( !EnemyFlag.bHome || EnemyFlag.Homebase.BotNearObjective(B) )
		{
			B.GoalString = "Near enemy flag!";
			return FindPathToObjective(B,EnemyFlag.Position());
		}
	}
	else if ( (GetOrders() != 'Defend') && !SquadLeader.IsA('PlayerController') )
	{
		// make flag carrier squad leader if on same squad
		FlagCarrier = EnemyFlag.Holder.Controller;
		if ( (FlagCarrier == None) && (EnemyFlag.Holder.DrivenVehicle != None) )
			FlagCarrier = EnemyFlag.Holder.DrivenVehicle.Controller;

		if ( (SquadLeader != FlagCarrier) && IsOnSquad(FlagCarrier) )
			SetLeader(FlagCarrier);

		if ( (B.Enemy != None) && B.Enemy.LineOfSightTo(FlagCarrier.Pawn) )
		{
			B.GoalString = "Fight enemy threatening flag carrier";
			B.FightEnemy(true,0);
			return true;
		}

		if ( ((FlagCarrier.MoveTarget == FriendlyFlag.HomeBase)
			|| ((FlagCarrier.RouteCache.Length > 1) && (FlagCarrier.RouteCache[1] == FriendlyFlag.HomeBase)))
			&& (B.Enemy != None)
			&& B.LineOfSightTo(FriendlyFlag.HomeBase) )
		{
			B.GoalString = "Fight enemy while waiting for flag carrier to score";
			if ( B.LostContact(7) )
				B.LoseEnemy();
			if ( B.Enemy != None )
			{
				B.FightEnemy(false,0);
				return true;
			}
		}

		if ( (AIController(FlagCarrier) != None) && (FlagCarrier.MoveTarget != None)
			&& (FlagCarrier.InLatentExecution(FlagCarrier.LATENT_MOVETOWARD)) )
		{
			if ( FlagCarrier.RouteCache.length > 1 && FlagCarrier.RouteCache[0] == FlagCarrier.MoveTarget
				&& FlagCarrier.RouteCache[1] != None )
			{
				FlagCarrierTarget = FlagCarrier.RouteCache[1];
			}
			else
			{
				FlagCarrierTarget = FlagCarrier.MoveTarget;
			}
		}
		else
			FlagCarrierTarget = FlagCarrier.Pawn;
		FindPathToObjective(B,FlagCarrierTarget);
		if ( (B.MoveTarget == FlagCarrierTarget) || (B.MoveTarget == FlagCarrier.MoveTarget) )
		{
			if ( B.Enemy != None )
			{
				B.GoalString = "Fight enemy while waiting for flag carrier";
				if ( B.LostContact(7) )
					B.LoseEnemy();
				if ( B.Enemy != None )
				{
					B.FightEnemy(false,0);
					return true;
				}
			}
			if ( !B.bInitLifeMessage )
			{
				B.bInitLifeMessage = true;
				B.SendMessage(EnemyFlag.Holder.PlayerReplicationInfo, 'OTHER', B.GetMessageIndex('GOTYOURBACK'), 10, 'TEAM');
			}
			if ( (B.MoveTarget == FlagCarrier.Pawn)
				&& ((VSize(B.Pawn.Location - FlagCarrier.Pawn.Location) < 250) || (FlagCarrier.Pawn.Acceleration == vect(0,0,0))) )
				return false;
			if ( B.Pawn.ReachedDestination(FlagCarrierTarget) || (FlagCarrier.Pawn.Acceleration == vect(0,0,0))
				|| (FlagCarrier.MoveTarget == FriendlyFlag.HomeBase)
				|| (FlagCarrier.RouteCache.length > 1 && FlagCarrier.RouteCache[1] == FriendlyFlag.HomeBase) )
			{
				B.WanderOrCamp();
				B.GoalString = "Back up the flag carrier!";
				return true;
			}
		}

		B.GoalString = "Find the flag carrier - move to "$B.MoveTarget;
		return ( B.MoveTarget != None );
	}
	return Super.CheckSquadObjectives(B);
}

function EnemyFlagTakenBy(Controller C)
{
	local UTBot M;

	if ( (PlayerController(SquadLeader) == None) && (SquadLeader != C) )
		SetLeader(C);

	if ( (UTBot(C) != None) && (VSize(C.Pawn.Location - EnemyFlag.HomeBase.Location) < 500) )
	{
		if (EnemyFlag.bHome)
		{
			UTBot(C).Pawn.SetAnchor(EnemyFlag.HomeBase);
		}
		SetAlternatePathTo(FriendlyFlag.HomeBase, UTBot(C));
	}

	for	( M=SquadMembers; M!=None; M=M.NextSquadMember )
		if ( (M.MoveTarget == EnemyFlag) || (M.MoveTarget == EnemyFlag.HomeBase) )
			M.MoveTimer = FMin(M.MoveTimer,0.05 + 0.15 * FRand());
}

function bool AllowTaunt(UTBot B)
{
	return ( (FRand() < 0.5) && (PriorityObjective(B) < 1));
}

function bool ShouldDeferTo(Controller C)
{
	if ( C.PlayerReplicationInfo.bHasFlag )
		return true;
	return Super.ShouldDeferTo(C);
}

function byte PriorityObjective(UTBot B)
{
	if ( B.PlayerReplicationInfo.bHasFlag )
	{
		if ( FriendlyFlag.HomeBase.BotNearObjective(B) )
			return 255;
		return 2;
	}

	if ( FriendlyFlag.Holder != None )
		return 1;

	return 0;
}

function float ModifyThreat(float current, Pawn NewThreat, bool bThreatVisible, UTBot B)
{
	if ( (NewThreat.PlayerReplicationInfo != None)
		&& NewThreat.PlayerReplicationInfo.bHasFlag
		&& bThreatVisible )
	{
		if ( (VSize(B.Pawn.Location - NewThreat.Location) < 1500) || (B.Pawn.Weapon != None && UTWeapon(B.Pawn.Weapon).bSniping)
			|| (VSize(NewThreat.Location - EnemyFlag.HomeBase.Location) < 2000) )
			return current + 6;
		else
			return current + 1.5;
	}
	else if ( NewThreat.IsHumanControlled() )
		return current + 0.5;
	else
		return current;
}

defaultproperties
{
	GatherThreshold=+0.6
	MaxSquadSize=3
}
