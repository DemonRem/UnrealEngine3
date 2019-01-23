/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_Scoreboard extends UTUIScene_Hud
	native(UI)
	abstract;

var transient UILabel MapName;
var transient UILabel GoalScore;

var transient string LastMapName;
var transient int LastGoalScore;


cpptext
{
	virtual void Tick( FLOAT DeltaTime );
}

event TickScene(float DeltaTime)
{
	local WorldInfo WI;
	local GameReplicationInfo GRI;
	WI = GetWorldInfo();
	GRI = WI.GRI;

    if (GRI != none)
    {
    	if (WI.Title != LastMapName)
    	{
			MapName.SetValue( WI.Title);
			LastMapName = WI.Title;
		}

		if (GRI.GoalScore != LastGoalScore)
		{
			LastGoalScore = GRI.GoalScore;
			GoalScore.SetValue( GetGoalScoreString( GRI) );
		}
	}
}



event PostInitialize()
{

	Super.PostInitialize();

	GoalScore = UILabel( FindChild('GoalScore',true));
	GoalScore.SetValue("0");

	MapName = UILabel( FindChild('MapName',true));
}


function string GetGoalScoreString(GameReplicationInfo GRI)
{
	GRI = GetWorldInfo().GRI;

   	if (GRI != none )
	{
		return string(GRI.GoalScore);
	}
	else
	{
		return "";
	}
}



defaultproperties
{
	bCloseOnLevelChange=true
}
