/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Profile creation screen for UT3.
 */
class UTUIFrontEnd_CreateProfile extends UTUIFrontEnd_CustomScreen;

`include(UTOnlineConstants.uci)

var transient UIEditBox	UserNameEditBox;
var transient UIEditBox	PasswordEditBox;
var transient UIEditBox	PasswordConfirmEditBox;
var transient UIEditBox	EMailEditBox;
var transient UTUIScene_MessageBox MessageBoxReference;
var transient OnlineAccountInterface AccountInterface;
var transient bool bSuccessfullyCreatedProfile;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize( )
{
	Super.PostInitialize();

	// Get widget references and setup delegates
	UserNameEditBox = UIEditBox(FindChild('txtUserName',true));
	UserNameEditBox.NotifyActiveStateChanged = OnNotifyActiveStateChanged;
	UserNameEditBox.OnSubmitText=OnSubmitText;
	UserNameEditBox.SetDataStoreBinding("");
	UserNameEditBox.SetValue("");
	UserNameEditBox.MaxCharacters = GS_USERNAME_MAXLENGTH;

	PasswordEditBox = UIEditBox(FindChild('txtPassword',true));
	PasswordEditBox.NotifyActiveStateChanged = OnNotifyActiveStateChanged;
	PasswordEditBox.OnSubmitText=OnSubmitText;
	PasswordEditBox.SetDataStoreBinding("");
	PasswordEditBox.SetValue("");
	PasswordEditBox.MaxCharacters = GS_PASSWORD_MAXLENGTH;

	PasswordConfirmEditBox = UIEditBox(FindChild('txtPasswordConfirm',true));
	PasswordConfirmEditBox.NotifyActiveStateChanged = OnNotifyActiveStateChanged;
	PasswordConfirmEditBox.OnSubmitText=OnSubmitText;
	PasswordConfirmEditBox.SetDataStoreBinding("");
	PasswordConfirmEditBox.SetValue("");
	PasswordConfirmEditBox.MaxCharacters = GS_PASSWORD_MAXLENGTH;

	EMailEditBox = UIEditBox(FindChild('txtEMail',true));
	EMailEditBox.NotifyActiveStateChanged = OnNotifyActiveStateChanged;
	EMailEditBox.OnSubmitText=OnSubmitText;
	EMailEditBox.SetDataStoreBinding("");
	EMailEditBox.SetValue("");	
	EMailEditBox.MaxCharacters = GS_EMAIL_MAXLENGTH;

	// Store reference to the account interface.
	AccountInterface=GetAccountInterface();
}

/** Sets up the scene's button bar. */
function SetupButtonBar()
{
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Cancel>", OnButtonBar_Cancel);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.CreateProfile>", OnButtonBar_CreateProfile);
}

/** Callback for when the user wants to back out of this screen. */
function OnCancel()
{
	CloseScene(self);
}

/** Tries to create the user's account. */
function OnCreateProfile()
{
	local string UserName;
	local string Password;
	local string PasswordConfirm;
	local string EMailAddress;
	local string CDKey;

	// @todo: Retrieve CD Key
	CDKey = "";

	UserName = UserNameEditBox.GetValue();
	Password = PasswordEditBox.GetValue();
	PasswordConfirm = PasswordConfirmEditBox.GetValue();
	EMailAddress = EMailEditBox.GetValue();

	// Verify contents of user name box
	if(Len(UserName) > 0)
	{
		if(Len(Password) > 0)
		{
			if(Password==PasswordConfirm)
			{
				if(Len(EMailAddress) > 0)
				{
					`Log("UTUIFrontEnd_CreateProfile::OnCreateProfile() - Creating profile for player with name '"$UserName$"'");

					if(AccountInterface != None)
					{
						MessageBoxReference = GetMessageBoxScene();
						MessageBoxReference.DisplayModalBox("<Strings:UTGameUI.Generic.CreatingProfile>","");

						AccountInterface.AddCreateOnlineAccountCompletedDelegate(OnCreateOnlineAccountCompleted);
						if(AccountInterface.CreateOnlineAccount(UserName, Password, EMailAddress, CDKey)==false)
						{
							OnCreateOnlineAccountCompleted(OACS_UnknownError);
						}
					}
					else
					{
						`Log("UTUIFrontEnd_CreateProfile::OnCreateProfile() - Unable to find account interface!");
					}
				}
				else
				{
					DisplayMessageBox("<Strings:UTGameUI.Errors.InvalidEMail_Message>","<Strings:UTGameUI.Errors.InvalidEMail_Title>");
				}
			}
			else
			{
				DisplayMessageBox("<Strings:UTGameUI.Errors.InvalidPasswordConfirm_Message>","<Strings:UTGameUI.Errors.InvalidPasswordConfirm_Title>");
			}
		}
		else
		{
			DisplayMessageBox("<Strings:UTGameUI.Errors.InvalidPassword_Message>","<Strings:UTGameUI.Errors.InvalidPassword_Title>");
		}
	}
	else
	{
		DisplayMessageBox("<Strings:UTGameUI.Errors.InvalidUserName_Message>","<Strings:UTGameUI.Errors.InvalidUserName_Title>");
	}
}


/**
 * Delegate used in notifying the UI/game that the account creation completed
 *
 * @param ErrorStatus whether the account created successfully or not
 */
function OnCreateOnlineAccountCompleted(EOnlineAccountCreateStatus ErrorStatus)
{
	// Hide modal box and clear delegates
	MessageBoxReference.OnClosed = OnCreateOnlineAccount_Closed;
	MessageBoxReference.Close();
	MessageBoxReference = None;

	bSuccessfullyCreatedProfile = ErrorStatus == OACS_CreateSuccessful;

}

/** Callback for when the creating account message finishes hiding. */
function OnCreateOnlineAccount_Closed()
{
	AccountInterface.ClearCreateOnlineAccountCompletedDelegate(OnCreateOnlineAccountCompleted);

	if(bSuccessfullyCreatedProfile)
	{
		// Display success messagebox
		DisplayMessageBox("<Strings:UTGameUI.MessageBox.ProfileCreated_Message>","<Strings:UTGameUI.MessageBox.ProfileCreated_Title>");
		CloseScene(self);
	}
	else
	{
		// Display failure messagebox based on reason
		DisplayMessageBox("<Strings:UTGameUI.Errors.ProfileCreationFailed_Message>","<Strings:UTGameUI.Errors.ProfileCreationFailed_Title>");
	}
}

/** Buttonbar Callbacks. */
function bool OnButtonBar_Cancel(UIScreenObject InButton, int InPlayerIndex)
{
	OnCancel();

	return true;
}

function bool OnButtonBar_CreateProfile(UIScreenObject InButton, int InPlayerIndex)
{
	OnCreateProfile();

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
			OnCancel();
			bResult=true;
		}
	}

	return bResult;
}

/**
 * Called when the user presses enter (or any other action bound to UIKey_SubmitText) while this editbox has focus.
 *
 * @param	Sender	the editbox that is submitting text
 * @param	PlayerIndex		the index of the player that generated the call to SetValue; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 *
 * @return	if TRUE, the editbox will clear the existing value of its textbox.
 */
function bool OnSubmitText( UIEditBox Sender, int PlayerIndex )
{
	OnCreateProfile();

	return false;
}


defaultproperties
{
	DescriptionMap.Add((WidgetTag="txtUserName",DataStoreMarkup="<Strings:UTgameUI.Profiles.UserName_Description>"));
	DescriptionMap.Add((WidgetTag="txtPassword",DataStoreMarkup="<Strings:UTgameUI.Profiles.Password_Description>"));
	DescriptionMap.Add((WidgetTag="txtPasswordConfirm",DataStoreMarkup="<Strings:UTgameUI.Profiles.ConfirmPassword_Description>"));
	DescriptionMap.Add((WidgetTag="txtEMail",DataStoreMarkup="<Strings:UTgameUI.Profiles.EMailAddress_Description>"));
}