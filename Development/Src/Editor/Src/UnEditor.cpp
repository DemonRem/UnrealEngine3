/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "Factories.h"
#include "BSPOps.h"

// needed for the RemotePropagator
#include "UnIpDrv.h"
#include "EnginePrefabClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineUISequenceClasses.h"
#include "EngineDecalClasses.h"
#include "EngineAudioDeviceClasses.h"
#include "EngineSoundNodeClasses.h"

#include "UnAudioCompress.h"

// For WAVEFORMATEXTENSIBLE
#pragma pack(push,8)
#include <mmreg.h>
#pragma pack(pop)

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

UEditorEngine*	GEditor;

static inline USelection*& PrivateGetSelectedActors()
{
	static USelection* SSelectedActors = NULL;
	return SSelectedActors;
};

static inline USelection*& PrivateGetSelectedObjects()
{
	static USelection* SSelectedObjects = NULL;
	return SSelectedObjects;
};

static void PrivateInitSelectedSets()
{
	PrivateGetSelectedActors() = new( UObject::GetTransientPackage(), TEXT("SelectedActors"), RF_Transactional ) USelection;
	PrivateGetSelectedActors()->AddToRoot();

	PrivateGetSelectedObjects() = new( UObject::GetTransientPackage(), TEXT("SelectedObjects"), RF_Transactional ) USelection;
	PrivateGetSelectedObjects()->AddToRoot();
}

static void PrivateDestroySelectedSets()
{
#if 0
	PrivateGetSelectedActors()->RemoveFromRoot();
	PrivateGetSelectedActors() = NULL;
	PrivateGetSelectedObjects()->RemoveFromRoot();
	PrivateGetSelectedObjects() = NULL;
#endif
}

/*-----------------------------------------------------------------------------
	UEditorEngine.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UEditorEngine);
IMPLEMENT_CLASS(ULightingChannelsObject);

/**
 * Returns the set of selected actors.
 */
USelection* UEditorEngine::GetSelectedActors() const
{
	return PrivateGetSelectedActors();
}

/**
 * Returns an FSelectionIterator that iterates over the set of selected actors.
 */
FSelectionIterator UEditorEngine::GetSelectedActorIterator() const
{
	return FSelectionIterator( *GetSelectedActors() );
};

/**
 * Returns the set of selected non-actor objects.
 */
USelection* UEditorEngine::GetSelectedObjects() const
{
	return PrivateGetSelectedObjects();
}

/**
 * Returns the appropriate selection set for the specified object class.
 */
USelection* UEditorEngine::GetSelectedSet( const UClass* Class ) const
{
	if ( Class->IsChildOf( AActor::StaticClass() ) )
	{
		return GetSelectedActors();
	}
	else
	{
		return GetSelectedObjects();
	}
}

/*-----------------------------------------------------------------------------
	Init & Exit.
-----------------------------------------------------------------------------*/

//
// Construct the UEditorEngine class.
//
void UEditorEngine::StaticConstructor()
{
}

//
// Editor early startup.
//
void UEditorEngine::InitEditor()
{
	// Call base.
	UEngine::Init();

	// Create selection sets.
	if( !GIsUCCMake )
	{
		PrivateInitSelectedSets();
	}

	// Make sure properties match up.
	VERIFY_CLASS_OFFSET(A,Actor,Owner);

	// Allocate temporary model.
	TempModel = new UModel( NULL, 1 );

	// Verify a sane value for camera speed
	if (MovementSpeed != MOVEMENTSPEED_SLOW &&
		MovementSpeed != MOVEMENTSPEED_NORMAL &&
		MovementSpeed != MOVEMENTSPEED_FAST)
	{
		// Reset to normal
		MovementSpeed = MOVEMENTSPEED_NORMAL;
	}
	// Settings.
	FBSPOps::GFastRebuild	= 0;
	Bootstrapping			= 0;
}

UBOOL UEditorEngine::ShouldDrawBrushWireframe( AActor* InActor )
{
	UBOOL bResult = TRUE;

	if(GEditorModeTools().GetCurrentMode())
	{
		bResult = GEditorModeTools().GetCurrentMode()->ShouldDrawBrushWireframe( InActor );
	}

	return bResult;
}

// Used for sorting ActorFactory classes.
IMPLEMENT_COMPARE_POINTER( UActorFactory, UnEditor, { return B->MenuPriority - A->MenuPriority; } )

//
// Init the editor.
//
void UEditorEngine::Init()
{
	check(!HasAnyFlags(RF_ClassDefaultObject));

	// Init editor.
	GEditor = this;
	InitEditor();

	// Init transactioning.
	Trans = CreateTrans();

	// Load classes for editing.
	BeginLoad();
	for( INT i=0; i<EditPackages.Num(); i++ )
	{
		if( !LoadPackage( NULL, *EditPackages(i), LOAD_NoWarn ) )
		{
			debugf( *LocalizeUnrealEd("Error_CantFindEditPackage"), *EditPackages(i) );
		}
	}
	EndLoad();

	// Init the client.
	UClass* ClientClass = StaticLoadClass( UClient::StaticClass(), NULL, TEXT("engine-ini:Editor.EditorEngine.Client"), NULL, LOAD_None, NULL );
	Client = (UClient*)StaticConstructObject( ClientClass );
	Client->Init( this );
	check(Client);

	// Objects.
	Results  = new( GetTransientPackage(), TEXT("Results") )UTextBuffer;

	// Create array of ActorFactory instances.
	for(TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Cls = *It;
		UObject* DefaultObject = Cls->GetDefaultObject();

		if(  Cls->IsChildOf(UActorFactory::StaticClass()) &&
		   !(Cls->ClassFlags & CLASS_Abstract) &&
		    (DefaultObject != NULL && ((UActorFactory*)DefaultObject)->bPlaceable) &&
			(appStricmp( *((UActorFactory*)DefaultObject)->SpecificGameName, TEXT("") ) == 0 ||
			 appStricmp( *((UActorFactory*)DefaultObject)->SpecificGameName, appGetGameName() ) == 0) )
		{
			UActorFactory* NewFactory = ConstructObject<UActorFactory>( Cls );
			ActorFactories.AddItem(NewFactory);
		}
	}

	// Sort by menu priority.
	Sort<USE_COMPARE_POINTER(UActorFactory,UnEditor)>( &ActorFactories(0), ActorFactories.Num() );

	// Purge garbage.
	Cleanse( FALSE, 0, TEXT("startup") );

	// Subsystem init messsage.
	debugf( NAME_Init, TEXT("Editor engine initialized") );

	// create the possible propagators
	InEditorPropagator = new FEdObjectPropagator;
	RemotePropagator = new FRemotePropagator;
};

/**
 * Constructs a default cube builder brush, this function MUST be called at the AFTER UEditorEngine::Init in order to guarantee builder brush and other required subsystems exist.
 */
void UEditorEngine::InitBuilderBrush()
{
	const UBOOL bOldDirtyState = GWorld->CurrentLevel->GetOutermost()->IsDirty();

	// For additive geometry mode, make the builder brush a small 256x256x256 cube so its visible.
	const INT CubeSize = 256;
	UCubeBuilder* CubeBuilder = ConstructObject<UCubeBuilder>( UCubeBuilder::StaticClass() );
	CubeBuilder->X = CubeSize;
	CubeBuilder->Y = CubeSize;
	CubeBuilder->Z = CubeSize;
	CubeBuilder->eventBuild();

	// Restore the level's dirty state, so that setting a builder brush won't mark the map as dirty.
	GWorld->CurrentLevel->MarkPackageDirty( bOldDirtyState );
}

