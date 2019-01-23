/**
 * Implementation file for all UDK datastore classes.
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
#include "UDKBase.h"

IMPLEMENT_CLASS(UUDKUIDataProvider_MapInfo);

//////////////////////////////////////////////////////////////////////////
// UUDKUIDataProvider_MenuOption
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIDataProvider_MenuOption);

/** @return 	TRUE if this menu option's configuration isn't compatible with the desired game settings  */
UBOOL UUDKUIDataProvider_MenuOption::IsFiltered()
{
	UBOOL bFiltered = Super::IsFiltered();

	if ( !bFiltered )
	{
		UDataStoreClient* DataStoreClient = UUIInteraction::GetDataStoreClient();
		if (DataStoreClient != NULL)
		{
			UUIDataStore_Registry* Registry = Cast<UUIDataStore_Registry>(DataStoreClient->FindDataStore(TEXT("Registry")));
			if(Registry)
			{
				FUIProviderFieldValue OutFieldValue(EC_EventParm);

				if ( Registry->GetDataStoreValue(TEXT("SelectedGameMode"), OutFieldValue ))
				{
					bFiltered = (RequiredGameMode != NAME_None && RequiredGameMode != *OutFieldValue.StringValue);
				}

				if ( !bFiltered && Registry->GetDataStoreValue(TEXT("StandaloneGame"), OutFieldValue) )
				{
								// bOnlineOnly option
					bFiltered = (bOnlineOnly && OutFieldValue.StringValue == TEXT("1"))
								// bOfflineOnly option
							||	(bOfflineOnly && OutFieldValue.StringValue == TEXT("0"));
				}
			}
		}

		// If we are the german version, remove gore option
		if( (GForceLowGore) && (GetName()==TEXT("GoreLevel")) )
		{
			bFiltered=TRUE;
		}

#if PS3
		// If this menu option corresponds to something that is only relevant for keyboard/mouse (i.e. mouse sensitivity, etc.),
		// filter it out if we don't have a keyboard or mouse
		if ( bKeyboardOrMouseOption )
		{
			if(GEngine && GEngine->GamePlayers.Num() && GEngine->GamePlayers(0))
			{
				AUDKPlayerController* PC = Cast<AUDKPlayerController>(GEngine->GamePlayers(0)->Actor);

				if(PC != NULL)
				{
					if(PC->IsKeyboardAvailable()==FALSE && PC->IsMouseAvailable()==FALSE)
					{
						bFiltered=TRUE;
					}
				}
			}
		}
#endif
	}

	return bFiltered || Super::IsFiltered();
}


//////////////////////////////////////////////////////////////////////////
// UUDKUIResourceDataProvider
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIResourceDataProvider);

/** @return Whether or not we should be filtered out of the list results. */
UBOOL UUDKUIResourceDataProvider::IsFiltered()
{
#if !CONSOLE
	return bRemoveOnPC || eventShouldBeFiltered();
#elif XBOX
	return bRemoveOn360 || eventShouldBeFiltered();
#elif PS3
	return bRemoveOnPS3 || eventShouldBeFiltered();
#elif MOBILE
	// @todo mobile: Add specific removal flags for mobile?
	return bRemoveOnPC || eventShouldBeFiltered();
#else
#error Please define your platform.
#endif
}

//////////////////////////////////////////////////////////////////////////
// UUDKUIDataStore_StringAliasMap
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIDataStore_StringAliasMap);

/**
 * Set MappedString to be the localized string using the FieldName as a key
 * Returns the index into the mapped string array of where it was found.
 */
INT UUDKUIDataStore_StringAliasMap::GetStringWithFieldName( const FString& FieldName, FString& MappedString )
{
	INT FieldIdx = INDEX_NONE;
	FString FinalFieldName = FieldName;

#if PS3
	// Swap accept and cancel on PS3 if we need to, this is a TRC requirement.
	if(appPS3UseCircleToAccept())
	{
		if(FinalFieldName==TEXT("Accept"))
		{
			FinalFieldName=TEXT("Cancel");
		}
		else if(FinalFieldName==TEXT("Cancel"))
		{
			FinalFieldName=TEXT("Accept");
		}
	}
#endif

	// Try to find platform specific versions first
	FString SetName;
#if XBOX
	SetName = TEXT("360");
#elif PS3
	SetName = TEXT("PS3");
#else
	switch ( FakePlatform )
	{
		case 1: SetName = TEXT("360"); break;
		case 2: SetName = TEXT("PS3"); break;
		default: SetName = TEXT("PC"); break;
	}
#endif
	FieldIdx = FindMappingWithFieldName(FinalFieldName, SetName);

	if(FieldIdx == INDEX_NONE)
	{
		FieldIdx = FindMappingWithFieldName(FinalFieldName);
	}

	if(FieldIdx == INDEX_NONE)
	{
		FieldIdx = FindMappingWithFieldName();
	}

	if(FieldIdx != INDEX_NONE)
	{
		MappedString = MenuInputMapArray(FieldIdx).MappedText;
	}

	return FieldIdx;
}

//////////////////////////////////////////////////////////////////////////
// UUDKUIDataStore_StringAliasBindingMap
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIDataStore_StringAliasBindingMap);

//Clear the command to input keybinding cache
void UUDKUIDataStore_StringAliasBindingMap::ClearBoundKeyCache()
{
	CommandToBindNames.Empty();
}

// Given an input command of the form GBA_ return the mapped keybinding string 
// Returns TRUE if it exists, FALSE otherwise
UBOOL UUDKUIDataStore_StringAliasBindingMap::FindMappingInBoundKeyCache(const FString& Command, FString& MappingStr, INT& FieldIndex)
{
	const FName Key(*Command);
	// Does the data already exist
	const FBindCacheElement* CacheElement = CommandToBindNames.Find(Key);
	if (CacheElement != NULL)
	{
		MappingStr = CacheElement->MappingString;
		FieldIndex = CacheElement->FieldIndex;
	}

	return (CacheElement != NULL);
}

//Given a input command of the form GBA_ and its mapping store that in a lookup for future use
void UUDKUIDataStore_StringAliasBindingMap::AddMappingToBoundKeyCache(const FString& Command, const FString& MappingStr, const INT FieldIndex)
{
	const FName Key(*Command);

	// Does the data already exist
	const FBindCacheElement* CacheElement = CommandToBindNames.Find(Key);

	if (CacheElement == NULL)
	{
		// Initialize a new FBindCacheElement.  It contains a FStringNoInit, so it needs to be initialized to zero.
		FBindCacheElement NewElement;
		appMemzero(&NewElement,sizeof(NewElement));

		NewElement.KeyName = Key;
		NewElement.MappingString = MappingStr;
		NewElement.FieldIndex = FieldIndex;
		CommandToBindNames.Set(Key, NewElement);
	}
}

/**
* Set MappedString to be the localized string using the FieldName as a key
* Returns the index into the mapped string array of where it was found.
*/
INT UUDKUIDataStore_StringAliasBindingMap::GetStringWithFieldName( const FString& FieldName, FString& MappedString )
{
	INT StartIndex = UCONST_SABM_FIND_FIRST_BIND;
	INT FieldIndex = INDEX_NONE;

	if (FindMappingInBoundKeyCache(FieldName, MappedString, FieldIndex) == FALSE)
	{
		FieldIndex = GetBoundStringWithFieldName( FieldName, MappedString, &StartIndex );
		AddMappingToBoundKeyCache(FieldName, MappedString, FieldIndex);
	}

	return FieldIndex;
}

/**
 * Called by GetStringWithFieldName() to retrieve the string using the input binding system.
 */
INT UUDKUIDataStore_StringAliasBindingMap::GetBoundStringWithFieldName( const FString& FieldName, FString& MappedString, INT* StartIndex/*=NULL*/, FString* BindString/*=NULL*/ )
{
	// String to set MappedString to
	FString LocalizedString = TEXT(" ");

	// Get the index in the MenuInputMapArray using FieldName as the key.
	INT FieldIdx = INDEX_NONE;
	FName KeyName = FName(*FieldName);
	for ( INT Idx = 0; Idx < MenuInputMapArray.Num(); Idx++ )
	{
		if ( KeyName == MenuInputMapArray(Idx).FieldName )
		{
			// Found it
			FieldIdx = Idx;
			break;
		}
	}

	// If we found the entry in our array find the binding and map it to a localized string.
	if ( FieldIdx != INDEX_NONE )
	{
		// Determine how the localized string will need to be mapped.
		// 0 = PC
		// 1 = XBox360
		// 2 = PS3
		INT Platform;

		// FIXME TEMP - for PC development of 360 controls
		Platform = 1;

#if XBOX
		Platform = 1;
#elif PS3
		Platform = 2;
#else
		switch ( FakePlatform )
		{
			case 1: Platform = 1; break;
			case 2: Platform = 2; break;
			default: Platform = 0; break;
		}
#endif

		// Get the player controller.
		ULocalPlayer* LP = GetPlayerOwner();
		AUDKPlayerController* UTPC = LP ? Cast<AUDKPlayerController>(LP->Actor) : NULL;

		FString NameSearch = TEXT("");
		INT BindIndex = -1;

		if ( UTPC )
		{
			// Get the bind using the mapped FieldName as the key
			FString KeyCommand = MenuInputMapArray(FieldIdx).FieldName.ToString();
			if ( KeyCommand.Len() > 0 )
			{
				UUDKPlayerInput* UTInput = Cast<UUDKPlayerInput>(UTPC->PlayerInput);
				if ( UTInput )
				{
					if ( StartIndex && *StartIndex == UCONST_SABM_FIND_FIRST_BIND )
					{
						// Get the game logic specific bind based from the command.
						KeyCommand = UTInput->GetUDKBindNameFromCommand( *KeyCommand );
					}
					else
					{
						// Get the bind starting from the back at index StartIndex.
						KeyCommand = UTInput->GetBindNameFromCommand( *KeyCommand, StartIndex );

						// Don't allow controller binds to be shown on PC.
						if ( Platform == 0 )
						{
							while( KeyCommand.StartsWith(TEXT("XBoxTypeS")) && (StartIndex && *StartIndex > -1) )
							{
								(*StartIndex)--;
								KeyCommand = UTInput->GetBindNameFromCommand( *KeyCommand, StartIndex );
							}
						}
					}

					// Set the bind string to the string we found.
					if ( BindString )
					{
						*BindString = KeyCommand;
					}

					// If this is a controller string we have to check the ControllerMapArray for the localized text.
					if ( KeyCommand.StartsWith(TEXT("XBoxTypeS")) )
					{
						// Prefix the mapping with the localized string variable prefix.
						FString SubString = FString::Printf(TEXT("GMS_%s"),*KeyCommand);

						// If this is the Xbox360 or PS3 map it to the localized button strings.
						if ( Platform > 0 )
						{
							FName CommandName = FName(*SubString);
							for ( INT Idx = 0; Idx < ControllerMapArray.Num(); Idx++ )
							{
								if ( CommandName == ControllerMapArray(Idx).KeyName )
								{
									// Found it, now set the correct mapping.
									if ( Platform == 1 )
									{
										SubString = ControllerMapArray(Idx).XBoxMapping;
									}
									else
									{
										SubString = ControllerMapArray(Idx).PS3Mapping;
									}

									// Try and localize it using the ButtonFont section.
									LocalizedString = Localize( TEXT("ButtonFont"), *SubString, TEXT("UDKGameUI") );
									break;
								}
							}
						}
						else
						{
							// Try and localize it using the GameMappedStrings section.
							LocalizedString = Localize( TEXT("GameMappedStrings"), *SubString, TEXT("UDKGameUI") );
						}
					}
					else
					{
						// Could not find a mapping... if this happens the game is trying to draw the string for a bind that
						// it didn't ensure would exist.
						if ( KeyCommand.Len() <= 0 )
						{
							LocalizedString = TEXT("");
						}
						// Found a bind.
						else
						{
							// Prefix the mapping with the localized string variable prefix.
							FString SubString = FString::Printf(TEXT("GMS_%s"),*KeyCommand);
							// Try and localize it using the GameMappedStrings section.
							LocalizedString = Localize( TEXT("GameMappedStrings"), *SubString, TEXT("UDKGameUI") );
						}
					}
				}
			}
		}
	}

	// Set the localized string and return the index.
	MappedString = LocalizedString;
	return FieldIdx;
}

//////////////////////////////////////////////////////////////////////////
// UUDKUIDataProvider_SearchResult
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIDataProvider_SearchResult);

/**
 * @return	TRUE if server corresponding to this search result is password protected.
 */
UBOOL UUDKUIDataProvider_SearchResult::IsPrivateServer()
{
	INT SettingValueIndex = 0;
	if ( Settings != NULL )
	{
		static FName PrivateServerSettingName = FName(TEXT("LockedServer"));
		verify(Settings->GetStringSettingValueByName(PrivateServerSettingName, SettingValueIndex));
	}
	return SettingValueIndex == 1;
}

/**
 * @return	TRUE if server corresponding to this search result allows players to use keyboard & mouse.
 */
