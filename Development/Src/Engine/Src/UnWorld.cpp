/*=============================================================================
	UnWorld.cpp: UWorld implementation
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineDecalClasses.h"
#include "UnNet.h"
#include "EngineSequenceClasses.h"
#include "UnStatChart.h"
#include "UnPath.h"
#include "EngineAudioDeviceClasses.h"
#include "DemoRecording.h"

/*-----------------------------------------------------------------------------
	UWorld implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UWorld);

/** Global world pointer */
UWorld* GWorld = NULL;
extern FParticleDataManager	GParticleDataManager;

/**
 * UWorld constructor called at game startup and when creating a new world in the Editor.
 * Please note that this constructor does NOT get called when a world is loaded from disk.
 *
 * @param	InURL	URL associated with this world.
 */
UWorld::UWorld( const FURL& InURL )
:	URL(InURL)
,	DecalManager(NULL)
#if !FINAL_RELEASE
,	bIsLevelStreamingFrozen(FALSE)
#endif
{
	SetFlags( RF_Transactional );
}

/**
 * Static constructor, called once during static initialization of global variables for native 
 * classes. Used to e.g. register object references for native- only classes required for realtime
 * garbage collection or to associate UProperties.
 */
void UWorld::StaticConstructor()
{
	UClass* TheClass = GetClass();
	TheClass->EmitObjectReference( STRUCT_OFFSET( UWorld, PersistentLevel ) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( UWorld, SaveGameSummary ) );
	TheClass->EmitObjectArrayReference( STRUCT_OFFSET( UWorld, Levels ) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( UWorld, CurrentLevel ) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( UWorld, CurrentLevelPendingVisibility ) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( UWorld, NetDriver ) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( UWorld, DemoRecDriver ) );
	TheClass->EmitObjectArrayReference( STRUCT_OFFSET( UWorld, NewlySpawned ) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( UWorld, LineBatcher ) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( UWorld, PersistentLineBatcher ) );
	TheClass->EmitObjectReference( STRUCT_OFFSET( UWorld, DecalManager ) );
	TheClass->EmitObjectArrayReference( STRUCT_OFFSET( UWorld, ExtraReferencedObjects ) );
}

/**
 * Serialize function.
 *
 * @param Ar	Archive to use for serialization
 */
void UWorld::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	Ar << PersistentLevel;

	Ar << EditorViews[0];
	Ar << EditorViews[1];
	Ar << EditorViews[2];
	Ar << EditorViews[3];

	Ar << SaveGameSummary;

	if ( Ar.Ver() >= VER_ADDED_DECAL_MANAGER )
	{
		Ar << DecalManager;
	}

	if( !Ar.IsLoading() && !Ar.IsSaving() )
	{
		Ar << Levels;
		Ar << CurrentLevel;
		Ar << URL;

		Ar << NetDriver;
		Ar << DemoRecDriver;
		
		Ar << LineBatcher;
		Ar << PersistentLineBatcher;
	}

	if (Ar.Ver() >= VER_ADDED_WORLD_EXTRA_REFERENCED_OBJECTS)
	{
		Ar << ExtraReferencedObjects;
	}

	// Mark archive and package as containing a map if we're serializing to disk.
	if( !HasAnyFlags( RF_ClassDefaultObject ) && Ar.IsPersistent() )
	{
		Ar.ThisContainsMap();
		GetOutermost()->ThisContainsMap();
	}
}

/**
 * Destroy function, cleaning up world components, delete octree, physics scene, ....
 */
void UWorld::FinishDestroy()
{
	// Avoid cleanup if the world hasn't been initialized. E.g. the default object or a world object that got loaded
	// due to level streaming.
	if( bIsWorldInitialized )
	{
		// Delete octree.
		delete Hash;
		Hash = NULL;

		// Delete navigation octree.
		delete NavigationOctree;
		NavigationOctree = NULL;

		// Release scene.
		Scene->Release();
		Scene = NULL;
	}
	else
	{
		check(Hash==NULL);
	}

	Super::FinishDestroy();
}

/**
 * Called after the object has been serialized. Currently ensures that CurrentLevel gets initialized as
 * it is required for saving secondary levels in the multi- level editing case.
 */
void UWorld::PostLoad()
{
	Super::PostLoad();
	CurrentLevel = PersistentLevel;
}

/**
 * Called from within SavePackage on the passed in base/ root. The return value of this function will be passed to
 * PostSaveRoot. This is used to allow objects used as base to perform required actions before saving and cleanup
 * afterwards.
 * @param Filename: Name of the file being saved to (includes path)
 *
 * @return	Whether PostSaveRoot needs to perform internal cleanup
 */
UBOOL UWorld::PreSaveRoot(const TCHAR* Filename)
{
	// allow default gametype an opportunity to modify the WorldInfo's GameTypesSupportedOnThisMap array before we save it
	UClass* GameClass = StaticLoadClass(AGameInfo::StaticClass(), NULL, TEXT("game-ini:Engine.GameInfo.DefaultGame"), NULL, LOAD_None, NULL);
	if (GameClass != NULL)
	{
		GameClass->GetDefaultObject<AGameInfo>()->AddSupportedGameTypes(GetWorldInfo(), Filename);
	}

	// Update components and keep track off whether we need to clean them up afterwards.
	UBOOL bCleanupIsRequired = PersistentLevel->bAreComponentsCurrentlyAttached == FALSE;
	PersistentLevel->UpdateComponents();
	return bCleanupIsRequired;
}

/**
 * Called from within SavePackage on the passed in base/ root. This function is being called after the package
 * has been saved and can perform cleanup.
 *
 * @param	bCleanupIsRequired	Whether PreSaveRoot dirtied state that needs to be cleaned up
 */
void UWorld::PostSaveRoot( UBOOL bCleanupIsRequired )
{
	if( bCleanupIsRequired )
	{
		PersistentLevel->ClearComponents();
	}
}

/**
 * Saves this world.  Safe to call on non-GWorld worlds.
 *
 * @param	Filename					The filename to use.
 * @param	bForceGarbageCollection		Whether to force a garbage collection before saving.
 * @param	bAutosaving					If TRUE, don't perform optional caching tasks.
 * @param	bPIESaving					If TRUE, don't perform tasks that will conflict with editor state.
 */
UBOOL UWorld::SaveWorld( const FString& Filename, UBOOL bForceGarbageCollection, UBOOL bAutosaving, UBOOL bPIESaving )
{
	check(PersistentLevel);
	check(GIsEditor);

	UBOOL bWasSuccessful = false;

	// pause propagation
	GObjectPropagator->Pause();

	// Don't bother with static mesh physics data or shrinking when only autosaving.
	if ( bAutosaving )
	{
		PersistentLevel->ClearPhysStaticMeshCache();
	}
	else
	{
		PersistentLevel->BuildPhysStaticMeshCache();
	}

	// Shrink model and clean up deleted actors.
	// Don't do this when autosaving or PIE saving so that actor adds can still undo.
	if ( !bPIESaving && !bAutosaving )
	{
		ShrinkLevel();
	}

	// Reset actor creation times.
	for( FActorIterator It; It; ++It )
	{
		AActor* Actor = *It;
		Actor->CreationTime = 0.0f;
	}

	if( bForceGarbageCollection )
	{
		// NULL empty or "invalid" entries (e.g. bDeleteMe) in actors array.
		UObject::CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );
	}

	// Compact and sort actors array to remove empty entries.
	// Don't do this when autosaving or PIE saving so that actor adds can still undo.
	if ( !bPIESaving && !bAutosaving )
	{
		PersistentLevel->SortActorList();
	}

	// Temporarily flag packages saved under a PIE filename as PKG_PlayInEditor for serialization so loading
	// them will have the flag set. We need to undo this as the object flagged isn't actually the PIE package, 
	// but rather only the loaded one will be.
	UPackage*	WorldPackage			= GetOutermost();
	DWORD		OriginalPIEFlagValue	= WorldPackage->PackageFlags & PKG_PlayInEditor;
	// PIE prefix detected, mark package.
	if( Filename.InStr( PLAYWORLD_PACKAGE_PREFIX ) != INDEX_NONE )
	{
		WorldPackage->PackageFlags |= PKG_PlayInEditor;
	}

	// Save package.
	const UBOOL bWarnOfLongFilename = !(bAutosaving | bPIESaving);
	bWasSuccessful = SavePackage(WorldPackage, this, 0, *Filename, GWarn, NULL, FALSE, bWarnOfLongFilename);
	if (!bWasSuccessful)
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("Error_CouldntSavePackage") );
	}
	
	// Restore original value of PKG_PlayInEditor.
	WorldPackage->PackageFlags &= ~PKG_PlayInEditor;
	WorldPackage->PackageFlags |= OriginalPIEFlagValue;

	// Empty the static mesh physics cache if necessary.
	if ( !bAutosaving )
	{
		PersistentLevel->ClearPhysStaticMeshCache();
	}

	// resume propagation
	GObjectPropagator->Unpause();

	return bWasSuccessful;
}

/**
 * Initializes the world, associates the persistent level and sets the proper zones.
 */
void UWorld::Init()
{
	if( PersistentLevel->GetOuter() != this )
	{
		// Move persistent level into world so the world object won't get garbage collected in the multi- level
		// case as it is still referenced via the level's outer. This is required for multi- level editing to work.
		PersistentLevel->Rename( *PersistentLevel->GetName(), this );
	}

	// Allocate the world's hash, navigation octree and scene.
	Hash				= new FPrimitiveOctree();
	NavigationOctree	= new FNavigationOctree();
	Scene				= AllocateScene( this, FALSE, TRUE );

	if ( DecalManager == NULL )
	{
		// No decal manager exists; create one.
		DecalManager		= CastChecked<UDecalManager>( StaticConstructObject( UDecalManager::StaticClass(), this ) );
		DecalManager->Init();
	}
	else
	{
		// A decal manager exists; update it with any policies that were added since the manager was first created.
		DecalManager->UpdatePolicies();
	}

	URL					= PersistentLevel->URL;
	CurrentLevel		= PersistentLevel;

	// See whether we're missing the default brush. It was possible in earlier builds to accidentaly delete the default
	// brush of sublevels so we simply spawn a new one if we encounter it missing.
	ABrush* DefaultBrush = PersistentLevel->Actors.Num()<2 ? NULL : Cast<ABrush>(PersistentLevel->Actors(1));
	if( GIsEditor && (!DefaultBrush || !DefaultBrush->IsStaticBrush() || !DefaultBrush->CsgOper==CSG_Active || !DefaultBrush->BrushComponent || !DefaultBrush->Brush) )
	{
		debugf( TEXT("Encountered missing default brush - spawning new one") );

		// Spawn the default brush.
		DefaultBrush = SpawnBrush();
		check(DefaultBrush->BrushComponent);
		DefaultBrush->Brush = new( GetOuter(), TEXT("Brush") )UModel( DefaultBrush, 1 );
		DefaultBrush->BrushComponent->Brush = DefaultBrush->Brush;
		DefaultBrush->SetFlags( RF_NotForClient | RF_NotForServer | RF_Transactional );
		DefaultBrush->Brush->SetFlags( RF_NotForClient | RF_NotForServer | RF_Transactional );

		// Find the index in the array the default brush has been spawned at. Not necessarily
		// the last index as the code might spawn the default physics volume afterwards.
		const INT DefaultBrushActorIndex = PersistentLevel->Actors.FindItemIndex( DefaultBrush );

		// The default brush needs to reside at index 1.
		Exchange(PersistentLevel->Actors(1),PersistentLevel->Actors(DefaultBrushActorIndex));

		// Re-sort actor list as we just shuffled things around.
		PersistentLevel->SortActorList();
	}

	Levels.Empty(1);
	Levels.AddItem( PersistentLevel );

	AWorldInfo* WorldInfo = GetWorldInfo();
	for( INT ActorIndex=0; ActorIndex<PersistentLevel->Actors.Num(); ActorIndex++ )
	{
		AActor* Actor = PersistentLevel->Actors(ActorIndex);
		if( Actor )
		{
			// We can't do this in PostLoad as GetWorldInfo() will point to the old WorldInfo.
			Actor->WorldInfo = WorldInfo;
			Actor->SetZone( 0, 1 );
		}
	}

	// If in the editor, load secondary levels.
	if( GIsEditor )
	{
		WorldInfo->LoadSecondaryLevels();
	}

	// We're initialized now.
	bIsWorldInitialized = TRUE;
}

/**
 * Static function that creates a new UWorld and replaces GWorld with it.
 */
void UWorld::CreateNew()
{
	// Clean up existing world and remove it from root set so it can be garbage collected.
	if( GWorld )
	{
		GWorld->FlushLevelStreaming( NULL, TRUE );
		GWorld->TermWorldRBPhys();
		GWorld->CleanupWorld();
		GWorld->RemoveFromRoot();
		GWorld = NULL;
	}

	// Create a new package unless we're a commandlet in which case we keep the dummy world in the transient package.
	UPackage*	WorldPackage		= GIsUCC ? UObject::GetTransientPackage() : CreatePackage( NULL, NULL );

	// Mark the package as containing a world.  This has to happen here rather than at serialization time,
	// so that e.g. the referenced assets browser will work correctly.
	if ( WorldPackage != UObject::GetTransientPackage() )
	{
		WorldPackage->ThisContainsMap();
	}

	// Create new UWorld, ULevel and UModel.
	GWorld							= new( WorldPackage				, TEXT("TheWorld")			) UWorld(FURL(NULL));
	GWorld->PersistentLevel			= new( GWorld					, TEXT("PersistentLevel")	) ULevel(FURL(NULL));
	GWorld->PersistentLevel->Model	= new( GWorld->PersistentLevel								) UModel( NULL, 1 );

	// Mark objects are transactional for undo/ redo.
	GWorld->SetFlags( RF_Transactional );
	GWorld->PersistentLevel->SetFlags( RF_Transactional );
	GWorld->PersistentLevel->Model->SetFlags( RF_Transactional );
	
	// Need to associate current level so SpawnActor doesn't complain.
	GWorld->CurrentLevel			= GWorld->PersistentLevel;

	// Spawn the level info.
	UClass* WorldInfoClass = StaticLoadClass( AWorldInfo::StaticClass(), AWorldInfo::StaticClass()->GetOuter(), TEXT("WorldInfo"), NULL, LOAD_None, NULL);
	check(WorldInfoClass);

	GWorld->SpawnActor( WorldInfoClass );
	check(GWorld->GetWorldInfo());

	// Initialize.
	GWorld->Init();

	// Update components.
	GWorld->UpdateComponents( FALSE );

	// Add to root set so it doesn't get garbage collected.
	GWorld->AddToRoot();
}

/**
 * Removes the passed in actor from the actor lists. Please note that the code actually doesn't physically remove the
 * index but rather clears it so other indices are still valid and the actors array size doesn't change.
 *
 * @param	Actor					Actor to remove.
 * @param	bShouldModifyLevel		If TRUE, Modify() the level before removing the actor if in the editor.
 * @return							Number of actors array entries cleared.
 */
void UWorld::RemoveActor(AActor* Actor, UBOOL bShouldModifyLevel)
{
	UBOOL	bSuccessfulRemoval	= FALSE;
	ULevel* CheckLevel			= Actor->GetLevel();
	
	if (HasBegunPlay())
	{
		// If we're in game, then only search dynamic actors.
		for (INT ActorIdx = CheckLevel->iFirstDynamicActor; ActorIdx < CheckLevel->Actors.Num(); ActorIdx++)
		{
			if (CheckLevel->Actors(ActorIdx) == Actor)
			{
				CheckLevel->Actors(ActorIdx)	= NULL;
				bSuccessfulRemoval				= TRUE;
				break;
			}
		}
	}
	else
	{
		// Otherwise search the entire list.
		for (INT ActorIdx = 0; ActorIdx < CheckLevel->Actors.Num(); ActorIdx++)
		{
			if (CheckLevel->Actors(ActorIdx) == Actor)
			{
				if ( bShouldModifyLevel && GUndo )
				{
					ModifyLevel( CheckLevel );
				}
				CheckLevel->Actors.ModifyItem(ActorIdx);
				CheckLevel->Actors(ActorIdx)	= NULL;
				bSuccessfulRemoval				= TRUE;
				break;
			}
		}
	}

	check(bSuccessfulRemoval);
}

/**
 * Returns whether the passed in actor is part of any of the loaded levels actors array.
 *
 * @param	Actor	Actor to check whether it is contained by any level
 *	
 * @return	TRUE if actor is contained by any of the loaded levels, FALSE otherwise
 */
UBOOL UWorld::ContainsActor( AActor* Actor )
{
	for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = Levels(LevelIndex);
		if( Level->Actors.ContainsItem( Actor ) )
		{
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Returns whether audio playback is allowed for this scene.
 *
 * @return TRUE if current world is GWorld, FALSE otherwise
 */
UBOOL UWorld::AllowAudioPlayback()
{
	return GWorld == this;
}

void UWorld::NotifyProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds )
{
	GEngine->SetProgress( Str1, Str2, Seconds );
}

void UWorld::ShrinkLevel()
{
	GetModel()->ShrinkModel();
}

/**
 * Clears all level components and world components like e.g. line batcher.
 */
void UWorld::ClearComponents()
{
	GParticleDataManager.Clear();

	for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = Levels(LevelIndex);
		Level->ClearComponents();
	}

	if(LineBatcher)
	{
		LineBatcher->ConditionalDetach();
	}
	if(PersistentLineBatcher)
	{
		PersistentLineBatcher->ConditionalDetach();
	}
}

/**
 * Updates world components like e.g. line batcher and all level components.
 *
 * @param	bCurrentLevelOnly		If TRUE, update only the current level.
 */
