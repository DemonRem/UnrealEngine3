/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Multiplayer Scene for UT3.
 */

class UTUIFrontEnd_Multiplayer extends UTUIFrontEnd_BasicMenu
	dependson(UTCustomChar_Data)
	dependson(UTUIScene_MessageBox);

const MULTIPLAYER_OPTION_QUICKMATCH = 0;
const MULTIPLAYER_OPTION_HOSTGAME = 1;
const MULTIPLAYER_OPTION_JOINGAME = 2;
const MULTIPLAYER_OPTION_CREATEPARTY = 3;

/** Reference to the quick match scene. */
var string	QuickMatchScene;

/** Reference to the host game scene. */
var string	HostScene;

/** Reference to the join game scene. */
var string	JoinScene;

/** Reference to the party scene. */
var string	PartyScene;

/** Reference to the character selection scene. */
var string	CharacterSelectionScene;

/** Whether or not we have already displayed the new player message box to the user. */
var transient	bool bDisplayedNewPlayerMessageBox;


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

/** @return bool Returns whether or not this user has saved character data. */
function bool HasSavedCharacterData()
{
	local bool bHaveLoadedCharData;
	local string CharacterDataStr;

	bHaveLoadedCharData = false;

	
	if(GetDataStoreStringValue("<OnlinePlayerData:ProfileData.CustomCharData>", CharacterDataStr, none, GetPlayerOwner()))
	{
		if(Len(CharacterDataStr) > 0)
		{
			bHaveLoadedCharData = true;
		}
	}

	return bHaveLoadedCharData;
}


/**
 * Called just before the scene is added to the ActiveScenes array, or when this scene has become the active scene as a result
 * of closing another scene.
 *
 * @param	bInitialActivation		TRUE if this is the first time this scene is being activated; FALSE if this scene has become active
 *									as a result of closing another scene or manually moving this scene in the stack.
 */
event SceneActivated( bool bInitialActivation )
{
	Super.SceneActivated(bInitialActivation);

	// See if the current player hasn't setup a character yet.
	if(bDisplayedNewPlayerMessageBox==false)
	{
		bDisplayedNewPlayerMessageBox=true;
		DisplayNewPlayerMessageBox();
	}
}

/** Displays a messagebox if the player doesn't have a custom character set. */
function DisplayNewPlayerMessageBox()
{
	local UTUIScene_MessageBox MessageBoxReference;
	local array<string> MessageBoxOptions;
	local array<PotentialOptionKeys> PotentialOptionKeyMappings;

	if(HasSavedCharacterData()==false)
	{
		// Pop up a message box asking the user if they want to edit their character or randomly generate one.
		MessageBoxReference = GetMessageBoxScene();

		if(MessageBoxReference != none)
		{
			MessageBoxOptions.AddItem("<Strings:UTGameUI.CharacterCustomization.RandomlyGenerate>");
			MessageBoxOptions.AddItem("<Strings:UTGameUI.CharacterCustomization.CreateCharacter>");
			MessageBoxOptions.AddItem("<Strings:UTGameUI.Generic.Cancel>");

			PotentialOptionKeyMappings.length = 3;
			PotentialOptionKeyMappings[0].Keys.AddItem('XboxTypeS_X');
			PotentialOptionKeyMappings[1].Keys.AddItem('XboxTypeS_A');
			PotentialOptionKeyMappings[2].Keys.AddItem('XboxTypeS_B');
			PotentialOptionKeyMappings[2].Keys.AddItem('Escape');

			MessageBoxReference.SetPotentialOptionKeyMappings(PotentialOptionKeyMappings);
			MessageBoxReference.SetPotentialOptions(MessageBoxOptions);
			MessageBoxReference.Display("<Strings:UTGameUI.MessageBox.FirstTimeCharacter_Message>", "<Strings:UTGameUI.MessageBox.FirstTimeCharacter_Title>", OnFirstTimeCharacter_Confirm);
		}
	}
}

/**
 * Callback when the user dismisses the firsttime character message box.
 */
 
function OnFirstTimeCharacter_Confirm(int SelectedOption, int PlayerIndex)
{
	local CustomCharData CustomData;
	local string DataString;

	switch(SelectedOption)
	{
	case 0:	// Randomly generate a character
		CustomData = class'UTCustomChar_Data'.static.MakeRandomCharData();
		DataString = class'UTCustomChar_Data'.static.CharDataToString(CustomData);

		if(SetDataStoreStringValue("<OnlinePlayerData:ProfileData.CustomCharData>", DataString, none, GetPlayerOwner()))
		{
			SavePlayerProfile();
		}
		break;
	case 1:	// Open the character selection scene
		OpenSceneByName(CharacterSelectionScene);
		bDisplayedNewPlayerMessageBox=false;	// Need to recheck for a customized character after the user has closed the player customization screen.
		break;
	case 2:	// Close this scene
		CloseScene(self);
		break;
	}
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
	case MULTIPLAYER_OPTION_QUICKMATCH:
		if(CheckLoginAndError() && CheckLinkConnectionAndError())
		{
			OpenSceneByName(QuickMatchScene);
		}
		break;
	case MULTIPLAYER_OPTION_HOSTGAME:
		if(CheckLoginAndError() && CheckLinkConnectionAndError())
		{
			OpenSceneByName(HostScene);
		}
		break;
	case MULTIPLAYER_OPTION_JOINGAME:
		if(CheckLoginAndError() && CheckLinkConnectionAndError())
		{
			OpenSceneByName(Joinscene);
		}
		break;
	case MULTIPLAYER_OPTION_CREATEPARTY:
		if(CheckOnlinePrivilegeAndError())
		{
			OpenSceneByName(PartyScene);
		}
		break;
	}
}

/** Callback for when the user wants to back out of the scene. */
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
	QuickMatchScene="UI_Scenes_ChrisBLayout.Scenes.QuickMatch"
	JoinScene="UI_Scenes_FrontEnd.Scenes.JoinGame"
	HostScene="UI_Scenes_ChrisBLayout.Scenes.HostGame"
	CharacterSelectionScene="UI_Scenes_FrontEnd.Scenes.CharacterFaction"
}