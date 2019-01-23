/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page for a set of options to filter join game server results by.
 */

class UTUITabPage_ServerFilter extends UTUITabPage_Options
	placeable;

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.JoinGame.Filter>");
}

/** Pass through the option callback. */
function OnOptionList_OptionChanged(UIScreenObject InObject, name OptionName, int PlayerIndex)
{
	local string OutStringValue;
	Super.OnOptionList_OptionChanged(InObject, OptionName, PlayerIndex);

	if(OptionName=='GameMode_Client')
	{
		if(GetDataStoreStringValue("<UTMenuItems:GameModeFilterClass>", OutStringValue))
		{
			`Log("UTUITabPage_ServerFilter::OnOptionList_OptionChanged() - Game mode filter class set to "$OutStringValue);
			SetDataStoreStringValue("<UTGameSettings:CustomGameMode>", OutStringValue);
		}
	}
}