void UWorld::UpdateComponents(UBOOL bCurrentLevelOnly)
{
	if(!LineBatcher)
	{
		LineBatcher = ConstructObject<ULineBatchComponent>(ULineBatchComponent::StaticClass());
	}

	LineBatcher->ConditionalDetach();
	LineBatcher->ConditionalAttach(Scene,NULL,FMatrix::Identity);

	if(!PersistentLineBatcher)
	{
		PersistentLineBatcher = ConstructObject<ULineBatchComponent>(ULineBatchComponent::StaticClass());
	}

	PersistentLineBatcher->ConditionalDetach();
	PersistentLineBatcher->ConditionalAttach(Scene,NULL,FMatrix::Identity);

	if ( bCurrentLevelOnly )
	{
		check( CurrentLevel );
		CurrentLevel->UpdateComponents();
	}
	else
	{
		for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
		{
			ULevel* Level = Levels(LevelIndex);
			Level->UpdateComponents();
		}
	}
}

/**
 * Updates all cull distance volumes.
 */
void UWorld::UpdateCullDistanceVolumes()
{
	checkMsg(LineBatcher && LineBatcher->IsAttached(),TEXT("Components need to be attached"));

	// Keep track of time spent.
	DOUBLE Duration = 0;
	{
		STAT(FScopeSecondsCounter DummyScopedCounter( Duration ));

		// Global reattach/ update context to propagate cull distance changes. This cannot be done on the individual
		// component level as we might have changed the component to no longer be affectable by volumes.
		FGlobalPrimitiveSceneAttachmentContext PropagatePrimitiveChanges;

		// Reset cached cull distance for all primitive components affectable by cull distance volumes.
		for( TObjectIterator<UPrimitiveComponent> It; It; ++It )
		{
			UPrimitiveComponent*	PrimitiveComponent	= *It;
	
			// Check whether primitive can be affected by cull distance volumes and reset the cached cull
			// distance to the level designer specified value so we have a clear baseline before iterating
			// over all volumes.
			if(	ACullDistanceVolume::CanBeAffectedByVolumes( PrimitiveComponent ) )
			{
				PrimitiveComponent->CachedCullDistance = PrimitiveComponent->LDCullDistance;
			}
		}

		// Iterate over all cull distance volumes and have them update associated primitives.
		for( FActorIterator It; It; ++It )
		{
			ACullDistanceVolume* CullDistanceVolume = Cast<ACullDistanceVolume>(*It);
			if( CullDistanceVolume )
			{
				CullDistanceVolume->UpdateVolume();
			}
		}
	}
	if( Duration > 1.f )
	{
		debugf(TEXT("Updating cull distance volumes took %5.2f seconds"),Duration);
	}
}

/**
 * Modifies all levels.
 *
 * @param	bAlwaysMarkDirty	if TRUE, marks the package dirty even if we aren't
 *								currently recording an active undo/redo transaction
 */
void UWorld::Modify( UBOOL bAlwaysMarkDirty/*=FALSE*/ )
{
	appErrorf( TEXT("UWorldModify is deprecated!") );
}

/**
 * Transacts the specified level -- the correct way to modify a level
 * as opposed to calling Level->Modify.
 */
void UWorld::ModifyLevel(ULevel* Level)
{
	if( Level )
	{
		Level->Modify( FALSE );
		check( Level->HasAnyFlags(RF_Transactional) );
		//Level->Actors.ModifyAllItems();
		Level->Model->Modify( FALSE );
	}
}

/**
 * Invalidates the cached data used to render the levels' UModel.
 *
 * @param	bCurrentLevelOnly		If TRUE, affect only the current level.
 */
void UWorld::InvalidateModelGeometry(UBOOL bCurrentLevelOnly)
{
	if ( bCurrentLevelOnly )
	{
		check( bCurrentLevelOnly );
		CurrentLevel->InvalidateModelGeometry();
	}
	else
	{
		for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
		{
			ULevel* Level = Levels(LevelIndex);
			Level->InvalidateModelGeometry();
		}
	}
}

/**
 * Discards the cached data used to render the levels' UModel.  Assumes that the
 * faces and vertex positions haven't changed, only the applied materials.
 *
 * @param	bCurrentLevelOnly		If TRUE, affect only the current level.
 */
void UWorld::InvalidateModelSurface(UBOOL bCurrentLevelOnly)
{
	if ( bCurrentLevelOnly )
	{
		check( bCurrentLevelOnly );
		CurrentLevel->InvalidateModelSurface();
	}
	else
	{
		for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
		{
			ULevel* Level = Levels(LevelIndex);
			Level->InvalidateModelSurface();
		}
	}
}

/**
 * Commits changes made to the surfaces of the UModels of all levels.
 */
void UWorld::CommitModelSurfaces()
{
	for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = Levels(LevelIndex);
		Level->CommitModelSurfaces();
	}
}

/**
 * Fixes up any cross level paths. Called from e.g. UpdateLevelStreaming when associating a new level with the world.
 *
 * @param	bIsRemovingLevel	Whether we are adding or removing a level
 * @param	Level				Level to add or remove
 */
void UWorld::FixupCrossLevelPaths( UBOOL bIsRemovingLevel, ULevel* Level )
{
	if (Level->HasPathNodes())
	{
		// tell controllers to clear any cross level path refs they have
		for (AController *Controller = GetFirstController(); Controller != NULL; Controller = Controller->NextController)
		{
			if (!Controller->IsPendingKill() && !Controller->bDeleteMe)
			{
				Controller->ClearCrossLevelPaths(Level);
			}
		}
		// grab a list of all navigation ref's
		TArray<FNavReference*> NavRefs;
		for (INT LevelIdx = 0; LevelIdx < Levels.Num(); LevelIdx++)
		{
			ULevel *ChkLevel = Levels(LevelIdx);
			for (INT Idx = 0; Idx < ChkLevel->CrossLevelActors.Num(); Idx++)
			{
				AActor *Actor = ChkLevel->CrossLevelActors(Idx);
				if (Actor != NULL)
				{
					Actor->GetNavReferences(NavRefs,bIsRemovingLevel);
				}
			}
		}
		// if removing the level then just null all ref's to navs in level being unloaded
		if (bIsRemovingLevel)
		{
			for (INT Idx = 0; Idx < NavRefs.Num(); Idx++)
			{
				ANavigationPoint *Nav = NavRefs(Idx)->Nav;
				if (Nav != NULL && Nav->IsInLevel(Level))
				{
					NavRefs(Idx)->Nav = NULL;
				}
			}
			// remove the level's nav list from the world
			RemoveLevelNavList( Level, TRUE );
			// for each nav in the level nav list
			for (ANavigationPoint *Nav = Level->NavListStart; Nav != NULL; Nav = Nav->nextNavigationPoint)
			{
				// remove the nav from the octree
				Nav->RemoveFromNavigationOctree();
			}
		}
		else
		{
			// start fixing up each ref,
			for (ANavigationPoint *Nav = GetFirstNavigationPoint(); Nav != NULL && NavRefs.Num() > 0; Nav = Nav->nextNavigationPoint)
			{
				if (Nav->NavGuid.IsValid())
				{
					for (INT Idx = 0; Idx < NavRefs.Num(); Idx++)
					{
						// if matching guid,
						if (NavRefs(Idx)->Guid == Nav->NavGuid)
						{
							// assign the nav
							NavRefs(Idx)->Nav = Nav;
							// remove this entry
							NavRefs.Remove(Idx--,1);
						}
					}
				}
			}
		}
#if !FINAL_RELEASE
		if (GIsGame && !bIsRemovingLevel)
		{
			// check to see if show paths/cover is enabled
			if (GEngine->GameViewport != NULL && (GEngine->GameViewport->ShowFlags & SHOW_Paths))
			{
				GEngine->GameViewport->ShowFlags &= ~SHOW_Paths;
				GEngine->GameViewport->Exec(TEXT("SHOW PATHS"),*GLog);
			}
		}
#endif
	}
}

/** verifies that the navlist pointers have not been corrupted.
 *	helps track down errors w/ dynamic pathnodes being added/removed from the navigation list
 */
void UWorld::VerifyNavList( const TCHAR* DebugTxt, ANavigationPoint* IngoreNav )
{
	INT ListErrorCount = 0;
	// for all navigation points
	for (FActorIterator It; It; ++It)
	{
		ANavigationPoint *Nav = Cast<ANavigationPoint>(*It);
		if( Nav == NULL || Nav == IngoreNav )
		{
			continue;
		}
		if (Nav->nextOrdered != NULL ||
			Nav->prevOrdered != NULL ||
			Nav->previousPath != NULL)
		{
			debugf(NAME_Warning,TEXT("%s has transient properties that haven't been cleared!"),*Nav->GetPathName());
		}
		UBOOL bInList = FALSE;
		// validate it is in the world list
		for (ANavigationPoint *TestNav = GWorld->GetFirstNavigationPoint(); TestNav != NULL; TestNav = TestNav->nextNavigationPoint)
		{
			if (Nav == TestNav)
			{
				bInList = TRUE;
				break;
			}
		}
		if (!bInList)
		{
			debugf(NAME_Warning,TEXT("%s is not in the nav list!"),*Nav->GetPathName());
			ListErrorCount++;
		}
		else
		if( Nav->IsPendingKill() )
		{
			debugf( NAME_Warning,TEXT("%s is in the nav list but about to be GC'd!!"), *Nav->GetPathName() );
			ListErrorCount++;
		}

		// validate cover list as well
		ACoverLink *TestLink = Cast<ACoverLink>(Nav);
		if (TestLink != NULL)
		{
			bInList = FALSE;
			for (ACoverLink *Link = GWorld->GetWorldInfo()->CoverList; Link != NULL; Link = Link->NextCoverLink)
			{
				if (Link == TestLink)
				{
					bInList = TRUE;
					break;
				}
			}
			if (!bInList)
			{
				debugf(NAME_Warning,TEXT("%s is not in the cover list!"),*TestLink->GetPathName());
				ListErrorCount++;
			}
			else
			if( TestLink->IsPendingKill() )
			{
				debugf( NAME_Warning,TEXT("%s is in the nav list but about to be GC'd!!"), *TestLink->GetPathName());
				ListErrorCount++;
			}
		}
	}
	if (ListErrorCount != 0)
	{
		debugf( DebugTxt );
		debugf(NAME_Warning,TEXT("%d errors found, currently loaded levels:"),ListErrorCount);
		for (INT Idx = 0; Idx < GWorld->Levels.Num(); Idx++)
		{
			debugf(NAME_Warning,TEXT("- %s"),*GWorld->Levels(Idx)->GetPathName());
		}
	}
}

/**
 * Static helper function for AddToWorld to determine whether we've already spent all the allotted time.
 *
 * @param	CurrentTask		Description of last task performed
 * @param	StartTime		StartTime, used to calculate time passed
 * @param	Level			Level work has been performed on
 *
 * @return TRUE if time limit has been exceeded, FALSE otherwise
 */
static UBOOL IsTimeLimitExceeded( const TCHAR* CurrentTask, DOUBLE StartTime, ULevel* Level )
{
	UBOOL bIsTimeLimitExceed = FALSE;
	// We don't spread work across several frames in the Editor to avoid potential side effects.
	if( GIsEditor == FALSE )
	{
		DOUBLE CurrentTime	= appSeconds();
		// Delta time in ms.
		DOUBLE DeltaTime	= (CurrentTime - StartTime) * 1000;
		if( DeltaTime > 5 )
		{
			// Log if a single event took way too much time.
			if( DeltaTime > 20 )
			{
				debugfSuppressed( NAME_DevStreaming, TEXT("UWorld::AddToWorld: %s for %s took (less than) %5.2f ms"), CurrentTask, *Level->GetOutermost()->GetName(), DeltaTime );
			}
			return TRUE;
		}
	}
	return bIsTimeLimitExceed;
}

extern UBOOL GPrecacheNextFrame;

/**
 * Associates the passed in level with the world. The work to make the level visible is spread across several frames and this
 * function has to be called till it returns TRUE for the level to be visible/ associated with the world and no longer be in
 * a limbo state.
 *
 * @param StreamingLevel	Level streaming object whose level we should add
 * @param RelativeOffset	Relative offset to move actors
 */
