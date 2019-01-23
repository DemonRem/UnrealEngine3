/**
 * A game viewport (FViewport) is a high-level abstract interface for the
 * platform specific rendering, audio, and input subsystems.
 * GameViewportClient is the engine's interface to a game viewport.
 * Exactly one GameViewportClient is created for each instance of the game.  The
 * only case (so far) where you might have a single instance of Engine, but
 * multiple instances of the game (and thus multiple GameViewportClients) is when
 * you have more than PIE window running.
 *
 * Responsibilities:
 * propagating input events to the global interactions list
 *
 *
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class GameViewportClient extends Object
	within Engine
	transient
	native
	DependsOn(Interaction)
	Inherits(FViewportClient)
	Inherits(FExec);

`include(Core/Globals.uci)

/** The platform-specific viewport which this viewport client is attached to. */
var const pointer Viewport{FViewport};

/** The platform-specific viewport frame which this viewport is contained by. */
var const pointer ViewportFrame{FViewportFrame};

/** A list of interactions which have a chance at all input before the player's interactions. */
var init array<Interaction> GlobalInteractions;

/** The class for the UI controller */
var	class<UIInteraction>	UIControllerClass;

/** The viewport's UI controller */
var UIInteraction			UIController;

/** The viewport's console.   Might be null on consoles */
var Console ViewportConsole;

/** The show flags used by the viewport's players. */
var const qword ShowFlags;

/** @name Localized transition messages. */
//@{
var localized string LoadingMessage;
var localized string SavingMessage;
var localized string ConnectingMessage;
var localized string PausedMessage;
var localized string PrecachingMessage;
//@}

/** if TRUE then the title safe border is drawn */
var bool bShowTitleSafeZone;
/** Max/Recommended screen viewable extents as a percentage */
struct native TitleSafeZoneArea
{
	var float MaxPercentX;
	var float MaxPercentY;
	var float RecommendedPercentX;
	var float RecommendedPercentY;
};
/** border of safe area */
var TitleSafeZoneArea TitleSafeZone;

/**
 * Indicates whether the UI is currently displaying a mouse cursor.  Prevents GameEngine::Tick() from recapturing
 * mouse input while the UI has active scenes that mouse input.
 */
var	transient	bool		bDisplayingUIMouseCursor;

/**
 * Indicates that the UI needs to receive all mouse input events.  Usually enabled when the user is interacting with a
 * draggable widget, such as a scrollbar or slider.
 */
var	transient	bool		bUIMouseCaptureOverride;

/**
 * Enum of the different splitscreen types
 */
enum ESplitScreenType
{
	eSST_NONE,				// No split
	eSST_2P_HORIZONTAL,		// 2 player horizontal split
	eSST_2P_VERTICAL,		// 2 player vertical split
	eSST_3P_FAVOR_TOP,		// 3 Player split with 1 player on top and 2 on bottom
	eSST_3P_FAVOR_BOTTOM,	// 3 Player split with 1 player on bottom and 2 on top
	eSST_4P,				// 4 Player split
	eSST_COUNT,
	eSST_NOVALUE,
};

/**
 * The 4 different kinds of safezones
 */
enum ESafeZoneType
{
	eSZ_TOP,
	eSZ_BOTTOM,
	eSZ_LEFT,
	eSZ_RIGHT,
};

/**
 * Structure to store splitscreen data.
 */
struct native PerPlayerSplitscreenData
{
	var float SizeX;
	var float SizeY;
	var float OriginX;
	var float OriginY;
};

/**
 * Structure containing all the player splitscreen datas per splitscreen configuration.
 */
struct native SplitscreenData
{
	var array<PerPlayerSplitscreenData> PlayerData;
};

/** Array of the screen data needed for all the different splitscreen configurations */
var array<SplitscreenData> SplitscreenInfo;
/** Current splitscreen type being used */
var ESplitScreenType SplitscreenType;
/** Defaults for intances where there are multiple configs for a certain number of players */
var const ESplitScreenType Default2PSplitType;
var const ESplitScreenType Default3PSplitType;

/** set to disable world rendering */
var bool bDisableWorldRendering;

