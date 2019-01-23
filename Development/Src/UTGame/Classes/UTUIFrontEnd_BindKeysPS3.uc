/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Key binding screen for the PS3.
 */
class UTUIFrontEnd_BindKeysPS3 extends UTUIFrontEnd
	dependson(UTProfileSettings);

enum EPossiblePS3Buttons
{
	UTPS3Button_X,
	UTPS3Button_Circle,
	UTPS3Button_Square,
	UTPS3Button_Triangle,
	UTPS3Button_Select,
	UTPS3Button_R1,
	UTPS3Button_R2,
	UTPS3Button_R3,
	UTPS3Button_L1,
	UTPS3Button_L2,
	UTPS3Button_L3,
	UTPS3Button_DPadUp,
	UTPS3Button_DPadDown,
	UTPS3Button_DPadLeft,
	UTPS3Button_DPadRight
};

/** PS3 presets */
struct PS3InputPreset
{
	var array<EDigitalButtonActions> Buttons;
};

var array<EDigitalButtonActions>		Preset1;
var array<EDigitalButtonActions>		Preset2;
var array<EDigitalButtonActions>		Preset3;

var array<PS3InputPreset>	PossiblePresets;
var array<int>				ButtonProfileMappings;

/** Whether or not the user is customizing the entire screen. */
var transient bool bCustomMode;

/** Reference to the string list datastore. */
var transient UTUIDataStore_StringList StringListDataStore;

/** Reference to the option list for this widget */
var transient UTUIOptionList OptionList;
var transient UTUIOptionList PresetList;
var transient array<UILabel> ButtonLabels;
var transient UILabel		 LeftStickLabel;
var transient UILabel		 RightStickLabel;
var transient array<name> LabelNames;

/** Localized labels for the left and right sticks depending on the current analog stick action. */
var transient array<string> LeftStickStrings;
var transient array<string> RightStickStrings;

/** Post initialize callback. */
event PostInitialize()
{
	local int LabelIdx;
	local UILabel LabelObj;

	Super.PostInitialize();

	// Get widget references
	OptionList = UTUIOptionList(FindChild('lstOptions', true));
	OptionList.OnOptionChanged = OnOptionList_OptionChanged;
	OptionList.OnAcceptOptions = OnAcceptOptions;

	PresetList = UTUIOptionList(FindChild('lstPresets', true));
	PresetList.OnOptionChanged = OnPresetList_OptionChanged;
	PresetList.OnAcceptOptions = OnAcceptOptions;

	for(LabelIdx=0; LabelIdx<LabelNames.length; LabelIdx++)
	{
		LabelObj = UILabel(FindChild(LabelNames[LabelIdx], true));

		if(LabelObj != None)
		{
			ButtonLabels.length = ButtonLabels.length + 1;
			ButtonLabels[ButtonLabels.length-1]=LabelObj;
		}
	}

	LeftStickLabel = UILabel(FindChild('lblLeftStick', true));
	RightStickLabel = UILabel(FindChild('lblRightStick', true));

	StringListDataStore = UTUIDataStore_StringList(GetCurrentUIController().DataStoreManager.FindDataStore('UTStringList'));

	PossiblePresets.length=3;
	PossiblePresets[0].Buttons = Preset1;
	PossiblePresets[1].Buttons = Preset2;
	PossiblePresets[2].Buttons = Preset3;

	bCustomMode = false;
	ModeChanged();
	UpdateButtonLabels();
}

/** Callback to setup the buttonbar for this scene. */
function SetupButtonBar()
{
	ButtonBar.Clear();
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Back>", OnButtonBar_Back);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Accept>", OnButtonBar_Accept);

	if(bCustomMode)
	{
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Presets>", OnButtonBar_ToggleCustomMode);
	}
	else
	{
		ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Customize>", OnButtonBar_ToggleCustomMode);
	}
}

