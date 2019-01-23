/**
 * UISceneClient used when playing a game.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class GameUISceneClient extends UISceneClient
	within UIInteraction
	native(UIPrivate)
	config(UI);

/**
 * the list of scenes currently open.  A scene corresponds to a top-level UI construct, such as a menu or HUD
 * There is always at least one scene in the stack - the transient scene.  The transient scene is used as the
 * container for all widgets created by unrealscript and is always rendered last.
 */
var	const	transient 							array<UIScene>		ActiveScenes;

/**
 * The mouse cursor that is currently being used.  Updated by scenes & widgets as they change states by calling ChangeMouseCursor.
 */
var	const	transient							UITexture			CurrentMouseCursor;

/**
 * Determines whether the cursor should be rendered.  Set by UpdateMouseCursor()
 */
var	const	transient							bool				bRenderCursor;

/** Cached DeltaTime value from the last Tick() call */
var	const	transient							float				LatestDeltaTime;

/** The time (in seconds) that the last "key down" event was recieved from a key that can trigger double-click events */
var	const	transient							float				DoubleClickStartTime;

/**
 * The location of the mouse the last time a key press was received.  Used to determine when to simulate a double-click
 * event.
 */
var const	transient							IntPoint			DoubleClickStartPosition;

/** Textures for general use by the UI */
var	const	transient							Texture				DefaultUITexture[EUIDefaultPenColor];

/** Controls whether debug information about the scene is rendered */
var	config										bool				bRenderDebugInfo;

/**
 * map of controllerID to list of keys which were pressed when the UI began processing input
 * used to ignore the initial "release" key event from keys which were already pressed when the UI began processing input.
 */
var	const	transient	native					Map_Mirror			InitialPressedKeys{TMap<INT,TArray<FName> >};

/**
 * Indicates that the input processing status of the UI has potentially changed; causes UpdateInputProcessingStatus to be called
 * in the next Tick().
 */
var	const	transient							bool				bUpdateInputProcessingStatus;

/**
* Indicates that the input processing status of the UI has potentially changed; causes UpdateCursorRenderStatus to be called
* in the next Tick().
*/
var const	transient							bool				bUpdateCursorRenderStatus;

/** Holds a list of all available animations for an object */
var transient array<UIAnimationSeq> AnimSequencePool;

/** Holds a list of UIObjects that have animations being applied to them */
var transient array<UIObject> AnimSubscribers;