UBOOL UUDKUIDataProvider_SearchResult::AllowsKeyboardMouse()
{
	INT SettingValueIndex = 0;
	if ( Settings != NULL )
	{
		static FName KMAllowedSettingName = FName(TEXT("AllowKeyboard"));
		verify(Settings->GetStringSettingValueByName(KMAllowedSettingName, SettingValueIndex));
	}
	return SettingValueIndex == 1;
}

/**
 * Resolves the value of the data field specified and stores it in the output parameter.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with data collections
 */
UBOOL UUDKUIDataProvider_SearchResult::GetFieldValue(const FString& FieldName,FUIProviderFieldValue& out_FieldValue,INT ArrayIndex)
{
	UBOOL bResult=FALSE;

	const FName FieldFName(*FieldName);
	if ( FieldFName == ServerFlagsTag )
	{
		out_FieldValue.PropertyTag = PlayerRatioTag;
		out_FieldValue.PropertyType = DATATYPE_Property;

		// since we always want this field value to be displayed using a certain font, use markup to ensure that font is used
		// this way we can just use the same style as the rest of the list or string
		out_FieldValue.StringValue = US + TEXT("<Fonts:") + IconFontPathName + TEXT(">");

		if ( !GIsGame )
		{
			out_FieldValue.StringValue += TEXT("0");
		}

		if ( IsPrivateServer() || !GIsGame )
		{
			out_FieldValue.StringValue += TEXT("7");
		}

		if ( AllowsKeyboardMouse() || !GIsGame )
		{
			out_FieldValue.StringValue += TEXT("6");
		}

		// closing data store markup
		out_FieldValue.StringValue += TEXT("<Fonts:/>");

		bResult = TRUE;
	}
	else if ( FieldFName == PlayerRatioTag )
	{
		FUIProviderFieldValue TotalValue(EC_EventParm);
		FUIProviderFieldValue OpenValue(EC_EventParm);
		if (Super::GetFieldValue(TEXT("NumPublicConnections"), TotalValue, ArrayIndex)
		&&	Super::GetFieldValue(TEXT("NumOpenPublicConnections"), OpenValue, ArrayIndex))
		{
			INT OpenNum = appAtoi(*OpenValue.StringValue);
			INT TotalNum = appAtoi(*TotalValue.StringValue);

			out_FieldValue.PropertyTag = PlayerRatioTag;
			out_FieldValue.StringValue = FString::Printf(TEXT("%i/%i"), TotalNum-OpenNum, TotalNum);
			bResult = TRUE;
		}
	}
	else if ( FieldFName == GameModeFriendlyNameTag || FieldFName == TEXT("CustomGameMode") )
	{
		FUIProviderFieldValue GameClassName(EC_EventParm);

		if ( Super::GetFieldValue(TEXT("CustomGameMode"), GameClassName, ArrayIndex) )
		{
			// Try to get the localized game name out of the localization file.
			FString LocalizedGameName;
			LocalizedGameName = GameClassName.StringValue;
			
			// Replace _Content class name with normal class name.
			if(LocalizedGameName.ReplaceInline(TEXT("UTGameContent."),TEXT("UTGame.")))
			{
				LocalizedGameName.ReplaceInline(TEXT("_Content"),TEXT(""));
			}

			INT PeriodPos = LocalizedGameName.InStr(TEXT("."));
			if(PeriodPos > 0 && PeriodPos < LocalizedGameName.Len())
			{	
				FString GamePackageName = LocalizedGameName.Left(PeriodPos);
				FString GameModeName = LocalizedGameName.Mid(PeriodPos + 1);
				FString LocalizedResult = Localize(*GameModeName, TEXT("GameName"), *GamePackageName, NULL, TRUE);
				if ( LocalizedResult.Len() > 0 )
				{
					LocalizedGameName = LocalizedResult;
				}
				else
				{
					LocalizedGameName = GameModeName;
				}
			}

			// Return the string field value.
			out_FieldValue.PropertyTag = GameModeFriendlyNameTag;
			out_FieldValue.StringValue = LocalizedGameName;
			bResult = TRUE;
		}
	}
	else if ( FieldFName == MapNameTag )
	{
		FUIProviderFieldValue ActualMapName(EC_EventParm);
		if ( Super::GetFieldValue(FieldName, ActualMapName, ArrayIndex) )
		{
			const FName MapTag(TEXT("Maps"));

			// same thing for the map
			// for maps however, we'll need to look up this map in the menu items (game resource) data store
			// for the friendly name because there isn't any loc section to look in for a map (no class)
			UDataStoreClient* DSClient = UUIInteraction::GetDataStoreClient();
			if ( DSClient != NULL )
			{
				UUDKUIDataStore_MenuItems* ResourceDataStore = Cast<UUDKUIDataStore_MenuItems>(DSClient->FindDataStore(TEXT("UTMenuItems")));
				if ( ResourceDataStore != NULL )
				{
					INT ProviderIndex = ResourceDataStore->FindValueInProviderSet(MapTag, TEXT("MapName"), ActualMapName.StringValue);
					if ( ProviderIndex != INDEX_NONE )
					{
						if ( ResourceDataStore->GetValueFromProviderSet(MapTag, TEXT("FriendlyName"), ProviderIndex, out_FieldValue.StringValue) )
						{
							out_FieldValue.PropertyTag = MapNameTag;
							out_FieldValue.PropertyType = DATATYPE_Property;
							bResult = TRUE;
						}
					}
				}
			}

			if ( !bResult && ActualMapName.StringValue.Len() > 0 )
			{
				out_FieldValue = ActualMapName;
				out_FieldValue.PropertyTag = MapNameTag;
				bResult = TRUE;
			}
		}
	}
	else if ( FieldName == TEXT("OwningPlayerName") )
	{
		FUIProviderFieldValue PlayerName(EC_EventParm);
		FString FinalName;

		// Get the player's name first
		if(Super::GetFieldValue(TEXT("OwningPlayerName"), PlayerName, ArrayIndex))
		{
			FinalName = PlayerName.StringValue;

			// See if we should append a server description.
			FUIProviderFieldValue ServerDescription(EC_EventParm);
			if(Super::GetFieldValue(TEXT("ServerDescription"), ServerDescription, ArrayIndex))
			{
				FString DescStr = ServerDescription.StringValue.Trim().TrimTrailing();
				if(DescStr.Len())
				{
					FinalName += TEXT(": ");
					FinalName += DescStr;
				}
			}

			out_FieldValue.PropertyTag = *FieldName;
			out_FieldValue.StringValue = FinalName;
			bResult = TRUE;
		}
	}
	else
	{
		bResult = Super::GetFieldValue(FieldName, out_FieldValue, ArrayIndex);
	}

	return bResult;
}

/**
 * Builds a list of available fields from the array of properties in the
 * game settings object
 *
 * @param OutFields	out value that receives the list of exposed properties
 */
void UUDKUIDataProvider_SearchResult::GetSupportedDataFields(TArray<FUIDataProviderField>& OutFields)
{
	Super::GetSupportedDataFields(OutFields);

	OutFields.AddItem(FUIDataProviderField(TEXT("PlayerRatio")));
	OutFields.AddItem(FUIDataProviderField(TEXT("GameModeFriendlyName")));

	// this field displays icons indicating whether the server is pure, locked, allows keyboard/mouse
	new(OutFields) FUIDataProviderField(TEXT("ServerFlags"));
}

/**
 * Gets the list of data fields (and their localized friendly name) for the fields exposed this provider.
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	out_CellTags	receives the name/friendly name pairs for all data fields in this provider.
 */
void UUDKUIDataProvider_SearchResult::GetElementCellTags( FName FieldName, TMap<FName,FString>& out_CellTags )
{
	Super::GetElementCellTags(FieldName, out_CellTags);

	if ( FieldName == UCONST_UnknownCellDataFieldName )
	{
		const FString SectionName = GetClass()->GetName();

		out_CellTags.Set( TEXT("PlayerRatio"), *Localize(*SectionName, TEXT("PlayerRatio"), TEXT("UDKGameUI")) );
		out_CellTags.Set( TEXT("GameModeFriendlyName"), *Localize(*SectionName, TEXT("GameModeFriendlyName"), TEXT("UDKGameUI")) );
		out_CellTags.Set( TEXT("ServerFlags"), *Localize(*SectionName, TEXT("ServerFlags"), TEXT("UDKGameUI")) );
	}
}


//////////////////////////////////////////////////////////////////////////
// UUDKDataStore_GameSearchBase
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKDataStore_GameSearchBase);

/**
 * Loads and creates an instance of the registered filter object
 */
void UUDKDataStore_GameSearchBase::InitializeDataStore(void)
{
	Super::InitializeDataStore();

	// Create server details object
	ServerDetailsProvider = ConstructObject<UUDKUIDataProvider_ServerDetails>(UUDKUIDataProvider_ServerDetails::StaticClass(), this);
}

/**
 * Retrieves the list of currently enabled mutators.
 *
 * @param	MutatorIndices	indices are from the list of UTUIDataProvider_Mutator data providers in the
 *							UTUIDataStore_MenuItems data store which are currently enabled.
 *
 * @return	TRUE if the list of enabled mutators was successfully retrieved.
 */
UBOOL UUDKDataStore_GameSearchBase::GetEnabledMutators( TArray<INT>& MutatorIndices )
{
	UBOOL bResult = FALSE;

	MutatorIndices.Empty();

	if ( ServerDetailsProvider != NULL )
	{
		UUIDataProvider_Settings* SearchResults = ServerDetailsProvider->GetSearchResultsProvider();
		if ( SearchResults != NULL )
		{
			// get the bitmask of enabled official mutators
			FUIProviderFieldValue MutatorValue(EC_EventParm);
			if ( SearchResults->GetFieldValue(TEXT("OfficialMutators"), MutatorValue) )
			{
				const INT MutatorBitmask = appAtoi(*MutatorValue.StringValue);
				for ( INT BitIdx = 0; BitIdx < sizeof(INT) * 8; BitIdx++ )
				{
					if ( (MutatorBitmask&(1<<BitIdx)) != 0 )
					{
						MutatorIndices.AddItem(BitIdx);
					}
				}

				bResult = TRUE;
			}

			// now get the list of enabled custom mutators
			MutatorValue.StringValue = TEXT("");
			if ( SearchResults->GetFieldValue(TEXT("CustomMutators"), MutatorValue) && MutatorValue.StringValue.Len() )
			{
				const TCHAR CustomMutatorDelimiter[2] = { 0x1C, 0x00 };
				TArray<FString> CustomMutatorNames;

				const INT MutatorBitmaskBoundary = sizeof(INT) * 8;
				MutatorValue.StringValue.ParseIntoArray(&CustomMutatorNames, CustomMutatorDelimiter, TRUE);
				for ( INT MutIndex = 0; MutIndex < CustomMutatorNames.Num(); MutIndex++ )
				{
					MutatorIndices.AddItem(MutatorBitmaskBoundary + MutIndex);
				}

				bResult = TRUE;
			}

			UDataStoreClient* DSClient = UUIInteraction::GetDataStoreClient();
			if ( DSClient != NULL )
			{
				UUDKUIDataStore_MenuItems* ResourceDataStore = Cast<UUDKUIDataStore_MenuItems>(DSClient->FindDataStore(TEXT("UTMenuItems")));
				if ( ResourceDataStore != NULL )
				{
					//@todo
					bResult = TRUE;
				}
			}
		}
	}

	return bResult;
}

/**
 * Returns the stats read results as a collection and appends the filter provider
 *
 * @param OutFields	out value that receives the list of exposed properties
 */
void UUDKDataStore_GameSearchBase::GetSupportedDataFields(TArray<FUIDataProviderField>& OutFields)
{
	Super::GetSupportedDataFields(OutFields);
	
	// Append the server details provider
	new(OutFields) FUIDataProviderField(FName(TEXT("CurrentServerDetails")),DATATYPE_Provider,ServerDetailsProvider);
	new(OutFields) FUIDataProviderField(TEXT("CurrentServerMutators"), DATATYPE_Collection);
}

/**
 * Retrieves the list of all data tags contained by this element provider which correspond to list element data.
 *
 * @return	the list of tags supported by this element provider which correspond to list element data.
 */
TArray<FName> UUDKDataStore_GameSearchBase::GetElementProviderTags()
{
	TArray<FName> Result = Super::GetElementProviderTags();

	Result.AddItem(TEXT("CurrentServerDetails"));
	Result.AddItem(TEXT("CurrentServerMutators"));

	return Result;
}

/**
 * Returns the number of list elements associated with the data tag specified.
 *
 * @param	FieldName	the name of the property to get the element count for.  guaranteed to be one of the values returned
 *						from GetElementProviderTags.
 *
 * @return	the total number of elements that are required to fully represent the data specified.
 */
INT UUDKDataStore_GameSearchBase::GetElementCount( FName FieldName )
{
	INT Result=0;

	if(FieldName==TEXT("CurrentServerDetails"))
	{
		Result = ServerDetailsProvider->GetElementCount();	
	}
	else if ( FieldName == TEXT("CurrentServerMutators") )
	{
		TArray<INT> Mutators;
		if ( GetEnabledMutators(Mutators) )
		{
			Result = Mutators.Num();
		}
	}
	else
	{
		Result = Super::GetElementCount(FieldName);
	}

	return Result;
}

