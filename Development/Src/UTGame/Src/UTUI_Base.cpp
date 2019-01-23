//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//
// This holds all of the base UI Objects
//=============================================================================

#include "UTGame.h"
#include "UTGameUIClasses.h"

IMPLEMENT_CLASS(UUTGameUISceneClient);
IMPLEMENT_CLASS(UUTUIScene);

/*=========================================================================================
  UTGameUIScenenClient - We have our own scene client so that we can provide Tick/Prerender
  passes.  We also use this object as a repository for all UTFontPool objects
  ========================================================================================= */

/**
 * Override Tick so that we can measure how fast it's working
 * @See UGameUISceneClient for more details
 */
void UUTGameUISceneClient::Tick(FLOAT DeltaTime)
{
	// Clock the tick pass
	TickTime=0.0f;


	clock(TickTime);
		Super::Tick(DeltaTime);
	unclock(TickTime);
}

/**
 * Render all the active scenes
 */
void UUTGameUISceneClient::RenderScenes( FCanvas* Canvas )
{
	// get player field of view
	FLOAT FOV = GEngine->GamePlayers(0)->Actor->eventGetFOVAngle();

	// Set 3D Projection
	Canvas->SetBaseTransform(FCanvas::CalcBaseTransform3D(RenderViewport->GetSizeX(),
		RenderViewport->GetSizeY(),FOV,NEAR_CLIPPING_PLANE));
	
	Super::RenderScenes(Canvas);
	
	// Restore 2D Projection
	Canvas->SetBaseTransform(FCanvas::CalcBaseTransform2D(RenderViewport->GetSizeX(),
		RenderViewport->GetSizeY()));
}

/**
 * We override the Render_Scene so that we can provide a PreRender pass
 * @See UGameUISceneClient for more details
 */
void UUTGameUISceneClient::Render_Scene(FCanvas* Canvas, UUIScene *Scene)
{
	PreRenderTime=0.0f;
	RenderTime=0.0f;

	// UTUIScene's support a pre-render pass.  If this is a UTUIScene, start that pass and clock it

	UUTUIScene* UTScene = Cast<UUTUIScene>(Scene);
	if ( UTScene )
	{
		clock(PreRenderTime);
			UTScene->PreRender(Canvas);
		unclock(PreRenderTime);
	}

	// Render the scene

	clock(RenderTime);
		Super::Render_Scene(Canvas,Scene);
	unclock(RenderTime);

	// Debug

	if (bShowRenderTimes)
	{

		FLOAT Mod =  GSecondsPerCycle * 1000.f;

		FLOAT TotalRenderTime = PreRenderTime + RenderTime;
		AvgRenderTime = AvgRenderTime + TotalRenderTime;

		FLOAT TotalTime = TickTime + PreRenderTime + RenderTime;
		AvgTime = AvgTime + TotalTime;

		FrameCount = FrameCount + 1.0;

		FString Test;
		Test = FString::Printf(TEXT("FrameTime : Total[%3.5f]   Tick[%3.5f]   PRend[%3.5f]   Rend[%3.5f]   Animation[%3.5f]"),
					TotalTime * Mod, (TickTime * Mod),(PreRenderTime * Mod),(RenderTime * Mod), (AnimTime * Mod));
		DrawString(Canvas, 0, 200, *Test, GEngine->SmallFont, FLinearColor(1.0f,1.0f,1.0f,1.0f));

		Test = FString::Printf(TEXT("Strings  : %3.5f"), StringRenderTime * Mod);
		DrawString(Canvas, 400, 220, *Test, GEngine->SmallFont, FLinearColor(1.0f,1.0f,1.0f,1.0f));

		Test = FString::Printf(TEXT("Averages  : Frames %f   Total %f    Render %f"), FrameCount,(AvgTime/FrameCount)*Mod, (AvgRenderTime/FrameCount) * Mod);
		DrawString(Canvas, 0, 230, *Test, GEngine->SmallFont, FLinearColor(1.0f,1.0f,1.0f,1.0f));
	}
	else if (FrameCount>0.0f)
	{
		FrameCount = 0.0f;
		AvgTime = 0.0f;
		AvgRenderTime = 0.0f;
	}
}

/** @return TRUE if there are any scenes currently accepting input, FALSE otherwise. */
UBOOL UUTGameUISceneClient::IsUIAcceptingInput()
{
	return IsUIActive(SCENEFILTER_InputProcessorOnly);
}

