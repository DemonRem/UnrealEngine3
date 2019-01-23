/**
 * operational AI control for TeamGame
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSquadAI extends ReplicationInfo
	native(AI);

var UTTeamInfo Team;
var Controller SquadLeader;
var UTPlayerReplicationInfo LeaderPRI;
var UTSquadAI NextSquad;	// list of squads on a team
var UTGameObjective SquadObjective;
var int Size;
var UTBot SquadMembers;
var localized string SupportString, DefendString, AttackString, HoldString, FreelanceString;
var localized string SupportStringTrailer;
var name CurrentOrders;
var Pawn Enemies[8];
var int MaxSquadSize;
var bool bFreelance;
var bool bFreelanceAttack;
var bool bFreelanceDefend;
var bool bRoamingSquad;
var bool bAddTransientCosts;

var float FormationSize;

// Alternate path support
var float GatherThreshold;
var NavigationPoint RouteObjective;
var array<NavigationPoint> ObjectiveRouteCache;
/** when generating a new ObjectiveRouteCache for the same objective, the previous is stored here so bots still following it don't get disrupted */
var array<NavigationPoint> PreviousObjectiveRouteCache;
var UTBot PendingSquadRouteMaker;

const NEAROBJECTIVEDIST = 2000.0;

replication
{
	if ( Role == ROLE_Authority )
		LeaderPRI, CurrentOrders, SquadObjective, bFreelance;
}

simulated function byte GetTeamNum()
{
	return (Team != None) ? Team.TeamIndex : 255;
}

function Reset()
{
	local int i;

	Super.Reset();

	NetUpdateTime = WorldInfo.Timeseconds - 1;
	SquadObjective = None;
	for ( i=0; i<8; i++ )
		Enemies[i] = None;
}

function CriticalObjectiveWarning(Pawn NewEnemy)
{
	local UTBot M;

	if ( !ValidEnemy(NewEnemy) )
		return;

	AddEnemy(NewEnemy);

	// reassess squad member enemies
	if ( !MustKeepEnemy(NewEnemy) )
		return;

	for	( M=SquadMembers; M!=None; M=M.NextSquadMember )
	{
		if ( (M.Enemy == None) )
			FindNewEnemyFor(M,false);
	}
}

function bool ShouldSuppressEnemy(UTBot B)
{
	return ( (FRand() < 0.7) && (VSize(B.Enemy.Location - B.FocalPoint) < 350)
			&& (WorldInfo.TimeSeconds - B.LastSeenTime < 4) );
}

function bool AllowDetourTo(UTBot B,NavigationPoint N)
{
	return true;
}

function Destroyed()
{
	if ( Team != None )
	{
		Team.AI.RemoveSquad(self);
	}
	if (SquadObjective != None && SquadObjective.DefenseSquad == self)
	{
		SquadObjective.DefenseSquad = None;
	}

	Super.Destroyed();
}

function bool AllowTranslocationBy(UTBot B)
{
	return true;
}

function bool AllowImpactJumpBy(UTBot B)
{
	return true;
}

function actor SetFacingActor(UTBot B)
{
	return None;
}

function UTVehicle GetLinkVehicle(UTBot B)
{
	local UTVehicle V;

	if (UTVehicleBase(SquadLeader.Pawn) == None)
	{
		return None;
	}
	else
	{
		V = UTVehicle(SquadLeader.Pawn.GetVehicleBase());
		if (V != None && (B.Enemy == None || V.bKeyVehicle))
		{
			return V;
		}
		else
		{
			return None;
		}
	}
}

/* GetFacingRotation()
return the direction the squad is moving towards its objective
*/
function rotator GetFacingRotation()
{
	local rotator Rot;
	// FIXME - use path to objective, rather than just direction

	if ( SquadObjective == None )
		Rot = SquadLeader.Rotation;
	else if ( SquadObjective.DefenderTeamIndex == Team.TeamIndex )
		Rot.Yaw = Rand(65536);
	else if ( SquadLeader.Pawn != None )
		Rot = rotator(SquadObjective.Location - SquadLeader.Pawn.Location);
	else
		Rot.Yaw = Rand(65536);

	Rot.Pitch = 0;
	Rot.Roll = 0;
	return Rot;
}

function actor FormationCenter()
{
	if ( (SquadObjective != None) && (SquadObjective.DefenderTeamIndex == Team.TeamIndex) )
		return SquadObjective;
	return SquadLeader.Pawn;
}

/* LostEnemy()
Bot lost track of enemy.  Change enemy for this bot, clear from list if no one can see it
*/
function bool LostEnemy(UTBot B)
{
	local pawn Lost;
	local bool bFound;
	local UTBot M;

	if ( (B.Enemy.Health <= 0) || (B.Enemy.Controller == None) )
	{
		B.Enemy = None;
		RemoveEnemy(B.Enemy);
		FindNewEnemyFor(B,false);
		return true;
	}

	if ( MustKeepEnemy(B.Enemy) )
		return false;
	Lost = B.Enemy;
	B.Enemy = None;

	for	( M=SquadMembers; M!=None; M=M.NextSquadMember )
		if ( (M != B) && (M.Enemy == Lost) && !M.LostContact(5) )
		{
			bFound = true;
			break;
		}

	if ( bFound )
		B.Enemy = Lost;
	else
	{
		RemoveEnemy(Lost);
		FindNewEnemyFor(B,false);
	}
	return (B.Enemy != Lost);
}

function bool MustKeepEnemy(Pawn E)
{
	return false;
}

/* AddEnemy()
adds an enemy - returns false if enemy was already on list
*/
function bool AddEnemy(Pawn NewEnemy)
{
	local int i;
	local UTBot M;
	local bool bCurrentEnemy;

	for ( i=0; i<ArrayCount(Enemies); i++ )
		if ( Enemies[i] == NewEnemy )
			return false;
	for ( i=0; i<ArrayCount(Enemies); i++ )
		if ( Enemies[i] == None )
		{
			Enemies[i] = NewEnemy;
			return true;
		}
	for ( i=0; i<ArrayCount(Enemies); i++ )
	{
		bCurrentEnemy = false;
		for	( M=SquadMembers; M!=None; M=M.NextSquadMember )
			if ( M.Enemy ==	Enemies[i] )
			{
				bCurrentEnemy = true;
				break;
			}
		if ( !bCurrentEnemy )
		{
			Enemies[i] = NewEnemy;
			return true;
		}
	}
	//`log("FAILED TO ADD ENEMY");
	return false;
}

function bool ValidEnemy(Pawn NewEnemy)
{
	return ( (NewEnemy != None) && !NewEnemy.bAmbientCreature && (NewEnemy.Health > 0) && (NewEnemy.Controller != None)	&& !FriendlyToward(NewEnemy) );
}

function bool SetEnemy( UTBot B, Pawn NewEnemy )
{
	local UTBot M;
	local bool bResult;

	if ( (NewEnemy == B.Enemy) || !ValidEnemy(NewEnemy) )
		return false;

	// add new enemy to enemy list - return if already there
	if ( !AddEnemy(NewEnemy) )
		return false;

	// reassess squad member enemies
	if ( MustKeepEnemy(NewEnemy) )
	{
		for	( M=SquadMembers; M!=None; M=M.NextSquadMember )
		{
			if ( (M != B) && (M.Enemy != NewEnemy) )
				FindNewEnemyFor(M,(M.Enemy !=None) && M.LineOfSightTo(B.Enemy));
		}
	}
	bResult = FindNewEnemyFor(B,(B.Enemy !=None) && B.LineOfSightTo(B.Enemy));
	if ( bResult && (B.Enemy == NewEnemy) )
		B.AcquireTime = WorldInfo.TimeSeconds;
	return bResult;

}

function byte PriorityObjective(UTBot B)
{
	return 0;
}

function bool IsOnSquad(Controller C)
{
	if ( UTBot(C) != None )
		return ( UTBot(C).Squad == self );

	return ( C == SquadLeader );
}

function RemoveEnemy(Pawn E)
{
	local UTBot B;
	local int i;

	for ( i=0; i<ArrayCount(Enemies); i++ )
		if ( Enemies[i] == E )
			Enemies[i] = None;

	if ( WorldInfo.Game.bGameEnded )
		return;

	for	( B=SquadMembers; B!=None; B=B.NextSquadMember )
		if ( B.Enemy == E )
		{
			B.Enemy = None;
			FindNewEnemyFor(B,false);
			if ( (B.Pawn != None) && (B.Enemy == None) && !B.bIgnoreEnemyChange )
			{
				if ( B.InLatentExecution(B.LATENT_MOVETOWARD) && (NavigationPoint(B.MoveTarget) != None)
					&& !B.bPreparingMove )
					B.GotoState('Roaming');
				else
					B.WhatToDoNext();
			}
		}
}

