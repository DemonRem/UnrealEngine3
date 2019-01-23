/*=============================================================================
	UnUIObjects.cpp: UI widget class implementations.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "CanvasScene.h"

#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"

#include "EngineSequenceClasses.h"
#include "EngineUISequenceClasses.h"

#include "ScopedObjectStateChange.h"

#include "UnUIKeys.h"

// Registration
IMPLEMENT_CLASS(UUIScreenObject);
	IMPLEMENT_CLASS(UUIObject);
		IMPLEMENT_CLASS(UUIButton);
			IMPLEMENT_CLASS(UUICheckbox);
			IMPLEMENT_CLASS(UUIToggleButton);
				IMPLEMENT_CLASS(UUITabButton);
			IMPLEMENT_CLASS(UUILabelButton);
			IMPLEMENT_CLASS(UUINumericEditBoxButton);
			IMPLEMENT_CLASS(UUIScrollbarButton);
				IMPLEMENT_CLASS(UUIScrollbarMarkerButton);
			IMPLEMENT_CLASS(UUIOptionListButton);
		IMPLEMENT_CLASS(UUIImage);
		IMPLEMENT_CLASS(UUILabel);
			IMPLEMENT_CLASS(UUIToolTip);
		IMPLEMENT_CLASS(UUIEditBox);
		IMPLEMENT_CLASS(UConsoleEntry);
		IMPLEMENT_CLASS(UUIScrollbar);
		IMPLEMENT_CLASS(UUIProgressBar);
		IMPLEMENT_CLASS(UUISlider);
		IMPLEMENT_CLASS(UUINumericEditBox);
		IMPLEMENT_CLASS(UUIComboBox);
		IMPLEMENT_CLASS(UUIMeshWidget);
		IMPLEMENT_CLASS(UUIOptionListBase);
			IMPLEMENT_CLASS(UUIOptionList);
			IMPLEMENT_CLASS(UUINumericOptionList);
	IMPLEMENT_CLASS(UUIScene);

IMPLEMENT_CLASS(UUIStringRenderer);
IMPLEMENT_CLASS(UUIStyleResolver);
IMPLEMENT_CLASS(UCustomPropertyItemHandler);

DECLARE_CYCLE_STAT(TEXT("UpdateScene Time"),STAT_UISceneUpdateTime,STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("PreRenderCallback Time"),STAT_UIPreRenderCallbackTime,STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("RefreshFormatting Time"),STAT_UIRefreshFormatting,STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("RebuildDockingStack Time"),STAT_UIRebuildDockingStack,STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("Resolve Positions Time"),STAT_UIResolveScenePositions,STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("RebuildNavigationLinks Time"),STAT_UIRebuildNavigationLinks,STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("RefreshStyles Time"),STAT_UIRefreshWidgetStyles,STATGROUP_UI);
//DECLARE_CYCLE_STAT(TEXT("AddDockingNode Time (General)"),STAT_UIAddDockingNode,STATGROUP_UI);
//DECLARE_CYCLE_STAT(TEXT("AddDockingNode Time (String)"),STAT_UIAddDockingNode_String,STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("SetWidgetPosition Time"),STAT_UISetWidgetPosition,STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("GetWidgetPosition Time"),STAT_UIGetWidgetPosition,STATGROUP_UI);
//DECLARE_CYCLE_STAT(TEXT(""),STAT,STATGROUP_UI);

INT FScopedDebugLogger::DebugIndent=0;

/* ==========================================================================================================
	Statics
========================================================================================================== */
void MigrateImageSettings( UUITexture*& Texture, const FTextureCoordinates& Coordinates, const FUIStyleReference& Style, UUIComp_DrawImage* DestinationComponent, const TCHAR* PropertyName )
{
	checkSlow(PropertyName);
	if ( Texture != NULL )
	{
		checkf(DestinationComponent, TEXT("%s still has value for deprecated %s property, but no component to migrate settings to"), *Texture->GetOuter()->GetFullName(), PropertyName);

		// migrate the old settings over to the image component
		DestinationComponent->ImageRef = Texture;
		if ( !Coordinates.IsZero() )
		{
			DestinationComponent->StyleCustomization.SetCustomCoordinates(Coordinates);
		}

		// migrate the style data over to the image component if the panel is using an image style
		if ( Style.RequiredStyleClass == NULL || Style.RequiredStyleClass->IsChildOf(UUIStyle_Image::StaticClass()) )
		{
			DestinationComponent->ImageStyle = Style;
		}

		// now clear the image reference.
		Texture = NULL;
	}
}

/**
 * Given a face, return the opposite face.
 *
 * @return	the EUIWidgetFace member corresponding to the opposite face of the input value, or UIFACE_MAX if the input
 *			value is invalid.
 */
EUIWidgetFace UUIRoot::GetOppositeFace( BYTE FaceIndex )
{
	EUIWidgetFace Result = UIFACE_MAX;
	if ( FaceIndex < UIFACE_MAX )
	{
		Result = (EUIWidgetFace) ((FaceIndex + 2) % UIFACE_MAX);
	}

	return Result;
}

/**
 * Returns the friendly name of for the specified face from the EUIWidgetFace enum.
 *
 * @return	the textual representation of the enum member specified, or "Unknown" if the value is invalid.
 */
FString UUIRoot::GetDockFaceText( BYTE Face )
{
	static UEnum* WidgetFaceEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EUIWidgetFace"), TRUE);
	if ( WidgetFaceEnum != NULL && Face <= UIFACE_MAX )
	{
		return WidgetFaceEnum->GetEnum(Face).ToString();
	}

	return TEXT("Unknown");
}

/**
 * Returns the friendly name for the specified input event from the EInputEvent enum.
 *
 * @return	the textual representation of the enum member specified, or "Unknown" if the value is invalid.
 */
FString UUIRoot::GetInputEventText( BYTE InputEvent )
{
	static UEnum* InputEventEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EInputEvent"), TRUE);
	if ( InputEventEnum != NULL && InputEvent <= IE_MAX )
	{
		return InputEventEnum->GetEnum(InputEvent).ToString();
	}

	return TEXT("Unknown");
}

/**
 * Returns the friendly name for the specified cell state from the UIListElementState enum.
 *
 * @return	the textual representation of the enum member specified, or "Unknown" if the value is invalid.
 */
FString UUIRoot::GetCellStateText( BYTE CellState )
{
	static UEnum* CellStateEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EUIListElementState"), TRUE);
	if ( CellStateEnum != NULL && CellState <= ELEMENT_MAX )
	{
		return CellStateEnum->GetEnum(CellState).ToString();
	}

	return TEXT("Unknown");
}

/**
 * Returns the friendly name for the specified field type from the UIDataProviderFieldType enum.
 *
 * @return	the textual representation of the enum member specified, or "Unknown" if the value is invalid.
 */
FString UUIRoot::GetDataProviderFieldTypeText( BYTE FieldType )
{
	static UEnum* ProviderFieldTypeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EUIDataProviderFieldType"), TRUE);
	if ( ProviderFieldTypeEnum != NULL && FieldType <= DATATYPE_MAX )
	{
		return ProviderFieldTypeEnum->GetEnum(FieldType).ToString();
	}

	return TEXT("Unknown");
}

/**
 * Wrapper for returns the orientation associated with the specified face.
 */
EUIOrientation UUIRoot::GetFaceOrientation( BYTE Face )
{
	checkSlow(Face<UIFACE_MAX);
	EUIOrientation Result = (EUIOrientation)(Face % UIORIENT_MAX);
	return Result;
}

/**
 * Returns the UIController class set for this game.
 *
 * @return	a pointer to a UIInteraction class which is set as the value for GameViewportClient.UIControllerClass.
 */
UClass* UUIRoot::GetUIControllerClass()
{
	UClass* GameViewportClass = GEngine->GameViewportClientClass;
	check(GameViewportClass);

	// first, find the GameViewportClient class configured for this game
	UGameViewportClient* DefaultGameViewport = GameViewportClass->GetDefaultObject<UGameViewportClient>();
	if ( DefaultGameViewport == NULL )
	{
		// if the configured GameViewportClient class couldn't be loaded, fallback to the base class
		DefaultGameViewport = UGameViewportClient::StaticClass()->GetDefaultObject<UGameViewportClient>();
	}
	check(DefaultGameViewport);

	// now get the UIInteraction class from the configured GameViewportClient
	return DefaultGameViewport->UIControllerClass;
}

/**
 * Returns the default object for the UIController class set for this game.
 *
 * @return	a pointer to the CDO for UIInteraction class configured for this game.
 */
UUIInteraction* UUIRoot::GetDefaultUIController()
{
	UClass* UIControllerClass = GetUIControllerClass();
	check(UIControllerClass);

	UUIInteraction* DefaultUIController = UIControllerClass->GetDefaultObject<UUIInteraction>();
	if ( DefaultUIController == NULL )
	{
		DefaultUIController = UUIInteraction::StaticClass()->GetDefaultObject<UUIInteraction>();
	}
	check(DefaultUIController);

	return DefaultUIController;
}
/**
 * Returns the UIInteraction instance currently controlling the UI system, which is valid in game.
 *
 * @return	a pointer to the UIInteraction object currently controlling the UI system.
 */
UUIInteraction* UUIRoot::GetCurrentUIController()
{
	UUIInteraction* UIController = NULL;
	if ( GEngine != NULL && GEngine->GameViewport != NULL )
	{
		UIController = GEngine->GameViewport->UIController;
	}

	return UIController;
}

/**
 * Returns the game's scene client.
 *
 * @return 	a pointer to the UGameUISceneClient instance currently managing the scenes for the UI System.
 */
UGameUISceneClient* UUIRoot::GetSceneClient()
{
	UGameUISceneClient* SceneClient = NULL;

	UUIInteraction* CurrentUIController = GetCurrentUIController();
	if ( CurrentUIController != NULL )
	{
		SceneClient = CurrentUIController->SceneClient;
	}

	return SceneClient;
}

/**
 * Returns the current position of the mouse or joystick cursor.
 *
 * @param	CursorPosition	receives the position of the cursor
 * @param	Scene			if specified, provides access to an FViewport through the scene's SceneClient that can be used
 *							for retrieving the mouse position when not in the game.
 *
 * @return	TRUE if the cursor position was retrieved correctly.
 */
UBOOL UUIRoot::GetCursorPosition( FVector2D& CursorPosition, const UUIScene* Scene/*=NULL*/ )
{
	UBOOL bResult = FALSE;

	UGameUISceneClient* GameSceneClient = GetSceneClient();
	if ( GameSceneClient != NULL )
	{
		CursorPosition.X = GameSceneClient->MousePosition.X;
		CursorPosition.Y = GameSceneClient->MousePosition.Y;
		bResult = TRUE;
	}
	else if ( Scene != NULL && Scene->SceneClient != NULL && Scene->SceneClient->RenderViewport != NULL )
	{
		CursorPosition.X = Scene->SceneClient->RenderViewport->GetMouseX();
		CursorPosition.Y = Scene->SceneClient->RenderViewport->GetMouseY();
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Returns the current position of the mouse or joystick cursor.
 *
 * @param	CursorX		receives the X position of the cursor
 * @param	CursorY		receives the Y position of the cursor
 * @param	Scene		if specified, provides access to an FViewport through the scene's SceneClient that can be used
 *						for retrieving the mouse position when not in the game.
 *
 * @return	TRUE if the cursor position was retrieved correctly.
 */
UBOOL UUIRoot::GetCursorPosition( INT& CursorX, INT& CursorY, const UUIScene* Scene/*=NULL*/ )
{
	UBOOL bResult = FALSE;

	UGameUISceneClient* GameSceneClient = GetSceneClient();
	if ( GameSceneClient != NULL )
	{
		CursorX = GameSceneClient->MousePosition.X;
		CursorY = GameSceneClient->MousePosition.Y;
		bResult = TRUE;
	}
	else if ( Scene != NULL && Scene->SceneClient != NULL && Scene->SceneClient->RenderViewport != NULL )
	{
		CursorX = Scene->SceneClient->RenderViewport->GetMouseX();
		CursorY = Scene->SceneClient->RenderViewport->GetMouseY();
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Returns the current position of the mouse or joystick cursor.
 *
 * @param	CursorXL	receives the width of the cursor
 * @param	CursorYL	receives the height of the cursor
 *
 * @return	TRUE if the cursor size was retrieved correctly.
 */
UBOOL UUIRoot::GetCursorSize( FLOAT& CursorXL, FLOAT& CursorYL )
{
	UBOOL bResult = FALSE;

	UGameUISceneClient* GameSceneClient = GetSceneClient();
	if ( GameSceneClient != NULL )
	{
		bResult = GameSceneClient->GetCursorSize(CursorXL, CursorYL);
	}

	return bResult;
}

/**
 * Changes the value of GameViewportClient.bUIMouseCaptureOverride to the specified value.  Used by widgets that process
 * dragging to ensure that the widget receives the mouse button release event.
 *
 * @param	bCaptureMouse	whether to capture all mouse input.
 */
void UUIRoot::SetMouseCaptureOverride( UBOOL bCaptureMouse )
{
	// no longer need all mouse input
	UUIInteraction* UIController = GetCurrentUIController();
	if ( UIController != NULL )
	{
		UIController->GetOuterUGameViewportClient()->SetMouseCaptureOverride(bCaptureMouse);
	}
}

/**
 * @return	TRUE if the specified key is a mouse key
 */
UBOOL UUIRoot::IsCursorInputKey( FName KeyName )
{
	return	KeyName == KEY_LeftMouseButton
		||	KeyName == KEY_MiddleMouseButton
		||	KeyName == KEY_RightMouseButton;
}

/**
 * Returns a matrix which includes the translation, rotation and scale necessary to transform a point from origin to the
 * the specified widget's position onscreen.  This matrix can then be passed to ConditionalUpdateTransform() for primitives
 * in use by the UI.
 *
 * @param	Widget	the widget to generate the matrix for
 * @param	bIncludeAnchorPosition	specify TRUE to include translation to the widget's anchor; if FALSE, the translation will move
 *									the point to the widget's upper left corner (in local space)
 * @param	bIncludeRotation		specify FALSE to remove the widget's rotation from the resulting matrix
 * @param	bIncludeScale			specify FALSE to remove the viewport's scale from the resulting matrix
 *
 * @return	a matrix which can be used to translate from origin (0,0) to the widget's position, including rotation and viewport scale.
 *
 * @todo ronp - implement bIncludeRotation and bIncludeScale
 */
FMatrix UUIRoot::GetPrimitiveTransform( UUIObject* Widget, UBOOL bIncludeAnchorPosition/*=FALSE*/, UBOOL bIncludeRotation/*=TRUE*/, UBOOL bIncludeScale/*=TRUE*/ )
{
	FMatrix Result = FMatrix::Identity;

	if ( Widget != NULL )
	{
		Result = FTranslationMatrix(Widget->RenderOffset) * FTranslationMatrix(FVector(0,0,Widget->ZDepth)) * Widget->GetRotationMatrix(FALSE);

		FVector AnchorPos = Widget->GetAnchorPosition();
		if ( bIncludeAnchorPosition )
		{
			Result *= FTranslationMatrix(AnchorPos);
		}

		FMatrix ViewportScaleOffsetMatrix(FMatrix::Identity);
		if( Widget->GetOwner() == NULL )
		{
			FLOAT ViewportScale = Widget->GetViewportScale();
			FVector ViewportOffset(0);
			Widget->GetViewportOffset((FVector2D&)ViewportOffset);
			ViewportScaleOffsetMatrix = FScaleMatrix(ViewportScale) * FTranslationMatrix(ViewportOffset);
		}

		FVector WidgetPos = Widget->GetPositionVector(FALSE);
		Result *= FTranslationMatrix(WidgetPos) * GetPrimitiveTransform(Widget->GetOwner(), bIncludeAnchorPosition) * ViewportScaleOffsetMatrix;
	}

	return Result;
}

// exec wrappers
void UUIRoot::execGetCurrentUIController( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	*(UUIInteraction**)Result=GetCurrentUIController();
}
void UUIRoot::execGetSceneClient( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	*(UGameUISceneClient**)Result=GetSceneClient();
}
void UUIRoot::execGetPrimitiveTransform( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UUIObject,Widget);
	P_GET_UBOOL_OPTX(bIncludeAnchorPosition,FALSE);
	P_GET_UBOOL_OPTX(bIncludeRotation,TRUE);
	P_GET_UBOOL_OPTX(bIncludeScale,TRUE);
	P_FINISH;
	*(FMatrix*)Result=GetPrimitiveTransform(Widget,bIncludeAnchorPosition,bIncludeRotation,bIncludeScale);
}
void UUIRoot::execGetFaceOrientation( FFrame& Stack, RESULT_DECL )
{
	P_GET_BYTE(Face);
	P_FINISH;
	*(BYTE*)Result=GetFaceOrientation(Face);
}
void UUIRoot::execGetCursorPosition( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT_REF(CursorX);
	P_GET_INT_REF(CursorY);
	P_GET_OBJECT_OPTX_REF(UUIScene,Scene,NULL);
	P_FINISH;
	*(UBOOL*)Result=GetCursorPosition(CursorX,CursorY,Scene);
}
void UUIRoot::execGetCursorSize( FFrame& Stack, RESULT_DECL )
{
	P_GET_FLOAT_REF(CursorXL);
	P_GET_FLOAT_REF(CursorYL);
	P_FINISH;
	*(UBOOL*)Result=GetCursorSize(CursorXL,CursorYL);
}
void UUIRoot::execSetMouseCaptureOverride( FFrame& Stack, RESULT_DECL )
{
	P_GET_UBOOL(bCaptureMouse);
	P_FINISH;
	SetMouseCaptureOverride(bCaptureMouse);
}

/* ==========================================================================================================
	UUIScreenObject
========================================================================================================== */

/* == Unrealscript Stubs == */
void UUIScreenObject::execIsInitialized( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	*(UBOOL*)Result=IsInitialized();
}
void UUIScreenObject::execRequestSceneUpdate( FFrame& Stack, RESULT_DECL )
{
	P_GET_UBOOL(bDockingStackChanged);
	P_GET_UBOOL(bPositionsChanged);
	P_GET_UBOOL_OPTX(bNavLinksOutdated,FALSE);
	P_GET_UBOOL_OPTX(bWidgetStylesChanged,FALSE);
	P_FINISH;
	RequestSceneUpdate(bDockingStackChanged,bPositionsChanged,bNavLinksOutdated);
}
void UUIScreenObject::execRequestFormattingUpdate( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	RequestFormattingUpdate();
}
void UUIScreenObject::execRequestPrimitiveReview( FFrame& Stack, RESULT_DECL )
{
	P_GET_UBOOL(bReinitializePrimitives);
	P_GET_UBOOL(bReviewPrimitiveUsage);
	P_FINISH;
	RequestPrimitiveReview(bReinitializePrimitives,bReviewPrimitiveUsage);
}
void UUIScreenObject::execGetWidgetPathName( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	*(FString*)Result=GetWidgetPathName();
}
void UUIScreenObject::execGetActivePlayerCount( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	*(INT*)Result=UUIInteraction::GetPlayerCount();
}

/**
 * Returns the maximum number of players that could potentially generate input for this scene.  If the owning scene's input mode
 * is INPUTMODE_Free, will correspond to the maximum number of simultaneous gamepads supported by this platform; otherwise, the
 * number of active players.
 */
INT UUIScreenObject::GetSupportedPlayerCount()
{
	INT Result = UUIInteraction::GetPlayerCount();

	// if this scene is configured to allow input from unbound gamepads, the max supported players is the number of gamepads that are valid for this platform
	if ( GIsGame && GetScene() && GetScene()->delegateGetSceneInputMode() == INPUTMODE_Free )
	{
		Result = UCONST_MAX_SUPPORTED_GAMEPADS;
	}

	return Result;
}

/**
 * Called when this widget is created.
 */
void UUIScreenObject::Created( UUIScreenObject* Creator )
{
	if ( Creator != NULL
	&&	(Creator->HasAnyFlags(RF_ArchetypeObject)
		||	Creator->IsInUIPrefab()
		||	Creator->IsA(UUIPrefab::StaticClass())))
	{
		Modify();
		SetFlags(RF_Transactional|Creator->GetMaskedFlags(RF_ArchetypeObject|RF_Public));
	}

	// notify the EventProvider for this widget that it has been created, so that it can instantiate any default events
	if ( EventProvider != NULL )
	{
		EventProvider->Created();
	}

	// get a list of all widgets contained within this widget - these will become the children of this widget.
	TArray<UUIScreenObject*> InnerObjects;
	TArchiveObjectReferenceCollector<UUIScreenObject> Collector(&InnerObjects, this, TRUE);
	Serialize(Collector);

	for ( INT i = 0; i < InnerObjects.Num(); i++ )
	{
		UUIScreenObject* InnerObject = InnerObjects(i);
		InnerObject->Created(this);
	}
}

/**
 * Called when the currently active skin has been changed.  Reapplies this widget's style and propagates
 * the notification to all children.
 */
void UUIScreenObject::NotifyActiveSkinChanged()
{
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		UUIObject* Child = Children(ChildIndex);
		Child->NotifyActiveSkinChanged();
	}

	// if the delegate is set, call it now
	if ( DELEGATE_IS_SET(NotifyActiveSkinChanged) )
	{
		delegateNotifyActiveSkinChanged();
	}
}
/**
 * Called immediately after a child has been added to this screen object.
 *
 * @param	WidgetOwner		the screen object that the NewChild was added as a child for
 * @param	NewChild		the widget that was added
 */
void UUIScreenObject::NotifyAddedChild( UUIScreenObject* WidgetOwner, UUIObject* NewChild )
{
	eventAddedChild(WidgetOwner, NewChild);
}

/**
 * Called immediately after a child has been removed from this screen object.
 *
 * @param	WidgetOwner		the screen object that the widget was removed from.
 * @param	OldChild		the widget that was removed
 * @param	ExclusionSet	used to indicate that multiple widgets are being removed in one batch; useful for preventing references
 *							between the widgets being removed from being severed.
 */
void UUIScreenObject::NotifyRemovedChild( UUIScreenObject* WidgetOwner, UUIObject* OldChild, TArray<UUIObject*>* ExclusionSet/*=NULL*/ )
{
	if ( ExclusionSet != NULL )
	{
		eventRemovedChild(WidgetOwner, OldChild, *ExclusionSet);
	}
	else
	{
		eventRemovedChild(WidgetOwner, OldChild);
	}
}

void UUIScreenObject::Initialize( UUIScene* InOwnerScene, UUIObject* InOwner/*=NULL*/ )
{
	if ( !HasAnyFlags(RF_Transient) )
	{
		SetFlags(RF_Transactional);
	}

	if ( EventProvider != NULL )
	{
		// restore references that have been corrupted by incorrect handling of component instancing in UObject::ConditionalPostLoad
		if ( EventProvider->GetOuter() != this )
		{
			UUIComp_Event* ExistingEventProvider = NULL;
			// attempt to locate an event provider that has this widget as its outer
			for ( TObjectIterator<UUIComp_Event> It; It; ++It )
			{
				if ( It->GetOuter() == this )
				{
					ExistingEventProvider = *It;
					break;
				}
			}

			if ( ExistingEventProvider == NULL )
			{
				// didn't find one - create a new one
				UUIComp_Event* EventProviderTemplate = GetArchetype<UUIScreenObject>()->EventProvider;
				if ( EventProviderTemplate != NULL )
				{
					ExistingEventProvider = ConstructObject<UUIComp_Event>( EventProviderTemplate->GetClass(), this, NAME_None,
						GetMaskedFlags(RF_PropagateToSubObjects), EventProviderTemplate, this);

					ExistingEventProvider->TemplateName = EventProviderTemplate->TemplateName;
				}
			}

			debugf(NAME_Warning, TEXT("Correcting EventProvider for: \r\n\t%s:\r\n\t%s\r\n\t%s"), *GetPathName(), *EventProvider->GetPathName(), *ExistingEventProvider->GetPathName());
			EventProvider = ExistingEventProvider;
			MarkPackageDirty();
		}

		checkSlow(EventProvider);
		check(EventProvider->GetOuter()==this);
		EventProvider->InitializeEventProvider();
	}

	// notify unrealscript
	eventInitialized();

	if ( InOwnerScene == NULL )
	{
		InOwnerScene = Cast<UUIScene>(this);
	}
	check(InOwnerScene);
	if ( InOwnerScene->SceneClient != NULL )
	{
		// ideally, the activation of the Initialized event would go in the UIScreenObject's Initialized event (unrealscript)
		// but since this event will be called for every widget in the scene, we'll do it from C++ for speed

		// load the initialized event class
		static UClass* InitializeEventClass = LoadClass<UUIEvent>(NULL, TEXT("Engine.UIEvent_Initialized"), NULL, LOAD_None, NULL);

		// fire off the "Initialized" event, which should [at the very least] activate a "ActivateInitialState" action (UUIAction_ActivateInitialState)
		// which should change the widget's state to whatever state is configured as its InitialState, which as a result initializes this widget's Style
		checkSlow(InitializeEventClass);

		INT CurrentlyActivePlayerCount = Max(1, GetSupportedPlayerCount());
		for ( INT PlayerIndex = 0; PlayerIndex < CurrentlyActivePlayerCount; PlayerIndex++ )
		{
			// activate the initial state for this widget which initializes this widget's style
			ActivateInitialState(PlayerIndex);

			// now fire the Initialized event
			ActivateEventByClass(PlayerIndex,InitializeEventClass,this,TRUE);
		}
	}
}

/**
 * Sets up the focus, input, and any other arrays which contain data that tracked uniquely for each active player.
 * Ensures that the arrays responsible for managing focus chains are synched up with the Engine.GamePlayers array.
 */
void UUIScreenObject::InitializePlayerTracking()
{
	const INT CurrentlyActivePlayerCount = GetSupportedPlayerCount();

	// first, synchronize the FocusControls array
	if ( FocusControls.Num() < CurrentlyActivePlayerCount )
	{
		FocusControls.InsertZeroed(FocusControls.Num(), CurrentlyActivePlayerCount - FocusControls.Num());
	}
	else if ( FocusControls.Num() > CurrentlyActivePlayerCount )
	{
		FocusControls.Remove(CurrentlyActivePlayerCount, FocusControls.Num() - CurrentlyActivePlayerCount);
	}
	
	// next, sync up the FocusPropagation array
	if ( FocusPropagation.Num() < CurrentlyActivePlayerCount )
	{
		FocusPropagation.InsertZeroed(FocusPropagation.Num(), CurrentlyActivePlayerCount - FocusPropagation.Num());
	}
	else if ( FocusPropagation.Num() > CurrentlyActivePlayerCount )
	{
		FocusPropagation.Remove(CurrentlyActivePlayerCount, FocusPropagation.Num() - CurrentlyActivePlayerCount);
	}
}

/**
 * Called when a new player has been added to the list of active players (i.e. split-screen join) after the scene
 * has been activated.
 *
 * @param	PlayerIndex		the index [into the GamePlayers array] where the player was inserted
 * @param	AddedPlayer		the player that was added
 */
void UUIScreenObject::CreatePlayerData( INT PlayerIndex, ULocalPlayer* AddedPlayer )
{
	// add new elements to the focus management arrays for the new player
	FocusControls.InsertZeroed(PlayerIndex);
	FocusPropagation.InsertZeroed(PlayerIndex);

	//@todo - might need to perform some sort of fixup for any existing PlayerInputMasks

	// propagate the notification to all child widgets
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		Children(ChildIndex)->CreatePlayerData(PlayerIndex, AddedPlayer);
	}
}

/**
 * Called when a player has been removed from the list of active players (i.e. split-screen players)
 *
 * @param	PlayerIndex		the index [into the GamePlayers array] where the player was located
 * @param	RemovedPlayer	the player that was removed
 */
void UUIScreenObject::RemovePlayerData( INT PlayerIndex, ULocalPlayer* RemovedPlayer )
{
	checkSlow(PlayerIndex>=0);
	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	// when a player is removed, scene ownership is migrated to the next player in the list
	// if the next player in line didn't have any focused controls (for any reason), migrate this player's focused
	// controls over to that player
	INT NextPlayerIndex = PlayerIndex+1;
	if ( NextPlayerIndex < FocusControls.Num() )
	{
		UUIObject* FocusedControl = GetFocusedControl(FALSE, PlayerIndex);
		UUIObject* OtherFocusedControl = GetFocusedControl(FALSE, NextPlayerIndex);

		if ( FocusedControl != NULL && OtherFocusedControl == NULL )
		{
			FocusControls(NextPlayerIndex) = FocusControls(PlayerIndex);
		}
	}

	//@todo - need to do some more cleanup here, such as removing this player's index from all PlayerInputMasks
	FocusControls.Remove(PlayerIndex);
	FocusPropagation.Remove(PlayerIndex);

	// propagate the notification to all child widgets
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		Children(ChildIndex)->RemovePlayerData(PlayerIndex, RemovedPlayer);
	}
}

/**
 * Retrieves a reference to a LocalPlayer.
 *
 * @param	PlayerIndex		if specified, returns the player at this index in the GamePlayers array.  Otherwise, returns
 *							the player associated with the owner scene.
 *
 * @return	the player that owns this scene or is located in the specified index of the GamePlayers array.
 */
ULocalPlayer* UUIScreenObject::GetPlayerOwner( INT PlayerIndex /*=INDEX_NONE*/ )
{
	ULocalPlayer* Result = NULL;

	if ( PlayerIndex == INDEX_NONE )
	{
		// get the player associated with the owning scene
		UUIScene* OwnerScene = GetScene();
		if ( OwnerScene != NULL )
		{
			Result = OwnerScene->PlayerOwner;
			if ( Result == NULL && GEngine != NULL && GEngine->GamePlayers.Num() > 0 )
			{
				// if no player is associated with this scene, return the primary player
				Result = GEngine->GamePlayers(0);
			}
		}
	}
	else if ( GEngine != NULL && GEngine->GamePlayers.IsValidIndex(PlayerIndex) )
	{
		Result = GEngine->GamePlayers(PlayerIndex);
	}

	return Result;
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUIScreenObject::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{
	out_KeyNames.AddItem(UIKEY_Consume);

	out_KeyNames.AddItem(UIKEY_NextControl);
	out_KeyNames.AddItem(UIKEY_PrevControl);

	out_KeyNames.AddItem(UIKEY_NavFocusUp);
	out_KeyNames.AddItem(UIKEY_NavFocusDown);
	out_KeyNames.AddItem(UIKEY_NavFocusLeft);
	out_KeyNames.AddItem(UIKEY_NavFocusRight);

	eventGetSupportedUIActionKeyNames( out_KeyNames );
}

/**
 * Iterates through the DefaultStates array checking that InactiveStates contains at least one instance of each
 * DefaultState.  If no instances are found, one is created and added to the InactiveStates array.
 */
void UUIScreenObject::CreateDefaultStates()
{
	// make sure that we have an InitialState and that we have a UIState_Enabled (or child) in the list of
	// states available to this widget (every widget must be able to enter the enabled state)
	ValidateRequiredStates();

	// for each element of the DefaultStates array,
	for ( INT DefaultIndex = 0; DefaultIndex < DefaultStates.Num(); DefaultIndex++ )
	{
		UClass* DefaultStateClass = DefaultStates(DefaultIndex);
		if ( !ContainsObjectOfClass(InactiveStates, DefaultStateClass) )
		{
			AddSupportedState(DefaultStateClass->GetDefaultObject<UUIState>());
		}
	}
}

/**
 * Checks that this screen object has an InitialState and contains a UIState_Enabled (or child class) in its 
 * InactiveStates array.  If any of the required states are missing, creates them.
 * This function must be called prior to executing CreateDefaultStates().
 */
void UUIScreenObject::ValidateRequiredStates()
{
	UClass* EnabledStateClass = UUIState_Enabled::StaticClass();

	// first, make sure that we have a valid InitialState
	if ( InitialState == NULL )
	{
		InitialState = EnabledStateClass;
	}

	UBOOL bHasInitialState = FALSE;
	UBOOL bHasEnabledState = FALSE;

	// first, check the default states for the required states, since they'll be instanced 
	for ( INT DefaultStateIndex = 0; DefaultStateIndex < DefaultStates.Num() && !(bHasEnabledState && bHasInitialState); DefaultStateIndex++ )
	{
		UClass* DefaultStateClass = DefaultStates(DefaultStateIndex);
		if ( !bHasInitialState && DefaultStateClass->IsChildOf(InitialState) )
		{
			bHasInitialState = TRUE;
		}
		if ( !bHasEnabledState && DefaultStateClass->IsChildOf(EnabledStateClass) )
		{
			bHasEnabledState = TRUE;
		}
	}

	for ( INT StateIndex = 0; StateIndex < InactiveStates.Num() && !(bHasEnabledState && bHasInitialState); StateIndex++ )
	{
		UUIState* InactiveState = InactiveStates(StateIndex);
		if ( InactiveState )
		{
			if ( !bHasInitialState && InactiveState->IsA(InitialState) )
			{
				bHasInitialState = TRUE;
			}

			if ( !bHasEnabledState && InactiveState->IsA(EnabledStateClass) )
			{
				bHasEnabledState = TRUE;
			}
		}
	}

	if ( !bHasInitialState )
	{
		DefaultStates.AddUniqueItem(InitialState);
	}

	if ( !bHasEnabledState )
	{
		DefaultStates.AddUniqueItem(EnabledStateClass);
	}
}

/**
 * Returns only those states [from the InactiveStates array] which were instanced from an entry in the DefaultStates array.
 */
void UUIScreenObject::GetInstancedStates( TMap<UClass*,UUIState*>& out_Instances )
{
	for ( INT StateIndex = 0; StateIndex < InactiveStates.Num(); StateIndex++ )
	{
		UUIState* StateInstance = InactiveStates(StateIndex);
		if ( StateInstance && StateInstance->GetArchetype() == StateInstance->GetClass()->GetDefaultObject() )
		{
			INT DefaultStateIndex = DefaultStates.FindItemIndex(StateInstance->GetClass());
			if (  DefaultStateIndex!= INDEX_NONE )
			{
				out_Instances.Set(DefaultStates(DefaultStateIndex), StateInstance);
			}
		}
	}
}

/**
 * Creates a new UIState instance based on the specified template and adds the new state to this widget's list of
 * InactiveStates.
 *
 * @param	StateTemplate	the state to use as the template for the new state
 */
void UUIScreenObject::AddSupportedState( UUIState* StateTemplate )
{
	if ( StateTemplate != NULL )
	{
		UUIState* StateInstance = ConstructObject<UUIState>(StateTemplate->GetClass(), this, NAME_None, RF_Transactional|GetMaskedFlags(RF_Transient|RF_Public|RF_ArchetypeObject), StateTemplate);
		if ( StateInstance != NULL )
		{
			Modify(TRUE);
			InactiveStates.AddItem(StateInstance);
		}
	}
}

/**
 * Utility function for encapsulating constructing a widget
 *
 * @param	Owner			the container for the widget.  Cannot be NULL.
 * @param	WidgetClass		the class of the widget to create.  Cannot be NULL.
 * @param	WidgetArchetype	the template to use for creating the widget
 * @param	WidgetName		the name to use for the new widget
 */
UUIObject* UUIScreenObject::CreateWidget( UUIScreenObject* Owner, UClass* WidgetClass, UObject* WidgetArchetype/*=NULL*/, FName WidgetName/*=NAME_None*/ )
{
	check(WidgetClass);
	check(Owner);

	EObjectFlags WidgetFlags = Owner->GetMaskedFlags(RF_Public|RF_ArchetypeObject);
	if ( !Owner->HasAnyFlags(RF_ArchetypeObject) && Owner->IsA(UUIScene::StaticClass()) )
	{
		// if this is a non-archetype UIScene, remove the public flag before creating child widgets.
		WidgetFlags &= ~RF_Public;
	}

	if ( WidgetArchetype == NULL )
	{
		WidgetArchetype = WidgetClass->GetDefaultObject<UUIObject>();
	}

#if DO_GUARD_SLOW
	if ( Owner->IsA(UUIPrefab::StaticClass()) || Owner->IsInUIPrefab() )
	{
		check((WidgetFlags&(RF_Public|RF_ArchetypeObject)) != 0);
	}
#endif

	FObjectDuplicationParameters DupParams(WidgetArchetype, Owner);
	DupParams.DestName = WidgetName;
	DupParams.bMigrateArchetypes = TRUE;
	DupParams.DestClass = WidgetClass;
	DupParams.FlagMask = WidgetFlags;
	DupParams.ApplyFlags = RF_Transactional;

	// the DefaultEvents array will have been duplicated, but we don't want this, so restore it now (can't do anything to avoid duplicating it, unfortunately)
	UUIScreenObject* WidgetArc = CastChecked<UUIScreenObject>(WidgetArchetype);
	if ( WidgetArc->EventProvider != NULL )
	{
		for ( INT EventIndex = 0; EventIndex < WidgetArc->EventProvider->DefaultEvents.Num(); EventIndex++ )
		{
			FDefaultEventSpecification& DefaultEvent = WidgetArc->EventProvider->DefaultEvents(EventIndex);
			if ( DefaultEvent.EventTemplate != NULL )
			{
				DupParams.DuplicationSeed.Set(DefaultEvent.EventTemplate, DefaultEvent.EventTemplate);
			}
		}
	}

	UUIObject* Result = CastChecked<UUIObject>(StaticDuplicateObjectEx(DupParams));

	Result->Modify();
	Result->Created(Owner);

	return Result;
}

/**
 * Retrieves the virtual viewport offset for the viewport which renders this widget's scene.  Only relevant in the UI editor;
 * non-zero if the user has panned or zoomed the viewport.
 *
 * @param	out_ViewportOffset	[out] will be filled in with the delta between the viewport's actual origin and virtual origin.
 *
 * @return	TRUE if the viewport origin was successfully retrieved
 */
UBOOL UUIScreenObject::GetViewportOffset( FVector2D& out_ViewportOffset ) const
{
	UBOOL bResult = FALSE;

	const UUIScene* OwnerScene = GetScene();
	if ( OwnerScene != NULL && OwnerScene->SceneClient != NULL )
	{
		checkf(OwnerScene->SceneClient->IsSceneInitialized(OwnerScene),TEXT("GetViewportOrigin called but owner scene not fully initialized: %s (%s)"), *GetPathName(), *GetWidgetPathName());
		bResult = OwnerScene->SceneClient->GetViewportOffset(OwnerScene,out_ViewportOffset);
	}

	return bResult;
}

/**
 * Retrieves the scale factor for the viewport which renders this widget's scene.  Only relevant in the UI editor.
 */
FLOAT UUIScreenObject::GetViewportScale() const
{
	FLOAT Result=1.f;
	const UUIScene* OwnerScene = GetScene();
	if ( OwnerScene != NULL && OwnerScene->SceneClient != NULL )
	{
		checkf(OwnerScene->SceneClient->IsSceneInitialized(OwnerScene),TEXT("GetViewportScale called but owner scene not fully initialized: %s (%s)"), *GetPathName(), *GetWidgetPathName());
		Result = OwnerScene->SceneClient->GetViewportScale(OwnerScene);
	}
	return Result;
}

/**
 * Retrieves the virtual origin of the viewport that this widget is rendered within.  See additional comments in UISceneClient
 *
 * In the game, this will be non-zero if Scene is for split-screen and isn't for the first player.
 * In the editor, this will be equal to the value of the gutter region around the viewport.
 *
 * @param	out_ViewportOrigin	[out] will be filled in with the origin point for the viewport that
 *								owns this screen object
 *
 * @return	TRUE if the viewport origin was successfully retrieved
 */
UBOOL UUIScreenObject::GetViewportOrigin( FVector2D& out_ViewportOrigin ) const
{
	UBOOL bResult = FALSE;

	const UUIScene* OwnerScene = GetScene();
	if ( OwnerScene != NULL && OwnerScene->SceneClient != NULL )
	{
		checkf(OwnerScene->SceneClient->IsSceneInitialized(OwnerScene),TEXT("GetViewportOrigin called but owner scene not fully initialized: %s (%s)"), *GetPathName(), *GetWidgetPathName());
		bResult = OwnerScene->SceneClient->GetViewportOrigin(OwnerScene,out_ViewportOrigin);
	}

	return bResult;
}

/**
 * Retrieves the viewport size, accounting for split-screen.
 *
 * @param	out_ViewportSize	[out] will be filled in with the width & height of the viewport that owns this screen object
 *
 * @return	TRUE if the viewport size was successfully retrieved
 */
UBOOL UUIScreenObject::GetViewportSize( FVector2D& out_ViewportSize ) const
{
	UBOOL bResult = FALSE;

	const UUIScene* OwnerScene = GetScene();
	if ( OwnerScene != NULL && OwnerScene->SceneClient != NULL )
	{
		checkf(OwnerScene->SceneClient->IsSceneInitialized(OwnerScene),TEXT("GetViewportSize called but owner scene not fully initialized: %s (%s)"), *GetPathName(), *GetWidgetPathName());
		out_ViewportSize = OwnerScene->CurrentViewportSize;
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Insert a widget at the specified location
 *
 * @param	NewChild		the widget to insert
 * @param	InsertIndex		the position to insert the widget.  If not specified, the widget is insert at the end of
 *							the list
 * @param	bRenameExisting	controls what happens if there is another widget in this widget's Children list with the same tag as NewChild.
 *							if TRUE, renames the existing widget giving a unique transient name.
 *							if FALSE, does not add NewChild to the list and returns FALSE.
 *
 * @return	the position that that the child was inserted in, or INDEX_NONE if the widget was not inserted
 */
INT UUIScreenObject::InsertChild( UUIObject* NewChild, INT InsertIndex/*=INDEX_NONE*/, UBOOL bRenameExisting/*=TRUE*/ )
{
	if ( NewChild == NULL )
	{
		return INDEX_NONE;
	}

	if ( InsertIndex == INDEX_NONE )
	{
		InsertIndex = Children.Num();
	}

	// can't add NewChild to our Children array if it already has a parent
	if ( NewChild->GetParent() != NULL )
	{
		return INDEX_NONE;
	}

	INT Result = INDEX_NONE;

	UUIObject* ExistingChild = FindChild(NewChild->WidgetTag,FALSE);
	if ( ExistingChild == NULL || (ExistingChild != NewChild && bRenameExisting) )
	{
		UBOOL bNeedsRename = ExistingChild != NULL;

		Result = Children.InsertItem(NewChild, InsertIndex);
		if ( Result != INDEX_NONE )
		{
			if ( ExistingChild != NULL && bRenameExisting == TRUE )
			{
				ExistingChild->Modify();

				// rename the existing child giving it a unique name
				// remove RF_Public before renaming to ensure that we don't create a redirect
				const EObjectFlags OriginalFlags = ExistingChild->GetFlags();
				ExistingChild->ClearFlags(RF_Public);

				ExistingChild->Rename(NULL, this, REN_ForceNoResetLoaders);

				ExistingChild->SetFlags(OriginalFlags);
			}

			// if the new widget doesn't have this widget as its Outer, update that now
			if ( NewChild->GetOuter() != this && NewChild->RequiresParentForOuter() )
			{
				NewChild->Modify();

				// in this case, we do want a redirect created if the widget was public
				NewChild->Rename( *NewChild->GetName(), this, REN_ForceNoResetLoaders );
			}

			if ( IsInitialized() && NewChild->GetScene() != GetScene() )
			{
				NewChild->InitializePlayerTracking();
				NewChild->Initialize(GetScene(), Cast<UUIObject>(this));
				NewChild->eventPostInitialize();
			}

			// notify unrealscript and our parent chain of this addition
			NotifyAddedChild(this, NewChild);

			if ( IsInitialized() )
			{
				if ( !HasAnyFlags(RF_ArchetypeObject) )
				{
					// if this child implements the UIDataStoreSubscriber interface, tell it to load the value value from its data store
					IUIDataStoreSubscriber* DataStoreWidget = (IUIDataStoreSubscriber*)NewChild->GetInterfaceAddress(IUIDataStoreSubscriber::UClassType::StaticClass());
					if ( DataStoreWidget != NULL )
					{
						DataStoreWidget->RefreshSubscriberValue();
					}
				}

				RequestSceneUpdate(TRUE,FALSE,TRUE);
				if ( NewChild->bSupports3DPrimitives )
				{
					UUIScene* OwnerScene = GetScene();
					if ( OwnerScene != NULL )
					{
						OwnerScene->bUsesPrimitives = TRUE;
						OwnerScene->bUpdatePrimitiveUsage = FALSE;
						OwnerScene->RequestPrimitiveReview(TRUE,FALSE);
					}
				}
			}
		}
	}
	return Result;
}

/**
 * Remove an existing child widget from this widget's children
 *
 * @param	ExistingChild	the widget to remove
 * @param	ExclusionSet	used to indicate that multiple widgets are being removed in one batch; useful for preventing references
 *							between the widgets being removed from being severed.
 *
 * @return	TRUE if the child was successfully removed from the list, or if the child was not contained by this widget
 *			FALSE if the child could not be removed from this widget's child list.
 */
UBOOL UUIScreenObject::RemoveChild( UUIObject* ExistingChild, TArray<UUIObject*>* ExclusionSet/*=NULL*/ )
{
	UBOOL bResult = FALSE;
	INT ChildIndex = Children.FindItemIndex(ExistingChild);
	if ( ChildIndex != INDEX_NONE )
	{
		// let the widget know that it's being unparented.
		ExistingChild->NotifyRemovedFromParent(this, ExclusionSet);

		Children.Remove(ChildIndex);
		bResult = TRUE;

		ExistingChild->OwnerScene = NULL;
		ExistingChild->Owner = NULL;

		NotifyRemovedChild(this,ExistingChild,ExclusionSet);
		RequestSceneUpdate(TRUE,FALSE,TRUE);

		if ( ExistingChild->bSupports3DPrimitives )
		{
			RequestPrimitiveReview(TRUE,TRUE);
		}
	}

	return bResult;
}
void UUIScreenObject::execRemoveChild( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UUIObject,ExistingChild);
	P_GET_TARRAY_OPTX(UUIObject*,ExclusionSet,TArray<UUIObject*>());
	const UBOOL bValueNotSpecified=(GRuntimeUCFlags&RUC_SkippedOptionalParm) != 0;
	P_FINISH;

	if ( bValueNotSpecified )
	{
		*(UBOOL*)Result=RemoveChild(ExistingChild);
	}
	else
	{
		*(UBOOL*)Result=RemoveChild(ExistingChild,&ExclusionSet);
	}
}

/**
 * Removes a group of children from this widget's Children array.  All removal notifications are delayed until all children
 * have been removed; useful for removing a group of child widgets without destroying the references between them.
 *
 * @param	ChildrenToRemove	the list of child widgets to remove
 *
 * @return	a list of children that could not be removed; if the return array is emtpy, all children were successfully removed.
 */
TArray<UUIObject*> UUIScreenObject::RemoveChildren( const TArray<UUIObject*>& cChildrenToRemove )
{
	TArray<UUIObject*> ChildrenToRemove = cChildrenToRemove;
	TArray<UUIObject*> RemainingChildren;
	TArray<UUIObject*> RemovedChildren;
	TArray<UUIObject*> AllRemovedChildren;	// includes children of those we're removing - used as the exclusion set

	RemovedChildren.Empty(ChildrenToRemove.Num());
	AllRemovedChildren.Empty(ChildrenToRemove.Num());

	for ( INT RemovalIndex = ChildrenToRemove.Num() - 1; RemovalIndex >= 0; RemovalIndex-- )
	{
		UUIObject* ExistingChild = ChildrenToRemove(RemovalIndex);
		if ( Children.ContainsItem(ExistingChild) )
		{
			RemovedChildren.AddItem(ExistingChild);
			AllRemovedChildren.AddItem(ExistingChild);
			ExistingChild->GetChildren(AllRemovedChildren, TRUE);
		}
		else
		{
			ChildrenToRemove.Remove(RemovalIndex);
			RemainingChildren.AddItem(ExistingChild);
		}
	}

	for ( INT RemovalIndex = 0; RemovalIndex < ChildrenToRemove.Num(); RemovalIndex++ )
	{
		UUIObject* ExistingChild = ChildrenToRemove(RemovalIndex);

		INT ChildIndex = Children.FindItemIndex(ExistingChild);

		// since we just removed all elements of the ChildrenToRemove that weren't in our Children array, this should never be INDEX_NONE
		checkSlow(ChildIndex!=INDEX_NONE);

		// let the widget know that it's being unparented.
		ExistingChild->NotifyRemovedFromParent(this, &AllRemovedChildren);
		Children.Remove(ChildIndex);
	}

	for ( INT ChildIndex = 0; ChildIndex < RemovedChildren.Num(); ChildIndex++ )
	{
		UUIObject* ExistingChild = RemovedChildren(ChildIndex);

		// clear a couple of properties which would normally remain active
		ExistingChild->OwnerScene = NULL;
		ExistingChild->Owner = NULL;

		NotifyRemovedChild(this,ExistingChild,&AllRemovedChildren);
	}

	if ( RemovedChildren.Num() > 0 )
	{
		RequestFormattingUpdate();
		RequestSceneUpdate(TRUE,TRUE,TRUE,TRUE);
	}
	return RemainingChildren;
}

/**
 * Replace an existing child widget with the specified widget.
 *
 * @param	ExistingChild	the widget to remove
 * @param	NewChild		the widget to replace ExistingChild with
 *
 * @return	TRUE if the ExistingChild was successfully replaced with the specified NewChild; FALSE otherwise.
 */
UBOOL UUIScreenObject::ReplaceChild( UUIObject* ExistingChild, UUIObject* NewChild )
{
	UBOOL bResult = FALSE;

	if ( ExistingChild != NULL && NewChild != NULL && NewChild != ExistingChild )
	{
		INT ChildIndex = Children.FindItemIndex(ExistingChild);
		Modify();
		ExistingChild->Modify();

		if ( ChildIndex != INDEX_NONE && RemoveChild(ExistingChild) )
		{
			NewChild->Modify();
			if ( InsertChild(NewChild, ChildIndex) != INDEX_NONE )
			{
				// success
				bResult = TRUE;
			}
			else
			{
				// failed to insert the new child, so re-add the old child back to the list
				InsertChild(ExistingChild, ChildIndex);
			}
		}
	}

	return bResult;
}

/**
 * Find a child widget with the specified name
 *
 * @param	WidgetName	the name of the child to find
 * @param	bRecurse	if TRUE, searches all children of this object recursively
 *
 * @return	a pointer to a widget contained by this object that has the specified name, or
 *			NULL if no widgets with that name were found
 */
UUIObject* UUIScreenObject::FindChild( FName WidgetName, UBOOL bRecurse/*=FALSE*/ ) const
{
	UUIObject* Result = NULL;
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		if ( Children(ChildIndex)->WidgetTag == WidgetName )
		{
			Result = Children(ChildIndex);
			break;
		}
	}

	if ( Result == NULL && bRecurse )
	{
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			Result = Children(ChildIndex)->FindChild(WidgetName,TRUE);
			if ( Result != NULL )
			{
				break;
			}
		}
	}

	return Result;
}

/**
 * Find a child widget with the specified GUID
 *
 * @param	WidgetID	the ID(GUID) of the child to find
 * @param	bRecurse	if TRUE, searches all children of this object recursively
 *
 * @return	a pointer to a widget contained by this object that has the specified GUID, or
 *			NULL if no widgets with that name were found
 */
UUIObject* UUIScreenObject::FindChildUsingID( FWIDGET_ID WidgetID, UBOOL bRecurse/*=FALSE*/ ) const
{
	UUIObject* Result = NULL;
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		if ( Children(ChildIndex)->WidgetID == WidgetID )
		{
			Result = Children(ChildIndex);
			break;
		}
	}

	if ( Result == NULL && bRecurse )
	{
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			Result = Children(ChildIndex)->FindChildUsingID(WidgetID,TRUE);
			if ( Result != NULL )
			{
				break;
			}
		}
	}

	return Result;
}

/**
 * Find the index for the child widget with the specified name
 *
 * @param	WidgetName	the name of the child to find
 *
 * @return	the index into the array of children for the widget that has the specified name, or
 *			INDEX_NONE if there aren't any widgets with that name.
 */
INT UUIScreenObject::FindChildIndex( FName WidgetName ) const
{
	INT Result = INDEX_NONE;
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		if ( Children(ChildIndex)->WidgetTag == WidgetName )
		{
			Result = ChildIndex;
			break;
		}
	}

	return Result;
}

/**
 * Returns whether this screen object contains the specified child in its list of children.
 *
 * @param	Child	the child to look for
 * @param	bRecurse	whether to search child widgets for the specified child.  if this value is FALSE,
 *						only the Children array of this screen object will be searched for Child.
 *
 * @return	TRUE if Child is contained by this screen object
 */
UBOOL UUIScreenObject::ContainsChild( UUIObject* Child, UBOOL bRecurse/*=TRUE*/ ) const
{
	UBOOL bResult = FALSE;
	if ( Child != NULL )
	{
		if ( Children.ContainsItem(Child) )
		{
			if ( Child->GetParent() != this )
			{
				if ( Child->GetParent() != NULL || Child->IsInitialized() )
				{
					debugf(NAME_Error, TEXT("Hierarchy mismatch: '%s' exists in Children array of '%s' but its parent is '%s'!"),
						*Child->GetFullName(), *GetFullName(), Child->GetParent() ? *Child->GetParent()->GetFullName() : TEXT("NULL"));
				}
			}

			bResult = TRUE;
		}

		if ( !bResult && bRecurse )
		{
			for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
			{
				if ( Children(ChildIndex)->ContainsChild(Child,TRUE) )
				{
					bResult = TRUE;
					break;
				}
			}
		}
	}

	return bResult;
}

/**
 * Returns whether this screen object contains a child of the specified class.
 *
 * @param	SearchClass	the class to search for.
 * @param	bRecurse	indicates whether to search child widgets.  if this value is FALSE,
 *						only the Children array of this screen object will be searched for instances of SearchClass.
 *
 * @return	TRUE if Child is contained by this screen object
 */
UBOOL UUIScreenObject::ContainsChildOfClass( UClass* SearchClass, UBOOL bRecurse/*=TRUE*/ ) const
{
	UBOOL bResult = FALSE;

	if ( SearchClass != NULL )
	{
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			if ( Children(ChildIndex)->IsA(SearchClass) )
			{
				bResult = TRUE;
				break;
			}
		}

		if ( !bResult && bRecurse )
		{
			for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
			{
				if ( Children(ChildIndex)->ContainsChildOfClass(SearchClass, TRUE) )
				{
					bResult = TRUE;
					break;
				}
			}
		}
	}
	else
	{
		warnf(TEXT("'None' specified for SearchClass in call to UIScreenObject.ContainsChildOfClass"));
	}

	return bResult;
}

/**
 * Gets a list of all children contained in this screen object.
 *
 * @param	out_Children	receives the list of child widgets.
 * @param	bRecurse		if FALSE, result will only contain widgets from this screen object's Children array
 *							if TRUE, result will contain all children of this screen object, including their children.
 * @param	ExclusionSet	if specified, any widgets contained in this array will not be added to the output array.
 *
 * @return	an array of widgets contained by this screen object.
 */
void UUIScreenObject::GetChildren( TArray<UUIObject*>& out_Children, UBOOL bRecurse/*=FALSE*/, TArray<UUIObject*>* ExclusionSet/*=NULL*/ ) const
{
	if ( ExclusionSet == NULL )
	{
		out_Children += Children;
		if ( bRecurse )
		{
			for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
			{
				Children(ChildIndex)->GetChildren(out_Children, TRUE);
			}
		}
	}
	else
	{
		TArray<UUIObject*> ValidChildren = Children;
		for ( INT ChildIndex = ValidChildren.Num() - 1; ChildIndex >= 0; ChildIndex-- )
		{
			if ( ExclusionSet->ContainsItem(ValidChildren(ChildIndex)) )
			{
				ValidChildren.Remove(ChildIndex);
			}
		}

		out_Children += ValidChildren;
		if ( bRecurse )
		{
			for ( INT ChildIndex = 0; ChildIndex < ValidChildren.Num(); ChildIndex++ )
			{
				ValidChildren(ChildIndex)->GetChildren(out_Children, TRUE, ExclusionSet);
			}
		}
	}
}

/**
 * Gets a list of all children contained in this screen object.
 *
 * @param	bRecurse		if FALSE, result will only contain widgets from this screen object's Children array
 *							if TRUE, result will contain all children of this screen object, including their children.
 * @param	ExclusionSet	if specified, any widgets contained in this array will not be added to the output array.
 *
 * @return	an array of widgets contained by this screen object.
 */
TArray<UUIObject*> UUIScreenObject::GetChildren( UBOOL bRecurse/*=FALSE*/, TArray<UUIObject*>* ExclusionSet/*=NULL*/ ) const
{
	TArray<UUIObject*> Result;
	GetChildren(Result, bRecurse, ExclusionSet);
	return Result;
}
void UUIScreenObject::execGetChildren( FFrame& Stack, RESULT_DECL )
{
	P_GET_UBOOL_OPTX(bRecurse,FALSE);
	P_GET_TARRAY_OPTX(UUIObject*,ExclusionSet,TArray<UUIObject*>());
	
	const UBOOL bExclusionSetSpecified=(GRuntimeUCFlags&RUC_SkippedOptionalParm) == 0;
	P_FINISH;
	
	if ( bExclusionSetSpecified )
	{
		*(TArray<UUIObject*>*)Result = GetChildren(bRecurse, &ExclusionSet);
	}
	else
	{
		*(TArray<UUIObject*>*)Result = GetChildren(bRecurse);
	}
}

/**
 * Returns the number of UIObjects owned by this UIScreenObject, recursively
 *
 * @return	the number of widgets (including this one) contained by this widget, including all
 *			child widgets
 *
 */
INT UUIScreenObject::GetObjectCount() const
{
	// this widget counts for 1
	INT Count = 1;

	for ( INT i = 0; i < Children.Num(); i++ )
	{
		Count += Children(i)->GetObjectCount();
	}

	return Count;
}


/**
 * Returns the default parent to use when placing widgets using the UI editor.  This widget is used when placing
 * widgets by dragging their outline using the mouse, for example.
 *
 * @return	a pointer to the widget that will contain newly placed widgets when a specific parent widget has not been
 *			selected by the user.
 */
UUIScreenObject* UUIScreenObject::GetEditorDefaultParentWidget()
{
	return GetScene();
}

/**
 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
 *
 * By default this function recursively calls itself on all of its children.
 */
void UUIScreenObject::PreRenderCallback()
{
	for ( INT ChildIdx = 0; ChildIdx < Children.Num(); ChildIdx++ )
	{
		Children(ChildIdx)->PreRenderCallback();
	}
}

/**
 * Generates a list of any children of this widget which are of a class that has been deprecated, recursively.
 */
void UUIScreenObject::FindDeprecatedWidgets( TArray<UUIScreenObject*>& out_DeprecatedWidgets )
{
	if ( GetClass()->HasAnyClassFlags(CLASS_Deprecated) )
	{
		out_DeprecatedWidgets.AddUniqueItem(this);
	}

	// recurse into our children
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		Children(ChildIndex)->FindDeprecatedWidgets(out_DeprecatedWidgets);
	}
}

/**
 * Called when a property value has been changed in the editor.  For some reason I need this here or calling Super::PostEditChange() from a child class of 
 * UUIScreenObject always seems to bind to the FEditPropertyChain version, resulting in a compiler error.
 */
void UUIScreenObject::PostEditChange( UProperty* PropertyThatChanged )
{
	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIScreenObject::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* OutermostProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( OutermostProperty != NULL )
		{
			FName PropertyName = OutermostProperty->GetFName();
			if ( PropertyName == TEXT("Position") )
			{
				RefreshPosition();
			}
		}
	}
	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after importing property values for this object (paste, duplicate or .t3d import)
 * Allow the object to perform any cleanup for properties which shouldn't be duplicated or
 * are unsupported by the script serialization
 */
void UUIScreenObject::PostEditImport()
{
	Super::PostEditImport();
}

/**
 * Called after this object has been de-serialized from disk.  This version removes any NULL entries from the Children array.
 */
void UUIScreenObject::PostLoad()
{
	Super::PostLoad();

	// remove any NULL entries.
	Children.RemoveItem(NULL);
}

/**
 * Called just after a property in this object's archetype is modified, immediately after this object has been de-serialized
 * from the archetype propagation archive.
 *
 * Allows objects to perform reinitialization specific to being de-serialized from an FArchetypePropagationArc and
 * reinitialized against an archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
 *
 * This version requests a full scene update.
 */
void UUIScreenObject::PostSerializeFromPropagationArchive()
{
	RequestSceneUpdate(TRUE, TRUE, TRUE, TRUE);
	RequestFormattingUpdate();

	Super::PostSerializeFromPropagationArchive();
}

/**
 * Builds a list of objects which have this object in their archetype chain.
 *
 * All archetype propagation for UIScreenObjects is handled by the UIPrefab/UIPrefabInstance code, so this version just
 * skips the iteration unless we're a CDO.
 *
 * @param	Instances	receives the list of objects which have this one in their archetype chain
 */
void UUIScreenObject::GetArchetypeInstances( TArray<UObject*>& Instances )
{
	if ( HasAnyFlags(RF_ClassDefaultObject) )
	{
		Super::GetArchetypeInstances(Instances);
	}
}

/**
 * Serializes all objects which have this object as their archetype into GMemoryArchive, then recursively calls this function
 * on each of those objects until the full list has been processed.
 * Called when a property value is about to be modified in an archetype object.
 *
 * Since archetype propagation for UIScreenObjects is handled by the UIPrefab code, this version simply routes the call 
 * to the owning UIPrefab so that it can handle the propagation at the appropriate time.
 *
 * @param	AffectedObjects		ignored
 */
void UUIScreenObject::SaveInstancesIntoPropagationArchive( TArray<UObject*>& AffectedObjects )
{
	UUIPrefab* OwnerPrefab = NULL;
	if ( IsInUIPrefab(&OwnerPrefab) )
	{
		checkSlow(OwnerPrefab);
		OwnerPrefab->SaveInstancesIntoPropagationArchive(AffectedObjects);
	}

	// otherwise, we just swallow this call - UIScreenObjects should never execute the UObject version
}

/**
 * De-serializes all objects which have this object as their archetype from the GMemoryArchive, then recursively calls this function
 * on each of those objects until the full list has been processed.
 *
 * Since archetype propagation for UIScreenObjects is handled by the UIPrefab code, this version simply routes the call 
 * to the owning UIPrefab so that it can handle the propagation at the appropriate time.
 *
 * @param	AffectedObjects		the array of objects which have this object in their ObjectArchetype chain and will be affected by the change.
 *								Objects which have this object as their direct ObjectArchetype are removed from the list once they're processed.
 */
void UUIScreenObject::LoadInstancesFromPropagationArchive( TArray<UObject*>& AffectedObjects )
{
	UUIPrefab* OwnerPrefab = NULL;
	if ( IsInUIPrefab(&OwnerPrefab) )
	{
		checkSlow(OwnerPrefab);
		OwnerPrefab->LoadInstancesFromPropagationArchive(AffectedObjects);
	}
	else
	{
		checkfSlow((GetParent() != NULL || GetScene() == this || !HasAnyFlags(RF_ArchetypeObject)),
			TEXT("LoadInstancesFromPropagationArchive called on widget archetype %s (%s) after it was removed from its owning UIPrefab!"), *GetFullName(), *GetTag().ToString());


	}

	// otherwise, we just swallow this call - UIScreenObjects should never execute the UObject version
}

/**
 * Determines whether this object is contained within a UIPrefab.
 *
 * @param	OwnerPrefab		if specified, receives a pointer to the owning prefab.
 *
 * @return	TRUE if this object is contained within a UIPrefab; FALSE if this object IS a UIPrefab or is not
 *			contained within a UIPrefab.
 */
UBOOL UUIScreenObject::IsAPrefabArchetype( UObject** OwnerPrefab/*=NULL*/ ) const
{
	return IsInUIPrefab((UUIPrefab**)OwnerPrefab);
}

/**
 * @return	TRUE if the object is a UIPrefabInstance or a child of a UIPrefabInstance.
 */
UBOOL UUIScreenObject::IsInPrefabInstance() const
{
	// see if this object's archetype is contained within a UUIPrefab object
	return GetArchetype<UUIScreenObject>()->IsInUIPrefab();
}

/**
 * Determines whether this UIScreenObject is contained by a UIPrefab.
 */
UBOOL UUIScreenObject::IsInUIPrefab( UUIPrefab** OwningPrefab/*=NULL*/ ) const
{
	UBOOL bResult = FALSE;

	UObject* OwnerWidget = GetOuter();
	if ( IsPendingKill() && OwnerWidget == UObject::GetTransientPackage() )
	{
		// in WxUIEditorBase::DeleteSelectedWidgets, we restore the widget's Owner after it's deleted so that we can find the owning UIPrefab
		OwnerWidget = GetParent();
	}

	for ( UObject* CheckOuter = OwnerWidget; CheckOuter; CheckOuter = CheckOuter->GetOuter() )
	{
		if ( CheckOuter->IsA(UUIPrefab::StaticClass()) )
		{
			if ( OwningPrefab != NULL )
			{
				*OwningPrefab = static_cast<UUIPrefab*>(CheckOuter);
			}
			bResult = TRUE;
			break;
		}
	}
	return bResult;
}

/**
 * Called when a property is modified that could potentially affect the widget's position onscreen.
 */
void UUIScreenObject::RefreshPosition()
{
	// if we haven't resolved any faces, it means we are pending a scene position update so we don't need to request another
	if ( GetNumResolvedFaces() != 0 )
	{
		RequestSceneUpdate(FALSE,TRUE);
	}

	if ( DELEGATE_IS_SET(NotifyPositionChanged) )
	{
		delegateNotifyPositionChanged( this );
	}
}

/**
 * Called when the scene receives a notification that the viewport has been resized.  Propagated down to all children.
 *
 * @param	OldViewportSize		the previous size of the viewport
 * @param	NewViewportSize		the new size of the viewport
 */
void UUIScreenObject::NotifyResolutionChanged( const FVector2D& OldViewportSize, const FVector2D& NewViewportSize )
{
	// invalidate any faces which are set to be 
	for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
	{
		EPositionEvalType ScaleType = Position.GetScaleType((EUIWidgetFace)FaceIndex);
		if ( ScaleType >= EVALPOS_PercentageViewport && ScaleType < EVALPOS_MAX )
		{
			InvalidatePosition(FaceIndex);
		}
	}

	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		UUIObject* Child = Children(ChildIndex);
		if ( Child != NULL )
		{
			Child->NotifyResolutionChanged(OldViewportSize, NewViewportSize);
			if ( OBJ_DELEGATE_IS_SET(Child,NotifyResolutionChanged) )
			{
				Child->delegateNotifyResolutionChanged(OldViewportSize, NewViewportSize);
			}
		}
	}
}

/**
 * Returns the position of this screen object, converted into the specified format.
 *
 * @param	Face			indicates which face to return the position for
 * @param	OutputType		determines the format of the result.
 *							EVALPOS_None:
 *								return value is formatted using this the ScaleType configured for that face of the widget
 *							EVALPOS_PercentageOwner:
 *							EVALPOS_PercentageScene:
 *							EVALPOS_PercentageViewport:
 *								return a value between 0.0 and 1.0, which represents the percentage of the corresponding
 *								base's actual size.
 *							EVALPOS_PixelOwner
 *							EVALPOS_PixelScene
 *							EVALPOS_PixelViewport
 *								return the actual pixel values of the specified face's position, relative to the base indicated
 *								by the evaluation type
 * @param	bZeroOrigin		specify TRUE to evaluate the position's value without considering the origin offset of the viewport
 *	@param	bIgnoreDockPadding
 *							used to prevent recursion when evaluting docking links
 *
 * @return	the value of this widget's position for the specified face, formatted into the specified output type.
 */
FLOAT UUIScreenObject::GetPosition( BYTE Face, BYTE OutputType/*=EVALPOS_None*/, UBOOL bZeroOrigin/*=FALSE*/, UBOOL bIgnoreDockPadding/*=FALSE*/ ) const
{
	SCOPE_CYCLE_COUNTER(STAT_UIGetWidgetPosition);

	EUIWidgetFace TargetFace = (EUIWidgetFace)Face;
	FLOAT Result = Position.GetPositionValue(this, TargetFace);
	if ( OutputType == EVALPOS_None )
		return Result;

	FLOAT BaseValue=0.f;
	FLOAT BaseExtent=1.f;

	const UUIObject* Widget = ConstCast<UUIObject>(this);
	if ( Widget != NULL && Position.IsPositionCurrent(Widget, TargetFace) )
	{
		// get the viewport origin
		Result = Widget->RenderBounds[Face];
		if ( !bZeroOrigin )
		{
			FVector2D ViewportOrigin;
			if ( !GetViewportOrigin(ViewportOrigin) )
			{
				ViewportOrigin.X = ViewportOrigin.Y = 0.f;
			}
			
			if ( Face % UIORIENT_MAX == 0 )
			{
				// left or right face
				Result += ViewportOrigin.X;
			}
			else
			{
				Result += ViewportOrigin.Y;
			}
		}
	}
	else
	{
		// first, determine the actual value of this screen position
		if ( Widget != NULL && Widget->DockTargets.IsDocked(TargetFace) )
		{
			UUIObject* DockTarget = Widget->DockTargets.GetDockTarget(TargetFace);
			if ( DockTarget != NULL )
			{
				Result = DockTarget->GetPosition(Widget->DockTargets.GetDockFace(TargetFace), EVALPOS_PixelViewport);
				if ( !bIgnoreDockPadding )
				{
					Result += Widget->DockTargets.GetDockPadding(TargetFace);
				}
			}
			else if ( Widget->GetScene() != NULL )
			{
				Result = Widget->GetScene()->GetPosition(Widget->DockTargets.GetDockFace(TargetFace), EVALPOS_PixelViewport);
				if ( !bIgnoreDockPadding )
				{
					Result += Widget->DockTargets.GetDockPadding(TargetFace);
				}
			}
			else
			{
				FUIScreenValue::CalculateBaseValue(this, TargetFace, Position.GetScaleType(TargetFace), BaseValue, BaseExtent);
				Result = BaseValue + (BaseExtent * Result);
			}
		}
		else
		{
			FUIScreenValue::CalculateBaseValue(this, TargetFace, Position.GetScaleType(TargetFace), BaseValue, BaseExtent);
			Result = BaseValue + (BaseExtent * Result);
		}

		// factor in the offset for the viewport origin, unless the result should not include viewport offsets
		if ( !bZeroOrigin )
		{
			// get the viewport origin
			FVector2D ViewportOrigin;
			if ( !GetViewportOrigin(ViewportOrigin) )
			{
				ViewportOrigin.X = ViewportOrigin.Y = 0.f;
			}

			if ( Face % UIORIENT_MAX == 0 )
			{
				// left or right face
				Result += ViewportOrigin.X;
			}
			else
			{
				// top or bottom face
				Result += ViewportOrigin.Y;
			}
		}
	}

	if ( OutputType != EVALPOS_PixelViewport )
	{
		// the output type might be 
		FUIScreenValue::CalculateBaseValue(this, TargetFace, (EPositionEvalType)OutputType, BaseValue, BaseExtent);

		//@todo - do we need to do any extra work if bZeroOrigin is TRUE?
		Result -= BaseValue;
		Result /= BaseExtent;
	}

	return Result;
}

/**
 * Returns the width or height for this widget
 *
 * @param	Dimension			UIORIENT_Horizontal to get the width, UIORIENT_Vertical to get the height
 * @param	OutputType			indicates the format of the returnedvalue
 * @param	bIgnoreDockPadding	used to prevent recursion when evaluting docking links
 */
FLOAT UUIScreenObject::GetBounds( /*EUIOrientation*/BYTE Dimension, /*EPositionEvalType*/BYTE OutputType/*=EVALPOS_None*/, UBOOL bIgnoreDockPadding/*=FALSE*/ ) const
{
	return Position.GetBoundsExtent( this, (EUIOrientation)Dimension, (EPositionEvalType)OutputType, bIgnoreDockPadding );
}

/**
 * Returns this widget's absolute normalized screen position as a vector.
 *
 * @param	bIncludeParentPosition	if TRUE, coordinates returned will be absolute (relative to the viewport origin); if FALSE
 *									returned coordinates will be relative to the owning widget's upper left corner, if applicable.
 * @param	bUseAbsolutePositions	specify TRUE to use the min of opposing faces as the resulting position vector (i.e. if the bottom face's
 *									value is actually less that the top face, use the bottom face's value instead)
 */
FVector UUIScreenObject::GetPositionVector( UBOOL bIncludeParentPosition/*=TRUE*/ ) const
{
	EPositionEvalType ResultType = bIncludeParentPosition ? EVALPOS_PixelViewport : EVALPOS_PixelOwner;
	return FVector(
		GetPosition(UIFACE_Left, ResultType, FALSE),
		GetPosition(UIFACE_Top, ResultType, FALSE),
		0.f);
}

/**
 * Changes this widget's position to the specified value for the specified face.
 *
 * @param	NewValue		the new value (in pixels or percentage) to use
 * @param	Face			indicates which face to change the position for
 * @param	InputType		indicates the format of the input value
 *							EVALPOS_None:
 *								NewValue will be considered to be in whichever format is configured as the ScaleType for the specified face
 *							EVALPOS_PercentageOwner:
 *							EVALPOS_PercentageScene:
 *							EVALPOS_PercentageViewport:
 *								Indicates that NewValue is a value between 0.0 and 1.0, which represents the percentage of the corresponding
 *								base's actual size.
 *							EVALPOS_PixelOwner
 *							EVALPOS_PixelScene
 *							EVALPOS_PixelViewport
 *								Indicates that NewValue is an actual pixel value, relative to the corresponding base.
 * @param	bZeroOrigin		FALSE indicates that the value specified includes the origin offset of the viewport.
 */
void UUIScreenObject::SetPosition( FLOAT NewValue, BYTE Face, BYTE InputType/*=EVALPOS_PixelOwner*/, UBOOL bZeroOrigin/*=FALSE*/ )
{
	SCOPE_CYCLE_COUNTER(STAT_UISetWidgetPosition);
	EUIWidgetFace TargetFace = (EUIWidgetFace)Face;

	FLOAT ConvertedValue = NewValue;
	if ( InputType != EVALPOS_None && InputType != Position.GetScaleType(TargetFace) )
	{
		FLOAT BaseValue, BaseExtent;

		if ( InputType != EVALPOS_PixelViewport )
		{
			// first, convert the input value into absolute pixel values, if necessary	
			FUIScreenValue::CalculateBaseValue(this, TargetFace, (EPositionEvalType)InputType, BaseValue, BaseExtent);

			ConvertedValue = BaseValue + (BaseExtent * NewValue);
		}

		// get the viewport origin
		FVector2D ViewportOrigin;

		//@fixme ronp - why aren't we doing anything with the ViewportOrigin here?  shouldn't we be doing something similar to
		// the code below (i.e. ConvertedValue -= ViewportOrigin[Face%UIORIENT_MAX])
		//if ( !GetViewportOrigin(ViewportOrigin) )
		//{
		//	ViewportOrigin.X = ViewportOrigin.Y = 0.f;
		//}

		// next, if the ScaleType for this face isn't in absolute pixels, translate the value into that format 
		if ( Position.GetScaleType(TargetFace) != EVALPOS_PixelViewport )
		{
			FUIScreenValue::CalculateBaseValue(this, TargetFace, Position.GetScaleType(TargetFace), BaseValue, BaseExtent);

			ConvertedValue -= BaseValue;
			ConvertedValue /= BaseExtent;
		}
	}
	else if ( InputType == EVALPOS_PixelViewport && !bZeroOrigin )
	{
		FVector2D ViewportOrigin;
		if ( !GetViewportOrigin(ViewportOrigin) )
		{
			ViewportOrigin.X = ViewportOrigin.Y = 0.f;
		}

		ConvertedValue -= ViewportOrigin[Face % UIORIENT_MAX];
	}

	if ( Position.GetPositionValue(this, TargetFace) != ConvertedValue )
	{
		InvalidatePosition(Face);
		RefreshPosition();
	}

	Position.SetRawPositionValue(Face,ConvertedValue);
}

/**
 * Changes this widget's position to the specified value.
 * 
 * @param	LeftFace		the value (in pixels or percentage) to set the left face to
 * @param	TopFace			the value (in pixels or percentage) to set the top face to
 * @param	RightFace		the value (in pixels or percentage) to set the right face to
 * @param	BottomFace		the value (in pixels or percentage) to set the bottom face to
 * @param	InputType		indicates the format of the input value.  All values will be evaluated as this type.
 *								EVALPOS_None:
 *									NewValue will be considered to be in whichever format is configured as the ScaleType for the specified face
 *								EVALPOS_PercentageOwner:
 *								EVALPOS_PercentageScene:
 *								EVALPOS_PercentageViewport:
 *									Indicates that NewValue is a value between 0.0 and 1.0, which represents the percentage of the corresponding
 *									base's actual size.
 *								EVALPOS_PixelOwner
 *								EVALPOS_PixelScene
 *								EVALPOS_PixelViewport
 *									Indicates that NewValue is an actual pixel value, relative to the corresponding base.
 * @param	bZeroOrigin		FALSE indicates that the value specified includes the origin offset of the viewport.
 * @param	bClampValues	if TRUE, clamps the values of RightFace and BottomFace so that they cannot be less than the values for LeftFace and TopFace
 */
void UUIScreenObject::SetPosition
( 
	const FLOAT LeftFace, const FLOAT TopFace, const FLOAT RightFace, const FLOAT BottomFace,
	EPositionEvalType InputType/*=EVALPOS_Viewport*/, UBOOL bZeroOrigin/*=FALSE*/, UBOOL bClampValues/*=TRUE*/
)
{
	FLOAT ValidLeft		= LeftFace;
	FLOAT ValidTop		= TopFace;
	FLOAT ValidRight	= RightFace;
	FLOAT ValidBottom	= BottomFace;
	if ( bClampValues )
	{
		// When resizing this object we must make sure the right face never crosses the left,
		// and bottom face never crosses the top
		
		// if the left face is greater than the right face, reverse them
		if(ValidLeft > ValidRight)
		{
			ValidLeft = RightFace;
			ValidRight = LeftFace;
		}

		if ( ValidTop > ValidBottom )
		{
			ValidTop = BottomFace;
			ValidBottom = TopFace;
		}
	}

	SetPosition(ValidLeft, UIFACE_Left, InputType, bZeroOrigin);
	SetPosition(ValidTop, UIFACE_Top, InputType, bZeroOrigin);
	SetPosition(ValidRight, UIFACE_Right, InputType, bZeroOrigin);
	SetPosition(ValidBottom, UIFACE_Bottom, InputType, bZeroOrigin);
}

/**
 * @param Point	Point to check against the renderbounds of the object.
 * @return Whether or not this screen object contains the point passed in within its renderbounds.
 */
UBOOL UUIScreenObject::ContainsPoint(const FVector2D& Point) const
{
	FLOAT RenderBounds[4];

	for(INT FaceIdx = 0; FaceIdx < 4; FaceIdx++)
	{
		RenderBounds[FaceIdx] = GetPosition((EUIWidgetFace)FaceIdx, EVALPOS_PixelViewport, FALSE);
	}

	if(Point.X < RenderBounds[UIFACE_Left] ||
		Point.X > RenderBounds[UIFACE_Right] ||
		Point.Y < RenderBounds[UIFACE_Top] ||
		Point.Y > RenderBounds[UIFACE_Bottom])
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

/**
 * Generates a list of all widgets which are docked to this one.
 *
 * @param	out_DockedWidgets	receives the list of widgets which are docked to this one
 * @param	SourceFace			if specified, only widgets which are docked to this one through the specified face will be considered
 * @param	TargetFace			if specified, only widgets which are docked to the specified face on this widget will be considered
 */
void UUIScreenObject::GetDockedWidgets( TArray<UUIObject*>& out_DockedWidgets, BYTE SourceFace/*=UIFACE_MAX*/, BYTE TargetFace/*=UIFACE_MAX*/ ) const
{
	const UUIScene* OwnerScene = GetScene();
	if ( OwnerScene != NULL )
	{
		TArray<UUIObject*> SceneChildren;
		OwnerScene->GetChildren(SceneChildren, TRUE);
		for ( INT ChildIndex = 0; ChildIndex < SceneChildren.Num(); ChildIndex++ )
		{
			const UUIObject* SceneChild = SceneChildren(ChildIndex);
			if ( SceneChild != NULL && SceneChild->IsDockedTo(this, SourceFace, TargetFace) )
			{
				out_DockedWidgets.AddUniqueItem(SceneChildren(ChildIndex));
			}
		}
	}
}

/**
 * Converts a coordinate from this widget's local space (that is, tranformed by the widget's rotation) into a 2D viewport
 * location, in pixels.
 *
 * @param	CanvasPosition	a vector representing a location in widget local space.
 *
 * @return	a coordinate representing a point on the screen in pixel space
 */
FVector UUIScreenObject::Project( const FVector& CanvasPosition ) const
{
	FVector4 ScreenPosition = CanvasToScreen(CanvasPosition);
	FVector Result(ScreenToPixel(ScreenPosition), 0.f);

#if 0
	const UUIScene* SceneOwner = GetScene();
	if ( SceneOwner != NULL && SceneOwner->SceneClient != NULL && SceneOwner->SceneClient->RenderViewport != NULL )
	{

		// WorldToScreen{
		const FMatrix LocalViewProjMatrix = GetCanvasToScreen();
		const FVector4 ScreenPoint = LocalViewProjMatrix.TransformFVector(LocalPosition);
		// WorldToScreen}

		// ProjectedScreenPoint is still in normalized device coordinates (origin at center of screen) so we need to convert
		// ScreenToPixel{
		FLOAT InvW = 1.0f / ScreenPoint.W;

		FVector2D ViewportOffset;
		SceneOwner->SceneClient->GetViewportOffset(SceneOwner, ViewportOffset);
		Result.X = (0.5f + ScreenPoint.X * 0.5f * InvW) * SceneOwner->SceneClient->RenderViewport->GetSizeX() - ViewportOffset.X;
		Result.Y = (0.5f - ScreenPoint.Y * 0.5f * InvW) * SceneOwner->SceneClient->RenderViewport->GetSizeY() - ViewportOffset.Y;
		Result.Z = 0.f;
		// ScreenToPixel}

		Result /= GetViewportScale();
	}
#endif

	return Result;
}

/**
 * Converts an absolute pixel position into 3D screen coordinates.
 *
 * @param	PixelPosition	the position of the 2D point, in pixels
 *
 * @return	a position tranformed using this widget's rotation and the scene client's projection matrix.
 */
FVector UUIScreenObject::DeProject( const FVector& PixelPosition ) const
{
	FVector4 ScreenLocation = PixelToScreen(FVector2D(PixelPosition));
	ScreenLocation.Z = PixelPosition.Z;
	ScreenLocation.W = 1.f;

	FVector Result(ScreenToCanvas(ScreenLocation));
	return Result;
	
#if 0
	const UUIScene* SceneOwner = GetScene();
	if ( SceneOwner != NULL && SceneOwner->SceneClient != NULL && SceneOwner->SceneClient->RenderViewport != NULL )
	{
		const FLOAT SizeX = SceneOwner->SceneClient->RenderViewport->GetSizeX();
		const FLOAT SizeY = SceneOwner->SceneClient->RenderViewport->GetSizeY();

		// convert the screen position into normalized device coordinates
		// PixelToScreen{
		const FLOAT NormalizedX = -1 + (ScreenPosition.X / SizeX * +2.f);
		const FLOAT NormalizedY = +1 + (ScreenPosition.Y / SizeY * -2.f);
		// PixelToScreen}

		// ScreenToWorld{
		const FMatrix InverseLocalViewProjMatrix = GetInverseCanvasToScreen();
		const FVector4 LocalPosition = InverseLocalViewProjMatrix.TransformFVector4(FVector4(NormalizedX, NormalizedY, ScreenPosition.Z, 1.f));
		Result = LocalPosition / LocalPosition.W;
		// ScreenToWorld}
	}

	return Result;
#endif
}

/**
 * Transforms a vector from canvas (widget local) space into screen (D3D device) space
 *
 * @param	CanvasPosition	a vector representing a location in widget local space.
 *
 * @return	a vector representing that location in screen space.
 */
FVector4 UUIScreenObject::CanvasToScreen( const FVector& CanvasPosition ) const
{
	const FMatrix LocalViewProjMatrix = GetCanvasToScreen();
	const FVector4 ScreenPoint = LocalViewProjMatrix.TransformFVector(CanvasPosition);
	return ScreenPoint;
}

/**
 * Transforms a vector from screen (D3D device space) into pixel (viewport pixels) space.
 *
 * @param	ScreenPosition	a vector representing a location in device space
 *
 * @return	a vector representing that location in pixel space.
 */
FVector2D UUIScreenObject::ScreenToPixel( const FVector4& ScreenPoint ) const
{
	FVector Result(0);

	const UUIScene* SceneOwner = GetScene();
	if ( SceneOwner != NULL && SceneOwner->SceneClient != NULL && SceneOwner->SceneClient->RenderViewport != NULL )
	{
		const FLOAT SizeX = SceneOwner->SceneClient->RenderViewport->GetSizeX();
		const FLOAT SizeY = SceneOwner->SceneClient->RenderViewport->GetSizeY();
		const FLOAT InvW = 1.0f / ScreenPoint.W;

		FVector2D ViewportOffset;
		SceneOwner->SceneClient->GetViewportOffset(SceneOwner, ViewportOffset);

		Result.X = (0.5f + ScreenPoint.X * 0.5f * InvW) * SizeX - ViewportOffset.X + GPixelCenterOffset;
		Result.Y = (0.5f - ScreenPoint.Y * 0.5f * InvW) * SizeY - ViewportOffset.Y + GPixelCenterOffset;
		Result /= GetViewportScale();
	}

	return FVector2D(Result);
}

/**
 * Transforms a vector from pixel (viewport pixels) space into screen (D3D device) space
 *
 * @param	PixelPosition	a vector representing a location in viewport pixel space
 *
 * @return	a vector representing that location in screen space.
 */
FVector4 UUIScreenObject::PixelToScreen( const FVector2D& PixelPosition ) const
{
	FVector4 Result(PixelPosition.X, PixelPosition.Y, 0.f, 1.f);

	const UUIScene* SceneOwner = GetScene();
	if ( SceneOwner != NULL && SceneOwner->SceneClient != NULL && SceneOwner->SceneClient->RenderViewport != NULL )
	{
		const FLOAT SizeX = SceneOwner->SceneClient->RenderViewport->GetSizeX();
		const FLOAT SizeY = SceneOwner->SceneClient->RenderViewport->GetSizeY();

		// convert the screen position into normalized device coordinates
		Result.X = -1 + (PixelPosition.X / SizeX * +2.f);
		Result.Y = +1 + (PixelPosition.Y / SizeY * -2.f);
	}

	return Result;
}

/**
 * Transforms a vector from screen (D3D device space) space into canvas (widget local) space
 *
 * @param	ScreenPosition	a vector representing a location in screen space.
 *
 * @return	a vector representing that location in screen space.
 */
FVector UUIScreenObject::ScreenToCanvas( const FVector4& ScreenPosition ) const
{
	const FMatrix InverseLocalViewProjMatrix = GetInverseCanvasToScreen();
	const FVector4 CanvasPosition = InverseLocalViewProjMatrix.TransformFVector4(ScreenPosition);
	FVector Result = CanvasPosition / CanvasPosition.W;
	return Result;
}

/**
 * Transforms a 2D screen coordinate into this widget's local space in canvas coordinates.  In other words, converts a screen point into
 * what that point would be on this widget if this widget wasn't rotated.
 *
 * @param	PixelPosition	the position of the 2D point; a value from 0 - size of the viewport.
 *
 * @return	a 2D screen coordinate corresponding to where PixelPosition would be if this widget was not rotated.
 */
FVector UUIScreenObject::PixelToCanvas( const FVector2D& PixelPosition ) const
{
	const FVector TransformedStart = DeProject(FVector(PixelPosition, 0.f));
	const FVector TransformedEnd = DeProject(FVector(PixelPosition, 1.f));

	// using MAX_FLT will cause FLinePlaneIntersection to return NAN's in some cases, and we just need a point really far away so just use half max flt.
	static const FLOAT HalfMaxFloat = (MAX_FLT*0.5f);
	const FVector IntersectionPoint = FLinePlaneIntersection(TransformedStart, TransformedStart + (TransformedEnd - TransformedStart).SafeNormal() * HalfMaxFloat, FVector(0,0,0), FVector(0,0,1));
	return IntersectionPoint - FVector(GPixelCenterOffset, GPixelCenterOffset, 0.f);
}

/**
 * Returns a matrix which includes the scene client's CanvasToScreen matrix and this widget's tranform matrix.
 */
FMatrix UUIScreenObject::GetCanvasToScreen() const
{
	const UUIScene* SceneOwner = GetScene();
	if ( SceneOwner != NULL && SceneOwner->SceneClient != NULL )
	{
		return SceneOwner->SceneClient->GetCanvasToScreen(ConstCast<UUIObject>(this));
	}

	return FMatrix::Identity;
}

/**
 * Returns the inverse of the canvas to screen matrix.
 */
FMatrix UUIScreenObject::GetInverseCanvasToScreen() const
{
	const UUIScene* SceneOwner = GetScene();
	if ( SceneOwner != NULL && SceneOwner->SceneClient != NULL )
	{
		// WorldToScreen{
		return SceneOwner->SceneClient->GetInverseCanvasToScreen(ConstCast<UUIObject>(this));
	}

	return FMatrix::Identity;
}

/**
 * Gets the current UIState of this screen object
 *
 * @param	PlayerIndex	the index [into the Engine.GamePlayers array] for the player that generated this call
 */
UUIState* UUIScreenObject::GetCurrentState( INT PlayerIndex/*=INDEX_NONE*/ )
{
	UUIState* TopState = NULL;
	for ( INT StateIndex = StateStack.Num() - 1; StateIndex >= 0; StateIndex-- )
	{
		UUIState* CurrentState = StateStack(StateIndex);
		if ( CurrentState != NULL && (!GIsGame || PlayerIndex == INDEX_NONE || CurrentState->IsActiveForPlayer(PlayerIndex)) )
		{
			TopState = CurrentState;
			break;
		}
	}

	return TopState;
}

/**
 * Activates the configured initial state for this widget.
 *
 * @param	PlayerIndex			the index [into the Engine.GamePlayers array] for the player to activate this initial state for
 */
void UUIScreenObject::ActivateInitialState( INT PlayerIndex )
{
	if ( !HasActiveStateOfClass(InitialState,PlayerIndex) )
	{
		UBOOL bSuccess = TRUE;

		// must always have either the enabled or disabled state in the state stack, so if the InitialState isn't one of these
		// activate the enabled state first.
		if ( InitialState != UUIState_Enabled::StaticClass() && InitialState != UUIState_Disabled::StaticClass() )
		{
			bSuccess = ActivateStateByClass(UUIState_Enabled::StaticClass(), PlayerIndex);
		}

		if ( !bSuccess || !ActivateStateByClass(InitialState, PlayerIndex) )
		{
			debugf(NAME_Error, TEXT("Failed to activate initial state '%s' for player %i: %s"), *InitialState->GetName(), PlayerIndex, *GetWidgetPathName());
		}
	}
}

/**
 * Determine whether there are any active states of the specified class
 *
 * @param	StateClass	the class to search for
 * @param	PlayerIndex	the index [into the Engine.GamePlayers array] for the player that generated this call
 * @param	StateIndex	if specified, will be set to the index of the last state in the list of active states that
 *						has the class specified
 *
 * @return	TRUE if there is at least one active state of the class specified
 */ 
UBOOL UUIScreenObject::HasActiveStateOfClass( UClass* StateClass, INT PlayerIndex, INT* StateIndex/*=NULL*/ ) const
{
	UBOOL bResult = FALSE;
	for ( INT i = 0; i < StateStack.Num(); i++ )
	{
		UUIState* State = StateStack(i);
		if ( State->IsA(StateClass) && State->IsActiveForPlayer(PlayerIndex) )
		{
			if ( StateIndex )
			{
				*StateIndex = i;
			}

			bResult = TRUE;
			break;
		}
	}

	return bResult;
}
void UUIScreenObject::execHasActiveStateOfClass( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UClass,StateClass);
	P_GET_INT(PlayerIndex);
	P_GET_INT_OPTX_REF(StateIndex,0);
	P_FINISH;
	*(UBOOL*)Result=HasActiveStateOfClass(StateClass,PlayerIndex,pStateIndex);
}

/**
 * Adds the specified state to the screen object's StateStack.
 *
 * @param	StateToActivate		the new state for the widget
 * @param	PlayerIndex			the index [into the Engine.GamePlayers array] for the player that generated this call
 *
 * @return	TRUE if the widget's state was successfully changed to the new state.
 */
UBOOL UUIScreenObject::ActivateState( UUIState* StateToActivate, INT PlayerIndex )
{
	UBOOL bResult = FALSE;
	if ( StateToActivate != NULL && InactiveStates.FindItemIndex(StateToActivate) != INDEX_NONE )
	{
		bResult = StateToActivate->ActivateState(this,PlayerIndex);
		debugfSuppressed(NAME_DevUIStates, TEXT("%-24s %s %s %i Success:%s"), TEXT(">> ActivateState"), *GetWidgetPathName(), *StateToActivate->GetName(), PlayerIndex, bResult ? GTrue : GFalse);
	}

	return bResult;
}
/**
 * Alternate version of ActivateState that activates the first state in the InactiveStates array with the specified class
 * that isn't already in the StateStack
 */
UBOOL UUIScreenObject::ActivateStateByClass( UClass* StateToActivate, INT PlayerIndex, UUIState** StateThatWasActivated/*=NULL*/ )
{
	UBOOL bResult = FALSE;
	if ( StateToActivate != NULL )
	{
		for ( INT AvailableIndex = 0; AvailableIndex < InactiveStates.Num(); AvailableIndex++ )
		{
			if ( InactiveStates(AvailableIndex)->IsA(StateToActivate) )
			{
				if ( ActivateState(InactiveStates(AvailableIndex), PlayerIndex) )
				{
					if ( StateThatWasActivated != NULL )
					{
						*StateThatWasActivated = InactiveStates(AvailableIndex);
					}
					bResult = TRUE;
					break;
				}
			}
		}
	}

	return bResult;
}

/**
 * Changes the specified preview state on the screen object's StateStack.
 *
 * @param	StateToActivate		the new preview state
 *
 * @return	TRUE if the state was successfully changed to the new preview state.  FALSE if couldn't change
 *			to the new state or the specified state already exists in the screen object's list of active states
 */
UBOOL UUIScreenObject::ActivatePreviewState( UUIState* StateToActivate )
{
	UBOOL bResult = FALSE;

	check(StateToActivate != NULL);
	check(StateStack.Num() == 1);

	if(StateToActivate != NULL && StateStack.Num() == 1 && InactiveStates.FindItemIndex(StateToActivate) != INDEX_NONE)
	{
		UUIState* CurrState = GetCurrentState();
		if ( CurrState != StateToActivate )
		{
			StateStack(0) = StateToActivate;
			bResult = TRUE;
		}

		debugfSuppressed(NAME_DevUIStates, TEXT("%-24s %s %s Success:%s"), TEXT(">> ActivatePreviewState"), *GetWidgetPathName(), *StateToActivate->GetName(), bResult ? GTrue : GFalse);
	}

	if ( bResult )
	{
		// for any children which aren't selectable, propagate the state change to them as well
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child->IsPrivateBehaviorSet(UCONST_PRIVATE_NotEditorSelectable) )
			{
				for ( INT ChildStateIndex = 0; ChildStateIndex < Child->InactiveStates.Num(); ChildStateIndex++ )
				{
					if ( Child->InactiveStates(ChildStateIndex)->GetClass() == StateToActivate->GetClass() )
					{
						Child->ActivatePreviewState(Child->InactiveStates(ChildStateIndex));
						break;
					}
				}
			}
		}
	}

	return bResult;
}

void UUIScreenObject::execActivateStateByClass( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UClass,StateToActivate);
	P_GET_INT(PlayerIndex);
	P_GET_OBJECT_OPTX_REF(UUIState,StateThatWasActivated,NULL);
	P_FINISH;
	*(UBOOL*)Result=ActivateStateByClass(StateToActivate,PlayerIndex,pStateThatWasActivated);
}

/**
 * Removes the specified state from the screen object's state stack.
 *
 * @param	StateToRemove	the state to be removed
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated this call
 *
 * @return	TRUE if the state was successfully removed, or if the state didn't exist in the widget's list of states;
 *			false if the state overrode the request to be removed
 */
UBOOL UUIScreenObject::DeactivateState( UUIState* StateToRemove, INT PlayerIndex )
{
	UBOOL bResult = FALSE;
	if ( StateToRemove != NULL && InactiveStates.FindItemIndex(StateToRemove) != INDEX_NONE  )
	{
		bResult = StateToRemove->DeactivateState(this,PlayerIndex);
		debugfSuppressed(NAME_DevUIStates, TEXT("%-24s %s %s %i Success:%s"), TEXT("<< DeactivateState"), *GetWidgetPathName(), *StateToRemove->GetName(), PlayerIndex, bResult ? GTrue : GFalse);
	}

	return bResult;
}
/**
 * Alternate version of DeactivateState that deactivates the last state in the StateStack array that has the specified class.
 */
UBOOL UUIScreenObject::DeactivateStateByClass( UClass* StateToRemove, INT PlayerIndex, UUIState** StateThatWasRemoved/*=NULL*/ )
{
	UBOOL bResult = FALSE;
	if ( StateToRemove != NULL )
	{
		UBOOL bFoundInstance = FALSE;
		for ( INT StateIndex = StateStack.Num() - 1; StateIndex >= 0; StateIndex-- )
		{
			if ( StateStack(StateIndex)->IsA(StateToRemove) )
			{
				bFoundInstance = TRUE;
				if ( DeactivateState(StateStack(StateIndex),PlayerIndex) )
				{
					if ( StateThatWasRemoved != NULL )
					{
						*StateThatWasRemoved = StateStack(StateIndex);
					}
					bResult = TRUE;
					break;
				}
			}
		}
	}

	return bResult;
}
void UUIScreenObject::execDeactivateStateByClass( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UClass,StateToRemove);
	P_GET_INT(PlayerIndex);
	P_GET_OBJECT_OPTX_REF(UUIState,StateThatWasRemoved,NULL);
	P_FINISH;
	*(UBOOL*)Result=DeactivateStateByClass(StateToRemove,PlayerIndex,pStateThatWasRemoved);
}

void UUIScreenObject::execActivateEventByClass( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(ControllerIndex);
	P_GET_OBJECT(UClass,EventClassToActivate);
	P_GET_OBJECT_OPTX(UObject,InEventActivator,NULL);
	P_GET_UBOOL_OPTX(bActivateImmediately,0);
	P_GET_TARRAY_OPTX(INT,IndicesToActivate,TArray<INT>());
	P_GET_TARRAY_OPTX_REF(UUIEvent*,out_ActivatedEvents,TArray<UUIEvent*>());
	P_FINISH;
	ActivateEventByClass(ControllerIndex,EventClassToActivate,InEventActivator,bActivateImmediately,&IndicesToActivate,pout_ActivatedEvents);
}

/**
 * Activate the event of the specified class.
 *
 * @param	PlayerIndex				the index of the player that activated this event
 * @param	EventClassToActivate	specifies the event class that should be activated.  If there is more than one instance
 *									of a particular event class in this screen object's list of events, all instances will
 *									be activated in the order in which they occur in the event provider's list.
 * @param	InEventActivator		an optional object that can be used for various purposes in UIEvents
 * @param	bActivateImmediately	TRUE to activate the event immediately, causing its output operations to also be processed immediately.
 * @param	IndicesToActivate		Indexes into this UIEvent's Output array to activate.  If not specified, all output links
 *									will be activated
 * @param	out_ActivatedEvents		filled with the event instances that were activated.
 */
void UUIScreenObject::ActivateEventByClass( INT PlayerIndex, UClass* EventClassToActivate, UObject* InEventActivator/*=NULL*/, UBOOL bActivateImmediately/*=FALSE*/, const TArray<INT>* IndicesToActivate/*=NULL*/,TArray<UUIEvent*>* out_ActivatedEvents/*=NULL*/ )
{
	if ( GIsGame )
	{
		TArray<UUIEvent*> Events;
		TArray<UUIEvent*>& ActivatedEvents = out_ActivatedEvents != NULL
			? *out_ActivatedEvents
			: Events;

		ActivatedEvents.Empty();
		FindEventsOfClass(EventClassToActivate, ActivatedEvents);

		for ( INT EventIndex = 0; EventIndex < ActivatedEvents.Num(); EventIndex++ )
		{
			UUIEvent* Event = ActivatedEvents(EventIndex);
			Event->ConditionalActivateUIEvent(PlayerIndex, this, InEventActivator, bActivateImmediately, IndicesToActivate);
		}

		// propagate the call upwards to our parent; this allows designers to receive all events from any child widgets by placing the event
		// in the parent's sequence
		UUIScreenObject* OwnerWidget = GetParent();
		if ( OwnerWidget != NULL )
		{
			OwnerWidget->ChildEventActivated(PlayerIndex, EventClassToActivate, InEventActivator, bActivateImmediately, IndicesToActivate, out_ActivatedEvents);
		}
	}
}

/**
 * Wrapper for ActivateEventByClass; called when an event is activated by one of our children and is being propagated upwards.  In cases where
 * there are multiple child classes of the specified class, only those event classes which have TRUE for the value of bPropagateEvents are 
 * activated.
 *
 * @param	PlayerIndex				the index of the player that activated this event
 * @param	EventClassToActivate	specifies the event class that should be activated.  If there is more than one instance
 *									of a particular event class in this screen object's list of events, all instances will
 *									be activated in the order in which they occur in the event provider's list.
 * @param	InEventActivator		the object that the event was originally generated for.
 * @param	bActivateImmediately	TRUE to activate the event immediately, causing its output operations to also be processed immediately.
 * @param	IndicesToActivate		Indexes into this UIEvent's Output array to activate.  If not specified, all output links
 *									will be activated
 * @param	out_ActivatedEvents		filled with the event instances that were activated.
 */
void UUIScreenObject::ChildEventActivated( INT PlayerIndex, UClass* EventClassToActivate, UObject* InEventActivator, UBOOL bActivateImmediately/*=FALSE*/, const TArray<INT>* IndicesToActivate/*=NULL*/, TArray<UUIEvent*>* out_ActivatedEvents/*=NULL*/ )
{
	if ( GIsGame )
	{
		TArray<UUIEvent*> Events;
		TArray<UUIEvent*>& ActivatedEvents = out_ActivatedEvents != NULL
			? *out_ActivatedEvents
			: Events;

		const INT StartIndex=ActivatedEvents.Num();
		FindEventsOfClass(EventClassToActivate, ActivatedEvents);

		// remove any events which shouldn't be activated through propagation
		for ( INT EventIndex = StartIndex; EventIndex < ActivatedEvents.Num(); EventIndex++ )
		{
			UUIEvent* Event = ActivatedEvents(EventIndex);
			if ( Event->bPropagateEvent )
			{
				Event->ConditionalActivateUIEvent(PlayerIndex, this, InEventActivator, bActivateImmediately, IndicesToActivate);
			}
			else
			{
				ActivatedEvents.Remove(EventIndex--);
			}
		}

		// propagate the call upwards to our parent; this allows designers to receive all events from any child widgets by placing the event
		// in the parent's sequence
		UUIScreenObject* OwnerWidget = GetParent();
		if ( OwnerWidget != NULL )
		{
			OwnerWidget->ChildEventActivated(PlayerIndex, EventClassToActivate, InEventActivator, bActivateImmediately, IndicesToActivate, out_ActivatedEvents);
		}
	}
}

/**
 * Finds UIEvent instances of the specified class.
 *
 * @param	EventClassToFind		specifies the event class to search for.
 * @param	out_EventInstances		an array that will contain the list of event instances of the specified class.
 * @param	LimitScope				if specified, only events contained by the specified state's sequence will be returned.
 * @param	bExactClass				if TRUE, only events that have the class specified will be found.  Otherwise, events of that class
 *									or any of its child classes will be found.
 */
void UUIScreenObject::FindEventsOfClass( UClass* EventClassToFind, TArray<UUIEvent*>& out_EventInstances, UUIState* LimitScope/*=NULL*/, UBOOL bExactClass/*=FALSE*/ )
{
	check(EventClassToFind);
	check(EventClassToFind->IsChildOf(UUIEvent::StaticClass()));

	// optimization - since this method is called recursively, StartingIndex is used to ensure that we only check the events we
	// added, when removing elements that don't match EventClassToFind if bExactClass is TRUE
	const INT StartingIndex = out_EventInstances.Num();

	if ( LimitScope == NULL && EventProvider != NULL && EventProvider->EventContainer != NULL )
	{
		// add the events contained in this widget's global sequence
		EventProvider->EventContainer->GetUIEvents(out_EventInstances, EventClassToFind);
	}

	// now search through all state sequences for any events of this type
	for ( INT StateIndex = 0; StateIndex < StateStack.Num(); StateIndex++ )
	{
		UUIState* State = StateStack(StateIndex);
		if ( LimitScope == NULL || LimitScope == State )
		{
			UUIStateSequence* Sequence = StateStack(StateIndex)->StateSequence;
			if ( Sequence != NULL )
			{
				Sequence->GetUIEvents(out_EventInstances,EventClassToFind);
			}
		}
	}

	// now remove any events which are children of the specified class, if we want events with that exact class
	if ( bExactClass == TRUE )
	{
		for ( INT EventIndex = out_EventInstances.Num() - 1; EventIndex >= StartingIndex; EventIndex-- )
		{
			UUIEvent* Event = out_EventInstances(EventIndex);
			if ( Event->GetClass() != EventClassToFind )
			{
				out_EventInstances.Remove(EventIndex);
			}
		}
	}
}

/**
 * Routes rendering calls to children of this screen object.
 *
 * @param	Canvas	the canvas to use for rendering
 */
void UUIScreenObject::Render_Children( FCanvas* Canvas )
{
	UUIScene* OwnerScene = GetScene();

	// store the current global alph modulation
	const FLOAT CurrentAlphaModulation = Canvas->AlphaModulate;

	// if we're focused, we'll need to render any focused children last so that they always render on top.
	// the easiest way to do this is to build a list of Children array indexes that we'll render - indexes for
	// focused children go at the end; indexes for non-focused children go just before the index of the first focused child.
	TArray<UUIObject*> RenderList = Children;
	{
		for ( INT PlayerIndex = 0; PlayerIndex < FocusControls.Num(); PlayerIndex++ )
		{
			UUIObject* FocusedPlayerControl = FocusControls(PlayerIndex).GetFocusedControl();
			if ( FocusedPlayerControl != NULL )
			{
				INT Idx = RenderList.FindItemIndex(FocusedPlayerControl);
				if ( Idx != INDEX_NONE )
				{
					RenderList.Remove(Idx);
					RenderList.AddItem(FocusedPlayerControl);
				}
			}
		}
	}

	for ( INT i = 0; i < RenderList.Num(); i++ )
	{
		UUIObject* Child = RenderList(i);

		// apply the widget's rotation
		if ( Child->IsVisible() )
		{
			// add this widget to the scene's render stack
			OwnerScene->RenderStack.Push(Child);

			// apply the widget's transform matrix
			Canvas->PushRelativeTransform(Child->GenerateTransformMatrix(FALSE));
			
			// use the widget's ZDepth as the sorting key for the canvas
			Canvas->PushDepthSortKey(appCeil(Child->ZDepth));

			// now render the child
			Render_Child(Canvas, Child);

			// restore the previous sort key
			Canvas->PopDepthSortKey();

			// restore the previous transform
			Canvas->PopTransform();
		}
	}

	// restore the previous global fade value
	Canvas->AlphaModulate = CurrentAlphaModulation;
}

/**
 * Wrapper for rendering a single child of this widget.
 *
 * @param	Canvas	the canvas to use for rendering
 * @param	Child	the child to render
 *
 * @note: this method is non-virtual for speed.  If you need to override this method, feel free to make it virtual.
 */
void UUIScreenObject::Render_Child( FCanvas* Canvas, UUIObject* Child )
{
	checkSlow(Canvas);
	checkSlow(Child);

	// store the current global alpha modulation
	const FLOAT CurrentAlphaModulation = Canvas->AlphaModulate;

	// if hit testing (which normally only happens in the editor), render a plain
	// white texture on a hit proxy using the render bounds of this child
	if ( Canvas->IsHitTesting() )
	{
		Canvas->SetHitProxy( new HUIHitProxy(Child) );
		Canvas->AlphaModulate = 1.f;

		FLOAT X1=0.f, X2=0.f, Y1=0.f, Y2=0.f;
		Child->GetPositionExtents(X1, X2, Y1, Y2, FALSE);
		::DrawTile( Canvas, X1, Y1, X2 - X1, Y2 - Y1, 0.f, 0.f, 0.f, 0.f,
			FLinearColor(1.f,1.f,1.f) );
	}

	// apply this child's Opacity to the global alpha modulation
	Canvas->AlphaModulate = CurrentAlphaModulation * Child->Opacity;

	// now render the widget
	Child->Render_Widget(Canvas);
	Canvas->SetHitProxy(NULL);

	// now render the widget's children
	Child->Render_Children(Canvas);

	// provide a way for widgets to perform additional rendering after all its children have been rendered
	Child->PostRender_Widget(Canvas);

	// restore the original alpha modulation
	Canvas->AlphaModulate = CurrentAlphaModulation;
}


/**
 * Attach and initialize any 3D primitives for this widget and its children.
 *
 * @param	CanvasScene		the scene to use for attaching 3D primitives
 */
void UUIScreenObject::InitializePrimitives( FCanvasScene* CanvasScene )
{
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		UUIObject* Child = Children(ChildIndex);
		Child->InitializePrimitives(CanvasScene);
	}
}

/**
 * Routes the call to UpdatePrimitives to all children of this widget.
 *
 * @param	CanvasScene		the scene to use for attaching any 3D primitives
 */
void UUIScreenObject::UpdateChildPrimitives( FCanvasScene* CanvasScene )
{
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		UUIObject* Child = Children(ChildIndex);

		Child->UpdateWidgetPrimitives(CanvasScene);
		Child->UpdateChildPrimitives(CanvasScene);
	}
}

/**
 * Plays the sound cue associated with the specified name;  simple wrapper method for calling UIInteraction::PlayUISound
 *
 * @param	SoundCueName	the name of the UISoundCue to play; should corresond to one of the values of the UISoundCueNames array.
 * @param	PlayerIndex		allows the caller to indicate which player controller should be used to play the sound cue.  For the most
 *							part, all sounds can be played by the first player, regardless of who generated the play sound event.
 *
 * @return	TRUE if the sound cue specified was found in the currently active skin, even if there was no actual USoundCue associated
 *			with that UISoundCue.
 */
UBOOL UUIScreenObject::PlayUISound( FName SoundCueName, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	UUIInteraction* UIController = GetCurrentUIController();
	if ( UIController != NULL )
	{
		bResult = UIController->PlayUISound(SoundCueName,PlayerIndex);
	}

	return bResult;
}
void UUIScreenObject::execPlayUISound( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(SoundCueName);
	P_GET_INT_OPTX(PlayerIndex,0);
	P_FINISH;

	*(UBOOL*)Result=PlayUISound(SoundCueName,PlayerIndex);
}

/**
 * Routing event for the input we received.  This function first sees if there are any kismet actions that are bound to the
 * input.  If not, it passes the input to the widget's default input event handler.
 *
 * Only called if this widget is in the owning scene's InputSubscribers map for the corresponding key.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIScreenObject::HandleInputKeyEvent( const FInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;

	// Pass it off to the delegate for the screenobject first..
	if ( DELEGATE_IS_SET(OnRawInputKey) )
	{
		bResult = delegateOnRawInputKey(EventParms);
	}

	// Allow bound kismet actions try to process the input first, if they fail to use the input, then pass it to the default widget key handlers.
	bResult = bResult || ProcessActions(EventParms);
	if ( !bResult )
	{
		// convert this input key/event/modifier into an input alias
		FName InputKeyAlias;
		if ( TranslateKey(EventParms, InputKeyAlias) )
		{
			FSubscribedInputEventParameters ProcessedEventParms( EventParms, InputKeyAlias );
			if ( ProcessedEventParms.EventType == IE_Axis )
			{
				if ( DELEGATE_IS_SET(OnProcessInputAxis) )
				{
					bResult = delegateOnProcessInputAxis(ProcessedEventParms);
				}

				bResult = bResult || ProcessInputAxis(ProcessedEventParms);
			}
			else
			{
				if ( DELEGATE_IS_SET(OnProcessInputKey) )
				{
					bResult = delegateOnProcessInputKey(ProcessedEventParms);
				}

				bResult = bResult || ProcessInputKey(ProcessedEventParms);
			}
		}
	}

	return bResult;
}

/**
 * Sees if there are any kismet actions that are responding to the input we received.  If so, execute the action
 * that is currently bound to the event we just received.
 *
 * Only called if this widget is in the owning scene's InputSubscribers map for the corresponding key.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIScreenObject::ProcessActions( const FInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;

	if ( EventProvider != NULL && EventProvider->InputProcessor != NULL )
	{
		TArray<FInputKeyAction> InputActions;
		EventProvider->InputProcessor->ActionMap.MultiFind(EventParms.InputKeyName, InputActions);
		for ( INT ActionIndex = 0; ActionIndex < InputActions.Num(); ActionIndex++ )
		{
			FInputKeyAction& Action = InputActions(ActionIndex);
			if ( Action.InputKeyState == EventParms.EventType )
			{
				const INT ExecuteCount = Action.ActionsToExecute.Num();
				if ( ExecuteCount )
				{
					for(INT ExecuteIdx=0; ExecuteIdx < ExecuteCount; ExecuteIdx++)
					{
						USequenceAction* ExecuteAction = Action.ActionsToExecute(ExecuteIdx);
						if(ExecuteAction)
						{
							// don't call OnReceivedImpulse, as it is assumed that if OnReceivedImpulse is called then the action was not activated directly
							// but rather from a linked event
							ExecuteAction->PlayerIndex = EventParms.PlayerIndex;
							bResult = ExecuteAction->ParentSequence->QueueSequenceOp(ExecuteAction);
						}
						else
						{
							bResult = TRUE;
						}
					}
					
				}
				else
				{
					return TRUE;
				}
			}
		}
	}

	return bResult;
}

/**
 * Determines whether this widget should process the specified input event + state.  If the widget is configured
 * to respond to this combination of input key/state, any actions associated with this input event are activated.
 *
 * Only called if this widget is in the owning scene's InputSubscribers map for the corresponding key.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIScreenObject::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	// ProcessInputKey should never be called with KEY_Character
	checkSlow(EventParms.InputKeyName != KEY_Character);

	// Translate the Unreal key to a UI Key Event.
	UBOOL bResult = FALSE;
	EUIWidgetFace NavigateFace = UIFACE_MAX;
	FName NavigateCue;

	if( EventParms.InputAliasName == UIKEY_Consume)
	{
		// The consume event just uses the key without performing any actions.
		bResult = TRUE;
	}
	else if ( EventParms.InputAliasName == UIKEY_NextControl || EventParms.InputAliasName == UIKEY_PrevControl )
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Repeat )
		{
			if ( EventParms.InputAliasName == UIKEY_NextControl )
			{
				bResult = NextControl(this, EventParms.PlayerIndex);
			}
			else
			{
				bResult = PrevControl(this, EventParms.PlayerIndex);
			}
		}

		// If this input key is mapped to navigation, return TRUE even if we can't navigate anywhere to indicate that we've processed the input
		bResult = TRUE;
	}
	else if(EventParms.InputAliasName == UIKEY_NavFocusUp)
	{
		NavigateFace = UIFACE_Top;
		NavigateCue = NavigateUpCue;
	}
	else if(EventParms.InputAliasName == UIKEY_NavFocusDown)
	{
		NavigateFace = UIFACE_Bottom;
		NavigateCue = NavigateDownCue;
	}
	else if(EventParms.InputAliasName == UIKEY_NavFocusLeft)
	{
		NavigateFace = UIFACE_Left;
		NavigateCue = NavigateLeftCue;
	}
	else if(EventParms.InputAliasName == UIKEY_NavFocusRight)
	{
		NavigateFace = UIFACE_Right;
		NavigateCue = NavigateRightCue;
	}

	// See if they pressed a navigate key.
	if(NavigateFace != UIFACE_MAX)
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Repeat )
		{
			// If the navigation was successful, play a navigate sound
			// @todo ronp - make sure we're not playing a "set focus" sound as well
			if ( NavigateFocus(NULL, NavigateFace, EventParms.PlayerIndex) )
			{
				PlayUISound(NavigateCue, EventParms.PlayerIndex);
			}
		}

		// If this input key is mapped to navigation, return TRUE even if we can't navigate anywhere to indicate that we've processed the input
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Determines whether this widget should process the specified axis input event (mouse/joystick movement).
 * If the widget is configured to respond to this axis input event, any actions associated with
 * this input event are activated.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the axis movement, FALSE to pass it on.
 */
UBOOL UUIScreenObject::ProcessInputAxis( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;

	EUIWidgetFace NavigateFace = UIFACE_MAX;
	if ( EventParms.InputAliasName == UIKEY_Consume )
	{
		// The consume event just uses the key without performing any actions.
		bResult = TRUE;
	}
	else if ( EventParms.InputAliasName == UIKEY_NavFocusUp )
	{
		NavigateFace = UIFACE_Top;
	}
	else if ( EventParms.InputAliasName == UIKEY_NavFocusDown )
	{
		NavigateFace = UIFACE_Bottom;
	}
	else if ( EventParms.InputAliasName == UIKEY_NavFocusLeft )
	{
		NavigateFace = UIFACE_Left;
	}
	else if ( EventParms.InputAliasName == UIKEY_NavFocusRight )
	{
		NavigateFace = UIFACE_Right;
	}

	// See if they pressed a navigate key.
	if ( NavigateFace != UIFACE_MAX )
	{
		bResult = NavigateFocus(NULL, NavigateFace, EventParms.PlayerIndex);
	}

	return bResult;
}

/**
 * Activates any actions assigned to the specified character in this widget's input processor.
 *
 * Only called if this widget is in the owning scene's InputSubscriptions map for the KEY_Unicode key.
 *
 * @param	PlayerIndex		index [into the Engine.GamePlayers array] of the player that generated this event
 * @param	Character		the character that was received
 *
 * @return	TRUE to consume the character, false to pass it on.
 */
UBOOL UUIScreenObject::ProcessInputChar( INT PlayerIndex, TCHAR Character )
{
	UBOOL bResult = FALSE;

	if ( EventProvider != NULL && EventProvider->InputProcessor != NULL )
	{
		// activate any UIActions that are hooked to this input event
		TArray<FInputKeyAction> InputActions;
		EventProvider->InputProcessor->ActionMap.MultiFind(KEY_Character, InputActions);
		for ( INT ActionIndex = 0; ActionIndex < InputActions.Num(); ActionIndex++ )
		{
			FInputKeyAction& Action = InputActions(ActionIndex);

			const INT ExecuteCount = Action.ActionsToExecute.Num();
			for(INT ExecuteIdx=0; ExecuteIdx<ExecuteCount; ExecuteIdx++)
			{
				USequenceAction* InputAction = Action.ActionsToExecute(ExecuteIdx);
				InputAction->PlayerIndex = PlayerIndex;
				bResult = EventProvider->EventContainer->QueueSequenceOp( InputAction ) || bResult;
			}
		}
	}

	return bResult;
}

/**
 * Generates an array of indexes, which correspond to indexes into the Engine.GamePlayers array for the players that
 * this control accepts input from.
 */
void UUIScreenObject::GetInputMaskPlayerIndexes( TArray<INT>& out_Indexes )
{
	out_Indexes.Empty();
	for ( INT PlayerIndex = 0; PlayerIndex < UCONST_MAX_SUPPORTED_GAMEPADS; PlayerIndex++ )
	{
		// if this control supports input for this player, add the index to the list.
		if ( AcceptsPlayerInput(PlayerIndex) )
		{
			out_Indexes.AddItem(PlayerIndex);
		}
	}
}

/**
 * Converts an input key name (e.g. KEY_Enter) to a UI action key name (UIKEY_Clicked)
 *
 * @param	EventParms		the parameters for the input event
 * @param	out_UIKeyName	will be set to the UI action key name that is mapped to the specified input key name.
 *
 * @return	TRUE if InputKeyName was successfully converted into a UI action key name.
 */
UBOOL UUIScreenObject::TranslateKey(  const FInputEventParameters& EventParms, FName& out_UIKeyName )
{
	UBOOL bTranslatedKey = FALSE;

	UUIInteraction* Interaction = GetCurrentUIController();

	// look up the input key alias table for this widget's class
	FUIInputAliasClassMap* WidgetKeyMapping = Interaction->WidgetInputAliasLookupTable.FindRef(GetClass());
	if(WidgetKeyMapping != NULL)
	{
		// each state can contain different input mappings, so iterate through the list of currently active states to
		// see if there is a corresponding UI action for this input key in any of the active states.
		for(INT StateIdx = StateStack.Num() - 1; !bTranslatedKey && StateIdx >= 0; StateIdx--)
		{
			const UUIState* State = StateStack(StateIdx);
			FUIInputAliasMap* InputMap = WidgetKeyMapping->StateLookupTable.Find(State->GetClass());
			if(InputMap != NULL)
			{
				// get the full list of input action aliases which can be triggered from the input key
				TArray<FUIInputAliasValue> InputKeyAliases;
				InputMap->InputAliasLookupTable.MultiFind(EventParms.InputKeyName, InputKeyAliases);
				for ( INT AliasIndex = 0; AliasIndex < InputKeyAliases.Num(); AliasIndex++ )
				{
					const FUIInputAliasValue& InputAliasValue = InputKeyAliases(AliasIndex);

					// if the current modifier key states satisfy the input alias value's modifier flag mask, return that input key
					if ( InputAliasValue.MatchesModifierState(EventParms.bAltPressed, EventParms.bCtrlPressed, EventParms.bShiftPressed) )
					{
						out_UIKeyName = InputAliasValue.InputAliasName;
						bTranslatedKey = TRUE;
						break;
					}
				}
			}
		}
	}

	return bTranslatedKey;
}

/**
 * Returns TRUE if the player associated with the specified PlayerIndex is holding the Ctrl key
 *
 * @fixme - doesn't currently respect the value of ControllerId
 */
UBOOL UUIScreenObject::IsHoldingCtrl( INT ControllerId )
{
	UBOOL bResult = FALSE;

	UUIScene* ParentScene = GetScene();
	if ( ParentScene != NULL && ParentScene->SceneClient != NULL && ParentScene->SceneClient->RenderViewport != NULL )
	{
		bResult = IsCtrlDown(ParentScene->SceneClient->RenderViewport);
	}

	return bResult;
}

/**
 * Returns TRUE if the player associated with the specified ControllerId is holding the Alt key
 *
 * @fixme - doesn't currently respect the value of ControllerId
 */
UBOOL UUIScreenObject::IsHoldingAlt( INT ControllerId )
{
	UBOOL bResult = FALSE;

	UUIScene* ParentScene = GetScene();
	if ( ParentScene != NULL && ParentScene->SceneClient != NULL && ParentScene->SceneClient->RenderViewport != NULL )
	{
		bResult = IsAltDown(ParentScene->SceneClient->RenderViewport);
	}

	return bResult;
}

/**
 * Returns TRUE if the player associated with the specified ControllerId is holding the Shift key
 *
 * @fixme - doesn't currently respect the value of ControllerId
 */
UBOOL UUIScreenObject::IsHoldingShift( INT ControllerId )
{
	UBOOL bResult = FALSE;

	UUIScene* ParentScene = GetScene();
	if ( ParentScene != NULL && ParentScene->SceneClient != NULL && ParentScene->SceneClient->RenderViewport != NULL )
	{
		bResult = IsShiftDown(ParentScene->SceneClient->RenderViewport);
	}

	return bResult;
}

// Wrappers for primary menu states

/** 
 * Attempts to set the object to the enabled/disabled state specified. 
 *
 * @param bEnabled		Whether to enable or disable the widget.
 * @param PlayerIndex	Player index to set the state for.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
UBOOL UUIScreenObject::SetEnabled( UBOOL bEnabled, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	if(bEnabled)
	{
		bResult = ActivateStateByClass(UUIState_Enabled::StaticClass(), PlayerIndex);
	}
	else
	{
		bResult = ActivateStateByClass(UUIState_Disabled::StaticClass(), PlayerIndex);
	}

	return bResult;
}

/**
 * Returns TRUE if this widget has a UIState_Enabled object in its StateStack
 *
 * @param	StateIndex	if specified, will be set to the index of the last state in the list of active states that
 *						has the class specified
 */
UBOOL UUIScreenObject::IsEnabled( INT PlayerIndex/*=0*/, INT* StateIndex/*=NULL*/ ) const
{
	return HasActiveStateOfClass(UUIState_Enabled::StaticClass(),PlayerIndex, StateIndex);
}
/**
 * Returns TRUE if this widget has a UIState_Focused object in its StateStack
 *
 * @param	StateIndex	if specified, will be set to the index of the last state in the list of active states that
 *						has the class specified
 */
UBOOL UUIScreenObject::IsFocused( INT PlayerIndex/*=0*/, INT* StateIndex/*=NULL*/ ) const
{
	return HasActiveStateOfClass(UUIState_Focused::StaticClass(),PlayerIndex, StateIndex);
}
/**
 * Returns TRUE if this widget has a UIState_Active object in its StateStack
 *
 * @param	StateIndex	if specified, will be set to the index of the last state in the list of active states that
 *						has the class specified
 */
UBOOL UUIScreenObject::IsActive( INT PlayerIndex/*=0*/, INT* StateIndex/*=NULL*/ ) const
{
	return HasActiveStateOfClass(UUIState_Active::StaticClass(),PlayerIndex, StateIndex);
}
/**
 * Returns TRUE if this widget has a UIState_Pressed object in its StateStack
 *
 * @param	StateIndex	if specified, will be set to the index of the last state in the list of active states that
 *						has the class specified
 */
UBOOL UUIScreenObject::IsPressed( INT PlayerIndex/*=0*/, INT* StateIndex/*=NULL*/ ) const
{
	return HasActiveStateOfClass(UUIState_Pressed::StaticClass(),PlayerIndex, StateIndex);
}

void UUIScreenObject::execIsEnabled( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT_OPTX(PlayerIndex,GetBestPlayerIndex());
	P_GET_INT_OPTX_REF(StateIndex,0);
	P_FINISH;
	*(UBOOL*)Result=IsEnabled(PlayerIndex,pStateIndex);
}
void UUIScreenObject::execIsFocused( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT_OPTX(PlayerIndex,GetBestPlayerIndex());
	P_GET_INT_OPTX_REF(StateIndex,0);
	P_FINISH;
	*(UBOOL*)Result=IsFocused(PlayerIndex,pStateIndex);
}
void UUIScreenObject::execIsActive( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT_OPTX(PlayerIndex,GetBestPlayerIndex());
	P_GET_INT_OPTX_REF(StateIndex,0);
	P_FINISH;
	*(UBOOL*)Result=IsActive(PlayerIndex,pStateIndex);
}
void UUIScreenObject::execIsPressed( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT_OPTX(PlayerIndex,GetBestPlayerIndex());
	P_GET_INT_OPTX_REF(StateIndex,0);
	P_FINISH;
	*(UBOOL*)Result=IsPressed(PlayerIndex,pStateIndex);
}

/**
 * Returns the index [into the Engine.GamePlayers array] for the player that this widget's owner scene last received
 * input from, or INDEX_NONE if the scene is NULL or hasn't received any input from players yet.
 */
INT UUIScreenObject::GetBestPlayerIndex() const
{
	INT Result = 0;

	const UUIScene* OwnerScene = GetScene();
	if ( OwnerScene != NULL )
	{
		Result = OwnerScene->LastPlayerIndex;
		if ( Result == INDEX_NONE && OwnerScene->PlayerOwner != NULL )
		{
			Result = Max(0, GEngine->GamePlayers.FindItemIndex(OwnerScene->PlayerOwner));
		}
	}

	return Max(0, Result);
}

/**
 * Rebuilds the navigation links between the children of this screen object and recalculates the child that should
 * be the first & last focused control.
 */
void UUIScreenObject::RebuildNavigationLinks()
{
	// generate tab indexes for any widgets which do not have a valid tab index
	GenerateAutomaticTabIndexes();

	// rebuild the links used for navigation with the keyboard
	RebuildKeyboardNavigationLinks();

	// now attempt to programmatically calculate the closest sibling for each face of this widget
	// and set that as the navigation link for that direction
	GenerateAutoNavigationLinks();

	// notify all children to rebuild their navigation links
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		UUIObject* Child = Children(ChildIndex);
		Child->RebuildNavigationLinks();
	}
}

/**
 * Calculates the ideal tab index for all children of this widget and assigns the tab index to the child widget, unless
 * that widget's tab index has been specifically set by the designer.
 */
void UUIScreenObject::GenerateAutomaticTabIndexes()
{
	if ( GIsGame )
	{
		// this will be used to set the TabIndexes for any children which haven't been assigned a valid TabIndex so that
		// the sorting order is stable
		INT AutoTabIndex=0;

		// we don't want our automatically assigned tab indexes to conflict with any that were set by the designer, so first
		// determine what the starting value should be by making it 1 higher than any assigned TabIndex.
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL )
			{
				AutoTabIndex = Max(AutoTabIndex, Child->TabIndex + 1);
			}
		}

		// now set the tab indexes for those that don't have one
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL && Child->TabIndex == INDEX_NONE )
			{
				for ( INT PlayerIndex = 0; PlayerIndex < FocusPropagation.Num(); PlayerIndex++ )
				{
					if ( Child->CanAcceptFocus(PlayerIndex) )
					{
						Child->TabIndex = AutoTabIndex++;
						break;
					}
				}
			}
		}
	}
}

IMPLEMENT_COMPARE_POINTER(UUIObject,UnUIObjects_FocusChildrenTabOrder,{ return A->TabIndex - B->TabIndex; });

/**
 * Assigns values to the links which are used for navigating through this widget using the keyboard.  Sets the first and
 * last focus targets for this widget as well as the next/prev focus targets for all children of this widget.
 */
void UUIScreenObject::RebuildKeyboardNavigationLinks()
{
	// first, clear out the old values
	for ( INT PlayerIndex = 0; PlayerIndex < FocusPropagation.Num(); PlayerIndex++ )
	{
		FocusPropagation(PlayerIndex).SetFirstFocusTarget(NULL);
		FocusPropagation(PlayerIndex).SetLastFocusTarget(NULL);
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);

			// verify that the child's FocusPropagation array is in sync with ours
			//check(Child->FocusPropagation.IsValidIndex(PlayerIndex));
			// @todo: Note that this is a work around to fix a problem with owner scene not being initialized before initialize player tracking is originally called.
			if(Child->FocusPropagation.IsValidIndex(PlayerIndex)==FALSE)
			{
				Child->InitializePlayerTracking();
			}

			Child->FocusPropagation(PlayerIndex).SetNextFocusTarget(NULL);
			Child->FocusPropagation(PlayerIndex).SetPrevFocusTarget(NULL);
		}

		// build an array of children which are capable of accepting focus
		TArray<UUIObject*> EligibleChildren;
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL && Child->CanAcceptFocus(PlayerIndex) )
			{
				EligibleChildren.AddItem(Child);
			}
		}

		Sort<USE_COMPARE_POINTER(UUIObject,UnUIObjects_FocusChildrenTabOrder)>( &EligibleChildren(0), EligibleChildren.Num()  );
		// setup the first & last focus targets for this widget, as well as the next/prev focus targets for children
		// of this widget
		if ( EligibleChildren.Num() > 0 )
		{
			// set the first focus target for this widget
			FocusPropagation(PlayerIndex).SetFirstFocusTarget(EligibleChildren(0));

			// set the last focus target for this widget
			FocusPropagation(PlayerIndex).SetLastFocusTarget(EligibleChildren.Last());

			// now setup the focus next/prev links for each of the children
			for ( INT ChildIndex = 0; ChildIndex < EligibleChildren.Num(); ChildIndex++ )
			{
				UUIObject* Child = EligibleChildren(ChildIndex);
				UUIObject* PrevLink = (ChildIndex > 0) ? EligibleChildren(ChildIndex-1) : NULL;
				UUIObject* NextLink = (ChildIndex + 1 < EligibleChildren.Num()) ? EligibleChildren(ChildIndex+1) : NULL;

				Child->FocusPropagation(PlayerIndex).SetNextFocusTarget(NextLink);
				Child->FocusPropagation(PlayerIndex).SetPrevFocusTarget(PrevLink);
			}
		}
	}
}

/**
 * Retrieves the child of this widget which is current focused.
 *
 * @param	bRecurse		if TRUE, returns the inner-most focused widget; i.e. the widget at the end of the focus chain
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 *
 * @return	a pointer to the child (either direct or indirect) widget which is in the focused state and is the focused control
 *			for its parent widget, or NULL if this widget doesn't have a focused control.
 */
UUIObject* UUIScreenObject::GetFocusedControl( UBOOL bRecurse/*=FALSE*/, INT PlayerIndex/*=0*/ ) const
{
	UUIObject* Result = NULL;

	checkSlow(PlayerIndex<FocusControls.Num());
	UUIObject* FocusedChild = FocusControls(PlayerIndex).GetFocusedControl();
	if ( FocusedChild != NULL && bRecurse )
	{
		Result = FocusedChild->GetFocusedControl(bRecurse,PlayerIndex);
	}

	if ( Result == NULL )
	{
		Result = FocusedChild;
	}

	return Result;
}

/**
 * Retrieves the child of this widget which last had focus.
 *
 * @param	bRecurse		if TRUE, returns the inner-most previously focused widget; i.e. the widget at the end of the focus chain
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 *
 * @return	a pointer to the child (either direct or indirect) widget which was previously the focused control for its parent,
 *			or NULL if this widget doesn't have a LastFocusedControl
 */
UUIObject* UUIScreenObject::GetLastFocusedControl( UBOOL bRecurse/*=FALSE*/, INT PlayerIndex/*=0*/ ) const
{
	UUIObject* Result = NULL;

	checkSlow(PlayerIndex<FocusControls.Num());
	UUIObject* LastFocusedChild = FocusControls(PlayerIndex).GetLastFocusedControl();
	if ( LastFocusedChild != NULL && bRecurse )
	{
		Result = LastFocusedChild->GetLastFocusedControl(bRecurse, PlayerIndex);
	}

	if ( Result == NULL )
	{
		Result = LastFocusedChild;
	}

	return Result;
}


/**
 * Determines whether this widget is allowed to propagate focus chains to and from the specified widget.
 *
 * @param	TestChild	the widget to check
 *
 * @return	TRUE if the this widget is allowed to route the focus chain through TestChild.
 */
UBOOL UUIScreenObject::CanPropagateFocusFor( UUIObject* TestChild ) const
{
	return TestChild != NULL

		// @todo ronp - might need an additional check if TestChild is a UIContextMenu...like verifying that we are its parent (or perhaps at least its InvokingWindow)
		&& (ContainsChild(TestChild, FALSE) || TestChild->IsA(UUIContextMenu::StaticClass()));
}

/**
 * Activates the UIState_Focused menu state and updates the pertinent members of FocusControls.
 *
 * @param	FocusedChild	the child of this widget that should become the "focused" control for this widget.
 *							A value of NULL indicates that there is no focused child.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 */
UBOOL UUIScreenObject::GainFocus( UUIObject* FocusedChild, INT PlayerIndex/*=0*/ )
{
	// code from UE2 that isn't yet necessary, but might be in the future, so leaving it here for now
// 	if ( bNeverFocus )
// 	{
// 		return FALSE;
// 	}

// 	if ( GetScene() != ActiveMenu )
// 	{
// 		// if our page isn't the active page, don't steal focus....just pass up the call so that
// 		// our MenuOwner's LastFocusedControl can be set properly
// 		UUIScreenObject* Owner = GetParent();
// 		if ( Owner != NULL )
// 		{
// 			Owner->SetFocus(this);
// 			return TRUE;
// 		}
// 	}

	UBOOL bCanBecomeFocused = TRUE;

	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	// only the scene or the newly focused control should kill focus on the previously focused widget
	UUIScreenObject* OwnerWidget = GetParent();
	if ( OwnerWidget == NULL || (IsFocused(PlayerIndex) && OwnerWidget->GetFocusedControl(FALSE, PlayerIndex) == this) )
	{
		UUIScreenObject* RootWidget = GetScene();
		if ( RootWidget == NULL )
		{
			RootWidget = this;
		}

		// set the flag to prevent indirectly calling KillFocus on ourselves as a result of a child or sibling losing focus
		FocusPropagation(PlayerIndex).bPendingReceiveFocus = IsFocused(PlayerIndex);

		// get the currently focused control
		UUIObject* CurrentlyFocusedControl = RootWidget->GetFocusedControl(TRUE, PlayerIndex);
		if ( CurrentlyFocusedControl != NULL )
		{
			bCanBecomeFocused = CurrentlyFocusedControl->KillFocus(NULL, PlayerIndex);
		}
	}

	UBOOL bResult = FALSE;
	UBOOL bIsFocused = IsFocused(PlayerIndex);
	if ( bCanBecomeFocused )
	{
		UUIState* FocusedState = NULL;

		// @fixme - hmmmmm, by activating the state from here (rather than calling GainFocus() from the UUIState_Focused::OnActivate(), for example),
		// we don't automatically catch cases where some code manually calls ActivateState on a UIState_Focused state....hummmm.
		bResult = bIsFocused || ActivateStateByClass(UUIState_Focused::StaticClass(), PlayerIndex, &FocusedState);
		if ( bResult )
		{
			// notify our parent that we've become focused
			if ( OwnerWidget != NULL )
			{
				OwnerWidget->SetFocus(this, PlayerIndex);
			}

			// register input events.  this is done here, instead of in the UUIState_Focused::Activated() so that children
			// are inserted into the scene's InputSubscribers array in the correct order (i.e. after the parent widget has inserted its input event handlers)
			check( bIsFocused||FocusedState != NULL);
			if ( FocusedState != NULL )
			{
				EventProvider->RegisterInputEvents(FocusedState, PlayerIndex);
			}

			// we've successfully activated the focused state
			FocusControls(PlayerIndex).SetFocusedControl(FocusedChild);
		}
	}
	else if ( bIsFocused && FocusedChild != NULL && GetFocusedControl(PlayerIndex) != FocusedChild )
	{
		// if we're here, it means we weren't able to kill focus on the scene's overall focused control, probably because it's us
		// but if one of our children gained focus, we still need to set it as the focused control so we'll do that here.
		FocusControls(PlayerIndex).SetFocusedControl(FocusedChild);
	}

	FocusPropagation(PlayerIndex).bPendingReceiveFocus = FALSE;

	// if we successfully set focus, and we are the innermost focused control, play the "received focus" sound cue
	if ( bResult == TRUE && FocusedChild == NULL )
	{
		PlayUISound(FocusedCue,PlayerIndex);
	}

	return bResult;
}

/**
 * Deactivates the UIState_Focused menu state and updates the pertinent members of FocusControls.
 *
 * @param	FocusedChild	the child of this widget that is currently "focused" control for this widget.
 *							A value of NULL indicates that there is no focused child.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 */
UBOOL UUIScreenObject::LoseFocus( UUIObject* FocusedChild, INT PlayerIndex )
{
	UBOOL bResult = FALSE;

	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	if ( !FocusPropagation(PlayerIndex).bPendingReceiveFocus )
	{
		bResult = DeactivateStateByClass(UUIState_Focused::StaticClass(),PlayerIndex);
		if ( bResult )
		{
			// clear the focused control for this widget
			FocusControls(PlayerIndex).SetFocusedControl(NULL);

			// and propagate the focus loss upwards to our parent
			UUIScreenObject* Parent = GetParent();
			if ( Parent != NULL )
			{
				Parent->KillFocus(this, PlayerIndex);
			}
		}
	}

	return bResult;
}

/**
 * Activates the focused state for this widget and sets it to be the focused control of its parent (if applicable)
 *
 * @param	Sender		Control that called SetFocus.  Possible values are:
 *						-	if NULL is specified, it indicates that this is the first step in a focus change.  The widget will
 *							attempt to set focus to its most eligible child widget.  If there are no eligible child widgets, this
 *							widget will enter the focused state and start propagating the focus chain back up through the Owner chain
 *							by calling SetFocus on its Owner widget.
 *						-	if Sender is the widget's owner, it indicates that we are in the middle of a focus change.  Everything else
 *							proceeds the same as if the value for Sender was NULL.
 *						-	if Sender is a child of this widget, it indicates that focus has been successfully changed, and the focus is now being
 *							propagated upwards.  This widget will now enter the focused state and continue propagating the focus chain upwards through
 *							the owner chain.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 */
UBOOL UUIScreenObject::SetFocus(UUIScreenObject* Sender,INT PlayerIndex/* =0 */)
{
	UBOOL bResult = FALSE;

	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	if ( IsVisible() )
	{
		// if Sender == NULL, then SetFocus was called on this widget directly.  We should either propagate focus to our
		// most appropriate child widget (if possible), or set ourselves to be the focused control
		UBOOL bSetFocusToChild = (Sender != NULL && Sender == GetParent()) || (Sender == NULL && CanAcceptFocus(PlayerIndex));
		if ( bSetFocusToChild )
		{
			// we're receiving focus from our parent, the focus chain is currently going down;
			// attempt to set focus to the last focused widget
			UUIObject* LastFocused = GetLastFocusedControl(FALSE, PlayerIndex);

			// propagate the focus chain downwards...
			bResult = SetFocusToChild(LastFocused,PlayerIndex);
			if ( !bResult && CanAcceptFocus(PlayerIndex) )
			{
				// if we haven't set focus to anything yet, it means that:
				// - we don't have any children
				// - none of our children can accept focus, but we can
				// In either case, we've reached the end of the focus chain, so just set focus to this widget
				bResult = GainFocus( NULL, PlayerIndex );
			}
		}

		else
		{
			// otherwise, if one of our children called SetFocus() on this widget, it means that the focus chain is coming back up.
			// Sender should be the child of this widget that just became focused
			UUIObject* WidgetSender = Cast<UUIObject>(Sender);
			if ( WidgetSender != NULL && CanPropagateFocusFor(WidgetSender) )
			{
				bResult = GainFocus( WidgetSender, PlayerIndex );
			}
		}
	}

	return bResult;
}

/**
 * Sets focus to the specified child of this widget.
 *
 * @param	ChildToFocus	the child to set focus to.  If not specified, attempts to set focus to the most elibible child,
 *							as determined by navigation links and FocusPropagation values.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 */
UBOOL UUIScreenObject::SetFocusToChild( UUIObject* ChildToFocus/*=NULL*/, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	if ( ChildToFocus != NULL )
	{
		// this widget might have changed parents since it was set as the last focused control, so double-check that here
		if ( CanPropagateFocusFor(ChildToFocus) )
		{
			bResult = ChildToFocus->SetFocus(this, PlayerIndex);
		}
		else
		{
			// nothing...
		}
	}

	if ( !bResult )
	{
		// if we don't have success yet,
		// attempt to set focus to the widget configured to be the default focus target if we either didn't have a previously focused control
		
		if ( ChildToFocus == NULL )
		{
			UUIObject* FocusTarget = FocusPropagation(PlayerIndex).GetFirstFocusTarget();
			if ( FocusTarget != NULL )
			{
				// same deal here - verify that the specified widget is still a child of this widget
				if ( CanPropagateFocusFor(FocusTarget) )
				{
					bResult = FocusTarget->SetFocus(this, PlayerIndex);
				}
			}
			else
			{
				// nothing...
			}
		}

		// but if we did have a previously focused control, then it means that it wouldn't accept focus.  If that is because it is already
		// our focused control, then simulate success - we don't have to do anything.
		else if ( ChildToFocus == GetFocusedControl(FALSE, PlayerIndex) )
		{
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Deactivates the focused state for this widget.
 *
 * @param	Sender			the control that called KillFocus.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 */
UBOOL UUIScreenObject::KillFocus( UUIScreenObject* Sender, INT PlayerIndex )
{
	UBOOL bResult = FALSE;

	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	// if Sender == NULL, then KillFocus was called on this widget directly by something
	UBOOL bKillFocusToChild = (Sender != NULL && Sender == GetParent()) || (Sender == NULL && IsFocused(PlayerIndex));
	if ( bKillFocusToChild )
	{
		UBOOL bAllowFocusChange = TRUE;

		// if Sender is not a child of this widget, the focus chain is currently going down;
		// attempt to kill focus on our currently focused widget
		UUIObject* Focused = GetFocusedControl(FALSE, PlayerIndex);
		if ( Focused != NULL )
		{
			// this widget might have changed parents since it was set as the last focused control, so double-check that here
			if ( CanPropagateFocusFor(Focused) )
			{
				bResult = bAllowFocusChange = Focused->KillFocus(this, PlayerIndex);
			}
			else
			{
				// the focused widget is no longer a child of this widget, so we can't kill its focus
				// @todo - is this the desired behavior....? this is probably an error condition...
				Focused->KillFocus(NULL,PlayerIndex);
			}
		}

		// if bResult is FALSE and bAllowFocusChange is FALSE, it means that we have a focused widget that refused to give up focus, so bail
		// if bResult is FALSE and bAllowFocusChange is TRUE, it means that we didn't have a focused control, so we are the bottom of the focus chain.
		if ( !bResult && bAllowFocusChange )
		{
			bResult = LoseFocus(NULL, PlayerIndex);
		}
	}

	else
	{
		// otherwise, if Sender is one of our children, it means that the focus chain is coming back up.
		// here is where we'll actually deactivate the focused state for this widget
		UUIObject* WidgetSender = Cast<UUIObject>(Sender);
		if ( WidgetSender != NULL && CanPropagateFocusFor(WidgetSender) )
		{
			bResult = LoseFocus( WidgetSender, PlayerIndex );
		}
	}

	return bResult;
}

/**
 * Sets focus to the first focus target within this container.
 *
 * @param	Sender	the widget that generated the focus change.  if NULL, this widget generated the focus change.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 *
 * @return	TRUE if focus was successfully propagated to the first focus target within this container.
 */
UBOOL UUIScreenObject::FocusFirstControl( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	// attempt to set focus to the widget associated with the first focus target for this widget
	UUIObject* NavTarget = FocusPropagation(PlayerIndex).GetFirstFocusTarget();
	if ( NavTarget != NULL )
	{
		checkSlow(NavTarget->FocusPropagation.IsValidIndex(PlayerIndex));
		bResult = NavTarget->FocusFirstControl(this, PlayerIndex) || SetFocusToChild(NavTarget,PlayerIndex);
	}

	return bResult;
}

/**
 * Sets focus to the last focus target within this container.
 *
 * @param	Sender			the widget that generated the focus change.  if NULL, this widget generated the focus change.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 *
 * @return	TRUE if focus was successfully propagated to the last focus target within this container.
 */
UBOOL UUIScreenObject::FocusLastControl( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	UUIObject* NavTarget = FocusPropagation(PlayerIndex).GetLastFocusTarget();
	if ( NavTarget != NULL )
	{
		checkSlow(NavTarget->FocusPropagation.IsValidIndex(PlayerIndex));
		bResult = NavTarget->FocusLastControl(this, PlayerIndex) || SetFocusToChild(NavTarget,PlayerIndex);
	}

	return bResult;
}

/**
 * Sets focus to the next control in the tab order (relative to Sender) for widget.  If Sender is the last control in
 * the tab order, propagates the call upwards to this widget's parent widget.
 *
 * @param	Sender			the widget to use as the base for determining which control to focus next
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 *
 * @return	TRUE if we successfully set focus to the next control in tab order.  FALSE if Sender was the last eligible
 *			child of this widget or we couldn't otherwise set focus to another control.	
 */
UBOOL UUIScreenObject::NextControl( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	UUIObject* WidgetSender = Cast<UUIObject>(Sender);

	// if Sender is a child of this widget, it means that we want to move focus to the next child of this widget, if possible
	if ( WidgetSender != NULL && ContainsChild(WidgetSender) )
	{
		checkSlow(PlayerIndex<WidgetSender->FocusPropagation.Num());

		// attempt to set focus to the widget associated with that navigation link of the source
		UUIObject* NavTarget = WidgetSender->FocusPropagation(PlayerIndex).GetNextFocusTarget();
		if ( NavTarget != NULL )
		{
			checkSlow(PlayerIndex<NavTarget->FocusPropagation.Num());

			// If NavTarget contains focusable children, then simply calling SetFocus on NavTarget will
			// result in it setting focus to whichever widget was last focused.  But in this case, that won't necessarily
			// be the first child of NavTarget; so we grab NavTarget's first focus target and set focus to that widget directly
			// so that when the user presses tab while the focused control is the last widget in a particular container widget, 
			// focus is correctly set to the first widget inside the next container widget.
			bResult = NavTarget->FocusFirstControl(this,PlayerIndex) || SetFocusToChild(NavTarget,PlayerIndex);
		}
	}

	// if we're here, it means that we couldn't navigate focus to another child within this widget, so
	// propagate the call to our parent so that the next most appropriate nav target receives focus
	if ( bResult == FALSE )
	{
		UUIScreenObject* Parent = GetParent();
		if ( Parent != NULL && Sender != Parent && Parent->NextControl(this, PlayerIndex) )
		{
			bResult = TRUE;
		}
		else
		{
			// attempt to set focus to the widget associated with the first focus target for this widget
			bResult = FocusFirstControl(NULL, PlayerIndex);
		}
	}

	return bResult;
}

/**
 * Sets focus to the previous control in the tab order (relative to Sender) for widget.  If Sender is the first control in
 * the tab order, propagates the call upwards to this widget's parent widget.
 *
 * @param	Sender			the widget to use as the base for determining which control to focus next
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 *
 * @return	TRUE if we successfully set focus to the previous control in tab order.  FALSE if Sender was the first eligible
 *			child of this widget or we couldn't otherwise set focus to another control.	
 */
UBOOL UUIScreenObject::PrevControl( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	UUIObject* WidgetSender = Cast<UUIObject>(Sender);

	// if Sender is a child of this widget,
	if ( WidgetSender != NULL && ContainsChild(WidgetSender) )
	{
		checkSlow(PlayerIndex<WidgetSender->FocusPropagation.Num());

		// attempt to set focus to the widget associated with that navigation link of the source
		UUIObject* NavTarget = WidgetSender->FocusPropagation(PlayerIndex).GetPrevFocusTarget();
		if ( NavTarget != NULL )
		{
			checkSlow(PlayerIndex<NavTarget->FocusPropagation.Num());

			bResult = NavTarget->FocusLastControl(this, PlayerIndex) || SetFocusToChild(NavTarget,PlayerIndex);
		}
	}

	// if we're here, it means that we couldn't navigate focus to another child within this widget, so
	// propagate the call to our parent so that the next most appropriate nav target receives focus
	if ( bResult == FALSE )
	{
		UUIScreenObject* Parent = GetParent();
		if ( Parent != NULL && Sender != Parent && Parent->PrevControl(this, PlayerIndex) )
		{
			bResult = TRUE;
		}
		else
		{
			// attempt to set focus to the widget associated with the last focus target for this widget
			bResult = FocusLastControl(NULL, PlayerIndex);
		}
	}

	return bResult;
}

/**
 * Sets focus to the child widget that is next in the specified direction in the navigation network within this widget.
 *
 * @param	Sender		Control that called NavigateFocus.  Possible values are:
 *						-	if NULL is specified, it indicates that this is the first step in a focus change.  The widget will
 *							attempt to set focus to its most eligible child widget.  If there are no eligible child widgets, this
 *							widget will enter the focused state and start propagating the focus chain back up through the Owner chain
 *							by calling SetFocus on its Owner widget.
 *						-	if Sender is the widget's owner, it indicates that we are in the middle of a focus change.  Everything else
 *							proceeds the same as if the value for Sender was NULL.
 *						-	if Sender is a child of this widget, it indicates that focus has been successfully changed, and the focus is now being
 *							propagated upwards.  This widget will now enter the focused state and continue propagating the focus chain upwards through
 *							the owner chain.
 * @param	Direction 		the direction to navigate focus.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 *
 * @return	TRUE if the next child widget in the navigation network for the specified direction successfully received focus.
 */
UBOOL UUIScreenObject::NavigateFocus( UUIScreenObject* Sender, BYTE Direction, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	UUIObject* WidgetSender = Cast<UUIObject>(Sender);
	EUIWidgetFace NavDirection = (EUIWidgetFace)Direction;

	// if Sender is a child of this widget,
	if ( WidgetSender != NULL && ContainsChild(WidgetSender) )
	{
		// attempt to set focus to the widget associated with that navigation link of the source.
		// Since we are basically looping over widgets in the direction specified, we need to check for any 
		// cycles that may exist in our navigation chain.  
		UUIObject* NavTarget = WidgetSender->NavigationTargets.GetNavigationTarget(NavDirection);
		UUIObject* StepOverTarget = NavTarget;

		while( NavTarget != NULL && !NavTarget->CanAcceptFocus(PlayerIndex) )
		{
			if(StepOverTarget)
			{
				//@todo ronp - this code seems questionable; talk to AM
				StepOverTarget = StepOverTarget->NavigationTargets.GetNavigationTarget(NavDirection);
				if( StepOverTarget == NavTarget )
				{
					// circular link - bail out
					NavTarget = NULL;
					break;
				}
				else if ( StepOverTarget != NULL )
				{
					StepOverTarget = StepOverTarget->NavigationTargets.GetNavigationTarget(NavDirection);
					if( StepOverTarget == NavTarget )
					{
						NavTarget = NULL;
						break;
					}
				}
			}

			NavTarget = NavTarget->NavigationTargets.GetNavigationTarget(NavDirection);
		}

		if ( NavTarget != NULL )
		{
			bResult = NavTarget->SetFocus(NULL, PlayerIndex);
		}
	}

	// if we're here, it means that we couldn't navigate focus to another child within this widget, so
	// propagate the call to our parent so that the next most appropriate nav target receives focus
	if ( bResult == FALSE )
	{
		UUIScreenObject* Parent = GetParent();
		if ( Parent != NULL )
		{
			bResult = Parent->NavigateFocus(this, NavDirection, PlayerIndex);
		}
	}

	return bResult;
}

/**
 * Determines whether this widget become the focused control.
 *
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 *
 * @return	TRUE if this widget (or one of its children) is capable of becoming the focused control.
 */
UBOOL UUIScreenObject::CanAcceptFocus( INT PlayerIndex/*=0*/ ) const
{
	UBOOL bResult = FALSE;

	// we must be visible and be enabled in order to accept focus
	if ( IsVisible() && IsEnabled(PlayerIndex) && AcceptsPlayerInput(PlayerIndex) )
	{
		if ( ContainsObjectOfClass(InactiveStates, UUIState_Focused::StaticClass()) )
		{
			// we can accept focus if InactiveStates array contains a focused state
			bResult = TRUE;
		}
		else
		{
			for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
			{
				// if any of our children can accept focus, then we can accept focus as well
				UUIObject* Child = Children(ChildIndex);
				if ( Child->CanAcceptFocus(PlayerIndex) )
				{
					bResult = TRUE;
					break;
				}
			}
		}
	}

	return bResult;
}

/**
 * Determines whether this widget can accept input from the player specified
 *
 * @param	PlayerIndex		the index of the player to check
 *
 * @return	TRUE if this widget's PlayerInputMask allows it to process input from the specified player.
 */
UBOOL UUIScreenObject::AcceptsPlayerInput( INT PlayerIndex ) const
{
	UBOOL bResult = FALSE;

	if ( PlayerIndex >= 0 && PlayerIndex < UCONST_MAX_SUPPORTED_GAMEPADS )
	{
		bResult = (PlayerInputMask & (1 << PlayerIndex)) != 0;
	}

	return bResult;
}

/* ==========================================================================================================
	UUIScene
========================================================================================================== */

/** returns the current worldinfo */
AWorldInfo* UUIScene::GetWorldInfo()
{
	return GWorld->GetWorldInfo();
}

/** gets the currently active skin */
UUISkin* UUIScene::GetActiveSkin() const
{
	check(SceneClient);
	return SceneClient->ActiveSkin;
}

/**
 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
 * once the scene has been completely initialized.
 * For widgets added at runtime, called after the widget has been inserted into its parent's
 * list of children.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUIScene::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	if ( !IsInitialized() )
	{
		CreateDefaultStates();

		USceneDataStore* SceneDataStore = GetSceneDataStore();
		check(SceneDataStore != NULL);

		SceneDataStore->OwnerScene = this;
		SceneDataStore->InitializeDataStore();

		// initialize the scene's default context menu unless this is the transient scene
		if ( GIsGame && SceneTag != NAME_Transient )
		{
			GetDefaultContextMenu();
		}

		Super::Initialize(inOwnerScene, inOwner);
	}

	// initialize all child widgets
	for ( INT i = 0; i < Children.Num(); i++ )
	{
		UUIObject* Child = Children(i);

		Child->InitializePlayerTracking();
		Child->Initialize(this);
		Child->eventPostInitialize();
	}

	bInitialized = TRUE;
}

/**
 * Sets up the focus, input, and any other arrays which contain data that tracked uniquely for each active player.
 * Ensures that the arrays responsible for managing focus chains are synched up with the Engine.GamePlayers array.
 *
 * This version also calls CalculateInputMask to initialize the scene's PlayerInputMask for use by the activation
 * and initialization events that will be called as the scene is activated.  
 */
void UUIScene::InitializePlayerTracking()
{
	eventCalculateInputMask();

	Super::InitializePlayerTracking();
}

/**
 * Called when a new player has been added to the list of active players (i.e. split-screen join) after the scene
 * has been activated.
 *
 * This version updates the scene's PlayerInputMask to reflect the newly added player.
 *
 * @param	PlayerIndex		the index [into the GamePlayers array] where the player was inserted
 * @param	AddedPlayer		the player that was added
 */
void UUIScene::CreatePlayerData( INT PlayerIndex, ULocalPlayer* AddedPlayer )
{
	Super::CreatePlayerData(PlayerIndex, AddedPlayer);

	// recalculate the scene's PlayerInputMask
	eventCalculateInputMask();

	// request an update to occur in the next frame
	RequestSceneUpdate(FALSE, TRUE, TRUE, FALSE);
}

/**
 * Called when a player has been removed from the list of active players (i.e. split-screen players)
 *
 * This version updates the scene's PlayerInputMask to reflect the removed player.
 *
 * @param	PlayerIndex		the index [into the GamePlayers array] where the player was located
 * @param	RemovedPlayer	the player that was removed
 */
void UUIScene::RemovePlayerData( INT PlayerIndex, ULocalPlayer* RemovedPlayer )
{
	Super::RemovePlayerData(PlayerIndex, RemovedPlayer);

	if ( PlayerOwner == RemovedPlayer )
	{
		// clear the PlayerOwner in case we can't find another
		PlayerOwner = NULL;

		// if the player being removed is the player that owned this scene, migrate the scene to another player if possible
		for ( INT AvailableIndex = 0; AvailableIndex < GEngine->GamePlayers.Num(); AvailableIndex++ )
		{
			PlayerOwner = GEngine->GamePlayers(AvailableIndex);
			break;
		}
	}

	// recalculate the scene's PlayerInputMask
	eventCalculateInputMask();

	// request an update to occur in the next frame
	RequestSceneUpdate(FALSE, TRUE, TRUE, FALSE);
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUIScene::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddItem(UIKEY_CloseScene);
	out_KeyNames.AddItem(UIKEY_Clicked);
}

/**
 * Creates and initializes this scene's data store.
 */
void UUIScene::CreateSceneDataStore()
{
	checkSlow(SceneData == NULL);
	SceneData = ConstructObject<USceneDataStore>( USceneDataStore::StaticClass(), this );
}

/**
 * Gets the data store for this scene, creating one if necessary.
 */
USceneDataStore* UUIScene::GetSceneDataStore()
{
	if ( SceneData == NULL )
	{
		CreateSceneDataStore();
	}

	return SceneData;
}


/**
 * Notifies all children that are bound to readable data stores to retrieve their initial value from those data stores.
 */
void UUIScene::LoadSceneDataValues()
{
	//@todo - load the values for the scene data store 

	// get the list of children owned by this scene
	TArray<UUIObject*> SceneChildren = GetChildren(TRUE);
	for ( INT ChildIndex = 0; ChildIndex < SceneChildren.Num(); ChildIndex++ )
	{
		UUIObject* Widget = SceneChildren(ChildIndex);
		Widget->ResolveDefaultDataBinding(UCONST_TOOLTIP_BINDING_INDEX);
		Widget->ResolveDefaultDataBinding(UCONST_CONTEXTMENU_BINDING_INDEX);

		// if this child implements the UIDataStoreSubscriber interface, tell the child to load the value from the data store
		IUIDataStoreSubscriber* DataStoreChild = (IUIDataStoreSubscriber*)Widget->GetInterfaceAddress(IUIDataStoreSubscriber::UClassType::StaticClass());
		if ( DataStoreChild != NULL )
		{
			DataStoreChild->RefreshSubscriberValue();
		}
	}
}

/**
 * Notifies all children of this scene that are bound to writable data stores to publish their values to those data stores.
 *
 * @param	bUnbindSubscribers	if TRUE, all data stores bound by widgets and strings in this scene will be unbound.
 */
void UUIScene::SaveSceneDataValues( UBOOL bUnbindSubscribers/*=FALSE*/ )
{
	// this is the list of data stores that we should call OnCommit for
	TArray<UUIDataStore*> SceneDataStores;

	TArray<IUIDataStoreSubscriber*> Subscribers;

	// get the list of children owned by this scene
	TArray<UUIObject*> SceneChildren = GetChildren(TRUE);
	for ( INT ChildIndex = 0; ChildIndex < SceneChildren.Num(); ChildIndex++ )
	{
		UUIObject* Widget = SceneChildren(ChildIndex);

		IUIDataStoreSubscriber* SubscriberChild = (IUIDataStoreSubscriber*)Widget->GetInterfaceAddress(IUIDataStoreSubscriber::UClassType::StaticClass());
		if ( SubscriberChild != NULL )
		{
			Subscribers.AddItem(SubscriberChild);
		}

		// if this child implements the UIDataStoreSubscriber interface, tell the child to load the value from the data store
		IUIDataStorePublisher* PublisherChild = (IUIDataStorePublisher*)Widget->GetInterfaceAddress(IUIDataStorePublisher::UClassType::StaticClass());
		if ( PublisherChild != NULL )
		{
			PublisherChild->SaveSubscriberValue(SceneDataStores);
		}
	}

	// add our own data store to the list
	if ( SceneData != NULL )
	{
		SceneDataStores.AddUniqueItem(SceneData);
	}

	// iterate through the list of data stores that were bound to children of this scene, and notify each one that all subscribers have finished storing data
	for ( INT DataStoreIndex = 0; DataStoreIndex < SceneDataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = SceneDataStores(DataStoreIndex);
		DataStore->OnCommit();
	}

	// if we want to unbind all data stores, do that now
	if ( bUnbindSubscribers ==TRUE )
	{
		for ( INT SubscriberIndex = 0; SubscriberIndex < Subscribers.Num(); SubscriberIndex++ )
		{
			IUIDataStoreSubscriber* Subscriber = Subscribers(SubscriberIndex);
			Subscriber->ClearBoundDataStores();
		}
	}
}

/** 
 * Makes all the widgets in this scene unsubscribe from their bound datastores.
 */
void UUIScene::UnbindSubscribers()
{
	// Get a list of all of the subscribers in the scene.
	TArray<IUIDataStoreSubscriber*> Subscribers;

	TArray<UUIObject*> SceneChildren = GetChildren(TRUE);
	for ( INT ChildIndex = 0; ChildIndex < SceneChildren.Num(); ChildIndex++ )
	{
		UUIObject* Widget = SceneChildren(ChildIndex);

		IUIDataStoreSubscriber* SubscriberChild = (IUIDataStoreSubscriber*)Widget->GetInterfaceAddress(IUIDataStoreSubscriber::UClassType::StaticClass());
		if ( SubscriberChild != NULL )
		{
			Subscribers.AddItem(SubscriberChild);
		}
	}

	// Unsubscribe all subscribers.
	for ( INT SubscriberIndex = 0; SubscriberIndex < Subscribers.Num(); SubscriberIndex++ )
	{
		IUIDataStoreSubscriber* Subscriber = Subscribers(SubscriberIndex);
		Subscriber->ClearBoundDataStores();
	}
}

/**
 * Returns the scene that is below this one in the scene client's stack of active scenes.
 *
 * @param	bRequireMatchingPlayerOwner		TRUE indicates that only a scene that has the same value for PlayerOwner as this
 *											scene may be considered the "previous" scene to this one
 */
UUIScene* UUIScene::GetPreviousScene( UBOOL bRequireMatchingPlayerOwner/*=TRUE*/ )
{
	UUIScene* ParentScene = NULL;

	UGameUISceneClient* GameSceneClient = Cast<UGameUISceneClient>(SceneClient);
	if ( GameSceneClient != NULL )
	{
		INT CurrentSceneIndex = GameSceneClient->ActiveScenes.FindItemIndex(this);
		if ( CurrentSceneIndex == INDEX_NONE )
		{
			// CurrentSceneIndex is INDEX_NONE if this scene hasn't been fully initialized yet
			CurrentSceneIndex = GameSceneClient->ActiveScenes.Num();
		}

		for ( INT ParentSceneIndex = CurrentSceneIndex - 1; ParentSceneIndex >= 0; ParentSceneIndex-- )
		{
			UUIScene* PotentialParentScene = GameSceneClient->ActiveScenes(ParentSceneIndex);
			if ( PotentialParentScene != NULL && (!bRequireMatchingPlayerOwner || PotentialParentScene->PlayerOwner == PlayerOwner) )
			{
				ParentScene = PotentialParentScene;
				break;
			}
		}
	}

	return ParentScene;
}


/**
 * Returns the scene's default tooltip widget, creating one if necessary.
 */
UUIToolTip* UUIScene::GetDefaultToolTip()
{
	if ( StandardToolTip == NULL )
	{
		if ( DefaultToolTipClass == NULL )
		{
			debugf(NAME_Warning, TEXT("DefaultToolTipClass couldn't be loaded for %s.  Falling back to UIToolTip"), *GetFullName());
			DefaultToolTipClass = UUIToolTip::StaticClass();
		}

		StandardToolTip = Cast<UUIToolTip>(CreateWidget(this, DefaultToolTipClass, NULL, TEXT("DefaultToolTip")));

		StandardToolTip->InitializePlayerTracking();
		StandardToolTip->Initialize(this);
		StandardToolTip->eventPostInitialize();
	}

	return StandardToolTip;
}

/**
 * Returns the scene's currently active tool-tip, if there is one.
 */
UUIToolTip* UUIScene::GetActiveToolTip() const
{
	return ActiveToolTip;
}

/**
 * Changes the scene's ActiveToolTip to the one specified.
 */
UBOOL UUIScene::SetActiveToolTip( UUIToolTip* NewToolTip )
{
	ActiveToolTip = NewToolTip;
	return TRUE;
}


/**
 * Returns the scene's currently active context menu, if there is one.
 */
UUIContextMenu* UUIScene::GetActiveContextMenu() const
{
	return ActiveContextMenu;
}

/**
 * Changes the scene's ActiveContextMenu to the one specified.
 *
 * @param	NewContextMenu	the new context menu to activate, or NULL to clear any active context menus.
 * @param	PlayerIndex		the index of the player to display the context menu for.
 *
 * @return	TRUE if the scene's ActiveContextMenu was successfully changed to the new value.
 */
UBOOL UUIScene::SetActiveContextMenu( UUIContextMenu* NewContextMenu, INT PlayerIndex )
{
	UBOOL bResult = TRUE;

	// if we have an existing context menu, close it now
	if ( ActiveContextMenu != NULL && ActiveContextMenu != NewContextMenu )
	{
		UUIContextMenu* CurrentContextMenu = ActiveContextMenu;

		// first, set ActiveContextMenu as UUIContextMenu::Close() is call SetActiveContextMenu;
		ActiveContextMenu = NULL;
		if ( !CurrentContextMenu->Close(PlayerIndex) )
		{
			// if the existing context menu couldn't be closed for some reason, bail.
			ActiveContextMenu = CurrentContextMenu;
			bResult = FALSE;
		}
	}

	if ( bResult )
	{
		// clear any tooltips
		SetActiveToolTip(NULL);

		ActiveContextMenu = NewContextMenu;
		if ( ActiveContextMenu != NULL )
		{
			// clear the currently active control
			UGameUISceneClient* GameSceneClient = Cast<UGameUISceneClient>(SceneClient);
			if ( GameSceneClient != NULL && GameSceneClient->ActiveControl != NULL )
			{
				GameSceneClient->ActiveControl->DeactivateStateByClass(UUIState_Active::StaticClass(), PlayerIndex);
			}

			// make the context menu visible
			ActiveContextMenu->eventSetVisibility(TRUE);

			// make sure we're focused so that we can accept input properly.
			if ( ActiveContextMenu->SetFocus(NULL, PlayerIndex) || ActiveContextMenu->IsFocused(PlayerIndex) )
			{
				// now make the context menu the scene's active control
				ActiveContextMenu->ActivateStateByClass(UUIState_Active::StaticClass(), PlayerIndex);
			}
			else
			{
				// if the context menu couldn't accept focus, don't show it
				ActiveContextMenu->eventSetVisibility(FALSE);
				bResult = FALSE;

				// finally restore the previously active control
				if ( GameSceneClient != NULL )
				{
					GameSceneClient->UpdateActiveControl();
				}
			}
		}
	}

	return bResult;
}

/**
 * Returns the scene's default context menu widget, creating one if necessary.
 */
UUIContextMenu* UUIScene::GetDefaultContextMenu()
{
	if ( StandardContextMenu == NULL )
	{
		if ( DefaultContextMenuClass == NULL )
		{
			debugf(NAME_Warning, TEXT("DefaultContextMenuClass couldn't be loaded for %s.  Falling back to UIContextMenu"), *GetFullName());
			DefaultContextMenuClass = UUIContextMenu::StaticClass();
		}

		StandardContextMenu = Cast<UUIContextMenu>(CreateWidget(this, DefaultContextMenuClass, NULL, TEXT("SceneContextMenu")));
		StandardContextMenu->InitializePlayerTracking();
		StandardContextMenu->Initialize(this);
		StandardContextMenu->eventPostInitialize();
	}

	return StandardContextMenu;
}

/**
 * Wrapper for easily determining whether this scene is in the scene client's list of active scenes.
 */
UBOOL UUIScene::IsSceneActive( UBOOL bTopmostScene/*=FALSE*/ ) const
{
	UBOOL bResult = FALSE;

	//@fixme ronp - it would be better to add an accessor to UISceneClient, so that IsSceneActive returns TRUE when a scene is open in the editor as well
	UGameUISceneClient* GameSceneClient = GetSceneClient();
	if ( GameSceneClient != NULL )
	{
		bResult = GameSceneClient->ActiveScenes.ContainsItem(const_cast<UUIScene*>(this));
		if ( bResult && bTopmostScene )
		{
			bResult = GameSceneClient->ActiveScenes.Last() == this;
		}
	}
	else
	{
		// the editor's scene client class only has one scene to worry about, so if we have a valid reference to it,
		// we're active.
		bResult = SceneClient != NULL;
	}

	return bResult;
}

/** Called when this scene is about to be added to the active scenes array */
void UUIScene::Activate()
{
	// this is already done in UGameUISceneClient::InitializeScene, but it may be out of date if an action which was linked to the Initialized
	// event (which is activated before the scene is added to the list of active scenes) changes the number of local players
	InitializePlayerTracking();

	// generate the initial network of navigation links
	RebuildNavigationLinks();

	// load the data store bindings for all widgets of this scene
	LoadSceneDataValues();

	// notify unrealscript
	eventSceneActivated(TRUE);

	// attempt to find the index of the player that activated this menu to play the 'scene opened' sound cue
	const INT PlayerIndex = Max(0, GEngine->GamePlayers.FindItemIndex(PlayerOwner));
	PlayUISound(SceneOpenedCue,PlayerIndex);
}

/** Called just after this scene is removed from the active scenes array */
void UUIScene::Deactivate()
{

	// notify unrealscript
	eventSceneDeactivated();

	// attempt to find the index of the player that activated this menu to play the 'scene closed' sound cue
	const INT PlayerIndex = Max(0, GEngine->GamePlayers.FindItemIndex(PlayerOwner));
	PlayUISound(SceneClosedCue,PlayerIndex);

	// if the save flag is set then tell all children of this scene which are bound to data stores to publish their values
	// otherwise, just unsubscribe all children.
	if(bSaveSceneValuesOnClose)
	{
		SaveSceneDataValues(TRUE);
	}
	else
	{
		UnbindSubscribers();
	}

	if ( DELEGATE_IS_SET(OnSceneDeactivated) )
	{
		delegateOnSceneDeactivated(this);
	}
}

/**
 * Notification that this scene becomes the active scene.  Called after other activation methods have been called
 * and after focus has been set on the scene.
 *
 * @param	bInitialActivation		TRUE if this is the first time this scene is being activated; FALSE if this scene has become active
 *									as a result of closing another scene or manually moving this scene in the stack.
 */
void UUIScene::OnSceneActivated( UBOOL bInitialActivation )
{
	// notify unrealscript and fire kismet event if this isn't the initial scene activation (for initial scene activations, the SceneActivated event
	// is called from UUIScene::Activate)
	if ( bInitialActivation == FALSE )
	{
		eventSceneActivated(bInitialActivation);
	}

	if ( DELEGATE_IS_SET(OnSceneActivated) )
	{
		delegateOnSceneActivated(this, bInitialActivation);
	}
}

/**
 * This notification is sent to the topmost scene when a different scene is about to become the topmost scene.
 * Provides scenes with a single location to perform any cleanup for its children.
 *
 * @param	NewTopScene		the scene that is about to become the topmost scene.
 */
void UUIScene::NotifyTopSceneChanged( UUIScene* NewTopScene )
{
	// should only be called on the top-most scene, so add an assertion to detect if this assumption is ever changed.
	check(IsSceneActive(TRUE));
	check(NewTopScene);

	if ( DELEGATE_IS_SET(OnTopSceneChanged) )
	{
		delegateOnTopSceneChanged(NewTopScene);
	}

	// If a widget in the previous scene is still in the pressed state, deactivate that state now
	// this normally happens when the widget opens a new scene on a button press or double-click event
	TArray<UUIObject*> SceneChildren;
	GetChildren(SceneChildren, TRUE);

	// make sure that none of this scene's widgets are still in the pressed state
	const INT ActivePlayerCount = UUIInteraction::GetPlayerCount();
	for ( INT PlayerIndex = 0; PlayerIndex < ActivePlayerCount; PlayerIndex++ )
	{
		for ( INT ChildIndex = 0; ChildIndex < SceneChildren.Num(); ChildIndex++ )
		{
			if ( SceneChildren(ChildIndex)->IsPressed(PlayerIndex) )
			{
				SceneChildren(ChildIndex)->DeactivateStateByClass(UUIState_Pressed::StaticClass(), PlayerIndex);
			}
		}
	}
}

/**
 * Notifies the owning UIScene that the primitive usage in this scene has changed and sets flags in the scene to indicate that
 * 3D primitives have been added or removed.
 *
 * @param	bReinitializePrimitives		specify TRUE to have the scene detach all primitives and reinitialize the primitives for
 *										the widgets which have them.  Normally TRUE if we have ADDED a new child to the scene which
 *										supports primitives.
 * @param	bReviewPrimitiveUsage		specify TRUE to have the scene re-evaluate whether its bUsesPrimitives flag should be set.  Normally
 *										TRUE if a child which supports primitives has been REMOVED.
 */
void UUIScene::RequestPrimitiveReview( UBOOL bReinitializePrimitives, UBOOL bReviewPrimitiveUsage )
{
	if ( bReinitializePrimitives )
	{
		if ( SceneClient != NULL )
		{
			SceneClient->RequestPrimitiveReinitialization(this);
		}		
	}

	if ( bReviewPrimitiveUsage )
	{
		bUpdatePrimitiveUsage = TRUE;
	}
}

/**
 * Updates the value of bUsesPrimitives.
 */
void UUIScene::UpdatePrimitiveUsage()
{
	// clear the flag that gets checked in UpdateScene()
	bUpdatePrimitiveUsage = FALSE;

	bUsesPrimitives = FALSE;
	if ( bSupports3DPrimitives )
	{
		bUsesPrimitives = TRUE;
	}
	else
	{
		TArray<UUIObject*> SceneChildren;
		GetChildren(SceneChildren,TRUE);

		for ( INT ChildIndex = 0; ChildIndex < SceneChildren.Num(); ChildIndex++ )
		{
			if ( SceneChildren(ChildIndex)->bSupports3DPrimitives )
			{
				bUsesPrimitives = TRUE;
				break;
			}
		}
	}
}

/**
 * Called when the scene receives a notification that the viewport has been resized.  Propagated down to all children.
 *
 * @param	OldViewportSize		the previous size of the viewport
 * @param	NewViewportSize		the new size of the viewport
 */
void UUIScene::NotifyResolutionChanged( const FVector2D& OldViewportSize, const FVector2D& NewViewportSize )
{
/*
	// first, invalidate all position values in this scene so t
	Position.InvalidateAllFaces();
	TArray<UUIObject*> SceneChildren;
	GetChildren(SceneChildren, TRUE);
	for ( INT ChildIndex = 0; ChildIndex < SceneChildren.Num(); ChildIndex++ )
	{
		SceneChildren(ChildIndex)->Position.InvalidateAllFaces();
	}
*/
	Super::NotifyResolutionChanged(OldViewportSize, NewViewportSize);

	// request an update to occur in the next frame
	if ( bIssuedPreRenderCallback )
	{
		// only request a formatting update is this resolution change notification was NOT triggered from the scene's first update
		RequestFormattingUpdate();
	}

	RefreshPosition();
}

/**
 *	Actually update the scene by rebuilding docking and resolving positions.
 */
void UUIScene::UpdateScene()
{
	SCOPE_CYCLE_COUNTER(STAT_UISceneUpdateTime);

#ifdef DEBUG_UIPERF
	UBOOL bFirstFrame = !bIssuedPreRenderCallback;
	DOUBLE StartTime = appSeconds();
	DOUBLE LocalStartTime;
#endif

	if( bIssuedPreRenderCallback == FALSE )
	{
		SCOPE_CYCLE_COUNTER(STAT_UIPreRenderCallbackTime);
		PreRenderCallback();

#ifdef DEBUG_UIPERF
		debugf(TEXT("\t%s PreRenderCallback took (%5.2f ms)"), *SceneTag, (appSeconds() - StartTime) * 1000);
#endif
	}

	if ( bRefreshStringFormatting )
	{
		// propagate the call to all children
		SCOPE_CYCLE_COUNTER(STAT_UIRefreshFormatting);
		RefreshFormatting();
		bRefreshStringFormatting = FALSE;

		// now update scene positions
		bUpdateScenePositions = TRUE;
	}

	if( bRefreshWidgetStyles )
	{
		SCOPE_CYCLE_COUNTER(STAT_UIRefreshWidgetStyles);
#ifdef DEBUG_UIPERF
		LocalStartTime = appSeconds();
		bRefreshWidgetStyles = FALSE;
		RefreshWidgetStyles();
		if ( bFirstFrame )
		{
			debugf(TEXT("\t%s RefreshWidgetStyles took (%5.2f ms)"), *SceneTag, (appSeconds() - LocalStartTime) * 1000);
		}
#else
		bRefreshWidgetStyles = FALSE;
		RefreshWidgetStyles();
#endif
	}

	if ( bUpdateDockingStack )
	{
		SCOPE_CYCLE_COUNTER(STAT_UIRebuildDockingStack);
#ifdef DEBUG_UIPERF
		LocalStartTime = appSeconds();
		RebuildDockingStack();
		if ( bFirstFrame )
		{
			debugf(TEXT("\t%s RebuildDockingStack took (%5.2f ms)"), *SceneTag, (appSeconds() - LocalStartTime) * 1000);
		}
#else
		RebuildDockingStack();
#endif
	}

	if ( bUpdateScenePositions )
	{
		SCOPE_CYCLE_COUNTER(STAT_UIResolveScenePositions);
#ifdef DEBUG_UIPERF
		LocalStartTime = appSeconds();
		ResolveScenePositions();
		if ( bFirstFrame )
		{
			debugf(TEXT("\t%s ResolveScenePositions took (%5.2f ms)"), *SceneTag, (appSeconds() - LocalStartTime) * 1000);
		}
#else
		ResolveScenePositions();
#endif
	}

	if ( bUpdateNavigationLinks )
	{
		SCOPE_CYCLE_COUNTER(STAT_UIRebuildNavigationLinks);
#ifdef DEBUG_UIPERF
		LocalStartTime = appSeconds();
		bUpdateNavigationLinks = FALSE;
		RebuildNavigationLinks();
		if ( bFirstFrame )
		{
			debugf(TEXT("\t%s RebuildNavigationLinks took (%5.2f ms)"), *SceneTag, (appSeconds() - LocalStartTime) * 1000);
		}
#else
		bUpdateNavigationLinks = FALSE;
		RebuildNavigationLinks();
#endif
	}

	if ( bUpdatePrimitiveUsage )
	{
		UpdatePrimitiveUsage();
	}

#ifdef DEBUG_UIPERF
	if ( bFirstFrame )
	{
		debugf(TEXT("%s UpdateScene for took (%5.2f ms)"), *SceneTag, (appSeconds() - StartTime) * 1000);
	}
#endif
}

/**
 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
 *
 * This version simulates a resolution change to propagate position conversions for any widgets which are using aspect ratio locking.
 */
void UUIScene::PreRenderCallback()
{
	const FVector2D OldViewportSize = CurrentViewportSize;
	verify(SceneClient->GetViewportSize(this, CurrentViewportSize));

	// widgets will need to swap out the scene CurrentViewportSize to perform translations, so
	// make a copy and pass that instead of CurrentViewportSize, since the NewViewportSize parameter
	// is a const ref
	const FVector2D NewViewportSize = CurrentViewportSize;
	NotifyResolutionChanged(OldViewportSize, NewViewportSize);

	// now notify all children.
	Super::PreRenderCallback();

	bIssuedPreRenderCallback = TRUE;
}

/**
 * Called to globally update the formatting of all UIStrings.
 */
void UUIScene::RefreshFormatting()
{
	TArray<UUIObject*> AllSceneChildren = GetChildren(TRUE);
	for ( INT ChildIndex = 0; ChildIndex < AllSceneChildren.Num(); ChildIndex++ )
	{
		UUIObject* SceneChild = AllSceneChildren(ChildIndex);
		SceneChild->RefreshFormatting();
	}
}

/**
 * Called once per frame to update the scene's state.
 *
 * @param	DeltaTime	the time since the last Tick call
 */
void UUIScene::Tick( FLOAT DeltaTime )
{
	// Update the scene.
	UpdateScene();
}

/**
 * Updates the sequences for this scene and its child widgets.
 *
 * @param	DeltaTime	the time since the last call to TickSequence.
 */
void UUIScene::TickSequence( FLOAT DeltaTime )
{
	//@fixme ronp kismet - for the ui-kismet prototype, currenty only ticking the sequence associated with the scene
	// need to come up with a way for ticking the sequences in all child widgets that isn't horribly slow
	if ( GIsGame && EventProvider != NULL && EventProvider->EventContainer != NULL )
	{
		EventProvider->EventContainer->UpdateOp(DeltaTime);
	}
}

/**
 * Renders this scene and its children.
 *
 * @param	Canvas	the canvas to use for rendering the scene
 */
void UUIScene::Render_Scene( FCanvas* Canvas )
{
	//@todo ronp - make this fading a bit more robust (cumulative scene fading ala UC2)

	// first, clear the existing render stack; widgets will add themselves to the stack
	// as they are rendered
	RenderStack.Empty(GetObjectCount()-1);

	// store the current global alpha modulation
	const FLOAT OriginalAlphaModulation = Canvas->AlphaModulate;

	// apply the scene's opacity to the global alpha modulation
	Canvas->AlphaModulate *= Opacity;

	// render all child widgets in the scene
	Render_Children(Canvas);

	// now render all special overlays
	RenderSceneOverlays(Canvas);

	// restore the previous global alpha modulation
	Canvas->AlphaModulate = OriginalAlphaModulation;
}

/**
 * Renders all special overlays for this scene, such as context menus or tooltips.
 *
 * @param	Canvas	the canvas to use for rendering the overlays
 */
void UUIScene::RenderSceneOverlays( FCanvas* Canvas )
{
	if ( ActiveContextMenu != NULL && ActiveContextMenu->IsVisible() )
	{
		if ( ActiveContextMenu->bResolvePosition )
		{
			ActiveContextMenu->ResolveContextMenuPosition();
		}

		// add this widget to the scene's render stack
		RenderStack.Push(ActiveContextMenu);
		Render_Child(Canvas, ActiveContextMenu);
	}

	if ( ActiveToolTip != NULL && ActiveToolTip->IsVisible() )
	{
		if ( ActiveToolTip->bPendingPositionUpdate || ActiveToolTip->bFollowCursor )
		{
			ActiveToolTip->UpdateToolTipPosition();
		}

		// resolve the tool-tip's position, if necessary
		ActiveToolTip->ResolveToolTipPosition();

		Render_Child(Canvas, ActiveToolTip);
	}
}


/**
 * Updates all 3D primitives in this scene.
 *
 * @param	CanvasScene		the scene to use for attaching any 3D primitives
 */
void UUIScene::UpdateScenePrimitives( FCanvasScene* CanvasScene )
{
	UpdateChildPrimitives(CanvasScene);
}

/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUIScene::InvalidatePositionDependencies( BYTE Face )
{
	// we must also invalidate any position values (in this widget or any of our children) for which GetPosition(EVALPOS_PixelViewport)
	// would no longer return the same value as the corresponding RenderBounds

	switch( Face )
	{
	case UIFACE_Left:
	// if this is the left face 
		// and the right face's scale type is PixelOwner, it's no longer valid
		if ( Position.IsPositionCurrent(NULL, UIFACE_Right)
		&&	(Position.GetScaleType(UIFACE_Right) == EVALPOS_PixelOwner
		||	Position.GetScaleType(UIFACE_Right) == EVALPOS_PercentageScene
		||	Position.GetScaleType(UIFACE_Right) == EVALPOS_PixelScene) )
		{
			Position.InvalidatePosition(UIFACE_Right);
		}

		// now we check our children
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL )
			{
				FUIScreenValue_Bounds& ChildPosition = Child->Position;
				if ( ChildPosition.IsPositionCurrent(NULL, UIFACE_Left) )
				{
					switch ( ChildPosition.GetScaleType(UIFACE_Left) )
					{
					case EVALPOS_PixelScene:
					case EVALPOS_PixelOwner:
					case EVALPOS_PercentageOwner:
					case EVALPOS_PercentageScene:
						Child->InvalidatePosition(UIFACE_Left);
						break;
					}
				}

				// for any children which have a right scale type of PercentageOwner/Scene, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Right)
				&&	(ChildPosition.GetScaleType(UIFACE_Right) == EVALPOS_PercentageOwner || ChildPosition.GetScaleType(UIFACE_Right) == EVALPOS_PercentageScene) )
				{
					Child->InvalidatePosition(UIFACE_Right);
				}
			}
		}
		break;

	case UIFACE_Right:
	// if this is the right face
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL )
			{
				FUIScreenValue_Bounds& ChildPosition = Child->Position;
				// for any children which have a left/right scale type of PercentageOwner, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Left)
				&&	(ChildPosition.GetScaleType(UIFACE_Left) == EVALPOS_PercentageOwner || ChildPosition.GetScaleType(UIFACE_Left) == EVALPOS_PercentageScene) )
				{
					Child->InvalidatePosition(UIFACE_Left);
				}
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Right)
				&&	(ChildPosition.GetScaleType(UIFACE_Right) == EVALPOS_PercentageOwner || ChildPosition.GetScaleType(UIFACE_Right) == EVALPOS_PercentageScene) )
				{
					Child->InvalidatePosition(UIFACE_Right);
				}
			}
		}
		break;

	case UIFACE_Top:
	// if this is the top face
		// and the bottom face's scale type is PixelOwner or PercentageOwner, it's no longer valid
		if ( Position.IsPositionCurrent(NULL,UIFACE_Bottom)
		&&	(Position.GetScaleType(UIFACE_Bottom) == EVALPOS_PixelOwner || Position.GetScaleType(UIFACE_Bottom) == EVALPOS_PercentageOwner) )
		{
			Position.InvalidatePosition(UIFACE_Bottom);
		}

		// now we check our children
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL )
			{
				FUIScreenValue_Bounds& ChildPosition = Child->Position;

				// any children which have a top scale type of PixelOwner or PercentageOwner, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Top) )
				{
					switch ( ChildPosition.GetScaleType(UIFACE_Top) )
					{
					case EVALPOS_PixelScene:
					case EVALPOS_PixelOwner:
					case EVALPOS_PercentageOwner:
					case EVALPOS_PercentageScene:
						Child->InvalidatePosition(UIFACE_Top);
						break;
					}
				}

				// for any children which have a bottom scale type of PercentageOwner, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Bottom)
				&&	(ChildPosition.GetScaleType(UIFACE_Bottom) == EVALPOS_PercentageOwner || ChildPosition.GetScaleType(UIFACE_Bottom) == EVALPOS_PercentageScene) )
				{
					Child->InvalidatePosition(UIFACE_Bottom);
				}
			}
		}
	    break;

	case UIFACE_Bottom:
	// if this is the bottom face
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL )
			{
				FUIScreenValue_Bounds& ChildPosition = Child->Position;
				// for any children which have a top/bottom scale type of PercentageOwner, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Top)
				&&	(ChildPosition.GetScaleType(UIFACE_Top) == EVALPOS_PercentageOwner || ChildPosition.GetScaleType(UIFACE_Top) == EVALPOS_PercentageScene) )
				{
					Child->InvalidatePosition(UIFACE_Top);
				}
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Bottom)
				&&	(ChildPosition.GetScaleType(UIFACE_Bottom) == EVALPOS_PercentageOwner || ChildPosition.GetScaleType(UIFACE_Bottom) == EVALPOS_PercentageScene) )
				{
					Child->InvalidatePosition(UIFACE_Bottom);
				}
			}
		}
	    break;
	}
}

/**
 * Clears and rebuilds the complete DockingStack.  It is not necessary to manually rebuild the DockingStack when
 * simply adding or removing widgets from the scene, since those types of actions automatically updates the DockingStack.
 */
void UUIScene::RebuildDockingStack()
{
	INT WidgetCount = 0;
	for ( INT i = 0; i < Children.Num(); i++ )
	{
		WidgetCount += Children(i)->GetObjectCount();
	}

	DockingStack.Empty(WidgetCount * UIFACE_MAX);
	for ( INT i = 0; i < Children.Num(); i++ )
	{
		Children(i)->AddDockingLink(DockingStack);
	}

	bUpdateDockingStack = FALSE;
	RequestSceneUpdate(FALSE,TRUE);

	if ( GCallbackEvent != NULL )
	{
		GCallbackEvent->Send(CALLBACK_UIEditor_ModifiedRenderOrder,this);
	}
}

/**
 * Determines whether the current docking relationships between the widgets in this scene are valid.
 *
 * @return	TRUE if all docking nodes were added to the list.  FALSE if any recursive docking relationships were detected.
 */
UBOOL UUIScene::ValidateDockingStack() const
{
	TArray<FUIDockingNode> TestDockingStack;

	INT WidgetCount = 0;
	for ( INT i = 0; i < Children.Num(); i++ )
	{
		WidgetCount += Children(i)->GetObjectCount();
	}

	// preallocate enough space for all widget faces to avoid the multiple reallocs
	TestDockingStack.Empty(WidgetCount * UIFACE_MAX);

	UBOOL bResult = TRUE;
	for ( INT i = 0; i < Children.Num(); i++ )
	{
		if ( !Children(i)->AddDockingLink(TestDockingStack) )
		{
			bResult = FALSE;
			break;
		}
	}

	return bResult;

}

/**
 * Iterates through the scene's DockingStack, and evaluates the Position values for each widget owned by this scene
 * into actual pixel values, then stores that result in the widget's RenderBounds field.
 */
void UUIScene::ResolveScenePositions()
{
	bUpdateScenePositions = FALSE;

	// since we're invalidating the positions of all widgets, we should update the auto-navigation network in case it's out of date
	bUpdateNavigationLinks = TRUE;

	// clear the flag which indicates whether a face has been resolved for all widgets in the docking stack
	for ( INT StackIndex = 0; StackIndex < DockingStack.Num(); StackIndex++ )
	{
		FUIDockingNode& DockingNode = DockingStack(StackIndex);
		DockingNode.Widget->DockTargets.bResolved[DockingNode.Face] = 0;
		DockingNode.Widget->Position.InvalidatePosition(DockingNode.Face);
	}

	for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
	{
		Position.ValidatePosition(FaceIndex);
	}

	for ( INT StackIndex = 0; StackIndex < DockingStack.Num(); StackIndex++ )
	{
		FUIDockingNode& DockingNode = DockingStack(StackIndex);
		DockingNode.Widget->ResolveFacePosition( (EUIWidgetFace)DockingNode.Face );
	}

	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		Children(ChildIndex)->UpdateRenderBoundsVertices(TRUE);
	}

	if ( GCallbackEvent != NULL )
	{
		GCallbackEvent->Send(CALLBACK_UIEditor_RefreshScenePositions);
	}
}

/**
 *	Iterates over all widgets in the scene and forces them to update their style
 */
void UUIScene::RefreshWidgetStyles()
{
	TArray<UUIObject*> SceneChildren = GetChildren(TRUE);
	for ( INT i = 0; i < SceneChildren.Num(); i++ )
	{
		// Force each widget in the scene to update its style
		UUIObject* ChildWidget = SceneChildren(i);
		if ( ChildWidget != NULL )
		{
			// Refresh widget style by marking its style references dirty
			ChildWidget->ToggleStyleDirtiness(TRUE);
			ChildWidget->ResolveStyles(FALSE);
		}
	}
}

/**
 * Called immediately after a child has been removed from this screen object.
 *
 * @param	WidgetOwner		the screen object that the widget was removed from.
 * @param	OldChild		the widget that was removed
 * @param	ExclusionSet	used to indicate that multiple widgets are being removed in one batch; useful for preventing references
 *							between the widgets being removed from being severed.
 */
void UUIScene::NotifyRemovedChild( UUIScreenObject* WidgetOwner, UUIObject* OldChild, TArray<UUIObject*>* ExclusionSet/*=NULL*/ )
{
	Super::NotifyRemovedChild( WidgetOwner, OldChild, ExclusionSet );

	if ( OldChild == NULL )
		return;



	TArray<UUIObject*> SceneChildren;
	GetChildren(SceneChildren, TRUE, ExclusionSet);
	for ( INT ChildIndex = 0; ChildIndex < SceneChildren.Num(); ChildIndex++ )
	{
		UUIObject* Child = SceneChildren(ChildIndex);
		for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
		{
			// remove this child from any docking sets
			if ( Child->IsDockedTo(OldChild, FaceIndex) )
			{
				// clear the docking link between this widget and the widget that has been removed
				// SetDockTarget calls Modify, so no need to do that here
				Child->Modify();
				Child->SetDockParameters(FaceIndex, NULL, UIFACE_MAX, 0.f);
			}

			EUIWidgetFace Face = (EUIWidgetFace)FaceIndex;
			if ( Child->GetNavigationTarget(Face, NAVLINK_Manual) == OldChild )
			{
				// clear the value for this nav target, preserving the value of bNullOverride
				// SetForcedNavigationTarget calls Modify, so no need to do that here
				Child->Modify();
				Child->SetForcedNavigationTarget(Face, NULL, Child->NavigationTargets.bNullOverride[Face]);
			}

			// Also clear any references from the child being removed to other children in the scene, in case this
			// widget is going to be moved into another scene or something.
			if ( OldChild->IsDockedTo(Child, FaceIndex) )
			{
				// clear the docking link between this widget and the widget that has been removed
				// SetDockTarget calls Modify, so no need to do that here
				OldChild->Modify();
				OldChild->SetDockParameters(FaceIndex, NULL, UIFACE_MAX, 0.f);
			}

			if ( OldChild->GetNavigationTarget(Face, NAVLINK_Manual) == Child )
			{
				// clear the value for this nav target, preserving the value of bNullOverride
				// SetForcedNavigationTarget calls Modify, so no need to do that here
				OldChild->Modify();
				OldChild->SetForcedNavigationTarget(Face, NULL, OldChild->NavigationTargets.bNullOverride[Face]);
			}
		}
	}
}

/**
 * Whenever you change the SceneInputMode, tell the SceneClient that the stack has been modified
 *
 * @param	NewInputMode	The new mode for this scene.
 */
void UUIScene::SetSceneInputMode(BYTE NewInputMode)
{
	const UBOOL bRecalculateSceneInputMask = SceneInputMode != NewInputMode;

	SceneInputMode = NewInputMode;

	// if we really changed the scene's input mode, we'll need to tell it to recalculate its PlayerInputMask
	if ( bRecalculateSceneInputMask )
	{
		eventCalculateInputMask();
	}
}


/**
 * Adds the specified widget to the list of subscribers for the specified input key
 *
 * @param	InputKey	the key that the widget wants to subscribe to
 * @param	Handler		the widget to add to the list of subscribers
 * @param	PlayerIndex	the index of the player to register the input events for
 *
 * @return	TRUE if the widget was successfully added to the subscribers list, FALSE if couldn't be added or there is
 *			already a subscription for this widget
 */
UBOOL UUIScene::SubscribeInputEvent( FName InputKey, UUIScreenObject* Handler, INT PlayerIndex )
{
	check(Handler);

	UBOOL bResult = FALSE;

	TArray<FInputEventSubscription>* AllSubscribers = InputSubscriptions.Find(InputKey);
	if ( AllSubscribers == NULL )
	{
		AllSubscribers = &InputSubscriptions.Set(InputKey, TArray<FInputEventSubscription>());
		for ( INT GamepadIndex = 0; GamepadIndex < UCONST_MAX_SUPPORTED_GAMEPADS; GamepadIndex++ )
		{
			new(*AllSubscribers) FInputEventSubscription(InputKey);
		}
	}
	check(AllSubscribers);

	// get the indexes of the players that this control supports
	TArray<INT> PlayerIndexes;
	Handler->GetInputMaskPlayerIndexes(PlayerIndexes);
	if ( PlayerIndexes.ContainsItem(PlayerIndex) )
	{
		// check the input event subscription for each potential player, adding this widget to the list
		// of widgets that can respond to input for this player
		FInputEventSubscription& SubscribersList = (*AllSubscribers)(PlayerIndex);
		if ( !SubscribersList.Subscribers.ContainsItem(Handler) )
		{
			SubscribersList.Subscribers.InsertItem(Handler, 0);
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Removes the specified widget from the list of subscribers for the specified input key
 *
 * @param	InputKey	the key that the widget wants to unsubscribe for
 * @param	Handler		the widget to remove from the list of subscribers
 * @param	PlayerIndex	the index of the player to unregister the input events for
 *
 * @return	TRUE if the widget was successfully removed from the subscribers list
 */
UBOOL UUIScene::UnsubscribeInputEvent( FName InputKey, UUIScreenObject* Handler, INT PlayerIndex )
{
	check(Handler);

	UBOOL bResult = FALSE;

	TArray<FInputEventSubscription>* AllSubscribers = InputSubscriptions.Find(InputKey);
	if ( AllSubscribers != NULL )
	{
		// get the indexes of the players that this control supports
		TArray<INT> PlayerIndexes;
		Handler->GetInputMaskPlayerIndexes(PlayerIndexes);
		if ( PlayerIndexes.ContainsItem(PlayerIndex) )
		{
			// check the input event subscription for each potential player, removing this widget from the list
			// of widgets that can respond to input for this player
			FInputEventSubscription& SubscribersList = (*AllSubscribers)(PlayerIndex);

			INT SubscriberIndex = SubscribersList.Subscribers.FindItemIndex(Handler);
			if ( SubscriberIndex != INDEX_NONE )
			{
				SubscribersList.Subscribers.Remove(SubscriberIndex);
				bResult = TRUE;
			}
		}
	}
	else
	{
		debugf(TEXT("UUIScene::UnsubscribeInputEvent - no handlers for this input key.  Key '%s' Handler '%s'"), *InputKey.ToString(), *Handler->GetFullName());
	}

	return bResult;
}

/** 
 * Retrieve the list of input event subscribers for the specified input key and player index.
 *
 * @param	InputKey				the input key name to retrieve subscribers for
 * @param	PlayerIndex				the index for the player to retrieve subscribed controls for
 * @param	out_SubscribersList		filled with the controls that respond to the specified input key for the specified player
 *
 * @return	TRUE if an input subscription was found for the specified input key and player index, FALSE otherwise.
 */
UBOOL UUIScene::GetInputEventSubscribers( FName InputKey, INT PlayerIndex, FInputEventSubscription** out_SubscriberList )
{
	UBOOL bResult = FALSE;

	if ( out_SubscriberList != NULL )
	{
		TArray<FInputEventSubscription>* AllSubscribers = InputSubscriptions.Find(InputKey);
		if ( AllSubscribers != NULL )
		{
			// found an entry for this input key, so return value is TRUE.
			bResult = TRUE;

			checkSlow(PlayerIndex>=0);
			check(PlayerIndex<AllSubscribers->Num());
			*out_SubscriberList = &(*AllSubscribers)(PlayerIndex);
		}
	}

	return bResult;
}

/**
 * Wrapper function for converting the controller Id specified into a PlayerIndex and grabbing the scene's input mode.
 *
 * @param	ControllerId			the gamepad id of the player that generated the input event
 * @param	out_ScreenInputMode		set to this scene's input mode
 * @param	out_PlayerIndex			the Engine.GamePlayers index for the player with the gamepad id specified.
 *
 * @return	TRUE if this scene can process input for the gamepad id specified, or FALSE if this scene should ignore
 *			and swallow this input
 */
UBOOL UUIScene::PreprocessInput( INT ControllerId, EScreenInputMode& out_ScreenInputMode, INT& out_PlayerIndex )
{
	UBOOL bCanProcessInput = TRUE;

	// get the screen input mode for this scene
	out_ScreenInputMode = (EScreenInputMode)delegateGetSceneInputMode();

	// all of the UI system's input methods take a PlayerIndex, rather than a ControllerId,
	// so translate the ControllerId into a player index now.
	out_PlayerIndex = UUIInteraction::GetPlayerIndex(ControllerId);

	if ( out_ScreenInputMode == INPUTMODE_None )
	{
		bCanProcessInput = FALSE;
	}
	else
	{
		if ( out_PlayerIndex == INDEX_NONE )
		{
			// this indicates that this input is coming from a gamepad that is connected, but isn't associated
			// with a player.  We can only process input from these controllers if our input mode is INPUTMODE_Free
			if ( out_ScreenInputMode != INPUTMODE_Free )
			{
				bCanProcessInput = FALSE;
			}
			else
			{
				// set the player index to the first available slot
				out_PlayerIndex = UUIInteraction::GetPlayerCount();
			}
		}
		else
		{
			if ( !AcceptsPlayerInput(out_PlayerIndex) )
			{
				// this scene doesn't respond to this player's input
				bCanProcessInput = FALSE;
			}
		}
	}

	if ( bCanProcessInput == TRUE )
	{
		// store the index of the last player to send input to this scene
		LastPlayerIndex = out_PlayerIndex;
	}

	return bCanProcessInput;
}

/**
 * Allow this scene the chance to respond to an input event.
 *
 * @param	ControllerId	controllerId corresponding to the viewport that generated this event
 * @param	Key				name of the key which an event occured for.
 * @param	Event			the type of event which occured.
 * @param	AmountDepressed	(analog keys only) the depression percent.
 * @param	bGamepad - input came from gamepad (ie xbox controller)
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIScene::InputKey( INT ControllerId, FName Key, EInputEvent Event, FLOAT AmountDepressed/*=1.f*/, UBOOL bGamepad/*=FALSE*/ )
{
	UBOOL bResult = FALSE;

#if _DEBUG
	if ( Key != KEY_LeftControl && Key != KEY_RightControl
	&&	Key != KEY_LeftShift && Key != KEY_RightShift
	&&	Key != KEY_LeftAlt && Key != KEY_RightAlt )
	{
		int i = 0;
	}
#endif

	EScreenInputMode InputMode;
	INT PlayerIndex;
	if ( !PreprocessInput(ControllerId, InputMode, PlayerIndex) )
	{
		debugfSlow(NAME_Input, TEXT("ProcessInputKey (FALSE) Scene:%s  Key:%s Event:%s  ControllerId:%d    PlayerIndex:%i"), *SceneTag.ToString(), *Key.ToString(), *UUIRoot::GetInputEventText(Event), ControllerId, PlayerIndex);
		// swallow the input unless this scene doesn't respond to any input or it is configured to pass through input from players other than
		// this scene's player owner
		return InputMode != INPUTMODE_None && InputMode != INPUTMODE_MatchingOnly;
	}

	debugfSlow(NAME_Input, TEXT("ProcessInputKey (TRUE) Scene:%s  Key:%s Event:%s  ControllerId:%d    PlayerIndex:%i"), *SceneTag.ToString(), *Key.ToString(), *UUIRoot::GetInputEventText(Event), ControllerId, PlayerIndex);

	FInputEventSubscription* SubscribersList = NULL;
	if ( GetInputEventSubscribers(Key, PlayerIndex, &SubscribersList) )
	{
		if ( SubscribersList != NULL )
		{
			FInputEventParameters EventParms(
				PlayerIndex, ControllerId, Key, Event, IsHoldingAlt(ControllerId),
				IsHoldingCtrl(ControllerId), IsHoldingShift(ControllerId), AmountDepressed);

			for ( INT HandlerIndex = 0; HandlerIndex < SubscribersList->Subscribers.Num(); HandlerIndex++ )
			{
				UUIScreenObject* Handler = SubscribersList->Subscribers(HandlerIndex);
				checkSlow(Handler);
				if ( Handler->HandleInputKeyEvent(EventParms) )
				{
					// swallowed the input event
					bResult = TRUE;
					break;
				}
			}

			//@todo ronp - at this position, the tooltip will only be hidden when we receive input that is actually
			// handled by something in this scene.  should this code be moved to occur even if no widgets handle the input?

			// if we have an tooltip and it's configured to auto-hide when input is received, deactivate it now
			if ( ActiveToolTip != NULL && ActiveToolTip->bAutoHideOnInput && ActiveToolTip->delegateDeactivateToolTip() )
			{
				SetActiveToolTip(NULL);
			}
		}
	}

	// Allow the scene to handle it's own event.
	if ( !bResult && DELEGATE_IS_SET(OnRawInputKey) )
	{
		//@fixme ronp - shouldn't be calling the delegate here unless the scene is subscribed for this input key!!
		// but this requires all scenes to setup input key subscriptions for the keys they want to respond to
		FInputEventParameters EventParms(
			PlayerIndex, ControllerId, Key, Event, IsHoldingAlt(ControllerId),
			IsHoldingCtrl(ControllerId), IsHoldingShift(ControllerId), AmountDepressed);

		bResult = delegateOnRawInputKey(EventParms);
	}


	// the MatchingOnly input mode is the only mode which allows input to be processed by non-active scenes
	if ( InputMode != INPUTMODE_MatchingOnly )
	{
		// swallow all input events 
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Allow this scene the chance to respond to an input axis event (mouse movement)
 *
 * @param	ControllerId	controllerId corresponding to the viewport that generated this event
 * @param	Key				name of the key which an event occured for.
 * @param	Delta 			the axis movement delta.
 * @param	DeltaTime		seconds since the last axis update.
 *
 * @return	TRUE to consume the axis movement, FALSE to pass it on.
 */
UBOOL UUIScene::InputAxis( INT ControllerId, FName Key, FLOAT Delta, FLOAT DeltaTime, UBOOL bGamepad/*=FALSE*/ )
{
	UBOOL bResult = bDisplayCursor;

	EScreenInputMode InputMode;
	INT PlayerIndex;
	if ( !PreprocessInput(ControllerId, InputMode, PlayerIndex) )
	{
		// swallow the input unless this scene doesn't respond to any input or it is configured to pass through input from players other than
		// this scene's player owner
		return InputMode != INPUTMODE_None && InputMode != INPUTMODE_MatchingOnly;
	}

	FInputEventSubscription* SubscribersList = NULL;
	if ( GetInputEventSubscribers(Key, PlayerIndex, &SubscribersList) )
	{
		if ( SubscribersList != NULL )
		{
			FInputEventParameters EventParms(PlayerIndex, ControllerId, Key, Delta, DeltaTime, IsHoldingAlt(ControllerId),
				IsHoldingCtrl(ControllerId), IsHoldingShift(ControllerId));

			for ( INT HandlerIndex = 0; HandlerIndex < SubscribersList->Subscribers.Num(); HandlerIndex++ )
			{
				UUIScreenObject* Handler = SubscribersList->Subscribers(HandlerIndex);
				checkSlow(Handler);
				if ( Handler->HandleInputKeyEvent(EventParms) )
				{
					// swallowed the input event
					bResult = TRUE;
					break;
				}
			}
		}
	}


	if ( !bResult && DELEGATE_IS_SET(OnRawInputAxis) )
	{
		//@fixme ronp - shouldn't be calling the delegate here unless the scene is subscribed for this input key!!
		FInputEventParameters EventParms(PlayerIndex, ControllerId, Key, Delta, DeltaTime, IsHoldingAlt(ControllerId),
			IsHoldingCtrl(ControllerId), IsHoldingShift(ControllerId));

		bResult = delegateOnRawInputAxis(EventParms);
	}

	// the MatchingOnly input mode is the only mode which allows input to be processed by non-active scenes
	if ( InputMode != INPUTMODE_MatchingOnly )
	{
		// swallow all input events 
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Allow this scene to respond to an input char event.
 *
 * @param	ControllerId	controllerId corresponding to the viewport that generated this event
 * @param	Character		the character that was received
 *
 * @return	TRUE to consume the character, false to pass it on.
 */
UBOOL UUIScene::InputChar( INT ControllerId, TCHAR Character )
{
	UBOOL bResult = FALSE;

	EScreenInputMode InputMode;
	INT PlayerIndex;
	if ( !PreprocessInput(ControllerId, InputMode, PlayerIndex) )
	{
		// swallow the input unless this scene doesn't respond to any input or it is configured to pass through input from players other than
		// this scene's player owner
		return InputMode != INPUTMODE_None && InputMode != INPUTMODE_MatchingOnly;
	}

	FInputEventSubscription* SubscribersList = NULL;
	if ( GetInputEventSubscribers(KEY_Character, PlayerIndex, &SubscribersList) && SubscribersList != NULL )
	{
		if ( SubscribersList != NULL )
		{
			for ( INT HandlerIndex = 0; HandlerIndex < SubscribersList->Subscribers.Num(); HandlerIndex++ )
			{
				UUIScreenObject* Handler = SubscribersList->Subscribers(HandlerIndex);
				checkSlow(Handler);

				// todo: should this go through HandleInputEvent as well?
				if ( Handler->ProcessInputChar(PlayerIndex,Character) )
				{
					// swallowed the input event
					bResult = TRUE;
					break;
				}
			}
		}
	}

	// the MatchingOnly input mode is the only mode which allows input to be processed by non-active scenes
	if ( InputMode != INPUTMODE_MatchingOnly )
	{
		// swallow all input events 
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Processes key events for the scene itself.
 *
 * Only called if this scene is in the InputSubscribers map for the corresponding key.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIScene::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;

	if ( EventParms.EventType == IE_Released )
	{
		if ( EventParms.InputAliasName == UIKEY_CloseScene )
		{
			checkSlow(SceneClient);

			// close this scene
			if ( !SceneClient->CloseScene(this) )
			{
				debugf(TEXT("Unable to close scene '%s'"), *GetFullName());
			}

			bResult = TRUE;
		}
		else if ( EventParms.InputAliasName == UIKEY_Clicked )
		{
			// clicked background....for now, just close any active context menus
			SetActiveContextMenu(NULL, EventParms.PlayerIndex);
			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
}

/**
 * Find the data store that has the specified tag.  If the data store's tag is SCENE_DATASTORE_TAG, the scene's
 * data store is returned, otherwise searches through the global data stores for a data store with a Tag matching
 * the specified name.
 *
 * @param	DataStoreTag	A name corresponding to the 'Tag' property of a data store
 * @param	InPlayerOwner		The player owner to use for resolving the datastore.  If NULL, the scene's playerowner will be used instead.
 *
 * @return	a pointer to the data store that has a Tag corresponding to DataStoreTag, or NULL if no data
 *			were found with that tag.
 */
UUIDataStore* UUIScene::ResolveDataStore( FName DataStoreTag, ULocalPlayer* InPlayerOwner )
{
	UUIDataStore* Result = NULL;

	//@fixme - this should use the UCONST_SCENE_DATASTORE_TAG const
	if ( DataStoreTag == NAME_SceneData )
	{
		// use the scene data store
		Result = GetSceneDataStore();
	}
	else if ( SceneClient != NULL && SceneClient->DataStoreManager != NULL )
	{
		if(InPlayerOwner == NULL)
		{
			InPlayerOwner = PlayerOwner;		
		}

		Result = SceneClient->DataStoreManager->FindDataStore(DataStoreTag, InPlayerOwner);
	}

	return Result;
}

/**
 * Called after importing property values for this object (paste, duplicate or .t3d import)
 * Allow the object to perform any cleanup for properties which shouldn't be duplicated or
 * are unsupported by the script serialization
 *
 * Updates the scene's SceneTag to match the name of the scene.
 */
void UUIScene::PostEditImport()
{
	Super::PostEditImport();
	if ( SceneTag != GetFName() )
	{
		SceneTag = GetFName();
	}
}

/**
 * Called after this scene is renamed.
 *
 * Updates the scene's SceneTag to match the name of the scene.
 */
void UUIScene::PostRename()
{
	Super::PostRename();

	if ( SceneTag != GetFName() )
	{
		SceneTag = GetFName();
		MarkPackageDirty();

		if ( GCallbackEvent != NULL )
		{
			GCallbackEvent->Send(CALLBACK_UIEditor_SceneRenamed,this);
		}
	}
}

/**
 * Called after duplication & serialization and before PostLoad.
 *
 * Updates the scene's SceneTag to match the name of the scene.
 */
void UUIScene::PostDuplicate()
{
	Super::PostDuplicate();


	if ( SceneTag != GetFName() )
	{
		SceneTag = GetFName();
	}
}

/**
 * Presave function. Gets called once before an object gets serialized for saving. This function is necessary
 * for save time computation as Serialize gets called three times per object from within UObject::SavePackage.
 *
 * @warning: Objects created from within PreSave will NOT have PreSave called on them!!!
 *
 * This version determines determines which sequences in this scene contains sequence ops that are capable of executing logic,
 * and marks sequence objects with the RF_NotForClient|RF_NotForServer if the op isn't linked to anything.
 */
void UUIScene::PreSave()
{
	Super::PreSave();

	if ( EventProvider != NULL && EventProvider->EventContainer != NULL )
	{
		EventProvider->EventContainer->CalculateSequenceLoadFlags();
	}

	// this handles removal of widgets that have deprecated classes when this package is resaved via a commandlet
	if ( GIsUCC )
	{
		if ( GetClass()->HasAnyClassFlags(CLASS_Deprecated) )
		{
			ClearFlags(RF_Standalone);
		}
		else
		{
			TArray<UUIScreenObject*> DeprecatedObjects;
			FindDeprecatedWidgets(DeprecatedObjects);
			for ( INT ObjIndex = 0; ObjIndex < DeprecatedObjects.Num(); ObjIndex++ )
			{
				UUIScreenObject* DeprecatedObj = DeprecatedObjects(ObjIndex);

				// for now, we'll just remove the object.  another alternative would be to reparent
				// its children, but that becomes a lot more complex because some of those children
				// might be deprecated or might have the EditorNoReparent flag set
				
				UUIScreenObject* WidgetOwner = Cast<UUIScreenObject>(DeprecatedObj->GetOuter());
				if ( WidgetOwner != NULL )
				{
					WidgetOwner->RemoveChild(Cast<UUIObject>(DeprecatedObj));

					// set the RF_Transient flag in case there are redirectors referencing this object
					TArray<UObject*> Subobjects;
					FArchiveObjectReferenceCollector Collector(
						&Subobjects,
						DeprecatedObj,
						FALSE,
						TRUE,
						TRUE,
						FALSE
						);
					
					DeprecatedObj->Serialize(Collector);
					for ( INT SubobjectIndex = 0; SubobjectIndex < Subobjects.Num(); SubobjectIndex++ )
					{
						Subobjects(SubobjectIndex)->SetFlags(RF_Transient);
					}

					DeprecatedObj->SetFlags(RF_Transient);
				}
			}
		}
	}
}

/* ==========================================================================================================
	UUIObject
========================================================================================================== */

/** get the currently active skin */
UUISkin* UUIObject::GetActiveSkin() const
{
	check(OwnerScene);
	return OwnerScene->GetActiveSkin();
}


/**
 * Returns a string representation of this widget's hierarchy.
 * i.e. SomeScene.SomeContainer.SomeWidget
 */
FString UUIObject::GetWidgetPathName() const
{
	FString Result;
	UUIScreenObject* Parent = GetParent();
	if ( Parent != NULL )
	{
		Result = Parent->GetWidgetPathName() + TEXT(".");
	}

	Result += WidgetTag.ToString();

	return Result;
}

/**
 * Verifies that this widget has a valid WIDGET_ID, and generates one if it doesn't.
 */
void UUIObject::ValidateWidgetID()
{
	if ( !WidgetID.IsValid() && !IsTemplate() )
	{
		MarkPackageDirty();
		WidgetID = appCreateGuid();
	}
}

/**
 * @param Point	Point to check against the renderbounds of the object.
 * @return Whether or not this screen object contains the point passed in within its renderbounds.
 */
UBOOL UUIObject::ContainsPoint(const FVector2D& Point) const
{
	const FVector IntersectionPoint(PixelToCanvas(Point));
	return	IntersectionPoint.X >= RenderBounds[UIFACE_Left]
		&&	IntersectionPoint.X <= RenderBounds[UIFACE_Right]
		&&	IntersectionPoint.Y >= RenderBounds[UIFACE_Top]
		&&	IntersectionPoint.Y <= RenderBounds[UIFACE_Bottom];
}

/**
 * Render this widget.
 *
 * @param	Canvas	the canvas to use for rendering this widget
 */
void UUIObject::PostRender_Widget( FCanvas* Canvas )
{
#if !FINAL_RELEASE
	if ( bDebugShowBounds )
	{
		DrawBox2D( Canvas, FVector2D(RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top]), FVector2D(RenderBounds[UIFACE_Right], RenderBounds[UIFACE_Bottom]), DebugBoundsColor.ReinterpretAsLinear() );
	}
#endif
}

/**
 * Sets the location of the widget's rotation anchor, relative to the top-left of this widget's bounds.
 *
 * @param	AnchorPosition		New location for the widget's rotation anchor.
 * @param	InputType		indicates which format the AnchorPos value is in
 */
void UUIObject::SetAnchorPosition( FVector AnchorPosition, BYTE InputType/*=EVALPOS_PixelViewport*/ )
{
	Rotation.SetAnchorLocation(this,AnchorPosition,(EPositionEvalType)InputType);
	UpdateRotationMatrix();
}

/**
 * Rotates the widget around the current anchor position by the amount specified.
 *
 * @param	RotationDelta			amount to rotate the widget by in DEGREES.
 * @param	bAccumulateRotation		if FALSE, set the widget's rotation to NewRotationAmount; if TRUE, increments the
 *									widget's rotation by NewRotationAmount
 */
void UUIObject::RotateWidget( FRotator NewRotationAmount, UBOOL bAccumulateRotation/*=FALSE*/ )
{
	if ( bAccumulateRotation )
	{
		Rotation.Rotation += NewRotationAmount;
	}
	else
	{
		Rotation.Rotation = NewRotationAmount;
	}

	UpdateRotationMatrix();
}


/**
 * Updates the widget's rotation matrix based on the widget's current rotation.
 */
void UUIObject::UpdateRotationMatrix()
{
	FVector AnchorPos = GetAnchorPosition();

	Rotation.TransformMatrix =
		FTranslationMatrix(FVector(-AnchorPos.X,-AnchorPos.Y,Rotation.AnchorPosition.ZDepth)) *
		FRotationMatrix( Rotation.Rotation ) *
		FTranslationMatrix(AnchorPos);

	// if we haven't resolved any faces, it means we are pending a scene position update so we don't need to request another
	RefreshPosition();
}

/**
 * Returns the current location of the anchor.
 *
 * @param	bRelativeToWidget	specify TRUE to return the anchor position relative to the widget's upper left corner.
 *								specify FALSE to return the anchor position relative to the viewport's origin.
 * @param	bPixelSpace			specify TRUE to convert the anchor position into pixel space (only relevant if the widget is rotated)
 */
FVector UUIObject::GetAnchorPosition( UBOOL bRelativeToWidget/*=TRUE*/, UBOOL bPixelSpace/*=FALSE*/ ) const
{
	FVector AnchorPos(0.f,0.f,0.f);

	switch (Rotation.AnchorType)
	{
		case RA_Absolute:
			AnchorPos.X = Rotation.AnchorPosition.GetValue(UIORIENT_Horizontal, EVALPOS_PixelOwner, this);
			AnchorPos.Y = Rotation.AnchorPosition.GetValue(UIORIENT_Vertical, EVALPOS_PixelOwner, this);
			break;

		case RA_Center:
			AnchorPos.X = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelOwner) * 0.5f;
			AnchorPos.Y = GetBounds(UIORIENT_Vertical, EVALPOS_PixelOwner) * 0.5f;
			break;

		case RA_PivotLeft:
			AnchorPos.X = AnchorPos.Y = GetBounds(UIORIENT_Vertical, EVALPOS_PixelOwner) * 0.5f;
			break;

		case RA_PivotRight:
			AnchorPos.Y = GetBounds(UIORIENT_Vertical, EVALPOS_PixelOwner) * 0.5f;
			AnchorPos.X = GetPosition(UIFACE_Right, EVALPOS_PixelOwner, TRUE) - AnchorPos.Y;
			break;

		case RA_PivotTop:
			AnchorPos.X = AnchorPos.Y = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelOwner) * 0.5f;
			break;

		case RA_PivotBottom:
			AnchorPos.X = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelOwner) * 0.5f;
			AnchorPos.Y = GetPosition(UIFACE_Bottom, EVALPOS_PixelOwner, TRUE) - AnchorPos.X;
			break;

		case RA_UpperLeft:
			AnchorPos.X = 0;
			AnchorPos.Y = 0;
			break;

		case RA_UpperRight:
			AnchorPos.X = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelOwner);
			AnchorPos.Y = 0;
			break;

		case RA_LowerLeft:
			AnchorPos.X = 0;
			AnchorPos.Y = GetBounds(UIORIENT_Vertical, EVALPOS_PixelOwner);
			break;

		case RA_LowerRight:
			AnchorPos.X = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelOwner);
			AnchorPos.Y = GetBounds(UIORIENT_Vertical, EVALPOS_PixelOwner);
			break;
	}

	//@todo ronp - hmmm, should I add viewportOrigin in before or after projection?
	FVector2D ViewportOrigin;
	if ( !GetViewportOrigin(ViewportOrigin) )
	{
		ViewportOrigin.X = ViewportOrigin.Y = 0;
	}

	// if we want the absolute position, add in the widget's position.
	const FVector WidgetPosition(GetPositionVector(TRUE) + FVector(ViewportOrigin,0.f));
	if ( !bRelativeToWidget )
	{
		AnchorPos += WidgetPosition;
		if ( bPixelSpace )
		{
			AnchorPos = Project(AnchorPos);
			AnchorPos.Z = 0.f;
		}
	}
	else if ( bPixelSpace )
	{
 		AnchorPos = Project(AnchorPos + WidgetPosition) - Project(WidgetPosition);
		AnchorPos.Z = 0.f;
	}

	return AnchorPos;
}

/**
 * Generates a matrix which contains a translation for this widget's position (from 0,0 screen space) as well as the widget's
 * current rotation, scale, etc.
 *
 * @param	bIncludeParentTransforms	if TRUE, the matrix will be relative to the parent widget's own transform matrix.
 *
 * @return	a matrix containing the translation and rotation values of this widget.
 */
FMatrix UUIObject::GenerateTransformMatrix( UBOOL bIncludeParentTransforms/*=TRUE*/ ) const
{
	// get the current X and Y position of the widget
	FVector PositionVect = GetPositionVector(TRUE);

	// translate the matrix back to origin, apply the rotation matrix, then transform back to the current position
 	FMatrix Result = FTranslationMatrix(RenderOffset) * FTranslationMatrix(FVector(-PositionVect.X, -PositionVect.Y, ZDepth)) * Rotation.TransformMatrix * FTranslationMatrix(PositionVect);
	if ( bIncludeParentTransforms )
	{
		UUIObject* OwnerWidget = GetOwner();
		if ( OwnerWidget != NULL )
		{
			Result *= OwnerWidget->GenerateTransformMatrix(TRUE);
		}
	}
	return Result;
}

/**
 * Returns this widget's current rotation matrix
 *
 * @param	bIncludeParentRotations	if TRUE, the matrix will be relative to the parent widget's own rotation matrix.
 */
FMatrix UUIObject::GetRotationMatrix( UBOOL bIncludeParentRotations/*=TRUE*/ ) const
{
	return (bIncludeParentRotations && GetOwner() != NULL) ? GetOwner()->GetRotationMatrix(TRUE) * Rotation.TransformMatrix : Rotation.TransformMatrix;
}

/**
 * Called when this widget is created
 */
void UUIObject::Created( UUIScreenObject* Creator )
{
	ValidateWidgetID();

	// set widget tag now
	if ( WidgetTag == NAME_None )
	{
		WidgetTag = GetFName();
	}

	Super::Created(Creator);

	if ( DELEGATE_IS_SET(OnCreate) )
	{
		delegateOnCreate(this, Creator);
	}
}

/**
 * Called immediately after a child has been added to this screen object.
 *
 * @param	WidgetOwner		the screen object that the NewChild was added as a child for
 * @param	NewChild		the widget that was added
 */
void UUIObject::NotifyAddedChild( UUIScreenObject* WidgetOwner, UUIObject* NewChild )
{
	Super::NotifyAddedChild(WidgetOwner,NewChild);

	UUIScreenObject* Parent = GetParent();
	if ( Parent != NULL )
	{
		Parent->NotifyAddedChild(WidgetOwner, NewChild);
	}
}

/**
 * Called immediately after a child has been removed from this screen object.
 *
 * @param	WidgetOwner		the screen object that the widget was removed from.
 * @param	OldChild		the widget that was removed
 * @param	ExclusionSet	used to indicate that multiple widgets are being removed in one batch; useful for preventing references
 *							between the widgets being removed from being severed.
 */
void UUIObject::NotifyRemovedChild( UUIScreenObject* WidgetOwner, UUIObject* OldChild, TArray<UUIObject*>* ExclusionSet/*=NULL*/ )
{
	Super::NotifyRemovedChild(WidgetOwner, OldChild, ExclusionSet);

	UUIScreenObject* Parent = GetParent();
	if ( Parent != NULL )
	{
		Parent->NotifyRemovedChild(WidgetOwner, OldChild, ExclusionSet);
	}
}

/**
 * Notification that this widget's parent is about to remove this widget from its children array.  Allows the widget
 * to clean up any references to the old parent.
 *
 * @param	WidgetOwner		the screen object that this widget was removed from.
 * @param	ExclusionSet	allows the caller to specify a group of widgets which should not have inter-references (i.e. references
 *							to other objects in the set)
 */
void UUIObject::NotifyRemovedFromParent( UUIScreenObject* WidgetOwner, TArray<UUIObject*>* ExclusionSet/*=NULL*/ )
{
	// when we're removed from the scene, make sure to unsubscribe this widget (and any of its children) from
	// any data stores they've been bound to.
	TArray<IUIDataStoreSubscriber*> Subscribers;
	IUIDataStoreSubscriber* Subscriber = (IUIDataStoreSubscriber*)GetInterfaceAddress(IUIDataStoreSubscriber::UClassType::StaticClass());
	if ( Subscriber != NULL )
	{
		Subscribers.AddItem(Subscriber);
	}

	// send a "removed child" notification for each of our children so that other widgets in the scene can clear any
	// references to those children
	{
		TArray<UUIObject*> WidgetsBeingRemoved;

		// note that we intentionally do NOT pass in the ExclusionSet here, as we need to allow objects not in the exclusion
		// set to clear references to all objects in the exclusion set and vice versa (so we'll need to grab all our children)
		GetChildren(WidgetsBeingRemoved, TRUE);
		if ( ExclusionSet != NULL )
		{
			for ( INT ChildIndex = 0; ChildIndex < WidgetsBeingRemoved.Num(); ChildIndex++ )
			{
				ExclusionSet->AddUniqueItem(WidgetsBeingRemoved(ChildIndex));
			}
		}
		else
		{
			ExclusionSet = &WidgetsBeingRemoved;
		}
		ExclusionSet->AddUniqueItem(this);

		for ( INT ChildIndex = 0; ChildIndex < WidgetsBeingRemoved.Num(); ChildIndex++ )
		{
			UUIObject* Child = WidgetsBeingRemoved(ChildIndex);

			IUIDataStoreSubscriber* ChildSubscriber = (IUIDataStoreSubscriber*)Child->GetInterfaceAddress(IUIDataStoreSubscriber::UClassType::StaticClass());
			if ( ChildSubscriber != NULL )
			{
				Subscribers.AddItem(ChildSubscriber);
			}

			// make sure to clear the reference to the owner scene
			Child->Modify();
			Child->OwnerScene = NULL;
			NotifyRemovedChild(Child->GetOwner(), Child, ExclusionSet);
		}
	}

	eventRemovedFromParent(WidgetOwner);

	// if we want to unbind all data stores, do that now
	for ( INT SubscriberIndex = 0; SubscriberIndex < Subscribers.Num(); SubscriberIndex++ )
	{
		IUIDataStoreSubscriber* Subscriber = Subscribers(SubscriberIndex);
		Subscriber->ClearBoundDataStores();
	}

	// clear any references from WidgetOwner's sequence to this widget's sequence
	if ( EventProvider != NULL && (ExclusionSet == NULL || !ExclusionSet->ContainsItem(GetOwner())) )
	{
		EventProvider->CleanupEventProvider();
	}
}

/**
 * Called when the currently active skin has been changed.  Reapplies this widget's style and propagates
 * the notification to all children.
 */
void UUIObject::NotifyActiveSkinChanged()
{
	ResolveStyles(TRUE);

	Super::NotifyActiveSkinChanged();
}

/**
 * Called whenever the value of the UIObject is modified (for those UIObjects which can have values).
 * Calls the OnValueChanged delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to SetValue; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
 */
void UUIObject::NotifyValueChanged( INT PlayerIndex/*=INDEX_NONE*/, INT NotifyFlags/*=0*/ )
{
	if ( DELEGATE_IS_SET(OnValueChanged) )
	{
		if ( PlayerIndex == INDEX_NONE )
		{
			PlayerIndex = GetBestPlayerIndex();
		}

		delegateOnValueChanged(this, PlayerIndex);
	}
}

/**
 * Called when the scene receives a notification that the viewport has been resized.  Propagated down to all children.
 *
 * @param	OldViewportSize		the previous size of the viewport
 * @param	NewViewportSize		the new size of the viewport
 */
void UUIObject::NotifyResolutionChanged( const FVector2D& OldViewportSize, const FVector2D& NewViewportSize )
{
	// invalidate any faces which are set to be 
	for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
	{
		EPositionEvalType ScaleType = Position.GetScaleType((EUIWidgetFace)FaceIndex);
		if ( ScaleType >= EVALPOS_PercentageViewport && ScaleType < EVALPOS_MAX )
		{
			InvalidatePosition(FaceIndex);
		}
	}

	UUIScene* ParentScene = GetScene();

	// resolution changes can cause issues for widgets which are docked on their right/bottom faces but not docked on the opposing face and use percentage values
	// for the opposing face's scale type.  So when we get a resolution change notification, go through all of the scene's children and adjust the child's position
	// values such that when ResolveFacePosition is called for those faces, the logic there results in the correct final position for the widget
	EPositionEvalType FaceScaleType = Position.GetScaleType(UIFACE_Left);
	if ( DockTargets.IsWidthLocked() )
	{
		if ( FaceScaleType >= EVALPOS_PercentageViewport && FaceScaleType < EVALPOS_MAX )
		{
			if ( DockTargets.IsDocked(UIFACE_Right) && !DockTargets.IsDocked(UIFACE_Left) )
			{
				// determine the size of the widget under the old viewport size by looking at its RenderBounds
				ParentScene->CurrentViewportSize = OldViewportSize;
				const FLOAT PreviousWidth = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelOwner);
				const FLOAT RightPixelPos = Position.GetPositionValue(this, UIFACE_Right, EVALPOS_PixelViewport);

				// then change the value of the widget's dependent face Position so that when the code in ResolveFacePositions is executed,
				// this face's Position resolves into something that will result in the correct RenderBounds under the new resolution
				ParentScene->CurrentViewportSize = NewViewportSize;
				Position.SetPositionValue(this, RightPixelPos - PreviousWidth, UIFACE_Left, EVALPOS_PixelViewport);
			}
		}
	}

	if ( DockTargets.IsHeightLocked() )
	{
		FaceScaleType = Position.GetScaleType(UIFACE_Top);
		if ( FaceScaleType >= EVALPOS_PercentageViewport && FaceScaleType < EVALPOS_MAX )
		{
			if ( DockTargets.IsDocked(UIFACE_Bottom) && !DockTargets.IsDocked(UIFACE_Top) )
			{
				// determine the size of the widget under the old viewport size by looking at its RenderBounds
				ParentScene->CurrentViewportSize = OldViewportSize;
				const FLOAT PreviousHeight = GetBounds(UIORIENT_Vertical, EVALPOS_PixelOwner);
				const FLOAT BottomPixelPos = Position.GetPositionValue(this, UIFACE_Bottom, EVALPOS_PixelViewport);

				// then change the value of the widget's dependent face Position so that when the code in ResolveFacePositions is executed,
				// this face's Position resolves into something that will result in the correct RenderBounds under the new resolution
				ParentScene->CurrentViewportSize = NewViewportSize;
				Position.SetPositionValue(this, BottomPixelPos - PreviousHeight, UIFACE_Top, EVALPOS_PixelViewport);
			}
		}
	}

	// if the aspect ratio has changed, we may need to adjust the widget's width or height so that it scales uniformly
	EUIAspectRatioConstraint AspectRatioConstraintMode = Position.GetAspectRatioMode();
	if ( AspectRatioConstraintMode == UIASPECTRATIO_AdjustWidth )
	{
		FaceScaleType = Position.GetScaleType(UIFACE_Right);
		if ( FaceScaleType >= EVALPOS_PercentageViewport && FaceScaleType < EVALPOS_MAX )
		{
			ParentScene->CurrentViewportSize = OldViewportSize;
			const FLOAT PreviousWidth = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelOwner);
			FLOAT DesiredWidth = PreviousWidth / (OldViewportSize.Y / NewViewportSize.Y);

			ParentScene->CurrentViewportSize = NewViewportSize;
			Position.SetPositionValue(this, DesiredWidth, UIFACE_Right, EVALPOS_PixelOwner);
		}
	}
	else if ( AspectRatioConstraintMode == UIASPECTRATIO_AdjustHeight )
	{
		FaceScaleType = Position.GetScaleType(UIFACE_Bottom);
		if ( FaceScaleType >= EVALPOS_PercentageViewport && FaceScaleType < EVALPOS_MAX )
		{
			ParentScene->CurrentViewportSize = OldViewportSize;
			const FLOAT PreviousHeight = GetBounds(UIORIENT_Vertical, EVALPOS_PixelOwner);
			const FLOAT DesiredHeight = PreviousHeight / (OldViewportSize.X / NewViewportSize.X);

			ParentScene->CurrentViewportSize = NewViewportSize;
			Position.SetPositionValue(this, DesiredHeight, UIFACE_Bottom, EVALPOS_PixelOwner);
		}
	}

	UpdateRotationMatrix();
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		UUIObject* Child = Children(ChildIndex);
		if ( Child != NULL )
		{
			Child->NotifyResolutionChanged(OldViewportSize, NewViewportSize);
			if ( OBJ_DELEGATE_IS_SET(Child,NotifyResolutionChanged) )
			{
				Child->delegateNotifyResolutionChanged(OldViewportSize, NewViewportSize);
			}
		}
	}
}

/**
 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
 * once the scene has been completely initialized.
 * For widgets added at runtime, called after the widget has been inserted into its parent's
 * list of children.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUIObject::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	DockTargets.InitializeDockingSet(this);
	ValidateWidgetID();

	OwnerScene = inOwnerScene;
	Owner = inOwner;

	if ( !IsInitialized() )
	{
		// provide this widget with an opportunity to initialize the StyleSubscribers array before any other initialization.
		InitializeStyleSubscribers();

		CreateDefaultStates();
		Super::Initialize(inOwnerScene,inOwner);

		bInitialized = TRUE;
		if ( !IsInUIPrefab() && !IsA(UUIPrefab::StaticClass()) )
		{

			checkf(WidgetID.IsValid(), TEXT("Invalid UIObject ID during Initialize: '%s' (%s)"), *GetFullName(), *GetWidgetPathName());
			FString ContextMenuMarkupString = GenerateSceneDataStoreMarkup(TEXT("ContextMenuItems"));
			if ( ContextMenuData.MarkupString.Len() == 0
			||	!appStrncmp(*(ContextMenuData.MarkupString), *ContextMenuMarkupString, ContextMenuMarkupString.Left(ContextMenuMarkupString.InStr(TEXT("."),TRUE)).Len()) )
			{
				ContextMenuData.MarkupString = ContextMenuMarkupString;
			}
		}
		else
		{
			checkf(!GIsGame, TEXT("Attempted to use a prefab widget in a live scene: %s (%s)"), *GetFullName(), *GetWidgetPathName());
		}
	}

	// initialize all children
	for ( INT i = 0; i < Children.Num(); i++ )
	{
		UUIObject* Child = Children(i);
		Child->InitializePlayerTracking();
		if ( PlayerInputMask != 255 )
		{
			Child->eventSetInputMask(PlayerInputMask);
		}
		Child->Initialize(OwnerScene, this);
		Child->eventPostInitialize();
	}
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUIObject::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddUniqueItem(UIKEY_ShowContextMenu);
}

/**
 * Determines whether this widget should process the specified input event + state.  If the widget is configured
 * to respond to this combination of input key/state, any actions associated with this input event are activated.
 *
 * Only called if this widget is in the owning scene's InputSubscribers map for the corresponding key.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIObject::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	// ProcessInputKey should never be called with KEY_Character
	checkSlow(EventParms.InputKeyName != KEY_Character);

	// Translate the Unreal key to a UI Key Event.
	UBOOL bResult = FALSE;

	if( EventParms.InputAliasName == UIKEY_ShowContextMenu )
	{
		if ( EventParms.EventType == IE_Released )
		{
			UUIScene* SceneOwner = GetScene();
			if ( SceneOwner != NULL )
			{
				UUIContextMenu* ContextMenu = ActivateContextMenu(EventParms.PlayerIndex);
				if ( ContextMenu != NULL )
				{
					ContextMenu->InvokingWidget = this;

					// attempt to change the active context menu; note this might not work if a currently
					// active context menu cannot be closed because the OnCloseContextMenu delegate returned FALSE or something.
					if ( ContextMenu->Open(EventParms.PlayerIndex) )
					{
						// other stuff
					}
					else
					{
						ContextMenu->InvokingWidget = NULL;
					}

					// either way, we'll swallow the input event because there's no reason to allow other handlers to attempt
					// to activate a context menu at this point.
					bResult = TRUE;
				}
				else
				{
					// this widget didn't want to display a context menu - allow the next handler the chance to display a context menu
				}
			}
		}

		//@todo ronp - do we want to always capture press/repeat events?
		//bResult = TRUE;
	}

	return bResult || Super::ProcessInputKey(EventParms);
}

/**
 * Tell the scene that it needs to be udpated
 *
 * @param	bDockingStackChanged	if TRUE, the scene will rebuild its DockingStack at the beginning
 *									the next frame
 * @param	bPositionsChanged		if TRUE, the scene will update the positions for all its widgets
 *									at the beginning of the next frame
 * @param	bNavLinksOutdated		if TRUE, the scene will update the navigation links for all widgets
 *									at the beginning of the next frame
 * @param	bWidgetStylesChanged			if TRUE, the scene will refresh the widgets reapplying their current styles
 */
void UUIObject::RequestSceneUpdate( UBOOL bDockingStackChanged, UBOOL bPositionsChanged, UBOOL bNavLinksOutdated/*=FALSE*/,
								UBOOL bWidgetStylesChanged/*=FALSE*/ )
{
	UUIScene* Scene = GetScene();
	if ( Scene != NULL )
	{
		Scene->RequestSceneUpdate(bDockingStackChanged, bPositionsChanged, bNavLinksOutdated, bWidgetStylesChanged);
	}
}

/**
 * Tells the scene that it should call RefreshFormatting on the next tick.
 */
void UUIObject::RequestFormattingUpdate()
{
	UUIScene* Scene = GetScene();
	if ( Scene != NULL )
	{
		Scene->RequestFormattingUpdate();
	}
}

/**
 * Notifies the owning UIScene that the primitive usage in this scene has changed and sets flags in the scene to indicate that
 * 3D primitives have been added or removed.
 *
 * @param	bReinitializePrimitives		specify TRUE to have the scene detach all primitives and reinitialize the primitives for
 *										the widgets which have them.  Normally TRUE if we have ADDED a new child to the scene which
 *										supports primitives.
 * @param	bReviewPrimitiveUsage		specify TRUE to have the scene re-evaluate whether its bUsesPrimitives flag should be set.  Normally
 *										TRUE if a child which supports primitives has been REMOVED.
 */
void UUIObject::RequestPrimitiveReview( UBOOL bReinitializePrimitives, UBOOL bReviewPrimitiveUsage )
{
	UUIScene* Scene = GetScene();
	if ( Scene != NULL )
	{
		Scene->RequestPrimitiveReview(bReinitializePrimitives, bReviewPrimitiveUsage);
	}
}

/**
 *	Actually update the scene by rebuilding docking and resolving positions.
 */
void UUIObject::UpdateScene()
{
	UUIScene* Scene = GetScene();
	if ( Scene != NULL )
	{
		Scene->UpdateScene();
	}
}

/**
 * Returns TRUE if this widget is docked to the specified widget.
 *
 * @param	TargetWidget	the widget to check for docking links to
 * @param	SourceFace		if specified, returns TRUE only if the specified face is docked to TargetWidget
 * @param	TargetFace		if specified, returns TRUE only if this widget is docked to the specified face on the target widget.
 */
UBOOL UUIObject::IsDockedTo( const UUIScreenObject* TargetWidget, BYTE SourceFace/*=UIFACE_MAX*/, BYTE TargetFace/*=UIFACE_MAX*/ ) const
{
	UBOOL bResult = FALSE;

	if ( TargetWidget != NULL )
	{
		if ( TargetWidget == GetScene() )
		{
			// Since GetDockTarget returns NULL if we're docked to the scene, clear the value of TargetWidget if we're searching for links to the scene
			// so that we only have to check GetDockTarget() == TargetWidget && IsDocked
			TargetWidget = NULL;
		}

		if ( SourceFace < UIFACE_MAX )
		{
			if ( DockTargets.IsDocked((EUIWidgetFace)SourceFace) )
			{
				if ( TargetFace >= UIFACE_MAX || DockTargets.GetDockFace((EUIWidgetFace)SourceFace) == TargetFace )
				{
					bResult = (TargetWidget == DockTargets.GetDockTarget((EUIWidgetFace)SourceFace));
				}
			}
		}
		else
		{
			for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
			{
				if ( DockTargets.IsDocked((EUIWidgetFace)FaceIndex) )
				{
					if ( TargetWidget == DockTargets.GetDockTarget((EUIWidgetFace)FaceIndex) )
					{
						bResult = TRUE;
						break;
					}
				}
			}
		}
	}

	return bResult;
}

/**
 * Sets the docking parameters for the specified face.
 *
 * @param	SourceFace	the face of this widget to apply the changes to
 * @param	Target		the widget to dock to
 * @param	TargetFace	the face on the Target widget that SourceFace will dock to
 *
 * @return	TRUE if the changes were successfully applied.
 */
UBOOL UUIObject::SetDockTarget( BYTE SourceFace, UUIScreenObject* Target, BYTE TargetFace )
{
	UBOOL bResult = FALSE;
	checkSlow(SourceFace<UIFACE_MAX);

	if( IsTransformable() )
	{
		if ( Target == this )
		{
			debugf(NAME_Warning, TEXT("Cannot dock a widget to itself: %s"), *GetTag().ToString());
		}
		else 
		{
			UUIObject* TargetWidget = Cast<UUIObject>(Target);
			if ( TargetWidget != NULL && TargetFace < UIFACE_MAX && TargetWidget->DockTargets.GetDockTarget((EUIWidgetFace)TargetFace) == this &&
				TargetWidget->DockTargets.GetDockFace((EUIWidgetFace)TargetFace) == SourceFace )
			{
				debugf(NAME_Warning, TEXT("Cannot create recursive docking relationship! Source: %s (%s)  Target: %s (%s)"), *GetTag().ToString(), *GetDockFaceText(SourceFace), *TargetWidget->GetTag().ToString(), *GetDockFaceText(TargetFace));
			}
			else
			{
				Modify();

				if ( DockTargets.SetDockTarget((EUIWidgetFace)SourceFace, Target, (EUIWidgetFace)TargetFace) )
				{
					RequestSceneUpdate(TRUE,TRUE);
					bResult=TRUE;
				}
			}
		}
	}

	return bResult;
}

/**
 * Sets the padding for the specified docking link.
 *
 * @param	SourceFace	the face of this widget to apply the changes to
 * @param	Padding		the amount of padding to use for this docking set.  Positive values will "push" this widget past the
 *						target face of the other widget, while negative values will "pull" this widget away from the target widget.
 * @param	PaddingInputType
 *						specifies how the Padding value should be interpreted.
 * @param	bModifyPaddingScaleType
 *						specify TRUE to change the DockPadding's ScaleType to the PaddingInputType.
 *
 * @return	TRUE if the changes were successfully applied.
 */
UBOOL UUIObject::SetDockPadding( BYTE SourceFace, FLOAT Padding, BYTE PaddingInputType/*=UIPADDINGEVAL_Pixels*/, UBOOL bModifyPaddingScaleType/*=FALSE*/ )
{
	UBOOL bResult = FALSE;
	checkSlow(SourceFace<UIFACE_MAX);

	if( IsTransformable() )
	{
		Modify();

		if ( DockTargets.SetDockPadding((EUIWidgetFace)SourceFace, Padding, static_cast<EUIDockPaddingEvalType>(PaddingInputType), bModifyPaddingScaleType) )
		{
			RequestSceneUpdate(TRUE,TRUE);
			bResult=TRUE;
		}
	}

	return bResult;
}

/**
 * Combines SetDockTarget and SetDockPadding into a single function.
 *
 * @param	SourceFace	the face of this widget to apply the changes to
 * @param	Target		the widget to dock to
 * @param	TargetFace	the face on the Target widget that SourceFace will dock to
 * @param	Padding		the amount of padding to use for this docking set.  Positive values will "push" this widget past the
 *						target face of the other widget, while negative values will "pull" this widget away from the target widget.
 * @param	PaddingInputType
 *						specifies how the Padding value should be interpreted.
 * @param	bModifyPaddingScaleType
 *						specify TRUE to change the DockPadding's ScaleType to the PaddingInputType.
 *
 * @return	TRUE if the changes were successfully applied.
 */
UBOOL UUIObject::SetDockParameters( BYTE SourceFace, UUIScreenObject* Target, BYTE TargetFace, FLOAT Padding, BYTE PaddingInputType/*=UIPADDINGEVAL_Pixels*/, UBOOL bModifyPaddingScaleType/*=FALSE*/ )
{
	UBOOL bResult = SetDockTarget(SourceFace, Target, TargetFace) && SetDockPadding(SourceFace, Padding, PaddingInputType, bModifyPaddingScaleType);
	return bResult;
}

/**
 * Adds docking nodes for all faces of this widget to the specified scene
 *
 * @param	DockingStack	the docking stack to add this widget's docking.  Generally the scene's DockingStack.
 *
 * @return	TRUE if docking nodes were successfully added for all faces of this widget.
 */
UBOOL UUIObject::AddDockingLink( TArray<FUIDockingNode>& DockingStack )
{
	UBOOL bResult = TRUE;

	// first add a docking node for each face of this widget
	for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
	{
		if ( !AddDockingNode(DockingStack, (EUIWidgetFace)FaceIndex) )
		{
			bResult = FALSE;
		}

		DockTargets.bResolved[FaceIndex] = FALSE;
	}

	// then recurse into this widget's children
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		if ( Children(ChildIndex)->AddDockingLink(DockingStack) )
		{
			bResult = FALSE;
		}
	}

	return bResult;
}

/**
 * Adds the specified face to the DockingStack for the specified widget
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUIObject::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	//SCOPE_CYCLE_COUNTER(STAT_UIAddDockingNode);
	checkSlow(Face<UIFACE_MAX);

	UBOOL bResult = TRUE;
	FUIDockingNode NewNode(this,Face);

	// determine if this node already exists in the docking stack
	INT StackPosition = DockingStack.FindItemIndex(NewNode);

	// if this node doesn't already exist
	if ( StackPosition == INDEX_NONE )
	{
		if ( DockTargets.bLinking[Face] != 0 )
		{
			if ( GIsGame )
			{
				UUIScreenObject* DockTarget = DockTargets.GetDockTarget(Face);
				if ( DockTarget == NULL && DockTargets.IsDocked(Face) )
				{
					DockTarget = GetScene();
				}
				debugf(TEXT("Circular docking relationship detected!  Widget:%s    Face:%s    Target:%s    TargetFace:%s"),
					*GetWidgetPathName(),
					*UUIRoot::GetDockFaceText(Face),
					DockTarget ? *DockTarget->GetWidgetPathName() : TEXT("NULL"),
					*UUIRoot::GetDockFaceText(DockTargets.GetDockFace(Face))
					);

				GetScene()->eventLogDockingStack();
			}
			return FALSE;
		}

		DockTargets.bLinking[Face] = 1;

		// if this face isn't docked to another widget
		UUIObject* DockTarget = DockTargets.GetDockTarget(Face);
		if ( DockTarget == NULL )
		{
			// if this face isn't docked to the scene
			if ( !DockTargets.IsDocked(Face) )
			{
				// add any intrinsic dependencies for this face
				if ( Face == UIFACE_Right )
				{
					// if this is the right face, the left face needs to be evaluated first
					bResult = AddDockingNode(DockingStack, UIFACE_Left) && bResult;
				}
				else if ( Face == UIFACE_Bottom )
				{
					// if this is the bottom face, the top face needs to be evaluated first
					bResult = AddDockingNode(DockingStack, UIFACE_Top) && bResult;
				}
				else
				{
					EUIWidgetFace OppositeFace = UUIRoot::GetOppositeFace(Face);
					if ( DockTargets.IsDocked(OppositeFace) )
					{
						bResult = AddDockingNode(DockingStack, OppositeFace) && bResult;
					}
				}
			}
		}
		else
		{
			// if there is a TargetWidget for this face, that widget's face must be evaluated
			// before this one, so that face must go into the DockingStack first
			bResult = DockTarget->AddDockingNode(DockingStack, DockTargets.GetDockFace(Face)) && bResult;
		}

		check(NewNode.Face != UIFACE_MAX);

		// add this face to the DockingStack
		new(DockingStack) FUIDockingNode(NewNode);

		DockTargets.bLinking[Face] = 0;
	}

	return bResult;
}

/**
 * Evaluates the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUIObject::ResolveFacePosition( EUIWidgetFace Face )
{
	DockTargets.UpdateDockingSet(Face);
}

/**
 * Returns the number of faces this widget has resolved (happens in UUIObject::ResolveFacePosition).
 */
INT UUIObject::GetNumResolvedFaces() const
{
	INT Result = 0;

	for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
	{
		if ( DockTargets.bResolved[FaceIndex] != 0 )
		{
			Result++;
		}
	}

	return Result;
}

/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUIObject::InvalidatePositionDependencies( BYTE Face )
{
	// we must also invalidate any position values (in this widget or any of our children) for which GetPosition(EVALPOS_PixelViewport)
	// would no longer return the same value as the corresponding RenderBounds

	switch( Face )
	{
	case UIFACE_Left:
	// if this is the left face 
		// and the right face's scale type is PixelOwner, it's no longer valid
		if ( Position.IsPositionCurrent(NULL,UIFACE_Right)
		&&	(Position.GetScaleType(UIFACE_Right) == EVALPOS_PixelOwner || Position.GetScaleType(UIFACE_Right) == EVALPOS_PercentageOwner) )
		{
			Position.InvalidatePosition(UIFACE_Right);
		}

		// now we check our children
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL )
			{
				FUIScreenValue_Bounds& ChildPosition = Child->Position;
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Left) )
				{
					// any children which have a left scale type of PixelOwner or PercentageOwner, invalidate
					if ( ChildPosition.GetScaleType(UIFACE_Left) == EVALPOS_PixelOwner || ChildPosition.GetScaleType(UIFACE_Left) == EVALPOS_PercentageOwner )
					{
						Child->InvalidatePosition(UIFACE_Left);
					}
				}

				// for any children which have a right scale type of PercentageOwner, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Right) && ChildPosition.GetScaleType(UIFACE_Right) == EVALPOS_PercentageOwner )
				{
					Child->InvalidatePosition(UIFACE_Right);
				}
			}
		}
		break;

	case UIFACE_Right:
		if ( DockTargets.IsWidthLocked() && Position.IsPositionCurrent(NULL, UIFACE_Left) )
		{
			Position.InvalidatePosition(UIFACE_Left);
		}

	// if this is the right face
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL )
			{
				FUIScreenValue_Bounds& ChildPosition = Child->Position;
				// for any children which have a left/right scale type of PercentageOwner, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Left) && ChildPosition.GetScaleType(UIFACE_Left) == EVALPOS_PercentageOwner )
				{
					Child->InvalidatePosition(UIFACE_Left);
				}
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Right) && ChildPosition.GetScaleType(UIFACE_Right) == EVALPOS_PercentageOwner )
				{
					Child->InvalidatePosition(UIFACE_Right);
				}
			}
		}
		break;

	case UIFACE_Top:
	// if this is the top face
		// and the bottom face's scale type is PixelOwner or PercentageOwner, it's no longer valid
		if ( Position.IsPositionCurrent(NULL,UIFACE_Bottom)
		&&	(Position.GetScaleType(UIFACE_Bottom) == EVALPOS_PixelOwner || Position.GetScaleType(UIFACE_Bottom) == EVALPOS_PercentageOwner) )
		{
			Position.InvalidatePosition(UIFACE_Bottom);
		}

		// now we check our children
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL )
			{
				FUIScreenValue_Bounds& ChildPosition = Child->Position;

				// any children which have a top scale type of PixelOwner or PercentageOwner, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Top)
				&&	(ChildPosition.GetScaleType(UIFACE_Top) == EVALPOS_PixelOwner || ChildPosition.GetScaleType(UIFACE_Top) == EVALPOS_PercentageOwner) )
				{
					Child->InvalidatePosition(UIFACE_Top);
				}

				// for any children which have a bottom scale type of PercentageOwner, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Bottom) && ChildPosition.GetScaleType(UIFACE_Bottom) == EVALPOS_PercentageOwner )
				{
					Child->InvalidatePosition(UIFACE_Bottom);
				}
			}
		}
	    break;

	case UIFACE_Bottom:
	// if this is the bottom face

		if ( DockTargets.IsHeightLocked() && Position.IsPositionCurrent(NULL, UIFACE_Top) )
		{
			Position.InvalidatePosition(UIFACE_Top);
		}

		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			UUIObject* Child = Children(ChildIndex);
			if ( Child != NULL )
			{
				FUIScreenValue_Bounds& ChildPosition = Child->Position;
				// for any children which have a top/bottom scale type of PercentageOwner, invalidate
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Top) && ChildPosition.GetScaleType(UIFACE_Top) == EVALPOS_PercentageOwner )
				{
					Child->InvalidatePosition(UIFACE_Top);
				}
				if ( ChildPosition.IsPositionCurrent(NULL,UIFACE_Bottom) && ChildPosition.GetScaleType(UIFACE_Bottom) == EVALPOS_PercentageOwner )
				{
					Child->InvalidatePosition(UIFACE_Bottom);
				}
			}
		}
	    break;
	}

		

}

/**
 * Returns TRUE if TestWidget is in this widget's Owner chain.
 */
UBOOL UUIObject::IsContainedBy( UUIObject* TestWidget )
{
	UBOOL bResult = FALSE;
	if ( TestWidget != NULL )
	{
		for ( UUIObject* Container = Owner; Container; Container = Container->Owner )
		{
			if ( Container == TestWidget )
			{
				bResult = TRUE;
				break;
			}
		}
	}

	return bResult;
}

/**
 * Adds the specified StyleResolver to the list of StyleSubscribers
 *
 * @param	Subscriber			the UIStyleResolver to add.
 */
void UUIObject::AddStyleSubscriber( const TScriptInterface<IUIStyleResolver>& Subscriber )
{
	if ( Subscriber )
	{
		INT SubscriberIndex = FindStyleSubscriberIndex(Subscriber);
		if ( SubscriberIndex == INDEX_NONE )
		{
			StyleSubscribers.AddItem(Subscriber);
		}
	}
}

/**
 * Removes the specified StyleResolver from the list of StyleSubscribers.
 *
 * @param	Subscriber		the subscriber to remove
 */
void UUIObject::RemoveStyleSubscriber( const TScriptInterface<IUIStyleResolver>& Subscriber )
{
	StyleSubscribers.RemoveItem(Subscriber);
}

/**
 * Returns the index [into the StyleSubscriber's array] for the specified UIStyleResolver, or INDEX_NONE if Subscriber
 * is NULL or is not found in the StyleSubscriber's array.
 *
 * @param	Subscriber		the subscriber to find
 * @param	SubscriberId	if specified, it will only be considered a match if the SubscriberId associated with Subscriber matches this value.
 */
INT UUIObject::FindStyleSubscriberIndex( const TScriptInterface<IUIStyleResolver>& Subscriber )
{
	return StyleSubscribers.FindItemIndex(Subscriber);
}

/**
 * Returns the index [into the StyleSubscriber's array] for the subscriber which has a StyleResolverTag that matches the specified value
 * or INDEX_NONE if StyleSubscriberId is None or is not found in the StyleSubscriber's array.
 *
 * @param	StyleSubscriberId	the tag associated with the UIStyleResolver to find
 */
INT UUIObject::FindStyleSubscriberIndexById( FName StyleSubscriberId )
{
	INT Result = INDEX_NONE;

	for ( INT SubscriberIndex = 0; SubscriberIndex < StyleSubscribers.Num(); SubscriberIndex++ )
	{
		TScriptInterface<IUIStyleResolver>& Subscriber = StyleSubscribers(SubscriberIndex);
		if ( StyleSubscriberId == Subscriber->GetStyleResolverTag() )
		{
			Result = SubscriberIndex;
			break;
		}
	}

	return Result;
}

/**
 * Assigns the style for this widget for the property provided and refreshes the widget's styles.
 *
 * @param	StyleProperty	The style property we are modifying.
 * @param	NewStyle		the new style to assign to this widget
 * @param	ArrayIndex		if the style property corresponds to an array index, specified the array index to apply the style to
 *
 * @return	TRUE if the style was successfully applied to this widget.
 */
UBOOL UUIObject::SetWidgetStyle( UUIStyle* NewStyle, FStyleReferenceId StyleProperty/*=FStyleReferenceId()*/, INT ArrayIndex/*=INDEX_NONE*/ )
{
	UBOOL bStyleChanged = FALSE;

	TArray<UObject*> StyleOwners;
	StyleOwners.AddItem(this);
	for ( INT SubscriberIndex = 0; SubscriberIndex < StyleSubscribers.Num(); SubscriberIndex++ )
	{
		TScriptInterface<IUIStyleResolver>& Subscriber = StyleSubscribers(SubscriberIndex);
		if ( Subscriber )
		{
			StyleOwners.AddItem(Subscriber.GetObject());
		}
	}

	for ( INT ObjIndex = 0; ObjIndex < StyleOwners.Num(); ObjIndex++ )
	{
		UObject* StyleOwner = StyleOwners(ObjIndex);

		TMultiMap<FStyleReferenceId,FUIStyleReference*> StyleReferences;
		GetStyleReferences(StyleReferences, StyleProperty, StyleOwner);

		TArray<FUIStyleReference*> References;
		// if a valid StyleProperty was specified (indicating that we're attempting to apply NewStyle to a specific style reference) and
		// this widget's style is managed by something else, then MultiFind will fail because StyleReferences will contain references to properties
		// in *this* widget, while StyleProperty is a reference to a property in the widget which manages this one's style.  So we perform a slightly
		// different search to find the ones we need to set
		if ( IsPrivateBehaviorSet(UCONST_PRIVATE_ManagedStyle) && StyleProperty.StyleProperty != NULL )
		{
			for ( TMultiMap<FStyleReferenceId,FUIStyleReference*>::TIterator It(StyleReferences); It; ++It )
			{
				const FStyleReferenceId& CurrentRefId = It.Key();
				if ( CurrentRefId.GetStyleReferenceName() == StyleProperty.GetStyleReferenceName() )
				{
					//@fixme ronp - I might need to do an InsertItem here; TMultiMaps return their elements in the reverse order in which they were added
					// might need to reverse the order in which I add to the array so that array indexes match
					References.AddItem(It.Value());
				}
			}
		}
		else
		{
			StyleReferences.MultiFind(StyleProperty, References);
		}

		for ( INT Index = References.Num() - 1; Index >= 0; Index-- )
		{
			if ( ArrayIndex == INDEX_NONE || Index == ArrayIndex )
			{
				FUIStyleReference* StyleRef = References(Index);
				if ( NewStyle != NULL )
				{
					// make sure the new style has a valid ID
					if ( NewStyle->StyleID.IsValid() )
					{
						bStyleChanged = StyleRef->SetStyle(NewStyle) || bStyleChanged;

						// if this widget's style is managed by someone else (usually its owner widget), do not mark the package dirty
						// and clear the assigned style ID so that this widget does not maintain a permanent mapping to that style
						if ( IsPrivateBehaviorSet(UCONST_PRIVATE_ManagedStyle) )
						{
							FSTYLE_ID ClearStyleID(EC_EventParm);
							StyleRef->SetStyleID(ClearStyleID);
						}
						else
						{
							// otherwise, mark the package dirty so that we save this style assignment
							StyleOwner->Modify(bStyleChanged);
						}
					}
				}
				else
				{
					// if NewStyle is NULL, it indicates that we want to resolve the reference using the reference's default style.
					// The most common reason for doing this is when a new widget is placed in the scene - since we don't know the
					// STYLE_ID of the widget's default style, we need to perform the initial lookup by name.
					bStyleChanged = StyleRef->SetStyle(NULL) || bStyleChanged;
				}
			}
		}
	}

	return bStyleChanged;
}

/**
 * Resolves the style references contained by this widget from the currently active skin.
 *
 * @param	bClearExistingValue		if TRUE, style references will be invalidated first.
 * @param	StyleProperty			if specified, only the style reference corresponding to the specified property
 *									will be resolved; otherwise, all style references will be resolved.
 *
 * @return	TRUE if all style references were successfully resolved.
 */
UBOOL UUIObject::ResolveStyles( UBOOL bClearExistingValue, FStyleReferenceId StylePropertyId/*=FStyleReferenceId()*/ )
{
	UBOOL bResult = FALSE;

	// get a reference to the currently active skin
	UUISkin* ActiveSkin = GetActiveSkin();
	check(ActiveSkin);

	// retrieve the list of style references contained by this widget
	TMultiMap<FStyleReferenceId,FUIStyleReference*> StyleReferenceMap;
	GetStyleReferences(StyleReferenceMap, StylePropertyId);

	const UBOOL bUsesManagedStyle = IsPrivateBehaviorSet(UCONST_PRIVATE_ManagedStyle);

	// if we're supposed to clear the existing values, do that now
	// if we have the PRIVATE_ManagedStyle flag set, then our owning widget should have already updated the ResolvedStyle
	if ( bClearExistingValue == TRUE && !bUsesManagedStyle )
	{
		for ( TMapBase<FStyleReferenceId,FUIStyleReference*>::TIterator It(StyleReferenceMap); It; ++It )
		{
			FUIStyleReference*& StyleRef = It.Value();
			StyleRef->InvalidateResolvedStyle();
		}
	}


	debugfSuppressed(NAME_DevUIStyles, TEXT("Resolving style references for '%s' (%s)"), *GetWidgetPathName(), StylePropertyId.StyleProperty != NULL ? *StylePropertyId.GetStyleReferenceName() : TEXT("ALL"));

	// now, iterate through all style reference properties
	TArray<FStyleReferenceId> StyleReferenceIds;
	INT StyleReferenceCount = StyleReferenceMap.Num(StyleReferenceIds);
	for ( INT RefIndex = 0; RefIndex < StyleReferenceCount; RefIndex++ )
	{
		// for all style reference values associated with this UIStyleReference property, resolve the style reference
		// into an actual UIStyle from the currently active skin
		TArray<FUIStyleReference*> StyleReferences;
		StyleReferenceMap.MultiFind(StyleReferenceIds(RefIndex), StyleReferences);

		INT ArrayIndex = 0;

		// MultiFind returns the elements in the reverse order, so iterate the array backwards
		for ( INT StyleIndex = StyleReferences.Num() - 1; StyleIndex >= 0; StyleIndex--, ArrayIndex++ )
		{
			FUIStyleReference* StyleRef = StyleReferences(StyleIndex);

			UBOOL bStyleChanged = FALSE;

			// GetResolvedStyle() is guaranteed to return a valid style if a valid UISkin is passed in, but if this object's style is managed by the owning widget,
			// never attempt to actually resolve our styles from the StyleReference's AssignedStyleID because it will be cleared by SetWidgetStyle.
			UUIStyle* ResolvedStyle = StyleRef->GetResolvedStyle(bUsesManagedStyle ? NULL : ActiveSkin, &bStyleChanged);
			checkf(ResolvedStyle, TEXT("Unable to resolve style property '%s (%s)' for '%s'"), *StyleReferenceIds(RefIndex).GetStyleReferenceName(), *StyleRef->DefaultStyleTag.ToString(), *GetWidgetPathName());

			// now allow child classes to perform additional logic with the newly resolved style
			OnStyleResolved(ResolvedStyle, StyleReferenceIds(RefIndex), ArrayIndex, bStyleChanged);
			bResult = TRUE;
		}
	}

	// now notify any additional listeners that we have resolved our style
	for ( INT ResolverIndex = 0; ResolverIndex < StyleSubscribers.Num(); ResolverIndex++ )
	{
		TScriptInterface<IUIStyleResolver>& Subscriber = StyleSubscribers(ResolverIndex);
		if ( Subscriber )
		{
			bResult = Subscriber->NotifyResolveStyle(ActiveSkin, bClearExistingValue, NULL, StylePropertyId.StyleProperty ? StylePropertyId.StyleProperty->GetFName() : NAME_None) || bResult;
		}
	}

	return bResult;
}

/**
 * Called when a style reference is resolved successfully.
 *
 * @param	ResolvedStyle			the style resolved by the style reference
 * @param	StylePropertyId			the style reference property that was resolved.
 * @param	ArrayIndex				the array index of the style reference that was resolved.  should only be >0 for style reference arrays.
 * @param	bInvalidateStyleData	if TRUE, the resolved style is different than the style that was previously resolved by this style reference.
 */
void UUIObject::OnStyleResolved( UUIStyle* ResolvedStyle, const FStyleReferenceId& StylePropertyId, INT ArrayIndex, UBOOL bInvalidateStyleData )
{
	// nothing
}

/**
 * Applies the value of bShouldBeDirty to the current style data for all style references in this widget.  Used to force
 * updating of style data.
 *
 * @param	bShouldBeDirty	the value to use for marking the style data for the current menu state of all style references
 *							in this widget as dirty.
 * @param	MenuState		if specified, the style data for that menu state will be modified; otherwise, uses the widget's current
 *							menu state
 */
void UUIObject::ToggleStyleDirtiness( UBOOL bShouldBeDirty, class UUIState* MenuState/*=NULL*/ )
{
	// retrieve the list of style references contained by this widget
	TMultiMap<FStyleReferenceId,FUIStyleReference*> StyleReferenceMap;
	GetStyleReferences(StyleReferenceMap, NULL, INVALID_OBJECT);

	UUIState* CurrentMenuState = MenuState;
	if ( CurrentMenuState == NULL )
	{
		CurrentMenuState = GetCurrentState();
	}

	check(CurrentMenuState);
	for ( TMapBase<FStyleReferenceId,FUIStyleReference*>::TIterator It(StyleReferenceMap); It; ++It )
	{
		FUIStyleReference*& StyleRef = It.Value();

		UUIStyle_Data* StyleData = StyleRef->GetStyleData(CurrentMenuState);
		check(StyleData);

		StyleData->SetDirtiness(bShouldBeDirty);
	}
}

/**
 * Determines whether this widget references the specified style.
 *
 * @param	CheckStyle		the style to check for referencers
 */
UBOOL UUIObject::UsesStyle( UUIStyle* CheckStyle )
{
	UBOOL bResult = FALSE;

	// retrieve the list of style references contained by this widget
	TMultiMap<FStyleReferenceId,FUIStyleReference*> StyleReferenceMap;
	GetStyleReferences(StyleReferenceMap, NULL, INVALID_OBJECT);

	for ( TMapBase<FStyleReferenceId,FUIStyleReference*>::TIterator It(StyleReferenceMap); It; ++It )
	{
		FUIStyleReference*& StyleRef = It.Value();

		// first, attempt to get style that has already been resolved
		UUIStyle* ReferencedStyle = StyleRef->GetResolvedStyle();
		if ( ReferencedStyle == NULL )
		{
			ReferencedStyle = StyleRef->ResolveStyleFromSkin(CheckStyle->GetOuterUUISkin());
		}

		if ( ReferencedStyle != NULL && ReferencedStyle->ReferencesStyle(CheckStyle) )
		{
			bResult = TRUE;
			break;
		}
	}

	return bResult;
}

/**
 * Retrieves the list of UIStyleReferences contained by this widget class.  Used to refresh the style data for all style
 * references contained by this widget whenever the active skin or menu state is changed.
 *
 * @param	out_StyleReferences		a list of the style references contained by this class
 * @param	TargetStyleRef			if specified, only style references associated with the value specified will be added to the map.
 * @param	bIncludeStyleResolvers	specify TRUE to search all subscribed style resolvers for style references as well
 */
void UUIObject::GetStyleReferences( TMultiMap<FStyleReferenceId,FUIStyleReference*>& out_StyleReferences, FStyleReferenceId TargetStyleRef/*=FStyleReferenceId()*/, UObject* SearchObject/*=NULL*/ )
{
	// note we do not empty the array first

	TArray<UObject*> StyleReferenceOwners;

	if ( SearchObject == NULL )
	{
		StyleReferenceOwners.AddItem(this);
	}
	else if ( SearchObject == INVALID_OBJECT )
	{
		StyleReferenceOwners.AddItem(this);
		for ( INT SubscriberIndex = 0; SubscriberIndex < StyleSubscribers.Num(); SubscriberIndex++ )
		{
			TScriptInterface<IUIStyleResolver>& Subscriber = StyleSubscribers(SubscriberIndex);
			if ( Subscriber )
			{
				StyleReferenceOwners.AddUniqueItem(Subscriber.GetObject());
			}
		}
	}
	else
	{
		StyleReferenceOwners.AddItem(SearchObject);
	}

	for ( INT ObjIndex = 0; ObjIndex < StyleReferenceOwners.Num(); ObjIndex++ )
	{
		UObject* ReferenceOwner = StyleReferenceOwners(ObjIndex);

		// get the list of UIStyleReference properties contained within this widget's class
		TArray<FStyleReferenceId> StyleReferences;
		GetStyleReferenceProperties(StyleReferences, ReferenceOwner);

		FName TargetStyleReferenceName = *TargetStyleRef.GetStyleReferenceName();
		for ( INT RefIndex = 0; RefIndex < StyleReferences.Num(); RefIndex++ )
		{
			FStyleReferenceId& StyleRef = StyleReferences(RefIndex);
			if ( TargetStyleRef.StyleProperty == NULL
			||	(TargetStyleReferenceName != NAME_None && TargetStyleReferenceName == *StyleRef.GetStyleReferenceName()) )
			{
				if ( StyleRef.StyleReferenceTag == NAME_None
				&&	(TargetStyleReferenceName != NAME_None || StyleRef.StyleProperty != NULL) )
				{
					// if the style ref only has a property reference, set the style reference name as well
					StyleRef.StyleReferenceTag = TargetStyleReferenceName != NAME_None
						? TargetStyleReferenceName
						: StyleRef.StyleProperty->GetFName();
				}

				UStructProperty* StructProp = Cast<UStructProperty>(StyleRef.StyleProperty,CLASS_IsAUStructProperty);
				if ( StructProp != NULL )
				{
					for ( INT ArrayIndex = 0; ArrayIndex < StructProp->ArrayDim; ArrayIndex++ )
					{
						FUIStyleReference* StyleValueReference = (FUIStyleReference*)((BYTE*)ReferenceOwner + StructProp->Offset + ArrayIndex * StructProp->ElementSize);
						out_StyleReferences.Add(StyleRef, StyleValueReference);
					}
				}
				else
				{
					// StyleProperties should only contain properties that are guaranteed to be of the type UIStyleReference, so if this property is not 
					// a struct property, it must be an array of UIStyleReferences...assert on this assumption
					UArrayProperty* ArrayProp = Cast<UArrayProperty>(StyleRef.StyleProperty);
					checkSlow(ArrayProp);

					// get the FArray containing the elements of this array property
					FArray* Array = (FArray*)((BYTE*)ReferenceOwner + ArrayProp->Offset);

					// get a convenient pointer to the data of the FArray
					BYTE* ArrayData = (BYTE*)Array->GetData();

					// now iterate through the elements of the array and add all style references to the list
					for ( INT ArrayIndex = 0; ArrayIndex < Array->Num(); ArrayIndex++ )
					{
						FUIStyleReference* StyleValueReference = (FUIStyleReference*)(ArrayData + ArrayIndex * ArrayProp->Inner->ElementSize);
						out_StyleReferences.Add(StyleRef, StyleValueReference);
					}
				}
			}
		}
	}
}


/**
 * Retrieves the list of UIStyleReference properties contained by this widget class.
 *
 * @param	out_StyleReferences		a list of the style references contained by this class
 * @param	SearchObject			the object to search for style reference properties in; if NULL, searches only in this object; specify INVALID_OBJECT to
 *									search in this object as well as all subscribed style resolvers
 */
void UUIObject::GetStyleReferenceProperties( TArray<FStyleReferenceId>& out_StyleReferences, UObject* SearchObject/*=NULL*/ )
{
	static FName StyleReferenceStructName = TEXT("UIStyleReference");
	static FName PrimaryStyleName = TEXT("PrimaryStyle");

	TArray<UObject*> ObjectSet;
	if ( SearchObject == NULL )
	{
		ObjectSet.AddItem(this);
	}
	else if ( SearchObject == INVALID_OBJECT )
	{
		ObjectSet.AddItem(this);
		for ( INT SubscriberIndex = 0; SubscriberIndex < StyleSubscribers.Num(); SubscriberIndex++ )
		{
			TScriptInterface<IUIStyleResolver>& Subscriber = StyleSubscribers(SubscriberIndex);
			if ( Subscriber )
			{
				ObjectSet.AddUniqueItem(Subscriber.GetObject());
			}
		}
	}
	else
	{
		ObjectSet.AddItem(SearchObject);
	}

	for ( INT ObjIndex = 0; ObjIndex < ObjectSet.Num(); ObjIndex++ )
	{
		UObject* StyleObj = ObjectSet(ObjIndex);
		FName StyleRefTag = NAME_None;
		UBOOL bAllowPrimaryStyle = FALSE;
		if ( StyleObj == this )
		{
			bAllowPrimaryStyle = bSupportsPrimaryStyle;
		}
		else if ( StyleObj->GetClass()->ImplementsInterface(UUIStyleResolver::StaticClass()) )
		{
			if ( StyleObj->GetOuter()->IsA(UUIObject::StaticClass()) )
			{
				bAllowPrimaryStyle = static_cast<UUIObject*>(StyleObj->GetOuter())->bSupportsPrimaryStyle;
			}
			else
			{
				bAllowPrimaryStyle = bSupportsPrimaryStyle;
			}
		}
		else if ( StyleObj->IsA(UUIObject::StaticClass()) )
		{
			bAllowPrimaryStyle = static_cast<UUIObject*>(StyleObj)->bSupportsPrimaryStyle;
		}

		// determine whether we are dealing with a UIStyleResolver or not.  If so, use its ResolverTag; otherwise use None for the tag
		IUIStyleResolver* StyleResolver = static_cast<IUIStyleResolver*>(StyleObj->GetInterfaceAddress(UUIStyleResolver::StaticClass()));
		if ( StyleResolver != NULL )
		{
			StyleRefTag = StyleResolver->GetStyleResolverTag();
		}

		UClass* SearchClass = StyleObj->GetClass();
		for ( UProperty* Property = SearchClass->PropertyLink; Property; Property = Property->PropertyLinkNext )
		{
			// skip over deprecated or native properties
			if ( (Property->PropertyFlags&(CPF_Deprecated|CPF_Native)) == 0 )
			{
				// search through the list of properties for struct properties using the UIStyleReference struct
				UStructProperty* StructProp = Cast<UStructProperty>(Property,CLASS_IsAUStructProperty);
				if ( StructProp == NULL )
				{
					// if this is an array, check the array type
					UArrayProperty* ArrayProp = Cast<UArrayProperty>(Property);
					if ( ArrayProp != NULL )
					{
						StructProp = Cast<UStructProperty>(ArrayProp->Inner,CLASS_IsAUStructProperty);
					}
				}

				if ( StructProp != NULL && StructProp->Struct->GetFName() == StyleReferenceStructName )
				{
					// Temp hack to allow widgets to remove "Primary Style" from the styles listed in the context menu for that widget if they no longer use it.
					// Will be removed once I am ready to deprecate the PrimaryStyle property.
					if ( bAllowPrimaryStyle || StructProp->GetFName() != PrimaryStyleName )
					{
						out_StyleReferences.AddUniqueItem(FStyleReferenceId(StyleRefTag, StructProp));
					}
				}

				// note that we do not iterate into the internal properties of structs contained by this class, so
				// if this class contains a struct that contains a UIStyleReference property, it won't be found.
			}
		}
	}
}

/**
 * Retrieves the list of data store bindings contained by this widget class.
 *
 * @param	out_DataBindings		a map of data binding property name to UIDataStoreBinding values for the data bindings contained by this class
 * @param	TargetPropertyName		if specified, only data bindings associated with the property specified will be added to the map.
 */
void UUIObject::GetDataBindings( TMultiMap<FName,FUIDataStoreBinding*>& out_DataBindings, const FName TargetPropertyName/*=NAME_None*/ )
{
	// note we do not empty the array first

	// get the list of UIDataStoreBinding properties contained within this widget's class
	TArray<UProperty*> DataStoreProperties;
	GetDataBindingProperties(DataStoreProperties);

	for ( INT PropertyIndex = 0; PropertyIndex < DataStoreProperties.Num(); PropertyIndex++ )
	{
		UProperty* Property = DataStoreProperties(PropertyIndex);
		if ( TargetPropertyName == NAME_None || Property->GetFName() == TargetPropertyName )
		{
			UStructProperty* StructProp = Cast<UStructProperty>(Property,CLASS_IsAUStructProperty);
			if ( StructProp != NULL )
			{
				for ( INT ArrayIndex = 0; ArrayIndex < StructProp->ArrayDim; ArrayIndex++ )
				{
					FUIDataStoreBinding* DataStoreBinding = (FUIDataStoreBinding*)((BYTE*)this + StructProp->Offset + ArrayIndex * StructProp->ElementSize);
					out_DataBindings.Add(Property->GetFName(), DataStoreBinding);
				}
			}
			else
			{
				// StyleProperties should only contain properties that are guaranteed to be of the type UIStyleReference, so if this property is not 
				// a struct property, it must be an array of UIStyleReferences...assert on this assumption
				UArrayProperty* ArrayProp = Cast<UArrayProperty>(Property);
				checkSlow(ArrayProp);

				// get the FArray containing the elements of this array property
				FArray* Array = (FArray*)((BYTE*)this + ArrayProp->Offset);

				// get a convenient pointer to the data of the FArray
				BYTE* ArrayData = (BYTE*)Array->GetData();

				// now iterate through the elements of the array and add all style references to the list
				for ( INT ArrayIndex = 0; ArrayIndex < Array->Num(); ArrayIndex++ )
				{
					FUIDataStoreBinding* DataStoreBinding = (FUIDataStoreBinding*)(ArrayData + ArrayIndex * ArrayProp->Inner->ElementSize);
					out_DataBindings.Add(Property->GetFName(), DataStoreBinding);
				}
			}
		}
	}
}

/**
 * Retrieves the list of UIDataStoreBinding properties contained by this widget class.
 *
 * @param	out_DataBindingProperties	a list of the data store binding properties contained by this class
 */
void UUIObject::GetDataBindingProperties( TArray<UProperty*>& out_DataBindingProperties )
{
	static UScriptStruct* UIDataStoreBindingStruct = FindObject<UScriptStruct>(UUIRoot::StaticClass(), TEXT("UIDataStoreBinding"));
	checkf(UIDataStoreBindingStruct!=NULL,TEXT("Unable to find UIDataStoreBinding struct within UUIRoot!"));

	for ( UProperty* Property = GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext )
	{
		// search through the list of properties for struct properties using the UIStyleReference struct
		UStructProperty* StructProp = Cast<UStructProperty>(Property,CLASS_IsAUStructProperty);
		if ( StructProp == NULL )
		{
			// if this is an array, check the array type
			UArrayProperty* ArrayProp = Cast<UArrayProperty>(Property);
			if ( ArrayProp != NULL )
			{
				StructProp = Cast<UStructProperty>(ArrayProp->Inner,CLASS_IsAUStructProperty);
			}
		}

		if ( StructProp != NULL && StructProp->Struct->IsChildOf(UIDataStoreBindingStruct) )
		{
			out_DataBindingProperties.AddUniqueItem(Property);
		}

		// note that we do not iterate into the internal properties of structs contained by this class, so
		// if this class contains a struct that contains a UIDataStoreBinding property, it won't be found.
	}
}

/** ===== Tool tips / Context menus ===== */
/**
 * Set the markup text for a default data binding to the value specified.
 *
 * @param	NewMarkupText	the new markup text for this widget, either a literal string or a data store markup string
 * @param	BindingIndex	indicates which data store binding to operate on.
 */
void UUIObject::SetDefaultDataBinding( const FString& NewMarkupText, INT BindingIndex )
{
	if ( BindingIndex == UCONST_TOOLTIP_BINDING_INDEX )
	{
		if ( ToolTip.MarkupString != NewMarkupText )
		{
			Modify();
			ToolTip.MarkupString = NewMarkupText;
			ResolveDefaultDataBinding(BindingIndex);
		}
	}
	else if ( BindingIndex == UCONST_CONTEXTMENU_BINDING_INDEX )
	{
		if ( ContextMenuData.MarkupString != NewMarkupText )
		{
			Modify();
			ContextMenuData.MarkupString = NewMarkupText;
			ResolveDefaultDataBinding(BindingIndex);
		}
	}
}

/**
 * Returns the data binding's current value.
 *
 * @param	BindingIndex	indicates which data store binding to operate on.
 */
FString UUIObject::GetDefaultDataBinding( INT BindingIndex ) const
{
	if ( BindingIndex == UCONST_TOOLTIP_BINDING_INDEX )
	{
		return ToolTip.MarkupString;
	}
	else if ( BindingIndex == UCONST_CONTEXTMENU_BINDING_INDEX )
	{
		return ContextMenuData.MarkupString;
	}

	return TEXT("");
}

/**
 * Resolves the data binding's markup string.
 *
 * @param	BindingIndex	indicates which data store binding to operate on.
 *
 * @return	TRUE if a data store field was successfully resolved from the data binding
 */
UBOOL UUIObject::ResolveDefaultDataBinding( INT BindingIndex )
{
	UBOOL bResult = FALSE;

	if ( !IsA(UUIPrefab::StaticClass()) && !IsInUIPrefab() )
	{
		IUIDataStoreSubscriber* SubscriberInterface = (IUIDataStoreSubscriber*)GetInterfaceAddress(IUIDataStoreSubscriber::UClassType::StaticClass());
		if ( SubscriberInterface != NULL )
		{
			TScriptInterface<IUIDataStoreSubscriber> Subscriber;
			Subscriber.SetObject(this);
			Subscriber.SetInterface(SubscriberInterface);

			if ( BindingIndex == UCONST_TOOLTIP_BINDING_INDEX )
			{
				bResult = ToolTip.ResolveMarkup(Subscriber);
			}
			else if ( BindingIndex == UCONST_CONTEXTMENU_BINDING_INDEX )
			{
				bResult = ContextMenuData.ResolveMarkup(Subscriber);
			}
		}
	}

	return bResult;
}

/**
 * Returns the data store providing the data for all default data bindings.
 */
void UUIObject::GetDefaultDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	if ( ToolTip )
	{
		out_BoundDataStores.AddUniqueItem(*ToolTip);
	}
	if ( ContextMenuData )
	{
		out_BoundDataStores.AddUniqueItem(*ContextMenuData);
	}
}

/**
 * Clears the reference to the bound data store, if applicable.
 *
 * @param	BindingIndex	indicates which data store binding to operate on.
 */
void UUIObject::ClearDefaultDataBinding( INT BindingIndex )
{
	if ( BindingIndex == UCONST_TOOLTIP_BINDING_INDEX )
	{
		ToolTip.ClearDataBinding();
	}
	else if ( BindingIndex == UCONST_CONTEXTMENU_BINDING_INDEX )
	{
		ContextMenuData.ClearDataBinding();
	}
}

/**
 * Generates a string which can be used to interact with temporary data in the scene data store specific to this widget.
 *
 * @param	Group	for now, doesn't matter, as only "ContextMenuItems" is supported
 *
 * @return	a data store markup string which can be used to reference content specific to this widget in the scene's
 *			data store.
 */
FString UUIObject::GenerateSceneDataStoreMarkup( const FString& Group/*=TEXT("ContextMenuItems")*/ ) const
{
	FString Result;
	if ( WidgetID.IsValid() )
	{
		Result = FString::Printf(TEXT("<SceneData:%s.%s>"), *Group, *WidgetID.String());
	}
	return Result;
}

/**
 * Returns the ToolTip data binding's current value after being resolved.
 */
FString UUIObject::GetToolTipValue()
{
	FString Result;

	if ( ToolTip || ResolveDefaultDataBinding(UCONST_TOOLTIP_BINDING_INDEX) )
	{
		FUIProviderFieldValue ToolTipValue(EC_EventParm);
		if ( ToolTip.GetBindingValue(ToolTipValue) )
		{
			Result = ToolTipValue.StringValue;
		}
	}

	return Result;
}

/**
 * Activates the tooltip associated with this widget.  Called when this widget (or one of its Children) becomes
 * the scene client's ActiveControl.  If this widget doesn't support tool-tips or does not have a valid tool-tip
 * binding, the call is propagated upwards to the parent widget.
 *
 * @return	a pointer to the tool-tip that was activated.  This value will be stored in the scene's ActiveToolTip member.
 */
UUIToolTip* UUIObject::ActivateToolTip()
{
	UUIToolTip* NewToolTip = NULL;

	if ( !DELEGATE_IS_SET(OnQueryToolTip) || delegateOnQueryToolTip(this, NewToolTip) )
	{
		//@todo ronp - activate the default tool tip
		// find a data binding named "ToolTipBinding".  If it has a valid data store binding
		// activate the default tool-tip
		if ( ToolTip )
		{
			// if a custom tooltip wasn't specified, use the default one
			if ( NewToolTip == NULL )
			{
				UUIScene* Scene = GetScene();
				if ( Scene != NULL )
				{
					NewToolTip = Scene->GetDefaultToolTip();
				}
			}

			if ( NewToolTip != NULL )
			{
				NewToolTip = NewToolTip->delegateActivateToolTip();
				if ( NewToolTip != NULL )
				{
					// then link it to this widget's data store binding
					NewToolTip->LinkBinding(&ToolTip);
				}
			}
		}

		// otherwise, propagate up to the parent
		else
		{
			UUIObject* WidgetOwner = GetOwner();
			if ( WidgetOwner != NULL )
			{
				NewToolTip = WidgetOwner->ActivateToolTip();
			}
		}
	}
	
	return NewToolTip;
}

/**
 * Activates the context menu for this widget, if it has one.  Called when the user right-clicks (or whatever input key
 * is configured to activate the ShowContextMenu UI input alias) while this widget (or one of its Children) is the
 * scene client's ActiveControl.
 *
 * @return	a pointer to the context menu that was activated.  This value will be stored in the scene's ActiveContextMenu member.
 */
UUIContextMenu* UUIObject::ActivateContextMenu( INT PlayerIndex )
{
	UUIContextMenu* NewContextMenu = NULL;

	if ( !DELEGATE_IS_SET(OnOpenContextMenu) || delegateOnOpenContextMenu(this, PlayerIndex, NewContextMenu) )
	{
		if ( NewContextMenu == NULL )
		{
			FUIProviderFieldValue MenuItems(EC_EventParm);
			if ( ContextMenuData && ContextMenuData.GetBindingValue(MenuItems) && MenuItems.ArrayValue.Num() > 0 )
			{
				// if a custom tooltip wasn't specified, use the default one
				UUIScene* Scene = GetScene();
				if ( Scene != NULL )
				{
					NewContextMenu = Scene->GetDefaultContextMenu();
				}

				if ( NewContextMenu != NULL )
				{
					NewContextMenu->DataSource = ContextMenuData;
					if ( !NewContextMenu->RefreshListData(TRUE) || NewContextMenu->GetItemCount() == 0 )
					{
						NewContextMenu = NULL;
					}
				}
			}
		}
		else
		{
			NewContextMenu->DataSource = ContextMenuData;
			if ( !NewContextMenu->RefreshListData(TRUE) || NewContextMenu->GetItemCount() == 0 )
			{
				NewContextMenu = NULL;
			}
		}

		if ( NewContextMenu == NULL )
		{
			UUIObject* WidgetOwner = GetOwner();
			if ( WidgetOwner != NULL )
			{
				NewContextMenu = WidgetOwner->ActivateContextMenu(PlayerIndex);
			}
		}
	}

	return NewContextMenu;
}

/**
 * Called after this object has been de-serialized from disk.
 *
 * This version converts the deprecated PRIVATE_DisallowReparenting flag to PRIVATE_EditorNoReparent, if set.
 */
void UUIObject::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerVersion() < VER_REMOVED_DISALLOW_REPARENTING_FLAG )
	{
		// this was the value of PRIVATE_DisallowReparenting
		if ( (PrivateFlags&0x040) != 0 )
		{
			MarkPackageDirty();

			PrivateFlags &= ~0x040;

			// this is the new value
			PrivateFlags |= UCONST_PRIVATE_EditorNoReparent;
		}

		// we also changed the meaning of the TreeHidden flag - it's previous meaning was what the TreeHiddenRecursive is now
		if ( (PrivateFlags&UCONST_PRIVATE_TreeHidden) !=  0 )
		{
			MarkPackageDirty();
			PrivateFlags |= UCONST_PRIVATE_TreeHiddenRecursive;
		}
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIObject::PostEditChange( UProperty* PropertyThatChanged )
{
	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIObject::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PostEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		TDoubleLinkedList<UProperty*>::TDoubleLinkedListNode* MemberNode = PropertyThatChanged.GetActiveMemberNode();
		UProperty* MemberProperty = MemberNode->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("Rotation") || PropertyName == TEXT("ZDepth") )
			{
				UpdateRotationMatrix();
			}
			else if ( PropertyName == TEXT("ToolTip") )
			{
				if ( !IsA(UUIPrefab::StaticClass()) && !IsInUIPrefab() )
				{
					ResolveDefaultDataBinding(UCONST_TOOLTIP_BINDING_INDEX);
				}
			}
			else if ( PropertyName == TEXT("ContextMenu") )
			{
				if ( !IsA(UUIPrefab::StaticClass()) && !IsInUIPrefab() )
				{
					ResolveDefaultDataBinding(UCONST_CONTEXTMENU_BINDING_INDEX);
				}
			}
		}
	}
}

/**
 * Called after importing property values for this object (paste, duplicate or .t3d import)
 * Allow the object to perform any cleanup for properties which shouldn't be duplicated or
 * are unsupported by the script serialization
 */
void UUIObject::PostEditImport()
{
	Super::PostEditImport();
}

/**
 * Called after this widget is renamed; ensures that the widget's tag matches the name of the widget.
 */
void UUIObject::PostRename()
{
	Super::PostRename();

	if ( WidgetTag != GetFName() )
	{
		WidgetTag = GetFName();
		MarkPackageDirty();
	}
}

/**
 * Adds the specified state to the screen object's StateStack and refreshes the widget's style using the new state.
 *
 * @param	StateToActivate		the new state for the widget
 * @param	PlayerIndex			the index [into the Engine.GamePlayers array] for the player that generated this call
 *
 * @return	TRUE if the widget's state was successfully changed to the new state.  FALSE if the widget couldn't change
 *			to the new state or the specified state already exists in the widget's list of active states
 */
UBOOL UUIObject::ActivateState( UUIState* StateToActivate, INT PlayerIndex )
{
	UUIState* PreviouslyActiveState = StateStack.Num() > 0 ? StateStack.Last() : NULL;
	UBOOL bResult = Super::ActivateState(StateToActivate, PlayerIndex);

	if ( bResult && (StateStack.Num() == 0 || StateStack.Last() != PreviouslyActiveState) )
	{
		ResolveStyles(FALSE);
	}

	return bResult;
}

/**
 * Adds the specified preview state to the screen object's StateStack and refreshes the widget style using the new state.
 *
 * @param	StateToActivate		the new preview state for the widget
 *
 * @return	TRUE if the widget's state was successfully changed to the new preview state.  FALSE if the widget couldn't change
 *			to the new state or the specified state already exists in the widget's list of active states
 */
UBOOL UUIObject::ActivatePreviewState( UUIState* StateToActivate )
{
	UBOOL bResult = Super::ActivatePreviewState(StateToActivate);
	if ( bResult && StateToActivate == StateStack.Last() )
	{
		ResolveStyles(FALSE);
	}

	return bResult;
}

/**
 * Removes the specified state from the screen object's state stack and refreshes the widget's style using the new state.
 *
 * @param	StateToRemove	the state to be removed
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated this call
 *
 * @return	TRUE if the state was successfully removed, or if the state didn't exist in the widget's list of states;
 *			false if the state overrode the request to be removed
 */
UBOOL UUIObject::DeactivateState( UUIState* StateToRemove, INT PlayerIndex )
{
	UUIState* PreviouslyActiveState = StateStack.Num() > 0 ? StateStack.Last() : NULL;
	UBOOL bResult = Super::DeactivateState(StateToRemove,PlayerIndex);
	if ( bResult && (StateStack.Num() == 0 || StateStack.Last() != PreviouslyActiveState) )
	{
		ResolveStyles(FALSE);
	}

	return bResult;
}

/**
 * Converts and stores the render bounds for a widget as vectors, for use in line checks.  Used by the code that generates the
 * automatic navigation links for widgets.
 */
struct FClosestLinkCalculationBox
{
	/** Constructor */
	FClosestLinkCalculationBox( UUIObject* InObject )
	: Obj(InObject)
	{
		checkSlow(Obj);
		Origin = FVector(Obj->GetPositionExtent(UIFACE_Left, TRUE), Obj->GetPositionExtent(UIFACE_Top, TRUE), Obj->ZDepth);
		Extent = FVector(Obj->GetPositionExtent(UIFACE_Right, TRUE), Obj->GetPositionExtent(UIFACE_Bottom, TRUE), Obj->ZDepth);
	}

	/**
	 * Determines whether the widget specified is a valid navigation link for the specified face of this widget.
	 *
	 * @param	Other	the bounds data for the widget that will possibly become a navigation link for our widget
	 * @param	Face	the face that is being evaluated
	 *
	 * @return	FALSE if the widget specified is on the wrong side of this widget (i.e. if we're evaluating the left
	 *			face, then any widgets that have a right face that is further left than our left face cannot become
	 *			navigation links)
	 */
	UBOOL IsValid( const FClosestLinkCalculationBox& Other, const EUIWidgetFace Face ) const
	{
		UBOOL bResult = FALSE;
		switch( Face )
		{
		case UIFACE_Top:
			// Target's bottom must be less than source's top
			bResult = Other.Extent.Y <= Origin.Y;
			break;

		case UIFACE_Bottom:
			// Target's top must be greater than source's bottom
			bResult = Other.Origin.Y >= Extent.Y;
			break;

		case UIFACE_Left:
			// Target's right must be less than source's left
			bResult = Other.Extent.X <= Origin.X;
			break;

		case UIFACE_Right:
			// Target's left must be greater than source's right
			bResult = Other.Origin.X >= Extent.X;
			break;
		}

		return bResult;
	}

	/**
	 * Gets the vectors representing the end points of the specified face.
	 *
	 * @param	Face		the face to get endpoints for.
	 * @param	StartPoint	[out] the left or top endpoint of the specified face
	 * @param	EndPoint	[out] the right or bottom point of the specified face
	 */
	void GetFaceVectors( const EUIWidgetFace Face, FVector& StartPoint, FVector& EndPoint ) const
	{
		StartPoint = Origin;
		EndPoint = Extent;

		switch ( Face )
		{
		case UIFACE_Top:
			EndPoint.Y = Origin.Y;
			break;

		case UIFACE_Bottom:
			StartPoint.Y = Extent.Y;
			break;

		case UIFACE_Left:
			EndPoint.X = StartPoint.X;
			break;

		case UIFACE_Right:
			StartPoint.X = Extent.X;
			break;
		}
	}

	/** the widget that this struct is tracking render bounds for */
	UUIObject*	Obj;

	/** stores the value of the widget's Top-left corner */
	FVector		Origin;

	/** stores the value of the widget's lower-right corner */
	FVector		Extent;
};

/**
 * Calculates the closes sibling widget for each face of this widget and sets that widget as the navigation target
 * for that face.
 *
 * @todo - LOTS of room for optimization here....better would be to have the parent build the list of face vectors, then pass
 * that list to all children so that we avoid the umpteen FVector copies in this function
 */
void UUIObject::GenerateAutoNavigationLinks()
{
	UUIScreenObject* Parent = GetParent();

	// only need to generate navigation links if this widget can receive focus
	if ( Parent != NULL && CanAcceptFocus() )
	{
		FClosestLinkCalculationBox Bounds(this);

		// only generate auto-nav links there is at least one face that doesn't already have a widget
		// configured [in the editor] as the navigation target
		if ( NavigationTargets.NeedsLinkGeneration() )
		{
			// we can only set navigation links to other children of our parent widget, so
			// grab the list of widgets contained by our parent
			TArray<UUIObject*> SiblingControls = Parent->GetChildren();
			TIndirectArray<FClosestLinkCalculationBox> PotentialLinks;

			// build the array of widgets that are capable of becoming navigation links (i.e. can receive focus)
			// @todo - encapsulate this into a function
			for ( INT SiblingIndex = 0; SiblingIndex < SiblingControls.Num(); SiblingIndex++ )
			{
				UUIObject* Sibling = SiblingControls(SiblingIndex);
				if ( Sibling != this && Sibling->CanAcceptFocus() )
				{
					PotentialLinks.AddRawItem( new FClosestLinkCalculationBox(Sibling) );
				}
			}

			// now go through each face of this widget
			for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
			{
				EUIWidgetFace CurrentFace = (EUIWidgetFace)FaceIndex;
				EUIWidgetFace OppositeFace = GetOppositeFace(FaceIndex);

				// there is no navigation target already configured for this face, so now we'll find the closest sibling
				// widget and use that one as the navigation target

				// the shortest-distance methods we'll use expect vectors, so translate the end points for this face
				// into vectors now
				FVector FaceStart, FaceEnd;
				Bounds.GetFaceVectors(CurrentFace, FaceStart, FaceEnd);

				UUIObject* ClosestSibling = NULL;
				FLOAT SmallestDistance = MAX_FLT;

				// iterate through the list of siblings
				for ( INT SiblingIndex = 0; SiblingIndex < PotentialLinks.Num(); SiblingIndex++ )
				{
					FClosestLinkCalculationBox& TestLink = PotentialLinks(SiblingIndex);

					// discard any widgets that aren't in the correct region to be a nav link for this face
					if ( Bounds.IsValid(TestLink, CurrentFace) )
					{
						// get vectors for the end points of the other widget's face
						FVector OtherFaceStart, OtherFaceEnd;
						TestLink.GetFaceVectors(OppositeFace, OtherFaceStart, OtherFaceEnd);

						// get the point from each widget's respective face that is closest to the other widget's face
						FVector NearestPoint, SiblingNearestPoint;
						SegmentDistToSegment(FaceStart, FaceEnd, OtherFaceStart, OtherFaceEnd, NearestPoint, SiblingNearestPoint);

						// see how far away this point is
						FLOAT CurrentDistance = FVector(NearestPoint - SiblingNearestPoint).Size();
						if ( CurrentDistance < SmallestDistance )
						{
							ClosestSibling = TestLink.Obj;
							SmallestDistance = CurrentDistance;
						}

						// otherwise, if the current candidate is [pretty much] the exact same distance from this widget as the one
						// previously deterimined as closest, then we'll need to drill down a bit more to find out who is really the closest
						else if ( Abs<FLOAT>(CurrentDistance - SmallestDistance) < KINDA_SMALL_NUMBER )
						{
							//@fixme - when evaluating several siblings that are all the same distance from this face, need to
							// perform more precise calculation based on the midpoints of the respective faces...to set this up
							// just add three short horizontal buttons (aligned vertically), then add one vertical button that
							// spans the distance from the first button's top face to the last button's bottom face
						}

					}
				}

				NavigationTargets.SetNavigationTarget(CurrentFace,ClosestSibling);
			}
		}
	}
}

/**
 * Sets the actual navigation target for the specified face.  If the new value is different from the current value,
 * requests the owning scene to update the navigation links for the entire scene.
 *
 * @param	Face			the face to set the navigation link for
 * @param	NewNavTarget	the widget to set as the link for the specified face
 *
 * @return	TRUE if the nav link was successfully set.
 */
UBOOL UUIObject::SetNavigationTarget( BYTE Face, UUIObject* NewNavTarget )
{
	UBOOL bResult = FALSE;
	if ( IsValidNavigationTarget((EUIWidgetFace)Face,NewNavTarget) )
	{
		if ( NavigationTargets.SetNavigationTarget((EUIWidgetFace)Face, NewNavTarget) )
		{
			RequestSceneUpdate(FALSE,FALSE,TRUE);
		}

		bResult = TRUE;
	}

	return bResult;
}
UBOOL UUIObject::SetNavigationTarget( UUIObject* LeftTarget, UUIObject* TopTarget, UUIObject* RightTarget, UUIObject* BottomTarget )
{
	UBOOL bResult = FALSE;
	if (	IsValidNavigationTarget(UIFACE_Left,LeftTarget)
		&&	IsValidNavigationTarget(UIFACE_Top,TopTarget)
		&&	IsValidNavigationTarget(UIFACE_Right,RightTarget)
		&&	IsValidNavigationTarget(UIFACE_Bottom,BottomTarget)	)
	{
		if ( NavigationTargets.SetNavigationTarget(LeftTarget, TopTarget, RightTarget, BottomTarget) )
		{
			RequestSceneUpdate(FALSE,FALSE,TRUE);
		}

		bResult = TRUE;
	}

	return TRUE;
}

/**
 * Sets the designer-specified navigation target for the specified face.  When navigation links for the scene are rebuilt,
 * the designer-specified navigation target will always override any auto-calculated targets.  If the new value is different from the current value,
 * requests the owning scene to update the navigation links for the entire scene.
 *
 * @param	Face				the face to set the navigation link for
 * @param	NavTarget			the widget to set as the link for the specified face
 * @param	bIsNullOverride		if NavTarget is NULL, specify TRUE to indicate that this face's nav target should not
 *								be automatically calculated.
 *
 * @return	TRUE if the nav link was successfully set.
 */
UBOOL UUIObject::SetForcedNavigationTarget( BYTE Face, UUIObject* NavTarget, UBOOL bIsNullOverride/*=FALSE*/ )
{
	UBOOL bResult = FALSE;
	if ( IsValidNavigationTarget((EUIWidgetFace)Face,NavTarget) )
	{
		Modify();
		if ( NavigationTargets.SetForcedNavigationTarget((EUIWidgetFace)Face, NavTarget, bIsNullOverride) )
		{
			RequestSceneUpdate(FALSE,FALSE,TRUE);
		}

		bResult = TRUE;
	}

	return bResult;
}

UBOOL UUIObject::SetForcedNavigationTarget( UUIObject* LeftTarget, UUIObject* TopTarget, UUIObject* RightTarget, UUIObject* BottomTarget )
{
	UBOOL bResult = FALSE;
	if (	IsValidNavigationTarget(UIFACE_Left,LeftTarget)
		&&	IsValidNavigationTarget(UIFACE_Top,TopTarget)
		&&	IsValidNavigationTarget(UIFACE_Right,RightTarget)
		&&	IsValidNavigationTarget(UIFACE_Bottom,BottomTarget)	)
	{
		Modify();

		if ( NavigationTargets.SetForcedNavigationTarget(LeftTarget, TopTarget, RightTarget, BottomTarget) )
		{
			RequestSceneUpdate(FALSE,FALSE,TRUE);
		}

		bResult = TRUE;
	}

	return bResult;
}

/**
 * Gets the navigation target for the specified face.  If a designer-specified nav target is set for the specified face,
 * that object is returned.
 *
 * @param	Face		the face to get the nav target for
 * @param	LinkType	specifies which navigation link type to return.
 *							NAVLINK_MAX: 		return the designer specified navigation target, if set; otherwise returns the auto-generated navigation target
 *							NAVLINK_Automatic:	return the auto-generated navigation target, even if the designer specified nav target is set
 *							NAVLINK_Manual:		return the designer specified nav target, even if it isn't set
 *
 * @return	a pointer to a widget that will be the navigation target for the specified direction, or NULL if there is
 *			no nav target for that face.
 */
UUIObject* UUIObject::GetNavigationTarget( EUIWidgetFace Face, ENavigationLinkType LinkType/*=NAVLINK_MAX*/ ) const
{
	UUIObject* NavTarget = NULL;
	if ( Face != UIFACE_MAX )
	{
		NavTarget = NavigationTargets.GetNavigationTarget(Face,LinkType);
	}

	return NavTarget;
}

/**
 * Determines if the specified widget is a valid candidate for being the nav target for the specified face of this widget.
 *
 * @param	Face			the face that we'd be assigning NewNavTarget to
 * @param	NewNavTarget	the widget that we'd like to make the nav target for that face
 *
 * @return	TRUE if NewNavTarget is allowed to be the navigation target for the specified face of this widget.
 */
UBOOL UUIObject::IsValidNavigationTarget( EUIWidgetFace Face, UUIObject* NewNavTarget ) const
{
	UBOOL bResult = FALSE;
	if ( Face != UIFACE_MAX && NewNavTarget != this )
	{
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Determines whether the specified widget can be set as a docking target for the specified face.
 *
 * @param	SourceFace		the face on this widget that the potential dock source
 * @param	Target			the potential docking target
 * @param	TargetFace		the face on the target widget that we want to check for
 *
 * @return	TRUE if a docking link can be safely established between SourceFace and TargetFace.
 */
UBOOL UUIObject::IsValidDockTarget( EUIWidgetFace SourceFace, UUIObject* Target, EUIWidgetFace TargetFace )
{
	UBOOL bResult = TRUE;

	if ( Target == this || IsPrivateBehaviorSet(UCONST_PRIVATE_NotDockable) )
	{
		bResult = FALSE;
	}
	else
	{
		// perform a more comprehensive test to see if there this docking relationship would create a recursive docking link

		// save the current values of this widget's docking link for the SourceFace
		UUIScreenObject* SavedTarget = DockTargets.GetDockTarget(SourceFace);
		EUIWidgetFace SavedFace = DockTargets.GetDockFace(SourceFace);

		if ( SavedTarget == NULL && DockTargets.IsDocked(SourceFace) )
		{
			SavedTarget = GetScene();
		}

		// temporarily set the docking target values for this widget to the values we want to test for
		DockTargets.SetDockTarget(SourceFace, Cast<UUIObject>(Target), TargetFace);

		// using a temporary docking stack, attempt to build a docking chain based from SourceFace....if there is a recursive docking relationship buried
		// somewhere in the docking chain, this function should return false
		TArray<FUIDockingNode> TestDockingStack;
		bResult = AddDockingNode(TestDockingStack, SourceFace);

		// @todo - if this test isn't reliable enough, we can use UUIScene::ValidateDockingStack()

		// restore the previous values of the docking link for SourceFace
		DockTargets.SetDockTarget(SourceFace, SavedTarget, SavedFace);
	}

	return bResult;
}

/**
 * Determines whether this widget can become the focused control. In the case of this widget we don't want it to gain focus.
 *
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to set focus for.
 *
 * @return	TRUE if this widget (or one of its children) is capable of becoming the focused control.
 */
UBOOL UUIObject::CanAcceptFocus( INT PlayerIndex/*=0*/ ) const
{
	UBOOL Result = FALSE;

	if( !IsPrivateBehaviorSet( UCONST_PRIVATE_NotFocusable ) )
		Result = Super::CanAcceptFocus( PlayerIndex );

	return Result;
}

/**
 * Checks to see if the specified private behavior is set. Valid behavior flags are defined in UIRoot.uc, as consts which begin with PRIVATE_
 *
 * @param	Behavior	the flag of the private behavior that is being checked
 *
 * @return	TRUE if the specified flag is set and FALSE if not.
 */
UBOOL UUIObject::IsPrivateBehaviorSet( INT Behavior ) const
{
	return (PrivateFlags & Behavior) == Behavior;
}

/**
 * Set the specified private behavior for this UIObject. Valid behavior flags are defined in UIRoot.uc, as consts which begin with PRIVATE_
 *
 * @param	Behavior	the flag of the private behavior that is being set
 * @param	Value		whether the flag is being enabled or disabled
 * @param	bRecurse	specify TRUE to apply the flag in all children of this widget as well.
 */
void UUIObject::SetPrivateBehavior( INT Behavior, UBOOL Value, UBOOL bRecurse/*=FALSE*/ )
{
	if( Value )
	{
		PrivateFlags |= Behavior;
	}
	else
	{
		PrivateFlags &= ~Behavior;
	}

	if ( bRecurse )
	{
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			Children(ChildIndex)->SetPrivateBehavior(Behavior, Value, bRecurse);
		}
	}
}

/**
 * Change the value of bEnableActiveCursorUpdates to the specified value.
 */
void UUIObject::SetActiveCursorUpdate( UBOOL bShouldReceiveCursorUpdates )
{
	bEnableActiveCursorUpdates = bShouldReceiveCursorUpdates;
}

/**
 * Returns the value of bEnableActiveCursorUpdates
 */
UBOOL UUIObject::NeedsActiveCursorUpdates() const
{
	return bEnableActiveCursorUpdates;
}


/**
 * Determines whether this widget has any tranformation applied to it.
 *
 * @param	bIncludeParentTransforms	specify TRUE to check whether this widget's parents are transformed if this one isn't.
 */
UBOOL UUIObject::HasTransform( UBOOL bIncludeParentTransforms/*=TRUE*/ ) const
{
	UBOOL bResult = Rotation.TransformMatrix != FMatrix::Identity || ZDepth != 0;
	if ( !bResult && bIncludeParentTransforms )
	{
		UUIObject* ParentWidget = GetOwner();
		if ( ParentWidget != NULL )
		{
			bResult = ParentWidget->HasTransform(TRUE);
		}
	}
	return bResult;
}

/**
 * Projects the vertices made from all faces of this widget and stores the results in the BoundsVertices array.
 *
 * @param	bRecursive	specify TRUE to propagate this call to all children of this widget.
 */
void UUIObject::UpdateRenderBoundsVertices( UBOOL bRecursive/*=TRUE*/ )
{
	if ( HasTransform() )
	{
		// get the location of the widget's four corners, in canvas space.
		FLOAT Left, Top, Right, Bottom;
		GetPositionExtents(Left, Right, Top, Bottom, FALSE);

		FVector CanvasVectors[4] =
		{
			FVector( Left, Top, 0.0f ), FVector( Right, Top, 0.0f ),
			FVector( Right, Bottom, 0.0f ), FVector( Left, Bottom, 0.0f )
		};

		for ( INT FaceIndex = 0; FaceIndex < 4; FaceIndex++ )
		{
			RenderBoundsVertices[FaceIndex] = FVector2D(Project(CanvasVectors[FaceIndex]));
		}
	}

	if ( bRecursive )
	{
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			Children(ChildIndex)->UpdateRenderBoundsVertices(TRUE);
		}
	}
}

/**
 * Gets the minimum and maximum values for the widget's face positions after rotation (if specified) has been applied.
 *
 * @todo - This should ideally be a lot more efficient and use the render bounds.
 *
 * @param	MinX				The minimum x position of this widget.
 * @param	MaxX				The maximum x position of this widget.
 * @param	MinY				The minimum y position of this widget.
 * @param	MaxY				The maximum y position of this widget.
 * @param	bIncludeRotation	Indicates whether the widget's rotation should be applied to the extent values.
 */
void UUIObject::GetPositionExtents( FLOAT& MinX, FLOAT& MaxX, FLOAT& MinY, FLOAT& MaxY, UBOOL bIncludeRotation/*=FALSE*/ ) const
{
	const FLOAT Left	= GetPosition(UIFACE_Left, EVALPOS_PixelViewport);
	const FLOAT Top		= GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
	const FLOAT Right	= GetPosition(UIFACE_Right, EVALPOS_PixelViewport);
	const FLOAT Bottom	= GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport);

	// if we aren't rotated or we don't want rotation taken into account, just use render bounds
	if ( !bIncludeRotation || !HasTransform() )
	{
		MinX = Left;
		MaxX = Right;
		MinY = Top;
		MaxY = Bottom;
	}
	else
	{
		FVector LeftTop( Left, Top, 0.0f );
		FVector RightTop( Right, Top, 0.0f );
		FVector LeftBottom( Left, Bottom, 0.0f );
		FVector RightBottom( Right, Bottom, 0.0f );

		const FMatrix LocalViewProjMatrix = GetCanvasToScreen();
		LeftTop = FVector(ScreenToPixel(LocalViewProjMatrix.TransformFVector(LeftTop)),0.f);
		RightTop = FVector(ScreenToPixel(LocalViewProjMatrix.TransformFVector(RightTop)),0.f);
		LeftBottom = FVector(ScreenToPixel(LocalViewProjMatrix.TransformFVector(LeftBottom)),0.f);
		RightBottom = FVector(ScreenToPixel(LocalViewProjMatrix.TransformFVector(RightBottom)),0.f);

		MinX = Min( Min(LeftTop.X, RightTop.X), Min(LeftBottom.X, RightBottom.X) );
		MinY = Min( Min(LeftTop.Y, RightTop.Y), Min(LeftBottom.Y, RightBottom.Y) );
		MaxX = Max( Max(LeftTop.X, RightTop.X), Max(LeftBottom.X, RightBottom.X) );
		MaxY = Max( Max(LeftTop.Y, RightTop.Y), Max(LeftBottom.Y, RightBottom.Y) );
	}
}

/**
 * Gets the minimum or maximum value for the specified widget face position after rotation has been applied.
 *
 * @todo - This should ideally be a lot more efficient and use the render bounds.
 *
 * @param	bIncludeRotation	Indicates whether the widget's rotation should be applied to the extent values.
 */
FLOAT UUIObject::GetPositionExtent( BYTE Face, UBOOL bIncludeRotation/*=FALSE*/ ) const
{
	checkSlow(Face<UIFACE_MAX);

	FLOAT Result = GetPosition(Face, EVALPOS_PixelViewport);

	// if we're rotated,
	if ( bIncludeRotation && HasTransform() )
	{
		const FLOAT Left	= GetPosition(UIFACE_Left, EVALPOS_PixelViewport);
		const FLOAT Top		= GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
		const FLOAT Right	= GetPosition(UIFACE_Right, EVALPOS_PixelViewport);
		const FLOAT Bottom	= GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport);

		FVector LeftTop( Left, Top, 0.0f );
		FVector RightTop( Right, Top, 0.0f );
		FVector LeftBottom( Left, Bottom, 0.0f );
		FVector RightBottom( Right, Bottom, 0.0f );

		const FMatrix LocalViewProjMatrix = GetCanvasToScreen();
		LeftTop = FVector(ScreenToPixel(LocalViewProjMatrix.TransformFVector(LeftTop)),0.f);
		RightTop = FVector(ScreenToPixel(LocalViewProjMatrix.TransformFVector(RightTop)),0.f);
		LeftBottom = FVector(ScreenToPixel(LocalViewProjMatrix.TransformFVector(LeftBottom)),0.f);
		RightBottom = FVector(ScreenToPixel(LocalViewProjMatrix.TransformFVector(RightBottom)),0.f);

		switch ( Face )
		{
		case UIFACE_Left:
			Result = Min( Min(LeftTop.X, RightTop.X), Min(LeftBottom.X, RightBottom.X) );
			break;
		case UIFACE_Top:
			Result = Min( Min(LeftTop.Y, RightTop.Y), Min(LeftBottom.Y, RightBottom.Y) );
			break;
		case UIFACE_Right:
			Result = Max( Max(LeftTop.X, RightTop.X), Max(LeftBottom.X,RightBottom.X) );
			break;
		case UIFACE_Bottom:
			Result = Max( Max(LeftTop.Y, RightTop.Y), Max(LeftBottom.Y, RightBottom.Y) );
			break;
		}
	}

	return Result;
}

/* --------- Animations ------------*/

/**
 * Itterate over the AnimStack and tick each active sequence
 *
 * @Param DeltaTime			How much time since the last call
 */

void UUIObject::TickAnim(FLOAT DeltaTime)
{
	for (INT SeqIndex = 0 ; SeqIndex < AnimStack.Num(); SeqIndex++)
	{
		if ( AnimStack(SeqIndex).bIsPlaying && TickSequence( DeltaTime, AnimStack(SeqIndex) ) )
		{
			eventUIAnimEnd( SeqIndex );
		}
	}
}

/**
 * Decide if we need to apply this animation
 *
 * @Param DeltaTime			How much time since the last call
 * @Param AnimSeqRef		*OUTPUT* The AnimSeq begin applied
 *
 * @Returns true if the animation has reached the end.
 */

UBOOL UUIObject::TickSequence(FLOAT DeltaTime, FUIAnimSeqRef& AnimSeqRef)
{

	UUIAnimationSeq* Seq = AnimSeqRef.SeqRef;
	UUIObject* AnimTarget = this;
	UBOOL bDoneAnimating = FALSE;

	// Quick out of we happen to be stopped
	if ( !AnimSeqRef.bIsPlaying )
	{
		return true;
	}

	// Step 1.  Figure out where we want to be in the sequence

	FLOAT AnimTime = AnimSeqRef.AnimTime + (DeltaTime * AnimSeqRef.PlaybackRate);
	FLOAT Position = AnimTime / Seq->SeqDuration;

	for (INT TrackIdx = 0 ; TrackIdx < Seq->Tracks.Num(); TrackIdx++)
	{

		// Step 2.  Find the frames we wall upon.
		UBOOL bSkipNormalUpdate = false;
		INT LFI = 0;
		INT NFI = 0;

		for (INT i=0; i<Seq->Tracks(TrackIdx).KeyFrames.Num(); i++)
		{
			if (Seq->Tracks(TrackIdx).KeyFrames(i).TimeMark <= Position )
			{
				LFI = i;
			}
			else
			{
				break;
			}
		}

		NFI = LFI + 1;

		// If the next frame index is beyond the track, we loop around
		if ( NFI >= Seq->Tracks(TrackIdx).KeyFrames.Num() )
		{
			NFI = 0;
		}

		// Step 3. We test to see if we need to stop/loop.

		if ( Position >= 1.0f )
		{
			// If we are not looping or only have 1 frame, quick exit;

			if ( !AnimSeqRef.bIsLooping || Seq->Tracks(TrackIdx).KeyFrames.Num() == 1 )
			{

				// We assume we are looping so setup the LFI to point to the last frame
				LFI = Seq->Tracks(TrackIdx).KeyFrames.Num() - 1;

				// Check to see if the Last Track is at Position 0.  If it's not, use the first track.

				if ( AnimSeqRef.SeqRef->Tracks(TrackIdx).KeyFrames(LFI).TimeMark < 1.0 )
				{
					LFI = 0;
				}

				UUIObject* TargetObj = Seq->Tracks(TrackIdx).TargetWidget;

				// Force the track to the final frame.
				Seq->ApplyAnimation(TargetObj? TargetObj : this, TrackIdx, 1.0, LFI, LFI, AnimSeqRef);
				
				bSkipNormalUpdate = true;
				bDoneAnimating = true;
			}
			
			if(AnimSeqRef.bIsLooping)
			{
				AnimSeqRef.LoopCount++;
			}
		}
		
		if(bSkipNormalUpdate==false)
		{

			// At this point LFI = the last frame we hit and NFI = the next frame we want to hit.
			// If NFI < LFI then we are looping.  

			// Step 4.  Apply the Animations.

			UUIObject* TargetObj = Seq->Tracks(TrackIdx).TargetWidget;

			// Calculate where we are within the frame
			FLOAT FramePos = Position - Seq->Tracks(TrackIdx).KeyFrames(LFI).TimeMark;
			if (LFI <= NFI )
			{
				FramePos /=  (Seq->Tracks(TrackIdx).KeyFrames(NFI).TimeMark - Seq->Tracks(TrackIdx).KeyFrames(LFI).TimeMark);
			}
			else
			{
				FramePos /= 1.0 - Seq->Tracks(TrackIdx).KeyFrames(LFI).TimeMark;
			}

			Seq->ApplyAnimation(TargetObj ? TargetObj : this, TrackIdx, FramePos, LFI, NFI, AnimSeqRef);
		}
	}

	// Step 5. Store the animation time and get on with it

	AnimSeqRef.AnimTime = appFmod(AnimTime , Seq->SeqDuration);

	return bDoneAnimating;
}

void UUIObject::AnimSetOpacity(FLOAT NewOpacity)
{
	Opacity = NewOpacity;
}
				
void UUIObject::AnimSetVisibility(UBOOL bIsVisible)
{
	eventSetVisibility(bIsVisible);
}

void UUIObject::AnimSetColor(FLinearColor NewColor)
{
}

void UUIObject::AnimSetPosition(FVector NewPosition)
{

	// TODO: Add code to set the position matrix here
	AnimationPosition = NewPosition;

	FLOAT NewLeft = RenderBounds[UIFACE_Left];
	FLOAT NewTop = RenderBounds[UIFACE_Top];

	FVector2D ViewportSize;
	GetViewportSize(ViewportSize);

	NewLeft = ViewportSize.X * NewPosition.X;
	NewTop = ViewportSize.Y * NewPosition.Y;

	if ( NewLeft != RenderBounds[UIFACE_Left] )
	{
		SetPosition(NewLeft, UIFACE_Left, EVALPOS_PixelViewport);
	}

	if ( NewTop != RenderBounds[UIFACE_Top] )
	{
		SetPosition(NewTop, UIFACE_Top, EVALPOS_PixelViewport);
	}
}

void UUIObject::AnimSetRelPosition(FVector NewPosition, FVector InitialRenderOffset)
{
	AnimationPosition = NewPosition;

	FLOAT Width = GetPosition(UIFACE_Right, EVALPOS_PixelViewport) - GetPosition(UIFACE_Left, EVALPOS_PixelViewport);
	FLOAT Height = GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport) - GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
	
	FLOAT DeltaX = Width * NewPosition.X;
	FLOAT DeltaY = Height * NewPosition.Y;
	FLOAT DeltaZ = ZDepth * NewPosition.Z;

	RenderOffset = InitialRenderOffset + FVector(DeltaX, DeltaY, DeltaZ);
}

void UUIObject::AnimSetRotation(FRotator NewRotation)
{
	Rotation.Rotation = NewRotation;
	UpdateRotationMatrix();
}

void UUIObject::AnimSetScale(FLOAT NewScale)
{
}

/* ==========================================================================================================
	UUILabel
========================================================================================================== */
/**
 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
 * once the scene has been completely initialized.
 * For widgets added at runtime, called after the widget has been inserted into its parent's
 * list of children.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUILabel::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	if ( StringRenderComponent != NULL )
	{
		TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
		StringRenderComponent->InitializeComponent(&Subscriber);
	}

	Super::Initialize(inOwnerScene, inOwner);
}

/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the LabelBackground (if non-NULL) to the StyleSubscribers array.
 */
void UUILabel::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	// add the string component to the list of style resolvers
	VALIDATE_COMPONENT(StringRenderComponent);
	AddStyleSubscriber(StringRenderComponent);

	// Add the label background to the list of style resolvers
	VALIDATE_COMPONENT(LabelBackground);
	AddStyleSubscriber(LabelBackground);
}

/**
 * Adds the specified face to the DockingStack for the specified widget
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUILabel::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	return StringRenderComponent ? StringRenderComponent->AddDockingNode(DockingStack, Face) : Super::AddDockingNode(DockingStack, Face);
}

/**
 * Change the value of this label at runtime.
 *
 * @param	NewText		the new text that should be displayed in the label
 */
void UUILabel::SetValue( const FString& NewText )
{
	if ( StringRenderComponent != NULL && StringRenderComponent->GetValue(FALSE) != NewText )
	{
		StringRenderComponent->SetValue(NewText);
	}
}

/**
 * Evaluates the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUILabel::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ResolveFacePosition(Face);
	}
}

/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUILabel::InvalidatePositionDependencies( BYTE Face )
{
	Super::InvalidatePositionDependencies(Face);
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->InvalidatePositionDependencies(Face);
	}
}

/**
 * Render this widget.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUILabel::Render_Widget(FCanvas* Canvas)
{
	if ( LabelBackground != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
			);

		LabelBackground->RenderComponent(Canvas, Parameters);
	}

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->Render_String(Canvas);
	}
}

/**
 * Called when a property is modified that could potentially affect the widget's position onscreen.
 */
void UUILabel::RefreshPosition()
{
	Super::RefreshPosition();
	RefreshFormatting();
}

/**
 * Called to globally update the formatting of all UIStrings.
 */
void UUILabel::RefreshFormatting()
{
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ReapplyFormatting();
	}
}

/**
 * Sets the text alignment for the string that the widget is rendering.
 *
 * @param	Horizontal		Horizontal alignment to use for text, UIALIGN_MAX means no change.
 * @param	Vertical		Vertical alignment to use for text, UIALIGN_MAX means no change.
 */
void UUILabel::SetTextAlignment(BYTE Horizontal, BYTE Vertical)
{
	if ( StringRenderComponent != NULL )
	{
		if( Horizontal != UIALIGN_MAX )
		{
			StringRenderComponent->SetAlignment(UIORIENT_Horizontal, Horizontal);
		}

		if(Vertical != UIALIGN_MAX)
		{
			StringRenderComponent->SetAlignment(UIORIENT_Vertical, Vertical);
		}
	}
}

/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
void UUILabel::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( MarkupText != DataSource.MarkupString )
	{
		Modify();
		DataSource.MarkupString = MarkupText;
	
		RefreshSubscriberValue(BindingIndex);
	}

}

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUILabel::GetDataStoreBinding( INT BindingIndex/*=-1*/ ) const
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}
	return DataSource.MarkupString;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUILabel::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( StringRenderComponent != NULL && IsInitialized() )
	{
		// resolve the value of this label into a renderable string
		StringRenderComponent->SetValue(DataSource.MarkupString);
		StringRenderComponent->ReapplyFormatting();
		bResult = TRUE;
	}

	return TRUE;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUILabel::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->GetResolvedDataStores(out_BoundDataStores);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUILabel::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}


/* === UObject interface === */
/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUILabel::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("LabelBackground") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty && LabelBackground != NULL )
				{
					// the user either cleared the value of the LabelBackground or is assigning a new value to it.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(LabelBackground);
				}
			}
			else if ( PropertyName == TEXT("StringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty && StringRenderComponent != NULL )
				{
					// the user either cleared the value of the StringRenderComponent or is assigning a new value to it.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(StringRenderComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value from a member struct or array has been changed in the editor.
 */
void UUILabel::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UBOOL bHandled = FALSE;
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("DataSource") )
			{
				bHandled = TRUE;
				if ( RefreshSubscriberValue() && StringRenderComponent != NULL )
				{
					if (StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Horizontal)
					||	StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Vertical)
					||	StringRenderComponent->GetWrapMode() != CLIP_None )
					{
						RefreshPosition();
					}
				}
			}
			else if ( PropertyName == TEXT("LabelBackground") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the ImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( LabelBackground != NULL )
					{
						UUIComp_DrawImage* LabelBackgroundTemplate = GetArchetype<UUILabel>()->LabelBackground;
						if ( LabelBackgroundTemplate != NULL )
						{
							LabelBackground->StyleResolverTag = LabelBackgroundTemplate->StyleResolverTag;
						}
						else
						{
							LabelBackground->StyleResolverTag = TEXT("Label Background Style");
						}

						// user added created a new label component background - add it to the list of style subscribers
						AddStyleSubscriber(LabelBackground);

						// now initialize the component's image
						LabelBackground->SetImage(LabelBackground->GetImage());
					}
				}
				else if ( LabelBackground != NULL )
				{
					// a property of the LabelBackground was changed
// 					if ( ModifiedProperty->GetFName() == TEXT("ImageRef") && LabelBackground->GetImage() != NULL )
// 					{
// 						USurface* CurrentValue = LabelBackground->GetImage();
// 
// 						// changed the value of the image texture/material
// 						// clear the data store binding
// 						//@fixme ronp - do we always need to clear the data store binding?
// 						SetDataStoreBinding(TEXT(""));
// 
// 						// clearing the data store binding value may have cleared the value of the image component's texture,
// 						// so restore the value now
// 						SetValue(CurrentValue);
// 					}
				}
			}
			else if ( PropertyName == TEXT("StringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the ImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( StringRenderComponent != NULL )
					{
						UUIComp_DrawString* StringComponentTemplate = GetArchetype<UUILabel>()->StringRenderComponent;
						if ( StringComponentTemplate != NULL )
						{
							StringRenderComponent->StyleResolverTag = StringComponentTemplate->StyleResolverTag;
						}

						// user added created a new string render component - add it to the list of style subscribers
						AddStyleSubscriber(StringRenderComponent);

						// now initialize the new string component
						TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
						StringRenderComponent->InitializeComponent(&Subscriber);

						// then initialize its style
						StringRenderComponent->NotifyResolveStyle(GetActiveSkin(), FALSE, GetCurrentState());

						// finally initialize its text
						RefreshSubscriberValue();
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after this object has been completely de-serialized.  This version migrates the PrimaryStyle for this label over to the label's component.
 */
void UUILabel::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake && PrimaryStyle.AssignedStyleID.IsValid() )
	{
		checkf(StringRenderComponent, TEXT("%s still has a valid value for PrimaryStyle, but no StringRenderComponent to migrate settings to"), *GetWidgetPathName());
		StringRenderComponent->StringStyle = PrimaryStyle;

		// now clear the StyleID for this label's style.
		PrimaryStyle.SetStyleID(FSTYLE_ID(EC_EventParm));
	}
}

/**
 * We override AnimSetColor and tell the String Render component to override the color 
 */
void UUILabel::AnimSetColor(FLinearColor NewColor)
{
	StringRenderComponent->SetColor(NewColor);
}

/* ==========================================================================================================
	UUIImage
========================================================================================================== */
/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the ImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUIImage::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(ImageComponent);
	AddStyleSubscriber(ImageComponent);
}

/**
 * Render this widget.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUIImage::Render_Widget( FCanvas* Canvas )
{
	if ( ImageComponent != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
			);

		ImageComponent->RenderComponent(Canvas, Parameters);
	}
}

/**
 * Changes this UIImage's Image to the specified surface, creates a wrapper [UITexture/UIMaterial] for the surface
 * (if necessary) and applies the current ImageStyle to the new wrapper.
 *
 * @param	NewImage		the new surface to use for this UIImage
 */
void UUIImage::SetValue( USurface* NewImage )
{
	if ( ImageComponent != NULL )
	{
		ImageComponent->SetImage(NewImage);
	}
}

/** UIDataSourceSubscriber interface */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
void UUIImage::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=INDEX_NONE*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( ImageDataSource.MarkupString != MarkupText )
	{
		Modify();
		ImageDataSource.MarkupString = MarkupText;

		RefreshSubscriberValue(BindingIndex);
	}

}

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUIImage::GetDataStoreBinding( INT BindingIndex/*=INDEX_NONE*/ ) const
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}
	return ImageDataSource.MarkupString;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUIImage::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( ImageDataSource.ResolveMarkup( this ) )
	{
		FUIProviderFieldValue ResolvedValue(EC_EventParm);
		if ( ImageDataSource.GetBindingValue(ResolvedValue) && ResolvedValue.ImageValue != NULL )
		{
			SetValue(ResolvedValue.ImageValue);
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUIImage::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);
	if ( ImageDataSource )
	{
		out_BoundDataStores.AddUniqueItem(*ImageDataSource);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUIImage::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	// clear data stores for child widgets
	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUIImage::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	FUIProviderScriptFieldValue Value(EC_EventParm);
	if ( ImageComponent != NULL )
	{
		Value.ImageValue = ImageComponent->GetImage();
	}

	// add our data store to the list of data stores to call OnCommit for
	GetBoundDataStores(out_BoundDataStores);
	return ImageDataSource.SetBindingValue(Value);
}

/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIImage::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("ImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty && ImageComponent != NULL )
				{
					// the user either cleared the value of the ImageComponent (which should never happen since
					// we use the 'noclear' keyword on the property declaration), or is assigning a new value to the ImageComponent.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(ImageComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIImage::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("ImageDataSource") )
			{
				RefreshSubscriberValue();
			}
			else if ( PropertyName == TEXT("ImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the ImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( ImageComponent != NULL )
					{
						UUIComp_DrawImage* ComponentTemplate = GetArchetype<UUIImage>()->ImageComponent;
						if ( ComponentTemplate != NULL )
						{
							ImageComponent->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}

						// user added created a new image component background - add it to the list of style subscribers
						AddStyleSubscriber(ImageComponent);

						// now initialize the component's image
						ImageComponent->SetImage(ImageComponent->GetImage());
					}
				}
				else if ( ImageComponent != NULL )
				{
					// a property of the ImageComponent was changed
					if ( ModifiedProperty->GetFName() == TEXT("ImageRef") && ImageComponent->GetImage() != NULL )
					{
						USurface* CurrentValue = ImageComponent->GetImage();

						// changed the value of the image texture/material
						// clear the data store binding
						//@fixme ronp - do we always need to clear the data store binding?
						SetDataStoreBinding(TEXT(""));

						// clearing the data store binding value may have cleared the value of the image component's texture,
						// so restore the value now
						SetValue(CurrentValue);
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated Image & Coordinates
 * properties over to the ImageComponent.
 */
void UUIImage::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake && GetLinkerVersion() < VER_FIXED_UISCROLLBARS )
	{
		MigrateImageSettings(Image, Coordinates, PrimaryStyle, ImageComponent, TEXT("Image"));
	}
}

/**
 * We override AnimSetColor and tell the ImageComponent component to override the color 
 */
void UUIImage::AnimSetColor(FLinearColor NewColor)
{
	ImageComponent->SetColor(NewColor);
}


/* ==========================================================================================================
	UIButton
========================================================================================================== */
/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUIButton::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(BackgroundImageComponent);
	AddStyleSubscriber(BackgroundImageComponent);
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUIButton::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddItem(UIKEY_Clicked);
}

/**
 * Render this widget.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUIButton::Render_Widget( FCanvas* Canvas )
{
	if ( BackgroundImageComponent != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
			);

		BackgroundImageComponent->RenderComponent(Canvas, Parameters);
	}
}

/**
 * Handles input events for this button.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIButton::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	
	// Pressed Event
	if ( EventParms.InputAliasName == UIKEY_Clicked )
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			const UBOOL bIsDoubleClickPress = EventParms.EventType==IE_DoubleClick;

			// notify unrealscript
			if ( DELEGATE_IS_SET(OnPressed) )
			{
				delegateOnPressed(this, EventParms.PlayerIndex);
			}

			if ( bIsDoubleClickPress && DELEGATE_IS_SET(OnDoubleClick) )
			{
				delegateOnDoubleClick(this, EventParms.PlayerIndex);
			}
		
			// activate the pressed state
			ActivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			if ( bIsDoubleClickPress )
			{
				ActivateEventByClass(EventParms.PlayerIndex, UUIEvent_OnDoubleClick::StaticClass(), this);
			}
			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Repeat )
		{
			// Play the ClickedCue
			PlayUISound(ClickedCue,EventParms.PlayerIndex);

			if ( DELEGATE_IS_SET(OnPressRepeat) )
			{
				delegateOnPressRepeat(this, EventParms.PlayerIndex);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released )
		{
			if ( DELEGATE_IS_SET(OnPressRelease) )
			{
				delegateOnPressRelease(this, EventParms.PlayerIndex);
			}

			if ( IsPressed(EventParms.PlayerIndex) )
			{
				// Play the ClickedCue
				PlayUISound(ClickedCue,EventParms.PlayerIndex);

				// Fire OnPressed Delegate
				FVector2D MousePos(0,0);				
				UBOOL bInputConsumed = FALSE;
				if ( !IsCursorInputKey(EventParms.InputKeyName) || !GetCursorPosition(MousePos, GetScene()) || ContainsPoint(MousePos) )
				{
					if ( DELEGATE_IS_SET(OnClicked) )
					{
						bInputConsumed = delegateOnClicked(this, EventParms.PlayerIndex);
					}

					// activate the on click event
					if( !bInputConsumed )
					{
						ActivateEventByClass(EventParms.PlayerIndex,UUIEvent_OnClick::StaticClass(), this);
					}
				}

				// deactivate the pressed state
				DeactivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			}

			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
}


/**
 * Changes the background image for this button, creating the wrapper UITexture if necessary.
 *
 * @param	NewImage		the new surface to use for this UIImage
 * @param	NewCoordinates	the optional coordinates for use with texture atlasing
 */
void UUIButton::SetImage( USurface* NewImage )
{
	if ( BackgroundImageComponent != NULL )
	{
		BackgroundImageComponent->SetImage(NewImage);
	}
}

/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIButton::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("BackgroundImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty && BackgroundImageComponent != NULL )
				{
					// the user either cleared the value of the BackgroundImageComponent (which should never happen since
					// we use the 'noclear' keyword on the property declaration), or is assigning a new value to the BackgroundImageComponent.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(BackgroundImageComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIButton::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
// 			if ( PropertyName == TEXT("ImageDataSource") )
// 			{
// 				RefreshSubscriberValue();
// 			}
// 			else if ( PropertyName == TEXT("ImageComponent") )
			if ( PropertyName == TEXT("BackgroundImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( BackgroundImageComponent != NULL )
					{
						UUIComp_DrawImage* ComponentTemplate = GetArchetype<UUIButton>()->BackgroundImageComponent;
						if ( ComponentTemplate != NULL )
						{
							BackgroundImageComponent->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}
						else
						{
							BackgroundImageComponent->StyleResolverTag = TEXT("Background Image Style");
						}

						// user created a new background image component - add it to the list of style subscribers
						AddStyleSubscriber(BackgroundImageComponent);

						// now initialize the component's image
						BackgroundImageComponent->SetImage(BackgroundImageComponent->GetImage());
					}
				}
				else if ( BackgroundImageComponent != NULL )
				{
					// a property of the ImageComponent was changed
					if ( ModifiedProperty->GetFName() == TEXT("ImageRef") && BackgroundImageComponent->GetImage() != NULL )
					{
#if 0
						USurface* CurrentValue = BackgroundImageComponent->GetImage();

						// changed the value of the image texture/material
						// clear the data store binding
						//@fixme ronp - do we always need to clear the data store binding?
 						SetDataStoreBinding(TEXT(""));

						// clearing the data store binding value may have cleared the value of the image component's texture,
						// so restore the value now
						SetImage(CurrentValue);
#endif
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated Image & Coordinates
 * properties over to the ImageComponent.
 */
void UUIButton::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake && GetLinkerVersion() < VER_FIXED_UISCROLLBARS )
	{
		MigrateImageSettings(ButtonBackground, Coordinates, PrimaryStyle, BackgroundImageComponent, TEXT("ButtonBackground"));
	}
}

/* ==========================================================================================================
	UUILabelButton
========================================================================================================== */

/**
 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
 * once the scene has been completely initialized.
 * For widgets added at runtime, called after the widget has been inserted into its parent's
 * list of children.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUILabelButton::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	if ( StringRenderComponent != NULL )
	{
		TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
		StringRenderComponent->InitializeComponent(&Subscriber);
	}

	Super::Initialize(inOwnerScene, inOwner);
}

/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the button caption component (if non-NULL) to the StyleSubscribers array.
 */
void UUILabelButton::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	// add the string component to the list of style resolvers
	VALIDATE_COMPONENT(StringRenderComponent);
	AddStyleSubscriber(StringRenderComponent);
}

/**
 * Adds the specified face to the DockingStack for the specified widget
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUILabelButton::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	return StringRenderComponent 
		? StringRenderComponent->AddDockingNode(DockingStack, Face)
		: Super::AddDockingNode(DockingStack, Face);
}

/**
 * Evaluates the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUILabelButton::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ResolveFacePosition(Face);
	}
}

/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUILabelButton::InvalidatePositionDependencies( BYTE Face )
{
	Super::InvalidatePositionDependencies(Face);
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->InvalidatePositionDependencies(Face);
	}
}

/**
 * Sets the caption for this button.
 *
 * @param	NewText		the new caption or markup for the button
 */
void UUILabelButton::SetCaption( const FString& NewText )
{
	if ( StringRenderComponent != NULL && StringRenderComponent->GetValue(FALSE) != NewText )
	{
		StringRenderComponent->SetValue(NewText);
	}
}

/**
 * Render this widget.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUILabelButton::Render_Widget( FCanvas* Canvas )
{
	Super::Render_Widget(Canvas);

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->Render_String(Canvas);
	}
}

/**
 * Called when a property is modified that could potentially affect the widget's position onscreen.
 */
void UUILabelButton::RefreshPosition()
{
	Super::RefreshPosition();

	RefreshFormatting();
}

/**
 * Called to globally update the formatting of all UIStrings.
 */
void UUILabelButton::RefreshFormatting()
{
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ReapplyFormatting();
	}
}

/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
void UUILabelButton::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( CaptionDataSource.MarkupString != MarkupText )
	{
		Modify();
		CaptionDataSource.MarkupString = MarkupText;

		RefreshSubscriberValue(BindingIndex);
	}

}

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUILabelButton::GetDataStoreBinding( INT BindingIndex/*=-1*/ ) const
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}
	return CaptionDataSource.MarkupString;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUILabelButton::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	// resolve the value of this label into a renderable string
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( StringRenderComponent != NULL && IsInitialized() )
	{
		StringRenderComponent->SetValue(CaptionDataSource.MarkupString);
		StringRenderComponent->ReapplyFormatting();
	}

	return TRUE;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUILabelButton::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->GetResolvedDataStores(out_BoundDataStores);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUILabelButton::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUILabelButton::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	FUIProviderScriptFieldValue Value(EC_EventParm);
	if ( StringRenderComponent != NULL )
	{
		Value.StringValue = StringRenderComponent->GetValue(TRUE);
		
		// get the list of data stores referenced by the UIString
		GetBoundDataStores(out_BoundDataStores);
	}

	// add the data store bound to this label button
	if ( CaptionDataSource.ResolvedDataStore != NULL )
	{
		out_BoundDataStores.AddUniqueItem(CaptionDataSource.ResolvedDataStore);
	}

	return CaptionDataSource.SetBindingValue(Value);
}

/**
 * Sets the text alignment for the string that the widget is rendering.
 *
 * @param	Horizontal		Horizontal alignment to use for text, UIALIGN_MAX means no change.
 * @param	Vertical		Vertical alignment to use for text, UIALIGN_MAX means no change.
 */
void UUILabelButton::SetTextAlignment(BYTE Horizontal, BYTE Vertical)
{
	if ( StringRenderComponent != NULL )
	{
		if( Horizontal != UIALIGN_MAX )
		{
			StringRenderComponent->SetAlignment(UIORIENT_Horizontal, Horizontal);
		}

		if(Vertical != UIALIGN_MAX)
		{
			StringRenderComponent->SetAlignment(UIORIENT_Vertical, Vertical);
		}
	}
}

/* === UObject interface === */
/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUILabelButton::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("StringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty && StringRenderComponent != NULL )
				{
					// the user either cleared the value of the StringRenderComponent or is assigning a new value to it.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(StringRenderComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUILabelButton::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("CaptionDataSource") )
			{
				if ( RefreshSubscriberValue() )
				{
					if (StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Horizontal)
					||	StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Vertical)
					||	StringRenderComponent->GetWrapMode() != CLIP_None )
					{
						RefreshPosition();
					}
				}
			}
			else if ( PropertyName == TEXT("StringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the ImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( StringRenderComponent != NULL )
					{
						// user added created a new string render component - add it to the list of style subscribers
						AddStyleSubscriber(StringRenderComponent);

						// now initialize the new string component
						TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
						StringRenderComponent->InitializeComponent(&Subscriber);

						// then initialize its style
						StringRenderComponent->NotifyResolveStyle(GetActiveSkin(), FALSE, GetCurrentState());

						// finally initialize its text
						RefreshSubscriberValue();
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after this object has been completely de-serialized.  This version migrates the PrimaryStyle for this label over to the label's component.
 */
void UUILabelButton::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake && PrimaryStyle.AssignedStyleID.IsValid() )
	{
		checkf(StringRenderComponent, TEXT("%s still has a valid value for PrimaryStyle, but no StringRenderComponent to migrate settings to"), *GetWidgetPathName());
		StringRenderComponent->StringStyle = PrimaryStyle;

		// now clear the StyleID for this label's style.
		PrimaryStyle.SetStyleID(FSTYLE_ID(EC_EventParm));
	}
}

/* ==========================================================================================================
	UUICheckbox
========================================================================================================== */
/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the ImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUICheckbox::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(CheckedImageComponent);
	AddStyleSubscriber(CheckedImageComponent);
}

/**
 * Render this checkbox.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUICheckbox::Render_Widget( FCanvas* Canvas )
{
	Super::Render_Widget( Canvas );

	if ( bIsChecked && CheckedImageComponent != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
			);

		CheckedImageComponent->RenderComponent( Canvas, Parameters );
	}
}

/**
 * Handles input events for this button.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUICheckbox::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	if(EventParms.InputAliasName == UIKEY_Clicked)
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			const UBOOL bIsDoubleClickPress = EventParms.EventType==IE_DoubleClick;

			// notify unrealscript
			if ( DELEGATE_IS_SET(OnPressed) )
			{
				delegateOnPressed(this, EventParms.PlayerIndex);
			}

			if ( bIsDoubleClickPress && DELEGATE_IS_SET(OnDoubleClick) )
			{
				delegateOnDoubleClick(this, EventParms.PlayerIndex);
			}

			// activate the pressed state
			ActivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			if ( bIsDoubleClickPress )
			{
				ActivateEventByClass(EventParms.PlayerIndex, UUIEvent_OnDoubleClick::StaticClass(), this);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Repeat )
		{
			// Play the ClickedCue
			PlayUISound(ClickedCue,EventParms.PlayerIndex);

			if ( DELEGATE_IS_SET(OnPressRepeat) )
			{
				delegateOnPressRepeat(this, EventParms.PlayerIndex);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released )
		{
			if ( DELEGATE_IS_SET(OnPressRelease) )
			{
				delegateOnPressRelease(this, EventParms.PlayerIndex);
			}

			if ( IsPressed(EventParms.PlayerIndex) )
			{
				// Play the ClickedCue
				PlayUISound(ClickedCue,EventParms.PlayerIndex);

				// toggle the checked state of this checkbox
				SetValue(!bIsChecked,EventParms.PlayerIndex);

				// Fire OnPressed Delegate
				FVector2D MousePos(0,0);				
				UBOOL bInputConsumed = FALSE;
				if ( !IsCursorInputKey(EventParms.InputKeyName) || !GetCursorPosition(MousePos, GetScene()) || ContainsPoint(MousePos) )
				{
					if ( DELEGATE_IS_SET(OnClicked) )
					{
						bInputConsumed = delegateOnClicked(this, EventParms.PlayerIndex);
					}

					// activate the on click event
					if( !bInputConsumed )
					{
						ActivateEventByClass(EventParms.PlayerIndex,UUIEvent_OnClick::StaticClass(), this);
					}
				}

				// deactivate the pressed state
				DeactivateStateByClass(UUIState_Pressed::StaticClass(), EventParms.PlayerIndex);
			}

			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
}

/**
 * Changes the checked image for this checkbox, creating the wrapper UITexture if necessary.
 *
 * @param	NewImage		the new surface to use for this UIImage
 */
void UUICheckbox::SetCheckImage( USurface* NewImage )
{
	if ( CheckedImageComponent != NULL )
	{
		CheckedImageComponent->SetImage(NewImage);
	}
}

/**
 * Changed the checked state of this checkbox and activates a checked event.
 *
 * @param	bShouldBeChecked	TRUE to turn the checkbox on, FALSE to turn it off
 * @param	PlayerIndex			the index of the player that generated the call to SetValue; used as the PlayerIndex when activating
 *								UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 */
void UUICheckbox::SetValue( UBOOL bShouldBeChecked, INT PlayerIndex/*=INDEX_NONE*/ )
{
	if ( bIsChecked != bShouldBeChecked )
	{
		bIsChecked = bShouldBeChecked;

		TArray<INT> OutputLinkIndexes;
		OutputLinkIndexes.AddItem(bIsChecked);

		if ( PlayerIndex == INDEX_NONE )
		{
			PlayerIndex = GetBestPlayerIndex();
		}

		// activate the OnCheckValue changed event to notify anyone who cares that this checkbox's value has been changed
		TArray<UUIEvent_CheckValueChanged*> ActivatedEvents;
		ActivateEventByClass(PlayerIndex, UUIEvent_CheckValueChanged::StaticClass(), this, FALSE, &OutputLinkIndexes, (TArray<UUIEvent*>*)&ActivatedEvents);
		for ( INT EventIdx = 0; EventIdx < ActivatedEvents.Num(); EventIdx++ )
		{
			UUIEvent_CheckValueChanged* Event = ActivatedEvents(EventIdx);

			// copy the current value of the checkbox into the "New Value" variable link
			TArray<UBOOL*> BoolVars;
			Event->GetBoolVars(BoolVars,TEXT("Value"));

			for (INT Idx = 0; Idx < BoolVars.Num(); Idx++)
			{
				*(BoolVars(Idx)) = bIsChecked;
			}
		}
	}
}

/** UIDataSourceSubscriber interface */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
void UUICheckbox::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=INDEX_NONE*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( ValueDataSource.MarkupString != MarkupText )
	{
		Modify();
		ValueDataSource.MarkupString = MarkupText;

		RefreshSubscriberValue(BindingIndex);
	}

}

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUICheckbox::GetDataStoreBinding( INT BindingIndex/*=INDEX_NONE*/ ) const
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}
	return ValueDataSource.MarkupString;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUICheckbox::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( ValueDataSource.ResolveMarkup( this ) )
	{
		FUIProviderFieldValue ResolvedValue(EC_EventParm);
		if ( ValueDataSource.GetBindingValue(ResolvedValue) && ResolvedValue.StringValue.Len() > 0 )
		{
			if ( ResolvedValue.StringValue == TEXT("1") || ResolvedValue.StringValue == TEXT("True") || ResolvedValue.StringValue == GTrue )
			{
				SetValue( TRUE );
				bResult = TRUE;
			}
			else if ( ResolvedValue.StringValue == TEXT("0") ||	ResolvedValue.StringValue == TEXT("False") || ResolvedValue.StringValue == GFalse )
			{
				SetValue(FALSE);
				bResult = TRUE;
			}
		}
	}


	return bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUICheckbox::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);
	if ( ValueDataSource )
	{
		out_BoundDataStores.AddUniqueItem(*ValueDataSource);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUICheckbox::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUICheckbox::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	FUIProviderScriptFieldValue Value(EC_EventParm);
	Value.StringValue = bIsChecked ? GTrue : GFalse;

	GetBoundDataStores(out_BoundDataStores);

	return ValueDataSource.SetBindingValue(Value);
}

/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUICheckbox::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("CheckedImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty && CheckedImageComponent != NULL )
				{
					// the user either cleared the value of the CheckedImageComponent (which should never happen since
					// we use the 'noclear' keyword on the property declaration), or is assigning a new value to the CheckedImageComponent.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(CheckedImageComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUICheckbox::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("ValueDataSource") )
			{
				RefreshSubscriberValue();
			}
			else if ( PropertyName == TEXT("CheckedImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( CheckedImageComponent != NULL )
					{
						UUIComp_DrawImage* ComponentTemplate = GetArchetype<UUICheckbox>()->CheckedImageComponent;
						if ( ComponentTemplate != NULL )
						{
							CheckedImageComponent->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}
						else
						{
							CheckedImageComponent->StyleResolverTag = TEXT("Check Style");
						}

						// user added created a new image component background - add it to the list of style subscribers
						AddStyleSubscriber(CheckedImageComponent);

						// now initialize the component's image
						CheckedImageComponent->SetImage(CheckedImageComponent->GetImage());
					}
				}
				else if ( CheckedImageComponent != NULL )
				{
					// a property of the ImageComponent was changed
					if ( ModifiedProperty->GetFName() == TEXT("ImageRef") && CheckedImageComponent->GetImage() != NULL )
					{
						USurface* CurrentValue = CheckedImageComponent->GetImage();

						// changed the value of the image texture/material
						// clear the data store binding
						//@fixme ronp - do we always need to clear the data store binding?
						SetDataStoreBinding(TEXT(""));

						// clearing the data store binding value may have cleared the value of the image component's texture,
						// so restore the value now
						SetCheckImage(CurrentValue);
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated Image & Coordinates
 * properties over to the ImageComponent.
 */
void UUICheckbox::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake && GetLinkerVersion() < VER_FIXED_UISCROLLBARS )
	{
		MigrateImageSettings(CheckedImage, CheckCoordinates, CheckStyle, CheckedImageComponent, TEXT("CheckedImage"));
	}
}

/* ==========================================================================================================
	UUIToggleButton
========================================================================================================== */


/**
 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
 * once the scene has been completely initialized.
 * For widgets added at runtime, called after the widget has been inserted into its parent's
 * list of children.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUIToggleButton::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	if ( CheckedStringRenderComponent != NULL )
	{
		TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
		CheckedStringRenderComponent->InitializeComponent(&Subscriber);
	}

	Super::Initialize(inOwnerScene, inOwner);
}

/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the button caption component (if non-NULL) to the StyleSubscribers array.
 */
void UUIToggleButton::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	// add the checked components to the list of style resolvers
	VALIDATE_COMPONENT(CheckedStringRenderComponent);
	AddStyleSubscriber(CheckedStringRenderComponent);

	VALIDATE_COMPONENT(CheckedBackgroundImageComponent);
	AddStyleSubscriber(CheckedBackgroundImageComponent);
}

/**
 * Adds the specified face to the DockingStack for the specified widget
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUIToggleButton::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	UBOOL bResult = CheckedStringRenderComponent && CheckedStringRenderComponent->AddDockingNode(DockingStack, Face);
	return Super::AddDockingNode(DockingStack, Face) || bResult;
}

/**
 * Evaluates the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUIToggleButton::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	if ( CheckedStringRenderComponent != NULL )
	{
		CheckedStringRenderComponent->ResolveFacePosition(Face);
	}
}

/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUIToggleButton::InvalidatePositionDependencies( BYTE Face )
{
	Super::InvalidatePositionDependencies(Face);
	if ( CheckedStringRenderComponent != NULL )
	{
		CheckedStringRenderComponent->InvalidatePositionDependencies(Face);
	}
}

/**
 * Sets the caption for this button.
 *
 * @param	NewText		the new caption or markup for the button
 */
void UUIToggleButton::SetCaption( const FString& NewText )
{
	Super::SetCaption(NewText);

	if ( CheckedStringRenderComponent != NULL && CheckedStringRenderComponent->GetValue(FALSE) != NewText )
	{
		CheckedStringRenderComponent->SetValue(NewText);
	}
}


/**
 * Render this widget.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUIToggleButton::Render_Widget( FCanvas* Canvas )
{
	if(bIsChecked)	// Checked Button
	{
		if ( CheckedBackgroundImageComponent != NULL )
		{
			FRenderParameters Parameters(
				RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
				RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
				RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
			);

			CheckedBackgroundImageComponent->RenderComponent(Canvas, Parameters);
		}

		if ( CheckedStringRenderComponent != NULL )
		{
			CheckedStringRenderComponent->Render_String(Canvas);
		}
	}
	else	// Non-Checked Button
	{
		if ( BackgroundImageComponent != NULL )
		{
			FRenderParameters Parameters(
				RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
				RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
				RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
			);

			BackgroundImageComponent->RenderComponent(Canvas, Parameters);
		}

		if ( StringRenderComponent != NULL )
		{
			StringRenderComponent->Render_String(Canvas);
		}
	}
}

/**
 * Handles input events for this button.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIToggleButton::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	if(EventParms.InputAliasName == UIKEY_Clicked)
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			const UBOOL bIsDoubleClickPress = EventParms.EventType==IE_DoubleClick;

			// notify unrealscript
			if ( DELEGATE_IS_SET(OnPressed) )
			{
				delegateOnPressed(this, EventParms.PlayerIndex);
			}

			if ( bIsDoubleClickPress && DELEGATE_IS_SET(OnDoubleClick) )
			{
				delegateOnDoubleClick(this, EventParms.PlayerIndex);
			}

			// activate the pressed state
			ActivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			if ( bIsDoubleClickPress )
			{
				ActivateEventByClass(EventParms.PlayerIndex, UUIEvent_OnDoubleClick::StaticClass(), this);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released )
		{
			if ( DELEGATE_IS_SET(OnPressRelease) )
			{
				delegateOnPressRelease(this, EventParms.PlayerIndex);
			}

			if ( IsPressed(EventParms.PlayerIndex) )
			{
				// toggle the checked state of this checkbox
				SetValue(!bIsChecked,EventParms.PlayerIndex);

				// Fire OnPressed Delegate
				FVector2D MousePos(0,0);				
				UBOOL bInputConsumed = FALSE;
				if ( !IsCursorInputKey(EventParms.InputKeyName) || !GetCursorPosition(MousePos, GetScene()) || ContainsPoint(MousePos) )
				{
					if ( DELEGATE_IS_SET(OnClicked) )
					{
						bInputConsumed = delegateOnClicked(this, EventParms.PlayerIndex);
					}

					// activate the on click event
					if( !bInputConsumed )
					{
						// activate the on click event
						ActivateEventByClass(EventParms.PlayerIndex,UUIEvent_OnClick::StaticClass(), this);
					}
				}

				// deactivate the pressed state
				DeactivateStateByClass(UUIState_Pressed::StaticClass(), EventParms.PlayerIndex);
			}

			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
}


/**
 * Changed the checked state of this togglebutton and activates a checked event.
 *
 * @param	bShouldBeChecked	TRUE to turn the checkbox on, FALSE to turn it off
 * @param	PlayerIndex			the index of the player that generated the call to SetValue; used as the PlayerIndex when activating
 *								UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 */
void UUIToggleButton::SetValue( UBOOL bShouldBeChecked, INT PlayerIndex/*=INDEX_NONE*/ )
{
	if ( bIsChecked != bShouldBeChecked )
	{
		bIsChecked = bShouldBeChecked;

		TArray<INT> OutputLinkIndexes;
		OutputLinkIndexes.AddItem(bIsChecked);

		if ( PlayerIndex == INDEX_NONE )
		{
			PlayerIndex = GetBestPlayerIndex();
		}

		// activate the OnCheckValue changed event to notify anyone who cares that this checkbox's value has been changed
		TArray<UUIEvent_CheckValueChanged*> ActivatedEvents;
		ActivateEventByClass(PlayerIndex, UUIEvent_CheckValueChanged::StaticClass(), this, FALSE, &OutputLinkIndexes, (TArray<UUIEvent*>*)&ActivatedEvents);
		for ( INT EventIdx = 0; EventIdx < ActivatedEvents.Num(); EventIdx++ )
		{
			UUIEvent_CheckValueChanged* Event = ActivatedEvents(EventIdx);

			// copy the current value of the checkbox into the "New Value" variable link
			TArray<UBOOL*> BoolVars;
			Event->GetBoolVars(BoolVars,TEXT("Value"));

			for (INT Idx = 0; Idx < BoolVars.Num(); Idx++)
			{
				*(BoolVars(Idx)) = bIsChecked;
			}
		}
	}
}

/**
 * Called to globally update the formatting of all UIStrings.
 */
void UUIToggleButton::RefreshFormatting()
{
	if ( CheckedStringRenderComponent != NULL )
	{
		CheckedStringRenderComponent->ReapplyFormatting();
	}

	Super::RefreshFormatting();
}


/** UIDataSourceSubscriber interface */
/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUIToggleButton::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	// Check value
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( ValueDataSource.ResolveMarkup( this ) )
	{
		FUIProviderFieldValue ResolvedValue(EC_EventParm);
		if ( ValueDataSource.GetBindingValue(ResolvedValue) && ResolvedValue.StringValue.Len() > 0 )
		{
			if ( ResolvedValue.StringValue == TEXT("1") || ResolvedValue.StringValue == TEXT("True") || ResolvedValue.StringValue == GTrue )
			{
				SetValue( TRUE );
				bResult = TRUE;
			}
			else if ( ResolvedValue.StringValue == TEXT("0") ||	ResolvedValue.StringValue == TEXT("False") || ResolvedValue.StringValue == GFalse )
			{
				SetValue(FALSE);
				bResult = TRUE;
			}
		}
	}

	// resolve the value of this label into a renderable string
	if ( CheckedStringRenderComponent != NULL && IsInitialized() )
	{
		CheckedStringRenderComponent->SetValue(CaptionDataSource.MarkupString);
		CheckedStringRenderComponent->ReapplyFormatting();
	}

	bResult = Super::RefreshSubscriberValue(BindingIndex) || bResult;

	return bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUIToggleButton::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	if ( ValueDataSource )
	{
		out_BoundDataStores.AddUniqueItem(*ValueDataSource);
	}

	Super::GetBoundDataStores(out_BoundDataStores);
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUIToggleButton::ClearBoundDataStores()
{
	Super::ClearBoundDataStores();

	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUIToggleButton::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = Super::SaveSubscriberValue(out_BoundDataStores, BindingIndex);

	FUIProviderScriptFieldValue Value(EC_EventParm);
	Value.StringValue = bIsChecked ? GTrue : GFalse;

	GetBoundDataStores(out_BoundDataStores);

	return ValueDataSource.SetBindingValue(Value) || bResult;
}

/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIToggleButton::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("CheckedStringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty && CheckedStringRenderComponent != NULL )
				{
					// the user either cleared the value of the StringRenderComponent or is assigning a new value to it.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(CheckedStringRenderComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIToggleButton::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("ValueDataSource") )
			{
				RefreshSubscriberValue();
			}
			else if ( PropertyName == TEXT("CheckedStringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty && CheckedStringRenderComponent != NULL )
				{
					// the user either cleared the value of the StringRenderComponent or is assigning a new value to it.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(CheckedStringRenderComponent);
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}


/* ==========================================================================================================
	UUIEditBox
========================================================================================================== */
/**
 * Initializes the button and creates the background image.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUIEditBox::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	if ( StringRenderComponent != NULL )
	{
		TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
		StringRenderComponent->InitializeComponent(&Subscriber);
	}

	Super::Initialize(inOwnerScene, inOwner);
}

/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUIEditBox::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	// add the string component to the list of style resolvers
	VALIDATE_COMPONENT(StringRenderComponent);
	AddStyleSubscriber(StringRenderComponent);

	// Add the editbox background to the list of style resolvers
	VALIDATE_COMPONENT(BackgroundImageComponent);
	AddStyleSubscriber(BackgroundImageComponent);
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUIEditBox::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddItem(UIKEY_Clicked);
	out_KeyNames.AddItem(UIKEY_SubmitText);
	out_KeyNames.AddItem(UIKEY_Char);
	out_KeyNames.AddItem(UIKEY_Backspace);
	out_KeyNames.AddItem(UIKEY_DeleteCharacter);
	out_KeyNames.AddItem(UIKEY_MoveCursorLeft);
	out_KeyNames.AddItem(UIKEY_MoveCursorRight);
	out_KeyNames.AddItem(UIKEY_MoveCursorToLineStart);
	out_KeyNames.AddItem(UIKEY_MoveCursorToLineEnd);
}

/**
 * Adds the specified face to the DockingStack for the specified widget
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUIEditBox::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	if ( StringRenderComponent != NULL )
	{
		return StringRenderComponent->AddDockingNode(DockingStack, Face);
	}

	return Super::AddDockingNode(DockingStack,Face);
}

/**
 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUIEditBox::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ResolveFacePosition(Face);
	}
}

/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUIEditBox::InvalidatePositionDependencies( BYTE Face )
{
	Super::InvalidatePositionDependencies(Face);
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->InvalidatePositionDependencies(Face);
	}
}

/**
 * Render this button.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUIEditBox::Render_Widget( FCanvas* Canvas )
{
	if ( BackgroundImageComponent != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
			);

		BackgroundImageComponent->RenderComponent(Canvas, Parameters);
	}

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->Render_String(Canvas);
	}
}

/**
 * Removes the character located to the right of the caret position.
 *
 * @return	TRUE if a character was successfully removed, FALSE if the caret was at the end of the string, there weren't
 *			any characters to remove, or the character couldn't be removed for some reason.
 */
UBOOL UUIEditBox::DeleteCharacter( INT PlayerIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	if ( StringRenderComponent != NULL )
	{
		FString UserText = StringRenderComponent->GetValue(TRUE);
		if ( UserText.Len() > 0 )
		{
			INT CaretPosition = StringRenderComponent->StringCaret.CaretPosition;
			if ( CaretPosition < UserText.Len() )
			{
				// caret is just to the left of the last character in the string
				if ( CaretPosition == UserText.Len() - 1 )
				{
					SetValue(UserText.LeftChop(1), PlayerIndex);
				}
				else
				{
					// caret is in the middle of the string somewhere
					SetValue( UserText.Left(CaretPosition) + UserText.Mid(CaretPosition+1), PlayerIndex );
				}
				bResult = TRUE;
			}
		}
	}

	return bResult;
}

/**
 * Handles non-text entry input events for this button.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIEditBox::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;

//	debugfSuppressed(NAME_Input, TEXT("%s::ProcessInputKey	Key:%s (%s)"), *GetWidgetPathName(), *EventParms.InputKeyName.ToString(), *UUIRoot::GetInputEventText(EventParms.EventType));
	if (EventParms.EventType == IE_Pressed
	||	EventParms.EventType == IE_Repeat
	||	EventParms.EventType == IE_Released 
	||	EventParms.EventType == IE_DoubleClick )
	{
		bResult = ProcessControlChar(EventParms);
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
}

/**
 * Processes special characters that affect the editbox but aren't displayed in the text field, such 
 * as arrow keys, backspace, delete, etc.
 *
 * Only called if this widget is in the owning scene's InputSubscriptions map for the KEY_Character key.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE if the character was processed, FALSE if it wasn't one a special character.
 */
UBOOL UUIEditBox::ProcessControlChar( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	if ( StringRenderComponent != NULL )
	{
		if ( EventParms.InputAliasName == UIKEY_Clicked )
		{
			switch( EventParms.EventType )
			{
			case IE_Pressed:
			case IE_DoubleClick:
				// notify unrealscript
				if ( DELEGATE_IS_SET(OnPressed) )
				{
					delegateOnPressed(this, EventParms.PlayerIndex);
				}
				if ( EventParms.EventType == IE_DoubleClick && DELEGATE_IS_SET(OnDoubleClick) )
				{
					delegateOnDoubleClick(this, EventParms.PlayerIndex);
				}

				// activate the pressed state
				ActivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
				if ( EventParms.EventType == IE_DoubleClick )
				{
					ActivateEventByClass(EventParms.PlayerIndex, UUIEvent_OnDoubleClick::StaticClass(), this);
				}
				bResult = TRUE;
				break;

			case IE_Repeat:
				// Play the ClickedCue
				//PlayUISound(ClickedCue,EventParms.PlayerIndex);

				if ( DELEGATE_IS_SET(OnPressRepeat) )
				{
					delegateOnPressRepeat(this, EventParms.PlayerIndex);
				}

				bResult = TRUE;
			    break;

			case IE_Released:
				{
					if ( DELEGATE_IS_SET(OnPressRelease) )
					{
						delegateOnPressRelease(this, EventParms.PlayerIndex);
					}

					if ( IsPressed(EventParms.PlayerIndex) )
					{
						// Play the ClickedCue
						//PlayUISound(ClickedCue,EventParms.PlayerIndex);

						// Fire OnPressed Delegate
						FVector2D MousePos(0,0);				
						UBOOL bInputConsumed = FALSE;
						if ( !IsCursorInputKey(EventParms.InputKeyName) || !GetCursorPosition(MousePos, GetScene()) || ContainsPoint(MousePos) )
						{
							if ( DELEGATE_IS_SET(OnClicked) )
							{
								bInputConsumed = delegateOnClicked(this, EventParms.PlayerIndex);
							}

							// activate the on click event
							if( !bInputConsumed )
							{
								ActivateEventByClass(EventParms.PlayerIndex,UUIEvent_OnClick::StaticClass(), this);
							}
						}

						// deactivate the pressed state
						DeactivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
					}
					bResult = TRUE;
					break;
				}
			}
		}
		// IE_Released/DoubleClick is only handled for click input
		else if ( StringRenderComponent != NULL && EventParms.EventType != IE_Released && EventParms.EventType != IE_DoubleClick )
		{
			const INT UserTextLength = StringRenderComponent->GetUserTextLength();
			if ( EventParms.InputAliasName == UIKEY_SubmitText )
			{
				bResult = TRUE;
				NotifySubmitText(EventParms.PlayerIndex);
			}
			else if ( EventParms.InputAliasName == UIKEY_MoveCursorLeft )
			{
				INT CaretPosition = StringRenderComponent->StringCaret.CaretPosition;
				if ( CaretPosition > 0 )
				{
					INT NewCaretPosition = CaretPosition - 1;

					// see if the user is holding down the ctrl key; if so, we'll move the caret to the next non-alphanumeric character
					// in password mode, we'll always move by one character
					if ( !bPasswordMode && IsHoldingCtrl(EventParms.PlayerIndex) )
					{
						NewCaretPosition = FindPreviousCaretJumpPosition();
					}
					StringRenderComponent->SetCaretPosition(NewCaretPosition);
				}

				bResult = TRUE;
			}
			else if ( EventParms.InputAliasName == UIKEY_MoveCursorRight )
			{
				INT CaretPosition = StringRenderComponent->StringCaret.CaretPosition;
				if ( StringRenderComponent->StringCaret.CaretPosition < UserTextLength )
				{
					INT NewCaretPosition = CaretPosition + 1;

					// see if the user is holding down the ctrl key; if so, we'll move the caret to the next non-alphanumeric character
					// in password mode, we'll always move by one character
					if ( !bPasswordMode && IsHoldingCtrl(EventParms.PlayerIndex) )
					{
						NewCaretPosition = FindNextCaretJumpPosition();
					}
					StringRenderComponent->SetCaretPosition(NewCaretPosition);
				}

				bResult = TRUE;
			}
			else if ( EventParms.InputAliasName == UIKEY_MoveCursorToLineStart )
			{
				StringRenderComponent->SetCaretPosition(0);
				bResult = TRUE;
			}
			else if ( EventParms.InputAliasName == UIKEY_MoveCursorToLineEnd )
			{
				StringRenderComponent->SetCaretPosition( UserTextLength );
				bResult = TRUE;
			}
			else if ( !bReadOnly )
			{
				if(EventParms.InputAliasName == UIKEY_Char)
				{
					// Consume any input that causes character events.
					bResult = TRUE;
				}
				else if ( EventParms.InputAliasName == UIKEY_Backspace )
				{
					if ( StringRenderComponent->StringCaret.CaretPosition > 0 )
					{
						StringRenderComponent->SetCaretPosition(StringRenderComponent->StringCaret.CaretPosition-1);
						DeleteCharacter(EventParms.PlayerIndex);
					}

					bResult = TRUE;
				}
				else if ( EventParms.InputAliasName == UIKEY_DeleteCharacter )
				{
					DeleteCharacter(EventParms.PlayerIndex);
					bResult = TRUE;
				}
			}
		}
	}

	return bResult;
}

/**
 * Adds the specified character to the editbox's text field, if the text field is currently eligible to receive
 * new characters
 *
 * Only called if this widget is in the owning scene's InputSubscriptions map for the KEY_Unicode key.
 *
 * @param	PlayerIndex		index [into the Engine.GamePlayers array] of the player that generated this event
 * @param	Character		the character that was received
 *
 * @return	TRUE to consume the character, false to pass it on.
 */
UBOOL UUIEditBox::ProcessInputChar( INT PlayerIndex, TCHAR Character )
{
	UBOOL bResult = FALSE;

//	debugf(NAME_Input, TEXT("%s::ProcessInputChar	Character:%c (%d)"), *GetWidgetPathName(), Character, Character);
	if ( StringRenderComponent != NULL )
	{
		if ( !bReadOnly || Character == 3 /*Ctrl-C*/ )
		{
			const FString UserText = StringRenderComponent->GetValue();

			// if this editbox has a limitation on the number of characters, check to see whether we're at the max already
			if ( MaxCharacters == 0 || UserText.Len() < MaxCharacters )
			{
				if ( IsValidCharacter(Character) )
				{
					INT CaretPosition = StringRenderComponent->StringCaret.CaretPosition;
					if ( CaretPosition == UserText.Len() )
					{
						SetValue(UserText + Character, PlayerIndex);
					}
					else
					{
						SetValue(UserText.Left(CaretPosition) + Character + UserText.Mid(CaretPosition), PlayerIndex);
					}

					StringRenderComponent->SetCaretPosition(StringRenderComponent->StringCaret.CaretPosition+1);
					bResult = TRUE;
				}
				else
				{
					//@todo - add support for selecting substrings of the text
					switch ( Character )
					{
					case 3:		// Ctrl-C
						appClipboardCopy( *UserText );
						bResult = TRUE;
						break;

					case 22:	//	Ctrl-V
						{
							INT CaretPosition = StringRenderComponent->StringCaret.CaretPosition;
							if ( CaretPosition == UserText.Len() )
							{
								SetValue( UserText + appClipboardPaste(), PlayerIndex );
							}
							else
							{
								SetValue(UserText.Left(CaretPosition) + appClipboardPaste() + UserText.Mid(CaretPosition), PlayerIndex);
							}
						}
						bResult = TRUE;
						break;

					case 24:	//	Ctrl-X
						appClipboardCopy(*UserText);
						SetValue(TEXT(""), PlayerIndex);
						bResult = TRUE;
						break;

					// @todo - accelerator keys (Ctrl+A, Ctrl-Z)
					}

					// send a notification that the user attempted to enter an invalid character
					//@todo - play a sound?  fire an event?
				}
			}
			else
			{
				// send a notification that the max character limit has been reached
				//@todo - play a sound?  fire an event?
			}
		}
		else
		{
			// send a notification that the user attempted to change the value of a read-only editbox
			//@todo - play a sound?  fire an event?
		}
	}

	return bResult;
}

/**
 * Determine whether the specified character should be displayed in the text field.
 */
UBOOL UUIEditBox::IsValidCharacter( TCHAR Character ) const
{
	UBOOL bResult = FALSE;

	switch(CharacterSet)
	{
		case CHARSET_NoSpecial:
		{
			// @todo ronp - is appIsPunct the correct method to call to meet the intent of this CharacterSet value?
			bResult = !appIsPunct(Character);
			break;
		}
		case CHARSET_All:
		{
			//@fixme - for now, any character code higher than 31 (32 is the first alphanumeric character - space) is allowed (except the NULL character)
			bResult = Character >= TCHAR(' ') && Character != 127;
			break;
		}

		case CHARSET_AlphaOnly:
		{
			bResult = appIsAlpha(Character);
			break;
		}

		case CHARSET_NumericOnly:
		{
			//@todo ronp - we need a few more of these types of functions
			// i.e. appIsNumeric (which returns TRUE for - and . as well), etc.
			bResult = appIsDigit(Character) || Character == TCHAR('.') || Character == TCHAR('-') || Character == TCHAR('+');
			break;
		}

		default:
		{
			break;
		}
	}

	return bResult;
}

/**
 * Calculates the position of the first non-alphanumeric character that appears before the current caret position.
 *
 * @param	StartingPosition	the location [index into UserText] to start searching.  If not specified, the
 *								current caret position is used.  The character located at StartingPosition will
 *								NOT be considered in the search.
 *
 * @return	an index into UserText for the location of the first non-alphanumeric character that appears
 *			before the current caret position, or 0 if the beginning of the string is reached first.
 */
INT UUIEditBox::FindPreviousCaretJumpPosition( INT StartingPosition/*=INDEX_NONE*/ ) const
{
	INT Result = 0;
	if ( StringRenderComponent != NULL )
	{
		if ( StartingPosition == INDEX_NONE )
		{
			StartingPosition = StringRenderComponent->StringCaret.CaretPosition;
		}

		// this function should never be called if the caret is at 0 position; if it is, it's a bug
		// so let's verify that now
		if ( StartingPosition > 0 && StringRenderComponent->GetUserTextLength() > 0 )
		{
			// grab the raw array so that we can index easily
			const FString& UserText = StringRenderComponent->GetUserTextRef();
			TArray<TCHAR>& CharArray = const_cast<FString&>(UserText).GetCharArray();

			// if the character to the left of the caret is not alpha, we want to stop when
			// the character to the left of the caret is an alpha character, and vice versa
			UBOOL bStopAtAlpha = !appIsAlpha( CharArray(StartingPosition-1) );

			// now move the cursor position back until we've found the right character
			for ( INT SearchPosition = StartingPosition; CharArray.IsValidIndex(SearchPosition - 1); SearchPosition-- )
			{
				TCHAR CurrentChar = CharArray(SearchPosition - 1);
				if ( appIsAlpha(CurrentChar) == bStopAtAlpha )
				{
					Result = SearchPosition;
					break;
				}
			}
		}
	}

	return Result;
}

/**
 * Calculates the position of the first non-alphanumeric character that appears after the current caret position.
 *
 * @param	StartingPosition	the location [index into UserText] to start searching.  If not specified, the
 *								current caret position is used.  The character located at StartingPosition will
 *								NOT be considered in the search.
 *
 * @return	an index into UserText for the location of the first non-alphanumeric character that appears
 *			after the current caret position, or the length of the string if the end of the string is reached first.
 */
INT UUIEditBox::FindNextCaretJumpPosition( INT StartingPosition/*=INDEX_NONE*/ ) const
{
	INT Result = 0;
	if ( StringRenderComponent != NULL )
	{
		const INT UserTextLength = StringRenderComponent->GetUserTextLength();
		Result = UserTextLength;
		if ( StartingPosition == INDEX_NONE )
		{
			StartingPosition = StringRenderComponent->StringCaret.CaretPosition;
		}

		// this function should never be called if the caret is at 0 position; if it is, it's a bug
		// so let's verify that now
		if ( UserTextLength > 0 && StartingPosition < UserTextLength )
		{
			// grab the raw array so that we can index easily
			const FString& UserText = StringRenderComponent->GetUserTextRef();
			TArray<TCHAR>& CharArray = const_cast<FString&>(UserText).GetCharArray();

			// if the character to the right of the caret is not alpha, we want to stop when
			// the character to the right of the caret is an alpha character, and vice versa
			UBOOL bStopAtAlpha = !appIsAlpha( CharArray(StartingPosition) );

			// We're going to start looking at the first character after the current one
			for ( INT SearchPosition = StartingPosition; CharArray.IsValidIndex(SearchPosition); SearchPosition++ )
			{
				TCHAR CurrentChar = CharArray(SearchPosition);
				if ( appIsAlpha(CurrentChar) == bStopAtAlpha )
				{
					Result = SearchPosition;
					break;
				}
			}
		}
	}

	return Result;
}

/**
 * Called when a property is modified that could potentially affect the widget's position onscreen.
 */
void UUIEditBox::RefreshPosition()
{
	Super::RefreshPosition();

	RefreshFormatting();
}

/**
 * Called to globally update the formatting of all UIStrings.
 */
void UUIEditBox::RefreshFormatting()
{
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ReapplyFormatting();
	}
}

/**
 * Called whenever the editbox's text is modified.  Activates the TextValueChanged kismet event and calls the OnValueChanged
 * delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to SetValue; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
 */
void UUIEditBox::NotifyValueChanged( INT PlayerIndex/*=INDEX_NONE*/, INT NotifyFlags/*=0*/ )
{
	if ( PlayerIndex == INDEX_NONE )
	{
		PlayerIndex = GetBestPlayerIndex();
	}

	Super::NotifyValueChanged(PlayerIndex, NotifyFlags);

	FString NewValue = GetValue(TRUE);
	TArray<UUIEvent_TextValueChanged*> ActivatedEvents;
	ActivateEventByClass(PlayerIndex, UUIEvent_TextValueChanged::StaticClass(), this, FALSE, NULL, (TArray<UUIEvent*>*)&ActivatedEvents);
	for ( INT EventIdx = 0; EventIdx < ActivatedEvents.Num(); EventIdx++ )
	{
		UUIEvent_TextValueChanged* Event = ActivatedEvents(EventIdx);

		// copy the current value of the editbox into the "Value" variable link
		TArray<FString*> StringVars;
		Event->GetStringVars(StringVars,TEXT("Value"));

		for (INT Idx = 0; Idx < StringVars.Num(); Idx++)
		{
			*(StringVars(Idx)) = NewValue;
		}
	}
}

/**
 * Called whenever the user presses enter while this editbox is focused.  Activated the SubmitText kismet event and calls the
 * OnSubmitText delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to SetValue; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 */
void UUIEditBox::NotifySubmitText( INT PlayerIndex/*=INDEX_NONE*/ )
{
	UBOOL bClearValue = FALSE;

	if ( PlayerIndex == INDEX_NONE )
	{
		PlayerIndex = GetBestPlayerIndex();
	}

	if ( DELEGATE_IS_SET(OnSubmitText) )
	{
		bClearValue = delegateOnSubmitText(this, PlayerIndex);
	}

	TArray<UUIEvent*> SubmitTextEvents;
	ActivateEventByClass(PlayerIndex, UUIEvent_SubmitTextData::StaticClass(), this, FALSE, NULL, &SubmitTextEvents);
	for ( INT EventIndex = 0; EventIndex < SubmitTextEvents.Num(); EventIndex++ )
	{
		UUIEvent_SubmitTextData* TextEvent = CastChecked<UUIEvent_SubmitTextData>(SubmitTextEvents(EventIndex));

		// set the event's string Value to the current value of the editbox.  this value will be copied into the
		// event's variable links via InitializeLinkedVariableValues().
		TextEvent->Value = GetValue(TRUE);
		TextEvent->PopulateLinkedVariableValues();

		if ( TextEvent->bClearValue )
		{
			bClearValue = TRUE;
		}
	}

	if ( bClearValue )
	{
		// clear the current value of the editbox
		SetValue(TEXT(""), PlayerIndex);
	}
}

/**
 * Changes the background image for this widget, creating the wrapper UITexture if necessary
 * and applies the current ImageStyle to the new wrapper.
 *
 * @param	NewImage		the new surface to use for this UIImage
 */
void UUIEditBox::SetBackgroundImage( USurface* NewImage )
{
	if ( BackgroundImageComponent != NULL )
	{
		BackgroundImageComponent->SetImage(NewImage);
	}
}

/**
 * Change the value of this editbox at runtime.
 *
 * @param	NewText			the new text that should be displayed in the label
 * @param	PlayerIndex		the index of the player that generated the call to SetValue; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 */
void UUIEditBox::SetValue( const FString& NewText, INT PlayerIndex/*=INDEX_NONE*/, UBOOL bSkipNotification/*=FALSE*/ )
{
	if (StringRenderComponent != NULL
	&&	StringRenderComponent->SetUserText(NewText)
	&&	!bSkipNotification )
	{
		NotifyValueChanged(PlayerIndex);
	}
}

/**
 * Gets the text that is currently in this editbox.
 *
 * @param	bReturnUserText		if TRUE, returns the text typed into the editbox by the user;  if FALSE, returns the resolved value
 *								of this editbox's data store binding.
 */
FString UUIEditBox::GetValue( UBOOL bReturnUserText/*=TRUE*/ )
{
	//@NOTE: NEVER call StringRenderComponent->GetValue(FALSE) from this method, since that just calls this method
	if ( bReturnUserText && StringRenderComponent != NULL )
	{
		return StringRenderComponent->GetValue(TRUE);
	}
	else if ( DataSource )
	{
		FUIProviderFieldValue DataStoreValue(EC_EventParm);
		if ( DataSource.GetBindingValue(DataStoreValue) )
		{
			return DataStoreValue.StringValue;
		}
	}

	return TEXT("");
}

/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
void UUIEditBox::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( DataSource.MarkupString != MarkupText )
	{
		Modify();
		DataSource.MarkupString = MarkupText;

		RefreshSubscriberValue(BindingIndex);
	}

}

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUIEditBox::GetDataStoreBinding( INT BindingIndex/*=-1*/ ) const
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}

	return DataSource.MarkupString;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUIEditBox::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( StringRenderComponent != NULL && IsInitialized() )
	{
		// if we have a data store binding or we're trying to clear the data store binding, use the markup string
		if ( DataSource.MarkupString.Len() > 0 || DataSource )
		{
			StringRenderComponent->SetValue(DataSource.MarkupString);
		}
		else
		{
			SetValue(InitialValue, GetBestPlayerIndex());
		}
		bResult = TRUE;
	}

	return FALSE;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUIEditBox::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);

	// add the data store that the editbox is bound to
	if ( DataSource )
	{
		out_BoundDataStores.AddUniqueItem(*DataSource);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUIEditBox::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}

	//@todo ronp - notify the string render component to detach from the image data store (caret)
}

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUIEditBox::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	FUIProviderScriptFieldValue Value(EC_EventParm);
	Value.PropertyType = DATATYPE_Property;
	Value.StringValue = GetValue(TRUE);

	GetBoundDataStores(out_BoundDataStores);

	return DataSource.SetBindingValue(Value);
}

/* === UObject interface === */
/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIEditBox::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("BackgroundImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty && BackgroundImageComponent != NULL )
				{
					// the user either cleared the value of the BackgroundImageComponent (which should never happen since
					// we use the 'noclear' keyword on the property declaration), or is assigning a new value to the BackgroundImageComponent.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(BackgroundImageComponent);
				}
			}
			else if ( PropertyName == TEXT("StringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty && StringRenderComponent != NULL )
				{
					// the user either cleared the value of the StringRenderComponent or is assigning a new value to it.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(StringRenderComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIEditBox::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("DataSource") )
			{
				if ( RefreshSubscriberValue() && StringRenderComponent != NULL )
				{
					if (StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Horizontal)
					||	StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Vertical)
					||	StringRenderComponent->GetWrapMode() != CLIP_None )
					{
						RefreshPosition();
					}
				}

				if ( DataSource.MarkupString.Len() == 0 && InitialValue.Len() != 0 )
				{
					SetValue(InitialValue);
					if (StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Horizontal)
					||	StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Vertical)
					||	StringRenderComponent->GetWrapMode() != CLIP_None )
					{
						RefreshPosition();
					}
				}
			}
			else if ( PropertyName == TEXT("InitialValue") )
			{
				if ( StringRenderComponent != NULL && DataSource.MarkupString.Len() == 0 )
				{
					SetValue(InitialValue);
					if (StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Horizontal)
					||	StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Vertical)
					||	StringRenderComponent->GetWrapMode() != CLIP_None )
					{
						RefreshPosition();
					}
				}
			}
			else if ( PropertyName == TEXT("BackgroundImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( BackgroundImageComponent != NULL )
					{
						UUIComp_DrawImage* ComponentTemplate = GetArchetype<UUIEditBox>()->BackgroundImageComponent;
						if ( ComponentTemplate != NULL )
						{
							BackgroundImageComponent->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}
						else
						{
							BackgroundImageComponent->StyleResolverTag = TEXT("Background Image Style");
						}

						// user created a new background image component - add it to the list of style subscribers
						AddStyleSubscriber(BackgroundImageComponent);

						// now initialize the component's image
						BackgroundImageComponent->SetImage(BackgroundImageComponent->GetImage());
					}
				}
				else if ( BackgroundImageComponent != NULL )
				{
					// a property of the ImageComponent was changed
					if ( ModifiedProperty->GetFName() == TEXT("ImageRef") && BackgroundImageComponent->GetImage() != NULL )
					{
#if 0
						USurface* CurrentValue = BackgroundImageComponent->GetImage();

						// changed the value of the image texture/material
						// clear the data store binding
						//@fixme ronp - do we always need to clear the data store binding?
						SetDataStoreBinding(TEXT(""));

						// clearing the data store binding value may have cleared the value of the image component's texture,
						// so restore the value now
						SetBackgroundImage(CurrentValue);
#endif
					}
				}
			}
			else if ( PropertyName == TEXT("StringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( StringRenderComponent != NULL )
					{
						UUIComp_DrawString* ComponentTemplate = GetArchetype<UUIEditBox>()->StringRenderComponent;
						if ( ComponentTemplate != NULL )
						{
							StringRenderComponent->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}

						// user added created a new string render component - add it to the list of style subscribers
						AddStyleSubscriber(StringRenderComponent);

						// now initialize the new string component
						TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
						StringRenderComponent->InitializeComponent(&Subscriber);

						// then initialize its style
// 						StringRenderComponent->SetStringStyle(GetCurrentComboStyle());
						StringRenderComponent->NotifyResolveStyle(GetActiveSkin(), FALSE, GetCurrentState());
						StringRenderComponent->ApplyCaretStyle(GetCurrentState());

						// finally initialize its text
						RefreshSubscriberValue();
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}


/**
 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated PanelBackground,
 * Coordinates, and PrimaryStyle properties over to the BackgroundImageComponent.
 */
void UUIEditBox::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake && GetLinkerVersion() < VER_FIXED_UISCROLLBARS  )
	{
		MigrateImageSettings(EditBoxBackground, Coordinates, PrimaryStyle, BackgroundImageComponent, TEXT("EditBoxBackground"));
		
		if ( PrimaryStyle.AssignedStyleID.IsValid() )
		{
			checkf(StringRenderComponent, TEXT("%s still has a valid value for PrimaryStyle, but no StringRenderComponent to migrate settings to"), *GetWidgetPathName());
			StringRenderComponent->StringStyle = PrimaryStyle;

			// now clear the StyleID for this label's style.
			PrimaryStyle.SetStyleID(FSTYLE_ID(EC_EventParm));
		}
	}
}

/* ==========================================================================================================
	UConsoleEntry
========================================================================================================== */
/**
 * Initializes the button and creates the background image.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UConsoleEntry::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	InputBox->StringRenderComponent->SetWrapMode(CLIP_None);
	InputBox->StringRenderComponent->bIgnoreMarkup = TRUE;
	InputBox->StringRenderComponent->StringCaret.bDisplayCaret = TRUE;
	InputBox->StringRenderComponent->StringCaret.CaretWidth = 1.5f;
	InputBox->StringRenderComponent->StringStyle.DefaultStyleTag = TEXT("ConsoleStyle");
	InputBox->StringRenderComponent->eventEnableAutoSizing(UIORIENT_Vertical);
	InputBox->StringRenderComponent->eventSetAutoSizePadding(UIORIENT_Vertical, 3.f, 1.f, UIEXTENTEVAL_Pixels, UIEXTENTEVAL_Pixels);
	InputBox->BackgroundImageComponent->SetStyleResolverTag(TEXT("Background Image Style"));
	InputBox->BackgroundImageComponent->ImageStyle.DefaultStyleTag = TEXT("ConsoleBufferImageStyle");

	ConsolePromptBackground->ImageComponent->ImageStyle.DefaultStyleTag = TEXT("ConsoleBufferImageStyle");

	ConsolePromptLabel->StringRenderComponent->bIgnoreMarkup = TRUE;
	ConsolePromptLabel->StringRenderComponent->eventEnableAutoSizing(UIORIENT_Horizontal);
	ConsolePromptLabel->StringRenderComponent->StringStyle.DefaultStyleTag = TEXT("ConsoleStyle");

	LowerConsoleBorder->ImageComponent->ImageStyle.DefaultStyleTag = TEXT("ConsoleImageStyle");
	UpperConsoleBorder->ImageComponent->ImageStyle.DefaultStyleTag = TEXT("ConsoleImageStyle");

	Super::Initialize(inOwnerScene, inOwner);
}

/**
 * Perform any additional rendering after this widget's children have been rendered.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UConsoleEntry::PostRender_Widget(FCanvas* Canvas)
{
	if ( InputBox != NULL && bRenderCursor )
		{
		UUIComp_DrawString* StringRenderComponent = InputBox->StringRenderComponent;
		
		//@hack - draw the underline

		// WrappedCursorPosition will be the position of the cursor relative to the beginning of the last line
		INT WrappedCursorPosition = CursorPosition;

		FLOAT CursorYOffset=0;
		INT LineIndex;

		checkSlow(StringRenderComponent->ValueString);
		TArray<FUIStringNode*>& StringNodes = StringRenderComponent->ValueString->Nodes;
		INT InputLineCount = StringNodes.Num();
		for ( LineIndex = 0; LineIndex < InputLineCount - 1; LineIndex++ )
		{
			FUIStringNode*& CurrentNode = StringNodes(LineIndex);
			const TCHAR* NodeValue = CurrentNode->GetValue(TRUE);
			checkSlow(NodeValue);

			INT LineLength = appStrlen(NodeValue); // add one extra for the whitespace character that caused the line break

			if ( WrappedCursorPosition < LineLength )
			{
				break;
			}

			WrappedCursorPosition -= LineLength;
			CursorYOffset += CurrentNode->Extent.Y;
		}

		FString ConsoleText;
		if ( InputLineCount > 0 )
		{
			FUIStringNode*& LastNode = StringNodes(LineIndex);
			ConsoleText = LastNode->GetValue(TRUE);
		}

		FRenderParameters Parameters;
		check(StringRenderComponent->ValueString->StringStyleData.IsInitialized());
		check(StringRenderComponent->ValueString->StringStyleData.DrawFont);

		Parameters.DrawFont = StringRenderComponent->ValueString->StringStyleData.DrawFont;
		UUIString::StringSize(Parameters, *ConsoleText.Left(WrappedCursorPosition));

		DrawString(
			Canvas,InputBox->RenderBounds[UIFACE_Left] + Parameters.DrawXL,
			InputBox->RenderBounds[UIFACE_Top] + CursorYOffset + InputBox->DockTargets.GetDockPadding(UIFACE_Bottom),
			TEXT("_"), Parameters.DrawFont,
			StringRenderComponent->ValueString->StringStyleData.TextColor
			);
	}
}



/* ==========================================================================================================
	UIScrollbar
========================================================================================================== */
/**
 * Returns the size of the scroll-zone (the region between the decrement and increment buttons), along the same orientation as the scrollbar.
 *
 * @param	ScrollZoneStart	receives the value of the location of the beginning of the scroll zone, in pixels relative to the scrollbar.
 *
 * @return	the height (if the scrollbar's orientation is vertical) or width (if horizontal) of the region between the
 *			increment and decrement buttons, in pixels.
 */
FLOAT UUIScrollbar::GetScrollZoneExtent( FLOAT* ScrollZoneStart/*=NULL*/ ) const
{
	const FLOAT ScrollZoneBegin = DecrementButton->GetPosition(
		ScrollbarOrientation == UIORIENT_Horizontal ? UIFACE_Right : UIFACE_Bottom,
		EVALPOS_PixelViewport, FALSE);
	const FLOAT ScrollZoneEnd = IncrementButton->GetPosition(
		ScrollbarOrientation == UIORIENT_Horizontal ? UIFACE_Left : UIFACE_Top,
		EVALPOS_PixelViewport, FALSE);

	if ( ScrollZoneStart != NULL )
	{
		*ScrollZoneStart = Min(ScrollZoneBegin, ScrollZoneEnd);
	}
	return Max(0.f, ScrollZoneEnd - ScrollZoneBegin);
}
void UUIScrollbar::execGetScrollZoneExtent(FFrame& Stack, RESULT_DECL)
{
	P_GET_FLOAT_OPTX_REF(ScrollZoneStart,0);
	P_FINISH;
	*(FLOAT*)Result=GetScrollZoneExtent(pScrollZoneStart);
}

/**
 * Returns the size of the scroll-zone (the region between the decrement and increment buttons), for the orientation opposite that of the scrollbar.
 *
 * @return	the height (if the scrollbar's orientation is vertical) or width (if horizontal) of the region between the
 *			increment and decrement buttons, in pixels.
 */
FLOAT UUIScrollbar::GetScrollZoneWidth() const
{
	return BarWidth.GetValue(this);
}

/**
 * Initializes the button and creates the bar image.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUIScrollbar::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner /*=NULL*/ )
{
	if ( IncrementButton == NULL )
	{
		IncrementButton = Cast<UUIScrollbarButton>(CreateWidget(this, UUIScrollbarButton::StaticClass(), NULL, TEXT("IncrementButton")));
		check(IncrementButton);
		check(IncrementButton->BackgroundImageComponent);

		// the button background's StyleResolverTag must match the name of the UIStyleReference property from this class
		// which contains the style data that will be used for the increment button or SetWidgetStyle when called on the button
		IncrementButton->BackgroundImageComponent->StyleResolverTag = TEXT("IncrementStyle");
	}
	
	if(DecrementButton == NULL)
	{
		DecrementButton = Cast<UUIScrollbarButton>(CreateWidget(this, UUIScrollbarButton::StaticClass(), NULL, TEXT("DecrementButton")));
		check(DecrementButton);
		check(DecrementButton->BackgroundImageComponent);

		// the button background's StyleResolverTag must match the name of the UIStyleReference property from this class
		// which contains the style data that will be used for the decrement button or SetWidgetStyle when called on the button
		DecrementButton->BackgroundImageComponent->StyleResolverTag = TEXT("DecrementStyle");
	}

	if(MarkerButton == NULL)
	{
		MarkerButton = Cast<UUIScrollbarMarkerButton>(CreateWidget(this, UUIScrollbarMarkerButton::StaticClass(), NULL, TEXT("Marker")));
		check(MarkerButton);

		MarkerButton->Position.SetRawPositionValue(
			ScrollbarOrientation == UIORIENT_Vertical ? UIFACE_Top : UIFACE_Left,
			0.f, EVALPOS_PixelOwner);

		MarkerButton->Position.SetRawPositionValue(
			ScrollbarOrientation == UIORIENT_Vertical ? UIFACE_Bottom : UIFACE_Right,
			MinimumMarkerSize.GetValue(this), EVALPOS_PixelOwner);
	}

	InsertChild( IncrementButton );
	InsertChild( DecrementButton );
	InsertChild( MarkerButton );

	// Because we manage the styles for our buttons, and the buttons' styles are actually stored in their StyleSubscribers, we need to
	// initialize the StyleSubscribers arrays for the buttons prior to calling Super::Initialize() so that the first time OnResolveStyles
	// is called, the buttons will be ready to receive those styles.
	IncrementButton->InitializeStyleSubscribers();
	DecrementButton->InitializeStyleSubscribers();
	MarkerButton->InitializeStyleSubscribers();

	Super::Initialize(inOwnerScene, inOwner);
}

/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUIScrollbar::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(BackgroundImageComponent);
	AddStyleSubscriber(BackgroundImageComponent);
}

/**
 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
 *
 * By default this function recursively calls itself on all of its children.
 */
void UUIScrollbar::PreRenderCallback()
{
	SetupDocLinks(TRUE);

	Super::PreRenderCallback();
}

/**
 * Render this scrollbar.
 *
 * @param	RI	the FRenderInterface to use for rendering this widget
 */
void UUIScrollbar::Render_Widget( FCanvas* Canvas )
{
	if ( BackgroundImageComponent != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
			);

		// only thing we need to render is the scrollzone background
		BackgroundImageComponent->RenderComponent(Canvas, Parameters);
	}
}

/**
* Generates a array of UI Action keys that this widget supports.
*
* @param	out_KeyNames	Storage for the list of supported keynames.
*/
void UUIScrollbar::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddItem(UIKEY_Clicked);
}


/**
 * Determines whether this widget should process the specified input event + state.  If the widget is configured
 * to respond to this combination of input key/state, any actions associated with this input event are activated.
 *
 * Only called if this widget is in the owning scene's InputSubscribers map for the corresponding key.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIScrollbar::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	if ( EventParms.InputAliasName == UIKEY_Clicked )
	{
		if ( EventParms.EventType == IE_Pressed )
		{
			if ( DELEGATE_IS_SET(OnPressed) )
			{
				delegateOnPressed(this, EventParms.PlayerIndex);
			}

			ScrollZoneClicked(EventParms);
			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
}

/**
 * Called when a property is modified that could potentially affect the widget's position onscreen.
 */
void UUIScrollbar::RefreshPosition()
{	
	Super::RefreshPosition();

	RefreshFormatting();
}

/**
 * Called to globally update the formatting of all UIStrings.
 */
void UUIScrollbar::RefreshFormatting()
{
	// Recalculate the region extent since the movement of the frame could have affected it.
	bInitializeMarker = TRUE;
}


/**
 * Returns TRUE if the inner faces of the decrement and increment buttons have both been resolved.
 */
UBOOL UUIScrollbar::CanResolveScrollZoneExtent() const
{
	// these are the faces on each button that face the other end of the scrollzone.
	EUIWidgetFace DecrementInnerFace = ScrollbarOrientation == UIORIENT_Horizontal ? UIFACE_Right : UIFACE_Bottom;
	EUIWidgetFace IncrementInnerFace = GetOppositeFace(DecrementInnerFace);

	UBOOL bResult = FALSE;
	if ( DecrementButton != NULL && !DecrementButton->HasPositionBeenResolved(DecrementInnerFace) )
	{
		bResult = TRUE;
	}
	else if ( IncrementButton != NULL && !IncrementButton->HasPositionBeenResolved(IncrementInnerFace) )
	{
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Calls RefreshMarker() if the increment/decrement buttons' positions aren't up-to-date.
 *
 * @return	TRUE if RefreshMarker() was called.
 */
UBOOL UUIScrollbar::ConditionalRefreshMarker()
{
	if ( !bInitializeMarker && !CanResolveScrollZoneExtent() )
	{
		RefreshMarker();
	}

	return bInitializeMarker;
}

/**
 * Sets a private flag to refresh the bound of the scrollbar
 */
void UUIScrollbar::RefreshMarker()
{
	bInitializeMarker = TRUE;

	if ( GetNumResolvedFaces() != 0 )
	{
		RequestSceneUpdate(FALSE, TRUE);
	}
}

/**
 * Called when a style reference is resolved successfully.
 *
 * @param	ResolvedStyle			the style resolved by the style reference
 * @param	StyleProperty			the name of the style reference property that was resolved.
 * @param	ArrayIndex				the array index of the style reference that was resolved.  should only be >0 for style reference arrays.
 * @param	bInvalidateStyleData	if TRUE, the resolved style is different than the style that was previously resolved by this style reference.
 */
void UUIScrollbar::OnStyleResolved( UUIStyle* ResolvedStyle, const FStyleReferenceId& StylePropertyId, INT ArrayIndex, UBOOL bInvalidateStyleData )
{
	Super::OnStyleResolved(ResolvedStyle,StylePropertyId,ArrayIndex,bInvalidateStyleData);

	FString StylePropertyName = StylePropertyId.GetStyleReferenceName();
	if( StylePropertyName == TEXT("IncrementStyle"))
	{
		// propagate the IncrementStyle to the increment button
		IncrementButton->SetWidgetStyle(ResolvedStyle,StylePropertyId,ArrayIndex);
	}
	else if( StylePropertyName == TEXT("DecrementStyle"))
	{
		// propagate the DecrementStyle to the decrement button
		DecrementButton->SetWidgetStyle(ResolvedStyle,StylePropertyId,ArrayIndex);
	}
	else if( StylePropertyName == TEXT("MarkerStyle"))
	{
		// propagate the MarkerStyle to the marker button
		MarkerButton->SetWidgetStyle(ResolvedStyle,StylePropertyId,ArrayIndex);
	}
}

/**
 * Verifies that marker can be moved by the given PositionChange value. If the PositionChange is too large
 * and would cause the marker to extend beyond the increment or decrement buttons, then it will be clamped to a 
 * value by which will the marker can move and not extend beyond its bar region
 */
FLOAT UUIScrollbar::GetClampedPositionChange(FLOAT PositionChange)
{
	FLOAT ClampedChange = PositionChange;

	//Calculate the position change
	if ( ScrollbarOrientation == UIORIENT_Horizontal )
	{
		if( PositionChange > 0 ) // Direction towards the increment button
		{
			//If the position change would move the marker past the increment button,
			//clamp positionchange to the distance from the marker to the increment button
			const FLOAT MarkerRight = MarkerButton->GetPosition(UIFACE_Right, EVALPOS_PixelViewport);
			const FLOAT IncButtonLeft = IncrementButton->GetPosition(UIFACE_Left, EVALPOS_PixelViewport);

			if( (MarkerRight + PositionChange) - IncButtonLeft > KINDA_SMALL_NUMBER )
			{
				ClampedChange = IncButtonLeft - MarkerRight;
			}			
		}
		else if ( PositionChange < 0)  // Direction towards the decrement button
		{
			//If the position change would move the marker past the decrement button,
			//clamp positionchange to the distance from the marker to the decrement button
			const FLOAT MarkerLeft = MarkerButton->GetPosition(UIFACE_Left, EVALPOS_PixelViewport);
			const FLOAT DecButtonRight = DecrementButton->GetPosition(UIFACE_Right, EVALPOS_PixelViewport);

			if ( DecButtonRight - (MarkerLeft + PositionChange) > KINDA_SMALL_NUMBER )
			{
				ClampedChange = DecButtonRight - MarkerLeft;
			}
		}
		else
		{	
			// do nothing, position will not change
		}
	}
	else
	{
		if ( PositionChange > 0 ) // Direction towards the increment button
		{
			//If the position change would move the marker past the increment button,
			//clamp positionchange to the distance from the marker to the increment button
			const FLOAT MarkerBottom = MarkerButton->GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport);
			const FLOAT IncButtonTop = IncrementButton->GetPosition(UIFACE_Top, EVALPOS_PixelViewport);

			if( (MarkerBottom + PositionChange) - IncButtonTop > KINDA_SMALL_NUMBER )
			{
				ClampedChange = IncButtonTop - MarkerBottom;
			}				
		}
		else if( PositionChange < 0) // Direction towards the decrement button
		{
			//If the position change would move the marker past the decrement button,
			//clamp positionchange to the distance from the marker to the decrement button
			const FLOAT MarkerTop = MarkerButton->GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
			const FLOAT DecButtonBottom = DecrementButton->GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport);

			if ( DecButtonBottom - (MarkerTop + PositionChange) > KINDA_SMALL_NUMBER )
			{
				ClampedChange =  DecButtonBottom - MarkerTop;
			}
		}
		else
		{	
			// do nothing, position will not change
		}
	}

	return ClampedChange;
}

/**
 *	Shifts the position of the marker button by the amount specified, clamps the PositionChange if it would extend the marker pass the increment/decrement buttons
 *  the direction of shift is based on the sign of the PositionChange and the ScrollbarOrientation setting
 * 
 *  @param	PositionChange	the amount of pixels that the marker widget will be shifted by, supply 
 *                          negative value to shift marker in opposite direction
 */
void UUIScrollbar::UpdateMarkerPosition( FLOAT PositionChange )	// MoveGripBy
{
	const FLOAT ValidPositionChange = GetClampedPositionChange(PositionChange);
	UBOOL bPositionMaxedOut; 

//debugf(TEXT("%s::UpdateMarkerPosition:  %f"), *GetName(), PositionChange);
	FLOAT ScrollZoneStart;
	FLOAT ScrollZoneSize = GetScrollZoneExtent(&ScrollZoneStart);

	if(ScrollbarOrientation == UIORIENT_Horizontal)
	{
		//Change the marker's position without triggering a scene update
		FLOAT NewPosition = MarkerButton->GetPosition(UIFACE_Left,EVALPOS_PixelOwner) + ValidPositionChange;
		MarkerButton->Position.SetRawPositionValue( UIFACE_Left, NewPosition, EVALPOS_PixelOwner );

		MarkerButton->InvalidatePosition(UIFACE_Left);
		MarkerButton->ResolveFacePosition(UIFACE_Left);
		MarkerButton->ResolveFacePosition(UIFACE_Right);

		//See if it has reached its extreme position
		bPositionMaxedOut = Abs(MarkerButton->GetPosition(UIFACE_Left,EVALPOS_PixelViewport) - ScrollZoneStart) < KINDA_SMALL_NUMBER
			|| Abs(MarkerButton->GetPosition(UIFACE_Right,EVALPOS_PixelViewport) - (ScrollZoneStart + ScrollZoneSize)) < KINDA_SMALL_NUMBER;
    }
	else
	{
		//Change the marker's position without triggering a scene update
		FLOAT NewPosition = MarkerButton->GetPosition(UIFACE_Top,EVALPOS_PixelOwner) + ValidPositionChange;
		MarkerButton->Position.SetRawPositionValue( UIFACE_Top, NewPosition, EVALPOS_PixelOwner );

		MarkerButton->InvalidatePosition(UIFACE_Top);
		MarkerButton->ResolveFacePosition(UIFACE_Top);
		MarkerButton->ResolveFacePosition(UIFACE_Bottom);

		//See if it has reached its extreme position
		bPositionMaxedOut = Abs(MarkerButton->GetPosition(UIFACE_Top,EVALPOS_PixelViewport) - ScrollZoneStart) < KINDA_SMALL_NUMBER
			|| Abs(MarkerButton->GetPosition(UIFACE_Bottom,EVALPOS_PixelViewport) - (ScrollZoneStart + ScrollZoneSize)) < KINDA_SMALL_NUMBER;
	}

	MousePositionDelta += ValidPositionChange;
	
	if( Abs<FLOAT>(MousePositionDelta) >= NudgeValue )
	{
		FLOAT Nudges = MousePositionDelta / NudgeValue;
		if ( DELEGATE_IS_SET(OnScrollActivity) )
		{
			//@todo ronp - what to do with the return value?
			delegateOnScrollActivity( this, Nudges, bPositionMaxedOut );
		}

		MousePositionDelta = 0.f;
	}

	// If the position was clamped, reset the mouse delta so that we can't trigger the OnScrollActivity delegate the next time around
	if( Abs(ValidPositionChange - PositionChange) > KINDA_SMALL_NUMBER )
	{
		MousePositionDelta = 0;
	}
}

/**
 * Sets marker's extent to a percentage of scrollbar size, the direction will be vertical or horizontal
 * depending scrollbar's orientation 
 *
 *	@param	SizePercentage	determines the size of the marker, value needs to be in the range [ 0 , 1 ] and
 *                          should be equal to the ratio of viewing area to total widget scroll area 
 */
void UUIScrollbar::SetMarkerSize(FLOAT PercentageSize)
{
	// PercentageSize should be in the range [0 - 1.0]
	const FLOAT NewSizeClamped = Clamp<FLOAT>(PercentageSize, 0.f, 1.f);
	if ( Abs(MarkerSizePercent - NewSizeClamped) > KINDA_SMALL_NUMBER )
	{
		// Update the internal member variable
		MarkerSizePercent = NewSizeClamped;

		if ( !ConditionalRefreshMarker() )
		{
			ResolveMarkerSize();
			ResolveMarkerPosition();
		}
	}
}

/**
 * Sets marker's top or left face position to start at some percentage of total scrollbar extent
 *
 * @param	PositionPercentage	determines where the marker should start, value needs to be in the range [ 0 , 1 ] and
 *                              should correspond to the position of the topmost or leftmost item in the viewing area
 */
void UUIScrollbar::SetMarkerPosition(FLOAT PositionPercentage)
{
	// PositionPercentage should be in range [0.0 - 1.0]
	const FLOAT NewPosClamped = Clamp<FLOAT>(PositionPercentage, 0.f, 1.f);
	if ( Abs(MarkerPosPercent - NewPosClamped) > KINDA_SMALL_NUMBER )
	{
		// Update internal member variable
		MarkerPosPercent = NewPosClamped;

		if ( !ConditionalRefreshMarker() )
		{
			ResolveMarkerPosition();
		}

		// Manually setting marker's position must reset the stored MousePositionDelta
		MousePositionDelta = 0;
	}
}

/**
 * Returns the position of this scrollbar's top face (if orientation is vertical) or left face (for horizontal), in pixels.
 */
FLOAT UUIScrollbar::GetMarkerButtonPosition() const
{
	return MarkerButton != NULL
		? MarkerButton->GetPosition(ScrollbarOrientation == UIORIENT_Vertical ? UIFACE_Top : UIFACE_Left, EVALPOS_PixelViewport, FALSE)
		: 0.f;
}

/**
 * Sets the amount by which the marker will move to cause one position tick
 *
 * @param	NudgePercentage 	percentage of total scrollbar area which will amount to one tick
 *                              value needs to be in the range [ 0 , 1 ]
 */
void UUIScrollbar::SetNudgeSizePercent(FLOAT NudgePercentage)
{
	// NudgePercentage must be in the range [0.0 - 1.0]
	const FLOAT NewNudgeClamped = Clamp<FLOAT>(NudgePercentage, 0.f, 1.f);	
	if ( Abs(NudgePercent - NewNudgeClamped) > KINDA_SMALL_NUMBER )
	{
		// Update the internal member variable
		NudgePercent = NewNudgeClamped;

		if ( !CanResolveScrollZoneExtent() )
		{
			ConditionalRefreshMarker();
		}
		else
		{
			ResolveNudgeSize();
		}
	}
}

/**
 * Sets the amount by which the marker will move to cause one position tick
 *
 * @param	NudgePixels 	Number of pixels by which the marker will need to be moved to cause one tick
 */
void UUIScrollbar::SetNudgeSizePixels( FLOAT NudgePixels )
{
	if( NudgePixels < 0 )
	{
		NudgePixels = 0;
	}
	
	NudgeValue = NudgePixels;	

	if ( ConditionalRefreshMarker() )
	{
		// set NudgePercent to 0 so that it will be recalculated in UUIScrollbar::ResolveFacePosition, once we
		// can accurately calculate the scrollzone's extent.
		NudgePercent = 0.f;
	}
	else
	{
		// Update the NudgePercent member variable
		const FLOAT ZoneSize = GetScrollZoneExtent();
		SetNudgeSizePercent(ZoneSize > 0.f ? (NudgeValue / ZoneSize) : NudgeValue);
	}
}

/**
 * Sets up marker button position based on the value of MarkerPosPercent
 */
void UUIScrollbar::ResolveMarkerPosition()
{
	FLOAT ScrollZoneStart;
	EUIWidgetFace TargetFace = ScrollbarOrientation == UIORIENT_Vertical ? UIFACE_Top : UIFACE_Left;
	const FLOAT AvailableScrollZoneSize = Max(0.f, GetScrollZoneExtent(&ScrollZoneStart) - MarkerButton->GetPosition(GetOppositeFace(TargetFace),
		EVALPOS_PixelOwner));

	const FLOAT NewMarkerPosition = (ScrollZoneStart + AvailableScrollZoneSize * MarkerPosPercent) - GetPosition(TargetFace,EVALPOS_PixelViewport);

	// Calculate final position of the marker in pixels
	MarkerButton->Position.SetRawPositionValue( TargetFace, NewMarkerPosition, EVALPOS_PixelOwner );
}

/**
 * Sets up marker button bounds based on the value of MarkerSizePercent
 */
void UUIScrollbar::ResolveMarkerSize()
{
	if ( !MarkerButton->IsPressed(GetBestPlayerIndex()) )
	{
		// obtain the actual marker size from the percentage value
		const FLOAT MarkerSize = Max<FLOAT>(GetScrollZoneExtent() * MarkerSizePercent, MinimumMarkerSize.GetValue(this));

		if ( MarkerButton->GetPosition(ScrollbarOrientation == UIORIENT_Horizontal ? UIFACE_Right : UIFACE_Bottom, EVALPOS_PixelOwner) != MarkerSize )
		{
			ConditionalRefreshMarker();
			MarkerButton->Position.SetRawPositionValue(ScrollbarOrientation == UIORIENT_Horizontal ? UIFACE_Right : UIFACE_Bottom, MarkerSize, EVALPOS_PixelOwner);
		}
	}
}

/**
 * Resolves the NudgeValue into actual pixels
 */
void UUIScrollbar::ResolveNudgeSize()
{
	FLOAT NudgeSize = GetScrollZoneExtent() * NudgePercent;

	// Update the NudgeValue
	if ( NudgeSize != 0.f )
	{
		NudgeValue = NudgeSize;
	}
}

/**
 *	Sets the value of the bAddCornerPadding flag
 */
void UUIScrollbar::EnableCornerPadding(UBOOL FlagValue)
{
	if(bAddCornerPadding != FlagValue)
	{
		bAddCornerPadding = FlagValue;

		// Changing padding value needs to update doc links 
		SetupDocLinks();
	}
}


/**
 * Responsible for handling of the marker dragging, it reads mouse position and slides the marker in the 
 * appropriate direction
 */
void UUIScrollbar::ProcessDragging()
{
	FLOAT PositionChange = 0;

	UGameUISceneClient* SceneClient = GetSceneClient();
	if ( SceneClient != NULL )
	{
		//Store the previous mouse position
		FUIScreenValue_Position PrevMousePos = MousePosition;

		//Update the internal mouse position with the current value
		FVector TransformedMousePos = PixelToCanvas(FVector2D(SceneClient->MousePosition));
		MousePosition = FUIScreenValue_Position(TransformedMousePos.X, TransformedMousePos.Y);

		//Calculate the position change
		if ( ScrollbarOrientation == UIORIENT_Horizontal )
		{
			//Check if mouse position was within the Marker's bounds to only scroll the marker when the mouse is over it
			if(PrevMousePos.Value[UIORIENT_Horizontal] > MarkerButton->GetPosition(UIFACE_Left, EVALPOS_PixelViewport, TRUE) &&
				PrevMousePos.Value[UIORIENT_Horizontal] < MarkerButton->GetPosition(UIFACE_Right, EVALPOS_PixelViewport, TRUE))
			{
				PositionChange = MousePosition.Value[UIORIENT_Horizontal] - PrevMousePos.Value[UIORIENT_Horizontal];
			}
		}
		else
		{
			//Check if mouse position was within the Marker's bounds to only scroll the marker when the mouse is over it
			if(PrevMousePos.Value[UIORIENT_Vertical] < MarkerButton->RenderBounds[UIFACE_Bottom] &&
				PrevMousePos.Value[UIORIENT_Vertical] > MarkerButton->RenderBounds[UIFACE_Top])
			{
				PositionChange = MousePosition.Value[UIORIENT_Vertical] - PrevMousePos.Value[UIORIENT_Vertical];
			}
		}

		if(PositionChange != 0)
		{
			UpdateMarkerPosition(PositionChange);	
		}
	}
}

/**
 * Responsible for handling paging which is invoked by mouse clicks on the empty bar space
 */
void UUIScrollbar::ScrollZoneClicked( const FInputEventParameters& EventParms )
{
	if ( DELEGATE_IS_SET(OnClickedScrollZone) )
	{
		UGameUISceneClient* SceneClient = GetSceneClient();
		if ( SceneClient != NULL )
		{
			const FLOAT ScrollZoneBegin = DecrementButton->GetPosition(
				ScrollbarOrientation == UIORIENT_Horizontal ? UIFACE_Right : UIFACE_Bottom,
				EVALPOS_PixelViewport, FALSE);

			const FLOAT ScrollZoneSize = GetScrollZoneExtent();

			FVector ProjectedMousePosition = Project(SceneClient->MousePosition);
			const FLOAT ClickPosition = ScrollbarOrientation == UIORIENT_Horizontal 
				? ProjectedMousePosition.X : ProjectedMousePosition.Y;

			FLOAT ClickLocationPercentage = (ClickPosition - ScrollZoneBegin) / ScrollZoneSize;

			delegateOnClickedScrollZone(this, ClickLocationPercentage, EventParms.PlayerIndex);
		}
	}
}

/**
 * Changes the background image for this scrollbar, creating the wrapper UITexture if necessary.
 *
 * @param	NewBarImage		the new surface to use for the scrollbar's background image
 */
void UUIScrollbar::SetBackgroundImage( USurface* NewBackgroundImage )
{
	if ( BackgroundImageComponent != NULL )
	{
		BackgroundImageComponent->SetImage(NewBackgroundImage);
	}
}

/**
 * Increments marker position by the amount of nudges specified in the NudgeMultiplier.  Increment direction is either down or right,
 * depending on scrollbar's orientation.
 *
 * @param	Sender		Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
void UUIScrollbar::ScrollIncrement( UUIScreenObject* Sender, INT PlayerIndex )
{
	FLOAT PositionChange = NudgeValue*NudgeMultiplier;

	UpdateMarkerPosition(PositionChange);
}

/**
 * Decrements marker position by the amount of nudges specified in the NudgeMultiplier.  Decrement direction is either up or left,
 * depending on marker's orientation
 *
 * @param	Sender		Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
void UUIScrollbar::ScrollDecrement( UUIScreenObject* Sender, INT PlayerIndex )
{
	FLOAT PositionChange = NudgeValue*NudgeMultiplier;

	UpdateMarkerPosition(-PositionChange);
}

/**
 * Initiates mouse drag scrolling. The scroll bar marker will slide on its axis with the mouse cursor 
 * until DragScrollEnd is called.
 *
 * @param	Sender		Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
void UUIScrollbar::DragScrollBegin( UUIScreenObject* Sender, INT PlayerIndex )
{
	// Set the initial mouse position
	UGameUISceneClient* SceneClient = GetSceneClient();
	if ( SceneClient != NULL )
	{
		FVector ProjectedMousePosition = PixelToCanvas(FVector2D(SceneClient->MousePosition));
		MousePosition = FUIScreenValue_Position(ProjectedMousePosition.X, ProjectedMousePosition.Y);

		// make sure we get all mouse input events while pressed.
		SetMouseCaptureOverride(TRUE);
	}
}

/**
 * Terminates mouse drag scrolling.
 *
 * @param	Sender		Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 *
 * @return	return TRUE to prevent the kismet OnClick event from firing.
 */
UBOOL UUIScrollbar::DragScrollEnd( UUIScreenObject* Sender, INT PlayerIndex )
{
	// Snap the marker's position to the last nudge position
	if(MousePositionDelta != 0)
	{
		UpdateMarkerPosition(-MousePositionDelta);
	}

	// Reset the mouse position
	MousePosition = FUIScreenValue_Position(0.0f,0.0f);

	// no longer need all mouse input
	SetMouseCaptureOverride(FALSE);
	return TRUE;
}

/**
 *	Called during the dragging process
 *
 * @param	Sender		Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
void UUIScrollbar::DragScroll(UUIScrollbarMarkerButton* Sender,INT PlayerIndex)
{
	ProcessDragging();
}

/**
 * Function overwritten to autoposition the scrollbar within the owner widget
 *
 * @param	Face	the face that should be resolved
 */
void UUIScrollbar::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	// If the scrollbar's dimensions changed, the marker button needs to be refreshed
	if ( bInitializeMarker && GetNumResolvedFaces() == UIFACE_MAX )
	{
		// in order to resolve the marker size and position correctly, we need to ensure that the increment & decrement buttons
		// have been completely updated
		if ( IncrementButton != NULL && DecrementButton != NULL )
		{
			for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
			{
				EUIWidgetFace FaceToResolve = (EUIWidgetFace)FaceIndex;
				DecrementButton->ResolveFacePosition(FaceToResolve);
				IncrementButton->ResolveFacePosition(FaceToResolve);
			}
		}

		//Resolve marker's size, position and nudge value
		ResolveMarkerSize();
		ResolveMarkerPosition();
		ResolveNudgeSize();	

		bInitializeMarker = FALSE;
	}
}

/**
 * Sets up dock links between the scrollbar and its owner widget as well as child buttons of scrollbar
 *
 * @param	bResetLinks		seting flag to TRUE will cause all existing links to be refreshed
 */
void UUIScrollbar::SetupDocLinks( UBOOL bResetLinks /*= FALSE*/ )
{
	BarWidth.Orientation = (ScrollbarOrientation + 1) % UIORIENT_MAX;
	MinimumMarkerSize.Orientation = ScrollbarOrientation;
	ButtonsExtent.Orientation = ScrollbarOrientation;

	UUIObject* OwnerWidget = Cast<UUIObject>(GetOuter());
	if(OwnerWidget)
	{
		const FLOAT Width = GetScrollZoneWidth();
		const FLOAT ButtonWidth = ButtonsExtent.GetValue(this);
		const FLOAT CornerPadding = (bAddCornerPadding==TRUE) ? Width : 0.0f;

		if( ScrollbarOrientation == UIORIENT_Vertical )
		{
			// Setup doclinks between the scrollbar and its owner widget
			// - the top and right faces of the scrollbar are docked to the corresponding faces of the owning widget.
			// - the bottom face is docked to the bottom face of the owning widget, padded by the size of the bar if the owning
			//		widget is also using a horizontal scrollbar
			//		@fixme ronp - I think using the width of the bar is incorrect here; we should probably be using either the width of the scrollbar
			// - the left face is docked to the right face of the owning widget, padded by the size of the bar
			if( bResetLinks || !DockTargets.IsDocked(UIFACE_Left))
			{
				SetDockParameters(UIFACE_Left, OwnerWidget, UIFACE_Right, -Width);
			}
			if( bResetLinks || !DockTargets.IsDocked(UIFACE_Top))
			{
				SetDockParameters(UIFACE_Top, OwnerWidget, UIFACE_Top, 0.f); 
			}
			if( bResetLinks || !DockTargets.IsDocked(UIFACE_Right))
			{
				SetDockParameters(UIFACE_Right, OwnerWidget, UIFACE_Right, 0.f);
			}
			if( bResetLinks || !DockTargets.IsDocked(UIFACE_Bottom) || DockTargets.GetDockPadding(UIFACE_Bottom) != -CornerPadding)
			{
				SetDockParameters(UIFACE_Bottom, OwnerWidget, UIFACE_Bottom, -CornerPadding);
			}


			// Setup doclinks between children buttons and the scrollbar
			// decrement button is the button at the top of the scrollbar; top, left, and right faces of the decrement button
			// are docked to the corresponding faces of this scrollbar; bottom face is docked to the top face of this scrollbar,
			// padded by the size of the button.
			if( bResetLinks || !DecrementButton->DockTargets.IsDocked(UIFACE_Left))
			{
				DecrementButton->SetDockParameters(UIFACE_Left, this, UIFACE_Left, 0.f);
			}
			if( bResetLinks || !DecrementButton->DockTargets.IsDocked(UIFACE_Top))
			{
				DecrementButton->SetDockParameters(UIFACE_Top, this, UIFACE_Top, 0.f);
			}
			if( bResetLinks || !DecrementButton->DockTargets.IsDocked(UIFACE_Right))
			{
				DecrementButton->SetDockParameters(UIFACE_Right, this, UIFACE_Right, 0.f);
			}
			if( bResetLinks || !DecrementButton->DockTargets.IsDocked(UIFACE_Bottom))
			{
				DecrementButton->SetDockParameters(UIFACE_Bottom, this, UIFACE_Top, ButtonWidth);
			}

			// increment button is the button at the bottom of the scrollbar; it's left, right, and bottom faces are docked to the
			// corresponding faces of this scrollbar; its top face is docked to the bottom of this scrollbar, padded by the size
			// of the button
			if( bResetLinks || !IncrementButton->DockTargets.IsDocked(UIFACE_Left))
			{
				IncrementButton->SetDockParameters(UIFACE_Left, this, UIFACE_Left, 0.f);
			}
			if( bResetLinks || !IncrementButton->DockTargets.IsDocked(UIFACE_Top))
			{
				IncrementButton->SetDockParameters(UIFACE_Top, this, UIFACE_Bottom, -ButtonWidth);
			}
			if( bResetLinks || !IncrementButton->DockTargets.IsDocked(UIFACE_Right))
			{
				IncrementButton->SetDockParameters(UIFACE_Right, this, UIFACE_Right, 0.f);
			}
			if( bResetLinks || !IncrementButton->DockTargets.IsDocked(UIFACE_Bottom))
			{
				IncrementButton->SetDockParameters(UIFACE_Bottom, this, UIFACE_Bottom, 0.f);
			}

			if ( MarkerButton->Position.GetScaleType(UIFACE_Bottom) != EVALPOS_PixelOwner )
			{
				RefreshMarker();
			}
			MarkerButton->Position.ChangeScaleType(this, UIFACE_Bottom, EVALPOS_PixelOwner);
		}
		else
		{
			// Setup doclinks between the scrollbar and its owner widget; for horizontal orientation,
			// - the left and bottom faces of the scrollbar are docked to the corresponding faces of the owning widget.
			// - the right face is docked to the right face of the owning widget, padded by the size of the bar if the owning
			//		widget is also using a vertical scrollbar
			// - the top face is docked to the bottom face of the owning widget, padded by the size of the bar
			if( bResetLinks || !DockTargets.IsDocked(UIFACE_Left))
			{
				SetDockParameters(UIFACE_Left, OwnerWidget, UIFACE_Left, 0.f);
			}
			if( bResetLinks || !DockTargets.IsDocked(UIFACE_Top))
			{
				SetDockParameters(UIFACE_Top, OwnerWidget, UIFACE_Bottom, -Width); 
			}
			if( bResetLinks || !DockTargets.IsDocked(UIFACE_Right) || DockTargets.GetDockPadding(UIFACE_Right) != -CornerPadding)
			{
				SetDockParameters(UIFACE_Right, OwnerWidget, UIFACE_Right, -CornerPadding);
			}
			if( bResetLinks || !DockTargets.IsDocked(UIFACE_Bottom))
			{
				SetDockParameters(UIFACE_Bottom, OwnerWidget, UIFACE_Bottom, 0.f);
			}

			// Setup doclinks between children buttons and the scrollbar
			// decrement button is the button at the left of the scrollbar; top, left, and bottom faces of the decrement button
			// are docked to the corresponding faces of this scrollbar; right face is docked to the left face of this scrollbar,
			// padded by the size of the button.
			if( bResetLinks || !DecrementButton->DockTargets.IsDocked(UIFACE_Left))
			{
				DecrementButton->SetDockParameters(UIFACE_Left, this, UIFACE_Left, 0.f);
			}
			if( bResetLinks || !DecrementButton->DockTargets.IsDocked(UIFACE_Top))
			{
				DecrementButton->SetDockParameters(UIFACE_Top, this, UIFACE_Top, 0.f);
			}
			if( bResetLinks || !DecrementButton->DockTargets.IsDocked(UIFACE_Right))
			{
				DecrementButton->SetDockParameters(UIFACE_Right, this, UIFACE_Left, ButtonWidth);
			}
			if( bResetLinks || !DecrementButton->DockTargets.IsDocked(UIFACE_Bottom))
			{
				DecrementButton->SetDockParameters(UIFACE_Bottom, this, UIFACE_Bottom, 0.f);
			}

			// increment button is the button at the right of the scrollbar; it's top, right, and bottom faces are docked to the
			// corresponding faces of this scrollbar; its left face is docked to the right of this scrollbar, padded by the size
			// of the button
			if( bResetLinks || !IncrementButton->DockTargets.IsDocked(UIFACE_Left))
			{
				IncrementButton->SetDockParameters(UIFACE_Left, this, UIFACE_Right, -ButtonWidth);
			}
			if( bResetLinks || !IncrementButton->DockTargets.IsDocked(UIFACE_Top))
			{
				IncrementButton->SetDockParameters(UIFACE_Top, this, UIFACE_Top, 0.f);
			}
			if( bResetLinks || !IncrementButton->DockTargets.IsDocked(UIFACE_Right))
			{
				IncrementButton->SetDockParameters(UIFACE_Right, this, UIFACE_Right, 0.f);
			}
			if( bResetLinks || !IncrementButton->DockTargets.IsDocked(UIFACE_Bottom))
			{
				IncrementButton->SetDockParameters(UIFACE_Bottom, this, UIFACE_Bottom, 0.f);
			}

			if ( MarkerButton->Position.GetScaleType(UIFACE_Right) != EVALPOS_PixelOwner )
			{
				RefreshMarker();
			}
			MarkerButton->Position.ChangeScaleType(this, UIFACE_Right, EVALPOS_PixelOwner);
		}
	}
}

/* === UObject interface === */
/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIScrollbar::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("BackgroundImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty && BackgroundImageComponent != NULL )
				{
					// the user either cleared the value of the BackgroundImageComponent (which should never happen since
					// we use the 'noclear' keyword on the property declaration), or is assigning a new value to the BackgroundImageComponent.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(BackgroundImageComponent);
				}
			}
		}
	}
}

/**
* Called when a property value from a member struct or array has been changed in the editor.
*/
void UUIScrollbar::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{	
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("BarWidth") )
			{
				// BarWidth affects the padding value used in the docking links between this scrollbar and its parent,
				// so we'll need to update the docking sets to reflect the change
				SetupDocLinks(TRUE);
				RefreshMarker();
			}
			else if ( PropertyName == TEXT("ButtonsExtent") )
			{
				// ButtonsExtent affects the padding value used in the docking links between this scrollbar and the increment/decrement buttons,
				// so we'll need to update the docking sets to reflect the change
				SetupDocLinks(TRUE);
				RefreshMarker();	
			}
			else if(PropertyName == TEXT("ScrollbarOrientation"))
			{
				// ScrollbarOrientation affects the docking links between this scrollbar and its parent, so we'll need to update the docking sets to reflect the change
				SetupDocLinks(TRUE);
				RefreshMarker();
			}
			else if(PropertyName == TEXT("bAddCornerPadding"))
			{
				// bAddCornerPadding affects the padding value used in the docking links between this scrollbar and its parent,
				// so we'll need to update the docking sets to reflect the change
				SetupDocLinks(TRUE);
				RefreshMarker();
			}
			else if ( PropertyName == TEXT("BackgroundImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( BackgroundImageComponent != NULL )
					{
						UUIComp_DrawImage* ComponentTemplate = GetArchetype<UUIScrollbar>()->BackgroundImageComponent;
						if ( ComponentTemplate != NULL )
						{
							BackgroundImageComponent->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}
						else
						{
							BackgroundImageComponent->StyleResolverTag = TEXT("Background Image Style");
						}

						// user created a new background image component - add it to the list of style subscribers
						AddStyleSubscriber(BackgroundImageComponent);

						// now initialize the component's image
						BackgroundImageComponent->SetImage(BackgroundImageComponent->GetImage());
					}
				}
				else if ( BackgroundImageComponent != NULL )
				{
					// a property of the ImageComponent was changed
					if ( ModifiedProperty->GetFName() == TEXT("ImageRef") && BackgroundImageComponent->GetImage() != NULL )
					{
#if 0
						USurface* CurrentValue = BackgroundImageComponent->GetImage();

						// changed the value of the image texture/material
						// clear the data store binding
						//@fixme ronp - do we always need to clear the data store binding?
						SetDataStoreBinding(TEXT(""));

						// clearing the data store binding value may have cleared the value of the image component's texture,
						// so restore the value now
						SetImage(CurrentValue);
#endif
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated PanelBackground,
 * Coordinates, and PrimaryStyle properties over to the BackgroundImageComponent.
 */
void UUIScrollbar::PostLoad()
{
	Super::PostLoad();

	const INT CurrentPackageVersion = GetLinkerVersion();
	if ( CurrentPackageVersion < VER_FIXED_UISCROLLBAR_BUTTONS )
	{
		if ( CurrentPackageVersion < VER_FIXED_UISCROLLBARS )
		{
			// this is to remove the old versions of the scrollbar's buttons
			// make sure none of our children were marked RF_RootSet, which can happen if the package containing this scrollbar was
			// loaded during editor startup
			for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
			{
				Children(ChildIndex)->ClearFlags(RF_RootSet);

				TArray<UObject*> ChildSubobjects;
				FArchiveObjectReferenceCollector Collector(
					&ChildSubobjects,
					Children(ChildIndex),
					FALSE,
					FALSE,
					TRUE,
					FALSE
					);

				Children(ChildIndex)->Serialize(Collector);
				for ( INT ObjIndex = 0; ObjIndex < ChildSubobjects.Num(); ObjIndex++ )
				{
					ChildSubobjects(ObjIndex)->ClearFlags(RF_RootSet);
				}
			}

			RemoveChildren(Children);
			IncrementButton = DecrementButton = NULL;
			MarkerButton = NULL;

			if ( !GIsUCCMake && Background != NULL )
			{
				MigrateImageSettings(Background, BackgroundCoordinates, PrimaryStyle, BackgroundImageComponent, TEXT("Background"));
			}
		}
		else
		{
			if ( IncrementButton != NULL )
			{
				// make sure none of our children were marked RF_RootSet, which can happen if the package containing this scrollbar was
				// loaded during editor startup
				IncrementButton->ClearFlags(RF_RootSet);

				TArray<UObject*> ButtonSubobjects;
				FArchiveObjectReferenceCollector Collector(
					&ButtonSubobjects,
					IncrementButton,
					FALSE,
					FALSE,
					TRUE,
					FALSE
					);

				IncrementButton->Serialize(Collector);
				for ( INT ObjIndex = 0; ObjIndex < ButtonSubobjects.Num(); ObjIndex++ )
				{
					ButtonSubobjects(ObjIndex)->ClearFlags(RF_RootSet);
				}
				RemoveChild(IncrementButton);
				IncrementButton = NULL;
			}
			if ( DecrementButton != NULL )
			{
				// make sure none of our children were marked RF_RootSet, which can happen if the package containing this scrollbar was
				// loaded during editor startup
				DecrementButton->ClearFlags(RF_RootSet);
				TArray<UObject*> ButtonSubobjects;
				FArchiveObjectReferenceCollector Collector(
					&ButtonSubobjects,
					DecrementButton,
					FALSE,
					FALSE,
					TRUE,
					FALSE
					);

				DecrementButton->Serialize(Collector);
				for ( INT ObjIndex = 0; ObjIndex < ButtonSubobjects.Num(); ObjIndex++ )
				{
					ButtonSubobjects(ObjIndex)->ClearFlags(RF_RootSet);
				}

				RemoveChild(DecrementButton);
				DecrementButton = NULL;
			}
		}
	}
}

/* ==========================================================================================================
	UUIScrollbarMarkerButton
========================================================================================================== */

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUIScrollbarMarkerButton::GetSupportedUIActionKeyNames( TArray<FName>& out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddUniqueItem(UIKEY_DragSlider);
}

/**
 * Processes input axis movement. Only called while the button is in the pressed state;
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIScrollbarMarkerButton::ProcessInputAxis( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	// Dragging Event
	if ( EventParms.InputAliasName == UIKEY_DragSlider )
	{
		// Invoke the OnButtonDragged delegate
		if( DELEGATE_IS_SET(OnButtonDragged) )
		{
			delegateOnButtonDragged(this, EventParms.PlayerIndex);
		}

		bResult = TRUE;
	}

	return bResult || Super::ProcessInputAxis(EventParms);
}

/* ==========================================================================================================
	UIProgressBar
========================================================================================================== */

/* === UUIProgressBar interface === */
/**
* Changes the background image for this progress bar.
*
* @param	NewBackgroundImage		the new surface to use for the progressbar's background image
*/
void UUIProgressBar::SetBackgroundImage( USurface* NewBackgroundImage )
{
	if ( BackgroundImageComponent != NULL )
	{
		BackgroundImageComponent->SetImage(NewBackgroundImage);
	}
}

/**
* Changes the fill image for this progress bar.
*
* @param	NewMarkerImage		the new surface to use for the progressbar's marker
*/
void UUIProgressBar::SetFillImage( USurface* NewFillImage )
{
	if ( FillImageComponent != NULL )
	{
		FillImageComponent->SetImage(NewFillImage);
	}
}

/**
 * Changes the overlay image for this progressbar, creating the wrapper UITexture if necessary.
 *
 * @param	NewOverlayImage		the new surface to use for the progressbar's overlay image
 */
void UUIProgressBar::SetOverlayImage( USurface* NewOverlayImage )
{
	if ( OverlayImageComponent != NULL )
	{
		OverlayImageComponent->SetImage(NewOverlayImage);
	}
}

/**
 * Returns the pixel extent of the progregressbar fill based on the current progressbar value
 */
FLOAT UUIProgressBar::GetBarFillExtent()
{
	// the total width of the progressbar region
	FLOAT ProgressBarWidth;
	
	if( ProgressBarOrientation == UIORIENT_Horizontal)
	{
		ProgressBarWidth = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];
	}
	else
	{
		ProgressBarWidth = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];
	}

	// the percentage of the total width of this progressbar where the bar fill should end
	FLOAT Percentage = GetValue(TRUE);

	return ProgressBarWidth * Percentage;
}

/**
 * Change the value of this progressbar at runtime.
 *
 * @param	NewValue			the new value for the progressbar.
 * @param	bPercentageValue	TRUE indicates that the new value is formatted as a percentage of the total range of this slider.
 *
 * @return	TRUE if the slider's value was changed.
 */
UBOOL UUIProgressBar::SetValue( FLOAT NewValue, UBOOL bPercentageValue/*=FALSE*/ )
{
	UBOOL bResult;

	FLOAT PreviousValue = ProgressBarValue.GetCurrentValue();
	if ( bPercentageValue == TRUE )
	{
		NewValue = ((ProgressBarValue.MaxValue - ProgressBarValue.MinValue) * Clamp<FLOAT>(NewValue, 0.f, 1.f)) + ProgressBarValue.MinValue;
	}

	ProgressBarValue.SetCurrentValue(NewValue,TRUE);

	bResult = Abs<FLOAT>(PreviousValue - ProgressBarValue.GetCurrentValue()) > KINDA_SMALL_NUMBER;
	
	if(bResult)
	{
		NotifyValueChanged();
	}

	return bResult;
}

/**
 * Gets the current value of this progressbar
 *
 * @param	bFormatAsPercent	TRUE to format the result as a percentage of the total range of this slider.
 */
FLOAT UUIProgressBar::GetValue( UBOOL bFormatAsPercent/*=FALSE*/ ) const
{
	FLOAT Result = ProgressBarValue.GetCurrentValue();
	if ( bFormatAsPercent && ProgressBarValue.MaxValue > ProgressBarValue.MinValue )
	{
		// the percentage of the total width of this slider where the marker is located
		Result = (Result - ProgressBarValue.MinValue) / (ProgressBarValue.MaxValue - ProgressBarValue.MinValue);
	}

	return Result;
}

/**
 * Render this progressbar.
 *
 * @param	Canvas	the canvas to use for rendering this widget
 */
void UUIProgressBar::Render_Widget( FCanvas* Canvas )
{
	FRenderParameters Parameters(
		RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
		RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
		RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
		);

	// first, render the progress bar background
	if ( BackgroundImageComponent != NULL )
	{
		BackgroundImageComponent->RenderComponent(Canvas, Parameters);
	}

	if ( FillImageComponent != NULL )
	{
		FRenderParameters BarRenderParameters = Parameters;
		FLOAT BarFillExtent = GetBarFillExtent();
		if(ProgressBarOrientation == UIORIENT_Horizontal)
		{
			BarRenderParameters.DrawXL = BarFillExtent;
		}
		else
		{
			BarRenderParameters.DrawY = Parameters.DrawY + Parameters.DrawYL - BarFillExtent;
			BarRenderParameters.DrawYL = BarFillExtent;
		}

		FillImageComponent->RenderComponent(Canvas, BarRenderParameters);
	}

	// render the Overlay after the barfill
	if ( bDrawOverlay && OverlayImageComponent != NULL )
	{
		OverlayImageComponent->RenderComponent(Canvas, Parameters);
	}
}

/**
 * Adds the specified face to the DockingStack for the specified widget
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUIProgressBar::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	return Super::AddDockingNode(DockingStack, Face);
}

/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIProgressBar::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			UUIComp_DrawImage* ModifiedImageComponent=NULL;

			if ( PropertyName == TEXT("BackgroundImageComponent") )
			{
				ModifiedImageComponent = BackgroundImageComponent;
			}
			else if ( PropertyName == TEXT("FillImageComponent") )
			{
				ModifiedImageComponent = FillImageComponent;
			}
			else if ( PropertyName == TEXT("OverlayImageComponent") )
			{
				ModifiedImageComponent = OverlayImageComponent;
			}

			if ( ModifiedImageComponent != NULL )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the component itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					// the user either cleared the value of the component (which should never happen since
					// we use the 'noclear' keyword on the property declaration), or is assigning a new value to the component.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(ModifiedImageComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value from a member struct or array has been changed in the editor.
 */
void UUIProgressBar::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("DataSource") )
			{
				RefreshSubscriberValue();
			}
			else
			{
				// see if was one of our components
				UUIComp_DrawImage* ModifiedImageComponent = NULL;
				UUIComp_DrawImage* ComponentTemplate=NULL;
				if ( PropertyName == TEXT("BackgroundImageComponent") )
				{
					ModifiedImageComponent = BackgroundImageComponent;
					ComponentTemplate = GetArchetype<UUIProgressBar>()->BackgroundImageComponent;
				}
				else if ( PropertyName == TEXT("FillImageComponent") )
				{
					ModifiedImageComponent = FillImageComponent;
					ComponentTemplate = GetArchetype<UUIProgressBar>()->FillImageComponent;
				}
				else if ( PropertyName == TEXT("OverlayImageComponent") )
				{
					ModifiedImageComponent = OverlayImageComponent;
					ComponentTemplate = GetArchetype<UUIProgressBar>()->OverlayImageComponent;
				}

				if ( ModifiedImageComponent != NULL )
				{
					// this represents the inner-most property that the user modified
					UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

					// if the value of the BackgroundImageComponent itself was changed
					if ( MemberProperty == ModifiedProperty )
					{
						if ( ComponentTemplate != NULL )
						{
							ModifiedImageComponent->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}

						// user created a new background image component - add it to the list of style subscribers
						AddStyleSubscriber(ModifiedImageComponent);

						// now initialize the component's image
						ModifiedImageComponent->SetImage(ModifiedImageComponent->GetImage());
					}
					else
					{
						// a property of the ImageComponent was changed
						if ( ModifiedProperty->GetFName() == TEXT("ImageRef") && ModifiedImageComponent->GetImage() != NULL )
						{
#if 0
							USurface* CurrentValue = ModifiedImageComponent->GetImage();

							// changed the value of the image texture/material
							// clear the data store binding
							//@fixme ronp - do we always need to clear the data store binding?
							SetDataStoreBinding(TEXT(""));

							// clearing the data store binding value may have cleared the value of the image component's texture,
							// so restore the value now
							SetImage(CurrentValue);
#endif
						}
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated PanelBackground,
 * Coordinates, and PrimaryStyle properties over to the BackgroundImageComponent.
 */
void UUIProgressBar::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake && GetLinkerVersion() < VER_FIXED_UISCROLLBARS )
	{
		MigrateImageSettings(Background, BackgroundCoordinates, PrimaryStyle, BackgroundImageComponent, TEXT("Background"));
		MigrateImageSettings(BarFill, BarFillCoordinates, FillStyle, FillImageComponent, TEXT("BarFill"));
		MigrateImageSettings(Overlay, OverlayCoordinates, OverlayStyle, OverlayImageComponent, TEXT("Overlay"));
	}
}

/**
 * Called whenever the value of the progressbar is modified.  Activates the ProgressBarValueChanged kismet event and calls the OnValueChanged
 * delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to this method; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
 */
void UUIProgressBar::NotifyValueChanged( INT PlayerIndex/*=INDEX_NONE*/, INT NotifyFlags/*=0*/ )
{
	if ( PlayerIndex == INDEX_NONE )
	{
		PlayerIndex = GetBestPlayerIndex();
	}

	Super::NotifyValueChanged(PlayerIndex, NotifyFlags);
	ActivateEventByClass(PlayerIndex,UUIEvent_ProgressBarValueChanged::StaticClass(),this);


	const FLOAT NewValue = GetValue();
	TArray<UUIEvent_ProgressBarValueChanged*> ActivatedEvents;
	ActivateEventByClass(PlayerIndex, UUIEvent_ProgressBarValueChanged::StaticClass(), this, FALSE, NULL, (TArray<UUIEvent*>*)&ActivatedEvents);
	for ( INT EventIdx = 0; EventIdx < ActivatedEvents.Num(); EventIdx++ )
	{
		UUIEvent_ProgressBarValueChanged* Event = ActivatedEvents(EventIdx);

		// copy the current value of the progressbar into the "Value" variable link
		TArray<FLOAT*> FloatVars;
		Event->GetFloatVars(FloatVars,TEXT("Value"));

		for (INT Idx = 0; Idx < FloatVars.Num(); Idx++)
		{
			*(FloatVars(Idx)) = NewValue;
		}
	}
}

/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUIProgressBar::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(BackgroundImageComponent);
	VALIDATE_COMPONENT(FillImageComponent);
	VALIDATE_COMPONENT(OverlayImageComponent);
	AddStyleSubscriber(BackgroundImageComponent);
	AddStyleSubscriber(FillImageComponent);
	AddStyleSubscriber(OverlayImageComponent);
}

/* === IUIDataStoreSubscriber interface === */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
void UUIProgressBar::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( MarkupText != DataSource.MarkupString )
	{
		Modify();
		DataSource.MarkupString = MarkupText;

		RefreshSubscriberValue(BindingIndex);
	}

}

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUIProgressBar::GetDataStoreBinding( INT BindingIndex/*=INDEX_NONE*/ ) const
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}
	return DataSource.MarkupString;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUIProgressBar::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( DataSource.ResolveMarkup( this ) )
	{
		FUIProviderFieldValue ResolvedValue(EC_EventParm);
		if ( DataSource.GetBindingValue(ResolvedValue) && ResolvedValue.StringValue.Len() > 0 )
		{
			if ( ResolvedValue.PropertyType == DATATYPE_RangeProperty )
			{
				ProgressBarValue = ResolvedValue.RangeValue;
				bResult = TRUE;
			}
			else
			{
				FLOAT FloatValue = appAtof(*ResolvedValue.StringValue);
				SetValue(FloatValue);
				bResult = TRUE;
			}
		}
	}

	return bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUIProgressBar::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);
	if ( DataSource )
	{
		out_BoundDataStores.AddUniqueItem(*DataSource);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUIProgressBar::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUIProgressBar::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	FUIProviderScriptFieldValue Value(EC_EventParm);
	Value.PropertyType = DATATYPE_RangeProperty;
	Value.StringValue = FString::Printf(TEXT("%f"), GetValue());

	// add the data store bound to this progressbar to the list
	GetBoundDataStores(out_BoundDataStores);

	return DataSource.SetBindingValue(Value);
}

/* ==========================================================================================================
	UISlider
========================================================================================================== */
/* === UUISlider interface === */
/**
 * Changes the background image for this slider.
 *
 * @param	NewBackgroundImage		the new surface to use for the slider's background image
 */
void UUISlider::SetBackgroundImage( USurface* NewBackgroundImage )
{
	if ( BackgroundImageComponent != NULL )
	{
		BackgroundImageComponent->SetImage(NewBackgroundImage);
	}
}

/**
 * Changes the bar image for this slider.
 *
 * @param	NewBarImage		the new surface to use for the slider's bar image
 */
void UUISlider::SetBarImage( USurface* NewBarImage )
{
	if ( SliderBarImageComponent != NULL )
	{
		SliderBarImageComponent->SetImage(NewBarImage);
	}
}

/**
 * Changes the marker image for this slider, creating the wrapper UITexture if necessary.
 *
 * @param	NewMarkerImage		the new surface to use for the slider's marker
 */
void UUISlider::SetMarkerImage( USurface* NewMarkerImage )
{
	if ( MarkerImageComponent != NULL )
	{
		MarkerImageComponent->SetImage(NewMarkerImage);
	}
}

/**
 * Change the value of this slider at runtime.
 *
 * @param	NewValue			the new value for the slider.
 * @param	bPercentageValue	TRUE indicates that the new value is formatted as a percentage of the total range of this slider.
 *
 * @return	TRUE if the slider's value was changed.
 */
UBOOL UUISlider::SetValue( FLOAT NewValue, UBOOL bPercentageValue/*=FALSE*/ )
{
	FLOAT PreviousValue = SliderValue.GetCurrentValue();
	if ( bPercentageValue == TRUE )
	{
		NewValue = ((SliderValue.MaxValue - SliderValue.MinValue) * Clamp<FLOAT>(NewValue, 0.f, 1.f)) + SliderValue.MinValue;
	}

	SliderValue.SetCurrentValue(NewValue,TRUE);
	return Abs<FLOAT>(PreviousValue - SliderValue.GetCurrentValue()) > KINDA_SMALL_NUMBER;
}

/**
 * Gets the current value of this slider
 *
 * @param	bFormatAsPercent	TRUE to format the result as a percentage of the total range of this slider.
 */
FLOAT UUISlider::GetValue( UBOOL bFormatAsPercent/*=FALSE*/ ) const
{
	FLOAT Result = SliderValue.GetCurrentValue();
	if ( bFormatAsPercent && SliderValue.MaxValue > SliderValue.MinValue )
	{
		// the percentage of the total width of this slider where the marker is located
		Result = (Result - SliderValue.MinValue) / (SliderValue.MaxValue - SliderValue.MinValue);
	}

	return Result;
}

/**
 * Returns the screen location (along the axis of the slider) for the marker, in absolute pixels.
 */
FLOAT UUISlider::GetMarkerPosition()
{
	// the dimensions of the marker texture
	if ( Marker != NULL )
	{
		FLOAT CurrentMarkerWidth = MarkerWidth.GetValue(this);
		FLOAT CurrentMarkerHeight = MarkerHeight.GetValue(this);
		if ( CurrentMarkerWidth == 0 || CurrentMarkerHeight == 0 )
		{
			FLOAT MarkerXL=0, MarkerYL=0;
			Marker->CalculateExtent(MarkerXL, MarkerYL);

			if ( CurrentMarkerWidth == 0 )
			{
				MarkerWidth.SetValue(this, MarkerXL);
			}

			if ( CurrentMarkerHeight == 0 )
			{
				MarkerHeight.SetValue(this, MarkerYL);
			}
		}
	}

	// the total width of the slider region
	FLOAT SliderWidth = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];

	// the percentage of the total width of this slider where the marker is located
	FLOAT Percentage = GetValue(TRUE);

	// the offset (in pixels) for the marker from the slider's left face
	FLOAT RelativeLocation = (SliderWidth - MarkerWidth.GetValue(this)) * Percentage;

	return RelativeLocation + (SliderOrientation == UIORIENT_Horizontal ? RenderBounds[UIFACE_Left] : RenderBounds[UIFACE_Bottom]);
}

/**
 * Retrieves the location of the mouse within the bounding region of this slider, in percentage of the
 * slider width (if orientation is horizontal) or height (if vertical).
 *
 * @param	out_Percentage	a value between 0.0 and 1.0 representing the percentage of the slider's size for the current
 *							position of the mouse cursor.
 *
 * @return	TRUE if the cursor is within the bounding region of this slider.
 */
UBOOL UUISlider::GetCursorPosition( FLOAT& out_Percentage )
{
	UBOOL bResult = FALSE;

	UGameUISceneClient* SceneClient = GetSceneClient();
	if ( SceneClient != NULL )
	{
		FVector MousePosition = Project(SceneClient->MousePosition);

		if ( SliderOrientation == UIORIENT_Horizontal )
		{
			bResult = MousePosition.X >= RenderBounds[UIFACE_Left] && MousePosition.X <= RenderBounds[UIFACE_Right];
		}
		else
		{
			bResult = MousePosition.Y >= RenderBounds[UIFACE_Top] && MousePosition.Y <= RenderBounds[UIFACE_Bottom];
		}

		if ( bResult )
		{
			//@fixme - make this work for vertical orientation
			// now calculate the percentage of the slider's width/height for the mouse location
			FLOAT MarkerSize = MarkerWidth.GetValue(this);
			FLOAT SliderWidth = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];

			out_Percentage = ((MousePosition.X - (RenderBounds[UIFACE_Left] + (MarkerSize * 0.5f))) / (SliderWidth - MarkerSize));
		}
	}

	return bResult;
}

/**
 * Called whenever the value of the UIObject is modified (for those UIObjects which can have values).
 * Calls the OnValueChanged delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to SetValue; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
 */
void UUISlider::NotifyValueChanged( INT PlayerIndex/*=INDEX_NONE*/, INT NotifyFlags/*=0*/ )
{
	if ( PlayerIndex == INDEX_NONE )
	{
		PlayerIndex = GetBestPlayerIndex();
	}

	Super::NotifyValueChanged(PlayerIndex, NotifyFlags);

	const FLOAT NewValue = GetValue();
	TArray<UUIEvent_SliderValueChanged*> ActivatedEvents;
	ActivateEventByClass(PlayerIndex, UUIEvent_SliderValueChanged::StaticClass(), this, FALSE, NULL, (TArray<UUIEvent*>*)&ActivatedEvents);
	for ( INT EventIdx = 0; EventIdx < ActivatedEvents.Num(); EventIdx++ )
	{
		UUIEvent_SliderValueChanged* Event = ActivatedEvents(EventIdx);

		// copy the current value of the slider into the "Value" variable link
		TArray<FLOAT*> FloatVars;
		Event->GetFloatVars(FloatVars,TEXT("Value"));

		for (INT Idx = 0; Idx < FloatVars.Num(); Idx++)
		{
			*(FloatVars(Idx)) = NewValue;
		}
	}
}

/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUISlider::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(CaptionRenderComponent);
	VALIDATE_COMPONENT(BackgroundImageComponent);
	VALIDATE_COMPONENT(SliderBarImageComponent);
	VALIDATE_COMPONENT(MarkerImageComponent);
	AddStyleSubscriber(CaptionRenderComponent);

	AddStyleSubscriber(BackgroundImageComponent);
	AddStyleSubscriber(SliderBarImageComponent);
	AddStyleSubscriber(MarkerImageComponent);
}

/* === IUIDataStoreSubscriber interface === */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
void UUISlider::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( MarkupText != DataSource.MarkupString )
	{
		Modify();
		DataSource.MarkupString = MarkupText;

		RefreshSubscriberValue(BindingIndex);
	}

}

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUISlider::GetDataStoreBinding( INT BindingIndex/*=INDEX_NONE*/ ) const
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}
	return DataSource.MarkupString;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUISlider::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( DataSource.ResolveMarkup( this ) )
	{
		FUIProviderFieldValue ResolvedValue(EC_EventParm);
		if ( DataSource.GetBindingValue(ResolvedValue) )
		{
			if ( ResolvedValue.PropertyType == DATATYPE_RangeProperty && ResolvedValue.RangeValue.HasValue() )
			{
				SliderValue = ResolvedValue.RangeValue;
				bResult = TRUE;
			}
			else if ( ResolvedValue.StringValue.Len() > 0 )
			{
				FLOAT FloatValue = appAtof(*ResolvedValue.StringValue);
				SetValue(FloatValue);
				bResult = TRUE;
			}
		}
	}

	return bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUISlider::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);
	if ( DataSource )
	{
		out_BoundDataStores.AddUniqueItem(*DataSource);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUISlider::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUISlider::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	FUIProviderScriptFieldValue Value(EC_EventParm);
	Value.PropertyType = DATATYPE_RangeProperty;
	Value.StringValue = FString::Printf(TEXT("%f"), GetValue());

	// add the data store bound to this slider to the list
	GetBoundDataStores(out_BoundDataStores);

	return DataSource.SetBindingValue(Value);
}

/* === UUIObject interface === */
/**
 * Initializes the button and creates the background image.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUISlider::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	if ( CaptionRenderComponent != NULL )
	{
		TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
		CaptionRenderComponent->InitializeComponent(&Subscriber);
	}
	Super::Initialize(inOwnerScene, inOwner);
}

/**
 * Adds the specified face to the DockingStack for the specified widget
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUISlider::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	return Super::AddDockingNode(DockingStack, Face);
}

/**
 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUISlider::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	if ( CaptionRenderComponent != NULL )
	{
		CaptionRenderComponent->ResolveFacePosition(Face);
	}
}

/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUISlider::InvalidatePositionDependencies( BYTE Face )
{
	Super::InvalidatePositionDependencies(Face);
	if ( CaptionRenderComponent != NULL )
	{
		CaptionRenderComponent->InvalidatePositionDependencies(Face);
	}
}

/**
 * Render this slider.
 *
 * @param	Canvas	the canvas to use for rendering this widget
 */
void UUISlider::Render_Widget( FCanvas* Canvas )
{
	FRenderParameters Parameters(
		RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
		RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
		RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
		);

	// first, render the slider background
	if ( BackgroundImageComponent != NULL )
	{
		//@todo - take into account image adjustments set in the marker style...i.e. alignment, buffer zones, etc.
		BackgroundImageComponent->RenderComponent(Canvas, Parameters);
	}

	// next, render the slider bar
	if ( SliderBarImageComponent != NULL )
	{
		FRenderParameters BarRenderParameters = Parameters;
		if ( SliderOrientation == UIORIENT_Horizontal )
		{
			BarRenderParameters.ImageExtent.Y = BarSize.GetValue(this);
		}
		else
		{
			BarRenderParameters.ImageExtent.X = BarSize.GetValue(this);
		}

		//@todo - take into account image adjustments set in the marker style...i.e. alignment, buffer zones, etc.
		SliderBarImageComponent->RenderComponent(Canvas, BarRenderParameters);
	}

	// next, render the marker 
	if ( MarkerImageComponent != NULL )
	{
		//@todo - take into account image adjustments set in the marker style...i.e. alignment, buffer zones, etc.
		FLOAT MarkerPosition = GetMarkerPosition();
		FRenderParameters MarkerRenderParameters;

		if ( SliderOrientation == UIORIENT_Horizontal )
		{
			MarkerRenderParameters.DrawX = MarkerPosition;
			MarkerRenderParameters.DrawY = RenderBounds[UIFACE_Top];
			MarkerRenderParameters.DrawXL = MarkerWidth.GetValue(this);
			MarkerRenderParameters.DrawYL = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];

			MarkerRenderParameters.ImageExtent.Y = MarkerHeight.GetValue(this);
		}
		else
		{
			FLOAT ActualMarkerHeight = MarkerHeight.GetValue(this);

			MarkerRenderParameters.DrawX = RenderBounds[UIFACE_Left];
			MarkerRenderParameters.DrawY = MarkerPosition + ActualMarkerHeight;
			MarkerRenderParameters.DrawXL = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];
			MarkerRenderParameters.DrawYL = ActualMarkerHeight;

			MarkerRenderParameters.ImageExtent.X = MarkerWidth.GetValue(this);
		}

		MarkerImageComponent->RenderComponent(Canvas, MarkerRenderParameters);
	}

	// finally, render the caption
	if ( CaptionRenderComponent != NULL && bRenderCaption )
	{
		CaptionRenderComponent->Render_String(Canvas);
	}
}

/**
 * Handles input events for this slider.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUISlider::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	// Pressed Event
	if ( EventParms.InputAliasName == UIKEY_Clicked )
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			const UBOOL bIsDoubleClickPress = EventParms.EventType==IE_DoubleClick;

			// notify unrealscript
			if ( DELEGATE_IS_SET(OnPressed) )
			{
				delegateOnPressed(this, EventParms.PlayerIndex);
			}

			if ( bIsDoubleClickPress && DELEGATE_IS_SET(OnDoubleClick) )
			{
				delegateOnDoubleClick(this, EventParms.PlayerIndex);
			}

			// activate the pressed state
			ActivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			if ( bIsDoubleClickPress )
			{
				ActivateEventByClass(EventParms.PlayerIndex, UUIEvent_OnDoubleClick::StaticClass(), this);
			}

			FLOAT CursorLocationPercentage;
			if ( GetCursorPosition(CursorLocationPercentage) )
			{
				if ( SetValue(CursorLocationPercentage,TRUE) )
				{
					NotifyValueChanged(EventParms.PlayerIndex);
				}
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released )
		{
			if ( DELEGATE_IS_SET(OnPressRelease) )
			{
				delegateOnPressRelease(this, EventParms.PlayerIndex);
			}

			if ( IsPressed(EventParms.PlayerIndex) )
			{
				// Play the ClickedCue
				//PlayUISound(ClickedCue,EventParms.PlayerIndex);

				// Fire OnPressed Delegate
				FVector2D MousePos(0,0);
				UBOOL bInputConsumed = FALSE;
				if ( !IsCursorInputKey(EventParms.InputKeyName) || !UUIRoot::GetCursorPosition(MousePos, GetScene()) || ContainsPoint(MousePos) )
				{
					if ( DELEGATE_IS_SET(OnClicked) )
					{
						bInputConsumed = delegateOnClicked(this, EventParms.PlayerIndex);
					}

					// activate the on click event
					if( !bInputConsumed )
					{
						ActivateEventByClass(EventParms.PlayerIndex,UUIEvent_OnClick::StaticClass(), this);
					}
				}

				// deactivate the pressed state
				DeactivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			}
			bResult = TRUE;
		}
	}
	else if ( EventParms.InputAliasName == UIKEY_IncrementSliderValue )
	{
		// swallow this input event
		bResult = TRUE;
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Repeat )
		{
			FLOAT NewValue = GetValue() + SliderValue.GetNudgeValue();
			if ( SetValue( NewValue ) )
			{
				// Play the increment sound
				PlayUISound(IncrementCue,EventParms.PlayerIndex);

				NotifyValueChanged(EventParms.PlayerIndex);
			}
		}
	}
	else if ( EventParms.InputAliasName == UIKEY_DecrementSliderValue )
	{
		// swallow this input event
		bResult = TRUE;

		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Repeat )
		{
			FLOAT NewValue = GetValue() - SliderValue.GetNudgeValue();
			if ( SetValue( NewValue ) )
			{
				// Play the decrement sound
				PlayUISound(DecrementCue,EventParms.PlayerIndex);

				NotifyValueChanged(EventParms.PlayerIndex);
			}
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
}

/**
 * Processes input axis movement. Only called while the slider is in the pressed state; handles adjusting the slider
 * value and moving the marker to the appropriate position.
 *
 * Only called if this widget is in the owning scene's InputSubscribers map for the corresponding key.
 *
 * @param	PlayerIndex		index [into the Engine.GamePlayers array] of the player that generated this event
 * @param	Key				name of the key which an event occurred for.
 * @param	Delta 			the axis movement delta.
 * @param	DeltaTime		seconds since the last axis update.
 *
 * @return	TRUE to consume the axis movement, FALSE to pass it on.
 */
UBOOL UUISlider::ProcessInputAxis( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
//	debugf(NAME_Input, TEXT("%s::ProcessInputAxis	Key:%s (%s)"), *GetWidgetPathName(), *EventParms.InputKeyName.ToString(), *EventParms.InputAliasName.ToString());
	if ( EventParms.InputAliasName == UIKEY_DragSlider )
	{
		bResult = TRUE;

		FLOAT CursorLocationPercentage;
		if ( GetCursorPosition(CursorLocationPercentage) )
		{
			if ( SetValue(CursorLocationPercentage,TRUE) )
			{
				NotifyValueChanged(EventParms.PlayerIndex);
			}
		}
	}

	return bResult/* || Super::ProcessInputAxis(EventParms)*/;  //@todo ronp - should we call super here?
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUISlider::GetSupportedUIActionKeyNames( TArray<FName>& out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddItem(UIKEY_Clicked);
	out_KeyNames.AddItem(UIKEY_DragSlider);
	out_KeyNames.AddItem(UIKEY_DecrementSliderValue);
	out_KeyNames.AddItem(UIKEY_IncrementSliderValue);
}

/* === UObject interface === */

/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUISlider::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			// this represents the inner-most property that the user modified
			UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

			// if the value of the component itself was changed
			if ( MemberProperty == ModifiedProperty )
			{
				FName PropertyName = MemberProperty->GetFName();
				TScriptInterface<IUIStyleResolver> Resolver;

				if ( PropertyName == TEXT("BackgroundImageComponent") )
				{
					Resolver = BackgroundImageComponent;
				}
				else if ( PropertyName == TEXT("SliderBarImageComponent") )
				{
					Resolver = SliderBarImageComponent;
				}
				else if ( PropertyName == TEXT("MarkerImageComponent") )
				{
					Resolver = MarkerImageComponent;
				}
				else if ( PropertyName == TEXT("CaptionRenderComponent") )
				{
					Resolver = CaptionRenderComponent;
				}

				if ( Resolver )
				{
					// the user either cleared the value of the component or is assigning a new value to the component.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(Resolver);
				}
			}
		}
	}
}

/**
 * Called when a property value from a member struct or array has been changed in the editor.
 */
void UUISlider::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("DataSource") )
			{
				RefreshSubscriberValue();
			}
			else
			{
				// see if was one of our components
				TScriptInterface<IUIStyleResolver> Resolver;
				TScriptInterface<IUIStyleResolver> ResolverTemplate;

				FName StyleResolverTag=NAME_None;
				if ( PropertyName == TEXT("BackgroundImageComponent") )
				{
					Resolver = BackgroundImageComponent;
					ResolverTemplate = GetArchetype<UUISlider>()->BackgroundImageComponent;
				}
				else if ( PropertyName == TEXT("SliderBarImageComponent") )
				{
					Resolver = SliderBarImageComponent;
					ResolverTemplate = GetArchetype<UUISlider>()->SliderBarImageComponent;
				}
				else if ( PropertyName == TEXT("MarkerImageComponent") )
				{
					Resolver = MarkerImageComponent;
					ResolverTemplate = GetArchetype<UUISlider>()->MarkerImageComponent;
				}
				else if ( PropertyName == TEXT("CaptionRenderComponent") )
				{
					Resolver = CaptionRenderComponent;
					ResolverTemplate = GetArchetype<UUISlider>()->CaptionRenderComponent;
					StyleResolverTag = TEXT("Caption Style");
				}

				if ( Resolver )
				{
					// this represents the inner-most property that the user modified
					UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

					// if the value of the BackgroundImageComponent itself was changed
					if ( MemberProperty == ModifiedProperty )
					{
						if ( ResolverTemplate )
						{
							FName TemplateResolverTag = ResolverTemplate->GetStyleResolverTag();
							if ( TemplateResolverTag != NAME_None )
							{
								StyleResolverTag = TemplateResolverTag;
							}
						}

						if ( StyleResolverTag != NAME_None )
						{
							Resolver->SetStyleResolverTag(StyleResolverTag);
						}

						// user created a new background image component - add it to the list of style subscribers
						AddStyleSubscriber(Resolver);

						// now initialize the component's image
						UUIComp_DrawImage* ModifiedImageComponent = Cast<UUIComp_DrawImage>(Resolver.GetObject());
						if ( ModifiedImageComponent != NULL )
						{
							ModifiedImageComponent->SetImage(ModifiedImageComponent->GetImage());
						}
						else if ( Resolver == CaptionRenderComponent )
						{
							// now initialize the new string component
							TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
							CaptionRenderComponent->InitializeComponent(&Subscriber);

							// then initialize its style
							CaptionRenderComponent->NotifyResolveStyle(GetActiveSkin(), FALSE, GetCurrentState());

						}

						// initialize any style components with the current values from this slider
						RefreshSubscriberValue();
					}
					else
					{
#if 0
						// a property of the ImageComponent was changed
						if ( ModifiedProperty->GetFName() == TEXT("ImageRef") && ModifiedImageComponent->GetImage() != NULL )
						{
							USurface* CurrentValue = ModifiedImageComponent->GetImage();

							// changed the value of the image texture/material
							// clear the data store binding
							//@fixme ronp - do we always need to clear the data store binding?
							SetDataStoreBinding(TEXT(""));

							// clearing the data store binding value may have cleared the value of the image component's texture,
							// so restore the value now
							SetImage(CurrentValue);
						}
#endif
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Transfers the old individual range values to the new UIRangeData struct.
 */
void UUISlider::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake && GetLinkerVersion() < VER_FIXED_UISCROLLBARS )
	{
		if ( MinValue != 0 || NudgeValue != 0 || MaxValue != 0 || bIntValuesOnly == TRUE )
		{
			// if any of the old values are set, we'll assume this means that this slider is out of date
			SliderValue.MinValue = MinValue;
			SliderValue.MaxValue = MaxValue;
			SliderValue.bIntRange = bIntValuesOnly;
			SliderValue.SetNudgeValue(NudgeValue);
			SliderValue.SetCurrentValue(CurrentValue);

			MinValue = 0;
			MaxValue = 0;
			bIntValuesOnly = FALSE;
			NudgeValue = 0;
			CurrentValue = 0;
		}

		MigrateImageSettings(Background, BackgroundCoordinates, PrimaryStyle, BackgroundImageComponent, TEXT("Background"));
		MigrateImageSettings(SliderBar, BarCoordinates, BarStyle, SliderBarImageComponent, TEXT("SliderBar"));
		MigrateImageSettings(Marker, MarkerCoordinates, MarkerStyle, MarkerImageComponent, TEXT("Marker"));
	}
}


/* ==========================================================================================================
	UINumericEditbox
========================================================================================================== */
/**
* Initializes the buttons and creates the background image.
*
* @param	inOwnerScene	the scene to add this widget to.
* @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
*							is being added to the scene's list of children.
*/
void UUINumericEditBox::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	if ( IncrementButton == NULL )
	{
		const FString ButtonName = Localize(TEXT("UINumericEditBox"), TEXT("IncrementButtonName"), TEXT("Engine"), NULL, TRUE);
		IncrementButton = Cast<UUINumericEditBoxButton>(CreateWidget(this, UUINumericEditBoxButton::StaticClass(), NULL, FName(*ButtonName)));
	}

	if ( DecrementButton == NULL )
	{
		const FString ButtonName = Localize(TEXT("UINumericEditBox"), TEXT("DecrementButtonName"), TEXT("Engine"), NULL, TRUE);
		DecrementButton = Cast<UUINumericEditBoxButton>(CreateWidget(this, UUINumericEditBoxButton::StaticClass(), NULL, FName(*ButtonName)));
	}

	InsertChild( IncrementButton );
	InsertChild( DecrementButton );

	Super::Initialize(inOwnerScene, inOwner);

	// By default, place the buttons where they would usually be in a standard numeric editbox.
	IncrementButton->Position.SetPositionValue( this, 0.75f, UIFACE_Left, EVALPOS_PercentageOwner );
	IncrementButton->Position.SetPositionValue( this, 0.25f, UIFACE_Right, EVALPOS_PercentageOwner );
	IncrementButton->Position.SetPositionValue( this, 0.0f, UIFACE_Top, EVALPOS_PercentageOwner );
	IncrementButton->Position.SetPositionValue( this, 0.45f, UIFACE_Bottom, EVALPOS_PercentageOwner );

	DecrementButton->Position.SetPositionValue( this, 0.75f, UIFACE_Left, EVALPOS_PercentageOwner );
	DecrementButton->Position.SetPositionValue( this, 0.25f, UIFACE_Right, EVALPOS_PercentageOwner );
	DecrementButton->Position.SetPositionValue( this, 0.55f, UIFACE_Top, EVALPOS_PercentageOwner );
	DecrementButton->Position.SetPositionValue( this, 0.45f, UIFACE_Bottom, EVALPOS_PercentageOwner );

	//@todo ronp - what purpose does this serve?
	IncButton_Position = IncrementButton->Position;
	DecButton_Position = DecrementButton->Position;

	// Initialize the value string.
	ValidateNumericInputString( );
}

/**
* Render this editbox.
*
* @param	Canvas	the FCanvas to use for rendering this widget
*/
void UUINumericEditBox::Render_Widget( FCanvas* Canvas )
{
	if ( BackgroundImageComponent != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
		);

		BackgroundImageComponent->RenderComponent(Canvas, Parameters);
	}

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->Render_String(Canvas);
	}	
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUINumericEditBox::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddItem(UIKEY_IncrementNumericValue);
	out_KeyNames.AddItem(UIKEY_DecrementNumericValue);
}

/**
 * Evaluates the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUINumericEditBox::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition( Face );

	//@fixme ronp - this is dumb
	if ( GetNumResolvedFaces() == 0 )
	{
		IncrementButton->Position = IncButton_Position;
		DecrementButton->Position = DecButton_Position;
	}
}

/**
 * Determine whether the specified character should be displayed in the text field.
 */
UBOOL UUINumericEditBox::IsValidCharacter( TCHAR Character ) const
{
	return Super::IsValidCharacter( Character );
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUINumericEditBox::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* OutermostProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( OutermostProperty != NULL )
		{
			FName PropertyName = OutermostProperty->GetFName();
			if ( PropertyName == TEXT("IncButton_Position") || PropertyName == TEXT("DecButton_Position") )
			{
				RefreshPosition();
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Determines whether this widget should process the specified input event + state.  If the widget is configured
 * to respond to this combination of input key/state, any actions associated with this input event are activated.
 *
 * Only called if this widget is in the owning scene's InputSubscribers map for the corresponding key.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUINumericEditBox::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	if ( EventParms.InputAliasName == UIKEY_IncrementNumericValue || EventParms.InputAliasName == UIKEY_DecrementNumericValue )
	{
		UUINumericEditBoxButton* TargetButton = EventParms.InputAliasName == UIKEY_IncrementNumericValue
			? IncrementButton
			: DecrementButton;

		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			const UBOOL bIsDoubleClickPress = EventParms.EventType==IE_DoubleClick;

			if ( EventParms.InputAliasName == UIKEY_IncrementNumericValue )
			{
				IncrementValue(TargetButton, EventParms.PlayerIndex);
			}
			else
			{
				DecrementValue(TargetButton, EventParms.PlayerIndex);
			}

			// notify unrealscript
			if ( OBJ_DELEGATE_IS_SET(TargetButton,OnPressed) )
			{
				TargetButton->delegateOnPressed(TargetButton, EventParms.PlayerIndex);
			}

			if ( bIsDoubleClickPress && OBJ_DELEGATE_IS_SET(TargetButton,OnDoubleClick) )
			{
				TargetButton->delegateOnDoubleClick(TargetButton, EventParms.PlayerIndex);
			}

			// activate the pressed state
			TargetButton->ActivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);

			if ( bIsDoubleClickPress )
			{
				TargetButton->ActivateEventByClass(EventParms.PlayerIndex, UUIEvent_OnDoubleClick::StaticClass(), TargetButton);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Repeat )
		{
			if ( EventParms.InputAliasName == UIKEY_IncrementNumericValue )
			{
				IncrementValue(TargetButton, EventParms.PlayerIndex);
			}
			else
			{
				DecrementValue(TargetButton, EventParms.PlayerIndex);
			}
			if ( OBJ_DELEGATE_IS_SET(TargetButton,OnPressRepeat) )
			{
				TargetButton->delegateOnPressRepeat(TargetButton, EventParms.PlayerIndex);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released )
		{
			// Fire OnPressed Delegate
			if ( DELEGATE_IS_SET(OnPressRelease) )
			{
				TargetButton->delegateOnPressRelease(TargetButton, EventParms.PlayerIndex);
			}

			if ( TargetButton->IsPressed(EventParms.PlayerIndex) )
			{
				FVector2D MousePos(0,0);				
				UBOOL bInputConsumed = FALSE;
				if ( !IsCursorInputKey(EventParms.InputKeyName) || !GetCursorPosition(MousePos, GetScene()) || ContainsPoint(MousePos) )
				{
					if ( OBJ_DELEGATE_IS_SET(TargetButton,OnClicked) )
					{
						bInputConsumed = TargetButton->delegateOnClicked(TargetButton, EventParms.PlayerIndex);
					}

					// activate the on click event
					if( !bInputConsumed )
					{
						TargetButton->ActivateEventByClass(EventParms.PlayerIndex,UUIEvent_OnClick::StaticClass(), TargetButton);
					}
				}

				// deactivate the pressed state
				TargetButton->DeactivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);

				SetFocus(TargetButton);
			}
			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	return bResult || Super::ProcessInputKey(EventParms);
}

/**
 * Called whenever the user presses enter while this editbox is focused.  Activated the SubmitText kismet event and calls the
 * OnSubmitText delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to this method; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 */
void UUINumericEditBox::NotifySubmitText( INT PlayerIndex/*=INDEX_NONE*/ )
{	
	// Only submit the event to Kismet if the user entered valid data.
	if( ValidateNumericInputString( ) )
	{
		TArray<UUIEvent*> SubmitTextEvents;
		ActivateEventByClass(PlayerIndex, UUIEvent_SubmitTextData::StaticClass(), this, FALSE, NULL, &SubmitTextEvents);
		for ( INT EventIndex = 0; EventIndex < SubmitTextEvents.Num(); EventIndex++ )
		{
			UUIEvent_SubmitTextData* TextEvent = CastChecked<UUIEvent_SubmitTextData>(SubmitTextEvents(EventIndex));

			// set the event's string Value to the current value of the editbox.  this value will be copied into the
			// event's variable links via InitializeLinkedVariableValues().
			TextEvent->Value = StringRenderComponent->GetValue();
			TextEvent->PopulateLinkedVariableValues();
		}
	}

	// Reset the user string to our current value.
// 	StringRenderComponent->UserText = StringRenderComponent->ValueString->GetValue( );
	StringRenderComponent->SetCaretPosition( StringRenderComponent->GetUserTextLength() );
}

/**
 * Evaluates the value string of the string component to verify that it is a legit numeric value.
 */
UBOOL UUINumericEditBox::ValidateNumericInputString()
{
	FString ValueString = GetValue(TRUE);
	UBOOL bResult = ValueString.IsNumeric();

	// If the user has entered valid data then store it otherwise revert back to the previous valid entry.
	if( bResult )
	{
		FLOAT NewValue = appAtof(*ValueString);

		// Early out if the new value is the same as the old value.
		if( NumericValue.GetCurrentValue( ) == NewValue )
		{
			return bResult;
		}

		NumericValue.SetCurrentValue( NewValue );
	}

	SetNumericValue( NumericValue.GetCurrentValue(), bResult );
	return bResult;
}

/**
 * Increments the numeric editbox's value. 
 *
 * @param	Sender		Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
void UUINumericEditBox::IncrementValue( UUIScreenObject* Sender, INT PlayerIndex/*=INDEX_NONE*/ )
{
	FLOAT CurrentValue = NumericValue.GetCurrentValue( );

	if( ValidateNumericInputString() && CurrentValue < NumericValue.MaxValue )
	{
		CurrentValue += NumericValue.GetNudgeValue();
		
		SetNumericValue( CurrentValue );

		NotifySubmitText(PlayerIndex);
	}
}

/**
 * Decrements the numeric editbox's value.
 *
 * @param	Sender		Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
void UUINumericEditBox::DecrementValue( UUIScreenObject* Sender, INT PlayerIndex/*=INDEX_NONE*/ )
{
	FLOAT CurrentValue = NumericValue.GetCurrentValue( );

	if( ValidateNumericInputString() && CurrentValue > NumericValue.MinValue )
	{
		CurrentValue -= NumericValue.GetNudgeValue( );
		
		SetNumericValue( CurrentValue );

		NotifySubmitText(PlayerIndex);
	}
}

/**
 * Change the value of this numeric editbox at runtime. Takes care of conversion from float to internal value string.
 *
 * @param	NewValue			the new value for the editbox.
 *
 * @return	TRUE if the editbox's value was changed
 */
UBOOL UUINumericEditBox::SetNumericValue( FLOAT NewValue, UBOOL bForceRefreshString )
{
	UBOOL Result = FALSE;

	if( NumericValue.SetCurrentValue( NewValue ) )
	{
		// Restrict the display string to the desired number of decimal places.
		FString ValueString = FString::Printf( TEXT("%.*f"), DecimalPlaces > 0 ? DecimalPlaces : 4, NumericValue.GetCurrentValue( ) );

		StringRenderComponent->SetValue( ValueString );
		Result = TRUE;
	}

	return Result;
}

/**
 * Gets the current value of this numeric editbox.
 */
FLOAT UUINumericEditBox::GetNumericValue() const
{
	return NumericValue.GetCurrentValue( );
}

/** UIDataSourceSubscriber interface */
/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUINumericEditBox::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( DataSource.ResolveMarkup( this ) )
	{
		FUIProviderFieldValue ResolvedValue(EC_EventParm);
		if ( DataSource.GetBindingValue(ResolvedValue) )
		{
			if ( ResolvedValue.PropertyType == DATATYPE_RangeProperty && ResolvedValue.RangeValue.HasValue() )
			{
				NumericValue = ResolvedValue.RangeValue;
				SetNumericValue( NumericValue.GetCurrentValue( ) );
				bResult = TRUE;
			}
			else if ( ResolvedValue.StringValue.Len() > 0 )
			{
				SetValue(ResolvedValue.StringValue);
				bResult = ValidateNumericInputString();
			}
		}
	}

	return bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUINumericEditBox::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);
	if ( DataSource )
	{
		out_BoundDataStores.AddUniqueItem(*DataSource);
	}
}

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUINumericEditBox::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	FUIProviderScriptFieldValue Value(EC_EventParm);
	Value.PropertyType = DATATYPE_RangeProperty;
	Value.StringValue = GetValue(TRUE);

	// add the data store bound to this slider to the list
	GetBoundDataStores(out_BoundDataStores);

	return DataSource.SetBindingValue(Value);
}

/* ==========================================================================================================
	UIComboBox
========================================================================================================== */
/* === UUIComboBox interface === */
/**
 * Creates the support controls which make up the combo box - the list, button, and editbox.
 *
 * @todo - verify that we're setting the correct private behaviors on these internal widgets
 */
void UUIComboBox::CreateInternalControls()
{
	if ( !IsTemplate(RF_ClassDefaultObject) )
	{
		INT ControlFlags=/*UCONST_PRIVATE_NotFocusable|*/UCONST_PRIVATE_NotDockable|UCONST_PRIVATE_Protected;
		if ( ComboEditbox == NULL )
		{
			// first, give script an opportunity to create a customized component
			if ( DELEGATE_IS_SET(CreateCustomComboEditbox) )
			{
				ComboEditbox = delegateCreateCustomComboEditbox(this);
			}

			if ( ComboEditbox == NULL )
			{
				// no custom component - use the configured class to create it
				UClass* EditboxClass = ComboEditboxClass;
				if ( EditboxClass == NULL )
				{
					warnf(TEXT("NULL ComboEditboxClass detected for '%s'.  Defaulting to Engine.UIEditBox"));
					EditboxClass = UUIEditBox::StaticClass();
				}

				// script compiler won't allow the wrong type to be assigned, but check to make sure it wasn't set natively to the wrong class.
				checkfSlow(EditboxClass && EditboxClass->IsChildOf(UUIEditBox::StaticClass()),
					TEXT("Invalid value assigned to ComboEditboxClass for '%s': %s"), 
					*GetFullName(), *EditboxClass->GetPathName());

				ComboEditbox = CastChecked<UUIEditBox>(CreateWidget(this, EditboxClass, GetArchetype<UUIComboBox>()->ComboEditbox, TEXT("ComboEditbox")));

				//@todo ronp - for now, we'll hardcode the values but eventually we should support a way for the designer to configure the internal layout
				ComboEditbox->SetDockTarget(UIFACE_Top, this, UIFACE_Top);
				ComboEditbox->SetDockTarget(UIFACE_Bottom, this, UIFACE_Bottom);

				// default to read-only combo editboxes
				ComboEditbox->bReadOnly = TRUE;
			}

			// now insert the widget into our children array and apply any constraints
			verify(InsertChild(ComboEditbox, 0, FALSE) != INDEX_NONE);
			ComboEditbox->SetPrivateBehavior(ControlFlags, TRUE, TRUE);
		}

		if ( ComboButton == NULL )
		{
			// first, give script an opportunity to create a customized component
			if ( DELEGATE_IS_SET(CreateCustomComboButton) )
			{
				ComboButton = delegateCreateCustomComboButton(this);
			}

			if ( ComboButton == NULL )
			{
				// no custom component - use the configured class to create it
				UClass* ButtonClass = ComboButtonClass;
				if ( ButtonClass == NULL )
				{
					warnf(TEXT("NULL ComboButtonClass detected for '%s'.  Defaulting to Engine.UIToggleButton"));
					ButtonClass = UUIToggleButton::StaticClass();
				}

				// script compiler won't allow the wrong type to be assigned, but check to make sure it wasn't set natively to the wrong class.
				checkfSlow(ButtonClass && ButtonClass->IsChildOf(UUIToggleButton::StaticClass()),
					TEXT("Invalid value assigned to ComboButtonClass for '%s': %s"), 
					*GetFullName(), *ButtonClass->GetPathName());

				ComboButton = CastChecked<UUIToggleButton>(CreateWidget(this, ButtonClass, GetArchetype<UUIComboBox>()->ComboButton, TEXT("ComboButton")));

				//@todo ronp - for now, we'll hardcode the values but eventually we should support a way for the designer to configure the internal layout

				// make sure the combo button is docked on all sides except the left
				ComboButton->SetDockTarget(UIFACE_Top, this, UIFACE_Top);
				ComboButton->SetDockTarget(UIFACE_Right, this, UIFACE_Right);
				ComboButton->SetDockTarget(UIFACE_Bottom, this, UIFACE_Bottom);

				// lock the combo button's width so that it's width remains the same even if the combobox is resized
				//ComboButton->DockTargets.LockWidth(TRUE);
				ComboButton->Position.SetRawPositionValue(UIFACE_Left, 224.f, EVALPOS_PixelOwner);

				// dock the right face of the editbox to the left face of the button. The button is always square (width determined by the height of the
				// combo box) and the editbox takes up the remaining space
				ComboEditbox->SetDockTarget(UIFACE_Right, ComboButton, UIFACE_Left);
			}

			// now insert the widget into our children array and apply any constraints
			verify(InsertChild(ComboButton, 1, FALSE) != INDEX_NONE);
			ComboButton->SetPrivateBehavior(ControlFlags, TRUE, TRUE);
		}

		if ( ComboList == NULL )
		{
			// first, give script an opportunity to create a customized component
			if ( DELEGATE_IS_SET(CreateCustomComboList) )
			{
				ComboList = delegateCreateCustomComboList(this);
			}

			if ( ComboList == NULL )
			{
				// no custom component - use the configured class to create it
				UClass* ListClass = ComboListClass;
				if ( ListClass == NULL )
				{
					warnf(TEXT("NULL ComboListClass detected for '%s'.  Defaulting to Engine.UIList"));
					ListClass = UUIList::StaticClass();
				}

				// script compiler won't allow the wrong type to be assigned, but check to make sure it wasn't set natively to the wrong class.
				checkfSlow(ListClass && ListClass->IsChildOf(UUIList::StaticClass()),
					TEXT("Invalid value assigned to ComboListClass for '%s': %s"), 
					*GetFullName(), *ListClass->GetPathName());

				ComboList = CastChecked<UUIList>(CreateWidget(this, ListClass, GetArchetype<UUIComboBox>()->ComboList, TEXT("ComboList")));

				ComboList->eventSetVisibility(FALSE);

				ComboList->SetDockTarget(UIFACE_Left, ComboEditbox, UIFACE_Left);
				ComboList->SetDockTarget(UIFACE_Top, this, UIFACE_Bottom);
				ComboList->SetDockTarget(UIFACE_Right, this, UIFACE_Right);

				//@fixme ronp - just to get us started, make the list's default height such that it can display 8 elements assuming a font height of 16 pixels
				// eventually this will be changed so that the designer can define how many elements should be displayed, and the list auto-sizes itself according
				// to the size of the font used for rendering the elements.
				ComboList->Position.SetRawPositionValue(UIFACE_Bottom, 16 * 8, EVALPOS_PixelOwner);
				ComboList->SetRowCount(8);
				ComboList->EnableColumnHeaderRendering(FALSE);
				ComboList->SetHotTracking(TRUE);
				ComboList->CellLinkType = LINKED_None;
				ComboList->bSingleClickSubmission = TRUE;
			}

			// now insert the widget into our children array and apply any constraints
			verify(InsertChild(ComboList, 2, FALSE) != INDEX_NONE);
			ComboList->SetPrivateBehavior(ControlFlags, TRUE, TRUE);
		}
	}
}

/* === UIObject interface === */
/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUIComboBox::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(CaptionRenderComponent);
	VALIDATE_COMPONENT(BackgroundRenderComponent);

	AddStyleSubscriber(CaptionRenderComponent);
	AddStyleSubscriber(BackgroundRenderComponent);
}

/**
 * Called whenever the selected item is modified.  Activates the SliderValueChanged kismet event and calls the OnValueChanged
 * delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to this method; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
 */
void UUIComboBox::NotifyValueChanged( INT PlayerIndex/*=INDEX_NONE*/, INT NotifyFlags/*=0*/ )
{
	if ( PlayerIndex == INDEX_NONE )
	{
		PlayerIndex = GetBestPlayerIndex();
	}

	Super::NotifyValueChanged(PlayerIndex, NotifyFlags);

	if ( GIsGame )
	{
		TArray<INT> IndicesToActivate;
		const UBOOL bTextChanged = (NotifyFlags&UCONST_TEXT_CHANGED_NOTIFY_MASK) != 0;
		const UBOOL bIndexChanged = !bTextChanged && (NotifyFlags&UCONST_INDEX_CHANGED_NOTIFY_MASK) != 0;
		if ( bTextChanged )
		{
			IndicesToActivate.AddItem(0);
		}
		else if ( bIndexChanged )
		{
			IndicesToActivate.AddItem(1);
		}

		FString TextValue;
		if ( ComboEditbox != NULL && ComboEditbox->StringRenderComponent != NULL )
		{
			TextValue = ComboEditbox->GetValue(TRUE);
		}

		INT CurrentItem = INDEX_NONE, CurrentIndex = INDEX_NONE;
		if ( ComboList != NULL )
		{
			CurrentItem = ComboList->GetCurrentItem();
			CurrentIndex = ComboList->Index;
		}
		TArray<UUIEvent_ComboboxValueChanged*> ActivatedEvents;
		ActivateEventByClass(PlayerIndex, UUIEvent_ComboboxValueChanged::StaticClass(), this, FALSE, &IndicesToActivate, (TArray<UUIEvent*>*)&ActivatedEvents);
		for ( INT EventIdx = 0; EventIdx < ActivatedEvents.Num(); EventIdx++ )
		{
			UUIEvent_ComboboxValueChanged* Event = ActivatedEvents(EventIdx);

			// copy the current value of the editbox into the "Text Value" variable link
			TArray<FString*> StringVars;
			Event->GetStringVars(StringVars,TEXT("Text Value"));
			for (INT Idx = 0; Idx < StringVars.Num(); Idx++)
			{
				*(StringVars(Idx)) = TextValue;
			}
	
			// copy the value of the list's selected items into the "Current Item" variable link
			TArray<INT*> IntVars;
			Event->GetIntVars(IntVars, TEXT("Current Item"));
			for ( INT Idx = 0; Idx < IntVars.Num(); Idx++ )
			{
				*(IntVars(Idx)) = CurrentItem;
			}

			// copy the list's current index into the "List Index" variable link
			IntVars.Empty();
			Event->GetIntVars(IntVars, TEXT("Current List Index"));
			for (INT Idx = 0; Idx < IntVars.Num(); Idx++)
			{
				*(IntVars(Idx)) = CurrentIndex;
			}
		}
	}
}

/**
 * Activates the UIState_Focused menu state and updates the pertinent members of FocusControls.
 *
 * @param	FocusedChild	the child of this widget that should become the "focused" control for this widget.
 *							A value of NULL indicates that there is no focused child.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 */
UBOOL UUIComboBox::GainFocus( UUIObject* FocusedChild, INT PlayerIndex )
{
//	tracef(TEXT("ComboBox::GainFocus  FocusedChild: %s"), FocusedChild != NULL ? *FocusedChild->GetTag().ToString() : TEXT("NULL"));
	UBOOL bResult = Super::GainFocus(FocusedChild, PlayerIndex);

//	tracef(TEXT("ComboBox::GainFocus  FocusedChild: %s"), FocusedChild != NULL ? *FocusedChild->GetTag().ToString() : TEXT("NULL"));
	return bResult;
}

/**
 * Deactivates the UIState_Focused menu state and updates the pertinent members of FocusControls.
 *
 * @param	FocusedChild	the child of this widget that is currently "focused" control for this widget.
 *							A value of NULL indicates that there is no focused child.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 */
UBOOL UUIComboBox::LoseFocus( UUIObject* FocusedChild, INT PlayerIndex )
{
//	tracef(TEXT("ComboBox::LoseFocus  FocusedChild: %s"), FocusedChild != NULL ? *FocusedChild->GetTag().ToString() : TEXT("NULL"));
	UBOOL bResult = Super::LoseFocus(FocusedChild, PlayerIndex);

	// if the list is losing focus, hide it.
	if ( FocusedChild != NULL && FocusedChild == ComboList )
	{
		eventHideList();
	}

//	tracef(TEXT("ComboBox::LoseFocus  FocusedChild: %s"), FocusedChild != NULL ? *FocusedChild->GetTag().ToString() : TEXT("NULL"));
	return bResult;
}

/**
 * Activates the focused state for this widget and sets it to be the focused control of its parent (if applicable)
 *
 * @param	Sender		Control that called SetFocus.  Possible values are:
 *						-	if NULL is specified, it indicates that this is the first step in a focus change.  The widget will
 *							attempt to set focus to its most eligible child widget.  If there are no eligible child widgets, this
 *							widget will enter the focused state and start propagating the focus chain back up through the Owner chain
 *							by calling SetFocus on its Owner widget.
 *						-	if Sender is the widget's owner, it indicates that we are in the middle of a focus change.  Everything else
 *							proceeds the same as if the value for Sender was NULL.
 *						-	if Sender is a child of this widget, it indicates that focus has been successfully changed, and the focus is now being
 *							propagated upwards.  This widget will now enter the focused state and continue propagating the focus chain upwards through
 *							the owner chain.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 */
UBOOL UUIComboBox::SetFocus( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	checkSlow(PlayerIndex<FocusControls.Num());
	checkSlow(PlayerIndex<FocusPropagation.Num());

	if ( IsVisible() )
	{
		// if Sender == NULL, then SetFocus was called on this widget directly.  We should either propagate focus to our
		// most appropriate child widget (if possible), or set ourselves to be the focused control
		UBOOL bSetFocusToChild = (Sender != NULL && Sender == GetParent()) || (Sender == NULL && CanAcceptFocus(PlayerIndex));
		if ( bSetFocusToChild )
		{
			// When the focus chain is going down, make sure that the LastFocusedControl is always the ComboEditbox so that
			// it's always the one that receives focus, even if the list or button had focus last.
			FocusControls(PlayerIndex).SetLastFocusedControl( ComboEditbox );
		}
	}

	return Super::SetFocus(Sender, PlayerIndex);
}

/**
 * Sets focus to the specified child of this widget.
 *
 * @param	ChildToFocus	the child to set focus to.  If not specified, attempts to set focus to the most eligible child,
 *							as determined by navigation links and FocusPropagation values.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 */
UBOOL UUIComboBox::SetFocusToChild( UUIObject* ChildToFocus/*=NULL*/, INT PlayerIndex/*=0*/ )
{
	return Super::SetFocusToChild(ChildToFocus, PlayerIndex);
}

/**
 * Deactivates the focused state for this widget.
 *
 * @param	Sender			the control that called KillFocus.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 */
UBOOL UUIComboBox::KillFocus( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	return Super::KillFocus(Sender, PlayerIndex);
}

/* === UUIScreenObject interface === */
/**
 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
 * once the scene has been completely initialized.
 * For widgets added at runtime, called after the widget has been inserted into its parent's
 * list of children.
 *
 * Creates the components of this combobox, if necessary.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUIComboBox::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	// If the internal controls of this combobox have never been created, do that now.
	CreateInternalControls();

	// initialize the caption rendering component
	if ( CaptionRenderComponent != NULL )
	{
		TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
		CaptionRenderComponent->InitializeComponent(&Subscriber);
	}

	Super::Initialize(inOwnerScene, inOwner);
}

/**
 * Assigns values to the links which are used for navigating through this widget using the keyboard.  Sets the first and
 * last focus targets for this widget as well as the next/prev focus targets for all children of this widget.
 */
void UUIComboBox::RebuildKeyboardNavigationLinks()
{
	// first, perform the default behavior in case we have additional children
	Super::RebuildKeyboardNavigationLinks();


	if ( ComboEditbox != NULL )
	{
		for ( INT PlayerIndex = 0; PlayerIndex < FocusPropagation.Num(); PlayerIndex++ )
		{
			FocusPropagation(PlayerIndex).SetFirstFocusTarget(ComboEditbox);
			FocusPropagation(PlayerIndex).SetLastFocusTarget(ComboButton);

			ComboEditbox->FocusPropagation(PlayerIndex).SetPrevFocusTarget(NULL);
			ComboEditbox->FocusPropagation(PlayerIndex).SetNextFocusTarget(ComboButton);
		}
	}

	if ( ComboButton != NULL )
	{
		for ( INT PlayerIndex = 0; PlayerIndex < FocusPropagation.Num(); PlayerIndex++ )
		{
			ComboButton->FocusPropagation(PlayerIndex).SetPrevFocusTarget(ComboEditbox);
			ComboButton->FocusPropagation(PlayerIndex).SetNextFocusTarget(ComboList);
		}
	}

	if ( ComboList != NULL )
	{
		for ( INT PlayerIndex = 0; PlayerIndex < FocusPropagation.Num(); PlayerIndex++ )
		{
			ComboList->FocusPropagation(PlayerIndex).SetPrevFocusTarget(ComboButton);
			ComboList->FocusPropagation(PlayerIndex).SetNextFocusTarget(NULL);
		}
	}
}

/**
 * Adds the specified face to the DockingStack, along with any dependencies that face might have.
 *
 * This version routes the call to the CaptionRenderComponent, if applicable.
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUIComboBox::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	UBOOL bResult = FALSE;
	
	// if we don't have a caption or the caption doesn't care about this face, just do the default behavior.
	if ( CaptionRenderComponent != NULL )
	{
		bResult = CaptionRenderComponent->AddDockingNode(DockingStack, Face);
	}
	else
	{
		bResult = Super::AddDockingNode(DockingStack, Face);
	}

	return bResult;
}

/**
 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUIComboBox::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	//@todo ronp - hmmm, if the caption is not configured to auto-expand, we might need to route the call to the
	// caption render component after adjusting the size of the button so that its string is clipped properly
	if ( CaptionRenderComponent != NULL )
	{
		CaptionRenderComponent->ResolveFacePosition(Face);
	}

	if ( GetNumResolvedFaces() == UIFACE_MAX )
	{
		// adjust the width of the button to ensure that its width always matches its height.
		// @todo ronp - eventually we may want to make this an option (like bConstrainButtonSize or something)
		const FLOAT ComboBoxHeight = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];

		ComboButton->Position.SetPositionValue(ComboButton, RenderBounds[UIFACE_Right] - ComboBoxHeight, UIFACE_Left, EVALPOS_PixelViewport, FALSE);
		if ( ComboButton->DockTargets.bResolved[UIFACE_Left] != 0 )
		{
			// It's already resolved its left face, so we'll need to update everything manually
			ComboButton->ResolveFacePosition(UIFACE_Left);
		}
	}
}

/**
 * Render this widget.  This version routes the render call to the draw components, if applicable.
 *
 * @param	Canvas	the canvas to use for rendering this widget
 */
void UUIComboBox::Render_Widget( FCanvas* Canvas )
{
	// No need to call Super as it's pure virtual - just render the components if applicable
	if ( BackgroundRenderComponent != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
		);

		BackgroundRenderComponent->RenderComponent(Canvas, Parameters);
	}

	if ( CaptionRenderComponent != NULL )
	{
		CaptionRenderComponent->Render_String(Canvas);
	}
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUIComboBox::GetSupportedUIActionKeyNames( TArray<FName>& out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	// add clicked to allow the combo box to respond to mouse clicks on the caption portion
	// the individual controls will handle input to their areas.
	out_KeyNames.AddUniqueItem(UIKEY_Clicked);
}

/**
 * Handles input events for this slider.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIComboBox::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	// Pressed Event
	if ( EventParms.InputAliasName == UIKEY_Clicked )
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			const UBOOL bIsDoubleClickPress = EventParms.EventType==IE_DoubleClick;

			// notify unrealscript
			if ( DELEGATE_IS_SET(OnPressed) )
			{
				delegateOnPressed(this, EventParms.PlayerIndex);
			}

			if ( bIsDoubleClickPress && DELEGATE_IS_SET(OnDoubleClick) )
			{
				delegateOnDoubleClick(this, EventParms.PlayerIndex);
			}

			// activate the pressed state
			ActivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			if ( bIsDoubleClickPress )
			{
				ActivateEventByClass(EventParms.PlayerIndex, UUIEvent_OnDoubleClick::StaticClass(), this);
			}
			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Repeat )
		{
			if ( DELEGATE_IS_SET(OnPressRepeat) )
			{
				delegateOnPressRepeat(this, EventParms.PlayerIndex);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released )
		{
			// Fire OnPressed Delegate
			if ( DELEGATE_IS_SET(OnPressRelease) )
			{
				delegateOnPressRelease(this, EventParms.PlayerIndex);
			}

			if ( IsPressed(EventParms.PlayerIndex) )
			{
				FVector2D MousePos(0,0);				
				UBOOL bInputConsumed = FALSE;
				if ( !IsCursorInputKey(EventParms.InputKeyName) || !GetCursorPosition(MousePos, GetScene()) || ContainsPoint(MousePos) )
				{
					if ( DELEGATE_IS_SET(OnClicked) )
					{
						bInputConsumed = delegateOnClicked(this, EventParms.PlayerIndex);
					}

					// activate the on click event
					if( !bInputConsumed )
					{
						ActivateEventByClass(EventParms.PlayerIndex,UUIEvent_OnClick::StaticClass(), this);
					}
				}

				// deactivate the pressed state
				DeactivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			}
			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
}

/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUIComboBox::InvalidatePositionDependencies( BYTE Face )
{
	Super::InvalidatePositionDependencies(Face);

	if ( CaptionRenderComponent != NULL )
	{
		CaptionRenderComponent->InvalidatePositionDependencies(Face);
	}
}

/* === UObject interface === */
/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIComboBox::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("LabelBackground") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty && BackgroundRenderComponent != NULL )
				{
					// the user either cleared the value of the BackgroundRenderComponent or is assigning a new value to it.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(BackgroundRenderComponent);
				}
			}
			else if ( PropertyName == TEXT("CaptionRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty && CaptionRenderComponent != NULL )
				{
					// the user either cleared the value of the CaptionRenderComponent or is assigning a new value to it.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(CaptionRenderComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value from a member struct or array has been changed in the editor.
 */
void UUIComboBox::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UBOOL bHandled = FALSE;
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
/*
			@todo ronp - not sure if we need this yet.
			if ( PropertyName == TEXT("CaptionDataSource") )
			{
				bHandled = TRUE;
				if ( RefreshSubscriberValue() && CaptionRenderComponent != NULL )
				{
					if ( CaptionRenderComponent->AutoSizeParameters[UIORIENT_Horizontal].bAutoSizeEnabled || CaptionRenderComponent->AutoSizeParameters[UIORIENT_Vertical].bAutoSizeEnabled || 
						(CaptionRenderComponent->WrapMode != CLIP_Normal && CaptionRenderComponent->WrapMode != CLIP_None) )
					{
						RefreshPosition();
					}
				}
			}

			else*/ if ( PropertyName == TEXT("BackgroundRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( BackgroundRenderComponent != NULL )
					{
						// the newly created image component won't have the correct StyleResolverTag, so fix that now
						UUIComp_DrawImage* BackgroundComponentTemplate = GetArchetype<UUIComboBox>()->BackgroundRenderComponent;
						if ( BackgroundComponentTemplate != NULL )
						{
							BackgroundRenderComponent->StyleResolverTag = BackgroundComponentTemplate->StyleResolverTag;
						}
						else
						{
							BackgroundRenderComponent->StyleResolverTag = TEXT("Caption Background Style");
						}

						// user created a new caption background component - add it to the list of style subscribers
						AddStyleSubscriber(BackgroundRenderComponent);

						// finally initialize the component's image
						BackgroundRenderComponent->SetImage(BackgroundRenderComponent->GetImage());
					}
				}
			}
			else if ( PropertyName == TEXT("CaptionRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the ImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					// if the user wasn't clearing the value
					if ( CaptionRenderComponent != NULL )
					{
						UUIComp_DrawCaption* CaptionRenderTemplate = GetArchetype<UUIComboBox>()->CaptionRenderComponent;
						if ( CaptionRenderTemplate != NULL )
						{
							CaptionRenderComponent->StyleResolverTag = CaptionRenderTemplate->StyleResolverTag;
						}
						else
						{
							CaptionRenderComponent->StyleResolverTag = UUIComp_DrawCaption::StaticClass()->GetDefaultObject<UUIComp_DrawCaption>()->StyleResolverTag;
						}

						// user added created a new string render component - add it to the list of style subscribers
						AddStyleSubscriber(CaptionRenderComponent);

						// now initialize the new string component
						TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
						CaptionRenderComponent->InitializeComponent(&Subscriber);

						// then initialize its style
						CaptionRenderComponent->NotifyResolveStyle(GetActiveSkin(), FALSE, GetCurrentState());

						// finally initialize its text
						RefreshSubscriberValue();
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/* === IUIDataStoreSubscriber interface === */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									0:	list data source
 *									1:	caption data source
 */
void UUIComboBox::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=INDEX_NONE*/ )
{
#if 0
	if ( BindingIndex == ListDataSource.BindingIndex )
	{
		// list data source
	}
#endif
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( BindingIndex == INDEX_NONE || BindingIndex == CaptionDataSource.BindingIndex )
	{
		// caption data source
		if ( MarkupText != CaptionDataSource.MarkupString )
		{
			Modify();
			CaptionDataSource.MarkupString = MarkupText;
		}

		RefreshSubscriberValue(BindingIndex);
	}
}

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									0:	list data source
 *									1:	caption data source
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUIComboBox::GetDataStoreBinding( INT BindingIndex/*=INDEX_NONE*/ ) const
{
	FString Result;
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}

#if 0
	if ( BindingIndex == INDEX_NONE || BindingIndex == ListDataSource.BindingIndex )
	{
		// list data source
	}
#endif
	if ( BindingIndex == INDEX_NONE || BindingIndex == CaptionDataSource.BindingIndex )
	{
		// caption data source
		Result = CaptionDataSource.MarkupString;
	}

	return Result;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									-1:	all data sources
 *									0:	list data source
 *									1:	caption data source
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUIComboBox::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;

#if 0
	if ( BindingIndex == INDEX_NONE || BindingIndex == ListDataSource.BindingIndex )
	{
		// list data source
	}
#endif
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( BindingIndex == INDEX_NONE || BindingIndex == CaptionDataSource.BindingIndex )
	{
		// caption data source
		if ( CaptionRenderComponent != NULL && IsInitialized() )
		{
			// resolve the value of this label into a renderable string
			CaptionRenderComponent->SetValue(CaptionDataSource.MarkupString);
			CaptionRenderComponent->ReapplyFormatting();
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUIComboBox::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);
	if ( CaptionRenderComponent != NULL )
	{
		CaptionRenderComponent->GetResolvedDataStores(out_BoundDataStores);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUIComboBox::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);
	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}


/* ==========================================================================================================
	UUIMeshWidget
========================================================================================================== */
/**
 * Attach and initialize any 3D primitives for this widget and its children.
 *
 * @param	CanvasScene		the scene to use for attaching 3D primitives
 */
void UUIMeshWidget::InitializePrimitives( FCanvasScene* CanvasScene )
{
	// initialize the static mesh component
	if ( Mesh != NULL )
	{
		Mesh->ConditionalDetach();
		Mesh->ConditionalAttach(CanvasScene,NULL,GenerateTransformMatrix());
	}

	Super::InitializePrimitives(CanvasScene);
}

/**
 * Updates 3D primitives for this widget.
 *
 * @param	CanvasScene		the scene to use for updating any 3D primitives
 */
void UUIMeshWidget::UpdateWidgetPrimitives( FCanvasScene* CanvasScene )
{
	// update the static mesh component's transform
	if ( Mesh != NULL )
	{
		FMatrix LocalToScreenMatrix = GetPrimitiveTransform(this);

#if 0

		static bool bFlag=false;
		FVector PosVect = GetPositionVector(TRUE);

		FMatrix LocalToCanvasMatrix = GetCanvasToScreen();
		FMatrix InvLocalToCanvasMatrix = GetInverseCanvasToScreen();

		FMatrix InvLocalToScreenMatrix = LocalToScreenMatrix.Inverse();

		FVector CanvasPos = LocalToCanvasMatrix.TransformFVector(PosVect);
		FVector CanvasPosInv = InvLocalToCanvasMatrix.TransformFVector(PosVect);
		FVector CanvasPosInv2 = LocalToCanvasMatrix.InverseTransformFVector(PosVect);
		FVector NormCanvasPos = LocalToCanvasMatrix.TransformNormal(PosVect);
		FVector NormCanvasPosInv = InvLocalToCanvasMatrix.TransformNormal(PosVect);
		FVector NormCanvasPosInv2 = LocalToCanvasMatrix.InverseTransformNormal(PosVect);

		FVector ScreenPos = LocalToScreenMatrix.TransformFVector(PosVect);
		FVector ScreenPosInv = InvLocalToScreenMatrix.TransformFVector(PosVect);
		FVector ScreenPosInv2 = LocalToScreenMatrix.InverseTransformFVector(PosVect);
		FVector NormScreenPos = LocalToScreenMatrix.TransformNormal(PosVect);
		FVector NormScreenPosInv = InvLocalToScreenMatrix.TransformNormal(PosVect);
		FVector NormScreenPosInv2 = LocalToScreenMatrix.InverseTransformNormal(PosVect);

		if ( bFlag )
		{
			Mesh->ConditionalUpdateTransform(LocalToCanvasMatrix);
		}
		else
		{
			Mesh->ConditionalUpdateTransform(LocalToScreenMatrix);
		}

#else

		//@fixme ronp - this doesn't take into account whether the parent is hidden
		Mesh->SetHiddenGame(IsHidden());
		Mesh->SetHiddenEditor(IsHidden());
		Mesh->ConditionalUpdateTransform(LocalToScreenMatrix);

#endif

	}

	Super::UpdateWidgetPrimitives(CanvasScene);
}

/* ==========================================================================================================
	UUIToolTip
========================================================================================================== */
/** === UUIToolTip interface === */
/**
 * Updates SecondsActive, hiding or showing the tooltip when appropriate.
 */
void UUIToolTip::TickToolTip( FLOAT DeltaTime )
{
	UUIInteraction* UIController = UUIRoot::GetCurrentUIController();
	if ( UIController != NULL )
	{
		SecondsActive += DeltaTime;
		if ( IsHidden() )
		{
			if ( SecondsActive > UIController->ToolTipInitialDelaySeconds
			&&	(UIController->ToolTipExpirationSeconds == 0.0 || SecondsActive <= UIController->ToolTipExpirationSeconds) )
			{
				eventSetVisibility(TRUE);
				UpdateToolTipPosition();
			}
		}
		else if ( UIController->ToolTipExpirationSeconds > 0.f && SecondsActive > UIController->ToolTipExpirationSeconds )
		{
			eventSetVisibility(FALSE);
			bPendingPositionUpdate = FALSE;
		}
	}
}

/**
 * Sets this tool-tip's displayed text to the resolved value of the specified data store binding.
 */
void UUIToolTip::LinkBinding( FUIDataStoreBinding* ToolTipBinding )
{
	if ( ToolTipBinding != NULL
	&&	(DataSource != *ToolTipBinding) )
	{
		DataSource = *ToolTipBinding;
		RefreshSubscriberValue();
	}
}

/**
 * Resolves the tool tip's Position values into pixel values and formats the tool tip string, if bPendingPositionUpdate is TRUE.
 */
void UUIToolTip::ResolveToolTipPosition()
{
	if ( bResolveToolTipPosition )
	{
		TArray<FUIDockingNode> DockingStack;
		AddDockingLink(DockingStack);

		for ( INT StackIndex = 0; StackIndex < DockingStack.Num(); StackIndex++ )
		{
			FUIDockingNode& DockingNode = DockingStack(StackIndex);
			DockingNode.Widget->DockTargets.bResolved[DockingNode.Face] = 0;
			DockingNode.Widget->Position.InvalidatePosition(DockingNode.Face);
		}

		for ( INT StackIndex = 0; StackIndex < DockingStack.Num(); StackIndex++ )
		{
			FUIDockingNode& DockingNode = DockingStack(StackIndex);
			DockingNode.Widget->ResolveFacePosition( (EUIWidgetFace)DockingNode.Face );
		}

		bResolveToolTipPosition = FALSE;
	}
}

/**
 * Initializes the timer used to determine when this tooltip should be shown.  Called when the a widet that supports a tooltip
 * becomes the scene client's ActiveControl.
 *
 * @return	returns the tooltip that began tracking, or None if no tooltips were activated.
 */
UUIToolTip* UUIToolTip::BeginTracking()
{
// 	StartTime = ActivationTime;

	UUIScene* OwnerScene = GetScene();
	if ( OwnerScene != NULL
	&& (OwnerScene->GetActiveToolTip() != NULL && OwnerScene->GetActiveToolTip() != this && OwnerScene->GetActiveToolTip()->IsVisible()) )
	{
		// if another tooltip is already being displayed, skip the initial activation delay.  I think this can only happen
		// if the currently active tooltip doesn't want to allow itself to be deactivated.
		UUIInteraction* UIController = UUIRoot::GetCurrentUIController();
		if ( UIController != NULL )
		{
			SecondsActive = UIController->ToolTipInitialDelaySeconds;
		}
		else
		{
			SecondsActive = 0.f;
		}
	}
	else
	{
		SecondsActive = 0.f;
	}

	bPendingPositionUpdate = TRUE;
	return this;
}

/**
 * Hides the tooltip and resets all internal tracking variables.  Called when a widget that supports tooltips is no longer
 * the scene client's ActiveControl.
 *
 * @return	FALSE if the tooltip wishes to abort the deactivation and continue displaying the tooltip.  The return value
 * 			may be ignored if the UIInteraction needs to force the tooltip to disappear, for example if the scene is being
 *			closed or a context menu is being activated.
 */
UBOOL UUIToolTip::EndTracking()
{
	SecondsActive = -1.f;

	eventSetVisibility(FALSE);
	UUIScene* OwnerScene = GetScene();
	if ( OwnerScene != NULL && OwnerScene->GetActiveToolTip() == this )
	{
		OwnerScene->SetActiveToolTip(NULL);
	}

	bPendingPositionUpdate = bResolveToolTipPosition = FALSE;
	return TRUE;
}

/**
 * Determines the best X position for the tooltip, based on the tool-tip's width and the current mouse position
 */
FLOAT UUIToolTip::GetIdealLeft() const
{
	FLOAT Result = 0.f;

	INT MouseX, MouseY;
	if ( GetCursorPosition(MouseX, MouseY, GetScene()) )
	{
		Result = MouseX;

		FVector2D ViewportSize;
		if ( GetViewportSize(ViewportSize) )
		{
			const FLOAT Width = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelViewport);
			if ( Result + Width > ViewportSize.X )
			{
				Result -= Width + 10.f;
			}
		}
	}

	return Result;
}

/**
 * Determines the best Y position for the tooltip, based on the tool-tip's height and the current mouse position
 */
FLOAT UUIToolTip::GetIdealTop() const
{
	FLOAT Result = 0.f;

	INT MouseX, MouseY;
	if ( GetCursorPosition(MouseX, MouseY, GetScene()) )
	{
		Result = MouseY;

		FVector2D ViewportRange;
		if ( GetViewportSize(ViewportRange) )
		{
			if ( Result > ViewportRange.Y / 8 )
			{
				Result = CalculateBestPositionAboveCursor();
			}
			else
			{
				Result = CalculateBestPositionBelowCursor();
			}
		}

		if ( GetViewportOrigin(ViewportRange) && Result < ViewportRange.Y )
		{
			Result = CalculateBestPositionBelowCursor();
		}
	}

	return Result;
}

/**
 * Calculates the best possible Y location for the tool-tip, ensuring this position is above the current mouse position.
 */
FLOAT UUIToolTip::CalculateBestPositionAboveCursor() const
{
	FLOAT Result = 0.f;

	INT MouseX, MouseY;
	if ( GetCursorPosition(MouseX, MouseY, GetScene()) )
	{
		FVector2D ViewportOrigin;
		if ( GetViewportOrigin(ViewportOrigin) )
		{
			Result = MouseY - GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport) - 10.f;
			if ( Result < ViewportOrigin.Y )
			{
				Result = Max(Result, CalculateBestPositionBelowCursor());
			}
		}
	}

	return Result;
}

/**
 * Calculates the best possible Y location for the tool-tip, ensuring this position is below the current mouse position.
 */
FLOAT UUIToolTip::CalculateBestPositionBelowCursor() const
{
	FLOAT Result = 0.f;
	INT MouseX, MouseY;
	if ( GetCursorPosition(MouseX, MouseY, GetScene()) )
	{
		FVector2D ViewportSize, ViewportOrigin;
		if ( GetViewportOrigin(ViewportOrigin) && GetViewportSize(ViewportSize) )
		{
			FLOAT CursorXL, CursorYL;
			if ( GetCursorSize(CursorXL, CursorYL) )
			{
				Result = Max<FLOAT>(MouseY, ViewportOrigin.Y + CursorYL) + 10.f;
				if ( Result + GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport) > ViewportSize.Y )
				{
					Result = Max(Result, CalculateBestPositionAboveCursor());
				}
			}
		}
	}

	return Result;
}

/**
 * Repositions this tool-tip to appear below the mouse cursor (or above if too near the bottom of the screen), without
 * triggering a full scene update.
 */
void UUIToolTip::UpdateToolTipPosition()
{
	bPendingPositionUpdate = FALSE;

	// determine the current mouse position and move us there
	FLOAT NewLeft = GetIdealLeft();
	FLOAT NewTop = GetIdealTop();

	Position.SetRawPositionValue(UIFACE_Left, NewLeft, EVALPOS_PixelViewport);
	Position.InvalidatePosition(UIFACE_Left);

	Position.SetRawPositionValue(UIFACE_Top, NewTop, EVALPOS_PixelViewport);
	Position.InvalidatePosition(UIFACE_Top);

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->bReapplyFormatting = TRUE;
	}

	bResolveToolTipPosition = TRUE;
}

/* === UUIScreenObject interface === */
/**
 * Overridden to prevent any of this widget's inherited methods or components from triggering a scene update, as tooltip
 * positions are updated differently.
 *
 * @param	bDockingStackChanged	if TRUE, the scene will rebuild its DockingStack at the beginning
 *									the next frame
 * @param	bPositionsChanged		if TRUE, the scene will update the positions for all its widgets
 *									at the beginning of the next frame
 * @param	bNavLinksOutdated		if TRUE, the scene will update the navigation links for all widgets
 *									at the beginning of the next frame
 * @param	bWidgetStylesChanged	if TRUE, the scene will refresh the widgets reapplying their current styles
 */
void UUIToolTip::RequestSceneUpdate( UBOOL bDockingStackChanged, UBOOL bPositionsChanged, UBOOL bNavLinksOutdated/*=FALSE*/, UBOOL bWidgetStylesChanged/*=FALSE*/ )
{
	if ( bPositionsChanged )
	{
		UpdateToolTipPosition();
	}
}


/*=========================================================================================
	UUIOptionListBase
========================================================================================= */
/**
 * Creates the support controls which make up the list button - the left button and the right button.
 */
void UUIOptionListBase::CreateInternalControls()
{
	if ( !IsTemplate(RF_ClassDefaultObject) )
	{
		// Create the decrement button
		if ( DecrementButton == NULL )
		{
			// First, give script an opportunity to create a customized component
			if ( DELEGATE_IS_SET(CreateCustomDecrementButton) )
			{
				DecrementButton = delegateCreateCustomDecrementButton(this);
			}
			else
			{
				// no custom component - use the configured class to create it
				UClass* ButtonClass = OptionListButtonClass;
				if ( ButtonClass == NULL )
				{
					warnf(TEXT("NULL OptionListButtonClass detected for '%s'.  Defaulting to Engine.UIOptionListButton"));
					ButtonClass = UUIOptionListButton::StaticClass();
				}

				// script compiler won't allow the wrong type to be assigned, but check to make sure it wasn't set natively to the wrong class.
				checkfSlow(ButtonClass && ButtonClass->IsChildOf(UUIOptionListButton::StaticClass()),
					TEXT("Invalid value assigned to OptionListButtonClass for '%s': %s"), 
					*GetFullName(), *ButtonClass->GetPathName());

				DecrementButton = CastChecked<UUIOptionListButton>(CreateWidget(this, ButtonClass, GetArchetype<UUIOptionListBase>()->DecrementButton, TEXT("DecrementButton")));

				// Take care of any post creation stuff people might want to override
				if ( DecrementButton != NULL )
				{
					// make sure the buttons are docked on all sides except one
					DecrementButton->SetDockTarget(UIFACE_Top, this, UIFACE_Top);
					DecrementButton->SetDockTarget(UIFACE_Bottom, this, UIFACE_Bottom);
					DecrementButton->SetDockTarget(UIFACE_Left, this, UIFACE_Left);

					// lock the arrow width so that it's width remains the same even if the listbutton is resized
					DecrementButton->Position.SetRawPositionValue(UIFACE_Right, 32.f, EVALPOS_PixelOwner);

				}
			}

			// Take care of any post creation stuff people would not want to change
			if ( DecrementButton != NULL )
			{
				// the button background's StyleResolverTag must match the name of the UIStyleReference property from this class
				// which contains the style data that will be used for the increment button or SetWidgetStyle when called on the button
				DecrementButton->BackgroundImageComponent->StyleResolverTag = TEXT("DecrementStyle");

				// Set tab index
				DecrementButton->TabIndex = 0;

				// Because we manage the styles for our buttons, and the buttons' styles are actually stored in their StyleSubscribers, we need to
				// initialize the StyleSubscribers arrays for the buttons prior to calling Super::Initialize() so that the first time OnResolveStyles
				// is called, the buttons will be ready to receive those styles.
				DecrementButton->InitializeStyleSubscribers();

				// now insert the widget into our children array and apply any constraints
				verify(InsertChild(DecrementButton, 0, FALSE) != INDEX_NONE);
			}
		}

		// Create the right button
		if ( IncrementButton == NULL )
		{
			// first, give script an opportunity to create a customized component
			if ( DELEGATE_IS_SET(CreateCustomIncrementButton) )
			{
				IncrementButton = delegateCreateCustomIncrementButton(this);
			}
			else
			{
				// no custom component - use the configured class to create it
				UClass* ButtonClass = OptionListButtonClass;
				if ( ButtonClass == NULL )
				{
					warnf(TEXT("NULL OptionListButtonClass detected for '%s'.  Defaulting to Engine.UIOptionListButton"));
					ButtonClass = UUIOptionListButton::StaticClass();
				}

				// script compiler won't allow the wrong type to be assigned, but check to make sure it wasn't set natively to the wrong class.
				checkfSlow(ButtonClass && ButtonClass->IsChildOf(UUIOptionListButton::StaticClass()),
					TEXT("Invalid value assigned to OptionListButtonClass for '%s': %s"), 
					*GetFullName(), *ButtonClass->GetPathName());

				IncrementButton = CastChecked<UUIOptionListButton>(CreateWidget(this, ButtonClass, GetArchetype<UUIOptionListBase>()->IncrementButton, TEXT("IncrementButton")));

				// Take care of any post creation stuff people might want to override
				if ( IncrementButton != NULL )
				{
					// make sure the buttons are docked on all sides except one
					IncrementButton->SetDockTarget(UIFACE_Top, this, UIFACE_Top);
					IncrementButton->SetDockTarget(UIFACE_Bottom, this, UIFACE_Bottom);
					IncrementButton->SetDockTarget(UIFACE_Right, this, UIFACE_Right);

					// lock the button width so that it's width remains the same even if the optionlist is resized
					IncrementButton->Position.SetRawPositionValue(UIFACE_Left, 224.f, EVALPOS_PixelOwner);
				}
			}

			// Take care of any post creation stuff people would not want to change
			if ( IncrementButton != NULL )
			{
				// the button background's StyleResolverTag must match the name of the UIStyleReference property from this class
				// which contains the style data that will be used for the increment button or SetWidgetStyle when called on the button
				IncrementButton->BackgroundImageComponent->StyleResolverTag = TEXT("IncrementStyle");

				// Set tab index
				IncrementButton->TabIndex = 1;

				// Update states for the button.
// 				IncrementButton->UpdateButtonState();

				// Because we manage the styles for our buttons, and the buttons' styles are actually stored in their StyleSubscribers, we need to
				// initialize the StyleSubscribers arrays for the buttons prior to calling Super::Initialize() so that the first time OnResolveStyles
				// is called, the buttons will be ready to receive those styles.
				IncrementButton->InitializeStyleSubscribers();

				// now insert the widget into our children array and apply any constraints
				verify(InsertChild(IncrementButton, 0, FALSE) != INDEX_NONE);
			}
		}
	}
}

/**
 * @return	TRUE if the user is allowed to decrement the value of this widget
 */
UBOOL UUIOptionListBase::HasPrevValue() const
{
	return FALSE;
}

/**
 * @return	TRUE if the user is allowed to increment the value of this widget
 */
UBOOL UUIOptionListBase::HasNextValue() const
{
	return FALSE;
}

/** Moves the current selection to the left. */
void UUIOptionListBase::OnMoveSelectionLeft( INT PlayerIndex )
{
}

/** Moves the current selection to the right. */
void UUIOptionListBase::OnMoveSelectionRight( INT PlayerIndex )
{
}

/* === UUIObject interface === */
/**
 * Called whenever the value of the slider is modified.  Activates the SliderValueChanged kismet event and calls the OnValueChanged
 * delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to this method; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
 */
void UUIOptionListBase::NotifyValueChanged( INT PlayerIndex, INT NotifyFlags )
{
	if ( PlayerIndex == INDEX_NONE )
	{
		PlayerIndex = GetBestPlayerIndex();
	}

	Super::NotifyValueChanged( PlayerIndex, NotifyFlags );

	if ( bInitialized )
	{
		UpdateStringComponent();

		// Update the buttons' states.
		DecrementButton->UpdateButtonState();
		IncrementButton->UpdateButtonState();

		if(GIsGame)
		{
			// activate the on value changed event
			if ( DELEGATE_IS_SET(OnValueChanged) )
			{
				// notify unrealscript that the user has submitted the selection
				delegateOnValueChanged( this, PlayerIndex );
			}

			ActivateEventOnNotifyValueChanged( PlayerIndex, NotifyFlags );
		}
	}
}

/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the LabelBackground (if non-NULL) to the StyleSubscribers array.
 */
void UUIOptionListBase::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(BackgroundImageComponent);
	VALIDATE_COMPONENT(StringRenderComponent);

	AddStyleSubscriber(BackgroundImageComponent);
	AddStyleSubscriber(StringRenderComponent);
}

/**
 * Adds the specified face to the DockingStack for the specified widget
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUIOptionListBase::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	return StringRenderComponent 
		? StringRenderComponent->AddDockingNode(DockingStack, Face)
		: Super::AddDockingNode(DockingStack, Face);
}

/**
 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUIOptionListBase::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ResolveFacePosition(Face);
	}

	if ( GetNumResolvedFaces() == UIFACE_MAX )
	{
		// adjust the width of the OptionListButtons to ensure that their widths always matches their height.
		// @todo ronp - eventually we may want to make this an option (like bConstrainButtonSize or something)
		const FLOAT WidgetHeight = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];

		if ( DecrementButton )
		{
			DecrementButton->Position.SetPositionValue(DecrementButton, RenderBounds[UIFACE_Left] + WidgetHeight, UIFACE_Right, EVALPOS_PixelViewport, FALSE);
			if ( DecrementButton->DockTargets.bResolved[UIFACE_Right] != 0 )
			{
				// It's already resolved its right face, so we'll need to update everything manually
				DecrementButton->ResolveFacePosition(UIFACE_Right);
			}
		}

		if ( IncrementButton )
		{
			IncrementButton->Position.SetPositionValue(IncrementButton, RenderBounds[UIFACE_Right] - WidgetHeight, UIFACE_Left, EVALPOS_PixelViewport, FALSE);
			if ( IncrementButton->DockTargets.bResolved[UIFACE_Left] != 0 )
			{
				// It's already resolved its left face, so we'll need to update everything manually
				IncrementButton->ResolveFacePosition(UIFACE_Left);
			}
		}
	}
}

/**
 * Called when a property is modified that could potentially affect the widget's position onscreen.
 */
void UUIOptionListBase::RefreshPosition()
{
	Super::RefreshPosition();

	for ( INT ChildIdx=0 ; ChildIdx < Children.Num(); ChildIdx++ )
	{
		Children(ChildIdx)->RefreshPosition();
	}

	RefreshFormatting();
}

/**
 * Called to globally update the formatting of all UIStrings.
 */
void UUIOptionListBase::RefreshFormatting()
{
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ReapplyFormatting();
	}
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUIOptionListBase::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddItem(UIKEY_Clicked);
	out_KeyNames.AddItem(UIKEY_MoveSelectionRight);
	out_KeyNames.AddItem(UIKEY_MoveSelectionLeft);
}

/**
 * Render this button.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUIOptionListBase::Render_Widget( FCanvas* Canvas )
{
	const FLOAT ButtonSpacingPixels = ButtonSpacing.GetValue(this);
	const FLOAT LeftButtonSize = DecrementButton->GetPosition(UIFACE_Right, EVALPOS_PixelViewport)-DecrementButton->GetPosition(UIFACE_Left, EVALPOS_PixelViewport);
	const FLOAT RightButtonSize = IncrementButton->GetPosition(UIFACE_Right, EVALPOS_PixelViewport)-IncrementButton->GetPosition(UIFACE_Left, EVALPOS_PixelViewport);

	FRenderParameters Parameters(
		RenderBounds[UIFACE_Left] + LeftButtonSize,
		RenderBounds[UIFACE_Top],
		IncrementButton->GetPosition(UIFACE_Left, EVALPOS_PixelViewport) - DecrementButton->GetPosition(UIFACE_Right, EVALPOS_PixelViewport),
		RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
	);

	// Render background
	if ( BackgroundImageComponent != NULL )
	{
		BackgroundImageComponent->RenderComponent(Canvas, Parameters);
	}

	// Render String Text
	StringRenderComponent->Render_String(Canvas);
}

/**
 * Called when a style reference is resolved successfully.
 *
 * @param	ResolvedStyle			the style resolved by the style reference
 * @param	StyleProperty			the name of the style reference property that was resolved.
 * @param	ArrayIndex				the array index of the style reference that was resolved.  should only be >0 for style reference arrays.
 * @param	bInvalidateStyleData	if TRUE, the resolved style is different than the style that was previously resolved by this style reference.
 */
void UUIOptionListBase::OnStyleResolved( UUIStyle* ResolvedStyle, const FStyleReferenceId& StylePropertyId, INT ArrayIndex, UBOOL bInvalidateStyleData )
{
	Super::OnStyleResolved( ResolvedStyle, StylePropertyId, ArrayIndex, bInvalidateStyleData );

	FString StylePropertyName = StylePropertyId.GetStyleReferenceName();
	if( StylePropertyName == TEXT("DecrementStyle") )
	{
		if ( DecrementButton != NULL )
		{
			// propagate the ArrowLeftStyle to the left arrow button
			DecrementButton->SetWidgetStyle( ResolvedStyle, StylePropertyId, ArrayIndex );
		}
	}
	else if( StylePropertyName == TEXT("IncrementStyle") )
	{
		if ( IncrementButton != NULL )
		{
			// propagate the DecrementStyle to the decrement button
			IncrementButton->SetWidgetStyle( ResolvedStyle, StylePropertyId, ArrayIndex );
		}
	}
}

/* === UUIScreenObject interface === */
/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUIOptionListBase::InvalidatePositionDependencies( BYTE Face )
{
	Super::InvalidatePositionDependencies(Face);
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->InvalidatePositionDependencies(Face);
	}
}

/**
 * Handles input events for this list.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIOptionListBase::ProcessInputKey( const struct FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;

	// Move Left Event
	// Pressed Event
	if ( EventParms.InputAliasName == UIKEY_Clicked )
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			const UBOOL bIsDoubleClickPress = EventParms.EventType==IE_DoubleClick;

			// notify unrealscript
			if ( DELEGATE_IS_SET(OnPressed) )
			{
				delegateOnPressed(this, EventParms.PlayerIndex);
			}

			if ( bIsDoubleClickPress && DELEGATE_IS_SET(OnDoubleClick) )
			{
				delegateOnDoubleClick(this, EventParms.PlayerIndex);
			}

			// activate the pressed state
			ActivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			if ( bIsDoubleClickPress )
			{
				ActivateEventByClass(EventParms.PlayerIndex, UUIEvent_OnDoubleClick::StaticClass(), this);
			}
			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Repeat )
		{
			if ( DELEGATE_IS_SET(OnPressRepeat) )
			{
				delegateOnPressRepeat(this, EventParms.PlayerIndex);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released )
		{
			// Fire OnPressed Delegate
			if ( DELEGATE_IS_SET(OnPressRelease) )
			{
				delegateOnPressRelease(this, EventParms.PlayerIndex);
			}

			if ( IsPressed(EventParms.PlayerIndex) )
			{
				FVector2D MousePos(0,0);				
				UBOOL bInputConsumed = FALSE;
				if ( !IsCursorInputKey(EventParms.InputKeyName) || !GetCursorPosition(MousePos, GetScene()) || ContainsPoint(MousePos) )
				{
					if ( DELEGATE_IS_SET(OnClicked) )
					{
						bInputConsumed = delegateOnClicked(this, EventParms.PlayerIndex);
					}

					// activate the on click event
					if( !bInputConsumed )
					{
						ActivateEventByClass(EventParms.PlayerIndex,UUIEvent_OnClick::StaticClass(), this);
					}
				}

				// deactivate the pressed state
				DeactivateStateByClass(UUIState_Pressed::StaticClass(),EventParms.PlayerIndex);
			}
			bResult = TRUE;
		}
	}
	else if ( EventParms.InputAliasName == UIKEY_MoveSelectionLeft )
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			// If there exists a value to decrement to, send the button to the pressed state
			if( HasPrevValue() )
			{
				DecrementButton->UpdateButtonState();
				DecrementButton->ActivateStateByClass( UUIState_Pressed::StaticClass(), EventParms.PlayerIndex );
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released || EventParms.EventType == IE_Repeat )
		{
			// If there exists a value to decrement to, make the decrement happen...
			if( HasPrevValue() )
			{
				OnMoveSelectionLeft( EventParms.PlayerIndex );
			}

			// ...and release the pressed state
			if ( EventParms.EventType == IE_Released )
			{
				DecrementButton->DeactivateStateByClass( UUIState_Pressed::StaticClass(), EventParms.PlayerIndex );
				DecrementButton->UpdateButtonState();
			}

			// Update the increment button in case it needs reactivated
			IncrementButton->UpdateButtonState();
			bResult = TRUE;
		}
	}

	// Move Right Event
	else if ( EventParms.InputAliasName == UIKEY_MoveSelectionRight )
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			// If there exists a value to increment to, send the button to the pressed state
			if( HasNextValue() )
			{
				IncrementButton->UpdateButtonState();
				IncrementButton->ActivateStateByClass( UUIState_Pressed::StaticClass(), EventParms.PlayerIndex );
			}
			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released || EventParms.EventType == IE_Repeat )
		{
			// If there exists a value to increment to, make the increment happen...
			if( HasNextValue() )
			{
				OnMoveSelectionRight( EventParms.PlayerIndex );
			}

			// ...and release the pressed state
			if ( EventParms.EventType == IE_Released )
			{
				IncrementButton->DeactivateStateByClass( UUIState_Pressed::StaticClass(), EventParms.PlayerIndex );
				IncrementButton->UpdateButtonState();
			}

			// Update the decrement button in case it needs reactivated
			DecrementButton->UpdateButtonState();
			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);

	return bResult;
}

/**
 * Activates the UIState_Focused menu state and updates the pertinent members of FocusControls.
 *
 * @param	FocusedChild	the child of this widget that should become the "focused" control for this widget.
 *							A value of NULL indicates that there is no focused child.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 */
UBOOL UUIOptionListBase::GainFocus( UUIObject* FocusedChild, INT PlayerIndex )
{
	// Make sure to call super first since it updates our focused state properly.
	const UBOOL bResult = Super::GainFocus(FocusedChild, PlayerIndex);

	// Update states for arrows.
	if ( bResult )
	{
		DecrementButton->UpdateButtonState();
		IncrementButton->UpdateButtonState();
	}

	return bResult;
}

/**
 * Deactivates the UIState_Focused menu state and updates the pertinent members of FocusControls.
 *
 * @param	FocusedChild	the child of this widget that is currently "focused" control for this widget.
 *							A value of NULL indicates that there is no focused child.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 */
UBOOL UUIOptionListBase::LoseFocus( UUIObject* FocusedChild, INT PlayerIndex )
{
	// Make sure to call super first since it updates our focused state properly.
	const UBOOL bResult = Super::LoseFocus(FocusedChild, PlayerIndex);

	// Update states for arrows.
	if ( bResult )
	{
		DecrementButton->UpdateButtonState();
		IncrementButton->UpdateButtonState();
	}
	return bResult;
}

#define BUTTONS_CREATED_IN_DEFPROPS 1
/**
 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
 * once the scene has been completely initialized.
 * For widgets added at runtime, called after the widget has been inserted into its parent's
 * list of children.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUIOptionListBase::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	// Initialize String Component
	if ( StringRenderComponent != NULL )
	{
		TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
		StringRenderComponent->InitializeComponent(&Subscriber);
	}

#if !BUTTONS_CREATED_IN_DEFPROPS
	if ( !IsInitialized() )
	{
		//@temp:
		// make sure none of our children were marked RF_RootSet, which can happen if the package containing this scrollbar was
		// loaded during editor startup
		TArray<UObject*> AllSubobjects;
		TArray<UObject*> Buttons;
		for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
		{
			AllSubobjects.AddItem(Children(ChildIndex));
			Buttons.AddItem(Children(ChildIndex));
			Children(ChildIndex)->ClearFlags(RF_RootSet|RF_Public);

			TArray<UObject*> ChildSubobjects;
			FArchiveObjectReferenceCollector Collector(
				&ChildSubobjects,
				Children(ChildIndex),
				FALSE,
				FALSE,
				TRUE,
				FALSE
				);

			Children(ChildIndex)->Serialize(Collector);
			for ( INT ObjIndex = 0; ObjIndex < ChildSubobjects.Num(); ObjIndex++ )
			{
				ChildSubobjects(ObjIndex)->ClearFlags(RF_RootSet|RF_Public);
			}
			AllSubobjects += ChildSubobjects;
		}

		// RemoveChildren immediately makes a copy of the input array, so it's safe to pass the children array directly
		RemoveChildren(Children);

		UUIOptionList* TransientList = ConstructObject<UUIOptionList>(UUIOptionList::StaticClass());
		Buttons(0)->Rename(*Buttons(0)->GetName(), TransientList, REN_ForceNoResetLoaders);
		Buttons(1)->Rename(*Buttons(1)->GetName(), TransientList, REN_ForceNoResetLoaders);
		IncrementButton = DecrementButton = NULL;

		TMap<UObject*,UObject*> ReplaceMap;
		for ( INT Idx = 0; Idx < AllSubobjects.Num(); Idx++ )
		{
			ReplaceMap.Set(AllSubobjects(Idx), NULL);
		}

		FArchiveReplaceObjectRef<UObject> ReplaceAr(
			this,
			ReplaceMap,
			TRUE,
			TRUE,
			FALSE);

		// If the internal controls of this combobox have never been created, do that now.
		CreateInternalControls();
	}
#else
	if ( DecrementButton != NULL )
	{
		// Because we manage the styles for our buttons, and the buttons' styles are actually stored in their StyleSubscribers, we need to
		// initialize the StyleSubscribers arrays for the buttons prior to calling Super::Initialize() so that the first time OnResolveStyles
		// is called, the buttons will be ready to receive those styles.
		DecrementButton->InitializeStyleSubscribers();
	}
	if ( IncrementButton != NULL )
	{
		// Because we manage the styles for our buttons, and the buttons' styles are actually stored in their StyleSubscribers, we need to
		// initialize the StyleSubscribers arrays for the buttons prior to calling Super::Initialize() so that the first time OnResolveStyles
		// is called, the buttons will be ready to receive those styles.
		IncrementButton->InitializeStyleSubscribers();
	}
#endif
	// Must initialize after the component
	Super::Initialize(inOwnerScene, inOwner);
}

/* === UObject interface === */
/**
 * Called just before just object is saved to disk.  Clears all references to the internal buttons.
 */
void UUIOptionListBase::PreSave()
{
	Super::PreSave();

#if !BUTTONS_CREATED_IN_DEFPROPS
	// make sure none of our children were marked RF_RootSet, which can happen if the package containing this scrollbar was
	// loaded during editor startup
	TArray<UObject*> AllSubobjects;
	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		AllSubobjects.AddItem(Children(ChildIndex));
		Children(ChildIndex)->ClearFlags(RF_RootSet|RF_Public);

		TArray<UObject*> ChildSubobjects;
		FArchiveObjectReferenceCollector Collector(
			&ChildSubobjects,
			Children(ChildIndex),
			FALSE,
			FALSE,
			TRUE,
			FALSE
			);

		Children(ChildIndex)->Serialize(Collector);
		for ( INT ObjIndex = 0; ObjIndex < ChildSubobjects.Num(); ObjIndex++ )
		{
			ChildSubobjects(ObjIndex)->ClearFlags(RF_RootSet|RF_Public);
		}

		AllSubobjects += ChildSubobjects;
	}

	TMap<UObject*,UObject*> ReplaceMap;
	for ( INT Idx = 0; Idx < AllSubobjects.Num(); Idx++ )
	{
		ReplaceMap.Set(AllSubobjects(Idx), NULL);
	}

	// RemoveChildren immediately makes a copy of the input array, so it's safe to pass the children array directly
	RemoveChildren(Children);
	IncrementButton = DecrementButton = NULL;

	FArchiveReplaceObjectRef<UObject> ReplaceAr(
		this,
		ReplaceMap,
		TRUE,
		TRUE,
		FALSE);
#endif
}

/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIOptionListBase::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("BackgroundImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty && BackgroundImageComponent != NULL )
				{
					// the user either cleared the value of the ImageComponent (which should never happen since
					// we use the 'noclear' keyword on the property declaration), or is assigning a new value to the ImageComponent.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(BackgroundImageComponent);
				}
			}
			else if ( PropertyName == TEXT("StringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the StringRenderComponent itself was changed
				if ( MemberProperty == ModifiedProperty && StringRenderComponent != NULL )
				{
					// the user either cleared the value of the StringRenderComponent or is assigning a new value to it.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(StringRenderComponent);
				}
			}
		}
	}
}

/**
 * Called when a property value from a member struct or array has been changed in the editor.
 */
void UUIOptionListBase::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("DataSource") )
			{
				if ( RefreshSubscriberValue() && StringRenderComponent != NULL )
				{
					if (StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Horizontal)
					||	StringRenderComponent->IsAutoSizeEnabled(UIORIENT_Vertical)
					||	StringRenderComponent->GetWrapMode() != CLIP_None )
					{
						RefreshPosition();
					}
				}
			}
			else if ( PropertyName == TEXT("ButtonSpacing") )
			{
				// for now, this is evaluated each frame in Render_Widget, so don't have to do anything
			}
			else if ( PropertyName == TEXT("BackgroundImageComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the ImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( BackgroundImageComponent != NULL )
					{
						UUIComp_DrawImage* BackgroundImageTemplate = GetArchetype<UUIOptionListBase>()->BackgroundImageComponent;
						if ( BackgroundImageTemplate != NULL )
						{
							BackgroundImageComponent->StyleResolverTag = BackgroundImageTemplate->StyleResolverTag;
						}
						else
						{
							BackgroundImageComponent->StyleResolverTag = TEXT("Background Image Style");
						}

						// user added created a new label component background - add it to the list of style subscribers
						AddStyleSubscriber(BackgroundImageComponent);

						// now initialize the component's image
						BackgroundImageComponent->SetImage(BackgroundImageComponent->GetImage());
					}
				}
			}
			else if ( PropertyName == TEXT("StringRenderComponent") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the ImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( StringRenderComponent != NULL )
					{
						UUIComp_DrawString* StringComponentTemplate = GetArchetype<ThisClass>()->StringRenderComponent;
						if ( StringComponentTemplate != NULL )
						{
							StringRenderComponent->StyleResolverTag = StringComponentTemplate->StyleResolverTag;
						}
						else
						{
							StringRenderComponent->StyleResolverTag = TEXT("Caption Style");
						}

						// user added created a new string render component - add it to the list of style subscribers
						AddStyleSubscriber(StringRenderComponent);

						// now initialize the new string component
						TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
						StringRenderComponent->InitializeComponent(&Subscriber);

						// then initialize its style
						StringRenderComponent->NotifyResolveStyle(GetActiveSkin(), FALSE, GetCurrentState());

						// finally initialize its text
						UpdateStringComponent();
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/** === UIDataSourceSubscriber interface === */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
void UUIOptionListBase::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding( MarkupText,BindingIndex );
	}
	else if ( DataSource.MarkupString != MarkupText )
	{
		Modify();
		DataSource.MarkupString = MarkupText;

		// Refresh 
		RefreshSubscriberValue(BindingIndex);
	}
}
/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUIOptionListBase::GetDataStoreBinding( INT BindingIndex/*=-1*/ ) const
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}
	return DataSource.MarkupString;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUIOptionListBase::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	return bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUIOptionListBase::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);

	// add overall data store to the list
	if ( DataSource )
	{
		out_BoundDataStores.AddUniqueItem(*DataSource);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUIOptionListBase::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUIOptionListBase::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	// add the data stores bound to this option list
	GetBoundDataStores(out_BoundDataStores);
	return FALSE;
}

/*=========================================================================================
	UUIOptionList
========================================================================================= */
/* === UUIOptionList interface === */
/** @return Returns the number of possible values for the field we are bound to. */
INT UUIOptionList::GetNumValues() const
{
	INT NumValues = 0;

	if ( DataProvider )
	{
		NumValues = DataProvider->GetElementCount(DataSource.DataStoreField);
	}

	return NumValues;
}

/** 
 * @param Index		List index to get the value of.
 *
 * @return Returns the number of possible values for the field we are bound to. 
 */
UBOOL UUIOptionList::GetListValue(INT ListIndex, FString& OutValue)
{
	UBOOL bResult = FALSE;

	if ( DataProvider )
	{
		FUIProviderFieldValue FieldValue(EC_EventParm);

		TScriptInterface<IUIListElementCellProvider> ValueProvider = DataProvider->GetElementCellValueProvider(DataSource.DataStoreField, ListIndex);
		if ( ValueProvider && ValueProvider->GetCellFieldValue(DataSource.DataStoreField, ListIndex, FieldValue) )
		{
			OutValue = FieldValue.StringValue;
			bResult = TRUE;
		}
	}

	return bResult;
}	

/**
 * Decrements the widget to the previous value
 */
void UUIOptionList::SetPrevValue()
{
	if( DataProvider )
	{
		const INT NumValues = GetNumValues();
		if( CurrentIndex > 0 )
		{
			SetCurrentIndex( CurrentIndex-1 );
		}
		else if( (NumValues > 1) && bWrapOptions )
		{
			SetCurrentIndex( NumValues - 1 );
		}
	}
}

/**
 * Increments the widget to the next value
 */
void UUIOptionList::SetNextValue()
{
	if( DataProvider )
	{
		const INT NumValues = GetNumValues();
		if ( (NumValues - 1 > CurrentIndex) && DataProvider )
		{
			SetCurrentIndex( CurrentIndex+1 );
		}
		else if( (NumValues > 1) && bWrapOptions )
		{
			SetCurrentIndex( 0 );
		}
	}
}

/**
 * @return Returns the current index for the option button.
 */
INT UUIOptionList::GetCurrentIndex() const
{
	return CurrentIndex;
}

/**
 * Sets a new index for the option button.
 *
 * @param NewIndex		New index for the option button.
 */
void UUIOptionList::SetCurrentIndex( INT NewIndex )
{
	if( (NewIndex != CurrentIndex) && (GetNumValues() > NewIndex) && (NewIndex >= 0) )
	{
		CurrentIndex = NewIndex;

		// Activate index changed events.
		TArray<INT> PlayerIndices;
		GetInputMaskPlayerIndexes( PlayerIndices );
		for ( INT PlayerIdx = 0; PlayerIdx < PlayerIndices.Num(); PlayerIdx++ )
		{
			NotifyValueChanged( PlayerIndices(PlayerIdx) );		
		}
	}
}

/* === UUIOptionListBase interface === */
/** Updates the string value using the current index. */
void UUIOptionList::UpdateStringComponent()
{
	FString OutValue;

	if ( GetListValue(CurrentIndex, OutValue) )
	{
		if ( OutValue != StringRenderComponent->GetValue(TRUE) )
		{
			StringRenderComponent->SetValue(OutValue);
		}
	}
}

/**
 * @return	TRUE if the user is allowed to decrement the value of this widget
 */
UBOOL UUIOptionList::HasPrevValue() const
{
	if( bWrapOptions )
	{
		return (GetNumValues() > 1); 
	}
	else
	{
		return (CurrentIndex > 0);
	}
}

/**
 * @return	TRUE if the user is allowed to increment the value of this widget
 */
UBOOL UUIOptionList::HasNextValue() const
{
	if( bWrapOptions )
	{
		return (GetNumValues() > 1);
	}
	else
	{
		return (CurrentIndex < (GetNumValues() - 1));
	}
}

/** Moves the current selection to the left. */
void UUIOptionList::OnMoveSelectionLeft( int PlayerIndex )
{
	if( HasPrevValue() )
	{
		// Move to the prev value
		SetPrevValue();

		// Play the decrement sound
		PlayUISound( DecrementCue, PlayerIndex );

		NotifyValueChanged(PlayerIndex);
	}
}

/** Moves the current selection to the right. */
void UUIOptionList::OnMoveSelectionRight( int PlayerIndex )
{
	if( HasNextValue() )
	{
		// Move to the next value
		SetNextValue();

		// Play the increment sound
		PlayUISound( IncrementCue, PlayerIndex );

		NotifyValueChanged(PlayerIndex);
	}
}

/**
* Sends the appropriate event when the NotifyValueChanged function is called.
*/
void UUIOptionList::ActivateEventOnNotifyValueChanged( INT PlayerIndex, INT NotifyFlags )
{
	TArray<UUIEvent_OptionListValueChanged*> ActivatedEvents;
	ActivateEventByClass(PlayerIndex, UUIEvent_OptionListValueChanged::StaticClass(), this, FALSE, NULL, (TArray<UUIEvent*>*)&ActivatedEvents);
	for ( INT EventIdx = 0; EventIdx < ActivatedEvents.Num(); EventIdx++ )
	{
		UUIEvent_OptionListValueChanged* Event = ActivatedEvents(EventIdx);

		// copy the current value of the option string into the "Text Value" variable link
		TArray<FString*> StringVars;
		Event->GetStringVars(StringVars,TEXT("Text Value"));
		for (INT Idx = 0; Idx < StringVars.Num(); Idx++)
		{
			*(StringVars(Idx)) = StringRenderComponent->GetValue(TRUE);
		}

		// copy the value of the list's selected items into the "Current Item" variable link
		// @todo ronp - implement Current Item correctly
		TArray<INT*> IntVars;
		Event->GetIntVars(IntVars, TEXT("Current Item"));
		for ( INT Idx = 0; Idx < IntVars.Num(); Idx++ )
		{
			*(IntVars(Idx)) = CurrentIndex;
		}

		// copy the list's current index into the "List Index" variable link
		IntVars.Empty();
		Event->GetIntVars(IntVars, TEXT("Current List Index"));
		for (INT Idx = 0; Idx < IntVars.Num(); Idx++)
		{
			*(IntVars(Idx)) = CurrentIndex;
		}
	}
}

/**
 * Resolves DataSource into the list element provider that it references.
 */
void UUIOptionList::ResolveListElementProvider()
{
	TScriptInterface<IUIListElementProvider> Result;
	if ( DataSource.ResolveMarkup( this ) )
	{
		DataProvider = DataSource->ResolveListElementProvider(DataSource.DataStoreField.ToString());
	}
}

/* === UUIDataStoreSubscriber interface === */
/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUIOptionList::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = Super::RefreshSubscriberValue(BindingIndex);
	if ( !bResult && IsInitialized() )
	{
		CurrentIndex = INDEX_NONE;
		ResolveListElementProvider();

		// Get the default value for the edit box.
		FUIProviderFieldValue FieldValue(EC_EventParm);
		if ( DataProvider && DataSource.GetBindingValue(FieldValue) )
// 		if( GetDataStoreFieldValue(GetDataStoreBinding(), FieldValue, GetScene(), GetPlayerOwner()) && FieldValue.HasValue())
		{
			// Loop through all list values and try to match the current string value to a index.
			FString StringValue;
			const INT NumValues = GetNumValues();
			for( INT OptIdx=0; OptIdx < NumValues; OptIdx++ )
			{
				if ( GetListValue(OptIdx, StringValue) && FieldValue.StringValue == StringValue )
				{
					CurrentIndex = OptIdx;
					if ( StringRenderComponent != NULL && StringValue != StringRenderComponent->GetValue(TRUE) )
					{
						StringRenderComponent->SetValue(StringValue);
					}

					bResult = TRUE;
					break;
				}
			}
		}
	}

	return bResult;
}

/* === UUIDataStorePublisher interface === */
/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUIOptionList::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = Super::SaveSubscriberValue(out_BoundDataStores, BindingIndex);
	if ( !bResult && DataProvider )
	{
		FUIProviderScriptFieldValue FieldValue(EC_EventParm);
		if ( GetListValue(CurrentIndex,FieldValue.StringValue) )
		{
			FieldValue.PropertyType = DataSource.RequiredFieldType;
			FieldValue.ArrayValue.AddItem(CurrentIndex);
			bResult = DataSource.SetBindingValue(FieldValue);
		}
	}

	return bResult;
}



/*=========================================================================================
	UUINumericOptionList
========================================================================================= */
/* === UUIOptionListBase interface === */
/**
 * Updates the string component with the current value of the button.
 */
void UUINumericOptionList::UpdateStringComponent()
{
	if ( StringRenderComponent != NULL )
	{
		// set the value of the string using the slider's current value.
		FString ValueString;

		if( RangeValue.bIntRange )
		{
			ValueString = FString::Printf(TEXT("%i"), appTrunc(RangeValue.GetCurrentValue()));
		}
		else
		{
			ValueString = FString::Printf(TEXT("%.2f"), RangeValue.GetCurrentValue());
		}

		if ( ValueString != StringRenderComponent->GetValue(TRUE) )
		{
			StringRenderComponent->SetValue(ValueString);
		}
	}
}

/**
 * @return	TRUE if the user is allowed to decrement the value of this widget
 */
UBOOL UUINumericOptionList::HasPrevValue() const
{
	UBOOL bResult = FALSE;
	
	// determine whether there is more than one possible value
	if ( (RangeValue.MaxValue - RangeValue.MinValue) / RangeValue.GetNudgeValue() > 1 ) 
	{
		if ( bWrapOptions )
		{
			bResult = TRUE;
		}
		else
		{
			// if wrapping isn't enabled, check whether the current value is the lowest possible value
			const FLOAT CurrentValue = GetValue();
			bResult = CurrentValue - RangeValue.GetNudgeValue() >= RangeValue.MinValue;
		}
	}

	return bResult;
}

/**
 * @return	TRUE if the user is allowed to increment the value of this widget
 */
UBOOL UUINumericOptionList::HasNextValue() const
{
	UBOOL bResult = FALSE;

	// determine whether there is more than one possible value
	if ( (RangeValue.MaxValue - RangeValue.MinValue) / RangeValue.GetNudgeValue() > 1 ) 
	{
		if ( bWrapOptions )
		{
			bResult = TRUE;
		}
		else
		{
			// if wrapping isn't enabled, check whether the current value is the highest possible value
			const FLOAT CurrentValue = GetValue();
			bResult = CurrentValue + RangeValue.GetNudgeValue() <= RangeValue.MaxValue;
		}
	}

	return bResult;
}

/** Moves the current selection to the left. */
void UUINumericOptionList::OnMoveSelectionLeft(INT PlayerIndex)
{
	if( HasPrevValue() )
	{
		FLOAT NewValue = GetValue() - RangeValue.GetNudgeValue();

		// Wrap values.
		if(NewValue < RangeValue.MinValue)
		{
			NewValue = RangeValue.MaxValue;
		}

		if ( SetValue( NewValue ) )
		{
			// Play the decrement sound
			PlayUISound(DecrementCue, PlayerIndex);

			NotifyValueChanged(PlayerIndex);
		}
	}
}

/** Moves the current selection to the right. */
void UUINumericOptionList::OnMoveSelectionRight(INT PlayerIndex)
{
	if ( HasNextValue() )
	{
		FLOAT NewValue = GetValue() + RangeValue.GetNudgeValue();

		// Wrap values.
		if(NewValue > RangeValue.MaxValue)
		{
			NewValue = RangeValue.MinValue;
		}

		if ( SetValue( NewValue ) )
		{
			// Play the increment sound
			PlayUISound(IncrementCue, PlayerIndex);

			NotifyValueChanged(PlayerIndex);
		}
	}
}

/**
* Sends the appropriate event when the NotifyValueChanged function is called.
*/
void UUINumericOptionList::ActivateEventOnNotifyValueChanged( INT PlayerIndex, INT NotifyFlags )
{
	TArray<UUIEvent_NumericOptionListValueChanged*> ActivatedEvents;
	ActivateEventByClass(PlayerIndex, UUIEvent_NumericOptionListValueChanged::StaticClass(), this, FALSE, NULL, (TArray<UUIEvent*>*)&ActivatedEvents);
	for ( INT EventIdx = 0; EventIdx < ActivatedEvents.Num(); EventIdx++ )
	{
		UUIEvent_NumericOptionListValueChanged* Event = ActivatedEvents(EventIdx);

		// copy the current value of the option string into the "Text Value" variable link
		TArray<FString*> StringVars;
		Event->GetStringVars(StringVars,TEXT("Text Value"));
		for (INT Idx = 0; Idx < StringVars.Num(); Idx++)
		{
			*(StringVars(Idx)) = StringRenderComponent->GetValue(TRUE);
		}

		// copy the list's current index into the "List Index" variable link
		TArray<FLOAT*> FloatVars;
		Event->GetFloatVars(FloatVars, TEXT("Numeric Value"));
		for (INT Idx = 0; Idx < FloatVars.Num(); Idx++)
		{
			if( RangeValue.bIntRange )
			{
				*(FloatVars(Idx)) = appTrunc(RangeValue.GetCurrentValue());
			}
			else
			{
				*(FloatVars(Idx)) = RangeValue.GetCurrentValue();
			}
		}
	}
}

/**
 * Change the value of this slider at runtime.
 *
 * @param	NewValue			the new value for the slider.
 * @param	bPercentageValue	TRUE indicates that the new value is formatted as a percentage of the total range of this slider.
 *
 * @return	TRUE if the slider's value was changed
 */
UBOOL UUINumericOptionList::SetValue( FLOAT NewValue, UBOOL bPercentageValue/*=FALSE*/ )
{
	FLOAT PreviousValue = RangeValue.GetCurrentValue();
	if ( bPercentageValue == TRUE )
	{
		NewValue = ((RangeValue.MaxValue - RangeValue.MinValue) * Clamp<FLOAT>(NewValue, 0.f, 1.f)) + RangeValue.MinValue;
	}

	RangeValue.SetCurrentValue(NewValue,TRUE);
	return Abs<FLOAT>(PreviousValue - RangeValue.GetCurrentValue()) > KINDA_SMALL_NUMBER;
}

/**
 * Gets the current value of this slider
 *
 * @param	bPercentageValue	TRUE to format the result as a percentage of the total range of this slider.
 */
FLOAT UUINumericOptionList::GetValue( UBOOL bFormatAsPercent/*=FALSE*/ ) const
{
	FLOAT Result = RangeValue.GetCurrentValue();
	if ( bFormatAsPercent && RangeValue.MaxValue > RangeValue.MinValue )
	{
		// the percentage of the total width of this range from where we currently are
		Result = (Result - RangeValue.MinValue) / (RangeValue.MaxValue - RangeValue.MinValue);
	}

	return Result;
}

/* === UUIDataStoreSubscriber interface === */
/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUINumericOptionList::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = Super::RefreshSubscriberValue(BindingIndex);
	if ( !bResult && DataSource.ResolveMarkup( this ) )
	{
		FUIProviderFieldValue ResolvedValue(EC_EventParm);
		if ( DataSource.GetBindingValue(ResolvedValue) )
		{
			if ( ResolvedValue.PropertyType == DATATYPE_RangeProperty && ResolvedValue.RangeValue.HasValue() )
			{
				RangeValue = ResolvedValue.RangeValue;
				bResult = TRUE;
			}
			else if ( ResolvedValue.StringValue.Len() > 0 )
			{
				FLOAT FloatValue = appAtof(*ResolvedValue.StringValue);
				SetValue(FloatValue);
				bResult = TRUE;
			}

			if ( bResult )
			{
				UpdateStringComponent();

				// Update buttons' states
				if ( DecrementButton != NULL )
				{
					DecrementButton->UpdateButtonState();
				}

				if ( IncrementButton != NULL )
				{
					IncrementButton->UpdateButtonState();
				}
			}
		}
	}

	return bResult;
}

/**
 * Called when a property value from a member struct or array has been changed in the editor.
 */
void UUINumericOptionList::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("RangeValue") )
			{
				UpdateStringComponent();
				if ( DecrementButton != NULL )
				{
					DecrementButton->UpdateButtonState();
				}

				if ( IncrementButton != NULL )
				{
					IncrementButton->UpdateButtonState();
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/* === UUIDataStorePublisher interface === */
/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL UUINumericOptionList::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = Super::SaveSubscriberValue(out_BoundDataStores, BindingIndex);
	if ( !bResult )
	{
		FUIProviderScriptFieldValue Value(EC_EventParm);
		Value.PropertyType = DATATYPE_RangeProperty;
		Value.RangeValue = RangeValue;
		Value.StringValue = RangeValue.bIntRange
			? FString::Printf(TEXT("%i"), (INT)RangeValue.GetCurrentValue())
			: FString::Printf(TEXT("%f"), RangeValue.GetCurrentValue());

		bResult = DataSource.SetBindingValue(Value);
	}

	return bResult;
}




/*=========================================================================================
	UUIOptionListButton
========================================================================================= */
/**
 * Determines which states this button should be in based on the state of the owner UIOptionListBase and synchronizes to those states.
 *
 * @param	PlayerIndex		the index of the player that generated the update; if not specified, states will be activated for all
 *							players that are eligible to generate input for this button.
 */
void UUIOptionListButton::UpdateButtonState( INT PlayerIndex/*=INDEX_NONE*/ )
{
	UUIOptionListBase* OwnerWidget = GetOuterUUIOptionListBase();
	if ( (OwnerWidget->DecrementButton == this) ? OwnerWidget->HasPrevValue() : OwnerWidget->HasNextValue() )
	{
		TArray<INT> PlayerIndices;
		GetInputMaskPlayerIndexes(PlayerIndices);

		const UBOOL bIsFocused = OwnerWidget->IsFocused();
		for ( INT PlayerIndex = 0; PlayerIndex < PlayerIndices.Num(); PlayerIndex++ )
		{
			SetEnabled(TRUE, PlayerIndices(PlayerIndex));
			if ( bIsFocused )
			{
				ActivateStateByClass(UUIState_Focused::StaticClass(), PlayerIndices(PlayerIndex));
			}
			else
			{
				DeactivateStateByClass(UUIState_Focused::StaticClass(), PlayerIndices(PlayerIndex));
			}

		}
	}
	else
	{
		TArray<INT> PlayerIndices;
		GetInputMaskPlayerIndexes(PlayerIndices);
		for ( INT PlayerIndex = 0; PlayerIndex < PlayerIndices.Num(); PlayerIndex++ )
		{
			SetEnabled( FALSE, PlayerIndices(PlayerIndex) );
		}
	}
}



// EOF