/**
 * Retrieves the list elements associated with the data tag specified.
 *
 * @param	FieldName		the name of the property to get the element count for.  guaranteed to be one of the values returned
 *							from GetElementProviderTags.
 * @param	out_Elements	will be filled with the elements associated with the data specified by DataTag.
 *
 * @return	TRUE if this data store contains a list element data provider matching the tag specified.
 */
UBOOL UUDKDataStore_GameSearchBase::GetListElements( FName FieldName, TArray<INT>& out_Elements )
{
	UBOOL bResult = FALSE;
	
	if(FieldName==TEXT("CurrentServerDetails"))
	{
		bResult = ServerDetailsProvider->GetListElements(FieldName, out_Elements);
	}
	else if ( FieldName == TEXT("CurrentServerMutators") )
	{
		bResult = GetEnabledMutators(out_Elements);
	}
	else
	{
		bResult = Super::GetListElements(FieldName, out_Elements);
	}

	return bResult;
}

/**
 * Retrieves a UIListElementCellProvider for the specified data tag that can provide the list with the available cells for this list element.
 * Used by the UI editor to know which cells are available for binding to individual list cells.
 *
 * @param	FieldName		the tag of the list element data field that we want the schema for.
 *
 * @return	a pointer to some instance of the data provider for the tag specified.  only used for enumerating the available
 *			cell bindings, so doesn't need to actually contain any data (i.e. can be the CDO for the data provider class, for example)
 */
TScriptInterface<class IUIListElementCellProvider> UUDKDataStore_GameSearchBase::GetElementCellSchemaProvider( FName FieldName )
{
	if( FieldName==TEXT("CurrentServerDetails") || FieldName == TEXT("CurrentServerMutators") )
	{
		return TScriptInterface<IUIListElementCellProvider>(ServerDetailsProvider);
	}
	return Super::GetElementCellSchemaProvider(FieldName);
}

/**
 * Retrieves a UIListElementCellProvider for the specified data tag that can provide the list with the values for the cells
 * of the list element indicated by CellValueProvider.DataSourceIndex
 *
 * @param	FieldName		the tag of the list element data field that we want the values for
 * @param	ListIndex		the list index for the element to get values for
 *
 * @return	a pointer to an instance of the data provider that contains the value for the data field and list index specified
 */
TScriptInterface<class IUIListElementCellProvider> UUDKDataStore_GameSearchBase::GetElementCellValueProvider( FName FieldName, INT ListIndex )
{
	if( FieldName==TEXT("CurrentServerDetails") || FieldName == TEXT("CurrentServerMutators") )
	{
		return TScriptInterface<IUIListElementCellProvider>(ServerDetailsProvider);
	}
	return Super::GetElementCellValueProvider(FieldName, ListIndex);
}

TScriptInterface<class IUIListElementProvider> UUDKDataStore_GameSearchBase::ResolveListElementProvider( const FString& PropertyName ) 
{
	if( PropertyName==TEXT("CurrentServerDetails") || PropertyName == TEXT("CurrentServerMutators") )
	{
		return TScriptInterface<IUIListElementProvider> (this);
	}
	
	return Super::ResolveListElementProvider(PropertyName);
}

//////////////////////////////////////////////////////////////////////////
// UUDKUIDataProvider_ServerDetails
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIDataProvider_ServerDetails);

/** @return Returns a reference to the search results provider that is used to generate data for this class. */
UUIDataProvider_Settings* UUDKUIDataProvider_ServerDetails::GetSearchResultsProvider()
{
	UUIDataProvider_Settings* ServerDetailsProvider = NULL;

	UUIDataStore_OnlineGameSearch* GameSearchDataStore = Cast<UUIDataStore_OnlineGameSearch>(GetOuter());
	if ( GameSearchDataStore == NULL )
	{
		// Find the game search datastore
		UDataStoreClient* DSClient = UUIInteraction::GetDataStoreClient();
		if( DSClient != NULL )
		{
			GameSearchDataStore = Cast<UUIDataStore_OnlineGameSearch>(DSClient->FindDataStore(TEXT("UTGameSearch")));
		}
	}

	if (GameSearchDataStore != NULL
	&&	GameSearchDataStore->GameSearchCfgList.IsValidIndex(GameSearchDataStore->SelectedIndex)
	&&	GameSearchDataStore->GameSearchCfgList(GameSearchDataStore->SelectedIndex).SearchResults.IsValidIndex(SearchResultsRow))
	{
		// Get the current server details given our search results row
		ServerDetailsProvider = GameSearchDataStore->GameSearchCfgList(GameSearchDataStore->SelectedIndex).SearchResults(SearchResultsRow);
	}

	return ServerDetailsProvider;
}

/**
 * Determines whether the specified field should be included when the user requests to see a list of this server's details.
 */
