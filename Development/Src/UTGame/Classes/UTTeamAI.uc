/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// UTTeamAI.
// strategic team AI control for TeamGame
//
//=============================================================================
class UTTeamAI extends Info;

var UTTeamInfo Team;
var UTTeamInfo EnemyTeam;
var	int	NumSupportingPlayer;

var UTGameObjective Objectives; // list of objectives to be defended or attacked by this team
var UTGameObjective PickedObjective;	// objective that was picked from a list of equal priority objectives

var UTSquadAI Squads;
var UTSquadAI AttackSquad, FreelanceSquad;
var class<UTSquadAI> SquadType;
var int OrderOffset;
var name OrderList[8];

/* FIXMESTEVE
var Pickup SuperPickups[16];
var int NumSuperPickups;
*/

function PostBeginPlay()
{
	Super.PostBeginPlay();
	SetTimer(5.0,true);
}

function Timer()
{
	ReAssessStrategy();
}


function CriticalObjectiveWarning(UTGameObjective G, Pawn NewEnemy);

function bool SuperPickupAvailable(UTBot B)
{
	return false;
/* FIXMESTEVE

	local int i;
	local Pickup P;

	if ( NumSuperPickups == 0 )
	{
		ForEach DynamicActors(class'Pickup', P)
			if ( P.IsSuperItem() )
			{
				SuperPickups[NumSuperPickups] = P;
				NumSuperPickups++;
				if ( NumSuperPickups == 16 )
					break;
			}
	}

	if ( NumSuperPickups <= 0 )
	{
		NumSuperPickups = -1;
		return false;
	}

	for ( i=0; i<NumSuperPickups; i++ )
	{
		if ( (SuperPickups[i] != None) && SuperPickups[i].ReadyToPickup(0)
			&& B.SuperPickupNotSpokenFor(SuperPickups[i]) )
			return true;
	}

	if ( NumSuperPickups < 16 )
		return false;

	ForEach DynamicActors(class'Pickup', P)
		if ( P.IsSuperItem() )
		{
			if ( P.ReadyToPickup(2) )
				return true;
		}

	return false;
*/
}

function Reset()
{
	Super.Reset();
	PickedObjective = None;
}

function ClearEnemies()
{
	local UTSquadAI S;
	local int i;
	local UTBot M;

	Reset();
	for ( S=Squads; S!=None; S=S.NextSquad )
	{
		for ( i=0; i<8; i++ )
			S.Reset();
		for	( M=S.SquadMembers; M!=None; M=M.NextSquadMember )
			M.Reset();
	}
}

function UTSquadAI GetSquadLedBy(Controller C)
{
	local UTSquadAI S;

	for ( S=Squads; S!=None; S=S.NextSquad )
		if ( S.SquadLeader == C )
			return S;
	return None;
}

// ReAssessStrategy()
//Look at current strategic situation, and decide whether to update squad objectives

function ReAssessStrategy()
{
	local UTGameObjective O;
	local int PlusDiff, MinusDiff;

	if ( FreelanceSquad == None )
		return;

	// decide whether to play defensively or aggressively
	if ( WorldInfo.Game.TimeLimit > 0 )
	{
		PlusDiff = 0;
		MinusDiff = 2;
		if ( UTGame(WorldInfo.Game).RemainingTime < 180 )
			MinusDiff = 0;
	}
	else
	{
		PlusDiff = 2;
		MinusDiff = 2;
	}

	FreelanceSquad.bFreelanceAttack = false;
	FreelanceSquad.bFreelanceDefend = false;
	if ( Team.Score > EnemyTeam.Score + PlusDiff )
	{
		FreelanceSquad.bFreelanceDefend = true;
		O = GetLeastDefendedObjective(FreelanceSquad.SquadLeader);
	}
	else if ( Team.Score < EnemyTeam.Score - MinusDiff )
	{
		FreelanceSquad.bFreelanceAttack = true;
		O = GetPriorityAttackObjectiveFor(FreelanceSquad, FreelanceSquad.SquadLeader);
	}
	else
		O = GetPriorityFreelanceObjectiveFor(FreelanceSquad);

	if ( (O != None) && (O != FreelanceSquad.SquadObjective) )
		FreelanceSquad.SetObjective(O,true);
}