void UWorld::AddToWorld( ULevelStreaming* StreamingLevel, const FVector& RelativeOffset )
{
	check(StreamingLevel);
	
	ULevel* Level = StreamingLevel->LoadedLevel;
	check(Level);
	check(!Level->IsPendingKill());
	check(!Level->HasAnyFlags(RF_Unreachable));
	check(!StreamingLevel->bIsVisible);

	// Set flags to indicate that we are associating a level with the world to e.g. perform slower/ better octree insertion 
	// and such, as opposed to the fast path taken for run-time/ gameplay objects.
	GIsAssociatingLevel					= TRUE;
	DOUBLE	StartTime					= appSeconds();
	UBOOL	bExecuteNextStep			= (CurrentLevelPendingVisibility == Level) || CurrentLevelPendingVisibility == NULL;
	UBOOL	bPerformedLastStep			= FALSE;
	// We're not done yet, aka we have a pending request in flight.
	Level->bHasVisibilityRequestPending	= TRUE;

	if( bExecuteNextStep && !Level->bAlreadyMovedActors )
	{
		// Mark level as being the one in process of being made visible.
		CurrentLevelPendingVisibility = Level;

		// We're adding the level to the world so we implicitly have a request pending.
		Level->bHasVisibilityRequestPending = TRUE;

		// Add to the UWorld's array of levels, which causes it to be rendered et al.
		Levels.AddUniqueItem( Level );

		// Don't bother moving if there isn't anything to do.
		UBOOL		bMoveActors	= RelativeOffset.Size() > KINDA_SMALL_NUMBER;
		AWorldInfo*	WorldInfo	= GetWorldInfo();
		// Iterate over all actors int he level and move them if necessary, associate them with the right world info, set the
		// proper zone and clear their Kismet events.
		for( INT ActorIndex=0; ActorIndex<Level->Actors.Num(); ActorIndex++ )
		{
			AActor* Actor = Level->Actors(ActorIndex);
			if( Actor )
			{
				// Shift actors by specified offset.
				if( bMoveActors )
				{
					Actor->Location += RelativeOffset;
					Actor->PostEditMove( TRUE );
				}

				// Associate with persistent world info actor and update zoning.
				Actor->WorldInfo = WorldInfo;
				Actor->SetZone( 0, 1 );

				// Clear any events for Kismet as well
				if( GIsGame )
				{
					Actor->GeneratedEvents.Empty();
				}
			}
		}
	
		// We've moved actors so mark the level package as being dirty.
		if( bMoveActors )
		{
			Level->MarkPackageDirty();
		}

		Level->bAlreadyMovedActors = TRUE;
		bExecuteNextStep = !IsTimeLimitExceeded( TEXT("moving actors"), StartTime, Level );
	}

	// Updates the level components (Actor components and UModelComponents).
	if( bExecuteNextStep && !Level->bAlreadyUpdatedComponents )
	{
		// Make sure code thinks components are not currently attached.
		Level->bAreComponentsCurrentlyAttached = FALSE;

		// Incrementally update components.
		do
		{
			Level->IncrementalUpdateComponents( (GIsEditor || GIsUCC) ? 0 : 50 );
		}
		while( !IsTimeLimitExceeded( TEXT("updating components"), StartTime, Level ) && !Level->bAreComponentsCurrentlyAttached );

		// We are done once all components are attached.
		Level->bAlreadyUpdatedComponents	= Level->bAreComponentsCurrentlyAttached;
		bExecuteNextStep					= Level->bAreComponentsCurrentlyAttached;
	}

	if( GIsGame )
	{
		// Initialize physics for level BSP mesh.
		if( bExecuteNextStep && !Level->bAlreadyCreateBSPPhysMesh )
		{
			Level->InitLevelBSPPhysMesh();
			Level->bAlreadyCreateBSPPhysMesh = TRUE;
			bExecuteNextStep = !IsTimeLimitExceeded( TEXT("initializing level bsp physics mesh"), StartTime, Level );
		}

		// Initialize physics for each actor.
		if( bExecuteNextStep && !Level->bAlreadyInitializedAllActorRBPhys )
		{
			// Incrementally initialize physics for actors.
			do
			{
				// This function will set bAlreadyInitializedAllActorRBPhys when it does the final Actor.
				Level->IncrementalInitActorsRBPhys( (GIsEditor || GIsUCC) ? 0 : 50 );
			}
			while( !IsTimeLimitExceeded( TEXT("initializing actor physics"), StartTime, Level ) && !Level->bAlreadyInitializedAllActorRBPhys );

			// Go on to next step if we are done here.
			bExecuteNextStep = Level->bAlreadyInitializedAllActorRBPhys;
		}

		// send a callback that a level was added to the world
		GCallbackEvent->Send(CALLBACK_LevelAddedToWorld, Level);

		// Initialize all actors and start execution.
		if( bExecuteNextStep && !Level->bAlreadyInitializedActors )
		{
			Level->InitializeActors();
			Level->bAlreadyInitializedActors = TRUE;
			bExecuteNextStep = !IsTimeLimitExceeded( TEXT("initializing actors"), StartTime, Level );
		}

		// Route various begin play functions and set volumes.
		if( bExecuteNextStep && !Level->bAlreadyRoutedActorBeginPlay )
		{
			Level->RouteBeginPlay();
			Level->bAlreadyRoutedActorBeginPlay = TRUE;
			bExecuteNextStep = !IsTimeLimitExceeded( TEXT("routing BeginPlay on actors"), StartTime, Level );
		}

		// Fixup any cross level paths.
		if( bExecuteNextStep && !Level->bAlreadyFixedUpCrossLevelPaths )
		{
			FixupCrossLevelPaths( FALSE, Level );
			Level->bAlreadyFixedUpCrossLevelPaths = TRUE;
			bExecuteNextStep = !IsTimeLimitExceeded( TEXT("fixing up cross-level paths"), StartTime, Level );
		}

		// Handle kismet scripts, if there are any.
		if( bExecuteNextStep && !Level->bAlreadyRoutedSequenceBeginPlay )
		{
			// find the parent persistent level to add this as the parent 
			USequence* RootSequence = PersistentLevel->GetGameSequence();
			if( RootSequence )
			{
				// at runtime, we could have multiple FakePersistentLevels and one of them could be the
				// parent sequence for this loaded object.
				// Instead of this level hierarchy:
				//     HappyLevel_P
				//       \-> HappySubLevel1
				//       \-> HappySubLevel2
				//     SadLevel_P
				//       \-> SadSubLevel1
				//       \-> SadSubLevel2
				// We could have this:
				//     MyGame_P
				//       \-> HappyLevel_P
				//       \-> HappySubLevel1
				//       \-> HappySubLevel2
				//       \-> SadLevel_P
				//       \-> SadSubLevel1
				//       \-> SadSubLevel2
				//
				// HappySubLevel1 needs HappyLevel_P's GameSequence to be the RootSequence for it's sequences, etc
				if (GIsGame)
				{
					// loop over all the loaded World objects, looking to find the one that has this level 
					// in it's streaming level list
					AWorldInfo* WorldInfo = GetWorldInfo();
					// get package name for the loaded level
					FName LevelPackageName = Level->GetOutermost()->GetFName();
					UBOOL bIsDone = FALSE;
					for (INT LevelIndex = 0; LevelIndex < WorldInfo->StreamingLevels.Num() && !bIsDone; LevelIndex++)
					{
						ULevelStreaming* LevelStreaming = WorldInfo->StreamingLevels(LevelIndex);
						// if the level streaming object has a loaded world that isn't this one, check it's
						// world's streaming levels
						if (LevelStreaming->LoadedLevel && LevelStreaming->LoadedLevel != Level)
						{
                            AWorldInfo* SubLevelWorldInfo = CastChecked<AWorldInfo>(LevelStreaming->LoadedLevel->Actors(0));
							if (SubLevelWorldInfo)
							{
								for (INT SubLevelIndex=0; SubLevelIndex < SubLevelWorldInfo->StreamingLevels.Num() && !bIsDone; SubLevelIndex++ )
								{
									ULevelStreaming* SubLevelStreaming = SubLevelWorldInfo->StreamingLevels(SubLevelIndex);
									// look to see if the sublevelWorld was the "owner" of this level
									if (SubLevelStreaming->PackageName == LevelPackageName)
									{
										RootSequence = LevelStreaming->LoadedLevel->GetGameSequence();
										// mark as as done to get out of nested loop
										bIsDone = TRUE;
									}
								}
							}
						}
					}
				}

				for( INT SequenceIndex=0; SequenceIndex<Level->GameSequences.Num(); SequenceIndex++ )
				{
					USequence* Sequence = Level->GameSequences(SequenceIndex);
					if( Sequence )
					{
						check(!RootSequence->SequenceObjects.ContainsItem(Sequence)); 
						check(!RootSequence->NestedSequences.ContainsItem(Sequence));
						// Add the game sequences to the persistent level's root sequence.
						RootSequence->SequenceObjects.AddItem(Sequence);
						RootSequence->NestedSequences.AddItem(Sequence);
						// Set the parent to the current root sequence.
						Sequence->ParentSequence = RootSequence;
						// And initialize the sequence, unless told not to
						if (!bDisallowRoutingSequenceBeginPlay)
						{
							Sequence->BeginPlay();
						}
					}
				}
			}
			Level->bAlreadyRoutedSequenceBeginPlay = TRUE;
			bExecuteNextStep = !IsTimeLimitExceeded( TEXT("routing BeginPlay on sequences"), StartTime, Level );
		}

		// Sort the actor list. @todo optimization: we could actually just serialize the indices
		if( bExecuteNextStep && !Level->bAlreadySortedActorList )
		{
			Level->SortActorList();
			Level->bAlreadySortedActorList = TRUE;
			bExecuteNextStep = !IsTimeLimitExceeded( TEXT("sorting actor list"), StartTime, Level );
			bPerformedLastStep = TRUE;
		}
	}
	// !GIsGame
	else
	{
		bPerformedLastStep = TRUE;
	}

	GIsAssociatingLevel = FALSE;

	// We're done.
	if( bPerformedLastStep )
	{
		// Reset temporary level properties for next time.
		Level->bHasVisibilityRequestPending				= FALSE;
		Level->bAlreadyMovedActors						= FALSE;
		Level->bAlreadyUpdatedComponents				= FALSE;
		Level->bAlreadyCreateBSPPhysMesh				= FALSE;
		Level->bAlreadyInitializedAllActorRBPhys		= FALSE;
		Level->bAlreadyInitializedActors				= FALSE;
		Level->bAlreadyRoutedActorBeginPlay				= FALSE;
		Level->bAlreadyFixedUpCrossLevelPaths			= FALSE;
		Level->bAlreadyRoutedSequenceBeginPlay			= FALSE;
		Level->bAlreadySortedActorList					= FALSE;

		// Finished making level visible - allow other levels to be added to the world.
		CurrentLevelPendingVisibility					= NULL;

		// notify server that the client has finished making this level visible
		for (FPlayerIterator It(GEngine); It; ++It)
		{
			if (It->Actor != NULL)
			{
				It->Actor->eventServerUpdateLevelVisibility(Level->GetOutermost()->GetFName(), TRUE);
			}
		}

		// GEMINI_TODO: A nicer precaching scheme.
		GPrecacheNextFrame = TRUE;
	}

	StreamingLevel->bIsVisible = !Level->bHasVisibilityRequestPending;
}

/** 
 * Dissociates the passed in level from the world. The removal is blocking.
 *
 * @param LevelStreaming	Level streaming object whose level we should remove
 */
void UWorld::RemoveFromWorld( ULevelStreaming* StreamingLevel )
{
	check(StreamingLevel);

	ULevel* Level = StreamingLevel->LoadedLevel;
	check(Level);
	check(!Level->IsPendingKill());
	check(!Level->HasAnyFlags(RF_Unreachable));
	check(StreamingLevel->bIsVisible);

	// let the universe know we removed a level
	GCallbackEvent->Send(CALLBACK_LevelRemovedFromWorld, Level);

	if( CurrentLevelPendingVisibility == NULL )
	{
		// Keep track of timing.
		DOUBLE StartTime = appSeconds();	

		if( GIsGame )
		{
			// Clean up cross level paths.
			FixupCrossLevelPaths( TRUE, Level );

			// Clear out kismet refs from the root sequence if there is one.
			for (INT Idx = 0; Idx < Level->GameSequences.Num(); Idx++) 
			{ 
				USequence *Seq = Level->GameSequences(Idx); 
				if (Seq != NULL) 
				{ 
					Seq->CleanUp(); 
					// This can happen if the persistent level never had the Kismet window opened.
					if( Seq->ParentSequence )
					{
						Seq->ParentSequence->SequenceObjects.RemoveItem(Seq); 
						Seq->ParentSequence->NestedSequences.RemoveItem(Seq); 
					}
				} 
			}

			// Shut down physics for the level object (ie BSP collision).
			Level->TermLevelRBPhys(NULL);

			// Shut down physics for all the Actors in the level.
			for(INT ActorIdx=0; ActorIdx<Level->Actors.Num(); ActorIdx++)
			{
				AActor* Actor = Level->Actors(ActorIdx);
				if(Actor)
				{
					Actor->TermRBPhys(NULL);
					Actor->bScriptInitialized = FALSE;
				}
			}

			// Remove any pawns from the pawn list that are about to be streamed out
			for (APawn *Pawn = GetFirstPawn(); Pawn != NULL; Pawn = Pawn->NextPawn)
			{
				if (Pawn->IsInLevel(Level))
				{
					RemovePawn(Pawn);
				}
			}
		}

		// Remove from the world's level array and destroy actor components.
		Levels.RemoveItem(Level );
		Level->ClearComponents();

		// notify server that the client has removed this level
		for (FPlayerIterator It(GEngine); It; ++It)
		{
			if (It->Actor != NULL)
			{
				It->Actor->eventServerUpdateLevelVisibility(Level->GetOutermost()->GetFName(), FALSE);
			}
		}

		StreamingLevel->bIsVisible = FALSE;
		debugfSuppressed( NAME_DevStreaming, TEXT("UWorld::RemoveFromWorld for %s took %5.2f ms"), *Level->GetOutermost()->GetName(), (appSeconds()-StartTime) * 1000.0 );
	}
}


/**
 * Helper structure encapsulating functionality used to defer marking actors and their components as pending
 * kill till right before garbage collection by registering a callback.
 */
struct FLevelStreamingGCHelper
{
	/**
	 * Request level associated with level streaming object to be unloaded.
	 *
	 * @param LevelStreaming	Level streaming object whose level should be unloaded
	 */
	static void RequestUnload( ULevelStreaming* LevelStreaming )
	{
		check( LevelStreaming->LoadedLevel );
		check( LevelStreamingObjects.FindItemIndex( LevelStreaming ) == INDEX_NONE );
		LevelStreamingObjects.AddItem( LevelStreaming );
		LevelStreaming->bHasUnloadRequestPending = TRUE;
	}

	/**
	 * Cancel any pending unload requests for passed in LevelStreaming object.
	 */
	static void CancelUnloadRequest( ULevelStreaming* LevelStreaming )
	{
		verify( LevelStreamingObjects.RemoveItem( LevelStreaming ) <= 1 );
		LevelStreaming->bHasUnloadRequestPending = FALSE;
	}

	/** 
	 * Prepares levels that are marked for unload for the GC call by marking their actors and components as
	 * pending kill.
	 */
	static void PrepareStreamedOutLevelsForGC()
	{
		// This can never ever be called during tick; same goes for GC in general.
		check( !GWorld || !GWorld->InTick );

		// The Editor always fully loads packages and we can't delete reference in edited levels so we can't use GIsGame here.
		if( !GIsEditor )
		{
			// Iterate over all streaming level objects that want their levels unloaded.
			for( INT LevelStreamingIndex=0; LevelStreamingIndex<LevelStreamingObjects.Num(); LevelStreamingIndex++ )
			{
				ULevelStreaming*	LevelStreaming	= LevelStreamingObjects(LevelStreamingIndex);
				ULevel*				Level			= LevelStreaming->LoadedLevel;
				check(Level);

				debugfSuppressed( NAME_DevStreaming, TEXT("PrepareStreamedOutLevelsForGC called on '%s'"), *Level->GetOutermost()->GetName() );

				// Make sure that this package has been unloaded after GC pass.
				LevelPackageNames.AddItem( Level->GetOutermost()->GetFName() );

				// Mark level as pending kill so references to it get deleted.
				Level->MarkPendingKill();

				// Mark all model components as pending kill so GC deletes references to them.
				for( INT ModelComponentIndex=0; ModelComponentIndex<Level->ModelComponents.Num(); ModelComponentIndex++ )
				{
					UModelComponent* ModelComponent = Level->ModelComponents(ModelComponentIndex);
					if( ModelComponent )
					{
						ModelComponent->MarkPendingKill();
					}
				}

				// Mark all actors and their components as pending kill so GC will delete references to them.
				for( INT ActorIndex=0; ActorIndex<Level->Actors.Num(); ActorIndex++ )
				{
					AActor* Actor = Level->Actors(ActorIndex);
					if( Actor )
					{
						if (GWorld)
						{
							// notify both net drivers about the actor destruction
							for (INT NetDriverIndex = 0; NetDriverIndex < 2; NetDriverIndex++)
							{
								UNetDriver* NetDriver = NetDriverIndex ? GWorld->DemoRecDriver : GWorld->NetDriver;

						// close any channels for this actor
								if (NetDriver != NULL)
								{
							// server
									NetDriver->NotifyActorDestroyed(Actor);
							// client
									if (NetDriver->ServerConnection != NULL)
									{
										// we can't kill the channel until the server says so, so just clear the actor ref and break the channel
										UActorChannel* Channel = NetDriver->ServerConnection->ActorChannels.FindRef(Actor);
										if (Channel != NULL)
										{
											NetDriver->ServerConnection->ActorChannels.Remove(Actor);
											Channel->Actor = NULL;
											Channel->Broken = TRUE;
										}
									}
								}
							}
						}
						Actor->MarkComponentsAsPendingKill();
						Actor->MarkPendingKill();
					}
				}

				for (INT SeqIdx = 0; SeqIdx < Level->GameSequences.Num(); SeqIdx++)
				{
					USequence* Sequence = Level->GameSequences(SeqIdx);
					if( Sequence != NULL )
					{
						Sequence->MarkPendingKill();
					}
				}

				// Remove reference so GC can delete it.
				LevelStreaming->LoadedLevel					= NULL;
				// We're no longer having an unload request pending.
				LevelStreaming->bHasUnloadRequestPending	= FALSE;
			}
		}
		LevelStreamingObjects.Empty();
	}

	/**
	 * Verify that the level packages are no longer around.
	 */
	static void VerifyLevelsGotRemovedByGC()
	{
		if( !GIsEditor )
		{
#if DO_GUARD_SLOW
			// Iterate over all objects and find out whether they reside in a GC'ed level package.
			for( FObjectIterator It; It; ++It )
			{
				UObject* Object = *It;
				// Check whether object's outermost is in the list.
				if( LevelPackageNames.FindItemIndex( Object->GetOutermost()->GetFName() ) != INDEX_NONE 
				// But disregard package object itself.
				&&	!Object->IsA(UPackage::StaticClass()) )
				{	
					debugf(TEXT("%s didn't get garbage collected! Trying to find culprit, though this might crash."), *Object->GetFullName());
					UObject::StaticExec(*FString::Printf(TEXT("OBJ REFS CLASS=%s NAME=%s"), *Object->GetClass()->GetName(), *Object->GetName()));
					TMap<UObject*,UProperty*>	Route		= FArchiveTraceRoute::FindShortestRootPath( Object, TRUE, GARBAGE_COLLECTION_KEEPFLAGS );
					FString						ErrorString	= FArchiveTraceRoute::PrintRootPath( Route, Object );				
					// We cannot safely recover from this :(
					appErrorf( TEXT("%s didn't get garbage collected!") LINE_TERMINATOR TEXT("%s"), *Object->GetFullName(), *ErrorString );
				}
			}
#endif
			LevelPackageNames.Empty();
		}
	}

private:
	/** Static array of level streaming objects that want their levels to be unloaded */
	static TArray<ULevelStreaming*> LevelStreamingObjects;
	/** Static array of level packages that have been marked by PrepareStreamedOutLevelsForGC */
	static TArray<FName> LevelPackageNames;
};
/** Static array of level streaming objects that want their levels to be unloaded */
TArray<ULevelStreaming*> FLevelStreamingGCHelper::LevelStreamingObjects;
/** Static array of level packages that have been marked by PrepareStreamedOutLevelsForGC */
TArray<FName> FLevelStreamingGCHelper::LevelPackageNames;

IMPLEMENT_PRE_GARBAGE_COLLECTION_CALLBACK( DUMMY_PrepareStreamedOutLevelsForGC, FLevelStreamingGCHelper::PrepareStreamedOutLevelsForGC, GCCB_PRE_PrepareLevelsForGC );
IMPLEMENT_POST_GARBAGE_COLLECTION_CALLBACK( DUMMY_VerifyLevelsGotRemovedByGC, FLevelStreamingGCHelper::VerifyLevelsGotRemovedByGC, GCCB_POST_VerifyLevelsGotRemovedByGC );

/**
 * Callback function used in UpdateLevelStreaming to pass to LoadPackageAsync. Sets LoadedLevel to map found in LinkerRoot.
 * @param	LevelPackage	level package that finished async loading
 * @param	Unused
 */
static void AsyncLevelLoadCompletionCallback( UObject* LevelPackage, void* /*Unused*/ )
{
	if( !GWorld )
	{
		// This happens if we change levels while there are outstanding load requests.
	}
	else if( LevelPackage )
	{
		// Try to find a UWorld object in the level package.
		UWorld* World = (UWorld*) UObject::StaticFindObjectFast( UWorld::StaticClass(), LevelPackage, NAME_TheWorld );
		ULevel* Level = World ? World->PersistentLevel : NULL;	
		if( Level )
		{
			// Iterate over level streaming objects to find the ones that have a package name matching the name of the LinkerRoot.
			AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
			for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
			{
				ULevelStreaming* StreamingLevel	= WorldInfo->StreamingLevels(LevelIndex);
				if( StreamingLevel && StreamingLevel->PackageName == LevelPackage->GetFName() )
				{
					// Propagate loaded level so garbage collection doesn't delete it in case async loading was flushed by GC.
					StreamingLevel->LoadedLevel = Level;
					
					// Cancel any pending unload requests.
					FLevelStreamingGCHelper::CancelUnloadRequest( StreamingLevel );
				}
			}
		}
		else
		{
			debugf( NAME_Warning, TEXT("Couldn't find ULevel object in package '%s'"), *LevelPackage->GetName() );
		}
	}
	else
	{
		debugf( NAME_Warning, TEXT("NULL LevelPackage as argument to AsyncLevelCompletionCallback") );
	}
}
 

