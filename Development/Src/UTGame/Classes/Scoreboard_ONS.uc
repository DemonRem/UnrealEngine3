/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class Scoreboard_ONS extends UTUIScene_TeamScoreboard;



function string GetGoalScoreString(GameReplicationInfo GRI)
{
   	if (GRI != none )
	{
		return "<Strings:UTGameUI.Scoreboards.ONSPreGoalScore>"@ String(GRI.GoalScore) @"<Strings:UTGameUI.Scoreboards.ONSPostGoalScore>";
	}

	return "";
}


defaultproperties
{

}