UBOOL UUDKUIDataProvider_ServerDetails::ShouldDisplayField( FName FieldName )
{
	static TLookupMap<FName> FieldsToHide;
	if ( FieldsToHide.Num() == 0 )
	{
		FieldsToHide.AddItem(FName(TEXT("MaxPlayers"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("MinNetPlayers"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bShouldAdvertise"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bIsLanMatch"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bAllowJoinInProgress"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bAllowInvites"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bUsesPresence"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bUsesStats"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bAllowJoinViaPresence"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bUsesArbitration"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bAntiCheatProtected"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("bIsListPlay"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("Campaign"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("CustomMapName"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("CustomGameMode"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("CustomMutators"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("GameMode"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("ServerDescription"),FNAME_Find));

		// these are the fields that are shown in the main server browser list
		FieldsToHide.AddItem(FName(TEXT("ServerFlags")));
		FieldsToHide.AddItem(FName(TEXT("OwningPlayerName"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("PingInMs"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("OfficialMutators"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("PlayerRatio"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("NumOpenPrivateConnections"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("NumOpenPublicConnections"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("NumPublicConnections"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("NumPrivateConnections"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("MapName"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("PureServer"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("LockedServer"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("IsFullServer"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("IsEmptyServer"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("AllowKeyboard"),FNAME_Find));
		FieldsToHide.AddItem(FName(TEXT("IsDedicated"),FNAME_Find));
	}

	return !FieldsToHide.HasKey(FieldName);
}

/**
 * Retrieves the list elements associated with the data tag specified.
 *
 * @param	FieldName		the name of the property to get the element count for.  guaranteed to be one of the values returned
 *							from GetElementProviderTags.
 * @param	out_Elements	will be filled with the elements associated with the data specified by DataTag.
 *
 * @return	TRUE if this data store contains a list element data provider matching the tag specified.
 */
UBOOL UUDKUIDataProvider_ServerDetails::GetListElements( FName FieldName, TArray<INT>& out_Elements )
{
	UBOOL bResult = FALSE;

	if ( FieldName == TEXT("CurrentServerDetails") )
	{
		UUIDataProvider_Settings* ServerDetailsProvider = GetSearchResultsProvider();
		if(ServerDetailsProvider != NULL)
		{
			TArray<FUIDataProviderField> SupportedFields;
			ServerDetailsProvider->GetSupportedDataFields(SupportedFields);

			for ( INT FieldIndex = 0; FieldIndex < SupportedFields.Num(); FieldIndex++ )
			{
				if ( ShouldDisplayField(SupportedFields(FieldIndex).FieldTag) )
				{
					out_Elements.AddItem(FieldIndex);
				}
			}
		}
		bResult = TRUE;
	}

	return bResult;
}

/** Returns the number of elements in the list. */
INT UUDKUIDataProvider_ServerDetails::GetElementCount()
{
	INT Result = 0;

	UUIDataProvider_Settings* ServerDetailsProvider = GetSearchResultsProvider();
	if(ServerDetailsProvider != NULL)
	{
		TArray<FUIDataProviderField> SupportedFields;
		ServerDetailsProvider->GetSupportedDataFields(SupportedFields);

		Result = SupportedFields.Num();
		for ( INT FieldIndex = 0; FieldIndex < SupportedFields.Num(); FieldIndex++ )
		{
			if ( !ShouldDisplayField(SupportedFields(FieldIndex).FieldTag) )
			{
				Result--;
			}
		}
	}
	
	return Result;
}

// IUIListElement interface

/**
 * Returns the names of the exposed members in the first entry in the array
 * of search results
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param OutCellTags the columns supported by this row
 */
void UUDKUIDataProvider_ServerDetails::GetElementCellTags( FName FieldName, TMap<FName,FString>& OutCellTags )
{
	OutCellTags.Empty();

	if ( FieldName == TEXT("CurrentServerDetails") )
	{
		OutCellTags.Set(TEXT("Key"),*Localize(TEXT("ServerBrowser"), TEXT("Key"), TEXT("UDKGameUI")));
		OutCellTags.Set(TEXT("Value"),*Localize(TEXT("ServerBrowser"), TEXT("Value"), TEXT("UDKGameUI")));
	}
	else if ( FieldName == TEXT("CurrentServerMutators") )
	{
		OutCellTags.Set(TEXT("CurrentServerMutators"), *Localize(TEXT("ServerBrowser"), TEXT("CurrentServerMutators"), TEXT("UDKGameUI")));
	}
	else
	{
		Super::GetElementCellTags(FieldName, OutCellTags);
	}
}


/**
 * Resolves the value of the cell specified by CellTag and stores it in the output parameter.
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the index of the value to fetch
 * @param	OutFieldValue	receives the resolved value for the property specified.
 * @param	ArrayIndex		ignored
 */
UBOOL UUDKUIDataProvider_ServerDetails::GetCellFieldValue(FName FieldName, const FName& CellTag,INT ListIndex,FUIProviderFieldValue& OutFieldValue,INT)
{
	UBOOL bResult = FALSE;
	UUIDataProvider_Settings* ServerDetailsProvider = GetSearchResultsProvider();

	if(ServerDetailsProvider != NULL)
	{
		if ( CellTag == TEXT("CurrentServerMutators") )
		{
			UDataStoreClient* DSClient = UUIInteraction::GetDataStoreClient();
			if ( DSClient != NULL )
			{
				UUDKUIDataStore_MenuItems* ResourceDataStore = Cast<UUDKUIDataStore_MenuItems>(DSClient->FindDataStore(TEXT("UTMenuItems")));
				if ( ResourceDataStore != NULL )
				{
					TArray<UUIResourceDataProvider*> Providers;
					ResourceDataStore->ListElementProviders.MultiFind(TEXT("Mutators"), Providers);

					if ( Providers.IsValidIndex(ListIndex) )
					{
						UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ListIndex));
						if ( DataProvider )
						{
							OutFieldValue.PropertyTag = CellTag;
							OutFieldValue.PropertyType = DATATYPE_Property;
							OutFieldValue.StringValue = DataProvider->FriendlyName;
							bResult = TRUE;
						}
					}
				}
			}
		}
		else
		{
			// Use the supported fields of the datastore for our row data.
			TMap<FName,FString> SupportedFields;
			ServerDetailsProvider->GetElementCellTags(UCONST_UnknownCellDataFieldName, SupportedFields);

			TMap<FName,FString>::TIterator Itor(SupportedFields, FALSE, ListIndex);
			if ( Itor )
			{
				if ( CellTag==TEXT("Key") )
				{
					OutFieldValue.PropertyTag = CellTag;
					OutFieldValue.PropertyType = DATATYPE_Property;

					// set the value to the localized friendly name for this field
					OutFieldValue.StringValue = Itor.Value();
					// but if it doesn't have one, just use the name of the field itself
					if ( OutFieldValue.StringValue.Len() == 0 )
					{
						OutFieldValue.StringValue = Itor.Key().ToString();
					}
					bResult = TRUE;
				}
				else if ( CellTag==TEXT("Value") )
				{
					bResult = ServerDetailsProvider->GetFieldValue(Itor.Key().ToString(), OutFieldValue);
				}
			}
			else
			{
				// if an invalid ListIndex was specified, it means that one of our fields was requested by name ("NumPlayers" instead of e.g. "Key[3]"),
				// so search for the named field and return just the value.
				if ( SupportedFields.HasKey(CellTag) )
				{
					OutFieldValue.PropertyTag = CellTag;
					OutFieldValue.PropertyType = DATATYPE_Property;
					bResult = ServerDetailsProvider->GetFieldValue(CellTag.ToString(), OutFieldValue);
				}
			}
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
// UUDKUIDataStore_MenuItems
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIDataStore_MenuItems);

void UUDKUIDataStore_MenuItems::GetAllResourceDataProviders(UClass* ProviderClass, TArray<UUDKUIResourceDataProvider*>& Providers)
{
	checkSlow(ProviderClass->IsChildOf(UUDKUIResourceDataProvider::StaticClass()));

	// Get a list of ini files from disk.
	TArray<FString> ConfigFileNamesFromDisk;
	GFileManager->FindFiles(ConfigFileNamesFromDisk, *(appGameConfigDir() + TEXT("*.ini")), TRUE, FALSE);

	// Get a list of ini files in memory.  Things like e.g. DLC that were merged at startup
	// will be in memory but not in the config dir.
	TArray<FFilename> ConfigFileNamesInMemory;
	GConfig->GetConfigFilenames( ConfigFileNamesInMemory );

	// Merge the two lists so that providers declared in inis in both lists don't get double-added to the menus.
	TArray<FString> ConfigFileNames;
	for (INT i = 0; i < ConfigFileNamesFromDisk.Num(); i++)	
	{
		ConfigFileNames.AddUniqueItem(ConfigFileNamesFromDisk(i));
	}
	for (INT i = 0; i < ConfigFileNamesInMemory.Num(); i++)	
	{
		// There may be loc files in memory -- we don't want these.
		if ( ConfigFileNamesInMemory(i).GetExtension() == TEXT("ini") )
		{
			ConfigFileNames.AddUniqueItem(ConfigFileNamesInMemory(i).GetCleanFilename());
		}
	}

	for (INT i = 0; i < ConfigFileNames.Num(); i++)
	{
		// ignore default .inis
		if (appStricmp(*ConfigFileNames(i).Left(7), TEXT("Default")) != 0)
		{
			FString FullConfigPath = appGameConfigDir() + ConfigFileNames(i);
			TArray<FString> GameTypeResourceSectionNames;
			if (GConfig->GetPerObjectConfigSections(*FullConfigPath, *ProviderClass->GetName(), GameTypeResourceSectionNames) )
			{
				for (INT SectionIndex = 0; SectionIndex < GameTypeResourceSectionNames.Num(); SectionIndex++)
				{
					INT POCDelimiterPosition = GameTypeResourceSectionNames(SectionIndex).InStr(TEXT(" "));
					// we shouldn't be here if the name was included in the list
					checkSlow(POCDelimiterPosition != INDEX_NONE);

					FName ObjectName = *GameTypeResourceSectionNames(SectionIndex).Left(POCDelimiterPosition);
					if (ObjectName != NAME_None)
					{
						UBOOL bLoadDataProvider = TRUE;

						// if map, make sure it exists
						if ( ProviderClass == MapInfoDataProviderClass )
						{
							FString PackageFileName;
							bLoadDataProvider = GPackageFileCache->FindPackageFile(*ObjectName.ToString(), NULL, PackageFileName);
						}
						
						if ( bLoadDataProvider )
						{
							//@note: names must be unique across all .ini files
							UUDKUIResourceDataProvider* NewProvider = Cast<UUDKUIResourceDataProvider>(StaticFindObject(ProviderClass, ANY_PACKAGE, *ObjectName.ToString(), TRUE));
							if (NewProvider == NULL)
							{
								NewProvider = ConstructObject<UUDKUIResourceDataProvider>(ProviderClass, IsTemplate() ? (UObject*)GetTransientPackage() : this, ObjectName);
								if (NewProvider != NULL)
								{
									// load the config and localized values from the current .ini name
									NewProvider->IniName = *FFilename(ConfigFileNames(i)).GetBaseFilename();
									NewProvider->LoadConfig(NULL, *FullConfigPath);
									LoadLocalizedStruct( ProviderClass, *NewProvider->IniName,
														*(ObjectName.ToString() + TEXT(" ") + ProviderClass->GetName()), NULL, NewProvider, (BYTE*)NewProvider );
								}
							}

							if (NewProvider != NULL)
							{
								Providers.AddItem(NewProvider);
							}
						}
					}
				}
			}
		}
	}

	if ( ProviderClass == MapInfoDataProviderClass )
	{
		// also look for packages with UDK extensions, and add them if not already
		const TArray<FString> FilesInPath( GPackageFileCache->GetPackageFileList() );
		if( FilesInPath.Num() > 0 )
		{
			// Iterate over all files looking for maps.
			for( INT FileIndex = 0 ; FileIndex < FilesInPath.Num() ; ++FileIndex )
			{
				const FFilename& FileName = FilesInPath(FileIndex);
				if ( FileName.GetExtension() == TEXT("UDK") )
				{
					const FString MapName = FileName.GetBaseFilename();

					if ( MapName != TEXT("EnvyEntry") && MapName != TEXT("UDKFrontEndMap"))
					{
				        // The 'FriendlyName' is the name that will be displayed in the menus.
				        // Strip the gametype from the front (e.g. 'DM-Blah' becomes 'Blah').
				        FString FriendlyName = MapName;
				        FString MapPrefix;
				        const INT DashPos = FriendlyName.InStr( TEXT("-") );
				        if (DashPos != -1)
				        {
					        MapPrefix = FriendlyName.Left( DashPos );
					        FriendlyName = FriendlyName.Right( (FriendlyName.Len() - DashPos) - 1);
				        }

						INT ProviderIndex;

						// Now add the map to the data provider
						UUDKUIResourceDataProvider* ExistingProvider = Cast<UUDKUIResourceDataProvider>(StaticFindObject(ProviderClass, ANY_PACKAGE, *MapName, TRUE));
						if (ExistingProvider == NULL)
						{
							UUDKUIDataProvider_MapInfo* NewProvider = Cast<UUDKUIDataProvider_MapInfo>(StaticFindObject(ProviderClass, ANY_PACKAGE, *MapName, TRUE));
							if ( NewProvider == NULL )
							{
								NewProvider = ConstructObject<UUDKUIDataProvider_MapInfo>(ProviderClass, IsTemplate() ? (UObject*)GetTransientPackage() : this, FName(*MapName));
								if (NewProvider != NULL)
								{
									// load the config and localized values from the current .ini name
									NewProvider->MapName = MapName;
									NewProvider->FriendlyName = FriendlyName;
									Providers.AddItem(NewProvider);
								}
							}
						}
						else if( !Providers.FindItem(ExistingProvider, ProviderIndex) )
						{
							Providers.AddItem(ExistingProvider);
						}
					}
				}	
			}
		}
	}
}



/**
 * Finds or creates the UIResourceDataProvider instances referenced by ElementProviderTypes, and stores the result
 * into the ListElementProvider map.
 */
void UUDKUIDataStore_MenuItems::InitializeListElementProviders()
{
	ListElementProviders.Empty();

	// for each configured provider type, retrieve the list of ini sections that contain data for that provider class
	for ( INT ProviderTypeIndex = 0; ProviderTypeIndex < ElementProviderTypes.Num(); ProviderTypeIndex++ )
	{
		FGameResourceDataProvider& ProviderType = ElementProviderTypes(ProviderTypeIndex);

		UClass* ProviderClass = ProviderType.ProviderClass;

#if !CONSOLE
		if (ProviderClass->IsChildOf(UUDKUIResourceDataProvider::StaticClass()) && ProviderClass->GetDefaultObject<UUDKUIResourceDataProvider>()->bSearchAllInis)
		{
			// search all .ini files for instances to create
			TArray<UUDKUIResourceDataProvider*> Providers;
			GetAllResourceDataProviders(ProviderClass, Providers);
			for (INT i = 0; i < Providers.Num(); i++)
			{
				ListElementProviders.Add(ProviderType.ProviderTag, Providers(i));
			}
		}
		else
#endif
		{
			// use default method of only searching the class's .ini file
			TArray<FString> GameTypeResourceSectionNames;
			if ( GConfig->GetPerObjectConfigSections(*ProviderClass->GetConfigName(), *ProviderClass->GetName(), GameTypeResourceSectionNames) )
			{
				for ( INT SectionIndex = 0; SectionIndex < GameTypeResourceSectionNames.Num(); SectionIndex++ )
				{
					INT POCDelimiterPosition = GameTypeResourceSectionNames(SectionIndex).InStr(TEXT(" "));
					// we shouldn't be here if the name was included in the list
					check(POCDelimiterPosition!=INDEX_NONE);

					FName ObjectName = *GameTypeResourceSectionNames(SectionIndex).Left(POCDelimiterPosition);
					if ( ObjectName != NAME_None )
					{
						UUIResourceDataProvider* Provider = Cast<UUIResourceDataProvider>( StaticFindObject(ProviderClass, ANY_PACKAGE, *ObjectName.ToString(), TRUE) );
						if ( Provider == NULL )
						{
							Provider = ConstructObject<UUIResourceDataProvider>(
								ProviderClass,
								this,
								ObjectName
							);
						}

						if ( Provider != NULL )
						{
							ListElementProviders.Add(ProviderType.ProviderTag, Provider);
						}
					}
				}
			}
		}
	}

	eventInitializeListElementProviders();

	SortRelevantProviders();
}

/**
  * Remove key from ListElementProviders multimap
  */
void UUDKUIDataStore_MenuItems::RemoveListElementProvidersKey(FName KeyName)
{
	ListElementProviders.RemoveKey(KeyName);
}

/**
  * Add to ListElementProviders multimap
  */
void UUDKUIDataStore_MenuItems::AddListElementProvidersKey(FName KeyName, UUDKUIResourceDataProvider* Provider)
{
	ListElementProviders.Add(KeyName, Provider);
}


IMPLEMENT_COMPARE_CONSTPOINTER(UUDKUIResourceDataProvider,UI_DataStores, {
	INT Result = 0;
	if ( A && B )
	{
		Result = appStricmp(*A->FriendlyName, *B->FriendlyName);
	}

	return Result;
})

/**
 * Sorts the list of map and mutator data providers according to whether they're official or not, then alphabetically.
 */
void UUDKUIDataStore_MenuItems::SortRelevantProviders()
{
	FName MapsName(TEXT("Maps"));
	// sort the maps
	TArray<UUDKUIResourceDataProvider*> MapProviders;
	ListElementProviders.MultiFind(MapsName, (TArray<UUIResourceDataProvider*>&)MapProviders);
	for ( INT ProviderIndex = 0; ProviderIndex < MapProviders.Num(); ProviderIndex++ )
	{
		UUDKUIResourceDataProvider* Provider = MapProviders(ProviderIndex);
	}

	Sort<USE_COMPARE_CONSTPOINTER(UUDKUIResourceDataProvider,UI_DataStores)>( &MapProviders(0), MapProviders.Num() );

	// now re-add the maps in reverse order so that calls to MultiFind will return them in the right order
	ListElementProviders.RemoveKey(MapsName);
	for ( INT ProviderIndex = MapProviders.Num() - 1; ProviderIndex >= 0; ProviderIndex-- )
	{
		ListElementProviders.Add(MapsName, MapProviders(ProviderIndex));
	}

	// now sort the mutator providers
	FName MutatorsName(TEXT("Mutators"));
	TArray<UUDKUIResourceDataProvider*> MutatorProviders;
	ListElementProviders.MultiFind(MutatorsName, (TArray<UUIResourceDataProvider*>&)MutatorProviders);

	Sort<USE_COMPARE_CONSTPOINTER(UUDKUIResourceDataProvider,UI_DataStores)>( &MutatorProviders(0), MutatorProviders.Num() );

	// now re-add the mutators in reverse order so that calls to MultiFind will return them in the right order
	ListElementProviders.RemoveKey(MutatorsName);
	for ( INT ProviderIndex = MutatorProviders.Num() - 1; ProviderIndex >= 0; ProviderIndex-- )
	{
		ListElementProviders.Add(MutatorsName, MutatorProviders(ProviderIndex));
	}
}

/** 
 * Gets the list of element providers for a fieldname with filtered elements removed.
 *
 * @param FieldName				Fieldname to use to search for providers.
 * @param OutElementProviders	Array to store providers in.
 */
void UUDKUIDataStore_MenuItems::GetFilteredElementProviders(FName FieldName, TArray<UUDKUIResourceDataProvider*> &OutElementProviders)
{
	OutElementProviders.Empty();

	if(FieldName==TEXT("EnabledMutators") || FieldName==TEXT("AvailableMutators"))
	{
		FieldName = TEXT("Mutators");
	}
	else if(FieldName==TEXT("MapCycle") || FieldName==TEXT("MapsNotInCycle"))
	{
		FieldName = TEXT("Maps");
	}
	else if(FieldName==TEXT("WeaponPriority"))
	{
		FieldName = TEXT("DropDownWeapons");
	}
	else if(FieldName==TEXT("GameModeFilter"))
	{
		FieldName = TEXT("GameModes");
	}

	TArray<UUIResourceDataProvider*> Providers;
	ListElementProviders.MultiFind(FieldName, Providers);

	for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
	{
		UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));
		if( DataProvider && !DataProvider->IsFiltered() )
		{
			OutElementProviders.AddUniqueItem(DataProvider);
		}
	}
}

/* === IUIListElementProvider interface === */
/**
 * Retrieves the list of all data tags contained by this element provider which correspond to list element data.
 *
 * @return	the list of tags supported by this element provider which correspond to list element data.
 */
TArray<FName> UUDKUIDataStore_MenuItems::GetElementProviderTags()
{
	TArray<FName> Tags = Super::GetElementProviderTags();

	Tags.AddItem(TEXT("AvailableMutators"));
	Tags.AddItem(TEXT("EnabledMutators"));
	Tags.AddItem(TEXT("OfficialMutators"));
	Tags.AddItem(TEXT("MapCycle"));
	Tags.AddItem(TEXT("MapsNotInCycle"));
	Tags.AddItem(TEXT("WeaponPriority"));
	Tags.AddItem(TEXT("GameModeFilter"));

	return Tags;
}


/**
 * Returns the number of list elements associated with the data tag specified.
 *
 * @param	FieldName	the name of the property to get the element count for.  guaranteed to be one of the values returned
 *						from GetElementProviderTags.
 *
 * @return	the total number of elements that are required to fully represent the data specified.
 */
INT UUDKUIDataStore_MenuItems::GetElementCount( FName FieldName )
{
	TArray<UUDKUIResourceDataProvider*> Providers;
	GetFilteredElementProviders(FieldName, Providers);

	return Providers.Num();
}

/**
 * Retrieves the list elements associated with the data tag specified.
 *
 * @param	FieldName		the name of the property to get the element count for.  guaranteed to be one of the values returned
 *							from GetElementProviderTags.
 * @param	out_Elements	will be filled with the elements associated with the data specified by DataTag.
 *
 * @return	TRUE if this data store contains a list element data provider matching the tag specified.
 */
UBOOL UUDKUIDataStore_MenuItems::GetListElements( FName FieldName, TArray<INT>& out_Elements )
{
	UBOOL bResult = FALSE;
	TArray<UUIResourceDataProvider*> Providers;

	if(FieldName==TEXT("EnabledMutators"))
	{
		FieldName = TEXT("Mutators");
		ListElementProviders.MultiFind(FieldName, Providers);

		// Use the enabled mutators array to determine which mutators to expose
		out_Elements.Empty();
		for ( INT MutatorIndex = 0; MutatorIndex < EnabledMutators.Num(); MutatorIndex++ )
		{
			INT ProviderIndex = EnabledMutators(MutatorIndex);
			if(Providers.IsValidIndex(ProviderIndex))
			{	
				UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));
				if(DataProvider && DataProvider->IsFiltered() == FALSE)
				{
					out_Elements.AddUniqueItem(ProviderIndex);
				}
			}
		}

		bResult = TRUE;
	}
	else if(FieldName==TEXT("AvailableMutators"))
	{
		FieldName = TEXT("Mutators");
		ListElementProviders.MultiFind(FieldName, Providers);

		// Make sure the provider index isnt in the enabled mutators array.
		out_Elements.Empty();
		for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
		{
			UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));
			if( DataProvider && !DataProvider->IsFiltered() && !EnabledMutators.ContainsItem(ProviderIndex) )
			{
				out_Elements.AddUniqueItem(ProviderIndex);
			}
		}
		bResult = TRUE;
	}
	else if ( FieldName == TEXT("OfficialMutators") )
	{
		ListElementProviders.MultiFind(TEXT("Mutators"), Providers);

		out_Elements.Empty();
		for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
		{
			UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));
			if( DataProvider && !DataProvider->IsFiltered() )
			{
				out_Elements.AddUniqueItem(ProviderIndex);
			}
		}
		bResult = TRUE;
	}
	else if(FieldName==TEXT("MapCycle"))
	{
		FieldName = TEXT("Maps");
		ListElementProviders.MultiFind(FieldName, Providers);

		// Use the enabled mutators array to determine which mutators to expose
		out_Elements.Empty();
		for ( INT MapIndex = 0; MapIndex < MapCycle.Num(); MapIndex++ )
		{
			INT ProviderIndex = MapCycle(MapIndex);
			if(Providers.IsValidIndex(ProviderIndex))
			{	
				UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));
				if(DataProvider && DataProvider->IsFiltered() == FALSE)
				{
					out_Elements.AddUniqueItem(ProviderIndex);
				}
			}
		}

		bResult = TRUE;
	}
	else if(FieldName==TEXT("MapsNotInCycle"))
	{
		FieldName = TEXT("Maps");
		ListElementProviders.MultiFind(FieldName, Providers);

		// Make sure the provider index isnt in the enabled mutators array.
		out_Elements.Empty();
		for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
		{
			UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));
			if(DataProvider && DataProvider->IsFiltered() == FALSE && MapCycle.ContainsItem(ProviderIndex)==FALSE)
			{
				out_Elements.AddUniqueItem(ProviderIndex);
			}
		}
		bResult = TRUE;
	}
	else if(FieldName==TEXT("WeaponPriority"))
	{
		FieldName = TEXT("DropDownWeapons");
		ListElementProviders.MultiFind(FieldName, Providers);

		// Use the weapon priority array to determine the order of weapons.
		out_Elements.Empty();
		for ( INT WeaponIdx = 0; WeaponIdx < WeaponPriority.Num(); WeaponIdx++ )
		{
			INT ProviderIndex = WeaponPriority(WeaponIdx);
			if(Providers.IsValidIndex(ProviderIndex))
			{	
				UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));
				if( DataProvider && !DataProvider->IsFiltered() )
				{
					out_Elements.AddUniqueItem(ProviderIndex);
				}
			}
		}

		bResult = TRUE;
	}
	else if(FieldName==TEXT("GameModeFilter"))
	{
		FieldName = TEXT("GameModes");

		ListElementProviders.MultiFind(FieldName, Providers);
		if ( Providers.Num() > 0 )
		{
			out_Elements.Empty();
			for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
			{
				UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));
				if( DataProvider && !DataProvider->IsFiltered() )
				{
					out_Elements.AddUniqueItem(ProviderIndex);
				}
			}
			bResult = TRUE;
		}
	}
	else
	{
		ListElementProviders.MultiFind(FieldName, Providers);
		if ( Providers.Num() > 0 )
		{
			out_Elements.Empty();
			for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
			{
				UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));
				if( DataProvider && !DataProvider->IsFiltered() )
				{
					out_Elements.AddUniqueItem(ProviderIndex);
				}
			}
			bResult = TRUE;
		}
		else
		{
			bResult = Super::GetListElements(FieldName, out_Elements);
		}
	}

	return bResult;
}