/**
 * Updates the world based on the current view location of the player and sets level LODs accordingly.
 *
 * @param ViewFamily	Optional collection of views to take into account
 */
void UWorld::UpdateLevelStreaming( FSceneViewFamily* ViewFamily )
{
#if !FINAL_RELEASE
	// do nothing if level streaming is frozen
	if (bIsLevelStreamingFrozen)
	{
		return;
	}
#endif

	AWorldInfo*	WorldInfo						= GetWorldInfo();
	UBOOL		bLevelsHaveBeenUnloaded			= FALSE;
	// whether one or more levels has a pending load request
	UBOOL		bLevelsHaveLoadRequestPending	= FALSE;

	// Iterate over level collection to find out whether we need to load/ unload any levels.
	for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
	{
		ULevelStreaming* StreamingLevel	= WorldInfo->StreamingLevels(LevelIndex);
		if( !StreamingLevel )
		{
			// This can happen when manually adding a new level to the array via the property editing code.
			continue;
		}
		
		//@todo seamless: add ability to abort load request.

		// Work performed to make a level visible is spread across several frames and we can't unload/ hide a level that is currently pending
		// to be made visible, so we fulfill those requests first.
		UBOOL bHasVisibilityRequestPending	= StreamingLevel->LoadedLevel && StreamingLevel->LoadedLevel->bHasVisibilityRequestPending;
		
		// Figure out whether level should be loaded, visible and block on load if it should be loaded but currently isn't.
		UBOOL bShouldBeLoaded				= bHasVisibilityRequestPending || (!GEngine->bUseBackgroundLevelStreaming && !StreamingLevel->bIsRequestingUnloadAndRemoval);
		UBOOL bShouldBeVisible				= bHasVisibilityRequestPending;
		UBOOL bShouldBlockOnLoad			= StreamingLevel->bShouldBlockOnLoad;

		// Don't update if the code requested this level object to be unloaded and removed.
		if( !StreamingLevel->bIsRequestingUnloadAndRemoval )
		{
			// Take all views into account if any were passed in.
			if( ViewFamily )
			{
				// Iterate over all views in collection.
				for( INT ViewIndex=0; ViewIndex<ViewFamily->Views.Num(); ViewIndex++ )
				{
					const FSceneView* View		= ViewFamily->Views(ViewIndex);
					const FVector& ViewLocation	= View->ViewOrigin;
					bShouldBeLoaded	= bShouldBeLoaded  || ( StreamingLevel && (!GIsGame || StreamingLevel->ShouldBeLoaded(ViewLocation)) );
					bShouldBeVisible= bShouldBeVisible || ( bShouldBeLoaded && StreamingLevel && StreamingLevel->ShouldBeVisible(ViewLocation) );
				}
			}
			// Or default to view location of 0,0,0
			else
			{
				FVector ViewLocation(0,0,0);
				bShouldBeLoaded		= bShouldBeLoaded  || ( StreamingLevel && (!GIsGame || StreamingLevel->ShouldBeLoaded(ViewLocation)) );
				bShouldBeVisible	= bShouldBeVisible || ( bShouldBeLoaded && StreamingLevel && StreamingLevel->ShouldBeVisible(ViewLocation) );
			}
		}

		// The below is not an invariant as streaming in might affect the result so it cannot be pulled out of the loop.
		UBOOL bAllowLevelLoadRequests =	AllowLevelLoadRequests() || bShouldBlockOnLoad;

		// See whether level is already loaded
		if(	bShouldBeLoaded 
		&&	!StreamingLevel->LoadedLevel )
		{
			if( !StreamingLevel->bHasLoadRequestPending )
			{
				// Try to find the [to be] loaded package.
				UPackage* LevelPackage = (UPackage*) UObject::StaticFindObjectFast( UPackage::StaticClass(), NULL, StreamingLevel->PackageName );
				
				// Package is already or still loaded.
				UBOOL bNeedToLoad = TRUE;
				if( LevelPackage )
				{
					// Find world object and use its PersistentLevel pointer.
					UWorld* World = (UWorld*) UObject::StaticFindObjectFast( UWorld::StaticClass(), LevelPackage, NAME_TheWorld );
					if (World != NULL)
					{
						checkMsg(World->PersistentLevel,TEXT("Most likely caused by reference to world of unloaded level and GC setting reference to NULL while keeping world object"));
						StreamingLevel->LoadedLevel	= World->PersistentLevel;
						bNeedToLoad = FALSE;
					}
				}
				// Async load package if world object couldn't be found and we are allowed to request a load.
				if( bNeedToLoad && bAllowLevelLoadRequests  )
				{
					if( GUseSeekFreeLoading )
					{
						// Only load localized package if it exists as async package loading doesn't handle errors gracefully.
						FString LocalizedPackageName = StreamingLevel->PackageName.ToString() + LOCALIZED_SEEKFREE_SUFFIX;
						FString LocalizedFileName;
						if( GPackageFileCache->FindPackageFile( *LocalizedPackageName, NULL, LocalizedFileName ) )
						{
							// Load localized part of level first in case it exists. We don't need to worry about GC or completion 
							// callback as we always kick off another async IO for the level below.
							UObject::LoadPackageAsync( *LocalizedPackageName, NULL, NULL );
						}
					}
						
					// Kick off async load request.
					UObject::LoadPackageAsync( *StreamingLevel->PackageName.ToString(), AsyncLevelLoadCompletionCallback, NULL );
					
					// streamingServer: server loads everything?
					// Editor immediately blocks on load and we also block if background level streaming is disabled.
					if( GIsEditor || !GEngine->bUseBackgroundLevelStreaming )
					{
						// Finish all async loading.
						UObject::FlushAsyncLoading();

						// Completion callback should have associated level by now.
						if( StreamingLevel->LoadedLevel == NULL )
						{
							debugfSuppressed( NAME_DevStreaming, TEXT("Failed to load %s"), *StreamingLevel->PackageName.ToString() );
						}
					}

					// Load request is still pending if level isn't set.
					StreamingLevel->bHasLoadRequestPending = (StreamingLevel->LoadedLevel == NULL) ? TRUE : FALSE;
				}
			}

			// We need to tell the game to bring up a loading screen and flush async loading during this or the next tick.
			if( StreamingLevel->bHasLoadRequestPending && bShouldBlockOnLoad && !GIsEditor )
			{
				GWorld->GetWorldInfo()->bRequestedBlockOnAsyncLoading = TRUE;
				debugfSuppressed( NAME_DevStreaming, TEXT("Requested blocking on load for level %s"), *StreamingLevel->PackageName.ToString() );
			}
		}

		// See whether we have a loaded level.
		if( StreamingLevel->LoadedLevel )
		{
			// Cache pointer for convenience. This cannot happen before this point as e.g. flushing async loaders
			// or such will modify StreamingLevel->LoadedLevel.
			ULevel* Level = StreamingLevel->LoadedLevel;

			// We have a level so there is no load request pending.
			StreamingLevel->bHasLoadRequestPending = FALSE;

			// Associate level if it should be visible and hasn't been in the past.
			if( bShouldBeVisible && !StreamingLevel->bIsVisible )
			{
				// Calculate relative offset and keep track of it so we can move maps around in the Editor.
				FVector RelativeOffset		= StreamingLevel->Offset - StreamingLevel->OldOffset;
				StreamingLevel->OldOffset	= StreamingLevel->Offset;

				// Make level visible.  Updates bIsVisible if level is finished being made visible.
				AddToWorld( StreamingLevel, RelativeOffset );
			}
			// Level was visible before but no longer should be.
			else if( !bShouldBeVisible && StreamingLevel->bIsVisible )
			{
				// Hide this level/ remove from world. Updates bIsVisible.
				RemoveFromWorld( StreamingLevel );
			}

			// Make sure level is referenced if it should be loaded or is currently visible.
			if( bShouldBeLoaded || StreamingLevel->bIsVisible )
			{
				// Cancel any pending requests to unload this level.
				FLevelStreamingGCHelper::CancelUnloadRequest( StreamingLevel );
			}
			else if( !StreamingLevel->bHasUnloadRequestPending )
			{
				// Request unloading this level.
				FLevelStreamingGCHelper::RequestUnload( StreamingLevel );
				bLevelsHaveBeenUnloaded	= TRUE;
			}
		}
		// Editor is always blocking on load and if we don't have a level now it is because it couldn't be found. Ditto if background
		// level streaming is disabled.
		else if( GIsEditor || !GEngine->bUseBackgroundLevelStreaming )
		{
			StreamingLevel->bHasLoadRequestPending = FALSE;
		}

		// If requested, remove this level from iterated over array once it is unloaded.
		if( StreamingLevel->bIsRequestingUnloadAndRemoval 
		&&	!bShouldBeLoaded 
		&&	!StreamingLevel->bIsVisible )
		{
			// The -- is required as we're forward iterating over the StreamingLevels array.
			WorldInfo->StreamingLevels.Remove( LevelIndex-- );
		}

		bLevelsHaveLoadRequestPending = (bLevelsHaveLoadRequestPending || StreamingLevel->bHasLoadRequestPending);
	}

	// Force initial loading to be "bShouldBlockOnLoad".
	if (bLevelsHaveLoadRequestPending && (!HasBegunPlay() || GetTimeSeconds() < 1.f))
	{
		// Block till all async requests are finished.
		UObject::FlushAsyncLoading();
	}

	if( bLevelsHaveBeenUnloaded && !GIsEditor )
	{
		//@todo seamless: we need to request GC to unload
	}
}

/**
 * Flushs level streaming in blocking fashion and returns when all levels are loaded/ visible/ hidden
 * so further calls to UpdateLevelStreaming won't do any work unless state changes. Basically blocks
 * on all async operation like updating components.
 *
 * @param ViewFamily	Optional collection of views to take into account
 * @param bOnlyFlushVisibility		Whether to only flush level visibility operations (optional)
 */
void UWorld::FlushLevelStreaming( FSceneViewFamily* ViewFamily, UBOOL bOnlyFlushVisibility )
{
	// Can't happen from within Tick as we are adding/ removing actors to the world.
	check( !InTick );

	AWorldInfo* WorldInfo = GetWorldInfo();

	// Allow queuing multiple load requests if we're performing a full flush and disallow if we're just
	// flushing level visibility.
	INT OldAllowLevelLoadOverride = AllowLevelLoadOverride;
	AllowLevelLoadOverride = bOnlyFlushVisibility ? 0 : 1;

	// Update internals with current loaded/ visibility flags.
	GWorld->UpdateLevelStreaming();

	// Only flush async loading if we're performing a full flush.
	if( !bOnlyFlushVisibility )
	{
		// Make sure all outstanding loads are taken care of
		UObject::FlushAsyncLoading();
	}

	// Kick off making levels visible if loading finished by flushing.
	GWorld->UpdateLevelStreaming();

	// Making levels visible is spread across several frames so we simply loop till it is done.
	UBOOL bLevelsPendingVisibility = TRUE;
	while( bLevelsPendingVisibility )
	{
		bLevelsPendingVisibility = FALSE;
		// Iterate over all levels to find out whether they have a request pending.
		for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
		{
			ULevelStreaming* LevelStreaming = WorldInfo->StreamingLevels(LevelIndex);
			
			// See whether there's a level with a pending request.
			if( LevelStreaming 
			&&	LevelStreaming->LoadedLevel 
			&&  LevelStreaming->LoadedLevel->bHasVisibilityRequestPending
			)
			{	
				// Bingo. We can early out at this point as we only care whether there is at least
				// one level in this phase.
				bLevelsPendingVisibility = TRUE;
				break;
			}
		}

		// Tick level streaming to make levels visible.
		if( bLevelsPendingVisibility )
		{
			// Only flush async loading if we're performing a full flush.
			if( !bOnlyFlushVisibility )
			{
				// Make sure all outstanding loads are taken care of...
				UObject::FlushAsyncLoading();
			}
	
			// Update level streaming.
			GWorld->UpdateLevelStreaming( ViewFamily );
		}
	}
	check( CurrentLevelPendingVisibility == NULL );

	// Update level streaming one last time to make sure all RemoveFromWorld requests made it.
	GWorld->UpdateLevelStreaming( ViewFamily );

	// We already blocked on async loading.
	if( !bOnlyFlushVisibility )
	{
		GWorld->GetWorldInfo()->bRequestedBlockOnAsyncLoading = FALSE;
	}

	AllowLevelLoadOverride = OldAllowLevelLoadOverride;
}

/**
 * Returns whether the level streaming code is allowed to issue load requests.
 *
 * @return TRUE if level load requests are allowed, FALSE otherwise.
 */
UBOOL UWorld::AllowLevelLoadRequests()
{
	UBOOL bAllowLevelLoadRequests = FALSE;
	// Hold off requesting new loads if there is an active async load request.
	if( !GIsEditor )
	{
		// Let code choose.
		if( AllowLevelLoadOverride == 0 )
		{
			// There are pending queued requests and gameplay has already started so hold off queueing.
			if( UObject::IsAsyncLoading() && GetTimeSeconds() > 1.f )
			{
				bAllowLevelLoadRequests = FALSE;
			}
			// No load requests or initial load so it's save to queue.
			else
			{
				bAllowLevelLoadRequests = TRUE;
			}
		}
		// Use override, < 0 == don't allow, > 0 == allow
		else
		{
			bAllowLevelLoadRequests = AllowLevelLoadOverride > 0 ? TRUE : FALSE;
		}
	}
	// Always allow load request in the Editor.
	else
	{
		bAllowLevelLoadRequests = TRUE;
	}
	return bAllowLevelLoadRequests;
}

UBOOL UWorld::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	//@todo seamless: move debug execs into UDebugManager
	if( NetDriver && NetDriver->Exec( Cmd, Ar ) )
	{
		return 1;
	}
	else if( DemoRecDriver && DemoRecDriver->Exec( Cmd, Ar ) )
	{
		return 1;
	}
	else if( ParseCommand( &Cmd, TEXT("SHOWEXTENTLINECHECK") ) )
	{
		bShowExtentLineChecks = !bShowExtentLineChecks;
		return 1;
	}
	else if( ParseCommand( &Cmd, TEXT("SHOWLINECHECK") ) )
	{
		bShowLineChecks = !bShowLineChecks;
		return 1;
	}
	else if( ParseCommand( &Cmd, TEXT("SHOWPOINTCHECK") ) )
	{
		bShowPointChecks = !bShowPointChecks;
		return 1;
    }
#if LINE_CHECK_TRACING
    else if( ParseCommand( &Cmd, TEXT("DUMPLINECHECKS") ) )
    {            
        LINE_CHECK_DUMP();
        return TRUE;
    }
    else if( ParseCommand( &Cmd, TEXT("RESETLINECHECKS") ) )
    {
        LINE_CHECK_RESET();
        return TRUE;
    }
    else if( ParseCommand( &Cmd, TEXT("TOGGLELINECHECKS") ) )
    {
        LINE_CHECK_TOGGLE();
        return TRUE;
    }
#endif //LINE_CHECK_TRACING
	else if( ParseCommand( &Cmd, TEXT("FLUSHPERSISTENTDEBUGLINES") ) )
	{
		PersistentLineBatcher->BatchedLines.Empty();
		PersistentLineBatcher->BeginDeferredReattach();
		return 1;
	}
	else if (ParseCommand(&Cmd, TEXT("DEMOREC")))
	{
		// Attempt to make the dir if it doesn't exist.
		FString DemoDir = appGameDir() + TEXT("Demos");
		
		GFileManager->MakeDirectory(*DemoDir, TRUE);

		FURL URL;
		FString DemoName;
		if( !ParseToken( Cmd, DemoName, 0 ) )
		{
#if NEED_THIS
			if ( !GConfig->GetString( TEXT("DemoRecording"), TEXT("DemoMask"), DemoName, TEXT("user.ini")) )
#endif
			{
				DemoName=TEXT("%m-%t");
			}
		}

		DemoName = DemoName.Replace(TEXT("%m"), *URL.Map);

		INT Year, Month, DayOfWeek, Day, Hour, Min,Sec,MSec;
		appSystemTime(Year, Month, DayOfWeek, Day, Hour, Min,Sec,MSec);

		DemoName = DemoName.Replace(TEXT("%td"), *FString::Printf(TEXT("%i-%i-%i-%i"),Year,Month,Day,((Hour*3600)+(Min*60)+Sec)*1000+MSec));
		DemoName = DemoName.Replace(TEXT("%d"),  *FString::Printf(TEXT("%i-%i-%i"),Month,Day,Year));
		DemoName = DemoName.Replace(TEXT("%t"),  *FString::Printf(TEXT("%i"),((Hour*3600)+(Min*60)+Sec)*1000+MSec));

//		UGameEngine* GameEngine = CastChecked<UGameEngine>( Engine );

		if (GEngine && 
			GEngine->GamePlayers(0) && 
			GEngine->GamePlayers(0)->Actor && 
			GEngine->GamePlayers(0)->Actor->PlayerReplicationInfo)
		{
			DemoName=DemoName.Replace( TEXT("%p"), *GEngine->GamePlayers(0)->Actor->PlayerReplicationInfo->PlayerName);
		}

		// replace bad characters with underscores
		DemoName = DemoName.Replace( TEXT("\\"), TEXT("_"));
		DemoName = DemoName.Replace( TEXT("/"),TEXT("_") );
		DemoName = DemoName.Replace( TEXT("."),TEXT("_") );
		DemoName = DemoName.Replace( TEXT(" "),TEXT("_") );

		// replace the current URL's map with a demo extension
		URL.Map = DemoDir * DemoName + TEXT(".demo");

		UClass* DemoDriverClass = StaticLoadClass(UDemoRecDriver::StaticClass(), NULL, TEXT("engine-ini:Engine.Engine.DemoRecordingDevice"), NULL, LOAD_None, NULL);
		check(DemoDriverClass);
		DemoRecDriver           = ConstructObject<UDemoRecDriver>(DemoDriverClass);
		check(DemoRecDriver);
		FString Error;
		if (!DemoRecDriver->InitListen(this, URL, Error))
		{
			Ar.Logf( TEXT("Demo recording failed: %s"), *Error );//!!localize!!
			DemoRecDriver = NULL;
		}
		else
		{
			Ar.Logf( TEXT("Demo recording started to %s"), *URL.Map );
		}
		return 1;
	}
	else if( ParseCommand( &Cmd, TEXT("DEMOPLAY") ) )
	{
		FString Temp;
		if( ParseToken( Cmd, Temp, 0 ) )
		{
			UGameEngine* GameEngine = CastChecked<UGameEngine>(GEngine);

			FURL URL( NULL, *Temp, TRAVEL_Absolute );			
			debugf( TEXT("Attempting to play demo %s"), *URL.Map );
			URL.Map = appGameDir() * TEXT("Demos") * FFilename(URL.Map).GetBaseFilename() + TEXT(".demo");

			if( GameEngine->GPendingLevel )
			{
				GameEngine->CancelPending();
			}

			GameEngine->GPendingLevel = new UDemoPlayPendingLevel( URL );
			if( !GameEngine->GPendingLevel->DemoRecDriver )
			{
				Ar.Logf( TEXT("Demo playback failed: %s"), *GameEngine->GPendingLevel->Error );//!!localize!!
				GameEngine->GPendingLevel = NULL;
			}
		}
		else
		{
			Ar.Log( TEXT("You must specify a filename") );//!!localize!!
		}
		return 1;
	}
	else if( Hash->Exec(Cmd,Ar) )
	{
		return 1;
	}
	else if (NavigationOctree->Exec(Cmd, Ar))
	{
		return 1;
	}
	else if( ExecRBCommands( Cmd, &Ar ) )
	{
		return 1;
	}
	else 
	{
		return 0;
	}
}

