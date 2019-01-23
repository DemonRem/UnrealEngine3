/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Map selection screen for UT3
 */

class UTUITabPage_MapSelection extends UTTabPage
	placeable;

/** Reference to the map cycle configuration scene. */
var string MapCycleScene;

/** Preview image for a map. */
var transient UIImage	MapPreviewImage[3];

/** Description label for the map. */
var transient UILabel	DescriptionLabel;

/** List of maps widget. */
var transient UTUIMenuList MapList;

/** Delegate for when the user selects a map on this page. */
delegate OnMapSelected(string InMapName, bool bSelectionSubmitted);

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Setup delegates
	MapList = UTUIMenuList(FindChild('lstMaps', true));
	if(MapList != none)
	{
		MapList.OnSubmitSelection = OnMapList_SubmitSelection;
		MapList.OnValueChanged = OnMapList_ValueChanged;
	}

	// Store widget references
	MapPreviewImage[0] = UIImage(FindChild('imgMapPreview1', true));
	MapPreviewImage[1] = UIImage(FindChild('imgMapPreview2', true));
	MapPreviewImage[2] = UIImage(FindChild('imgMapPreview3', true));

	DescriptionLabel = UILabel(FindChild('lblDescription', true));

	// Setup tab caption
	SetDataStoreBinding("<Strings:UTGameUI.FrontEnd.TabCaption_Map>");
}


/** Sets up the button bar for the parent scene. */
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.MapCycle>", OnButtonBar_MapCycle);
}

/** Shows the map cycle configuration scene. */
function OnShowMapCycle()
{
	UTUIScene(GetScene()).OpenSceneByName(MapCycleScene);
}

/**
 * Called when the user changes the current list index.
 *
 * @param	Sender	the list that is submitting the selection
 */
function OnMapList_SubmitSelection( UIList Sender, optional int PlayerIndex=0 )
{
	local int SelectedItem;
	local UTUIMenuList MenuList;
	local string MapName;

	MenuList = UTUIMenuList(Sender);

	if(MenuList != none)
	{
		SelectedItem = MenuList.GetCurrentItem();

		if(MenuList.GetCellFieldString(MenuList, 'MapName', SelectedItem, MapName))
		{
			`Log("Current Map: " $ MapName);
			OnMapSelected(MapName, true);
		}
	}
}

/**
 * Called when the user presses Enter (or any other action bound to UIKey_SubmitListSelection) while this list has focus.
 *
 * @param	Sender	the list that is submitting the selection
 */
function OnMapList_ValueChanged( UIObject Sender, optional int PlayerIndex=0 )
{
	local int SelectedItem;
	local UTUIMenuList MenuList;
	local string StringValue;
	local string MapName;

	// Get the map's name from the list.
	MenuList = UTUIMenuList(Sender);

	if(MenuList != none)
	{
		SelectedItem = MenuList.GetCurrentItem();

		// Preview Image
		if(MenuList.GetCellFieldString(MenuList, 'PreviewImageMarkup', SelectedItem, StringValue))
		{
			SetPreviewImageMarkup(StringValue);
		}

		// Map Description
		if(MenuList.GetCellFieldString(MenuList, 'Description', SelectedItem, StringValue))
		{
			DescriptionLabel.SetDatastoreBinding(StringValue);
		}

		// Map name
		if(MenuList.GetCellFieldString(MenuList, 'MapName', SelectedItem, MapName))
		{
			OnMapSelected(MapName, false);
		}
	}
}

/** Changes the preview image for a map. */
function SetPreviewImageMarkup(string InMarkup)
{
	MapPreviewImage[0].SetDatastoreBinding(InMarkup);
	MapPreviewImage[1].SetDatastoreBinding(InMarkup);
	MapPreviewImage[2].SetDatastoreBinding(InMarkup);
}

/** Buttonbar Callbacks. */
function bool OnButtonBar_MapCycle(UIScreenObject InButton, int PlayerIndex)
{
	OnShowMapCycle();

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
		if(EventParms.InputKeyName=='XboxTypeS_Y')		// Move mutator
		{
			OnShowMapCycle();

			bResult=true;
		}
	}

	return bResult;
}

defaultproperties
{
	MapCycleScene="UI_Scenes_FrontEnd.Scenes.MapCycle"
}