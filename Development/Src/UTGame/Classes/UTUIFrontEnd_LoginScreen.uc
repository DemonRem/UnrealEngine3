/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Profile login screen for UT3.
 */
class UTUIFrontEnd_LoginScreen extends UTUIFrontEnd_CustomScreen
	Config(Game);

`include(UTOnlineConstants.uci)

var string	CreateProfileScene;
var transient UIEditBox	UserNameEditBox;
var transient UIEditBox	PasswordEditBox;
var transient UTUIScene_MessageBox MessageBoxReference;
var transient EOnlineServerConnectionStatus LoginErrorCode;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize( )
{
	Super.PostInitialize();

	UserNameEditBox = UIEditBox(FindChild('txtUserName',true));
	UserNameEditBox.NotifyActiveStateChanged=OnNotifyActiveStateChanged;
	UserNameEditBox.OnSubmitText=OnSubmitText;

	PasswordEditBox = UIEditBox(FindChild('txtPassword',true));
	PasswordEditBox.NotifyActiveStateChanged=OnNotifyActiveStateChanged;
	PasswordEditBox.OnSubmitText=OnSubmitText;

	UserNameEditBox.MaxCharacters = GS_USERNAME_MAXLENGTH;
	UserNameEditBox.SetDataStoreBinding("");

	PasswordEditBox.MaxCharacters = GS_PASSWORD_MAXLENGTH;
	PasswordEditBox.SetDataStoreBinding("");

	//@todo: Remove this, only for testing purposes.
	UserNameEditBox.SetDataStoreBinding("ut3test");
	PasswordEditBox.SetDataStoreBinding("ut3test");
	UserNameEditBox.SetValue("ut3test");
	PasswordEditBox.SetValue("ut3test");
}

/** Sets up the scene's button bar. */
function SetupButtonBar()
{
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Cancel>", OnButtonBar_Cancel);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Login>", OnButtonBar_Login);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.CreateProfileLoginScreen>", OnButtonBar_CreateProfile);
}

/** Callback for when the user wants to back out of this screen. */
function OnCancel()
{
	CloseScene(self);
}

/** Tries to login the user */
function OnLogin()
{
	local string UserName;
	local string Password;
	local OnlinePlayerInterface PlayerInt;

	UserName = UserNameEditBox.GetValue();
	Password = PasswordEditBox.GetValue();

	// Verify contents of user name box
	if(Len(UserName) > 0)
	{
		if(Len(Password) > 0)
		{
			// Try logging in
			`Log("UTUIFrontEnd_LoginScreen::OnLogin() - Attempting to login with name '"$UserName$"'");

			PlayerInt = GetPlayerInterface();

			if(PlayerInt!=none)
			{
				// Set flag
				SetDataStoreStringValue("<Registry:bChangingLoginStatus>", "1");

				// Display a modal messagebox
				MessageBoxReference = GetMessageBoxScene();
				MessageBoxReference.DisplayModalBox("<Strings:UTGameUI.Generic.LoggingIn>","");

				PlayerInt.AddLoginChangeDelegate(OnLoginChange);
				PlayerInt.AddLoginFailedDelegate(GetPlayerOwner().ControllerId,OnLoginFailed);
				PlayerInt.AddLoginCancelledDelegate(OnLoginCancelled);
				if(PlayerInt.Login(GetPlayerOwner().ControllerId, UserName, Password)==false)
				{
					`Log("UTUIFrontEnd_LoginScreen::OnLogin() - Login call failed.");
					OnLoginCancelled();
				}
			}
			else
			{
				`Log("UTUIFrontEnd_LoginScreen::OnLogin() - Failed to find player interface.");
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

/** Clears login delegates. */
function ClearLoginDelegates()
{
	local OnlinePlayerInterface PlayerInt;
	PlayerInt = GetPlayerInterface();

	PlayerInt.ClearLoginChangeDelegate(OnLoginChange);
	PlayerInt.ClearLoginFailedDelegate(GetPlayerOwner().ControllerId,OnLoginFailed);
	PlayerInt.ClearLoginCancelledDelegate(OnLoginCancelled);

	// Clear flag
	SetDataStoreStringValue("<Registry:bChangingLoginStatus>", "0");
}

/**
 * Delegate used in login notifications
 */
function OnLoginChange()
{
	// Close the modal messagebox
	MessageBoxReference.OnClosed = OnLoginChange_Closed;
	MessageBoxReference.Close();
}

/** Callback for when the login message closes for login change. */
function OnLoginChange_Closed()
{
	MessageBoxReference = none;

	ClearLoginDelegates();

	// Logged in successfully so close the login scene.
	CloseScene(self);
}


/**
 * Delegate used to notify when a login request was cancelled by the user
 */
function OnLoginCancelled()
{
	// Close the modal messagebox
	MessageBoxReference.Close();
	MessageBoxReference = none;

	ClearLoginDelegates();
}

/**
* Delegate used in notifying the UI/game that the manual login failed
*
* @param LocalUserNum the controller number of the associated user
* @param ErrorCode the async error code that occurred
*/
function OnLoginFailed(byte LocalUserNum,EOnlineServerConnectionStatus ErrorCode)
{
	// Close the modal messagebox
	LoginErrorCode=ErrorCode;
	MessageBoxReference.OnClosed=OnLoginFailed_Closed;
	MessageBoxReference.Close();
}

/** Callback for when the login message closes. */
function OnLoginFailed_Closed()
{
	MessageBoxReference=None;

	// Display an error message
	switch(LoginErrorCode)
	{
	case OSCS_ConnectionDropped:
		DisplayMessageBox("<Strings:UTGameUI.Errors.ConnectionLost_Message>","<Strings:UTGameUI.Errors.ConnectionLost_Title>");
		break;
	case OSCS_NoNetworkConnection:
		DisplayMessageBox("<Strings:UTGameUI.Errors.LinkDisconnected_Message>","<Strings:UTGameUI.Errors.LinkDisconnected_Title>");
		break;
	case OSCS_ServiceUnavailable:
		DisplayMessageBox("<Strings:UTGameUI.Errors.ServiceUnavailable_Message>","<Strings:UTGameUI.Errors.ServiceUnavailable_Title>");
		break;
	case OSCS_UpdateRequired:
		DisplayMessageBox("<Strings:UTGameUI.Errors.UpdateRequired_Message>","<Strings:UTGameUI.Errors.UpdateRequired_Title>");
		break;
	case OSCS_ServersTooBusy:
		DisplayMessageBox("<Strings:UTGameUI.Errors.ServersTooBusy_Message>","<Strings:UTGameUI.Errors.ServersTooBusy_Title>");
		break;
	case OSCS_DuplicateLoginDetected:
		DisplayMessageBox("<Strings:UTGameUI.Errors.DuplicateLogin_Message>","<Strings:UTGameUI.Errors.DuplicateLogin_Title>");
		break;
	case OSCS_InvalidUser:
		DisplayMessageBox("<Strings:UTGameUI.Errors.InvalidUserLogin_Message>","<Strings:UTGameUI.Errors.InvalidUserLogin_Title>");
		break;
	default:
		break;
	}

	ClearLoginDelegates();
}

/** Displays the account creation scene */
function OnCreateProfile()
{
	OpenSceneByName(CreateProfileScene);
}

/** Buttonbar Callbacks. */
function bool OnButtonBar_Cancel(UIScreenObject InButton, int InPlayerIndex)
{
	OnCancel();

	return true;
}

function bool OnButtonBar_Login(UIScreenObject InButton, int InPlayerIndex)
{
	OnLogin();

	return true;
}

function bool OnButtonBar_CreateProfile(UIScreenObject InButton, int InPlayerIndex)
{
	OnCreateProfile();

	return true;
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
	OnLogin();

	return false;
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
		else if(EventParms.InputKeyName=='XboxTypeS_Y')
		{
			OnCreateProfile();
			bResult=true;
		}
	}

	return bResult;
}


defaultproperties
{
	CreateProfileScene="UI_Scenes_FrontEnd.Scenes.CreateProfile"
	DescriptionMap.Add((WidgetTag="txtUserName",DataStoreMarkup="<Strings:UTgameUI.Profiles.LoginUserName_Description>"));
	DescriptionMap.Add((WidgetTag="txtPassword",DataStoreMarkup="<Strings:UTgameUI.Profiles.LoginPassword_Description>"));
}