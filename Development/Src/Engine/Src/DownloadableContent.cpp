/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "DownloadableContent.h"

// @todo: figure out a good way to use Combine without including FConfigCacheIni.h maybe?
#include "FConfigCacheIni.h"


/**
 * This will add files that were downloaded (or similar mechanism) to the engine. The
 * files themselves will have been found in a platform-specific method.
 *
 * @param PackageFiles The list of files that are game content packages (.upk etc)
 * @param NonPacakgeFiles The list of files in the DLC that are everything but packages (.ini, etc)
 * @param UserIndex Optional user index to associate with the content (used for removing)
 */
void appInstallDownloadableContent(const TArray<FString>& PackageFiles, const TArray<FString>& NonPackageFiles, INT UserIndex)
{
	// tell the package cache about all the new packages
	for (INT PackageIndex = 0; PackageIndex < PackageFiles.Num(); PackageIndex++)
	{
		GPackageFileCache->CacheDownloadedPackage(*PackageFiles(PackageIndex), UserIndex);
	}

	// temporarily allow the GConfigCache to perform file operations if they were off
	UBOOL bWereFileOpsDisabled = GConfig->AreFileOperationsDisabled();
	GConfig->EnableFileOperations();

	// tell ini cache that any new ini sections in the loop below are special downloaded sections
	GConfig->StartUsingDownloadedCache(UserIndex);

	// get a list of all known config files
	TArray<FFilename> ConfigFiles;
	GConfig->GetConfigFilenames(ConfigFiles);

	// tell the ini cache about all the new ini files
	for (INT FileIndex = 0; FileIndex < NonPackageFiles.Num(); FileIndex++)
	{
		FFilename ContentFile = FFilename(NonPackageFiles(FileIndex)).GetCleanFilename();
		// get filename extension
		FString Ext = ContentFile.GetExtension();
			
		// skip any non-ini/loc (for current language) files
		if (Ext != TEXT("ini") && Ext != UObject::GetLanguage())
		{
			continue;
		}

		// look for the optional special divider string
		#define DividerString TEXT("__")
		INT Divider = ContentFile.InStr(DividerString);
		// if we found it, just use what's after the divider
		if (Divider != -1)
		{
			ContentFile = ContentFile.Right(ContentFile.Len() - (Divider + appStrlen(DividerString)));
		}

		FConfigFile* NewConfigFile = NULL;
		FString NewConfigFilename;

		// look for the filename in the config files
		UBOOL bWasCombined = FALSE;
		for (INT ConfigFileIndex = 0; ConfigFileIndex < ConfigFiles.Num() && !bWasCombined; ConfigFileIndex++)
		{
			// does the config file (without path) match the DLC file?
			if (ConfigFiles(ConfigFileIndex).GetCleanFilename() == ContentFile)
			{
				// get the configfile object
				NewConfigFile = GConfig->FindConfigFile(*ConfigFiles(ConfigFileIndex));
				check(NewConfigFile);

				// merge our ini file into the existing one
				NewConfigFile->Combine(*NonPackageFiles(FileIndex));

				debugf(TEXT("Merged DLC config file '%s' into existing config '%s'"), *NonPackageFiles(FileIndex), *ConfigFiles(ConfigFileIndex));

				// mark that we have combined
				bWasCombined = TRUE;

				// remember the filename
				NewConfigFilename = ConfigFiles(ConfigFileIndex);
			}
		}

		// if it wasn't combined, add a new file
		if (!bWasCombined)
		{
			// we need to create a usable pathname for the new ini/loc file
			if (Ext == TEXT("ini"))
			{
				NewConfigFilename = appGameConfigDir() + ContentFile;
			}
			else
			{
				// put this into any localization directory in the proper language sub-directory (..\ExampleGame\Localization\fra\DLCMap.fra)
				NewConfigFilename = GSys->LocalizationPaths(0) * Ext * ContentFile;
			}
			// first we set a value into the config for this filename (since we will be reading from a different
			// path than we want to store the config under, we can't use LoadFile)
			GConfig->SetBool(TEXT("DLCDummy"), TEXT("A"), FALSE, *NewConfigFilename);

			// now get the one we just made
			NewConfigFile = GConfig->FindConfigFile(*NewConfigFilename);
			
			// read in the file
			NewConfigFile->Combine(*NonPackageFiles(FileIndex));

			debugf(TEXT("Read new DLC config file '%s' into the config cache"), *NonPackageFiles(FileIndex));
		}
		
		check(NewConfigFile);
		UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
		if (GameEngine)
		{
			// look for packages to load for maps
			TMap<FName, TArray<FName> > MapMappings;
			GConfig->Parse1ToNSectionOfNames(TEXT("Engine.PackagesToFullyLoadForDLC"), TEXT("MapName"), TEXT("Package"), MapMappings, *NewConfigFilename);

			// tell the game engine about the parsed settings
			for(TMap<FName, TArray<FName> >::TIterator It(MapMappings); It; ++It)
			{
				GameEngine->AddPackagesToFullyLoad(FULLYLOAD_Map, It.Key().ToString(), It.Value(), TRUE);
			}

			// now get the per-gametype packages
			for (INT PrePost = 0; PrePost < 2; PrePost++)
			{
				TMap<FString, TArray<FString> > GameMappings;
				GConfig->Parse1ToNSectionOfStrings(TEXT("Engine.PackagesToFullyLoadForDLC"), PrePost ? TEXT("GameType_PostLoadClass") : TEXT("GameType_PreLoadClass"), TEXT("Package"), GameMappings, *NewConfigFilename);

				// tell the game engine about the parsed settings
				for(TMap<FString, TArray<FString> >::TIterator It(GameMappings); It; ++It)
				{
					// convert array of string package names to names
					TArray<FName> PackageNames;
					for (INT PackageIndex = 0; PackageIndex < It.Value().Num(); PackageIndex++)
					{
						PackageNames.AddItem(FName(*It.Value()(PackageIndex)));
					}
					GameEngine->AddPackagesToFullyLoad(PrePost ? FULLYLOAD_Game_PostLoadClass : FULLYLOAD_Game_PreLoadClass, It.Key(), PackageNames, TRUE);
				}
			}
		}
	}

	// put the ini cache back to normal
	GConfig->StopUsingDownloadedCache();
	
	// re-disable file ops if they were before
	if (bWereFileOpsDisabled)
	{
		GConfig->DisableFileOperations();
	}

	// let per-game native code run to update anything needed
	appOnDownloadableContentChanged(TRUE);
}

/**
 * Removes downloadable content from the system. Can use UserIndex's or not. Will always remove
 * content that did not have a user index specified
 * 
 * @param MaxUserIndex Max number of users to flush (this will iterate over all users from 0 to MaxNumUsers), as well as NO_USER 
 */
void appRemoveDownloadableContent(INT MaxUserIndex)
{
	// remove content not associated with a user
	GPackageFileCache->ClearDownloadedPackages();
	GConfig->RemoveDownloadedSections();

	// remove any existing content packages for all users
	for (INT UserIndex = 0; UserIndex < MaxUserIndex; UserIndex++)
	{
		GPackageFileCache->ClearDownloadedPackages(UserIndex);
		GConfig->RemoveDownloadedSections(UserIndex);
	}

	// let per-game native code run to update anything needed
	appOnDownloadableContentChanged(FALSE);

	// cleanup the list of packages to load for maps
	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	if (GameEngine)
	{
		// cleanup all of the fully loaded packages for maps (may not free memory until next GC)
		GameEngine->CleanupAllPackagesToFullyLoad();
	}
}
