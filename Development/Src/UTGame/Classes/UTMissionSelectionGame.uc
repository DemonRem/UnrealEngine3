/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTMissionSelectionGame extends UTEntryGame;

enum ESinglePlayerMissionResult
{
	ESPMR_None,
	ESPMR_Win,
	ESPMR_Loss
};


var ESinglePlayerMissionResult LastMissionResult;
var string NextMissionURL;

/**
 * Figure out if the last match was won or lost
 */
event InitGame( string Options, out string ErrorMessage )
{
	local string InOpt;

	Super.InitGame(Options, ErrorMessage);

	InOpt = ParseOption( Options, "SPResult");
	if ( InOpt != "" )
	{
		LastMissionResult = bool(InOpt) ? ESPMR_Win : ESPMR_Loss;
	}
	else
	{
		LastMissionResult = ESPMR_None;
	}

	InOpt = ParseOption(Options,"SPI");
	if ( InOpt != "" )
	{
		SinglePlayerMissionIndex = int(InOpt);
	}
	else
	{
		SinglePlayerMissionIndex = -1;
	}
}

/**
 * We override InitGRI and setup the initial replication of the mission information.  We
 * also at this point request a menu to be brought up.  NOTE:  The GRI is responsible for
 * making sure everything it needs has been replicated and bring the menu up.  See
 * UTMissionGRI for more information.
 */
function InitGameReplicationInfo()
{
	local UTMissionGRI GRI;
	Super.InitGameReplicationInfo();

	GRI= UTMissionGRI(GameReplicationInfo);

	if (GRI != none)
	{
		GRI.LastMissionResult = LastMissionResult;
		GRI.LastMissionIndex = SinglePlayerMissionIndex;

		GRI.RequestMenu('SelectionMenu');
	}
}

/**
 * A new player has joined the server.  Figure out if that player
 * is the host.
 */
event PostLogin( PlayerController NewPlayer )
{
	local UTMissionSelectionPRI PRI;

	Super.PostLogin(NewPlayer);

	PRI = UTMissionSelectionPRI(NewPlayer.PlayerReplicationInfo);
	if ( PRI != None )
	{
		PRI.bIsHost = LocalPlayer(NewPlayer.Player) != none;
	}
}


/**
 * A new player has returned to the server via Seamless travel.  We need
 * to determine if they are the host and setup the PRI accordingly
 */
event HandleSeamlessTravelPlayer(Controller C)
{
	local UTMissionSelectionPRI PRI;
	local PlayerController PC;

	Super.HandleSeamlessTravelPlayer(C);

	PC = PlayerController(C);
	if ( PC != none )
	{
		PRI = UTMissionSelectionPRI( PC.PlayerReplicationInfo);
		if ( PRI != None )
		{
			PRI.bIsHost = LocalPlayer(PC.Player) != none;
		}
	}
}

/**
 * The host has accepted a mission.  Signal the clients to travel and wait for
 * them to sync up.
 */

function AcceptMission(int MissionIndex, string URL)
{
	// Pop up the menu on all clients and

    UTMissionGRI(GameReplicationInfo).CurrentMissionIndex = MissionIndex;
	UTMissionGRI(GameReplicationInfo).RequestMenu('BriefingMenu');
	NextMissionURL = URL;
	GotoState('SyncClients');
}

/**
 * We need to give clients time to bring the briefing menu up and become ready to travel.  We do
 * this by pausing here and waiting for bReadyToPlay to be set for each player
 */
state SyncClients
{
	// Check ready status 4x a second
	function BeginState(name PrevStateName)
	{
		Super.BeginState(PrevStateName);
		SetTimer(0.25,false);
	}

	// Look at the PRI list and see if anyone still isn't ready.
	function Timer()
	{
		local int i;
		local bool bReadyToGo;
		local PlayerReplicationInfo PRI;

		bReadyToGo = true;
		for (i=0;i<GameReplicationInfo.PRIArray.Length;i++)
		{
			PRI = GameReplicationInfo.PRIArray[i];
			if ( !PRI.bOnlySpectator && !PRI.bReadyToPlay && !PRI.bBot)
			{
				bReadyToGo = false;
			}
		}

		if (bReadyToGo)
		{

			WorldInfo.ServerTravel(NextMissionURL);
		}
	}
}



defaultproperties
{
	PlayerReplicationInfoClass=class'UTMissionSelectionPRI'
	GameReplicationInfoClass=class'UTMissionGRI'

	PlayerControllerClass=class'UTGame.UTMissionPlayerController'
	ConsolePlayerControllerClass=class'UTGame.UTMissionPlayerController'


	bExportMenuData=false
}
