/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// UTTeamInfo.
// includes list of bots on team for multiplayer games
//
//=============================================================================

class UTTeamInfo extends TeamInfo
	native;

var int DesiredTeamSize;
var UTTeamAI AI;
var string TeamSymbolName;
var RepNotify Material TeamIcon;
var UTGameObjective HomeBase;			// key objective associated with this team
var UTCarriedObject TeamFlag;
/** only bot characters in this faction will be used */
var string Faction;
/** characters in use by this team - array size should mirror UTCustomChar_Data.default.Characters */
var array<bool> CharactersInUse;


var color BaseTeamColor[4];
var color TextColor[4];
var localized string TeamColorNames[4];

replication
{
	// Variables the server should send to the client.
	if (bNetInitial)
		TeamIcon;

	if (bNetDirty)
		HomeBase, TeamFlag;

}

simulated function string GetHumanReadableName()
{
	if ( TeamName == Default.TeamName )
	{
		if ( TeamIndex < 4 )
			return TeamColorNames[TeamIndex];
		return TeamName@TeamIndex;
	}
	return TeamName;
}

simulated function color GetHUDColor()
{
	return BaseTeamColor[TeamIndex];
}

simulated function color GetTextColor()
{
	return TextColor[TeamIndex];
}

/* Reset()
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
	Super.Reset();
	if ( !UTGame(WorldInfo.Game).bTeamScoreRounds )
		Score = 0;
}

/* FIXMESTEVE
event ReplicatedEvent(name VarName)
{
	if ( VarName == 'TeamIcon' )
	{
		TeamSymbolNotify();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function TeamSymbolNotify()
{
	local Actor A;

	ForEach AllActors(class'Actor', A)
		A.SetTeamSymbol(self);
}
*/

function bool AllBotsSpawned()
{
	return false;
}

function Initialize(int TeamBots);

function bool NeedsBotMoreThan(UTTeamInfo T)
{
	return ( (DesiredTeamSize - Size) > (T.DesiredTeamSize - T.Size) );
}

function SetBotOrders(UTBot NewBot)
{
	if (AI != None)
	{
		AI.SetBotOrders(NewBot);
	}
}

function RemoveFromTeam(Controller Other)
{
	local int i;

	Super.RemoveFromTeam(Other);
	if (AI != None)
	{
		AI.RemoveFromTeam(Other);
	}

	// clear in use flag
	if (UTBot(Other) != None && Other.PlayerReplicationInfo != None)
	{
		for (i = 0; i < CharactersInUse.Length; i++)
		{
			if (CharactersInUse[i] && Other.PlayerReplicationInfo.PlayerName ~= class'UTCustomChar_Data'.default.Characters[i].CharName)
			{
				CharactersInUse[i] = false;
				break;
			}
		}
	}
}

/** retrieves bot info, for the named bot if a valid name is specified, otherwise from a random bot */
function CharacterInfo GetBotInfo(string BotName)
{
	local int Index, StartIndex, Best;

	// make sure characters array is properly sized
	CharactersInUse.length = class'UTCustomChar_Data'.default.Characters.length;

	Index = class'UTCustomChar_Data'.default.Characters.Find('CharName', BotName);
	if (Index == INDEX_NONE)
	{
		// choose a bot at random
		Index = Rand(class'UTCustomChar_Data'.default.Characters.length);
		StartIndex = Index;
		Best = INDEX_NONE;
		while (CharactersInUse[Index] || (Faction != "" && class'UTCustomChar_Data'.default.Characters[Index].Faction != Faction))
		{
			if (Best == INDEX_NONE && class'UTCustomChar_Data'.default.Characters[Index].Faction ~= Faction)
			{
				// remember best choice that was already in use (if all are in use, use this)
				Best = Index;
			}
			Index++;
			if (Index >= CharactersInUse.length)
			{
				Index = 0;
			}
			if (Index == StartIndex)
			{
				// searched entire array, give up
				if (Best != INDEX_NONE)
				{
					Index = Best;
				}
				break;
			}
		}
	}

	CharactersInUse[Index] = true;
	return class'UTCustomChar_Data'.default.Characters[Index];
}

simulated event Destroyed()
{
	Super.Destroyed();

	if (AI != None)
	{
		AI.Destroy();
	}
}

defaultproperties
{
	DesiredTeamSize=8
	BaseTeamColor(0)=(r=255,g=64,b=64,a=255)
	BaseTeamColor(1)=(r=64,g=64,b=255,a=255)
	BaseTeamColor(2)=(r=65,g=255,b=64,a=255)
	BaseTeamColor(3)=(r=255,g=255,b=0,a=255)

	TextColor(0)=(r=255,g=96,b=96,a=255)
	TextColor(1)=(r=128,g=128,b=255,a=255)
	TextColor(2)=(r=96,g=255,b=96,a=255)
	TextColor(3)=(r=255,g=255,b=96,a=255)
}

