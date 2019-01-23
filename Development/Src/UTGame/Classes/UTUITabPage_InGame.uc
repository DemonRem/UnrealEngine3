/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUITabPage_InGame extends UTTabPage;

var transient UTSimpleMenu Menu;

function PostInitialize()
{
	local UTGameReplicationInfo GRI;
	GRI = UTGameReplicationInfo(UTUIScene(GetScene()).GetWorldInfo().GRI);
	Menu = UTSimpleMenu(FindChild('Menu',true));
	if ( Menu != none )
	{
		Menu.OnItemChosen = OnMenuItemChosen;
		if ( GRI != none )
		{
			GRI.PopulateMidGameMenu(Menu);
		}
	}
}


function OnMenuItemChosen(UTSimpleList SourceList, int SelectedIndex, int PlayerIndex)
{
	local UTGameReplicationInfo GRI;
	local UTPlayerController UTPC;
	GRI = UTGameReplicationInfo(UTUIScene(GetScene()).GetWorldInfo().GRI);
	UTPC = UTUIScene(GetScene()).GetUTPlayerOwner(PlayerIndex);
	if ( GRI != none )
	{
		if ( GRI.MidMenuMenu(UTPC,SourceList, SelectedIndex) )
		{
			GetSceneClient().CloseScene( GetScene() );
		}
	}
}
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	Super.SetupButtonBar(ButtonBar);
	ButtonBar.SetButton(1, "<Strings:UTGameUI.ButtonCallouts.Select>",none) ;
}


defaultproperties
{
}
