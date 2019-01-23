/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page to let user's select which mutators to use.
 */

class UTUITabPage_Mutators extends UTTabPage
	placeable;

/** List of available mutators. */
var transient UIList AvailableList;

/** List of enabled mutators. */
var transient UIList EnabledList;

/** Label describing the currently selected mutator. */
var transient UILabel DescriptionLabel;

/** Arrow images. */
var transient UIImage ShiftRightImage;
var transient UIImage ShiftLeftImage;

/** Reference to the menu datastore */
var transient UTUIDataStore_MenuItems MenuDataStore;

/** Callback for when the user decides to accept the current set of mutators. */
delegate OnAcceptMutators(string InEnabledMutators);

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Store widget references
	AvailableList = UIList(FindChild('lstAvailable', true));
	AvailableList.OnValueChanged = OnAvailableList_ValueChanged;
	AvailableList.OnSubmitSelection = OnAvailableList_SubmitSelection;
	AvailableList.NotifyActiveStateChanged = OnAvailableList_NotifyActiveStateChanged;
	AvailableList.OnRawInputKey=OnMutatorList_RawInputKey;

	EnabledList = UIList(FindChild('lstEnabled', true));
	EnabledList.OnValueChanged = OnEnabledList_ValueChanged;
	EnabledList.OnSubmitSelection = OnEnabledList_SubmitSelection;
	EnabledList.NotifyActiveStateChanged = OnEnabledList_NotifyActiveStateChanged;
	EnabledList.OnRawInputKey=OnMutatorList_RawInputKey;

	DescriptionLabel = UILabel(FindChild('lblDescription', true));
	ShiftRightImage = UIImage(FindChild('imgArrowLeft', true));
	ShiftLeftImage = UIImage(FindChild('imgArrowRight', true));

	// Get reference to the menu datastore
	MenuDataStore = UTUIDataStore_MenuItems(GetCurrentUIController().DataStoreManager.FindDataStore('UTMenuItems'));

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.FrontEnd.TabCaption_Mutators>");
}

/** Sets up the button bar for the parent scene. */
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	if(IsVisible())
	{
		ButtonBar.SetButton(1,"<Strings:UTGameUI.ButtonCallouts.Next>", OnButtonBar_Next);

		if(EnabledList.IsFocused())
		{
			ButtonBar.SetButton(2,"<Strings:UTGameUI.ButtonCallouts.RemoveMutator>", OnButtonBar_MoveMutator);
		}
		else
		{
			ButtonBar.SetButton(2,"<Strings:UTGameUI.ButtonCallouts.AddMutator>", OnButtonBar_MoveMutator);
		}

		// Only set the configure button if they are able to configure the currently selected mutator.
		ButtonBar.ClearButton(3);

		if(EnabledList.IsFocused() && IsCurrentMutatorConfigurable())
		{
			ButtonBar.SetButton(3,"<Strings:UTGameUI.ButtonCallouts.ConfigureMutator>", OnButtonBar_ConfigureMutator);
		}
	}
}


/** Attempts to filter the mutator list to ensure that there are no duplicate groups or mutators enabled that can not be enabled. */
function AddMutatorAndFilterList(int NewMutator)
{
	local bool bFiltered;
	local int MutatorIdx;
	local string GroupName;
	local string CompareGroupName;
	local array<int> FinalItems;

	// Group Name
	if(class'UTUIMenuList'.static.GetCellFieldString(EnabledList, 'GroupName', NewMutator, GroupName)==false)
	{
		GroupName="";
	}	

	// we can only have 1 mutator of a specified group enabled at a time, so filter all of the mutators that are of the group we are currently adding.
	if(Len(GroupName) > 0)
	{	
		for(MutatorIdx=0; MutatorIdx<EnabledList.Items.length; MutatorIdx++)
		{
			bFiltered = false;

			if(class'UTUIMenuList'.static.GetCellFieldString(EnabledList, 'GroupName', EnabledList.Items[MutatorIdx], CompareGroupName))
			{
				if(CompareGroupName==GroupName)
				{
					bFiltered = true;
				}
			}

			if(bFiltered == false)
			{
				FinalItems.AddItem(EnabledList.Items[MutatorIdx]);
			}
		}
	}

	// Update final item list.
	FinalItems.AddItem(NewMutator);
	MenuDataStore.EnabledMutators = FinalItems;
}

