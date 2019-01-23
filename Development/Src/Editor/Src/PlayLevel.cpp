/*=============================================================================
	PlayLevel.cpp: In-editor level playing.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "EngineAudioDeviceClasses.h"
#include "RemoteControl.h"
#include "LevelUtils.h"
#include "BusyCursor.h"
#include "ScopedTransaction.h"

UBOOL UEditorPlayer::Exec(const TCHAR* Cmd,FOutputDevice& Ar)
{
	UBOOL Handled = FALSE;

	if( ParseCommand(&Cmd,TEXT("CloseEditorViewport")) 
	||	ParseCommand(&Cmd,TEXT("Exit")) 
	||	ParseCommand(&Cmd,TEXT("Quit")))
	{
		ViewportClient->CloseRequested(ViewportClient->Viewport);
		Handled = TRUE;
	}
	else if( Super::Exec(Cmd,Ar) )
	{
		Handled = TRUE;
	}

	return Handled;
}
IMPLEMENT_CLASS(UEditorPlayer);

void UEditorEngine::EndPlayMap()
{
	const FScopedBusyCursor BusyCursor;
	check(PlayWorld);

#if USING_REMOTECONTROL
	if ( RemoteControlExec )
	{
		// Notify RemoteControl that PIE is ending.
		RemoteControlExec->OnEndPlayMap();
	}
#endif

	// let the editor know
	GCallbackEvent->Send(CALLBACK_EndPIE);

	// clean up any previous Play From Here sessions
	if ( GameViewport != NULL && GameViewport->Viewport != NULL )
	{
		GameViewport->CloseRequested(GameViewport->Viewport);
	}
	CleanupGameViewport();

	// no longer queued
	bIsPlayWorldQueued = FALSE;
	
	// Stop all audio and remove references to temp level.
	if( Client && Client->GetAudioDevice() )
	{
		Client->GetAudioDevice()->Flush();
	}

	// find objects like Textures in the playworld levels that won't get garbage collected as they are marked RF_Standalone
	for( FObjectIterator It; It; ++It )
	{
		UObject* Object = *It;
		if( Object->HasAnyFlags(RF_Standalone) && (Object->GetOutermost()->PackageFlags & PKG_PlayInEditor)  )
		{
			// Clear RF_Standalone flag from objects in the levels used for PIE so they get cleaned up.
			Object->ClearFlags(RF_Standalone);
		}
	}

	// Clean up the temporary play level.
	PlayWorld->TermWorldRBPhys();
	PlayWorld->CleanupWorld();
	PlayWorld = NULL;
	
	CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

	// Make sure that all objects in the temp levels were entirely garbage collected.
	for( FObjectIterator ObjectIt; ObjectIt; ++ObjectIt )
	{
		UObject* Object = *ObjectIt;
		if( Object->GetOutermost()->PackageFlags & PKG_PlayInEditor )
		{
			UObject::StaticExec(*FString::Printf(TEXT("OBJ REFS CLASS=WORLD NAME=%s.TheWorld"), *Object->GetOutermost()->GetName()));

			TMap<UObject*,UProperty*>	Route		= FArchiveTraceRoute::FindShortestRootPath( Object, TRUE, GARBAGE_COLLECTION_KEEPFLAGS );
			FString						ErrorString	= FArchiveTraceRoute::PrintRootPath( Route, Object );
				
			// We cannot safely recover from this.
			appErrorf( *LocalizeUnrealEd("Error_PIEObjectStillReferenced"), *Object->GetFullName(), *ErrorString );
		}
	}

	// Restore the world package's dirty flag.
	UPackage* Package = GWorld->GetOutermost();
	Package->MarkPackageDirty( bWorldPackageWasDirty );

	// Spawn note actors dropped in PIE.
	if(GEngine->PendingDroppedNotes.Num() > 0)
	{
		const FScopedTransaction Transaction( TEXT("Create PIE Notes") );

		for(INT i=0; i<GEngine->PendingDroppedNotes.Num(); i++)
		{
			FDropNoteInfo& NoteInfo = GEngine->PendingDroppedNotes(i);
			ANote* NewNote = Cast<ANote>( GWorld->SpawnActor(ANote::StaticClass(), NAME_None, NoteInfo.Location, NoteInfo.Rotation) );
			if(NewNote)
			{
				NewNote->Text = NoteInfo.Comment;
				NewNote->Tag = FName(*NoteInfo.Comment);
				NewNote->SetDrawScale(2.f);
			}
		}
		Package->MarkPackageDirty(TRUE);
		GEngine->PendingDroppedNotes.Empty();
	}

	// Restors realtime viewports that have been disabled for PIE.
	RestoreRealtimeViewports();

	GEditor->ResetAutosaveTimer();
}

void UEditorEngine::PlayMap(FVector* StartLocation, FRotator* StartRotation, INT Destination)
{
	// queue up a Play From Here request, this way the load/save won't conflict with the TransBuffer, which doesn't like 
	// loading and saving to happen during a transaction

	// save the StartLocation if we have one
	if (StartLocation)
	{
		PlayWorldLocation = *StartLocation;
		PlayWorldRotation = StartRotation ? *StartRotation : FRotator(0, 0, 0);
		bHasPlayWorldPlacement = true;
	}
	else
		bHasPlayWorldPlacement = false;

	// remember where to send the play map request
	PlayWorldDestination = Destination;

	// tell the editor to kick it off next Tick()
	bIsPlayWorldQueued = true;

	// Record the world package's dirty flag status so it can be reset after play.
	UPackage* Package = CastChecked<UPackage>( GWorld->GetOuter() );
	bWorldPackageWasDirty = Package->IsDirty();
}

void UEditorEngine::StartQueuedPlayMapRequest()
{
	// note that we no longer have a queued request
	bIsPlayWorldQueued = false;

	if (PlayWorldDestination == -1)
		PlayInEditor();
	else
		PlayOnConsole(PlayWorldDestination);
}

// @todo DB: appSaveAllWorlds declared this way because they're implemented in UnrealEd -- fix!

/**
 * Save all packages containing UWorld objects with the option to override their path and also
 * apply a prefix.
 *
 * @param	OverridePath	Path override, can be NULL
 * @param	Prefix			Optional prefix for base filename, can be NULL
 * @param	bIncludeGWorld	If TRUE, save GWorld along with other worlds.
 * @param	bCheckDirty		If TRUE, don't save level packages that aren't dirty.
 * @param	bPIESaving		Should be set to TRUE if saving for PIE; passed to UWorld::SaveWorld.
 * @return					TRUE if at least one level was saved.
 */
