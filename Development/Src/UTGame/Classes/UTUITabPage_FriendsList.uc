/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Tab page for a user's friends list.
 */
class UTUITabPage_FriendsList extends UTTabPage
	placeable;

var transient UIList	FriendsList;

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Get widget references
	FriendsList = UIList(FindChild('lstFriends', true));
	FriendsList.OnSubmitSelection = OnFriendsList_SubmitSelection;

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.Community.Friends>");
}

/** Callback allowing the tabpage to setup the button bar for the current scene. */
function SetupButtonBar(UTUIButtonBar ButtonBar)
{
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.AddFriend>", OnButtonBar_AddFriend);
	ButtonBar.AppendButton("<Strings:UTGameUI.ButtonCallouts.Details>", OnButtonBar_Details);
}

function OnAddFriend()
{
	
}

function OnDetails()
{
	
}


/**
 * Called when the user submits the current list index.
 *
 * @param	Sender	the list that is submitting the selection
 */
function OnFriendsList_SubmitSelection( UIList Sender, optional int PlayerIndex=0 )
{
	OnDetails();
}

/** Buttonbar Callbacks */
function bool OnButtonBar_AddFriend(UIScreenObject InButton, int PlayerIndex)
{
	OnAddFriend();

	return true;
}

function bool OnButtonBar_Details(UIScreenObject InButton, int PlayerIndex)
{
	OnDetails();

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
		if(EventParms.InputKeyName=='XboxTypeS_Y')
		{
			OnAddFriend();
			bResult=true;
		}
	}

	return bResult;
}

DefaultProperties
{

}