void UEditorEngine::FinishDestroy()
{
	if ( !HasAnyFlags(RF_ClassDefaultObject) )
	{
		// this needs to be already cleaned up
		check(PlayWorld == NULL);

		// free the propagators
		delete InEditorPropagator;
		delete RemotePropagator;

		// GWorld == NULL if we're compiling script.
		if( !GIsUCCMake )
		{
			ClearComponents();
			GWorld->CleanupWorld();
		}

		// Shut down transaction tracking system.
		if( Trans )
		{
			if( GUndo )
			{
				debugf( NAME_Warning, TEXT("Warning: A transaction is active") );
			}
			Trans->Reset( TEXT("shutdown") );
		}

		// Destroy selection sets.
		PrivateDestroySelectedSets();

		// Remove editor array from root.
		debugf( NAME_Exit, TEXT("Editor shut down") );
	}

	Super::FinishDestroy();
}
void UEditorEngine::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);
	if(!Ar.IsLoading() && !Ar.IsSaving())
	{
		// Serialize viewport clients.

		for(UINT ViewportIndex = 0;ViewportIndex < (UINT)ViewportClients.Num();ViewportIndex++)
			Ar << *ViewportClients(ViewportIndex);

		// Serialize ActorFactories
		Ar << ActorFactories;

		// Serialize components used in UnrealEd modes

		GEditorModeTools().Serialize( Ar );
	}
}

/*-----------------------------------------------------------------------------
	Tick.
-----------------------------------------------------------------------------*/

//
// Time passes...
//
void UEditorEngine::Tick( FLOAT DeltaSeconds )
{
	check( GWorld );
	check( GWorld != PlayWorld );

	// Tick client code.
	if( Client )
	{
		Client->Tick(DeltaSeconds);
	}

	// Clean up the game viewports that have been closed.
	CleanupGameViewport();

	// If all viewports closed, close the current play level.
	if( GameViewport == NULL && PlayWorld )
	{
		EndPlayMap();
	}

	// Update subsystems.
	{
		// This assumes that UObject::StaticTick only calls ProcessAsyncLoading.	
		UObject::StaticTick( DeltaSeconds );
	}

	// Look for realtime flags.
	UBOOL IsRealtime = FALSE;
	UBOOL IsAudioRealTime = FALSE;
	for( INT i = 0; i < ViewportClients.Num(); i++ )
	{
		if( ViewportClients( i )->GetScene() == GWorld->Scene )
		{
			if( ViewportClients( i )->IsRealtime() )
			{
				IsRealtime = TRUE;
				IsAudioRealTime = TRUE;
			}
		}

		if( ViewportClients( i )->bAudioRealTime )
		{
			IsAudioRealTime = TRUE;
		}
	}

	// Tick level.
	GWorld->Tick( IsRealtime ? LEVELTICK_ViewportsOnly : LEVELTICK_TimeOnly, DeltaSeconds );

	// Perform editor level streaming previs if no PIE session is currently in progress.
	if( !PlayWorld )
	{
		FEditorLevelViewportClient* PerspectiveViewportClient = NULL;
		for ( INT i = 0 ; i < ViewportClients.Num() ; ++i )
		{
			FEditorLevelViewportClient* ViewportClient = ViewportClients(i);

			// Previs level streaming volumes in the Editor.
			if ( ViewportClient->bLevelStreamingVolumePrevis )
			{
				UBOOL bProcessViewer = FALSE;
				const FVector& ViewLocation = ViewportClient->ViewLocation;

				// Iterate over streaming levels and compute whether the ViewLocation is in their associated volumes.
				TMap<ALevelStreamingVolume*, UBOOL> VolumeMap;

				AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
				for( INT LevelIndex = 0 ; LevelIndex < WorldInfo->StreamingLevels.Num() ; ++LevelIndex )
				{
					ULevelStreaming* StreamingLevel = WorldInfo->StreamingLevels(LevelIndex);
					if( StreamingLevel )
					{
						// Assume the streaming level is invisible until we find otherwise.
						UBOOL bStreamingLevelShouldBeVisible = FALSE;

						// We're not going to change level visibility unless we encounter at least one
						// volume associated with the level.
						UBOOL bFoundValidVolume = FALSE;

						// For each streaming volume associated with this level . . .
						for ( INT i = 0 ; i < StreamingLevel->EditorStreamingVolumes.Num() ; ++i )
						{
							ALevelStreamingVolume* StreamingVolume = StreamingLevel->EditorStreamingVolumes(i);
							if ( StreamingVolume && !StreamingVolume->bDisabled )
							{
								bFoundValidVolume = TRUE;

								UBOOL bViewpointInVolume;
								UBOOL* bResult = VolumeMap.Find(StreamingVolume);
								if ( bResult )
								{
									// This volume has already been considered for another level.
									bViewpointInVolume = *bResult;
								}
								else
								{
									// Compute whether the viewpoint is inside the volume and cache the result.
									bViewpointInVolume = StreamingVolume->Encompasses( ViewLocation );
									VolumeMap.Set( StreamingVolume, bViewpointInVolume );
								}

								// Halt when we find a volume associated with the level that the viewpoint is in.
								if ( bViewpointInVolume )
								{
									bStreamingLevelShouldBeVisible = TRUE;
									break;
								}
							}
						}

						// Set the streaming level visibility status if we encountered at least one volume.
						if ( bFoundValidVolume && StreamingLevel->bShouldBeVisibleInEditor != bStreamingLevelShouldBeVisible )
						{
							StreamingLevel->bShouldBeVisibleInEditor = bStreamingLevelShouldBeVisible;
							bProcessViewer = TRUE;
						}
					}
				}

				// Call UpdateLevelStreaming if the visibility of any streaming levels was modified.
				if ( bProcessViewer )
				{
					GWorld->UpdateLevelStreaming();
					GCallbackEvent->Send( CALLBACK_RefreshEditor_LevelBrowser );
				}
				break;
			}
		}
	}

	// kick off a "Play From Here" if we got one
	if (bIsPlayWorldQueued)
	{
		StartQueuedPlayMapRequest();
	}

	// if we have the side-by-side world for "Play From Here", tick it!
	if(PlayWorld)
	{
		// Use the PlayWorld as the GWorld, because who knows what will happen in the Tick.
		UWorld* OldGWorld = SetPlayInEditorWorld( PlayWorld );

		// Release mouse if the game is paused. The low level input code might ignore the request when e.g. in fullscreen mode.
		if ( GameViewport != NULL && GameViewport->Viewport != NULL )
		{
			GameViewport->Viewport->CaptureMouseInput( !PlayWorld->IsPaused() );
		}

		// Update the level.
		GameCycles=0;
		clock(GameCycles);

		// Decide whether to drop high detail because of frame rate
		if ( Client )
		{
			PlayWorld->GetWorldInfo()->bDropDetail = (DeltaSeconds > 1.f/Clamp(Client->MinDesiredFrameRate,1.f,100.f)) && !GIsBenchmarking;
			PlayWorld->GetWorldInfo()->bAggressiveLOD = (DeltaSeconds > 1.f/Clamp(Client->MinDesiredFrameRate - 5.f,1.f,100.f)) && !GIsBenchmarking;
		}

		{
			// So that hierarchical stats work in PIE
			SCOPE_CYCLE_COUNTER(STAT_FrameTime);
			// tick the level
			PlayWorld->Tick( LEVELTICK_All, DeltaSeconds );
		}

		unclock(GameCycles);

		// Tick the viewports.
		if ( GameViewport != NULL )
		{
			GameViewport->Tick(DeltaSeconds);
		}

//#if defined(WITH_FACEFX) && defined(FX_TRACK_MEMORY_STATS)
#if OLD_STATS
		if( GFaceFXStats.Enabled )
		{
			GFaceFXStats.NumCurrentAllocations.Value  += OC3Ent::Face::FxGetCurrentNumAllocations();
			GFaceFXStats.CurrentAllocationsSize.Value += OC3Ent::Face::FxGetCurrentBytesAllocated() / 1024.0f;
			GFaceFXStats.PeakAllocationsSize.Value    += OC3Ent::Face::FxGetMaxBytesAllocated() / 1024.0f;
		}
#endif // WITH_FACEFX && FX_TRACK_MEMORY_STATS

		// Pop the world
		RestoreEditorWorld( OldGWorld );
	}

	// Clean up any game viewports that may have been closed during the level tick (eg by Kismet).
	CleanupGameViewport();

	// If all viewports closed, close the current play level.
	if( GameViewport == NULL && PlayWorld )
	{
		EndPlayMap();
	}

	// Handle decal update requests.
	if ( bDecalUpdateRequested )
	{
		bDecalUpdateRequested = FALSE;
		for( FActorIterator It; It; ++It )
		{
			ADecalActor* DecalActor = Cast<ADecalActor>( *It );
			if ( DecalActor && DecalActor->Decal )
			{
				FComponentReattachContext ReattachContext( DecalActor->Decal );
			}
		}
	}

	// Update viewports.
	for(INT ViewportIndex = 0;ViewportIndex < ViewportClients.Num();ViewportIndex++)
	{
		ViewportClients(ViewportIndex)->Tick(DeltaSeconds);
	}

	// Commit changes to the BSP model.
	GWorld->CommitModelSurfaces();

	/////////////////////////////
	// Redraw viewports.

	// Render view parents, then view children.
	for(UBOOL bRenderingChildren = 0;bRenderingChildren < 2;bRenderingChildren++)
	{
		for(INT ViewportIndex=0; ViewportIndex<ViewportClients.Num(); ViewportIndex++ )
		{
			FEditorLevelViewportClient* ViewportClient = ViewportClients(ViewportIndex);
			const UBOOL bIsViewParent = ViewportClient->ViewState->IsViewParent();
			if(	(bRenderingChildren && !bIsViewParent) ||
				(!bRenderingChildren && bIsViewParent))
			{
				// Add view information for perspective viewports.
				if( ViewportClient->ViewportType == LVT_Perspective )
				{
					GStreamingManager->AddViewInformation( ViewportClient->ViewLocation, ViewportClient->Viewport->GetSizeX(), ViewportClient->Viewport->GetSizeX() / appTan(ViewportClient->ViewFOV) );
				}
				// Redraw the viewport if it's realtime.
				if( ViewportClient->IsRealtime() )
				{
					ViewportClient->Viewport->Draw();
				}
				// Redraw the viewport if there are pending redraw.
				if(ViewportClient->NumPendingRedraws > 0)
				{
					ViewportClient->Viewport->Draw();
					ViewportClient->NumPendingRedraws--;
				}
			}
		}
	}

	// Render playworld. This needs to happen after the other viewports for screenshots to work correctly in PIE.
	if(PlayWorld)
	{
		// Use the PlayWorld as the GWorld, because who knows what will happen in the Tick.
		UWorld* OldGWorld = SetPlayInEditorWorld( PlayWorld );
		
		// Render everything.
		if ( GameViewport != NULL )
		{
			GameViewport->eventLayoutPlayers();
			check(GameViewport->Viewport);
			GameViewport->Viewport->Draw();
		}

		// Pop the world
		RestoreEditorWorld( OldGWorld );
	}

	// Update resource streaming after both regular Editor viewports and PIE had a chance to add viewers.
	GStreamingManager->UpdateResourceStreaming( DeltaSeconds );

	// Update Audio. This needs to occur after rendering as the rendering code updates the listener position.
	if( Client && Client->GetAudioDevice() )
	{
		UWorld* OldGWorld = NULL;
		if( PlayWorld )
		{
			// Use the PlayWorld as the GWorld if we're using PIE.
			OldGWorld = SetPlayInEditorWorld( PlayWorld );
		}

		// Update audio device.
		Client->GetAudioDevice()->Update( IsAudioRealTime || ( PlayWorld && !PlayWorld->IsPaused() ) );

		if( PlayWorld )
		{
			// Pop the world.
			RestoreEditorWorld( OldGWorld );
		}
	}

	if( GScriptCallGraph )
	{
		GScriptCallGraph->Tick();
	}
}

