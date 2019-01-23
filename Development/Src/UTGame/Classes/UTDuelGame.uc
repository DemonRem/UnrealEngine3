class UTDuelGame extends UTTeamGame;

/** queue of players that will take on the winner */
var array<UTDuelPRI> Queue;

/** how many rounds before we switch maps */
var config int NumRounds;
/** current round number */
var int CurrentRound;
/** whether to rotate the queue each kill instead of each round (Survival mode) */
var bool bRotateQueueEachKill;

// Returns whether a mutator should be allowed with this gametype
static function bool AllowMutator( string MutatorClassName )
{
	if ( MutatorClassName ~= "UTGameContent.UTMutator_Survival")
	{
		// survival mutator only for Duel
		return true;
	}
	return Super.AllowMutator(MutatorClassName);
}

event InitGame(string Options, out string ErrorMessage)
{
	Super.InitGame(Options, ErrorMessage);

	if (bRotateQueueEachKill)
	{
		NumRounds = 1;
	}
	else
	{
		NumRounds = Max(1, GetIntOption(Options, "NumRounds", NumRounds));
	}

	DesiredPlayerCount = Max(DesiredPlayerCount, 2);
	bTempForceRespawn = true;
}

event PostLogin(PlayerController NewPlayer)
{
	local UTDuelPRI PRI;

	Super.PostLogin(NewPlayer);

	if (NumPlayers + NumBots > 2)
	{
		PRI = UTDuelPRI(NewPlayer.PlayerReplicationInfo);
		if (PRI != None && !PRI.bOnlySpectator)
		{
			AddToQueue(PRI);
		}
	}
}

