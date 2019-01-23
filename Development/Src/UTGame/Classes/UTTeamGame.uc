/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTTeamGame extends UTDeathmatch;

var globalconfig	bool		bPlayersBalanceTeams;	// players balance teams
var config			bool		bAllowNonTeamChat;
var 				bool		bScoreTeamKills;
var 				bool		bSpawnInTeamArea;		// players spawn in marked team playerstarts
var					bool		bScoreVictimsTarget;	// Should we check a victims target for bonuses
var					bool		bForceAllRed;			// for AI testing

var					float		FriendlyFireScale;		//scale friendly fire damage by this value
var					float		TeammateBoost;
var					UTTeamInfo	Teams[2];
var					string		CustomTeamName[2];		// when specific pre-designed teams are specified on the URL
var					class<UTTeamAI> TeamAIType[2];
var					string		TeamFactions[2];

var class<UTTeamScoreMessage>	TeamScoreMessageClass;

/** If a player requests a team change, but team balancing prevents it, we allow a swap within a few seconds */
var	PlayerController	PendingTeamSwap;
var	float				SwapRequestTime;

function PreBeginPlay()
{
	Super.PreBeginPlay();

	CreateTeam(0);
	CreateTeam(1);
	Teams[0].AI.EnemyTeam = Teams[1];
	Teams[1].AI.EnemyTeam = Teams[0];
}

event PostLogin ( playerController NewPlayer )
{
	local Actor A;

	Super.PostLogin(NewPlayer);

	// If we are a local player, send the initial you are on team XX notice.  We have
	// to do this here because when the inital set team occurs before the player has been
	// initialized.

	if ( NewPlayer.IsLocalPlayerController() )
	{
		NewPlayer.ReceiveLocalizedMessage( class'UTTeamGameMessage', NewPlayer.PlayerReplicationInfo.Team.TeamIndex+1, NewPlayer.PlayerReplicationInfo);
	}


	if ( LocalPlayer(NewPlayer.Player) == None )
		return;

	// if local player, notify level actors
	ForEach AllActors(class'Actor', A)
		A.NotifyLocalPlayerTeamReceived();
}

function FindNewObjectives( UTGameObjective DisabledObjective )
{
	// have team AI retask bots
	Teams[0].AI.FindNewObjectives(DisabledObjective);
	Teams[1].AI.FindNewObjectives(DisabledObjective);
}

/* create a player team, and fill from the team roster
*/
function CreateTeam(int TeamIndex)
{
	local class<UTTeamInfo> RosterClass;

    if ( CustomTeamName[TeamIndex] != "" )
		RosterClass = class<UTTeamInfo>(DynamicLoadObject(CustomTeamName[TeamIndex],class'Class'));
	else
		RosterClass = class<UTTeamInfo>(DynamicLoadObject(DefaultEnemyRosterClass,class'Class'));

	Teams[TeamIndex] = spawn(RosterClass);
	Teams[TeamIndex].Faction = TeamFactions[TeamIndex];
	Teams[TeamIndex].Initialize(0.5*DesiredPlayerCount);
	Teams[TeamIndex].TeamIndex = TeamIndex;
	Teams[TeamIndex].AI = Spawn(TeamAIType[TeamIndex]);
	Teams[TeamIndex].AI.Team = Teams[TeamIndex];
	GameReplicationInfo.SetTeam(TeamIndex, Teams[TeamIndex]);
	Teams[TeamIndex].AI.SetObjectiveLists();
}

exec function AddRedBots(int Num)
{
	// Disable auto balancing of bot teams.
	bCustomBots=true;

	DesiredPlayerCount = Clamp(DesiredPlayerCount+Num, 1, 32);
	while ( (NumPlayers + NumBots < DesiredPlayerCount) && AddBot(,true,0) != none )
	{
		`log("added red bot");
	}
}

exec function AddBlueBots(int Num)
{
	// Disable auto balancing of bot teams.
	bCustomBots=true;

	DesiredPlayerCount = Clamp(DesiredPlayerCount+Num, 1, 32);
	while ( (NumPlayers + NumBots < DesiredPlayerCount) && AddBot(,true,1) != none )
	{
		`log("added blue bot");
	}
}

/** return a value based on how much this pawn needs help */
function int GetHandicapNeed(Pawn Other)
{
	local int ScoreDiff;

	if ( (Other.PlayerReplicationInfo == None) || (Other.PlayerReplicationInfo.Team == None) )
	{
		return 0;
	}

	// base handicap on how far team is behind
	ScoreDiff = Teams[1 - Other.PlayerReplicationInfo.Team.TeamIndex].Score - Other.PlayerReplicationInfo.Team.Score;
	if ( ScoreDiff < 5 )
	{
		// team is ahead or close
		return 0;
	}
	return ScoreDiff/5;
}

function UTTeamInfo GetBotTeam(optional int TeamBots,optional bool bUseTeamIndex, optional int TeamIndex)
{
	local int first, second;
	local PlayerController PC;

	if( bUseTeamIndex )
	{
		return Teams[TeamIndex];
	}

	if ( bForceAllRed )
	{
		return Teams[0];
	}

	if ( bPlayersVsBots && (WorldInfo.NetMode != NM_Standalone) )
	{
		return Teams[0];
	}

	if ( WorldInfo.NetMode == NM_Standalone )
	{
		if ( Teams[0].AllBotsSpawned() )
	    {
		    if ( !Teams[1].AllBotsSpawned() )
			{
			    return Teams[1];
			}
	    }
	    else if ( Teams[1].AllBotsSpawned() )
	    {
		    return Teams[0];
		}
	}

	second = 1;
	// always imbalance teams in favor of bot team in single player
	if (  WorldInfo.NetMode == NM_Standalone )
	{
		ForEach LocalPlayerControllers(class'PlayerController', PC)
		{
			if ( (PC.PlayerReplicationInfo.Team != None) && (PC.PlayerReplicationInfo.Team.TeamIndex == 1) )
			{
				first = 1;
				second = 0;
			}
			break;
		}
	}
	if ( Teams[first].Size < Teams[second].Size )
	{
		return Teams[first];
	}
	else
	{
		return Teams[second];
	}
}