function NotifyKilled(Controller Killer, Controller Killed, Pawn KilledPawn)
{
	local UTSquadAI S;

	for ( S=Squads; S!=None; S=S.NextSquad )
		S.NotifyKilled(Killer,Killed,KilledPawn);
}

function FindNewObjectives(UTGameObjective DisabledObjective)
{
	local UTSquadAI S;

	for (S = Squads; S != None; S = S.NextSquad)
	{
		if (DisabledObjective == None || S.SquadObjective == DisabledObjective)
		{
			FindNewObjectiveFor(S, true);
		}
	}
}

// FindNewObjectiveFor()
//pick a new objective for a squad that has completed its current objective

function FindNewObjectiveFor(UTSquadAI S, bool bForceUpdate)
{
	local UTGameObjective O, Temp;

	if ( PlayerController(S.SquadLeader) != None )
		return;
	if ( S.bFreelance )
		O = GetPriorityFreelanceObjectiveFor(S);
	else if ( S.GetOrders() == 'Attack' )
		O = GetPriorityAttackObjectiveFor(S, S.SquadLeader);
	if ( O == None )
	{
		O = GetLeastDefendedObjective(S.SquadLeader);
		if ( (O != None) && (O.DefenseSquad != None) )
		{
			if ( S.GetOrders() == 'Attack' )
			{
				S.MergeWith(O.DefenseSquad);
				return;
			}
			else
			{
				Temp = O;
				O = GetPriorityAttackObjectiveFor(S, S.SquadLeader);
				if ( O == None )
				{
					S.MergeWith(Temp.DefenseSquad);
					return;
				}
			}
		}
	}
	if ( (O == None) && (S.bFreelance || (S.GetOrders() == 'Defend')) )
		O = GetPriorityAttackObjectiveFor(S, S.SquadLeader);
	S.SetObjective(O,bForceUpdate);
}

function RemoveSquad(UTSquadAI Squad)
{
	local UTSquadAI S;

	if (Squad == AttackSquad)
	{
		AttackSquad = None;
	}
	if (Squad == FreelanceSquad)
	{
		FreelanceSquad = None;
	}
	if ( Squad == Squads )
	{
		Squads = Squads.NextSquad;
	}
	else
	{
		for (S = Squads; S != None; S = S.NextSquad)
		{
			if (S.NextSquad == Squad)
			{
				S.NextSquad = S.NextSquad.NextSquad;
				return;
			}
		}
	}
}


function UTSquadAI FindSquadOf(Controller C)
{
	local UTSquadAI S;

	if ( UTBot(C) != None )
		return UTBot(C).Squad;

	for ( S=Squads; S!=None; S=S.NextSquad )
		if ( S.SquadLeader == C )
			return S;
	return None;
}

function bool FriendlyToward(Pawn Other)
{
	return WorldInfo.GRI.OnSameTeam(self,Other);
}

simulated function byte GetTeamNum()
{
	return Team.TeamIndex;
}

function SetObjectiveLists()
{
	local UTGameObjective O;

	foreach WorldInfo.AllNavigationPoints(class'UTGameObjective', O)
	{
		if (O.bFirstObjective)
		{
			Objectives = O;
			break;
		}
	}
}

function UTSquadAI FindHumanSquad()
{
	local UTSquadAI S;

	for ( S=Squads; S!=None; S=S.NextSquad )
		if ( S.SquadLeader.IsA('PlayerController') )
			return S;

	return None;
}

function UTSquadAI AddHumanSquad()
{
	local UTSquadAI S;
	local PlayerController P;

	S = FindHumanSquad();
	if ( S != None )
		return S;

	// add human squad
	foreach WorldInfo.AllControllers(class'PlayerController', P)
	{
		if (P.PlayerReplicationInfo.Team == Team && !P.PlayerReplicationInfo.bOnlySpectator)
		{
			return AddSquadWithLeader(P, None);
		}
	}

	return None;
}

function PutBotOnSquadLedBy(Controller C, UTBot B)
{
	local UTSquadAI S;

	for ( S=Squads; S!=None; S=S.NextSquad )
		if ( S.SquadLeader == C )
			break;

	if ( (S == None) && (PlayerController(C) != None) )
		S = AddSquadWithLeader(C,None);

	if ( S != None )
		S.AddBot(B);
}