function name GetClassNameFromIndex(int MutatorIndex)
{
	local name Result;
	local string DataStoreMarkup;
	local string OutValue;

	Result = '';

	DataStoreMarkup = "<UTMenuItems:Mutators;"$MutatorIndex$".ClassName>";
	if(GetDataStoreStringValue(DataStoreMarkup, OutValue))
	{
		Result = name(OutValue);
	}

	return Result;
}

/** Called whenever one of the mutator lists changes. */
function OnMutatorListChanged()
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

/** Modifies the enabled mutator array to enable/disable a mutator. */
function SetMutatorEnabled(int MutatorId, bool bEnabled)
{
	`Log("UTUITabPage_Mutators::SetMutatorEnabled() - MutatorId: "$MutatorId$", bEnabled: "$bEnabled);

	// Get Mutator class from index
	if(bEnabled)
	{
		if(MenuDataStore.EnabledMutators.Find(MutatorId)==INDEX_NONE)
		{
			AddMutatorAndFilterList(MutatorId);
			OnMutatorListChanged();
		}
	}
	else
	{
		if(MenuDataStore.EnabledMutators.Find(MutatorId)!=INDEX_NONE)
		{
			MenuDataStore.EnabledMutators.RemoveItem(MutatorId);

			// If we removed all of the enabled mutators, set focus back to the available list.
			if(MenuDataStore.EnabledMutators.length==0 && EnabledList.IsFocused())
			{
				AvailableList.SetFocus(none);
			}

			OnMutatorListChanged();
		}
	}
}

/** Clears the enabled mutator list. */
function OnClearMutators()
{
	MenuDataStore.EnabledMutators.length=0;
	OnMutatorListChanged();
	
	// Set focus to the available list.
	AvailableList.SetFocus(none);
	OnSelectedMutatorChanged();
}

/** Updates widgets when the currently selected mutator changes. */
function OnSelectedMutatorChanged()
{
	UpdateDescriptionLabel();
	SetupButtonBar(UTUIFrontEnd(GetScene()).ButtonBar);

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

/** Callback for when the user tries to move a mutator from one list to another. */
function OnMoveMutator()
{
	if(EnabledList.IsFocused())
	{
		SetMutatorEnabled(EnabledList.GetCurrentItem(), false);
	}
	else if(AvailableList.IsFocused())
	{
		SetMutatorEnabled(AvailableList.GetCurrentItem(), true);
	}

	OnSelectedMutatorChanged();
}

/** @return Returns whether or not the current mutator is configurable. */
function bool IsCurrentMutatorConfigurable()
{
	local string ConfigureSceneName;
	local bool bResult;

	bResult = false;

	if(class'UTUIMenuList'.static.GetCellFieldString(EnabledList, 'UIConfigScene', EnabledList.GetCurrentItem(), ConfigureSceneName))
	{
		if(Len(ConfigureSceneName)>0)
		{
			bResult = true;
		}
	}

	return bResult;
}

/** Loads the configuration scene for the currently selected mutator. */
function OnConfigureMutator()
{
	local string ConfigureSceneName;
	local UIScene ConfigureScene;
	local UTUIScene OwnerUTScene;

	if(class'UTUIMenuList'.static.GetCellFieldString(EnabledList, 'UIConfigScene', EnabledList.GetCurrentItem(), ConfigureSceneName))
	{
		if (class'WorldInfo'.static.IsConsoleBuild())
		{
			ConfigureScene = UIScene(FindObject(ConfigureSceneName, class'UIScene'));
		}
		else
		{
			ConfigureScene = UIScene(DynamicLoadObject(ConfigureSceneName, class'UIScene'));
		}

		if(ConfigureScene != none)
		{
			OwnerUTScene = UTUIScene(GetScene());
			OwnerUTScene.OpenScene(ConfigureScene);
		}
		else
		{
			`Log("UTUITabPage_Mutators::OnConfigureMutator() - Unable to find scene '"$ConfigureSceneName$"'");
		}
	}
}

