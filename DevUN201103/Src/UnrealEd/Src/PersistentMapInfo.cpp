/**
 *	This is a helper class - PersistentMapInfo
 *	It creates a maintains lists of:
 *		PMaps-->Levels
 *		Levels-->PMap that 'owns' them
 *	This is used during cooking to generate persistent facefx sets,
 *	as well as 
 * 
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "UnrealEd.h"

/**
 *	Generate the mappings of sub-levels to PMaps... (LevelToPersistentLevelMap)
 *
 *	@param	MapList			The array of persistent map names to analyze
 *	@param	bClearExisting	If TRUE, clear all existing entries
 *	@param	bPMapsOnly		If TRUE, the list contains ONLY PMaps
 *							If FALSE, the list can contain any maps (ie sub-levels of PMaps)
 */
void FPersistentMapInfo::GeneratePersistentMapList(const TArray<FString>& MapList, UBOOL bClearExisting, UBOOL bPMapsOnly)
{
	if (bClearExisting == TRUE)
	{
		if (GenerationVerboseLevel != VL_Silent)
		{
			warnf(NAME_Log, TEXT("GeneratePersistentMapList> Clearing existing lists"));
		}
		LevelToPersistentLevelMap.Empty();
		PersistentLevelToContainedLevelsMap.Empty();
		PMapAliasMap.Empty();
	}

	// Load the PMap Aliases first
	FString PMapAliasSectionName = TEXT("Cooker.PMapAliases");
	FConfigSectionMap* ConfigPMapAliasMap = GConfig->GetSectionPrivate(*PMapAliasSectionName, FALSE, TRUE, GEditorIni);
	if (ConfigPMapAliasMap != NULL)
	{
		for (FConfigSectionMap::TIterator It(*ConfigPMapAliasMap); It; ++It)
		{
			FString PMapName = It.Key().ToString();
			FString AliasName = It.Value();

			PMapName = PMapName.ToUpper();
			AliasName = AliasName.ToUpper();

			if (GenerationVerboseLevel == VL_Verbose)
			{
				warnf(NAME_Log, TEXT("GeneratePersistentMapList> Setting PMap alias for %s to %s"), *PMapName, *AliasName);
			}
			PMapAliasMap.Set(PMapName, AliasName);
		}
	}

	// Cull out a list of PMaps (or 'root' maps for MP ones)
	// A mapping of MapName to list of maps that contain it in their StreamingLevels list
	TMap<FString,TArray<FString>> LevelToStreamingLevelsMap;
	TMap<FString, TArray<FString>> LevelToContainedInLevelsMap;

	for (INT MapIndex = 0; MapIndex < MapList.Num(); MapIndex++)
	{
		FFilename MapPackageFile = MapList(MapIndex);
		FString UpperMapName = MapPackageFile.GetBaseFilename().ToUpper();
		if (PMapAliasMap.Find(UpperMapName) == NULL)
		{
			FString FullMapName;
			if (GPackageFileCache->FindPackageFile(*UpperMapName, NULL, FullMapName))
			{
				FArchive* CheckMapPackageFile = GFileManager->CreateFileReader(*FullMapName);
				if (CheckMapPackageFile)
				{
					// read the package summary, which has list of sub levels
					FPackageFileSummary Summary;
					(*CheckMapPackageFile) << Summary;
					// close the map
					delete CheckMapPackageFile;
					// Make sure that it is a map!
					if ((Summary.PackageFlags & PKG_ContainsMap) != 0)
					{
						// if it's an old map, then we have to load it to get the list of sublevels
						if (Summary.GetFileVersion() < VER_ADDITIONAL_COOK_PACKAGE_SUMMARY)
						{
							warnf(NAME_Log, TEXT("  Old map, SKIPPING %s"), *MapPackageFile);
						}
						else
						{
							// Add EVERYTHING to the LevelToStreamingLevels map
							TArray<FString>* InsertCheck = LevelToStreamingLevelsMap.Find(UpperMapName);
							if (InsertCheck == NULL)
							{
								TArray<FString> Temp;
								LevelToStreamingLevelsMap.Set(UpperMapName, Temp);
							}

							// Streaming levels present?
							for (INT AdditionalPackageIndex = 0; AdditionalPackageIndex < Summary.AdditionalPackagesToCook.Num(); AdditionalPackageIndex++)
							{
								FString ContainedLevelName;
								if (GPackageFileCache->FindPackageFile(*Summary.AdditionalPackagesToCook(AdditionalPackageIndex), NULL, ContainedLevelName))
								{
									FArchive* AdditionalPackageFile = GFileManager->CreateFileReader(*ContainedLevelName);
									if (AdditionalPackageFile)
									{
										// read the package summary, which has list of sub levels
										FPackageFileSummary AdditionalSummary;
										(*AdditionalPackageFile) << AdditionalSummary;
										// close the map
										delete AdditionalPackageFile;

										if ((AdditionalSummary.PackageFlags & PKG_ContainsMap) != 0)
										{
											FFilename ContainedFileName = ContainedLevelName;
											FString ContainedName = ContainedFileName.GetBaseFilename().ToUpper();
											// Add it to the streaming levels for this map
											TArray<FString>* StreamingLevels = LevelToStreamingLevelsMap.Find(UpperMapName);
											// The source map should have already been added above!!!
											check(StreamingLevels);
											StreamingLevels->AddUniqueItem(ContainedName);

											TArray<FString>* ContainedInLevels = LevelToContainedInLevelsMap.Find(ContainedName);
											if (ContainedInLevels == NULL)
											{
												TArray<FString> Temp;
												LevelToContainedInLevelsMap.Set(ContainedName, Temp);
												ContainedInLevels = LevelToContainedInLevelsMap.Find(ContainedName);
											}
											check(ContainedInLevels);
											FString UpperContained = MapPackageFile.ToUpper();
											ContainedInLevels->AddUniqueItem(UpperContained);
										}
									}
								}
							}
						}
					}
				}
			}
			else
			{
				warnf(NAME_Log, TEXT("Failed to find package %s"), *UpperMapName);
			}
		}
		else
		{
			if (GenerationVerboseLevel == VL_Verbose)
			{
				warnf(NAME_Log, TEXT("GeneratePersistentMapList> Skipping aliased PMap %s"), *UpperMapName);
			}
		}
	}

	// Clean up the LevelToStreamingLevelsMap
	if (GenerationVerboseLevel == VL_Verbose)
	{
		debugf(TEXT("--- Cleaning up LevelToStreamingLevelsMap"));
	}
	TArray<FString> RemoveList;
	for (TMap<FString,TArray<FString>>::TIterator RemoveIt(LevelToStreamingLevelsMap); RemoveIt; ++RemoveIt)
	{
		FString SourceLevel = RemoveIt.Key();
		FFilename SourceFilename = SourceLevel;
		FString FindName = SourceFilename.GetBaseFilename().ToUpper();
		if (LevelToContainedInLevelsMap.Find(FindName) != NULL)
		{
			// It is contained in another level, so remove it
			RemoveList.AddUniqueItem(SourceLevel);
		}
	}
	for (INT RemoveIdx = 0; RemoveIdx < RemoveList.Num(); RemoveIdx++)
	{
		if (GenerationVerboseLevel == VL_Verbose)
		{
			debugf(TEXT("\tRemoving %s"), *(RemoveList(RemoveIdx)));
		}
		LevelToStreamingLevelsMap.Remove(RemoveList(RemoveIdx));
	}

	// Dump Results
	if (GenerationVerboseLevel == VL_Verbose)
	{
		// This block can be enabled to find sub-levels that are contained in multiple P-Maps
// 		debugf(TEXT("---- Level to contained in levels dump"));
// 		for (TMap<FString, TArray<FString>>::TIterator It(LevelToContainedInLevelsMap); It; ++It)
// 		{
// 			FString CheckLevel = It.Key();
// 			TArray<FString>& ContainedInList = It.Value();
// 
// 			debugf(TEXT("Checking level %s"), *CheckLevel);
// 			debugf(TEXT("\tContained in %2d levels"), ContainedInList.Num());
// 			for (INT LevelIdx = 0; LevelIdx < ContainedInList.Num(); LevelIdx++)
// 			{
// 				FString ContainerLevel = ContainedInList(LevelIdx);
// 				debugf(TEXT("\t%s"), *ContainerLevel);
// 				// If the map is in more than 1 level, then check if the levels it is
// 				// contained in are themselves contained in a level. This should be
// 				// the overall p-map.
// 			}
// 		}

		debugf(TEXT("---- Level to streaming levels dump"));
		for (TMap<FString,TArray<FString>>::TIterator DumpIt(LevelToStreamingLevelsMap); DumpIt; ++DumpIt)
		{
			FString SourceLevel = DumpIt.Key();
			TArray<FString>& StreamingLevels = DumpIt.Value();
			debugf(TEXT("PMap: %s"), *SourceLevel);
			for (INT DumpIdx = 0; DumpIdx < StreamingLevels.Num(); DumpIdx++)
			{
				debugf(TEXT("\t%2d - %s"), DumpIdx, *(StreamingLevels(DumpIdx)));
			}
		}
	}

	// Now, narrow down maps --> pmaps
	for (TMap<FString,TArray<FString>>::TIterator FixupIt(LevelToStreamingLevelsMap); FixupIt; ++FixupIt)
	{
		FString SourceLevel = FixupIt.Key();
		TArray<FString>& StreamingLevels = FixupIt.Value();

		// See if the source level is contained in the LevelToContainInLevelsMap
		FFilename SourceFilename = SourceLevel;
		FString FindName = SourceFilename.GetBaseFilename().ToUpper();

		TArray<FString>* LevelToContainedLevels = LevelToContainedInLevelsMap.Find(FindName);
		if (LevelToContainedLevels != NULL)
		{
			// We need to move all its sublevels to the map it is contained in,
			// then remove it from the LevelToStreamingLevelsMap
			if (GenerationVerboseLevel == VL_Verbose)
			{
				debugf(TEXT("Found potential P-map contained in another level!"));
				debugf(TEXT("\tPotential map: %s"), *SourceFilename);
				for (INT DumpIdx = 0; DumpIdx < LevelToContainedLevels->Num(); DumpIdx++)
				{
					FString ContainedIn = (*LevelToContainedLevels)(DumpIdx);
					debugf(TEXT("\tFound in map: %s"), *ContainedIn);
				}
			}
			checkf(0, TEXT("This needs to be handled!!!"));
		}
	}

	// create the final list
	for (TMap<FString,TArray<FString>>::TIterator FinalIt(LevelToStreamingLevelsMap); FinalIt; ++FinalIt)
	{
		FString SourceLevel = FinalIt.Key();

		TArray<FString>* FinalContainedList = PersistentLevelToContainedLevelsMap.Find(SourceLevel);
		if (FinalContainedList == NULL)
		{
			TArray<FString> Temp;
			PersistentLevelToContainedLevelsMap.Set(SourceLevel, Temp);
			FinalContainedList = PersistentLevelToContainedLevelsMap.Find(SourceLevel);
		}
		check(FinalContainedList);

		// Add it as itself, just to eliminate the need to handle that case
		LevelToPersistentLevelMap.Set(SourceLevel, SourceLevel);

		TArray<FString>& StreamingLevels = FinalIt.Value();
		for (INT FinalIdx = 0; FinalIdx < StreamingLevels.Num(); FinalIdx++)
		{
			FFilename StreamingName = StreamingLevels(FinalIdx);
			FString StreamingNameUpper = StreamingName.GetBaseFilename().ToUpper();
			LevelToPersistentLevelMap.Set(StreamingNameUpper, SourceLevel);
			FinalContainedList->AddUniqueItem(StreamingNameUpper);
		}
	}

	if (GenerationVerboseLevel != VL_Silent)
	{
		warnf(NAME_Log, TEXT("\tThere are %d levels to process"), LevelToPersistentLevelMap.Num());
	}

	if (GenerationVerboseLevel == VL_Verbose)
	{
// 		warnf(NAME_Log, TEXT("\t..... Level to PMap list ....."));
// 		for (TMap<FString, FString>::TIterator L2PMapIt(LevelToPersistentLevelMap); L2PMapIt; ++L2PMapIt)
// 		{
// 			warnf(NAME_Log, TEXT("\t\t%32s........%s"), *(L2PMapIt.Key()), *(L2PMapIt.Value()));
// 		}
// 		warnf(NAME_Log, TEXT("\t..... End Level to PMap list ....."));

		warnf(NAME_Log, TEXT("\t..... PMap to contained levels list ....."));
		for (TMap<FString, TArray<FString>>::TIterator P2ContainedIt(PersistentLevelToContainedLevelsMap); P2ContainedIt; ++P2ContainedIt)
		{
			FString PMapName = P2ContainedIt.Key();
			warnf(NAME_Log, TEXT("\t\t%s"), *PMapName);
			TArray<FString>& ContainedList = P2ContainedIt.Value();
			for (INT ContainedIdx = 0; ContainedIdx < ContainedList.Num(); ContainedIdx++)
			{
				warnf(NAME_Log, TEXT("\t\t\t%s"), *(ContainedList(ContainedIdx)));
			}
		}
		warnf(NAME_Log, TEXT("\t..... End PMap to contained levels list ....."));
	}
}