function NotifyKilled(Controller Killer, Controller Killed, pawn KilledPawn)
{
	local UTBot B;
	local bool bPreviousRouteInUse;

	if ( Killed == None )
		return;

	// re-generate alternate route whenever squad leader gets killed
	// unless some bots are still following the previous route that would get clobbered
	if (Killed == SquadLeader)
	{
		for (B = SquadMembers; B != None; B = B.NextSquadMember)
		{
			if (B.bUsePreviousSquadRoute)
			{
				bPreviousRouteInUse = true;
				break;
			}
		}
		if (!bPreviousRouteInUse)
		{
			PendingSquadRouteMaker = UTBot(SquadLeader);
		}
	}

	// if teammate killed, no need to update enemy list
	if ( (Team != None) && (Killed.PlayerReplicationInfo != None)
		&& (Killed.PlayerReplicationInfo.Team == Team) )
	{
		if ( IsOnSquad(Killed) )
		{
			// check if death was witnessed
			for	( B=SquadMembers; B!=None; B=B.NextSquadMember )
				if ( (B != Killed) && B.LineOfSightTo(KilledPawn) )
				{
					B.SendMessage(None, 'OTHER', B.GetMessageIndex('MANDOWN'), 10, 'TEAM');
					break;
				}
		}
		return;
	}
	RemoveEnemy(KilledPawn);

	B = UTBot(Killer);
	if ( (B != None) && (B.Squad == self) && (B.Enemy == None) && (B.Pawn != None) && AllowTaunt(B) )
	{
		B.Focus = KilledPawn;
		B.Celebrate();
	}
}

function bool FindNewEnemyFor(UTBot B, bool bSeeEnemy)
{
	local int i;
	local Pawn BestEnemy, OldEnemy;
	local bool bSeeNew;
	local float BestThreat,NewThreat;

	if ( B.Pawn == None )
		return true;
	if ( (B.Enemy != None) && MustKeepEnemy(B.Enemy) && B.LineOfSightTo(B.Enemy) )
		return false;

	BestEnemy = B.Enemy;
	OldEnemy = B.Enemy;
	if ( BestEnemy != None )
	{
		if ( (BestEnemy.Health < 0) || (BestEnemy.Controller == None) )
		{
			B.Enemy = None;
			BestEnemy = None;
		}
		else
		{
			if ( ModifyThreat(0,BestEnemy,bSeeEnemy,B) > 5 )
				return false;
			BestThreat = AssessThreat(B,BestEnemy,bSeeEnemy);
		}
	}
	for ( i=0; i<ArrayCount(Enemies); i++ )
	{
		if ( (Enemies[i] != None) && (Enemies[i].Health > 0) && (Enemies[i].Controller != None) )
		{
			if ( BestEnemy == None )
			{
				BestEnemy = Enemies[i];
				bSeeEnemy = B.CanSee(Enemies[i]);
				BestThreat = AssessThreat(B,BestEnemy,bSeeEnemy);
			}
			else if ( Enemies[i] != BestEnemy )
			{
				if ( VSize(Enemies[i].Location - B.Pawn.Location) < 1500 )
					bSeeNew = B.LineOfSightTo(Enemies[i]);
				else
					bSeeNew = B.CanSee(Enemies[i]);	// only if looking at him
				NewThreat = AssessThreat(B,Enemies[i],bSeeNew);
				if ( NewThreat > BestThreat )
				{
					BestEnemy = Enemies[i];
					BestThreat = NewThreat;
					bSeeEnemy = bSeeNew;
				}
			}
		}
		else
			Enemies[i] = None;
	}
	B.Enemy = BestEnemy;
	if ( (B.Enemy != OldEnemy) && (B.Enemy != None) )
	{
		B.EnemyChanged(bSeeEnemy);
		return true;
	}
	return false;
}

/* ModifyThreat()
return a modified version of the threat value passed in for a potential enemy
*/
function float ModifyThreat(float current, Pawn NewThreat, bool bThreatVisible, UTBot B)
{
	return current;
}

function bool UnderFire(Pawn NewThreat, UTBot Ignored)
{
	local UTBot B;

	for ( B=SquadMembers; B!=None; B=B.NextSquadMember )
	{
		if ( (B != Ignored) && (B.Pawn != None) && (B.Enemy == NewThreat) && (B.Focus == NewThreat)
			&& (VSize(Ignored.Pawn.Location - NewThreat.Location + NewThreat.Velocity) > VSize(B.Pawn.Location - NewThreat.Location + NewThreat.Velocity))
			&& B.LineOfSightTo(B.Enemy) )
			return true;
	}
	return false;
}

function float AssessThreat( UTBot B, Pawn NewThreat, bool bThreatVisible )
{
	local float ThreatValue, NewStrength, Dist;

	NewStrength = B.RelativeStrength(NewThreat);
	ThreatValue = FClamp(NewStrength, 0, 1);
	Dist = VSize(NewThreat.Location - B.Pawn.Location);
	if ( Dist < 2000 )
	{
		ThreatValue += 0.2;
		if ( Dist < 1500 )
			ThreatValue += 0.2;
		if ( Dist < 1000 )
			ThreatValue += 0.2;
		if ( Dist < 500 )
			ThreatValue += 0.2;
	}

	if ( bThreatVisible )
		ThreatValue += 1;
	if ( (NewThreat != B.Enemy) && (B.Enemy != None) )
	{
		if ( !bThreatVisible )
			ThreatValue -= 5;
		else if ( WorldInfo.TimeSeconds - B.LastSeenTime > 2 )
			ThreatValue += 1;
		if ( Dist > 0.7 * VSize(B.Enemy.Location - B.Pawn.Location) )
			ThreatValue -= 0.25;
		ThreatValue -= 0.2;

		if ( B.IsHunting() && (NewStrength < 0.2)
			&& (WorldInfo.TimeSeconds - FMax(B.LastSeenTime,B.AcquireTime) < 2.5) )
			ThreatValue -= 0.3;
	}

	ThreatValue = ModifyThreat(ThreatValue,NewThreat,bThreatVisible,B);
	if ( NewThreat.IsHumanControlled() )
	{
		// tend to ignore godmode players if in vehicle (E3 hack)
		if ( NewThreat.Controller.bGodMode && (Vehicle(B.Pawn) != None) )
			ThreatValue = -20;
		else
			ThreatValue += 0.25;
	}

	//`log(B.GetHumanReadableName()$" assess threat "$ThreatValue$" for "$NewThreat.GetHumanReadableName());
	return ThreatValue;
}

/*
Return true if squad should defer to C
*/
function bool ShouldDeferTo(Controller C)
{
	return ( C == SquadLeader );
}

/* WaitAtThisPosition()
Called by bot to see if its pawn should stay in this position
returns true if bot has human leader holding near this position
*/
function bool WaitAtThisPosition(Pawn P)
{
	if ( UTBot(P.Controller).NeedWeapon() || (PlayerController(SquadLeader) == None) || (SquadLeader.Pawn == None) )
		return false;
	return CloseToLeader(P);
}

function bool WanderNearLeader(UTBot B)
{
	if ( (Vehicle(B.Pawn) != None) || B.NeedWeapon() || (PlayerController(SquadLeader) == None) || (SquadLeader.Pawn == None) || !CloseToLeader(B.Pawn) )
		return false;
	if ( B.FindInventoryGoal(0.0005) )
		return true;
}

function bool NearFormationCenter(Pawn P)
{
	local Actor Center;

	Center = FormationCenter();
	if ( Center == None )
		return true;
	if ( Center == SquadLeader.Pawn )
	{
		if ( PlayerController(SquadLeader) != None )
			return CloseToLeader(P);
		else
			return false;
	}
	if ( VSize(Center.Location - P.Location) > FormationSize )
		return false;
	return ( P.Controller.LineOfSightTo(Center) );
}

/* CloseToLeader()
Called by bot to see if his pawn is in an acceptable position relative to the squad leader
*/
function bool CloseToLeader(Pawn P)
{
	local float dist;

	if ( (P == None) || (SquadLeader.Pawn == None) )
		return true;

	if ( Vehicle(P) == None )
	{
		if ( (UTVehicle(SquadLeader.Pawn) != None)
			&& UTVehicle(SquadLeader.Pawn).OpenPositionFor(P) )
			return false;
	}
	else if (PlayerController(SquadLeader) != None && Vehicle(SquadLeader.Pawn) == None)
	{
		return false;
	}
	// if in flying vehicle and above leader, give him air support instead of going down where he is
	else if (P.bCanFly && VolumePathNode(P.Anchor) != None && P.Anchor.GetReachSpecTo(SquadLeader.Pawn.Anchor) != None)
	{
		return true;
	}

	if ( (P.GetVehicleBase() == SquadLeader.Pawn)
		|| (SquadLeader.Pawn.GetVehicleBase() == P) )
		return true;

	// for certain games, have bots wait for leader for a while
	if ( (P.Base != None) && (SquadLeader.Pawn.Base != None) && (SquadLeader.Pawn.Base != P.Base) )
		return false;

	dist = VSize(P.Location - SquadLeader.Pawn.Location);
	if ( dist > FormationSize )
		return false;

	// check if leader is moving away
	if ( PhysicsVolume.bWaterVolume )
	{
		if ( VSize(SquadLeader.Pawn.Velocity) > 0 )
			return false;
	}
	else if ( VSize(SquadLeader.Pawn.Velocity) > SquadLeader.Pawn.WalkingPct * SquadLeader.Pawn.GroundSpeed )
		return false;

	return ( P.Controller.LineOfSightTo(SquadLeader.Pawn) );
}

// don't actually merge squads by default, just share objective
function MergeWith(UTSquadAI S)
{
	if ( SquadObjective != S.SquadObjective )
	{
		SquadObjective = S.SquadObjective;
		NetUpdateTime = WorldInfo.Timeseconds - 1;
	}
}

