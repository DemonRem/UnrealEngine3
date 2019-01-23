/*=============================================================================
	DownloadableContent.h
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/


/**
 * This will add files that were downloaded (or similar mechanism) to the engine. The
 * files themselves will have been found in a platform-specific method.
 *
 * @param PackageFiles The list of files that are game content packages (.upk etc)
 * @param NonPacakgeFiles The list of files in the DLC that are everything but packages (.ini, etc)
 * @param UserIndex Optional user index to associate with the content (used for removing)
 */
void appInstallDownloadableContent(const TArray<FString>& PackageFiles, const TArray<FString>& NonPackageFiles, INT UserIndex=NO_USER_SPECIFIED);

/**
 * Removes downloadable content from the system. Can use UserIndex's or not. Will always remove
 * content that did not have a user index specified
 * 
 * @param MaxUserIndex Max number of users to flush (this will iterate over all users from 0 to MaxNumUsers), as well as NO_USER 
 */
void appRemoveDownloadableContent(INT MaxUserIndex=0);

/**
 * Game-specific code to handle DLC being added or removed
 * Each game _must_ implement this function, or you will get a link error.
 * 
 * @param bContentWasInstalled TRUE if DLC was installed, FALSE if it was removed
 */
void appOnDownloadableContentChanged(UBOOL bContentWasInstalled);