static function PrecacheGameAnnouncements(UTAnnouncer Announcer)
{
	Super.PrecacheGameAnnouncements(Announcer);

	default.TeamScoreMessageClass.static.PrecacheGameAnnouncements(Announcer, default.Class);
	class'UTTeamGameMessage'.static.PrecacheGameAnnouncements(Announcer, default.Class);
}

function int LevelRecommendedPlayers()
{
	/* //@todo steve - implement
	local int Num;

		if (CurrentGameProfile == None)
			Num = Min(12,(WorldInfo.IdealPlayerCountMax + WorldInfo.IdealPlayerCountMin)/2);
		if (Num % 2 == 1)
		{
			Num++;
		}
		return Num;
	*/
	return 1;
}

// check if all other players are out
function bool CheckMaxLives(PlayerReplicationInfo Scorer)
{
    local Controller C;
    local PlayerReplicationInfo Living;
    local bool bNoneLeft;

    if ( MaxLives > 0 )
    {
		if ( (Scorer != None) && !Scorer.bOutOfLives )
			Living = Scorer;
	bNoneLeft = true;
	foreach WorldInfo.AllControllers(class'Controller', C)
	{
	    if ( (C.PlayerReplicationInfo != None) && C.bIsPlayer
		&& !C.PlayerReplicationInfo.bOutOfLives
		&& !C.PlayerReplicationInfo.bOnlySpectator )
	    {
		if ( Living == None )
		{
			Living = C.PlayerReplicationInfo;
		}
		else if ( (C.PlayerReplicationInfo != Living) && (C.PlayerReplicationInfo.Team != Living.Team) )
		{
    	        	bNoneLeft = false;
	            	break;
		}
	    }
	}
	if ( bNoneLeft )
	{
			if ( Living != None )
				EndGame(Living,"LastMan");
			else
				EndGame(Scorer,"LastMan");
			return true;
		}
    }
    return false;
}

// Parse options for this game...
event InitGame( string Options, out string ErrorMessage )
{
	local string InOpt;
	local class<UTTeamAI> InType;
	//local string RedSymbolName,BlueSymbolName;
	//local texture NewSymbol;

	Super.InitGame(Options, ErrorMessage);
	InOpt = ParseOption( Options, "RedAI");
	if ( InOpt != "" )
	{
		InType = class<UTTeamAI>(DynamicLoadObject(InOpt, class'Class'));
		if ( InType != None )
			TeamAIType[0] = InType;
	}

	InOpt = ParseOption( Options, "BlueAI");
	if ( InOpt != "" )
	{
		InType = class<UTTeamAI>(DynamicLoadObject(InOpt, class'Class'));
		if ( InType != None )
			TeamAIType[1] = InType;
	}

	InOpt = ParseOption( Options, "AllRed");
	if ( InOpt != "" )
	{
		bForceAllRed = true;
	}

/* //@todo steve
	// get passed in teams
	CustomTeamName[0] = ParseOption( Options, "RedTeam");
	CustomTeamName[1] = ParseOption( Options, "BlueTeam");

	if ( (CustomTeamName[0] != "") || (CustomTeamName[1] != "") )
	{
		bCustomBots = true;
		if ( CustomTeamName[0] == "" )
			CustomTeamName[0] = "UTGame.UTTeamBlueConfigured";
		if ( CustomTeamName[1] == "" )
			CustomTeamName[1] = "UTGame.UTTeamRedConfigured";
	}
	// set teamsymbols (optional)
	RedSymbolName = ParseOption( Options, "RedTeamSymbol");
	BlueSymbolName = ParseOption( Options, "BlueTeamSymbol");
	if ( RedSymbolName != "" )
	{
		NewSymbol = Texture(DynamicLoadObject(RedSymbolName,class'Texture'));
		if ( NewSymbol != None )
			TempSymbols[0] = NewSymbol;
	}
	if ( BlueSymbolName != "" )
	{
		NewSymbol = Texture(DynamicLoadObject(BlueSymbolName,class'Texture'));
		if ( NewSymbol != None )
			TempSymbols[1] = NewSymbol;
	}
*/
	InOpt = ParseOption(Options, "BalanceTeams");
	if ( InOpt != "" )
	{
		bPlayersBalanceTeams = bool(InOpt);
	}

	TeamFactions[0] = ParseOption(Options, "RedFaction");
	TeamFactions[1] = ParseOption(Options, "BlueFaction");
}

