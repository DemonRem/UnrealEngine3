/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCTFGame extends UTTeamGame
	abstract;

var UTCTFFlag Flags[2];

var class<UTLocalMessage> AnnouncerMessageClass;

function PostBeginPlay()
{
	local int i;

	Super.PostBeginPlay();

	for ( i=0; i<2; i++ )
	{
		Flags[i].Team = Teams[i];
		Teams[i].HomeBase = Flags[i].HomeBase;
		Flags[i].Team.TeamFlag = Flags[i];
		UTCTFTeamAI(Teams[i].AI).FriendlyFlag = Flags[i];
		UTCTFTeamAI(Teams[i].AI).EnemyFlag = Flags[1-i];
	}
}

function RegisterFlag(UTCTFFlag F, int TeamIndex)
{
	Flags[TeamIndex] = F;
}

static function PrecacheGameAnnouncements(UTAnnouncer Announcer)
{
	Super.PrecacheGameAnnouncements(Announcer);
	class'UTLastSecondMessage'.static.PrecacheGameAnnouncements(Announcer, default.class);
}

function bool NearGoal(Controller C)
{
	local UTGameObjective B;

	B = UTTeamInfo(C.PlayerReplicationInfo.Team).HomeBase;
	return ( UTCTFBase(B).myFlag.bHome && (VSize(C.Pawn.Location - B.Location) < 1000) );
}

function bool WantFastSpawnFor(AIController B)
{
	if ( NumBots < 6 )
		return true;
	if ( NumBots > 16 )
		return false;

	return !UTCTFSquadAI(UTBot(B).Squad).FriendlyFlag.bHome;
}

function bool CheckEndGame(PlayerReplicationInfo Winner, string Reason)
{
	local UTCTFFlag BestFlag;
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

	if ( CheckModifiedEndGame(Winner, Reason) )
		return false;

	if ( bLastMan )
	{
		GameReplicationInfo.Winner = Winner.Team;
	}
	else
	{
		if ( Teams[1].Score == Teams[0].Score )
		{
			// Don't allow overtime with automated perf testing.
			if( bAutomatedPerfTesting )
			{
				GameReplicationInfo.Winner = None;
			}
			else
			{
				if ( !bOverTimeBroadcast )
				{
					StartupStage = 7;
					PlayStartupMessage();
					bOverTimeBroadcast = true;
				}
				return false;
			}
		}
		else if ( Teams[1].Score > Teams[0].Score )
		{
			GameReplicationInfo.Winner = Teams[1];
		}
		else
		{
			GameReplicationInfo.Winner = Teams[0];
		}
	}

	BestFlag = UTCTFTeamAI(UTTeamInfo(GameReplicationInfo.Winner).AI).FriendlyFlag;

	if ( Winner != None )
	{
		EndGameFocus = Controller(Winner.Owner).Pawn;
	}
	
	if ( EndGameFocus == None )
	{
		EndGameFocus = BestFlag.HomeBase;
	}

	EndTime = WorldInfo.TimeSeconds + EndTimeDelay;
	foreach WorldInfo.AllControllers(class'Controller', P)
	{
		P.GameHasEnded( EndGameFocus, (P.PlayerReplicationInfo != None) && (P.PlayerReplicationInfo.Team == GameReplicationInfo.Winner) );
	}
	BestFlag.HomeBase.SetHidden(False);
	return true;
}