/**
* Callback for when a editor property changed.
*
* @param	PropertyThatChanged	Property that changed and initiated the callback.
*/
void UEditorEngine::PostEditChange(UProperty* PropertyThatChanged)
{
	if( PropertyThatChanged )
	{
		// Check to see if the FOVAngle property changed, if so loop through all viewports and update their fovs.
		const UBOOL bFOVAngleChanged = (appStricmp( *PropertyThatChanged->GetName(), TEXT("FOVAngle") ) == 0) ? TRUE : FALSE;
		
		if( bFOVAngleChanged )
		{
			// Clamp the FOV value to valid ranges.
			GEditor->FOVAngle = Clamp<FLOAT>( GEditor->FOVAngle, 1.0f, 179.0f );

			// Loop through all viewports and update their FOV angles and invalidate them, forcing them to redraw.
			for( INT ViewportIndex = 0 ; ViewportIndex < ViewportClients.Num() ; ++ViewportIndex )
			{
				if (ViewportClients(ViewportIndex) && ViewportClients(ViewportIndex)->Viewport)
				{
					ViewportClients( ViewportIndex )->ViewFOV = GEditor->FOVAngle;
					ViewportClients( ViewportIndex )->Invalidate();
				}
			}
		}
	}

	// Propagate the callback up to the superclass.
	Super::PostEditChange(PropertyThatChanged);
}

FViewport* UEditorEngine::GetAViewport()
{
	if(GameViewport && GameViewport->Viewport)
	{
		return GameViewport->Viewport;
	}

	for(INT i=0; i<ViewportClients.Num(); i++)
	{
		if(ViewportClients(i) && ViewportClients(i)->Viewport)
		{
			return ViewportClients(i)->Viewport;
		}
	}

	return NULL;
}

/*-----------------------------------------------------------------------------
	Cleanup.
-----------------------------------------------------------------------------*/

/**
 * Cleans up after major events like e.g. map changes.
 *
 * @param	ClearSelection	Whether to clear selection
 * @param	Redraw			Whether to redraw viewports
 * @param	TransReset		Human readable reason for resetting the transaction system
 */
void UEditorEngine::Cleanse( UBOOL ClearSelection, UBOOL Redraw, const TCHAR* TransReset )
{
	check(TransReset);
	if( GIsRunning && !Bootstrapping )
	{
		if( ClearSelection )
		{
			// Clear selection sets.
			GetSelectedActors()->DeselectAll();
			GetSelectedObjects()->DeselectAll();
		}

		// Reset the transaction tracking system.
		Trans->Reset( TransReset );

		// Redraw the levels.
		if( Redraw )
		{
			RedrawLevelEditingViewports();
		}

		// Collect garbage.
		CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );
	}
}

/*---------------------------------------------------------------------------------------
	Components.
---------------------------------------------------------------------------------------*/

void UEditorEngine::ClearComponents()
{
	FEdMode* CurrentMode = GEditorModeTools().GetCurrentMode();
	if(CurrentMode)
	{
		CurrentMode->ClearComponent();
	}
	GWorld->ClearComponents();
}

void UEditorEngine::UpdateComponents()
{
	FEdMode* CurrentMode = GEditorModeTools().GetCurrentMode();
	if(CurrentMode)
	{
		CurrentMode->UpdateComponent();
	}
	GWorld->UpdateComponents( FALSE );
}

/** 
 * Returns an audio component linked to the current scene that it is safe to play a sound on
 *
 * @param	SoundCue	A sound cue to attach to the audio component
 * @param	SoundNode	A sound node that is attached to the audio component when the sound cue is NULL
 */
UAudioComponent* UEditorEngine::GetPreviewAudioComponent( USoundCue* SoundCue, USoundNode* SoundNode )
{
	if( Client && Client->GetAudioDevice() )
	{
		if( PreviewAudioComponent == NULL )
		{
			PreviewSoundCue = ConstructObject<USoundCue>( USoundCue::StaticClass() );
			PreviewAudioComponent = Client->GetAudioDevice()->CreateComponent( PreviewSoundCue, GWorld->Scene, NULL, FALSE );
		}

		check( PreviewAudioComponent );

		if( SoundNode != NULL )
		{
			PreviewSoundCue->FirstNode = SoundNode;
			PreviewAudioComponent->SoundCue = PreviewSoundCue;
		}
		else
		{
			PreviewAudioComponent->SoundCue = SoundCue;
		}
	}

	return( PreviewAudioComponent );
}

/** 
 * Stop any sounds playing on the preview audio component and allowed it to be garbage collected
 */
