/*=============================================================================
	UnPlayer.cpp: Unreal player implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "CanvasScene.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "EngineParticleClasses.h"
#include "EngineInterpolationClasses.h"
#include "EngineAnimClasses.h"
#include "EngineAudioDeviceClasses.h"
#include "EngineSoundNodeClasses.h"
#include "SceneRenderTargets.h"

// needed for adding components when typing "show paths" in game
#include "EngineAIClasses.h"

#include "UnStatChart.h"
#include "UnTerrain.h"
#include "UnSubtitleManager.h"
#include "UnNet.h"

IMPLEMENT_CLASS(UPlayer);
IMPLEMENT_CLASS(ULocalPlayer);

/** High-res screenshot variables */
UBOOL		GIsTiledScreenshot = FALSE;
INT			GScreenshotResolutionMultiplier = 2;
INT			GScreenshotMargin = 64;	// In pixels
INT			GScreenshotTile = 0;
FIntRect	GScreenshotRect;		// Rendered tile
/** TRUE if we should grab a screenshot at the end of the frame */
UBOOL		GScreenShotRequest = FALSE;

/** Whether to tick and render the UI. */
UBOOL		GTickAndRenderUI = TRUE;

UBOOL		ULocalPlayer::bOverrideView = FALSE;
FVector		ULocalPlayer::OverrideLocation;
FRotator	ULocalPlayer::OverrideRotation;

IMPLEMENT_CLASS(UGameViewportClient);
IMPLEMENT_CLASS(UPlayerManagerInteraction);

/** Whether to show the FPS counter */
UBOOL GShowFpsCounter = FALSE;
/** Whether to show the level stats */
static UBOOL GShowLevelStats = FALSE;	

UBOOL GShouldLogOutAFrameOfSkelCompTick = FALSE;

/** UPlayerManagerInteraction */
/**
 * Routes an input key event to the player's interactions array
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
UBOOL UPlayerManagerInteraction::InputKey(INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed/*=1.f*/,UBOOL bGamepad/*=FALSE*/)
{
	UBOOL bResult = FALSE;

	INT PlayerIndex = UUIInteraction::GetPlayerIndex(ControllerId);
	if ( GEngine->GamePlayers.IsValidIndex(PlayerIndex) )
	{
		ULocalPlayer* TargetPlayer = GEngine->GamePlayers(PlayerIndex);
		if ( TargetPlayer != NULL && TargetPlayer->Actor != NULL )
		{
			APlayerController* PC = TargetPlayer->Actor;
			for ( INT InteractionIndex = 0; !bResult && InteractionIndex < PC->Interactions.Num(); InteractionIndex++ )
			{
				UInteraction* PlayerInteraction = PC->Interactions(InteractionIndex);
				if ( OBJ_DELEGATE_IS_SET(PlayerInteraction,OnReceivedNativeInputKey) )
				{
					bResult = PlayerInteraction->delegateOnReceivedNativeInputKey(ControllerId, Key, Event, AmountDepressed, bGamepad);
				}

				bResult = bResult || PlayerInteraction->InputKey(ControllerId, Key, Event, AmountDepressed, bGamepad);
			}
		}
	}

	return bResult;
}

/**
 * Routes an axis input event to the player's interactions array.
 *
 * @param	Viewport - The viewport which the axis movement is from.
 * @param	ControllerId - The controller which the axis movement is from.
 * @param	Key - The name of the axis which moved.
 * @param	Delta - The axis movement delta.
 * @param	DeltaTime - The time since the last axis update.
 *
 * @return	True to consume the axis movement, false to pass it on.
 */
UBOOL UPlayerManagerInteraction::InputAxis( INT ControllerId, FName Key, FLOAT Delta, FLOAT DeltaTime, UBOOL bGamepad/*=FALSE*/ )
{
	UBOOL bResult = FALSE;

	INT PlayerIndex = UUIInteraction::GetPlayerIndex(ControllerId);
	if ( GEngine->GamePlayers.IsValidIndex(PlayerIndex) )
	{
		ULocalPlayer* TargetPlayer = GEngine->GamePlayers(PlayerIndex);
		if ( TargetPlayer != NULL && TargetPlayer->Actor != NULL )
		{
			APlayerController* PC = TargetPlayer->Actor;
			for ( INT InteractionIndex = 0; !bResult && InteractionIndex < PC->Interactions.Num(); InteractionIndex++ )
			{
				UInteraction* PlayerInteraction = PC->Interactions(InteractionIndex);
				if ( OBJ_DELEGATE_IS_SET(PlayerInteraction,OnReceivedNativeInputAxis) )
				{
					bResult = PlayerInteraction->delegateOnReceivedNativeInputAxis(ControllerId, Key, Delta, DeltaTime, bGamepad);
				}

				bResult = bResult || PlayerInteraction->InputAxis(ControllerId, Key, Delta, DeltaTime, bGamepad);
			}
		}
	}

	return bResult;
}

/**
 * Routes a character input to the player's Interaction array.
 *
 * @param	Viewport - The viewport which the axis movement is from.
 * @param	ControllerId - The controller which the axis movement is from.
 * @param	Character - The character.
 *
 * @return	True to consume the character, false to pass it on.
 */
UBOOL UPlayerManagerInteraction::InputChar( INT ControllerId, TCHAR Character )
{
	UBOOL bResult = FALSE;

	INT PlayerIndex = UUIInteraction::GetPlayerIndex(ControllerId);
	if ( GEngine->GamePlayers.IsValidIndex(PlayerIndex) )
	{
		ULocalPlayer* TargetPlayer = GEngine->GamePlayers(PlayerIndex);
		if ( TargetPlayer != NULL && TargetPlayer->Actor != NULL )
		{
			APlayerController* PC = TargetPlayer->Actor;
			for ( INT InteractionIndex = 0; !bResult && InteractionIndex < PC->Interactions.Num(); InteractionIndex++ )
			{
				UInteraction* PlayerInteraction = PC->Interactions(InteractionIndex);
				if ( OBJ_DELEGATE_IS_SET(PlayerInteraction,OnReceivedNativeInputChar) )
				{
					TCHAR CharString[2] = { Character, 0 };
					bResult = PlayerInteraction->delegateOnReceivedNativeInputChar(ControllerId, CharString);
				}

				bResult = bResult || PlayerInteraction->InputChar(ControllerId, Character);
			}
		}
	}

	return bResult;
}

UGameViewportClient::UGameViewportClient():
	ShowFlags((SHOW_DefaultGame&~SHOW_ViewMode_Mask)|SHOW_ViewMode_Lit)
{}

/**
 * Cleans up all rooted or referenced objects created or managed by the GameViewportClient.  This method is called
 * when this GameViewportClient has been disassociated with the game engine (i.e. is no longer the engine's GameViewport).
 */
void UGameViewportClient::DetachViewportClient()
{
	// notify all interactions to clean up their own references
	eventGameSessionEnded();

	// if we have a UIController, tear it down now
	if ( UIController != NULL )
	{
		UIController->TearDownUI();
	}

	UIController = NULL;
	ViewportConsole = NULL;
	RemoveFromRoot();
}

/**
 * Called every frame to allow the game viewport to update time based state.
 * @param	DeltaTime - The time since the last call to Tick.
 */
void UGameViewportClient::Tick( FLOAT DeltaTime )
{
	// first call the unrealscript tick
	eventTick(DeltaTime);

	// now tick all interactions
	for ( INT i = 0; i < GlobalInteractions.Num(); i++ )
	{
		UInteraction* Interaction = GlobalInteractions(i);
		Interaction->Tick(DeltaTime);
	}
}

FString UGameViewportClient::ConsoleCommand(const FString& Command)
{
	FString TruncatedCommand = Command.Left(1000);
	FConsoleOutputDevice ConsoleOut(ViewportConsole);
	Exec(*TruncatedCommand,ConsoleOut);
	return *ConsoleOut;
}

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
UBOOL UGameViewportClient::InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent EventType,FLOAT AmountDepressed,UBOOL bGamepad)
{

#if !CONSOLE
	// if a movie is playing then handle input key
	if( GFullScreenMovie && 
		GFullScreenMovie->GameThreadIsMoviePlaying(TEXT("")) )
	{
		if ( GFullScreenMovie->InputKey(Viewport,ControllerId,Key,EventType,AmountDepressed,bGamepad) )
		{
			return TRUE;
		}
	}
#endif

	UBOOL bResult = FALSE;

	if ( DELEGATE_IS_SET(HandleInputKey) )
	{
		bResult = delegateHandleInputKey(ControllerId, Key, EventType, AmountDepressed, bGamepad);
	}

	// if it wasn't handled by script, route to the interactions array
	for ( INT InteractionIndex = 0; !bResult && InteractionIndex < GlobalInteractions.Num(); InteractionIndex++ )
	{
		UInteraction* Interaction = GlobalInteractions(InteractionIndex);
		if ( OBJ_DELEGATE_IS_SET(Interaction,OnReceivedNativeInputKey) )
		{
			bResult = Interaction->delegateOnReceivedNativeInputKey(ControllerId, Key, EventType, AmountDepressed, bGamepad);
		}

		bResult = bResult || Interaction->InputKey(ControllerId, Key, EventType, AmountDepressed, bGamepad);
	}

	return bResult;
}

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
UBOOL UGameViewportClient::InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime, UBOOL bGamepad)
{
	UBOOL bResult = FALSE;

	// give script the chance to process this input first
	if ( DELEGATE_IS_SET(HandleInputAxis) )
	{
		bResult = delegateHandleInputAxis(ControllerId, Key, Delta, DeltaTime, bGamepad);
	}

	// if it wasn't handled by script, route to the interactions array
	for ( INT InteractionIndex = 0; !bResult && InteractionIndex < GlobalInteractions.Num(); InteractionIndex++ )
	{
		UInteraction* Interaction = GlobalInteractions(InteractionIndex);
		if ( OBJ_DELEGATE_IS_SET(Interaction,OnReceivedNativeInputAxis) )
		{
			bResult = Interaction->delegateOnReceivedNativeInputAxis(ControllerId, Key, Delta, DeltaTime, bGamepad);
		}

		bResult = bResult || Interaction->InputAxis(ControllerId, Key, Delta, DeltaTime, bGamepad);
	}

	return bResult;
}

/**
 * Routes a character input event (typing) received from the viewport to the Interactions array for processing.
 *
 * @param	Viewport		the viewport the input event was received from
 * @param	ControllerId	the controller that generated this character input event
 * @param	Character		the character that was typed
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UGameViewportClient::InputChar(FViewport* Viewport,INT ControllerId,TCHAR Character)
{
	UBOOL bResult = FALSE;

	// should probably just add a ctor to FString that takes a TCHAR
	FString CharacterString;
	CharacterString += Character;

	if ( DELEGATE_IS_SET(HandleInputChar) )
	{
		bResult = delegateHandleInputChar(ControllerId, CharacterString);
	}

	// if it wasn't handled by script, route to the interactions array
	for ( INT InteractionIndex = 0; !bResult && InteractionIndex < GlobalInteractions.Num(); InteractionIndex++ )
	{
		UInteraction* Interaction = GlobalInteractions(InteractionIndex);
		if ( OBJ_DELEGATE_IS_SET(Interaction,OnReceivedNativeInputChar) )
		{
			bResult = Interaction->delegateOnReceivedNativeInputChar(ControllerId, CharacterString);
		}

		bResult = bResult || Interaction->InputChar(ControllerId, Character);
	}

	return bResult;
}

/**
 * Returns the forcefeedback manager associated with the PlayerController.
 */
class UForceFeedbackManager* UGameViewportClient::GetForceFeedbackManager(INT ControllerId)
{
	for(INT PlayerIndex = 0;PlayerIndex < GEngine->GamePlayers.Num();PlayerIndex++)
	{
		ULocalPlayer* Player = GEngine->GamePlayers(PlayerIndex);
		if(Player->ViewportClient == this && Player->ControllerId == ControllerId)
		{
			// Only play force feedback on gamepad
			if( Player->Actor && 
				Player->Actor->ForceFeedbackManager && 
				(Player->Actor->PlayerInput == NULL || Player->Actor->PlayerInput->bUsingGamepad) )
			{
				return Player->Actor->ForceFeedbackManager;
			}
			break;
		}
	}

	return NULL;
}


/** @returns whether the controller is active **/
UBOOL UGameViewportClient::IsControllerTiltActive() const
{
	//warnf( TEXT( "UGameViewportClient::SetControllerTiltActive" ) );
	return FALSE;
}

void UGameViewportClient::SetControllerTiltActive( UBOOL bActive )
{
	//warnf( TEXT( "UGameViewportClient::SetControllerTiltActive" ) );
}

void UGameViewportClient::SetOnlyUseControllerTiltInput( UBOOL bActive )
{
	//warnf( TEXT( "UGameViewportClient::SetOnlyUseControllerTiltInput" ) );
}

void UGameViewportClient::SetUseTiltForwardAndBack( UBOOL bActive )
{
	//warnf( TEXT( "UGameViewportClient::SetOnlyUseControllerTiltInput" ) );
}




/**
 * Determines whether this viewport client should receive calls to InputAxis() if the game's window is not currently capturing the mouse.
 * Used by the UI system to easily receive calls to InputAxis while the viewport's mouse capture is disabled.
 */