cpptext
{
	/* =======================================
		FExec interface
	======================================= */
	virtual UBOOL Exec(const TCHAR* Cmd,FOutputDevice& Ar);

	/* === FCallbackEventDevice interface === */
	/**
	 * Called when the viewport has been resized.
	 */
	virtual void Send( ECallbackEventType InType, class FViewport* InViewport, UINT InMessage);

	/* =======================================
		UUISceneClient interface
	======================================= */
	/**
	 * Performs any initialization for the UISceneClient.
	 *
	 * @param	InitialSkin		UISkin that should be set to the initial ActiveSkin
	 */
	virtual void InitializeClient( UUISkin* InitialSkin );

	/**
	 * Changes the active skin to the skin specified, initializes the skin and performs all necessary cleanup and callbacks.
	 * This method should only be called from script.
	 *
	 * @param	NewActiveScene	The skin to activate
	 *
	 * @return	TRUE if the skin was successfully changed.
	 */
	virtual UBOOL ChangeActiveSkin( UUISkin* NewActiveSkin );

	/**
	 * Refreshes all existing UI elements with the styles from the currently active skin.
	 */
	virtual void OnActiveSkinChanged();

	/**
	 * Assigns the viewport that scenes will use for rendering.
	 *
	 * @param	inViewport	the viewport to use for rendering scenes
	 */
	virtual void SetRenderViewport( FViewport* SceneViewport );

	/**
	 * Retrieves the point of origin for the viewport for the scene specified.  This should always be 0,0 during the game,
	 * but may be different in the UI editor if the editor window is configured to have a gutter around the viewport.
	 *
	 * @param	out_ViewportOrigin	[out] will be filled in with the position of the starting point of the viewport.
	 *
	 * @return	TRUE if the viewport origin was successfully retrieved
	 */
	virtual UBOOL GetViewportOrigin( const UUIScene* Scene, FVector2D& out_ViewportOrigin );

	/**
	 * Retrieves the size of the viewport for the scene specified.
	 *
	 * @param	out_ViewportSize	[out] will be filled in with the width & height that the scene should use as the viewport size
	 *
	 * @return	TRUE if the viewport size was successfully retrieved
	 */
	virtual UBOOL GetViewportSize( const UUIScene* Scene, FVector2D& out_ViewportSize );

	/**
	 * Recalculates the matrix used for projecting local coordinates into screen (normalized device)
	 * coordinates.  This method should be called anytime the viewport size or origin changes.
	 */
	virtual void UpdateCanvasToScreen();

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
	virtual UBOOL InputKey(INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed=1.f,UBOOL bGamepad=FALSE);

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
	virtual UBOOL InputAxis(INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime, UBOOL bGamepad=FALSE);

	/**
	 * Check a character input received by the viewport.
	 *
	 * @param	Viewport - The viewport which the axis movement is from.
	 * @param	ControllerId - The controller which the axis movement is from.
	 * @param	Character - The character.
	 *
	 * @return	True to consume the character, false to pass it on.
	 */
	virtual UBOOL InputChar(INT ControllerId,TCHAR Character);

	/* =======================================
		UGameUISceneClient interface
	======================================= */
	/**
	 * Creates the scene that will be used to contain transient widgets that are created in unrealscript
	 */
	void CreateTransientScene();

	/**
	 * Creates a new instance of the scene class specified.
	 *
	 * @param	SceneTemplate	the template to use for the new scene
	 * @param	InOuter			the outer for the scene
	 * @param	SceneTag		if specified, the scene will be given this tag when created
	 *
	 * @return	a UIScene instance of the class specified
	 */
	 UUIScene* CreateScene( UUIScene* SceneTemplate, UObject* InOuter, FName SceneTag = NAME_None );

	/**
	 * Determines which widget is currently under the mouse cursor by performing hit tests against bounding regions.
	 */
	void UpdateActiveControl();

	/**
	 * Resets the time and mouse position values used for simulating double-click events to the current value or invalid values.
	 */
	void ResetDoubleClickTracking( UBOOL bClearValues );

	/**
	 * Checks the current time and mouse position to determine whether a double-click event should be simulated.
	 */
	UBOOL ShouldSimulateDoubleClick() const;

	/**
	 * Set the mouse position to the coordinates specified
	 *
	 * @param	NewX	the X position to move the mouse cursor to (in pixels)
	 * @param	NewY	the Y position to move the mouse cursor to (in pixels)
	 */
	virtual void SetMousePosition( INT NewMouseX, INT NewMouseY );

	/**
	 * Sets the values of MouseX & MouseY to the current position of the mouse
	 */
	virtual void UpdateMousePosition();

	/**
	 * Gets the size (in pixels) of the mouse cursor current in use.
	 *
	 * @return	TRUE if MouseXL/YL were filled in; FALSE if there is no mouse cursor or if the UI is configured to not render a mouse cursor.
	 */
	virtual UBOOL GetCursorSize( FLOAT& MouseXL, FLOAT& MouseYL );

	/**
	 * Called whenever a scene is added or removed from the list of active scenes.  Calls any functions that handle updating the
	 * status of various tracking variables, such as whether the UI is currently capable of processing input.
	 */
	virtual void SceneStackModified();

	/**
	 * Changes the resource that is currently being used as the mouse cursor.  Called by widgets as they changes states which
	 * affect the mouse cursor
	 *
	 * @param	CursorName	the name of the mouse cursor resource to use.  Should correspond to a name from the active UISkin's
	 *						MouseCursorMap
	 *
	 * @return	TRUE if the cursor was successfully changed.  FALSE if the cursor name was invalid or wasn't found in the current
	 *			skin's MouseCursorMap
	 */
	virtual UBOOL ChangeMouseCursor( FName CursorName );

	/**
	 * Adds the specified scene to the list of active scenes, loading the scene and performing initialization as necessary.
	 *
	 * @param	Scene			the scene to open; if the scene specified is contained in a content package, a copy of the scene will be created
	 *							and the copy will be opened instead.
	 * @param	SceneOwner		the player that should be associated with the new scene.  Will be assigned to the scene's
	 *							PlayerOwner property.
	 * @param	OpenedScene		the scene that was actually opened.  If Scene is located in a content package, OpenedScene will be
	 *							the copy of the scene that was created.  Otherwise, OpenedScene will be the same as the scene passed in.
	 *
	 * @return TRUE if the scene was successfully opened
	 */
	virtual UBOOL OpenScene( class UUIScene* Scene, class ULocalPlayer* SceneOwner=NULL, class UUIScene** OpenedScene=NULL );

	/**
	 * Removes the specified scene from the ActiveScenes array.
	 *
	 * @param	Scene	the scene to deactivate
	 *
	 * @return true if the scene was successfully removed from the ActiveScenes array
	 */
	virtual UBOOL CloseScene( UUIScene* Scene );

	/**
	 * Called once a frame to update the UI's state.
	 *
	 * @param	DeltaTime - The time since the last frame.
	 */
	virtual void Tick(FLOAT DeltaTime);

	/**
	 * Render all the active scenes
	 */
	virtual void RenderScenes( FCanvas* Canvas );

	/**
	 * Re-initializes all primitives in the specified scene.  Will occur on the next tick.
	 *
	 * @param	Sender	the scene to re-initialize primitives for.
	 */
	virtual void RequestPrimitiveReinitialization( class UUIScene* Sender );

	/**
	 * Gives all UIScenes a chance to create, attach, and/or initialize any primitives contained in the UIScene.
	 *
	 * @param	CanvasScene		the scene to use for attaching any 3D primitives
	 */
	virtual void InitializePrimitives( class FCanvasScene* CanvasScene );

	/**
	 * Updates 3D primitives for all active scenes
	 *
	 * @param	CanvasScene		the scene to use for attaching any 3D primitives
	 */
	virtual void UpdateActivePrimitives( class FCanvasScene* CanvasScene );

	/**
	 * Returns true if there is an unhidden fullscreen UI active
	 *
	 * @param	Flags	modifies the logic which determines wether hte UI is active
	 *
	 * @return TRUE if the UI is currently active
	 */
	virtual UBOOL IsUIActive( DWORD Flags=0 ) const;

	/**
	 * Returns whether the specified scene has been fully initialized.  Different from UUIScene::IsInitialized() in that this
	 * method returns true only once all objects related to this scene have been created and initialized (e.g. in the UI editor
	 * only returns TRUE once the editor window for this scene has finished creation).
	 *
	 * @param	Scene	the scene to check.
	 */
	virtual UBOOL IsSceneInitialized( const class UUIScene* Scene ) const;

protected:

	/**
	 * Gets the list of scenes that should be rendered.  Some active scenes might not be rendered if scenes later in the
	 * scene stack prevent it, for example.
	 */
	void GetScenesToRender( TArray<UUIScene*>& ScenesToRender );

	/**
	 * Adds the specified scene to the list of active scenes.
	 *
	 * @param	SceneToActivate		the scene to activate
	 */
	virtual void ActivateScene( UUIScene* SceneToActivate );

	/**
	 * Removes the specified scene from the list of active scenes.  If this scene is not the top-most scene, all
	 * scenes which occur after the specified scene in the ActiveScenes array will be deactivated as well.
	 *
	 * @param	SceneToDeactivate	the scene to remove
	 *
	 * @return	TRUE if the scene was successfully removed from the list of active scenes.
	 */
	virtual UBOOL DeactivateScene( UUIScene* SceneToDeactivate );

	/**
	 * Searches all scenes to determine if any are configured to display a cursor.  Sets the value of bRenderCursor accordingly.
	 */
	virtual void UpdateCursorRenderStatus();

	/**
	 * Updates the value of UIInteraction.bProcessingInput to reflect whether any scenes are capable of processing input.
	 */
	void UpdateInputProcessingStatus();

	/**
	 * Ensures that the game's paused state is appropriate considering the state of the UI.  If any scenes are active which require
	 * the game to be paused, pauses the game...otherwise, unpauses the game.
	 */
	virtual void UpdatePausedState();

	/**
	 * Clears the arrays of pressed keys for all local players in the game; used when the UI begins processing input.  Also
	 * updates the InitialPressedKeys maps for all players.
	 */
	void FlushPlayerInput();

	/**
	 * Renders debug information to the screen canvas.
	 */
	virtual void RenderDebugInfo( FCanvas* Canvas );

public:
}