function UTSquadAI AddSquadWithLeader(Controller C, UTGameObjective O)
{
	local UTSquadAI S;

	S = spawn(SquadType);
	S.Initialize(Team,O,C);
	S.NextSquad = Squads;
	Squads = S;
	return S;
}

function UTGameObjective GetLeastDefendedObjective(Controller InController)
{
	local UTGameObjective O, Best;
	local bool bCheckDistance;
	local float BestDistSq, NewDistSq;

	bCheckDistance = (InController != None) && (InController.Pawn != None);
	for ( O=Objectives; O!=None; O=O.NextObjective )
	{
		if ( (O.DefenderTeamIndex == Team.TeamIndex) && !O.IsDisabled() )
		{
			if ( (Best == None) || (Best.DefensePriority < O.DefensePriority) )
			{
				Best = O;
				BestDistSq = VSizeSq(Best.Location - InController.Pawn.Location);
			}
			else if ( Best.DefensePriority == O.DefensePriority )
			{
				// prioritize less defended or closer nodes
				NewDistSq = VSizeSq(O.Location - InController.Pawn.Location);
				if ( (Best.GetNumDefenders() > O.GetNumDefenders()) || (bCheckDistance && (NewDistSq < BestDistSq)) )
				{
					Best = O;
					BestDistSq = NewDistSq;
				}
			}
		}
	}
	return Best;
}

function UTGameObjective GetPriorityAttackObjectiveFor(UTSquadAI InAttackSquad, Controller InController)
{
	local UTGameObjective O;
	local bool bCheckDistance;
	local float BestDistSq, NewDistSq;

	bCheckDistance = (InController != None) && (InController.Pawn != None);

	if ( (PickedObjective != None) && PickedObjective.IsDisabled() )
		PickedObjective = None;
	if ( PickedObjective == None )
	{
		for ( O=Objectives; O!=None; O=O.NextObjective )
		{
			if ( (O.DefenderTeamIndex != Team.TeamIndex) && !O.IsDisabled() )
			{
				if ( (PickedObjective == None) || (PickedObjective.DefensePriority < O.DefensePriority) )
				{
					PickedObjective = O;
					BestDistSq = VSizeSq(PickedObjective.Location - InController.Pawn.Location);
				}
				else if ( bCheckDistance && (PickedObjective.DefensePriority == O.DefensePriority) )
				{
					// prioritize closer nodes
					NewDistSq = VSizeSq(O.Location - InController.Pawn.Location);
					if ( NewDistSq < BestDistSq )
					{
						PickedObjective = O;
						BestDistSq = NewDistSq;
					}
				}
			}
		}
	}
	return PickedObjective;
}

function UTGameObjective GetPriorityFreelanceObjectiveFor(UTSquadAI InFreelanceSquad)
{
	return GetPriorityAttackObjectiveFor(InFreelanceSquad, InFreelanceSquad.SquadLeader);
}

function bool PutOnDefense(UTBot B)
{
	local UTGameObjective O;

	O = GetLeastDefendedObjective(B);
	if ( O != None )
	{
		if ( O.DefenseSquad == None )
			O.DefenseSquad = AddSquadWithLeader(B, O);
		else
			O.DefenseSquad.AddBot(B);
		return true;
	}
	return false;
}

function PutOnOffense(UTBot B)
{
	if ( (AttackSquad == None) || (AttackSquad.Size >= AttackSquad.MaxSquadSize) )
		AttackSquad = AddSquadWithLeader(B, GetPriorityAttackObjectiveFor(None, B));
	else
		AttackSquad.AddBot(B);
}

function PutOnFreelance(UTBot B)
{
	if ( (FreelanceSquad == None) || (FreelanceSquad.Size >= FreelanceSquad.MaxSquadSize) )
		FreelanceSquad = AddSquadWithLeader(B, GetPriorityFreelanceObjectiveFor(None));
	else
		FreelanceSquad.AddBot(B);
	if ( !FreelanceSquad.bFreelance )
	{
	FreelanceSquad.bFreelance = true;
		NetUpdateTime = WorldInfo.TimeSeconds - 1;
	}
}


//SetBotOrders - sets bot's initial orders

//FIXME - need assault type pick leader when leader dies for attacking
//freelance squad - backs up defenders under attack, or joins in attacks

