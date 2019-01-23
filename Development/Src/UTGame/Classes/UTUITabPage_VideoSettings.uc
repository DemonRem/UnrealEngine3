/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page for a user's Video settings.
 */

class UTUITabPage_VideoSettings extends UTUITabPage_Options
	placeable;

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.Settings.Video>");

	SetupDefaults();
}

/** Callback allowing the tabpage to setup the button bar for the current scene. */
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	if(UTUIScene(GetScene()).GetPlatform()==UTPlatform_PC)
	{
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Apply>", OnButtonBar_Apply);
	}
}

/** Pulls default settings for screen resolution from the engine and sets the widgets appropriately. */
function SetupDefaults()
{
	local LocalPlayer LP;
	local Vector2D ViewportSize;
	local int OptionIndex;
	LP = GetPlayerOwner();

	// Screen Resolution
	LP.ViewportClient.GetViewportSize(ViewportSize);
	SetDataStoreStringValue("<UTStringList:ScreenResolution>", int(ViewportSize.X)$"x"$int(ViewportSize.Y));
	OptionIndex = OptionList.GetObjectInfoIndexFromName('ScreenResolution');
	UIDataStoreSubscriber(OptionList.GeneratedObjects[OptionIndex].OptionObj).RefreshSubscriberValue();

	// Full Screen
	if(LP.ViewportClient.IsFullScreenViewport())
	{
		StringListDataStore.SetCurrentValueIndex('FullScreen',0);
	}
	else
	{
		StringListDataStore.SetCurrentValueIndex('FullScreen',1);
	}
	OptionIndex = OptionList.GetObjectInfoIndexFromName('FullScreen');
	UIDataStoreSubscriber(OptionList.GeneratedObjects[OptionIndex].OptionObj).RefreshSubscriberValue();
}

/** Applies the current settings to the screen, this is usually used for changing screen resolution. */
function OnApplySettings()
{
	local string NewResolution;
	local int CurrentIndex;
	local string FullScreenStr;
	local bool bChangeResolution;

	if(UTUIScene(GetScene()).GetPlatform()==UTPlatform_PC)
	{
		// Apply screen resolution change.
		if(StringListDataStore.GetCurrentValue('ScreenResolution', NewResolution))
		{
			`Log("Screen Resolution Changed: " $ NewResolution);
			bChangeResolution = true;
		}

		// Check our full screen setting.
		CurrentIndex = StringListDataStore.GetCurrentValueIndex('FullScreen');
		if(CurrentIndex==0)
		{
			FullScreenStr = " F";
		}
		else
		{
			FullScreenStr = " W";
		}

		bChangeResolution = true;

		if(bChangeResolution)
		{
			GetPlayerOwner().ViewportClient.UIController.ViewportConsole.ConsoleCommand("SetRes "$NewResolution$FullScreenStr);
		}
	}
}

function bool OnButtonBar_Apply(UIScreenObject InButton, int PlayerIndex)
{
	OnApplySettings();

	return true;
}


function OnOptionList_OptionChanged(UIScreenObject InObject, name OptionName, int PlayerIndex)
{
	Super.OnOptionList_OptionChanged(InObject, OptionName, PlayerIndex);
}