void UEditorEngine::ClearPreviewAudioComponents( void )
{
	if( PreviewAudioComponent )
	{
		PreviewAudioComponent->Stop();

		// Just null out so they get GC'd
		PreviewSoundCue = NULL;
		PreviewAudioComponent = NULL;
	}
}

/*---------------------------------------------------------------------------------------
	Misc.
---------------------------------------------------------------------------------------*/

/**
 * Issued by code reuqesting that decals be reattached.
 */
void UEditorEngine::IssueDecalUpdateRequest()
{
	bDecalUpdateRequested = TRUE;
}

void UEditorEngine::SetObjectPropagationDestination(INT Destination)
{
	// Layout of the ComboBox:
	//   No Propagation
	//   Local Standalone (a game running, but not In Editor)
	//   Console 0
	//   Console 1
	//   ...

	// remember out selection for when we restart the editor
	GConfig->SetInt(TEXT("ObjectPropagation"), TEXT("Destination"), Destination, GEditorIni);

	// first one is no propagation
	if (Destination == OPD_None)
	{
		FObjectPropagator::ClearPropagator();
	}
	else
	{
		// the rest are network propagations
		FObjectPropagator::SetPropagator(RemotePropagator);
		// the first one of these is a local standalone game (so we use 127.0.0.1)
		if (Destination == OPD_LocalStandalone)
		{
			GObjectPropagator->SetIPAddress(0x7F000001, __INTEL_BYTE_ORDER__); // 127.0.0.1
		}
		else
		{
			// the rest are retrieved from the console support classes
			FConsoleSupport* Console = GConsoleSupportContainer->GetConsoleSupport(Destination - OPD_ConsoleStart);
			GObjectPropagator->SetIPAddress(Console->GetIPAddress(-1), Console->GetIntelByteOrder());
		}
	}
}

/**
 * The support DLL may have changed the IP address to propagate, so update the IP address
 */
void UEditorEngine::UpdateObjectPropagationIPAddress()
{
	INT Destination;
	GConfig->GetInt(TEXT("ObjectPropagation"), TEXT("Destination"), Destination, GEditorIni);
	if (Destination >= OPD_ConsoleStart)
	{
		FConsoleSupport* Console = GConsoleSupportContainer->GetConsoleSupport(Destination - OPD_ConsoleStart);
		GObjectPropagator->SetIPAddress(Console->GetIPAddress(-1), Console->GetIntelByteOrder());
	}
}

/**
 *	Returns pointer to a temporary 256x256 render target.
 *	If it has not already been created, does so here.
 */
UTextureRenderTarget2D* UEditorEngine::GetScratchRenderTarget()
{
	if(!ScratchRenderTarget)
	{
		UTextureRenderTargetFactoryNew* NewFactory = CastChecked<UTextureRenderTargetFactoryNew>( StaticConstructObject(UTextureRenderTargetFactoryNew::StaticClass()) );
		NewFactory->Width = 256;
		NewFactory->Height = 256;

		UObject* NewObj = NewFactory->FactoryCreateNew( UTextureRenderTarget2D::StaticClass(), UObject::GetTransientPackage(), NAME_None, RF_Transient, NULL, GWarn );
		ScratchRenderTarget = CastChecked<UTextureRenderTarget2D>(NewObj);
	}
	check(ScratchRenderTarget);
	return ScratchRenderTarget;
}

/**
 * Check for any PrefabInstances which are out of date.  For any PrefabInstances which have a TemplateVersion less than its PrefabTemplate's
 * PrefabVersion, propagates the changes to the source Prefab to the PrefabInstance.
 */
void UEditorEngine::UpdatePrefabs()
{
	TArray<UPrefab*> UpdatedPrefabs;
	TArray<APrefabInstance*> RemovedPrefabs;
	UBOOL bResetInvalidPrefab = FALSE;

	USelection* SelectedActors = GetSelectedActors();
	for( FActorIterator It; It; ++It )
	{
		APrefabInstance* PrefabInst = Cast<APrefabInstance>(*It);

		// If this is a valid PrefabInstance
		if(	PrefabInst && !PrefabInst->bDeleteMe && !PrefabInst->IsPendingKill() )
		{
			// first, verify that this PrefabInstance is still bound to a valid prefab
			UPrefab* SourcePrefab = PrefabInst->TemplatePrefab;
			if ( SourcePrefab != NULL )
			{
				// first, verify that all archetypes in the prefab's ArchetypeToInstanceMap exist.
				if ( !PrefabInst->VerifyMemberArchetypes() )
				{
					bResetInvalidPrefab = TRUE;
				}

				// If the PrefabInstance's version number is less than the source Prefab's version (ie there is 
				// a newer version of the Prefab), update it now.
				if ( PrefabInst->TemplateVersion < SourcePrefab->PrefabVersion )
				{
					PrefabInst->UpdatePrefabInstance( GetSelectedActors() );

					// Mark the level package as dirty, so we are prompted to save the map.
					PrefabInst->MarkPackageDirty();

					// Add prefab to list of ones that we needed to update instances of.
					UpdatedPrefabs.AddUniqueItem(SourcePrefab);
				}
				else if ( PrefabInst->TemplateVersion > SourcePrefab->PrefabVersion )
				{
					bResetInvalidPrefab = TRUE;
					warnf(NAME_Warning, TEXT("PrefabInstance '%s' has a version number that is higher than the source Prefab's version.  Resetting existing PrefabInstance from source: '%s'"), *PrefabInst->GetPathName(), *SourcePrefab->GetPathName());

					// this PrefabInstance's version number is higher than the source Prefab's version number,
					// this is normally the result of updating a prefab, then saving the map but not the package containing
					// the prefab.  If this has occurred, we'll need to replace the existing PrefabInstance with a new copy 
					// of the older version of the Prefab, but we must warn the user when we do this!
					PrefabInst->DestroyPrefab(SelectedActors);
					PrefabInst->InstancePrefab(SourcePrefab);
				}
			}
			else
			{
				// if the PrefabInstance's TemplatePrefab is NULL, it probably means that the user created a prefab,
				// then reloaded the map containing the prefab instance without saving the package containing the prefab.
				PrefabInst->DestroyPrefab(SelectedActors);
				GWorld->DestroyActor(PrefabInst);
				SelectedActors->Deselect(PrefabInst);

				RemovedPrefabs.AddItem(PrefabInst);
			}
		}
	}

	// If we updated some prefab instances, display it in a dialog.
	if(UpdatedPrefabs.Num() > 0)
	{
		FString UpdatedPrefabList;
		for(INT i=0; i<UpdatedPrefabs.Num(); i++)
		{
			// Add name to list of updated Prefabs.
			UpdatedPrefabList += FString::Printf( TEXT("%s\n"), *(UpdatedPrefabs(i)->GetPathName()) );
		}

		appMsgf( AMT_OK, *LocalizeUnrealEd("Prefab_OldPrefabInstancesUpdated"), *UpdatedPrefabList );
	}

	if ( RemovedPrefabs.Num() )
	{
		FString RemovedPrefabList;
		for(INT i=0; i<RemovedPrefabs.Num(); i++)
		{
			// Add name to list of updated Prefabs.
			RemovedPrefabList += FString::Printf( TEXT("%s") LINE_TERMINATOR, *RemovedPrefabs(i)->GetPathName());
		}

		appMsgf( AMT_OK, *LocalizeUnrealEd("Prefab_MissingSourcePrefabs"), *RemovedPrefabList );
	}

	if ( bResetInvalidPrefab )
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("Prefab_ResetInvalidPrefab") );
	}
}


/** 
 *	Create a new instance of a prefab in the level. 
 *
 *	@param	Prefab		The prefab to create an instance of.
 *	@param	Location	Location to create the new prefab at.
 *	@param	Rotation	Rotation to create the new prefab at.
 *	@return				Pointer to new PrefabInstance actor in the level, or NULL if it fails.
 */