function SetBotOrders(UTBot NewBot)
{
	local UTSquadAI HumanSquad;
	local name NewOrders;

	if ( Objectives == None )
		SetObjectiveLists();

	if ( UTTeamGame(WorldInfo.Game).bForceAllRed )
		NewOrders = 'DEFEND';
	/* @FIXME: get singleplayer orders from somewhere
	else if ( (R==None) || R.NoRecommendation() )
	{
		// pick orders
		if ( Team.Size == 0 )
			OrderOffset = 0;
		NewOrders = OrderList[OrderOffset % 8];
		OrderOffset++;
	}
	else if ( R.RecommendDefense() )
		NewOrders = 'DEFEND';
	else if ( R.RecommendAttack() )
		NewOrders = 'ATTACK';
	else if ( R.RecommendSupport() )
		NewOrders = 'FOLLOW';
	else
		NewOrders = 'FREELANCE';
	*/
	else
	{
		// resset orders list if only bot on team
		//@FIXME: this still doesn't handle players joining/leaving in e.g. Players vs Bots screwing up orders over time
		if (Team.Size == 1)
		{
			OrderOffset = 0;
		}
		NewOrders = OrderList[OrderOffset % 8];
		OrderOffset++;
	}

	// `log(NewBot$" set Initial orders "$NewOrders);
	if ( (NewOrders == 'DEFEND') && PutOnDefense(NewBot) )
		return;

	if ( NewOrders == 'FREELANCE' )
	{
		PutOnFreelance(NewBot);
		return;
	}

	if ( NewOrders == 'ATTACK' )
	{
		PutOnOffense(NewBot);
		return;
	}

	if ( NewOrders == 'FOLLOW' )
	{
		// Follow any human player
		HumanSquad = AddHumanSquad();
		if ( HumanSquad != None )
		{
			HumanSquad.AddBot(NewBot);
			return;
		}
	}
	PutOnOffense(NewBot);
}

// SetOrders()
// Called when player gives orders to bot
function SetOrders(UTBot B, name NewOrders, Controller OrderGiver)
{
	local UTPlayerReplicationInfo PRI;

	PRI = UTPlayerReplicationInfo(B.PlayerReplicationInfo);
	if ( UTHoldSpot(B.DefensePoint) != None )
	{
		PRI.bHolding = false;
		B.FreePoint();
	}
	//`log("Team New orders "$NewOrders@OrderGiver);
	if ( NewOrders == 'Hold' )
	{
		PRI.bHolding = true;
		PutBotOnSquadLedBy(OrderGiver,B);
		B.DefensePoint = PlayerController(OrderGiver).ViewTarget.Spawn(class'UTHoldSpot');
		if ( Vehicle(PlayerController(OrderGiver).ViewTarget) != None )
			UTHoldSpot(B.DefensePoint).HoldVehicle = UTVehicle(PlayerController(OrderGiver).ViewTarget);
		if ( PlayerController(OrderGiver).ViewTarget.Physics == PHYS_Ladder )
			B.DefensePoint.SetPhysics(PHYS_Ladder);
	}
	else if ( NewOrders == 'Defend' )
		PutOnDefense(B);
	else if ( NewOrders == 'Attack' )
		PutOnOffense(B);
	else if ( NewOrders == 'Follow' )
	{
		B.FreePoint();
		PutBotOnSquadLedBy(OrderGiver,B);
	}
	else if ( NewOrders == 'Freelance' )
	{
		PutOnFreelance(B);
		return;
	}
}

function CallForHelp(UTBot B)
{
}

function RemoveFromTeam(Controller Other)
{
	local UTSquadAI S;

	if ( PlayerController(Other) != None )
	{
		for ( S=Squads; S!=None; S=S.NextSquad )
			S.RemovePlayer(PlayerController(Other));
	}
	else if ( UTBot(Other) != None )
	{
		for ( S=Squads; S!=None; S=S.NextSquad )
			S.RemoveBot(UTBot(Other));
	}
}

defaultproperties
{
	RemoteRole=ROLE_None
	SquadType=class'UTGame.UTSquadAI'

	Components.Remove(Sprite)

	OrderList(0)=FOLLOW
	OrderList(1)=ATTACK
	OrderList(2)=DEFEND
	OrderList(3)=FREELANCE
	OrderList(4)=FOLLOW
	OrderList(5)=ATTACK
	OrderList(6)=DEFEND
	OrderList(7)=FREELANCE
}