/** Set Caller class information */
void FPersistentMapInfo::SetCallerInfo(UCommandlet* InCommandlet, _GarbageCollectFn InGarbageCollectFn)
{
	CallerCommandlet = InCommandlet;
	GarbageCollectFn = InGarbageCollectFn;
}

/**
 *	Return the PMap name for the given level.
 *
 *	@param	InLevelName		The level to retrieve the name of the PMap that contains it
 *	@param	OutPMapName		The resulting PMap name that contains the given level
 *
 *	@return	UBOOL			TRUE if found, FALSE if not
 */
UBOOL FPersistentMapInfo::GetPersistentMapForLevel(const FString& InLevelName, FString& OutPMapName) const
{
	FFilename TempLevelName = InLevelName;
	FString UpperLevelName = TempLevelName.GetBaseFilename().ToUpper();
	const FString* PMapName = LevelToPersistentLevelMap.Find(UpperLevelName);
	if (PMapName != NULL)
	{
		OutPMapName = *PMapName;
		return TRUE;
	}

	OutPMapName = InLevelName;

	return FALSE;
}

/**
 *	Get the list of contained levels in a given persistent level
 *
 *	@param	InPMapName			The name of the persistent level
 *
 *	@return	TArray<FString>*	The contained levels if found, NULL if not
 */
