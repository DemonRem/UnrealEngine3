/*=============================================================================
	UnInteraction.cpp: See .UC for for info
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "CanvasScene.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "UnUIKeys.h"

IMPLEMENT_CLASS(UConsole);
IMPLEMENT_CLASS(UInteraction);
IMPLEMENT_CLASS(UUIInteraction);
IMPLEMENT_CLASS(UUIInputConfiguration);

// Initialize all of the UI event keys.
#define DEFINE_UIKEY(Name) FName UIKEY_##Name;
	#include "UnUIKeys.h"
#undef DEFINE_UIKEY



/**
 * Minimal initialization constructor.
 */
UInteraction::UInteraction()
{
	if ( !HasAnyFlags(RF_ClassDefaultObject) )
	{
		// Initialize script execution.
		InitExecution();
	}
}

/**
 * Called when the interaction is added to the GlobalInteractions array
 */
void UInteraction::Init()
{
	delegateOnInitialize();
}

void UInteraction::execInit( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	Init();
}

/* ==========================================================================================================
	UUIInteraction
========================================================================================================== */
/**
 * Initializes the singleton data store client that will manage the global data stores.
 */
void UUIInteraction::InitializeGlobalDataStore()
{
	if ( DataStoreManager == NULL )
	{
		DataStoreManager = CreateGlobalDataStoreClient(this);
	}
}

/**
 * Constructor
 */
UUIInteraction::UUIInteraction()
: CanvasScene(NULL), bIsUIPrimitiveSceneInitialized(FALSE)
{
}

/**
 * Called when UIInteraction is added to the GameViewportClient's Interactions array
 */
void UUIInteraction::Init()
{
	Super::Init();


	// initialize the list of keys that can generate double-click events
#define DEFINE_KEY(Name, SupportedEvent) if ( SupportedEvent == SIE_MouseButton ) { SupportedDoubleClickKeys.AddItem(KEY_##Name); }
	#include "UnKeys.h"
#undef DEFINE_KEY

	// Initialize the UI Input Key Maps
	InitializeUIInputAliasNames();
	InitializeInputAliasLookupTable();

	InitializeAxisInputEmulations();

	// create the global data store manager
	InitializeGlobalDataStore();

	SceneClient = ConstructObject<UGameUISceneClient>(SceneClientClass, this, NAME_None, RF_Transient);
	SceneClient->DataStoreManager = DataStoreManager;

	UUISkin* InitialSkin = LoadInitialSkin();
	SceneClient->InitializeClient( InitialSkin );

	// create the canvas scene for rendering prims of this UI
	if( CanvasScene )
	{
		CanvasScene->Release();
	}	
	CanvasScene = new FCanvasScene();
}

/**
 * Cleans up all objects created by this UIInteraction, including unrooting objects and unreferencing any other objects.
 * Called when the UI system is being closed down (such as when exiting PIE).
 */
void UUIInteraction::TearDownUI()
{
	// remove any helper objects that we've added to the root set so they can be GC'd once unreferenced
	if ( DataStoreManager != NULL )
	{
		DataStoreManager->RemoveFromRoot();
	}

	// now unreference any objects we created
	DataStoreManager = NULL;
	if ( GCallbackEvent != NULL && SceneClient != NULL )
	{
		// no longer receive notifications when the viewport is resized
		GCallbackEvent->Unregister(CALLBACK_ViewportResized, SceneClient);
	}
	SceneClient = NULL;

	if ( CanvasScene )
	{
		CanvasScene->DetachAllComponents();
		CanvasScene->Release();
		CanvasScene = NULL;
	}

	// finally, remove ourselves from the root set
	RemoveFromRoot();
}

/**
 * Called to finish destroying the object.
 */
void UUIInteraction::FinishDestroy()
{
	if( CanvasScene )
	{
		CanvasScene->DetachAllComponents();
		CanvasScene->Release();
		CanvasScene = NULL;
	}

	Super::FinishDestroy();
}

/**
 * Returns the number of players currently active.
 */
INT UUIInteraction::GetPlayerCount()
{
	return GEngine->GamePlayers.Num();
}
void UUIInteraction::execGetPlayerCount( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	*(INT*)Result=GetPlayerCount();
}

/**
 * Retrieves the index (into the Engine.GamePlayers array) for the player which has the ControllerId specified
 *
 * @param	GamepadIndex	the gamepad index of the player to search for
 */
