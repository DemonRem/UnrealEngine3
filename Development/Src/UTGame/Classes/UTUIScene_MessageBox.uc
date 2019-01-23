/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Generic Message Box Scene for UT3
 */
class UTUIScene_MessageBox extends UTUIScene
	native(UI);

const MESSAGEBOX_MAX_POSSIBLE_OPTIONS = 4;

cpptext
{
	virtual void Tick( FLOAT DeltaTime );
}


/** Message box message. */
var private transient UILabel					MessageLabel;

/** Message box title markup. */
var private transient UILabel					TitleLabel;

/** Reference to the owning widget that has all of the content of the message box scene. */
var private transient UIImage					BackgroundImage;

/** References to the scene's buttonbar. */
var private transient UTUIButtonBar				ButtonBar;

/** Array of potential options markup. */
var private transient array<string>				PotentialOptions;

/** Arrays of key's that trigger each potential option. */
struct native PotentialOptionKeys
{
	var array<name>		Keys;
};

var private transient array<PotentialOptionKeys>		PotentialOptionKeyMappings;

/** Whether or not the message box is fully visible. */
var private transient bool						bFullyVisible;

/** Stores which option the user selected. */
var private transient int						PreviouslySelectedOption;

/** Index of the player that selected the option. */
var private transient int						SelectingPlayer;

/** Index of the option that is selected by default, valid for PC only. */
var private transient int						DefaultOptionIdx;

/** The time the message box was fully displayed. */
var private transient float						DisplayTime;

/** The minimum amount of time to display modal message boxes for. */
var private transient float						MinimumDisplayTime;

/** How long to take to display/hide the message box. */
var transient float						FadeDuration;

/** Direction we are currently fading, positive is fade in, negative is fade out, 0 is not fading. */
var private transient int						FadeDirection;

/** Time fading started. */
var private transient float						FadeStartTime;

/** Flag that lets the dialog know it should hide itself. */
var private transient bool						bHideOnNextTick;

/**
 * The user has made a selection of the choices available to them.
 */
delegate OnSelection(int SelectedOption, int PlayerIndex);

/** Delegate called after the message box has been competely closed. */
delegate OnClosed();

/** Delegate to trap any input to the message box. */
delegate bool OnMBInputKey( const out InputEventParameters EventParms );

/** Sets delegates for the scene. */
event PostInitialize()
{
	Super.PostInitialize();

	// Setup a key handling delegate.
	OnRawInputKey=HandleInputKey;	

	// Store references to key objects.
	MessageLabel = UILabel(FindChild('lblMessage',true));
	TitleLabel = UILabel(FindChild('lblTitle',true));
	ButtonBar = UTUIButtonBar(Findchild('pnlButtonBar', true));
	BackgroundImage = UIImage(FindChild('imgBackground', true));
}

/**
 * Sets the potential options for a message box.
 *
 * @param InPotentialOptions				Potential options the user has to choose from.
 * @param InPotentialOptionsKeyMappings		Button keys that map to the options specified, usually this can be left to defaults.
 */
function SetPotentialOptions(array<string> InPotentialOptions, optional array<PotentialOptionKeys> InPotentialOptionKeyMappings)
{
	PotentialOptions = InPotentialOptions;
}

/**
 * Sets the potential option key mappings for a message box, usually this can be left to defaults.
 *
 * @param InPotentialOptions				Potential options the user has to choose from.
 * @param InPotentialOptionsKeyMappings		Button keys that map to the options specified, usually this can be left to defaults.
 */
function SetPotentialOptionKeyMappings(array<PotentialOptionKeys> InPotentialOptionKeyMappings)
{
	PotentialOptionKeyMappings=InPotentialOptionKeyMappings;
}

/** Closes the message box, used for modal message boxes. */
function Close()
{
	bHideOnNextTick = true;
}

/** Displays a message box that has a cancel button only. */
function DisplayCancelBox(string Message, optional string Title="", optional delegate<OnSelection> InSelectionDelegate)
{
	PotentialOptions.length = 1;
	PotentialOptions[0]="<strings:UTGameUI.Generic.Cancel>";
	PotentialOptionKeyMappings.length = 1;
	PotentialOptionKeyMappings[0].Keys.length=2;
	PotentialOptionKeyMappings[0].Keys[0]='XboxTypeS_B';
	PotentialOptionKeyMappings[0].Keys[1]='Escape';
	Display(Message,Title);
}

/** Displays a message box that has no buttons and must be closed by the scene that opened it. */
function DisplayModalBox(string Message, optional string Title="", optional float InMinDisplayTime=2.0f)
{
	// This is a ship requirement, we need to display modal dialogs for a minimum amount of time.
	MinimumDisplayTime = InMinDisplayTime;
	PotentialOptions.length = 0;
	PotentialOptionKeyMappings.length = 0;
	ButtonBar.SetVisibility(false);
	Display(Message,Title);
}


/**
 * Displays the message box.
 *
 * @param Message				Message for the message box.  Should be datastore markup.
 * @param Title					Title for the message box.  Should be datastore markup.
 * @param InSelectionDelegate	Delegate to call when the user dismisses the message box.
 */