/* == Delegates == */

/* == Natives == */

/**
 * Get a reference to the transient scene, which is used to contain transient widgets that are created by unrealscript
 *
 * @return	pointer to the UIScene that owns transient widgets
 */
native final function UIScene GetTransientScene() const;

/**
 * Creates an instance of the scene class specified.  Used to create scenes from unrealscript.  Does not initialize
 * the scene - you must call activate scene
 *
 * @param	SceneClass	the scene class to open
 * @param	SceneTag	if specified, the scene will be given this tag when created
 *
 * @return	a UIScene instance of the class specified
 */
native final noexport function coerce UIScene CreateScene( class<UIScene> SceneClass, optional name SceneTag );

/**
 * Create a temporary widget for presenting data from unrealscript
 *
 * @param	WidgetClass		the widget class to create
 * @param	WidgetTag		the tag to assign to the widget.
 * @param	Owner			the UIObject that should contain the widget
 *
 * @return	a pointer to a fully initialized widget of the class specified, contained within the transient scene
 * @todo - add support for metacasting using a property flag (i.e. like spawn auto-casts the result to the appropriate type)
 */
native final function coerce UIObject CreateTransientWidget( class<UIObject> WidgetClass, Name WidgetTag, optional UIObject Owner );

/**
 * Searches through the ActiveScenes array for a UIScene with the tag specified
 *
 * @param	SceneTag	the name of the scene to locate
 * @param	SceneOwner	if specified, only scenes that have the specified SceneOwner will be considered.
 *
 * @return	pointer to the UIScene that has a SceneName matching SceneTag, or NULL if no scenes in the ActiveScenes
 *			stack have that name
 */