void UWorld::SetGameInfo(const FURL& InURL)
{
	AWorldInfo* Info = GetWorldInfo();

	if( IsServer() && !Info->Game )
	{
		// Init the game info.
		TCHAR Options[1024]=TEXT("");
		TCHAR GameClassName[256]=TEXT("");
		FString	Error=TEXT("");
		for( INT i=0; i<InURL.Op.Num(); i++ )
		{
			appStrcat( Options, TEXT("?") );
			appStrcat( Options, *InURL.Op(i) );
			Parse( *InURL.Op(i), TEXT("GAME="), GameClassName, ARRAY_COUNT(GameClassName) );
		}

		UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);

		// Get the GameInfo class.
		UClass* GameClass=NULL;
		if ( GameClassName[0] )
		{
			// if the gamename was specified, we can use it to fully load the pergame PreLoadClass packages
			if (GameEngine)
			{
				GameEngine->LoadPackagesFully(FULLYLOAD_Game_PreLoadClass, GameClassName);
			}

			GameClass = StaticLoadClass( AGameInfo::StaticClass(), NULL, GameClassName, NULL, LOAD_None, NULL);
		}
		if ( !GameClass )
		{
			GameClass = StaticLoadClass( AGameInfo::StaticClass(), NULL, (GEngine->Client != NULL && !InURL.HasOption(TEXT("Listen"))) ? TEXT("game-ini:Engine.GameInfo.DefaultGame") : TEXT("game-ini:Engine.GameInfo.DefaultServerGame"), NULL, LOAD_None, NULL);
		}
        if ( !GameClass ) 
			GameClass = AGameInfo::StaticClass();
		else
			GameClass = Cast<AGameInfo>(GameClass->GetDefaultActor())->eventSetGameType(*InURL.Map, Options);

		// no matter how the game was specified, we can use it to load the PostLoadClass packages
		if (GameEngine)
		{
			GameEngine->LoadPackagesFully(FULLYLOAD_Game_PostLoadClass, GameClass->GetPathName());
		}

		// Spawn the GameInfo.
		debugf( NAME_Log, TEXT("Game class is '%s'"), *GameClass->GetName() );
		Info->Game = (AGameInfo*)SpawnActor( GameClass );
		check(Info->Game!=NULL);
	}
}

//#define DEBUG_CHECKCOLLISIONCOMPONENTS 1
// used to check if actors have more than one collision component (most should not, but can easily happen accidentally)

/** BeginPlay - Begins gameplay in the level.
 * @param InURL commandline URL
 * @param bResetTime (optional) whether the WorldInfo's TimeSeconds should be reset to zero
 */
void UWorld::BeginPlay(const FURL& InURL, UBOOL bResetTime)
{
	check(bIsWorldInitialized);
	DOUBLE StartTime = appSeconds();

	AWorldInfo* Info = GetWorldInfo();

	// Don't reset time for seamless world transitions.
	if (bResetTime)
	{
		GetWorldInfo()->TimeSeconds = 0.0f;
		GetWorldInfo()->RealTimeSeconds = 0.0f;
	}

	// Get URL Options
	TCHAR Options[1024]=TEXT("");
	FString	Error=TEXT("");
	for( INT i=0; i<InURL.Op.Num(); i++ )
	{
		appStrcat( Options, TEXT("?") );
		appStrcat( Options, *InURL.Op(i) );
	}

	// Set level info.
	if( !InURL.GetOption(TEXT("load"),NULL) )
		URL = InURL;
	Info->EngineVersion = FString::Printf( TEXT("%i"), GEngineVersion );
	Info->MinNetVersion = FString::Printf( TEXT("%i"), GEngineMinNetVersion );
	Info->ComputerName = appComputerName();

	// Update world and the components of all levels.
	UpdateComponents( TRUE );

	// Clear any existing stat charts.
	if(GStatChart)
	{
		GStatChart->Reset();
	}

	// Reset indices till we have a chance to rearrange actor list at the end of this function.
	for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = Levels(LevelIndex);
		Level->iFirstDynamicActor		= 0;
		Level->iFirstNetRelevantActor	= 0;
	}

	// Initialize rigid body physics.
	InitWorldRBPhys();

	// Initialise physics engine for persistant level. 
	// This creates physics engine BSP representation and iterates over actors calling InitRBPhys for each.
	PersistentLevel->InitLevelBSPPhysMesh();
	PersistentLevel->IncrementalInitActorsRBPhys(0);

	// Init level gameplay info.
	if( !HasBegunPlay() )
	{
		// Check that paths are valid
		if ( !GetWorldInfo()->bPathsRebuilt )
		{
			warnf(TEXT("*** WARNING - PATHS MAY NOT BE VALID ***"));
		}

		// Lock the level.
		debugf( NAME_Log, TEXT("Bringing %s up for play (%i) at %s"), *GetFullName(), appRound(GEngine->GetMaxTickRate(0,FALSE)), *appSystemTimeString() );
		GetWorldInfo()->GetDefaultPhysicsVolume()->bNoDelete = true;

		// Initialize all actors and start execution.
		PersistentLevel->InitializeActors();

		// Enable actor script calls.
		Info->bBegunPlay	= 1;
		Info->bStartup		= 1;

		// Init the game.
		if (Info->Game != NULL && !Info->Game->bScriptInitialized)
		{
			Info->Game->eventInitGame( Options, Error );
		}

		// Route various begin play functions and set volumes.
		PersistentLevel->RouteBeginPlay();

		// Intialize any scripting sequences
		if (GetGameSequence() != NULL)
		{
			GetGameSequence()->BeginPlay();
		}

		Info->bStartup = 0;
	}

	// Rearrange actors: static not net relevant actors first, then static net relevant actors and then others.
	check( Levels.Num() );
	check( PersistentLevel );
	check( Levels(0) == PersistentLevel );
	for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel*	Level = Levels(LevelIndex);
		Level->SortActorList();
	}

	debugf(TEXT("Bringing up level for play took: %f"), appSeconds() - StartTime );
}

/**
 * Cleans up components, streaming data and assorted other intermediate data.
 * @param bSessionEnded whether to notify the viewport that the game session has ended
 */
void UWorld::CleanupWorld(UBOOL bSessionEnded)
{
	check(CurrentLevelPendingVisibility == NULL);

	if (bSessionEnded && GEngine != NULL && GEngine->GameViewport != NULL)
	{
		GEngine->GameViewport->eventGameSessionEnded();
	}

	// Tell actors to remove their components from the scene.
	ClearComponents();
	
	// Remove all objects from octree.
	if( NavigationOctree )
	{
		NavigationOctree->RemoveAllObjects();
	}

	// Clear standalone flag when switching maps in the Editor. This causes resources placed in the map
	// package to be garbage collected together with the world.
	if( GIsEditor && !IsTemplate() )
	{
		UPackage* WorldPackage = GetOutermost();
		// Iterate over all objects to find ones that reside in the same package as the world.
		for( FObjectIterator It; It; ++It )
		{
			UObject* Object = *It;
			if( Object->IsIn( WorldPackage ) )
			{
				Object->ClearFlags( RF_Standalone );
			}
		}
	}
}

/*-----------------------------------------------------------------------------
	Accessor functions.
-----------------------------------------------------------------------------*/

/**
 * Returns the current netmode
 *
 * @return current netmode
 */
ENetMode UWorld::GetNetMode() const
{
	return (ENetMode) GetWorldInfo()->NetMode;
}

/**
 * Returns the current game info object.
 *
 * @return current game info object
 */
AGameInfo* UWorld::GetGameInfo() const
{
	return GetWorldInfo()->Game;
}

/**
 * Returns the first navigation point. Note that ANavigationPoint contains
 * a pointer to the next navigation point so this basically returns a linked
 * list of navigation points in the world.
 *
 * @return first navigation point
 */
ANavigationPoint* UWorld::GetFirstNavigationPoint() const
{
	return GetWorldInfo()->NavigationPointList;
}

/**
 * Returns the first controller. Note that AController contains a pointer to
 * the next controller so this basically returns a linked list of controllers
 * associated with the world.
 *
 * @return first controller
 */
AController* UWorld::GetFirstController() const
{
	return GetWorldInfo()->ControllerList;
}

/**
 * Returns the first pawn. Note that APawn contains a pointer to
 * the next pawn so this basically returns a linked list of pawns
 * associated with the world.
 *
 * @return first pawn
 */
APawn* UWorld::GetFirstPawn() const
{
	return GetWorldInfo()->PawnList;
}