extern UBOOL appSaveAllWorlds(const TCHAR* OverridePath, const TCHAR* Prefix, UBOOL bIncludeGWorld, UBOOL bCheckDirty, UBOOL bPIESaving);

/**
 * Save all packages corresponding to the specified UWorlds, with the option to override their path and also
 * apply a prefix.
 *
 * @param	WorldsArray		The set of UWorlds to save.
 * @param	OverridePath	Path override, can be NULL
 * @param	Prefix			Optional prefix for base filename, can be NULL
 * @param	bIncludeGWorld	If TRUE, save GWorld along with other worlds.
 * @param	bCheckDirty		If TRUE, don't save level packages that aren't dirty.
 * @param	bPIESaving		Should be set to TRUE if saving for PIE; passed to UWorld::SaveWorld.
 * @return					TRUE if at least one level was saved.
 */
extern UBOOL appSaveWorlds(const TArray<UWorld*>& WorldsArray, const TCHAR* OverridePath, const TCHAR* Prefix, UBOOL bIncludeGWorld, UBOOL bCheckDirty, UBOOL bPIESaving);

/**
 * Assembles a list of worlds whose PIE packages need resaving.
 *
 * @param	FullAutoSaveDir
 * @param	FilenamePrefix				The PIE filename prefix.
 * @param	OutWorldsNeedingPIESave		[out] The list worlds that need to be saved for PIE.
 */