function Display(string Message, optional string Title="", optional delegate<OnSelection> InSelectionDelegate, optional int InDefaultOptionIdx=INDEX_NONE)
{
	local int OptionIdx;

	MessageLabel.SetDatastoreBinding(Message);
	TitleLabel.SetDatastoreBinding(Title);
	OnSelection=InSelectionDelegate;
	DefaultOptionIdx = InDefaultOptionIdx;

	// Setup buttons.  We only show enough buttons to cover each of the options the user specified.
	ButtonBar.Clear();
	for(OptionIdx=0;  OptionIdx<MESSAGEBOX_MAX_POSSIBLE_OPTIONS && OptionIdx<PotentialOptions.length; OptionIdx++)
	{
		// Reverse the index when displaying buttons so cancel is always shown to the far left.
		ButtonBar.AppendButton(PotentialOptions[PotentialOptions.length-OptionIdx-1], OnOptionButton_Clicked);
	}

	// Start showing the scene.
	BeginShow();
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
	local int OptionIdx;
	local int ButtonIdx;
	local array<name> KeyMappings;

	bResult=false;

	if(bFullyVisible)
	{	
		// Give the mb delegate first chance at input
		bResult = OnMBInputKey(EventParms);

		if(bResult==false)
		{
			if(EventParms.EventType==IE_Released)
			{
				// See if the key pressed is mapped to any of the potential options.
				for(OptionIdx=0; OptionIdx<PotentialOptions.length && OptionIdx<MESSAGEBOX_MAX_POSSIBLE_OPTIONS && OptionIdx<PotentialOptionKeyMappings.length && bResult==false; OptionIdx++)
				{
					KeyMappings=PotentialOptionKeyMappings[OptionIdx].Keys;

					for(ButtonIdx=0; ButtonIdx<KeyMappings.length; ButtonIdx++)
					{
						if(EventParms.InputKeyName==KeyMappings[ButtonIdx])
						{
							OptionSelected(OptionIdx, EventParms.PlayerIndex);
							bResult=true;
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		bResult = true;
	}

	return bResult;
}

/** Starts showing the message box. */
native function BeginShow();

/** Called when the dialog has finished showing itself. */
event OnShowComplete()
{
	`Log("UTUIMessageBox::OnShowComplete() - Finished showing messagebox");

	//@todo: Enable input maybe?
	bFullyVisible = true;
	FadeDirection = 0;

	// Set focus to the default option
	if(PotentialOptions.length > 0)
	{
		if(DefaultOptionIdx==INDEX_NONE)
		{
			ButtonBar.Buttons[PotentialOptions.length-1].SetFocus(none);
		}
		else
		{
			ButtonBar.Buttons[PotentialOptions.length-1-DefaultOptionIdx].SetFocus(none);
		}
	}
}

/** Starts hiding the message box. */
native function BeginHide();

/** Called when the dialog is finished hiding itself. */
event OnHideComplete()
{
	`Log("UTUIMessageBox::OnHideComplete() - Finished hiding messagebox");

	FadeDirection = 0;

	// Close ourselves.
	CloseScene(self);

	// Fire the OnSelection delegate
	OnSelection(PreviouslySelectedOption, SelectingPlayer);

	// Fire the OnClosed delegate
	OnClosed();
}

/**
 * Callback for the OnClicked event.
 *
 * @param EventObject	Object that issued the event.
 * @param PlayerIndex	Player that performed the action that issued the event.
 *
 * @return	return TRUE to prevent the kismet OnClick event from firing.
 */
function bool OnOptionButton_Clicked(UIScreenObject EventObject, int PlayerIndex)
{
	local int OptionIdx;

	if(bFullyVisible)
	{	
		for(OptionIdx=0; OptionIdx<MESSAGEBOX_MAX_POSSIBLE_OPTIONS; OptionIdx++)
		{
			if(EventObject==ButtonBar.Buttons[OptionIdx])
			{
				// Need to reverse the index
				OptionSelected(PotentialOptions.length-OptionIdx-1, PlayerIndex);
				break;
			}
		}
	}

	return false;
}

/**
 * Called when a user has chosen one of the possible options available to them.
 * Begins hiding the dialog and calls the On
 *
 * @param OptionIdx		Index of the selection option.
 * @param PlayerIndex	Index of the player that selected the option.
 */
function OptionSelected(int OptionIdx, int PlayerIndex)
{
	PreviouslySelectedOption=OptionIdx;
	SelectingPlayer=PlayerIndex;
	bHideOnNextTick = true;
}

defaultproperties
{
	PotentialOptions=("<strings:UTGameUI.Generic.OK>")
	PotentialOptionKeyMappings=((Keys=("XboxTypeS_A")), (Keys=("XboxTypeS_B","Escape")), (Keys=("XboxTypeS_X")), (Keys=("XboxTypeS_Y"))  )
	DefaultOptionIdx=INDEX_NONE
	FadeDuration=0.25f

	Begin Object Class=UIComp_Event Name=WidgetEventComponent
		DisabledEventAliases.Add(CloseScene)
	End Object
	EventProvider=WidgetEventComponent
}
