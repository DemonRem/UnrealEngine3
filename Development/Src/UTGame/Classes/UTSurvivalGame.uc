/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTSurvivalGame extends UTTeamGame
	config(Game);

var config	bool 	bEndGameIfChampion;		// Ends the game if the player has reached the champion level

/* Survivor, Challengers and queue define who is currently in combat and who is waiting */

var		UTSurvivalPRI			Survivor;
var		UTSurvivalPRI			FirstOpponent;		// Who did this Survivor first play against
var array<UTSurvivalPRI> 		Challengers;
var array<UTSurvivalPRI>		Queue;

var config bool					bLoaded;			// Should the players spawn fully loaded
var array<string>				LoadedWeaponList;	// Weapons to preload

/**
 * Setup the game type.  Init game needs to insure that MinNetPlayers >=0
 */

event InitGame( string Options, out string ErrorMessage )
{
    Super.InitGame(Options, ErrorMessage);

	MinNetPlayers = max(MinNetPlayers,2);

    if (HasOption(Options, "Loaded"))
    	bLoaded = true;

}

/**
 * Update the Server details
 */

function GetServerDetails( out ServerResponseLine ServerState )
{
	super.GetServerDetails(ServerState);

	if (bLoaded)
	AddServerDetail( ServerState, "Loaded", "True" );

}


/**
 * During login, add the player to the queue
 */

event PostLogin( PlayerController NewPlayer )
{
	local UTSurvivalPRI PRI;

	super.PostLogin(NewPlayer);

	PRI = UTSurvivalPRI(NewPlayer.PlayerReplicationInfo);
	if (PRI!=None && !PRI.bOnlySpectator)
	{
		AddToQueue(PRI);
	}
}

/**
 * We need to handle adding bots to the queue
 */

function UTBot AddBot(optional string botName,optional bool bUseTeamIndex, optional int TeamIndex)
{
    local UTBot NewBot;

	NewBot = super.AddBot(botName,bUseTeamIndex,TeamIndex);
	if (NewBot!=none)
	{
		AddToQueue(UTSurvivalPRI(NewBot.PlayerReplicationInfo));
	}

	return NewBot;
}


/**
 * Gracefully handle players logging out
 */

function Logout( Controller Exiting )
{
	local  int i;
	local UTSurvivalPRI PRI;

	super.LogOut(Exiting);

	PRI = UTSurvivalPRI(Exiting.PlayerReplicationInfo);

	if (PRI == none)
		return;

    // Handle if the player was a survivor

	if (Survivor == PRI)
	{
		Survivor = none;
		NewSurvivor(none);
	}

	// Remove the player from the Challenger list

	for (i=0;i<Challengers.Length;i++)
	{
		if ( Challengers[i] == PRI )
		{
			Challengers.Remove(i,1);
			break;
		}
	}

	// Remove the player from the queue

	for (i=0;i<Queue.Length;i++)
	{
		if ( Queue[i] == PRI )
		{
			Queue.Remove(i,1);
			break;
		}
	}

	// Check the challenger count

	CheckChallengerCount();
}

/**
 * This function returns the next person in the queue and adjusts the QuePositions of everyone else
 */

function UTSurvivalPRI NextInQueue(optional bool bDontRemove)
{
	local int i;
	local UTSurvivalPRI PRI;

	if (Queue.Length<1)
		return none;

	// Pull the first player

	PRI = Queue[0];

	// Remove them if desired

	if (!bDontRemove)
	{
		Queue.Remove(0,1);
		for (i=0; i<Queue.Length ;i++)
			Queue[i].QueuePosition = i;
	}

	return PRI;
}

/**
 * This function Adds a player to the Queue
 */

function AddtoQueue(UTSurvivalPRI Who)
{
	local int i;

	// Make sure this player isn't a challenger or survivor

	if (Survivor==Who)
		Survivor = none;

	i = IsAChallenger(Who);
	if (i>=0)
		Challengers.Remove(i,1);

	// Add the player to the end of the queue

	i = Queue.Length;

	Queue.Length = i+1;
	Queue[i] = Who;
	Queue[i].QueuePosition = i;

	// Reset survivor information

	Queue[i].ConsecutiveKills = 0;
	Queue[i].CombatLevel =0;

    ChangeTeam(Controller(Who.Owner),255,true);
	Who.Owner.GotoState('InQueue');
}

/**
 * Someone has become the new survivor
 */