/** 
 * Attempts to retrieve all providers with the specified provider field name.
 *
 * @param ProviderFieldName		Name of the provider set to search for
 * @param OutProviders			A set of providers with the given name
 * 
 * @return	TRUE if the set was found, FALSE otherwise.
 */
UBOOL UUDKUIDataStore_MenuItems::GetProviderSet(FName ProviderFieldName,TArray<class UUDKUIResourceDataProvider*>& OutProviders)
{
	OutProviders.Empty();
	
	TArray<UUIResourceDataProvider*> Providers;
	ListElementProviders.MultiFind(ProviderFieldName, Providers);
	for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
	{
		UUDKUIResourceDataProvider* DataProvider = Cast<UUDKUIResourceDataProvider>(Providers(ProviderIndex));

		if(DataProvider!=NULL)
		{
			OutProviders.AddUniqueItem(DataProvider);
		}
	}

	return (OutProviders.Num()>0);
}

/* === UIDataProvider interface === */
/**
 * Resolves the value of the data field specified and stores it in the output parameter.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with data collections
 *
 * @todo - not yet implemented
 */
UBOOL UUDKUIDataStore_MenuItems::GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	if(FieldName==TEXT("GameModeFilter"))
	{
		bResult = GetCellFieldValue(*FieldName, TEXT("GameModeFilter"), GameModeFilter, out_FieldValue);
	}
	else if(FieldName==TEXT("GameModeFilterClass"))
	{
		TArray<UUIResourceDataProvider*> Providers;
		ListElementProviders.MultiFind(TEXT("GameModes"), Providers);

		if(Providers.IsValidIndex(GameModeFilter))
		{
			UUDKUIResourceDataProvider* GameModeInfo = Cast<UUDKUIResourceDataProvider>(Providers(GameModeFilter));
			out_FieldValue.PropertyTag = *FieldName;
			out_FieldValue.PropertyType = DATATYPE_Property;
			out_FieldValue.StringValue = GameModeInfo->FriendlyName;
			out_FieldValue.ArrayValue.AddItem(GameModeFilter);
			bResult = TRUE;
		}
	}

	if(bResult==FALSE)
	{
		bResult = Super::GetFieldValue(FieldName, out_FieldValue, ArrayIndex);
	}

	return bResult;
}

/**
 * Gets the list of data fields exposed by this data provider.
 *
 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
 */
void UUDKUIDataStore_MenuItems::GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields )
{
	TArray<UUIResourceDataProvider*> Providers;

	/*
	out_Fields.Empty();

	for ( INT ProviderIndex = 0; ProviderIndex < ElementProviderTypes.Num(); ProviderIndex++ )
	{
		FGameResourceDataProvider& Provider = ElementProviderTypes(ProviderIndex);

		TArray<UUDKUIResourceDataProvider*> ResourceProviders;
		GetFilteredElementProviders(Provider.ProviderTag, ResourceProviders);

		// for each of the game resource providers, add a tag to allow the UI to access the list of providers
		new(out_Fields) FUIDataProviderField( Provider.ProviderTag, (TArray<UUIDataProvider*>&)ResourceProviders );
	}
	*/
	Super::GetSupportedDataFields(out_Fields);

	// Add the enabled/available mutators fields.
	Providers.Empty();
	ListElementProviders.MultiFind(TEXT("Mutators"), Providers);
	out_Fields.AddItem(FUIDataProviderField(TEXT("EnabledMutators"), (TArray<UUIDataProvider*>&)Providers));
	out_Fields.AddItem(FUIDataProviderField(TEXT("AvailableMutators"), (TArray<UUIDataProvider*>&)Providers));

	// Add the mapcycle fields.
	Providers.Empty();
	ListElementProviders.MultiFind(TEXT("Maps"), Providers);
	out_Fields.AddItem(FUIDataProviderField(TEXT("MapCycle"), (TArray<UUIDataProvider*>&)Providers));
	out_Fields.AddItem(FUIDataProviderField(TEXT("MapsNotInCycle"), (TArray<UUIDataProvider*>&)Providers));

	Providers.Empty();
	ListElementProviders.MultiFind(TEXT("GameModes"), Providers);
	out_Fields.AddItem(FUIDataProviderField(TEXT("GameModeFilter"), (TArray<UUIDataProvider*>&)Providers));
	out_Fields.AddItem(FUIDataProviderField(TEXT("GameModeFilterClass"), DATATYPE_Property));

	Providers.Empty();
	ListElementProviders.MultiFind(TEXT("DropDownWeapons"), Providers);
	out_Fields.AddItem(FUIDataProviderField(TEXT("WeaponPriority"), (TArray<UUIDataProvider*>&)Providers));
}

/**
 * Retrieves a list element for the specified data tag that can provide the list with the available cells for this list element.
 * Used by the UI editor to know which cells are available for binding to individual list cells.
 *
 * @param	FieldName		the tag of the list element data provider that we want the schema for.
 *
 * @return	a pointer to some instance of the data provider for the tag specified.  only used for enumerating the available
 *			cell bindings, so doesn't need to actually contain any data (i.e. can be the CDO for the data provider class, for example)
 */
TScriptInterface<class IUIListElementCellProvider> UUDKUIDataStore_MenuItems::GetElementCellSchemaProvider( FName FieldName )
{
	TScriptInterface<class IUIListElementCellProvider> Result;

	// Replace enabled/available mutators with the straight mutators schema.
	if(FieldName==TEXT("EnabledMutators") || FieldName==TEXT("AvailableMutators") || FieldName == TEXT("OfficialMutators") )
	{
		FieldName = TEXT("Mutators");
	}
	else if(FieldName==TEXT("MapCycle") || FieldName==TEXT("MapsNotInCycle"))
	{
		FieldName = TEXT("Maps");
	}
	else if(FieldName==TEXT("WeaponPriority"))
	{
		FieldName = TEXT("DropDownWeapons");
	}
	else if(FieldName==TEXT("GameModeFilter"))
	{
		return this;
	}

	Result = Super::GetElementCellSchemaProvider(FieldName);

	return Result;
}

/**
 * Retrieves a UIListElementCellProvider for the specified data tag that can provide the list with the values for the cells
 * of the list element indicated by CellValueProvider.DataSourceIndex
 *
 * @param	FieldName		the tag of the list element data field that we want the values for
 * @param	ListIndex		the list index for the element to get values for
 *
 * @return	a pointer to an instance of the data provider that contains the value for the data field and list index specified
 */
TScriptInterface<class IUIListElementCellProvider> UUDKUIDataStore_MenuItems::GetElementCellValueProvider( FName FieldName, INT ListIndex )
{
	TScriptInterface<class IUIListElementCellProvider> Result;

	// Replace enabled/available mutators with the straight mutators schema.
		if(FieldName==TEXT("EnabledMutators") || FieldName==TEXT("AvailableMutators") || FieldName == TEXT("OfficialMutators") )
	{
		FieldName = TEXT("Mutators");
	}
	else if(FieldName==TEXT("MapCycle") || FieldName==TEXT("MapsNotInCycle"))
	{
		FieldName = TEXT("Maps");
	}
	else if(FieldName==TEXT("WeaponPriority"))
	{
		FieldName = TEXT("DropDownWeapons");
	}
	else if(FieldName==TEXT("GameModeFilter"))
	{
		return this;
	}

	Result = Super::GetElementCellValueProvider(FieldName, ListIndex);

	return Result;
}

