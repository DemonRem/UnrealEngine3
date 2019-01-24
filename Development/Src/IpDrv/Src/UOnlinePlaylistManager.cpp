/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "UnIpDrv.h"

#if WITH_UE3_NETWORKING

#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"

IMPLEMENT_CLASS(UOnlinePlaylistManager);

/**
 * Uses the configuration data to create the requested objects and then applies any
 * specific game settings changes to them
 */
void UOnlinePlaylistManager::FinalizePlaylistObjects(void)
{
	// Process the config entries creating and updating as specified
	for (INT PlaylistIndex = 0; PlaylistIndex < Playlists.Num(); PlaylistIndex++)
	{
		FPlaylist& Playlist = Playlists(PlaylistIndex);
		// Create each game setting object and update its data
		for (INT GameIndex = 0; GameIndex < Playlist.ConfiguredGames.Num(); GameIndex++)
		{
			FConfiguredGameSetting& ConfiguredGame = Playlist.ConfiguredGames(GameIndex);
			// If there is a valid class name specified try to load it and instance it
			if (ConfiguredGame.GameSettingsClassName.Len())
			{
				UClass* GameSettingsClass = LoadClass<UOnlineGameSettings>(NULL,
					*ConfiguredGame.GameSettingsClassName,
					NULL,
					LOAD_None,
					NULL);
				if (GameSettingsClass)
				{
					// Now create an instance with that class
					ConfiguredGame.GameSettings = ConstructObject<UOnlineGameSettings>(GameSettingsClass);
					if (ConfiguredGame.GameSettings)
					{
						// Update the game object with these settings, if not using the defaults
						if (ConfiguredGame.URL.Len())
						{
							ConfiguredGame.GameSettings->UpdateFromURL(ConfiguredGame.URL,NULL);
						}
					}
					else
					{
						debugf(NAME_DevOnline,
							TEXT("Failed to create class (%s) for playlist (%s)"),
							*ConfiguredGame.GameSettingsClassName,
							*Playlist.Name);
					}
				}
				else
				{
					debugf(NAME_DevOnline,
						TEXT("Failed to load class (%s) for playlist (%s)"),
						*ConfiguredGame.GameSettingsClassName,
						*Playlist.Name);
				}
			}
		}
	}
	if (DatastoresToRefresh.Num())
	{
		INT DatastoreIndex = INDEX_NONE;
		// Iterate through the registered set of datastores and refresh them
		for (TObjectIterator<UUIDataStore_GameResource> ObjIt; ObjIt; ++ObjIt)
		{
			DatastoresToRefresh.FindItem(ObjIt->Tag,DatastoreIndex);
			// Don't refresh it if it isn't in our list
			if (DatastoreIndex != INDEX_NONE)
			{
				(*ObjIt)->InitializeListElementProviders();
			}
		}
	}
}

/** Uses the current loc setting and game ini name to build the download list */
void UOnlinePlaylistManager::DetermineFilesToDownload(void)
{
	PlaylistFileNames.Empty(4);
	// Build the game specific playlist ini
	PlaylistFileNames.AddItem(FString::Printf(TEXT("%sPlaylist.ini"),appGetGameName()));
	FFilename GameIni(GGameIni);
	// Add the game ini for downloading per object config
	PlaylistFileNames.AddItem(GameIni.GetCleanFilename());
	// Now build the loc file name from the ini filename
	PlaylistFileNames.AddItem(FString::Printf(TEXT("Engine.%s"),*appGetLanguageExt()));
	PlaylistFileNames.AddItem(FString::Printf(TEXT("%sGame.%s"),appGetGameName(),*appGetLanguageExt()));
}

/**
 * Converts the data into the structure used by the playlist manager
 *
 * @param Data the data that was downloaded
 */