/*=========================================================================================
  UTUIScene - Our scenes support a PreRender and Tick pass
  ========================================================================================= */

/** 
 * Automatically add any Instanced children and auto-link transient children
 */

void UUTUIScene::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);
	AutoPlaceChildren( this );
 
}

/** Activates a level remote event in kismet. */
void UUTUIScene::ActivateLevelEvent(FName EventName)
{
	ULocalPlayer* LP = GetPlayerOwner();

	if(LP)
	{
		APlayerController* PlayerOwner = LP->Actor;
		UBOOL bFoundRemoteEvents = FALSE;

		// now find the level's sequence
		AWorldInfo* WorldInfo = GetWorldInfo();
		if ( WorldInfo != NULL )
		{
			USequence* GameSequence = WorldInfo->GetGameSequence();
			if ( GameSequence != NULL )
			{
				// now find the root sequence
				USequence* RootSequence = GameSequence->GetRootSequence();
				if ( RootSequence != NULL )
				{
					TArray<USeqEvent_RemoteEvent*> RemoteEvents;
					RootSequence->FindSeqObjectsByClass(USeqEvent_RemoteEvent::StaticClass(), (TArray<USequenceObject*>&)RemoteEvents);

					// now iterate through the list of events, activating those events that have a matching event tag
					for ( INT EventIndex = 0; EventIndex < RemoteEvents.Num(); EventIndex++ )
					{
						USeqEvent_RemoteEvent* RemoteEvt = RemoteEvents(EventIndex);
						if ( RemoteEvt != NULL && RemoteEvt->EventName == EventName && RemoteEvt->bEnabled == TRUE )
						{
							// attempt to activate the remote event
							RemoteEvt->CheckActivate(WorldInfo, PlayerOwner);
							bFoundRemoteEvents = TRUE;
						}
					}
				}
			}
		}
	}
}

#define INSTANCED_PROPERTY_FLAG (CPF_EditInline|CPF_ExportObject|CPF_NeedCtorLink)

/**
 * Search the object for all property that either require auto-linkup (to widgets already in the scene) or
 * instanced
 *
 * @Param	BaseObj - The object to search for properties in
 */
void UUTUIScene::AutoPlaceChildren( UUIScreenObject *const BaseObj)
{
	// Look at all properties of the UTUIScene and perform any needed initialization on them

	for( TFieldIterator<UObjectProperty,CLASS_IsAUObjectProperty> It( BaseObj->GetClass() ); It; ++It)
	{
		UObjectProperty* P = *It;

		//@todo xenon: fix up with May XDK
		if ( (P->PropertyFlags&(CPF_EditInline|CPF_ExportObject|CPF_Transient)) != 0)
		{

			/* We handle 2 cases here. 
			
			First we look to see if we can fill-out any tranisent UIObject member references.  This allows us to have
			a script base scene class that automatically pick up Widgets provided we maintain good nomenclature.  

			Second is auto-adding instanced widgets in a script-based class.
			*/
			if ( (P->PropertyFlags & CPF_Transient) != 0 )
			{
				// We are a transient property, try to auto-assign us. FIXME: Finish support for Arrays by appending _<index> to
				// the property name for element lookup.

				for (INT Index=0; Index < It->ArrayDim; Index++)
				{
					// Find the location of the data
					BYTE* ObjLoc = (BYTE*)BaseObj + It->Offset + (Index * It->ElementSize);
					UObject* ObjPtr = *((UObject**)( ObjLoc ));

					// Skip the assignment if already assigned via default properties.

					if ( !ObjPtr )
					{
						// TypeCheck!!
						UUIObject* SourceObj = BaseObj->FindChild( P->GetFName(), FALSE);
						if (SourceObj && SourceObj->GetClass()->IsChildOf(P->PropertyClass ) )
						{
							// Copy the value in to place
							P->CopyCompleteValue( ObjLoc, &SourceObj);
						}
					}
				}
			}
			else if ( (P->PropertyFlags&INSTANCED_PROPERTY_FLAG) == INSTANCED_PROPERTY_FLAG )
			{
				// Look to see if there is a UIObject assocaited with this property and if so
				// Insert it in to the children array
				for (INT Index=0; Index < It->ArrayDim; Index++)
				{
					UObject* ObjPtr = *((UObject**)((BYTE*)BaseObj + It->Offset + (Index * It->ElementSize) ));
					if ( ObjPtr )
					{
						UUIObject* UIObject = Cast<UUIObject>(ObjPtr);
						if ( UIObject )
						{
							// Should not be trying to attach a default object!
							checkf(!UIObject->IsTemplate(RF_ClassDefaultObject), TEXT("UUTUIObject::Initialize : Trying to insert Default Object %s (from property %s) into %s."), *UIObject->GetName(), *P->GetName(), *BaseObj->GetName());

							//@HACK - Big hack time.  The current UI doesn't support any type of render ordering
							// so rendering is determined solely by the order in which the components are added to
							// the children array.  To kind of account for this, we insert each child in to the 
							// array at the head. 
							//
							// This means you need to be careful with how you layout your member variables.
							// Members in a child will be found before members in the parent.  Otherwise they are in the 
							// order as defined by the code (top down).
							
							// Once a render ordering scheme is in place, remove the forced index and just let nature do the work.

							BaseObj->InsertChild(UIObject, 0);
						}
					}
				}
			}
		}
	}
}

