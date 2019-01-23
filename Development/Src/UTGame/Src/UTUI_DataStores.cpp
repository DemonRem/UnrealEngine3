/**
 * UTUI_DataStores.cpp: Implementation file for all UT3 datastore classes.
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "UTGameUIClasses.h"
#include "UTGameUIFrontEndClasses.h"

IMPLEMENT_CLASS(UUTUIDataProvider_GameModeInfo);
IMPLEMENT_CLASS(UUTUIDataProvider_DemoFile);
IMPLEMENT_CLASS(UUTUIDataProvider_MainMenuItems);
IMPLEMENT_CLASS(UUTUIDataProvider_KeyBinding);
IMPLEMENT_CLASS(UUTUIDataProvider_MultiplayerMenuItem);
IMPLEMENT_CLASS(UUTUIDataProvider_CommunityMenuItem);
IMPLEMENT_CLASS(UUTUIDataProvider_SettingsMenuItem);
IMPLEMENT_CLASS(UUTUIDataProvider_Weapon);


//////////////////////////////////////////////////////////////////////////
// UUTUIDataStore_StringAliasMap
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataStore_StringAliasMap);

/**
 * Set MappedString to be the localized string using the FieldName as a key
 * Returns the index into the mapped string array of where it was found.
 */
INT UUTUIDataStore_StringAliasMap::GetStringWithFieldName( const FString& FieldName, FString& MappedString )
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
#else 
	#if PS3
		SetName = TEXT("PS3");
	#else
		SetName = TEXT("PC");
	#endif
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
// UUTUIResourceDataProvider
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIResourceDataProvider);

/** @return Whether or not we should be filtered out of the list results. */
UBOOL UUTUIResourceDataProvider::IsFiltered()
{
#if !CONSOLE
	return bRemoveOnPC;
#else
	#if XBOX
		return bRemoveOn360;
	#else
		return bRemoveOnPS3;
	#endif
#endif
}


//////////////////////////////////////////////////////////////////////////
// UUTUIDataProvider_SearchResult
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataProvider_SearchResult);

/**
 * Resolves the value of the data field specified and stores it in the output parameter.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with data collections
 */
UBOOL UUTUIDataProvider_SearchResult::GetFieldValue(const FString& FieldName,FUIProviderFieldValue& out_FieldValue,INT ArrayIndex)
{
	UBOOL bResult=FALSE;
	FResultImageInfo *DisplayImage = NULL;

	if(FieldName==TEXT("PureServerImage"))
	{
		DisplayImage = &PureImage;
	}
	else if(FieldName==TEXT("LockedImage"))
	{
		DisplayImage = &LockedImage;
	}
	else if(FieldName==TEXT("FriendOnlineImage"))
	{
		DisplayImage = &FriendOnlineImage;
	}

	if(DisplayImage!=NULL)
	{
		UUITexture* ResolvedImage = NULL;
		USurface* ImageResource = LoadObject<USurface>(NULL, *DisplayImage->ImageName, NULL, LOAD_None, NULL);
		if ( ImageResource == NULL )
		{
			ResolvedImage = FindObject<UUITexture>(ANY_PACKAGE, *DisplayImage->ImageName);
			if ( ResolvedImage != NULL )
			{
				ImageResource = ResolvedImage->GetSurface();
			}
		}

		if ( ImageResource != NULL )
		{
			if ( ResolvedImage == NULL )
			{
				ResolvedImage = CreateTextureWrapper(ImageResource);
			}

			if ( ResolvedImage != NULL )
			{
				FUIStringNode_Image* ImageNode = new FUIStringNode_Image(*DisplayImage->ImageName);

				ImageNode->RenderedImage = ResolvedImage;
				ImageNode->RenderedImage->ImageTexture = ImageResource;
				ImageNode->ForcedExtent.X = 20;
				ImageNode->ForcedExtent.Y = 20;
				ImageNode->TexCoords.U = DisplayImage->ImageX;
				ImageNode->TexCoords.V = DisplayImage->ImageY;
				ImageNode->TexCoords.UL = DisplayImage->ImageXL;
				ImageNode->TexCoords.VL = DisplayImage->ImageYL;

				out_FieldValue.PropertyTag = ImageResource->GetFName();
				out_FieldValue.PropertyType = DATATYPE_Property;
				out_FieldValue.CustomStringNode = ImageNode;
				out_FieldValue.ImageValue = ImageResource;

				bResult = TRUE;
			}
		}

		bResult = TRUE;
	}

	return bResult || Super::GetFieldValue(FieldName, out_FieldValue, ArrayIndex);
}

/**
 * Builds a list of available fields from the array of properties in the
 * game settings object
 *
 * @param OutFields	out value that receives the list of exposed properties
 */
void UUTUIDataProvider_SearchResult::GetSupportedDataFields(TArray<FUIDataProviderField>& OutFields)
{
	Super::GetSupportedDataFields(OutFields);

	OutFields.AddItem(FUIDataProviderField(TEXT("PureServerImage")));
	OutFields.AddItem(FUIDataProviderField(TEXT("LockedImage")));
	OutFields.AddItem(FUIDataProviderField(TEXT("FriendOnlineImage")));
}

//////////////////////////////////////////////////////////////////////////
// UUTUIDataStore_MenuItems
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataStore_MenuItems);

