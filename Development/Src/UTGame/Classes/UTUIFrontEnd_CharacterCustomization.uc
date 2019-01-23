/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Character customization screen for UT3
 */
class UTUIFrontEnd_CharacterCustomization extends UTUIFrontEnd
	native(UIFrontEnd)
	dependson(UTCustomChar_Preview);

const CHARACTERCUSTOMIZATION_BUTTONBAR_ACCEPT = 0;
const CHARACTERCUSTOMIZATION_BUTTONBAR_BACK = 1;

cpptext
{
	virtual void Tick( FLOAT DeltaTime );
}

/** Reference to the actor that we are previewing part changes on. */
var transient UTCustomChar_Preview PreviewActor;

/** Reference to the currently visible parts panel. */
var transient UTUITabPage_CharacterPart CurrentPanel;
var transient UTUITabPage_CharacterPart PreviousPanel;

/** Panel that contains a bunch of widgets to show when loading a character package. */
var transient UIObject	LoadingPanel;

/** Panel that contains widgets to show when a character is finished loading. */
var transient UIObject	CharacterPanel;

/** List of customizable part panels. */
var transient array<UTUITabPage_CharacterPart>	PartPanels;

/** List of paper doll images. */
var transient array<UIImage> PartImages;

/** Data structure that contains information about the character package that is already loaded. */
var transient UTCharFamilyAssetStore	LoadedPackage;

/** Data structure that contains information about the character package that is being loaded. */
var transient UTCharFamilyAssetStore	PendingPackage;

/** Sound cue to play when shifting between tabs. */
var() transient SoundCue	TabShiftSoundCue;

/** The current faction we are viewing. */
var	transient string	Faction;

/** The name of the current character we are viewing. */
var transient string	CharacterID;

/** The current character we are viewing. */
var transient CharacterInfo	Character;

/** Whether or not we loaded character data. */
var transient bool bHaveLoadedCharData;

/** Loaded character data. */
var transient CustomCharData LoadedCharData;

/** Current direction to rotate the character in. */
var transient int RotationDirection;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize( )
{
	local int PartPanelIdx;

	local UIImage PartImage;
	local UTUITabPage_CharacterPart PartPanel;
	local UTCustomChar_Preview	PreviewActorIter;

	Super.PostInitialize();

	// Find a preview actor to set changes on.
	ForEach PlayerOwner.GetCurrentWorldInfo().AllActors(class'UTCustomChar_Preview', PreviewActorIter)
	{
		PreviewActor=PreviewActorIter;
		break;
	}

	// Store widget references
	PartPanelIdx = 0;
	PartPanel = UTUITabPage_CharacterPart(FindChild(name("pnlCharacterPart" $ PartPanelIdx), true));
	PartImage = UIImage(FindChild(name("imgPart" $ PartPanelIdx), true));

	while(PartPanel != none)
	{
		// Add the page to the tab control.
		TabControl.InsertPage(PartPanel, 0);

		// Display the first panel by default.
		if(PartPanelIdx==0)
		{
			PartPanel.SetVisibility(true);
		}
		else
		{
			PartPanel.SetVisibility(false);
		}

		PartPanel.OnPartSelected = OnPartSelected;
		PartPanel.OnPreviewPartChanged = OnPreviewPartChanged;
		PartPanels.AddItem(PartPanel);

		if(PartImage != none)
		{
			PartImage.SetVisibility(false);
		}
		PartImages.AddItem(PartImage);

		// Try to get another panel.
		PartPanelIdx++;
		PartPanel = UTUITabPage_CharacterPart(FindChild(name("pnlCharacterPart" $ PartPanelIdx), true));
		PartImage = UIImage(FindChild(name("imgPart" $ PartPanelIdx), true));
	}

	// Default to the first page.
	TabControl.ActivatePage(PartPanels[0], 0);
	CurrentPanel = UTUITabPage_CharacterPart(TabControl.ActivePage);

	// Store a reference to the loading panel.
	LoadingPanel = FindChild('pnlLoading', true);

	// Show the first part panel.
	PartPanels[0].SetVisibility(true);
	PartImages[0].SetVisibility(true);

	// Activate kismet for entering scene
	ActivateLevelEvent('CharacterCustomizationEnter');

	// Load the player's character data from the profile.
	LoadCharacterData();
}

