/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCinematicGame extends UTEntryGame;

function StartMatch()
{
	local Actor A;

	// tell all actors the game is starting
	ForEach AllActors(class'Actor', A)
	{
		A.MatchStarting();
	}

	bWaitingToStartMatch = false;

	// Notify all clients that the match has begun
	GameReplicationInfo.StartMatch();

	// fire off any level startup events
	WorldInfo.NotifyMatchStarted();
}

event InitGame( string Options, out string ErrorMessage )
{
	Super(UTGame).InitGame(Options, ErrorMessage);
}

event HandleSeamlessTravelPlayer(Controller C)
{
	local UTBot B;

	// make sure bots get a new squad
	B = UTBot(C);
	if ( B != None )
	{
		KillBot(B);
		return;
	}
	super.HandleSeamlessTravelPlayer(C);
}

event PostLogin( PlayerController NewPlayer )
{
	Super.PostLogin(NewPlayer);
	StartMatch();
}

event PostSeamlessTravel()
{
	StartMatch();
}

defaultproperties
{
	HUDType=class'Engine.hud'
}