function bool TooManyBots(Controller botToRemove)
{
	local TeamInfo BotTeam, OtherTeam;

	if ( bForceAllRed )
		return false;
	if ( (!bPlayersVsBots || (WorldInfo.NetMode == NM_Standalone)) && (UTBot(botToRemove) != None) &&
		(!bCustomBots || (WorldInfo.NetMode != NM_Standalone)) && botToRemove.PlayerReplicationInfo.Team != None )
	{
		BotTeam = botToRemove.PlayerReplicationInfo.Team;
		OtherTeam = Teams[1-BotTeam.TeamIndex];
		if ( OtherTeam.Size < BotTeam.Size - 1 )
		{
			return true;
		}
		else if ( OtherTeam.Size > BotTeam.Size )
		{
			return false;
		}
	}
	if ( (WorldInfo.NetMode != NM_Standalone) && bPlayersVsBots )
		return ( NumBots > Min(16,BotRatio*NumPlayers) );
	if ( bPlayerBecameActive )
	{
		bPlayerBecameActive = false;
		return true;
	}
	return ( NumBots + NumPlayers > DesiredPlayerCount );
}

/* For TeamGame, tell teams about kills rather than each individual bot
*/
function NotifyKilled(Controller Killer, Controller KilledPlayer, Pawn KilledPawn)
{
	Teams[0].AI.NotifyKilled(Killer,KilledPlayer,KilledPawn);
	Teams[1].AI.NotifyKilled(Killer,KilledPlayer,KilledPawn);
}

function bool CheckEndGame(PlayerReplicationInfo Winner, string Reason)
{
	local Controller P;
    local bool bLastMan;

	if ( bOverTime )
	{
		if ( Numbots + NumPlayers == 0 )
			return true;
		bLastMan = true;
		foreach WorldInfo.AllControllers(class'Controller', P)
		{
			if ( (P.PlayerReplicationInfo != None) && !P.PlayerReplicationInfo.bOutOfLives )
			{
				bLastMan = false;
				break;
			}
		}
		if ( bLastMan )
			return true;
	}

    bLastMan = ( Reason ~= "LastMan" );
	if ( !bLastMan && CheckModifiedEndGame(Winner, Reason) )
		return false;

	if ( bTeamScoreRounds )
	{
		if ( Winner != None )
		{
			Winner.Team.Score += 1;
			Winner.Team.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		}
	}
	else if ( !bLastMan && (Teams[1].Score == Teams[0].Score) )
	{
		// tie
		if ( !bOverTimeBroadcast )
		{
			StartupStage = 7;
			PlayStartupMessage();
			bOverTimeBroadcast = true;
		}
		return false;
	}
	if ( bLastMan )
		GameReplicationInfo.Winner = Winner.Team;
	else if ( Teams[1].Score > Teams[0].Score )
		GameReplicationInfo.Winner = Teams[1];
	else
		GameReplicationInfo.Winner = Teams[0];

	if ( Winner == None )
	{
		foreach WorldInfo.AllControllers(class'Controller', P)
		{
			if ( (P.PlayerReplicationInfo != None) && (P.PlayerReplicationInfo.Team == GameReplicationInfo.Winner)
				&& ((Winner == None) || (P.PlayerReplicationInfo.Score > Winner.Score)) )
			{
				Winner = P.PlayerReplicationInfo;
			}
		}
	}

	EndTime = WorldInfo.TimeSeconds + EndTimeDelay;

	SetEndGameFocus(Winner);
	return true;
}

function SetEndGameFocus(PlayerReplicationInfo Winner)
{
	local Controller P;

	if ( Winner != None )
		EndGameFocus = Controller(Winner.Owner).Pawn;
	if ( EndGameFocus != None )
		EndGameFocus.bAlwaysRelevant = true;

	foreach WorldInfo.AllControllers(class'Controller', P)
	{
		P.GameHasEnded( EndGameFocus, (P.PlayerReplicationInfo != None) && (P.PlayerReplicationInfo.Team == GameReplicationInfo.Winner) );
	}
}

/**
 * returns true if Viewer is allowed to spectate ViewTarget
 **/
function bool CanSpectate( PlayerController Viewer, PlayerReplicationInfo ViewTarget )
{
	if ( (ViewTarget == None) || ViewTarget.bOnlySpectator )
		return false;
	return ( Viewer.PlayerReplicationInfo.bOnlySpectator || (ViewTarget.Team == Viewer.PlayerReplicationInfo.Team) );
}

/* Return a picked team number if none was specified
*/
function byte PickTeam(byte num, Controller C)
{
	local UTTeamInfo SmallTeam, BigTeam, NewTeam;
	local Controller B;
	local int BigTeamBots, SmallTeamBots;

	if ( bForceAllRed )
		return 0;

	if ( bPlayersVsBots && (WorldInfo.NetMode != NM_Standalone) )
	{
		if ( PlayerController(C) != None )
			return 1;
		return 0;
	}

	SmallTeam = Teams[0];
	BigTeam = Teams[1];

	if ( SmallTeam.Size > BigTeam.Size )
	{
		SmallTeam = Teams[1];
		BigTeam = Teams[0];
	}

	if ( num < 2 )
	{
		NewTeam = Teams[num];
	}

	if ( NewTeam == None )
		NewTeam = SmallTeam;
	else if ( bPlayersBalanceTeams && (WorldInfo.NetMode != NM_Standalone) && (PlayerController(C) != None) )
	{
		if ( SmallTeam.Size < BigTeam.Size )
			NewTeam = SmallTeam;
		else
		{
			// count number of bots on each team
			foreach WorldInfo.AllControllers(class'Controller', B)
			{
				if ( (B.PlayerReplicationInfo != None) && B.PlayerReplicationInfo.bBot )
				{
					if ( B.PlayerReplicationInfo.Team == BigTeam )
					{
						BigTeamBots++;
					}
					else if ( B.PlayerReplicationInfo.Team == SmallTeam )
					{
						SmallTeamBots++;
					}
				}
			}

			if ( BigTeamBots > 0 )
			{
				// balance the number of players on each team
				if ( SmallTeam.Size - SmallTeamBots < BigTeam.Size - BigTeamBots )
					NewTeam = SmallTeam;
				else if ( BigTeam.Size - BigTeamBots < SmallTeam.Size - SmallTeamBots )
					NewTeam = BigTeam;
				else if ( SmallTeamBots == 0 )
					NewTeam = BigTeam;
			}
			else if ( SmallTeamBots > 0 )
				NewTeam = SmallTeam;
			else if ( (C.PlayerReplicationInfo != None) && (C.PlayerReplicationInfo.Team != None) )
			{
				// don't allow team changes if teams are even, and player is already on a team
				NewTeam = UTTeamInfo(C.PlayerReplicationInfo.Team);
			}
		}
	}

	return NewTeam.TeamIndex;
}

