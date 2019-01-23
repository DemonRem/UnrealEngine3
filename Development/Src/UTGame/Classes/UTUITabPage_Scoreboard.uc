/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUITabPage_Scoreboard extends UTTabPage;


var array<UTScoreboardPanel> Scoreboards;
var UTPlayerReplicationInfo SelectedPRI;

function PostInitialize()
{
	Super.PostInitialize();
	FindScoreboards();
}

function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	Super.SetupButtonBar(ButtonBar);
	ButtonBar.SetButton(1, "<Strings:UTGameUI.ButtonCallouts.GamerCard>", OnPlayerStats);
}

function FindScoreboards()
{
	local int i;
	local array<UIObject> Kids;
	local UTScoreboardPanel SB;

	Kids = GetChildren(true);
	for (i=0;i<Kids.Length;i++)
	{
		SB = UTScoreboardPanel(Kids[i]);
		if ( SB != none )
		{
			Scoreboards[Scoreboards.Length] = SB;
			SB.OnSelectionChange = OnScoreboardSelectionChange;
		}
	}
}


function OnScoreboardSelectionChange(UTScoreboardPanel TargetScoreboard, UTPlayerReplicationInfo PRI)
{
	local int i;
	for (i=0;i<Scoreboards.Length;i++)
	{
		if ( Scoreboards[i] != TargetScoreboard )
		{
			Scoreboards[i].ClearSelection();
		}
	}

    SelectedPRI = PRI;
}

function bool OnPlayerStats(UIScreenObject InButton, int InPlayerIndex)
{
	if ( SelectedPRI != none )
	{
	 	UTUIScene(GetScene()).DisplayMessageBox("Stats for "$SelectedPRI.PlayerName$"!","Stats");
	}
	return true;
}


defaultproperties
{
}