APrefabInstance* UEditorEngine::Prefab_InstancePrefab(UPrefab* Prefab, const FVector& Location, const FRotator& Rotation) const
{
	// First, create a new PrefabInstance actor.
	APrefabInstance* NewInstance = CastChecked<APrefabInstance>( GWorld->SpawnActor(APrefabInstance::StaticClass(), NAME_None, Location, Rotation) );
	if(NewInstance)
	{
		NewInstance->InstancePrefab(Prefab);
		return NewInstance;
	}
	else
	{
		return NULL;
	}
}

/** Util that looks for and fixes any incorrect ParentSequence pointers in Kismet objects in memory. */
void UEditorEngine::FixKismetParentSequences()
{
	INT NumFixed = 0;

	for( TObjectIterator<USequenceObject> It; It; ++It )
	{
		USequenceObject* SeqObj = *It;

		// if this sequence object is a subobject template, skip it as it won't be initialized (no ParentSequence, etc.)
		if ( SeqObj->IsTemplate() )
		{
			continue;
		}

		USequence* RootSeq = SeqObj->GetRootSequence();
		const UBOOL bIsUISequenceObject = RootSeq != NULL && RootSeq->IsA(UUISequence::StaticClass());

		// check it for general errors
		if ( !bIsUISequenceObject )
		{
			SeqObj->CheckForErrors();
		}
		
		// If we have a ParentSequence, but it does not contain me - thats a bug. Fix it.
		if( SeqObj->ParentSequence && 
			!SeqObj->ParentSequence->SequenceObjects.ContainsItem(SeqObj) )
		{
			UBOOL bFoundCorrectParent = FALSE;

			// Find the sequence that _does_ contain SeqObj
			for( FObjectIterator TestIt; TestIt; ++TestIt )
			{
				UObject* TestObj = *TestIt;
				USequence* TestSeq = Cast<USequence>(TestObj);
				if(TestSeq && TestSeq->SequenceObjects.ContainsItem(SeqObj))
				{
					// If we have already found a sequence that contains SeqObj - warn about that too!
					if(bFoundCorrectParent)
					{
						debugf( TEXT("Multiple Sequences Contain '%s'"), *SeqObj->GetName() );
					}
					else
					{
						NumFixed++;

						// Change ParentSequence pointer to correct sequence
						SeqObj->ParentSequence = TestSeq;

						// Mark package as dirty
						SeqObj->MarkPackageDirty();
						bFoundCorrectParent = TRUE;
					}
				}
			}

			// It's also bad if we couldn't find the correct parent!
			if(!bFoundCorrectParent)
			{
				debugf( TEXT("No correct parent found for '%s'.  Try entering 'OBJ REFS class=%s name=%s' into the command window to determine how this sequence object is being referenced."),
					*SeqObj->GetName(), *SeqObj->GetClass()->GetName(), *SeqObj->GetPathName() );
			}
		}
	}

	// If we corrected some things - tell the user
	if(NumFixed > 0)
	{	
		appMsgf( AMT_OK, *LocalizeUnrealEd("FixedKismetParentSequences"), NumFixed);
	}
}

/**
 * Warns the user of any hidden streaming levels, and prompts them with a Yes/No dialog
 * for whether they wish to continue with the operation.  No dialog is presented if all
 * streaming levels are visible.  The return value is TRUE if no levels are hidden or
 * the user selects "Yes", or FALSE if the user selects "No".
 *
 * @param	AdditionalMessage		An additional message to include in the dialog.  Can be NULL.
 * @return							FALSE if the user selects "No", TRUE otherwise.
 */
UBOOL UEditorEngine::WarnAboutHiddenLevels(const TCHAR* AdditionalMessage) const
{
	UBOOL bResult = TRUE;

	// Make a list of all hidden levels.
	AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
	TArray<FString> HiddenLevels;
	for( INT LevelIndex = 0 ; LevelIndex< WorldInfo->StreamingLevels.Num() ; ++LevelIndex )
	{
		ULevelStreaming* StreamingLevel = WorldInfo->StreamingLevels( LevelIndex );
		if( StreamingLevel && !StreamingLevel->bShouldBeVisibleInEditor )
		{
			HiddenLevels.AddItem(StreamingLevel->PackageName.ToString() );
		}
	}

	// Warn the user that some levels are hidden and prompt for continue.
	if ( HiddenLevels.Num() > 0 )
	{
		FString ContinueMessage( LocalizeUnrealEd(TEXT("TheFollowingLevelsAreHidden")) );
		for ( INT LevelIndex = 0 ; LevelIndex < HiddenLevels.Num() ; ++LevelIndex )
		{
			ContinueMessage += FString::Printf( TEXT("\n    %s"), *HiddenLevels(LevelIndex) );
		}
		if ( AdditionalMessage )
		{
			ContinueMessage += FString::Printf( TEXT("\n%s"), AdditionalMessage );
		}
		else
		{
			ContinueMessage += FString::Printf( TEXT("\n%s"), *LocalizeUnrealEd(TEXT("ContinueQ")) );
		}
		bResult = appMsgf( AMT_YesNo, TEXT("%s"), *ContinueMessage );
	}

	return bResult;
}

/**
 *	Sets the texture to use for displaying StreamingBounds.
 *
 *	@param	InTexture	The source texture for displaying StreamingBounds.
 *						Pass in NULL to disable displaying them.
 */
void UEditorEngine::SetStreamingBoundsTexture(UTexture2D* InTexture)
{
	if (StreamingBoundsTexture != InTexture)
	{
		// Clear the currently stored streaming bounds information

		// Set the new texture
		StreamingBoundsTexture = InTexture;
		if (StreamingBoundsTexture != NULL)
		{
			// Fill in the new streaming bounds info
			for (TObjectIterator<ULevel> It; It; ++It)
			{
				ULevel* Level = *It;
				Level->BuildStreamingData(InTexture);
			}

			// Turn on the StreamingBounds show flag
			for(UINT ViewportIndex = 0; ViewportIndex < (UINT)ViewportClients.Num(); ViewportIndex++)
			{
				FEditorLevelViewportClient* ViewportClient = ViewportClients(ViewportIndex);
				if (ViewportClient)
				{
					ViewportClient->ShowFlags |= SHOW_StreamingBounds;
					ViewportClient->Invalidate(FALSE,TRUE);
				}
			}
		}
		else
		{
			// Clear the streaming bounds info
			// Turn off the StreamingBounds show flag
			for(UINT ViewportIndex = 0; ViewportIndex < (UINT)ViewportClients.Num(); ViewportIndex++)
			{
				FEditorLevelViewportClient* ViewportClient = ViewportClients(ViewportIndex);
				if (ViewportClient)
				{
					ViewportClient->ShowFlags &= ~SHOW_StreamingBounds;
					ViewportClient->Invalidate(FALSE,TRUE);
				}
			}
		}
	}
}