/**
 * Perform a tick pass on all of the children requesting it from this UTUIScene
 *
 * @Param	DeltaTime		How much time since the last tick
 */
void UUTUIScene::Tick( FLOAT DeltaTime )
{
	if (GIsGame || bEditorRealTimePreview)
	{
		TickChildren(this, DeltaTime * GWorld->GetWorldInfo()->TimeDilation);
	}

	Super::Tick(DeltaTime);
}

/**
 * Tick the children before the scene.  This way children and change their dims and still
 *  be insured they are updated correctly.
 *
 * @Param	ParentObject		The Parent object who's children will be ticked
 * @Param	DeltaTime			How much time since the last time
 */

void UUTUIScene::TickChildren(UUIScreenObject* ParentObject, FLOAT DeltaTime)
{
	for ( INT ChildIndex = 0; ChildIndex < ParentObject->Children.Num() ; ChildIndex++ )
	{
		UUTUI_Widget* UTWidget = Cast<UUTUI_Widget>( ParentObject->Children(ChildIndex) );
		if (UTWidget && UTWidget->bRequiresTick)
		{
			// Perform the Tick on this widget
			UTWidget->Tick_Widget(DeltaTime);
		}
		else
		{
			UUTTabPage* UTTabPage = Cast<UUTTabPage>( ParentObject->Children(ChildIndex) );
			if (UTTabPage && UTTabPage->bRequiresTick)
			{
				// Perform the Tick on this widget
				UTTabPage->Tick_Widget(DeltaTime);
			}
			else
			{
				UUTUIMenuList* MenuList = Cast<UUTUIMenuList>( ParentObject->Children(ChildIndex) );
				if (MenuList)
				{
					// Perform the Tick on this widget
					MenuList->Tick_Widget(DeltaTime);
				}
			}
		}

		TickChildren( ParentObject->Children(ChildIndex) ,DeltaTime );
	}
}

/**
 * Perform a pre-render pass on all children that are UTUI_Widgets
 *
 * @Param	Canvas		The drawing surface
 */
void UUTUIScene::PreRender(FCanvas* Canvas)
{
	// Prerender the children

	for ( INT ChildIndex = 0; ChildIndex < Children.Num(); ChildIndex++ )
	{
		UUTUI_Widget* UTWidget = Cast<UUTUI_Widget>( Children(ChildIndex) );
		if ( UTWidget )
		{
			// Perform the Tick on this widget
			UTWidget->PreRender_Widget(Canvas);
		}
	}
}

/**
 * @Return the WorldInfo for the current game
 */
AWorldInfo* UUTUIScene::GetWorldInfo()
{
	return GWorld ? GWorld->GetWorldInfo() : NULL;
}


/**
 * This function will attempt to find the UTPlayerController associated with an Index.  If that Index is < 0 then it will find the first 
 * available UTPlayerController.  
 */
AUTPlayerController* UUTUIScene::GetUTPlayerOwner(INT PlayerIndex)
{
	if ( PlayerIndex < 0 )
	{
		if ( PlayerOwner )
		{
			// Attempt to find the first available
			AUTPlayerController* UTPC = Cast<AUTPlayerController>( PlayerOwner->Actor );

			if (UTPC)
			{
				return UTPC;
			}
		}
		PlayerIndex = 0;
	}

	ULocalPlayer* LP = GetPlayerOwner(PlayerIndex);
	if ( LP )
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>( LP->Actor );
		return UTPC;
	}
		
	return NULL;
}