function NewSurvivor(UTSurvivalPRI Who)
{
	local Controller WhoOwner;
	local int Index;

	// If we don't have a survivor in mind, grab the first person from the queue

	if (Who==none)
	{
		Who = NextInQueue();

		if (Who==none)
		{
			`log("Error: Attempting to find a new survivor when there arn't any");
			return;		// Nobody in the game
		}
	}

	// Make sure we remove this player from the Challengers array

	Index = IsAChallenger(Who);
	if (Index>=0)
		Challengers.Remove(Index,1);


	// Make this guy the survivor

	WhoOwner = Controller(Who.Owner);
	if (WhoOwner != none)
	{
		ChangeTeam(WhoOwner,0,true);
		Survivor = Who;
		Survivor.QueuePosition = -1;	// Not in queue anymore
		RestartPlayer(WhoOwner);

	}

	// Clear the First Opponent field to start tracking anew

	FirstOpponent = none;


	// Get Challengers if needed

	CheckChallengerCount();
}

/**
 * New Challenger looks at the queue and spawns a player.  If there are not enough people
 * in the queue, add a bot
 */

function NewChallenger()
{
	local int i;
	local Controller ChallengerOwner;
	local UTSurvivalPRI Challenger;

	// If noone is in the queue and bots are allowed, add some bots

	if (Queue.Length==0)
	{
		if ( NumBots + NumPlayers < DesiredPlayerCount )
			AddBot();
		else
			return;
	}

	// Remove the player from the queue

	Challenger = NextInQueue();
	if (Challenger==none)
	{
		`log("ERROR: Attempting to get a challenger when there aren't any");
	}

	// Add them to the Challenger list and set them on the right team/etc.

	ChallengerOwner = Controller(Challenger.Owner);
	i = Challengers.Length;
	Challengers.Length = i+1;
	Challengers[i] = Challenger;

	Challenger.QueuePosition = -1;

	ChangeTeam(ChallengerOwner,1,true);
	RestartPlayer(ChallengerOwner);

	if (FirstOpponent==none)
		FirstOpponent = Challenger;
	else if (FirstOpponent==Challenger)	// We have Looped
		IncreaseCombatLevel();


	// Recheck the Challenger Count to see if we need to add more

	CheckChallengerCount();

}

/**
 * Increase the Survivors combat level by 1 because he survived the whole que
 */
function IncreaseCombatLevel()
{

	// Dead function for now.

}

/**
 * Checks the challenger count, and adds new challengers if needed
 */

function CheckChallengerCount()
{
	local int NumChallengersNeeded;

	// If there isn't a Survivor, there is no need for a challenger

	if (Survivor == none)
		return;

	NumChallengersNeeded = Survivor.CombatLevel+1;

	if (Challengers.Length < NumChallengersNeeded)
		NewChallenger();
}

/**
 * This function returns the index in to the challenger array of a PRI or -1 if he's not a challenger
 */

function int IsAChallenger(UTSurvivalPRI Who)
{
	local int i;

	if (Who!=none)
	{
		for (i=0;i<Challengers.Length;i++)
			if (Challengers[i] == who)
				return i;
	}

	return -1;
}

/**
 * Short-circuit the normal spawming system
 */

function StartHumans();
function StartBots();

/**
 * Instead of the normal spawning, spawn a New Survivor which will jump start everything
 */

function StartMatch()
{
	NewSurvivor(none);

	super.StartMatch();

	// Start the chain by getting a new survivor
}

function RestartPlayer(Controller aPlayer)
{
	local int i;
	local UTSurvivalPRI PRI;

	// Make sure this player is "in" the game right now and not in the queue

	PRI = UTSurvivalPRI(aPlayer.PlayerReplicationInfo);
	if (PRI == Survivor)
	{
		Super.RestartPlayer(aPlayer);
	}
	else
	{
		for (i=0;i<Challengers.Length;i++)
		{
			if (PRI == Challengers[i])
			{
				Super.RestartPlayer(aPlayer);
			}
		}
	}
}

// Handle Scoring

function ScoreKill(Controller Killer, Controller Other)
{
	local UTSurvivalPRI KillerPRI, VictimPRI;
	local int ChallengerIndex;

	super.ScoreKill(Killer, Other);

	KillerPRI = UTSurvivalPRI(Killer.PlayerReplicationInfo);
	VictimPRI = UTSurvivalPRI(Other.PlayerReplicationInfo);

	// If the Survivor was killed, pick a new one

	if (VictimPRI == Survivor)
	{
		ChallengerIndex = IsAChallenger(KillerPRI);
		if ( ChallengerIndex<0 )					// Killed by someone not a challenger
		{
			// FIXME: Find a better solution here for if/when we support mulitple players at 1 time

			KillerPRI = Challengers[0];
		}

		AddToQueue(Survivor);
		NewSurvivor(KillerPRI);
	}

	// Otherwise, if the Survivor made a kill and it wasn't a suicide on the other's part, Score it

	else if (KillerPRI == Survivor || Killer!=Other)
	{
		SurvivorGotAKill(Killer, Other);
		AddToQueue(VictimPRI);
	}
	else
		AddToQueue(VictimPRI);

}

/**
 *  This function is called each time a survivor scores a kill.  FIXME: Add more score points and log it with
 *  stats
 */

function SurvivorGotAKill(Controller Killer, Controller Challenger)
{
	// Don't track it if the survivor died as well

	if ( Killer != none && Killer.Pawn!=none && Killer.Pawn.Health>0 )
		Survivor.ConsecutiveKills++;
}


/**
 * We never end the game because of this
 */

function bool CheckMaxLives(PlayerReplicationInfo Scorer)
{
	return false;
}


state MatchInProgress
{
    function Timer()
    {
		// Play Startup Message

	Global.Timer();
		if ( !bFinalStartup )
		{
			bFinalStartup = true;
			PlayStartupMessage();
		}

		// Check the Clock

	if ( TimeLimit > 0 )
	{
	    GameReplicationInfo.bStopCountDown = false;
	    RemainingTime--;
	    GameReplicationInfo.RemainingTime = RemainingTime;
	    if ( RemainingTime % 60 == 0 )
		GameReplicationInfo.RemainingMinute = RemainingTime;
	    if ( RemainingTime <= 0 )
		EndGame(None,"TimeLimit");
	}

	ElapsedTime++;
	GameReplicationInfo.ElapsedTime = ElapsedTime;

		// Try to keep the game going.

		CheckChallengerCount();
		if (Survivor==none)
		{
			NewSurvivor(None);
		}
    }
}

function bool CanSpectate( PlayerController Viewer, PlayerReplicationInfo ViewTarget )
{
	local UTSurvivalPRI PRI;

	PRI = UTSurvivalPRI(ViewTarget);
	return ( (PRI != None) && !PRI.bOnlySpectator && !PRI.bIsSpectator && PRI.Team != none && PRI.Team.TeamIndex!=255 );
}


function bool TooManyBots(Controller botToRemove)
{
	return false;
}

function SetPlayerDefaults(Pawn PlayerPawn)
{
	local int i;

	super.SetPlayerDefaults(PlayerPawn);

	if (bLoaded)
	{
		// Give the weapons

		for (i=0;i<LoadedWeaponList.Length;i++)
			GiveWeapon(PlayerPawn, LoadedWeaponList[i]);

		// Max out the ammo

		if (UTInventoryManager(PlayerPawn.InvManager)!=none)
			UTInventoryManager(PlayerPawn.InvManager).AllAmmo();
	}
}

/**
 * Give a specified weapon to the Pawn.
 * If weapon is not carried by player, then it is created.
 * Weapon given is returned as the function's return parmater.
 */
function Weapon GiveWeapon( Pawn PlayerPawn, String WeaponClassStr )
{
	Local Weapon		Weap;
	local class<Weapon> WeaponClass;

	WeaponClass = class<Weapon>(DynamicLoadObject(WeaponClassStr, class'Class'));
	Weap		= Weapon(PlayerPawn.FindInventoryType(WeaponClass));
	if( Weap != None )
	{
		return Weap;
	}
	return Weapon(PlayerPawn.CreateInventory( WeaponClass ));
}

defaultproperties
{
    Acronym="SRV"
    MapPrefix="SRV"
	HUDType=class'UTGame.UTSurvivalHUD'
    PlayerReplicationInfoClass=class'UTGame.UTSurvivalPRI'

	LoadedWeaponList(0)="UTGame.UTWeap_FlakCannon"
	LoadedWeaponList(1)="UTGame.UTWeap_Minigun"
	LoadedWeaponList(2)="UTGame.UTWeap_RocketLauncher"
	LoadedWeaponList(3)="UTGame.UTWeap_ShockRifle"
	LoadedWeaponList(4)="UTGameContent.UTWeap_SniperRifle_Content"
	LoadedWeaponList(5)="UTGame.UTWeap_BioRifle"
	LoadedWeaponList(6)="UTGameContent.UTWeap_LinkGunContent"
}