void UEditorEngine::ApplyDeltaToActor(AActor* InActor,
									  UBOOL bDelta,
									  const FVector* InTrans,
									  const FRotator* InRot,
									  const FVector* InScale,
									  UBOOL bAltDown,
									  UBOOL bShiftDown,
									  UBOOL bControlDown) const
{
	InActor->Modify();
	if( InActor->IsBrush() )
	{
		ABrush* Brush = (ABrush*)InActor;
		if( Brush->BrushComponent && Brush->BrushComponent->Brush )
		{
			Brush->BrushComponent->Brush->Polys->Element.ModifyAllItems();
		}
	}

	///////////////////
	// Rotation

	// Unfortunately this can't be moved into ABrush::EditorApplyRotation, as that would
	// create a dependence in Engine on Editor.
	if ( InRot )
	{
		const FRotator& InDeltaRot = *InRot;
		const UBOOL bRotatingActor = !bDelta || !InDeltaRot.IsZero();
		if( bRotatingActor )
		{
			if( InActor->IsBrush() )
			{
				FBSPOps::RotateBrushVerts( (ABrush*)InActor, InDeltaRot, TRUE );
			}
			else
			{
				if ( bDelta )
				{
					InActor->EditorApplyRotation( InDeltaRot, bAltDown, bShiftDown, bControlDown );
				}
				else
				{
					InActor->SetRotation( InDeltaRot );
				}
			}

			if ( bDelta )
			{
				FVector NewActorLocation = InActor->Location;
				NewActorLocation -= GEditorModeTools().PivotLocation;
				NewActorLocation = FRotationMatrix( InDeltaRot ).TransformFVector( NewActorLocation );
				NewActorLocation += GEditorModeTools().PivotLocation;
				InActor->Location = NewActorLocation;
			}
		}
	}

	///////////////////
	// Translation
	if ( InTrans )
	{
		if ( bDelta )
		{
			InActor->EditorApplyTranslation( *InTrans, bAltDown, bShiftDown, bControlDown );
		}
		else
		{
			InActor->SetLocation( *InTrans );
		}
	}

	///////////////////
	// Scaling
	if ( InScale )
	{
		const FVector& InDeltaScale = *InScale;
		const UBOOL bScalingActor = !bDelta || !InDeltaScale.IsNearlyZero(0.000001f);
		if( bScalingActor )
		{
			// If the actor is a brush, update the vertices.
			if( InActor->IsBrush() )
			{
				ABrush* Brush = (ABrush*)InActor;
				FVector ModifiedScale = InDeltaScale;

				// Get Box Extents
				const FBox BrushBox = InActor->GetComponentsBoundingBox( TRUE );
				const FVector BrushExtents = BrushBox.GetExtent();

				// Make sure brushes are clamped to a minimum size.
				FLOAT MinThreshold = 1.0f;

				for (INT Idx=0; Idx<3; Idx++)
				{
					const UBOOL bBelowAllowableScaleThreshold = ((InDeltaScale[Idx] + 1.0f) * BrushExtents[Idx]) < MinThreshold;

					if(bBelowAllowableScaleThreshold)
					{
						ModifiedScale[Idx] = (MinThreshold / BrushExtents[Idx]) - 1.0f;
					}
				}

				// If we are uniformly scaling, make sure that the modified scale is always the same for all 3 axis.
				if(GEditorModeTools().GetWidgetMode() == FWidget::WM_Scale)
				{
					INT Min = 0;
					for(INT Idx=1; Idx < 3; Idx++)
					{
						if(Abs(ModifiedScale[Idx]) < Abs(ModifiedScale[Min]))
						{
							Min=Idx;
						}
					}

					for(INT Idx=0; Idx < 3; Idx++)
					{
						if(Min != Idx)
						{
							ModifiedScale[Idx] = ModifiedScale[Min];
						}
					}
				}

				// Scale all of the polygons of the brush.
				const FScaleMatrix matrix( FVector( ModifiedScale.X , ModifiedScale.Y, ModifiedScale.Z ) );
				
				if(Brush->BrushComponent->Brush && Brush->BrushComponent->Brush->Polys)
				{
					for( INT poly = 0 ; poly < Brush->BrushComponent->Brush->Polys->Element.Num() ; poly++ )
					{
						FPoly* Poly = &(Brush->BrushComponent->Brush->Polys->Element(poly));

						FBox bboxBefore(0);
						for( INT vertex = 0 ; vertex < Poly->Vertices.Num() ; vertex++ )
						{
							bboxBefore += Brush->LocalToWorld().TransformFVector( Poly->Vertices(vertex) );
						}

						// Scale the vertices

						for( INT vertex = 0 ; vertex < Poly->Vertices.Num() ; vertex++ )
						{
							FVector Wk = Brush->LocalToWorld().TransformFVector( Poly->Vertices(vertex) );
							Wk -= GEditorModeTools().PivotLocation;
							Wk += matrix.TransformFVector( Wk );
							Wk += GEditorModeTools().PivotLocation;
							Poly->Vertices(vertex) = Brush->WorldToLocal().TransformFVector( Wk );
						}

						FBox bboxAfter(0);
						for( INT vertex = 0 ; vertex < Poly->Vertices.Num() ; vertex++ )
						{
							bboxAfter += Brush->LocalToWorld().TransformFVector( Poly->Vertices(vertex) );
						}

						FVector Wk = Brush->LocalToWorld().TransformFVector( Poly->Base );
						Wk -= GEditorModeTools().PivotLocation;
						Wk += matrix.TransformFVector( Wk );
						Wk += GEditorModeTools().PivotLocation;
						Poly->Base = Brush->WorldToLocal().TransformFVector( Wk );

						// Scale the texture vectors

						for( INT a = 0 ; a < 3 ; ++a )
						{
							const FLOAT Before = bboxBefore.GetExtent()[a];
							const FLOAT After = bboxAfter.GetExtent()[a];

							if( After != 0.0 )
							{
								const FLOAT Pct = Before / After;

								if( Pct != 0.0 )
								{
									Poly->TextureU[a] *= Pct;
									Poly->TextureV[a] *= Pct;
								}
							}
						}

						// Recalc the normal for the poly

						Poly->Normal = FVector(0,0,0);
						Poly->Finalize((ABrush*)InActor,0);
					}

					Brush->BrushComponent->Brush->BuildBound();

					if( !Brush->IsStaticBrush() )
					{
						FBSPOps::csgPrepMovingBrush( Brush );
					}
				}
			}
			else
			{
				if ( bDelta )
				{
					const FScaleMatrix matrix( FVector( InDeltaScale.X , InDeltaScale.Y, InDeltaScale.Z ) );
					InActor->EditorApplyScale( InDeltaScale,
												matrix,
												&GEditorModeTools().PivotLocation,
												bAltDown,
												bShiftDown,
												bControlDown );
				}
				else
				{
					InActor->DrawScale3D = InDeltaScale;
				}
			}

			InActor->ClearComponents();
		}
	}

	// Update the actor before leaving.
	InActor->MarkPackageDirty();
	InActor->InvalidateLightingCache();
	InActor->PostEditMove( FALSE );
	InActor->ForceUpdateComponents();
}

#if !FINAL_RELEASE
/**
 * Handles freezing/unfreezing of rendering
 */
void UEditorEngine::ProcessToggleFreezeCommand()
{
	if (GIsPlayInEditorWorld)
	{
		GamePlayers(0)->ViewportClient->Viewport->ProcessToggleFreezeCommand();
	}
	else
	{
		// pass along the freeze command to all perspective viewports
		for(INT ViewportIndex = 0; ViewportIndex < ViewportClients.Num(); ViewportIndex++)
		{
			if (ViewportClients(ViewportIndex)->ViewportType == LVT_Perspective)
			{
				ViewportClients(ViewportIndex)->Viewport->ProcessToggleFreezeCommand();
			}
		}
	}

	// tell editor to update views
	RedrawAllViewports();
}

/**
 * Handles frezing/unfreezing of streaming
 */
void UEditorEngine::ProcessToggleFreezeStreamingCommand()
{
	// freeze vis in PIE
	if (GIsPlayInEditorWorld)
	{
		PlayWorld->bIsLevelStreamingFrozen = !PlayWorld->bIsLevelStreamingFrozen;
	}
}

#endif

/*-----------------------------------------------------------------------------
	Reimporting.
-----------------------------------------------------------------------------*/

/** Constructor */
FReimportManager::FReimportManager()
{
	// Create reimport handler for textures
	// NOTE: New factories can be created anywhere, inside or outside of editor
	// This is just here for convenience
	new UReimportTextureFactory();
}


/** 
* Singleton function
* @return Singleton instance of this manager
*/
FReimportManager* FReimportManager::Instance()
{
	static FReimportManager Inst;
	return &Inst;
}

/**
* Reimports specified texture from its source material, if the meta-data exists
* @param Package texture to reimport
* @return TRUE if handled
*/
UBOOL FReimportManager::Reimport( UObject* Obj )
{
	for(INT I=0;I<Handlers.Num();I++)
	{
		if(Handlers(I)->Reimport(Obj))
			return TRUE;
	}
	return FALSE;
}

/*-----------------------------------------------------------------------------
	PIE helpers.
-----------------------------------------------------------------------------*/

/**
 * Sets GWorld to the passed in PlayWorld and sets a global flag indicating that we are playing
 * in the Editor.
 *
 * @param	PlayInEditorWorld		PlayWorld
 * @return	the original GWorld
 */