/** @return Returns the current platform the game is running on. */
BYTE UUTUIScene::GetPlatform()
{
#if PS3
	return UTPlatform_PS3;
#elif XBOX
	return UTPlatform_360;
#else
	return UTPlatform_PC;
#endif
}

/**
 * This function will attempt to resolve the pawn that is associated with this scene
 */
APawn* UUTUIScene::GetPawnOwner()
{
	AUTPlayerController* PlayerOwner = GetUTPlayerOwner();
	if ( PlayerOwner )
	{
		APawn* ViewedPawn = PlayerOwner->ViewTarget ? PlayerOwner->ViewTarget->GetAPawn() : NULL;
		if ( !ViewedPawn )
		{
			ViewedPawn = PlayerOwner->Pawn;
		}

		return ViewedPawn;
	}
	return NULL;
}

/** 
 * This function returns the UTPlayerReplicationInfo of the Player that owns the hud.  We have 
 * to look at the pawn first to support remote viewing in a network game
 */
AUTPlayerReplicationInfo* UUTUIScene::GetPRIOwner()
{
	APawn* PawnOwner = GetPawnOwner();
	if ( PawnOwner && !PawnOwner->bDeleteMe && PawnOwner->Health > 0 )
	{
		return Cast<AUTPlayerReplicationInfo>(PawnOwner->PlayerReplicationInfo);
	}
	else
	{
		AUTPlayerController* PlayerOwner = GetUTPlayerOwner();
		if ( PlayerOwner )
		{
			return Cast<AUTPlayerReplicationInfo>(PlayerOwner->PlayerReplicationInfo);
		}
	}
	return NULL;
}



/**
 * @Returns GIsGame
 */
UBOOL UUTUIScene::IsGame()
{
	return GIsGame;
}

/** Starts a dedicated server. */
void UUTUIScene::StartDedicatedServer(const FString &TravelURL)
{
	//@Todo: Actually launch the dedicated server.
	appCreateProc(TEXT("UTGame.exe"), *FString::Printf(TEXT("Server %s"), *TravelURL));

	

	// Kill our own process.
	AUTPlayerController* PC = GetUTPlayerOwner(0);
	PC->ConsoleCommand(TEXT("Exit"));
}

//////////////////////////////////////////////////////////////////////////
// UTUIScene_MessageBox
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIScene_MessageBox);

void UUTUIScene_MessageBox::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	if(FadeDirection > 0)
	{
		FLOAT Alpha = (GWorld->GetRealTimeSeconds() - FadeStartTime) / FadeDuration;

		if(Alpha > 1.0f)
		{
			eventOnShowComplete();
			Alpha = 1.0f;
		}

		BackgroundImage->Opacity = Alpha;
	}
	else if(FadeDirection < 0)
	{
		FLOAT Alpha = (GWorld->GetRealTimeSeconds() - FadeStartTime) / FadeDuration;

		if(Alpha > 1.0f)
		{
			eventOnHideComplete();
			Alpha = 1.0f;
		}

		BackgroundImage->Opacity = 1.0f - Alpha;
	}
	else if(bHideOnNextTick && ((GWorld->GetRealTimeSeconds() - DisplayTime) > MinimumDisplayTime))	// Delay the hide if we need to wait until a minimum display time has passed.
	{
		BeginHide();
	}
}

void UUTUIScene_MessageBox::BeginShow()
{
	debugf(TEXT("UUTUIScene_MessageBox::BeginShow() - Showing MessageBox"));
	bFullyVisible = false;
	FadeStartTime = GWorld->GetRealTimeSeconds();
	DisplayTime = FadeStartTime+FadeDuration;
	FadeDirection = 1;
	BackgroundImage->Opacity = 0;

	// Show this scene.
	eventSetVisibility(true);
}

void UUTUIScene_MessageBox::BeginHide()
{
	debugf(TEXT("UUTUIScene_MessageBox::BeginHide() - Hiding MessageBox"));
	bHideOnNextTick=FALSE;
	bFullyVisible = false;
	FadeStartTime = GWorld->GetRealTimeSeconds();
	FadeDirection = -1;
}