//debug
//pathdebug
#if 0 && !PS3 && !FINAL_RELEASE
#define CHECKNAVLIST(b, x) \
		if( !GIsEditor && ##b ) \
		{ \
			debugf(*##x); \
			for (ANavigationPoint *T = GWorld->GetFirstNavigationPoint(); T != NULL; T = T->nextNavigationPoint) \
			{ \
				T->ClearForPathFinding(); \
			} \
			UWorld::VerifyNavList(*##x); \
		}
#else
#define CHECKNAVLIST(b, x)
#endif

void UWorld::AddLevelNavList( ULevel *Level, UBOOL bDebugNavList )
{
	if (Level != NULL && Level->NavListStart != NULL && Level->NavListEnd != NULL)
	{
		// don't add if we're in the editor
		if (GIsGame)
		{
			// for each nav in the level nav list
			for (ANavigationPoint *Nav = Level->NavListStart; Nav != NULL; Nav = Nav->nextNavigationPoint)
			{
				// add the nav to the octree
				Nav->AddToNavigationOctree();
			}
		}
		AWorldInfo *Info = GetWorldInfo();
		// insert the level at the beginning of the nav list
		Level->NavListEnd->nextNavigationPoint = Info->NavigationPointList;
		Info->NavigationPointList = Level->NavListStart;

		// insert the cover list as well
		if (Level->CoverListStart != NULL && Level->CoverListEnd != NULL)
		{
			Level->CoverListEnd->NextCoverLink = Info->CoverList;
			Info->CoverList = Level->CoverListStart; 
		}

		//debug
		CHECKNAVLIST(bDebugNavList , FString::Printf(TEXT("AddLevelNavList %s"), *Level->GetFullName()));
	}
}

void UWorld::RemoveLevelNavList( ULevel *Level, UBOOL bDebugNavList )
{
	if (Level != NULL && Level->NavListStart != NULL && Level->NavListEnd != NULL)
	{
		//debug
		CHECKNAVLIST(bDebugNavList , FString::Printf(TEXT("RemoveLevelNavList %s"), *Level->GetFullName()));

		AWorldInfo *Info = GetWorldInfo();
		// if this level is at the start of the list,
		if (Level->NavListStart == Info->NavigationPointList)
		{
			// then just move the start of the end to the next level
			Info->NavigationPointList = Level->NavListEnd->nextNavigationPoint;
		}
		else
		{
			// otherwise find the nav that is referencing the start of this level's list
			ANavigationPoint *End = NULL;
			for (INT LevelIdx = 0; LevelIdx < Levels.Num(); LevelIdx++)
			{
				ULevel *ChkLevel = Levels(LevelIdx);
				if (ChkLevel != Level &&
					ChkLevel->NavListEnd != NULL &&
					ChkLevel->NavListEnd->nextNavigationPoint == Level->NavListStart)
				{
					End = ChkLevel->NavListEnd;
					break;
				}
			}
			if (End != NULL)
			{
				// point the current end to the level's end's next point
				End->nextNavigationPoint = Level->NavListEnd->nextNavigationPoint;
			}
		}
		// and clear the level's end
		Level->NavListEnd->nextNavigationPoint = NULL;

		// update the cover list as well
		if (Level->CoverListStart != NULL && Level->CoverListEnd != NULL)
		{
			if (Level->CoverListStart == Info->CoverList)
			{
				Info->CoverList = Level->CoverListEnd->NextCoverLink;
			}
			else
			{
				ACoverLink *End = NULL;
				for (INT LevelIdx = 0; LevelIdx < Levels.Num(); LevelIdx++)
				{
					ULevel *ChkLevel = Levels(LevelIdx);
					if (ChkLevel != Level &&
						ChkLevel->CoverListEnd != NULL &&
						ChkLevel->CoverListEnd->NextCoverLink == Level->CoverListStart)
					{
						End = ChkLevel->CoverListEnd;
						break;
					}
				}
				if (End != NULL)
				{
					End->NextCoverLink = Level->CoverListEnd->NextCoverLink;
				}
			}
			if (Level->CoverListEnd != NULL)
			{
				Level->CoverListEnd->NextCoverLink = NULL;
			}
		}
	}
}

void UWorld::ResetNavList()
{
	// tada!
	GetWorldInfo()->NavigationPointList = NULL;
	GetWorldInfo()->CoverList = NULL;
}

ANavigationPoint* UWorld::FindNavByGuid(FGuid &Guid)
{
	ANavigationPoint *ResultNav = NULL;
	for (INT LevelIdx = 0; LevelIdx < Levels.Num() && ResultNav == NULL; LevelIdx++)
	{
		ULevel *Level = Levels(LevelIdx);
		for (ANavigationPoint *Nav = Level->NavListStart; Nav != NULL; Nav = Nav->nextNavigationPoint)
		{
			if (Nav->NavGuid == Guid)
			{
				ResultNav = Nav;
				break;
			}
		}
	}
	if (GIsEditor && ResultNav == NULL)
	{
		// use an actor iterator in the editor since paths may not be rebuilt
		for (FActorIterator It; It; ++It)
		{
			ANavigationPoint *Nav = Cast<ANavigationPoint>(*It);
			if (Nav != NULL && Nav->NavGuid == Guid)
			{
				ResultNav = Nav;
				break;
			}
		}
	}
	return ResultNav;
}

/**
 * Inserts the passed in controller at the front of the linked list of controllers.
 *
 * @param	Controller	Controller to insert, use NULL to clear list
 */
void UWorld::AddController( AController* Controller )
{
	if( Controller )
	{
		Controller->NextController = GetWorldInfo()->ControllerList;
	}
	GetWorldInfo()->ControllerList = Controller;
}

/**
 * Removes the passed in controller from the linked list of controllers.
 *
 * @param	Controller	Controller to remove
 */
void UWorld::RemoveController( AController* Controller )
{
	AController* Next = GetFirstController();
	if( Next == Controller )
	{
		GetWorldInfo()->ControllerList = Controller->NextController;
	}
	else
	{
		while( Next != NULL )
		{
			if( Next->NextController == Controller )
			{
				Next->NextController = Controller->NextController;
				break;
			}
			Next = Next->NextController;
		}
	}
}

/**
 * Inserts the passed in pawn at the front of the linked list of pawns.
 *
 * @param	Pawn	Pawn to insert, use NULL to clear list
 */
void UWorld::AddPawn( APawn* Pawn )
{
	if( Pawn )
	{
		// make sure it's not already in the list
		RemovePawn(Pawn);
		// and add to the beginning
		Pawn->NextPawn = GetWorldInfo()->PawnList;
	}
	GetWorldInfo()->PawnList = Pawn;
}

/**
 * Removes the passed in pawn from the linked list of pawns.
 *
 * @param	Pawn	Pawn to remove
 */
void UWorld::RemovePawn( APawn* Pawn )
{
	APawn* Next = GetFirstPawn();
	if( Next == Pawn )
	{
		GetWorldInfo()->PawnList = Pawn->NextPawn;
	}
	else
	{
		while( Next != NULL )
		{
			if( Next->NextPawn == Pawn )
			{
				Next->NextPawn = Pawn->NextPawn;
				break;
			}
			Next = Next->NextPawn;
		}
	}
}

/**
 * Returns the default brush.
 *
 * @return default brush
 */
ABrush* UWorld::GetBrush() const
{
	check(PersistentLevel);
	return PersistentLevel->GetBrush();
}

/**
 * Returns whether game has already begun play.
 *
 * @return TRUE if game has already started, FALSE otherwise
 */
UBOOL UWorld::HasBegunPlay() const
{
	return PersistentLevel->Actors.Num() && GetWorldInfo()->bBegunPlay;
}

/**
 * Returns whether gameplay has already begun and we are not associating a level
 * with the world.
 *
 * @return TRUE if game has already started and we're not associating a level, FALSE otherwise
 */
UBOOL UWorld::HasBegunPlayAndNotAssociatingLevel() const
{
	return HasBegunPlay() && !GIsAssociatingLevel;
}

/**
 * Returns time in seconds since world was brought up for play, IS stopped when game pauses.
 *
 * @return time in seconds since world was brought up for play
 */
FLOAT UWorld::GetTimeSeconds() const
{
	return GetWorldInfo()->TimeSeconds;
}

/**
* Returns time in seconds since world was brought up for play, is NOT stopped when game pauses.
*
* @return time in seconds since world was brought up for play
*/
FLOAT UWorld::GetRealTimeSeconds() const
{
	return GetWorldInfo()->RealTimeSeconds;
}

/**
 * Returns the frame delta time in seconds adjusted by e.g. time dilation.
 *
 * @return frame delta time in seconds adjusted by e.g. time dilation
 */
FLOAT UWorld::GetDeltaSeconds() const
{
	return GetWorldInfo()->DeltaSeconds;
}

/**
 * Returns the default physics volume and creates it if necessary.
 * 
 * @return default physics volume
 */
APhysicsVolume* UWorld::GetDefaultPhysicsVolume() const
{
	return GetWorldInfo()->GetDefaultPhysicsVolume();
}

/**
 * Returns the physics volume a given actor is in.
 *
 * @param	Location
 * @param	Actor
 * @param	bUseTouch
 *
 * @return physics volume given actor is in.
 */
APhysicsVolume* UWorld::GetPhysicsVolume(FVector Loc, AActor *A, UBOOL bUseTouch) const
{
	return GetWorldInfo()->GetPhysicsVolume(Loc,A,bUseTouch);
}

/**
 * Returns the global/ persistent Kismet sequence for the specified level.
 *
 * @param	OwnerLevel		the level to get the sequence from - must correspond to one of the levels in GWorld's Levels array;
 *							thus, only applicable when editing a multi-level map.  Defaults to the level currently being edited.
 *
 * @return	a pointer to the sequence located at the specified element of the specified level's list of sequences, or
 *			NULL if that level doesn't exist or the index specified is invalid for that level's list of sequences
 */
USequence* UWorld::GetGameSequence(ULevel* OwnerLevel/* =NULL  */) const
{
	if( OwnerLevel == NULL )
	{
		OwnerLevel = CurrentLevel;
	}
	check(OwnerLevel);
	return OwnerLevel->GetGameSequence();
}

/**
 * Sets the current (or specified) level's kismet sequence to the sequence specified.
 *
 * @param	GameSequence	the sequence to add to the level
 * @param	OwnerLevel		the level to add the sequence to - must correspond to one of the levels in GWorld's Levels array;
 *							thus, only applicable when editing a multi-level map.  Defaults to the level currently being edited.
 */
void UWorld::SetGameSequence(USequence* GameSequence, ULevel* OwnerLevel/* =NULL  */)
{
	if( OwnerLevel == NULL )
	{
		OwnerLevel = CurrentLevel;
	}

	if( OwnerLevel->GameSequences.Num() == 0 )
	{
		OwnerLevel->GameSequences.AddItem( GameSequence );
	}
	else
	{
		OwnerLevel->GameSequences(0) = GameSequence;
	}
	check( OwnerLevel->GameSequences.Num() == 1 );
}


/**
 * Returns the AWorldInfo actor associated with this world.
 *
 * @return AWorldInfo actor associated with this world
 */
AWorldInfo* UWorld::GetWorldInfo() const
{
	checkSlow(PersistentLevel);
	checkSlow(PersistentLevel->Actors.Num());
	checkSlow(PersistentLevel->Actors(0));
	checkSlow(PersistentLevel->Actors(0)->IsA(AWorldInfo::StaticClass()));
	return (AWorldInfo*)PersistentLevel->Actors(0);
}

/**
 * Returns the current levels BSP model.
 *
 * @return BSP UModel
 */
UModel* UWorld::GetModel() const
{
	check(CurrentLevel);
	return CurrentLevel->Model;
}

/**
 * Returns the Z component of the current world gravity.
 *
 * @return Z component of current world gravity.
 */
FLOAT UWorld::GetGravityZ() const
{
	return GetWorldInfo()->GetGravityZ();
}

/**
 * Returns the Z component of the default world gravity.
 *
 * @return Z component of the default world gravity.
 */
FLOAT UWorld::GetDefaultGravityZ() const
{
	return GetWorldInfo()->DefaultGravityZ;
}

/**
 * Returns the Z component of the current world gravity scaled for rigid body physics.
 *
 * @return Z component of the current world gravity scaled for rigid body physics.
 */
FLOAT UWorld::GetRBGravityZ() const
{
	return GetWorldInfo()->GetRBGravityZ();
}

/**
 * Returns the name of the current map, taking into account using a dummy persistent world
 * and loading levels into it via PrepareMapChange.
 *
 * @return	name of the current map
 */
const FString UWorld::GetMapName() const
{
	// Default to the world's package as the map name.
	FString MapName = GetOutermost()->GetName();
	
	// In the case of a seamless world check to see whether there are any persistent levels in the levels
	// array and use its name if there is one.
	AWorldInfo* WorldInfo = GetWorldInfo();
	for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
	{
		ULevelStreamingPersistent* PersistentLevel = Cast<ULevelStreamingPersistent>( WorldInfo->StreamingLevels(LevelIndex) );
		// Use the name of the first found persistent level.
		if( PersistentLevel )
		{
			MapName = PersistentLevel->PackageName.ToString();
			break;
		}
	}

	return MapName;
}

/*-----------------------------------------------------------------------------
	Network interface.
-----------------------------------------------------------------------------*/

//
// The network driver is about to accept a new connection attempt by a
// connectee, and we can accept it or refuse it.
//
EAcceptConnection UWorld::NotifyAcceptingConnection()
{
	check(NetDriver);
	if( NetDriver->ServerConnection )
	{
		// We are a client and we don't welcome incoming connections.
		debugf( NAME_DevNet, TEXT("NotifyAcceptingConnection: Client refused") );
		return ACCEPTC_Reject;
	}
	else if( GetWorldInfo()->NextURL!=TEXT("") )
	{
		// Server is switching levels.
		debugf( NAME_DevNet, TEXT("NotifyAcceptingConnection: Server %s refused"), *GetName() );
		return ACCEPTC_Ignore;
	}
	else
	{
		// Server is up and running.
		debugf( NAME_DevNet, TEXT("NotifyAcceptingConnection: Server %s accept"), *GetName() );
		return ACCEPTC_Accept;
	}
}

//
// This server has accepted a connection.
//
void UWorld::NotifyAcceptedConnection( UNetConnection* Connection )
{
	check(NetDriver!=NULL);
	check(NetDriver->ServerConnection==NULL);
	debugf( NAME_NetComeGo, TEXT("Open %s %s %s"), *GetName(), appTimestamp(), *Connection->LowLevelGetRemoteAddress() );
}

//
// The network interface is notifying this level of a new channel-open
// attempt by a connectee, and we can accept or refuse it.
//
UBOOL UWorld::NotifyAcceptingChannel( UChannel* Channel )
{
	check(Channel);
	check(Channel->Connection);
	check(Channel->Connection->Driver);
	UNetDriver* Driver = Channel->Connection->Driver;

	if( Driver->ServerConnection )
	{
		// We are a client and the server has just opened up a new channel.
		//debugf( "NotifyAcceptingChannel %i/%i client %s", Channel->ChIndex, Channel->ChType, *GetName() );
		if( Channel->ChType==CHTYPE_Actor )
		{
			// Actor channel.
			//debugf( "Client accepting actor channel" );
			return 1;
		}
		else
		{
			// Unwanted channel type.
			debugf( NAME_DevNet, TEXT("Client refusing unwanted channel of type %i"), (BYTE)Channel->ChType );
			return 0;
		}
	}
	else
	{
		// We are the server.
		if( Channel->ChIndex==0 && Channel->ChType==CHTYPE_Control )
		{
			// The client has opened initial channel.
			debugf( NAME_DevNet, TEXT("NotifyAcceptingChannel Control %i server %s: Accepted"), Channel->ChIndex, *GetFullName() );
			return 1;
		}
		else if( Channel->ChType==CHTYPE_File )
		{
			// The client is going to request a file.
			debugf( NAME_DevNet, TEXT("NotifyAcceptingChannel File %i server %s: Accepted"), Channel->ChIndex, *GetFullName() );
			return 1;
		}
		else
		{
			// Client can't open any other kinds of channels.
			debugf( NAME_DevNet, TEXT("NotifyAcceptingChannel %i %i server %s: Refused"), (BYTE)Channel->ChType, Channel->ChIndex, *GetFullName() );
			return 0;
		}
	}
}

//
// Welcome a new player joining this server.
//
void UWorld::WelcomePlayer( UNetConnection* Connection, const TCHAR* Optional )
{
	check(CurrentLevel);
	Connection->PackageMap->Copy( Connection->Driver->MasterMap );
	Connection->SendPackageMap();

	FString LevelName = CurrentLevel->GetOutermost()->GetName();
	Connection->ClientWorldPackageName = GetOutermost()->GetFName();
	if( Optional[0] )
	{
		Connection->Logf( TEXT("WELCOME LEVEL=%s %s"), *LevelName, Optional );
	}
	else
	{
		Connection->Logf( TEXT("WELCOME LEVEL=%s"), *LevelName );
	}
	Connection->FlushNet();
	Connection->bWelcomed = TRUE;
	// don't count initial join data for netspeed throttling
	// as it's unnecessary, since connection won't be fully open until it all gets received, and this prevents later gameplay data from being delayed to "catch up"
	Connection->QueuedBytes = 0;
}

/** verifies that the client has loaded or can load the package with the specified information
 * if found, sets the Info's Parent to the package and notifies the server of our generation of the package
 * if not, handles downloading the package, skipping it, or disconnecting, depending on the requirements of the package
 * @param Info the info on the package the client should have
 */
UBOOL UWorld::VerifyPackageInfo(FPackageInfo& Info)
{
	check(!GWorld->IsServer());

	if( GUseSeekFreeLoading )
	{
		FString PackageNameString = Info.PackageName.ToString();
		// try to find the package in memory
		Info.Parent = FindPackage(NULL, *PackageNameString);
		if (Info.Parent == NULL)
		{
			if (IsAsyncLoading())
			{
				// delay until async loading is complete
				return FALSE;
			}
			else if (Info.ForcedExportBasePackageName == NAME_None)
			{
				// attempt to async load the package
				FString Filename;
				if (GPackageFileCache->FindPackageFile(*PackageNameString, &Info.Guid, Filename))
				{
					// check for localized package first as that may be required to load the base package
					FString LocalizedPackageName = PackageNameString + LOCALIZED_SEEKFREE_SUFFIX;
					FString LocalizedFileName;
					if (GPackageFileCache->FindPackageFile(*LocalizedPackageName, NULL, LocalizedFileName))
					{
						LoadPackageAsync(*LocalizedPackageName, NULL, NULL);
					}
					// load base package
					LoadPackageAsync(PackageNameString, NULL, NULL);
					return FALSE;
				}
			}
			if (Info.ForcedExportBasePackageName != NAME_None)
			{
				// this package is a forced export inside another package, so try loading that other package
				FString BasePackageNameString(Info.ForcedExportBasePackageName.ToString());
				FString FileName;
				if (GPackageFileCache->FindPackageFile(*BasePackageNameString, NULL, FileName))
				{
					// check for localized package first as that may be required to load the base package
					FString LocalizedPackageName = BasePackageNameString + LOCALIZED_SEEKFREE_SUFFIX;
					FString LocalizedFileName;
					if (GPackageFileCache->FindPackageFile(*LocalizedPackageName, NULL, LocalizedFileName))
					{
						LoadPackageAsync(*LocalizedPackageName, NULL, NULL);
					}
					// load base package
					LoadPackageAsync(*BasePackageNameString, NULL, NULL);
					return FALSE;
				}
			}
			debugf(NAME_DevNet, TEXT("Failed to find required package '%s'"), *PackageNameString);
			GEngine->SetClientTravel(TEXT("?failed"), TRAVEL_Absolute);
			GEngine->SetProgress(*FString::Printf(TEXT("Failed to find required package '%s'"), *PackageNameString), TEXT(""), 4.0f);
			NetDriver->ServerConnection->Close();
		}
		else if (Info.Parent->GetGuid() != Info.Guid)
		{
			debugf(NAME_DevNet, TEXT("Package '%s' mismatched - Server GUID: %s Client GUID: %s"), *Info.Parent->GetName(), *Info.Guid.String(), *Info.Parent->GetGuid().String());
			GEngine->SetClientTravel(TEXT("?failed"), TRAVEL_Absolute);
			GEngine->SetProgress(*FString::Printf(TEXT("Package '%s' version mismatch"), *Info.Parent->GetName()), TEXT(""), 4.0f);
			NetDriver->ServerConnection->Close();
		}
		else
		{
			Info.LocalGeneration = Info.Parent->GetGenerationNetObjectCount().Num();
			// tell the server what we have
			NetDriver->ServerConnection->Logf(TEXT("HAVE GUID=%s GEN=%i"), *Info.Parent->GetGuid().String(), Info.LocalGeneration);
		}
	}
	else
	{
		// verify that we have this package, or it is downloadable
		FString Filename;
		if (GPackageFileCache->FindPackageFile(*Info.PackageName.ToString(), &Info.Guid, Filename))
		{
			Info.Parent = FindPackage(NULL, *Info.PackageName.ToString());
			if (Info.Parent == NULL)
			{
				if (IsAsyncLoading())
				{
					// delay until async loading is complete
					return FALSE;
				}
				Info.Parent = CreatePackage(NULL, *Info.PackageName.ToString());
			}
			// check that the GUID matches (meaning it is the same package or it has been conformed)
			if (!Info.Parent->GetGuid().IsValid() || Info.Parent->GetGenerationNetObjectCount().Num() == 0)
			{
				if (IsAsyncLoading())
				{
					// delay until async loading is complete
					return FALSE;
				}
				// we need to load the linker to get the info
				BeginLoad();
				GetPackageLinker(Info.Parent, NULL, LOAD_NoWarn | LOAD_NoVerify | LOAD_Quiet, NULL, &Info.Guid);
				EndLoad();
			}
			if (Info.Parent->GetGuid() != Info.Guid)
			{
				// incompatible files
				//@todo FIXME: we need to be able to handle this better - have the client ignore this version of the package and download the correct one
				debugf(NAME_DevNet, TEXT("Package '%s' mismatched - Server GUID: %s Client GUID: %s"), *Info.Parent->GetName(), *Info.Guid.String(), *Info.Parent->GetGuid().String());
				GEngine->SetClientTravel(TEXT("?failed"), TRAVEL_Absolute);
				GEngine->SetProgress(*FString::Printf(TEXT("Package '%s' version mismatch"), *Info.Parent->GetName()), TEXT(""), 4.0f);
				NetDriver->ServerConnection->Close();
			}
			else
			{
				Info.LocalGeneration = Info.Parent->GetGenerationNetObjectCount().Num();
				// tell the server what we have
				NetDriver->ServerConnection->Logf(TEXT("HAVE GUID=%s GEN=%i"), *Info.Parent->GetGuid().String(), Info.LocalGeneration);
			}
		}
		else
		{
			// we need to download this package
			//@fixme: FIXME: handle
			debugf(NAME_DevNet, TEXT("Failed to find required package '%s'"), *Info.Parent->GetName());
			GEngine->SetClientTravel(TEXT("?failed"), TRAVEL_Absolute);
			GEngine->SetProgress(*FString::Printf(TEXT("Downloading '%s' not allowed"), *Info.Parent->GetName()), TEXT(""), 4.0f);
			NetDriver->ServerConnection->Close();
		}
	}

	// on success, update the entry in the package map
	NetDriver->ServerConnection->PackageMap->AddPackageInfo(Info);

	return TRUE;
}

//
// Received text on the control channel.
//
void UWorld::NotifyReceivedText( UNetConnection* Connection, const TCHAR* Text )
{
	if( ParseCommand(&Text,TEXT("USERFLAG")) )
	{
		Connection->UserFlags = appAtoi(Text);
	}
	else if( NetDriver->ServerConnection )
	{
		check(Connection == NetDriver->ServerConnection);

		// We are the client.
		debugf( NAME_DevNet, TEXT("Level client received: %s"), Text );
		if( ParseCommand(&Text,TEXT("FAILURE")) )
		{
			if (GEngine->GamePlayers.Num() == 0 || GEngine->GamePlayers(0) == NULL || GEngine->GamePlayers(0)->Actor == NULL || !GEngine->GamePlayers(0)->Actor->eventNotifyConnectionLost())
			{
				GEngine->SetClientTravel(TEXT("?closed"), TRAVEL_Absolute);
			}
		}
		else if ( ParseCommand(&Text,TEXT("BRAWL")) )
		{
			debugf(TEXT("The client has failed one of the MD5 tests"));
			GEngine->SetClientTravel( TEXT("?failed"), TRAVEL_Absolute );
		}
		else if (ParseCommand(&Text, TEXT("USES")))
		{
			// Dependency information.
			FPackageInfo Info(NULL);
			Connection->ParsePackageInfo(Text, Info);
			// add to the packagemap immediately even if we can't verify it to guarantee its place in the list as it needs to match the server
			Connection->PackageMap->AddPackageInfo(Info);
			// it's important to verify packages in order so that we're not reshuffling replicated indices during gameplay, so don't try if there are already others pending
			if (Connection->PendingPackageInfos.Num() > 0 || !VerifyPackageInfo(Info))
			{
				// we can't verify the package until we have finished async loading
				Connection->PendingPackageInfos.AddItem(Info);
			}
		}
		else if (ParseCommand(&Text, TEXT("UNLOAD")))
		{
			// remove a package from the package map
			FGuid Guid;
			Parse(Text, TEXT("GUID="), Guid);
			// remove ref from pending list, if present
			for (INT i = 0; i < Connection->PendingPackageInfos.Num(); i++)
			{
				if (Connection->PendingPackageInfos(i).Guid == Guid)
				{
					Connection->PendingPackageInfos.Remove(i);
					// if the package was pending, the server is expecting a response, so tell it we aborted as requested
					Connection->Logf(TEXT("ABORT GUID=%s"), *Guid.String());
					break;
				}
			}
			// now remove from package map itself
			Connection->PackageMap->RemovePackageByGuid(Guid);
		}
	}
	else
	{
		// We are the server.
		debugf( NAME_DevNet, TEXT("Level server received: %s"), Text );
		if( ParseCommand(&Text,TEXT("HELLO")) )
		{
			// Versions.
			INT RemoteMinVer=219, RemoteVer=219;
			Parse( Text, TEXT("MINVER="), RemoteMinVer );
			Parse( Text, TEXT("VER="),    RemoteVer    );
			if( RemoteVer<GEngineMinNetVersion || RemoteMinVer>GEngineVersion )
			{
				Connection->Logf( TEXT("UPGRADE MINVER=%i VER=%i"), GEngineMinNetVersion, GEngineVersion );
				Connection->FlushNet();
				Connection->State = USOCK_Closed;
				return;
			}
			Connection->NegotiatedVer = Min(RemoteVer,GEngineVersion);

			// Get byte limit.
			INT Stats = 0;//GetGameInfo()->bEnableStatLogging;
			Connection->Challenge = appCycles();
			Connection->Logf( TEXT("CHALLENGE VER=%i CHALLENGE=%i STATS=%i"), Connection->NegotiatedVer, Connection->Challenge, Stats );
			Connection->FlushNet();
		}
		else if( ParseCommand(&Text,TEXT("NETSPEED")) )
		{
			INT Rate = appAtoi(Text);
			Connection->CurrentNetSpeed = Clamp( Rate, 1800, NetDriver->MaxClientRate );
			debugf( TEXT("Client netspeed is %i"), Connection->CurrentNetSpeed );
		}
		else if( ParseCommand(&Text,TEXT("HAVE")) )
		{
			// Client specifying his generation.
			FGuid Guid(0,0,0,0);
			Parse( Text, TEXT("GUID=" ), Guid );
			UBOOL bFound = FALSE;
			for (TArray<FPackageInfo>::TIterator It(Connection->PackageMap->List); It; ++It)
			{
				if (It->Guid == Guid)
				{
					Parse( Text, TEXT("GEN=" ), It->RemoteGeneration );
					//@warning: it's important we compute here before executing any pending removal, so we're sure all object counts have been set
					Connection->PackageMap->Compute();
					// if the package was pending removal, we can do that now that we're synchronized
					if (Connection->PendingRemovePackageGUIDs.RemoveItem(Guid) > 0)
					{
						Connection->PackageMap->RemovePackage(It->Parent, FALSE);
					}
					bFound = TRUE;
					break;
				}
			}
			if (!bFound)
			{
				// the client specified a package with an incorrect GUID or one that it should not be using; kick it out
				Connection->Logf(TEXT("FAILURE"));
				Connection->Close();
			}
		}
		else if (ParseCommand(&Text, TEXT("ABORT")))
		{
			// client was verifying a package, but received an "UNLOAD" and aborted it
			FGuid Guid(0,0,0,0);
			Parse(Text, TEXT("GUID="), Guid);
			// if the package was pending removal, we can do that now that we're synchronized
			if (Connection->PendingRemovePackageGUIDs.RemoveItem(Guid) > 0)
			{
				Connection->PackageMap->RemovePackageByGuid(Guid);
			}
			else
			{
				/* @todo: this can happen and be valid if the server loads a package, unloads a package, then reloads that package again within a short span of time
							(e.g. streaming) - the package then won't be in the PendingRemovePackageGUIDs list, but the client may send an ABORT for the UNLOAD in between the two USES
							possibly can resolve by keeping track of how many responses we're expecting... or somethin
				debugf(NAME_DevNet, TEXT("Received ABORT with invalid GUID %s, disconnecting client"), *Guid.String());
				Connection->Logf(TEXT("FAILURE"));
				Connection->Close();
				*/
				debugf(NAME_DevNet, TEXT("Received ABORT with invalid GUID %s"), *Guid.String());
			}
		}
		else if( ParseCommand( &Text, TEXT("SKIP") ) )
		{
			FGuid Guid(0,0,0,0);
			Parse( Text, TEXT("GUID=" ), Guid );
			if( Connection->PackageMap )
			{
				for( INT i=0;i<Connection->PackageMap->List.Num();i++ )
					if( Connection->PackageMap->List(i).Guid == Guid )
					{
						debugf(NAME_DevNet, TEXT("User skipped download of '%s'"), *Connection->PackageMap->List(i).PackageName.ToString());
						Connection->PackageMap->List.Remove(i);
						break;
					}
			}
		}
		else if( ParseCommand(&Text,TEXT("LOGIN")) )
		{
			// Admit or deny the player here.
			INT Response=0;
			if
			(	!Parse(Text,TEXT("RESPONSE="),Response)
			||	GEngine->ChallengeResponse(Connection->Challenge)!=Response )	
			{
				// Challenge failed, abort right now

				debugf( NAME_DevNet, TEXT("Client %s failed CHALLENGE."), *Connection->LowLevelGetRemoteAddress() );
				Connection->Logf( TEXT("FAILCODE CHALLENGE") );
				Connection->FlushNet();
				Connection->State = USOCK_Closed;
				return;
			}
			TCHAR Str[1024]=TEXT("");
			FString Error, FailCode;
			Parse( Text, TEXT("URL="), Str, ARRAY_COUNT(Str) );
			Connection->RequestURL = Str;
			debugf( NAME_DevNet, TEXT("Login request: %s"), *Connection->RequestURL );
			const TCHAR* Tmp;
			for( Tmp=Str; *Tmp && *Tmp!='?'; Tmp++ );
			GetGameInfo()->eventPreLogin( Tmp, Connection->LowLevelGetRemoteAddress(), Error, FailCode );
			if( Error!=TEXT("") )
			{
				debugf( NAME_DevNet, TEXT("PreLogin failure: %s (%s)"), *Error, *FailCode );
				Connection->Logf( TEXT("FAILURE %s"), *Error );
				if( (*FailCode)[0] )
					Connection->Logf( TEXT("FAILCODE %s"), *FailCode );
				Connection->FlushNet();
				Connection->State = USOCK_Closed;
				return;
			}
			WelcomePlayer( Connection );
		}
		else if( ParseCommand(&Text,TEXT("JOIN")) && !Connection->Actor )
		{
			// verify that the client informed us about all its packages
			/* @fixme: can't do this because if we're in the middle of loading, we could've recently sent some packages and therefore there's no way the client would pass this
			   @fixme: need another way to verify (timelimit?)
			for (INT i = 0; i < Connection->PackageMap->List.Num(); i++)
			{
				if (Connection->PackageMap->List(i).RemoteGeneration == 0)
				{
					FString Error = FString::Printf(TEXT("Join failure: failed to match package '%s'"), *Connection->PackageMap->List(i).PackageName.ToString());
					debugf(NAME_DevNet, *Error);
					Connection->Logf(TEXT("FAILURE %s"), *Error);
					Connection->FlushNet();
					Connection->State = USOCK_Closed;
					return;
				}
			}
			*/

			// Finish computing the package map.
			Connection->PackageMap->Compute();

			// Spawn the player-actor for this network player.
			FString Error;
			debugf( NAME_DevNet, TEXT("Join request: %s"), *Connection->RequestURL );
			Connection->Actor = SpawnPlayActor( Connection, ROLE_AutonomousProxy, FURL(NULL,*Connection->RequestURL,TRAVEL_Absolute), Error );
			if( !Connection->Actor )
			{
				// Failed to connect.
				debugf( NAME_DevNet, TEXT("Join failure: %s"), *Error );
				Connection->Logf( TEXT("FAILURE %s"), *Error );
				Connection->FlushNet();
				Connection->State = USOCK_Closed;
			}
			else
			{
				// Successfully in game.
				debugf( NAME_DevNet, TEXT("Join succeeded: %s"), *Connection->Actor->PlayerReplicationInfo->PlayerName );
				// if we're in the middle of a transition or the client is in the wrong world, tell it to travel
				FString LevelName;
				if (GSeamlessTravelHandler.IsInTransition())
				{
					// tell the client to go to the destination map
					LevelName = GSeamlessTravelHandler.GetDestinationMapName();
				}
				else if (!Connection->Actor->HasClientLoadedCurrentWorld())
				{
					// tell the client to go to our current map
					LevelName = FString(GWorld->GetOutermost()->GetName());
				}
				if (LevelName != TEXT(""))
				{
					Connection->Actor->eventClientTravel(LevelName, TRAVEL_Relative, TRUE);
				}
			}
		}
		// Handle server-side request for spawning a new controller using a child connection.
		else if (ParseCommand(&Text, TEXT("JOINSPLIT")))
		{
			FString SplitRequestURL = Connection->RequestURL;

			// create a child network connection using the existing connection for its parent
			check(Connection->GetUChildConnection() == NULL);
			UChildConnection* ChildConn = NetDriver->CreateChild(Connection);

			// create URL from string
			FURL URL(NULL, *SplitRequestURL, TRAVEL_Absolute);

			FString Error;
			debugf(NAME_DevNet, TEXT("JOINSPLIT: Join request: URL=%s"), *URL.String());
			APlayerController* PC = SpawnPlayActor(ChildConn, ROLE_AutonomousProxy, URL, Error);
			if (PC == NULL)
			{
				// Failed to connect.
				debugf(NAME_DevNet, TEXT("JOINSPLIT: Join failure: %s"), *Error);
				// remove the child connection
				Connection->Children.RemoveItem(ChildConn);
				// if any splitscreen viewport fails to join, all viewports on that client also fail
				Connection->Logf(TEXT("FAILURE %s"), *Error);
				Connection->FlushNet();
				Connection->State = USOCK_Closed;
			}
			else
			{
				PC->NetPlayerIndex = BYTE(Connection->Children.Num());
				// Successfully spawned in game.
				debugf(NAME_DevNet, TEXT("JOINSPLIT: Succeeded: %s"), *ChildConn->Actor->PlayerReplicationInfo->PlayerName);
			}
		}
	}
}

//
// Called when a file receive is about to begin.
//
void UWorld::NotifyReceivedFile( UNetConnection* Connection, INT PackageIndex, const TCHAR* Error, UBOOL Skipped )
{
	appErrorf( TEXT("Level received unexpected file") );
}

//
// Called when other side requests a file.
//
UBOOL UWorld::NotifySendingFile( UNetConnection* Connection, FGuid Guid )
{
	if( NetDriver->ServerConnection )
	{
		// We are the client.
		debugf( NAME_DevNet, TEXT("Server requested file: Refused") );
		return 0;
	}
	else
	{
		// We are the server.
		debugf( NAME_DevNet, TEXT("Client requested file: Allowed") );
		return 1;
	}
}

//
// Start listening for connections.
//
UBOOL UWorld::Listen( FURL InURL, FString& Error)
{
	if( NetDriver )
	{
		Error = LocalizeError(TEXT("NetAlready"),TEXT("Engine"));
		return 0;
	}

	// Create net driver.
	UClass* NetDriverClass = StaticLoadClass( UNetDriver::StaticClass(), NULL, TEXT("engine-ini:Engine.Engine.NetworkDevice"), NULL, LOAD_None, NULL );
	NetDriver = (UNetDriver*)StaticConstructObject( NetDriverClass );
	if( !NetDriver->InitListen( this, InURL, Error ) )
	{
		debugf( TEXT("Failed to listen: %s"), *Error );
		NetDriver = NULL;
		return 0;
	}
	static UBOOL LanPlay = ParseParam(appCmdLine(),TEXT("lanplay"));
	if ( !LanPlay && (NetDriver->MaxInternetClientRate < NetDriver->MaxClientRate) && (NetDriver->MaxInternetClientRate > 2500) )
	{
		NetDriver->MaxClientRate = NetDriver->MaxInternetClientRate;
	}

	if ( GetGameInfo() && GetGameInfo()->MaxPlayers > 16)
	{
		NetDriver->MaxClientRate = ::Min(NetDriver->MaxClientRate, 10000);
	}

	// Load everything required for network server support.
	if( !GUseSeekFreePackageMap )
	{
		BuildServerMasterMap();
	}

	// Spawn network server support.
	GEngine->SpawnServerActors();

	// Set WorldInfo properties.
	GetWorldInfo()->NetMode = GEngine->Client ? NM_ListenServer : NM_DedicatedServer;
	GetWorldInfo()->NextSwitchCountdown = NetDriver->ServerTravelPause;
	debugf(TEXT("NetMode is now %d"),GetWorldInfo()->NetMode);
	return 1;
}

/** @return whether this level is a client */
UBOOL UWorld::IsClient()
{
	return (GEngine->Client != NULL);
}

//
// Return whether this level is a server.
//
UBOOL UWorld::IsServer()
{
	return (!NetDriver || !NetDriver->ServerConnection) && (!DemoRecDriver || !DemoRecDriver->ServerConnection);
}

/**
 * Builds the master package map from the loaded levels, server packages,
 * and game type
 */
void UWorld::BuildServerMasterMap(void)
{
	check(NetDriver);
	NetDriver->MasterMap->AddNetPackages();
	UPackage::NetObjectNotify = NetDriver;
}

/** asynchronously loads the given levels in preparation for a streaming map transition.
 * @param LevelNames the names of the level packages to load. LevelNames[0] will be the new persistent (primary) level
 */
void AWorldInfo::PrepareMapChange(const TArray<FName>& LevelNames)
{
	// Only the game can do async map changes.
	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	if (GameEngine != NULL)
	{
		// Kick off async loading request for those maps.
		if( !GameEngine->PrepareMapChange(LevelNames) )
		{
			debugf(NAME_Warning,TEXT("Preparing map change via %s was not successful: %s"), *GetFullName(), *GameEngine->GetMapChangeFailureDescription() );
		}
	}
}

/** returns whether there's a map change currently in progress */
UBOOL AWorldInfo::IsPreparingMapChange()
{
	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	return (GameEngine != NULL) ? GameEngine->IsPreparingMapChange() : FALSE;
}

/** if there is a map change being prepared, returns whether that change is ready to be committed
 * (if no change is pending, always returns false)
 */
UBOOL AWorldInfo::IsMapChangeReady()
{
	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	return (GameEngine != NULL) ? GameEngine->IsReadyForMapChange() : FALSE;
}

/**
 * Actually performs the map transition prepared by PrepareMapChange().
 *
 * @param bShouldSkipLevelStartupEvent TRUE if this function NOT fire the SeqEvent_LevelBStartup event.
 * @param bShouldSkipLevelBeginningEvent TRUE if this function NOT fire the SeqEvent_LevelBeginning event. Useful for when skipping to the middle of a map by savegame or something
 */
void AWorldInfo::CommitMapChange(UBOOL bShouldSkipLevelStartupEvent, UBOOL bShouldSkipLevelBeginningEvent)
{
	// Only the game can do async map changes.
	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	if( GameEngine != NULL )
	{
		if( IsPreparingMapChange() )
		{
			GameEngine->bShouldCommitPendingMapChange				= TRUE;
			GameEngine->bShouldSkipLevelStartupEventOnMapCommit		= bShouldSkipLevelStartupEvent;
			GameEngine->bShouldSkipLevelBeginningEventOnMapCommit	= bShouldSkipLevelBeginningEvent;
		}
		else
		{
			debugf(TEXT("AWorldInfo::CommitMapChange being called without a pending map change!"));
		}
	}
}

/*-----------------------------------------------------------------------------
	Seamless world traveling
-----------------------------------------------------------------------------*/

FSeamlessTravelHandler GSeamlessTravelHandler;

/** callback sent to async loading code to inform us when the level package is complete */
void FSeamlessTravelHandler::SeamlessTravelLoadCallback(UObject* LevelPackage, void* Handler)
{
	// defer until tick when it's safe to perform the transition
	if (((FSeamlessTravelHandler*)Handler)->IsInTransition())
	{
		((FSeamlessTravelHandler*)Handler)->LoadedPackage = LevelPackage;
	}
}

UBOOL FSeamlessTravelHandler::StartTravel(const FURL& InURL)
{
	debugf(TEXT("SeamlessTravel to: %s"), *InURL.Map);
	FString FileName;
	if (!GPackageFileCache->FindPackageFile(*InURL.Map, NULL, FileName))
	{
		debugf(NAME_Error, TEXT("Unable to travel to '%s' - file not found"), *InURL.Map);
		return FALSE;
		// @todo: might have to handle this more gracefully to handle downloading (might also need to send GUID and check it here!)
	}
	else if (IsInTransition())
	{
		debugf(NAME_Error, TEXT("Unable to travel to '%s' because a travel to '%s' is already in progress"), *InURL.Map, *PendingTravelURL.Map);
		return FALSE;
	}
	else
	{
		PendingTravelURL = InURL;
		bSwitchedToDefaultMap = FALSE;
		LoadedPackage = NULL;
		bTransitionInProgress = TRUE;

		FString DefaultMapName = FFilename(FURL::DefaultTransitionMap).GetBaseFilename();
		// if we're already in the default map, skip loading it and just go to the destination
		if (FName(*DefaultMapName) == GWorld->GetOutermost()->GetFName())
		{
			debugf(TEXT("Already in default map, continuing to destination"));
			bSwitchedToDefaultMap = TRUE;
			UObject::LoadPackageAsync(PendingTravelURL.Map, &SeamlessTravelLoadCallback, this);
		}
		else
		{
			// first, load the entry level package
			UObject::LoadPackageAsync(DefaultMapName, &SeamlessTravelLoadCallback, this);
		}

		return TRUE;
	}
}

/** cancels transition in progress */
void FSeamlessTravelHandler::CancelTravel()
{
	LoadedPackage = NULL;
	bTransitionInProgress = FALSE;
}

void FSeamlessTravelHandler::Tick()
{
	if( IsInTransition() )
	{
		// allocate more time for async loading during transition
		UObject::ProcessAsyncLoading(TRUE, 0.003f);
	}
	if (LoadedPackage != NULL)
	{
		// find the new world
		UWorld* NewWorld = (UWorld*) UObject::StaticFindObjectFast( UWorld::StaticClass(), LoadedPackage, NAME_TheWorld );
		if (NewWorld == NULL || NewWorld->PersistentLevel == NULL)
		{
			debugf(NAME_Error, TEXT("Unable to travel to '%s' - package is not a level"), *LoadedPackage->GetName());
			// abort
			LoadedPackage = NULL;
			bTransitionInProgress = FALSE;
		}
		else
		{
			// Dump and reset the FPS chart on map changes.
			GEngine->DumpFPSChart();
			GEngine->ResetFPSChart();

			// Dump and reset the Memory chart on map changes.
			GEngine->DumpMemoryChart();
			GEngine->ResetMemoryChart();

			AWorldInfo* OldWorldInfo = GWorld->GetWorldInfo();
			AWorldInfo* NewWorldInfo = NewWorld->GetWorldInfo();

			// clean up the old world
			for (INT i = 0; i < GWorld->PersistentLevel->GameSequences.Num(); i++)
			{
				// note that sequences for streaming levels are attached to the persistent level's sequence in AddToWorld(), so this should get everything
				USequence* Seq = GWorld->PersistentLevel->GameSequences(i);
				if (Seq != NULL)
				{
					Seq->CleanUp();
				}
			}
			// Make sure there are no pending visiblity requests.
			GWorld->FlushLevelStreaming( NULL, TRUE );
			GWorld->TermWorldRBPhys();
			// only consider session ended if we're making the final switch so that HUD, scoreboard, etc. UI elements stay around until the end
			GWorld->CleanupWorld(bSwitchedToDefaultMap); 
			GWorld->RemoveFromRoot();
			// Stop all audio to remove references to old world
			if (GEngine->Client != NULL && GEngine->Client->GetAudioDevice() != NULL)
			{
				GEngine->Client->GetAudioDevice()->Flush();
			}

			// mark actors we want to keep

			// keep GameInfo if traveling to entry
			if (!bSwitchedToDefaultMap && OldWorldInfo->Game != NULL)
			{
				OldWorldInfo->Game->SetFlags(RF_Marked);
			}
			// always keep Controllers that belong to players
			if (GWorld->GetNetMode() == NM_Client)
			{
				for (FPlayerIterator It(GEngine); It; ++It)
				{
					if (It->Actor != NULL)
					{
						It->Actor->SetFlags(RF_Marked);
					}
				}
			}
			else
			{
				for (AController* C = OldWorldInfo->ControllerList; C != NULL; C = C->NextController)
				{
					if (C->bIsPlayer || C->GetAPlayerController() != NULL)
					{
						C->SetFlags(RF_Marked);
					}
				}
			}

			// ask the game class what else we should keep
			TArray<AActor*> KeepActors;
			if (OldWorldInfo->Game != NULL)
			{
				OldWorldInfo->Game->eventGetSeamlessTravelActorList(!bSwitchedToDefaultMap, KeepActors);
			}
			// ask players what else we should keep
			for (FPlayerIterator It(GEngine); It; ++It)
			{
				if (It->Actor != NULL)
				{
					It->Actor->eventGetSeamlessTravelActorList(!bSwitchedToDefaultMap, KeepActors);
				}
			}
			// mark all valid actors specified
			for (INT i = 0; i < KeepActors.Num(); i++)
			{
				if (KeepActors(i) != NULL)
				{
					KeepActors(i)->SetFlags(RF_Marked);
				}
			}

			// rename dynamic actors in the old world's PersistentLevel that we want to keep into the new world
			for (FActorIterator It; It; ++It)
			{
				// keep if it's dynamic and has been marked or we don't control it
				if (It->GetLevel() == GWorld->PersistentLevel && !It->bStatic && !It->bNoDelete && (It->HasAnyFlags(RF_Marked) || It->Role < ROLE_Authority))
				{
					It->ClearFlags(RF_Marked);
					It->Rename(NULL, NewWorld->PersistentLevel);
					It->WorldInfo = NewWorldInfo;
					// if it's a Controller or a Pawn, add it to the appopriate list in the new world's WorldInfo
					if (It->GetAController())
					{
						NewWorld->AddController((AController*)*It);
					}
					else if (It->GetAPawn())
					{
						NewWorld->AddPawn((APawn*)*It);
					}
					// add to new world's actor list and remove from old
					NewWorld->PersistentLevel->Actors.AddItem(*It);
					It.ClearCurrent();
				}
				else
				{
					// otherwise, set to be deleted
					It->ClearFlags(RF_Marked);
					It->MarkPendingKill();
					It->MarkComponentsAsPendingKill();
					// close any channels for this actor
					if (GWorld->NetDriver != NULL)
					{
						// server
						GWorld->NetDriver->NotifyActorDestroyed(*It);
						// client
						if (GWorld->NetDriver->ServerConnection != NULL)
						{
							// we can't kill the channel until the server says so, so just clear the actor ref and break the channel
							UActorChannel* Channel = GWorld->NetDriver->ServerConnection->ActorChannels.FindRef(*It);
							if (Channel != NULL)
							{
								GWorld->NetDriver->ServerConnection->ActorChannels.Remove(*It);
								Channel->Actor = NULL;
								Channel->Broken = TRUE;
							}
						}
					}
				}
			}

			// mark everything else contained in the world to be deleted
			for (TObjectIterator<UObject> It; It; ++It)
			{
				if (It->IsIn(GWorld))
				{
					It->MarkPendingKill();
				}
			}

			// copy over other info
			NewWorld->NetDriver = GWorld->NetDriver;
			if (NewWorld->NetDriver != NULL)
			{
				NewWorld->NetDriver->Notify = NewWorld;
			}
			NewWorldInfo->NetMode = GWorld->GetNetMode();
			NewWorldInfo->Game = OldWorldInfo->Game;
			NewWorldInfo->GRI = OldWorldInfo->GRI;
			NewWorldInfo->TimeSeconds = OldWorldInfo->TimeSeconds;
			NewWorldInfo->RealTimeSeconds = OldWorldInfo->RealTimeSeconds;

			// the new world should not be garbage collected
			NewWorld->AddToRoot();
			// set GWorld to the new world and initialize it
			GWorld = NewWorld;
			GWorld->Init();

			// collect garbage to delete the old world
			// because we marked everything in it pending kill, references will be NULL'ed so we shouldn't end up with any dangling pointers
			UObject::CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS, TRUE );

#if !FINAL_RELEASE
			// verify that we successfully cleaned up the old world
			for (TObjectIterator<UWorld> It; It; ++It)
			{
				UWorld* World = *It;
				if (World != NewWorld)
				{
					// Print some debug information...
					UObject::StaticExec(*FString::Printf(TEXT("OBJ REFS CLASS=WORLD NAME=%s.TheWorld"), *World->GetOutermost()->GetName()));
					TMap<UObject*,UProperty*>	Route		= FArchiveTraceRoute::FindShortestRootPath( World, TRUE, GARBAGE_COLLECTION_KEEPFLAGS );
					FString						ErrorString	= FArchiveTraceRoute::PrintRootPath( Route, World );
					debugf(TEXT("%s"),*ErrorString);
					// before asserting.
					appErrorf( TEXT("%s not cleaned up by garbage collection!") LINE_TERMINATOR TEXT("%s") , *World->GetFullName(), *ErrorString );
				}
			}
#endif

			// GEMINI_TODO: A nicer precaching scheme.
			GPrecacheNextFrame = TRUE;

			// if we've already switched to entry before and this is the transition to the new map, re-create the gameinfo
			if (bSwitchedToDefaultMap && GWorld->GetNetMode() != NM_Client)
			{
				GWorld->SetGameInfo(PendingTravelURL);
			}

			// call begin play functions on everything that wasn't carried over from the old world
			GWorld->BeginPlay(PendingTravelURL, FALSE);

			// send loading complete notifications for all local players
			for (FPlayerIterator It(GEngine); It; ++It)
			{
				if (It->Actor != NULL)
				{
					It->Actor->eventNotifyLoadedWorld(GWorld->GetOutermost()->GetFName());
					It->Actor->eventServerNotifyLoadedWorld(GWorld->GetOutermost()->GetFName());
				}
			}

			// we've finished the transition
			LoadedPackage = NULL;
			if (bSwitchedToDefaultMap)
			{
				// we've now switched to the final destination, so we're done
				UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
				if (GameEngine != NULL)
				{
					// remember the last used URL
					GameEngine->LastURL = PendingTravelURL;
				}

				// Flag our transition as completed before we call PostSeamlessTravel.  This 
				// allows for chaining of maps.

				bTransitionInProgress = FALSE;

				if (NewWorldInfo->Game != NULL)
				{
					// inform the new GameInfo so it can handle players that persisted
					NewWorldInfo->Game->eventPostSeamlessTravel();
				}
			}
			else
			{
				bSwitchedToDefaultMap = TRUE;
				// now load the desired map
				UObject::LoadPackageAsync(PendingTravelURL.Map, SeamlessTravelLoadCallback, this);
			}
		}
	}
}

/** seamlessly travels to the given URL by first loading the entry level in the background,
 * switching to it, and then loading the specified level. Does not disrupt network communication or disconnet clients.
 * You may need to implement GameInfo::GetSeamlessTravelActorList(), PlayerController::GetSeamlessTravelActorList(),
 * GameInfo::PostSeamlessTravel(), and/or GameInfo::HandleSeamlessTravelPlayer() to handle preserving any information
 * that should be maintained (player teams, etc)
 * This codepath is designed for worlds that use little or no level streaming and gametypes where the game state
 * is reset/reloaded when transitioning. (like UT)
 * @param URL the URL to travel to; must be relative to the current URL (same server)
 */
void AWorldInfo::SeamlessTravel(const FString& URL)
{
	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	if (GameEngine != NULL)
	{
		// construct the URL
		FURL NewURL(&GameEngine->LastURL, *URL, TRAVEL_Relative);
		if (NewURL.HasOption(TEXT("Restart")))
		{
			NewURL = GameEngine->LastURL;
		}
		// tell the handler to start the transition
		GSeamlessTravelHandler.StartTravel(NewURL);
	}
}

/** @return whether we're currently in a seamless transition */
UBOOL AWorldInfo::IsInSeamlessTravel()
{
	return GSeamlessTravelHandler.IsInTransition();
}

/**
 * Enables/disables the standby cheat detection code
 *
 * @param bShouldEnable true to enable checking, false to disable it
 * @param StandbyTimeout the timeout to use
 */
void AWorldInfo::SetStandbyCheatDetection(UBOOL bShouldEnable,FLOAT StandbyTimeout)
{
	if (GWorld->NetDriver)
	{
		GWorld->NetDriver->bIsStandbyCheckingEnabled = bShouldEnable;
		GWorld->NetDriver->StandbyCheatTime = StandbyTimeout;
	}
}

/**
 * @return the name of the map.  If Title isn't assigned, it will attempt to create the name from the full name
 */

FString AWorldInfo::GetMapName()
{
	if ( Title != TEXT("") )
	{
		return Title;
	}
	else	// Make a default
	{

		FString MapName = GWorld->GetMapName();
		INT pos = MapName.InStr(TEXT("-"));
		if (pos>=0)
		{
			MapName = MapName.Right(MapName.Len() - pos - 1);
		}

		MapName = MapName.ToUpper();
		return MapName;
	}

}

BYTE AWorldInfo::GetDetailMode()
{
	return EDetailMode(GSystemSettings.DetailMode);
}

/** @return the current MapInfo that should be used. May return one of the inner StreamingLevels' MapInfo if a streaming level
 *	transition has occurred via PrepareMapChange()
 */
UMapInfo* AWorldInfo::GetMapInfo()
{
	AWorldInfo* CurrentWorldInfo = this;
	if ( StreamingLevels.Num() > 0 &&
		StreamingLevels(0)->LoadedLevel && 
		Cast<ULevelStreamingPersistent>(StreamingLevels(0)) )
	{
		CurrentWorldInfo = StreamingLevels(0)->LoadedLevel->GetWorldInfo();
	}
	return CurrentWorldInfo->MyMapInfo;
}

/** sets the current MapInfo to the passed in one */
void AWorldInfo::SetMapInfo(UMapInfo* NewMapInfo)
{
	AWorldInfo* CurrentWorldInfo = this;
	if ( StreamingLevels.Num() > 0 &&
		StreamingLevels(0)->LoadedLevel && 
		Cast<ULevelStreamingPersistent>(StreamingLevels(0)) )
	{
		CurrentWorldInfo = StreamingLevels(0)->LoadedLevel->GetWorldInfo();
	}
	CurrentWorldInfo->MyMapInfo = NewMapInfo;
}

/*-----------------------------------------------------------------------------
	ULevelStreaming* implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(ULevelStreaming);
IMPLEMENT_CLASS(ULevelStreamingKismet);
IMPLEMENT_CLASS(ULevelStreamingDistance);
IMPLEMENT_CLASS(ULevelStreamingPersistent);

/**
 * Returns whether this level should be present in memory which in turn tells the 
 * streaming code to stream it in. Please note that a change in value from FALSE 
 * to TRUE only tells the streaming code that it needs to START streaming it in 
 * so the code needs to return TRUE an appropriate amount of time before it is 
 * needed.
 *
 * @param ViewLocation	Location of the viewer
 * @return TRUE if level should be loaded/ streamed in, FALSE otherwise
 */
UBOOL ULevelStreaming::ShouldBeLoaded( const FVector& ViewLocation )
{
	return TRUE;
}

/**
 * Returns whether this level should be visible/ associated with the world if it
 * is loaded.
 * 
 * @param ViewLocation	Location of the viewer
 * @return TRUE if the level should be visible, FALSE otherwise
 */
UBOOL ULevelStreaming::ShouldBeVisible( const FVector& ViewLocation )
{
	if( GIsGame )
	{
		// Game and play in editor viewport codepath.
		return ShouldBeLoaded( ViewLocation );
	}
	else
	{
		// Editor viewport codepath.
		return bShouldBeVisibleInEditor;
	}
}

void ULevelStreaming::PostEditChange(FEditPropertyChain& PropertyThatChanged)
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* OutermostProperty = PropertyThatChanged.GetHead()->GetValue();
		if ( OutermostProperty != NULL )
		{
			const FName PropertyName = OutermostProperty->GetFName();
			if ( PropertyName == TEXT("Offset") )
			{
				GWorld->UpdateLevelStreaming();
			}
		}
	}

	Super::PostEditChange( PropertyThatChanged );
}