cpptext
{
	// Constructor.
	UGameViewportClient();

	/**
	 * Cleans up all rooted or referenced objects created or managed by the GameViewportClient.  This method is called
	 * when this GameViewportClient has been disassociated with the game engine (i.e. is no longer the engine's GameViewport).
	 */
	virtual void DetachViewportClient();

	/**
	 * Called every frame to allow the game viewport to update time based state.
	 * @param	DeltaTime	The time since the last call to Tick.
	 */
	void Tick( FLOAT DeltaTime );

	// FViewportClient interface.
	virtual void RedrawRequested(FViewport* InViewport) {}

	/**
	 * Routes an input key event received from the viewport to the Interactions array for processing.
	 *
	 * @param	Viewport		the viewport the input event was received from
	 * @param	ControllerId	gamepad/controller that generated this input event
	 * @param	Key				the name of the key which an event occured for (KEY_Up, KEY_Down, etc.)
	 * @param	EventType		the type of event which occured (pressed, released, etc.)
	 * @param	AmountDepressed	(analog keys only) the depression percent.
	 * @param	bGamepad - input came from gamepad (ie xbox controller)
	 *
	 * @return	TRUE to consume the key event, FALSE to pass it on.
	 */
	virtual UBOOL InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent EventType,FLOAT AmountDepressed=1.f,UBOOL bGamepad=FALSE);

	/**
	 * Routes an input axis (joystick, thumbstick, or mouse) event received from the viewport to the Interactions array for processing.
	 *
	 * @param	Viewport		the viewport the input event was received from
	 * @param	ControllerId	the controller that generated this input axis event
	 * @param	Key				the name of the axis that moved  (KEY_MouseX, KEY_XboxTypeS_LeftX, etc.)
	 * @param	Delta			the movement delta for the axis
	 * @param	DeltaTime		the time (in seconds) since the last axis update.
	 *
	 * @return	TRUE to consume the axis event, FALSE to pass it on.
	 */
	virtual UBOOL InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime, UBOOL bGamepad=FALSE);

	/**
	 * Routes a character input event (typing) received from the viewport to the Interactions array for processing.
	 *
	 * @param	Viewport		the viewport the input event was received from
	 * @param	ControllerId	the controller that generated this character input event
	 * @param	Character		the character that was typed
	 *
	 * @return	TRUE to consume the key event, FALSE to pass it on.
	 */
	virtual UBOOL InputChar(FViewport* Viewport,INT ControllerId,TCHAR Character);

	/** Returns the platform specific forcefeedback manager associated with this viewport */
	virtual class UForceFeedbackManager* GetForceFeedbackManager(INT ControllerId);

	/**
 	 * @return whether or not this Controller has Tilt Turned on
	 **/
	virtual UBOOL IsControllerTiltActive() const;

	/**
	 * sets whether or not the Tilt functionality is turned on
	 **/
	virtual void SetControllerTiltActive( UBOOL bActive );

	/**
	 * sets whether or not to ONLY use the tilt input controls
	 **/
	virtual void SetOnlyUseControllerTiltInput( UBOOL bActive );

	/**
	 * sets whether or not to use the tilt forward and back input controls
	 **/
	virtual void SetUseTiltForwardAndBack( UBOOL bActive );

	/**
	 * Updates the current mouse capture state to reflect the state of the game.
	 */
	void UpdateMouseCapture();

	/**
	 * Changes the value of bUIMouseCaptureOverride.
	 */
	FORCEINLINE void SetMouseCaptureOverride( UBOOL bOverride )
	{
		bUIMouseCaptureOverride = bOverride;
	}

	/**
	 * Retrieves the cursor that should be displayed by the OS
	 *
	 * @param	Viewport	the viewport that contains the cursor
	 * @param	X			the x position of the cursor
	 * @param	Y			the Y position of the cursor
	 *
	 * @return	the cursor that the OS should display
	 */
	virtual EMouseCursor GetCursor( FViewport* Viewport, INT X, INT Y );

	virtual void Precache();
	virtual void Draw(FViewport* Viewport,FCanvas* Canvas);
	virtual void LostFocus(FViewport* Viewport);
	virtual void ReceivedFocus(FViewport* Viewport);
	virtual void CloseRequested(FViewport* Viewport);
	virtual UBOOL RequiresHitProxyStorage() { return 0; }

	/**
	 * Determines whether this viewport client should receive calls to InputAxis() if the game's window is not currently capturing the mouse.
	 * Used by the UI system to easily receive calls to InputAxis while the viewport's mouse capture is disabled.
	 */
	virtual UBOOL RequiresUncapturedAxisInput() const;

	// FExec interface.
	virtual UBOOL Exec(const TCHAR* Cmd,FOutputDevice& Ar);

	/**
	 * Set this GameViewportClient's viewport to the viewport specified
	 */
	virtual void SetViewport( FViewportFrame* InViewportFrame );

}

/**
 * Provides script-only child classes the opportunity to handle input key events received from the viewport.
 * This delegate is called before the input key event is passed to the interactions array for processing.
 *
 * @param	ControllerId	the controller that generated this input key event
 * @param	Key				the name of the key which an event occured for (KEY_Up, KEY_Down, etc.)
 * @param	EventType		the type of event which occured (pressed, released, etc.)
 * @param	AmountDepressed	for analog keys, the depression percent.
 * @param	bGamepad		input came from gamepad (ie xbox controller)
 *
 * @return	return TRUE to indicate that the input event was handled.  if the return value is TRUE, this input event will not
 *			be passed to the interactions array.
 */
delegate bool HandleInputKey( int ControllerId, name Key, EInputEvent EventType, float AmountDepressed, optional bool bGamepad );

