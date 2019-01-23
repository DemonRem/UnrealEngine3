/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTSentryGunAIController extends AIController;

var byte TeamNum;
/** the controller that placed us and gets credit for our kills */
var Controller InstigatorController;

simulated function byte GetTeamNum()
{
	return TeamNum;
}

function Controller GetKillerController()
{
	return (InstigatorController != None && WorldInfo.GRI.OnSameTeam(self, InstigatorController)) ? InstigatorController : self;
}

/** picks a direction to look at while searching, doing a simple world trace to avoid staring at walls */
function PickViewDirection()
{
	local int i;

	do
	{
		FocalPoint = Pawn.Location + VRand() * 512.f;
	} until ((++i) > 5 || FastTrace(FocalPoint, Pawn.Location));
}

/** checks if the enemy has become invalid (dead or destroyed) and returns to 'Searching' state if so */
function CheckEnemyValid()
{
	if (Enemy == None || Enemy.bDeleteMe || Enemy.Health <= 0 || Enemy.Controller == None)
	{
		GotoState('Searching');
	}
}

auto state Searching
{
	event SeePlayer(Pawn Seen)
	{
		if (Seen.Health > 0 && !Seen.bAmbientCreature && !WorldInfo.GRI.OnSameTeam(self, Seen))
		{
			Enemy = Seen;
			Focus = Seen;
			GotoState('Attacking');
		}
	}

	event BeginState(name PreviousStateName)
	{
		SetTimer(1.0, true, 'PickViewDirection');
	}

	event EndState(name NextStateName)
	{
		ClearTimer('PickViewDirection');
	}
}

state Attacking
{
	event EnemyNotVisible()
	{
		GotoState('Searching');
	}

	event BeginState(name PreviousStateName)
	{
		Pawn.BotFire(false);
		SetTimer(1.0, true, 'CheckEnemyValid');
	}

	event EndState(name NextStateName)
	{
		Enemy = None;
		Focus = None;
		StopFiring();
		ClearTimer('CheckEnemyValid');
	}
}

defaultproperties
{
	bIsPlayer=false
}