UWorld* SetPlayInEditorWorld( UWorld* PlayInEditorWorld )
{
	check(!GIsPlayInEditorWorld);
	UWorld* SavedWorld = GWorld;
	GIsPlayInEditorWorld = TRUE;
	GIsGame = (!GIsEditor || GIsPlayInEditorWorld);
	GWorld = PlayInEditorWorld;
	return SavedWorld;
}

/**
 * Restores GWorld to the passed in one and reset the global flag indicating whether we are a PIE
 * world or not.
 *
 * @param EditorWorld	original world being edited
 */
void RestoreEditorWorld( UWorld* EditorWorld )
{
	check(GIsPlayInEditorWorld);
	GIsPlayInEditorWorld = FALSE;
	GIsGame = (!GIsEditor || GIsPlayInEditorWorld);
	GWorld = EditorWorld;
}


/*-----------------------------------------------------------------------------
	Cooking helpers.
-----------------------------------------------------------------------------*/
/*
 * Create a struct to pass to tools from the parsed wave data
 */
void CreateWaveFormat( FWaveModInfo& WaveInfo, WAVEFORMATEXTENSIBLE& WaveFormat )
{
	appMemzero( &WaveFormat, sizeof( WAVEFORMATEXTENSIBLE ) );

	WaveFormat.Format.nAvgBytesPerSec = *WaveInfo.pAvgBytesPerSec;
	WaveFormat.Format.nBlockAlign = *WaveInfo.pBlockAlign;
	WaveFormat.Format.nChannels = *WaveInfo.pChannels;
	WaveFormat.Format.nSamplesPerSec = *WaveInfo.pSamplesPerSec;
	WaveFormat.Format.wBitsPerSample = *WaveInfo.pBitsPerSample;
	WaveFormat.Format.wFormatTag = WAVE_FORMAT_PCM;
}

/**
 * Read a wave file header from bulkdata
 */
UBOOL ReadWaveHeader( FWaveModInfo& WaveInfo, BYTE* RawWaveData, INT Size, INT Offset )
{
	if( Size == 0 )
	{
		return( FALSE );
	}

	// Parse wave info.
	if( !WaveInfo.ReadWaveInfo( RawWaveData + Offset, Size ) )
	{
		return( FALSE );
	}

	// Validate the info
	if( ( *WaveInfo.pChannels != 1 && *WaveInfo.pChannels != 2 ) || *WaveInfo.pBitsPerSample != 16 )
	{
		return( FALSE );
	}

	return( TRUE );
}

/**
 * Cook a simple mono or stereo wave
 */
void CookSimpleWave( USoundNodeWave* SoundNodeWave, FConsoleSoundCooker* SoundCooker, FByteBulkData& DestinationData )
{
	FWaveModInfo			WaveInfo;
	WAVEFORMATEXTENSIBLE	WaveFormat;						

	// Lock raw wave data.
	BYTE* RawWaveData = ( BYTE * )SoundNodeWave->RawData.Lock( LOCK_READ_ONLY );

	if( !ReadWaveHeader( WaveInfo, RawWaveData, SoundNodeWave->RawData.GetBulkDataSize(), 0 ) )
	{
		SoundNodeWave->RawData.Unlock();
		warnf( TEXT( "Only mono or stereo 16 bit wavs allowed: %s" ), *SoundNodeWave->GetFullName() );
		return;
	}

	// Create wave format structure for encoder. Set up like a regular WAVEFORMATEX.
	CreateWaveFormat( WaveInfo, WaveFormat );

	debugf( TEXT( "Cooking: %s" ), *SoundNodeWave->GetFullName() );

	// Cook the data.
	if( SoundCooker->Cook( ( short * )WaveInfo.SampleDataStart, WaveInfo.SampleDataSize, &WaveFormat, SoundNodeWave->CompressionQuality ) == TRUE ) 
	{
		// Make place for cooked data.
		DestinationData.Lock( LOCK_READ_WRITE );
		BYTE* RawCompressedData = ( BYTE* )DestinationData.Realloc( SoundCooker->GetCookedDataSize() );

		// Retrieve cooked data.
		SoundCooker->GetCookedData( RawCompressedData );
		DestinationData.Unlock();

		SoundNodeWave->SampleRate = *WaveInfo.pSamplesPerSec;
		SoundNodeWave->NumChannels = WaveFormat.Format.nChannels;
		SoundNodeWave->SampleDataSize = WaveInfo.SampleDataSize;
		SoundNodeWave->Duration = ( FLOAT )SoundNodeWave->SampleDataSize / ( SoundNodeWave->SampleRate * sizeof( SWORD ) );
	}
	else
	{
		warnf( NAME_Warning, TEXT( "Cooking simple sound failed: %s" ), *SoundNodeWave->GetPathName() );

		// Empty data and set invalid format token.
		DestinationData.RemoveBulkData();
	}

	// Unlock source as we no longer need the data
	SoundNodeWave->RawData.Unlock();
}

/**
 * Cook a multistream (normally 5.1) wave
 */
void CookSurroundWave( USoundNodeWave* SoundNodeWave, FConsoleSoundCooker* SoundCooker, FByteBulkData& DestinationData )
{
	INT						i, ChannelCount;
	DWORD					SampleDataSize;
	FWaveModInfo			WaveInfo;
	WAVEFORMATEXTENSIBLE	WaveFormat;						
	short *					SourceBuffers[SPEAKER_Count] = { NULL };

	BYTE* RawWaveData = ( BYTE * )SoundNodeWave->RawData.Lock( LOCK_READ_ONLY );

	// Front left channel is the master
	ChannelCount = 1;
	if( ReadWaveHeader( WaveInfo, RawWaveData, SoundNodeWave->ChannelSizes( SPEAKER_FrontLeft ), SoundNodeWave->ChannelOffsets( SPEAKER_FrontLeft ) ) )
	{
		SampleDataSize = WaveInfo.SampleDataSize;
		SourceBuffers[SPEAKER_FrontLeft] = ( short * )WaveInfo.SampleDataStart;

		// Create wave format structure for encoder. Set up like a regular WAVEFORMATEX.
		CreateWaveFormat( WaveInfo, WaveFormat );

		// Extract all the info for the other channels (may be blank)
		for( i = 1; i < SPEAKER_Count; i++ )
		{
			if( ReadWaveHeader( WaveInfo, RawWaveData, SoundNodeWave->ChannelSizes( i ), SoundNodeWave->ChannelOffsets( i ) ) )
			{
				// Only mono files allowed
				if( *WaveInfo.pChannels == 1 )
				{
					SourceBuffers[i] = ( short * )WaveInfo.SampleDataStart;
					ChannelCount++;

					// Truncating to the shortest sound
					if( WaveInfo.SampleDataSize < SampleDataSize )
					{
						SampleDataSize = WaveInfo.SampleDataSize;
					}
				}
			}
		}
	
		// Only allow the formats that can be played back through
		if( ChannelCount == 4 || ChannelCount == 6 || ChannelCount == 7 || ChannelCount == 8 )
		{
			debugf( TEXT( "Cooking %d channels for: %s" ), ChannelCount, *SoundNodeWave->GetFullName() );

			if( SoundCooker->CookSurround( SourceBuffers, SampleDataSize, &WaveFormat, SoundNodeWave->CompressionQuality ) == TRUE ) 
			{
				// Make place for cooked data.
				DestinationData.Lock( LOCK_READ_WRITE );
				BYTE * RawCompressedData = ( BYTE * )DestinationData.Realloc( SoundCooker->GetCookedDataSize() );

				// Retrieve cooked data.
				SoundCooker->GetCookedData( RawCompressedData );
				DestinationData.Unlock();

				SoundNodeWave->SampleRate = *WaveInfo.pSamplesPerSec;
				SoundNodeWave->NumChannels = ChannelCount;
				SoundNodeWave->SampleDataSize = SampleDataSize * ChannelCount;
				SoundNodeWave->Duration = ( FLOAT )SoundNodeWave->SampleDataSize / ( SoundNodeWave->SampleRate * sizeof( SWORD ) );
			}
			else
			{
				warnf( NAME_Warning, TEXT( "Cooking surround sound failed: %s" ), *SoundNodeWave->GetPathName() );

				// Empty data and set invalid format token.
				DestinationData.RemoveBulkData();
			}
		}
		else
		{
			warnf( NAME_Warning, TEXT( "No format available for a %d channel surround sound: %s" ), ChannelCount, *SoundNodeWave->GetFullName() );
		}
	}
	else
	{
		warnf( NAME_Warning, TEXT( "Cooking surround sound failed: %s" ), *SoundNodeWave->GetPathName() );
	}

	SoundNodeWave->RawData.Unlock();
}