/**
 * Resolves the value of the data field specified and stores the value specified to the appropriate location for that field.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	FieldValue		the value to store for the property specified.
 * @param	ArrayIndex		optional array index for use with data collections
 */
UBOOL UUDKUIDataStore_MenuItems::SetFieldValue( const FString& FieldName, const struct FUIProviderScriptFieldValue& FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	if(FieldName==TEXT("GameModeFilter"))
	{
		if ( FieldValue.StringValue.Len() > 0 || FieldValue.ArrayValue.Num() > 0 )
		{
			INT NumItems = GetElementCount(*FieldName);

			for(INT ValueIdx=0; ValueIdx<NumItems; ValueIdx++)
			{
				FUIProviderFieldValue out_FieldValue(EC_EventParm);

				if(GetCellFieldValue(*FieldName, TEXT("GameModeFilter"), ValueIdx, out_FieldValue))
				{
					if ((FieldValue.StringValue.Len() > 0 && FieldValue.StringValue == out_FieldValue.StringValue)
					||	(FieldValue.StringValue.Len() == 0 && FieldValue.ArrayValue.Num() > 0 && FieldValue.ArrayValue.ContainsItem(ValueIdx)))
					{
						GameModeFilter = ValueIdx;
						bResult = TRUE;
						break;
					}
				}
			}
		}
	}

	if(bResult==FALSE)
	{
		bResult = Super::SetFieldValue(FieldName, FieldValue, ArrayIndex);
	}

	return bResult;
}

/**
 * Determines whether a member of a collection should be considered "enabled" by subscribed lists.  Disabled elements will still be displayed in the list
 * but will be drawn using the disabled state.
 *
 * @param	FieldName			the name of the collection data field that CollectionIndex indexes into.
 * @param	CollectionIndex		the index into the data field collection indicated by FieldName to check
 *
 * @return	TRUE if FieldName doesn't correspond to a valid collection data field, CollectionIndex is an invalid index for that collection,
 *			or the item is actually enabled; FALSE only if the item was successfully resolved into a data field value, but should be considered disabled.
 */
UBOOL UUDKUIDataStore_MenuItems::IsElementEnabled( FName FieldName, INT CollectionIndex )
{
	return TRUE;
}

/** @return Returns the number of providers for a given field name. */
INT UUDKUIDataStore_MenuItems::GetProviderCount(FName FieldName) const
{
	TArray<UUDKUIResourceDataProvider*> Providers;
	UUDKUIDataStore_MenuItems* MutableThis = const_cast<UUDKUIDataStore_MenuItems*>( this );
	MutableThis->GetFilteredElementProviders(FieldName, Providers);

	return Providers.Num();
}




/**
 * Returns the names of the exposed members in the first entry in the array
 * of search results
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param OutCellTags the columns supported by this row
 */
void UUDKUIDataStore_MenuItems::GetElementCellTags( FName FieldName, TMap<FName,FString>& out_CellTags )
{
	static UProperty* GameModeFilterProperty = FindField<UProperty>(GetClass(),TEXT("GameModeFilter"));
	checkSlow(GameModeFilterProperty);

	out_CellTags.Set(TEXT("GameModeFilter"), *GameModeFilterProperty->GetFriendlyName(GetClass()));
}

/**
 * Retrieves the field type for the specified cell.
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	CellTag				the tag for the element cell to get the field type for
 * @param	out_CellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
 *
 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
 */
UBOOL UUDKUIDataStore_MenuItems::GetCellFieldType( FName FieldName, const FName& CellTag, BYTE& out_CellFieldType )
{
	return DATATYPE_Property;
}

/**
 * Resolves the value of the cell specified by CellTag and stores it in the output parameter.
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the UIList's item index for the element that contains this cell.  Useful for data providers which
 *							do not provide unique UIListElement objects for each element.
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with cell tags that represent data collections.  Corresponds to the
 *							ArrayIndex of the collection that this cell is bound to, or INDEX_NONE if CellTag does not correspond
 *							to a data collection.
 */
UBOOL UUDKUIDataStore_MenuItems::GetCellFieldValue( FName FieldName, const FName& CellTag, INT ListIndex, FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;
	TArray<UUIResourceDataProvider*> Providers;
	Providers.Empty();
	FString FieldToFind=CellTag.ToString();
	ListElementProviders.MultiFind(TEXT("GameModes"), Providers);

	if(CellTag==TEXT("GameModeFilter"))
	{
		FieldToFind=TEXT("FriendlyName");
	}
	
	if(Providers.IsValidIndex(ListIndex))
	{
		UUDKUIResourceDataProvider* GameMode = Cast<UUDKUIResourceDataProvider>(Providers(ListIndex));
		bResult = GameMode->GetFieldValue(FieldToFind, out_FieldValue);
		if ( bResult && out_FieldValue.StringValue.Len() > 0 )
		{
			out_FieldValue.ArrayValue.AddItem(ListIndex);
		}
	}

	return bResult;
}

/**
 * Attempts to find the index of a provider given a provider field name, a search tag, and a value to match.
 *
 * @return	Returns the index of the provider or INDEX_NONE if the provider wasn't found.
 */
INT UUDKUIDataStore_MenuItems::FindValueInProviderSet(FName ProviderFieldName,FName SearchTag,const FString& SearchValue)
{
	INT Result = INDEX_NONE;
	TArray<INT> PossibleItems;
	GetListElements(ProviderFieldName, PossibleItems);

	for(INT ItemIdx=0; ItemIdx<PossibleItems.Num(); ItemIdx++)
	{
		FUIProviderFieldValue OutValue(EC_EventParm);
		TScriptInterface<IUIListElementCellProvider> DataProvider = GetElementCellValueProvider(ProviderFieldName, PossibleItems(ItemIdx));

		if ( DataProvider && DataProvider->GetCellFieldValue(ProviderFieldName,SearchTag, PossibleItems(ItemIdx), OutValue))
		{
			if(SearchValue==OutValue.StringValue)
			{
				Result = PossibleItems(ItemIdx);
				break;
			}
		}
	}

	return Result;
}


/**
 * @return	Returns whether or not the specified provider is filtered or not.
 */
UBOOL UUDKUIDataStore_MenuItems::IsProviderFiltered(FName ProviderFieldName, INT ProviderIdx)
{
	UBOOL bResult = TRUE;

	TScriptInterface<IUIListElementCellProvider> DataProvider = GetElementCellValueProvider(ProviderFieldName, ProviderIdx);
	UUDKUIResourceDataProvider* UTDataProvider = Cast<UUDKUIResourceDataProvider>(DataProvider->GetUObjectInterfaceUIListElementCellProvider());
	if(UTDataProvider)
	{
		bResult = UTDataProvider->IsFiltered();
	}

	return bResult;
}

/**
 * Attempts to find the value of a provider given a provider cell field.
 *
 * @return	Returns true if the value was found, false otherwise.
 */
UBOOL UUDKUIDataStore_MenuItems::GetValueFromProviderSet(FName ProviderFieldName,FName SearchTag,INT ListIndex,FString& OutValue)
{
	UBOOL bResult = FALSE;
	FUIProviderFieldValue FieldValue(EC_EventParm);
	TScriptInterface<IUIListElementCellProvider> DataProvider = GetElementCellValueProvider(ProviderFieldName, ListIndex);

	if( DataProvider && DataProvider->GetCellFieldValue(ProviderFieldName,SearchTag, ListIndex, FieldValue))
	{
		OutValue = FieldValue.StringValue;
		bResult = TRUE;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
// UUDKUIDataStore_Options
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIDataStore_Options);


/**
 * Called when this data store is added to the data store manager's list of active data stores.
 *
 * Initializes the ListElementProviders map
 *
 * @param	PlayerOwner		the player that will be associated with this DataStore.  Only relevant if this data store is
 *							associated with a particular player; NULL if this is a global data store.
 */
void UUDKUIDataStore_Options::OnRegister( ULocalPlayer* PlayerOwner )
{
	Super::OnRegister(PlayerOwner);

	// Initialize all of the option providers, go backwards because of the way MultiMap appends items.
	TArray<UUIResourceDataProvider*> Providers;
	ListElementProviders.MultiFind(TEXT("OptionSets"), Providers);

	for ( INT ProviderIndex = Providers.Num()-1; ProviderIndex >= 0; ProviderIndex-- )
	{
		UUDKUIDataProvider_MenuOption* DataProvider = Cast<UUDKUIDataProvider_MenuOption>(Providers(ProviderIndex));
		if(DataProvider)
		{
			for (INT OptionIndex=0;OptionIndex<DataProvider->OptionSet.Num();OptionIndex++)
			{
				OptionProviders.Add(DataProvider->OptionSet(OptionIndex), DataProvider);
			}
		}
	}
}

/** 
 * Gets the list of element providers for a fieldname with filtered elements removed.
 *
 * @param FieldName				Fieldname to use to search for providers.
 * @param OutElementProviders	Array to store providers in.
 */
void UUDKUIDataStore_Options::GetFilteredElementProviders(FName FieldName, TArray<UUDKUIResourceDataProvider*> &OutElementProviders)
{
	OutElementProviders.Empty();

	TArray<UUDKUIResourceDataProvider*> Providers;
	OptionProviders.MultiFind(FieldName, Providers);

	for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
	{
		UUDKUIResourceDataProvider* DataProvider = Providers(ProviderIndex);
		if(DataProvider && DataProvider->IsFiltered() == FALSE)
		{
			OutElementProviders.AddUniqueItem(DataProvider);
		}
	}
}

/* === IUIListElementProvider interface === */
/**
 * Retrieves the list of all data tags contained by this element provider which correspond to list element data.
 *
 * @return	the list of tags supported by this element provider which correspond to list element data.
 */
TArray<FName> UUDKUIDataStore_Options::GetElementProviderTags()
{
	TLookupMap<FName> Keys;
	TArray<FName> OptionSets;
	OptionProviders.GetKeys(Keys);

	for(INT OptionIdx=0; OptionIdx<Keys.Num(); OptionIdx++)
	{
		OptionSets.AddUniqueItem(Keys(OptionIdx));
	}

	return OptionSets;
}


/**
 * Returns the number of list elements associated with the data tag specified.
 *
 * @param	FieldName	the name of the property to get the element count for.  guaranteed to be one of the values returned
 *						from GetElementProviderTags.
 *
 * @return	the total number of elements that are required to fully represent the data specified.
 */
INT UUDKUIDataStore_Options::GetElementCount( FName FieldName )
{
	TArray<UUDKUIResourceDataProvider*> Providers;
	GetFilteredElementProviders(FieldName, Providers);

	return Providers.Num();
}

/**
 * Retrieves the list elements associated with the data tag specified.
 *
 * @param	FieldName		the name of the property to get the element count for.  guaranteed to be one of the values returned
 *							from GetElementProviderTags.
 * @param	out_Elements	will be filled with the elements associated with the data specified by DataTag.
 *
 * @return	TRUE if this data store contains a list element data provider matching the tag specified.
 */
UBOOL UUDKUIDataStore_Options::GetListElements( FName FieldName, TArray<INT>& out_Elements )
{
	UBOOL bResult = FALSE;

	TArray<UUDKUIResourceDataProvider*> Providers;
	OptionProviders.MultiFind(FieldName, Providers);

	if ( Providers.Num() > 0 )
	{
		out_Elements.Empty();
		for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
		{
			UUDKUIResourceDataProvider* DataProvider = Providers(ProviderIndex);
			if(DataProvider && DataProvider->IsFiltered() == FALSE)
			{
				out_Elements.AddUniqueItem(ProviderIndex);
			}
		}
		bResult = TRUE;
	}

	return bResult;
}

/* === UIDataProvider interface === */
/**
 * Resolves the value of the data field specified and stores it in the output parameter.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with data collections
 *
 * @todo - not yet implemented
 */
UBOOL UUDKUIDataStore_Options::GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	return Super::GetFieldValue(FieldName, out_FieldValue, ArrayIndex);
}

/**
 * Gets the list of data fields exposed by this data provider.
 *
 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
 */
void UUDKUIDataStore_Options::GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields )
{
	out_Fields.Empty();

	TLookupMap<FName> OptionSets;
	OptionProviders.GetKeys(OptionSets);

	for ( INT ProviderIndex = 0; ProviderIndex < OptionSets.Num(); ProviderIndex++ )
	{
		FName ProviderTag = *OptionSets(ProviderIndex);

		TArray<UUDKUIResourceDataProvider*> ResourceProviders;
		OptionProviders.MultiFind(ProviderTag, ResourceProviders);

		// for each of the game resource providers, add a tag to allow the UI to access the list of providers
		new(out_Fields) FUIDataProviderField( ProviderTag, (TArray<UUIDataProvider*>&)ResourceProviders );
	}
}

/**
 * Retrieves a list element for the specified data tag that can provide the list with the available cells for this list element.
 * Used by the UI editor to know which cells are available for binding to individual list cells.
 *
 * @param	FieldName		the tag of the list element data provider that we want the schema for.
 *
 * @return	a pointer to some instance of the data provider for the tag specified.  only used for enumerating the available
 *			cell bindings, so doesn't need to actually contain any data (i.e. can be the CDO for the data provider class, for example)
 */