function Initialize(UTTeamInfo T, UTGameObjective O, Controller C)
{
	Team = T;
	SetLeader(C);
	SetObjective(O,false);
}

/** returns whether bots should use an alternate squad route to reach SquadObjective instead of the shortest possible route */
function bool ShouldUseAlternatePaths()
{
	return false;
}

function SetAlternatePathTo(NavigationPoint NewRouteObjective, UTBot RouteMaker)
{
	local UTBot M;

	if (ShouldUseAlternatePaths() && (RouteObjective != NewRouteObjective || PendingSquadRouteMaker != None))
	{
		if (RouteObjective != NewRouteObjective)
		{
			RouteObjective = NewRouteObjective;
			// re-enable squad routes for any bots that had it disabled for the old objective
			for (M = SquadMembers; M != None; M = M.NextSquadMember)
			{
				M.bUsingSquadRoute = true;
			}
		}
		else
		{
			// copy old route to this objective and make sure bots that were using it continue to do so
			PreviousObjectiveRouteCache = ObjectiveRouteCache;
			for (M = SquadMembers; M != None; M = M.NextSquadMember)
			{
				if (M != RouteMaker)
				{
					M.bUsePreviousSquadRoute = true;
				}
			}
		}
		if (FRand() < 0.4)
		{
			RouteMaker.BuildSquadRoute(1);
		}
		else
		{
			RouteMaker.BuildSquadRoute(2 + Rand(3));
		}
	}
}