/**
 * Provides script-only child classes the opportunity to handle input axis events received from the viewport.
 * This delegate is called before the input axis event is passed to the interactions array for processing.
 *
 * @param	ControllerId	the controller that generated this input axis event
 * @param	Key				the name of the axis that moved  (KEY_MouseX, KEY_XboxTypeS_LeftX, etc.)
 * @param	Delta			the movement delta for the axis
 * @param	DeltaTime		the time (in seconds) since the last axis update.
 * @param	bGamepad		input came from gamepad (ie xbox controller)
 *
 * @return	return TRUE to indicate that the input event was handled.  if the return value is TRUE, this input event will not
 *			be passed to the interactions array.
 */
delegate bool HandleInputAxis( int ControllerId, name Key, float Delta, float DeltaTime, bool bGamepad);

/**
 * Provides script-only child classes the opportunity to handle character input (typing) events received from the viewport.
 * This delegate is called before the character event is passed to the interactions array for processing.
 *
 * @param	ControllerId	the controller that generated this character input event
 * @param	Unicode			the character that was typed
 *
 * @return	return TRUE to indicate that the input event was handled.  if the return value is TRUE, this input event will not
 *			be passed to the interactions array.
 */
delegate bool HandleInputChar( int ControllerId, string Unicode );

/**
 * Executes a console command in the context of this viewport.
 * @param	Command - The command to execute.
 * @return  The output of the command will be returned.
 */
native function string ConsoleCommand(string Command);

/**
 * Retrieve the size of the main viewport.
 *
 * @param	out_ViewportSize	[out] will be filled in with the size of the main viewport
 */
native final function GetViewportSize( out Vector2D out_ViewportSize );

/** @return Whether or not the main viewport is fullscreen or windowed. */
native final function bool IsFullScreenViewport();

/**
 * Adds a new player.
 * @param ControllerId - The controller ID the player should accept input from.
 * @param OutError - If no player is returned, OutError will contain a string describing the reason.
 * @param SpawnActor - True if an actor should be spawned for the new player.
 * @return The player which was created.
 */
event LocalPlayer CreatePlayer(int ControllerId, out string OutError, bool bSpawnActor)
{
	local LocalPlayer NewPlayer;


	`log("Creating new player with ControllerId" @ ControllerId,,'PlayerManagement');
	Assert(LocalPlayerClass != None);

	NewPlayer = new(Outer) LocalPlayerClass;
	NewPlayer.ViewportClient = Self;
	NewPlayer.ControllerId = ControllerId;

	AddLocalPlayer(NewPlayer);
	if (bSpawnActor)
	{
		if (GetCurrentWorldInfo().NetMode != NM_Client)
		{
			// server; spawn a new PlayerController immediately
			if (!NewPlayer.SpawnPlayActor("", OutError))
			{
				RemoveLocalPlayer(NewPlayer);
				NewPlayer = None;
			}
		}
		else
		{
			// client; ask the server to let the new player join
			NewPlayer.SendSplitJoin();
		}
	}

	if (OutError != "")
	{
		`Log("Player creation failed with error:" @ OutError);
	}
	else
	{
		`log("Successfully created new player with ControllerId" @ ControllerId $ ":" @ NewPlayer,,'PlayerManagement');
	}
	return NewPlayer;
}

/**
 * Removes a player.
 * @param Player - The player to remove.
 * @return whether the player was successfully removed. Removal is not allowed while connected to a server.
 */
event bool RemovePlayer(LocalPlayer ExPlayer)
{
	// can't destroy viewports while connected to a server
	if (ExPlayer.Actor.Role == ROLE_Authority)
	{
		`log("Removing player" @ ExPlayer @ " with ControllerId" @ ExPlayer.ControllerId,,'PlayerManagement');

		// Disassociate this viewport client from the player.
		ExPlayer.ViewportClient = None;

		if(ExPlayer.Actor != None)
		{
			// Destroy the player's actors.

			if(ExPlayer.Actor.Pawn != None)
			{
				ExPlayer.Actor.Pawn.Destroy();
			}

			ExPlayer.Actor.Destroy();
		}

		// Remove the player from the global and viewport lists of players.
		RemoveLocalPlayer(ExPlayer);
		`log("Finished removing player " @ ExPlayer @ " with ControllerId" @ ExPlayer.ControllerId,,'PlayerManagement');

		return true;
	}
	else
	{
		`log("Not removing player" @ ExPlayer @ " with ControllerId" @ ExPlayer.ControllerId @ "because player does not have appropriate role (" $ GetEnum(enum'ENetRole',ExPlayer.Actor.Role) $ ")",,'PlayerManagement');
		return false;
	}
}

/**
 * Finds a player by controller ID.
 * @param ControllerId - The controller ID to search for.
 * @return None or the player with matching controller ID.
 */
final event LocalPlayer FindPlayerByControllerId(int ControllerId)
{
	local int PlayerIndex;
	for(PlayerIndex = 0;PlayerIndex < GamePlayers.Length;PlayerIndex++)
	{
		if(GamePlayers[PlayerIndex].ControllerId == ControllerId)
		{
			return GamePlayers[PlayerIndex];
		}
	}
	return None;
}

/**
 * Debug console command to create a player.
 * @param ControllerId - The controller ID the player should accept input from.
 */