INT UUIInteraction::GetPlayerIndex( INT GamepadIndex )
{
	INT Result = INDEX_NONE;

	for ( INT PlayerIndex = 0; PlayerIndex < GEngine->GamePlayers.Num(); PlayerIndex++ )
	{
		ULocalPlayer* Player = GEngine->GamePlayers(PlayerIndex);
		if ( Player != NULL && Player->ControllerId == GamepadIndex )
		{
			Result = PlayerIndex;
			break;
		}
	}

	return Result;
}
void UUIInteraction::execGetPlayerIndex( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(GamepadIndex);
	P_FINISH;
	*(INT*)Result=GetPlayerIndex(GamepadIndex);
}

/**
 * Returns the index [into the Engine.GamePlayers array] for the player specified.
 *
 * @param	Player	the player to search for
 *
 * @return	the index of the player specified, or INDEX_NONE if the player is not in the game's list of active players.
 */
INT UUIInteraction::GetPlayerIndex( ULocalPlayer* Player )
{
	INT Result = INDEX_NONE;

	if ( Player != NULL && GEngine != NULL )
	{
		Result = GEngine->GamePlayers.FindItemIndex(Player);
	}

	return Result;
}

/**
 * Retrieves the ControllerId for the player specified.
 *
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to retrieve the ControllerId for
 *
 * @return	the ControllerId for the player at the specified index in the GamePlayers array, or INDEX_NONE if the index is invalid
 */
INT UUIInteraction::GetPlayerControllerId( INT PlayerIndex )
{
	INT Result = INDEX_NONE;

	if ( GEngine != NULL && GEngine->GamePlayers.IsValidIndex(PlayerIndex) )
	{
		Result = GEngine->GamePlayers(PlayerIndex)->ControllerId;
	}

	return Result;
}
void UUIInteraction::execGetPlayerControllerId( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(PlayerIndex);
	P_FINISH;
	*(INT*)Result=GetPlayerControllerId(PlayerIndex);
}

/**
 * Returns a reference to the global data store client, if it exists.
 *
 * @return	the global data store client for the game.
 */
UDataStoreClient* UUIInteraction::GetDataStoreClient()
{
	UDataStoreClient* Result = NULL;

	if ( GEngine != NULL && GEngine->GameViewport != NULL && GEngine->GameViewport->UIController != NULL )
	{
		Result = GEngine->GameViewport->UIController->DataStoreManager;
	}

	return Result;
}
void UUIInteraction::execGetDataStoreClient( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	*(UDataStoreClient**)Result = GetDataStoreClient();
}

/**
 * Plays the sound cue associated with the specified name
 *
 * @param	SoundCueName	the name of the UISoundCue to play; should corresond to one of the values of the UISoundCueNames array.
 * @param	PlayerIndex		allows the caller to indicate which player controller should be used to play the sound cue.  For the most
 *							part, all sounds can be played by the first player, regardless of who generated the play sound event.
 *
 * @return	TRUE if the sound cue specified was found in the currently active skin, even if there was no actual USoundCue associated
 *			with that UISoundCue.
 */
UBOOL UUIInteraction::PlayUISound( FName SoundCueName, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	if ( SceneClient != NULL && SceneClient->ActiveSkin != NULL )
	{
		USoundCue* UISoundCue=NULL;
		if ( SceneClient->ActiveSkin->GetUISoundCue(SoundCueName, UISoundCue) )
		{
			// indicate success
			bResult = TRUE;

			if ( UISoundCue != NULL )
			{
				// determine which PlayerController should be used for playing this sound
				APlayerController* PlayerOwner = NULL;
				if ( !GEngine->GamePlayers.IsValidIndex(PlayerIndex) )
				{
					debugf(NAME_Warning, TEXT("UUIInteraction::PlayUISound: Invalid player index specified for sound '%s': %i (%i players active).  Falling back to primary PlayerController."), *SoundCueName.ToString(), PlayerIndex, GEngine->GamePlayers.Num());
					PlayerIndex = 0;
				}

				if ( GEngine->GamePlayers.IsValidIndex(PlayerIndex) )
				{
					PlayerOwner = GEngine->GamePlayers(PlayerIndex)->Actor;

					if ( PlayerOwner != NULL )
					{
						// now play the sound
						PlayerOwner->PlaySound(UISoundCue, TRUE, TRUE, TRUE,NULL);
					}
					else
					{
						debugf(NAME_Warning, TEXT("UUIInteraction::PlayUISound: NULL PlayerController for player %i while trying to play sound '%s'."), PlayerIndex, *SoundCueName.ToString());
					}
				}
			}
		}
	}

	return bResult;
}

