/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_TeamScoreboard extends UTUIScene_Scoreboard;


var transient UTScoreboardPanel Scoreboards[2];
var transient UILabel TeamScores[2];

event PostInitialize()
{
	Scoreboards[0] = UTScoreboardPanel( FindChild('RedScores',true));
	Scoreboards[1] = UTScoreboardPanel( FindChild('BlueScores',true));

	TeamScores[0] = UILabel( FindChild('RedTeamScore',true));
	TeamScores[1] = UILabel( FindChild('BlueTeamScore',true));

	TeamScores[0].SetValue("0");
	TeamScores[1].SetValue("0");

	Super.PostInitialize();
}




event TickScene(float DeltaTime)
{
	local int i;
	local GameReplicationInfo GRI;
	GRI = GetWorldInfo().GRI;

   	if (GRI != none )
	{

		for (i=0;i<2;i++)
		{
			if (GRI.Teams[i] != none )
			{
				TeamScores[i].SetValue( string( int(GRI.Teams[i].Score) ) );
			}
			else
			{
				TeamScores[i].SetValue( "0" );
			}
		}
	}

	Super.TickScene(DeltaTime);
}


defaultproperties
{
}