/** ChangeTeam()
* verify whether controller Other is allowed to change team, and if so change his team by calling SetTeam().
* @param Other:  the controller which wants to change teams
* @param num:  the teamindex of the desired team.  If 255, pick the smallest team.
* @param bNewTeam:  if true, broadcast team change notification
*/
function bool ChangeTeam(Controller Other, int num, bool bNewTeam)
{
	local UTTeamInfo NewTeam, PendingTeam;

	// no team changes after initial change if single player campaign
	if ( (SinglePlayerMissionIndex != -1) && (Other.PlayerReplicationInfo.Team != None) )
	{
		return false;
	}

	// check if only allow team changes before match starts
	if ( bMustJoinBeforeStart && GameReplicationInfo.bMatchHasBegun )
		return false;

	// don't add spectators to teams
	if ( Other.IsA('PlayerController') && Other.PlayerReplicationInfo.bOnlySpectator )
	{
		Other.PlayerReplicationInfo.Team = None;
		return true;
	}

	NewTeam = (num < 255) ? Teams[PickTeam(num,Other)] : None;

	// check if already on this team
	if ( Other.PlayerReplicationInfo.Team == NewTeam )
	{
		// may have returned current team if not allowed to switch
		// if not allowed to switch, set up or complete proposed swap
		if ( (num < 2) && (num != NewTeam.TeamIndex) && !bPlayersVsBots && (PlayerController(Other) != None)
			&& (UTTeamInfo(Other.PlayerReplicationInfo.Team) != None) )
		{
			// check if swap request is pending
			if ( PendingTeamSwap != None )
			{
				if ( PendingTeamSwap.bDeleteMe || (WorldInfo.TimeSeconds - SwapRequestTime > 8.0) )
				{
					PendingTeamSwap = None;
				}
				else if ( PendingTeamSwap.PlayerReplicationInfo.Team.TeamIndex == num )
				{
					// do the swap!
					PendingTeam = UTTeamInfo(PendingTeamSwap.PlayerReplicationInfo.Team);
					if (PendingTeam != None )
					{
						SetTeam(PendingTeamSwap, UTTeamInfo(Other.PlayerReplicationInfo.Team), true);
						if ( PendingTeamSwap.Pawn != None )
							PendingTeamSwap.Pawn.PlayerChangedTeam();
						PendingTeamSwap = None;
						SetTeam(Other, PendingTeam, bNewTeam);
						return true;
					}
				}
			}

			// set pending swap request
			PendingTeamSwap = PlayerController(Other);
			SwapRequestTime = WorldInfo.TimeSeconds;

			// broadcast swap request to members of other team
			BroadcastLocalizedTeam(num, Other,class'UTTeamGameMessage', 0, PendingTeamSwap.PlayerReplicationInfo);
		}
		return false;
	}

	// set the new team for Other
	SetTeam(Other, NewTeam, bNewTeam);
	return true;
}

/** SetTeam()
* Change Other's team to NewTeam.
* @param Other:  the controller which wants to change teams
* @param NewTeam:  the desired team.
* @param bNewTeam:  if true, broadcast team change notification
*/
function SetTeam(Controller Other, UTTeamInfo NewTeam, bool bNewTeam)
{
	local Actor A;
	local UTGameReplicationInfo GRI;
	local UTPlayerReplicationInfo PRI;

	if (Other.PlayerReplicationInfo.Team != None || !ShouldSpawnAtStartSpot(Other))
	{
		// clear the StartSpot, which was a valid start for his old team
		Other.StartSpot = None;
	}

	PRI = UTPlayerReplicationInfo(Other.PlayerReplicationInfo);
	// remove the player's old mesh before we switch teams, so that in Standalone/Listen server mode
	// the about-to-be-killed pawn doesn't switch team colors
	if (WorldInfo.NetMode != NM_DedicatedServer && PRI != None)
	{
		PRI.SetCharacterMesh(None);
	}

	// remove the controller from his old team
	if ( Other.PlayerReplicationInfo.Team != None )
	{
		Other.PlayerReplicationInfo.Team.RemoveFromTeam(Other);
		Other.PlayerReplicationInfo.Team = none;
	}

	if ( NewTeam==None || (NewTeam!= none && NewTeam.AddToTeam(Other)) )
	{
		if ( (NewTeam!=None) && ((WorldInfo.NetMode != NM_Standalone) || (PlayerController(Other) == None) || (PlayerController(Other).Player != None)) )
			BroadcastLocalizedMessage( GameMessageClass, 3, Other.PlayerReplicationInfo, None, NewTeam );

		if ( bNewTeam && PlayerController(Other)!=None )
			GameEvent('TeamChange',NewTeam.TeamIndex,Other.PlayerReplicationInfo);
	}

	if ( (PlayerController(Other) != None) && (LocalPlayer(PlayerController(Other).Player) != None) )
	{
		// if local player, notify level actors
		ForEach AllActors(class'Actor', A)
			A.NotifyLocalPlayerTeamReceived();
	}

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		// regenerate character mesh for new team
		GRI = UTGameReplicationInfo(WorldInfo.GRI);
		if (GRI != None && PRI != None)
		{
			GRI.ProcessCharacterData(PRI);
		}
	}

	// If we are a local player, notify us of the team change.  Note, we have to
	// check to make sure the PC's Player isn't none since the first time through,
	// this will be called from within login and before the player is ready.  That
	// initial instance is handled in PostLogin().

	if (Other.PlayerReplicationInfo.Team != None && Other.IsLocalPlayerController())
	{
		PlayerController(Other).ReceiveLocalizedMessage( class'UTTeamGameMessage', Other.PlayerReplicationInfo.Team.TeamIndex+1, Other.PlayerReplicationInfo);
	}
}

