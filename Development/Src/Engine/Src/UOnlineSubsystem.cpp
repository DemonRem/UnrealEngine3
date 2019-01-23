/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "DownloadableContent.h"


IMPLEMENT_CLASS(UOnlineSubsystem);
IMPLEMENT_CLASS(UOnlineGameSettings);
IMPLEMENT_CLASS(UOnlineGameSearch);

/**
 * Handle downloaded content in a platform-independent way
 *
 * @param Content The content descriptor that describes the downloaded content files
 *
 * @param return TRUE is successful
 */
UBOOL UOnlineSubsystem::ProcessDownloadedContent(const FOnlineContent& Content)
{
	// use engine DLC function
	appInstallDownloadableContent(Content.ContentPackages, Content.ContentFiles, Content.UserIndex);

	return TRUE;
}

/**
 * Flush downloaded content for all users, making the engine stop using the content
 *
 * @param MaxNumUsers Platform specific max number of users to flush (this will iterate over all users from 0 to MaxNumUsers, as well as NO_USER 
 */
void UOnlineSubsystem::FlushAllDownloadedContent(INT MaxNumUsers)
{
	appRemoveDownloadableContent(MaxNumUsers);
}