function bool TryToIntercept(UTBot B, Pawn P, Actor RouteGoal)
{
	if ( (P == B.Enemy) && B.Pawn.RecommendLongRangedAttack() && (P != None) && B.LineOfSightTo(P) )
	{
		B.FightEnemy(false,0);
		return true;
	}

	if ( (P == None) || (NavigationPoint(RouteGoal) == None) || (B.Skill + B.Tactics < 4) )
		return FindPathToObjective(B,P);

	B.MoveTarget = None;
	if ( B.ActorReachable(P) )
	{
		B.GoalString = "almost to "$P;
		if ( B.Enemy != P )
			SetEnemy(B,P);
		if ( B.Enemy != None )
		{
			B.FightEnemy(true,0);
			return true;
		}
		else
		{
			`log("Not attacking intercepted enemy!");
			B.MoveTarget = P;
			B.SetAttractionState();
			return true;
		}
	}
	B.MoveTarget = B.FindPathToIntercept(P,RouteGoal,true);
	if ( B.MoveTarget == None )
	{
		if ( P == B.Enemy )
		{
			B.FailedHuntEnemy = B.Enemy;
			B.FailedHuntTime = WorldInfo.TimeSeconds;
		}
	}
	else if ( B.Pawn.ReachedDestination(B.MoveTarget) )
		return false;
	return B.StartMoveToward(P);
}

/** returns whether the bot shouldn't get any closer to the given objective with the vehicle it's using */
function bool CloseEnoughToObjective(UTBot B, Actor O)
{
	return ( B.Pawn.Location.Z - O.Location.Z < 500.0 &&
		((VSize(B.Pawn.Location - O.Location) < UTVehicle(B.Pawn).ObjectiveGetOutDist && B.LineOfSightTo(O)) || B.ActorReachable(O)) );
}

/** checks if the bot's vehicle is close enough to leave and proceed on foot
 * (assumes the objective cannot be completed while inside a vehicle)
 * @return true if the bot left the vehicle and is continuing on foot, false if it did nothing
 */
function bool LeaveVehicleToReachObjective(UTBot B, Actor O)
{
	local Vehicle OldVehicle;

	if (CloseEnoughToObjective(B, O))
	{
		OldVehicle = Vehicle(B.Pawn);
		B.MoveTarget = None;
		B.DirectionHint = Normal(O.Location - OldVehicle.Location);
		B.NoVehicleGoal = O;
		B.RouteGoal = O;
		B.LeaveVehicle(true);
		return true;
	}

	return false;
}

/** returns whether bot must be on foot to use the given objective (if so, bot will get out of any vehicle just before reaching it) */
function bool MustCompleteOnFoot(Actor O)
{
	local UTGameObjective Objective;

	//@fixme FIXME: should return false if defending the objective (GetOrders() == 'Defend' && O.GetTeamNum() == Team.TeamIndex)?

	if (UTCarriedObject(O) != None)
	{
		return true;
	}
	else
	{
		Objective = UTGameObjective(O);
		return (Objective != None && Objective.GetFlag() != None);
	}
}

/** called when the bot is trying to get to the given objective and has reached one of its parking spots */
function LeaveVehicleAtParkingSpot(UTBot B, Actor O)
{
	local UTVehicle_Deployable DeployableVehicle;

	if (!B.Pawn.bStationary && UTVehicle(B.Pawn) != None && UTVehicle(B.Pawn).bKeyVehicle)
	{
		if ( Team.Size == UTVehicle(B.Pawn).NumPassengers() )
		{
			B.NoVehicleGoal = O;
			B.RouteGoal = O;
			UTVehicle(B.Pawn).bKeyVehicle = false;
			B.LeaveVehicle(true);
		}
		else
		{
			DeployableVehicle = UTVehicle_Deployable(B.Pawn);
			if (DeployableVehicle != None && !DeployableVehicle.IsDeployed())
			{
				DeployableVehicle.ServerToggleDeploy();
			}
			if ( B.Enemy != None )
			{
				B.FightEnemy(false, 0);
			}
			else
			{
				B.GotoState('Defending','Pausing');
			}
		}
	}
	else
	{
		B.NoVehicleGoal = O;
		B.RouteGoal = O;
		B.LeaveVehicle(true);
	}
}

/* FindPathToObjective()
Returns path a bot should use moving toward a base
*/
function bool FindPathToObjective(UTBot B, Actor O)
{
	local Vehicle V;
	local UTGameObjective Objective;
	local int i;

	if ( O == None )
	{
		O = SquadObjective;
		if ( O == None )
		{
			B.GoalString = "No SquadObjective";
			return false;
		}
	}

	Objective = UTGameObjective(O);
	if (Objective != None)
	{
		if ( B.Pawn.bStationary )
		{
			V = B.Pawn.GetVehicleBase();
			if ( V == None )
			{
				V = Vehicle(B.Pawn);
				if (V == None)
				{
					return false;
				}
			}
			if (Objective.ReachedParkingSpot(V))
			{
				LeaveVehicleAtParkingSpot(B, O);
				return true;
			}
			else
			{
				return false;
			}
		}
	 	else if (Vehicle(B.Pawn) != None)
		{
			if (Objective.ReachedParkingSpot(B.Pawn))
			{
				LeaveVehicleAtParkingSpot(B, O);
				return true;
			}
		}
	}

	if (Vehicle(B.Pawn) != None && MustCompleteOnFoot(O))
	{
		if (UTVehicle(B.Pawn) != None && UTVehicle(B.Pawn).bKeyVehicle && UTVehicle(B.Pawn).NumPassengers() < Team.Size )
		{
			if (CloseEnoughToObjective(B, O))
			{
				return false;
			}
		}
		else if (LeaveVehicleToReachObjective(B, O))
		{
			return true;
		}
	}

	// if we should use parking spots, but haven't reached any of them, mark them as endpoints for pathfinding to the objective
	if (Vehicle(B.Pawn) != None && Objective != None)
	{
		for (i = 0; i < Objective.VehicleParkingSpots.length; i++)
		{
			Objective.VehicleParkingSpots[i].bTransientEndPoint = true;
		}
	}

	if ( O != RouteObjective )
		return B.SetRouteToGoal(O);

	B.MoveTarget = B.FindPathToSquadRoute(B.Pawn.bCanPickupInventory && (Vehicle(B.Pawn) == None));
	return B.StartMoveToward(O);
}

event SetLeader(Controller C)
{
	SquadLeader = C;
	if ( LeaderPRI != C.PlayerReplicationInfo )
	{
		LeaderPRI = UTPlayerReplicationInfo(C.PlayerReplicationInfo);
		NetUpdateTime = WorldInfo.Timeseconds - 1;
	}
	if ( UTBot(C) != None )
		AddBot(UTBot(C));
}

function RemovePlayer(PlayerController P)
{
	local UTGameObjective NewObjective;

	if ( SquadLeader != P )
		return;
	if ( SquadMembers == None )
	{
		destroy();
		return;
	}

	NewObjective = Team.AI.GetPriorityAttackObjectiveFor(self, none);
	if ( NewObjective != SquadObjective )
	{
		SquadObjective = NewObjective;
		NetUpdateTime = WorldInfo.Timeseconds - 1;
	}
	PickNewLeader();
}

function RemoveBot(UTBot B)
{
	local UTBot Prev;

	if ( B.Squad != self )
		return;

	B.Squad = None;
	Size --;

	if ( SquadMembers == B )
	{
		SquadMembers = B.NextSquadMember;
		if ( SquadMembers == None )
		{
			destroy();
			return;
		}
	}
	else
	{
		for ( Prev=SquadMembers; Prev!=None; Prev=Prev.NextSquadMember )
			if ( Prev.NextSquadMember == B )
			{
				Prev.NextSquadMember = B.NextSquadMember;
				break;
			}
	}
	if ( SquadLeader == B )
		PickNewLeader();
}

function AddBot(UTBot B)
{
	if ( B.Squad == self )
		return;
	if ( B.Squad != None )
		B.Squad.RemoveBot(B);

	Size++;

	B.NextSquadMember = SquadMembers;
	SquadMembers = B;
	B.Squad = self;
	if ( UTPlayerReplicationInfo(B.PlayerReplicationInfo) != None )
	{
		UTPlayerReplicationInfo(B.PlayerReplicationInfo).Squad = self;
	}
}

function SetDefenseScriptFor(UTBot B)
{
	local UTDefensePoint OldPoint, S;
	local bool bPrioritizeSameGroup, bAutoPointsInUse, bAllDefenseGroupsCovered;
	local int NumChecked;
	local UTBot OtherBot;
	local array<name> DefendedGroups;

	if ( (B.DefensePoint != None) && (SquadObjective == B.DefensePoint.DefendedObjective) && (!B.DefensePoint.bOnlyOnFoot || Vehicle(B.Pawn) == None) )
	{
		// don't change defensepoints if fighting, recently fought, or if haven't reached it yet
		if (B.Enemy != None || WorldInfo.TimeSeconds - FMax(B.LastSeenTime, B.AcquireTime) < 5.0 || !B.Pawn.ReachedDestination(B.DefensePoint.GetMoveTarget()))
		{
			return;
		}
	}

	// determine what DefenseGroups are already being defended by other bots on this team
	foreach WorldInfo.AllControllers(class'UTBot', OtherBot)
	{
		if (OtherBot.PlayerReplicationInfo != None && OtherBot.PlayerReplicationInfo.Team == Team && OtherBot != B)
		{
			if (OtherBot.DefensePoint != None && DefendedGroups.Find(OtherBot.DefensePoint.DefenseGroup) == INDEX_NONE)
			{
				DefendedGroups[DefendedGroups.length] = OtherBot.DefensePoint.DefenseGroup;
			}
			else if (OtherBot.DefensivePosition != None && OtherBot.IsDefending())
			{
				// defending using automatic point
				bAutoPointsInUse = true;
			}
		}
	}

	//`Log("SET NEW DEFENSEPOINT FOR "$B.PlayerReplicationInfo.PlayerName);
	if ( B.DefensePoint != None )
	{
		OldPoint = B.DefensePoint;
		B.DefensePoint.FreePoint();
		bPrioritizeSameGroup = (FRand() < 0.85);
	}

	NumChecked = 1;
	bAllDefenseGroupsCovered = true;
	for (S = SquadObjective.DefensePoints; S != None; S = S.NextDefensePoint)
	{
		if (S != OldPoint && DefendedGroups.Find(S.DefenseGroup) == INDEX_NONE)
		{
			bAllDefenseGroupsCovered = false;
			if (S.HigherPriorityThan(B.DefensePoint, B, bAutoPointsInUse, bPrioritizeSameGroup, NumChecked))
			{
				B.DefensePoint = S;
			}
		}
	}
	if (bAllDefenseGroupsCovered && bAutoPointsInUse)
	{
		// try again, ignoring DefenseGroups
		NumChecked = 1;
		for (S = SquadObjective.DefensePoints; S != None; S = S.NextDefensePoint)
		{
			if (S != OldPoint && S.HigherPriorityThan(B.DefensePoint, B, bAutoPointsInUse, bPrioritizeSameGroup, NumChecked))
			{
				B.DefensePoint = S;
			}
		}
	}

	if (B.DefensePoint != None)
	{
		B.DefensePoint.CurrentUser = B;
	}
}

function SetObjective(UTGameObjective O, bool bForceUpdate)
{
	local UTBot M;

	//`Log(SquadLeader.PlayerReplicationInfo.PlayerName$" SET OBJECTIVE"@O@"Forced update"@bForceUpdate);
	if ( SquadObjective == O )
	{
		if ( SquadObjective == None )
			return;
		if ( (SquadObjective.DefenderTeamIndex == Team.TeamIndex) && (SquadObjective.DefenseSquad == None) )
			SquadObjective.DefenseSquad = self;
		if ( !bForceUpdate )
			return;
	}
	else
	{
		if ( (SquadObjective != None) && (SquadObjective.DefenderTeamIndex == Team.TeamIndex) && (SquadObjective.DefenseSquad == self) )
			SquadObjective.DefenseSquad = None;
		NetUpdateTime = WorldInfo.Timeseconds - 1;
		SquadObjective = O;
		if ( SquadObjective != None )
		{
			if ( (SquadObjective.DefenderTeamIndex == Team.TeamIndex) && (SquadObjective.DefenseSquad == None) )
					SquadObjective.DefenseSquad = self;
			RouteObjective = None;
			if ( UTBot(SquadLeader) != None )
				SetAlternatePathTo(SquadObjective, UTBot(SquadLeader));
		}
	}
	for	( M=SquadMembers; M!=None; M=M.NextSquadMember )
		if ( M.Pawn != None )
			Retask(M);
}

function Retask(UTBot B)
{
	if ( (Vehicle(B.Pawn) != None) && B.Pawn.bStationary && (B.Pawn.GetVehicleBase() == None) )
	{
		//get out of turrets when objective changes
		Vehicle(B.Pawn).DriverLeave(false); // should be OK to call directly here
		B.bPreparingMove = false;
		B.MoveTarget = None; //so bot won't immediately get back in
		B.RouteGoal = None;
		B.WhatToDoNext();
	}
	else if ( B.InLatentExecution(B.LATENT_MOVETOWARD) )
	{
		if ( B.bPreparingMove )
		{
			B.bPreparingMove = false;
			B.WhatToDoNext();
		}
		/* //@todo steve
		else if ( (B.Pawn.Physics == PHYS_Falling) && (JumpSpot(B.Movetarget) != None) )
			return;
		*/
		else if ( B.MoveTimer > 0.3 )
			B.MoveTimer = 0.05 + 0.15 * FRand();
	}
	else
	{
		B.RetaskTime = WorldInfo.TimeSeconds + 0.05 + 0.15 * FRand();
		GotoState('Retasking');
	}
}

State Retasking
{
	function Tick(float DeltaTime)
	{
		local UTBot M;
		local bool bStillTicking;

		for	( M=SquadMembers; M!=None; M=M.NextSquadMember )
			if ( (M.Pawn != None) && (M.RetaskTime > 0) )
			{
				if ( WorldInfo.TimeSeconds > M.RetaskTime )
					M.WhatToDoNext();
				else
					bStillTicking = true;
			}

		if ( !bStillTicking )
			GotoState('');
	}
}

function name GetOrders()
{
	local name NewOrders;

	if ( PlayerController(SquadLeader) != None )
		NewOrders = 'Human';
	else if ( bFreelance && !bFreelanceAttack && !bFreelanceDefend )
		NewOrders = 'Freelance';
	else if ( (SquadObjective != None) && (SquadObjective.DefenderTeamIndex == Team.TeamIndex) )
		NewOrders = 'Defend';
	else
		NewOrders = 'Attack';
	if ( NewOrders != CurrentOrders )
	{
		NetUpdateTime = WorldInfo.Timeseconds - 1;
		CurrentOrders = NewOrders;
	}
	return CurrentOrders;
}

simulated function String GetOrderStringFor(UTPlayerReplicationInfo PRI)
{
	if ( (LeaderPRI != None) && !LeaderPRI.bBot )
	{
		// FIXME - holding replication
		if ( PRI.bHolding )
			return HoldString;

		return SupportString@LeaderPRI.PlayerName@SupportStringTrailer;
	}
	if ( bFreelance || (SquadObjective == None) )
		return FreelanceString;
	else
	{
		GetOrders();
		if ( CurrentOrders == 'defend' )
			return DefendString@SquadObjective.GetHumanReadableName();
		if ( CurrentOrders == 'attack' )
			return AttackString@SquadObjective.GetHumanReadableName();
	}
	return string(CurrentOrders);
}

simulated function String GetShortOrderStringFor(UTPlayerReplicationInfo PRI)
{
	if ( (LeaderPRI != None) && !LeaderPRI.bBot )
	{
		// FIXME - holding replication
		if ( PRI.bHolding )
			return HoldString;

		return SupportString;
	}
	if ( bFreelance || (SquadObjective == None) )
		return FreelanceString;
	else
	{
		GetOrders();
		if ( CurrentOrders == 'defend' )
			return DefendString;
		if ( CurrentOrders == 'attack' )
			return AttackString;
	}
	return string(CurrentOrders);
}

function int GetSize()
{
	if ( PlayerController(SquadLeader) != None )
		return Size + 1; // add 1 for leader
	else
		return Size;
}

function PickNewLeader()
{
	local UTBot B;

	// FIXME - pick best based on distance to objective

	// pick a leader that isn't out of the game or in a vehicle turret
	for ( B=SquadMembers; B!=None; B=B.NextSquadMember )
		if ( !B.PlayerReplicationInfo.bOutOfLives && (B.Pawn == None || !B.Pawn.bStationary || B.Pawn.GetVehicleBase() == None) )
			break;

	if ( B == None )
	{
		for ( B=SquadMembers; B!=None; B=B.NextSquadMember )
			if ( !B.PlayerReplicationInfo.bOutOfLives )
				break;
	}

	if ( SquadLeader != B )
	{
		SquadLeader = B;
		if ( SquadLeader == None )
			LeaderPRI = None;
		else
			LeaderPRI = UTPlayerReplicationInfo(SquadLeader.PlayerReplicationInfo);
		NetUpdateTime = WorldInfo.Timeseconds - 1;
	}
}

function bool TellBotToFollow(UTBot B, Controller C)
{
	local Pawn Leader;
	local UTGameObjective O, Best;
	local float NewDist, BestDist;

	if ( (C == None) || C.bDeleteMe )
	{
		PickNewLeader();
		C = SquadLeader;
	}

	if ( B == C )
		return false;

	B.GoalString = "Follow Leader";
	Leader = C.Pawn;
	if ( Leader == None )
		return false;

	if ( CloseToLeader(B.Pawn) )
	{
		if ( !B.bInitLifeMessage )
		{
			B.bInitLifeMessage = true;
		  	B.SendMessage(SquadLeader.PlayerReplicationInfo, 'OTHER', B.GetMessageIndex('GOTYOURBACK'), 10, 'TEAM');
		}
		if ( B.Enemy == None )
		{
			// look for destroyable objective
			for ( O=Team.AI.Objectives; O!=None; O=O.NextObjective )
			{
				if ( !O.IsDisabled() && O.Shootable()
					&& ((Best == None) || (Best.DefensePriority < O.DefensePriority)) )
				{
					NewDist = VSize(B.Pawn.Location - O.Location);
					if ( ((Best == None) || (NewDist < BestDist)) && B.LineOfSightTo(O) )
					{
						Best = O;
						BestDist = NewDist;
					}
				}
			}
			if ( Best != None )
			{
				if (Best.DefenderTeamIndex != Team.TeamIndex)
				{
					if (Best.TellBotHowToDisable(B))
						return true;
				}
				else if ( (BestDist < 1600) && Best.TellBotHowToHeal(B) )
				{
					return true;
				}
			}

			if ( B.FindInventoryGoal(0.0004) )
			{
				B.SetAttractionState();
				return true;
			}
			B.WanderOrCamp();
			return true;
		}
		else if ( (UTWeapon(B.Pawn.Weapon) != None) && UTWeapon(B.Pawn.Weapon).FocusOnLeader(false) )
		{
			B.FightEnemy(false,0);
			return true;
		}
		return false;
	}
	else if ( B.SetRouteToGoal(Leader) )
		return true;
	else
	{
		B.GoalString = "Can't reach leader";
		return false;
	}
}

function bool AllowTaunt(UTBot B)
{
	return ( FRand() < 0.5 );
}

function AddTransientCosts(UTBot B, float f)
{
	local UTBot S;

	for ( S=SquadMembers; S!=None; S=S.NextSquadMember )
		if ( (S != B) && (NavigationPoint(S.MoveTarget) != None) && S.InLatentExecution(S.LATENT_MOVETOWARD) )
			NavigationPoint(S.MoveTarget).TransientCost = 1000 * f;
}

function bool AssignSquadResponsibility(UTBot B)
{
	// set up route cache if pending
	if ( PendingSquadRouteMaker == B )
	{
		SetAlternatePathTo(RouteObjective, B);
	}

	// set new defense script
	if ( (GetOrders() == 'Defend') && !B.Pawn.bStationary )
		SetDefenseScriptFor(B);
	else if ( (B.DefensePoint != None) && (UTHoldSpot(B.DefensePoint) == None) )
		B.FreePoint();

	if ( bAddTransientCosts )
		AddTransientCosts(B,1);
	// check for major game objective responsibility
	if ( CheckSquadObjectives(B) )
		return true;

	if ( B.Enemy == None && !B.Pawn.bStationary )
	{
		// suggest inventory hunt
		// FIXME - don't load up on unnecessary ammo in DM
		if ( B.FindInventoryGoal(0) )
		{
			B.SetAttractionState();
			return true;
		}

		// roam around WorldInfo.?
		if ( ((B == SquadLeader) && bRoamingSquad) || (GetOrders() == 'Freelance') )
			return B.FindRoamDest();
	}
	return false;
}

function float MaxVehicleDist(Pawn P)
{
	if (SquadObjective != None && MustCompleteOnFoot(SquadObjective))
	{
		return FMin(3000.0, VSize(SquadObjective.Location - P.Location));
	}
	else
	{
		return 3000.0;
	}
}

function bool NeverBail(Pawn P)
{
	return (UTVehicle(P) != None && UTVehicle(P).bKeyVehicle);
}

function BotEnteredVehicle(UTBot B)
{
	if ( (PlayerController(SquadLeader) != None) )
	{
		if ( (SquadLeader.Pawn != None) && (B.Pawn.GetVehicleBase() == SquadLeader.Pawn) )
			B.SendMessage(None, 'OTHER', B.GetMessageIndex('INPOSITION'), 10, 'TEAM');
	}
	else if (B.Pawn.bStationary && B.Pawn.GetVehicleBase() != None)
		PickNewLeader();
}

/** returns whether the bot should be considered as moving towards SquadObjective if its goal is the given Actor */
function bool IsOnPathToSquadObjective(Actor Goal)
{
	local NavigationPoint Nav;

	if (Goal == SquadObjective)
	{
		return true;
	}
	else
	{
		Nav = NavigationPoint(Goal);
		if (Nav != None)
		{
			if ( SquadObjective.VehicleParkingSpots.Find(Nav) != INDEX_NONE ||
				(SquadObjective.Shootable() && SquadObjective.ShootSpots.Find(Nav) != INDEX_NONE) ||
				(SquadObjective == RouteObjective && ObjectiveRouteCache.Find(Nav) != INDEX_NONE) )
			{
				return true;
			}
		}
	}

	return false;
}

/** used with bot's CustomAction interface to tell it to get on the hoverboard */
function bool GetOnHoverboard(UTBot B)
{
	local UTPawn P;

	P = UTPawn(B.Pawn);
	if (P != None)
	{
		P.ServerHoverboard();
	}

	return true;
}

/** used with bot's CustomAction interface to enter and exit a vehicle in the same action */
function bool EnterAndExitVehicle(UTBot B)
{
	local UTVehicle V;

	V = UTVehicle(B.RouteGoal);
	if (V != None && V.TryToDrive(B.Pawn))
	{
		Vehicle(B.Pawn).DriverLeave(false);
	}

	return true;
}

/** tells the given bot to attempt to move toward and enter the given vehicle */
function bool GotoVehicle(UTVehicle SquadVehicle, UTBot B)
{
	local Actor BestEntry, BestPath;

	BestEntry = SquadVehicle.GetMoveTargetFor(B.Pawn);

	if ( B.Pawn.ReachedDestination(BestEntry) )
	{
		B.EnterVehicle(SquadVehicle);
		return true;
	}

	if ( B.ActorReachable(BestEntry) )
	{
		B.RouteGoal = SquadVehicle;
		B.MoveTarget = BestEntry;
		SquadVehicle.SetReservation(B);
		B.GoalString = "Go to vehicle 1 "$BestEntry;
		B.SetAttractionState();
		return true;
	}

	BestPath = B.FindPathToward(BestEntry,B.Pawn.bCanPickupInventory);
	if ( BestPath != None )
	{
		B.RouteGoal = SquadVehicle;
		SquadVehicle.SetReservation(B);
		B.MoveTarget = BestPath;
		B.GoalString = "Go to vehicle 2 "$BestPath;
		B.SetAttractionState();
		return true;
	}

	if ( (VSize(BestEntry.Location - B.Pawn.Location) < 1200)
		&& B.LineOfSightTo(BestEntry) )
	{
		B.RouteGoal = SquadVehicle;
		SquadVehicle.SetReservation(B);
		B.MoveTarget = BestEntry;
		B.GoalString = "Go to vehicle 3 "$BestEntry;
		B.SetAttractionState();
		return true;
	}

	return false;
}

/* go to squad vehicle (driven by squad leader - or squad leader objective), if nearby,
else try to find vehicle
*/
function bool CheckVehicle(UTBot B)
{
	local UTVehicle V, SquadVehicle;
	local Vehicle BotVehicle;
	local float NewDist, BestDist, NewRating, BestRating, BaseRadius;
	local UTBot S;
	local PlayerController PC;
	local bool bSkip, bVisible;
	local UTPawn P;

	if ( UTGame(WorldInfo.Game).bDemoMode )
		return false;

	if (B.NoVehicleGoal != None)
	{
		// if NoVehicleGoal is a flag, use its Anchor instead as RouteGoal is usually a NavigationPoint
		if (B.NoVehicleGoal.IsA('UTCarriedObject'))
		{
			B.NoVehicleGoal = UTCarriedObject(B.NoVehicleGoal).LastAnchor;
		}
		// if the bot's current goal is the NoVehicleGoal
		// or NoVehicleGoal is our SquadObjective and the bot's current goal is towards our SquadObjective
		if ( Vehicle(B.Pawn) == None &&
			(B.RouteGoal == B.NoVehicleGoal || (B.NoVehicleGoal == SquadObjective && IsOnPathToSquadObjective(B.RouteGoal))) )
		{
			// don't use a vehicle to get to this goal
			return false;
		}
		else
		{
			B.NoVehicleGoal = None;
		}
	}

	if ( (Vehicle(B.Pawn) == None) && (Vehicle(B.RouteGoal) != None) && (NavigationPoint(B.Movetarget) != None) )
	{
		if ( VSize(B.Pawn.Location - B.RouteGoal.Location) < B.Pawn.GetCollisionRadius() + Vehicle(B.RouteGoal).GetCollisionRadius() + B.Pawn.VehicleCheckRadius * 1.5 )
			B.MoveTarget = B.RouteGoal;
	}
	V = UTVehicle(B.MoveTarget);
	if (Vehicle(B.Pawn) == None && V != None)
	{
		if (V.PlayerStartTime > WorldInfo.TimeSeconds)
		{
			ForEach LocalPlayerControllers(class'PlayerController', PC)
			{
				if ( (PC.PlayerReplicationInfo.Team == Team) && (PC.Pawn != None) && (Vehicle(PC.Pawn) == None) )
				{
					bSkip = true;
					break;
				}
			}
		}
		if (!bSkip)
		{
			//consider healing vehicle before getting in
			if (V.Health < V.Default.Health && (B.Enemy == None || !B.LineOfSightTo(B.Enemy)) && B.CanAttack(V))
			{
				//get in and out to steal vehicle for team so bot can heal it
				if ( !WorldInfo.GRI.OnSameTeam(V,self) )
				{
					B.RouteGoal = V;
					B.PerformCustomAction(EnterAndExitVehicle);
					return true;
				}

				if (V.TeamLink(Team.TeamIndex))
				{
					if (UTWeapon(B.Pawn.Weapon) != None && UTWeapon(B.Pawn.Weapon).CanHeal(V))
					{
						B.GoalString = "Heal "$V;
						B.DoRangedAttackOn(V);
						return true;
					}
					else
					{
						B.SwitchToBestWeapon();
						if (UTWeapon(B.Pawn.InvManager.PendingWeapon) != None && UTWeapon(B.Pawn.InvManager.PendingWeapon).CanHeal(V))
						{
							B.GoalString = "Heal "$V;
							B.DoRangedAttackOn(V);
							return true;
						}
					}
				}
			}
			if ( V.GetVehicleBase() != None )
				BaseRadius = V.GetVehicleBase().GetCollisionRadius();
			else
				BaseRadius = V.GetCollisionRadius();
			if ( VSize(B.Pawn.Location - V.Location) < B.Pawn.GetCollisionRadius() + BaseRadius + B.Pawn.VehicleCheckRadius )
			{
				B.EnterVehicle(V);
				return true;
			}
		}
	}
	if ( B.LastSearchTime == WorldInfo.TimeSeconds )
		return false;

	BotVehicle = Vehicle(B.Pawn);
	if (BotVehicle != None)
	{
		if (!NeverBail(BotVehicle))
		{
			if (BotVehicle.StuckCount > 3)
			{
				// vehicle is stuck
				if (BotVehicle.IsA('UTVehicle'))
				{
					UTVehicle(BotVehicle).VehicleLostTime = WorldInfo.TimeSeconds + 20;
				}
				B.LeaveVehicle(true);
				return true;
			}
			else if ((BotVehicle.Health < BotVehicle.Default.Health * 0.125) && !BotVehicle.bStationary && (B.Skill + B.Tactics > 4.0 + 7.0 * FRand()))
			{
				//about to blow up, bail
				if (BotVehicle.IsA('UTVehicle'))
				{
					UTVehicle(BotVehicle).VehicleLostTime = WorldInfo.TimeSeconds + 10;
				}
				B.LeaveVehicle(true);
				return true;
			}
			else
			{
				V = UTVehicle(BotVehicle);
				if (V == None)
				{
					V = UTVehicle(BotVehicle.GetVehicleBase());
				}
				if (V != None && V.bShouldLeaveForCombat && B.LineOfSightTo(B.Enemy))
				{
					B.LeaveVehicle(true);
					return true;
				}
			}
		}
		V = UTVehicle(BotVehicle.GetVehicleBase());
		if ( V != None )
		{
			// if in passenger seat of a multi-person vehicle, get out if no driver
			if ( V.Driver == None && (SquadLeader == B || SquadLeader.RouteGoal == None
			     || (SquadLeader.RouteGoal != V)) )
			{
				B.LeaveVehicle(true);
				return true;
			}
		}

/* //@todo steve
		if (!B.Pawn.bStationary && PriorityObjective(B) == 0)
			return CheckSpecialVehicleObjectives(B);
*/
		return false;
	}
/* //@todo steve
	if (SpecialVehicleObjective(B.RouteGoal) != None && CheckSpecialVehicleObjectives(B))
		return true;
*/
	if ( (UTVehicle(SquadLeader.Pawn) != None) && (VSize(SquadLeader.Pawn.Location - B.Pawn.Location) < 4000) )
	{
		SquadVehicle = UTVehicle(SquadLeader.Pawn).OpenPositionFor(B.Pawn) ? UTVehicle(SquadLeader.Pawn) : None;
	}
	else if ( PlayerController(SquadLeader) != None )
		return false;

	BestDist = MaxVehicleDist(B.Pawn);
	if ( SquadVehicle == None )
	{
		for ( S=SquadMembers; S!=None; S=S.NextSquadMember )
		{
			if ( (UTVehicle(S.Pawn) != None) && (VSize(S.Pawn.Location - B.Pawn.Location) < BestDist) && UTVehicle(S.Pawn).OpenPositionFor(B.Pawn) )
			{
				SquadVehicle = UTVehicle(S.Pawn);
				break;
			}
		}
	}

	if ( SquadVehicle == None && UTVehicle(SquadLeader.RouteGoal) != None && !UTVehicle(SquadLeader.RouteGoal).Occupied()
	     && VSize(SquadLeader.RouteGoal.Location - B.Pawn.Location) < BestDist && UTVehicle(SquadLeader.RouteGoal).OpenPositionFor(B.Pawn) )
	{
		SquadVehicle = UTVehicle(SquadLeader.RouteGoal);
	}

	if ( SquadVehicle == None && UTVehicle(B.RouteGoal) != None && !UTVehicle(B.RouteGoal).Occupied() &&
		VSize(B.RouteGoal.Location - B.Pawn.Location) < BestDist )
	{
		SquadVehicle = UTVehicle(B.RouteGoal);
	}

	if ( (SquadVehicle != None) && (SquadVehicle.PlayerStartTime > WorldInfo.TimeSeconds) )
	{
		ForEach LocalPlayerControllers(class'PlayerController', PC)
		{
			if ( (PC.PlayerReplicationInfo.Team == Team) && (PC.Pawn != None) && (Vehicle(PC.Pawn) == None) )
			{
				SquadVehicle = None;
				break;
			}
		}
	}

	if ( SquadVehicle == None )
	{
		// look for nearby vehicle
		GetOrders();
		for ( V=UTGame(WorldInfo.Game).VehicleList; V!=None; V=V.NextVehicle )
		{
			NewDist = VSize(B.Pawn.Location - V.Location);
			if (NewDist < BestDist)
			{
				bVisible = V.FastTrace(V.Location, B.Pawn.Location + B.Pawn.GetCollisionHeight() * vect(0,0,1));
				if (!bVisible)
				{
					NewDist *= 1.5;
				}
				if (NewDist < BestDist)
				{
					NewRating = VehicleDesireability(V, B);
					if (NewRating > 0.0)
					{
						NewRating += BestDist / NewDist * 0.01;
						if (NewRating > BestRating && (V.bTeamLocked || V.bKeyVehicle || bVisible))
						{
							SquadVehicle = V;
							BestRating = NewRating;
						}
					}
				}
			}
		}
	}


	if (SquadVehicle == None)
	{
		P = UTPawn(B.Pawn);
		// if no vehicle nearby, objective is far away, and no visible enemies, use hoverboard
		if ( P != None && P.bHasHoverboard && !B.NeedWeapon() && B.LostContact(3.0) && !B.Pawn.IsFiring() && !B.IsShootingObjective() &&
			( !B.IsDefending() || ( (B.DefensePoint == None || !P.ReachedDestination(B.DefensePoint)) &&
						(B.DefensivePosition == None || P.ReachedDestination(B.DefensivePosition)) ) ) &&
			(SquadObjective == None || (VSize(SquadObjective.Location - P.Location) > 1200.0 && !B.ActorReachable(SquadObjective))) &&
			( P.Anchor == None || ( P.HoverboardClass.default.CylinderComponent.CollisionRadius <= P.Anchor.MaxPathSize.Radius &&
						P.HoverboardClass.default.CylinderComponent.CollisionHeight <= P.Anchor.MaxPathSize.Height ) ) )
		{
			// can't start up hoverboard during async work - call it delayed instead
			B.GoalString = "Get on hoverboard";
			B.PerformCustomAction(GetOnHoverboard);
			return true;
		}
		return false;
	}

	return GotoVehicle(SquadVehicle, B);
}

//return a value indicating how useful this vehicle is to the bot
function float VehicleDesireability(UTVehicle V, UTBot B)
{
	local float result;

	// if bot has the flag and the vehicle can't carry flags, ignore it
	if ( B.PlayerReplicationInfo.bHasFlag && !V.bCanCarryFlag )
	{
		return 0;
	}
	// if we're attacking and vehicle is meant for defenders (or vice versa), ignore it
	if ((CurrentOrders == 'Defend') != V.bDefensive)
	{
		return 0;
	}
	// if vehicle is low on health and there's an enemy nearby (so don't have time to heal it), ignore it
	if (V.Health < V.Default.Health * 0.125 && B.Enemy != None && B.LineOfSightTo(B.Enemy))
	{
		return 0;
	}
	// otherwise, let vehicle rate itself
	result = V.BotDesireability(self, (Team != None) ? Team.TeamIndex : 255, SquadObjective);
	if ( V.SpokenFor(B) )
	{
		return result * V.ReservationCostMultiplier(B.Pawn);
	}
	else
	{
		return result;
	}
}

/* //@todo steve
function bool CheckSpecialVehicleObjectives(UTBot B)
{
	local UTGame G;
	local SpecialVehicleObjective O, Best;

	G = UTGame(WorldInfo.Game);
	if (G == None)
		return false;

	Best = SpecialVehicleObjective(B.RouteGoal);
	if (Best != None)
	{
		if (Vehicle(B.Pawn) == None)
		{
			if ( Best.bEnabled && !B.Pawn.ReachedDestination(Best.AssociatedActor)
			     && B.FindBestPathToward(Best.AssociatedActor, false, true) )
			{
				B.RouteGoal = Best;
				B.GoalString = "Reached SpecialVehicleObjective, now heading for "$B.RouteGoal;
				B.SetAttractionState();
				return true;
			}
			else
				return false;
		}
		else if (!Best.IsAccessibleTo(B.Pawn))
		{
			if (Team != None && Team.TeamIndex < 4 && Best.TeamOwner[Team.TeamIndex] == B.Pawn)
				Best.TeamOwner[Team.TeamIndex] = None;
			Best = None;
		}
	}

	if (Best == None)
		for (O = G.SpecialVehicleObjectives; O != None; O = O.NextSpecialVehicleObjective)
			if ( O.IsAccessibleTo(B.Pawn) && (Team == None || Team.TeamIndex >= 4 || O.TeamOwner[Team.TeamIndex] == None)
			     && (Best == None || FRand() < 0.5) )
				Best = O;

	if (Best != None)
	{
		if (B.Pawn.ReachedDestination(Best))
		{
			Vehicle(B.Pawn).DriverLeave(false);
			if (B.FindBestPathToward(Best.AssociatedActor, false, true))
			{
				B.RouteGoal = Best;
				B.GoalString = "Reached SpecialVehicleObjective, now heading for "$Best.AssociatedActor;
				B.SetAttractionState();
				return true;
			}
		}

		if (B.ActorReachable(Best))
		{
			B.RouteGoal = Best;
			B.MoveTarget = Best;
			B.GoalString = "Head for reachable SpecialVehicleObjective "$Best;
			B.SetAttractionState();
			return true;
		}
		B.MoveTarget = B.FindPathToward(Best, B.Pawn.bCanPickupInventory);
		if (B.MoveTarget != None)
		{
			B.RouteGoal = Best;
			B.GoalString = "Head for SpecialVehicleObjective "$Best;
			B.SetAttractionState();
			return true;
		}
	}

	return false;
}
*/
function bool OverrideFollowPlayer(UTBot B)
{
	local UTGameObjective PickedObjective;

	PickedObjective = Team.AI.GetPriorityAttackObjectiveFor(self, B);
	if ( (PickedObjective == None) )
		return false;
	if ( PickedObjective.BotNearObjective(B) )
	{
		if ( PickedObjective.DefenderTeamIndex == Team.TeamIndex )
		{
			return PickedObjective.TellBotHowToHeal(B);
		}
		else
			return PickedObjective.TellBotHowToDisable(B);
	}
	if ( PickedObjective.DefenderTeamIndex == Team.TeamIndex )
		return false;
	if ( PickedObjective.Shootable() && B.LineOfSightTo(PickedObjective) )
		return PickedObjective.TellBotHowToDisable(B);
	return false;
}

function bool CheckSquadObjectives(UTBot B)
{
	local Actor DesiredPosition;
	local bool bInPosition, bMovingToSuperPickup;
	local float SuperDist;

	if ( (UTHoldSpot(B.DefensePoint) == None) && CheckVehicle(B) )
		return true;
	if ( B.NeedWeapon() && B.FindInventoryGoal(0) )
	{
		B.GoalString = "Need weapon or ammo";
		B.SetAttractionState();
		return true;
	}

	if ( (PlayerController(SquadLeader) != None) && (SquadLeader.Pawn != None) )
	{
		if ( UTHoldSpot(B.DefensePoint) == None )
		{
			// attack objective if close by
			if ( OverrideFollowPlayer(B) )
				return true;

			// follow human leader
			return TellBotToFollow(B,SquadLeader);
		}
		// hold position as ordered (position specified by DefensePoint)
	}
	if ( ShouldDestroyTranslocator(B) )
		return true;

	if ( B.Pawn.bStationary && (Vehicle(B.Pawn) != None) )
	{
		if ( UTHoldSpot(B.DefensePoint) != None )
		{
			if ( UTHoldSpot(B.DefensePoint).HoldVehicle != B.Pawn )
			{
				B.LeaveVehicle(true);
				return true;
			}
		}
		else if (UTVehicle(B.Pawn) != None && UTVehicle(B.Pawn).bKeyVehicle)
		{
			if ( B.DefensePoint != None )
				B.FreePoint();
			return false;
		}
	}
	// see if should get superweapon/ pickup
	if ( B.Pawn.bCanPickupInventory && (B.Skill > 1) && (UTHoldSpot(B.DefensePoint) == None) )
	{
		if ( PriorityObjective(B) > 0 )
			SuperDist = 800;
		else if ( GetOrders() == 'Attack' )
			SuperDist = 3000;
		else if ( (GetOrders() == 'Defend') && (B.Enemy != None) )
			SuperDist = 1200;
		else
			SuperDist = 3200;
		bMovingToSuperPickup = ( (PickupFactory(B.RouteGoal) != None)
								&& PickupFactory(B.RouteGoal).bIsSuperItem
								&& (B.RouteDist < 1.1*SuperDist)
								&&  PickupFactory(B.RouteGoal).ReadyToPickup(2)
								&& (B.RatePickup(B.RouteGoal, PickupFactory(B.RouteGoal).InventoryType) > 0) );
		if ( (bMovingToSuperPickup && B.FindBestPathToward(B.RouteGoal, false, true))
			||  (Team.AI.SuperPickupAvailable(B)
				&& (B.Pawn.Anchor != None) && B.Pawn.ReachedDestination(B.Pawn.Anchor)
				&& B.FindSuperPickup(SuperDist)) )
		{
			B.GoalString = "Get super pickup "$B.RouteGoal;
			B.SetAttractionState();
			return true;
		}
	}

	if ( B.DefensePoint != None )
	{
		DesiredPosition = B.DefensePoint.GetMoveTarget();
		bInPosition = (B.Pawn == DesiredPosition) || B.Pawn.ReachedDestination(DesiredPosition);
		if ( bInPosition && (Vehicle(DesiredPosition) != None) )
		{
			if ( (Vehicle(B.Pawn) != None) && (B.Pawn != DesiredPosition) )
			{
				B.LeaveVehicle(true);
				return true;
			}
			if ( Vehicle(B.Pawn) == None )
			{
				B.EnterVehicle(Vehicle(DesiredPosition));
				return true;
			}
		}
		if (B.ShouldDefendPosition())
		{
			return true;
		}
	}
	else if ( SquadObjective == None )
		return TellBotToFollow(B,SquadLeader);
	else if ( GetOrders() == 'Freelance' )
		return false;
	else
	{
		if ( SquadObjective.DefenderTeamIndex != Team.TeamIndex )
		{
			if ( SquadObjective.IsDisabled() )
			{
				B.GoalString = "Objective already disabled";
				return false;
			}
			B.GoalString = "Disable Objective "$SquadObjective;
			return SquadObjective.TellBotHowToDisable(B);
		}
		if ( (B.DefensivePosition != None) && AcceptableDefensivePosition(B.DefensivePosition, B) )
			DesiredPosition = B.DefensivePosition;
		else
			DesiredPosition = SquadObjective;
		bInPosition = ( (VSize(SquadObjective.Location - B.Pawn.Location) < NEAROBJECTIVEDIST) && B.LineOfSightTo(SquadObjective) );
	}

	if ( B.Enemy != None )
	{
		if ( B.LostContact(5) )
			B.LoseEnemy();
		if ( B.Enemy != None )
		{
			if ( B.LineOfSightTo(B.Enemy) || (WorldInfo.TimeSeconds - B.LastSeenTime < 3 && (SquadObjective == None || !SquadObjective.TeamLink(Team.TeamIndex))) )
			{
				B.FightEnemy(false, 0);
				return true;
			}
		}
	}
	if ( bInPosition )
	{
		B.GoalString = "Near defense position" @ DesiredPosition;
		if ( !B.bInitLifeMessage )
		{
			B.bInitLifeMessage = true;
			B.SendMessage(None, 'OTHER', B.GetMessageIndex('INPOSITION'), 10, 'TEAM');
		}

		if ( B.DefensePoint != None )
			B.MoveToDefensePoint();
		else
		{
			if ( SquadObjective.TellBotHowToHeal(B) )
				return true;

			if (B.Enemy != None && (B.LineOfSightTo(B.Enemy) || WorldInfo.TimeSeconds - B.LastSeenTime < 3))
			{
				B.FightEnemy(false, 0);
				return true;
			}

			B.WanderOrCamp();
		}
		return true;
	}

	if (B.Pawn.bStationary)
		return false;

	B.GoalString = "Follow path to "$DesiredPosition;
	B.FindBestPathToward(DesiredPosition,false,true);
	if ( B.StartMoveToward(DesiredPosition) )
		return true;

	if ( (B.DefensePoint != None) && (DesiredPosition == B.DefensePoint) )
	{
		if ( (B.Pawn.Anchor != None) && B.Pawn.ReachedDestination(B.Pawn.Anchor) )
			`log(B.PlayerReplicationInfo.PlayerName$" had no path to "$B.DefensePoint$" from "$B.Pawn.Anchor);
		else
			`log(B.PlayerReplicationInfo.PlayerName$" had no path to "$B.DefensePoint);

		B.FreePoint();
		if ( (SquadObjective != None) && (VSize(B.Pawn.Location - SquadObjective.Location) > 1200) )
		{
			B.FindBestPathToward(SquadObjective,false,true);
			if ( B.StartMoveToward(SquadObjective) )
				return true;
		}
	}
	return false;
}

function bool ShouldDestroyTranslocator(UTBot B)
{
	local UTGame G;
	local UTProj_TransDisc T;

	if ( (Vehicle(B.Pawn) != None) || (B.Enemy != None) || (B.Skill < 2) )
		return false;
	G = UTGame(WorldInfo.Game);
	if ( G == None )
		return false;

	for ( T=G.BeaconList; T!=None; T=T.NextBeacon )
	{
		if ( T.bCollideActors && !T.Disrupted() && (T.Instigator != None) && (T.Instigator.Controller != None)
			&& !WorldInfo.GRI.OnSameTeam(B,T.Instigator.Controller)
			&& (VSize(B.Pawn.Location - T.Location) < 1500)
			&& B.LineOfSightTo(T) )
		{
			B.SwitchToBestWeapon();
			B.GoalString = "Destroy Translocator";
			B.DoRangedAttackOn(T);
			return true;
		}
	}
	return false;
}

function float BotSuitability(UTBot B)
{
	//@todo steve - determine suitablity based on bot attributes
	return 1;
}

/* PickBotToReassign()
pick a bot to lose
*/
function UTBot PickBotToReassign()
{
	local UTBot B,Best;
	local float Val, BestVal;
	local float Suitability, BestSuitability;

	// pick bot furthest from SquadObjective, with highest suitability
	for	( B=SquadMembers; B!=None; B=B.NextSquadMember )
		if ( !B.PlayerReplicationInfo.bOutOfLives )
		{
			Val = VSize(B.Pawn.Location - SquadObjective.Location);
			if ( B == SquadLeader )
				Val -= 10000000.0;
			Suitability = BotSuitability(B);
			if ( (Best == None) || (Suitability > BestSuitability)
				|| ((Suitability == BestSuitability) && (Val > BestVal)) )
			{
				Best = B;
				BestVal = Val;
				BestSuitability = Suitability;
			}
		}
	return Best;
}

simulated function DisplayDebug(HUD HUD, out float YL, out float YPos)
{
	local string EnemyList;
	local int i;
	local Canvas Canvas;

	Canvas = HUD.Canvas;
	Canvas.SetDrawColor(255,255,255);
	if ( SquadObjective == None )
		Canvas.DrawText("     ORDERS "$GetOrders()$" on "$GetItemName(string(self))$" no objective. Leader "$SquadLeader.GetHumanReadableName(), false);
	else
		Canvas.DrawText("     ORDERS "$GetOrders()$" on "$GetItemName(string(self))$" objective "$GetItemName(string(SquadObjective))$". Leader "$SquadLeader.GetHumanReadableName(), false);

	YPos += YL;
	Canvas.SetPos(4,YPos);
	EnemyList = "     Enemies: ";
	for ( i=0; i<ArrayCount(Enemies); i++ )
		if ( Enemies[i] != None )
			EnemyList = EnemyList@Enemies[i].GetHumanReadableName();
	Canvas.DrawText(EnemyList, false);

	YPos += YL;
	Canvas.SetPos(4,YPos);
}

/* BeDevious()
return true if bot should use guile in hunting opponent (more expensive)
*/
function bool BeDevious()
{
	return false;
}

function bool PickRetreatDestination(UTBot B)
{
	// FIXME - fall back to other squad members (furthest), or defense objective, or home base
	return B.PickRetreatDestination();
}

/* ClearPathForLeader()
make all squad members close to leader get out of his way
*/
function bool ClearPathFor(Controller C)
{
	local UTBot B;
	local bool bForceDefer;
	local vector Dir;

	bForceDefer = ShouldDeferTo(C);

	for ( B=SquadMembers; B!=None; B=B.NextSquadMember )
		if ( (B != C) && (B.Pawn != None) )
		{
			Dir = B.Pawn.Location - C.Pawn.Location;
			if ( (Abs(Dir.Z) < B.Pawn.GetCollisionHeight() + C.Pawn.GetCollisionHeight())
				&& (VSize2D(Dir) < 8.f * B.Pawn.GetCollisionRadius()) )
			{
				if ( bForceDefer || B.IsDefending() || B.Stopped() )
					B.ClearPathFor(C);
			}
		}
	return bForceDefer;
}

function bool IsDefending(UTBot B)
{
	if ( GetOrders() == 'Defend' )
		return true;

	return ( B.DefensePoint != None );
}

function bool FriendlyToward(Pawn Other)
{
	if ( Team == None )
		return false;
	return Team.AI.FriendlyToward(Other);
}

/** @return the maximum distance a bot should be from the given Actor it wants to defend */
function float GetMaxDefenseDistanceFrom(Actor Center)
{
	return (Pawn(Center) != None ? FormationSize : NEAROBJECTIVEDIST);
}

function NavigationPoint FindDefensivePositionFor(UTBot B)
{
	local NavigationPoint N, Best;
	local int Num;
	local float CurrentRating, BestRating;
	local Actor Center;

	Center = FormationCenter();
	if (Center == None)
	{
		Center = B.Pawn;
	}

	foreach WorldInfo.RadiusNavigationPoints(class'NavigationPoint', N, Center.Location, GetMaxDefenseDistanceFrom(Center))
	{
		CurrentRating = RateDefensivePosition(N, B, Center);
		if (CurrentRating > BestRating)
		{
			BestRating = CurrentRating;
			Best = N;
			Num = 1;
		}
		else if ( CurrentRating == BestRating )
		{
			Num++;
			if ( (Best == None) || (Rand(Num) == 0) )
			{
				Best = N;
			}
		}
	}
	return Best;
}

function float RateDefensivePosition(NavigationPoint N, UTBot CurrentBot, Actor Center)
{
	local float Rating;
	local float Dist;
	local UTBot B;

	if (N.IsA('Teleporter') || N.IsA('PortalMarker') || !FastTrace(N.Location, Center.Location))
	{
		return -1;
	}

	// make sure no squadmate using this point, and adjust rating based on proximity
	Rating = 1;
	for ( B=SquadMembers; B!=None; B=B.NextSquadMember )
	{
		if ( B != CurrentBot )
		{
			if ( (B.DefensePoint == N) || (B.DefensivePosition == N) )
			{
				return -1;
			}
			else if ( B.Pawn != None )
			{
				Rating *= 0.002*VSize(B.Pawn.Location - N.Location);
			}
		}
	}

	Dist = VSize(N.Location - Center.Location);
	if (Dist < 400.0)
	{
		return (0.00025 * Dist);
	}

	return Rating;
}

function bool AcceptableDefensivePosition(NavigationPoint N, UTBot B)
{
	local Actor Center;

	Center = FormationCenter();
	if (Center == None)
	{
		Center = B.Pawn;
	}
	return (VSize(N.Location - Center.Location) <= GetMaxDefenseDistanceFrom(Center) && RateDefensivePosition(N, B, Center) > 0);
}

/** @return the objective the given bot wants to spawn at */
function UTGameObjective GetStartObjective(UTBot B)
{
	// Bots return SquadObjective, or nearest spawnable node to it
	// @todo steve - should they pick a node with a good vehicle in some situations?
	return (SquadObjective != None ? SquadObjective.FindNearestFriendlyNode(Team.TeamIndex) : None);
}

/** called when a ReachSpec the given bot wants to use is blocked by a dynamic obstruction
 * gives the AI an opportunity to do something to get rid of it instead of trying to find another path
 * @note MoveTarget is the actor the AI wants to move toward, CurrentPath the ReachSpec it wants to use
 * @param B the bot whose path is blocked
 * @param BlockedBy the object blocking the path
 * @return true if the AI did something about the obstruction and should use the path anyway, false if the path
 * is unusable and the bot must find some other way to go
 */
function bool HandlePathObstruction(UTBot B, Actor BlockedBy)
{
	local int i;

	// if the bot is blocked getting to a VehicleParkingSpot, just pretend we reached it
	if (B.MoveTarget == B.RouteGoal && SquadObjective != None && Vehicle(B.Pawn) != None)
	{
		for (i = 0; i < SquadObjective.VehicleParkingSpots.length; i++)
		{
			if (B.RouteGoal == SquadObjective.VehicleParkingSpots[i])
			{
				B.bPreparingMove = true;
				B.MoveTimer = -1.0;
				LeaveVehicleAtParkingSpot(B, SquadObjective);
				return true;
			}
		}
	}

	return false;
}

defaultproperties
{
	 GatherThreshold=+0.0
	 MaxSquadSize=2
	 bRoamingSquad=true
	 NetUpdateFrequency=1
	 FormationSize=+900.0
}
