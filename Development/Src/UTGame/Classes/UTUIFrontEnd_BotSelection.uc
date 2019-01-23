/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * UI scene that allows the user to select which bots they want to play with.
 */
class UTUIFrontEnd_BotSelection extends UTUIFrontEnd
	config(Game);

/** List of available maps. */
var transient UIList AvailableList;

/** List of enabled maps. */
var transient UIList EnabledList;

/** The last focused UI List. */
var transient UIList LastFocused;

/** Label describing the currently selected mutator. */
var transient UILabel DescriptionLabel;

/** Arrow images. */
var transient UIImage ShiftRightImage;
var transient UIImage ShiftLeftImage;

/** Reference to the customchar datastore */
var transient UTUIDataStore_CustomChar CharacterDataStore;

/** Delegate or when the user accepts their current bot selection set. */
delegate OnAcceptedBots();

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Store widget references
	AvailableList = UIList(FindChild('lstAvailable', true));
	AvailableList.OnValueChanged = OnAvailableList_ValueChanged;
	AvailableList.OnSubmitSelection = OnAvailableList_SubmitSelection;
	AvailableList.NotifyActiveStateChanged = OnAvailableList_NotifyActiveStateChanged;

	EnabledList = UIList(FindChild('lstEnabled', true));
	EnabledList.OnValueChanged = OnEnabledList_ValueChanged;
	EnabledList.OnSubmitSelection = OnEnabledList_SubmitSelection;
	EnabledList.NotifyActiveStateChanged = OnEnabledList_NotifyActiveStateChanged;

	DescriptionLabel = UILabel(FindChild('lblDescription', true));
	ShiftRightImage = UIImage(FindChild('imgArrowLeft', true));
	ShiftLeftImage = UIImage(FindChild('imgArrowRight', true));

	// Get reference to the menu datastore
	CharacterDataStore = UTUIDataStore_CustomChar(GetCurrentUIController().DataStoreManager.FindDataStore('UTCustomChar'));

	// Load the bot list from INI
	LoadBotList();
}

/** Sets up the button bar for the parent scene. */
function SetupButtonBar()
{
	ButtonBar.Clear();
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Cancel>", OnButtonBar_Back);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Accept>", OnButtonBar_Accept);

	if(LastFocused==EnabledList)
	{
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.RemoveBot>", OnButtonBar_MoveBot);
	}
	else
	{
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.AddBot>", OnButtonBar_MoveBot);
	}
}


/** Loads the bot list from UTGame.default */
function LoadBotList()
{
	local int LocateIdx;
	local int BotIdx;

	CharacterDataStore.ActiveBots.length = 0;

	for(BotIdx=0; BotIdx<class'UTGame'.default.ActiveBots.length; BotIdx++)
	{
		LocateIdx = CharacterDataStore.FindValueInProviderSet('Character', 'FriendlyName', class'UTGame'.default.ActiveBots[BotIdx].BotName);

		if(LocateIdx != INDEX_NONE)
		{
			CharacterDataStore.ActiveBots.AddItem(LocateIdx);
		}
	}

	OnBotListChanged();
}

/** Saves out the current list bot bot names to use to UTGame.default */
function SaveBotList()
{
	local int BotIdx;
	local string MapName;
	local ActiveBotInfo NewInfo;

	class'UTGame'.default.ActiveBots.length = 0;

	for(BotIdx=0; BotIdx<CharacterDataStore.ActiveBots.length; BotIdx++)
	{
		if(CharacterDataStore.GetValueFromProviderSet('Character', 'FriendlyName', CharacterDataStore.ActiveBots[BotIdx], MapName))
		{
			NewInfo.BotName = MapName;
			class'UTGame'.default.ActiveBots.AddItem(NewInfo);
		}
	}

	// Save the config for this class.
	class'UTGame'.static.StaticSaveConfig();
}

/** Called whenever one of the bot lists changes. */
function OnBotListChanged()
{
	local int EnabledIndex;
	local int AvailableIndex;

	AvailableIndex = AvailableList.Index;
	EnabledIndex = EnabledList.Index;

	// Have both lists refresh their subscriber values
	AvailableList.RefreshSubscriberValue();
	EnabledList.RefreshSubscriberValue();

	AvailableList.SetIndex(AvailableIndex);
	EnabledList.SetIndex(EnabledIndex);
}

/** Clears the enabled bot list. */
function OnClearBots()
{
	CharacterDataStore.ActiveBots.length=0;
	OnBotListChanged();

	// Set focus to the available list.
	AvailableList.SetFocus(none);
	OnSelectedBotChanged();
}