function ScoreFlag(Controller Scorer, UTCTFFlag theFlag)
{
	local float Dist,oppDist;
	local int i;
	local float ppp,numtouch;
	local vector FlagLoc;
	local UTPlayerReplicationInfo ScorerPRI;

	ScorerPRI = UTPlayerReplicationInfo(Scorer.PlayerReplicationInfo);
	if ( ScorerPRI.Team == theFlag.Team )
	{
		FlagLoc = TheFlag.Position().Location;
		Dist = vsize(FlagLoc - TheFlag.HomeBase.Location);
		oppDist = vsize(FlagLoc - Teams[1-TheFlag.Team.TeamIndex].HomeBase.Location);

		GameEvent('flag_returned',theFlag.Team.TeamIndex,ScorerPRI);
		BroadcastLocalizedMessage( AnnouncerMessageClass, 1+7*TheFlag.Team.TeamIndex, ScorerPRI, None, TheFlag.Team );

		if (Dist>1024)
		{
			// figure out who's closer
			if (Dist<=oppDist)	// In your team's zone
			{
				ScorerPRI.Score += 3;
				ObjectiveEvent('flag_return_friendly',TheFlag.Team.TeamIndex,ScorerPRI,0);
			}
			else
			{
				ScorerPRI.Score += 5;
				ObjectiveEvent('flag_return_enemy',TheFlag.Team.TeamIndex,ScorerPRI,0);

				if (oppDist<=1024)	// Denial
				{
  					ScorerPRI.Score += 7;
  					BonusEvent('flag_denial',ScorerPRI);
				}

			}
		}
		return;
	}

	// Figure out Team based scoring.
	if (TheFlag.FirstTouch!=None)	// Original Player to Touch it gets 5
	{
		BonusEvent('flag_cap_1st_touch',TheFlag.FirstTouch.PlayerReplicationInfo);
		TheFlag.FirstTouch.PlayerReplicationInfo.Score += 5;
		TheFlag.FirstTouch.PlayerReplicationInfo.NetUpdateTime = WorldInfo.TimeSeconds - 1;
	}

	// Guy who caps gets 5
	ScorerPRI.NetUpdateTime = WorldInfo.TimeSeconds - 1;
	ScorerPRI.Score += 5;
	ScorerPRI.GoalsScored += 1;
	if ( (ScorerPRI.GoalsScored == 3) && (UTPlayerController(ScorerPRI.Owner) != None) )
	{
		UTPlayerController(ScorerPRI.Owner).ClientPlayAnnouncement(TeamScoreMessageClass,6);
		BonusEvent('hat_trick',ScorerPRI);
	}

	// Each player gets 20/x but it's guarenteed to be at least 1 point but no more than 5 points
	numtouch=0;
	for (i=0;i<TheFlag.Assists.length;i++)
	{
		if (TheFlag.Assists[i]!=None)
			numtouch = numtouch + 1.0;
	}

	ppp = FClamp(20/numtouch,1,5);

	for (i=0;i<TheFlag.Assists.length;i++)
	{
		if (TheFlag.Assists[i]!=None)
		{
			BonusEvent('flag_cap_assist',TheFlag.Assists[i].PlayerReplicationInfo);
			TheFlag.Assists[i].PlayerReplicationInfo.Score += int(ppp);
		}
	}

	// Apply the team score
	ScorerPRI.Team.Score += 1.0;
	ScorerPRI.Team.NetUpdateTime = WorldInfo.TimeSeconds - 1;
	BonusEvent('flag_cap_final',ScorerPRI);
	GameEvent('flag_captured',theflag.Team.TeamIndex,ScorerPRI);

	AnnounceScore(ScorerPRI.Team.TeamIndex);
	CheckScore(ScorerPRI);

    if ( bOverTime )
    {
		EndGame(ScorerPRI,"timelimit");
    }
}

State MatchOver
{
	function ScoreFlag(Controller Scorer, UTCTFFlag theFlag)
	{
	}
}


function ViewObjective(PlayerController PC)
{
	local actor a;
	if (PC.ViewTarget != none &&
	      (PC.ViewTarget == Flags[0] || (PC.ViewTarget == Flags[0].Holder) || PC.VIewTarget == Flags[0].HomeBase) )
	{
		a = Flags[1].HomeBase.GetBestViewTarget();
		PC.SetViewTarget( a );
	}
	else
	{
		a = Flags[0].HomeBase.GetBestViewTarget();
		PC.SetViewTarget( a );
	}
}

function string TeamColorText(int TeamNum)
{
	if (TeamNum==1)
	{
		return "Blue";
	}
	else
	{
		return "Red";
	}
}

function string DecodeEvent(name EventType, int TeamNo, string InstigatorName, string AdditionalName, class<object> AdditionalObj)
{
	if (EventType == 'flag_returned')
	{
		return InstigatorName@"returned his team's flag ("$TeamColorText(TeamNo)$")";
	}
	else if (EventType == 'flag_returned_friendly')
	{
		return InstigatorName@"made a quick return of team's flag ("$TeamColorText(TeamNo)$")";
	}
	else if (EventType == 'flag_returned')
	{
		return InstigatorName@"saved the day by returning his team's flag deep in the enemy zone ("$TeamColorText(TeamNo)$")";
	}
	else if (EventType == 'flag_captured')
	{
		return InstigatorName@"captured the enemy flag scoring a point for the"@TeamColorText(TeamNo)@"team!";
	}
	else if (EventType == 'flag_taken')
	{
		return InstigatorName@"grabbed the "$TeamColorText(TeamNo)$"flag!";
	}
	else
	{
		return super.DecodeEvent(EventType, TeamNo, InstigatorName, AdditionalName, AdditionalObj);
	}
}

defaultproperties
{
	bAllowTranslocator=true
	bUndrivenVehicleDamage=true
	bSpawnInTeamArea=true
	bScoreTeamKills=False
	MapPrefix="CTF"
	TeamAIType(0)=class'UTGame.UTCTFTeamAI'
	TeamAIType(1)=class'UTGame.UTCTFTeamAI'
	bScoreVictimsTarget=true
	DeathMessageClass=class'UTTeamDeathMessage'
}