/** Updates the labels for the buttons on the 360 controller diagram. */
function UpdateButtonLabels()
{
	local int CurrentPreset;
	local UTProfileSettings Profile;
	local int LabelIdx;
	local string StickLabel;
	local string ButtonLabel;


	Profile = UTProfileSettings(GetUTPlayerOwner().OnlinePlayerData.ProfileProvider.Profile);

	for(LabelIdx=0; LabelIdx<ButtonLabels.length; LabelIdx++)
	{
		ButtonLabels[LabelIdx].RefreshSubscriberValue();
	}

	// Analog Sticks
	if(Profile.GetProfileSettingValueId(class'UTProfileSettings'.const.UTPID_GamepadBinding_AnalogStickPreset, CurrentPreset))
	{
		// Left Stick
		if(Profile.GetProfileSettingValue(class'UTProfileSettings'.const.UTPID_GamepadBinding_LeftThumbstickPressed, ButtonLabel))
		{
			StickLabel=LeftStickStrings[CurrentPreset]$"<Strings:UTGameUI.AnalogStick.Press> "$ButtonLabel;
			LeftStickLabel.SetDataStoreBinding(StickLabel);
		}

		// Right Stick
		if(Profile.GetProfileSettingValue(class'UTProfileSettings'.const.UTPID_GamepadBinding_RightThumbstickPressed, ButtonLabel))
		{
			StickLabel=RightStickStrings[CurrentPreset]$"<Strings:UTGameUI.AnalogStick.Press> "$ButtonLabel;
			RightStickLabel.SetDataStoreBinding(StickLabel);
		}
	}

}

/** Delegate for when the user accepts either option or preset list. */
function OnAcceptOptions(UIScreenObject InObject, int PlayerIndex)
{
	OnAccept();
}

/** Delegate for when the user changes one of the options in this option list. */
function OnOptionList_OptionChanged(UIScreenObject InObject, name OptionName, int PlayerIndex)
{
	local array<UIDataStore> BoundDataStores;
	UIDataStorePublisher(InObject).SaveSubscriberValue(BoundDataStores);
	UpdateButtonLabels();
}

/** Delegate for when the user changes one of the options in the preset list. */
function OnPresetList_OptionChanged(UIScreenObject InObject, name OptionName, int PlayerIndex)
{
	SaveSceneDataValues();
	SetPresetValues();
}

/** Sets the buttons to the current preset setting */
function SetPresetValues()
{
	local int CurrentPreset;
	local UTProfileSettings Profile;
	local int ButtonIdx;

	Profile = UTProfileSettings(GetUTPlayerOwner().OnlinePlayerData.ProfileProvider.Profile);

	CurrentPreset = StringListDataStore.GetCurrentValueIndex('PS3ButtonPresets');

	for(ButtonIdx=0; ButtonIdx<UTPS3Button_MAX; ButtonIdx++)
	{
		Profile.SetProfileSettingValueId(ButtonProfileMappings[ButtonIdx], PossiblePresets[CurrentPreset].Buttons[ButtonIdx]);
	}


	UpdateButtonLabels();
}

/** Callback for when the user wants to exit this screen. */
function OnBack()
{
	CloseScene(self);
}

/** Callback for when the user wants to save their options. */
function OnAccept()
{
	// Force widgets to save their new values back to the profile.
	SaveSceneDataValues(FALSE);
	CloseScene(self);
}

/** Handles visiblity/focus changes depending on which mode we are in. */
function ModeChanged()
{
	if(IsGame())
	{
		if(bCustomMode)
		{
			PresetList.SetVisibility(false);
			OptionList.SetVisibility(true);

			OptionList.SetFocus(none);
		}
		else
		{
			PresetList.SetVisibility(true);
			OptionList.SetVisibility(false);

			PresetList.SetFocus(none);

			SetPresetValues();
		}

		SetupButtonBar();
	}
}