/**
 * Check a key event received by the viewport.
 *
 * @param	Viewport - The viewport which the key event is from.
 * @param	ControllerId - The controller which the key event is from.
 * @param	Key - The name of the key which an event occured for.
 * @param	Event - The type of event which occured.
 * @param	AmountDepressed - For analog keys, the depression percent.
 * @param	bGamepad - input came from gamepad (ie xbox controller)
 *
 * @return	True to consume the key event, false to pass it on.
 */
UBOOL UUIInteraction::InputKey(INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed/*=1.f*/,UBOOL bGamepad/*=FALSE*/)
{
	UBOOL bResult = FALSE;

	const UBOOL bIsDoubleClickKey = SupportedDoubleClickKeys.ContainsItem(Key);
	if ( bProcessInput == TRUE && SceneClient != NULL )
	{
		if ( bIsDoubleClickKey )
		{
			DOUBLE CurrentTimeInSeconds = appSeconds();
			if ( Event == IE_Pressed )
			{
				if ( SceneClient->ShouldSimulateDoubleClick() )
				{
					Event = IE_DoubleClick;
				}

				// this is the first time we're sending a repeat event for this mouse button, so set the initial repeat delay
				MouseButtonRepeatInfo.NextRepeatTime = CurrentTimeInSeconds + MouseButtonRepeatDelay * 1.5f;
				MouseButtonRepeatInfo.CurrentRepeatKey = Key;
			}
			else if ( Event == IE_Repeat )
			{
				if ( MouseButtonRepeatInfo.CurrentRepeatKey == Key )
				{
					if ( CurrentTimeInSeconds < MouseButtonRepeatInfo.NextRepeatTime )
					{
						// this key hasn't been held long enough to generate the "repeat" keypress, so just swallow the input event
						bResult = TRUE;
					}
					else
					{
						// it's time to generate another key press; subsequence repeats should take a little less time than the initial one
						MouseButtonRepeatInfo.NextRepeatTime = CurrentTimeInSeconds + MouseButtonRepeatDelay * 0.5f;
					}
				}
				else
				{
					// this is the first time we're sending a repeat event for this mouse button, so set the initial repeat delay
					MouseButtonRepeatInfo.NextRepeatTime = CurrentTimeInSeconds + MouseButtonRepeatDelay * 1.5f;
					MouseButtonRepeatInfo.CurrentRepeatKey = Key;
					Event = IE_Pressed;
				}
			}
		}

		bResult = bResult || SceneClient->InputKey(ControllerId,Key,Event,AmountDepressed,bGamepad);

		if ( bIsDoubleClickKey && (Event == IE_Pressed || Event == IE_DoubleClick) )
		{
			SceneClient->ResetDoubleClickTracking(Event == IE_DoubleClick);
		}
	}

	if ( bIsDoubleClickKey && Event == IE_Repeat && !bResult )
	{
		// don't allow IE_Repeat events to be passed to the game for mouse buttons.
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Check an axis movement received by the viewport.
 *
 * @param	Viewport - The viewport which the axis movement is from.
 * @param	ControllerId - The controller which the axis movement is from.
 * @param	Key - The name of the axis which moved.
 * @param	Delta - The axis movement delta.
 * @param	DeltaTime - The time since the last axis update.
 *
 * @return	True to consume the axis movement, false to pass it on.
 */
UBOOL UUIInteraction::InputAxis(INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime, UBOOL bGamepad)
{
	UBOOL bResult = FALSE;

	if ( bProcessInput == TRUE && SceneClient != NULL )
	{
		FUIAxisEmulationDefinition* EmulationDef = AxisEmulationDefinitions.Find(Key);

		// If this axis input event was generated by an axis we can emulate, check that it is outside the UI's dead zone.
		if ( EmulationDef != NULL && EmulationDef->bEmulateButtonPress == TRUE )
		{
			INT PlayerIndex = GetPlayerIndex(ControllerId);
			if ( PlayerIndex >= 0 && PlayerIndex < ARRAY_COUNT(AxisInputEmulation) && AxisInputEmulation[PlayerIndex].bEnabled == TRUE )
			{
				FInputEventParameters EmulatedEventParms(PlayerIndex, ControllerId, EmulationDef->InputKeyToEmulate[Delta > 0 ? 0 : 1], IE_MAX,
					IsAltDown(SceneClient->RenderViewport), IsCtrlDown(SceneClient->RenderViewport), IsShiftDown(SceneClient->RenderViewport), 1.f);

				UBOOL bValidDelta = Abs<FLOAT>(Delta) >= UIJoystickDeadZone;

				// if the current delta is within the dead-zone, and this key is set as the CurrentRepeatKey for that gamepad,
				// generate a "release" event
				if ( bValidDelta == FALSE )
				{
					// if this key was the key that was being held down, simulate the release event
					// Only signal a release if this is the last key pressed
					if ( AxisInputEmulation[PlayerIndex].CurrentRepeatKey == Key )
					{
						// change the event type to "release"
						EmulatedEventParms.EventType = IE_Released;

						// and clear the emulated repeat key for this player
						AxisInputEmulation[PlayerIndex].CurrentRepeatKey = NAME_None;
					}
					else
					{
						// otherwise, ignore and swallow it
						bResult = bCaptureUnprocessedInput;
					}
				}

				// we have a valid delta for this axis; need to determine what to do with it
				else
				{
					// if this is the same key as the current repeat key, it means the user is still holding the joystick in the same direction
					// so we'll need to determine whether enough time has passed to generate another button press event
					if ( AxisInputEmulation[PlayerIndex].CurrentRepeatKey == Key )
					{
						// we might need to simulate another "repeat" event
						EmulatedEventParms.EventType = IE_Repeat;
					}

					else
					{
						// if the new key isn't the same as the current repeat key, but the new key is another axis input, ignore it
						// this basically means that as long as we have a valid delta on one joystick axis, we're going to ignore all other joysticks for that player
						if ( AxisInputEmulation[PlayerIndex].CurrentRepeatKey != NAME_None && Key != EmulationDef->AdjacentAxisInputKey )
						{
							//@todo ronp - should this return bCaptureUnprocessedInput?
							bResult = TRUE;
						}
						else
						{
							EmulatedEventParms.EventType = IE_Pressed;
							AxisInputEmulation[PlayerIndex].CurrentRepeatKey = Key;
						}
					}
				}

				DOUBLE CurrentTimeInSeconds = appSeconds();
				if ( EmulatedEventParms.EventType == IE_Repeat )
				{
					// this key hasn't been held long enough to generate the "repeat" keypress, so just swallow the input event
					if ( CurrentTimeInSeconds < AxisInputEmulation[PlayerIndex].NextRepeatTime )
					{
						EmulatedEventParms.EventType = IE_MAX;
						bResult = TRUE;
					}
					else
					{
						// it's time to generate another key press; subsequence repeats should take a little less time than the initial one
						AxisInputEmulation[PlayerIndex].NextRepeatTime = CurrentTimeInSeconds + AxisRepeatDelay * 0.5f;
					}
				}
				else if ( EmulatedEventParms.EventType == IE_Pressed )
				{
					// this is the first time we're sending a keypress event for this axis's emulated button, so set the initial repeat delay
					AxisInputEmulation[PlayerIndex].NextRepeatTime = CurrentTimeInSeconds + AxisRepeatDelay * 1.5f;
				}

				// we're supposed to generate the emulated button input key event if the EmulatedEventParms.EventType is not IE_MAX
				if ( EmulatedEventParms.EventType != IE_MAX )
				{
					bResult = SceneClient->InputKey(ControllerId, EmulatedEventParms.InputKeyName, (EInputEvent)EmulatedEventParms.EventType, bGamepad);
				}
			}
		}

		bResult = bResult || SceneClient->InputAxis(ControllerId,Key,Delta,DeltaTime, bGamepad);
	}

	return bResult;
}

/**
 * Check a character input received by the viewport.
 *
 * @param	Viewport - The viewport which the axis movement is from.
 * @param	ControllerId - The controller which the axis movement is from.
 * @param	Character - The character.
 *
 * @return	True to consume the character, false to pass it on.
 */
UBOOL UUIInteraction::InputChar(INT ControllerId,TCHAR Character)
{
	UBOOL bResult = FALSE;

	if ( bProcessInput && SceneClient != NULL )
	{
		bResult = SceneClient->InputChar(ControllerId,Character);
	}

	return bResult;
}

/**
 * Initializes the axis button-press/release emulation map.
 */
void UUIInteraction::InitializeAxisInputEmulations()
{
	const TArray<FUIAxisEmulationDefinition>& EmulationDefinitions = UUIInputConfiguration::StaticClass()->GetDefaultObject<UUIInputConfiguration>()->AxisEmulationDefinitions;

	AxisEmulationDefinitions.Empty();
	for ( INT KeyIndex = 0; KeyIndex < EmulationDefinitions.Num(); KeyIndex++ )
	{
		const FUIAxisEmulationDefinition& Definition = EmulationDefinitions(KeyIndex);
		AxisEmulationDefinitions.Set(Definition.AxisInputKey, Definition);
	}
}

/**
 * Initializes all of the UI Input Key FNames.
 */
void UUIInteraction::InitializeUIInputAliasNames()
{
	#define DEFINE_UIKEY(Name) UIKEY_##Name = FName(TEXT(#Name));
		#include "UnUIKeys.h"
	#undef DEFINE_UIKEY
}

/**
 * Initializes all of the UI event key lookup maps.  Safe to call more than once.
 */
void UUIInteraction::InitializeInputAliasLookupTable()
{
	// clear the existing values
	WidgetInputAliasLookupTable.Empty();

	// make sure we have an input settings object
	GetInputSettings();
	check(UIInputConfig);

	// Initialize the mapping of widget class to input alias data with all of the classes which are registered in the .ini.  We must do this part first
	// so that the input alias map for each class can add the data from their parent classes as well.
	TArray<FUIInputAliasClassMap>& InputAliases = UIInputConfig->GetInputAliasList();
	for(INT MappingIdx = 0; MappingIdx < InputAliases.Num(); MappingIdx++)
	{
		FUIInputAliasClassMap& WidgetKeyMapping = InputAliases(MappingIdx);
		WidgetInputAliasLookupTable.Set(WidgetKeyMapping.WidgetClass, &WidgetKeyMapping);
	}

	// now initialize them all.
	for ( TMap<UClass*,FUIInputAliasClassMap*>::TIterator It(WidgetInputAliasLookupTable); It; ++It )
	{
		FUIInputAliasClassMap* WidgetKeyMapping = It.Value();
		WidgetKeyMapping->InitializeLookupTable(WidgetInputAliasLookupTable);
	}
}

/**
 * Load the UISkin specified by UISkinName
 *
 * @return	a pointer to the UISkin object corresponding to UISkinName, or
 *			the default UISkin if the configured skin couldn't be loaded
 */
UUISkin* UUIInteraction::LoadInitialSkin() const
{
	UUISkin* InitialSkin = NULL;
	if ( UISkinName.Len() > 0 )
	{
		InitialSkin = LoadObject<UUISkin>(NULL, *UISkinName, NULL, 0, NULL);
	}
	else
	{
		debugf(NAME_Warning, TEXT("No value specified for UIInteraction.UISkinName!"));
	}

	if ( InitialSkin == NULL )
	{
		debugf(NAME_Warning, TEXT("Failed to load configured UISkin '%s'.  Defaulting to %s"), *UISkinName, UCONST_DEFAULT_UISKIN);
		InitialSkin = LoadObject<UUISkin>(NULL, UCONST_DEFAULT_UISKIN, NULL, 0, NULL);
	}

	check(InitialSkin);
	return InitialSkin;
}

/**
 * Called once a frame to update the interaction's state.
 *
 * @param	DeltaTime - The time since the last frame.
 */
void UUIInteraction::Tick( FLOAT DeltaTime )
{
	// Only tick if enabled.
	extern UBOOL GTickAndRenderUI;
	if( GTickAndRenderUI )
	{
		SCOPE_CYCLE_COUNTER(STAT_UITickTime);

		Super::Tick(DeltaTime);

		SceneClient->Tick(DeltaTime);

		if( UsesUIPrimitiveScene() &&
			CanvasScene )
		{
			// init the UI scene for 3d primitive rendering
			//@todo ronp - move this functionality to scene client so that we can do this on a per-scene basis instead of updating all scenes anytime
			// any scene changes.
			if( NeedsInitUIPrimitiveScene() )
			{
				InitUIPrimitiveScene(CanvasScene);
			}
			// update the 3d primitive scene
			UpdateUIPrimitiveScene(CanvasScene);
		}
	}
}

/**
 * Notifies the scene client to render all scenes
 */
void UUIInteraction::RenderUI( FCanvas* Canvas )
{
	SceneClient->RenderScenes(Canvas);
}

/**
 * Returns the CDO for the configured scene client class.
 */
UGameUISceneClient* UUIInteraction::GetDefaultSceneClient() const
{
	check(SceneClientClass);

	return SceneClientClass->GetDefaultObject<UGameUISceneClient>();
}

/**
 * Returns the UIInputConfiguration singleton, creating one if necessary.
 */
UUIInputConfiguration* UUIInteraction::GetInputSettings()
{
	if ( UIInputConfig == NULL )
	{
		UIInputConfig = ConstructObject<UUIInputConfiguration>(UUIInputConfiguration::StaticClass(), INVALID_OBJECT, NAME_None, RF_Transient);
		UIInputConfig->LoadInputAliasClasses();
	}

	return UIInputConfig;
}

/**
 * Handles processing console commands.
 * 
 * @param	Cmd		the text that was entered into the console
 * @param	Ar		the archive to use for serializing responses
 *
 * @return	TRUE if the command specified was processed
 */
UBOOL UUIInteraction::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	if ( ScriptConsoleExec(Cmd, Ar, this) )
	{
		return TRUE;
	}
	else if ( SceneClient->Exec(Cmd, Ar) )
	{
		return TRUE;
	}

	return FALSE;
}

//@warning: this function's return value is marked 'coerce' in script, so this must return a UUIObject of class WidgetClass
UUIObject* UUIInteraction::CreateTransientWidget(UClass* WidgetClass, FName WidgetTag, UUIObject* Owner)
{
	return SceneClient->CreateTransientWidget(WidgetClass, WidgetTag, Owner);
}

//@warning: this function's return value is marked 'coerce' in script, so this must return a UUIScene of class SceneClass
UUIScene* UUIInteraction::CreateScene(UClass* SceneClass, FName SceneTag)
{
	if (SceneClass == NULL)
	{
		debugf(NAME_Warning, TEXT("CreateScene called with NULL SceneClass - defaulting to UIScene"));
		SceneClass = UUIScene::StaticClass();
	}
	return SceneClient->CreateScene(SceneClass->GetDefaultObject<UUIScene>(), GetTransientPackage(), SceneTag);
}

/**
 * Returns if this UI requires a CanvasScene for rendering 3D primitives
 *
 * @return TRUE if 3D primitives are used
 */
UBOOL UUIInteraction::UsesUIPrimitiveScene() const
{
	return SceneClient->IsUIActive(UUISceneClient::SCENEFILTER_PrimitiveUsersOnly);
}

/**
 * Returns the internal CanvasScene that may be used by this UI
 *
 * @return canvas scene or NULL
 */
FCanvasScene* UUIInteraction::GetUIPrimitiveScene()
{
	return CanvasScene;	
}

/**
 * Determine if the canvas scene for primitive rendering needs to be initialized
 *
 * @return TRUE if InitUIPrimitiveScene should be called
 */
UBOOL UUIInteraction::NeedsInitUIPrimitiveScene()
{
	return !bIsUIPrimitiveSceneInitialized;
}

/**
 * Setup a canvas scene by adding primitives and lights to it from this UI
 *
 * @param InCanvasScene - scene for rendering 3D prims
 */
void UUIInteraction::InitUIPrimitiveScene( FCanvasScene* InCanvasScene )
{
	check(InCanvasScene);
	
	// mark as initialized
	bIsUIPrimitiveSceneInitialized = TRUE;

	// initialize scene
	InCanvasScene->DetachAllComponents();

	// attach components to the scene.
	SceneClient->InitializePrimitives(InCanvasScene);
}

/**
 * Updates the actor components in the canvas scene
 *
 * @param InCanvasScene - scene for rendering 3D prims
 */
void UUIInteraction::UpdateUIPrimitiveScene( FCanvasScene* InCanvasScene )
{
	check(InCanvasScene);
	checkSlow(SceneClient);

	// route to scene client for updates
	SceneClient->UpdateActivePrimitives(InCanvasScene);
}

