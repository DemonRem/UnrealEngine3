/*=============================================================================
	UnGameCookerHelper.cpp: Game specific cooking helper class implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnGameCookerHelper.h"

//-----------------------------------------------------------------------------
//	GenerateGameContentSeekfree
//-----------------------------------------------------------------------------
#define GAME_CONTENT_PKG_PREFIX TEXT("MPContent")

static DOUBLE		GenerateGameContentTime;
static DOUBLE		GenerateGameContentInitTime;
static DOUBLE		GenerateGameContentCommonInitTime;
static DOUBLE		GenerateGameContentListGenerationTime;
static DOUBLE		GenerateGameContentCommonGenerationTime;
static DOUBLE		GenerateGameContentPackageGenerationTime;
static DOUBLE		GenerateGameContentCommonPackageGenerationTime;

static DOUBLE		ForcedContentTime;

/**
 *	Helper funtion for loading the object or class of the given name.
 *	If supplied, it will also add it to the given object referencer.
 *
 *	@param	Commandlet		The commandlet being run
 *	@param	ObjectName		The name of the object/class to attempt to load
 *	@param	ObjReferencer	If not NULL, the object referencer to add the loaded object/class to
 *
 *	@return	UObject*		The loaded object/class, NULL if not found
 */
UObject* FGameContentCookerHelper::LoadObjectOrClass(class UCookPackagesCommandlet* Commandlet, const FString& ObjectName, UObjectReferencer* ObjRefeferencer)
{
	if (ObjectName.InStr(TEXT(":")) != INDEX_NONE)
	{
		// skip these objects under the assumption that their outer will have been loaded
		return NULL;
	}

	// load the requested content object (typically a content class) 
	UObject* ContentObj = LoadObject<UObject>(NULL, *ObjectName, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	if (!ContentObj)
	{
		ContentObj = LoadObject<UClass>(NULL, *ObjectName, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	}

	if (ContentObj)
	{
		if (ObjRefeferencer != NULL)
		{
			ObjRefeferencer->ReferencedObjects.AddUniqueItem(ContentObj);					
		}
	}

	return ContentObj;
}


/**
 *	Mark the given object as cooked
 *
 *	@param	InObject	The object to mark as cooked.
 */
void FGameContentCookerHelper::MarkObjectAsCooked(UObject* InObject)
{
	InObject->ClearFlags(RF_ForceTagExp);
	InObject->SetFlags(RF_MarkedByCooker);
	UObject* MarkOuter = InObject->GetOuter();
	while (MarkOuter)
	{
		MarkOuter->ClearFlags(RF_ForceTagExp);
		MarkOuter->SetFlags(RF_MarkedByCooker);
		MarkOuter = MarkOuter->GetOuter();
	}
}

/**
 *	Attempt to load each of the objects in the given list.
 *	If loaded, iterate over all objects that get loaded along with it and add them to
 *	the given OutLoadedObjectMap - incrementing the count of objects found.
 *
 *	@param	InObjectsToLoad			The list of objects to load
 *	@param	OutLoadedObjectMap		All objects loaded, and the number of times they were loaded
 *	
 *	@return	INT						The number of object in the InObjectsToLoad list that successfully loaded
 */
INT FGameContentCookerHelper::GenerateLoadedObjectList(class UCookPackagesCommandlet* Commandlet, const TArray<FString>& InObjectsToLoad, TMap<FString,INT>& OutLoadedObjectMap)
{
	INT FoundObjectCount = 0;
	for (INT LoadIdx = 0; LoadIdx < InObjectsToLoad.Num(); LoadIdx++)
	{
		FString ObjectStr = InObjectsToLoad(LoadIdx);
		UObject* ContentObj = LoadObjectOrClass(Commandlet, ObjectStr, NULL);
		if (ContentObj != NULL)
		{
			FoundObjectCount++;

			// Iterate over all objects that are loaded and not marked by cooker, adding them to the list
			for (TObjectIterator<UObject> ObjIt; ObjIt; ++ObjIt)
			{
				UObject* CheckObject = *ObjIt;

				if (CheckObject->GetOuter() != NULL)
				{
					if (!CheckObject->HasAnyFlags(RF_Transient) && !CheckObject->IsIn(UObject::GetTransientPackage()))
					{
						if (CheckObject->HasAnyFlags(RF_MarkedByCooker) == FALSE)
						{
							FString ExportPathName = CheckObject->GetPathName();
							ExportPathName = ExportPathName.ToUpper();
							INT* EntryPtr = OutLoadedObjectMap.Find(ExportPathName);
							if (EntryPtr == NULL)
							{
								INT Temp = 0;
								OutLoadedObjectMap.Set(ExportPathName, Temp);
								EntryPtr = OutLoadedObjectMap.Find(ExportPathName);
							}
							check(EntryPtr);
							*EntryPtr = *EntryPtr + 1;
						}
					}
				}
			}
			UObject::CollectGarbage(RF_Native);
		}
	}

	return FoundObjectCount;
}

/**
 * Helper for adding game content standalone packages for cooking
 */

/**
 * Initialize options from command line options
 *
 *	@param	Tokens			Command line tokens parsed from app
 *	@param	Switches		Command line switches parsed from app
 */
void FGenerateGameContentSeekfree::InitOptions(const TArray<FString>& Tokens, const TArray<FString>& Switches)
{
	SCOPE_SECONDS_COUNTER(GenerateGameContentTime);
	// Check for flag to recook seekfree game type content
	bForceRecookSeekfreeGameTypes = Switches.ContainsItem(TEXT("RECOOKSEEKFREEGAMETYPES"));
	if (Switches.FindItemIndex(TEXT("MTCHILD")) != INDEX_NONE )
	{
		// child will only cook this if asked, so add them all to the cook list
		bForceRecookSeekfreeGameTypes = TRUE; 
	}
}

/**
 *	Check if the given object or any of its dependencies are newer than the given time
 *
 *	@param	Commandlet		The commandlet being run
 *	@param	ContentStr		The name of the object in question
 *	@param	CheckTime		The time to check against
 *	@param	bIsSeekfreeFileDependencyNewerMap	A mapping of base package states to prevent repeated examination of the same content
 *
 *	@return	UBOOL			TRUE if the content (or any of its dependencies) is newer than the specified time
 */
UBOOL FGenerateGameContentSeekfree::CheckIfContentIsNewer(
	class UCookPackagesCommandlet* Commandlet,
	const FString& ContentStr, 
	DOUBLE CheckTime,
	TMap<FString,UBOOL>& bIsSeekfreeFileDependencyNewerMap
	)
{
	UBOOL bIsSeekfreeFileDependencyNewer = FALSE;

	FString SrcContentPackageFileBase = ContentStr;
	// strip off anything after the base package name
	INT FoundIdx = ContentStr.InStr(TEXT("."));
	if( FoundIdx != INDEX_NONE )
	{
		SrcContentPackageFileBase = ContentStr.LeftChop(ContentStr.Len() - FoundIdx);
	}

	FString SrcContentPackageFilePath;
	// find existing content package
	if (GPackageFileCache->FindPackageFile(*SrcContentPackageFileBase, NULL, SrcContentPackageFilePath))
	{
		if (Commandlet->bCoderMode == FALSE)
		{
			// check if we've already done the (slow) dependency check for this package
			UBOOL* bExistingEntryPtr = bIsSeekfreeFileDependencyNewerMap.Find(SrcContentPackageFileBase);					
			if( bExistingEntryPtr )
			{
				// use existing entry to trigger update
				bIsSeekfreeFileDependencyNewer |= *bExistingEntryPtr;
			}
			else
			{
				// do dependency check for packages coming from exports of the current content package
				UBOOL bHasNewer = Commandlet->AreSeekfreeDependenciesNewer(NULL, *SrcContentPackageFilePath, CheckTime);
				bIsSeekfreeFileDependencyNewer |= bHasNewer;
				// keep track of this content package entry to check for duplicates
				bIsSeekfreeFileDependencyNewerMap.Set(SrcContentPackageFileBase,bHasNewer);
			}
		}
		else
		{
			DOUBLE SrcTime = GFileManager->GetFileTimestamp(*SrcContentPackageFilePath);
			if (SrcTime > CheckTime)
			{
				bIsSeekfreeFileDependencyNewer = TRUE;
			}
		}
	}

	return bIsSeekfreeFileDependencyNewer;
}

/**
 *	Add the standalone seekfree entries to the package list for the given ContentMap
 *
 *	@param	InContentMap		The content map being processed for this call
 *	@param	Commandlet			The commandlet being run
 *	@param	Platform			The platform being cooked for
 *	@param	ShaderPlatform		The shader platform being cooked for
 *	@param	*Pairs				The filename pair lists
 *
 *	@return TRUE if succeeded
 */
UBOOL FGenerateGameContentSeekfree::GeneratePackageListForContentMap( 
	TMap<FString,FGameContentEntry>& InContentMap,
	class UCookPackagesCommandlet* Commandlet, 
	UE3::EPlatformType Platform,
	EShaderPlatform ShaderPlatform,
	TArray<FPackageCookerInfo>& NotRequiredFilenamePairs,
	TArray<FPackageCookerInfo>& RegularFilenamePairs,
	TArray<FPackageCookerInfo>& MapFilenamePairs,
	TArray<FPackageCookerInfo>& ScriptFilenamePairs,
	TArray<FPackageCookerInfo>& StartupFilenamePairs,
	TArray<FPackageCookerInfo>& StandaloneSeekfreeFilenamePairs)
{
	UBOOL bSuccess=TRUE;

	FString OutDir = appGameDir() + PATH_SEPARATOR + FString(GAME_CONTENT_PKG_PREFIX) + PATH_SEPARATOR;
	TMap<FString,UBOOL> bIsSeekfreeFileDependencyNewerMap;

	for (TMap<FString,FGameContentEntry>::TIterator GTIt(InContentMap); GTIt; ++GTIt)
	{
		const FString& SourceName = GTIt.Key();
		FGameContentEntry& ContentEntry = GTIt.Value();

		if (ContentEntry.ContentList.Num() > 0)
		{
			// Source filename of temp package to be cooked
			ContentEntry.PackageCookerInfo.SrcFilename = OutDir + SourceName + TEXT(".upk");
			// Destination filename for cooked seekfree package
			ContentEntry.PackageCookerInfo.DstFilename = Commandlet->GetCookedPackageFilename(ContentEntry.PackageCookerInfo.SrcFilename);

			// setup flags to cook as seekfree standalone package
			ContentEntry.PackageCookerInfo.bShouldBeSeekFree = TRUE;
			ContentEntry.PackageCookerInfo.bIsNativeScriptFile = FALSE;
			ContentEntry.PackageCookerInfo.bIsCombinedStartupPackage = FALSE;
			ContentEntry.PackageCookerInfo.bIsStandaloneSeekFreePackage = TRUE;
			ContentEntry.PackageCookerInfo.bShouldOnlyLoad = FALSE;

			// check to see if this seekfree package's dependencies require it to be recooked
			UBOOL bIsSeekfreeFileDependencyNewer = FALSE;
			if (bForceRecookSeekfreeGameTypes)
			{
				// force recook
				bIsSeekfreeFileDependencyNewer = TRUE;
			}
			else
			{
				FString ActualDstName = ContentEntry.PackageCookerInfo.DstFilename.GetBaseFilename(FALSE);
				if (Commandlet->bCoderMode == FALSE)
				{
					ActualDstName += STANDALONE_SEEKFREE_SUFFIX;
				}
				ActualDstName += FString(TEXT(".")) + ContentEntry.PackageCookerInfo.DstFilename.GetExtension();
				// get dest cooked file timestamp
				DOUBLE Time = GFileManager->GetFileTimestamp(*ActualDstName);
				// iterate over source content that needs to be cooked for this game type
				for( INT ContentIdx=0; ContentIdx < ContentEntry.ContentList.Num(); ContentIdx++ )
				{
					const FString& ContentStr = ContentEntry.ContentList(ContentIdx);
					bIsSeekfreeFileDependencyNewer |= CheckIfContentIsNewer(Commandlet, ContentStr, Time, bIsSeekfreeFileDependencyNewerMap);
				}

				for (INT ForcedIdx = 0; ForcedIdx < ContentEntry.ForcedContentList.Num(); ForcedIdx++)
				{
					const FString& ContentStr = ContentEntry.ForcedContentList(ForcedIdx);
					bIsSeekfreeFileDependencyNewer |= CheckIfContentIsNewer(Commandlet, ContentStr, Time, bIsSeekfreeFileDependencyNewerMap);
				}
			}

			//@todo. Check if the language version being cooked for is present as well...
			if( !bIsSeekfreeFileDependencyNewer )
			{
			}

			if( !bIsSeekfreeFileDependencyNewer )
			{
				debugf(NAME_Log, TEXT("GamePreloadContent: standalone seekfree package for %s is UpToDate, skipping"), *SourceName);
			}
			else
			{
				// add the entry to the standalone seekfree list
				StandaloneSeekfreeFilenamePairs.AddItem(ContentEntry.PackageCookerInfo);

				debugf(NAME_Log, TEXT("GamePreloadContent: Adding standalone seekfree package for %s"), *SourceName);
			}
		}
		else
		{
			debugf(NAME_Log, TEXT("GeneratePackageListForContentMap: Empty content list for %s"), *SourceName);
		}
	}

	return bSuccess;
}

/**
 * Adds one standalone seekfree entry to the package list for each game type specified from ini.  
 *
 * @return TRUE if succeeded
 */
UBOOL FGenerateGameContentSeekfree::GeneratePackageList( 
	class UCookPackagesCommandlet* Commandlet, 
	UE3::EPlatformType Platform,
	EShaderPlatform ShaderPlatform,
	TArray<FPackageCookerInfo>& NotRequiredFilenamePairs,
	TArray<FPackageCookerInfo>& RegularFilenamePairs,
	TArray<FPackageCookerInfo>& MapFilenamePairs,
	TArray<FPackageCookerInfo>& ScriptFilenamePairs,
	TArray<FPackageCookerInfo>& StartupFilenamePairs,
	TArray<FPackageCookerInfo>& StandaloneSeekfreeFilenamePairs)
{
	SCOPE_SECONDS_COUNTER(GenerateGameContentTime);

	//@todo. DLC needs to be able to support new gametypes!
	if (Commandlet->bIsInUserMode)
	{
		return TRUE;
	}

	UBOOL bSuccess=TRUE;

	{
		// Generate the common package list
		SCOPE_SECONDS_COUNTER(GenerateGameContentCommonGenerationTime);
		if (GeneratePackageListForContentMap(
			GameTypeCommonContentMap, 
			Commandlet, 
			Platform,
			ShaderPlatform,
			NotRequiredFilenamePairs,
			RegularFilenamePairs,
			MapFilenamePairs,
			ScriptFilenamePairs,
			StartupFilenamePairs,
			StandaloneSeekfreeFilenamePairs) == FALSE)
		{
			bSuccess = FALSE;
		}
	}

	{
		// update the package cooker info for each content entry
		SCOPE_SECONDS_COUNTER(GenerateGameContentListGenerationTime);
		if (GeneratePackageListForContentMap(
			GameContentMap, 
			Commandlet, 
			Platform,
			ShaderPlatform,
			NotRequiredFilenamePairs,
			RegularFilenamePairs,
			MapFilenamePairs,
			ScriptFilenamePairs,
			StartupFilenamePairs,
			StandaloneSeekfreeFilenamePairs) == FALSE)
		{
			bSuccess = FALSE;
		}
	}

	return bSuccess;
}

/**
 * Cooks passed in object if it hasn't been already.
 *
 *	@param	Commandlet					The cookpackages commandlet being run
 *	@param	Package						Package going to be saved
 *	@param	Object						Object to cook
 *	@param	bIsSavedInSeekFreePackage	Whether object is going to be saved into a seekfree package
 *
 *	@return	UBOOL						TRUE if the object should continue the 'normal' cooking operations.
 *										FALSE if the object should not be processed any further.
 */
UBOOL FGenerateGameContentSeekfree::CookObject(class UCookPackagesCommandlet* Commandlet, UPackage* Package, UObject* Object, UBOOL bIsSavedInSeekFreePackage)
{
	if (CurrentCommonPackageInfo == NULL)
	{
		return TRUE;
	}

	// Check for the object in the current list.
	FString FindName = Object->GetPathName();
	FindName = FindName.ToUpper();
	if (CurrentCommonPackageInfo->CookedContentList.Find(FindName) != NULL)
	{
		MarkObjectAsCooked(Object);
		// Return FALSE to indicate no further processing of this object should be done
		return FALSE;
	}
	return TRUE;
}

/** 
 *	PreLoadPackageForCookingCallback
 *	This function will be called in LoadPackageForCooking, prior to the actual loading of the package.
 *	It is intended for pre-loading any required packages, etc.
 *
 *	@param	Commandlet		The cookpackages commandlet being run
 *	@param	Filename		The name of the package to load.
 *
 *	@return	UBOOL		TRUE if the package should be processed further.
 *						FALSE if the cook of this package should be aborted.
 */
UBOOL FGenerateGameContentSeekfree::PreLoadPackageForCookingCallback(class UCookPackagesCommandlet* Commandlet, const TCHAR* Filename)
{
	// Is it a map?
	FString CheckMapExt = TEXT(".");
	CheckMapExt += FURL::DefaultMapExt;
	if (appStristr(Filename, *CheckMapExt) == NULL)
	{
		CurrentCommonPackageInfo = NULL;
		// Not a map - let it go
		return TRUE;
	}

	FFilename TempFilename(Filename);
	AGameInfo* GameInfoDefaultObject = NULL;
	UClass* GameInfoClass = FindObject<UClass>(ANY_PACKAGE, TEXT("GameInfo"));
	if(GameInfoClass != NULL)
	{
		GameInfoDefaultObject = GameInfoClass->GetDefaultObject<AGameInfo>();
	}

	if (GameInfoDefaultObject != NULL)
	{
		// The intention here is to 'load' the prefix-common package prior to loading
		// the map. This is to prevent cooking objects in the common pkg into the map.

		FString CheckName = TempFilename.GetBaseFilename();
		FGameTypePrefix GTPrefix;
		appMemzero(&GTPrefix, sizeof(FGameTypePrefix));
		if (GameInfoDefaultObject->GetSupportedGameTypes(CheckName, GTPrefix) == FALSE)
		{
			// Didn't find a prefix type??
			CurrentCommonPackageInfo = NULL;
			warnf(NAME_Log, TEXT("Failed to find supported game type(s) for %s"), *CheckName);
		}
		else 
		{
			if (GTPrefix.bUsesCommonPackage == TRUE)
			{
				// Only preload the common package
				FString CommonName = GTPrefix.Prefix + TEXT("_Common");
				CommonName = CommonName.ToUpper();

				FForceCookedInfo* CheckCommonInfo = Commandlet->PersistentCookerData->GetCookedPrefixCommonInfo(CommonName, FALSE);
				if (!CheckCommonInfo) 
				{
					FGameContentEntry* GameContentEntry = GameTypeCommonContentMap.Find(CommonName);
					if (GameContentEntry)
					{
						debugf(TEXT("Generating %s '%s' on the fly"),*CommonName,*GameContentEntry->PackageCookerInfo.SrcFilename);
						GenerateCommonPackageData(Commandlet,*GameContentEntry->PackageCookerInfo.SrcFilename);

						// reload it; may have been created
						CheckCommonInfo = Commandlet->PersistentCookerData->GetCookedPrefixCommonInfo(CommonName, FALSE);
						if (!CheckCommonInfo)
						{
							warnf(TEXT("Failed to generate %s '%s' on the fly."),*CommonName,*GameContentEntry->PackageCookerInfo.SrcFilename);
							// force an entry so we don't keep trying
							CheckCommonInfo = Commandlet->PersistentCookerData->GetCookedPrefixCommonInfo(CommonName, TRUE);
						}
					}
				}
				if (CurrentCommonPackageInfo != CheckCommonInfo)
				{
					warnf(NAME_Log, TEXT("Setting map prefix GameType to %s"), *CommonName);
					CurrentCommonPackageInfo = CheckCommonInfo;
				}
			}
			else
			{
				if (CurrentCommonPackageInfo != NULL)
				{
					warnf(NAME_Log, TEXT("Clearing map prefix GameType"));
				}
				CurrentCommonPackageInfo = NULL;
			}
		}
 	}
	else
	{
		warnf(NAME_Log, TEXT("Failed to find gameinfo object."));
		CurrentCommonPackageInfo = NULL;
	}

	return TRUE;
}

/**
 *	Create the package for the given filename if it is one that is generated
 *	
 *	@param	Commandlet		The cookpackages commandlet being run
 *	@param	InContentMap	The content map to check
 *	@param	Filename		Current filename that needs to be loaded for cooking
 *	
 *	@return	UPakcage*		The create package; NULL if failed or invalid
 */
UPackage* FGenerateGameContentSeekfree::CreateContentPackageForCooking(class UCookPackagesCommandlet* Commandlet, TMap<FString,FGameContentEntry>& InContentMap, const TCHAR* Filename)
{
	UPackage* Result=NULL;
	FString CheckFilename = FString(Filename).ToUpper();
	for (TMap<FString,FGameContentEntry>::TIterator GTIt(InContentMap); GTIt; ++GTIt)
	{
		const FString& CommonName = GTIt.Key();
		FGameContentEntry& ContentEntry = GTIt.Value();

		if (ContentEntry.PackageCookerInfo.SrcFilename == CheckFilename)
		{
			if ((ContentEntry.ContentList.Num() > 0) || (ContentEntry.ForcedContentList.Num() > 0))
			{
				// create a temporary package to import the content objects
				Result = UObject::CreatePackage(NULL, *ContentEntry.PackageCookerInfo.SrcFilename);
				if (Result == NULL)
				{
					warnf(NAME_Warning, TEXT("GamePreloadContent: Couldn't generate package %s"), *ContentEntry.PackageCookerInfo.SrcFilename);
				}
				else
				{
					// create a referencer to keep track of the content objects added in the package
					UObjectReferencer* ObjRefeferencer = ConstructObject<UObjectReferencer>(UObjectReferencer::StaticClass(),Result);
					ObjRefeferencer->SetFlags( RF_Standalone );

					// load all content objects and add them to the package
					for (INT ContentIdx = 0; ContentIdx < ContentEntry.ContentList.Num(); ContentIdx++)
					{
						const FString& ContentStr = ContentEntry.ContentList(ContentIdx);
						// load the requested content object (typically a content class) 
						LoadObjectOrClass(Commandlet, ContentStr, ObjRefeferencer);
					}

					for (INT ForcedIdx = 0; ForcedIdx < ContentEntry.ForcedContentList.Num(); ForcedIdx++)
					{
						const FString& ContentStr = ContentEntry.ForcedContentList(ForcedIdx);
						// load the requested content object (typically a content class) 
						LoadObjectOrClass(Commandlet, ContentStr, ObjRefeferencer);
					}
					// need to fully load before saving 
					Result->FullyLoad();
				}
			}
		}
	}
	return Result;
}

/**
 * Match game content package filename to the current filename being cooked and 
 * create a temporary package for it instead of loading a package from file. This
 * will also load all the game content needed and add an entry to the object
 * referencer in the package.
 * 
 * @param Commandlet - commandlet that is calling back
 * @param Filename - current filename that needs to be loaded for cooking
 *
 * @return TRUE if succeeded
 */
UPackage* FGenerateGameContentSeekfree::LoadPackageForCookingCallback(class UCookPackagesCommandlet* Commandlet, const TCHAR* Filename)
{
	SCOPE_SECONDS_COUNTER(GenerateGameContentTime);

	UPackage* Result=NULL;

	// Is it a prefix common package?
	{
		SCOPE_SECONDS_COUNTER(GenerateGameContentCommonPackageGenerationTime);

		if (GenerateCommonPackageData(Commandlet, Filename) == TRUE)
		{
			Result = CreateContentPackageForCooking(Commandlet, GameTypeCommonContentMap, Filename);
		}
	}

	// find package that needs to be generated by looking up the filename
	if (Result == NULL)
	{
		SCOPE_SECONDS_COUNTER(GenerateGameContentPackageGenerationTime);
		Result = CreateContentPackageForCooking(Commandlet, GameContentMap, Filename);
	}

	return Result;
}

/**
 *	Dump out stats specific to the game cooker helper.
 */
void FGenerateGameContentSeekfree::DumpStats()
{
	warnf( NAME_Log, TEXT("") );
	warnf( NAME_Log, TEXT("  GameContent Stats:") );
	warnf( NAME_Log, TEXT("    Total time                             %7.2f seconds"), GenerateGameContentTime );
	warnf( NAME_Log, TEXT("    Init time                              %7.2f seconds"), GenerateGameContentInitTime );
	warnf( NAME_Log, TEXT("    Common init time                       %7.2f seconds"), GenerateGameContentCommonInitTime );
	warnf( NAME_Log, TEXT("    List generation time                   %7.2f seconds"), GenerateGameContentListGenerationTime );
	warnf( NAME_Log, TEXT("    Common generation time                 %7.2f seconds"), GenerateGameContentCommonGenerationTime );
	warnf( NAME_Log, TEXT("    Package generation time                %7.2f seconds"), GenerateGameContentPackageGenerationTime );
	warnf( NAME_Log, TEXT("    Common package generation time         %7.2f seconds"), GenerateGameContentCommonPackageGenerationTime );
}

/**
 * Initializes the list of game content packages that need to generated. 
 * Game types along with the content to be loaded for each is loaded from ini.
 * Set in [Cooker.MPGameContentCookStandalone] section
 */
void FGenerateGameContentSeekfree::Init()
{
	SCOPE_SECONDS_COUNTER(GenerateGameContentTime);
	SCOPE_SECONDS_COUNTER(GenerateGameContentInitTime);

	bForceRecookSeekfreeGameTypes=FALSE;
	GameContentMap.Empty();

	// check to see if gametype content should be cooked into its own SF packages
	// enabling this flag will remove the hard game references from the map files
	if (GEngine->bCookSeparateSharedMPGameContent)
	{
		debugf(NAME_Log, TEXT("Saw bCookSeparateSharedMPGameContent flag."));
		// Allow user to specify a single game type on command line.
		FString CommandLineGameType;
		Parse( appCmdLine(), TEXT("MPGAMETYPE="), CommandLineGameType );

		// find the game strings and content to be loaded for each
		TMap<FString, TArray<FString> > MPGameContentStrings;
		GConfig->Parse1ToNSectionOfStrings(TEXT("Cooker.MPGameContentCookStandalone"), TEXT("GameType"), TEXT("Content"), MPGameContentStrings, GEditorIni);
		for( TMap<FString,TArray<FString> >::TConstIterator It(MPGameContentStrings); It; ++It )
		{
			const FString& GameTypeStr = It.Key();
			const TArray<FString>& GameContent = It.Value();
			for( INT Idx=0; Idx < GameContent.Num(); Idx++ )
			{
				const FString& GameContentStr = GameContent(Idx);
				// Only add game type if it matches command line or no command line override was specified.
				if( CommandLineGameType.Len() == 0 || CommandLineGameType == GameTypeStr )
				{
					AddGameContentEntry(GameContentMap, *GameTypeStr, *GameContentStr, FALSE);
				}
			}
		}

		// Setup the common prefix pakages
		InitializeCommonPrefixPackages();
	}
}

/**
 *	Checks to see if any of the map prefix --> gametypes use a common package.
 *	If so, it will store off the information required to create it.
 */
void FGenerateGameContentSeekfree::InitializeCommonPrefixPackages()
{
	SCOPE_SECONDS_COUNTER(GenerateGameContentCommonInitTime);

	if (GEngine->bCookSeparateSharedMPGameContent)
	{
		GameTypeCommonContentMap.Empty();

		// Grab the GameInfo class
		AGameInfo* GameInfoDefaultObject = NULL;
		UClass* GameInfoClass = FindObject<UClass>(ANY_PACKAGE, TEXT("GameInfo"));
		if(GameInfoClass != NULL)
		{
			GameInfoDefaultObject = GameInfoClass->GetDefaultObject<AGameInfo>();
		}

		// Iterate over the prefix mappings
		if (GameInfoDefaultObject != NULL)
		{
			INT DefaultCount = GameInfoDefaultObject->DefaultMapPrefixes.Num();
			INT CustomCount = GameInfoDefaultObject->CustomMapPrefixes.Num();
			INT TotalCount = DefaultCount + CustomCount;

			TMap<FString, TSet<FDependencyRef>> GameTypeToObjectDependenciesMap;
			TMap<FString, TArray<FString>> GameTypeToObjectLoadedMap;

			for (INT MapPrefixIdx = 0; MapPrefixIdx < TotalCount; MapPrefixIdx++)
			{
				FGameTypePrefix* GTPrefix = NULL;
				if (MapPrefixIdx < DefaultCount)
				{
					GTPrefix = &(GameInfoDefaultObject->DefaultMapPrefixes(MapPrefixIdx));
				}
				else
				{
					INT TempIdx = MapPrefixIdx - DefaultCount;
					GTPrefix = &(GameInfoDefaultObject->CustomMapPrefixes(TempIdx));
				}

				if (GTPrefix && (GTPrefix->bUsesCommonPackage == TRUE))
				{
					// Make the name
					FString CommonPackageName = GTPrefix->Prefix;
					CommonPackageName += TEXT("_Common");

					// Add each of the supported gameplay types as 'content'
					for (INT GameTypeIdx = -1; GameTypeIdx < GTPrefix->AdditionalGameTypes.Num(); GameTypeIdx++)
					{
						FString GameContentStr;
						if (GameTypeIdx == -1)
						{
							GameContentStr = GTPrefix->GameType;
						}
						else
						{
							GameContentStr = GTPrefix->AdditionalGameTypes(GameTypeIdx);
						}

						AddGameContentEntry(GameTypeCommonContentMap, *CommonPackageName, *GameContentStr, FALSE);
					}

					for (INT ForcedIdx = 0; ForcedIdx < GTPrefix->ForcedObjects.Num(); ForcedIdx++)
					{
						AddGameContentEntry(GameTypeCommonContentMap, *CommonPackageName, *GTPrefix->ForcedObjects(ForcedIdx), TRUE);
					}
				}
			}
		}
	}
}

/**
 *	Generates the list of common content for the given filename.
 *	This will be stored in both the GameTypeCommonContentMap content list for the prefix
 *	as well as the persistent cooker data list (for retrieval and usage in subsequent cooks)
 *
 *	@param	Commandlet		commandlet that is calling back
 *	@param	Filename		current filename that needs to be loaded for cooking
 *
 *	@return UBOOL			TRUE if successful (and the package should be created and cooked)
 *							FALSE if not (and the package should not be created and cooked)
 */
UBOOL FGenerateGameContentSeekfree::GenerateCommonPackageData(class UCookPackagesCommandlet* Commandlet, const TCHAR* Filename)
{
	UBOOL bSuccessful = FALSE;

	TArray<FString> GameTypesToLoad;
	TArray<FString> ForcedObjectsToLoad;

	FGameContentEntry* CurrContentEntry = NULL;
	FForceCookedInfo* CurrCookedPrefixCommonInfo = NULL;

	FString CheckFilename = FString(Filename).ToUpper();
	for (TMap<FString,FGameContentEntry>::TIterator GTIt(GameTypeCommonContentMap); GTIt; ++GTIt)
	{
		FGameContentEntry& ContentEntry = GTIt.Value();
		if (ContentEntry.PackageCookerInfo.SrcFilename == CheckFilename)
		{
			const FString& CommonName = GTIt.Key();

			//@todo. Handle DLC properly!
			CurrCookedPrefixCommonInfo = Commandlet->PersistentCookerData->GetCookedPrefixCommonInfo(CommonName, TRUE);
			CurrContentEntry = &ContentEntry;
			// We've found the entry that will be used to mark objects cooked...
			// Regenerate the lists here
			AGameInfo* GameInfoDefaultObject = NULL;
			UClass* GameInfoClass = FindObject<UClass>(ANY_PACKAGE, TEXT("GameInfo"));
			if(GameInfoClass != NULL)
			{
				GameInfoDefaultObject = GameInfoClass->GetDefaultObject<AGameInfo>();
			}

			// Iterate over the prefix mappings
			if (GameInfoDefaultObject != NULL)
			{
				INT DefaultCount = GameInfoDefaultObject->DefaultMapPrefixes.Num();
				INT CustomCount = GameInfoDefaultObject->CustomMapPrefixes.Num();
				INT TotalCount = DefaultCount + CustomCount;

				TMap<FString, TSet<FDependencyRef>> GameTypeToObjectDependenciesMap;
				TMap<FString, TArray<FString>> GameTypeToObjectLoadedMap;

				for (INT MapPrefixIdx = 0; MapPrefixIdx < TotalCount; MapPrefixIdx++)
				{
					FGameTypePrefix* GTPrefix = NULL;
					if (MapPrefixIdx < DefaultCount)
					{
						GTPrefix = &(GameInfoDefaultObject->DefaultMapPrefixes(MapPrefixIdx));
					}
					else
					{
						INT TempIdx = MapPrefixIdx - DefaultCount;
						GTPrefix = &(GameInfoDefaultObject->CustomMapPrefixes(TempIdx));
					}
					FString CheckPrefix = TEXT("\\");
					CheckPrefix += GTPrefix->Prefix;
					CheckPrefix += TEXT("_COMMON");
					if ((GTPrefix->bUsesCommonPackage == TRUE) && (CheckFilename.InStr(CheckPrefix, FALSE, TRUE) != INDEX_NONE))
					{
						GameTypesToLoad.AddUniqueItem(GTPrefix->GameType);
						for (INT AdditionalIdx = 0; AdditionalIdx < GTPrefix->AdditionalGameTypes.Num(); AdditionalIdx++)
						{
							GameTypesToLoad.AddUniqueItem(GTPrefix->AdditionalGameTypes(AdditionalIdx));
						}

						for (INT ForcedIdx = 0; ForcedIdx < GTPrefix->ForcedObjects.Num(); ForcedIdx++)
						{
							ForcedObjectsToLoad.AddUniqueItem(GTPrefix->ForcedObjects(ForcedIdx));
						}
						break;
					}
				}
			}
		}
	}

	// If there are any objects common to all the supported gametypes
	// or if there are any forced objects, but them into the prefix
	// common package.
	if ((GameTypesToLoad.Num() > 0) || (ForcedObjectsToLoad.Num() > 0))
	{
		if (CurrContentEntry != NULL)
		{
			TMap<FString,INT> CommonContentMap;
			TMap<FString,INT> ForcedContentMap;

			INT FoundGameTypeCount = GenerateLoadedObjectList(Commandlet, GameTypesToLoad, CommonContentMap);
			GenerateLoadedObjectList(Commandlet, ForcedObjectsToLoad, ForcedContentMap);

			// Reset the CurrContentEntry content list (ie remove the gametypes) and 
			// fill it with the actual content used to generate the package to cook...
			// Also, fill in the info stored in the GPCD for subsequent cooks
			CurrContentEntry->ContentList.Empty();
			CurrCookedPrefixCommonInfo->CookedContentList.Empty();


			// For 'common' objects, we only want ones that are in ALL of the loaded types
			for (TMap<FString,INT>::TIterator ObjIt(CommonContentMap); ObjIt; ++ObjIt)
			{
				FString ObjName = ObjIt.Key();
				INT ObjCount = ObjIt.Value();
				if (ObjCount == FoundGameTypeCount)
				{
					// This is a common object!!!
					CurrContentEntry->ContentList.AddItem(ObjName);
					ObjName = ObjName.ToUpper();
					UBOOL bFound = TRUE;
					CurrCookedPrefixCommonInfo->CookedContentList.Set(ObjName, bFound);
				}
			}

			// For forced objects, we want all of them
			for (TMap<FString,INT>::TIterator ForcedIt(ForcedContentMap); ForcedIt; ++ForcedIt)
			{
				FString ObjName = ForcedIt.Key();
				CurrContentEntry->ContentList.AddItem(ObjName);
				UBOOL bFound = TRUE;
				CurrCookedPrefixCommonInfo->CookedContentList.Set(ObjName, bFound);
			}

			bSuccessful = TRUE;
		}
	}

	return bSuccessful;
}

/** 
*	Returns true if a package is a GameContent package
*
*	@param InPackage			Package to check	
*/
UBOOL FGenerateGameContentSeekfree::IsPackageGameContent(UPackage* InPackage)
{
	FString ChoppedPackageName = InPackage->GetName();
	INT SFIndex = ChoppedPackageName.InStr(TEXT("_SF"));
	if (SFIndex != -1)
	{
		ChoppedPackageName = ChoppedPackageName.Left(SFIndex);
	}

	return GameContentMap.Find(ChoppedPackageName) != NULL;
}

/** 
*	Returns true if a package is a GameContent Common package, that is content shared between several GameContent packages
*
*	@param InPackage			Package to check
*/
UBOOL FGenerateGameContentSeekfree::IsPackageGameCommonContent(UPackage* InPackage)
{
	FString ChoppedPackageName = InPackage->GetName();
	INT SFIndex = ChoppedPackageName.InStr(TEXT("_SF"));
	if (SFIndex != -1)
	{
		ChoppedPackageName = ChoppedPackageName.Left(SFIndex);
	}

	return GameTypeCommonContentMap.Find(ChoppedPackageName) != NULL;
}

/**
 * Adds a unique entry for the given game type
 * Adds the content string for each game type given
 *
 * @param InGameStr - game string used as base for package filename
 * @param InContentStr - content to be loaded for the game type package
 */
void FGenerateGameContentSeekfree::AddGameContentEntry(TMap<FString,FGameContentEntry>& InContentMap, const TCHAR* InGameStr,const TCHAR* InContentStr, UBOOL bForced)
{
	if (!InGameStr || !InContentStr)
	{
		return;
	}

	FString GameStr = FString(InGameStr).ToUpper();
	FGameContentEntry* GameContentEntryPtr = InContentMap.Find(GameStr);
	if( GameContentEntryPtr == NULL)
	{
		FGameContentEntry GameContentEntry;
		InContentMap.Set(GameStr,GameContentEntry);
		GameContentEntryPtr = InContentMap.Find(GameStr);
	}
	check(GameContentEntryPtr);
	if (bForced == FALSE)
	{
		GameContentEntryPtr->ContentList.AddUniqueItem(FString(InContentStr).ToUpper());
	}
	else
	{
		GameContentEntryPtr->ForcedContentList.AddUniqueItem(FString(InContentStr).ToUpper());
	}
}

/**
 * @return FGenerateGameContentSeekfree	Singleton instance for game content seekfree helper class
 */
class FGenerateGameContentSeekfree* FGameCookerHelper::GetGameContentSeekfreeHelper()
{
	static FGenerateGameContentSeekfree MapPreloadHelper;
	return &MapPreloadHelper;
}

//-----------------------------------------------------------------------------
//	ForcedContentHelper
//-----------------------------------------------------------------------------
/**
 * Initialize options from command line options
 *
 *	@param	Tokens			Command line tokens parsed from app
 *	@param	Switches		Command line switches parsed from app
 */
void FForcedContentHelper::InitOptions(const TArray<FString>& Tokens, const TArray<FString>& Switches)
{
}

/**
 * Cooks passed in object if it hasn't been already.
 *
 *	@param	Commandlet					The cookpackages commandlet being run
 *	@param	Package						Package going to be saved
 *	@param	Object						Object to cook
 *	@param	bIsSavedInSeekFreePackage	Whether object is going to be saved into a seekfree package
 *
 *	@return	UBOOL						TRUE if the object should continue the 'normal' cooking operations.
 *										FALSE if the object should not be processed any further.
 */
UBOOL FForcedContentHelper::CookObject(class UCookPackagesCommandlet* Commandlet, UPackage* Package, UObject* Object, UBOOL bIsSavedInSeekFreePackage)
{
	SCOPE_SECONDS_COUNTER(ForcedContentTime);
	if ((bPMapForcedObjectsEnabled == TRUE) && (bPMapBeingProcessed == FALSE) && (CurrentPMapForcedObjectsList != NULL))
	{
		// See if the object being cooked is in the forced PMap list
		FString ObjectName = Object->GetPathName().ToUpper();
		if (CurrentPMapForcedObjectsList->CookedContentList.Find(ObjectName) != NULL)
		{
			MarkObjectAsCooked(Object);
			return FALSE;
		}
	}
	return TRUE;
}

/** 
 *	PreLoadPackageForCookingCallback
 *	This function will be called in LoadPackageForCooking, prior to the actual loading of the package.
 *	It is intended for pre-loading any required packages, etc.
 *
 *	@param	Commandlet		The cookpackages commandlet being run
 *	@param	Filename		The name of the package to load.
 *
 *	@return	UBOOL		TRUE if the package should be processed further.
 *						FALSE if the cook of this package should be aborted.
 */
UBOOL FForcedContentHelper::PreLoadPackageForCookingCallback(class UCookPackagesCommandlet* Commandlet, const TCHAR* Filename)
{
	SCOPE_SECONDS_COUNTER(ForcedContentTime);
	if (bPMapForcedObjectsEnabled == FALSE)
	{
		return TRUE;
	}

	FFilename LookupFilename = Filename;
	FString CheckName = LookupFilename.GetBaseFilename().ToUpper();
	FString PMapSource;
	if (Commandlet->PersistentMapInfoHelper.GetPersistentMapAlias(CheckName, PMapSource) == TRUE)
	{
		// Copy the alias in
		CheckName = PMapSource;
	}
	PMapBeingProcessed = TEXT("");
	if (Commandlet->PersistentMapInfoHelper.GetPersistentMapForLevel(CheckName, PMapSource) == TRUE)
	{
		// This will generate it the first time the PMap-list is requested
		FForceCookedInfo* ForceCookedInfo = GetPMapForcedObjectList(Commandlet, PMapSource, TRUE);
		if (ForceCookedInfo != NULL)
		{
			// We are cooking a map that has a PMap
			CurrentPMapForcedObjectsList = ForceCookedInfo;
			if (CheckName == PMapSource)
			{
				// It is the PMap itself!
				bPMapBeingProcessed = TRUE;
				PMapBeingProcessed = PMapSource;
			}
			else
			{
				bPMapBeingProcessed = FALSE;
			}
		}
		else
		{
			bPMapBeingProcessed = FALSE;
			CurrentPMapForcedObjectsList = NULL;
		}
	}
	else
	{
		bPMapBeingProcessed = FALSE;
		CurrentPMapForcedObjectsList = NULL;
	}

	return TRUE;
}

/**
 */
UBOOL FForcedContentHelper::PostLoadPackageForCookingCallback(class UCookPackagesCommandlet* Commandlet, UPackage* InPackage)
{
	SCOPE_SECONDS_COUNTER(ForcedContentTime);
	if ((bPMapForcedObjectsEnabled == TRUE) && (bPMapBeingProcessed == TRUE) && (CurrentPMapForcedObjectsList != NULL))
	{
		// Find the world info and add the object referencer!
		for (TObjectIterator<AWorldInfo> It; It; ++It)
		{
			AWorldInfo* WorldInfo = *It;
			if (WorldInfo->GetOutermost()->GetName().InStr(PMapBeingProcessed, FALSE, TRUE) != INDEX_NONE)
			{
				// This is the one we want
				UObjectReferencer* ObjReferencer = ConstructObject<UObjectReferencer>(UObjectReferencer::StaticClass(), WorldInfo);
				check(ObjReferencer);
				ObjReferencer->SetFlags(RF_Standalone);
				// Load each object in the forced list and add it to the object referencer.
				for (TMap<FString,UBOOL>::TIterator ContentIt(CurrentPMapForcedObjectsList->CookedContentList); ContentIt; ++ContentIt)
				{
					FString ContentStr = ContentIt.Key();
					// load the requested content object (typically a content class) 
					LoadObjectOrClass(Commandlet, ContentStr, ObjReferencer);
				}
				WorldInfo->PersistentMapForcedObjects = ObjReferencer;
				break;
			}
		}
	}
	return TRUE;
}

/**
 *	Called when the FPersistentMapInfo class has been initialized and filled in.
 *	Can be used to do any special setup for PMaps the cooker may require.
 *
 *	@param	Commandlet		The commandlet that is being run.
 */
void FForcedContentHelper::PersistentMapInfoGeneratedCallback(class UCookPackagesCommandlet* Commandlet)
{
	SCOPE_SECONDS_COUNTER(ForcedContentTime);
	// Force Map Objects
	if (bPMapForcedObjectsEnabled == TRUE)
	{
		// If object are being forced into the PMaps, then setup the lists here!
		TArray<FString> PMapList;
		Commandlet->PersistentMapInfoHelper.GetPersistentMapList(PMapList);

		// Iterate thru the list and 
		for (INT PMapIdx = 0; PMapIdx < PMapList.Num(); PMapIdx++)
		{
			FString PMapName = PMapList(PMapIdx);
			// find the game strings and content to be loaded for each
			FString PMapForcedIniSection = FString::Printf(TEXT("Cooker.ForcedMapObjects.%s"), *PMapName);
			FConfigSection* PMapForceObjectsMap = GConfig->GetSectionPrivate(*PMapForcedIniSection, FALSE, TRUE, GEditorIni);
			if (PMapForceObjectsMap != NULL)
			{
				// Split up the remotes and the pawns...
				TArray<FString> ForcedObjectsList;
				for (FConfigSectionMap::TIterator It(*PMapForceObjectsMap); It; ++It)
				{
					FString ObjectName = It.Value();
					ForcedObjectsList.AddUniqueItem(ObjectName);
				}

				if (ForcedObjectsList.Num() > 0)
				{
					TMap<FString,UBOOL>* ForcedObjectsMap = PMapForcedObjectsMap.Find(PMapName);
					if (ForcedObjectsMap == NULL)
					{
						TMap<FString,UBOOL> Temp;
						PMapForcedObjectsMap.Set(PMapName, Temp);
						ForcedObjectsMap = PMapForcedObjectsMap.Find(PMapName);
					}
					check(ForcedObjectsMap);
				
					for (INT ForcedIdx = 0; ForcedIdx < ForcedObjectsList.Num(); ForcedIdx++)
					{
						FString ForcedUpper = ForcedObjectsList(ForcedIdx).ToUpper();
						ForcedObjectsMap->Set(ForcedUpper, TRUE);
					}
				}
			}
		}
	}

}

/**
 *	Dump out stats specific to the game cooker helper.
 */
void FForcedContentHelper::DumpStats()
{
	warnf( NAME_Log, TEXT("") );
	warnf( NAME_Log, TEXT("  ForcedContent Stats:") );
	warnf( NAME_Log, TEXT("    Total time                             %7.2f seconds"), ForcedContentTime );
}

/**
 */
void FForcedContentHelper::Init()
{
	SCOPE_SECONDS_COUNTER(ForcedContentTime);
	GConfig->GetBool(TEXT("Cooker.ForcedMapObjects"), TEXT("bAllowPMapForcedObjects"), bPMapForcedObjectsEnabled, GEditorIni);
}

/**
 *	Retrieves the list of forced objects for the given PMap.
 *
 *	@param	Commandlet			commandlet that is calling back
 *	@param	InPMapName			the PMap to get the forced object list for
 *	@param	bCreateIfNotFound	if TRUE, generate the list for the given PMap
 *
 *	@return FForceCookedInfo*	The force cooked info for the given PMap
 *								NULL if not found (or not a valid PMap)
 */
FForceCookedInfo* FForcedContentHelper::GetPMapForcedObjectList(class UCookPackagesCommandlet* Commandlet, const FString& InPMapName, UBOOL bCreateIfNotFound)
{
	FForceCookedInfo* Result = NULL;
	// Generate the information if the PMap is a valid one
	FString UpperPMapName = InPMapName.ToUpper();
	TMap<FString,UBOOL>* ForcedObjectsMap = PMapForcedObjectsMap.Find(UpperPMapName);
	if (ForcedObjectsMap != NULL)
	{
		// We have a valid PMap w/ forced objects
		//@todo. We need to handle detecting that the object list stored is different than the current forced list.
		// This can be done by using the UBOOL flags stored in the ForceCokedInfo list, setting it to TRUE for
		// objects that are in the ForcedObject list, and FALSE to ones that are loaded as a side-effect.
		// For now we will simply assume that the list it correct if it is found.
		Result = Commandlet->PersistentCookerData->GetPMapForcedObjectInfo(InPMapName, FALSE);
		if ((Result == NULL) && (bCreateIfNotFound == TRUE))
		{
			Result = Commandlet->PersistentCookerData->GetPMapForcedObjectInfo(UpperPMapName, TRUE);

			// Create a list of objects to load (ie, the forced objects)
			TArray<FString> ForceObjectList;
			for (TMap<FString,UBOOL>::TIterator ForceObjIt(*ForcedObjectsMap); ForceObjIt; ++ForceObjIt)
			{
				ForceObjectList.AddUniqueItem(ForceObjIt.Key());
			}

			TMap<FString,INT> LoadedObjectMap;
			INT LoadedObjCount = GenerateLoadedObjectList(Commandlet, ForceObjectList, LoadedObjectMap);
			if (LoadedObjCount > 0)
			{
				// Add these to the PCD list
				for (TMap<FString,INT>::TIterator AddIt(LoadedObjectMap); AddIt; ++AddIt)
				{
					// We don't care about the counts for forced objects
					FString ObjectName = AddIt.Key();
					ObjectName = ObjectName.ToUpper();
					Result->CookedContentList.Set(ObjectName, TRUE);
				}
			}
		}
	}
	return Result;
}

/**
 */
class FForcedContentHelper* FGameCookerHelper::GetForcedContentHelper()
{
	static FForcedContentHelper ForcedContentHelper;
	return &ForcedContentHelper;
}

//-----------------------------------------------------------------------------
//	GameCookerHelper
//-----------------------------------------------------------------------------

/**
* Initialize the cooker helpr and process any command line params
*
*	@param	Commandlet		The cookpackages commandlet being run
*	@param	Tokens			Command line tokens parsed from app
*	@param	Switches		Command line switches parsed from app
*/
void FGameCookerHelper::Init(
	class UCookPackagesCommandlet* Commandlet, 
	const TArray<FString>& Tokens, 
	const TArray<FString>& Switches )
{
	GetGameContentSeekfreeHelper()->InitOptions(Tokens,Switches);
	GetForcedContentHelper()->InitOptions(Tokens,Switches);
}

/**
 *	Create an instance of the persistent cooker data given a filename. 
 *	First try to load from disk and if not found will construct object and store the 
 *	filename for later use during saving.
 *
 *	The cooker will call this first, and if it returns NULL, it will use the standard
 *		UPersistentCookerData::CreateInstance function. 
 *	(They are static hence the need for this)
 *
 * @param	Filename					Filename to use for serialization
 * @param	bCreateIfNotFoundOnDisk		If FALSE, don't create if couldn't be found; return NULL.
 * @return								instance of the container associated with the filename
 */
UPersistentCookerData* FGameCookerHelper::CreateInstance( const TCHAR* Filename, UBOOL bCreateIfNotFoundOnDisk )
{
	return FGameCookerHelperBase::CreateInstance(Filename,bCreateIfNotFoundOnDisk);
}

/** 
 *	Generate the package list that is specific for the game being cooked.
 *
 *	@param	Commandlet							The cookpackages commandlet being run
 *	@param	Platform							The platform being cooked for
 *	@param	ShaderPlatform						The shader platform being cooked for
 *	@param	NotRequiredFilenamePairs			The package lists being filled in...
 *	@param	RegularFilenamePairs				""
 *	@param	MapFilenamePairs					""
 *	@param	ScriptFilenamePairs					""
 *	@param	StartupFilenamePairs				""
 *	@param	StandaloneSeekfreeFilenamePairs		""
 *	
 *	@return	UBOOL		TRUE if successful, FALSE is something went wrong.
 */
UBOOL FGameCookerHelper::GeneratePackageList( 
	class UCookPackagesCommandlet* Commandlet, 
	UE3::EPlatformType Platform,
	EShaderPlatform ShaderPlatform,
	TArray<FPackageCookerInfo>& NotRequiredFilenamePairs,
	TArray<FPackageCookerInfo>& RegularFilenamePairs,
	TArray<FPackageCookerInfo>& MapFilenamePairs,
	TArray<FPackageCookerInfo>& ScriptFilenamePairs,
	TArray<FPackageCookerInfo>& StartupFilenamePairs,
	TArray<FPackageCookerInfo>& StandaloneSeekfreeFilenamePairs)
{
	UBOOL bSuccess=TRUE;

	// Add to list of seekfree packages needed for standalon game content
	if (GetGameContentSeekfreeHelper()->GeneratePackageList(
		Commandlet,
		Platform,
		ShaderPlatform,
		NotRequiredFilenamePairs,
		RegularFilenamePairs,
		MapFilenamePairs,
		ScriptFilenamePairs,
		StartupFilenamePairs,
		StandaloneSeekfreeFilenamePairs
		) == FALSE)
	{
		bSuccess = FALSE;
	}

	return bSuccess;
}

/**
 * Cooks passed in object if it hasn't been already.
 *
 *	@param	Commandlet					The cookpackages commandlet being run
 *	@param	Package						Package going to be saved
 *	@param	Object						Object to cook
 *	@param	bIsSavedInSeekFreePackage	Whether object is going to be saved into a seekfree package
 *
 *	@return	UBOOL						TRUE if the object should continue the 'normal' cooking operations.
 *										FALSE if the object should not be processed any further.
 */
UBOOL FGameCookerHelper::CookObject(class UCookPackagesCommandlet* Commandlet, UPackage* Package, UObject* Object, UBOOL bIsSavedInSeekFreePackage)
{
	UBOOL bContinueProcessing = TRUE;

	// pre process loaded package for standalone seekfree game content
	if (GetGameContentSeekfreeHelper()->CookObject(Commandlet,Package,Object,bIsSavedInSeekFreePackage) == FALSE)
	{
		bContinueProcessing = FALSE;
	}

	if (bContinueProcessing && (GetForcedContentHelper()->CookObject(Commandlet,Package,Object,bIsSavedInSeekFreePackage) == FALSE))
	{
		bContinueProcessing = FALSE;
	}

	return bContinueProcessing;
}

/** 
 *	PreLoadPackageForCookingCallback
 *	This function will be called in LoadPackageForCooking, prior to the actual loading of the package.
 *	It is intended for pre-loading any required packages, etc.
 *
 *	@param	Commandlet		The cookpackages commandlet being run
 *	@param	Filename		The name of the package to load.
 *
 *	@return	UBOOL		TRUE if the package should be processed further.
 *						FALSE if the cook of this package should be aborted.
 */
UBOOL FGameCookerHelper::PreLoadPackageForCookingCallback(class UCookPackagesCommandlet* Commandlet, const TCHAR* Filename)
{
	UBOOL bContinueProcessing = TRUE;

	// pre process loaded package for standalone seekfree game content
	if (GetGameContentSeekfreeHelper()->PreLoadPackageForCookingCallback(Commandlet,Filename) == FALSE)
	{
		bContinueProcessing = FALSE;
	}

	if (bContinueProcessing && (GetForcedContentHelper()->PreLoadPackageForCookingCallback(Commandlet,Filename) == FALSE))
	{
		bContinueProcessing = FALSE;
	}

	return bContinueProcessing;
}

/** 
 *	LoadPackageForCookingCallback
 *	This function will be called in LoadPackageForCooking, allowing the cooker
 *	helper to handle the package creation as they wish.
 *
 *	@param	Commandlet		The cookpackages commandlet being run
 *	@param	Filename		The name of the package to load.
 *
 *	@return	UPackage*		The package generated/loaded
 *							NULL if the commandlet should load the package normally.
 */
UPackage* FGameCookerHelper::LoadPackageForCookingCallback(class UCookPackagesCommandlet* Commandlet, const TCHAR* Filename)
{
	UPackage* Result=NULL;
	
	// load package for standalone seekfree game content for the given filename
	Result = GetGameContentSeekfreeHelper()->LoadPackageForCookingCallback(Commandlet,Filename);

	return Result;
}

/** 
 *	PostLoadPackageForCookingCallback
 *	This function will be called in LoadPackageForCooking, prior to any
 *	operations occurring on the contents...
 *
 *	@param	Commandlet	The cookpackages commandlet being run
 *	@param	Package		The package just loaded.
 *
 *	@return	UBOOL		TRUE if the package should be processed further.
 *						FALSE if the cook of this package should be aborted.
 */
UBOOL FGameCookerHelper::PostLoadPackageForCookingCallback(class UCookPackagesCommandlet* Commandlet, UPackage* InPackage)
{
	UBOOL bContinueProcessing = TRUE;

	// post process loaded package for standalone seekfree game content
	if (GetGameContentSeekfreeHelper()->PostLoadPackageForCookingCallback(Commandlet,InPackage) == FALSE)
	{
		bContinueProcessing = FALSE;
	}

	if (bContinueProcessing && (GetForcedContentHelper()->PostLoadPackageForCookingCallback(Commandlet,InPackage) == FALSE))
	{
		bContinueProcessing = FALSE;
	}

	return bContinueProcessing;
}

/**
 *	Clean up the kismet for the given level...
 *	Remove 'danglers' - sequences that don't actually hook up to anything, etc.
 *
 *	@param	Commandlet	The cookpackages commandlet being run
 *	@param	Package		The package being cooked.
 */
void FGameCookerHelper::CleanupKismet(class UCookPackagesCommandlet* Commandlet, UPackage* InPackage)
{
}

/**
 *	Return TRUE if the sound cue should be ignored when generating persistent FaceFX list.
 *
 *	@param	Commandlet		The commandlet being run
 *	@param	InSoundCue		The sound cue of interest
 *
 *	@return	UBOOL			TRUE if the sound cue should be ignored, FALSE if not
 */
UBOOL FGameCookerHelper::ShouldSoundCueBeIgnoredForPersistentFaceFX(class UCookPackagesCommandlet* Commandlet, const USoundCue* InSoundCue)
{
	return FALSE;
}

/**
 *	Called when the FPersistentMapInfo class has been initialized and filled in.
 *	Can be used to do any special setup for PMaps the cooker may require.
 *
 *	@param	Commandlet		The commandlet that is being run.
 */
void FGameCookerHelper::PersistentMapInfoGeneratedCallback(class UCookPackagesCommandlet* Commandlet)
{
	// Allow the seekfree helper to do what it needs
	GetGameContentSeekfreeHelper()->PersistentMapInfoGeneratedCallback(Commandlet);
	GetForcedContentHelper()->PersistentMapInfoGeneratedCallback(Commandlet);
}

/**
 *	Initialize any special information for a cooked shader cache, such as priority
 *
 *	@param	Commandlet			The cookpackages commandlet being run
 *	@param	Package				The package being cooked.
 *	@param	ShaderCache			Shader cache object that is being initialize
 *	@param	IsStartupPackage	If true, this is a package that is loaded on startup
 */
void FGameCookerHelper::InitializeShaderCache(class UCookPackagesCommandlet* Commandlet, UPackage* InPackage, UShaderCache *InShaderCache, UBOOL bIsStartupPackage)
{	
	int Priority = -1;

	if (bIsStartupPackage)
	{
		// Startup packages get best priority
		Priority = 0;
	}

	if (Priority == -1)
	{
		// Check if we're a game content common package
		if (GetGameContentSeekfreeHelper()->IsPackageGameCommonContent(InPackage))
		{
			Priority = 1;
		}
	}

	if (Priority == -1)
	{
		// Check if we're a game content common package
		if (GetGameContentSeekfreeHelper()->IsPackageGameContent(InPackage))
		{
			Priority = 2;
		}
	}

	if (Priority == -1)
	{	
		FString CheckName = InPackage->GetName();
		INT SFIndex = CheckName.InStr(TEXT("_SF"));
		if (SFIndex != -1)
		{
			CheckName = CheckName.Left(SFIndex);
		}
		
		FString PMapSource;
		if (Commandlet->PersistentMapInfoHelper.GetPersistentMapAlias(CheckName, PMapSource) == TRUE)
		{
			// Copy the alias in
			CheckName = PMapSource;
		}	
		if (Commandlet->PersistentMapInfoHelper.GetPersistentMapForLevel(CheckName, PMapSource) == TRUE)
		{
			if (CheckName == PMapSource)
			{
				// We're a persistent map
				Priority = 3;
			}
		}
	}

	if (Priority == -1)
	{
		// Default priority of 10
		Priority = 10;
	}
	
	InShaderCache->SetPriority(Priority);
}

/**
 *	Dump out stats specific to the game cooker helper.
 */
void FGameCookerHelper::DumpStats()
{
	warnf( NAME_Log, TEXT("") );
	warnf( NAME_Log, TEXT("Game-specific Cooking Stats:") );
	GetGameContentSeekfreeHelper()->DumpStats();
	GetForcedContentHelper()->DumpStats();
}