/** The user has finished setting up their mutators and wants to continue on. */
function OnNext()
{
	local string MutatorString;
	local string ClassName;
	local int MutatorIdx;

	for(MutatorIdx=0; MutatorIdx<MenuDataStore.EnabledMutators.length; MutatorIdx++)
	{
		if(class'UTUIMenuList'.static.GetCellFieldString(EnabledList, 'ClassName', MenuDataStore.EnabledMutators[MutatorIdx], ClassName))
		{
			if(MutatorIdx > 0)
			{
				MutatorString = MutatorString$","$ClassName;
			}
			else
			{
				MutatorString = ClassName;
			}
		}
	}

	// Fire our mutators accepted delegate.
	OnAcceptMutators(MutatorString);
}

/** Updates the description label. */
function UpdateDescriptionLabel()
{
	local string NewDescription;
	local int SelectedItem;
	local UIList CurrentList;

	CurrentList = AvailableList.IsFocused() ? AvailableList : EnabledList;
	SelectedItem = CurrentList.GetCurrentItem();

	if(class'UTUIMenuList'.static.GetCellFieldString(CurrentList, 'Description', SelectedItem, NewDescription))
	{
		DescriptionLabel.SetDataStoreBinding(NewDescription);
	}
}

/**
 * Callback for when the user selects a new item in the available list.
 */
function OnAvailableList_ValueChanged( UIObject Sender, int PlayerIndex )
{
	OnSelectedMutatorChanged();
}

/**
 * Callback for when the user submits the selection on the available list.
 */
function OnAvailableList_SubmitSelection( UIList Sender, optional int PlayerIndex=GetBestPlayerIndex() )
{
	OnMoveMutator();
}

/** Callback for when the object's active state changes. */
function OnAvailableList_NotifyActiveStateChanged( UIScreenObject Sender, int PlayerIndex, UIState NewlyActiveState, optional UIState PreviouslyActiveState )
{
	if(NewlyActiveState.Class == class'UIState_Focused'.default.Class)
	{
		OnSelectedMutatorChanged();
	}
}

/**
 * Callback for when the user selects a new item in the enabled list.
 */
function OnEnabledList_ValueChanged( UIObject Sender, int PlayerIndex )
{
	OnSelectedMutatorChanged();
}

/**
 * Callback for when the user submits the selection on the enabled list.
 */
function OnEnabledList_SubmitSelection( UIList Sender, optional int PlayerIndex=GetBestPlayerIndex() )
{
	OnMoveMutator();
}

/** Callback for when the object's active state changes. */
function OnEnabledList_NotifyActiveStateChanged( UIScreenObject Sender, int PlayerIndex, UIState NewlyActiveState, optional UIState PreviouslyActiveState )
{
	if(NewlyActiveState.Class == class'UIState_Focused'.default.Class)
	{
		OnSelectedMutatorChanged();
	}
}

/** Callback for the mutator lists, captures the accept button before the mutators get to it. */
function bool OnMutatorList_RawInputKey( const out InputEventParameters EventParms )
{
	local bool bResult;

	bResult = false;

	if(EventParms.EventType==IE_Released && EventParms.InputKeyName=='XboxTypeS_A')
	{
		OnNext();
		bResult = true;
	}

	return bResult;
}


/** Buttonbar Callbacks. */
function bool OnButtonBar_ConfigureMutator(UIScreenObject InButton, int PlayerIndex)
{
	OnConfigureMutator();

	return true;
}

function bool OnButtonBar_ClearMutators(UIScreenObject InButton, int PlayerIndex)
{
	OnClearMutators();

	return true;
}

function bool OnButtonBar_MoveMutator(UIScreenObject InButton, int PlayerIndex)
{
	OnMoveMutator();

	return true;
}

function bool OnButtonBar_Next(UIScreenObject InButton, int PlayerIndex)
{
	OnNext();

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
		if(EventParms.InputKeyName=='XboxTypeS_A' || EventParms.InputKeyName=='XboxTypeS_Enter')		// Accept mutators
		{
			OnNext();

			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_Y')		// Move mutator
		{
			OnMoveMutator();

			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_X')		// Clear mutators
		{
			OnConfigureMutator();

			bResult=true;
		}
	}

	return bResult;
}