static void GetWorldsNeedingPIESave(const FString& FullAutoSaveDir, const TCHAR* FilenamePrefix, TArray<UWorld*>& OutWorldsNeedingPIESave)
{
	OutWorldsNeedingPIESave.Empty();

	// Get the set of all reference worlds.
	TArray<UWorld*> Worlds;
	FLevelUtils::GetWorlds( Worlds, TRUE );

	// Assemble a list of worlds that need to be PIE-saved.
	for ( INT WorldIndex = 0 ; WorldIndex < Worlds.Num() ; ++WorldIndex )
	{
		UWorld*		World = Worlds(WorldIndex);
		UPackage*	Package = Cast<UPackage>( World->GetOuter() );

		if ( Package )
		{
			UBOOL bNeedsPIESave = FALSE;

			if ( Package->IsDirty() )
			{
				// A PIE save is needed if the level is dirty.
				bNeedsPIESave = TRUE;
			}
			else
			{
				FString PackageName = Package->GetName();

				// Does a file already exist for this world package?
				FFilename ExistingFilename;
				const UBOOL bPackageExists = GPackageFileCache->FindPackageFile( *PackageName, NULL, ExistingFilename );

				if ( !bPackageExists )
				{
					// The world hasn't been saved before, and so it needs a PIE save.
					bNeedsPIESave = TRUE;
				}
				else
				{
					// Build a PIE filename from the existing filename.
					const FFilename	CleanFilename = ExistingFilename.GetCleanFilename();
					const FString	PIEFilename = FullAutoSaveDir + PATH_SEPARATOR + FString( FilenamePrefix ) + CleanFilename;

					const DOUBLE	PIEPackageAgeInSeconds = GFileManager->GetFileAgeSeconds(*PIEFilename);
					if ( PIEPackageAgeInSeconds < 0.0 )
					{
						// The world hasn't been PIE-saved before.
						bNeedsPIESave = TRUE;
					}
					else
					{
						// Compare the PIE package age against the existing package age.
						const DOUBLE ExisingPackageAgeInSeconds	= GFileManager->GetFileAgeSeconds(*ExistingFilename);
						if ( ExisingPackageAgeInSeconds < PIEPackageAgeInSeconds )
						{
							// The level has been saved since the last PIE session.
							bNeedsPIESave = TRUE;
						}
					}
					
				}
			}

			if ( bNeedsPIESave )
			{
				OutWorldsNeedingPIESave.AddItem( World );
			}
		}
	}
}

/**
 * Saves play in editor levels and also fixes up references in AWorldInfo to other levels.
 *
 * @param	Prefix				Prefix used to save files to disk.
 * @param	bPlayOnConsole		For PIE we only save the map, for POC (console), we save all packages so they can be baked, etc
 *
 * @return	False if the save failed and the user wants to abort what they were doing
 */
UBOOL UEditorEngine::SavePlayWorldPackages( const TCHAR* Prefix, UBOOL bPlayOnConsole )
{
	// if we are going to an offline destination (say Xenon), then we should save all packages so the latest content
	// is copied over to the destination
	if (bPlayOnConsole)
	{
		// if this returns false, it means we should stop what we're doing and return to the editor
        if (!GEditor->SaveDirtyPackages(false))
		{
			return false;
		}
	}

	// Update cull distance volumes before saving.
	GWorld->UpdateCullDistanceVolumes();

	// generate the proper path to put this into the Autosave directory
	FString	FullAutoSaveDir = FString(appBaseDir()) * AutoSaveDir;
	GFileManager->MakeDirectory(*FullAutoSaveDir, 1);

	UBOOL bSaved = TRUE;

	// save the world and levels out to a temporary package (kind of like autosave) and keep track of package names
	if (bPlayOnConsole)
	{
		bSaved = appSaveAllWorlds( *FullAutoSaveDir, Prefix, TRUE, FALSE, TRUE );
	}
	else
	{
		TArray<UWorld*> WorldsNeedingPIESave;
		GetWorldsNeedingPIESave( FullAutoSaveDir, Prefix, WorldsNeedingPIESave );

		// Record dirty flags for world packages to be PIE saved.
		TArray<UBOOL> bDirtyWorlds;
		for ( INT WorldIndex = 0 ; WorldIndex < WorldsNeedingPIESave.Num() ; ++WorldIndex )
		{
			UWorld*		World = WorldsNeedingPIESave( WorldIndex );
			UPackage*	Package = Cast<UPackage>( World->GetOuter() );
			const UBOOL bWorldDirty = ( Package && Package->IsDirty() );
			bDirtyWorlds.AddItem( bWorldDirty );
		}

		debugf(NAME_Log, TEXT("====\nWorlds needing PIE Save:") );
		for ( INT i = 0 ; i < WorldsNeedingPIESave.Num() ; ++i )
		{
			UWorld*			World = WorldsNeedingPIESave(i);
			UPackage*		Package = Cast<UPackage>( World->GetOuter() );
			FString			PackageName = Package->GetName();
			debugf(NAME_Log, TEXT("%s"), *PackageName);
		}
		debugf(NAME_Log, TEXT("==== %i total"), WorldsNeedingPIESave.Num());

		if (WorldsNeedingPIESave.Num() > 0)
		{
			bSaved = appSaveWorlds( WorldsNeedingPIESave, *FullAutoSaveDir, Prefix, TRUE, FALSE, TRUE );
		}

		// Restore dirty flags for PIE-saved worlds.
		for ( INT WorldIndex = 0 ; WorldIndex < WorldsNeedingPIESave.Num() ; ++WorldIndex )
		{
			UWorld*		World = WorldsNeedingPIESave( WorldIndex );
			UPackage*	Package = Cast<UPackage>( World->GetOuter() );
			const UBOOL bWorldDirty = bDirtyWorlds( WorldIndex );
			if ( Package )
			{
				Package->MarkPackageDirty( bWorldDirty );
			}
		}
	}

	return bSaved;
}