UBOOL UGameViewportClient::RequiresUncapturedAxisInput() const
{
	return Viewport != NULL && bDisplayingUIMouseCursor == TRUE;
}

/**
 * Updates the current mouse capture state to reflect the state of the game.
 */
void UGameViewportClient::UpdateMouseCapture()
{
	if ( Viewport != NULL )
	{
		UBOOL bShouldCaptureMouse = bUIMouseCaptureOverride || (!GWorld->IsPaused() && !bDisplayingUIMouseCursor);
		Viewport->CaptureMouseInput(bShouldCaptureMouse, !bUIMouseCaptureOverride);
	}
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
EMouseCursor UGameViewportClient::GetCursor( FViewport* Viewport, INT X, INT Y )
{
	// if Y is less than zero, the user is mousing over the title bar
	if ( bDisplayingUIMouseCursor && (Viewport->IsFullscreen() || Y >= 0) )
	{
		//@todo - returning MC_None causes the OS to not render a mouse...might want to change
		// this so that UI can indicate which mouse cursor should be displayed
		return MC_None;
	}
	
	return FViewportClient::GetCursor(Viewport, X, Y);
}

/**
 * Set this GameViewportClient's viewport to the viewport specified
 */
void UGameViewportClient::SetViewport( FViewportFrame* InViewportFrame )
{
	FViewport* PreviousViewport = Viewport;
	ViewportFrame = InViewportFrame;
	Viewport = ViewportFrame ? ViewportFrame->GetViewport() : NULL;

	if ( PreviousViewport == NULL && Viewport != NULL )
	{
		// ensure that the player's Origin and Size members are initialized the moment we get a viewport
		eventLayoutPlayers();
	}

	if ( UIController != NULL )
	{
		UIController->SceneClient->SetRenderViewport(Viewport);
	}

}

/**
 * Retrieve the size of the main viewport.
 *
 * @param	out_ViewportSize	[out] will be filled in with the size of the main viewport
 */
void UGameViewportClient::GetViewportSize( FVector2D& out_ViewportSize )
{
	if ( Viewport != NULL )
	{
		out_ViewportSize.X = Viewport->GetSizeX();
		out_ViewportSize.Y = Viewport->GetSizeY();
	}
}

/** @return Whether or not the main viewport is fullscreen or windowed. */
UBOOL UGameViewportClient::IsFullScreenViewport()
{
	return Viewport->IsFullscreen();
}

/** Whether we should precache during the next frame. */
UBOOL GPrecacheNextFrame = FALSE;
/** Whether texture memory has been corrupted because we ran out of memory in the pool. */
UBOOL GIsTextureMemoryCorrupted = FALSE;

void UGameViewportClient::Draw(FViewport* Viewport,FCanvas* Canvas)
{
	if(GPrecacheNextFrame)
	{
#if !CONSOLE
		FSceneViewFamily PrecacheViewFamily(Viewport,GWorld->Scene,ShowFlags,GWorld->GetTimeSeconds(),GWorld->GetRealTimeSeconds(),NULL);
		PrecacheViewFamily.Views.AddItem(
			new FSceneView(
				&PrecacheViewFamily,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				0,
				0,
				1,
				1,
				FMatrix(
					FPlane(1.0f / WORLD_MAX,0.0f,0.0f,0.0f),
					FPlane(0.0f,1.0f / WORLD_MAX,0.0f,0.0f),
					FPlane(0.0f,0.0f,1.0f / WORLD_MAX,0.0f),
					FPlane(0.5f,0.5f,0.5f,1.0f)
					),
				FMatrix::Identity,
				FLinearColor::Black,
				FLinearColor::Black,
				FLinearColor::White,
				TArray<FPrimitiveSceneInfo*>()
				)
			);
		BeginRenderingViewFamily(Canvas,&PrecacheViewFamily);
#endif

		GPrecacheNextFrame = FALSE;
	}

	// Create a temporary canvas if there isn't already one.
	UCanvas* CanvasObject = FindObject<UCanvas>(UObject::GetTransientPackage(),TEXT("CanvasObject"));
	if( !CanvasObject )
	{
		CanvasObject = ConstructObject<UCanvas>(UCanvas::StaticClass(),UObject::GetTransientPackage(),TEXT("CanvasObject"));
		CanvasObject->AddToRoot();
	}
	CanvasObject->Canvas = Canvas;	

	// Setup a FSceneViewFamily for the player.
	if( GWorld->bGatherDynamicShadowStatsSnapshot )
	{
		// Start with clean slate.
		GWorld->DynamicShadowStats.Empty();
	}

	// see if the UI has post processing
	UUISceneClient* UISceneClient = UUIRoot::GetSceneClient(); 
	UBOOL bUISceneHasPostProcess = UISceneClient && UISceneClient->UsesPostProcess();
	// see if canvas scene has any primitives
	FCanvasScene* CanvasScene = UIController->UsesUIPrimitiveScene() ? UIController->GetUIPrimitiveScene() : NULL;
	// disable final copy to viewport RT if there are canvas prims or the ui scene needs to be post processed
	UBOOL bDisableViewportCopy = GTickAndRenderUI && (bUISceneHasPostProcess || (CanvasScene && CanvasScene->GetNumPrimitives() > 0));

	// create the view family for rendering the world scene to the viewport's render target
	FSceneViewFamilyContext ViewFamily(
		Viewport,
		GWorld->Scene,
		ShowFlags,
		GWorld->GetTimeSeconds(),
		GWorld->GetRealTimeSeconds(), 
		GWorld->bGatherDynamicShadowStatsSnapshot ? &GWorld->DynamicShadowStats : NULL, 
		FALSE,
		FALSE,					// disable the initial scene color clear in game
		!bDisableViewportCopy	// disable the final copy to the viewport RT if still rendering to scene color
		);
	TMap<INT,FSceneView*> PlayerViewMap;
	UBOOL bHaveReverbSettingsBeenSet = FALSE;
	for(INT PlayerIndex = 0;PlayerIndex < GEngine->GamePlayers.Num();PlayerIndex++)
	{
		ULocalPlayer* Player = GEngine->GamePlayers(PlayerIndex);
		if (Player->Actor)
		{
			// Calculate the player's view information.
			FVector		ViewLocation;
			FRotator	ViewRotation;

			FSceneView* View = Player->CalcSceneView( &ViewFamily, ViewLocation, ViewRotation, Viewport );
			if(View)
			{
				// Tiled rendering for high-res screenshots.
				if ( GIsTiledScreenshot )
				{
					// Calculate number of overlapping tiles:
					INT TileWidth	= Viewport->GetSizeX();
					INT TileHeight	= Viewport->GetSizeY();
					INT TotalWidth	= GScreenshotResolutionMultiplier * TileWidth;
					INT TotalHeight	= GScreenshotResolutionMultiplier * TileHeight;
					INT NumColumns	= appCeil( FLOAT(TotalWidth) / FLOAT(TileWidth - 2*GScreenshotMargin) );
					INT NumRows		= appCeil( FLOAT(TotalHeight) / FLOAT(TileHeight - 2*GScreenshotMargin) );
					TileWidth		= appTrunc(View->SizeX);
					TileHeight		= appTrunc(View->SizeY);
					TotalWidth		= GScreenshotResolutionMultiplier * TileWidth;
					TotalHeight		= GScreenshotResolutionMultiplier * TileHeight;

					// Report back to UD3DRenderDevice::TiledScreenshot():
					GScreenshotRect.Min.X = appTrunc(View->X);
					GScreenshotRect.Min.Y = appTrunc(View->Y);
					GScreenshotRect.Max.X = appTrunc(View->X + View->SizeX);
					GScreenshotRect.Max.Y = appTrunc(View->Y + View->SizeY);

					// Calculate tile position (upper-left corner, screen space):
					INT TileRow		= GScreenshotTile / NumColumns;
					INT TileColumn	= GScreenshotTile % NumColumns;
					INT PosX		= TileColumn*TileWidth - (2*TileColumn + 1)*GScreenshotMargin;
					INT PosY		= TileRow*TileHeight - (2*TileRow + 1)*GScreenshotMargin;

					// Calculate offsets to center tile (screen space):
					INT OffsetX		= (TotalWidth - TileWidth) / 2 - PosX;
					INT OffsetY		= (TotalHeight - TileHeight) / 2 - PosY;

					// Convert to projection space:
					FLOAT Scale		= FLOAT(GScreenshotResolutionMultiplier);
					FLOAT OffsetXp	= 2.0f * FLOAT(OffsetX) / FLOAT(TotalWidth);
					FLOAT OffsetYp	= -2.0f * FLOAT(OffsetY) / FLOAT(TotalHeight);

					// Apply offsets and scales:
					FTranslationMatrix OffsetMtx( FVector( OffsetXp, OffsetYp, 0.0f) );
					FScaleMatrix ScaleMtx( FVector(Scale, Scale, 1.0f) );
					View->ViewProjectionMatrix = View->ViewMatrix * View->ProjectionMatrix;
					View->ViewProjectionMatrix = View->ViewProjectionMatrix * OffsetMtx * ScaleMtx;
					View->InvViewProjectionMatrix = View->ViewProjectionMatrix.Inverse();
					//RI->SetOrigin2D( -PosX, -PosY );
					//RI->SetZoom2D( Scale );
				}

				PlayerViewMap.Set(PlayerIndex,View);

				// Update the listener.
				check(GEngine->Client);
				UAudioDevice* AudioDevice = GEngine->Client->GetAudioDevice();
				if( AudioDevice )
				{
					FMatrix CameraToWorld		= View->ViewMatrix.Inverse();

					FVector ProjUp				= CameraToWorld.TransformNormal(FVector(0,1000,0));
					FVector ProjRight			= CameraToWorld.TransformNormal(FVector(1000,0,0));
					FVector ProjFront			= ProjRight ^ ProjUp;

					ProjUp.Z = Abs( ProjUp.Z ); // Don't allow flipping "up".

					ProjUp.Normalize();
					ProjRight.Normalize();
					ProjFront.Normalize();

					AudioDevice->SetListener(PlayerIndex, ViewLocation, ProjUp, ProjRight, ProjFront);

					// Update reverb settings based on the view of the first player we encounter.
					if ( !bHaveReverbSettingsBeenSet )
					{
						bHaveReverbSettingsBeenSet = TRUE;
						FReverbSettings ReverbSettings;
						AReverbVolume* ReverbVolume = GWorld->GetWorldInfo()->GetReverbSettings( ViewLocation, TRUE/*bReverbVolumePrevis*/, ReverbSettings );
						AudioDevice->SetReverbSettings( ReverbSettings );
					}
				}

				if (!bDisableWorldRendering)
				{
					// Set the canvas transform for the player's view rectangle.
					CanvasObject->Init();
					CanvasObject->SizeX = appTrunc(View->SizeX);
					CanvasObject->SizeY = appTrunc(View->SizeY);
					CanvasObject->SceneView = View;
					CanvasObject->Update();
					Canvas->PushAbsoluteTransform(FTranslationMatrix(FVector(View->X,View->Y,0)));

					// PreRender the player's view.
					Player->Actor->eventPreRender(CanvasObject);

					Canvas->PopTransform();
				}

				// Add view information for resource streaming.
				GStreamingManager->AddViewInformation( View->ViewOrigin, View->SizeX, View->SizeX * View->ProjectionMatrix.M[0][0] );
			}
		}
	}

	// Update level streaming.
	GWorld->UpdateLevelStreaming( &ViewFamily );

	// Draw the player views.
	if (!bDisableWorldRendering)
	{
		BeginRenderingViewFamily(Canvas,&ViewFamily);
	}

	// Clear areas of the rendertarget (backbuffer) that aren't drawn over by the views.
	{
		// Find largest rectangle bounded by all rendered views.
		UINT MinX=Viewport->GetSizeX(),	MinY=Viewport->GetSizeY(), MaxX=0, MaxY=0;
		UINT TotalArea = 0;
		for( INT ViewIndex = 0; ViewIndex < ViewFamily.Views.Num(); ++ViewIndex )
		{
			const FSceneView* View = ViewFamily.Views(ViewIndex);
			MinX = Min<UINT>(appTrunc(View->X), MinX);
			MinY = Min<UINT>(appTrunc(View->Y), MinY);
			MaxX = Max<UINT>(appTrunc(View->X + View->SizeX), MaxX);
			MaxY = Max<UINT>(appTrunc(View->Y + View->SizeY), MaxY);
			TotalArea += appTrunc(View->SizeX) * appTrunc(View->SizeY);
		}
		// If the views don't cover the entire bounding rectangle, clear the entire buffer.
		if ( ViewFamily.Views.Num() == 0 || TotalArea != (MaxX-MinX)*(MaxY-MinY) )
		{
			DrawTile(Canvas,0,0,Viewport->GetSizeX(),Viewport->GetSizeY(),0.0f,0.0f,1.0f,1.f,FLinearColor::Black,NULL,FALSE);
		}
		else
		{
			// clear left
			if( MinX > 0 )
			{
				DrawTile(Canvas,0,0,MinX,Viewport->GetSizeY(),0.0f,0.0f,1.0f,1.f,FLinearColor::Black,NULL,FALSE);
			}
			// clear right
			if( MaxX < Viewport->GetSizeX() )
			{
				DrawTile(Canvas,MaxX,0,Viewport->GetSizeX(),Viewport->GetSizeY(),0.0f,0.0f,1.0f,1.f,FLinearColor::Black,NULL,FALSE);
			}
			// clear top
			if( MinY > 0 )
			{
				DrawTile(Canvas,MinX,0,MaxX,MinY,0.0f,0.0f,1.0f,1.f,FLinearColor::Black,NULL,FALSE);
			}
			// clear bottom
			if( MaxY < Viewport->GetSizeY() )
			{
				DrawTile(Canvas,MinX,MaxY,MaxX,Viewport->GetSizeY(),0.0f,0.0f,1.0f,1.f,FLinearColor::Black,NULL,FALSE);
			}
		}
	}

	// Remove temporary debug lines.
	if (GWorld->LineBatcher != NULL && GWorld->LineBatcher->BatchedLines.Num())
	{
		GWorld->LineBatcher->BatchedLines.Empty();
		GWorld->LineBatcher->BeginDeferredReattach();
	}

	// End dynamic shadow stats gathering now that snapshot is collected.
	if( GWorld->bGatherDynamicShadowStatsSnapshot )
	{
		GWorld->bGatherDynamicShadowStatsSnapshot = FALSE;
		// Update the dynamic shadow stats browser.
		if( GCallbackEvent )
		{
			GCallbackEvent->Send( CALLBACK_RefreshEditor_DynamicShadowStatsBrowser );
		}
		// Dump stats to the log outside the editor.
		if( !GIsEditor )
		{
			for( INT RowIndex=0; RowIndex<GWorld->DynamicShadowStats.Num(); RowIndex++ )
			{
				const FDynamicShadowStatsRow& Row = GWorld->DynamicShadowStats(RowIndex);
				FString RowStringified;
				for( INT ColumnIndex=0; ColumnIndex<Row.Columns.Num(); ColumnIndex++ )
				{
					RowStringified += FString::Printf(TEXT("%s,"),*Row.Columns(ColumnIndex));
				}
				debugf(TEXT("%s"),*RowStringified);
			}
		}
	}

	if( CanvasScene && 
		CanvasScene->GetNumPrimitives() > 0 )
	{
		// Use the same show flags for the canvas scene as the main scene
		QWORD CanvasSceneShowFlags = ShowFlags;

		// Create a FSceneViewFamilyContext for the canvas scene
		FSceneViewFamilyContext CanvasSceneViewFamily(
			Viewport,
			CanvasScene,
			CanvasSceneShowFlags,
			GWorld->GetTimeSeconds(),
			GWorld->GetRealTimeSeconds(), 
			NULL, 
			FALSE,			
			FALSE,					// maintain scene color from player scene rendering
			!bUISceneHasPostProcess	// disable the final copy to the viewport RT if still rendering to scene color
			);

		// Generate the view for the canvas scene
		const FVector2D ViewOffsetScale(0,0);
		const FVector2D ViewSizeScale(1,1);
		CanvasScene->CalcSceneView(
			&CanvasSceneViewFamily,
			ViewOffsetScale,
			ViewSizeScale,
			Viewport,
			NULL,
			NULL
			);

		// Render the canvas scene
		BeginRenderingViewFamily(Canvas,&CanvasSceneViewFamily);
	}

	// render the canvas scene if needed
	// get optional UI scene for rendering its primitives	
	if( bUISceneHasPostProcess )
	{
		// create a proxy of the scene color buffer to render to
		static FSceneRenderTargetProxy SceneColorTarget;
		// set the scene color render target
		Canvas->SetRenderTarget(&SceneColorTarget);
		// render the global UIScenes
		UIController->RenderUI(Canvas);
		// restore viewport RT
		Canvas->SetRenderTarget(Viewport);

		// Create a FSceneViewFamilyContext for rendering the post process
		FSceneViewFamilyContext PostProcessViewFamily(
			Viewport,
			NULL,
			ShowFlags,
			GWorld->GetTimeSeconds(),
			GWorld->GetRealTimeSeconds(), 
			NULL, 
			FALSE,			
			FALSE,	// maintain scene color from player scene rendering
			TRUE	// enable the final copy to the viewport RT
			);
		// Compute the view's screen rectangle.
		INT X = 0;
		INT Y = 0;
		UINT SizeX = Viewport->GetSizeX();
		UINT SizeY = Viewport->GetSizeY();
		const FLOAT fFOV = 90.0f;
		// Take screen percentage option into account if percentage != 100.
		GSystemSettings.ScaleScreenCoords(X,Y,SizeX,SizeY);

		// add the new view to the scene
		FSceneView* View = new FSceneView(
			&PostProcessViewFamily,
			NULL,
			NULL,
			NULL,
			UISceneClient->UIScenePostProcess,
			NULL,
			NULL,
			X,
			Y,
			SizeX,
			SizeY,
			FCanvas::CalcViewMatrix(SizeX,SizeY,fFOV),
			FCanvas::CalcProjectionMatrix(SizeX,SizeY,fFOV,NEAR_CLIPPING_PLANE),
			FLinearColor::Black,
			FLinearColor(0,0,0,0),
			FLinearColor::White,
			TArray<FPrimitiveSceneInfo*>()
			);
		PostProcessViewFamily.Views.AddItem(View);
		// Render the post process pass
		BeginRenderingViewFamily(Canvas,&PostProcessViewFamily);
	}	

	// Render the UI if enabled.
	if( GTickAndRenderUI )
	{
		SCOPE_CYCLE_COUNTER(STAT_UIDrawingTime);

		for(INT PlayerIndex = 0;PlayerIndex < GEngine->GamePlayers.Num();PlayerIndex++)
		{
			ULocalPlayer* Player = GEngine->GamePlayers(PlayerIndex);
			if(Player->Actor)
			{
				FSceneView* View = PlayerViewMap.FindRef(PlayerIndex);
				if (View != NULL)
				{
					// Set the canvas transform for the player's view rectangle.
					CanvasObject->Init();
					CanvasObject->SizeX = appTrunc(View->SizeX);
					CanvasObject->SizeY = appTrunc(View->SizeY);
					CanvasObject->SceneView = View;
					CanvasObject->Update();
					Canvas->PushAbsoluteTransform(FTranslationMatrix(FVector(View->X,View->Y,0)));

					// Render the player's HUD.
					if( Player->Actor->myHUD )
					{
						SCOPE_CYCLE_COUNTER(STAT_HudTime);				

						Player->Actor->myHUD->Canvas = CanvasObject;
						Player->Actor->myHUD->eventPostRender();
						Player->Actor->myHUD->Canvas = NULL;
					}

					Canvas->PopTransform();

					// draw subtitles
					if (PlayerIndex == 0)
					{
						FVector2D MinPos(0.f, 0.f);
						FVector2D MaxPos(1.f, 1.f);
						eventGetSubtitleRegion(MinPos, MaxPos);

						UINT SizeX = Canvas->GetRenderTarget()->GetSizeX();
						UINT SizeY = Canvas->GetRenderTarget()->GetSizeY();
						FIntRect SubtitleRegion(appTrunc(SizeX * MinPos.X), appTrunc(SizeY * MinPos.Y), appTrunc(SizeX * MaxPos.X), appTrunc(SizeY * MaxPos.Y));
						FSubtitleManager::GetSubtitleManager()->DisplaySubtitles( Canvas, SubtitleRegion );
					}
				}
			}
		}

		// Reset the canvas for rendering to the full viewport.
		CanvasObject->Init();
		CanvasObject->SizeX = Viewport->GetSizeX();
		CanvasObject->SizeY = Viewport->GetSizeY();
		CanvasObject->SceneView = NULL;
		CanvasObject->Update();

		if( !bUISceneHasPostProcess )
		{
			// render the global UIScenes directly to the viewport RT
			UIController->RenderUI(Canvas);
		}

		// Allow the viewport to render additional stuff
		eventPostRender(CanvasObject);
	}

#if STATS
	DWORD DrawStatsBeginTime = appCycles();
#endif

	//@todo joeg: Move this stuff to a function, make safe to use on consoles by
	// respecting the various safe zones, and make it compile out.
#if CONSOLE
		const INT FPSXOffset	= 250;
		const INT StatsXOffset	= 100;
#else
		const INT FPSXOffset	= 70;
		const INT StatsXOffset	= 4;
#endif

#if !FINAL_RELEASE
	if( GIsTextureMemoryCorrupted )
	{
		DrawShadowedString(Canvas,
			100,
			200,
			TEXT("RAN OUT OF TEXTURE MEMORY, EXPECT CORRUPTION AND GPU HANGS!"),
			GEngine->MediumFont,
			FColor(255,0,0)
			);
	}

	if( GWorld->GetWorldInfo()->bMapNeedsLightingFullyRebuilt )
	{
		DrawShadowedString(Canvas,
			10,
			100,
			TEXT("LIGHTING NEEDS TO BE REBUILT"),
			GEngine->SmallFont,
			FColor(128,128,128)
			);
	}

	if (GWorld->bIsLevelStreamingFrozen)
	{
		DrawShadowedString(Canvas,
			10,
			160,
			TEXT("Level streaming frozen..."),
			GEngine->SmallFont,
			FColor(128,128,128)
			);
	}
#endif

	// Render the FPS counter.
	INT	Y = 20;
	if( GShowFpsCounter )
	{
#if STATS
		// Calculate the average FPS.
		FLOAT AverageMS = (FLOAT)GFPSCounter.GetAverage() * 1000.0;
		FLOAT AverageFPS = 1000.f / AverageMS;

		// Choose the counter color based on the average framerate.
		FColor FPSColor = AverageFPS < 20.0f ? FColor(255,0,0) : (AverageFPS < 29.5f ? FColor(255,255,0) : FColor(0,255,0));

		// Draw the FPS counter.
		DrawShadowedString(Canvas,
			Viewport->GetSizeX() - FPSXOffset,
			96,
			*FString::Printf(TEXT("%2.2f FPS"),AverageFPS),
#if PS3
			GEngine->MediumFont,
#else
			GEngine->SmallFont,
#endif
			FPSColor
			);

		// Draw the frame time.
		DrawShadowedString(Canvas,
			Viewport->GetSizeX() - FPSXOffset,
			120,
			*FString::Printf(TEXT("(%2.2f ms)"), AverageMS),
#if PS3
			GEngine->MediumFont,
#else
			GEngine->SmallFont,
#endif
			FPSColor
			);


#else
		// Calculate rough frame time even if stats are disabled.
		static DOUBLE LastTime	= 0;
		DOUBLE CurrentTime		= appSeconds();
		FLOAT FrameTime			= (CurrentTime - LastTime) * 1000;
		LastTime				= CurrentTime;

		// Draw the frame time in ms.
		DrawShadowedString(Canvas,
			Viewport->GetSizeX() - 220,
			120,
			*FString::Printf(TEXT("%4.2f ms"), FrameTime),
			GEngine->MediumFont,
			FColor( 255, 255, 255 )
			);
#endif
	}

	// Level stats.
	if( GShowLevelStats )
	{
		enum EStreamingStatus
		{
			LEVEL_Unloaded,
			LEVEL_UnloadedButStillAround,
			LEVEL_Loaded,
			LEVEL_MakingVisible,
			LEVEL_Visible,
			LEVEL_Preloading,
		};
		// Iterate over the world info's level streaming objects to find and see whether levels are loaded, visible or neither.
		TMap<FName,INT> StreamingLevels;	
		AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
		for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
		{
			ULevelStreaming* LevelStreaming = WorldInfo->StreamingLevels(LevelIndex);
			if( LevelStreaming 
			&&  LevelStreaming->PackageName != NAME_None 
			&&	LevelStreaming->PackageName != GWorld->GetOutermost()->GetFName() )
			{
				if( LevelStreaming->LoadedLevel && !LevelStreaming->bHasUnloadRequestPending )
				{
					if( GWorld->Levels.FindItemIndex( LevelStreaming->LoadedLevel ) != INDEX_NONE )
					{
						if( LevelStreaming->LoadedLevel->bHasVisibilityRequestPending )
						{
							StreamingLevels.Set( LevelStreaming->PackageName, LEVEL_MakingVisible );
						}
						else
						{
							StreamingLevels.Set( LevelStreaming->PackageName, LEVEL_Visible );
						}
					}
					else
					{
						StreamingLevels.Set( LevelStreaming->PackageName, LEVEL_Loaded );
					}
				}
				else
				{
					// See whether the level's world object is still around.
					UPackage* LevelPackage	= Cast<UPackage>(StaticFindObjectFast( UPackage::StaticClass(), NULL, LevelStreaming->PackageName ));
					UWorld*	  LevelWorld	= NULL;
					if( LevelPackage )
					{
						LevelWorld = Cast<UWorld>(StaticFindObjectFast( UWorld::StaticClass(), LevelPackage, NAME_TheWorld ));
					}

					if( LevelWorld )
					{
						StreamingLevels.Set( LevelStreaming->PackageName, LEVEL_UnloadedButStillAround );
					}
					else
					{
						StreamingLevels.Set( LevelStreaming->PackageName, LEVEL_Unloaded );
					}
				}
			}
		}

		UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
		if (GameEngine != NULL)
		{
			// toss in the levels being loaded by PrepareMapChange
			for( INT LevelIndex=0; LevelIndex < GameEngine->LevelsToLoadForPendingMapChange.Num(); LevelIndex++ )
			{
				const FName LevelName = GameEngine->LevelsToLoadForPendingMapChange(LevelIndex);
				StreamingLevels.Set(LevelName, LEVEL_Preloading);
			}
		}

		// Render unloaded levels in red, loaded ones in yellow and visible ones in green. Blue signifies that a level is unloaded but
		// hasn't been garbage collected yet.
		DrawShadowedString(Canvas, StatsXOffset, Y, TEXT("Level streaming"), GEngine->SmallFont, FLinearColor::White );
		Y+=12;


		ULevel *LevelPlayerIsIn = NULL;

		for( AController* Controller = GWorld->GetWorldInfo()->ControllerList; 
			Controller != NULL; 
			Controller = Controller->NextController
			)
		{
			APlayerController* PC = Cast<APlayerController>( Controller );

			if( ( PC != NULL )
				&&( PC->Pawn != NULL )
				)
			{
				// need to do a trace down here
				//TraceActor = Trace( out_HitLocation, out_HitNormal, TraceDest, TraceStart, false, TraceExtent, HitInfo, true );
				FCheckResult Hit(1.0f);
				DWORD TraceFlags;
				TraceFlags = TRACE_World;

				FVector TraceExtent(0,0,0);

				// this will not work for flying around :-(
				GWorld->SingleLineCheck( Hit, PC->Pawn, (PC->Pawn->Location-FVector(0, 0, 256 )), PC->Pawn->Location, TraceFlags, TraceExtent );

				if (Hit.Level != NULL)
				{
					LevelPlayerIsIn = Hit.Level;
				}
				else
				if (Hit.Actor != NULL)
				{
					LevelPlayerIsIn = Hit.Actor->GetLevel();
				}
				else
				if (Hit.Component != NULL)
				{
					LevelPlayerIsIn = Hit.Component->GetOwner()->GetLevel();
				}
			}
		}

		// now draw the "map" name
		FString MapName				= GWorld->CurrentLevel->GetOutermost()->GetName();
		FString LevelPlayerIsInName = LevelPlayerIsIn != NULL ? LevelPlayerIsIn->GetOutermost()->GetName() : TEXT("None");

		if( LevelPlayerIsInName == MapName )
		{
			MapName = *FString::Printf( TEXT("-> %s"), *MapName );
		}
		else
		{
			MapName = *FString::Printf( TEXT("   %s"), *MapName );
		}

		DrawShadowedString(Canvas, StatsXOffset, Y, *MapName, GEngine->SmallFont, FColor(127,127,127) );
		Y+=12;

		// now draw the levels
		for( TMap<FName,INT>::TIterator It(StreamingLevels); It; ++It )
		{
			FString	LevelName	= It.Key().ToString();
			INT		Status		= It.Value();
			FColor	Color		= FColor(255,255,255);
			switch( Status )
			{
			case LEVEL_Visible:
				Color = FColor(255,0,0);	// red  loaded and visible
				break;
			case LEVEL_MakingVisible:
				Color = FColor(255,128,0);	// orange, in process of being made visible
				break;
			case LEVEL_Loaded:
				Color = FColor(255,255,0);	// yellow loaded but not visible
				break;
			case LEVEL_UnloadedButStillAround:
				Color = FColor(0,0,255);	// blue  (GC needs to occur to remove this)
				break;
			case LEVEL_Unloaded:
				Color = FColor(0,255,0);	// green
				break;
			case LEVEL_Preloading:
				Color = FColor(255,0,255);	// purple (preloading)
				break;
			default:
				break;
			};

			UPackage* LevelPackage = FindObject<UPackage>( NULL, *LevelName );

			if( LevelPackage 
			&& (LevelPackage->GetLoadTime() > 0) 
			&& (Status != LEVEL_Unloaded) )
			{
				LevelName += FString::Printf(TEXT(" - %4.1f sec"), LevelPackage->GetLoadTime());
			}
			else if( UObject::GetAsyncLoadPercentage( *LevelName ) >= 0 )
			{
				INT Percentage = appTrunc( UObject::GetAsyncLoadPercentage( *LevelName ) );
				LevelName += FString::Printf(TEXT(" - %3i %%"), Percentage ); 
			}

			if( LevelPlayerIsInName == LevelName )
			{
				LevelName = *FString::Printf( TEXT("->  %s"), *LevelName );
			}
			else
			{
				LevelName = *FString::Printf( TEXT("    %s"), *LevelName );
			}

			DrawShadowedString(Canvas, StatsXOffset + 4, Y, *LevelName, GEngine->SmallFont, Color );
			Y+=12;
		}
	}

#if STATS
	GStatManager.Render(Canvas,StatsXOffset,Y);
#endif

	// Render the stat chart.
	if(GStatChart)
	{
		GStatChart->Render(Viewport, Canvas);
	}

	if( GIsDumpingMovie || GScreenShotRequest )
	{
		Canvas->Flush();

		// Read the contents of the viewport into an array.
		TArray<FColor>	Bitmap;
		if(Viewport->ReadPixels(Bitmap))
		{
			check(Bitmap.Num() == Viewport->GetSizeX() * Viewport->GetSizeY());

			// Create screenshot folder if not already present.
			GFileManager->MakeDirectory( *GSys->ScreenShotPath, TRUE );

			FString ScreenFileName( GSys->ScreenShotPath * (GIsDumpingMovie ? TEXT("MovieFrame") : TEXT("ScreenShot")) );

			// Save the contents of the array to a bitmap file.
			appCreateBitmap(*ScreenFileName,Viewport->GetSizeX(),Viewport->GetSizeY(),&Bitmap(0),GFileManager);			
		}
		GScreenShotRequest=FALSE;
	}

#if PS3
	extern void PrintFrameRatePS3();
	PrintFrameRatePS3();
#endif

#if STATS
	DWORD DrawStatsEndTime = appCycles();
	SET_CYCLE_COUNTER(STAT_DrawStats, DrawStatsEndTime - DrawStatsBeginTime, 1);
#endif
}

void UGameViewportClient::Precache()
{
	if(!GIsEditor)
	{
		// Precache sounds...
		UAudioDevice* AudioDevice = GEngine->Client ? GEngine->Client->GetAudioDevice() : NULL;
		if( AudioDevice )
		{
			debugf(TEXT("Precaching sounds..."));
			for(TObjectIterator<USoundNodeWave> It;It;++It)
			{
				USoundNodeWave* SoundNodeWave = *It;
				AudioDevice->Precache( SoundNodeWave );
			}
			debugf(TEXT("Precaching sounds completed..."));
		}
	}

	// Log time till first precache is finished.
	static UBOOL bIsFirstCallOfFunction = TRUE;
	if( bIsFirstCallOfFunction )
	{
		debugf(TEXT("%5.2f seconds passed since startup."),appSeconds()-GStartTime);
		bIsFirstCallOfFunction = FALSE;
	}
}

void UGameViewportClient::LostFocus(FViewport* Viewport)
{
	// UpdateMouseCapture will capture the mouse as long as the game isn't paused
	// and the UI isn't active; we've just lost focus and want to force mouse capture
	// to be lost, so call the function directly instead of using UpdateMouseCapture
	Viewport->CaptureMouseInput(FALSE);
}

void UGameViewportClient::ReceivedFocus(FViewport* Viewport)
{
	UpdateMouseCapture();
}

void UGameViewportClient::CloseRequested(FViewport* Viewport)
{
	check(Viewport == this->Viewport);
	if( GFullScreenMovie )
	{
		// force movie playback to stop
		GFullScreenMovie->GameThreadStopMovie(0,FALSE,TRUE);
	}
	GEngine->Client->CloseViewport(this->Viewport);
	SetViewport(NULL);
}

UBOOL UGameViewportClient::Exec(const TCHAR* Cmd,FOutputDevice& Ar)
{
	if( ParseCommand(&Cmd,TEXT("SHOW")) )
	{
		struct { const TCHAR* Name; EShowFlags Flag; }	Flags[] =
		{
			{ TEXT("BOUNDS"),				SHOW_Bounds					},
			{ TEXT("BSP"),					SHOW_BSP					},
			{ TEXT("BSPSPLIT"),				SHOW_BSPSplit				},
			{ TEXT("CAMFRUSTUMS"),			SHOW_CamFrustums			},
			{ TEXT("COLLISION"),			SHOW_Collision				},
			{ TEXT("CONSTRAINTS"),			SHOW_Constraints			},
			{ TEXT("COVER"),				SHOW_Cover					},
			{ TEXT("DECALINFO"),			SHOW_DecalInfo				},
			{ TEXT("DECALS"),				SHOW_Decals					},
			{ TEXT("DYNAMICSHADOWS"),		SHOW_DynamicShadows			},
			{ TEXT("FOG"),					SHOW_Fog					},
			{ TEXT("FOLIAGE"),				SHOW_Foliage				},
			{ TEXT("HITPROXIES"),			SHOW_HitProxies				},
			{ TEXT("LENSFLARES"),			SHOW_LensFlares				},
			{ TEXT("LEVELCOLORATION"),		SHOW_LevelColoration		},
			{ TEXT("MESHEDGES"),			SHOW_MeshEdges				},
			{ TEXT("MISSINGCOLLISION"),		SHOW_MissingCollisionModel	},
			{ TEXT("NAVNODES"),				SHOW_NavigationNodes		},
			{ TEXT("PARTICLES"),			SHOW_Particles				},
			{ TEXT("PATHS"),				SHOW_Paths					},
			{ TEXT("PORTALS"),				SHOW_Portals				},
			{ TEXT("POSTPROCESS"),			SHOW_PostProcess			},
			{ TEXT("SHADOWFRUSTUMS"),		SHOW_ShadowFrustums			},
			{ TEXT("SKELMESHES"),			SHOW_SkeletalMeshes			},
			{ TEXT("SKELETALMESHES"),		SHOW_SkeletalMeshes			},
			{ TEXT("SPEEDTREES"),			SHOW_SpeedTrees				},
			{ TEXT("SPRITES"),				SHOW_Sprites				},
			{ TEXT("STATICMESHES"),			SHOW_StaticMeshes			},
			{ TEXT("TERRAIN"),				SHOW_Terrain				},
			{ TEXT("TERRAINPATCHES"),		SHOW_TerrainPatches			},
			{ TEXT("UNLITTRANSLUCENCY"),	SHOW_UnlitTranslucency		},
			{ TEXT("ZEROEXTENT"),			SHOW_CollisionZeroExtent	},
			{ TEXT("NONZEROEXTENT"),		SHOW_CollisionNonZeroExtent	},
			{ TEXT("RIGIDBODY"),			SHOW_CollisionRigidBody		},
			{ TEXT("TERRAINCOLLISION"),		SHOW_TerrainCollision		},
		};

		// First, look for skeletal mesh shwo commands

		UBOOL bUpdateSkelMeshCompDebugFlags = FALSE;
		static UBOOL bShowSkelBones = FALSE;
		static UBOOL bShowPrePhysSkelBones = FALSE;

		if(ParseCommand(&Cmd,TEXT("BONES")))
		{
			bShowSkelBones = !bShowSkelBones;
			bUpdateSkelMeshCompDebugFlags = TRUE;
		}
		else if(ParseCommand(&Cmd,TEXT("PREPHYSBONES")))
		{
			bShowPrePhysSkelBones = !bShowPrePhysSkelBones;
			bUpdateSkelMeshCompDebugFlags = TRUE;
		}

		// If we changed one of the skel mesh debug show flags, set it on each of the components in the GWorld.
		if(bUpdateSkelMeshCompDebugFlags)
		{
			for (TObjectIterator<USkeletalMeshComponent> It; It; ++It)
			{
				USkeletalMeshComponent* SkelComp = *It;
				if( SkelComp->GetScene() == GWorld->Scene )
				{
					SkelComp->bDisplayBones = bShowSkelBones;
					SkelComp->bShowPrePhysBones = bShowPrePhysSkelBones;
					SkelComp->BeginDeferredReattach();
				}
			}

			// Now we are done.
			return TRUE;
		}

		// Search for a specific show flag and toggle it if found.
		for(UINT FlagIndex = 0;FlagIndex < ARRAY_COUNT(Flags);FlagIndex++)
		{
			if(ParseCommand(&Cmd,Flags[FlagIndex].Name))
			{
				ShowFlags ^= Flags[FlagIndex].Flag;
				// special case: for the SHOW_Collision flag, we need to un-hide any primitive components that collide so their collision geometry gets rendered
				if (Flags[FlagIndex].Flag == SHOW_Collision ||
					Flags[FlagIndex].Flag == SHOW_CollisionNonZeroExtent || 
					Flags[FlagIndex].Flag == SHOW_CollisionZeroExtent || 
					Flags[FlagIndex].Flag == SHOW_CollisionRigidBody )
				{
					// Ensure that all flags, other than the one we just typed in, is off.
					ShowFlags &= ~(Flags[FlagIndex].Flag ^ (SHOW_Collision_Any | SHOW_Collision));

					for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
					{
						UPrimitiveComponent* PrimitiveComponent = *It;
						if( PrimitiveComponent->HiddenGame && PrimitiveComponent->ShouldCollide() && PrimitiveComponent->GetScene() == GWorld->Scene )
						{
							check( !GIsEditor || (PrimitiveComponent->GetOutermost()->PackageFlags & PKG_PlayInEditor) );
							PrimitiveComponent->SetHiddenGame(false);
						}
					}
				}
				else
				if (Flags[FlagIndex].Flag == SHOW_Paths || (GIsGame && Flags[FlagIndex].Flag == SHOW_Cover))
				{
					UBOOL bShowPaths = (ShowFlags & SHOW_Paths) != 0;
					UBOOL bShowCover = (ShowFlags & SHOW_Cover) != 0;
					// make sure all nav points have path rendering components
					for (FActorIterator It; It; ++It)
					{
						ACoverLink *Link = Cast<ACoverLink>(*It);
						if (Link != NULL)
						{
							UBOOL bHasComponent = FALSE;
							for (INT Idx = 0; Idx < Link->Components.Num(); Idx++)
							{
								UCoverMeshComponent *PathRenderer = Cast<UCoverMeshComponent>(Link->Components(Idx));
								if (PathRenderer != NULL)
								{
									PathRenderer->SetHiddenGame(!(bShowPaths || bShowCover));
									bHasComponent = TRUE;
									break;
								}
							}
							if (!bHasComponent)
							{
								UClass *MeshCompClass = FindObject<UClass>(ANY_PACKAGE,*GEngine->DynamicCoverMeshComponentName);
								if (MeshCompClass == NULL)
								{
									MeshCompClass = UCoverMeshComponent::StaticClass();
								}
								UCoverMeshComponent *PathRenderer = ConstructObject<UCoverMeshComponent>(MeshCompClass,Link);
								PathRenderer->SetHiddenGame(!(bShowPaths || bShowCover));
								Link->AttachComponent(PathRenderer);
							}
						}
						else
						{
							ANavigationPoint *Nav = Cast<ANavigationPoint>(*It);
							if (Nav != NULL)
							{
								UBOOL bHasComponent = FALSE;
								for (INT Idx = 0; Idx < Nav->Components.Num(); Idx++)
								{
									UPathRenderingComponent *PathRenderer = Cast<UPathRenderingComponent>(Nav->Components(Idx));
									if (PathRenderer != NULL)
									{
										bHasComponent = TRUE;
										PathRenderer->SetHiddenGame(!bShowPaths);
										break;
									}
								}
								if (!bHasComponent)
								{
									UPathRenderingComponent *PathRenderer = ConstructObject<UPathRenderingComponent>(UPathRenderingComponent::StaticClass(),Nav);
									PathRenderer->SetHiddenGame(!bShowPaths);
									Nav->AttachComponent(PathRenderer);
								}
							}
						}
					}
				}
				else
				if (Flags[FlagIndex].Flag == SHOW_TerrainCollision)
				{
					ATerrain::ShowCollisionCallback((ShowFlags & SHOW_TerrainCollision) != 0);
				}
				return TRUE;
			}
		}

		// The specified flag wasn't found -- list all flags and their current value.
		for(UINT FlagIndex = 0;FlagIndex < ARRAY_COUNT(Flags);FlagIndex++)
		{
			Ar.Logf(TEXT("%s : %s"),
				Flags[FlagIndex].Name,
				(ShowFlags & Flags[FlagIndex].Flag) ? TEXT("TRUE") :TEXT("FALSE"));
		}
		return TRUE;
	}
	else if (ParseCommand(&Cmd,TEXT("VIEWMODE")))
	{
#ifndef _DEBUG
		// If there isn't a cheat manager, exit out
		UBOOL bCheatsEnabled = FALSE;
		for (FPlayerIterator It((UEngine*)GetOuter()); It; ++It)
		{
			if (It->Actor != NULL && It->Actor->CheatManager != NULL)
			{
				bCheatsEnabled = TRUE;
				break;
			}
		}
		if (!bCheatsEnabled)
		{
			return TRUE;
		}
#endif

		if( ParseCommand(&Cmd,TEXT("WIREFRAME")) )
		{
			ShowFlags &= ~SHOW_ViewMode_Mask;
			ShowFlags |= SHOW_ViewMode_Wireframe;
		}
		else if( ParseCommand(&Cmd,TEXT("BRUSHWIREFRAME")) )
		{
			ShowFlags &= ~SHOW_ViewMode_Mask;
			ShowFlags |= SHOW_ViewMode_BrushWireframe;
		}
		else if( ParseCommand(&Cmd,TEXT("UNLIT")) )
		{
			ShowFlags &= ~SHOW_ViewMode_Mask;
			ShowFlags |= SHOW_ViewMode_Unlit;
		}
		else if( ParseCommand(&Cmd,TEXT("LIGHTINGONLY")) )
		{
			ShowFlags &= ~SHOW_ViewMode_Mask;
			ShowFlags |= SHOW_ViewMode_LightingOnly;
		}			
		else if( ParseCommand(&Cmd,TEXT("LIGHTCOMPLEXITY")) )
		{
			ShowFlags &= ~SHOW_ViewMode_Mask;
			ShowFlags |= SHOW_ViewMode_LightComplexity;
		}
		else if( ParseCommand(&Cmd,TEXT("SHADERCOMPLEXITY")) )
		{
			ShowFlags &= ~SHOW_ViewMode_Mask;
			ShowFlags |= SHOW_ViewMode_ShaderComplexity;
		}
		else if( ParseCommand(&Cmd,TEXT("TEXTUREDENSITY")))
		{
			ShowFlags &= ~SHOW_ViewMode_Mask;
			ShowFlags |= SHOW_ViewMode_TextureDensity;
		}
		else
		{
			ShowFlags &= ~SHOW_ViewMode_Mask;
			ShowFlags |= SHOW_ViewMode_Lit;
		}

		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("NEXTVIEWMODE")))
	{
#ifndef _DEBUG
		// If there isn't a cheat manager, exit out
		UBOOL bCheatsEnabled = FALSE;
		for (FPlayerIterator It((UEngine*)GetOuter()); It; ++It)
		{
			if (It->Actor != NULL && It->Actor->CheatManager != NULL)
			{
				bCheatsEnabled = TRUE;
				break;
			}
		}
		if (!bCheatsEnabled)
		{
			return TRUE;
		}
#endif

		QWORD OldShowFlags = ShowFlags;
		ShowFlags &= ~SHOW_ViewMode_Mask;
		switch (OldShowFlags & SHOW_ViewMode_Mask)
		{
			case SHOW_ViewMode_Lit:
				ShowFlags |= SHOW_ViewMode_LightingOnly;
				break;
			case SHOW_ViewMode_LightingOnly:
				ShowFlags |= SHOW_ViewMode_LightComplexity;
				break;
			case SHOW_ViewMode_LightComplexity:
				ShowFlags |= SHOW_ViewMode_Wireframe;
				break;
			case SHOW_ViewMode_Wireframe:
				ShowFlags |= SHOW_ViewMode_BrushWireframe;
				break;
			case SHOW_ViewMode_BrushWireframe:
				ShowFlags |= SHOW_ViewMode_Unlit;
				break;
			case SHOW_ViewMode_Unlit:
				ShowFlags |= SHOW_ViewMode_TextureDensity;
				break;
			case SHOW_ViewMode_TextureDensity:
				ShowFlags |= SHOW_ViewMode_Lit;
				break;
			default:
				ShowFlags |= SHOW_ViewMode_Lit;
				break;
		}

		return TRUE;
	}
	else if (ParseCommand(&Cmd, TEXT("PREVVIEWMODE")))
	{
#ifndef _DEBUG
		// If there isn't a cheat manager, exit out
		UBOOL bCheatsEnabled = FALSE;
		for (FPlayerIterator It((UEngine*)GetOuter()); It; ++It)
		{
			if (It->Actor != NULL && It->Actor->CheatManager != NULL)
			{
				bCheatsEnabled = TRUE;
				break;
			}
		}
		if (!bCheatsEnabled)
		{
			return TRUE;
		}
#endif

		QWORD OldShowFlags = ShowFlags;
		ShowFlags &= ~SHOW_ViewMode_Mask;
		switch (OldShowFlags & SHOW_ViewMode_Mask)
		{
			case SHOW_ViewMode_Lit:
				ShowFlags |= SHOW_ViewMode_TextureDensity;
				break;
			case SHOW_ViewMode_LightingOnly:
				ShowFlags |= SHOW_ViewMode_Lit;
				break;
			case SHOW_ViewMode_LightComplexity:
				ShowFlags |= SHOW_ViewMode_LightingOnly;
				break;
			case SHOW_ViewMode_Wireframe:
				ShowFlags |= SHOW_ViewMode_LightComplexity;
				break;
			case SHOW_ViewMode_BrushWireframe:
				ShowFlags |= SHOW_ViewMode_Wireframe;
				break;
			case SHOW_ViewMode_Unlit:
				ShowFlags |= SHOW_ViewMode_BrushWireframe;
				break;
			case SHOW_ViewMode_TextureDensity:
				ShowFlags |= SHOW_ViewMode_Unlit;
				break;
			default:
				ShowFlags |= SHOW_ViewMode_Lit;
				break;
		}

		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("STAT")) )
	{
		const TCHAR* Temp = Cmd;
		// Check for FPS counter
		if (ParseCommand(&Temp,TEXT("FPS")))
		{
			GShowFpsCounter ^= TRUE;
			return TRUE;
		}
		// Check for Level stats.
		else if (ParseCommand(&Temp,TEXT("LEVELS")))
		{
			GShowLevelStats ^= TRUE;
			return TRUE;
		}
#if STATS
		// Forward the call to the stat manager
		else
		{
			return GStatManager.Exec(Cmd,Ar);
		}
#else
		return FALSE;
#endif
	}
	else
	if( ParseCommand(&Cmd,TEXT("DUMPDYNAMICSHADOWSTATS")) )
	{
		GWorld->bGatherDynamicShadowStatsSnapshot = TRUE;
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("PRECACHE")) )
	{
		Precache();
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("SETRES")) )
	{
		if(Viewport)
		{
			INT X=appAtoi(Cmd);
			const TCHAR* CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
			INT Y=appAtoi(CmdTemp);
			Cmd = CmdTemp;
			UBOOL	Fullscreen = Viewport->IsFullscreen();
			if(appStrchr(Cmd,'w') || appStrchr(Cmd,'W'))
				Fullscreen = 0;
			else if(appStrchr(Cmd,'f') || appStrchr(Cmd,'F'))
				Fullscreen = 1;
			if( X && Y )
				ViewportFrame->Resize(X,Y,Fullscreen);
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("TILEDSHOT")) )
	{
		GIsTiledScreenshot = TRUE;
		GScreenshotResolutionMultiplier = appAtoi(Cmd);
		GScreenshotResolutionMultiplier = Clamp<INT>( GScreenshotResolutionMultiplier, 2, 128 );
		const TCHAR* CmdTemp = appStrchr(Cmd, ' ');
		GScreenshotMargin = CmdTemp ? Clamp<INT>(appAtoi(CmdTemp), 0, 320) : 64;
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("SHOT")) || ParseCommand(&Cmd,TEXT("SCREENSHOT")) )
	{
		if(Viewport)
		{
			GScreenShotRequest=TRUE;
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("KILLPARTICLES")) )
	{
		// Don't kill in the Editor to avoid potential content clobbering.
		if( !GIsEditor )
		{
			extern UBOOL GIsAllowingParticles;
			// Deactivate system and kill existing particles.
			for( TObjectIterator<UParticleSystemComponent> It; It; ++It )
			{
				UParticleSystemComponent* ParticleSystemComponent = *It;
				ParticleSystemComponent->DeactivateSystem();
				ParticleSystemComponent->KillParticlesForced();
			}
			// No longer initialize particles from here on out.
			GIsAllowingParticles = FALSE;
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("FORCESKELLOD")) )
	{
		INT ForceLod = 0;
		if(Parse(Cmd,TEXT("LOD="),ForceLod))
		{
			ForceLod++;
		}

		for (TObjectIterator<USkeletalMeshComponent> It; It; ++It)
		{
			USkeletalMeshComponent* SkelComp = *It;
			if( SkelComp->GetScene() == GWorld->Scene && !SkelComp->IsTemplate())
			{
				SkelComp->ForcedLodModel = ForceLod;
			}
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLEOCCLUSION")) )
	{
		extern UBOOL GIgnoreAllOcclusionQueries;
		GIgnoreAllOcclusionQueries = !GIgnoreAllOcclusionQueries;
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLEUI")) )
	{
		GTickAndRenderUI = !GTickAndRenderUI;
		return TRUE;
	}
	else if ( UIController->Exec(Cmd,Ar) )
	{
		return TRUE;
	}
	else if(ScriptConsoleExec(Cmd,Ar,NULL))
	{
		return TRUE;
	}
	else if( GEngine->Exec(Cmd,Ar) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * Dynamically assign Controller to Player and set viewport.
 *
 * @param    PC - new player controller to assign to player
 **/
void UPlayer::SwitchController(class APlayerController* PC)
{
	// Detach old player.
	if( this->Actor )
	{
		this->Actor->Player = NULL;
	}

	// Set the viewport.
	PC->Player = this;
	this->Actor = PC;
}

ULocalPlayer::ULocalPlayer()
{
	if ( !IsTemplate() )
	{
		ViewState = AllocateViewState();

		if( !PlayerPostProcess )
		{
			// initialize to global post process if one is not set
			if (InsertPostProcessingChain(GEngine->DefaultPostProcess, 0, TRUE) == FALSE)
			{
				warnf(TEXT("LocalPlayer %d - Failed to setup default post process..."), ControllerId);
			}
		}

		// Initialize the actor visibility history.
		ActorVisibilityHistory.Init();
	}
}

UBOOL ULocalPlayer::GetActorVisibility(AActor* TestActor) const
{
	return ActorVisibilityHistory.GetActorVisibility(TestActor);
}

UBOOL ULocalPlayer::SpawnPlayActor(const FString& URL,FString& OutError)
{
	if(GWorld->IsServer())
	{
		Actor = GWorld->SpawnPlayActor(this,ROLE_SimulatedProxy,FURL(NULL,*URL,TRAVEL_Absolute),OutError);
	}
	else
	{
		// The PlayerController gets replicated from the client though the engine assumes that every Player always has
		// a valid PlayerController so we spawn a dummy one that is going to be replaced later.
		Actor = CastChecked<APlayerController>(GWorld->SpawnActor(APlayerController::StaticClass()));
	}
	return Actor != NULL;
}

void ULocalPlayer::SendSplitJoin()
{
	if (GWorld == NULL || GWorld->NetDriver == NULL || GWorld->NetDriver->ServerConnection == NULL || GWorld->NetDriver->ServerConnection->State != USOCK_Open)
	{
		debugf(NAME_Warning, TEXT("SendSplitJoin(): Not connected to a server"));
	}
	else if (!bSentSplitJoin)
	{
		// make sure we don't already have a connection
		UBOOL bNeedToSendJoin = FALSE;
		if (Actor == NULL)
		{
			bNeedToSendJoin = TRUE;
		}
		else if (GWorld->NetDriver->ServerConnection->Actor != Actor)
		{
			bNeedToSendJoin = TRUE;
			for (INT i = 0; i < GWorld->NetDriver->ServerConnection->Children.Num(); i++)
			{
				if (GWorld->NetDriver->ServerConnection->Children(i)->Actor == Actor)
				{
					bNeedToSendJoin = FALSE;
					break;
				}
			}
		}

		if (bNeedToSendJoin)
		{
			//@todo: send a seperate URL?
			GWorld->NetDriver->ServerConnection->Logf(TEXT("JOINSPLIT"));
			bSentSplitJoin = TRUE;
		}
	}
}

void ULocalPlayer::FinishDestroy()
{
	if ( !IsTemplate() )
	{
		ViewState->Destroy();
		ViewState = NULL;
	}
	Super::FinishDestroy();
}

/**
 * Add the given post process chain to the chain at the given index.
 *
 *	@param	InChain		The post process chain to insert.
 *	@param	InIndex		The position to insert the chain in the complete chain.
 *						If -1, insert it at the end of the chain.
 *	@param	bInClone	If TRUE, create a deep copy of the chains effects before insertion.
 */
UBOOL ULocalPlayer::InsertPostProcessingChain(class UPostProcessChain* InChain, INT InIndex, UBOOL bInClone)
{
	if (InChain == NULL)
	{
		return FALSE;
	}

	// Create a new chain...
	UPostProcessChain* ClonedChain = Cast<UPostProcessChain>(StaticDuplicateObject(InChain, InChain, UObject::GetTransientPackage(), TEXT("None"), RF_AllFlags & (~RF_Standalone)));
	if (ClonedChain)
	{
		INT InsertIndex = 0;
		if ((InIndex == -1) || (InIndex >= PlayerPostProcessChains.Num()))
		{
			InsertIndex = PlayerPostProcessChains.Num();
		}
		else
		{
			InsertIndex = InIndex;
		}

		PlayerPostProcessChains.InsertItem(ClonedChain, InsertIndex);

		RebuildPlayerPostProcessChain();

		return TRUE;
	}

	return FALSE;
}

/**
 * Remove the post process chain at the given index.
 *
 *	@param	InIndex		The position to insert the chain in the complete chain.
 */
UBOOL ULocalPlayer::RemovePostProcessingChain(INT InIndex)
{
	if ((InIndex >= 0) && (InIndex < PlayerPostProcessChains.Num()))
	{
		PlayerPostProcessChains.Remove(InIndex);
		RebuildPlayerPostProcessChain();
		return TRUE;
	}

	return FALSE;
}

/**
 * Remove all post process chains.
 *
 *	@return	boolean		TRUE if the chain array was cleared
 *						FALSE if not
 */
UBOOL ULocalPlayer::RemoveAllPostProcessingChains()
{
	PlayerPostProcessChains.Empty();
	RebuildPlayerPostProcessChain();
	return TRUE;
}

/**
 *	Get the PPChain at the given index.
 *
 *	@param	InIndex				The index of the chain to retrieve.
 *
 *	@return	PostProcessChain	The post process chain if found; NULL if not.
 */
class UPostProcessChain* ULocalPlayer::GetPostProcessChain(INT InIndex)
{
	if ((InIndex >= 0) && (InIndex < PlayerPostProcessChains.Num()))
	{
		return PlayerPostProcessChains(InIndex);
	}
	return NULL;
}

/**
 *	Forces the PlayerPostProcess chain to be rebuilt.
 *	This should be called if a PPChain is retrieved using the GetPostProcessChain,
 *	and is modified directly.
 */
void ULocalPlayer::TouchPlayerPostProcessChain()
{
	RebuildPlayerPostProcessChain();
}

/**
 * Helper structure for sorting textures by relative cost.
 */
struct FSortedTexture 
{
 	INT		OrigSizeX;
 	INT		OrigSizeY;
	INT		CurSizeX;
	INT		CurSizeY;
 	INT		LODBias;
	INT		MaxSize;
 	INT		CurrentSize;
	FString Name;
 	INT		LODGroup;
 	UBOOL	bIsStreaming;

	/** Constructor, initializing every member variable with passed in values. */
	FSortedTexture(	INT InOrigSizeX, INT InOrigSizeY, INT InCurSizeX, INT InCurSizeY, INT InLODBias, INT InMaxSize, INT InCurrentSize, const FString& InName, INT InLODGroup, UBOOL bInIsStreaming )
	:	OrigSizeX( InOrigSizeX )
	,	OrigSizeY( InOrigSizeY )
	,	CurSizeX( InCurSizeX )
	,	CurSizeY( InCurSizeY )
 	,	LODBias( InLODBias )
	,	MaxSize( InMaxSize )
	,	CurrentSize( InCurrentSize )
	,	Name( InName )
 	,	LODGroup( InLODGroup )
 	,	bIsStreaming( bInIsStreaming )
	{}
};
IMPLEMENT_COMPARE_CONSTREF( FSortedTexture, UnPlayer, { return B.MaxSize - A.MaxSize; } )

/** Helper struct for sorting anim sets by size */
struct FSortedAnimSet
{
	FString Name;
	INT		Size;

	FSortedAnimSet( const FString& InName, INT InSize )
	:	Name(InName)
	,	Size(InSize)
	{}
};
IMPLEMENT_COMPARE_CONSTREF( FSortedAnimSet, UnPlayer, { return B.Size - A.Size; } )


/**
 *	Rebuilds the PlayerPostProcessChain.
 *	This should be called whenever the chain array has items inserted/removed.
 */
void ULocalPlayer::RebuildPlayerPostProcessChain()
{
	// Release the current PlayerPostProcessChain.
	if (PlayerPostProcessChains.Num() == 0)
	{
		PlayerPostProcess = NULL;
		return;
	}

	PlayerPostProcess = ConstructObject<UPostProcessChain>(UPostProcessChain::StaticClass(), UObject::GetTransientPackage());
	check(PlayerPostProcess);
	
	UBOOL bUberEffectInserted = FALSE;
	for (INT ChainIndex = 0; ChainIndex < PlayerPostProcessChains.Num(); ChainIndex++)
	{
		UPostProcessChain* PPChain = PlayerPostProcessChains(ChainIndex);
		if (PPChain)
		{
			for (INT EffectIndex = 0; EffectIndex < PPChain->Effects.Num(); EffectIndex++)
			{
				UPostProcessEffect* PPEffect = PPChain->Effects(EffectIndex);
				if (PPEffect)
				{
					if (PPEffect->IsA(UUberPostProcessEffect::StaticClass())== TRUE)
					{
						if (bUberEffectInserted == FALSE)
						{
							PlayerPostProcess->Effects.AddItem(PPEffect);
							bUberEffectInserted = TRUE;
						}
						else
						{
							warnf(TEXT("LocalPlayer %d - Multiple UberPostProcessEffects present..."), ControllerId);
						}
					}
					else
					{
						PlayerPostProcess->Effects.AddItem(PPEffect);
					}
				}
			}
		}
	}
}

//
void ULocalPlayer::UpdatePostProcessSettings(const FVector& ViewLocation)
{
	// Find the post-process settings for the view.
	FPostProcessSettings NewSettings;
	APostProcessVolume* NewVolume;

	// Give priority to local PP override flag
	if ( bOverridePostProcessSettings )
	{
		NewVolume = NULL;
		NewSettings = PostProcessSettingsOverride;
		CurrentPPInfo.BlendStartTime = PPSettingsOverrideStartBlend;
	}
	// If not forcing an override on the LocalPlayer, see if we have Camera that wants to override PP settings
	// If so, we just grab those settings straight away and return - no blending.
	else if(Actor && Actor->PlayerCamera && Actor->PlayerCamera->bCamOverridePostProcess)
	{
		NewVolume = NULL;
		CurrentPPInfo.LastSettings = Actor->PlayerCamera->CamPostProcessSettings;
		return;
	}
	else
	{
		NewVolume = GWorld->GetWorldInfo()->GetPostProcessSettings(ViewLocation,TRUE,NewSettings);
	}
	
	// Give the camera an opportunity to do any non-overriding modifications (e.g. additive effects)
	if (Actor != NULL)
	{
		Actor->ModifyPostProcessSettings(NewSettings);
	}


	FLOAT CurrentWorldTime = GWorld->GetRealTimeSeconds();

	// Update info for when a new volume goes into use
	if( CurrentPPInfo.LastVolumeUsed != NewVolume )
	{
		CurrentPPInfo.LastVolumeUsed = NewVolume;
		CurrentPPInfo.BlendStartTime = CurrentWorldTime;
	}

	// Calculate the blend factors.
	const FLOAT DeltaTime = Max(CurrentWorldTime - CurrentPPInfo.LastBlendTime,0.f);
	const FLOAT ElapsedBlendTime = Max(CurrentPPInfo.LastBlendTime - CurrentPPInfo.BlendStartTime,0.f);

	// Calculate the blended settings.
	FPostProcessSettings BlendedSettings;
	const FPostProcessSettings& CurrentSettings = CurrentPPInfo.LastSettings;

	// toggles
	BlendedSettings.bEnableBloom = NewSettings.bEnableBloom;
	BlendedSettings.bEnableDOF = NewSettings.bEnableDOF;
	BlendedSettings.bEnableMotionBlur = NewSettings.bEnableMotionBlur;
	BlendedSettings.bEnableSceneEffect = NewSettings.bEnableSceneEffect;

	// calc bloom lerp amount
	FLOAT BloomFade = 1.f;
	const FLOAT RemainingBloomBlendTime = Max(NewSettings.Bloom_InterpolationDuration - ElapsedBlendTime,0.f);
	if(RemainingBloomBlendTime > DeltaTime)
	{
		BloomFade = Clamp<FLOAT>(DeltaTime / RemainingBloomBlendTime,0.f,1.f);
	}
	// bloom values
	BlendedSettings.Bloom_Scale = Lerp<FLOAT>(CurrentSettings.Bloom_Scale,NewSettings.Bloom_Scale,BloomFade);

	// calc dof lerp amount
	FLOAT DOFFade = 1.f;
	const FLOAT RemainingDOFBlendTime = Max(NewSettings.DOF_InterpolationDuration - ElapsedBlendTime,0.f);
	if(RemainingDOFBlendTime > DeltaTime)
	{
		DOFFade = Clamp<FLOAT>(DeltaTime / RemainingDOFBlendTime,0.f,1.f);
	}
	// dof values		
	BlendedSettings.DOF_FalloffExponent = Lerp<FLOAT>(CurrentSettings.DOF_FalloffExponent,NewSettings.DOF_FalloffExponent,DOFFade);
	BlendedSettings.DOF_BlurKernelSize = Lerp<FLOAT>(CurrentSettings.DOF_BlurKernelSize,NewSettings.DOF_BlurKernelSize,DOFFade);
	BlendedSettings.DOF_MaxNearBlurAmount = Lerp<FLOAT>(CurrentSettings.DOF_MaxNearBlurAmount,NewSettings.DOF_MaxNearBlurAmount,DOFFade);
	BlendedSettings.DOF_MaxFarBlurAmount = Lerp<FLOAT>(CurrentSettings.DOF_MaxFarBlurAmount,NewSettings.DOF_MaxFarBlurAmount,DOFFade);
	BlendedSettings.DOF_ModulateBlurColor = FColor(Lerp<FLinearColor>(CurrentSettings.DOF_ModulateBlurColor,NewSettings.DOF_ModulateBlurColor,DOFFade));
	BlendedSettings.DOF_FocusType = NewSettings.DOF_FocusType;
	BlendedSettings.DOF_FocusInnerRadius = Lerp<FLOAT>(CurrentSettings.DOF_FocusInnerRadius,NewSettings.DOF_FocusInnerRadius,DOFFade);
	BlendedSettings.DOF_FocusDistance = Lerp<FLOAT>(CurrentSettings.DOF_FocusDistance,NewSettings.DOF_FocusDistance,DOFFade);
	BlendedSettings.DOF_FocusPosition = Lerp<FVector>(CurrentSettings.DOF_FocusPosition,NewSettings.DOF_FocusPosition,DOFFade);

	// calc motion blur lerp amount
	FLOAT MotionBlurFade = 1.f;
	const FLOAT RemainingMotionBlurBlendTime = Max(NewSettings.MotionBlur_InterpolationDuration - ElapsedBlendTime,0.f);
	if(RemainingMotionBlurBlendTime > DeltaTime)
	{
		MotionBlurFade = Clamp<FLOAT>(DeltaTime / RemainingMotionBlurBlendTime,0.f,1.f);
	}
	// motion blur values
	BlendedSettings.MotionBlur_MaxVelocity = Lerp<FLOAT>(CurrentSettings.MotionBlur_MaxVelocity,NewSettings.MotionBlur_MaxVelocity,MotionBlurFade);
	BlendedSettings.MotionBlur_Amount = Lerp<FLOAT>(CurrentSettings.MotionBlur_Amount,NewSettings.MotionBlur_Amount,MotionBlurFade);
	BlendedSettings.MotionBlur_CameraRotationThreshold = Lerp<FLOAT>(CurrentSettings.MotionBlur_CameraRotationThreshold,NewSettings.MotionBlur_CameraRotationThreshold,MotionBlurFade);
	BlendedSettings.MotionBlur_CameraTranslationThreshold = Lerp<FLOAT>(CurrentSettings.MotionBlur_CameraTranslationThreshold,NewSettings.MotionBlur_CameraTranslationThreshold,MotionBlurFade);
	BlendedSettings.MotionBlur_FullMotionBlur = NewSettings.MotionBlur_FullMotionBlur;

	// calc scene material lerp amount
	FLOAT SceneMaterialFade = 1.f;
	const FLOAT RemainingSceneBlendTime = Max(NewSettings.Scene_InterpolationDuration - ElapsedBlendTime,0.f);
	if(RemainingSceneBlendTime > DeltaTime)
	{
		SceneMaterialFade = Clamp<FLOAT>(DeltaTime / RemainingSceneBlendTime,0.f,1.f);
	}
	// scene material values
	BlendedSettings.Scene_Desaturation	= Lerp<FLOAT>(CurrentSettings.Scene_Desaturation,NewSettings.Scene_Desaturation*PP_DesaturationMultiplier,SceneMaterialFade);
	BlendedSettings.Scene_HighLights	= Lerp<FVector>(CurrentSettings.Scene_HighLights,NewSettings.Scene_HighLights*PP_HighlightsMultiplier,SceneMaterialFade);
	BlendedSettings.Scene_MidTones		= Lerp<FVector>(CurrentSettings.Scene_MidTones,NewSettings.Scene_MidTones*PP_MidTonesMultiplier,SceneMaterialFade);
	BlendedSettings.Scene_Shadows		= Lerp<FVector>(CurrentSettings.Scene_Shadows,NewSettings.Scene_Shadows*PP_ShadowsMultiplier,SceneMaterialFade);

	// Clamp desaturation to 0..1 range to allow desaturation multipliers > 1 without color shifts at high desaturation.
	BlendedSettings.Scene_Desaturation = Clamp( BlendedSettings.Scene_Desaturation, 0.f, 1.f );

	// the scene material only needs to be enabled if the values don't match the default
	// as it should be setup to not have any affect in the default case
	if( BlendedSettings.bEnableSceneEffect )
	{
		if( BlendedSettings.Scene_Desaturation == 0.0f &&
			BlendedSettings.Scene_HighLights.Equals(FVector(1,1,1)) &&
			BlendedSettings.Scene_MidTones.Equals(FVector(1,1,1)) &&
			BlendedSettings.Scene_Shadows.Equals(FVector(0,0,0)) )
		{
			BlendedSettings.bEnableSceneEffect = FALSE;
		}
	}

	// Update the current settings and timer.
	CurrentPPInfo.LastSettings = BlendedSettings;
	CurrentPPInfo.LastBlendTime = CurrentWorldTime;
}

/**
 * Calculate the view settings for drawing from this view actor
 *
 * @param	View - output view struct
 * @param	ViewLocation - output actor location
 * @param	ViewRotation - output actor rotation
 * @param	Viewport - current client viewport
 */
FSceneView* ULocalPlayer::CalcSceneView( FSceneViewFamily* ViewFamily, FVector& ViewLocation, FRotator& ViewRotation, FViewport* Viewport )
{
	if( !Actor )
	{
		return NULL;
	}

	// do nothing if the viewport size is zero - this allows the viewport client the capability to temporarily remove a viewport without actually destroying and recreating it
	if (Size.X <= 0.f || Size.Y <= 0.f)
	{
		return NULL;
	}

	check(Viewport);

	// Compute the view's screen rectangle.
	INT X = appTrunc(Origin.X * Viewport->GetSizeX());
	INT Y = appTrunc(Origin.Y * Viewport->GetSizeY());
	UINT SizeX = appTrunc(Size.X * Viewport->GetSizeX());
	UINT SizeY = appTrunc(Size.Y * Viewport->GetSizeY());

	// Take screen percentage option into account if percentage != 100.
	GSystemSettings.ScaleScreenCoords(X,Y,SizeX,SizeY);
	
	// if the object propagtor is pushing us new values, use them instead of the player
	if (bOverrideView)
	{
		ViewLocation = OverrideLocation;
		ViewRotation = OverrideRotation;
	}
	else
	{
		Actor->eventGetPlayerViewPoint( ViewLocation, ViewRotation );
	}
	FLOAT fFOV = Actor->eventGetFOVAngle();
	// scale distances for cull distance purposes by the ratio of our current FOV to the default FOV
	Actor->LODDistanceFactor = fFOV / Max<FLOAT>(0.01f, (Actor->PlayerCamera != NULL) ? Actor->PlayerCamera->DefaultFOV : Actor->DefaultFOV);
	
	FMatrix ViewMatrix = FTranslationMatrix(-ViewLocation);
	ViewMatrix = ViewMatrix * FInverseRotationMatrix(ViewRotation);
	ViewMatrix = ViewMatrix * FMatrix(
		FPlane(0,	0,	1,	0),
		FPlane(1,	0,	0,	0),
		FPlane(0,	1,	0,	0),
		FPlane(0,	0,	0,	1));

	UGameUISceneClient* SceneClient = UUIRoot::GetSceneClient();

	FMatrix ProjectionMatrix;
	if( Actor && Actor->PlayerCamera != NULL && Actor->PlayerCamera->bConstrainAspectRatio && !SceneClient->IsUIActive() )
	{
		ProjectionMatrix = FPerspectiveMatrix(
			fFOV * (FLOAT)PI / 360.0f,
			Actor->PlayerCamera->ConstrainedAspectRatio,
			1.0f,
			NEAR_CLIPPING_PLANE
			);

		// Enforce a particular aspect ratio for the render of the scene. 
		// Results in black bars at top/bottom etc.
		Viewport->CalculateViewExtents( 
				Actor->PlayerCamera->ConstrainedAspectRatio, 
				X, Y, SizeX, SizeY );
	}
	else 
	{
		FLOAT CurViewAspectRatio = ((FLOAT)Viewport->GetSizeX()) / ((FLOAT)Viewport->GetSizeY());
		ProjectionMatrix = FPerspectiveMatrix(
			fFOV * (FLOAT)PI / 360.0f,
			SizeX * Viewport->GetDesiredAspectRatio() / CurViewAspectRatio,
			SizeY,
			NEAR_CLIPPING_PLANE
			);
	}

	FLinearColor OverlayColor(0,0,0,0);
	FLinearColor ColorScale(FLinearColor::White);

	if( Actor && Actor->PlayerCamera && !SceneClient->IsUIActive() )
	{
		// Apply screen fade effect to screen.
		if(Actor->PlayerCamera->bEnableFading)
		{
			OverlayColor = Actor->PlayerCamera->FadeColor.ReinterpretAsLinear();
			OverlayColor.A = Clamp(Actor->PlayerCamera->FadeAmount,0.0f,1.0f);
		}

		// Do color scaling if desired.
		if(Actor->PlayerCamera->bEnableColorScaling)
		{
			ColorScale = FLinearColor(
				Actor->PlayerCamera->ColorScale.X,
				Actor->PlayerCamera->ColorScale.Y,
				Actor->PlayerCamera->ColorScale.Z
				);
		}
	}

	// Update the player's post process settings.
	UpdatePostProcessSettings(ViewLocation);

	TArray<FPrimitiveSceneInfo*> HiddenPrimitives;

	// Translate the camera's hidden actors list to a hidden primitive list.
	Actor->UpdateHiddenActors(ViewLocation);
	const TArray<AActor*>& HiddenActors = Actor->PlayerCamera ? Actor->PlayerCamera->HiddenActors : Actor->HiddenActors;
	for(INT ActorIndex = 0;ActorIndex < HiddenActors.Num();ActorIndex++)
	{
		AActor* HiddenActor = HiddenActors(ActorIndex);
		if(HiddenActor)
		{
			for(INT ComponentIndex = 0;ComponentIndex < HiddenActor->AllComponents.Num();ComponentIndex++)
			{
				UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(HiddenActor->AllComponents(ComponentIndex));
				if(PrimitiveComponent && PrimitiveComponent->SceneInfo && !PrimitiveComponent->bIgnoreHiddenActorsMembership)
				{
					HiddenPrimitives.AddItem(PrimitiveComponent->SceneInfo);
				}
			}
		}
	}

	FSceneView* View = new FSceneView(
		ViewFamily,
		ViewState,
		&ActorVisibilityHistory,
		Actor->GetViewTarget(),
		PlayerPostProcess,
		&CurrentPPInfo.LastSettings,
		NULL,
		X,
		Y,
		SizeX,
		SizeY,
		ViewMatrix,
		ProjectionMatrix,
		FLinearColor::Black,
		OverlayColor,
		ColorScale,
		HiddenPrimitives,
		Actor->LODDistanceFactor
		);
	ViewFamily->Views.AddItem(View);

	return View;
}

IMPLEMENT_COMPARE_POINTER(USequenceOp, UnPlayer, { return(B->ActivateCount - A->ActivateCount); } );

//
//	ULocalPlayer::Exec
//

UBOOL ULocalPlayer::Exec(const TCHAR* Cmd,FOutputDevice& Ar)
{
	if(ParseCommand(&Cmd,TEXT("LISTTEXTURES")))
	{
		UBOOL bShouldOnlyListStreaming		= ParseCommand(&Cmd, TEXT("STREAMING"));
		UBOOL bShouldOnlyListNonStreaming	= ParseCommand(&Cmd, TEXT("NONSTREAMING"));
		debugf( TEXT("Listing %s textures."), bShouldOnlyListNonStreaming ? TEXT("non streaming") : bShouldOnlyListStreaming ? TEXT("streaming") : TEXT("all")  );

		// Traverse streamable list, creating a map of all streamable textures for fast lookup.
		TMap<UTexture2D*,UBOOL> StreamableTextureMap;
		for( TLinkedList<UTexture2D*>::TIterator It(UTexture2D::GetStreamableList()); It; It.Next() )
		{	
			UTexture2D* Texture	= *It;
			StreamableTextureMap.Set( Texture, TRUE );
		}
		
		FTextureLODSettings PlatformLODSettings;
		PlatformLODSettings.Initialize( GEngineIni, TEXT("TextureLODSettings") );

		// Collect textures.
		TArray<FSortedTexture> SortedTextures;
		for( TObjectIterator<UTexture2D> It; It; ++It )
		{
			UTexture2D*		Texture				= *It;
			INT				OrigSizeX			= Texture->SizeX;
			INT				OrigSizeY			= Texture->SizeY;
			INT				DroppedMips			= Texture->Mips.Num() - Texture->ResidentMips;
			INT				CurSizeX			= Texture->SizeX >> DroppedMips;
			INT				CurSizeY			= Texture->SizeY >> DroppedMips;
			UBOOL			bIsStreamingTexture = StreamableTextureMap.Find( Texture ) != NULL;
			INT				LODGroup			= Texture->LODGroup;
			INT				LODBias				= Texture->LODBias + PlatformLODSettings.GetTextureLODGroupLODBias( LODGroup );
			INT				NumMips				= Texture->Mips.Num();	
			INT				MaxMips				= Max( 1, Min( NumMips - Texture->GetCachedLODBias(), GMaxTextureMipCount ) );
			INT				MaxSize				= 0;
			INT				CurrentSize			= 0;

			for( INT MipIndex=NumMips-MaxMips; MipIndex<NumMips; MipIndex++ )
			{
				const FTexture2DMipMap& Mip = Texture->Mips(MipIndex);
				MaxSize += Mip.Data.GetBulkDataSize();
			}
			for( INT MipIndex=NumMips-Texture->ResidentMips; MipIndex<NumMips; MipIndex++ )
			{
				const FTexture2DMipMap& Mip = Texture->Mips(MipIndex);
				CurrentSize += Mip.Data.GetBulkDataSize();
			}

			if( (bShouldOnlyListStreaming && bIsStreamingTexture) 
			||	(bShouldOnlyListNonStreaming && !bIsStreamingTexture) 
			||	(!bShouldOnlyListStreaming && !bShouldOnlyListNonStreaming) )
			{
				new(SortedTextures) FSortedTexture( 
										OrigSizeX, 
										OrigSizeY, 
										CurSizeX,
										CurSizeY,
										LODBias, 
										MaxSize / 1024, 
										CurrentSize / 1024, 
										Texture->GetPathName(), 
										LODGroup, 
										bIsStreamingTexture );
			}
		}

		// Sort textures by cost.
		Sort<USE_COMPARE_CONSTREF(FSortedTexture,UnPlayer)>(SortedTextures.GetTypedData(),SortedTextures.Num());

		// Retrieve mapping from LOD group enum value to text representation.
		TArray<FString> TextureGroupNames = FTextureLODSettings::GetTextureGroupNames();

		// Display.
		INT TotalMaxSize		= 0;
		INT TotalCurrentSize	= 0;
		debugf( TEXT(",Orig Width,Orig Height,Cur Width,Cur Height,Max Size,Cur Size,LODBias,LODGroup,Name,Streaming") );
		for( INT TextureIndex=0; TextureIndex<SortedTextures.Num(); TextureIndex++ )
 		{
 			const FSortedTexture& SortedTexture = SortedTextures(TextureIndex);
			debugf( TEXT(",%i,%i,%i,%i,%i,%i,%i,%s,%s,%s"),
 				SortedTexture.OrigSizeX,
 				SortedTexture.OrigSizeY,
				SortedTexture.CurSizeX,
				SortedTexture.CurSizeY,
				SortedTexture.MaxSize,
 				SortedTexture.CurrentSize,
				SortedTexture.LODBias,
 				*TextureGroupNames(SortedTexture.LODGroup),
				*SortedTexture.Name,
				SortedTexture.bIsStreaming ? TEXT("YES") : TEXT("NO") );
			
			TotalMaxSize		+= SortedTexture.MaxSize;
			TotalCurrentSize	+= SortedTexture.CurrentSize;
 		}

		debugf(TEXT("Total size: Max= %d  Current= %d"), TotalMaxSize, TotalCurrentSize );
		return TRUE;
	}
	else if(ParseCommand(&Cmd,TEXT("LISTANIMSETS")))
	{
		TArray<FSortedAnimSet> SortedSets;
		for( TObjectIterator<UAnimSet> It; It; ++It )
		{
			UAnimSet* Set = *It;
			new(SortedSets) FSortedAnimSet(Set->GetPathName(), Set->GetResourceSize());
		}

		// Sort anim sets by cost
		Sort<USE_COMPARE_CONSTREF(FSortedAnimSet,UnPlayer)>(SortedSets.GetTypedData(),SortedSets.Num());

		// Now print them out.
		debugf(TEXT("Loaded AnimSets:"));
		INT TotalSize = 0;
		for(INT i=0; i<SortedSets.Num(); i++)
		{
			FSortedAnimSet& SetInfo = SortedSets(i);
			TotalSize += SetInfo.Size;
			debugf(TEXT("Size: %d\tName: %s"), SetInfo.Size, *SetInfo.Name);
		}
		debugf(TEXT("Total Size:%d"), TotalSize);
		return TRUE;
	}
	else if(ParseCommand(&Cmd,TEXT("ANIMSEQSTATS")))
	{
		extern void GatherAnimSequenceStats(FOutputDevice& Ar);
		GatherAnimSequenceStats( Ar );
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("SHOWHOTKISMET")) )
	{
		// First make array of all USequenceOps
		TArray<USequenceOp*> AllOps;
		for( TObjectIterator<USequenceOp> It; It; ++It )
		{
			USequenceOp* Op = *It;
			AllOps.AddItem(Op);
		}

		// Then sort them
		Sort<USE_COMPARE_POINTER(USequenceOp, UnPlayer)>(&AllOps(0), AllOps.Num());

		// Then print out the top 10
		INT TopNum = ::Min(10, AllOps.Num());
		Ar.Logf( TEXT("----- TOP 10 KISMET SEQUENCEOPS ------") );
		for(INT i=0; i<TopNum; i++)
		{
			Ar.Logf( TEXT("%6d: %s (%d)"), i, *(AllOps(i)->GetPathName()), AllOps(i)->ActivateCount );
		}
		return TRUE;
	}
	// Create a pending Note actor (only in PIE)
	else if( ParseCommand(&Cmd,TEXT("DN")) )
	{
		// Do nothing if not in editor
		if(GIsEditor && Actor && Actor->Pawn)
		{
			FString Comment = FString(Cmd);
			INT NewNoteIndex = GEngine->PendingDroppedNotes.AddZeroed();
			FDropNoteInfo& NewNote = GEngine->PendingDroppedNotes(NewNoteIndex);
			NewNote.Location = Actor->Pawn->Location;
			NewNote.Rotation = Actor->Rotation;
			NewNote.Comment = Comment;
			debugf(TEXT("Note Dropped: (%3.2f,%3.2f,%3.2f) - '%s'"), NewNote.Location.X, NewNote.Location.Y, NewNote.Location.Z, *NewNote.Comment);
		}
		return TRUE;
	}
    /** This will show all of the SkeletalMeshComponents that were ticked for one frame **/
	else if( ParseCommand(&Cmd,TEXT("SHOWSKELCOMPTICKTIME")) )
	{
		GShouldLogOutAFrameOfSkelCompTick = TRUE;
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("EXEC")) )
	{
		TCHAR Filename[512];
		if( ParseToken( Cmd, Filename, ARRAY_COUNT(Filename), 0 ) )
		{
			ExecMacro( Filename, Ar );
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLEDRAWEVENTS")) )
	{
		if( GEmitDrawEvents )
		{
			GEmitDrawEvents = FALSE;
			debugf(TEXT("Draw events are now DISABLED"));
		}
		else
		{
			GEmitDrawEvents = TRUE;
			debugf(TEXT("Draw events are now ENABLED"));
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLESTREAMINGVOLUMES")) )
	{
		if (ParseCommand(&Cmd, TEXT("ON")))
		{
			GWorld->DelayStreamingVolumeUpdates( 0 );
		}
		else if (ParseCommand(&Cmd, TEXT("OFF")))
		{
			GWorld->DelayStreamingVolumeUpdates( INDEX_NONE );
		}
		else
		{
			if( GWorld->StreamingVolumeUpdateDelay == INDEX_NONE )
			{
				GWorld->DelayStreamingVolumeUpdates( 0 );
			}
			else
			{
				GWorld->DelayStreamingVolumeUpdates( INDEX_NONE );
			}
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("PUSHVIEW")) )
	{
		if (ParseCommand(&Cmd, TEXT("START")))
		{
			bOverrideView = TRUE;
		}
		else if (ParseCommand(&Cmd, TEXT("STOP")))
		{
			bOverrideView = FALSE;
		}
		else if (ParseCommand(&Cmd, TEXT("SYNC")))
		{
			if (bOverrideView)
			{
				// @todo: with PIE, this maybe be the wrong PlayWorld!
				GWorld->FarMoveActor(Actor->Pawn ? (AActor*)Actor->Pawn : Actor, OverrideLocation, FALSE, TRUE, TRUE);
				Actor->SetRotation(OverrideRotation);
			}
		}
		else
		{
			OverrideLocation.X = appAtof(*ParseToken(Cmd, 0));
			OverrideLocation.Y = appAtof(*ParseToken(Cmd, 0));
			OverrideLocation.Z = appAtof(*ParseToken(Cmd, 0));

			OverrideRotation.Pitch = appAtoi(*ParseToken(Cmd, 0));
			OverrideRotation.Yaw   = appAtoi(*ParseToken(Cmd, 0));
			OverrideRotation.Roll  = appAtoi(*ParseToken(Cmd, 0));
		}
		return TRUE;
	}
	// @hack: This is a test matinee skipping function, quick and dirty to see if it's good enough for
	// gameplay. Will fix up better when we have some testing done!
	else if (ParseCommand(&Cmd, TEXT("CANCELMATINEE")))
	{
		UBOOL bMatineeSkipped = FALSE;

		// allow optional parameter for initial time in the matinee that this won't work (ie, 
		// 'cancelmatinee 5' won't do anything in the first 5 seconds of the matinee)
		FLOAT InitialNoSkipTime = appAtof(Cmd);

		// is the player in cinematic mode?
		if (Actor->bCinematicMode)
		{
			// if so, look for all active matinees that has this Player in a director group
			for (TObjectIterator<USeqAct_Interp> It; It; ++It)
			{
				// isit currently playing (and skippable)?
				if (It->bIsPlaying && It->bIsSkippable && (It->bClientSideOnly || GWorld->IsServer()))
				{
					for (INT GroupIndex = 0; GroupIndex < It->GroupInst.Num(); GroupIndex++)
					{
						// is the PC the group actor?
						if (It->GroupInst(GroupIndex)->GetGroupActor() == Actor)
						{
							const FLOAT RightBeforeEndTime = 0.1f;
							// make sure we aren';t already at the end (or before the allowed skip time)
							if ((It->Position < It->InterpData->InterpLength - RightBeforeEndTime) && 
								(It->Position >= InitialNoSkipTime))
							{
								// skip to end
								It->SetPosition(It->InterpData->InterpLength - RightBeforeEndTime, TRUE);

								// send a callback that this was skipped
								GCallbackEvent->Send(CALLBACK_MatineeCanceled, *It);

								bMatineeSkipped = TRUE;

								extern FLOAT HACK_DelayAfterSkip;
								// for 2 seconds after actually skipping a matinee, don't allow savegame loadng
								HACK_DelayAfterSkip = 2.0f;
							}
						}
					}
				}
			}

			if(bMatineeSkipped && GWorld && GWorld->GetGameInfo())
			{
				GWorld->GetGameInfo()->eventMatineeCancelled();
			}
		}
		return TRUE;
	}
	else if(ViewportClient && ViewportClient->Exec(Cmd,Ar))
	{
		return TRUE;
	}
	else if(Actor)
	{
		// Since UGameViewportClient calls Exec on UWorld, we only need to explicitly
		// call UWorld::Exec if we either have a null GEngine or a null ViewportClient
		UBOOL bWorldNeedsExec = GEngine == NULL || ViewportClient == NULL;
		if( bWorldNeedsExec && GWorld->Exec(Cmd,Ar) )
		{
			return TRUE;
		}
		else if( Actor->PlayerInput && Actor->PlayerInput->ScriptConsoleExec(Cmd,Ar,Actor->Pawn) )
		{
			return TRUE;
		}
		else if( Actor->ScriptConsoleExec(Cmd,Ar,Actor->Pawn) )
		{
			return TRUE;
		}
		else if( Actor->Pawn )
		{
			if( Actor->Pawn->ScriptConsoleExec(Cmd,Ar,Actor->Pawn) )
			{
				return TRUE;
			}
			else if( Actor->Pawn->InvManager && Actor->Pawn->InvManager->ScriptConsoleExec(Cmd,Ar,Actor->Pawn) )
			{
				return TRUE;
			}
			else if( Actor->Pawn->Weapon && Actor->Pawn->Weapon->ScriptConsoleExec(Cmd,Ar,Actor->Pawn) )
			{
				return TRUE;
			}
		}
		if( Actor->myHUD && Actor->myHUD->ScriptConsoleExec(Cmd,Ar,Actor->Pawn) )
		{
			return TRUE;
		}
		else if( GWorld->GetGameInfo() && GWorld->GetGameInfo()->ScriptConsoleExec(Cmd,Ar,Actor->Pawn) )
		{
			return TRUE;
		}
		else if( Actor->CheatManager && Actor->CheatManager->ScriptConsoleExec(Cmd,Ar,Actor->Pawn) )
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
}

void ULocalPlayer::ExecMacro( const TCHAR* Filename, FOutputDevice& Ar )
{
	// make sure Binaries is specified in the filename
	FString FixedFilename;
	if (!appStristr(Filename, TEXT("Binaries")))
	{
		FixedFilename = FString(TEXT("..\\Binaries\\")) + Filename;
		Filename = *FixedFilename;
	}

	FString Text;
	if (appLoadFileToString(Text, Filename))
	{
		debugf(TEXT("Execing %s"), Filename);
		const TCHAR* Data = *Text;
		FString Line;
		while( ParseLine(&Data, Line) )
		{
			Exec(*Line, Ar);
		}
	}
	else
	{
		Ar.Logf( NAME_ExecWarning, *LocalizeError("FileNotFound",TEXT("Core")), Filename );
	}
}

void FConsoleOutputDevice::Serialize(const TCHAR* Text,EName Event)
{
	FStringOutputDevice::Serialize(Text,Event);
	FStringOutputDevice::Serialize(TEXT("\n"),Event);
	GLog->Serialize(Text,Event);

	if( Console != NULL )
	{
		Console->eventOutputText(Text);
	}
}