native final function UIScene FindSceneByTag( name SceneTag, optional LocalPlayer SceneOwner ) const;

/**
 * Triggers a call to UpdateInputProcessingStatus on the next Tick().
 */
native final function RequestInputProcessingUpdate();

/**
 * Triggers a call to UpdateCursorRenderStatus on the next Tick().
 */
native final function RequestCursorRenderUpdate();

/**
 * Callback which allows the UI to prevent unpausing if scenes which require pausing are still active.
 * @see PlayerController.SetPause
 */
native final function bool CanUnpauseInternalUI();

/**
 * Changes this scene client's ActiveControl to the specified value, which might be NULL.  If there is already an ActiveControl
 *
 * @param	NewActiveControl	the widget that should become to ActiveControl, or NULL to clear the ActiveControl.
 *
 * @return	TRUE if the ActiveControl was updated successfully.
 */
native function bool SetActiveControl( UIObject NewActiveControl );

/* == Events == */

/**
 * Toggles the game's paused state if it does not match the desired pause state.
 *
 * @param	bDesiredPauseState	TRUE indicates that the game should be paused.
 */
event ConditionalPause( bool bDesiredPauseState )
{
	local PlayerController PlayerOwner;

	if ( GamePlayers.Length > 0 )
	{
		PlayerOwner = GamePlayers[0].Actor;
		if ( PlayerOwner != None )
		{
			if ( bDesiredPauseState != PlayerOwner.IsPaused() )
			{
				PlayerOwner.SetPause(bDesiredPauseState, CanUnpauseInternalUI);
			}
		}
	}
}

/**
 * Returns whether widget tooltips should be displayed.
 */
event bool CanShowToolTips()
{
	// if tooltips are disabled globally, can't show them
	if ( bDisableToolTips )
		return false;

	// the we're currently dragging a slider or resizing a list column or something, don't display tooltips
//	if ( ActivePage != None && ActivePage.bCaptureMouse )
//		return false;
//
	// if we're currently in the middle of a drag-n-drop operation, don't show tooltips
//	if ( DropSource != None || DropTarget != None )
//		return false;

	return true;
}

