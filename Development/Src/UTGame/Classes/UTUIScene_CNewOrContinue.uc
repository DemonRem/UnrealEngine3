/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_CNewOrContinue extends UTUIScene_Campaign;

var transient UTSimpleMenu Menu;
var string OptionsSceneTemplate;

event PostInitialize( )
{
	Super.PostInitialize();

	Menu = UTSimpleMenu( FindChild('SimpleMenu',true) );
	Menu.OnItemChosen = ItemChosen;
	Menu.SelectItem(0);
}

function ItemChosen(UTSimpleList SourceList, int SelectedIndex, int PlayerIndex)
{
	local UTUIScene_MessageBox MB;

	if (SelectedIndex == 0)
	{
		MB = DisplayMessageBox ("<Strings:UTGameUI.Campaign.NewWarning>","<Strings:UTGameUI.Campaign.Confirmation");
	    MB.OnSelection = MB_Selection;
	}
	else
	{
		GotoOptions(false);
	}
}

function GotoOptions(bool bNewMission)
{

	local UTUIScene_COptions OptionsScene;

	OptionsScene = UTUIScene_COptions( OpenSceneByName(OptionsSceneTemplate) );
	if ( OptionsScene != none )
	{
		OptionsScene.Configure(bNewMission);
	}
}


function MB_Selection(int SelectedOption, int PlayerIndex)
{
	if (SelectedOption == 0)
	{
		GotoOptions(true);
	}
}


defaultproperties
{
	OptionsSceneTemplate="UI_Scenes_Campaign.Scenes.CampOptions"
}