/**
 * Builds a URL for game spawned by the editor (not including map name!). Has stuff like the teleporter, spectating, etc.
 * @param	MapName		The name of the map to put into the URL
 *
 * @return	The URL for the game
 */
FString UEditorEngine::BuildPlayWorldURL(const TCHAR* MapName)
{
	// the URL we are building up
	FString URL(MapName);

	if (bHasPlayWorldPlacement)
	{
		// tell the player to start in at the teleporter
		URL += TEXT("#PlayWorldStart");
	}

	// If we hold down control, start in spectating mode
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		URL += TEXT("?SpectatorOnly=1");
	}

	// Add any game-specific options set in the .ini file
	URL += InEditorGameURLOptions;

	return URL;
}


/**
 * Spawns a teleporter in the given world
 * @param	World	The World to spawn in (for PIE this may not be GWorld)
 *
 * @return	The spawned teleporter actor
 */
UBOOL UEditorEngine::SpawnPlayFromHereTeleporter(UWorld* World, ATeleporter*& PlayerStart)
{
	// null it out in case we don't need to spawn one, and the caller relies on us setting it
	PlayerStart = NULL;

	// are we doing Play From here?
	if (bHasPlayWorldPlacement)
	{
		// spawn the Teleporter in the given world
		PlayerStart = Cast<ATeleporter>(World->SpawnActor(ATeleporter::StaticClass(), NAME_None, PlayWorldLocation, PlayWorldRotation));
		// make sure we were able to spawn the teleporter there
		if(!PlayerStart)
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Prompt_22"));
			return false;
		}
		// add the teleporter to the nav list
		World->PersistentLevel->AddToNavList(PlayerStart);
		// name the teleporter
		PlayerStart->Tag = TEXT("PlayWorldStart");
	}
	// true means we didn't need to spawn, or we succeeded
	return true;
}

void UEditorEngine::PlayInEditor()
{
	// Prompt the user that Matinee must be closed before PIE can occur.
	if( GEditorModeTools().GetCurrentModeID() == EM_InterpEdit )
	{
		const UBOOL bContinuePIE = appMsgf( AMT_YesNo, *LocalizeUnrealEd(TEXT("PIENeedsToCloseMatineeQ")) );
		if ( !bContinuePIE )
		{
			return;
		}
		GEditorModeTools().SetCurrentMode( EM_Default );
	}

	const FScopedBusyCursor BusyCursor;

	// pause any object propagation
	GObjectPropagator->Pause();

	// If there's level already being played, close it.
	if(PlayWorld)
	{
		// immediately end the playworld
		EndPlayMap();
	}

	// save out the map to disk and get it's filename
	if (!SavePlayWorldPackages(PLAYWORLD_PACKAGE_PREFIX, FALSE))
	{
		// false from this function means to stop what we're doing and return to the editor
		return;
	}

	// remember out old GWorld
	UWorld* OldGWorld = GWorld;

	FString PlayWorldMapName = FString(PLAYWORLD_PACKAGE_PREFIX) + *GWorld->GetOutermost()->GetName();

	// load up a new UWorld but keep the old one
	Exec(*FString::Printf( TEXT("MAP LOAD PLAYWORLD=1 FILE=\"%s\""), *PlayWorldMapName));

	if (!GWorld)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Error_FailedCreateEditorPreviewWorld"));
		// The load failed, so restore GWorld.
		RestoreEditorWorld( OldGWorld );
		return;
	}

	// remember what we just loaded
	PlayWorld = GWorld;
	// make sure we can clean up this world!
	PlayWorld->ClearFlags(RF_Standalone);

	// If a start location is specified, spawn a temporary Teleporter actor at the start location and use it as the portal.
	ATeleporter* PlayerStart = NULL;
	if (SpawnPlayFromHereTeleporter(PlayWorld, PlayerStart) == FALSE)
	{
		// go back to using the real world as GWorld
		GWorld = OldGWorld;
		EndPlayMap();
		return;
	}

	SetPlayInEditorWorld( PlayWorld );

	// make a URL
	FURL URL(NULL, *BuildPlayWorldURL( *PlayWorldMapName ), TRAVEL_Absolute);
	PlayWorld->SetGameInfo(URL);
	PlayWorld->BeginPlay(URL);

	// Initialize the viewport client.
	UGameViewportClient* ViewportClient = NULL;
	if(Client)
	{
		ViewportClient = ConstructObject<UGameViewportClient>(GameViewportClientClass,this);
		GameViewport = ViewportClient;

		FString Error;
		if(!ViewportClient->eventInit(Error))
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Error_CouldntSpawnPlayer"),*Error);
			// go back to using the real world as GWorld
			RestoreEditorWorld( OldGWorld );
			EndPlayMap();
			return;
		}
	}

	// Spawn the player's actor.
	for(FPlayerIterator It(this);It;++It)
	{
		FString Error;
		if(!It->SpawnPlayActor(URL.String(1),Error))
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Error_CouldntSpawnPlayer"),*Error);
			// go back to using the real world as GWorld
			RestoreEditorWorld( OldGWorld );
			EndPlayMap();
			return;
		}
	}