/* == Unrealscript == */

/**
 * Called when the current map is being unloaded.  Cleans up any references which would prevent garbage collection.
 */
function NotifyGameSessionEnded()
{
	local int i;
	local array<UIScene> CurrentlyActiveScenes;

	// copy the list of active scenes into a temporary array in case scenes start removing themselves when
	// they receive this notification
	CurrentlyActiveScenes = ActiveScenes;

	// starting with the most recently opened scene (or the top-most, anyway), notify them all that the
	// map is about to change
	for ( i = CurrentlyActiveScenes.Length - 1; i >= 0; i-- )
	{
		if ( CurrentlyActiveScenes[i] != None )
		{
			CurrentlyActiveScenes[i].NotifyGameSessionEnded();
		}
	}
}

/**
 * Called when a new player has been added to the list of active players (i.e. split-screen join)
 *
 * @param	PlayerIndex		the index [into the GamePlayers array] where the player was inserted
 * @param	AddedPlayer		the player that was added
 */
function NotifyPlayerAdded( int PlayerIndex, LocalPlayer AddedPlayer )
{
	local int SceneIndex;

	// notify all currently active scenes about the new player
	for ( SceneIndex = 0; SceneIndex < ActiveScenes.Length; SceneIndex++ )
	{
		ActiveScenes[SceneIndex].CreatePlayerData(PlayerIndex, AddedPlayer);
	}
}

/**
 * Called when a player has been removed from the list of active players (i.e. split-screen players)
 *
 * @param	PlayerIndex		the index [into the GamePlayers array] where the player was located
 * @param	RemovedPlayer	the player that was removed
 */
function NotifyPlayerRemoved( int PlayerIndex, LocalPlayer RemovedPlayer )
{
	local int SceneIndex;

	// notify all currently active scenes about the player removal
	for ( SceneIndex = 0; SceneIndex < ActiveScenes.Length; SceneIndex++ )
	{
		ActiveScenes[SceneIndex].RemovePlayerData(PlayerIndex, RemovedPlayer);
	}
}

/**
 * @return Returns the top scene on the stack.
 */
function UIScene GetTopmostScene()
{
	if(ActiveScenes.Length > 0)
	{
		return ActiveScenes[ActiveScenes.Length-1];
	}
	else
	{
		return None;
	}
}

exec function TestLabel( string LabelText, int PosX = 200, int PosY = 200, int Width = 400, int Height = 400 )
{
	local UIScene TransientScene;
	local UILabel Label;

	TransientScene = GetTransientScene();
	Label = UILabel(TransientScene.FindChild('TempLabel'));

	if ( Label == None )
	{
		Label = CreateTransientWidget(class'UILabel', 'TempLabel');
	}

	Label.Position.ScaleType[EUIWidgetFace.UIFACE_Left] = EVALPOS_PixelOwner;
	Label.Position.ScaleType[EUIWidgetFace.UIFACE_Top] = EVALPOS_PixelOwner;
	Label.Position.ScaleType[EUIWidgetFace.UIFACE_Right] = EVALPOS_PixelOwner;
	Label.Position.ScaleType[EUIWidgetFace.UIFACE_Bottom] = EVALPOS_PixelOwner;

	Label.SetValue(LabelText);
	Label.Position.Value[EUIWidgetFace.UIFACE_Left] = PosX;
	Label.Position.Value[EUIWidgetFace.UIFACE_Top] = PosY;
	Label.Position.Value[EUIWidgetFace.UIFACE_Right] = Width;
	Label.Position.Value[EUIWidgetFace.UIFACE_Bottom] = Height;
}

exec function ShowDockingStacks()
{
	local int i;

	for ( i = 0; i < ActiveScenes.Length; i++ )
	{
		ActiveScenes[i].LogDockingStack();
	}
}

exec function ShowRenderBounds()
{
	local int i;

	for ( i = 0; i < ActiveScenes.Length; i++ )
	{
		ActiveScenes[i].LogRenderBounds(0);
	}
}

exec function ShowMenuStates()
{
	local int i;

	for ( i = 0; i < ActiveScenes.Length; i++ )
	{
		ActiveScenes[i].LogCurrentState(0);
	}
}