/** Toggles between preset and custom modes. */
function OnToggleCustomMode()
{
	local UTUIScene_MessageBox MessageBoxReference;
	local array<string> MessageBoxOptions;

	if(bCustomMode)
	{
		MessageBoxReference = GetMessageBoxScene();

		if(MessageBoxReference != none)
		{
			MessageBoxOptions.AddItem("<Strings:UTGameUI.ButtonCallouts.Accept>");
			MessageBoxOptions.AddItem("<Strings:UTGameUI.ButtonCallouts.Cancel>");

			MessageBoxReference.SetPotentialOptions(MessageBoxOptions);
			MessageBoxReference.Display("<Strings:UTGameUI.MessageBox.ResetToPreset_Message>", "<Strings:UTGameUI.MessageBox.ResetToPreset_Title>", OnSwitchMode_Confirm);
		}
	}
	else
	{
		bCustomMode = !bCustomMode;
		ModeChanged();
	}
}

/** Confirmation for the switch mode dialog. */
function OnSwitchMode_Confirm(int SelectedItem, int PlayerIndex)
{
	if(SelectedItem==0)
	{
		bCustomMode = !bCustomMode;
		ModeChanged();
	}
}

/** Button bar callbacks. */
function bool OnButtonBar_Accept(UIScreenObject InButton, int PlayerIndex)
{
	OnAccept();

	return true;
}

function bool OnButtonBar_ToggleCustomMode(UIScreenObject InButton, int PlayerIndex)
{
	OnToggleCustomMode();

	return true;
}

function bool OnButtonBar_Back(UIScreenObject InButton, int PlayerIndex)
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

	bResult = false;


	if(EventParms.EventType==IE_Released)
	{
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
			OnToggleCustomMode();
			bResult=true;
		}
	}


	return bResult;
}