#if USING_REMOTECONTROL
	if ( RemoteControlExec )
	{
		// Notify RemoteControl that PIE is beginning
		RemoteControlExec->OnPlayInEditor( PlayWorld );
	}
#endif

	// Open initial Viewport.
	if( Client )
	{
		DisableRealtimeViewports();

		// Create a viewport for the local player.

		// Note that we are naming this with a hardcoded value. This is how the PreWindowMessage callback (used in FCallbackDeviceEditor)
		// knows that the viewport receiving input is the playworld viewport and to switch the GWorld to the PlayWorld.
		// If the name changes to something else, the code in "FCallbackDeviceEditor::Send(ECallbackType, FViewport*, UINT)"
		// will need to be updated as well.

		// Note again that we are no longer supporting fullscreen for in-editor playing, so we pass in 0 as the last parameter.
		// As well, we use the hardcoded name in the Viewport code to make sure we can't go fullscreen. If the name changes,
		// the code in FWindowsViewport::Resize() will have to change.

		FViewportFrame* ViewportFrame = Client->CreateViewportFrame(
			ViewportClient,
			PLAYWORLD_VIEWPORT_NAME,//*LocalizeGeneral("Product",GPackage),
			Client->StartupResolutionX,
			Client->StartupResolutionY,
			0
			);
		ViewportClient->SetViewport( ViewportFrame );
	}

	// go back to using the real world as GWorld
	RestoreEditorWorld( OldGWorld );

	// unpause any object propagation
	GObjectPropagator->Unpause();
}


void UEditorEngine::PlayOnConsole(INT ConsoleIndex)
{
	// pause any object propagation
	GObjectPropagator->Pause();

	// get the platform that we want to play on
	FConsoleSupport* Console = GConsoleSupportContainer->GetConsoleSupport(ConsoleIndex);

	// allow for a teleporter to make the player appear at the "Play Form Here" location
	ATeleporter* PlayerStart = NULL;
	// if it failed abort early
	if (SpawnPlayFromHereTeleporter(GWorld, PlayerStart) == FALSE)
	{
		return;
	}

	// make a per-platform name for the map
	FString Prefix	= FString(PLAYWORLD_CONSOLE_BASE_PACKAGE_PREFIX) + FString(Console->GetConsoleName());
	FString MapName	= Prefix + *GWorld->GetOutermost()->GetName();

	// save out all open packages to be synced to xenon
	if (!SavePlayWorldPackages(*Prefix, TRUE))
	{
		// false from this function means to stop what we're doing and return to the editor
		// remove the teleporter from the world
		if (PlayerStart)
		{
			GWorld->DestroyActor(PlayerStart);
		}
		return;
	}

	// remove the teleporter from the world
	if (PlayerStart)
	{
		GWorld->DestroyActor(PlayerStart);
	}

	// let the platform do what it wants to run the map
	TCHAR OutputConsoleCommand[1024] = TEXT("\0");
	Console->RunGame(*BuildPlayWorldURL(*MapName), OutputConsoleCommand, sizeof(OutputConsoleCommand));

	// if the RunGame command needs to exec a command, do it now
	if (OutputConsoleCommand[0] != 0)
	{
		FString Temp(OutputConsoleCommand);
		GEngine->Exec(*Temp, *GLog);
	}

	// resume object propagation
	GObjectPropagator->Unpause();
}