exec function CreateMenu( class<UIScene> SceneClass, optional int PlayerIndex=INDEX_NONE )
{
	local UIScene Scene;
	local LocalPlayer SceneOwner;

	`log("Attempting to create script menu '" $ SceneClass $"'");

	Scene = CreateScene(SceneClass);
	if ( Scene != None )
	{
		if ( PlayerIndex != INDEX_NONE )
		{
			SceneOwner = GamePlayers[PlayerIndex];
		}

		OpenScene(Scene, SceneOwner);
	}
	else
	{
		`log("Failed to create menu '" $ SceneClass $"'");
	}
}

exec function OpenMenu( string MenuPath, optional int PlayerIndex=INDEX_NONE )
{
	local UIScene Scene;
	local LocalPlayer SceneOwner;

	`log("Attempting to load menu by name '" $ MenuPath $"'");
	Scene = UIScene(DynamicLoadObject(MenuPath, class'UIScene'));
	if ( Scene != None )
	{
		if ( PlayerIndex != INDEX_NONE )
		{
			SceneOwner = GamePlayers[PlayerIndex];
		}

		OpenScene(Scene,SceneOwner);
	}
	else
	{
		`log("Failed to load menu '" $ MenuPath $"'");
	}
}

exec function CloseMenu( name SceneName )
{
	local int i;
	for ( i = 0; i < ActiveScenes.Length; i++ )
	{
		if ( ActiveScenes[i].SceneTag == SceneName )
		{
			`log("Closing scene '"$ ActiveScenes[i].GetWidgetPathName() $ "'");
			CloseScene(ActiveScenes[i]);
			return;
		}
	}

	`log("No scenes found in ActiveScenes array with name matching '"$SceneName$"'");
}

/**
 * Debug console command for dumping all registered data stores to the log
 *
 * @param	bFullDump	specify TRUE to show detailed information about each registered data store.
 */
exec function ShowDataStores( optional bool bVerbose )
{
	`log("Dumping data store info to log - if you don't see any results, you probably need to unsuppress DevDataStore");

	if ( DataStoreManager != None )
	{
		DataStoreManager.DebugDumpDataStoreInfo(bVerbose);
	}
	else
	{
		`log(Self @ "has a NULL DataStoreManager!",,'DevDataStore');
	}
}


/**
 * Returns the currently active scene
 */
function UIScene GetActiveScene()
{
	if (ActiveScenes.Length > 0)
	{
		return ActiveScenes[ActiveScenes.Length-1];
	}
	return none;
}


// ===============================================
// ANIMATIONS
// ===============================================


/**
 * Subscribes a UIObject so that it will receive TickAnim calls
 */
function AnimSubscribe(UIObject Target)
{
	local int i;
	i = AnimSubscribers.Find(Target);
	if (i == INDEX_None )
	{
		AnimSubscribers[AnimSubscribers.Length] = Target;
		`log("### Subscribed:"@Target);
	}
}

/**
 * UnSubscribe a UIObject so that it will receive TickAnim calls
 */
function AnimUnSubscribe(UIObject Target)
{
	local int i;
	i = AnimSubscribers.Find(Target);
	if (i != INDEX_None )
	{
		AnimSubscribers.Remove(i,1);
		`log("### UnSubscribed"@Target);
	}
}

/**
 * Attempt to find an animation in the AnimSequencePool.
 *
 * @Param SequenceName		The sequence to find
 * @returns the sequence if it was found otherwise returns none
 */
function UIAnimationSeq AnimLookupSequence(name SequenceName)
{
	local int i;
	for (i=0;i<AnimSequencePool.Length;i++)
	{
		if ( AnimSequencePool[i].SeqName == SequenceName )
		{
			return AnimSequencePool[i];
		}
	}
	return none;
}

DefaultProperties
{
	DefaultUITexture(UIPEN_White)=Texture2D'EngineResources.White'
	DefaultUITexture(UIPEN_Black)=Texture2D'EngineResources.Black'
	DefaultUITexture(UIPEN_Grey)=Texture2D'EngineResources.Gray'
}