void UUTUIDataStore_MenuItems::GetAllResourceDataProviders(UClass* ProviderClass, TArray<UUTUIResourceDataProvider*>& Providers)
{
	checkSlow(ProviderClass->IsChildOf(UUTUIResourceDataProvider::StaticClass()));

	TArray<FString> ConfigFileNames;
	GFileManager->FindFiles(ConfigFileNames, *(appGameConfigDir() + TEXT("*.ini")), TRUE, FALSE);
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
						//@note: names must be unique across all .ini files
						UUTUIResourceDataProvider* NewProvider = Cast<UUTUIResourceDataProvider>(StaticFindObject(ProviderClass, ANY_PACKAGE, *ObjectName.ToString(), TRUE));
						if (NewProvider == NULL)
						{
							NewProvider = ConstructObject<UUTUIResourceDataProvider>(ProviderClass, IsTemplate() ? (UObject*)GetTransientPackage() : this, ObjectName);
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

/**
 * Finds or creates the UIResourceDataProvider instances referenced by ElementProviderTypes, and stores the result
 * into the ListElementProvider map.
 */
void UUTUIDataStore_MenuItems::InitializeListElementProviders()
{
	ListElementProviders.Empty();

	// for each configured provider type, retrieve the list of ini sections that contain data for that provider class
	for ( INT ProviderTypeIndex = 0; ProviderTypeIndex < ElementProviderTypes.Num(); ProviderTypeIndex++ )
	{
		FGameResourceDataProvider& ProviderType = ElementProviderTypes(ProviderTypeIndex);

		UClass* ProviderClass = ProviderType.ProviderClass;

#if !CONSOLE
		if (ProviderClass->IsChildOf(UUTUIResourceDataProvider::StaticClass()) && ProviderClass->GetDefaultObject<UUTUIResourceDataProvider>()->bSearchAllInis)
		{
			// search all .ini files for instances to create
			TArray<UUTUIResourceDataProvider*> Providers;
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

	// Get a list of demo file names. - @todo: This may need to be generated at runtime when entering the demo playback menu.
	TArray<FString> DemoFileNames;
	FString DemoDir = appGameDir() + TEXT("Demos/");
	GFileManager->FindFiles(DemoFileNames, *(DemoDir+ TEXT("*.demo")), TRUE, FALSE);
	for (INT FileIdx = 0; FileIdx < DemoFileNames.Num(); FileIdx++)
	{
		UUIResourceDataProvider* Provider = ConstructObject<UUIResourceDataProvider>(
			UUTUIDataProvider_DemoFile::StaticClass(),
			this);

		if ( Provider != NULL )
		{
			UUTUIDataProvider_DemoFile* DemoProvider = Cast<UUTUIDataProvider_DemoFile>(Provider);
			DemoProvider->Filename = DemoFileNames(FileIdx);
			ListElementProviders.Add(TEXT("DemoFiles"), Provider);
		}
	}

	// Generate a placeholder entry for the editor.
	if(GIsEditor && !GIsGame)
	{
		UUIResourceDataProvider* Provider = ConstructObject<UUIResourceDataProvider>(
			UUTUIDataProvider_DemoFile::StaticClass(),
			this);

		if ( Provider != NULL )
		{
			UUTUIDataProvider_DemoFile* DemoProvider = Cast<UUTUIDataProvider_DemoFile>(Provider);
			DemoProvider->Filename = TEXT("Placeholder.demo");
			ListElementProviders.Add(TEXT("DemoFiles"), Provider);
		}
	}
}

/** 
 * Gets the list of element providers for a fieldname with filtered elements removed.
 *
 * @param FieldName				Fieldname to use to search for providers.
 * @param OutElementProviders	Array to store providers in.
 */
void UUTUIDataStore_MenuItems::GetFilteredElementProviders(FName FieldName, TArray<UUTUIResourceDataProvider*> &OutElementProviders)
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
		FieldName = TEXT("Weapons");
	}
	else if(FieldName==TEXT("GameModeFilter"))
	{
		FieldName = TEXT("GameModes");
	}

	TArray<UUIResourceDataProvider*> Providers;
	ListElementProviders.MultiFind(FieldName, Providers);

	for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
	{
		UUTUIResourceDataProvider* DataProvider = Cast<UUTUIResourceDataProvider>(Providers(ProviderIndex));
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
TArray<FName> UUTUIDataStore_MenuItems::GetElementProviderTags()
{
	TArray<FName> Tags = Super::GetElementProviderTags();
	
	// Loop through all of the simple menus and get tags.
	TArray<UUIResourceDataProvider*> Providers;
	ListElementProviders.MultiFind(TEXT("SimpleMenus"), Providers);

	for(INT MenuIdx=0; MenuIdx<Providers.Num(); MenuIdx++)
	{
		UUTUIDataProvider_SimpleMenu* Menu = Cast<UUTUIDataProvider_SimpleMenu>(Providers(MenuIdx));

		if(Menu)
		{
			Tags.AddItem(Menu->FieldName);
		}
	}

	Tags.AddItem(TEXT("AvailableMutators"));
	Tags.AddItem(TEXT("EnabledMutators"));
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
INT UUTUIDataStore_MenuItems::GetElementCount( FName FieldName )
{
	INT Result = 0;
	UBOOL bFound = FALSE;

	// Check for simple menu first.
	UUTUIDataProvider_SimpleMenu* Menu = FindSimpleMenu(FieldName);

	if(Menu)
	{
		Result = Menu->Options.Num();
		bFound = TRUE;
	}

	if(bFound==FALSE)
	{
		TArray<UUTUIResourceDataProvider*> Providers;
		GetFilteredElementProviders(FieldName, Providers);

		return Providers.Num();
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
UBOOL UUTUIDataStore_MenuItems::GetListElements( FName FieldName, TArray<INT>& out_Elements )
{
	UBOOL bResult = FALSE;

	UUTUIDataProvider_SimpleMenu* Menu = FindSimpleMenu(FieldName);

	if(Menu)
	{
		for(INT OptionIdx=0; OptionIdx<Menu->Options.Num(); OptionIdx++)
		{
			out_Elements.AddItem(OptionIdx);
		}

		bResult = TRUE;
	}

	if(bResult==FALSE)
	{
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
					UUTUIResourceDataProvider* DataProvider = Cast<UUTUIResourceDataProvider>(Providers(ProviderIndex));
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
				UUTUIResourceDataProvider* DataProvider = Cast<UUTUIResourceDataProvider>(Providers(ProviderIndex));
				if(DataProvider && DataProvider->IsFiltered() == FALSE && EnabledMutators.ContainsItem(ProviderIndex)==FALSE)
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
					UUTUIResourceDataProvider* DataProvider = Cast<UUTUIResourceDataProvider>(Providers(ProviderIndex));
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
				UUTUIResourceDataProvider* DataProvider = Cast<UUTUIResourceDataProvider>(Providers(ProviderIndex));
				if(DataProvider && DataProvider->IsFiltered() == FALSE && MapCycle.ContainsItem(ProviderIndex)==FALSE)
				{
					out_Elements.AddUniqueItem(ProviderIndex);
				}
			}
			bResult = TRUE;
		}
		else if(FieldName==TEXT("WeaponPriority"))
		{
			FieldName = TEXT("Weapons");
			ListElementProviders.MultiFind(FieldName, Providers);

			// Use the enabled mutators array to determine which mutators to expose
			out_Elements.Empty();
			for ( INT WeaponIdx = 0; WeaponIdx < WeaponPriority.Num(); WeaponIdx++ )
			{
				INT ProviderIndex = WeaponPriority(WeaponIdx);
				if(Providers.IsValidIndex(ProviderIndex))
				{	
					UUTUIResourceDataProvider* DataProvider = Cast<UUTUIResourceDataProvider>(Providers(ProviderIndex));
					if(DataProvider && DataProvider->IsFiltered() == FALSE)
					{
						out_Elements.AddUniqueItem(ProviderIndex);
					}
				}
			}

			bResult = TRUE;
		}
		else
		{
			if(FieldName==TEXT("GameModeFilter"))
			{
				FieldName = TEXT("GameModes");
			}

			ListElementProviders.MultiFind(FieldName, Providers);
			if ( Providers.Num() > 0 )
			{
				out_Elements.Empty();
				for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
				{
					UUTUIResourceDataProvider* DataProvider = Cast<UUTUIResourceDataProvider>(Providers(ProviderIndex));
					if(DataProvider && DataProvider->IsFiltered() == FALSE)
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
UBOOL UUTUIDataStore_MenuItems::GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	if(FieldName==TEXT("GameModeFilter"))
	{
		bResult = GetCellFieldValue(TEXT("GameModeFilter"), GameModeFilter, out_FieldValue);
	}
	else if(FieldName==TEXT("GameModeFilterClass"))
	{
		TArray<UUIResourceDataProvider*> Providers;
		ListElementProviders.MultiFind(TEXT("GameModes"), Providers);

		if(Providers.IsValidIndex(GameModeFilter))
		{
			UUTUIDataProvider_GameModeInfo* GameModeInfo = Cast<UUTUIDataProvider_GameModeInfo>(Providers(GameModeFilter));
			out_FieldValue.PropertyTag = *FieldName;
			out_FieldValue.PropertyType = DATATYPE_Property;
			out_FieldValue.StringValue = GameModeInfo->GameMode;
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
void UUTUIDataStore_MenuItems::GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields )
{
	TArray<UUIResourceDataProvider*> Providers;

	/*
	out_Fields.Empty();

	for ( INT ProviderIndex = 0; ProviderIndex < ElementProviderTypes.Num(); ProviderIndex++ )
	{
		FGameResourceDataProvider& Provider = ElementProviderTypes(ProviderIndex);

		TArray<UUTUIResourceDataProvider*> ResourceProviders;
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
	ListElementProviders.MultiFind(TEXT("Weapons"), Providers);
	out_Fields.AddItem(FUIDataProviderField(TEXT("WeaponPriority"), (TArray<UUIDataProvider*>&)Providers));

	// Loop through all of the simple menus.
	Providers.Empty();
	ListElementProviders.MultiFind(TEXT("SimpleMenus"), Providers);

	for(INT MenuIdx=0; MenuIdx<Providers.Num(); MenuIdx++)
	{
		UUTUIDataProvider_SimpleMenu* Menu = Cast<UUTUIDataProvider_SimpleMenu>(Providers(MenuIdx));

		if(Menu)
		{
			TArray<UUIDataProvider*> Providers;
			Providers.AddItem(Menu);

			FUIDataProviderField NewField(Menu->FieldName,  Providers);
			out_Fields.AddItem(NewField);
		}
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
TScriptInterface<class IUIListElementCellProvider> UUTUIDataStore_MenuItems::GetElementCellSchemaProvider( FName FieldName )
{
	TScriptInterface<class IUIListElementCellProvider> Result;
	UUTUIDataProvider_SimpleMenu* Menu = FindSimpleMenu(FieldName);

	if(Menu)
	{
		Result = Menu;
	}
	else
	{
		// Replace enabled/available mutators with the straight mutators schema.
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
			FieldName = TEXT("Weapons");
		}
		else if(FieldName==TEXT("GameModeFilter"))
		{
			return this;
		}

		Result = Super::GetElementCellSchemaProvider(FieldName);
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
TScriptInterface<class IUIListElementCellProvider> UUTUIDataStore_MenuItems::GetElementCellValueProvider( FName FieldName, INT ListIndex )
{
	TScriptInterface<class IUIListElementCellProvider> Result;
	UUTUIDataProvider_SimpleMenu* Menu = FindSimpleMenu(FieldName);

	if(Menu)
	{
		Result = Menu;
	}
	else
	{
		// Replace enabled/available mutators with the straight mutators schema.
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
			FieldName = TEXT("Weapons");
		}
		else if(FieldName==TEXT("GameModeFilter"))
		{
			return this;
		}

		Result = Super::GetElementCellValueProvider(FieldName, ListIndex);
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
UBOOL UUTUIDataStore_MenuItems::SetFieldValue( const FString& FieldName, const struct FUIProviderScriptFieldValue& FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	if(FieldName==TEXT("GameModeFilter"))
	{
		INT NumItems = GetElementCount(*FieldName);

		for(INT ValueIdx=0; ValueIdx<NumItems; ValueIdx++)
		{
			FUIProviderFieldValue out_FieldValue(EC_EventParm);

			if(GetCellFieldValue(TEXT("GameModeFilter"), ValueIdx, out_FieldValue))
			{
				if(FieldValue.StringValue == out_FieldValue.StringValue)
				{
					GameModeFilter = ValueIdx;
					bResult = TRUE;
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
UBOOL UUTUIDataStore_MenuItems::IsElementEnabled( FName FieldName, INT CollectionIndex )
{
	return TRUE;
}

/**
 * Attempts to find a simple menu with the field name provided.
 *
 * @param FieldName		Field name of the simple menu.  Defined in UUTUIDataProvider_SimpleMenu::FieldName
 */
UUTUIDataProvider_SimpleMenu* UUTUIDataStore_MenuItems::FindSimpleMenu(FName FieldName)
{
	UUTUIDataProvider_SimpleMenu* Result = NULL;

	TArray<UUIResourceDataProvider*> Providers;
	ListElementProviders.MultiFind(TEXT("SimpleMenus"), Providers);

	for(INT MenuIdx=0; MenuIdx<Providers.Num(); MenuIdx++)
	{
		UUTUIDataProvider_SimpleMenu* Menu = Cast<UUTUIDataProvider_SimpleMenu>(Providers(MenuIdx));

		if(Menu && Menu->FieldName==FieldName)
		{
			Result = Menu;
			break;
		}
	}

	return Result;
}

/** @return Returns the number of providers for a given field name. */
INT UUTUIDataStore_MenuItems::GetProviderCount(FName FieldName)
{
	TArray<UUTUIResourceDataProvider*> Providers;
	GetFilteredElementProviders(FieldName, Providers);

	return Providers.Num();
}




/**
 * Returns the names of the exposed members in the first entry in the array
 * of search results
 *
 * @param OutCellTags the columns supported by this row
 */
void UUTUIDataStore_MenuItems::GetElementCellTags( TMap<FName,FString>& out_CellTags )
{
	out_CellTags.Set(TEXT("GameModeFilter"),TEXT("GameModeFilter"));
}

/**
 * Retrieves the field type for the specified cell.
 *
 * @param	CellTag				the tag for the element cell to get the field type for
 * @param	out_CellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
 *
 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
 */
UBOOL UUTUIDataStore_MenuItems::GetCellFieldType( const FName& CellTag, BYTE& out_CellFieldType )
{
	return DATATYPE_Property;
}

/**
 * Resolves the value of the cell specified by CellTag and stores it in the output parameter.
 *
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the UIList's item index for the element that contains this cell.  Useful for data providers which
 *							do not provide unique UIListElement objects for each element.
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with cell tags that represent data collections.  Corresponds to the
 *							ArrayIndex of the collection that this cell is bound to, or INDEX_NONE if CellTag does not correspond
 *							to a data collection.
 */
UBOOL UUTUIDataStore_MenuItems::GetCellFieldValue( const FName& CellTag, INT ListIndex, FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	if(CellTag==TEXT("GameModeFilter"))
	{
		TArray<UUIResourceDataProvider*> Providers;
		Providers.Empty();
		ListElementProviders.MultiFind(TEXT("GameModes"), Providers);

		if(Providers.IsValidIndex(ListIndex))
		{
			UUTUIDataProvider_GameModeInfo* GameMode = Cast<UUTUIDataProvider_GameModeInfo>(Providers(ListIndex));
			out_FieldValue.StringValue = GameMode->FriendlyName;
			bResult = TRUE;
		}
	}

	return bResult;
}


/**
 * Attempts to find the index of a provider given a provider field name, a search tag, and a value to match.
 *
 * @return	Returns the index of the provider or INDEX_NONE if the provider wasn't found.
 */
INT UUTUIDataStore_MenuItems::FindValueInProviderSet(FName ProviderFieldName,FName SearchTag,const FString& SearchValue)
{
	INT Result = INDEX_NONE;
	TArray<INT> PossibleItems;
	GetListElements(ProviderFieldName, PossibleItems);

	for(INT ItemIdx=0; ItemIdx<PossibleItems.Num(); ItemIdx++)
	{
		FUIProviderFieldValue OutValue(EC_EventParm);
		TScriptInterface<IUIListElementCellProvider> DataProvider = GetElementCellValueProvider(ProviderFieldName, PossibleItems(ItemIdx));

		if(DataProvider->GetCellFieldValue(SearchTag, PossibleItems(ItemIdx), OutValue))
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
 * Attempts to find the value of a provider given a provider cell field.
 *
 * @return	Returns true if the value was found, false otherwise.
 */
UBOOL UUTUIDataStore_MenuItems::GetValueFromProviderSet(FName ProviderFieldName,FName SearchTag,INT ListIndex,FString& OutValue)
{
	UBOOL bResult = FALSE;
	FUIProviderFieldValue FieldValue(EC_EventParm);
	TScriptInterface<IUIListElementCellProvider> DataProvider = GetElementCellValueProvider(ProviderFieldName, ListIndex);

	if(DataProvider->GetCellFieldValue(SearchTag, ListIndex, FieldValue))
	{
		OutValue = FieldValue.StringValue;
		bResult = TRUE;
	}

	return bResult;
}


//////////////////////////////////////////////////////////////////////////
// UUTUIDataProvider_SimpleMenu
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataProvider_SimpleMenu);

/**
 * Returns the data tag associated with the specified provider.
 *
 * @return	the data field tag associated with the provider specified, or NAME_None if the provider specified is not
 *			contained by this data store.
 */
FName UUTUIDataProvider_SimpleMenu::GetProviderDataTag( UUIDataProvider* Provider )
{
	return FieldName;
}

/**
 * Retrieves the list of tags that can be bound to individual cells in a single list element.
 *
 * @param	out_CellTags	receives the list of tag/column headers that can be bound to element cells for the specified property.
 */
void UUTUIDataProvider_SimpleMenu::GetElementCellTags( TMap<FName,FString>& out_CellTags )
{
	out_CellTags.Set(TEXT("OptionName"),TEXT("OptionName"));
}

/**
 * Retrieves the field type for the specified cell.
 *
 * @param	CellTag				the tag for the element cell to get the field type for
 * @param	out_CellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
 *
 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
 */
UBOOL UUTUIDataProvider_SimpleMenu::GetCellFieldType( const FName& CellTag, BYTE& out_CellFieldType )
{
	UBOOL bResult = FALSE;

	if(CellTag==TEXT("OptionName"))
	{
		out_CellFieldType = DATATYPE_Property;
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Resolves the value of the cell specified by CellTag and stores it in the output parameter.
 *
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the UIList's item index for the element that contains this cell.  Useful for data providers which
 *							do not provide unique UIListElement objects for each element.
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with cell tags that represent data collections.  Corresponds to the
 *							ArrayIndex of the collection that this cell is bound to, or INDEX_NONE if CellTag does not correspond
 *							to a data collection.
 */
UBOOL UUTUIDataProvider_SimpleMenu::GetCellFieldValue( const FName& CellTag, INT ListIndex, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	if((CellTag==TEXT("OptionName") || CellTag == FieldName) && Options.IsValidIndex(ListIndex))
	{
		out_FieldValue.PropertyType = DATATYPE_Property;
		out_FieldValue.PropertyTag = CellTag;
		out_FieldValue.StringValue = Options(ListIndex);
		bResult = TRUE;
	}

	return bResult;
}


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
UBOOL UUTUIDataProvider_SimpleMenu::GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	if(FieldName==TEXT("OptionName") && Options.IsValidIndex(ArrayIndex))
	{
		out_FieldValue.PropertyTag = TEXT("OptionName");
		out_FieldValue.StringValue = Options(ArrayIndex);
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Gets the list of data fields exposed by this data provider.
 *
 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
 * 						Will call GetScriptDataTags to allow script-only child classes to add to this list.
 */
void UUTUIDataProvider_SimpleMenu::GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields )
{
	Super::GetSupportedDataFields(out_Fields);

	new(out_Fields) FUIDataProviderField( TEXT("OptionName"), DATATYPE_Property );
}

//////////////////////////////////////////////////////////////////////////
// UUTUIDataProvider_MapInfo
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataProvider_MapInfo);

/** @return Whether or not we should be filtered out of the list results. */
UBOOL UUTUIDataProvider_MapInfo::IsFiltered()
{
	return (eventSupportedByCurrentGameMode()==FALSE);
}

//////////////////////////////////////////////////////////////////////////
// UUTUIDataProvider_Mutator
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataProvider_Mutator);

/** @return Whether or not we should be filtered out of the list results. */
UBOOL UUTUIDataProvider_Mutator::IsFiltered()
{
	return (eventSupportsCurrentGameMode()==FALSE) || Super::IsFiltered();
}

//////////////////////////////////////////////////////////////////////////
// UUTUIDataStore_CustomChar
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataStore_CustomChar);

/** @return Returns the enum value for a part type given its tag. */
ECharPart UUTUIDataStore_CustomChar::ResolvePartEnumFromTag(const FName &FieldTag)
{
	ECharPart Result = PART_MAX;

	for(INT PartIdx=0; PartIdx<PartTags.Num(); PartIdx++)
	{
		if(FieldTag==PartTags(PartIdx))
		{
			Result = (ECharPart)PartIdx;
			break;
		}
	}

	return Result;
}

/** @return Returns the tag value for a part type given its enum. */
FName UUTUIDataStore_CustomChar::ResolveTagFromPartEnum(ECharPart PartType)
{
	FName Result(NAME_None);

	if(PartTags.IsValidIndex(PartType))
	{
		Result = PartTags(PartType);
	}

	return Result;
}

/**
 * Initializes the dataproviders for all of the various character parts.
 */
void UUTUIDataStore_CustomChar::InitializeDataStore()
{
	UUTCustomChar_Data* Data = CastChecked<UUTCustomChar_Data>(UUTCustomChar_Data::StaticClass()->GetDefaultObject());

	// Initialize all of the part providers, do it backwards because of the way the multimap stores items.
	for(INT PartIdx=Data->Parts.Num()-1; PartIdx>=0; PartIdx--)	
	{
		FCustomCharPart &Part = Data->Parts(PartIdx);

		UUTUIDataProvider_CharacterPart* Provider = ConstructObject<UUTUIDataProvider_CharacterPart>(UUTUIDataProvider_CharacterPart::StaticClass(),this);
		Provider->SetPartData(Part);
		PartProviders.Add(ResolveTagFromPartEnum((ECharPart)Part.Part), Provider);
	}

	// Initialize Faction Providers, do it backwards because of the way the multimap stores items.
	for(INT FactionIdx=Data->Factions.Num()-1; FactionIdx>=0; FactionIdx--)	
	{
		FFactionInfo &Faction = Data->Factions(FactionIdx);

		UUTUIDataProvider_CharacterFaction* Provider = ConstructObject<UUTUIDataProvider_CharacterFaction>(UUTUIDataProvider_CharacterFaction::StaticClass(),this);
		Provider->SetData(Faction);
		PartProviders.Add(TEXT("Faction"), Provider);
	}

	// Initialize Character Providers, do it backwards because of the way the multimap stores items.
	for(INT CharacterIdx=Data->Characters.Num()-1; CharacterIdx>=0; CharacterIdx--)	
	{
		FCharacterInfo &Character = Data->Characters(CharacterIdx);

		UUTUIDataProvider_Character* Provider = ConstructObject<UUTUIDataProvider_Character>(UUTUIDataProvider_Character::StaticClass(),this);
		Provider->SetData(Character);
		PartProviders.Add(TEXT("Character"), Provider);
	}
}

/** 
 * Gets the list of element providers for a fieldname with filtered elements removed.
 *
 * @param FieldName				Fieldname to use to search for providers.
 * @param OutParts				Array to store part indices.
 */
void UUTUIDataStore_CustomChar::GetFilteredElementProviders(FName FieldName, TArray<UUIDataProvider*> &OutParts)
{
	OutParts.Empty();

	TArray<UUIDataProvider*> FilteredParts;
	PartProviders.MultiFind(FieldName, FilteredParts);

	for ( INT ProviderIndex = 0; ProviderIndex < FilteredParts.Num(); ProviderIndex++ )
	{
		OutParts.AddUniqueItem(FilteredParts(ProviderIndex));
	}
}


/**
 * Resolves PropertyName into a list element provider that provides list elements for the property specified.
 *
 * @param	PropertyName	the name of the property that corresponds to a list element provider supported by this data store
 *
 * @return	a pointer to an interface for retrieving list elements associated with the data specified, or NULL if
 *			there is no list element provider associated with the specified property.
 */
TScriptInterface<IUIListElementProvider> UUTUIDataStore_CustomChar::ResolveListElementProvider( const FString& PropertyName )
{
	TScriptInterface<IUIListElementProvider> Result;

	FName DataTag = *PropertyName;
	//@todo - this might not be the correct behavior
	if ( IsDataTagSupported(DataTag) )
	{
		Result = this;
	}

	return Result;
}


/* === IUIListElementProvider interface === */
/**
 * Retrieves the list of all data tags contained by this element provider which correspond to list element data.
 *
 * @return	the list of tags supported by this element provider which correspond to list element data.
 */
TArray<FName> UUTUIDataStore_CustomChar::GetElementProviderTags()
{
	TArray<FName> ProviderTags;

	for ( INT PartIdx = 0; PartIdx < PartTags.Num(); PartIdx++ )
	{
		ProviderTags.AddItem(PartTags(PartIdx));
	}

	ProviderTags.AddItem(TEXT("Faction"));
	ProviderTags.AddItem(TEXT("Character"));
	ProviderTags.AddItem(TEXT("AvailableBots"));
	ProviderTags.AddItem(TEXT("ActiveBots"));

	return ProviderTags;
}


/**
 * Returns the number of list elements associated with the data tag specified.
 *
 * @param	FieldName	the name of the property to get the element count for.  guaranteed to be one of the values returned
 *						from GetElementProviderTags.
 *
 * @return	the total number of elements that are required to fully represent the data specified.
 */
INT UUTUIDataStore_CustomChar::GetElementCount( FName FieldName )
{
	TArray<UUIDataProvider*> FilteredParts;

	if(FieldName==TEXT("ActiveBots") || FieldName==TEXT("AvailableBots"))
	{
		FieldName = TEXT("Character");
	}

	GetFilteredElementProviders(FieldName, FilteredParts);

	return FilteredParts.Num();
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
UBOOL UUTUIDataStore_CustomChar::GetListElements( FName FieldName, TArray<INT>& out_Elements )
{
	UBOOL bResult = FALSE;

	TArray<UUIDataProvider*> FilteredParts;

	out_Elements.Empty();

	// See if we should filter any of the parts.
	if(ResolvePartEnumFromTag(FieldName) != PART_MAX)	// Filter parts by family
	{
		PartProviders.MultiFind(FieldName, FilteredParts);

		// Only display elements for the current family.
		for ( INT PartIdx = 0; PartIdx < FilteredParts.Num(); PartIdx++ )
		{
			UUTUIDataProvider_CharacterPart* CharacterPart = Cast<UUTUIDataProvider_CharacterPart>(FilteredParts(PartIdx));
			
			if(CharacterPart && (CurrentFamily.Len()==0 || CharacterPart->PartData.FamilyID==CurrentFamily))
			{
				out_Elements.AddUniqueItem(PartIdx);
			}
		}
	}
	else if(FieldName==TEXT("Character"))	// Filter characters by faction
	{
		PartProviders.MultiFind(FieldName, FilteredParts);

		// Only display elements for the current faction.
		for ( INT PartIdx = 0; PartIdx < FilteredParts.Num(); PartIdx++ )
		{
			UUTUIDataProvider_Character* Character = Cast<UUTUIDataProvider_Character>(FilteredParts(PartIdx));

			if(Character && (CurrentFaction.Len()==0 || Character->CustomData.Faction==CurrentFaction))
			{
				out_Elements.AddUniqueItem(PartIdx);
			}
		}
	}
	else if(FieldName==TEXT("ActiveBots"))		// All of the characters in the active bots array
	{
		PartProviders.MultiFind(TEXT("Character"), FilteredParts);

		// Use the active bots array
		for ( INT ItemIdx = 0; ItemIdx < ActiveBots.Num(); ItemIdx++ )
		{
			INT ProviderIndex = ActiveBots(ItemIdx);
			if(FilteredParts.IsValidIndex(ProviderIndex))
			{	
				UUTUIDataProvider_Character* DataProvider = Cast<UUTUIDataProvider_Character>(FilteredParts(ProviderIndex));
				if(DataProvider)
				{
					out_Elements.AddUniqueItem(ProviderIndex);
				}
			}
		}

		bResult = TRUE;
	}
	else if(FieldName==TEXT("AvailableBots"))	// All of the characters that are NOT in the active bots array
	{
		PartProviders.MultiFind(TEXT("Character"), FilteredParts);

		// Make sure the provider index isnt in the active array
		for ( INT ProviderIndex = 0; ProviderIndex < FilteredParts.Num(); ProviderIndex++ )
		{
			UUTUIDataProvider_Character* DataProvider = Cast<UUTUIDataProvider_Character>(FilteredParts(ProviderIndex));
			if(DataProvider && ActiveBots.ContainsItem(ProviderIndex)==FALSE)
			{
				out_Elements.AddUniqueItem(ProviderIndex);
			}
		}
		bResult = TRUE;
	}
	else	// Display all elements
	{
		PartProviders.MultiFind(FieldName, FilteredParts);

		for ( INT ElementIdx = 0; ElementIdx < FilteredParts.Num(); ElementIdx++ )
		{
			out_Elements.AddUniqueItem(ElementIdx);
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
UBOOL UUTUIDataStore_CustomChar::GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	if(FieldName==TEXT("Faction"))
	{
		out_FieldValue.StringValue = CurrentFaction;
		out_FieldValue.PropertyTag = *FieldName;
		out_FieldValue.PropertyType = DATATYPE_Property;

		bResult = TRUE;
	}
	else if(FieldName==TEXT("Character") || FieldName==TEXT("ActiveBots") || FieldName==TEXT("AvailableBots"))
	{
		out_FieldValue.StringValue = CurrentCharacter;
		out_FieldValue.PropertyTag = *FieldName;
		out_FieldValue.PropertyType = DATATYPE_Property;

		bResult = TRUE;
	}
	else if(FieldName==TEXT("Family"))
	{
		out_FieldValue.StringValue = CurrentFamily;
		out_FieldValue.PropertyTag = *FieldName;
		out_FieldValue.PropertyType = DATATYPE_Property;

		bResult = TRUE;
	}

	if(bResult==FALSE)
	{
		bResult = Super::GetFieldValue(FieldName, out_FieldValue, ArrayIndex);
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
UBOOL UUTUIDataStore_CustomChar::SetFieldValue( const FString& FieldName, const struct FUIProviderScriptFieldValue& FieldValue, INT ArrayIndex )
{
	UBOOL bResult = FALSE;

	if(FieldName=="Faction")
	{
		CurrentFaction = FieldValue.StringValue;
		bResult = TRUE;
	}
	else if(FieldName=="Character")
	{
		CurrentCharacter = FieldValue.StringValue;
		bResult = TRUE;
	}
	else if(FieldName=="Family")
	{
		CurrentFamily = FieldValue.StringValue;
		bResult = TRUE;
	}

	if(bResult==FALSE)
	{
		bResult = Super::SetFieldValue(FieldName, FieldValue, ArrayIndex);
	}

	return bResult;
}

/**
 * Generates filler data for a given tag.  This is used by the editor to generate a preview that gives the
 * user an idea as to what a bound datastore will look like in-game.
 *
 * @param		DataTag		the tag corresponding to the data field that we want filler data for
 *
 * @return		a string of made-up data which is indicative of the typical [resolved] value for the specified field.
 */
FString UUTUIDataStore_CustomChar::GenerateFillerData( const FString& DataTag )
{
	//@todo
	return Super::GenerateFillerData(DataTag);
}

/**
 * Gets the list of data fields exposed by this data provider.
 *
 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
 */
void UUTUIDataStore_CustomChar::GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields )
{
	out_Fields.Empty();

	TArray<UUIDataProvider*> Providers;

	for ( INT PartIdx = 0; PartIdx < PartTags.Num(); PartIdx++ )
	{
		Providers.Empty();
		PartProviders.MultiFind(PartTags(PartIdx), Providers);
		new(out_Fields) FUIDataProviderField( PartTags(PartIdx), (TArray<UUIDataProvider*>&)Providers);
	}

	// Faction
	Providers.Empty();
	PartProviders.MultiFind(TEXT("Faction"), Providers);
	new(out_Fields) FUIDataProviderField( TEXT("Faction"), (TArray<UUIDataProvider*>&)Providers);

	// Character
	Providers.Empty();
	PartProviders.MultiFind(TEXT("Character"), Providers);
	new(out_Fields) FUIDataProviderField( TEXT("Character"), (TArray<UUIDataProvider*>&)Providers);
	new(out_Fields) FUIDataProviderField( TEXT("ActiveBots"), (TArray<UUIDataProvider*>&)Providers);
	new(out_Fields) FUIDataProviderField( TEXT("AvailableBots"), (TArray<UUIDataProvider*>&)Providers);
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
TScriptInterface<class IUIListElementCellProvider> UUTUIDataStore_CustomChar::GetElementCellSchemaProvider( FName FieldName )
{
	TScriptInterface<IUIListElementCellProvider> Result;

	// search for the provider that has the matching tag
	if(ResolvePartEnumFromTag(FieldName) != PART_MAX)
	{
		Result = UUTUIDataProvider_CharacterPart::StaticClass()->GetDefaultObject<UUTUIDataProvider_CharacterPart>();
	}
	else if(FieldName==TEXT("Faction"))
	{
		Result = UUTUIDataProvider_CharacterFaction::StaticClass()->GetDefaultObject<UUTUIDataProvider_CharacterFaction>();
	}
	else if(FieldName==TEXT("Character") || FieldName==TEXT("ActiveBots") || FieldName==TEXT("AvailableBots"))
	{
		Result = UUTUIDataProvider_Character::StaticClass()->GetDefaultObject<UUTUIDataProvider_Character>();
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
TScriptInterface<class IUIListElementCellProvider> UUTUIDataStore_CustomChar::GetElementCellValueProvider( FName FieldName, INT ListIndex )
{
	TScriptInterface<IUIListElementCellProvider> Result;

	if(FieldName==TEXT("ActiveBots") || FieldName==TEXT("AvailableBots"))
	{
		FieldName=TEXT("Character");
	}

	// search for the provider that has the matching tag
	TArray<UUIDataProvider*> Providers;
	PartProviders.MultiFind(FieldName, Providers);

	if(Providers.IsValidIndex(ListIndex))
	{
		if(ResolvePartEnumFromTag(FieldName) != PART_MAX)
		{
			Result = Cast<UUTUIDataProvider_CharacterPart>(Providers(ListIndex));
		}
		else if(FieldName==TEXT("Faction"))
		{
			Result = Cast<UUTUIDataProvider_CharacterFaction>(Providers(ListIndex));
		}
		else if(FieldName==TEXT("Character"))
		{
			Result = Cast<UUTUIDataProvider_Character>(Providers(ListIndex));
		}
	}

	return Result;
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
UBOOL UUTUIDataStore_CustomChar::IsElementEnabled( FName FieldName, INT CollectionIndex )
{
	return TRUE;
}


/* === UObject interface === */
/** Required since maps are not yet supported by script serialization */
void UUTUIDataStore_CustomChar::AddReferencedObjects( TArray<UObject*>& ObjectArray )
{
	Super::AddReferencedObjects(ObjectArray);

	for ( TMultiMap<FName,UUIDataProvider*>::TIterator It(PartProviders); It; ++It )
	{
		UUIDataProvider* Provider = It.Value();
		if ( Provider != NULL )
		{
			AddReferencedObject(ObjectArray, Provider);
		}
	}
}

void UUTUIDataStore_CustomChar::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	if ( !Ar.IsPersistent() )
	{
		for ( TMultiMap<FName,UUIDataProvider*>::TIterator It(PartProviders); It; ++It )
		{
			UUIDataProvider* Provider = It.Value();
			Ar << Provider;
		}
	}
}


/**
 * Called from ReloadConfig after the object has reloaded its configuration data.
 */
void UUTUIDataStore_CustomChar::PostReloadConfig( UProperty* PropertyThatWasLoaded )
{
	Super::PostReloadConfig( PropertyThatWasLoaded );

	if ( !HasAnyFlags(RF_ClassDefaultObject) )
	{
		if ( PropertyThatWasLoaded == NULL || PropertyThatWasLoaded->GetFName() == TEXT("ElementProviderTypes") )
		{
			// now notify any widgets that are subscribed to this datastore that they need to re-resolve their bindings
			eventRefreshSubscribers();
		}
	}
}


/**
 * Attempts to find the index of a provider given a provider field name, a search tag, and a value to match.
 *
 * @return	Returns the index of the provider or INDEX_NONE if the provider wasn't found.
 */
INT UUTUIDataStore_CustomChar::FindValueInProviderSet(FName ProviderFieldName,FName SearchTag,const FString& SearchValue)
{
	INT Result = INDEX_NONE;
	TArray<INT> PossibleItems;
	GetListElements(ProviderFieldName, PossibleItems);

	for(INT ItemIdx=0; ItemIdx<PossibleItems.Num(); ItemIdx++)
	{
		FUIProviderFieldValue OutValue(EC_EventParm);
		TScriptInterface<IUIListElementCellProvider> DataProvider = GetElementCellValueProvider(ProviderFieldName, PossibleItems(ItemIdx));

		if(DataProvider->GetCellFieldValue(SearchTag, PossibleItems(ItemIdx), OutValue))
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
 * Attempts to find the value of a provider given a provider cell field.
 *
 * @return	Returns true if the value was found, false otherwise.
 */
UBOOL UUTUIDataStore_CustomChar::GetValueFromProviderSet(FName ProviderFieldName,FName SearchTag,INT ListIndex,FString& OutValue)
{
	UBOOL bResult = FALSE;
	FUIProviderFieldValue FieldValue(EC_EventParm);
	TScriptInterface<IUIListElementCellProvider> DataProvider = GetElementCellValueProvider(ProviderFieldName, ListIndex);

	if(DataProvider->GetCellFieldValue(SearchTag, ListIndex, FieldValue))
	{
		OutValue = FieldValue.StringValue;
		bResult = TRUE;
	}

	return bResult;
}



//////////////////////////////////////////////////////////////////////////
// UUTUIDataProvider_CharacterPart
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataProvider_CharacterPart);

/** Sets the part data for this provider. */
void UUTUIDataProvider_CharacterPart::SetPartData(FCustomCharPart &InPartData)
{
	PartData = InPartData;
}

// IUIListElement interface

/**
 * Returns the names of the exposed members in the first entry in the array
 * of search results
 *
 * @param OutCellTags the columns supported by this row
 */
void UUTUIDataProvider_CharacterPart::GetElementCellTags( TMap<FName,FString>& OutCellTags )
{
	OutCellTags.Empty();

	//@todo: Find a better way to expose all of the tags for a part struct.
	OutCellTags.Set(TEXT("ObjectName"),TEXT("ObjectName"));
	OutCellTags.Set(TEXT("PartID"),TEXT("PartID"));
	OutCellTags.Set(TEXT("Family"),TEXT("Family"));
}

/**
 * Retrieves the field type for the specified cell.
 *
 * @param	CellTag				the tag for the element cell to get the field type for
 * @param	OutCellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
 *
 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
 */
UBOOL UUTUIDataProvider_CharacterPart::GetCellFieldType(const FName& CellTag,BYTE& OutCellFieldType)
{
	TMap<FName,FString> CellTags;
	GetElementCellTags(CellTags);
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
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the index of the value to fetch
 * @param	OutFieldValue	receives the resolved value for the property specified.
 * @param	ArrayIndex		ignored
 */
UBOOL UUTUIDataProvider_CharacterPart::GetCellFieldValue(const FName& CellTag,INT ListIndex,FUIProviderFieldValue& OutFieldValue,INT)
{
	UBOOL bResult = FALSE;



	if(CellTag==TEXT("PartID"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = PartData.PartID;
		bResult = TRUE;
	}
	else if(CellTag==TEXT("ObjectName"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = PartData.ObjectName;
		bResult = TRUE;
	}
	else if(CellTag==TEXT("Family"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = PartData.FamilyID;
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
void UUTUIDataProvider_CharacterPart::GetSupportedDataFields( TArray<FUIDataProviderField>& out_Fields )
{
	out_Fields.Empty();

	TMap<FName,FString> Elements;
	GetElementCellTags(Elements);

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
UBOOL UUTUIDataProvider_CharacterPart::GetFieldValue( const FString& FieldName, FUIProviderFieldValue& out_FieldValue, INT ArrayIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	FName PropertyFName(*FieldName, FNAME_Find);
	if ( PropertyFName != NAME_None )
	{
		bResult = GetCellFieldValue(PropertyFName,INDEX_NONE,out_FieldValue,ArrayIndex);
	}

	return bResult;
}


//////////////////////////////////////////////////////////////////////////
// UUTUIDataProvider_CharacterFaction
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataProvider_CharacterFaction);

/** Sets the Faction data for this provider. */
void UUTUIDataProvider_CharacterFaction::SetData(FFactionInfo &InFactionData)
{
	CustomData = InFactionData;
}

// IUIListElement interface

/**
 * Returns the names of the exposed members in the first entry in the array
 * of search results
 *
 * @param OutCellTags the columns supported by this row
 */
void UUTUIDataProvider_CharacterFaction::GetElementCellTags( TMap<FName,FString>& OutCellTags )
{
	OutCellTags.Empty();
	OutCellTags.Set(TEXT("FactionID"),TEXT("FactionID"));
	OutCellTags.Set(TEXT("FriendlyName"),TEXT("FriendlyName"));
	OutCellTags.Set(TEXT("Description"),TEXT("Description"));
	OutCellTags.Set(TEXT("PreviewImageMarkup"),TEXT("PreviewImageMarkup"));
}

/**
 * Retrieves the field type for the specified cell.
 *
 * @param	CellTag				the tag for the element cell to get the field type for
 * @param	OutCellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
 *
 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
 */
UBOOL UUTUIDataProvider_CharacterFaction::GetCellFieldType(const FName& CellTag,BYTE& OutCellFieldType)
{
	TMap<FName,FString> CellTags;
	GetElementCellTags(CellTags);
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
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the index of the value to fetch
 * @param	OutFieldValue	receives the resolved value for the property specified.
 * @param	ArrayIndex		ignored
 */
UBOOL UUTUIDataProvider_CharacterFaction::GetCellFieldValue(const FName& CellTag,INT ListIndex,FUIProviderFieldValue& OutFieldValue,INT)
{
	UBOOL bResult = FALSE;

	if(CellTag==TEXT("FriendlyName"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = CustomData.FriendlyName;
		bResult = TRUE;
	}
	else if(CellTag==TEXT("FactionID"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = CustomData.Faction;
		bResult = TRUE;
	}
	else if(CellTag==TEXT("Description"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = CustomData.Description;
		bResult = TRUE;
	}
	else if(CellTag==TEXT("PreviewImageMarkup"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = CustomData.PreviewImageMarkup;
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
void UUTUIDataProvider_CharacterFaction::GetSupportedDataFields( TArray<FUIDataProviderField>& out_Fields )
{
	out_Fields.Empty();

	TMap<FName,FString> Elements;
	GetElementCellTags(Elements);

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
UBOOL UUTUIDataProvider_CharacterFaction::GetFieldValue( const FString& FieldName, FUIProviderFieldValue& out_FieldValue, INT ArrayIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	FName PropertyFName(*FieldName, FNAME_Find);
	if ( PropertyFName != NAME_None )
	{
		bResult = GetCellFieldValue(PropertyFName,INDEX_NONE,out_FieldValue,ArrayIndex);
	}

	return bResult;
}



//////////////////////////////////////////////////////////////////////////
// UUTUIDataProvider_Character
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataProvider_Character);

/** Sets the Faction data for this provider. */
void UUTUIDataProvider_Character::SetData(FCharacterInfo &InData)
{
	CustomData = InData;
}

// IUIListElement interface

/**
 * Returns the names of the exposed members in the first entry in the array
 * of search results
 *
 * @param OutCellTags the columns supported by this row
 */
void UUTUIDataProvider_Character::GetElementCellTags( TMap<FName,FString>& OutCellTags )
{
	OutCellTags.Empty();
	OutCellTags.Set(TEXT("CharacterID"),TEXT("CharacterID"));
	OutCellTags.Set(TEXT("FriendlyName"),TEXT("FriendlyName"));
	OutCellTags.Set(TEXT("PreviewImageMarkup"),TEXT("PreviewImageMarkup"));
	OutCellTags.Set(TEXT("Description"),TEXT("Description"));
}

/**
 * Retrieves the field type for the specified cell.
 *
 * @param	CellTag				the tag for the element cell to get the field type for
 * @param	OutCellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
 *
 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
 */
UBOOL UUTUIDataProvider_Character::GetCellFieldType(const FName& CellTag,BYTE& OutCellFieldType)
{
	TMap<FName,FString> CellTags;
	GetElementCellTags(CellTags);
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
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the index of the value to fetch
 * @param	OutFieldValue	receives the resolved value for the property specified.
 * @param	ArrayIndex		ignored
 */
UBOOL UUTUIDataProvider_Character::GetCellFieldValue(const FName& CellTag,INT ListIndex,FUIProviderFieldValue& OutFieldValue,INT)
{
	UBOOL bResult = FALSE;

	if(CellTag==TEXT("FriendlyName"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = CustomData.CharName;
		bResult = TRUE;
	}
	else if(CellTag==TEXT("CharacterID"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = CustomData.CharID;
		bResult = TRUE;
	}
	else if(CellTag==TEXT("Description"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = CustomData.Description;
		bResult = TRUE;
	}
	else if(CellTag==TEXT("PreviewImageMarkup"))
	{
		OutFieldValue.PropertyTag = CellTag;
		OutFieldValue.PropertyType = DATATYPE_Property;
		OutFieldValue.StringValue = CustomData.PreviewImageMarkup;
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
void UUTUIDataProvider_Character::GetSupportedDataFields( TArray<FUIDataProviderField>& out_Fields )
{
	out_Fields.Empty();

	TMap<FName,FString> Elements;
	GetElementCellTags(Elements);

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
UBOOL UUTUIDataProvider_Character::GetFieldValue( const FString& FieldName, FUIProviderFieldValue& out_FieldValue, INT ArrayIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	FName PropertyFName(*FieldName, FNAME_Find);
	if ( PropertyFName != NAME_None )
	{
		bResult = GetCellFieldValue(PropertyFName,INDEX_NONE,out_FieldValue,ArrayIndex);
	}

	return bResult;
}






//////////////////////////////////////////////////////////////////////////
// UUTUIDataStore_Options
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataStore_Options);

/**
 * Initializes the dataproviders for all of the various character parts.
 */
void UUTUIDataStore_Options::InitializeDataStore()
{
	Super::InitializeDataStore();

	// Initialize all of the option providers, go backwards because of the way MultiMap appends items.
	TArray<UUIResourceDataProvider*> Providers;
	ListElementProviders.MultiFind(TEXT("OptionSets"), Providers);

	for ( INT ProviderIndex = Providers.Num()-1; ProviderIndex >= 0; ProviderIndex-- )
	{
		UUTUIDataProvider_MenuOption* DataProvider = Cast<UUTUIDataProvider_MenuOption>(Providers(ProviderIndex));
		if(DataProvider)
		{
			OptionProviders.Add(DataProvider->OptionSet, DataProvider);
		}
	}
}


/** 
 * Gets the list of element providers for a fieldname with filtered elements removed.
 *
 * @param FieldName				Fieldname to use to search for providers.
 * @param OutElementProviders	Array to store providers in.
 */
void UUTUIDataStore_Options::GetFilteredElementProviders(FName FieldName, TArray<UUTUIResourceDataProvider*> &OutElementProviders)
{
	OutElementProviders.Empty();

	TArray<UUTUIResourceDataProvider*> Providers;
	OptionProviders.MultiFind(FieldName, Providers);

	for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
	{
		UUTUIResourceDataProvider* DataProvider = Providers(ProviderIndex);
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
TArray<FName> UUTUIDataStore_Options::GetElementProviderTags()
{
	TArray<FName> Keys;
	TArray<FName> OptionSets;
	OptionProviders.GenerateKeyArray(Keys);

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
INT UUTUIDataStore_Options::GetElementCount( FName FieldName )
{
	TArray<UUTUIResourceDataProvider*> Providers;
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
UBOOL UUTUIDataStore_Options::GetListElements( FName FieldName, TArray<INT>& out_Elements )
{
	UBOOL bResult = FALSE;

	TArray<UUTUIResourceDataProvider*> Providers;
	OptionProviders.MultiFind(FieldName, Providers);

	if ( Providers.Num() > 0 )
	{
		out_Elements.Empty();
		for ( INT ProviderIndex = 0; ProviderIndex < Providers.Num(); ProviderIndex++ )
		{
			UUTUIResourceDataProvider* DataProvider = Providers(ProviderIndex);
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
UBOOL UUTUIDataStore_Options::GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	return Super::GetFieldValue(FieldName, out_FieldValue, ArrayIndex);
}

/**
 * Gets the list of data fields exposed by this data provider.
 *
 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
 */
void UUTUIDataStore_Options::GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields )
{
	out_Fields.Empty();

	TArray<FName> OptionSets;
	OptionProviders.GenerateKeyArray(OptionSets);

	for ( INT ProviderIndex = 0; ProviderIndex < OptionSets.Num(); ProviderIndex++ )
	{
		FName ProviderTag = OptionSets(ProviderIndex);

		TArray<UUTUIResourceDataProvider*> ResourceProviders;
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
TScriptInterface<class IUIListElementCellProvider> UUTUIDataStore_Options::GetElementCellSchemaProvider( FName FieldName )
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
TScriptInterface<class IUIListElementCellProvider> UUTUIDataStore_Options::GetElementCellValueProvider( FName FieldName, INT ListIndex )
{
	TScriptInterface<class IUIListElementCellProvider> Result;
	
	// search for the provider that has the matching tag
	TArray<UUTUIResourceDataProvider*> Providers;
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
UBOOL UUTUIDataStore_Options::SetFieldValue( const FString& FieldName, const struct FUIProviderScriptFieldValue& FieldValue, INT ArrayIndex )
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
UBOOL UUTUIDataStore_Options::IsElementEnabled( FName FieldName, INT CollectionIndex )
{
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
// UUTUIDataProvider_MenuOption
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIDataProvider_MenuOption);

/** @return Whether or not we should be filtered out of the list results. */
UBOOL UUTUIDataProvider_MenuOption::IsFiltered()
{
	UBOOL bFiltered = FALSE;

	UGameUISceneClient* SceneClient = UUIRoot::GetSceneClient();

	if(SceneClient)
	{
		UUIDataStore_Registry* Registry = Cast<UUIDataStore_Registry>(SceneClient->DataStoreManager->FindDataStore(TEXT("Registry")));

		if(Registry)
		{
			FUIProviderFieldValue OutFieldValue;

			appMemzero(&OutFieldValue, sizeof(FUIProviderFieldValue));

			if(Registry->GetDataStoreValue(TEXT("SelectedGameMode"), OutFieldValue))
			{
				bFiltered = (RequiredGameMode != NAME_None && RequiredGameMode != *OutFieldValue.StringValue);
			}
		}
	}

	return bFiltered || Super::IsFiltered();
}

//////////////////////////////////////////////////////////////////////////
// UUTDataStore_StringList
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(UUTUIDataStore_StringList);

/**
 * @param FieldName		Name of the String List to find
 * @return the index of a string list
 */

INT UUTUIDataStore_StringList::GetFieldIndex(FName FieldName)
{
	for (INT i=0;i<StringData.Num();i++)
	{
		if (StringData(i).Tag == FieldName)
		{
				return i;
		}
	}
	return -1;
}

/**
 * Retrieves the list of all data tags contained by this element provider which correspond to list element data.
 *
 * @return	the list of tags supported by this element provider which correspond to list element data.
 */
TArray<FName> UUTUIDataStore_StringList::GetElementProviderTags()
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
INT UUTUIDataStore_StringList::GetElementCount( FName FieldName )
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		return StringData(FieldIndex).Strings.Num();
	}
	return 0;
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
UBOOL UUTUIDataStore_StringList::GetListElements( FName FieldName, TArray<INT>& out_Elements )
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		for (INT i=0;i<StringData(FieldIndex).Strings.Num();i++)
		{
			out_Elements.AddUniqueItem(i);
		}
		return true;
	}
	return false;
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
TScriptInterface<class IUIListElementCellProvider> UUTUIDataStore_StringList::GetElementCellSchemaProvider( FName FieldName )
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		return TScriptInterface<IUIListElementCellProvider>(this);
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
TScriptInterface<class IUIListElementCellProvider> UUTUIDataStore_StringList::GetElementCellValueProvider( FName FieldName, INT ListIndex )
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		return TScriptInterface<IUIListElementCellProvider>(this);
	}
	return TScriptInterface<IUIListElementCellProvider>();
}

TScriptInterface<class IUIListElementProvider> UUTUIDataStore_StringList::ResolveListElementProvider( const FString& PropertyName ) 
{
	return TScriptInterface<IUIListElementProvider> (this);
}

/* === UIListElementCellProvider === */

/**
 * Retrieves the list of tags that can be bound to individual cells in a single list element, along with the human-readable,
 * localized string that should be used in the header for each cell tag (in lists which have column headers enabled).
 *
 * @param	out_CellTags	receives the list of tag/column headers that can be bound to element cells for the specified property.
 */
void UUTUIDataStore_StringList::GetElementCellTags( TMap<FName,FString>& out_CellTags )
{
	out_CellTags.Empty();
	for (INT i=0;i<StringData.Num(); i++)
	{
		out_CellTags.Set( StringData(i).Tag, *StringData(i).Tag.ToString() );			
	}
}

//	virtual void GetElementCellTags( TMap<FName,FString>& out_CellTags )=0;

/**
 * Retrieves the field type for the specified cell.
 *
 * @param	CellTag				the tag for the element cell to get the field type for
 * @param	out_CellFieldType	receives the field type for the specified cell; should be a EUIDataProviderFieldType value.
 *
 * @return	TRUE if this element cell provider contains a cell with the specified tag, and out_CellFieldType was changed.
 */
UBOOL UUTUIDataStore_StringList::GetCellFieldType( const FName& CellTag, BYTE& out_CellFieldType )
{
	return DATATYPE_Property;
}

/**
 * Resolves the value of the cell specified by CellTag and stores it in the output parameter.
 *
 * @param	CellTag			the tag for the element cell to resolve the value for
 * @param	ListIndex		the UIList's item index for the element that contains this cell.  Useful for data providers which
 *							do not provide unique UIListElement objects for each element.
 * @param	out_FieldValue	receives the resolved value for the property specified.
 *							@see GetDataStoreValue for additional notes
 * @param	ArrayIndex		optional array index for use with cell tags that represent data collections.  Corresponds to the
 *							ArrayIndex of the collection that this cell is bound to, or INDEX_NONE if CellTag does not correspond
 *							to a data collection.
 */
UBOOL UUTUIDataStore_StringList::GetCellFieldValue( const FName& CellTag, INT ListIndex, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex)
{
	INT FieldIndex = GetFieldIndex(CellTag);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		if (FieldIndex < StringData.Num() && ListIndex < StringData(FieldIndex).Strings.Num() )
		{
			out_FieldValue.StringValue = StringData(FieldIndex).Strings(ListIndex);
		}
		else
		{
			out_FieldValue.StringValue = FString::Printf(TEXT(""));
		}
		out_FieldValue.PropertyType = DATATYPE_Property;
		return true;
	}
	return false;
}


/* === UIDataProvider interface === */
/**
 * Gets the list of data fields exposed by this data provider.
 *
 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
 */
void UUTUIDataStore_StringList::GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields )
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
UBOOL UUTUIDataStore_StringList::GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex )
{
	INT FieldIndex = GetFieldIndex(FName(*FieldName));

	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		out_FieldValue.PropertyTag = *FieldName;
		out_FieldValue.PropertyType = DATATYPE_Property;
		out_FieldValue.StringValue = StringData(FieldIndex).CurrentValue;
		return true;
	}
	return false;
}

/**
 * Resolves the value of the data field specified and stores the value specified to the appropriate location for that field.
 *
 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
 * @param	FieldValue		the value to store for the property specified.
 * @param	ArrayIndex		optional array index for use with data collections
 */
UBOOL UUTUIDataStore_StringList::SetFieldValue( const FString& FieldName, const struct FUIProviderScriptFieldValue& FieldValue, INT ArrayIndex )
{
	INT FieldIndex = GetFieldIndex(FName(*FieldName));

	if ( FieldIndex > UCONST_INVALIDFIELD)
	{
		StringData(FieldIndex).CurrentValue = FieldValue.StringValue;
		return true;
	}
	return false;
}

/**
 * Adds a new field to the list
 *
 * @param	FieldName		the data field to resolve the value for
 * @param	NewString		The first string to add.
 * @returns the index of the new field
 */

INT UUTUIDataStore_StringList::AddNewField(FName FieldName, const FString &NewString)
{
	INT FieldIndex;
	FEStringListData NewData;
	appMemzero(&NewData, sizeof(FEStringListData));
	NewData.Tag = FieldName;
	NewData.CurrentValue = NewString;
	NewData.Strings.AddItem(NewString);

	FieldIndex = StringData.Num();
	StringData.AddZeroed(1);
	StringData(FieldIndex) = NewData;
	StringData(FieldIndex).CurrentValue = NewString;

	eventRefreshSubscribers();

	return FieldIndex;
}


/**
 * Add a string to the list
 *
 * @Param FieldName		The string list to work on
 * @Param NewString		The new string to add
 */

void UUTUIDataStore_StringList::AddStr(FName FieldName, const FString &NewString)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		StringData(FieldIndex).Strings.AddItem(NewString);
		if ( StringData(FieldIndex).Strings.Num() == 1 )
		{
			StringData(FieldIndex).CurrentValue = NewString;
		}

		eventRefreshSubscribers();

	}
	else	// Create a new list and prime it
	{
		AddNewField(FieldName, NewString);
	}
}

/**
 * Insert a string in to the list at a given index
 *
 * @Param FieldName		The string list to work on
 * @Param NewString		The new string to add
 * @Param InsertIndex	The index where you wish to insert the string
 */

void UUTUIDataStore_StringList::InsertStr(FName FieldName, const FString &NewString, INT InsertIndex)
{
	INT FieldIndex;
	
	// See if we can find this field.

	FieldIndex = GetFieldIndex(FieldName);

	if ( FieldIndex > UCONST_INVALIDFIELD )  // Found it, add the string
	{
		// Don't duplicate the strings

		INT StrIndex;
		if ( !StringData(FieldIndex).Strings.FindItem(NewString, StrIndex) )
		{
			StringData(FieldIndex).Strings.InsertItem(NewString, InsertIndex);
		}
		eventRefreshSubscribers();

	}
	else	// Create a new list and prime it
	{
		AddNewField(FieldName, NewString);
	}
}



/**
 * Remove a string from the list
 *
 * @Param FieldName		The string list to work on
 * @Param StringToRemove 	The string to remove
 */

void UUTUIDataStore_StringList::RemoveStr(FName FieldName, const FString &NewString)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		INT StrIndex;
		if ( StringData(FieldIndex).Strings.FindItem(NewString, StrIndex) )
		{
			StringData(FieldIndex).Strings.Remove(StrIndex,1);
		}
	}
	eventRefreshSubscribers();

}

/**
 * Remove a string (or multiple strings) by the index.
 *
 * @Param FieldName		The string list to work on
 * @Param Index			The index to remove
 * @Param Count			<Optional> # of strings to remove
 */

void UUTUIDataStore_StringList::RemoveStrByIndex(FName FieldName, INT Index, INT Count)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		StringData(FieldIndex).Strings.Remove(Index, Count);
	}
	eventRefreshSubscribers();

}

/**
 * Empty a string List
 *
 * @Param FieldName		The string list to work on
 */

void UUTUIDataStore_StringList::Empty(FName FieldName)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{
		StringData(FieldIndex).Strings.Empty();
	}
	eventRefreshSubscribers();

}

/**
 * Finds a string in the list
 *
 * @Param FieldName		The string list to add the new string to
 * @Param SearchStr		The string to find
 *
 * @returns the index in the list or INVALIDFIELD
 */

INT UUTUIDataStore_StringList::FindStr(FName FieldName, const FString &SearchString)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{	
		INT StrIndex = -1;
		if ( StringData(FieldIndex).Strings.FindItem(SearchString, StrIndex) )
		{
				return StrIndex;
		}
	}

	return UCONST_INVALIDFIELD;
}

/**
 * Returns the a string by the index
 *
 * @Param FieldName		The string list to add the new string to
 * @Param StrIndex		The index of the string to get
 *
 * @returns the string.
 */

FString UUTUIDataStore_StringList::GetStr(FName FieldName, INT StrIndex)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{	
		if ( StrIndex >= 0 && StrIndex < StringData(FieldIndex).Strings.Num() )
		{
			return StringData(FieldIndex).Strings(StrIndex);
		}
	}

	return FString::Printf(TEXT(""));
}

/**
 * Get a list
 *
 * @Param FieldName		The string list to add the new string to
 * @returns a copy of the list
 */

TArray<FString> UUTUIDataStore_StringList::GetList(FName FieldName)
{
	INT FieldIndex = GetFieldIndex(FieldName);
	if ( FieldIndex > UCONST_INVALIDFIELD )
	{	
		return StringData(FieldIndex).Strings;		
	}
	else	// Create a new list and prime it
	{
		FieldIndex = AddNewField(FieldName, TEXT("") );
		return StringData(FieldIndex).Strings;
	}
}

//////////////////////////////////////////////////////////////////////////
// UUTDataStore_OnlineStats
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTLeaderboardSettings);
IMPLEMENT_CLASS(UUTDataStore_OnlineStats);

/**
 * Loads and creates an instance of the registered filter object
 */
void UUTDataStore_OnlineStats::InitializeDataStore(void)
{
	Super::InitializeDataStore();
	// Create settings object
	LeaderboardSettings = ConstructObject<USettings>(LeaderboardSettingsClass);
	if (LeaderboardSettings != NULL)
	{
		SettingsProvider = ConstructObject<UUIDataProvider_Settings>(UUIDataProvider_Settings::StaticClass());
		if (SettingsProvider->BindSettings(LeaderboardSettings) == FALSE)
		{
			debugf(NAME_Error,TEXT("Failed to bind leaderboard filter settings object to %s"),
				*LeaderboardSettings->GetName());
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Failed to create leaderboard filter settings object %s"),
			*LeaderboardSettingsClass->GetName());
	}
}

/**
 * Returns the stats read results as a collection and appends the filter provider
 *
 * @param OutFields	out value that receives the list of exposed properties
 */
void UUTDataStore_OnlineStats::GetSupportedDataFields(TArray<FUIDataProviderField>& OutFields)
{
	Super::GetSupportedDataFields(OutFields);
	// Append our settings provider
	new(OutFields)FUIDataProviderField(FName(TEXT("Filters")),DATATYPE_Provider,SettingsProvider);
}



