/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUITabPage_MapTab extends UTTabPage;

var transient UTDrawMapPanel Map;


function PostInitialize()
{
	Super.PostInitialize();
	Map = UTDrawMapPanel(FindChild('MapPanel',true));
}


function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	Super.SetupButtonBar(ButtonBar);
	ButtonBar.SetButton(1, "<Strings:UTGameUI.MidGameMenu.SetSpawnLoc>", OnSetSpawn);
}


function bool OnSetSpawn(UIScreenObject InButton, int InPlayerIndex)
{
   	local UTPlayerController UTPC;

	UTPC = UTUIScene(GetScene()).GetUTPlayerOwner();
	if (UTPC != none && Map != none )
	{
		Map.HomeNodeSelected(UTPC);
	}
	return true;
}


defaultproperties
{
}