const TArray<FString>* FPersistentMapInfo::GetPersistentMapContainedLevelsList(const FString& InPMapName) const
{
	FFilename PMapFilename = InPMapName;
	FString UpperPMapName = PMapFilename.GetBaseFilename().ToUpper();
	FString AliasCheck;
	if (GetPersistentMapAlias(UpperPMapName, AliasCheck) == TRUE)
	{
		UpperPMapName = AliasCheck;
	}

	return PersistentLevelToContainedLevelsMap.Find(UpperPMapName);
}

/**
 *	Get a list of PMaps
 */
UBOOL FPersistentMapInfo::GetPersistentMapList(TArray<FString>& OutPMapList)
{
	OutPMapList.Empty();
	for (TMap<FString,TArray<FString>>::TIterator It(PersistentLevelToContainedLevelsMap); It; ++It)
	{
		OutPMapList.AddUniqueItem(It.Key());
	}
	return TRUE;
}

/**
 *	Get the alias PMap for a level
 */
UBOOL FPersistentMapInfo::GetPersistentMapAlias(const FString& InLevelName, FString& OutPMapAlias) const
{
	FFilename CheckFilename = InLevelName;
	FString CheckName = CheckFilename.GetBaseFilename().ToUpper();
	const FString* AliasName = PMapAliasMap.Find(CheckName);
	if (AliasName != NULL)
	{
		OutPMapAlias = *AliasName;
		return TRUE;
	}
	return FALSE;
}