/**
 * Returns whether this level should be visible/ associated with the world if it
 * is loaded.
 * 
 * @param ViewLocation	Location of the viewer
 * @return TRUE if the level should be visible, FALSE otherwise
 */
UBOOL ULevelStreamingKismet::ShouldBeVisible( const FVector& ViewLocation )
{
	return bShouldBeVisible || (bShouldBeVisibleInEditor && !GIsGame);
}

/**
 * Returns whether this level should be present in memory which in turn tells the 
 * streaming code to stream it in. Please note that a change in value from FALSE 
 * to TRUE only tells the streaming code that it needs to START streaming it in 
 * so the code needs to return TRUE an appropriate amount of time before it is 
 * needed.
 *
 * @param ViewLocation	Location of the viewer
 * @return TRUE if level should be loaded/ streamed in, FALSE otherwise
 */
UBOOL ULevelStreamingKismet::ShouldBeLoaded( const FVector& ViewLocation )
{
	return bShouldBeLoaded;
}


/**
 * Returns whether this level should be present in memory which in turn tells the 
 * streaming code to stream it in. Please note that a change in value from FALSE 
 * to TRUE only tells the streaming code that it needs to START streaming it in 
 * so the code needs to return TRUE an appropriate amount of time before it is 
 * needed.
 *
 * @param ViewLocation	Location of the viewer
 * @return TRUE if level should be loaded/ streamed in, FALSE otherwise
 */
UBOOL ULevelStreamingDistance::ShouldBeLoaded( const FVector& ViewLocation )
{
	return FDist( ViewLocation, Origin ) <= MaxDistance;
}

