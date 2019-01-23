/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class Scoreboard_CTF extends UTUIScene_TeamScoreboard;

function string GetGoalScoreString(GameReplicationInfo GRI)
{
   	if (GRI != none )
	{
		return String(GRI.GoalScore) $ "<Strings:UTGameUI.Scoreboards.CapsToWin>";
	}

	return "";
}


defaultproperties
{
}