/** Sets up the buttonbar for this scene. */
function SetupButtonBar()
{
	if(ButtonBar != None)
	{
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Cancel>", OnButtonBar_Back);
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Accept>", OnButtonBar_Accept);
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.ToggleShoulderType>", OnButtonBar_ToggleShoulderType);
	}
}

/** Starts loading a asset package. */
function StartLoadingPackage(string FamilyID)
{
	`Log("UTUIFrontEnd_CharacterCustomization::StartLoadingPackage() - Starting to load character part package (" $ Character.CharData.FamilyID $ ")");

	LoadedPackage = none;
	PendingPackage = class'UTCustomChar_Data'.static.LoadFamilyAssets(FamilyID, false, false);

	// Show loading panel.
	LoadingPanel.SetVisibility(true);

	// Hide character menus
	TabControl.SetVisibility(false);

	// Hide Preview Actor
	PreviewActor.SetHidden(true);
}

/** Checks the status of the package that is currently loading. */
event UpdateLoadingPackage()
{
	local int PanelIdx;
	local int ItemIdx;
	local string PartID;

	if(PendingPackage != none && PendingPackage.NumPendingPackages == 0)
	{
		`Log("UTUIFrontEnd_CharacterCustomization::UpdateLoadingPackage() - Pending character part package done loading (" $ Character.CharData.FamilyID $ ")");

		// Hide loading panel
		LoadingPanel.SetVisibility(false);	

		// set the character's data
		PreviewActor.SetCharacter(Character.Faction, Character.CharID);
		if(bHaveLoadedCharData)
		{
			PreviewActor.SetCharacterData(LoadedCharData);
		}

		// Update the currently selected list item for all parts.
		for(PanelIdx=0; PanelIdx<PartPanels.length; PanelIdx++)
		{
			switch(PartPanels[PanelIdx].CharPartType)
			{
			case PART_ShoPad:
				PartID = PreviewActor.Character.CharData.ShoPadID;
				break;
			case PART_Boots:
				PartID = PreviewActor.Character.CharData.BootsID;
				break;
			case PART_Thighs:
				PartID = PreviewActor.Character.CharData.ThighsID;
				break;
			case PART_Arms:
				PartID = PreviewActor.Character.CharData.ArmsID;
				break;
			case PART_Torso:
				PartID = PreviewActor.Character.CharData.TorsoID;
				break;
			case PART_Goggles:
				PartID = PreviewActor.Character.CharData.GogglesID;
				break;
			case PART_Facemask:
				PartID = PreviewActor.Character.CharData.FacemaskID;
				break;
			case PART_Helmet:
				PartID = PreviewActor.Character.CharData.HelmetID;
				break;
			default:
				`Log("UTUIFrontEnd_CharacterCustomization::UpdateLoadingPackage() - Invalid PartType "$PartPanels[PanelIdx].CharPartType);
				break;
			}

			ItemIdx = PartPanels[PanelIdx].PartList.FindCellFieldString(PartPanels[PanelIdx].PartList, 'PartID', PartID, false);
			PartPanels[PanelIdx].PartList.SetIndex(ItemIdx);
		}

		// Show Actor
		PreviewActor.SetHidden(false);

		// Show character menus
		TabControl.SetVisibility(true);
		TabControl.SetFocus(None);

		LoadedPackage = PendingPackage;
		PendingPackage = none;
	}
}

/** Loads the character data from the datastore. */
function LoadCharacterData()
{
	local bool bUseDefault;
	local string CharacterDataStr;
	local LocalPlayer LP;
	local string FamilyID;

	bUseDefault = true;
	LP = GetPlayerOwner();

	// Try to load data from the profile
	if(GetDataStoreStringValue("<OnlinePlayerData:ProfileData.CustomCharData>", CharacterDataStr, none, LP))
	{
		`Log("UTUIFrontEnd_CharacterCustomization::LoadCharacterData() - Loaded Profile Data, Value string: " $ CharacterDataStr);

		if(Len(CharacterDataStr) > 0)
		{
			LoadedCharData = class'UTCustomChar_Data'.static.CharDataFromString(CharacterDataStr);
			PreviewActor.Character.CharData = LoadedCharData;
			FamilyID = LoadedCharData.FamilyID;
			bUseDefault = false;
			bHaveLoadedCharData = true;
		}
	}


	if(bUseDefault)
	{
		`Log("UTUIFrontEnd_CharacterCustomization::LoadCharacterData() - Unable to load profile data, using default.");
		GetDataStoreStringValue("<UTCustomChar:Faction>", Faction);
		GetDataStoreStringValue("<UTCustomChar:Character>", CharacterID);

		Character = class'UTCustomChar_Data'.static.FindCharacter(Faction, CharacterID);
		FamilyID = Character.CharData.FamilyID;
		bHaveLoadedCharData = false;
	}

	// Finally load the package using our FamilyID
	SetDataStoreStringValue("<UTCustomChar:Family>", FamilyID);

	StartLoadingPackage(FamilyID);
}

/** Saves the character data to the datastore. */
function SaveCharacterData()
{
	local UIDataStore_OnlinePlayerData	PlayerDataStore;

	local string CharacterDataStr;
	local LocalPlayer LP;

	LP = GetPlayerOwner();

	// Save the character data to the profile.
	CharacterDataStr = class'UTCustomChar_Data'.static.CharDataToString(PreviewActor.Character.CharData);
	`Log("UTUIFrontEnd_CharacterCustomization::SaveCharacterData() - Saving Profile Data, Value string: " $ CharacterDataStr);

	SetDataStoreStringValue("<OnlinePlayerData:ProfileData.CustomCharData>", CharacterDataStr, none, LP);


	// Save profile
	PlayerDataStore = UIDataStore_OnlinePlayerData(FindDataStore('OnlinePlayerData', LP));

	if(PlayerDataStore != none)
	{
		`Log("UTUIFrontEnd_SettingsPanels::OnBack() - Saving player profile.");
		PlayerDataStore.SaveProfileData();
	}
	else
	{
		`Log("UTUIFrontEnd_SettingsPanels::OnBack() - Unable to locate OnlinePlayerData datastore for saving out profile.");
	}

}

/** Callback for when the user is trying to back out of the character customization menu. */
function OnBack()
{
	// Activate kismet for exiting scene
	ActivateLevelEvent('CharacterCustomizationExit');

	// Close the scene
	CloseScene(self);
}

/** Callback for when the player has accepted the changes to their character. */
function OnAccept()
{
	local UIScene FactionSelectionScene;

	// Save their character data
	SaveCharacterData();

	// Activate kismet for exiting scene
	ActivateLevelEvent('CharacterCustomizationExit');

	// Close the scene, if we came through the create new character flow, then close the faction scene
	// so that all of the scenes above it are closed.
	FactionSelectionScene = GetSceneClient().FindSceneByTag('CharacterFaction');

	if(FactionSelectionScene!=None)
	{
		CloseScene(FactionSelectionScene);
	}
	else
	{
		CloseScene(self);
	}
}

/** Callback for when the user is trying to toggle the shoulder type of the character. */
function OnToggleShoulderType()
{
	// @todo: This could probably use an enum.
	if(PreviewActor.Character.CharData.bHasLeftShoPad && PreviewActor.Character.CharData.bHasRightShoPad)	// Both
	{
		// None
		PreviewActor.Character.CharData.bHasLeftShoPad = false;
		PreviewActor.Character.CharData.bHasRightShoPad = false;
	}
	else if(PreviewActor.Character.CharData.bHasLeftShoPad==false && PreviewActor.Character.CharData.bHasRightShoPad==false)	// None
	{
		// Left Only
		PreviewActor.Character.CharData.bHasLeftShoPad = true;
		PreviewActor.Character.CharData.bHasRightShoPad = false;
	}
	else if(PreviewActor.Character.CharData.bHasLeftShoPad==true && PreviewActor.Character.CharData.bHasRightShoPad==false)	// Left
	{
		// Right Only
		PreviewActor.Character.CharData.bHasLeftShoPad = false;
		PreviewActor.Character.CharData.bHasRightShoPad = true;
	}
	else if(PreviewActor.Character.CharData.bHasLeftShoPad==false && PreviewActor.Character.CharData.bHasRightShoPad==true)	// Right
	{
		// Both
		PreviewActor.Character.CharData.bHasLeftShoPad = true;
		PreviewActor.Character.CharData.bHasRightShoPad = true;
	}

	// Tell the actor to update its look since the data changed.
	PreviewActor.NotifyCharacterDataChanged();
}

/** Button bar callbacks - Accept Button */
function bool OnButtonBar_Accept(UIScreenObject InButton, int InPlayerIndex)
{
	OnAccept();

	return true;
}

/** Button bar callbacks - Back Button */
function bool OnButtonBar_Back(UIScreenObject InButton, int InPlayerIndex)
{
	OnBack();

	return true;
}

/** Button bar callbacks - Toggle shoulder type Button */
function bool OnButtonBar_ToggleShoulderType(UIScreenObject InButton, int InPlayerIndex)
{
	OnToggleShoulderType();

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

	if(EventParms.EventType==IE_Released || EventParms.EventType==IE_Repeat)
	{
		if(CurrentPanel.IsAnimating()==false)
		{
			//@todo joew - could make this an alias "ShowPrevTab" & "ShowNextTab", then hook into OnProcessInputKey rather than OnRawInputKey
			if(EventParms.InputKeyName=='Gamepad_RightStick_Left' || EventParms.InputKeyName=='Gamepad_LeftStick_Left' || EventParms.InputKeyName=='XboxTypeS_DPad_Left' || EventParms.InputKeyName=='Left')
			{
				ShowPrevTab();
				bResult=true;
			}
			else if(EventParms.InputKeyName=='Gamepad_RightStick_Right' || EventParms.InputKeyName=='Gamepad_LeftStick_Right' || EventParms.InputKeyName=='XboxTypeS_DPad_Right' || EventParms.InputKeyName=='Right')
			{
				ShowNextTab();
				bResult=true;
			}
		}

		if(EventParms.InputKeyName=='XboxTypeS_B' || EventParms.InputKeyName=='Escape')
		{
			OnBack();
			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_A')
		{
			OnAccept();
			bResult=true;
		}
		else if(EventParms.InputKeyName=='XboxTypeS_Y')
		{
			OnToggleShoulderType();
			bResult=true;
		}
	}

	if(EventParms.EventType==IE_Pressed)
	{
		if(EventParms.InputKeyName=='X' || EventParms.InputKeyName=='XboxTypeS_RightTrigger')
		{
			RotationDirection = -1;
		}
		else if(EventParms.InputKeyName=='Z' || EventParms.InputKeyName=='XboxTypeS_LeftTrigger')
		{
			RotationDirection = 1;
		}
	}
	else if(EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='X' || EventParms.InputKeyName=='Z' || EventParms.InputKeyName=='XboxTypeS_LeftTrigger' || EventParms.InputKeyName=='XboxTypeS_RightTrigger')
		{
			RotationDirection = 0;
		}
	}

	return bResult;
}


/**
 * Called when the accepts a current list selection.
 *
 * @param	PartType	The type of part we are changing.
 * @param	string		The id of the part we are changing.
 */
function OnPartSelected( ECharPart PartType, string PartID )
{
	OnAccept();
}

/**
 * Called when the user changes the selected index of a parts list.
 *
 * @param	PartType	The type of part we are changing.
 * @param	string		The id of the part we are changing.
 */
function OnPreviewPartChanged( ECharPart PartType, string PartID )
{
	PreviewActor.SetPart(PartType, PartID);
}

/**
 * Called when a new page is activated.
 *
 * @param	Sender			the tab control that activated the page
 * @param	NewlyActivePage	the page that was just activated
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated this event.
 */
function OnPageActivated( UITabControl Sender, UITabPage NewlyActivePage, int PlayerIndex )
{
	local int PrevIndex;
	local int CurrentIndex;
	local bool bMoveLeft;

	if(CurrentPanel != NewlyActivePage)
	{
		PreviousPanel = CurrentPanel;
		CurrentPanel = UTUITabPage_CharacterPart(NewlyActivePage);

		PrevIndex = Sender.FindPageIndexByPageRef(PreviousPanel);
		CurrentIndex = Sender.FindPageIndexByPageRef(CurrentPanel);

		// Check to see which way to move the items, 
		// if we are wrapping the list we need to move opposite of the direction as normal
		
		if(PrevIndex > CurrentIndex)
		{
			bMoveLeft = true;
		}
		else
		{
			bMoveLeft = false;
		}

		if(Abs(PrevIndex-CurrentIndex) > 1.0f)
		{
			bMoveLeft = !bMoveLeft;
		}

		if(bMoveLeft)	
		{
			PreviousPanel.StartAnim(TAD_RightOut);
			CurrentPanel.StartAnim(TAD_LeftIn);
		}
		else
		{
			PreviousPanel.StartAnim(TAD_LeftOut);
			CurrentPanel.StartAnim(TAD_RightIn);
		}

		UpdatePaperDoll();
	}
}

/** Shows the previous parts panel. */
function ShowPrevTab()
{
	PreviousPanel = CurrentPanel;
	TabControl.ActivatePreviousPage(0);
	CurrentPanel = UTUITabPage_CharacterPart(TabControl.ActivePage);

	PreviousPanel.StartAnim(TAD_RightOut);
	CurrentPanel.StartAnim(TAD_LeftIn);

	UpdatePaperDoll();
}

/** Shows the next parts panel. */
function ShowNextTab()
{
	PreviousPanel = CurrentPanel;
	TabControl.ActivateNextPage(0);
	CurrentPanel = UTUITabPage_CharacterPart(TabControl.ActivePage);

	PreviousPanel.StartAnim(TAD_LeftOut);
	CurrentPanel.StartAnim(TAD_RightIn);

	UpdatePaperDoll();
}

/** Updates the paper doll part visibility using the current/previous part panels. */
function UpdatePaperDoll()
{
	local int PreviousIdx;
	local int CurrentIdx;

	if(TabControl != None)
	{
		PreviousIdx = TabControl.FindPageIndexByPageRef(PreviousPanel);
		CurrentIdx = TabControl.FindPageIndexByPageRef(CurrentPanel);

		if(PreviousIdx != INDEX_NONE)
		{
			PartImages[PreviousIdx].SetVisibility(false);
		}

		if(CurrentIdx != INDEX_NONE)
		{
			PartImages[CurrentIdx].SetVisibility(true);
		}
	}
	
	ActivateLevelEventForCurrentPart();
}

/** Activates the level kismet remote event for the currently selected part. */
function ActivateLevelEventForCurrentPart()
{
	local name EventName;

	switch(CurrentPanel.CharPartType)
	{
	case PART_Boots:
		EventName = 'CharacterCustomization_Boots';
		break;
	case PART_Thighs:
		EventName = 'CharacterCustomization_Thighs';
		break;
	case PART_Arms:
		EventName = 'CharacterCustomization_Arms';
		break;
	case PART_Torso:
		EventName = 'CharacterCustomization_Torso';
		break;
	case PART_ShoPad:
		EventName = 'CharacterCustomization_ShoPad';
		break;
	case PART_Helmet:
		EventName = 'CharacterCustomization_Helmet';
		break;
	case PART_Facemask:
		EventName = 'CharacterCustomization_Facemask';
		break;
	case PART_Goggles:
		EventName = 'CharacterCustomization_Goggles';
		break;
	}

	ActivateLevelEvent(EventName);
}

defaultproperties
{
	TabShiftSoundCue=SoundCue'A_Interface.Menu.UT3ServerSignOutCue'
	Faction="IronGuard"
	CharacterID="A"
}