function UTBot AddBot(optional string botName, optional bool bUseTeamIndex, optional int TeamIndex)
{
	local UTBot NewBot;

	NewBot = SpawnBot(botName, bUseTeamIndex, TeamIndex);
	if (NewBot == None)
	{
		`Warn("Failed to spawn bot.");
		return None;
	}
	else
	{
		NewBot.PlayerReplicationInfo.PlayerID = CurrentID++;
		NumBots++;

		if (NumPlayers + NumBots > 2)
		{
			AddToQueue(UTDuelPRI(NewBot.PlayerReplicationInfo));
		}
		else if (WorldInfo.NetMode == NM_StandAlone)
		{
			RestartPlayer(NewBot);
		}
		else
		{
			NewBot.GotoState('Dead', 'MPStart');
		}
	}

	return NewBot;
}

function Logout(Controller Exiting)
{
	local int Index, i;
	local Controller C;
	local PlayerReplicationInfo Winner;

	Super.LogOut(Exiting);

	Index = Queue.Find(UTDuelPRI(Exiting.PlayerReplicationInfo));
	if (Index != INDEX_NONE)
	{
		Queue.Remove(Index, 1);
		for (i = 0; i < Queue.length; i++)
		{
			Queue[i].QueuePosition = i;
		}
	}
	else if (Exiting.PlayerReplicationInfo != None && Exiting.PlayerReplicationInfo.Team != None && Exiting.PlayerReplicationInfo.Team.Size == 1)
	{
		foreach WorldInfo.AllControllers(class'Controller', C)
		{
			if (C != Exiting && C.bIsPlayer && C.Pawn != None && UTDuelPRI(C.PlayerReplicationInfo) != None)
			{
				Winner = C.PlayerReplicationInfo;
				break;
			}
		}
		EndGame(Winner, "LastMan");
	}
}

function AddToQueue(UTDuelPRI Who)
{
	local PlayerController PC;
	local int i;

	// Add the player to the end of the queue
	i = Queue.Length;
	Queue.Length = i + 1;
	Queue[i] = Who;
	Queue[i].QueuePosition = i;

	SetTeam(Controller(Who.Owner), None, false);
	if (!bGameEnded)
	{
		Who.Owner.GotoState('InQueue');
		PC = PlayerController(Who.Owner);
		if (PC != None)
		{
			PC.ClientGotoState('InQueue');
		}
	}
}

function StartHumans()
{
	local Controller C;

	// just start everybody now
	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		if (bGameEnded)
		{
			return;
		}
		else if (C.bIsPlayer && (PlayerController(C) == None || PlayerController(C).CanRestartPlayer()))
		{
			RestartPlayer(C);
		}
	}
}

function StartBots();

function bool CheckEndGame(PlayerReplicationInfo Winner, string Reason)
{
	local bool bResult;

	bResult = Super.CheckEndGame(Winner, Reason);
	if (bResult)
	{
		// go to round over state instead of game over if there are more rounds to play
		if (CurrentRound < NumRounds)
		{
			bResult = false;
			GotoState('RoundOver');
		}
	}

	return bResult;
}

/** removes a player from the queue, sets it up to play, and returns the Controller
 * @note: doesn't spawn the player in (i.e. doesn't call RestartPlayer()), calling code is responsible for that
 */
function Controller GetPlayerFromQueue()
{
	local Controller C;
	local UTDuelPRI PRI;
	local UTTeamInfo NewTeam;

	PRI = Queue[0];
	Queue.Remove(0, 1);
	PRI.QueuePosition = -1;

	NewTeam = Teams[Teams[0].Size > Teams[1].Size ? 1 : 0];
	C = Controller(PRI.Owner);
	SetTeam(C, NewTeam, false);
	if (C.IsA('UTBot'))
	{
		NewTeam.SetBotOrders(UTBot(C));
	}

	return C;
}

function ScoreKill(Controller Killer, Controller Other)
{
	local UTDuelPRI PRI;
	local Controller C;

	Super.ScoreKill(Killer, Other);

	if (bRotateQueueEachKill && !bGameEnded)
	{
		PRI = UTDuelPRI(Other.PlayerReplicationInfo);
		if (PRI != None)
		{
			AddToQueue(PRI);
			C = GetPlayerFromQueue();
			RestartPlayer(C);
			if (C.PlayerReplicationInfo.Team != None)
			{
				C.PlayerReplicationInfo.Team.Score = C.PlayerReplicationInfo.Score;
			}
		}
	}
}

/** figures out the new combatants for the next round */
function UpdateCombatants()
{
	local int NumPlayersNeeded, i;
	local UTDuelPRI PRI;
	local Controller C;

	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		PRI = UTDuelPRI(C.PlayerReplicationInfo);
		if (PRI != None)
		{
			if (C.PlayerReplicationInfo.Team == GameReplicationInfo.Winner)
			{
				PRI.ConsecutiveWins++;
			}
			else
			{
				PRI.ConsecutiveWins = 0;
				if (Queue.Find(PRI) == INDEX_NONE)
				{
					AddToQueue(PRI);
					NumPlayersNeeded++;
				}
			}
		}
	}

	while (NumPlayersNeeded > 0 && Queue.length > 0)
	{
		GetPlayerFromQueue();
		NumPlayersNeeded--;
	}

	for (i = 0; i < Queue.length; i++)
	{
		Queue[i].QueuePosition = i;
	}
}

function RestartPlayer(Controller aPlayer)
{
	if (Queue.Find(UTDuelPRI(aPlayer.PlayerReplicationInfo)) == INDEX_NONE)
	{
		Super.RestartPlayer(aPlayer);
	}
}

function ResetLevel()
{
	local Controller C;

	Super.ResetLevel();

	// make sure everyone's in the correct state
	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		if (Queue.Find(UTDuelPRI(C.PlayerReplicationInfo)) != INDEX_NONE)
		{
			C.GotoState('InQueue');
			if (C.IsA('PlayerController'))
			{
				PlayerController(C).ClientGotoState('InQueue');
			}
		}
	}
}

state RoundOver
{
	function ResetLevel()
	{
		// note that we need to change the state BEFORE calling ResetLevel() so that we don't unintentionally override
		// functions that ResetLevel() may call
		UpdateCombatants();
		GotoState('');
		Global.ResetLevel();
		// redo warmup round for new players
		WarmupRemaining = WarmupTime;
		GotoState('PendingMatch');
		ResetCountDown = 0;

		CurrentRound++;
	}
}

function ProcessServerTravel(string URL)
{
	UpdateCombatants();

	Super.ProcessServerTravel(URL);
}

event PostSeamlessTravel()
{
	local int i;
	local UTDuelPRI PRI;

	// reconstruct the Queue from the PRIs
	for (i = 0; i < GameReplicationInfo.PRIArray.length; i++)
	{
		PRI = UTDuelPRI(GameReplicationInfo.PRIArray[i]);
		if (PRI != None && PRI.QueuePosition >= 0)
		{
			Queue[PRI.QueuePosition] = PRI;
		}
	}

	Super.PostSeamlessTravel();
}

event HandleSeamlessTravelPlayer(Controller C)
{
	local UTDuelPRI OldPRI, NewPRI;
	local int Index;
	local bool bInQueue;

	// replace the old PRI with the new one in the queue array
	// if it's not there, but we already have enough active players, add it
	OldPRI = UTDuelPRI(C.PlayerReplicationInfo);
	Super.HandleSeamlessTravelPlayer(C);
	NewPRI = UTDuelPRI(C.PlayerReplicationInfo);
	if (OldPRI != None)
	{
		if (NewPRI != None)
		{
			Index = Queue.Find(OldPRI);
			if (Index != INDEX_NONE)
			{
				Queue[Index] = NewPRI;
				NewPRI.QueuePosition = Index;
				SetTeam(C, None, false);
				bInQueue = true;
			}
		}
		else
		{
			Queue.RemoveItem(OldPRI);
		}
	}
	else if (NewPRI != None && NumPlayers + NumBots > 2)
	{
		AddToQueue(NewPRI);
		bInQueue = true;
	}

	if (bInQueue)
	{
		C.GotoState('InQueue');
		if (C.IsA('PlayerController'))
		{
			PlayerController(C).ClientGotoState('InQueue');
		}
	}
}

defaultproperties
{
	Acronym="Duel"
	MapPrefix="DM"
	HUDType=class'UTDuelHUD'
	PlayerReplicationInfoClass=class'UTDuelPRI'

	CurrentRound=1
	bWeaponStay=false
	bMustJoinBeforeStart=true
}
