/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Leaderboards scene for UT3.
 */
class UTUIFrontEnd_Leaderboards extends UTUIFrontEnd;

/** Reference to the stats datastore. */
var transient UTDataStore_OnlineStats StatsDataStore;

/** Reference to the stats interface. */
var transient OnlineStatsInterface StatsInterface;

/** Refreshing label. */
var transient UILabel RefreshingLabel;

/** List of stats results. */
var transient UIList StatsList;

/** Reference to the scene to show when displaying details for a player. */
var string DetailsScene;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize()
{
	local OnlineSubsystem OnlineSub;

	Super.PostInitialize();

	// Figure out if we have an online subsystem registered
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		// Grab the game interface to verify the subsystem supports it
		StatsInterface = OnlineSub.StatsInterface;

		if(StatsInterface==None)
		{
			`Log("UTUIFrontEnd_Leaderboards::PostInitialize() - Stats Interface is None!");
		}
	}
	else
	{
		`Log("UTUIFrontEnd_Leaderboards::PostInitialize() - OnlineSub is None!");
	}

	// Store a reference to the stats datastore.
	StatsDataStore = UTDataStore_OnlineStats(FindDataStore('UTLeaderboards'));

	RefreshingLabel = UILabel(FindChild('lblRefreshing',true));
	
	// Stats list submitting a selection
	StatsList = UIList(FindChild('lstStats', true));
	StatsList.OnSubmitSelection = OnStatsList_SubmitSelection;

	RefreshStats();
}

/** Setup the scene's button bar. */
function SetupButtonBar()
{
	ButtonBar.Clear();
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.PlayerDetails>", OnButtonBar_Details);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.ToggleGameMode>", OnButtonBar_ToggleGameMode);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.ToggleMatchType>", OnButtonBar_ToggleMatchType);
}

/** Callback for when the user wants to exit the scene. */
function OnBack()
{
	CloseScene(self);
}

/** Displays the details page for the currently selected leaderboard player. */
function OnDetails()
{
	local UTUIFrontEnd_PlayerStats PlayerStatsScene;

	PlayerStatsScene = UTUIFrontEnd_PlayerStats(OpenSceneByName(DetailsScene));
	PlayerStatsScene.SetStatsSource(StatsList,StatsList.GetCurrentItem());
}

/** Toggles the current game mode. */
function OnToggleGameMode()
{
	`Log("UTUIFrontEnd_Leaderboards::OnToggleGameMode() - Moving to next game mode");
	UTLeaderboardSettings(StatsDataStore.LeaderboardSettings).MoveToNextSettingValue(LF_GameMode);

	RefreshStats();
}

/** Toggles the current match type. */
function OnToggleMatchType()
{
	`Log("UTUIFrontEnd_Leaderboards::OnToggleMatchType() - Moving to next match type");
	UTLeaderboardSettings(StatsDataStore.LeaderboardSettings).MoveToNextSettingValue(LF_MatchType);

	RefreshStats();
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
		if(EventParms.InputKeyName=='XboxTypeS_X')
		{
			OnToggleGameMode();
			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_Y')
		{
			OnToggleMatchType();
			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
		{
			OnBack();
			bResult=true;
		}
	}

	return bResult;
}


/** Buttonbar Callbacks. */
function bool OnButtonBar_Back(UIScreenObject InButton, int PlayerIndex)
{
	OnBack();

	return true;
}

function bool OnButtonBar_Details(UIScreenObject InButton, int PlayerIndex)
{
	OnDetails();

	return true;
}


function bool OnButtonBar_ToggleGameMode(UIScreenObject InButton, int PlayerIndex)
{
	OnToggleGameMode();

	return true;
}

function bool OnButtonBar_ToggleMatchType(UIScreenObject InButton, int PlayerIndex)
{
	OnToggleMatchType();

	return true;
}



/** Refreshes the leaderboard stats list. */
function RefreshStats()
{
	// Add the delegate for when the read is complete.
	StatsInterface.AddReadOnlineStatsCompleteDelegate(OnStatsReadComplete);

	// Show the refreshing label
	RefreshingLabel.SetVisibility(true);

	// Issue the stats.
	`Log("UTUIFrontEnd_Leaderboards::RefreshStats() - Refreshing leaderboard.");
	if( StatsDataStore.RefreshStats(GetPlayerOwner().ControllerId) == false )
	{
		StatsInterface.ClearReadOnlineStatsCompleteDelegate(OnStatsReadComplete);
		RefreshingLabel.SetVisibility(false);
	}
}

/** Callback for when the stats read has completed. */
function OnStatsReadComplete(bool bWasSuccessful)
{
	`Log("UTUIFrontEnd_Leaderboards::OnStatsReadComplete() - Stats read completed, bWasSuccessful: " $ bWasSuccessful);

	// Hide refreshing label.
	RefreshingLabel.SetVisibility(false);
	StatsInterface.ClearReadOnlineStatsCompleteDelegate(OnStatsReadComplete);
}

/** Callback for when the user submits the currently selected list item. */
function OnStatsList_SubmitSelection( UIList Sender, optional int PlayerIndex )
{
	OnDetails();
}

defaultproperties
{
	DetailsScene="UI_Scenes_FrontEnd.Scenes.PlayerStatsDetails"
}
