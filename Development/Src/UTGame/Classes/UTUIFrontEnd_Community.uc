/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Community scene for UT3.
 */
class UTUIFrontEnd_Community extends UTUIFrontEnd_BasicMenu;

const COMMUNITY_OPTION_NEWS = 0;
const COMMUNITY_OPTION_DLC = 1;
const COMMUNITY_OPTION_STATS = 2;
const COMMUNITY_OPTION_DEMOPLAYBACK = 3;
const COMMUNITY_OPTION_FRIENDS = 4;
const COMMUNITY_OPTION_ACHIEVEMENTS = 5;

/** Scene references. */
var string	StatsScene;
var string	NewsScene;
var string	DemoPlaybackScene;
var string	FriendsScene;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize( )
{
	Super.PostInitialize();
}

/** Sets up the scene's button bar. */
function SetupButtonBar()
{
	ButtonBar.Clear();

	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Select>", OnButtonBar_Select);
}

/**
 * Executes a action based on the currently selected menu item. 
 */
function OnSelectItem(int PlayerIndex=0)
{
	local int SelectedItem;
	SelectedItem = MenuList.GetCurrentItem();

	switch(SelectedItem)
	{
	case COMMUNITY_OPTION_NEWS:
		if(CheckLoginAndError(INDEX_NONE, true))
		{
			OnShowNews();
		}
		break;
	case COMMUNITY_OPTION_DLC:
		if(CheckLoginAndError(INDEX_NONE, true))
		{
			OnShowDLC();
		}
		break;
	case COMMUNITY_OPTION_STATS:
		if(CheckLoginAndError(INDEX_NONE, true))
		{
			OnShowStats();
		}
		break;
	case COMMUNITY_OPTION_DEMOPLAYBACK:
		OnShowDemoPlayback();
		break;
	case COMMUNITY_OPTION_ACHIEVEMENTS:
		if(CheckLoginAndError())
		{
			OnShowAchievements(PlayerIndex);
		}
		break;
	case COMMUNITY_OPTION_FRIENDS:
		if(CheckLoginAndError())
		{
			OnShowFriends();
		}
		break;
	}
}

/** Shows the news scene. */
function OnShowNews()
{
	OpenSceneByName(NewsScene);
}

/** Shows the DLC scene or blade. */
function OnShowDLC()
{
	local OnlineSubsystem OnlineSub;

	if(GetPlatform()==UTPlatform_360)	// Display DLC blade
	{
		OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
		if (OnlineSub != None)
		{
			if (OnlineSub.PlayerInterfaceEx != None)
			{
				if (OnlineSub.PlayerInterfaceEx.ShowContentMarketplaceUI(GetPlayerOwner().ControllerId) == false)
				{
					`Log("Failed to show the marketplace UI");
				}
			}
			else
			{
				`Log("OnlineSubsystem does not support the extended player interface");
			}
		}
	}
	else
	{
		OpenSceneByName(NewsScene);
	}
}

/** Shows the stats scene. */
function OnShowStats()
{
	OpenSceneByName(StatsScene);
}

/** Shows the demo playback scene. */
function OnShowDemoPlayback()
{
	OpenSceneByName(DemoPlaybackScene);
}

/** Achievements option selected, displays the achievements blade for the specified PlayerIndex. */
function OnShowAchievements(int PlayerIndex)
{
	local OnlineSubsystem OnlineSub;
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterfaceEx != None)
		{
			// Show them for the requesting player
			if (OnlineSub.PlayerInterfaceEx.ShowAchievementsUI(class'UIInteraction'.static.GetPlayerControllerId(PlayerIndex)) == false)
			{
				`Log("Failed to show the achievements UI");
			}
		}
		else
		{
			`Log("OnlineSubsystem does not support the extended player interface");
		}
	}
}

/** Shows the friends screen. */
function OnShowFriends()
{
	OpenSceneByName(FriendsScene);
}

/** Callback for when the user wants to back out of this screen. */
function OnBack()
{
	CloseScene(self);
}

/** Buttonbar Callbacks. */
function bool OnButtonBar_Back(UIScreenObject InButton, int InPlayerIndex)
{
	OnBack();

	return true;
}

/**
 * Provides a hook for unrealscript to respond to input using actual input key names (i.e. Left, Tab, etc.)
 *
 * Called when an input key event is received which this widget responds to and is in the correct state to process.  The
 * keys and states widgets receive input for is managed through the UI editor's key binding dialog (F8).
 *
 * This delegate is called BEFORE kismet is given a chance to process the input.
 *
 * @param	EventParms	information about the input event.
 *
 * @return	TRUE to indicate that this input key was processed; no further processing will occur on this input key event.
 */
function bool HandleInputKey( const out InputEventParameters EventParms )
{
	local bool bResult;

	bResult=false;

	if(EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
		{
			OnBack();
			bResult=true;
		}
	}

	return bResult;
}


defaultproperties
{
	StatsScene="UI_Scenes_FrontEnd.Scenes.Leaderboards"
	NewsScene="UI_Scenes_FrontEnd.Scenes.News"
	DemoPlaybackScene="UI_Scenes_FrontEnd.Scenes.DemoPlayback"
	FriendsScene="UI_Scenes_FrontEnd.Scenes.Friends"
}