/** RatePlayerStart()
* Return a score representing how desireable a playerstart is.
* @param P is the playerstart being rated
* @param Team is the team of the player choosing the playerstart
* @param Player is the controller choosing the playerstart
* @returns playerstart score
*/
function float RatePlayerStart(PlayerStart P, byte Team, Controller Player)
{
	if ( bSpawnInTeamArea )
	{
		// never use playerstarts not belonging to this team
		if ( UTTeamPlayerStart(P) == None )
		{
			`warn(P$" is not a team playerstart!");
			return -9;
		}
		if ( Team != UTTeamPlayerStart(P).TeamNumber )
			return -9;
	}
	return Super.RatePlayerStart(P,Team,Player);
}


/* CheckScore()
see if this score means the game ends
*/
function bool CheckScore(PlayerReplicationInfo Scorer)
{
	if (CheckMaxLives(Scorer))
	{
		return false;
	}
	else if (!Super(GameInfo).CheckScore(Scorer))
	{
		return false;
	}
	else if (!bOverTime && GoalScore == 0)
	{
		return false;
	}
	else if (Scorer != None && Scorer.Team != None && Scorer.Team.Score >= GoalScore)
	{
		EndGame(Scorer,"teamscorelimit");
		return true;
	}
	else if (Scorer != None && bOverTime)
	{
		EndGame(Scorer,"timelimit");
		return true;
	}
	else
	{
		return false;
	}
}

function bool CriticalPlayer(Controller Other)
{
	/* //@todo steve
	if ( (GameRulesModifiers != None) && (GameRulesModifiers.CriticalPlayer(Other)) )
		return true;
	*/
	if ( (Other.PlayerReplicationInfo != None) && Other.PlayerReplicationInfo.bHasFlag )
		return true;

	return false;
}
// ==========================================================================
// FindVictimsTarget - Tries to determine who the victim was aiming at
// ==========================================================================

function Pawn FindVictimsTarget(Controller Other)
{

	local Vector Start,X,Y,Z;
	local float Dist,Aim;
	local Actor Target;

	if (Other==None || Other.Pawn==None || Other.Pawn.Weapon==None)	// If they have no weapon, they can't be targetting someone
		return None;

	GetAxes(Other.Pawn.GetViewRotation(),X,Y,Z);
	Start = Other.Pawn.Location + vect(0,0,1)*Other.Pawn.BaseEyeHeight;
	Aim = 0.97;
	Target = Other.PickTarget(class'Pawn', aim,dist,X,Start,4000.f);

	return Pawn(Target);

}

/** returns whether the given game object-holding player was near an objective the game object can be scored at */
function bool NearGoal(Controller C)
{
	return false;
}

function ScoreKill(Controller Killer, Controller Other)
{
	local Pawn Target;
	local UTPlayerReplicationInfo KillerPRI;

	if ( !Other.bIsPlayer || ((Killer != None) && (!Killer.bIsPlayer || (Killer.PlayerReplicationInfo == None))) )
	{
		Super.ScoreKill(Killer, Other);
		if ( !bScoreTeamKills && (Killer != None) && Killer.bIsPlayer && (MaxLives > 0) )
			CheckScore(Killer.PlayerReplicationInfo);
		return;
	}

	if ( (Killer == None) || (Killer == Other)
		|| (Killer.PlayerReplicationInfo.Team != Other.PlayerReplicationInfo.Team) )
	{
		if ( Killer != None )
		{
			KillerPRI = UTPlayerReplicationInfo(Killer.PlayerReplicationInfo);
			if ( KillerPRI.Team != Other.PlayerReplicationInfo.Team )
			{
				if ( Other.PlayerReplicationInfo.bHasFlag )
				{
					UTPlayerReplicationInfo(Other.PlayerReplicationInfo).GetFlag().bLastSecondSave = NearGoal(Other);
				}

				// Kill Bonuses work as follows (in additional to the default 1 point
				//	+1 Point for killing an enemy targetting an important player on your team
				//	+2 Points for killing an enemy important player

				if ( CriticalPlayer(Other) )
				{
					Killer.PlayerReplicationInfo.Score+= 2;
					Killer.PlayerReplicationInfo.NetUpdateTime = WorldInfo.TimeSeconds - 1;
					BonusEvent('critical_frag',Killer.PlayerReplicationInfo);
					if ( UTPlayerController(Killer) != None )
						UTPlayerController(Killer).ClientMusicEvent(6);
				}

				if (bScoreVictimsTarget)
				{
					Target = FindVictimsTarget(Other);
					if ( (Target!=None) && (Target.PlayerReplicationInfo!=None) &&
						(Target.PlayerReplicationInfo.Team == Killer.PlayerReplicationInfo.Team) && CriticalPlayer(Target.Controller) )
					{
						Killer.PlayerReplicationInfo.Score+=1;
						Killer.PlayerReplicationInfo.NetUpdateTime = WorldInfo.TimeSeconds - 1;
						BonusEvent('team_protect_frag',Killer.PlayerReplicationInfo);
					}
				}
			}
		}
		Super.ScoreKill(Killer, Other);
	}
	/* //@todo steve
	else if ( GameRulesModifiers != None )
		GameRulesModifiers.ScoreKill(Killer, Other);
	*/
	if ( !bScoreTeamKills )
	{
		if ( Other.bIsPlayer && (Killer != None) && Killer.bIsPlayer && (Killer != Other) && (Killer.PlayerReplicationInfo != None)
			&& (Killer.PlayerReplicationInfo.Team == Other.PlayerReplicationInfo.Team) )
		{
			Killer.PlayerReplicationInfo.Score -= 1;
			Killer.PlayerReplicationInfo.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		}
		if ( (MaxLives > 0) && (Killer != None) )
			CheckScore(Killer.PlayerReplicationInfo);
		return;
	}
	if ( Other.bIsPlayer )
	{
		if ( (Killer == None) || (Killer == Other) )
		{
			Other.PlayerReplicationInfo.Team.Score -= 1;
			Other.PlayerReplicationInfo.Team.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		}
		else if ( Killer.PlayerReplicationInfo.Team != Other.PlayerReplicationInfo.Team )
		{
			Killer.PlayerReplicationInfo.Team.Score += 1;
			Killer.PlayerReplicationInfo.Team.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		}
		else if ( FriendlyFireScale > 0 )
		{
			Killer.PlayerReplicationInfo.NetUpdateTime = WorldInfo.TimeSeconds - 1;
			Killer.PlayerReplicationInfo.Score -= 1;
			Killer.PlayerReplicationInfo.Team.Score -= 1;
			Killer.PlayerReplicationInfo.Team.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		}
	}

	// check score again to see if team won
    if ( (Killer != None) && bScoreTeamKills )
		CheckScore(Killer.PlayerReplicationInfo);
}

function ReduceDamage( out int Damage, pawn injured, Controller instigatedBy, vector HitLocation, out vector Momentum, class<DamageType> DamageType )
{
	local int InjuredTeam, InstigatorTeam;

	if ( instigatedBy == None )
	{
		Super.ReduceDamage( Damage,injured,instigatedBy,HitLocation,Momentum,DamageType );
		return;
	}

	InjuredTeam = Injured.GetTeamNum();
	InstigatorTeam = instigatedBy.GetTeamNum();
	if (instigatedBy != injured.Controller && (Injured.DrivenVehicle == None || InstigatedBy.Pawn != Injured.DrivenVehicle))
	{
		if ( (InjuredTeam != 255) && (InstigatorTeam != 255) )
		{
			if ( InjuredTeam == InstigatorTeam )
			{
				Momentum *= TeammateBoost;

				if ( (UTBot(injured.Controller) != None) && (instigatedBy.Pawn != None) )
					UTBot(Injured.Controller).YellAt(instigatedBy.Pawn);
				/* //@todo steve
				else if ( (UTPlayerController(Injured.Controller) != None) && UTPlayerController(Injured.Controller).bAutoTaunt )
					Injured.Controller.SendMessage(instigatedBy.PlayerReplicationInfo, 'FRIENDLYFIRE', Rand(3), 5, 'TEAM');
				*/
				if ( FriendlyFireScale==0.0 )
				{
					Damage = 0;
					Super.ReduceDamage(Damage, Injured, InstigatedBy, HitLocation, Momentum, DamageType);
					return;
				}
				Damage *= FriendlyFireScale;
			}
			/* //@todo steve
			else if ( !injured.IsHumanControlled() && (injured.Controller != None)
					&& (injured.PlayerReplicationInfo != None) && injured.PlayerReplicationInfo.bHasFlag )
				injured.Controller.SendMessage(None, 'OTHER', injured.Controller.GetMessageIndex('INJURED'), 15, 'TEAM');
			*/
		}
	}
	Super.ReduceDamage( Damage,injured,instigatedBy,HitLocation,Momentum,DamageType );
}

function bool DominatingVictory()
{
	return ( ((Teams[0].Score == 0) || (Teams[1].Score == 0))
		&& (Teams[0].Score + Teams[1].Score >= 3) );
}

function bool IsAWinner( PlayerController C )
{
	return ( C.PlayerReplicationInfo.bOnlySpectator || IsWinningTeam(C.PlayerReplicationInfo.Team) );
}

function bool IsWinningTeam( TeamInfo T )
{
	if ( Teams[0].Score > Teams[1].Score )
		return (T == Teams[0]);
	else
		return (T == Teams[1]);
}

function PlayRegularEndOfMatchMessage()
{
	local UTPlayerController PC;
	local int Index;

	if ( Teams[0].Score > Teams[1].Score )
		Index = 4;
	else
		Index = 5;
	foreach WorldInfo.AllControllers(class'UTPlayerController', PC)
	{
		PC.ClientPlayAnnouncement(VictoryMessageClass, Index);
	}
}

function AnnounceScore(int ScoringTeam)
{
	local UTPlayerController PC;
	local int OtherTeam, MessageIndex;

	OtherTeam = 1 - ScoringTeam;

	if ( Teams[ScoringTeam].Score == Teams[OtherTeam].Score + 1 )
	{
		MessageIndex = 4 + ScoringTeam;
	}
	else if ( Teams[ScoringTeam].Score == Teams[OtherTeam].Score + 2 )
	{
		MessageIndex = 2 + ScoringTeam;
	}
	else
	{
		MessageIndex = ScoringTeam;
	}

	foreach WorldInfo.AllControllers(class'UTPlayerController', PC)
	{
		PC.ReceiveLocalizedMessage(TeamScoreMessageClass, MessageIndex);
	}
}

/** OverridePRI()
* override as needed properties of NewPRI with properties from OldPRI which were assigned during the login process
*/
function OverridePRI(PlayerController PC, PlayerReplicationInfo OldPRI)
{
	local UTTeamInfo DesiredTeam;

	DesiredTeam = UTTeamInfo(PC.PlayerReplicationInfo.Team);
	PC.PlayerReplicationInfo.OverrideWith(OldPRI);

	// try to use old team
	if ( (DesiredTeam != PC.PlayerReplicationInfo.Team) && (DesiredTeam != None) )
	{
		SetTeam(PC, DesiredTeam, true);
	}
}

/**
 * This function allows the server to override any requested teleport attempts from a client
 *
 * @returns 	returns true if the teleport is allowed
 */
function bool AllowClientToTeleport(UTPlayerReplicationInfo ClientPRI, Actor DestinationActor)
{
	return GameReplicationInfo.OnSameTeam(ClientPRI, DestinationActor);
}

function ShowPathTo(PlayerController P, int TeamNum)
{
	local UTGameObjective G, Best;

	Best = None;
	for (G = Teams[0].AI.Objectives; G != None; G = G.NextObjective)
	{
		if (G.BetterObjectiveThan(Best, TeamNum, P.PlayerReplicationInfo.Team.TeamIndex))
		{
			Best = G;
		}
	}

	if (Best != None && P.FindPathToward(Best, false) != None)
	{
		Spawn(class'UTWillowWhisp', P,, P.Pawn.Location);
	}
}

event GetSeamlessTravelActorList(bool bToEntry, out array<Actor> ActorList)
{
	Super.GetSeamlessTravelActorList(bToEntry, ActorList);

	ActorList[ActorList.length] = WorldInfo.GRI.Teams[0];
	ActorList[ActorList.length] = WorldInfo.GRI.Teams[1];
}

event HandleSeamlessTravelPlayer(Controller C)
{
	local TeamInfo NewTeam;

	if (C.PlayerReplicationInfo != None)
	{
		if (C.PlayerReplicationInfo.Team != None && C.PlayerReplicationInfo.Team.TeamIndex < ArrayCount(Teams))
		{
			// move this player to the new team object with the same team index
			NewTeam = Teams[C.PlayerReplicationInfo.Team.TeamIndex];
			// if the old team would now be empty, we don't need it anymore, so destroy it
			if (C.PlayerReplicationInfo.Team.Size <= 1)
			{
				C.PlayerReplicationInfo.Team.Destroy();
			}
			else
			{
				C.PlayerReplicationInfo.Team.RemoveFromTeam(C);
			}
			NewTeam.AddToTeam(C);

			if (C.IsA('UTBot') && NewTeam.IsA('UTTeamInfo'))
			{
				// init bot orders
				UTTeamInfo(NewTeam).SetBotOrders(UTBot(C));
			}
		}
		else if (!C.PlayerReplicationInfo.bOnlySpectator)
		{
			//@FIXME: get team preference from somewhere?
			ChangeTeam(C, 0, false);
		}
	}

	Super.HandleSeamlessTravelPlayer(C);
}

/** parses the given speech for the bots that should receive it */
function ParseSpeechRecipients(UTPlayerController Speaker, const out array<SpeechRecognizedWord> Words, out array<UTBot> Recipients)
{
	local int i;
	local UTBot B;
	local bool bEntireTeam;
	local UTPlayerReplicationInfo PRI;

	bEntireTeam = (Words[0].WordText ~= "Team");
	for (i = 0; i < GameReplicationInfo.PRIArray.length; i++)
	{
		PRI = UTPlayerReplicationInfo(GameReplicationInfo.PRIArray[i]);
		if ( PRI != None && PRI.Team != None && PRI.Team == Speaker.PlayerReplicationInfo.Team &&
			(bEntireTeam || PRI.GetCallSign() ~= Words[0].WordText) )
		{
			B = UTBot(GameReplicationInfo.PRIArray[i].Owner);
			if (B != None)
			{
				Recipients[Recipients.length] = B;
			}
		}
	}
}

/** used for "status" voice command */
function int GetStatus(UTPlayerController Sender, UTBot B)
{
	return 0;
/** @FIXME: implement when have voice messages
	local name BotOrders;
	local int i, count;

	BotOrders = B.GetOrders();
	if ( B.Pawn == None )
	{
		if ( (BotOrders == 'DEFEND') && (B.Squad.Size == 1) )
			return 0;
	}
	else if ( B.PlayerReplicationInfo.HasFlag != None )
	{
		if ( B.Pawn.Health < 50 )
			return 13;
		return 2;
	}
	else if ( B.Enemy == None )
	{
		if ( BotOrders == 'DEFEND' )
			return 11;
		if ( (BotOrders == 'ATTACK') && B.bReachedGatherPoint )
			return 9;
	}
	else if ( B.EnemyVisible() )
	{
		if ( (B.Enemy.PlayerReplicationInfo != None) && (B.Enemy.PlayerReplicationInfo.HasFlag != None) )
			return BallCarrierMessage();
		if ( (BotOrders == 'DEFEND') && (((B.GoalScript != None) && (VSize(B.GoalScript.Location - B.Pawn.Location) < 1500)) || B.Squad.SquadObjective.BotNearObjective(B)) )
		{
			for ( i=0; i<8; i++ )
				if ( (B.Squad.Enemies[i] != None) && (B.Squad.Enemies[i].Health > 0) )
					Count++;

			if ( Count > 2 )
			{
				if ( B.Pawn.Health < 60 )
					return 21;
				return 22;
			}
			return 20;
		}
		if ( (BotOrders != 'FOLLOW') || (B.Squad.SquadLeader != Sender) )
		{
			for ( i=0; i<8; i++ )
				if ( (B.Squad.Enemies[i] != None) && (B.Squad.Enemies[i].Health > 0) )
					Count++;

			if ( Count > 1 )
			{
				if ( B.Pawn.Health < 60 )
				{
					if ( (BotOrders == 'ATTACK') || (BotOrders == 'FREELANCE') )
						return 13;
					return 21;
				}
			}
		}
	}
	else if ( B.Pawn.Health < 50 )
		return 13;
	else if ( (BotOrders == 'DEFEND') && (B.Squad.SquadObjective != None)
				&& (((B.GoalScript != None) && (VSize(B.GoalScript.Location - B.Pawn.Location) < 1500)) || B.Squad.SquadObjective.BotNearObjective(B)) )
		return 20;
	if ( (BotOrders == 'HOLD') &&  B.Pawn.ReachedDestination(B.GoalScript) )
		return 9;
	if ( (BotOrders == 'FOLLOW') && (B.Squad.SquadLeader == Sender) && (B.Squad.SquadLeader.Pawn != None)
			&& B.LineOfSightTo(B.Squad.SquadLeader.Pawn) )
		return 3;
	if ( BotOrders == 'DEFEND' )
		return 26;
	if ( BotOrders == 'ATTACK' )
	{
		if ( B.bFinalStretch )
			return 10;
		return 27;
	}
	return 11;
*/
}

/** parses and sends the orders in the given speech to the Recipients */
function ProcessSpeechOrders(UTPlayerController Speaker, const out array<SpeechRecognizedWord> Words, const out array<UTBot> Recipients)
{
	local name Orders;
	local int i;
	local Vehicle V;

	switch (Caps(Words[1].WordText))
	{
		case "ATTACK":
		case "DEFEND":
		case "FREELANCE":
		case "HOLD":
		//@todo: maybe support "Follow/Cover Alpha", etc to force bots to group with other bots?
		case "FOLLOW":
			Orders = name(Words[1].WordText);
			break;
		case "COVER":
			Orders = 'Follow';
			break;
		case "TAUNT":
			for (i = 0; i < Recipients.length; i++)
			{
				Recipients[i].ForceCelebrate();
			}
			break;
		case "SUICIDE":
			for (i = 0; i < Recipients.length; i++)
			{
				if (Recipients[i].Pawn != None)
				{
					Recipients[i].Pawn.Suicide();
				}
			}
			break;
		case "STATUS":
			for (i = 0; i < Recipients.length; i++)
			{
				Recipients[i].SendMessage(Speaker.PlayerReplicationInfo, 'OTHER', GetStatus(Speaker, Recipients[i]), 0, 'TEAM');
			}
			break;
		case "JUMP":
			for (i = 0; i < Recipients.length; i++)
			{
				if (Recipients[i].Pawn != None)
				{
					Recipients[i].Pawn.bWantsToCrouch = false;
					Recipients[i].Pawn.DoJump(false);
				}
			}
			break;
		case "GIMME":
			for (i = 0; i < Recipients.length; i++)
			{
				Recipients[i].ForceGiveWeapon();
			}
			break;
		case "GET OUT":
			for (i = 0; i < Recipients.length; i++)
			{
				V = Vehicle(Recipients[i].Pawn);
				if (V != None)
				{
					V.DriverLeave(false);
				}
			}
			break;
		default:
			break;
	}

	if (Orders != 'None')
	{
		for (i = 0; i < Recipients.length; i++)
		{
			Recipients[i].SetOrders(Orders, Speaker);
		}
	}
}

function ProcessSpeechRecognition(UTPlayerController Speaker, const out array<SpeechRecognizedWord> Words)
{
	local array<UTBot> Recipients;

	Super.ProcessSpeechRecognition(Speaker, Words);

	ParseSpeechRecipients(Speaker, Words, Recipients);
	ProcessSpeechOrders(Speaker, Words, Recipients);
}

defaultproperties
{
	bScoreTeamKills=true
	bMustJoinBeforeStart=false
	bTeamGame=True
	TeamAIType(0)=class'UTGame.UTTeamAI'
	TeamAIType(1)=class'UTGame.UTTeamAI'
	EndMessageWait=3
	TeammateBoost=+0.3
	FriendlyFireScale=+0.0
	bMustHaveMultiplePlayers=true
	DefaultEnemyRosterClass="UTGame.UTTeamInfo"

	Acronym="TDM"
	MapPrefix="DM"
 	TeamScoreMessageClass=class'UTGame.UTTeamScoreMessage'
 	ScoreBoardType=class'UTGame.UTTeamScoreboard'
 	HUDType=class'UTGame.UTTeamHUD'
 	MidGameMenuTemplate=UTUIScene_MidGameMenu'UI_Scenes_HUD.Menus.MidGameMenuTDM';
}



