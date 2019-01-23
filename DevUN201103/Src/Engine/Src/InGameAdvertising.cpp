/*=============================================================================
	InGameAdvertising.cpp: Base implementation for ingame advertising management
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineGameEngineClasses.h"

IMPLEMENT_CLASS(UInGameAdManager);

/**
 * Call through to subclass's native implementations function
 */
void UInGameAdManager::Init()
{
	NativeInit();
}
void UInGameAdManager::ShowBanner(UBOOL bShowOnBottomOfScreen)
{
	NativeShowBanner(bShowOnBottomOfScreen);
}
void UInGameAdManager::HideBanner()
{
	NativeHideBanner();
}
void UInGameAdManager::ForceCloseAd()
{
	NativeForceCloseAd();
}

/**
 * Called by platform when the user clicks on the ad banner
 */
void UInGameAdManager::OnUserClickedBanner()
{
	if (bShouldPauseWhileAdOpen && GWorld->GetWorldInfo()->NetMode == NM_Standalone)
	{
		// pause the first player controller
		if (GEngine && GEngine->GamePlayers.Num() && GEngine->GamePlayers(0))
		{
			GEngine->GamePlayers(0)->Actor->ConsoleCommand(TEXT("PAUSE"));
		}
	}

	// call the delegates
	const TArray<FScriptDelegate> Delegates = ClickedBannerDelegates;
	InGameAdManager_eventOnUserClickedBanner_Parms Parms(EC_EventParm);
	// Iterate through the delegate list
	for (INT Index = 0; Index < Delegates.Num(); Index++)
	{
		// Make sure the pointer if valid before processing
		const FScriptDelegate* ScriptDelegate = &Delegates(Index);
		if (ScriptDelegate != NULL)
		{
			// Send the notification of completion
			ProcessDelegate(NAME_None,ScriptDelegate,&Parms);
		}
	}
}


/**
 * Called by platform when an opened ad is closed
 */
void UInGameAdManager::OnUserClosedAd()
{
	if (bShouldPauseWhileAdOpen && GWorld->GetWorldInfo()->NetMode == NM_Standalone)
	{
		// pause the first player controller
		if (GEngine && GEngine->GamePlayers.Num() && GEngine->GamePlayers(0))
		{
			GEngine->GamePlayers(0)->Actor->ConsoleCommand(TEXT("PAUSE"));
		}
	}

	// call the delegates
	const TArray<FScriptDelegate> Delegates = ClosedAdDelegates;
	InGameAdManager_eventOnUserClosedAdvertisement_Parms Parms(EC_EventParm);
	// Iterate through the delegate list
	for (INT Index = 0; Index < Delegates.Num(); Index++)
	{
		// Make sure the pointer if valid before processing
		const FScriptDelegate* ScriptDelegate = &Delegates(Index);
		if (ScriptDelegate != NULL)
		{
			// Send the notification of completion
			ProcessDelegate(NAME_None,ScriptDelegate,&Parms);
		}
	}
}