TScriptInterface<class IUIListElementCellProvider> UUDKUIDataStore_Options::GetElementCellSchemaProvider( FName FieldName )
{
	TScriptInterface<IUIListElementCellProvider> Result;

	// search for the provider that has the matching tag
	INT ProviderIndex = FindProviderTypeIndex(TEXT("OptionSets"));
	if ( ProviderIndex != INDEX_NONE )
	{
		FGameResourceDataProvider& Provider = ElementProviderTypes(ProviderIndex);
		if ( Provider.ProviderClass != NULL )
		{
			Result = Provider.ProviderClass->GetDefaultObject<UUIResourceDataProvider>();
		}
	}

	return Result;
}

/**
 * Retrieves a UIListElementCellProvider for the specified data tag that can provide the list with the values for the cells
 * of the list element indicated by CellValueProvider.DataSourceIndex
 *
 * @param	FieldName		the tag of the list element data field that we want the values for
 * @param	ListIndex		the list index for the element to get values for
 *
 * @return	a pointer to an instance of the data provider that contains the value for the data field and list index specified
 */
TScriptInterface<class IUIListElementCellProvider> UUDKUIDataStore_Options::GetElementCellValueProvider( FName FieldName, INT ListIndex )
{
	TScriptInterface<class IUIListElementCellProvider> Result;
	
	// search for the provider that has the matching tag
	TArray<UUDKUIResourceDataProvider*> Providers;
	OptionProviders.MultiFind(FieldName, Providers);

	if(Providers.IsValidIndex(ListIndex))
	{
		Result = Providers(ListIndex);
	}

	return Result;
}

/**
 * Resolves the value of the data field specified and stores the value specified to the appropriate location for that field.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	FieldValue		the value to store for the property specified.
 * @param	ArrayIndex		optional array index for use with data collections
 */
UBOOL UUDKUIDataStore_Options::SetFieldValue( const FString& FieldName, const struct FUIProviderScriptFieldValue& FieldValue, INT ArrayIndex )
{
	return Super::SetFieldValue(FieldName, FieldValue, ArrayIndex);
}

/**
 * Determines whether a member of a collection should be considered "enabled" by subscribed lists.  Disabled elements will still be displayed in the list
 * but will be drawn using the disabled state.
 *
 * @param	FieldName			the name of the collection data field that CollectionIndex indexes into.
 * @param	CollectionIndex		the index into the data field collection indicated by FieldName to check
 *
 * @return	TRUE if FieldName doesn't correspond to a valid collection data field, CollectionIndex is an invalid index for that collection,
 *			or the item is actually enabled; FALSE only if the item was successfully resolved into a data field value, but should be considered disabled.
 */
UBOOL UUDKUIDataStore_Options::IsElementEnabled( FName FieldName, INT CollectionIndex )
{
	return TRUE;
}

/** 
 * Clears all options in the specified set.
 *
 * @param SetName		Set to clear
 */
void UUDKUIDataStore_Options::ClearSet(FName SetName)
{
	TArray<UUDKUIResourceDataProvider*> Providers;
	OptionProviders.MultiFind(SetName, Providers);

	for(INT ProviderIdx=0; ProviderIdx<Providers.Num(); ProviderIdx++)
	{
		DynamicProviders.RemoveItem(Providers(ProviderIdx));
	}

	// Remove teh key
	OptionProviders.RemoveKey(SetName);
}

/** 
 * Appends N amount of providers to the specified set.
 *
 * @param SetName		Set to append to
 * @param NumOptions	Number of options to append
 */
void UUDKUIDataStore_Options::AppendToSet(FName SetName, INT NumOptions)
{
	for(INT AddIdx=0; AddIdx<NumOptions; AddIdx++)
	{
		UUDKUIDataProvider_MenuOption* NewProvider = ConstructObject<UUDKUIDataProvider_MenuOption>(UUDKUIDataProvider_MenuOption::StaticClass(), this);
		OptionProviders.Add(SetName, NewProvider);
		DynamicProviders.AddUniqueItem(NewProvider);
	}
}

/**
 * Retrieves a set of option providers.
 *
 * @param SetName	Set to retreieve
 * 
 * @return Array of dataproviders for all the options in the set.
 */
void UUDKUIDataStore_Options::GetSet(FName SetName, TArray<UUDKUIResourceDataProvider*> &Providers)
{
	Providers.Empty();
	OptionProviders.MultiFind(SetName, Providers);
}

//////////////////////////////////////////////////////////////////////////
// UUDKDataStore_StringList
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(UUDKUIDataStore_StringList);

/**
 * @param FieldName		Name of the String List to find
 * @return the index of a string list
 */

INT UUDKUIDataStore_StringList::GetFieldIndex(FName FieldName)
{
	INT Result = INDEX_NONE;
	for (INT i=0;i<StringData.Num();i++)
	{
		if (StringData(i).Tag == FieldName)
		{
			Result = i;
			break;
		}
	}
	return Result;
}

/**
 * Retrieves the list of all data tags contained by this element provider which correspond to list element data.
 *
 * @return	the list of tags supported by this element provider which correspond to list element data.
 */
TArray<FName> UUDKUIDataStore_StringList::GetElementProviderTags()
{
	TArray<FName> Result;
	for (INT i=0;i<StringData.Num();i++)
	{
		Result.AddUniqueItem(StringData(i).Tag);
	}
	return Result;
}

/**
 * Returns the number of list elements associated with the data tag specified.
 *
 * @param	FieldName	the name of the property to get the element count for.  guaranteed to be one of the values returned
 *						from GetElementProviderTags.
 *
 * @return	the total number of elements that are required to fully represent the data specified.
 */
INT UUDKUIDataStore_StringList::GetElementCount( FName FieldName )
{
	INT Result = 0;
	const INT FieldIndex = GetFieldIndex(FieldName);
	if ( StringData.IsValidIndex(FieldIndex) )
	{
		Result = StringData(FieldIndex).Strings.Num();
	}
	return Result;
}

/**
 * Retrieves the list elements associated with the data tag specified.
 *
 * @param	FieldName		the name of the property to get the element count for.  guaranteed to be one of the values returned
 *							from GetElementProviderTags.
 * @param	out_Elements	will be filled with the elements associated with the data specified by DataTag.
 *
 * @return	TRUE if this data store contains a list element data provider matching the tag specified.
 */
UBOOL UUDKUIDataStore_StringList::GetListElements( FName FieldName, TArray<INT>& out_Elements )
{
	UBOOL bResult = FALSE;

	const INT FieldIndex = GetFieldIndex(FieldName);
	if ( StringData.IsValidIndex(FieldIndex) )
	{
		for (INT i=0;i<StringData(FieldIndex).Strings.Num();i++)
		{
			out_Elements.AddUniqueItem(i);
		}
		bResult = TRUE;
	}
	return bResult;
}

/**
 * Retrieves a UIListElementCellProvider for the specified data tag that can provide the list with the available cells for this list element.
 * Used by the UI editor to know which cells are available for binding to individual list cells.
 *
 * @param	FieldName		the tag of the list element data field that we want the schema for.
 *
 * @return	a pointer to some instance of the data provider for the tag specified.  only used for enumerating the available
 *			cell bindings, so doesn't need to actually contain any data (i.e. can be the CDO for the data provider class, for example)
 */
TScriptInterface<class IUIListElementCellProvider> UUDKUIDataStore_StringList::GetElementCellSchemaProvider( FName FieldName )
{
	const INT FieldIndex = GetFieldIndex(FieldName);
	if ( StringData.IsValidIndex(FieldIndex) )
	{
		// Create a dataprovider for this string data object if one doesn't already exist.
		if(StringData(FieldIndex).DataProvider==NULL)
		{
			StringData(FieldIndex).DataProvider=ConstructObject<UUDKUIDataProvider_StringArray>(UUDKUIDataProvider_StringArray::StaticClass());
		}
		StringData(FieldIndex).DataProvider->Strings = StringData(FieldIndex).Strings;
		

		return TScriptInterface<IUIListElementCellProvider>(StringData(FieldIndex).DataProvider);
	}
	return TScriptInterface<IUIListElementCellProvider>();
}

/**
 * Retrieves a UIListElementCellProvider for the specified data tag that can provide the list with the values for the cells
 * of the list element indicated by CellValueProvider.DataSourceIndex
 *
 * @param	FieldName		the tag of the list element data field that we want the values for
 * @param	ListIndex		the list index for the element to get values for
 *
 * @return	a pointer to an instance of the data provider that contains the value for the data field and list index specified
 */
TScriptInterface<class IUIListElementCellProvider> UUDKUIDataStore_StringList::GetElementCellValueProvider( FName FieldName, INT ListIndex )
{
	const INT FieldIndex = GetFieldIndex(FieldName);
	if ( StringData.IsValidIndex(FieldIndex) )
	{
		// Create a dataprovider for this string data object if one doesn't already exist.
		if(StringData(FieldIndex).DataProvider==NULL)
		{
			StringData(FieldIndex).DataProvider=ConstructObject<UUDKUIDataProvider_StringArray>(UUDKUIDataProvider_StringArray::StaticClass());
		}
		StringData(FieldIndex).DataProvider->Strings = StringData(FieldIndex).Strings;

		return TScriptInterface<IUIListElementCellProvider>(StringData(FieldIndex).DataProvider);
	}
	return TScriptInterface<IUIListElementCellProvider>();
}

TScriptInterface<class IUIListElementProvider> UUDKUIDataStore_StringList::ResolveListElementProvider( const FString& PropertyName ) 
{
	return TScriptInterface<IUIListElementProvider> (this);
}

/* === UIListElementCellProvider === */

/**
 * Retrieves the list of tags that can be bound to individual cells in a single list element, along with the human-readable,
 * localized string that should be used in the header for each cell tag (in lists which have column headers enabled).
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	out_CellTags	receives the list of tag/column headers that can be bound to element cells for the specified property.
 */
void UUDKUIDataStore_StringList::GetElementCellTags( FName FieldName, TMap<FName,FString>& out_CellTags )
{
	out_CellTags.Empty();
	for ( INT DataIndex = 0; DataIndex < StringData.Num(); DataIndex++ )
	{
		FEStringListData& ListData = StringData(DataIndex);
		out_CellTags.Set( ListData.Tag, ListData.ColumnHeaderText.Len() > 0 ? *ListData.ColumnHeaderText : *ListData.Tag.ToString() );			
	}
}

//	virtual void GetElementCellTags( TMap<FName,FString>& out_CellTags )=0;

/**
 * Retrieves the field type for the specified cell.
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	CellTag				the tag for the element cell to get the field type for
 * @param	out_CellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
 *
 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
 */
UBOOL UUDKUIDataStore_StringList::GetCellFieldType( FName FieldName, const FName& CellTag, BYTE& out_CellFieldType )
{
	return DATATYPE_Property;
}

/**
 * Resolves the value of the cell specified by CellTag and stores it in the output parameter.
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the UIList's item index for the element that contains this cell.  Useful for data providers which
 *							do not provide unique UIListElement objects for each element.
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with cell tags that represent data collections.  Corresponds to the
 *							ArrayIndex of the collection that this cell is bound to, or INDEX_NONE if CellTag does not correspond
 *							to a data collection.
 */
UBOOL UUDKUIDataStore_StringList::GetCellFieldValue( FName FieldName, const FName& CellTag, INT ListIndex, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex)
{
	UBOOL bResult = FALSE;

	const INT FieldIndex = GetFieldIndex(CellTag);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		if (FieldIndex < StringData.Num() && ListIndex < StringData(FieldIndex).Strings.Num() )
		{
			out_FieldValue.StringValue = StringData(FieldIndex).Strings(ListIndex);
		}
		else
		{
			out_FieldValue.StringValue = TEXT("");
		}
		out_FieldValue.PropertyType = DATATYPE_Property;
		bResult = TRUE;
	}
	return bResult;
}


/* === UIDataProvider interface === */
/**
 * Gets the list of data fields exposed by this data provider.
 *
 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
 */
void UUDKUIDataStore_StringList::GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields )
{
	for (INT i=0;i<StringData.Num();i++)
	{
		new(out_Fields) FUIDataProviderField( StringData(i).Tag, DATATYPE_Collection );			
	}
}

/**
 * Resolves the value of the data field specified and stores it in the output parameter.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with data collections
 */
UBOOL UUDKUIDataStore_StringList::GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	const INT FieldIndex = GetFieldIndex(FName(*FieldName));

	if ( StringData.IsValidIndex(FieldIndex) )
	{
		FEStringListData& StringListItem = StringData(FieldIndex);

		out_FieldValue.PropertyTag = *FieldName;
		out_FieldValue.PropertyType = DATATYPE_Property;
		out_FieldValue.StringValue = StringListItem.CurrentValue;

		// fill in ArrayValue for lists.
		if ( out_FieldValue.StringValue.Len() > 0 )
		{
			INT ValueIndex = StringListItem.Strings.FindItemIndex(out_FieldValue.StringValue);
			if ( ValueIndex != INDEX_NONE )
			{
				out_FieldValue.ArrayValue.AddItem(ValueIndex);
			}
		}
		bResult = TRUE;
	}
	return bResult;
}