exec function DebugCreatePlayer(int ControllerId)
{
	local string Error;

	CreatePlayer(ControllerId, Error, TRUE);
}

/** Rotates controller ids among gameplayers, useful for testing splitscreen with only one controller. */
exec function SSSwapControllers()
{
	local int Idx, TmpControllerID;
	TmpControllerID = GamePlayers[0].ControllerID;

	for (Idx=0; Idx<GamePlayers.Length-1; ++Idx)
	{
		GamePlayers[Idx].ControllerID = GamePlayers[Idx+1].ControllerID;
	}
	GamePlayers[GamePlayers.Length-1].ControllerID = TmpControllerID;
}

/**
 * Debug console command to remove the player with a given controller ID.
 * @param ControllerId - The controller ID to search for.
 */
exec function DebugRemovePlayer(int ControllerId)
{
	local LocalPlayer ExPlayer;

	ExPlayer = FindPlayerByControllerId(ControllerId);
	if(ExPlayer != None)
	{
		RemovePlayer(ExPlayer);
	}
}

/**
 * Initialize the game viewport.
 * @param OutError - If an error occurs, returns the error description.
 * @return False if an error occurred, true if the viewport was initialized successfully.
 */
event bool Init(out string OutError)
{
	local PlayerManagerInteraction PlayerInteraction;

	assert(Outer.ConsoleClass != None);

	// Create the viewport's console.
	ViewportConsole = new(Self) Outer.ConsoleClass;
	if ( InsertInteraction(ViewportConsole) == -1 )
	{
		OutError = "Failed to add interaction to GlobalInteractions array:" @ ViewportConsole;
		return false;
	}

	assert(UIControllerClass != None);

	// Create a interaction to handle UI input.
	UIController = new(Self) UIControllerClass;
	if ( InsertInteraction(UIController) == -1 )
	{
		OutError = "Failed to add interaction to GlobalInteractions array:" @ UIController;
		return false;
	}

	// Create the viewport's player management interaction.
	PlayerInteraction = new(Self) class'PlayerManagerInteraction';
	if ( InsertInteraction(PlayerInteraction) == -1 )
	{
		OutError = "Failed to add interaction to GlobalInteractions array:" @ PlayerInteraction;
		return false;
	}

	// Create a single initial player.
	return CreatePlayer(0,OutError,false) != None;
}

/**
 * Inserts an interaction into the GlobalInteractions array at the specified index
 *
 * @param	NewInteraction	the interaction that should be inserted into the array
 * @param	Index			the position in the GlobalInteractions array to insert the element.
 *							if no value (or -1) is specified, inserts the interaction at the end of the array
 *
 * @return	the position in the GlobalInteractions array where the element was placed, or -1 if the element wasn't
 *			added to the array for some reason
 */