defaultproperties
{
	Preset1(UTPS3Button_X)=DBA_Jump;
	Preset1(UTPS3Button_Circle)=DBA_ToggleMelee;
	Preset1(UTPS3Button_Square)=DBA_Use;
	Preset1(UTPS3Button_Triangle)=DBA_ShowMap;
	Preset1(UTPS3Button_Select)=DBA_ShowScores;
	Preset1(UTPS3Button_R1)=DBA_SmartDodge;
	Preset1(UTPS3Button_L1)=DBA_WeaponPicker;
	Preset1(UTPS3Button_R2)=DBA_Fire;
	Preset1(UTPS3Button_L2)=DBA_AltFire;
	Preset1(UTPS3Button_R3)=DBA_BestWeapon;
	Preset1(UTPS3Button_L3)=DBA_Jump;
	Preset1(UTPS3Button_DPadUp)=DBA_ToggleMinimap;
	Preset1(UTPS3Button_DPadDown)=DBA_FeignDeath;
	Preset1(UTPS3Button_DPadLeft)=DBA_ToggleSpeaking;
	Preset1(UTPS3Button_DPadRight)=DBA_ToggleSpeaking;

	Preset2(UTPS3Button_X)=DBA_Jump;
	Preset2(UTPS3Button_Circle)=DBA_ToggleMelee;
	Preset2(UTPS3Button_Square)=DBA_Use;
	Preset2(UTPS3Button_Triangle)=DBA_ShowMap;
	Preset2(UTPS3Button_Select)=DBA_ShowScores;
	Preset2(UTPS3Button_R1)=DBA_Fire;
	Preset2(UTPS3Button_L1)=DBA_AltFire;
	Preset2(UTPS3Button_R2)=DBA_SmartDodge;
	Preset2(UTPS3Button_L2)=DBA_WeaponPicker;
	Preset2(UTPS3Button_R3)=DBA_BestWeapon;
	Preset2(UTPS3Button_L3)=DBA_Jump;
	Preset2(UTPS3Button_DPadUp)=DBA_ToggleMinimap;
	Preset2(UTPS3Button_DPadDown)=DBA_FeignDeath;
	Preset2(UTPS3Button_DPadLeft)=DBA_ToggleSpeaking;
	Preset2(UTPS3Button_DPadRight)=DBA_ToggleSpeaking;

	Preset3(UTPS3Button_X)=DBA_ToggleMelee;
	Preset3(UTPS3Button_Circle)=DBA_Jump;
	Preset3(UTPS3Button_Square)=DBA_ShowMap;
	Preset3(UTPS3Button_Triangle)=DBA_Use;
	Preset3(UTPS3Button_Select)=DBA_ShowScores;
	Preset3(UTPS3Button_R1)=DBA_SmartDodge;
	Preset3(UTPS3Button_L1)=DBA_WeaponPicker;
	Preset3(UTPS3Button_R2)=DBA_Fire;
	Preset3(UTPS3Button_L2)=DBA_AltFire;
	Preset3(UTPS3Button_R3)=DBA_BestWeapon;
	Preset3(UTPS3Button_L3)=DBA_Jump;
	Preset3(UTPS3Button_DPadUp)=DBA_ToggleMinimap;
	Preset3(UTPS3Button_DPadDown)=DBA_FeignDeath;
	Preset3(UTPS3Button_DPadLeft)=DBA_ToggleSpeaking;
	Preset3(UTPS3Button_DPadRight)=DBA_ToggleSpeaking;

	ButtonProfileMappings(UTPS3Button_X)=UTPID_GamepadBinding_ButtonA;
	ButtonProfileMappings(UTPS3Button_Circle)=UTPID_GamepadBinding_ButtonX;
	ButtonProfileMappings(UTPS3Button_Square)=UTPID_GamepadBinding_ButtonX;
	ButtonProfileMappings(UTPS3Button_Triangle)=UTPID_GamepadBinding_ButtonY;
	ButtonProfileMappings(UTPS3Button_Select)=UTPID_GamepadBinding_Back;
	ButtonProfileMappings(UTPS3Button_R1)=UTPID_GamepadBinding_RightBumper;
	ButtonProfileMappings(UTPS3Button_L1)=UTPID_GamepadBinding_LeftBumper;
	ButtonProfileMappings(UTPS3Button_R2)=UTPID_GamepadBinding_RightTrigger;
	ButtonProfileMappings(UTPS3Button_L2)=UTPID_GamepadBinding_LeftTrigger;
	ButtonProfileMappings(UTPS3Button_R3)=UTPID_GamepadBinding_RightThumbstickPressed;
	ButtonProfileMappings(UTPS3Button_L3)=UTPID_GamepadBinding_LeftThumbstickPressed;
	ButtonProfileMappings(UTPS3Button_DPadUp)=UTPID_GamepadBinding_DPadUp;
	ButtonProfileMappings(UTPS3Button_DPadDown)=UTPID_GamepadBinding_DPadDown;
	ButtonProfileMappings(UTPS3Button_DPadLeft)=UTPID_GamepadBinding_DPadLeft;
	ButtonProfileMappings(UTPS3Button_DPadRight)=UTPID_GamepadBinding_DPadRight;

	LabelNames=("lblR1","lblR2","lblL1","lblL2","lblX","lblCircle","lblTriangle","lblSquare","lblDPadUp","lblDPadDown","lblDPadLeft","lblDPadRight")

	LeftStickStrings(ESA_Normal)="<Strings:UTGameUI.AnalogStick.Normal_LeftStick>";
	LeftStickStrings(ESA_SouthPaw)="<Strings:UTGameUI.AnalogStick.SouthPaw_LeftStick>";
	LeftStickStrings(ESA_Legacy)="<Strings:UTGameUI.AnalogStick.Legacy_LeftStick>";
	LeftStickStrings(ESA_LegacySouthPaw)="<Strings:UTGameUI.AnalogStick.LegacySouthPaw_LeftStick>";

	RightStickStrings(ESA_Normal)="<Strings:UTGameUI.AnalogStick.Normal_RightStick>";
	RightStickStrings(ESA_SouthPaw)="<Strings:UTGameUI.AnalogStick.SouthPaw_RightStick>";
	RightStickStrings(ESA_Legacy)="<Strings:UTGameUI.AnalogStick.Legacy_RightStick>";
	RightStickStrings(ESA_LegacySouthPaw)="<Strings:UTGameUI.AnalogStick.LegacySouthPaw_RightStick>";
}