/**
 * Resolves the value of the data field specified and stores the value specified to the appropriate location for that field.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	FieldValue		the value to store for the property specified.
 * @param	ArrayIndex		optional array index for use with data collections
 */
UBOOL UUDKUIDataStore_StringList::SetFieldValue( const FString& FieldName, const struct FUIProviderScriptFieldValue& FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;
	const INT FieldIndex = GetFieldIndex(FName(*FieldName));

	if ( StringData.IsValidIndex(FieldIndex) )
	{
		if ( FieldValue.ArrayValue.Num()==0 && FieldValue.StringValue.Len() )
		{
			StringData(FieldIndex).CurrentValue = FieldValue.StringValue;
			bResult = TRUE;
		}
		else if ( FieldValue.ArrayValue.Num() > 0 && StringData(FieldIndex).Strings.IsValidIndex(FieldValue.ArrayValue(0)) )
		{
			StringData(FieldIndex).CurrentValue = StringData(FieldIndex).Strings(FieldValue.ArrayValue(0));
			bResult = TRUE;
		}
	}
	return bResult;
}

/**
 * Adds a new field to the list
 *
 * @param	FieldName		the data field to resolve the value for
 * @param	NewString		The first string to add.
 * @param	bBatchOp		if TRUE, doesn't call RefreshSubscribers()
 * @returns the index of the new field
 */

INT UUDKUIDataStore_StringList::AddNewField(FName FieldName, const FString &NewString, UBOOL bBatchOp/*=FALSE*/)
{
	FEStringListData* NewData = new(StringData) FEStringListData(EC_EventParm);

	NewData->Tag = FieldName;
	NewData->CurrentValue = NewString;
	new(NewData->Strings) FString(NewString);

	if ( !bBatchOp )
	{
		eventRefreshSubscribers(FieldName);
	}

	return StringData.Num() - 1;
}


/**
 * Add a string to the list
 *
 * @Param FieldName		The string list to work on
 * @Param NewString		The new string to add
 * @param bBatchOp		if TRUE, doesn't call RefreshSubscribers()
 */

void UUDKUIDataStore_StringList::AddStr(FName FieldName, const FString &NewString, UBOOL bBatchOp/*=FALSE*/)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( StringData.IsValidIndex(FieldIndex) )
	{
		StringData(FieldIndex).Strings.AddItem(NewString);
		if ( StringData(FieldIndex).Strings.Num() == 1 )
		{
			StringData(FieldIndex).CurrentValue = NewString;
		}

		if ( !bBatchOp )
		{
			eventRefreshSubscribers(FieldName);
		}
	}
	else	// Create a new list and prime it
	{
		AddNewField(FieldName, NewString, bBatchOp);
	}
}

/**
 * Insert a string in to the list at a given index
 *
 * @Param FieldName		The string list to work on
 * @Param NewString		The new string to add
 * @Param InsertIndex	The index where you wish to insert the string
 * @param bBatchOp		if TRUE, doesn't call RefreshSubscribers()
 */

void UUDKUIDataStore_StringList::InsertStr(FName FieldName, const FString &NewString, INT InsertIndex, UBOOL bBatchOp/*=FALSE*/)
{
	INT FieldIndex;
	
	// See if we can find this field.

	FieldIndex = GetFieldIndex(FieldName);

	if ( StringData.IsValidIndex(FieldIndex) )  // Found it, add the string
	{
		// Don't duplicate the strings

		INT StrIndex;
		if ( !StringData(FieldIndex).Strings.FindItem(NewString, StrIndex) )
		{
			StringData(FieldIndex).Strings.InsertItem(NewString, InsertIndex);
		}

		if ( !bBatchOp )
		{
			eventRefreshSubscribers(FieldName);
		}
	}
	else	// Create a new list and prime it
	{
		AddNewField(FieldName, NewString, bBatchOp);
	}
}



/**
 * Remove a string from the list
 *
 * @Param FieldName		The string list to work on
 * @Param StringToRemove 	The string to remove
 * @param bBatchOp		if TRUE, doesn't call RefreshSubscribers()
 */

void UUDKUIDataStore_StringList::RemoveStr(FName FieldName, const FString &StringToRemove, UBOOL bBatchOp/*=FALSE*/)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( StringData.IsValidIndex(FieldIndex) )
	{
		StringData(FieldIndex).Strings.RemoveItem(StringToRemove);
	}

	if ( !bBatchOp )
	{
		eventRefreshSubscribers(FieldName);
	}
}

/**
 * Remove a string (or multiple strings) by the index.
 *
 * @Param FieldName		The string list to work on
 * @Param Index			The index to remove
 * @Param Count			<Optional> # of strings to remove
 * @param bBatchOp		if TRUE, doesn't call RefreshSubscribers()
 */

void UUDKUIDataStore_StringList::RemoveStrByIndex(FName FieldName, INT Index, INT Count, UBOOL bBatchOp/*=FALSE*/)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if (StringData.IsValidIndex(FieldIndex)
	&&	StringData(FieldIndex).Strings.IsValidIndex(Index))
	{
		StringData(FieldIndex).Strings.Remove(Index, Count);
	}
	if ( !bBatchOp )
	{
		eventRefreshSubscribers(FieldName);
	}
}

/**
 * Empty a string List
 *
 * @Param FieldName		The string list to work on
 * @param bBatchOp		if TRUE, doesn't call RefreshSubscribers()
 */

void UUDKUIDataStore_StringList::Empty(FName FieldName, UBOOL bBatchOp/*=FALSE*/)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( StringData.IsValidIndex(FieldIndex) )
	{
		StringData(FieldIndex).Strings.Empty();
	}

	if ( !bBatchOp )
	{
		eventRefreshSubscribers(FieldName);
	}
}

/**
 * Finds a string in the list
 *
 * @Param FieldName		The string list to add the new string to
 * @Param SearchStr		The string to find
 *
 * @returns the index in the list or INVALIDFIELD
 */

INT UUDKUIDataStore_StringList::FindStr(FName FieldName, const FString &SearchString)
{
	INT Result = UCONST_INVALIDFIELD;

	const INT FieldIndex = GetFieldIndex(FieldName);
	if ( StringData.IsValidIndex(FieldIndex) )
	{	
		Result = StringData(FieldIndex).Strings.FindItemIndex(SearchString);
	}

	return Result;
}

/**
 * Returns the a string by the index
 *
 * @Param FieldName		The string list to add the new string to
 * @Param StrIndex		The index of the string to get
 *
 * @returns the string.
 */

FString UUDKUIDataStore_StringList::GetStr(FName FieldName, INT StrIndex)
{
	FString Result;

	const INT FieldIndex = GetFieldIndex(FieldName);
	if (StringData.IsValidIndex(FieldIndex)
	&&	StringData(FieldIndex).Strings.IsValidIndex(StrIndex))
	{	
		Result = StringData(FieldIndex).Strings(StrIndex);
	}

	return Result;
}

/**
 * Get a list
 *
 * @Param FieldName		The string list to add the new string to
 * @returns a copy of the list
 */

TArray<FString> UUDKUIDataStore_StringList::GetList(FName FieldName)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( StringData.IsValidIndex(FieldIndex) )
	{	
		return StringData(FieldIndex).Strings;		
	}
	else	// Create a new list and prime it
	{
		FieldIndex = AddNewField(FieldName, TEXT("") );
		return StringData(FieldIndex).Strings;
	}

	return TArray<FString>();
}


//////////////////////////////////////////////////////////////////////////
// UUDKUIDataProvider_StringArray
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIDataProvider_StringArray)


/** @return Returns the number of elements(rows) provided. */
INT UUDKUIDataProvider_StringArray::GetElementCount()
{
	return Strings.Num();
}

// IUIListElement interface

/**
 * Returns the names of the exposed members in the first entry in the array
 * of search results
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param OutCellTags the columns supported by this row
 */
void UUDKUIDataProvider_StringArray::GetElementCellTags( FName FieldName, TMap<FName,FString>& OutCellTags )
{
	OutCellTags.Empty();
	OutCellTags.Set(TEXT("Strings"),TEXT("Strings"));
}

/**
 * Resolves the value of the cell specified by CellTag and stores it in the output parameter.
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the index of the value to fetch
 * @param	OutFieldValue	receives the resolved value for the property specified.
 * @param	ArrayIndex		ignored
 */
UBOOL UUDKUIDataProvider_StringArray::GetCellFieldValue(FName FieldName, const FName& CellTag,INT ListIndex,FUIProviderFieldValue& OutFieldValue,INT)
{
	UBOOL bResult = FALSE;
	FOnlineStatsRow StatsRow(EC_EventParm);

	
	if(ListIndex < Strings.Num())
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = Strings(ListIndex);
		bResult = TRUE;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
// UUDKUIDataProvider_SimpleElementProvider
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUDKUIDataProvider_SimpleElementProvider)


/** @return Returns the number of elements(rows) provided. */
INT UUDKUIDataProvider_SimpleElementProvider::GetElementCount()
{
	return 0;
}

// IUIListElement interface

/**
 * Returns the names of the exposed members in the first entry in the array
 * of search results
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param OutCellTags the columns supported by this row
 */
void UUDKUIDataProvider_SimpleElementProvider::GetElementCellTags( FName FieldName, TMap<FName,FString>& OutCellTags )
{
	OutCellTags.Empty();
}

/**
 * Retrieves the field type for the specified cell.
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	CellTag				the tag for the element cell to get the field type for
 * @param	OutCellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
 *
 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
 */
UBOOL UUDKUIDataProvider_SimpleElementProvider::GetCellFieldType(FName FieldName, const FName& CellTag,BYTE& OutCellFieldType)
{
	TMap<FName,FString> CellTags;
	GetElementCellTags(FieldName,CellTags);
	UBOOL bResult = FALSE;

	if(CellTags.Find(CellTag) != NULL)
	{
		OutCellFieldType = DATATYPE_Property;
		bResult = TRUE;
	}
	

	return bResult;
}

/**
 * Resolves the value of the cell specified by CellTag and stores it in the output parameter.
 *
 * @param	FieldName		the name of the field the desired cell tags are associated with.  Used for cases where a single data provider
 *							instance provides element cells for multiple collection data fields.
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the index of the value to fetch
 * @param	OutFieldValue	receives the resolved value for the property specified.
 * @param	ArrayIndex		ignored
 */
UBOOL UUDKUIDataProvider_SimpleElementProvider::GetCellFieldValue(FName FieldName, const FName& CellTag,INT ListIndex,FUIProviderFieldValue& OutFieldValue,INT)
{
	return FALSE;
}


/* === UIDataProvider interface === */
/**
 * Gets the list of data fields exposed by this data provider.
 *
 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
 */
void UUDKUIDataProvider_SimpleElementProvider::GetSupportedDataFields( TArray<FUIDataProviderField>& out_Fields )
{
	out_Fields.Empty();

	TMap<FName,FString> Elements;
	GetElementCellTags(UCONST_UnknownCellDataFieldName,Elements);

	TArray<FName> ElementTags;
	Elements.GenerateKeyArray(ElementTags);

	for(INT TagIdx=0; TagIdx<ElementTags.Num(); TagIdx++)
	{
		// for each property contained by this ResourceDataProvider, add a provider field to expose them to the UI
		new(out_Fields) FUIDataProviderField( ElementTags(TagIdx), DATATYPE_Property );
	}
}

/**
 * Resolves the value of the data cell specified by FieldName and stores it in the output parameter.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with data collections
 */
UBOOL UUDKUIDataProvider_SimpleElementProvider::GetFieldValue( const FString& FieldName, FUIProviderFieldValue& out_FieldValue, INT ArrayIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	FName PropertyFName(*FieldName, FNAME_Find);
	if ( PropertyFName != NAME_None )
	{
		bResult = GetCellFieldValue(UCONST_UnknownCellDataFieldName,PropertyFName,ArrayIndex,out_FieldValue,INDEX_NONE);
	}

	return bResult;
}

/* 
 * Given a string containing some amount of time in seconds, 
 * convert to a string of the form HHHH:MM:SS 
 */
FString ConvertSecondsToFormattedString(const FString& SecondsString)
{
	INT Hours = 0;
	INT Minutes = 0;
	INT SecondsToConvert = Clamp(appAtoi(*SecondsString), 0, 9999 * 3600 + 59 * 60 + 59); //Clamp to 9999:59:59

	if (SecondsToConvert > 0)
	{
		//Slice up the seconds
		Hours = SecondsToConvert / 3600;
		SecondsToConvert = SecondsToConvert % 3600;
		if (SecondsToConvert > 0)
		{
			Minutes = SecondsToConvert / 60;
			SecondsToConvert = SecondsToConvert % 60;
		}
	}
	   
	//debugf(TEXT("Converted %s to %s"), *SecondsString, *FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, SecondsToConvert));
	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, SecondsToConvert);
}


