/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Player stats scene for UT3.
 */
class UTUIFrontEnd_PlayerStats extends UTUIFrontEnd;

/** Reference to the leaderboard list and the result index we will be using to retrieve detailed stats. */
var transient UIList LeaderboardList;
var transient int StatsIndex;
var transient UTUIStatsList StatsList;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize()
{
	Super.PostInitialize();

	StatsList = UTUIStatsList(FindChild('lstStats',true));
}

function SetStatsSource(UIList InLeaderboardList, int InStatsIndex)
{
	LeaderboardList = InLeaderboardList;
	StatsIndex = InStatsIndex;

	StatsList.SetStatsIndex(StatsIndex);
}

/** Setup the scene's button bar. */
function SetupButtonBar()
{
	ButtonBar.Clear();
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
}

/** Callback for when the user wants to exit the scene. */
function OnBack()
{
	CloseScene(self);
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


/** Buttonbar Callbacks. */
function bool OnButtonBar_Back(UIScreenObject InButton, int PlayerIndex)
{
	OnBack();

	return true;
}

defaultproperties
{


}