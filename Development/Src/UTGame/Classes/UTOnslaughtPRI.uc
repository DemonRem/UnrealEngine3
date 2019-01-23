/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtPRI extends UTPlayerReplicationInfo;

/* FIXMESTEVE
enum EMapPreference
{
	MAP_Never,
	MAP_WhenBodyStill,
	MAP_Immediately
};

var globalconfig EMapPreference ShowMapOnDeath;
*/
var bool bPendingMapDisplay;
var float PendingHealBonus; //node healing score bonus that hasn't yet been added to Score

// returns the powecore/node the player is currently standing on or in the node teleport trigger radius of,
// if that core/node is currently constructed for the same team as the player
simulated function UTOnslaughtObjective GetCurrentNode()
{
	local UTOnslaughtObjective Core;
	local Pawn P;

	P = Controller(Owner).Pawn;
	if (P != None)
	{
		if (P.Base != None)
		{
			Core = UTOnslaughtObjective(P.Base);
			if (Core == None)
			{
				Core = UTOnslaughtObjective(P.Base.Owner);
			}
		}

		if (Core == None)
		{
			foreach P.OverlappingActors(class'UTOnslaughtObjective', Core, P.VehicleCheckRadius)
			{
				break;
			}
		}
	}

	if (Core != None && Core.IsActive() && Core.DefenderTeamIndex == Team.TeamIndex)
	{
		return Core;
	}
	else
	{
		return None;
	}
}

	/* FIXMESTEVE

simulated event Timer()
{
	local PlayerController PC;

	if (bPendingMapDisplay)
	{
		PC = Worldinfo.GetLocalPlayerController();
		if (PC != None && !PC.bDemoOwner && PC.Player != None && GUIController(PC.Player.GUIController) != None && !GUIController(PC.Player.GUIController).bActive)
		{
			if (PC.Pawn != None && PC.Pawn.Health > 0) //never automatically pop up map while player is playing!
			{
				bPendingMapDisplay = false;
				SetTimer(0, false);
			}
			else if (ShowMapOnDeath == MAP_Immediately || PC.ViewTarget == None || PC.ViewTarget == PC || VSize(PC.ViewTarget.Velocity) < 100)
			{
				PC.bMenuBeforeRespawn = false;
				bPendingMapDisplay = false;
				SetTimer(0, false);
				PC.ShowMidGameMenu(false);
			}
			else
				PC.bMenuBeforeRespawn = true;
		}
		else
		{
			bPendingMapDisplay = False;
			SetTimer(0, false);
		}
	}

	Super.Timer();
}
*/

//scoring bonus for healing powernodes
//we wait until the healing bonus >= 1 point before adding to score to minimize ScoreEvent() calls
function AddHealBonus(float Bonus)
{
	PendingHealBonus += Bonus;
	if (PendingHealBonus >= 1.f)
	{
		Worldinfo.Game.ScoreObjective(self, PendingHealBonus);
		Worldinfo.Game.ScoreEvent(self, PendingHealBonus, "heal_powernode");
		PendingHealBonus = 0;
	}
}

defaultproperties
{
//	ShowMapOnDeath=MAP_WhenBodyStill
}
