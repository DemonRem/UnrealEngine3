/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtObjective extends UTGameObjective
	abstract
	native(Onslaught)
	nativereplication;

var float           LastAttackMessageTime;
var float           LastAttackTime;
var float           LastAttackExpirationTime;
var float           LastAttackAnnouncementTime;
var int				LastAttackSwitch;

/** link distance from each team's powercore, used for automatic player respawn selection */
var byte            FinalCoreDistance[2];

var UTPlayerReplicationInfo LastDamagedBy;
var Pawn LastAttacker;

var(ObjectiveHealth) float DamageCapacity;
var repnotify float Health;
var(ObjectiveHealth) float LinkHealMult;			// If > 0, Link Gun secondary heals an amount equal to its damage times this

/** true when in Neutral state */
var bool bIsNeutral;

cpptext
{
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
}

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		Health;
}

function InitCloseActors()
{
	PlayerStarts.length = 0;
	VehicleFactories.length = 0;
}

function TarydiumBoost(float Quantity);

simulated function bool PoweredBy(byte TeamNum)
{
	return false;
}

function bool LegitimateTargetOf(UTBot B)
{
	return false;
}

function bool HasActiveDefenseSystem()
{
	return false;
}

function ClearUnderAttack()
{
	SetUnderAttack(false);
}

function SetUnderAttack(bool bNewUnderAttack)
{
	bUnderAttack = bNewUnderAttack;
	if (bUnderAttack)
	{
		SetTimer(5.0, false, 'ClearUnderAttack');
	}
	else
	{
		ClearTimer('ClearUnderAttack');
	}
}

simulated event bool IsConstructing()
{
	return false;
}

simulated function bool IsNeutral()
{
	return bIsNeutral;
}

function bool IsCurrentlyDestroyed()
{
	return false;
}

function bool HasUsefulVehicles(Controller Asker)
{
	return false;
}

function bool LinkedToCoreConstructingFor(byte Team)
{
	return false;
}

simulated event bool IsActive()
{
	return true;
}

simulated function UpdateEffects(bool bPropagate)
{
}

function bool TooClose(UTBot B)
{
	local UTBot SquadMate;
	local int R;

	if ( (VSize(Location - B.Pawn.Location) < 2*B.Pawn.GetCollisionRadius()) && (PathList.Length > 1) )
	{
		//standing right on top of it, move away a little
		B.GoalString = "Move away from "$self;
		R = Rand(PathList.Length-1);
		B.MoveTarget = PathList[R].End.Nav;
		for ( SquadMate=B.Squad.SquadMembers; SquadMate!=None; SquadMate=SquadMate.NextSquadMember )
		{
			if ( (SquadMate.Pawn != None) && (VSize(SquadMate.Pawn.Location - B.MoveTarget.Location) < B.Pawn.GetCollisionRadius()) )
			{
				B.MoveTarget = PathList[R+1].End.Nav;
				break;
			}
		}
		B.SetAttractionState();
		return true;
	}
	return false;
}

// if bot is in important vehicle, and other bots can do the dirty work,
// return false if within get out distance
function bool StandGuard(UTBot B)
{
	local UTBot SquadMate;
	local float Dist;
	local int i;
	local UTVehicle BotVehicle;
	local UTVehicle_Deployable DeployableVehicle;

	if (DefenderTeamIndex != B.PlayerReplicationInfo.Team.TeamIndex && DefenderTeamIndex < 2)
	{
		return false;
	}

	BotVehicle = UTVehicle(B.Pawn);
	if (BotVehicle != None && BotVehicle.ImportantVehicle())
	{
		Dist = VSize(BotVehicle.Location - Location);
		if (ReachedParkingSpot(BotVehicle) || (Dist < BotVehicle.ObjectiveGetOutDist && B.LineOfSightTo(self)))
		{
			if (BotVehicle.bKeyVehicle && BotVehicle.CanAttack(self))
			{
				DeployableVehicle = UTVehicle_Deployable(B.Pawn);
				if (DeployableVehicle != None && !DeployableVehicle.IsDeployed())
				{
					DeployableVehicle.ServerToggleDeploy();
				}
				return true;
			}
			if ( DefenderTeamIndex == B.PlayerReplicationInfo.Team.TeamIndex ||
				(B.Enemy != None && WorldInfo.TimeSeconds - B.LastSeenTime < 2) )
			{
				return true;
			}

			// check if there's a passenger
			if (BotVehicle.Seats.length > 1)
			{
				for (i = 1; i < BotVehicle.Seats.length; i++)
				{
					if (BotVehicle.Seats[i].SeatPawn != None)
					{
						SquadMate = UTBot(BotVehicle.Seats[i].SeatPawn.Controller);
						if (SquadMate != None)
						{
							BotVehicle.Seats[i].SeatPawn.DriverLeave(false);
							return true;
						}
					}
				}
			}

			// check if there's another bot around to do it
			for (SquadMate = B.Squad.SquadMembers; SquadMate != None; SquadMate = SquadMate.NextSquadMember)
			{
				if ( SquadMate.Pawn != None && (UTVehicle(SquadMate.Pawn) == None || !UTVehicle(SquadMate.Pawn).ImportantVehicle())
					&& VSize(SquadMate.Pawn.Location - Location) < Dist + 2000.0 && SquadMate.RouteGoal == self )
				{
					return true;
				}
			}
		}
	}

	return false;
}

/** Onslaught Objectives are considered critical if their health is < 20% of max */
simulated event bool IsCritical()
{
	if ( Health < DamageCapacity * 0.2 )
	{
		return true;
	}
	return false;
}

simulated function name GetHudMapStatus()
{
	if ( IsUnderAttack() )
	{
		return 'UnderAttack';
	}
	else if ( !IsDisabled() && !IsNeutral() )
	{
		if ( IsConstructing() )
		{
			return 'Building';
		}
		else if ( IsCritical() )
		{
			return 'Critical';
		}
	}

	return Super.GetHudMapStatus();
}

defaultproperties
{
	bStatic=false
}



