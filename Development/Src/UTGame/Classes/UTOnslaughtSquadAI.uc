/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtSquadAI extends UTSquadAI;

/** pointer to Team.AI so we don't have to cast all the time */
var UTOnslaughtTeamAI ONSTeamAI;
var bool bDefendingSquad;
var float LastFailedNodeTeleportTime;
var float MaxObjectiveGetOutDist; //cached highest ObjectiveGetOutDist of all the vehicles available on this level

function Initialize(UTTeamInfo T, UTGameObjective O, Controller C)
{
	Super.Initialize(T, O, C);

	ONSTeamAI = UTOnslaughtTeamAI(T.AI);
	`Warn("TeamAI is not a subclass of UTOnslaughtTeamAI",ONSTeamAI == None);
}

function Reset()
{
	Super.Reset();
	bDefendingSquad = false;
}

function name GetOrders()
{
	local name NewOrders;

	if ( PlayerController(SquadLeader) != None )
		NewOrders = 'Human';
	else if ( bFreelance && !bFreelanceAttack && !bFreelanceDefend )
		NewOrders = 'Freelance';
	else if ( bDefendingSquad || bFreelanceDefend || (SquadObjective != None && SquadObjective.DefenseSquad == self) )
		NewOrders = 'Defend';
	else
		NewOrders = 'Attack';
	if ( NewOrders != CurrentOrders )
	{
		CurrentOrders = NewOrders;
		NetUpdateTime = WorldInfo.Timeseconds - 1;
	}
	return CurrentOrders;
}

function byte PriorityObjective(UTBot B)
{
	local UTOnslaughtObjective Core;

	if (GetOrders() == 'Defend')
	{
		Core = UTOnslaughtObjective(SquadObjective);
		if (Core != None && (Core.DefenderTeamIndex == Team.TeamIndex) && Core.bUnderAttack)
			return 1;
	}
	else if (CurrentOrders == 'Attack')
	{
		Core = UTOnslaughtObjective(SquadObjective);
		if (Core != None)
		{
			if ( UTOnslaughtPowerCore(Core) != None )
			{
				if (B.Enemy != None && Core.BotNearObjective(B))
					return 1;
			}
			else if (VSize(B.Pawn.Location - Core.Location) < 1000)
				return 1;
		}
	}
	return 0;
}

function SetDefenseScriptFor(UTBot B)
{
	local UTOnslaughtObjective Core;

	//don't look for defense scripts when heading for neutral node
	Core = UTOnslaughtObjective(SquadObjective);
	if (Core == None || (Core.DefenderTeamIndex == Team.TeamIndex && (Core.IsConstructing() || Core.IsActive())) )
	{
		Super.SetDefenseScriptFor(B);
		return;
	}

	if (B.DefensePoint != None)
		B.FreePoint();
}

/** returns the maximum distance the given Pawn should search for vehicles to enter */
function float MaxVehicleDist(Pawn P)
{
	local UTOnslaughtPowerNode Node;

	Node = UTOnslaughtPowerNode(SquadObjective);
	if (Node != None && Node.DefenderTeamIndex != Team.TeamIndex && ONSTeamAI.Flag != None && ONSTeamAI.Flag.Holder == None)
	{
		return FMin(FMin(3000.0, VSize(SquadObjective.Location - P.Location)), VSize(ONSTeamAI.Flag.Position().Location - P.Location));
	}
	else
	{
		return Super.MaxVehicleDist(P);
	}
}

function bool ShouldUseAlternatePaths()
{
	local UTOnslaughtObjective ONSObjective;

	// use alternate paths only when attacking active enemy objective
	ONSObjective = UTOnslaughtObjective(SquadObjective);
	return (ONSObjective != None && ONSObjective.DefenderTeamIndex != Team.TeamIndex && ONSObjective.IsActive());
}

function bool MustCompleteOnFoot(Actor O)
{
	local UTOnslaughtNodeObjective Node;

	Node = UTOnslaughtNodeObjective(O);
	//@FIXME: only need to be on foot for non-neutral powernode if have orb
	if (Node != None && (Node.IsNeutral() || UTOnslaughtPowerNode(Node) != None))
	{
		return true;
	}
	else
	{
		return Super.MustCompleteOnFoot(O);
	}
}

/** used with bot's CustomAction interface to process a node teleport */
function bool NodeTeleport(UTBot B)
{
	local UTOnslaughtObjective Objective;

	Objective = UTOnslaughtObjective(B.RouteGoal);
	if (Objective == None || !Objective.TeleportTo(UTPawn(B.Pawn)))
	{
		LastFailedNodeTeleportTime = WorldInfo.TimeSeconds;
	}

	return true;
}

function bool CheckVehicle(UTBot B)
{
	local UTOnslaughtObjective Best, Core;
	local UTGameObjective O;
	local float NewRating, BestRating;
	local byte SourceDist;
	local Weapon SuperWeap;
	local int i, j;
	local UTVehicle V;
	local Actor Goal;
	local UTVehicle_Deployable DeployableVehicle;
	local UTOnslaughtGame ONSGame;
	local UTOnslaughtNodeObjective Node;
	local NavigationPoint TeleportSource;
	local UTOnslaughtObjective ONSObjective;

	ONSObjective = UTOnslaughtObjective(SquadObjective);
	if ( (UTVehicle(B.Pawn) != None) && ((B.Skill + B.Tactics >= 5) || UTVehicle(B.Pawn).bKeyVehicle) && UTVehicle(B.Pawn).IsArtillery() )
	{
		DeployableVehicle = UTVehicle_Deployable(B.Pawn);
		if (DeployableVehicle != None && DeployableVehicle.IsDeployed())
		{
			// if possible, just target and fire at nodes or important enemies
			if ( (SquadObjective != None) && (SquadObjective.DefenderTeamIndex != Team.TeamIndex) && (ONSObjective != None)
				&& ONSObjective.LegitimateTargetOf(B) && B.Pawn.CanAttack(SquadObjective) )
			{
				B.DoRangedAttackOn(SquadObjective);
				B.GoalString = "Artillery Attack Objective";
				return true;
			}
			if ( (B.Enemy != None) && B.Pawn.CanAttack(B.Enemy) )
			{
				B.DoRangedAttackOn(B.Enemy);
				B.GoalString = "Artillery Attack Enemy";
				return true;
			}
			// check squad enemies
			for ( i=0; i<8; i++ )
			{
				if ( (Enemies[i] != None) && (Enemies[i] != B.Enemy) && B.Pawn.CanAttack(Enemies[i]) )
				{
					B.DoRangedAttackOn(Enemies[i]);
					B.GoalString = "Artillery Attack Squad Enemy";
					return true;
				}
			}
			// check other nodes
			for ( O=Team.AI.Objectives; O!=None; O=O.NextObjective )
			{
				Core = UTOnslaughtObjective(O);
				if ( (Core != None) && Core.PoweredBy(Team.TeamIndex) && (Core.DefenderTeamIndex != Team.TeamIndex) && Core.LegitimateTargetOf(B) && B.Pawn.CanAttack(Core) )
				{
					B.DoRangedAttackOn(Core);
					B.GoalString = "Artillery Attack Other Node";
					return true;
				}
			}

			// check important enemies
			for ( V=UTGame(WorldInfo.Game).VehicleList; V!=None; V=V.NextVehicle )
			{
				if ( (V.Controller != None) && !V.bCanFly && (V.ImportantVehicle() || V.IsArtillery()) && !WorldInfo.GRI.OnSameTeam(V,B) && B.Pawn.CanAttack(V) )
				{
					B.DoRangedAttackOn(V);
					B.GoalString = "Artillery Attack important vehicle";
					return true;
				}
			}

			if ( Team.Size == DeployableVehicle.NumPassengers() ||
				( VSize(B.Pawn.Location - SquadObjective.Location) > DeployableVehicle.ObjectiveGetOutDist &&
					!SquadObjective.ReachedParkingSpot(B.Pawn) && !B.Pawn.CanAttack(SquadObjective) ) )
			{
				DeployableVehicle.ServerToggleDeploy();
			}
		}
		else if ( !B.Pawn.IsFiring() && (B.Enemy != None) && B.Pawn.CanAttack(B.Enemy) )
		{
			B.Focus = B.Enemy;
			B.FireWeaponAt(B.Enemy);
		}
	}

	if ( SquadObjective != None )
	{
		if ( GetOrders() == 'Attack' )
		{
			if ( ONSObjective != None && !ONSObjective.IsNeutral() && !WorldInfo.GRI.OnSameTeam(ONSObjective, B)
				&& ONSObjective.PoweredBy(1 - Team.TeamIndex) && (UTVehicle(B.Pawn) == None || !UTVehicle(B.Pawn).bKeyVehicle) )
			{
				SuperWeap = B.HasSuperWeapon();
				if ( (SuperWeap != None) &&  B.LineOfSightTo(SquadObjective) )
				{
					if (Vehicle(B.Pawn) != None)
					{
						B.DirectionHint = Normal(SquadObjective.Location - B.Pawn.Location);
						B.LeaveVehicle(true);
						return true;
					}
					return SquadObjective.TellBotHowToDisable(B);
				}
			}
		}
		else if ( UTVehicle(B.Pawn) != None && GetOrders() == 'Defend' && !UTVehicle(B.Pawn).bDefensive
			&& VSize(B.Pawn.Location - SquadObjective.Location) < 1600 && !UTVehicle(B.Pawn).bKeyVehicle
			&& ((B.Enemy == None) || (WorldInfo.TimeSeconds - B.LastSeenTime > 4) || (!UTVehicle(B.Pawn).ImportantVehicle() && !B.LineOfSightTo(B.Enemy))) )
		{
			B.DirectionHint = Normal(SquadObjective.Location - B.Pawn.Location);
			B.LeaveVehicle(true);
			return true;
		}
	}
	if (Vehicle(B.Pawn) == None && ONSObjective != None)
	{
		if ( ONSObjective.IsActive() )
		{
			if ( GetOrders() == 'Defend' && (B.Enemy == None || (!B.LineOfSightTo(B.Enemy) && WorldInfo.TimeSeconds - B.LastSeenTime > 3))
			     && VSize(B.Pawn.Location - SquadObjective.Location) < GetMaxObjectiveGetOutDist()
			     && ONSObjective.Health < ONSObjective.DamageCapacity
			     && ( (UTWeapon(B.Pawn.Weapon) != None && UTWeapon(B.Pawn.Weapon).CanHeal(SquadObjective)) ||
			     		(B.Pawn.InvManager != None && UTWeapon(B.Pawn.InvManager.PendingWeapon) != None && UTWeapon(B.Pawn.InvManager.PendingWeapon).CanHeal(SquadObjective)) ) )
				return false;
		}
		else if ( ONSObjective.IsConstructing() )
		{
			if ( (B.Enemy == None || !B.LineOfSightTo(B.Enemy)) && VSize(B.Pawn.Location - SquadObjective.Location) < GetMaxObjectiveGetOutDist() )
				return false;
		}
		else if ((ONSObjective.IsNeutral() || ONSObjective.IsCurrentlyDestroyed()) && VSize(B.Pawn.Location - SquadObjective.Location) < GetMaxObjectiveGetOutDist())
			return false;
	}

	if (Super.CheckVehicle(B))
		return true;
	if ( Vehicle(B.Pawn) != None || (B.Enemy != None && B.LineOfSightTo(B.Enemy))
		|| LastFailedNodeTeleportTime > WorldInfo.TimeSeconds - 20.0 || ONSObjective == None
		|| UTOnslaughtPRI(B.PlayerReplicationInfo) == None || ONSObjective.HasUsefulVehicles(B)
		|| B.PlayerReplicationInfo.bHasFlag || B.Skill + B.Tactics < 2.0 + FRand() )
	{
		return false;
	}

	// no vehicles around
	if (SquadObjective.IsA('UTOnslaughtPowerNode') && ONSTeamAI.Flag != None && ONSTeamAI.Flag.Holder == None)
	{
		Goal = ONSTeamAI.Flag.Position();
	}
	else
	{
		Goal = SquadObjective;
	}
	if (VSize(B.Pawn.Location - Goal.Location) > 5000 && !B.LineOfSightTo(Goal))
	{
		// really want a vehicle to get to Goal, so teleport to a different node to find one
		ONSGame = UTOnslaughtGame(WorldInfo.Game);
		if (ONSGame.IsTouchingNodeTeleporter(B.Pawn))
		{
			SourceDist = UTOnslaughtObjective(B.RouteGoal).FinalCoreDistance[Abs(1 - Team.TeamIndex)];
			for (O = Team.AI.Objectives; O != None; O = O.NextObjective)
			{
				if ( O != B.RouteGoal && O.IsA('UTOnslaughtObjective') &&
					UTOnslaughtObjective(O).ValidSpawnPointFor(Team.TeamIndex) )
				{
					NewRating = UTOnslaughtNodeObjective(O).TeleportRating(B, Team.TeamIndex, SourceDist);
					if (NewRating > BestRating || (NewRating == BestRating && FRand() < 0.5))
					{
						Best = UTOnslaughtObjective(O);
						BestRating = NewRating;
					}
				}
			}

			if (Best == None)
			{
				LastFailedNodeTeleportTime = WorldInfo.TimeSeconds;
				return false;
			}
			else
			{
				B.RouteGoal = Best;
				B.PerformCustomAction(NodeTeleport);
				return true;
			}
		}

		// mark all usable nearby node teleporters as pathing endpoints
		for (i = 0; i < ONSGame.PowerNodes.Length; i++)
		{
			Node = UTOnslaughtNodeObjective(ONSGame.PowerNodes[i]);
			if (Node != None && Node.IsActive() && WorldInfo.GRI.OnSameTeam(B, Node))
			{
				/* disable teleporting from nodes for now
				if (VSize(Node.Location - B.Pawn.Location) < 2000.0)
				{
					Node.bTransientEndPoint = true;
					TeleportSource = Node;
				}
				*/
				for (j = 0; j < Node.NodeTeleporters.Length; j++)
				{
					if (VSize(Node.NodeTeleporters[j].Location - B.Pawn.Location) < 2000.0)
					{
						Node.NodeTeleporters[j].bTransientEndPoint = true;
						TeleportSource = Node.NodeTeleporters[j];
					}
				}
			}
		}
		// if we didn't find any, abort
		if (TeleportSource == None)
		{
			return false;
		}
		// otherwise, try to find path to one of them
		B.MoveTarget = B.FindPathToward(TeleportSource, false);
		if (B.MoveTarget != None)
		{
			B.NoVehicleGoal = B.RouteGoal;
			B.GoalString = "Node teleport from" @ B.RouteGoal;
			B.SetAttractionState();
			return true;
		}

		/*
		Nearest = UTOnslaughtGame(WorldInfo.Game).ClosestObjectiveTo(B.Pawn);
		if ( Nearest == None || !Nearest.IsActive() || Nearest.DefenderTeamIndex != Team.TeamIndex
		     || VSize(Nearest.Location - B.Pawn.Location) > 2000 )
			return false;

		B.MoveTarget = B.FindPathToward(Nearest, false);
		if (B.MoveTarget != None)
		{
			B.RouteGoal = Nearest;
			B.NoVehicleGoal = Nearest; // so bots don't try to use the hoverboard and get confused
			B.GoalString = "Node teleport from "$B.RouteGoal;
			B.SetAttractionState();
			return true;
		}
		*/
	}

	return false;
}

function BotEnteredVehicle(UTBot B)
{
	local UTBot SquadMate, Best;
	local float BestDist, NewDist;
	local PlayerController PC;

	Super.BotEnteredVehicle(B);

	if ( !UTGame(WorldInfo.Game).JustStarted(20) || (UTVehicle(B.Pawn) == None) || !UTVehicle(B.Pawn).FastVehicle()
		|| (self == Team.AI.FreelanceSquad) || (Team.AI.FreelanceSquad == None) )
		return;

	// don't do on teams with humans, since this might contradict their orders
	if ( WorldInfo.NetMode == NM_Standalone )
	{
		ForEach LocalPlayerControllers(class'PlayerController', PC)
			if ( PC.PlayerReplicationInfo.Team == Team )
				return;
	}
	else if ( !UTGame(WorldInfo.Game).bPlayersVsBots )
		return;

	// if in fast vehicle, and freelance squad doesn't have one, switch
	for ( SquadMate=Team.AI.FreelanceSquad.SquadMembers; SquadMate!=None; SquadMate=SquadMate.NextSquadMember )
	{
		if ( (UTVehicle(SquadMate.Pawn) != None) && UTVehicle(SquadMate.Pawn).FastVehicle() )
			return;
		else if ( Best == None )
		{
			Best = SquadMate;
			if ( Best.Pawn != None )
				BestDist = VSize(B.Pawn.Location - SquadObjective.Location);
			else
				BestDist = 1000000;
		}
		else if (SquadMate.Pawn != None )
		{
			NewDist = VSize(SquadMate.Pawn.Location - SquadObjective.Location);
			if ( NewDist < BestDist )
			{
				Best = SquadMate;
				BestDist = NewDist;
			}
		}
	}

	// make the switch
	// FIXMESTEVE SwitchBots(B,Best);
}

//return a value indicating how useful this vehicle is to the bot
function float VehicleDesireability(UTVehicle V, UTBot B)
{
	local float Rating;

	if (CurrentOrders == 'Defend')
	{
		if ((SquadObjective == None || VSize(SquadObjective.Location - B.Pawn.Location) < 2000))
		{
			if (Super.VehicleDesireability(V, B) <= 0.0)
			{
				return 0.0;
			}
		}
		else if (B.PlayerReplicationInfo.bHasFlag && !V.bCanCarryFlag)
		{
			return 0.0;
		}
		else if (V.Health < V.Default.Health * 0.125 && B.Enemy != None && B.LineOfSightTo(B.Enemy))
		{
			return 0.0;
		}
		Rating = V.BotDesireability(self, Team.TeamIndex, SquadObjective);
		if (Rating <= 0.0)
		{
			return 0.0;
		}

		if (V.bDefensive || V.bStationary)
		{
			if (UTOnslaughtObjective(SquadObjective) != None)
			{
				//turret can't hit priority enemy
				if ( V.bStationary && B.Enemy != None && UTOnslaughtObjective(SquadObjective).LastDamagedBy == B.Enemy.PlayerReplicationInfo
				     && !FastTrace(B.Enemy.Location + B.Enemy.GetCollisionHeight() * vect(0,0,1), V.Location) )
				{
					return 0.0;
				}
				if (UTOnslaughtGame(WorldInfo.Game).ClosestObjectiveTo(V) != SquadObjective)
				{
					return 0.0;
				}
			}
		}

		return V.SpokenFor(B) ? (Rating * V.ReservationCostMultiplier(B.Pawn)) : Rating;
	}
	else
	{
		return Super.VehicleDesireability(V, B);
	}
}

function bool CheckSpecialVehicleObjectives(UTBot B)
{
/* FIXMESTEVE
	local UTOnslaughtObjective Core;
	local UTSquadAI S;

	if ( ONSTeamAI.OverrideCheckSpecialVehicleObjectives(B) )
		return false;

	if (Size > 1)
		return Super.CheckSpecialVehicleObjectives(B);

	//if bot is the only bot headed to a neutral (unclaimed) node, that's more important, so don't head to any SpecialVehicleObjectives
	Core = UTOnslaughtObjective(SquadObjective);
	if (Core == None || !Core.IsNeutral() || !Core.PoweredBy(Team.TeamIndex))
		return Super.CheckSpecialVehicleObjectives(B);

	for (S = Team.AI.Squads; S != None; S = S.NextSquad)
		if (S != self && S.SquadObjective == Core)
			return Super.CheckSpecialVehicleObjectives(B);
*/
	return false;
}

function float GetMaxObjectiveGetOutDist()
{
	local UTVehicleFactory F;

	if (MaxObjectiveGetOutDist == 0.0)
		foreach DynamicActors(class'UTVehicleFactory', F)
			if (F.VehicleClass != None)
				MaxObjectiveGetOutDist = FMax(MaxObjectiveGetOutDist, F.VehicleClass.default.ObjectiveGetOutDist);

	return MaxObjectiveGetOutDist;
}

function bool AssignSquadResponsibility(UTBot B)
{
	// if we have the flag, but we're not doing anything with a powernode, drop it
	if ( B.PlayerReplicationInfo.bHasFlag && UTOnslaughtPowerNode(SquadObjective) == None &&
		B.PlayerReplicationInfo.IsA('UTPlayerReplicationInfo') )
	{
		UTPlayerReplicationInfo(B.PlayerReplicationInfo).GetFlag().Drop();
	}

	return Super.AssignSquadResponsibility(B);
}

function bool CheckSquadObjectives(UTBot B)
{
	local bool bResult;

	if ( (B.Enemy != None) && (B.Enemy == ONSTeamAI.FinalCore.LastAttacker) )
	{
		if ( B.NeedWeapon() && B.FindInventoryGoal(0) )
		{
			B.GoalString = "Need weapon or ammo";
			B.SetAttractionState();
			return true;
		}
		return false;
	}

	if ( !ONSTeamAI.bAllNodesTaken && (self == Team.AI.FreelanceSquad) )
	{
		// keep moving to any unpowered nodes if current objective is constructing
		if ( UTOnslaughtObjective(SquadObjective).DefenderTeamIndex == Team.TeamIndex )
			Team.AI.ReAssessStrategy();
	}
	bResult = Super.CheckSquadObjectives(B);

	if (!bResult && CurrentOrders == 'Freelance' && B.Enemy == None && UTOnslaughtObjective(SquadObjective) != None)
	{
		if ( UTOnslaughtObjective(SquadObjective).PoweredBy(Team.TeamIndex) )
		{
			B.GoalString = "Disable Objective "$SquadObjective;
			return SquadObjective.TellBotHowToDisable(B);
		}
		else if ( !B.LineOfSightTo(SquadObjective) )
		{
			B.GoalString = "Harass enemy at "$SquadObjective;
			return FindPathToObjective(B, SquadObjective);
		}
	}

	return bResult;
}

function float ModifyThreat(float current, Pawn NewThreat, bool bThreatVisible, UTBot B)
{
	if ( NewThreat.PlayerReplicationInfo != None && UTOnslaughtObjective(SquadObjective) != None
	     && UTOnslaughtObjective(SquadObjective).LastDamagedBy == NewThreat.PlayerReplicationInfo
	     && UTOnslaughtObjective(SquadObjective).bUnderAttack )
	{
		if ( NewThreat == ONSTeamAI.FinalCore.LastAttacker )
		{
			return current + 6.0;
		}
		if (!bThreatVisible)
		{
			return current + 0.5;
		}
		if ( (VSize(B.Pawn.Location - NewThreat.Location) < 2000) || B.Pawn.IsA('Vehicle') || UTWeapon(B.Pawn.Weapon).bSniping
			|| UTOnslaughtObjective(SquadObjective).Health < UTOnslaughtObjective(SquadObjective).DamageCapacity * 0.5 )
		{
			return current + 6.0;
		}
		else
		{
			return current + 1.5;
		}
	}
	else if (NewThreat.PlayerReplicationInfo != None && NewThreat.PlayerReplicationInfo.bHasFlag && bThreatVisible)
	{
		if ( VSize(B.Pawn.Location - NewThreat.Location) < 1500.0 || (B.Pawn.Weapon != None && UTWeapon(B.Pawn.Weapon).bSniping)
			|| (UTOnslaughtPowerNode(SquadObjective) != None && VSize(NewThreat.Location - SquadObjective.Location) < 2000.0) )
		{
			return current + 6.0;
		}
		else
		{
			return current + 1.5;
		}
	}
	else
	{
		return current;
	}
}

function bool MustKeepEnemy(Pawn E)
{
	if ( (E.PlayerReplicationInfo != None) && (UTOnslaughtObjective(SquadObjective) != None) )
	{
		if ( UTOnslaughtObjective(SquadObjective).bUnderAttack && (UTOnslaughtObjective(SquadObjective).LastDamagedBy == E.PlayerReplicationInfo) )
			return true;
		if ( E == ONSTeamAI.FinalCore.LastAttacker )
			return true;
	}
	return false;
}

function SetObjective(UTGameObjective O, bool bForceUpdate)
{
	local UTOnslaughtNodeObjective Node;
	local UTOnslaughtSpecialObjective Best;
	local int i;

	Node = UTOnslaughtNodeObjective(O);
	if (Node != None && Node.ActivatedObjectives.length > 0)
	{
		// consider attacking side objectives instead
		if ( UTOnslaughtSpecialObjective(SquadObjective) != None &&
			Node.ActivatedObjectives.Find(UTOnslaughtSpecialObjective(SquadObjective)) != INDEX_NONE &&
			SquadObjective.IsActive() )
		{
			// keep the current one
			Super.SetObjective(SquadObjective, bForceUpdate);
		}
		else
		{
			Best = None;
			for (i = 0; i < Node.ActivatedObjectives.length; i++)
			{
				if (Node.ActivatedObjectives[i].IsActive())
				{
					if ( Best == None ||
						(!Best.bMustCompleteToAttackNode && Node.ActivatedObjectives[i].bMustCompleteToAttackNode) ||
						Best.DefensePriority < Node.ActivatedObjectives[i].DefensePriority ||
						( ONSTeamAI.ObjectiveCoveredByAnotherSquad(Best, self) &&
							!ONSTeamAI.ObjectiveCoveredByAnotherSquad(Node.ActivatedObjectives[i], self) ) ||
						(Best.DefensePriority == Node.ActivatedObjectives[i].DefensePriority && FRand() < 0.5) )
					{
						Best = Node.ActivatedObjectives[i];
					}
				}
			}
			// if we don't have to do it, sometimes skip it
			if (Best != None && (Best.bMustCompleteToAttackNode || FRand() < (0.1 * Best.DefensePriority)))
			{
				Super.SetObjective(Best, bForceUpdate);
			}
			else
			{
				Super.SetObjective(Node, bForceUpdate);
			}
		}
	}
	else
	{
		Super.SetObjective(O, bForceUpdate);
	}
}

function UTGameObjective GetStartObjective(UTBot B)
{
	local UTOnslaughtFlagBase FlagBase;

	// if bot is attacking a node (not core) and orb is available at base, spawn there instead
	//@todo: what about defending bots if have closest node to enemy core?
	if (GetOrders() == 'ATTACK' && UTOnslaughtPowerNode(SquadObjective) != None && ONSTeamAI.Flag != None && ONSTeamAI.Flag.IsNearlyHome())
	{
		FlagBase = UTOnslaughtFlagBase(ONSTeamAI.Flag.HomeBase);
		if (FlagBase != None && FlagBase.ControllingNode != None && FlagBase.ControllingNode.ValidSpawnPointFor(Team.TeamIndex))
		{
			return FlagBase.ControllingNode;
		}
	}

	return Super.GetStartObjective(B);
}

defaultproperties
{
	MaxSquadSize=3
	bAddTransientCosts=true
}
