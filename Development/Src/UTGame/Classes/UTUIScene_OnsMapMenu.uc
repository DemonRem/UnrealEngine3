/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_OnsMapMenu extends UTUIScene_Hud;

/** Pointer to the button bar for this scene. */
var transient UTUIButtonBar	ButtonBar;
var transient UTDrawMapPanel Map;
var transient UILabel Caption;
var transient bool bTeleportDisabled;
var transient bool bEditMap;
var transient bool bFineTune;

/**
 * PostInitialize event - Sets delegates for the scene.
 */
event PostInitialize( )
{
	Super.PostInitialize();

	Map = UTDrawMapPanel(FindChild('MapPanel',true));
	Map.OnCurrentActorChanged = MapCurrentActorChanged;
	Map.OnActorSelected = MapActorSelected;
	Map.FindBestActor();


	// Store a reference to the button bar.
	ButtonBar = UTUIButtonBar(FindChild('MapButtons', true));

	if(ButtonBar != none)
	{
		ButtonBar.SetButton(0, "<Strings:UTGameUI.ButtonCallouts.Cancel>", ButtonBarCancel);
		ButtonBar.SetButton(1, "<Strings:UTGameUI.ButtonCallouts.Select>", ButtonBarSelect);
		ButtonBar.SetButton(2, "<Strings:UTGameUI.MidGameMenu.SetSpawnLoc>", ButtonBarHomeNode);
	}

	Caption = UILabel(FindChild('MapCaption', true));
	OnRawInputKey = InputKey;

}


function TeleportToActor(UTPlayerController PCToTeleport, Actor Destination)
{
	if ( !bTeleportDisabled && PCToTeleport != none && Destination != none )
	{
		UTPlayerReplicationInfo(PCToTeleport.PlayerReplicationInfo).ServerTeleportToActor(Destination);
		SceneClient.CloseScene(self);
	}
}


/**
 * Teleport to the node
 */
function bool ButtonBarSelect(UIScreenObject InButton, int InPlayerIndex)
{
	local UTPlayerController UTPC;
	local UTMapInfo Mi;
	UTPC = GetUTPlayerOwner();
	if ( UTPC != none && Map != none )
	{
		MI = Map.GetMapInfo();
		if ( MI != none && MI.CurrentActor != none )
		{
			TeleportToActor( UTPC, MI.CurrentActor);
		}
	}
	return true;
}

/**
 * Close the map
 */
function bool ButtonBarCancel(UIScreenObject InButton, int InPlayerIndex)
{
	SceneClient.CloseScene(self);
	return true;
}

/**
 * Call back - Attempt to teleport
 */
function MapActorSelected(Actor Selected, UTPlayerController SelectedBy)
{
	if ( SelectedBy != none && Selected != none && Map != none )
	{
		TeleportToActor(SelectedBy, Selected);
	}
}

/**
 * The selected node on the map has changed.  Update the information screen
 */
function MapCurrentActorChanged(Actor Selected)
{
	// To-Do - Update info panel
}

/**
 * The home node was selected
 * @Param	InButton			The button that selected
 * @Param	InPlayerIndex		Index of the local player that made the selection
 */
function bool ButtonBarHomeNode(UIScreenObject InButton, int InPlayerIndex)
{
	local UTPlayerController UTPC;

	UTPC = GetUTPlayerOwner();
	if (UTPC != none && Map != none )
	{
		Map.HomeNodeSelected(UTPC);
	}
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
function bool InputKey( const out InputEventParameters EventParms )
{

`log("###"@EventParms.InputKEyName@EventParms.EventType);

	if (EventParms.EventType == IE_Released)
	{
		if ( EventParms.InputKeyName == 'XboxTypeS_A' )
		{
			ButtonBarSelect(none,EventParms.PlayerIndex);
		}

		else if ( EventParms.InputKeyName == 'Escape' || EventParms.InputKeyName == 'XboxTypeS_B' || (EventParms.InputKeyName =='F2' && bTeleportDisabled) )
		{
			ButtonBarCancel(none, EventParms.PlayerIndex);
		}
	}

	if ( EventParms.EventType == IE_Released && EventParms.InputKeyName == 'F12' && GetWorldInfo().NetMode == NM_Standalone )
	{
		bEditMap = !bEditMap;
		Map.bShowExtents = bEditMap;
	}

	if (bEditMap)
	{

		if ( (EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Released) &&
				(EventParms.InputKeyName == 'LeftShift' || EventParms.InputKeyName == 'RightShift') )
		{
			bFineTune = !bFineTune;
		}

		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Repeat )
		{
			if ( EventParms.InputKeyName == 'Minus' || EventParms.InputKeyName == 'Subtract' )
			{
				AdjustMapExtent(bFineTune ? -16.0 : -256.0);
			}
			else if ( EventParms.InputKeyName =='Equals' || EventParms.InputKeyName == 'Add' )
			{
				AdjustMapExtent(bFineTune ? 16.0 : 256.0);
			}

			else if ( EventParms.InputKeyName == 'I' || EventParms.InputKeyName =='NumPadEight' )
			{
				AdjustMapCenter(bFineTune ? vect(0,-16,0) : vect(0,-256,0));
			}
			else if ( EventParms.InputKeyName == 'M' || EventParms.InputKeyName =='NumPadTwo' )
			{
				AdjustMapCenter(bFineTune ? vect(0,16,0) : vect(0,256,0));
			}
			else if ( EventParms.InputKeyName == 'J' || EventParms.InputKeyName =='NumPadFour' )
			{
				AdjustMapCenter(bFineTune ? vect(-16,0,0) : vect(-256,0,0));
			}
			else if ( EventParms.InputKeyName == 'K' || EventParms.InputKeyName =='NumPadSix' )
			{
				AdjustMapCenter(bFineTune ? vect(16,0,0) : vect(256,0,0));
			}
		}
	}

	return true;
}


function AdjustMapExtent(float Adjustment)
{
	local UTOnslaughtMapInfo UMI;

	UMI = UTOnslaughtMapInfo( GetWorldInfo().GetMapInfo() );
	if ( UMI != None )
	{
		UMI.MapExtent += Adjustment;
		return;
	}
}

function AdjustMapCenter(vector Adjustment)
{
	local UTOnslaughtMapInfo UMI;

	UMI = UTOnslaughtMapInfo( GetWorldInfo().GetMapInfo() );
	if ( UMI != None )
	{
		UMI.MapCenter += Adjustment;
		return;
	}
}



function DisableTeleport()
{
	bTeleportDisabled = true;
	ButtonBar.ClearButton(1);
	Caption.SetVisibility(false);
}


defaultproperties
{
	SceneInputMode=INPUTMODE_MatchingOnly
	SceneRenderMode=SPLITRENDER_PlayerOwner
	bDisplayCursor=true
	bRenderParentScenes=false
	bAlwaysRenderScene=true
	bCloseOnLevelChange=true
}
