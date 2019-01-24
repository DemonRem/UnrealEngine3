/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 *
 * This is the base class for Facebook integration (each platform has a subclass
 */
class FacebookIntegration extends Object
	native
	config(Engine)
	transient;


cpptext
{
	// the exec functions will call these virtual functions to allow intrinsic subclasses to handle
	// the actual functionality
	virtual UBOOL NativeInit()
	{
		return FALSE;
	}
	virtual UBOOL NativeAuthorize()
	{
		return FALSE;
	}
	virtual UBOOL NativeIsAuthorized()
	{
		return FALSE;
	}
	virtual void NativeDisconnect()
	{
	}
	virtual void NativeWebRequest(const FString& URL, const FString& POSTPayload)
	{
	}
	virtual void NativeFacebookRequest(const FString& GraphRequest)
	{
	}

	/**
	 * Perform any per-frame actions
	 */
	virtual void Tick(FLOAT DeltaSeconds)
	{

	}

	/**
	 * Called by platform when authorization has completed
	 */
	void OnAuthorizationComplete(UBOOL bSucceeded);

	/**
	 * Called by platform when FB request has completed
	 */
	void OnFacebookRequestComplete(const FString& JsonString);

	/**
	 * Called by platform when web request has completed
	 */
	void OnWebRequestComplete(const FString& Response);
}


/** The application ID to link to */
var config string AppID;

/** Username of the current user */
var string Username;

/** Id of the current user */
var string UserId;

/** Access token as retrieved from FB */
var string AccessToken;

/** Delegates to call when all async initialization and authorization has completed */
var protected array<delegate<OnAuthorizationComplete> > AuthorizationDelegates;

/** Delegates to call when a facebook request has completed */
var protected array<delegate<OnFacebookRequestComplete> > FacebookRequestCompleteDelegates;

/** Delegates to call when a web request has completed */
var protected array<delegate<OnWebRequestComplete> > WebRequestCompleteDelegates;



/**
 * Perform any needed initialization
 */
native event bool Init();

/**
 * Starts the process of allowing the app to use Facebook
 */
native event bool Authorize();

/**
 * @return true if the app has been authorized by the current user
 */
native event bool IsAuthorized();

/**
 * Kicks off a generic web request (response will come via delegate call)
 *
 * @param URL The URL for the request, can be http or https (if the current platform supports sending https)
 * @param POSTPayload If specified, the request will use the POST method, and the given string will be the payload (as UTF8)
 */
native event WebRequest(string URL, string POSTPayload);

/**
 * Kicks off a Facebook GraphAPI request (response will come via delegate)
 *
 * @param GraphRequest The request to make (like "me/friends")
 */
native event FacebookRequest(string GraphRequest);

/**
 * Call this to disconnect from Facebook. Next time authorization happens, the auth webpage
 * will be shown again
 */
native event Disconnect();


/**
 * Delegate type to call when authorization is all completed
 */
delegate OnAuthorizationComplete(bool bSucceeded);

/**
 * Adds a delegate to the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function AddAuthorizationCompleteDelegate(delegate<OnAuthorizationComplete> InDelegate)
{
	// Add this delegate to the array if not already present
	if (AuthorizationDelegates.Find(InDelegate) == INDEX_NONE)
	{
		AuthorizationDelegates.AddItem(InDelegate);
	}
}

/**
 * Removes a delegate from the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function ClearAuthorizationCompleteDelegate(delegate<OnAuthorizationComplete> InDelegate)
{
	local int RemoveIndex;

	// Remove this delegate from the array if found
	RemoveIndex = AuthorizationDelegates.Find(InDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		AuthorizationDelegates.Remove(RemoveIndex,1);
	}
}

/**
 * Delegate type to call when a generic facebook request is completed
 */
delegate OnFacebookRequestComplete(string JsonString);

/**
 * Adds a delegate to the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function AddFacebookRequestCompleteDelegate(delegate<OnFacebookRequestComplete> InDelegate)
{
	// Add this delegate to the array if not already present
	if (FacebookRequestCompleteDelegates.Find(InDelegate) == INDEX_NONE)
	{
		FacebookRequestCompleteDelegates.AddItem(InDelegate);
	}
}

/**
 * Removes a delegate from the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function ClearFacebookRequestCompleteDelegate(delegate<OnFacebookRequestComplete> InDelegate)
{
	local int RemoveIndex;

	// Remove this delegate from the array if found
	RemoveIndex = FacebookRequestCompleteDelegates.Find(InDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		FacebookRequestCompleteDelegates.Remove(RemoveIndex,1);
	}
}


/**
 * Delegate type to call when a generic Web request is completed
 */
delegate OnWebRequestComplete(string Response);

/**
 * Adds a delegate to the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function AddWebRequestCompleteDelegate(delegate<OnWebRequestComplete> InDelegate)
{
	// Add this delegate to the array if not already present
	if (WebRequestCompleteDelegates.Find(InDelegate) == INDEX_NONE)
	{
		WebRequestCompleteDelegates.AddItem(InDelegate);
	}
}

/**
 * Removes a delegate from the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function ClearWebRequestCompleteDelegate(delegate<OnWebRequestComplete> InDelegate)
{
	local int RemoveIndex;

	// Remove this delegate from the array if found
	RemoveIndex = WebRequestCompleteDelegates.Find(InDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		WebRequestCompleteDelegates.Remove(RemoveIndex,1);
	}
}