void UOnlinePlaylistManager::ParsePlaylistPopulationData(const TArray<BYTE>& Data)
{
	// Make sure the string is null terminated
	((TArray<BYTE>&)Data).AddItem(0);
	// Convert to a string that we can work with
	FString StrData = ANSI_TO_TCHAR((ANSICHAR*)Data.GetTypedData());
	TArray<FString> Lines;
	// Now split into lines
	StrData.ParseIntoArray(&Lines,TEXT("\r\n"),TRUE);
	FString Token(TEXT("="));
	FString Right;
	// Mimic how the config cache stores them by removing the PopulationData= from the entries
	for (INT Index = 0; Index < Lines.Num(); Index++)
	{
		if (Lines(Index).Split(Token,NULL,&Right))
		{
			Lines(Index) = Right;
		}
	}
	// If there was data in the file, then import that into the array
	if (Lines.Num() > 0)
	{
		// Find the property on our object
		UArrayProperty* Array = FindField<UArrayProperty>(GetClass(),FName(TEXT("PopulationData")));
		if (Array != NULL)
		{
			const INT Size = Array->Inner->ElementSize;
			FScriptArray* ArrayPtr = (FScriptArray*)((BYTE*)this + Array->Offset);
			// Destroy any data that was already there
			Array->DestroyValue(ArrayPtr);
			// Size everything to the number of lines that were downloaded
			ArrayPtr->AddZeroed(Lines.Num(),Size);
			// Import each line of the population data
			for (INT ArrayIndex = Lines.Num() - 1, Count = 0; ArrayIndex >= 0; ArrayIndex--, Count++)
			{
				Array->Inner->ImportText(*Lines(ArrayIndex),(BYTE*)ArrayPtr->GetData() + Count * Size,PPF_ConfigOnly,this);
			}
		}
	}
	WorldwideTotalPlayers = RegionTotalPlayers = 0;
	// Total up the worldwide and region data
	for (INT DataIndex = 0; DataIndex < PopulationData.Num(); DataIndex++)
	{
		const FPlaylistPopulation& Data = PopulationData(DataIndex);
		WorldwideTotalPlayers += Data.WorldwideTotal;
		RegionTotalPlayers += Data.RegionTotal;
	}
}

/**
 * Determines whether an update of the playlist population information is needed or not
 *
 * @param DeltaTime the amount of time that has passed since the last tick
 */
void UOnlinePlaylistManager::Tick(FLOAT DeltaTime)
{
	UBOOL bNeedsAnUpdate = FALSE;
	// Determine if we've passed our update window and mark that we need an update
	// NOTE: to handle starting a match, reporting, and quiting, the code has to always
	// tick the interval so we don't over/under report
	NextPlaylistPopulationUpdateTime += DeltaTime;
	if (NextPlaylistPopulationUpdateTime >= PlaylistPopulationUpdateInterval &&
		PlaylistPopulationUpdateInterval > 0.f)
	{
		bNeedsAnUpdate = TRUE;
		NextPlaylistPopulationUpdateTime = 0.f;
	}
	// We can only update if we are the server
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
	if (WorldInfo != NULL &&
		WorldInfo->NetMode != NM_Standalone &&
		WorldInfo->NetMode != NM_Client &&
		// Don't send updates when we aren't playing a playlist
		CurrentPlaylistId > MinPlaylistIdToReport)
	{
		if (bNeedsAnUpdate)
		{
			INT NumPlayers = 0;
			// Work through the controller list counting players and skipping bots
			for (AController* Controller = WorldInfo->ControllerList; Controller; Controller = Controller->NextController)
			{
				APlayerController* PC = Cast<APlayerController>(Controller);
				if (PC)
				{
					NumPlayers++;
				}
			}
			eventSendPlaylistPopulationUpdate(NumPlayers);
		}
	}
}


/**
 * Converts the data into the datacenter id
 *
 * @param Data the data that was downloaded
 */
void UOnlinePlaylistManager::ParseDataCenterId(const TArray<BYTE>& Data)
{
	// Make sure the string is null terminated
	((TArray<BYTE>&)Data).AddItem(0);
	// Convert to a string that we can work with
	const FString StrData = ANSI_TO_TCHAR((ANSICHAR*)Data.GetTypedData());
	// Find the property on our object
	UIntProperty* Property = FindField<UIntProperty>(GetClass(),FName(TEXT("DataCenterId")));
	if (Property != NULL)
	{
		// Import the text sent to us by the network layer
		if (Property->ImportText(*StrData,(BYTE*)this + Property->Offset,PPF_ConfigOnly,this) == NULL)
		{
			debugf(NAME_Error,
				TEXT("LoadConfig (%s): import failed for %s in: %s"),
				*GetPathName(),
				*Property->GetName(),
				*StrData);
		}
	}
}

#endif