/** Updates widgets when the currently selected bot changes. */
function OnSelectedBotChanged()
{
	UpdateDescriptionLabel();
	SetupButtonBar();

	// Update arrows
	if(EnabledList.IsFocused())
	{
		ShiftLeftImage.SetEnabled(false);
		ShiftRightImage.SetEnabled(true);
	}
	else
	{
		ShiftLeftImage.SetEnabled(true);
		ShiftRightImage.SetEnabled(false);
	}
}

/** Callback for when the user tries to move a bot from one list to another. */
function OnMoveBot()
{
	local int BotId;

	if(LastFocused==AvailableList)
	{
		BotId = AvailableList.GetCurrentItem();
		if(CharacterDataStore.ActiveBots.Find(BotId)==INDEX_NONE)
		{
			CharacterDataStore.ActiveBots.AddItem(BotId);
			OnBotListChanged();
		}
	}
	else
	{
		BotId = EnabledList.GetCurrentItem();
		if(CharacterDataStore.ActiveBots.Find(BotId)!=INDEX_NONE)
		{
			CharacterDataStore.ActiveBots.RemoveItem(BotId);

			// If we removed all of the enabled mutators, set focus back to the available list.
			if(CharacterDataStore.ActiveBots.length==0 && EnabledList.IsFocused())
			{
				AvailableList.SetFocus(none);
			}

			OnBotListChanged();
		}
	}

	OnSelectedBotChanged();
}


/** The user has finished setting up their cycle and wants to save changes. */
function OnAccept()
{
	local int NumBots;

	// Save out the bot list to INI
	SaveBotList();

	// Update num bots option
	NumBots = EnabledList.GetItemCount();
	SetDataStoreStringValue("<UTGameSettings:NumBots>", string(NumBots));

	// Let the previous scene know we accepted our changes.
	OnAcceptedBots();

	CloseScene(self);
}

/** The user wants to back out of the map cycle scene. */
function OnBack()
{
	CloseScene(self);
}

/** Updates the description label. */
function UpdateDescriptionLabel()
{
	local string NewDescription;
	local int SelectedItem;

	SelectedItem = LastFocused.GetCurrentItem();

	if(class'UTUIMenuList'.static.GetCellFieldString(LastFocused, 'Description', SelectedItem, NewDescription))
	{
		DescriptionLabel.SetDataStoreBinding(NewDescription);
	}
}

/**
 * Callback for when the user selects a new item in the available list.
 */
function OnAvailableList_ValueChanged( UIObject Sender, int PlayerIndex )
{
	OnSelectedBotChanged();
}

/**
 * Callback for when the user submits the selection on the available list.
 */
function OnAvailableList_SubmitSelection( UIList Sender, optional int PlayerIndex=GetBestPlayerIndex() )
{
	OnMoveBot();
}

/** Callback for when the object's active state changes. */
function OnAvailableList_NotifyActiveStateChanged( UIScreenObject Sender, int PlayerIndex, UIState NewlyActiveState, optional UIState PreviouslyActiveState )
{
	if(NewlyActiveState.Class == class'UIState_Focused'.default.Class)
	{
		LastFocused = AvailableList;
		OnSelectedBotChanged();
	}
}

/**
 * Callback for when the user selects a new item in the enabled list.
 */
function OnEnabledList_ValueChanged( UIObject Sender, int PlayerIndex )
{
	OnSelectedBotChanged();
}

/**
 * Callback for when the user submits the selection on the enabled list.
 */
function OnEnabledList_SubmitSelection( UIList Sender, optional int PlayerIndex=GetBestPlayerIndex() )
{
	OnMoveBot();
}

/** Callback for when the object's active state changes. */
function OnEnabledList_NotifyActiveStateChanged( UIScreenObject Sender, int PlayerIndex, UIState NewlyActiveState, optional UIState PreviouslyActiveState )
{
	if(NewlyActiveState.Class == class'UIState_Focused'.default.Class)
	{
		LastFocused = EnabledList;
		OnSelectedBotChanged();
	}
}


/** Buttonbar Callbacks. */
function bool OnButtonBar_Accept(UIScreenObject InButton, int PlayerIndex)
{
	OnAccept();

	return true;
}

function bool OnButtonBar_Back(UIScreenObject InButton, int PlayerIndex)
{
	OnBack();

	return true;
}

function bool OnButtonBar_ClearBots(UIScreenObject InButton, int PlayerIndex)
{
	OnClearBots();

	return true;
}

function bool OnButtonBar_MoveBot(UIScreenObject InButton, int PlayerIndex)
{
	OnMoveBot();

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
		if(EventParms.InputKeyName=='XboxTypeS_A' || EventParms.InputKeyName=='XboxTypeS_Enter')	// Accept Cycle
		{
			OnAccept();

			bResult=true;
		}
		if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')				// Cancel
		{
			OnBack();

			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_Y')		// Move map
		{
			OnMoveBot();

			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_X')		// Clear map cycle
		{
			OnClearBots();

			bResult=true;
		}
	}

	return bResult;
}


