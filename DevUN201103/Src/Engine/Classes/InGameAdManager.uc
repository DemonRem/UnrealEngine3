/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 *
 * This object is responsible for the display and callbacks associated
 * with handling ingame advertisements
 */
class InGameAdManager extends Object
	native(GameEngine);

cpptext
{
	// the exec functions will call these virtual functions to allow intrinsic subclasses to handle
	// the actual functionality
	virtual void NativeInit()
	{
	}
	virtual void NativeShowBanner(UBOOL bShowOnBottomOfScreen)
	{
	}
	virtual void NativeHideBanner()
	{
	}
	virtual void NativeForceCloseAd()
	{
	}

	/**
	 * Called by platform when the user clicks on the ad banner
	 */
	void OnUserClickedBanner();

	/**
	 * Called by platform when an opened ad is closed
	 */
	void OnUserClosedAd();
}

/** If true, the game will pause when the user clicks on the ad, which could take over the screen */
var bool bShouldPauseWhileAdOpen;

/** Delegates to call when the user clicks on a banner ad */
var protected array<delegate<OnUserClickedBanner> > ClickedBannerDelegates;

/** Delegates to call when the user closes an opened ad */
var protected array<delegate<OnUserClosedAdvertisement> > ClosedAdDelegates;




/**
 * Perform any needed initialization
 */
native event Init();

/**
 * Allows the platform to put up an advertisement on top of the viewport. Note that 
 * this will not resize the viewport, simply cover up a portion of it.
 *
 * @param bShowOnBottomOfScreen If TRUE, advertisement will be shown on the bottom, otherwise, the top
 */
native function ShowBanner(bool bShowBottomOfScreen);

/**
 * Hides the advertisement banner shown with ShowInGameAdvertisementBanner. If the ad is currently open
 * (ie, the user is interacting with the ad), the ad will be forcibly closed (see ForceCloseInGameAdvertisement)
 */
native function HideBanner();

/**
 * If the game absolutely must close an opened (clicked on) advertisement, call this function.
 * This may lead to loss of revenue, so don't do it unnecessarily.
 */
native function ForceCloseAd();



/**
 * Sets the value of bShouldPauseWhileAdOpen
 */
function SetPauseWhileAdOpen(bool bShouldPause)
{
	bShouldPauseWhileAdOpen = bShouldPause;
}


/**
 * Delegate called when user clicks on an banner ad. Base class already handles pausing,
 * so a delegate is only needed if you need extra handling. If you set it to a PC or other actor
 * function, make sure to set it back when switching levels.
 */
delegate OnUserClickedBanner();

/**
 * Adds a delegate to the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function AddClickedBannerDelegate(delegate<OnUserClickedBanner> InDelegate)
{
	// Add this delegate to the array if not already present
	if (ClickedBannerDelegates.Find(InDelegate) == INDEX_NONE)
	{
		ClickedBannerDelegates.AddItem(InDelegate);
	}
}

/**
 * Removes a delegate from the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function ClearClickedBannerDelegate(delegate<OnUserClickedBanner> InDelegate)
{
	local int RemoveIndex;

	// Remove this delegate from the array if found
	RemoveIndex = ClickedBannerDelegates.Find(InDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		ClickedBannerDelegates.Remove(RemoveIndex,1);
	}
}


/**
 * Delegate called when user closes an ad (after clicking on banner). Base class already handles 
 * pausing, so a delegate is only needed if you need extra handling.  If you set it to a PC or other actor
 * function, make sure to set it back when switching levels.
 */
delegate OnUserClosedAdvertisement();

/**
 * Adds a delegate to the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function AddClosedAdDelegate(delegate<OnUserClosedAdvertisement> InDelegate)
{
	// Add this delegate to the array if not already present
	if (ClosedAdDelegates.Find(InDelegate) == INDEX_NONE)
	{
		ClosedAdDelegates.AddItem(InDelegate);
	}
}

/**
 * Removes a delegate from the list of listeners
 *
 * @param InDelegate the delegate to use for notifications
 */
function ClearClosedAdDelegate(delegate<OnUserClosedAdvertisement> InDelegate)
{
	local int RemoveIndex;

	// Remove this delegate from the array if found
	RemoveIndex = ClosedAdDelegates.Find(InDelegate);
	if (RemoveIndex != INDEX_NONE)
	{
		ClosedAdDelegates.Remove(RemoveIndex,1);
	}
}


defaultproperties
{
	bShouldPauseWhileAdOpen=true
}