event int InsertInteraction( Interaction NewInteraction, optional int InIndex = -1 )
{
	local int Result;

	Result = -1;
	if ( NewInteraction != None )
	{
		// if the specified index is -1, assume that the item should be added to the end of the array
		if ( InIndex == -1 )
		{
			InIndex = GlobalInteractions.Length;
		}

		// if the index is a negative value other than -1, don't add the element as someone made a mistake
		if ( InIndex >= 0 )
		{
			// clamp the Index to avoid expanding the array needlessly
			Result = Clamp(InIndex, 0, GlobalInteractions.Length);

			// now insert the item
			GlobalInteractions.Insert(Result, 1);
			GlobalInteractions[Result] = NewInteraction;
			NewInteraction.Init();
		}
		else
		{
			`warn("Invalid insertion index specified:" @ InIndex);
		}
	}

	return Result;
}

/**
 * Called when the current map is being unloaded.  Cleans up any references which would prevent garbage collection.
 */
event GameSessionEnded()
{
	local int i;

	for ( i = 0; i < GlobalInteractions.Length; i++ )
	{
		GlobalInteractions[i].NotifyGameSessionEnded();
	}
}

/** debug test for testing splitscreens */
exec function SetSplit( int mode )
{
	SetSplitscreenConfiguration( ESplitScreenType(mode) );
}

/** Sets up the splitscreen configuration and does some sanity checking */
function SetSplitscreenConfiguration( ESplitScreenType SplitType )
{
	local int Idx;

	SplitscreenType = SplitType;

	// Perform sanity check
	switch ( GamePlayers.length )
	{
		case 0:
		case 1:
			SplitscreenType = eSST_NONE;
			break;
		case 2:
			if ( (SplitscreenType != eSST_2P_HORIZONTAL) && (SplitscreenType != eSST_2P_VERTICAL) )
			{
				SplitscreenType = Default2PSplitType;
			}
			break;
		case 3:
			if ( (SplitscreenType != eSST_3P_FAVOR_TOP) && (SplitscreenType != eSST_3P_FAVOR_BOTTOM) )
			{
				SplitscreenType = Default3PSplitType;
			}
			break;
		default:
			SplitscreenType = eSST_4P;
	}

	// Initialize the players
	for ( Idx = 0; Idx < SplitscreenInfo[SplitscreenType].PlayerData.length; Idx++ )
	{
		GamePlayers[Idx].Size.X =	SplitscreenInfo[SplitscreenType].PlayerData[Idx].SizeX;
		GamePlayers[Idx].Size.Y =	SplitscreenInfo[SplitscreenType].PlayerData[Idx].SizeY;
		GamePlayers[Idx].Origin.X =	SplitscreenInfo[SplitscreenType].PlayerData[Idx].OriginX;
		GamePlayers[Idx].Origin.Y =	SplitscreenInfo[SplitscreenType].PlayerData[Idx].OriginY;
	}
}

/**
 * Called before rendering to allow the game viewport to allocate subregions to players.
 */
event LayoutPlayers()
{
	SetSplitscreenConfiguration( SplitscreenType );
}

/** called before rending subtitles to allow the game viewport to determine the size of the subtitle area
 * @param Min top left bounds of subtitle region (0 to 1)
 * @param Max bottom right bounds of subtitle region (0 to 1)
 */
event GetSubtitleRegion(out vector2D MinPos, out vector2D MaxPos)
{
	MaxPos.X = 1.0f;
	MaxPos.Y = (GamePlayers.length == 1) ? 0.9f : 0.5f;
}

/**
* Convert a LocalPlayer to it's index in the GamePlayer array
* Returns -1 if the index could not be found.
*/
final function int ConvertLocalPlayerToGamePlayerIndex( LocalPlayer LPlayer )
{
	return GamePlayers.Find( LPlayer );
}

/**
 * Whether the player at LocalPlayerIndex's viewport has a "top of viewport" safezone or not.
 */
final function bool HasTopSafeZone( int LocalPlayerIndex )
{
	switch ( SplitscreenType )
	{
		case eSST_NONE:
		case eSST_2P_VERTICAL:
			return true;

		case eSST_2P_HORIZONTAL:
		case eSST_3P_FAVOR_TOP:
			return (LocalPlayerIndex == 0) ? true : false;

		case eSST_3P_FAVOR_BOTTOM:
		case eSST_4P:
			return (LocalPlayerIndex < 2) ? true : false;
	}

	return false;
}

/**
* Whether the player at LocalPlayerIndex's viewport has a "bottom of viewport" safezone or not.
*/
final function bool HasBottomSafeZone( int LocalPlayerIndex )
{
	switch ( SplitscreenType )
	{
		case eSST_NONE:
		case eSST_2P_VERTICAL:
			return true;

		case eSST_2P_HORIZONTAL:
		case eSST_3P_FAVOR_TOP:
			return (LocalPlayerIndex == 0) ? false : true;

		case eSST_3P_FAVOR_BOTTOM:
		case eSST_4P:
			return (LocalPlayerIndex > 1) ? true : false;
	}

	return false;
}

/**
 * Whether the player at LocalPlayerIndex's viewport has a "left of viewport" safezone or not.
 */
final function bool HasLeftSafeZone( int LocalPlayerIndex )
{
	switch ( SplitscreenType )
	{
		case eSST_NONE:
		case eSST_2P_HORIZONTAL:
			return true;

		case eSST_2P_VERTICAL:
			return (LocalPlayerIndex == 0) ? true : false;

		case eSST_3P_FAVOR_TOP:
			return (LocalPlayerIndex < 2) ? true : false;

		case eSST_3P_FAVOR_BOTTOM:
		case eSST_4P:
			return (LocalPlayerIndex == 0 || LocalPlayerIndex == 2) ? true : false;
	}

	return false;
}

/**
 * Whether the player at LocalPlayerIndex's viewport has a "right of viewport" safezone or not.
 */
final function bool HasRightSafeZone( int LocalPlayerIndex )
{
	switch ( SplitscreenType )
	{
		case eSST_NONE:
		case eSST_2P_HORIZONTAL:
			return true;

		case eSST_2P_VERTICAL:
		case eSST_3P_FAVOR_BOTTOM:
			return (LocalPlayerIndex > 0) ? true : false;

		case eSST_3P_FAVOR_TOP:
			return (LocalPlayerIndex == 1) ? false : true;

		case eSST_4P:
			return (LocalPlayerIndex == 0 || LocalPlayerIndex == 2) ? false : true;
	}

	return false;
}

/**
* Get the total pixel size of the screen.
* This is different from the pixel size of the viewport since we could be in splitscreen
*/
final function GetPixelSizeOfScreen( out float out_Width, out float out_Height, canvas Canvas, int LocalPlayerIndex )
{
	switch ( SplitscreenType )
	{
	case eSST_NONE:
		out_Width = Canvas.ClipX;
		out_Height = Canvas.ClipY;
		return;
	case eSST_2P_HORIZONTAL:
		out_Width = Canvas.ClipX;
		out_Height = Canvas.ClipY * 2;
		return;
	case eSST_2P_VERTICAL:
		out_Width = Canvas.ClipX * 2;
		out_Height = Canvas.ClipY;
		return;
	case eSST_3P_FAVOR_TOP:
		if ( LocalPlayerIndex == 0 )
		{
			out_Width = Canvas.ClipX;
		}
		else
		{
			out_Width = Canvas.ClipX * 2;
		}
		out_Height = Canvas.ClipY * 2;
		return;
	case eSST_3P_FAVOR_BOTTOM:
		if ( LocalPlayerIndex == 2 )
		{
			out_Width = Canvas.ClipX;
		}
		else
		{
			out_Width = Canvas.ClipX * 2;
		}
		out_Height = Canvas.ClipY * 2;
		return;
	case eSST_4P:
		out_Width = Canvas.ClipX * 2;
		out_Height = Canvas.ClipY * 2;
		return;
	}
}

/**
* Calculate the amount of safezone needed for a single side for both vertical and horizontal dimensions
*/
final function CalculateSafeZoneValues( out float out_Horizontal, out float out_Vertical, canvas Canvas, int LocalPlayerIndex, bool bUseMaxPercent )
{
	local float ScreenWidth, ScreenHeight, XSafeZoneToUse, YSafeZoneToUse;

	XSafeZoneToUse = bUseMaxPercent ? TitleSafeZone.MaxPercentX : TitleSafeZone.RecommendedPercentX;
	YSafeZoneToUse = bUseMaxPercent ? TitleSafeZone.MaxPercentY : TitleSafeZone.RecommendedPercentY;

	GetPixelSizeOfScreen( ScreenWidth, ScreenHeight, Canvas, LocalPlayerIndex );
	out_Horizontal = (ScreenWidth * (1 - XSafeZoneToUse) / 2.0f);
	out_Vertical = (ScreenHeight * (1 - YSafeZoneToUse) / 2.0);
}

/*
 * Return pixel size of the deadzone based on which local player it is, and which zone they want to inquire
 */
final function float CalculateDeadZone( LocalPlayer LPlayer, ESafeZoneType SZType, canvas Canvas, optional bool bUseMaxPercent )
{
	local bool bHasSafeZone;
	local int LocalPlayerIndex;
	local float HorizSafeZoneValue, VertSafeZoneValue;

	if ( LPlayer != None )
	{
		LocalPlayerIndex = ConvertLocalPlayerToGamePlayerIndex( LPlayer );

		if ( LocalPlayerIndex != -1 )
		{
			// see if this player should have a safe zone for this particular zonetype
			switch ( SZType )
			{
			case eSZ_TOP:
				bHasSafeZone = HasTopSafeZone( LocalPlayerIndex );
				break;
			case eSZ_BOTTOM:
				bHasSafeZone = HasBottomSafeZone( LocalPlayerIndex );
				break;
			case eSZ_LEFT:
				bHasSafeZone = HasLeftSafeZone( LocalPlayerIndex );
				break;
			case eSZ_RIGHT:
				bHasSafeZone = HasRightSafeZone( LocalPlayerIndex );
				break;
			}

			// if they need a safezone, then calculate it and return it
			if ( bHasSafeZone )
			{
				// calculate the safezones
				CalculateSafeZoneValues( HorizSafeZoneValue, VertSafeZoneValue, Canvas, LocalPlayerIndex, bUseMaxPercent );

				if ( SZType == eSZ_TOP || SZType == eSZ_BOTTOM )
				{
					return VertSafeZoneValue;
				}
				else
				{
					return HorizSafeZoneValue;
				}
			}
		}
	}

	return 0.f;
}

/**
 * Calculate the pixel value of the center of the viewport - this takes the safezones into consideration.
 */
final function CalculatePixelCenter( out float out_CenterX, out float out_CenterY, LocalPlayer LPlayer, canvas Canvas, optional bool bUseMaxPercent )
{
	local int LocalPlayerIndex;
	local float HorizSafeZoneValue, VertSafeZoneValue;

	// get the center of the viewport
	out_CenterX = Canvas.ClipX / 2.f;
	out_CenterY = Canvas.ClipY / 2.f;

	// calculate any safezone adjustments
	if ( LPlayer != None )
	{
		// get the index into the GamePlayer array
		LocalPlayerIndex = ConvertLocalPlayerToGamePlayerIndex( LPlayer );

		if ( LocalPlayerIndex != -1 )
		{
			// calculate the safezones
			CalculateSafeZoneValues( HorizSafeZoneValue, VertSafeZoneValue, Canvas, LocalPlayerIndex, bUseMaxPercent );

			// apply the safezone adjustments where needed
			switch ( SplitscreenType )
			{
				case eSST_NONE:
				return;

				case eSST_2P_HORIZONTAL:
					if ( LocalPlayerIndex == 0 )
					{
						out_CenterY += VertSafeZoneValue/2;
					}
					else
					{
						out_CenterY -= VertSafeZoneValue/2;
					}
				return;

				case eSST_2P_VERTICAL:
					if ( LocalPlayerIndex == 0 )
					{
						out_CenterX += HorizSafeZoneValue/2;
					}
					else
					{
						out_CenterX -= HorizSafeZoneValue/2;
					}
				return;

				case eSST_3P_FAVOR_TOP:
					if ( LocalPlayerIndex == 0 )
					{
						out_CenterY += VertSafeZoneValue/2;
					}
					else
					{
						out_CenterY -= VertSafeZoneValue/2;
						if ( LocalPlayerIndex == 1 )
						{
							out_CenterX += HorizSafeZoneValue/2;
						}
						else
						{
							out_CenterX -= HorizSafeZoneValue/2;
						}
					}
				return;

				case eSST_3P_FAVOR_BOTTOM:
					if ( LocalPlayerIndex == 2 )
					{
						out_CenterY -= VertSafeZoneValue/2;
					}
					else
					{
						out_CenterY += VertSafeZoneValue/2;
						if ( LocalPlayerIndex == 0 )
						{
							out_CenterX += HorizSafeZoneValue/2;
						}
						else
						{
							out_CenterX -= HorizSafeZoneValue/2;
						}
					}
				return;

				case eSST_4P:
					if ( LocalPlayerIndex < 2 )
					{
						out_CenterY += VertSafeZoneValue/2;
					}
					else
					{
						out_CenterY -= VertSafeZoneValue/2;
					}

					if ( LocalPlayerIndex == 0 || LocalPlayerIndex == 2 )
					{
						out_CenterX += HorizSafeZoneValue/2;
					}
					else
					{
						out_CenterX -= HorizSafeZoneValue/2;
					}
				return;
			}
		}
	}
}

/**
 * Called every frame to allow the game viewport to update time based state.
 * @param	DeltaTime - The time since the last call to Tick.
 */
event Tick(float DeltaTime);

/**
* Exec for toggling the display of the title safe area
*/
exec function ShowTitleSafeArea()
{
	bShowTitleSafeZone = !bShowTitleSafeZone;
}

/**
* Draw the safe area using the current TitleSafeZone settings
*/
function DrawTitleSafeArea( canvas Canvas )
{
	// red colored max safe area box
	Canvas.SetDrawColor(255,0,0,255);
	Canvas.SetPos(Canvas.ClipX * (1 - TitleSafeZone.MaxPercentX) / 2.0, Canvas.ClipY * (1 - TitleSafeZone.MaxPercentY) / 2.0);
	Canvas.DrawBox(Canvas.ClipX * TitleSafeZone.MaxPercentX, Canvas.ClipY * TitleSafeZone.MaxPercentY);

	// yellow colored recommended safe area box
	Canvas.SetDrawColor(255,255,0,255);
	Canvas.SetPos(Canvas.ClipX * (1 - TitleSafeZone.RecommendedPercentX) / 2.0, Canvas.ClipY * (1 - TitleSafeZone.RecommendedPercentY) / 2.0);
	Canvas.DrawBox(Canvas.ClipX * TitleSafeZone.RecommendedPercentX, Canvas.ClipY * TitleSafeZone.RecommendedPercentY);
}

/**
 * Called after rendering the player views and HUDs to render menus, the console, etc.
 * This is the last rendering call in the render loop
 * @param Canvas - The canvas to use for rendering.
 */
event PostRender(Canvas Canvas)
{
	if( bShowTitleSafeZone )
	{
		DrawTitleSafeArea(Canvas);
	}

	// Render the console.
	ViewportConsole.PostRender_Console(Canvas);

	// Draw the transition screen.
	DrawTransition(Canvas);
}

/**
 * Displays the transition screen.
 * @param Canvas - The canvas to use for rendering.
 */
function DrawTransition(Canvas Canvas)
{
	switch(Outer.TransitionType)
	{
		case TT_Loading:
			DrawTransitionMessage(Canvas,LoadingMessage);
			break;
		case TT_Saving:
			DrawTransitionMessage(Canvas,SavingMessage);
			break;
		case TT_Connecting:
			DrawTransitionMessage(Canvas,ConnectingMessage);
			break;
		case TT_Precaching:
			DrawTransitionMessage(Canvas,PrecachingMessage);
			break;
		case TT_Paused:
			DrawTransitionMessage(Canvas,PausedMessage);
			break;
	}
}

/**
 * Print a centered transition message with a drop shadow.
 */
function DrawTransitionMessage(Canvas Canvas,string Message)
{
	local float XL, YL;

	Canvas.Font = class'Engine'.Static.GetLargeFont();
	Canvas.bCenter = false;
	Canvas.StrLen( Message, XL, YL );
	Canvas.SetPos(0.5 * (Canvas.ClipX - XL) + 1, 0.66 * Canvas.ClipY - YL * 0.5 + 1);
	Canvas.SetDrawColor(0,0,0);
	Canvas.DrawText( Message, false );
	Canvas.SetPos(0.5 * (Canvas.ClipX - XL), 0.66 * Canvas.ClipY - YL * 0.5);
	Canvas.SetDrawColor(0,0,255);;
	Canvas.DrawText( Message, false );
}

/**
 * Sets the player which console commands will be executed in the context of.
 */
exec function SetConsoleTarget(int PlayerIndex)
{
	if(PlayerIndex >= 0 && PlayerIndex < GamePlayers.Length)
	{
		ViewportConsole.ConsoleTargetPlayer = GamePlayers[PlayerIndex];
	}
	else
	{
		ViewportConsole.ConsoleTargetPlayer = None;
	}
}

/**
 * Notifies all interactions that a new player has been added to the list of active players.
 *
 * @param	PlayerIndex		the index [into the GamePlayers array] where the player was inserted
 * @param	AddedPlayer		the player that was added
 */
final function NotifyPlayerAdded( int PlayerIndex, LocalPlayer AddedPlayer )
{
	local int InteractionIndex;

	for ( InteractionIndex = 0; InteractionIndex < GlobalInteractions.Length; InteractionIndex++ )
	{
		if ( GlobalInteractions[InteractionIndex] != None )
		{
			GlobalInteractions[InteractionIndex].NotifyPlayerAdded(PlayerIndex, AddedPlayer);
		}
	}
}

/**
 * Notifies all interactions that a new player has been added to the list of active players.
 *
 * @param	PlayerIndex		the index [into the GamePlayers array] where the player was located
 * @param	RemovedPlayer	the player that was removed
 */
final function NotifyPlayerRemoved( int PlayerIndex, LocalPlayer RemovedPlayer )
{
	local int InteractionIndex;

	for ( InteractionIndex = GlobalInteractions.Length - 1; InteractionIndex >= 0; InteractionIndex-- )
	{
		if ( GlobalInteractions[InteractionIndex] != None )
		{
			GlobalInteractions[InteractionIndex].NotifyPlayerRemoved(PlayerIndex, RemovedPlayer);
		}
	}
}

/**
 * Adds a LocalPlayer to the local and global list of Players.
 *
 * @param	NewPlayer	the player to add
 */
private final function AddLocalPlayer( LocalPlayer NewPlayer )
{
	local int InsertIndex;

	// add to list
	InsertIndex = GamePlayers.Length;
	GamePlayers[InsertIndex] = NewPlayer;

	// let all interactions know about this
	NotifyPlayerAdded(InsertIndex, NewPlayer);
}

/**
 * Removes a LocalPlayer from the local and global list of Players.
 *
 * @param	ExistingPlayer	the player to remove
 */
private final function RemoveLocalPlayer( LocalPlayer ExistingPlayer )
{
	local int Index;

	Index = GamePlayers.Find(ExistingPlayer);
	if ( Index != -1 )
	{
		GamePlayers.Remove(Index,1);

		// let all interactions know about this
		NotifyPlayerRemoved(Index, ExistingPlayer);
	}
}

defaultproperties
{
	UIControllerClass=class'Engine.UIInteraction'
	TitleSafeZone=(MaxPercentX=0.9,MaxPercentY=0.9,RecommendedPercentX=0.8,RecommendedPercentY=0.8)

	SplitscreenInfo={(
		(PlayerData=((SizeX=1.0f,SizeY=1.0f,OriginX=0.0f,OriginY=0.0f))),	// eSST_NONE
		(PlayerData=((SizeX=1.0f,SizeY=0.5f,OriginX=0.0f,OriginY=0.0f),		// eSST_2P_HORIZONTAL
						(SizeX=1.0f,SizeY=0.5f,OriginX=0.0f,OriginY=0.5f))),
		(PlayerData=((SizeX=0.5f,SizeY=1.0f,OriginX=0.0f,OriginY=0.0f),		// eSST_2P_VERTICAL
						(SizeX=0.5f,SizeY=1.0f,OriginX=0.5f,OriginY=0.0))),
		(PlayerData=((SizeX=1.0f,SizeY=0.5f,OriginX=0.0f,OriginY=0.0f),		// eSST_3P_FAVOR_TOP
						(SizeX=0.5f,SizeY=0.5f,OriginX=0.0f,OriginY=0.5f),
						(SizeX=0.5f,SizeY=0.5f,OriginX=0.5f,OriginY=0.5f))),
		(PlayerData=((SizeX=0.5f,SizeY=0.5f,OriginX=0.0f,OriginY=0.0f),		// eSST_3P_FAVOR_BOTTOM
						(SizeX=0.5f,SizeY=0.5f,OriginX=0.5f,OriginY=0.0f),
						(SizeX=1.0f,SizeY=0.5f,OriginX=0.0f,OriginY=0.5f))),
		(PlayerData=((SizeX=0.5f,SizeY=0.5f,OriginX=0.0f,OriginY=0.0f),		// eSST_4P
						(SizeX=0.5f,SizeY=0.5f,OriginX=0.5f,OriginY=0.0f),
						(SizeX=0.5f,SizeY=0.5f,OriginX=0.0f,OriginY=0.5f),
						(SizeX=0.5f,SizeY=0.5f,OriginX=0.5f,OriginY=0.5f)))
	)}

	Default2PSplitType=eSST_2P_HORIZONTAL;
	Default3PSplitType=eSST_3P_FAVOR_TOP;
	SplitscreenType=eSST_NOVALUE
}