/**
 * Cooks SoundNodeWave to a specific platform
 *
 * @param	SoundNodeWave			Wave file to cook
 * @param	SoundCooker				Platform specific cooker object to cook with
 * @param	DestinationData			Destination bulk data
 */
UBOOL CookSoundNodeWave( USoundNodeWave* SoundNodeWave, FConsoleSoundCooker* SoundCooker, FByteBulkData& DestinationData )
{
	check( SoundCooker );

	if( DestinationData.GetBulkDataSize() > 0 && SoundNodeWave->NumChannels > 0 )
	{
		// Already cooked for this platform
		return( FALSE );
	}

	// Compress the sound using the platform specific cooker compression (if not already compressed)
	if( SoundNodeWave->ChannelSizes.Num() == 0 )
	{
		check( SoundNodeWave->ChannelOffsets.Num() == 0 );
		check( SoundNodeWave->ChannelSizes.Num() == 0 );
		CookSimpleWave( SoundNodeWave, SoundCooker, DestinationData );
	}
	else if( SoundNodeWave->ChannelSizes.Num() > 0 )
	{
		check( SoundNodeWave->ChannelOffsets.Num() == SPEAKER_Count );
		check( SoundNodeWave->ChannelSizes.Num() == SPEAKER_Count );
		CookSurroundWave( SoundNodeWave, SoundCooker, DestinationData );
	}

	return( TRUE );
}

/**
 * Decompresses sound data compressed for a specific platform
 *
 * @param	SourceData				Compressed sound data
 * @param	SoundCooker				Platform specific cooker object to cook with
 * @param	Destination				Where to put the decompressed sounds
 */
void ThawSoundNodeWave( FConsoleSoundCooker* SoundCooker, USoundNodeWave* Wave, SWORD*& Destination )
{
	Destination = NULL;

	if( SoundCooker->Decompress( Wave->ResourceData, Wave->ResourceSize, Wave ) )
	{
		Destination = ( SWORD* )appMalloc( SoundCooker->GetRawDataSize() );
		SoundCooker->GetRawData( ( BYTE* )Destination );
	}
}

/**
 * Cooks SoundNodeWave to all available platforms
 *
 * @param	SoundNodeWave			Wave file to cook
 * @param	Platform				Platform to cook for - PLATFORM_Unknown for all
 */
UBOOL CookSoundNodeWave( USoundNodeWave* SoundNodeWave, UE3::EPlatformType Platform )
{
	UBOOL bDirty = FALSE;

	if( Platform == UE3::PLATFORM_Unknown || Platform == UE3::PLATFORM_Xenon )
	{
		FConsoleSupport* XenonSupport = GConsoleSupportContainer->GetConsoleSupport( TEXT( "Xenon" ) );
		if( XenonSupport )
		{
			bDirty |= CookSoundNodeWave( SoundNodeWave, XenonSupport->GetGlobalSoundCooker(), SoundNodeWave->CompressedXbox360Data );
		}
	}

	if( Platform == UE3::PLATFORM_Unknown || Platform == UE3::PLATFORM_PS3 )
	{
		FConsoleSupport* PS3Support = GConsoleSupportContainer->GetConsoleSupport( TEXT( "PS3" ) );
		if( PS3Support )
		{
			bDirty |= CookSoundNodeWave( SoundNodeWave, PS3Support->GetGlobalSoundCooker(), SoundNodeWave->CompressedPS3Data );
		}
	}

	if( Platform == UE3::PLATFORM_Unknown || Platform == UE3::PLATFORM_Windows )
	{
		bDirty |= CookSoundNodeWave( SoundNodeWave, GetPCSoundCooker(), SoundNodeWave->CompressedPCData );
	}

	return( bDirty );
}

/**
 * Compresses SoundNodeWave for all available platforms, and then decompresses to PCM 
 *
 * @param	SoundNodeWave			Wave file to compress
 * @param	PreviewInfo				Compressed stats and decompressed data
 */
void SoundNodeWaveQualityPreview( USoundNodeWave* SoundNode, FPreviewInfo* PreviewInfo )
{
	// Compress to all platforms
	SoundNode->CompressionQuality = PreviewInfo->QualitySetting;

	SoundNode->CompressedPCData.RemoveBulkData();
	SoundNode->CompressedXbox360Data.RemoveBulkData();
	SoundNode->CompressedPS3Data.RemoveBulkData();
	SoundNode->NumChannels = 0;

	CookSoundNodeWave( SoundNode );

	// Extract the stats
	PreviewInfo->OriginalSize = SoundNode->SampleDataSize;
	PreviewInfo->OggVorbisSize = SoundNode->CompressedPCData.GetBulkDataSize();
	PreviewInfo->XMASize = SoundNode->CompressedXbox360Data.GetBulkDataSize();
	PreviewInfo->ATRAC3Size = SoundNode->CompressedPS3Data.GetBulkDataSize();

	// Expand back to PCM data
	SoundNode->RemoveAudioResource();
	SoundNode->InitAudioResource( SoundNode->CompressedPCData );
	ThawSoundNodeWave( GetPCSoundCooker(), SoundNode, PreviewInfo->DecompressedOggVorbis );

	// PCF added debug info about decompression failed
	if( PreviewInfo->DecompressedOggVorbis == NULL )
	{
		debugfSuppressed( NAME_DevAudio, TEXT( "PC decompression failed" ) );
	}
	
	FConsoleSupport* XenonSupport = GConsoleSupportContainer->GetConsoleSupport( TEXT( "Xenon" ) );
	if( XenonSupport )
	{
		SoundNode->RemoveAudioResource();
		SoundNode->InitAudioResource( SoundNode->CompressedXbox360Data );
		ThawSoundNodeWave( XenonSupport->GetGlobalSoundCooker(), SoundNode, PreviewInfo->DecompressedXMA );
		if( PreviewInfo->DecompressedXMA == NULL )
		{
			debugfSuppressed( NAME_DevAudio, TEXT( "Xenon decompression failed" ) );
		}
	}

	FConsoleSupport* PS3Support = GConsoleSupportContainer->GetConsoleSupport( TEXT( "PS3" ) );
	if( PS3Support )
	{
		SoundNode->RemoveAudioResource();
		SoundNode->InitAudioResource( SoundNode->CompressedPS3Data );
		ThawSoundNodeWave( PS3Support->GetGlobalSoundCooker(), SoundNode, PreviewInfo->DecompressedATRAC3 );
		if( PreviewInfo->DecompressedATRAC3 == NULL )
		{
			debugfSuppressed( NAME_DevAudio, TEXT( "PS3 decompression failed" ) );
		}
	}
	// PCF End
}

/** 
 * Makes sure ogg vorbis data is available for this sound node by converting on demand
 */
UBOOL USoundNodeWave::ValidateData( void )
{
	return( CookSoundNodeWave( this, GetPCSoundCooker(), CompressedPCData ) );
}

/** 
 * Makes sure ogg vorbis data is available for all sound nodes in this cue by converting on demand
 */
UBOOL USoundCue::ValidateData( void )
{
	TArray<USoundNodeWave*> Waves;
	RecursiveFindWaves( FirstNode, Waves );

	UBOOL Converted = FALSE;
	for( INT WaveIndex = 0; WaveIndex < Waves.Num(); ++WaveIndex )
	{
		USoundNodeWave* Sound = Waves( WaveIndex );
		Converted |= Sound->ValidateData();
	}

	return( Converted );
}

// end

