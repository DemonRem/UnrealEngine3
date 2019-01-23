/*=============================================================================
	UnUIContainers.cpp: Implementations for complex UI widget classes
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/


/* ==========================================================================================================
	Includes
========================================================================================================== */
#include "EnginePrivate.h"
#include "CanvasScene.h"

#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"

#include "EngineSequenceClasses.h"
#include "EngineUISequenceClasses.h"

#include "ScopedObjectStateChange.h"

#include "UnUIKeys.h"


/* ==========================================================================================================
	Definitions
========================================================================================================== */
IMPLEMENT_CLASS(UUIContainer);
	IMPLEMENT_CLASS(UUIPanel);
	IMPLEMENT_CLASS(UUIFrameBox);
	IMPLEMENT_CLASS(UUISafeRegionPanel);
	IMPLEMENT_CLASS(UUIScrollFrame);
	IMPLEMENT_CLASS(UUITabPage);

IMPLEMENT_CLASS(UUITabControl);


/* ==========================================================================================================
	Implementations
========================================================================================================== */


/* ==========================================================================================================
	UUIContainer
========================================================================================================== */
/**
 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUIContainer::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	if ( AutoAlignment != NULL )
	{
		AutoAlignment->ResolveFacePosition(Face);
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIContainer::PostEditChange( UProperty* PropertyThatChanged )
{
	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIContainer::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PostEditChange(PropertyThatChanged);
}


/* ==========================================================================================================
	UIPanel
========================================================================================================== */
/* === UIPanel interface === */
/**
 * Changes the background image for this button, creating the wrapper UITexture if necessary.
 *
 * @param	NewImage		the new surface to use for this UIImage
 * @param	NewCoordinates	the optional coordinates for use with texture atlasing
 */
void UUIPanel::SetBackgroundImage( USurface* NewImage )
{
	if ( BackgroundImageComponent != NULL )
	{
		BackgroundImageComponent->SetImage(NewImage);
	}
}

/* === UIObject interface === */
/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUIPanel::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(BackgroundImageComponent);
	AddStyleSubscriber(BackgroundImageComponent);
}

/* === UUIScreenObject interface === */
/**
 * Render this widget.
 *
 * @param	Canvas	the canvas to use for rendering this widget
 */
void UUIPanel::Render_Widget( FCanvas* Canvas )
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

/* === UObject interface === */
/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIPanel::PreEditChange( FEditPropertyChain& PropertyThatChanged )
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
void UUIPanel::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
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
				if ( MemberProperty == ModifiedProperty )
				{
					if ( BackgroundImageComponent != NULL )
					{
						UUIComp_DrawImage* ComponentTemplate = GetArchetype<UUIPanel>()->BackgroundImageComponent;
						if ( ComponentTemplate != NULL )
						{
							BackgroundImageComponent->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}
						else
						{
							BackgroundImageComponent->StyleResolverTag = TEXT("Panel Background Style");
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
void UUIPanel::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake && GetLinkerVersion() < VER_FIXED_UISCROLLBARS  )
	{
		MigrateImageSettings(PanelBackground, Coordinates, PrimaryStyle, BackgroundImageComponent, TEXT("PanelBackground"));
	}
}


/* ==========================================================================================================
	UIFrameBox
========================================================================================================== */
/* === UIFrameBox interface === */
/**
 * Changes the background image for one of the image components.
 *
 * @param	ImageToSet		The image component we are going to set the image for.
 * @param	NewImage		the new surface to use for this UIImage
 */
void UUIFrameBox::SetBackgroundImage( EFrameBoxImage ImageToSet, USurface* NewImage )
{
	if ( BackgroundImageComponent[ImageToSet] != NULL )
	{
		BackgroundImageComponent[ImageToSet]->SetImage(NewImage);
	}
}

/* === UIObject interface === */
/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUIFrameBox::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	for(INT ImageIdx=0; ImageIdx<FBI_MAX; ImageIdx++)
	{
		VALIDATE_COMPONENT(BackgroundImageComponent[ImageIdx]);
		AddStyleSubscriber(BackgroundImageComponent[ImageIdx]);
	}
}

/* === UUIScreenObject interface === */
/**
 * Render this widget.
 *
 * @param	Canvas	the canvas to use for rendering this widget
 */
void UUIFrameBox::Render_Widget( FCanvas* Canvas )
{
	if ( BackgroundImageComponent != NULL )
	{
		FRenderParameters ImageParams;
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
			);

		// Top Left
		ImageParams = Parameters;
		ImageParams.DrawXL = BackgroundCornerSizes.TopLeft[0];
		ImageParams.DrawYL = BackgroundCornerSizes.TopLeft[1];
		BackgroundImageComponent[FBI_TopLeft]->RenderComponent(Canvas, ImageParams);

		// Top
		ImageParams = Parameters;
		ImageParams.DrawX += BackgroundCornerSizes.TopLeft[0];
		ImageParams.DrawXL -= (BackgroundCornerSizes.TopLeft[0] + BackgroundCornerSizes.TopRight[0]);
		ImageParams.DrawYL = BackgroundCornerSizes.TopHeight;
		BackgroundImageComponent[FBI_Top]->RenderComponent(Canvas, ImageParams);

		// Top Right
		ImageParams = Parameters;
		ImageParams.DrawX += Parameters.DrawXL - BackgroundCornerSizes.TopRight[0];
		ImageParams.DrawXL = BackgroundCornerSizes.TopRight[0];
		ImageParams.DrawYL = BackgroundCornerSizes.TopRight[1];
		BackgroundImageComponent[FBI_TopRight]->RenderComponent(Canvas, ImageParams);

		// Center Left
		ImageParams = Parameters;
		ImageParams.DrawY += BackgroundCornerSizes.TopLeft[1];
		ImageParams.DrawXL = BackgroundCornerSizes.CenterLeftWidth;
		ImageParams.DrawYL -= (BackgroundCornerSizes.TopLeft[1] + BackgroundCornerSizes.BottomLeft[1]);
		BackgroundImageComponent[FBI_CenterLeft]->RenderComponent(Canvas, ImageParams);

		// Center
		ImageParams = Parameters;
		ImageParams.DrawX += BackgroundCornerSizes.CenterLeftWidth;
		ImageParams.DrawY += BackgroundCornerSizes.TopHeight;
		ImageParams.DrawXL -= (BackgroundCornerSizes.CenterLeftWidth + BackgroundCornerSizes.CenterRightWidth);
		ImageParams.DrawYL -= (BackgroundCornerSizes.TopHeight + BackgroundCornerSizes.BottomHeight);
		BackgroundImageComponent[FBI_Center]->RenderComponent(Canvas, ImageParams);

		// Center Right
		ImageParams = Parameters;
		ImageParams.DrawX += Parameters.DrawXL - BackgroundCornerSizes.CenterRightWidth;
		ImageParams.DrawY += BackgroundCornerSizes.TopRight[1];
		ImageParams.DrawXL = BackgroundCornerSizes.CenterRightWidth;
		ImageParams.DrawYL -= (BackgroundCornerSizes.TopRight[1] + BackgroundCornerSizes.BottomRight[1]);
		BackgroundImageComponent[FBI_CenterRight]->RenderComponent(Canvas, ImageParams);

		// Bottom Left
		ImageParams = Parameters;
		ImageParams.DrawY += Parameters.DrawYL - BackgroundCornerSizes.BottomLeft[1];
		ImageParams.DrawXL = BackgroundCornerSizes.BottomLeft[0];
		ImageParams.DrawYL = BackgroundCornerSizes.BottomLeft[1];
		BackgroundImageComponent[FBI_BottomLeft]->RenderComponent(Canvas, ImageParams);

		// Bottom
		ImageParams = Parameters;
		ImageParams.DrawX += BackgroundCornerSizes.BottomLeft[0];
		ImageParams.DrawY += Parameters.DrawYL - BackgroundCornerSizes.BottomHeight;
		ImageParams.DrawXL -= (BackgroundCornerSizes.BottomLeft[0] + BackgroundCornerSizes.BottomRight[0]);
		ImageParams.DrawYL = BackgroundCornerSizes.BottomHeight;
		BackgroundImageComponent[FBI_Bottom]->RenderComponent(Canvas, ImageParams);

		// Bottom Right
		ImageParams = Parameters;
		ImageParams.DrawX += Parameters.DrawXL - BackgroundCornerSizes.BottomRight[0];
		ImageParams.DrawY += Parameters.DrawYL - BackgroundCornerSizes.BottomRight[1];
		ImageParams.DrawXL = BackgroundCornerSizes.BottomRight[0];
		ImageParams.DrawYL = BackgroundCornerSizes.BottomRight[1];
		BackgroundImageComponent[FBI_BottomRight]->RenderComponent(Canvas, ImageParams);
	}
}

/* === UObject interface === */
/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIFrameBox::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	/*
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
	*/
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIFrameBox::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	/*
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
				if ( MemberProperty == ModifiedProperty )
				{
					if ( BackgroundImageComponent != NULL )
					{
						UUIComp_DrawImage* ComponentTemplate = GetArchetype<UUIPanel>()->BackgroundImageComponent;
						if ( ComponentTemplate != NULL )
						{
							BackgroundImageComponent->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}
						else
						{
							BackgroundImageComponent->StyleResolverTag = TEXT("Panel Background Style");
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
	*/

	Super::PostEditChange(PropertyThatChanged);
}

/* ==========================================================================================================
	UUISafeRegionPanel
========================================================================================================== */

/**
* Initializes the panel and sets its position to match the safe region.
* This is ugly.
*/
void UUISafeRegionPanel::ResolveFacePosition(EUIWidgetFace Face)
{
	// if this is the first face of this panel that is being resolved, set the position of the panel according to the current viewport size
	if ( GetNumResolvedFaces() == 0 )
	{
		AlignPanel();
	}

	Super::ResolveFacePosition(Face);
}


void UUISafeRegionPanel::Render_Widget( FCanvas* Canvas )
{
	if ( !GIsGame && !bHideBounds ) 
	{
		// Render the bounds 
		FLOAT X  = RenderBounds[UIFACE_Left];
		FLOAT Y  = RenderBounds[UIFACE_Top];
		FLOAT tW = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];
		FLOAT tH = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];

		DrawTile(Canvas, X,Y, tW, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(0.6,0.6,0.2,1.0));
		DrawTile(Canvas, X,Y, 1.0f, tH, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(0.6,0.6,0.2,1.0));

		DrawTile(Canvas, X,Y+tH, tW, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(0.6,0.6,0.2,1.0));
		DrawTile(Canvas, X+tW,Y, 1.0f, tH, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(0.6,0.6,0.2,1.0));
	}

	Super::Render_Widget(Canvas);
}

void UUISafeRegionPanel::AlignPanel()
{
	FVector2D ViewportSize;

	// Calculate the Preview Platform.

	if( OwnerScene && OwnerScene->GetViewportSize(ViewportSize) )
	{
		if ( bForce4x3AspectRatio )
		{
			FLOAT PercentageAmount;
			
			if ( bUseFullRegionIn4x3 )
			{
				PercentageAmount = RegionPercentages( ESRT_FullRegion );
			}
			else
			{
				PercentageAmount = RegionPercentages( RegionType );
			}
				
			// Caculate the Height/Width maintining a 4:3 ratio using the height

			INT Height = appTrunc( ViewportSize.Y * PercentageAmount );
			INT Width = appTrunc( FLOAT(Height) * 1.3333334f );

			FLOAT Left = (ViewportSize.X - Width) * 0.5;
			FLOAT Top = (ViewportSize.Y - Height) * 0.5;

			SetPosition(Left,Top,Left+Width, Top+Height, EVALPOS_PixelViewport, FALSE);
		}
		else
		{
			FLOAT PercentageAmount;

			if ( bUseFullRegionIn4x3 && ( (ViewportSize.X / ViewportSize.Y) - 1.333333 <= KINDA_SMALL_NUMBER ) )
			{
				PercentageAmount = RegionPercentages( ESRT_FullRegion );
			}
			else
			{
				PercentageAmount = RegionPercentages( RegionType );
			}

			FLOAT BorderPerc = (1.0 - PercentageAmount) * 0.5;

			SetPosition(BorderPerc, BorderPerc, PercentageAmount, PercentageAmount, EVALPOS_PercentageOwner, FALSE);
		}
	}
}

void UUISafeRegionPanel::PostEditChange(UProperty* PropertyThatChanged )
{
	if ( GetScene() != NULL && GetScene()->SceneClient != NULL && GetScene()->SceneClient->IsSceneInitialized(GetScene()) )
	{
 		AlignPanel();
	}
	Super::PostEditChange(PropertyThatChanged);
}

/* ==========================================================================================================
	UITabControl
========================================================================================================== */
/* === UITabControl interface === */
/**
 * Enables the bUpdateLayout flag and triggers a scene update to occur during the next frame.
 */
void UUITabControl::RequestLayoutUpdate()
{
	bUpdateLayout = TRUE;
	RequestSceneUpdate(TRUE,TRUE);
}

/**
 * Positions and resizes the tab buttons according the tab control's configuration.
 */
void UUITabControl::ReapplyLayout()
{
	// first, setup all the docking links between the tab control, tab buttons, and tab pages
	SetupDockingRelationships();

	// now resize the tabs according to the value of TabSizeMode
	switch ( TabSizeMode )
	{
	case TAST_Manual:
		// nothing - the size for each button is set manually
		break;

	case TAST_Fill:
		// in order to resize the tab buttons, we must first know the width (or height, if dockface is left or right)
		// of the tab control.  We won't know that until after ResolveFacePosition is called for both faces.  So here
		// we just make sure that autosizing is NOT enabled for the tab buttons.
		// the width (or height if the tabs are vertical) of each tab button is determined
		// by the length of its caption; so just enable auto-sizing
		for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
		{
			UUITabButton* TabButton = Pages(PageIndex)->TabButton;
			if ( TabButton != NULL && TabButton->StringRenderComponent != NULL )
			{
				TabButton->StringRenderComponent->eventEnableAutoSizing(UIORIENT_Horizontal, FALSE);
			}
		}
		break;

	case TAST_Auto:
		{
			// the width (or height if the tabs are vertical) of each tab button is determined
			// by the length of its caption; so just enable auto-sizing
			for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
			{
				UUITabButton* TabButton = Pages(PageIndex)->TabButton;
				if ( TabButton != NULL && TabButton->StringRenderComponent != NULL )
				{
					const FLOAT SplitPadding = TabButtonPadding.Value * 0.5f;

					TabButton->StringRenderComponent->eventSetAutoSizePadding(UIORIENT_Horizontal, 
						SplitPadding, SplitPadding, TabButtonPadding.ScaleType, TabButtonPadding.ScaleType);

					TabButton->StringRenderComponent->eventEnableAutoSizing(UIORIENT_Horizontal, TRUE);
				}
			}
		}
		break;
	}
}

/**
 * Set up the docking links between the tab control, buttons, and pages, based on the TabDockFace.
 */
void UUITabControl::SetupDockingRelationships()
{
	if ( TabDockFace < UIFACE_MAX )
	{
		FLOAT ActualButtonHeight = TabButtonSize.GetValue(this);

		EUIWidgetFace TargetFace=(EUIWidgetFace)TabDockFace;
		EUIWidgetFace SourceFace = GetOppositeFace(TabDockFace);

		EUIWidgetFace AlignmentSourceFace, AlignmentTargetFace;
		if ( TargetFace == UIFACE_Top || TargetFace == UIFACE_Bottom )
		{
			AlignmentSourceFace = AlignmentTargetFace = UIFACE_Left;
			if ( TargetFace == UIFACE_Bottom )
			{
				ActualButtonHeight *= -1;
			}
		}
		else
		{
			AlignmentSourceFace = AlignmentTargetFace = UIFACE_Top;
		}

		// the first button will dock to the tab control itself.
		UUIObject* CurrentAlignmentDockTarget = this;
		for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
		{
			UUITabPage* TabPage = Pages(PageIndex);
			UUITabButton* TabButton = TabPage->TabButton;

			checkSlow(TabPage!=NULL);
			checkSlow(TabButton!=NULL);

			// dock the buttons to the tab control - (the following comments assume we're docked to the
			// top face of the tab control, for sake of example):
			// dock the top face of the button to the top face of the tab control
			TabButton->SetDockParameters(TargetFace, this, TargetFace, 0.f);
			// dock the bottom face of the button to the top face of the tab control using the configured
			// button height as the dock padding
			TabButton->SetDockParameters(SourceFace, this, TargetFace, ActualButtonHeight);
			// dock the left face of the button to the left face of the tab control (if it's the first button),
			// or the right face of the previous button in the list (if not the first button).  The first time
			// through this loop, AlignmentSourceFace and AlignmentTargetFace are the same value, but we'll switch
			// AlignmentTargetFace to the opposite face right after this
			TabButton->SetDockParameters(AlignmentSourceFace, CurrentAlignmentDockTarget, AlignmentTargetFace, 0.f);

			// the next button in the list will dock to this button
			CurrentAlignmentDockTarget = TabButton;

			// the next button in the list will dock to the face opposite the face it's docking (i.e. dock left face to right face)
			AlignmentTargetFace = GetOppositeFace(AlignmentSourceFace);


			// now the page itself - dock the top face of the panel to the bottom face of its TabButton
			TabPage->SetDockParameters(TargetFace, TabButton, SourceFace, 0.f);

			// then dock the remaining faces of the page to the tab control
			for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
			{
				if ( FaceIndex != TargetFace )
				{
					TabPage->SetDockParameters(FaceIndex, this, FaceIndex, 0.f);
				}
			}
		}
	}
}

/**
 * Returns the number of pages in this tab control.
 */
INT UUITabControl::GetPageCount() const
{
	return Pages.Num();
}

/**
 * Returns a reference to the page at the specified index.
 */
UUITabPage* UUITabControl::GetPageAtIndex( INT PageIndex ) const
{
	UUITabPage* Result = NULL;
	if ( Pages.IsValidIndex(PageIndex) )
	{
		Result = Pages(PageIndex);
	}
	return Result;
}

/**
 * Returns a reference to the tab button which is currently in the Targeted state, or NULL if no buttons are in that state.
 */
UUITabButton* UUITabControl::FindTargetedTab( INT PlayerIndex ) const
{
	UUITabButton* Result = NULL;

	for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
	{
		UUITabPage* Page = Pages(PageIndex);
		if ( Page != NULL && Page->TabButton != NULL && Page->TabButton->IsTargeted(PlayerIndex) )
		{
			Result = Page->TabButton;
			break;
		}
	}

	return Result;
}

/**
 * Creates a new UITabPage of the specified class as well as its associated tab button.
 *
 * @param	TabPageClass	the class to use for creating the tab page.
 *
 * @return	a pointer to a new instance of the specified UITabPage class
 */
UUITabPage* UUITabControl::CreateTabPage( UClass* TabPageClass, UUITabPage* PagePrefab/*=0*/ )
{
	UUITabPage* Result = NULL;

	if ( TabPageClass != NULL )
	{
		checkSlow(TabPageClass->IsChildOf(UUITabPage::StaticClass()));

		// first, we need to create the tab button; to do this, we call a static method in the tab page class,
		// which we'll need the TabPageClass's CDO for
		UUITabButton* TabButton = NULL;
		if ( PagePrefab == NULL )
		{
			UUITabPage* TabPageCDO = TabPageClass->GetDefaultObject<UUITabPage>();
			TabButton = TabPageCDO->eventCreateTabButton(this);
		}
		else
		{
			TabButton = PagePrefab->eventCreateTabButton(this);
		}

		if ( TabButton != NULL )
		{
			FScopedObjectStateChange TabButtonNotification(TabButton);

			// now that we have the TabButton, have it create the tab page using the specified class
			UUITabPage* NewTabPage = Cast<UUITabPage>(TabButton->CreateWidget(TabButton, TabPageClass, PagePrefab));
			FScopedObjectStateChange TabPageNotification(NewTabPage);

			// need to link the tab page and tab button together.
			if ( NewTabPage->eventLinkToTabButton(TabButton, this) )
			{
				Result = NewTabPage;
			}
			else
			{
				TabPageNotification.CancelEdit();
				TabButtonNotification.CancelEdit();
			}
		}
	}
	return Result;
}

/* === UIObject interface === */
/**
 * Render this widget.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUITabControl::Render_Widget( FCanvas* Canvas )
{
	Super::Render_Widget(Canvas);
}

/**
 * Adds docking nodes for all faces of this widget to the specified scene
 *
 * @param	DockingStack	the docking stack to add this widget's docking.  Generally the scene's DockingStack.
 *
 * @return	TRUE if docking nodes were successfully added for all faces of this widget.
 */
UBOOL UUITabControl::AddDockingLink( TArray<FUIDockingNode>& DockingStack )
{
	if ( bUpdateLayout )
	{
		bUpdateLayout = FALSE;
		ReapplyLayout();
	}

	return Super::AddDockingLink(DockingStack);
}

/**
 * Adds the specified face to the DockingStack for the specified widget.
 *
 * This version ensures that the tab buttons faces (and thus, the size of their captions) have already been resolved
 * Only relevant when the TabSizeMode is TAST_Fill, because we must make sure that all buttons are at least wide enough
 * to fit the largest caption of the group.
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUITabControl::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	checkSlow(Face<UIFACE_MAX);

	if ( TabSizeMode == TAST_Fill )
	{
		switch( TabDockFace )
		{
		// docked to the top or bottom faces
		case UIFACE_Top:
		case UIFACE_Bottom:
			{
				if ( Face == UIFACE_Right && !DockingStack.ContainsItem(FUIDockingNode(this,Face)) )
				{
					for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
					{
						UUITabPage* Page = Pages(PageIndex);
						UUIButton* TabButton = Page->TabButton;
						for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
						{
							TabButton->AddDockingNode(DockingStack, (EUIWidgetFace)FaceIndex);
						}
					}
				}
			}
			break;

		case UIFACE_Left:
		case UIFACE_Right:
			if ( Face == UIFACE_Bottom && !DockingStack.ContainsItem(FUIDockingNode(this,Face)) )
			{
				for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
				{
					UUITabPage* Page = Pages(PageIndex);
					UUIButton* TabButton = Page->TabButton;
					for ( INT FaceIndex = 0; FaceIndex < UIFACE_MAX; FaceIndex++ )
					{
						TabButton->AddDockingNode(DockingStack, (EUIWidgetFace)FaceIndex);
					}
				}
			}
			break;

		default:
			break;
		}
	}

	return Super::AddDockingNode(DockingStack, Face);
}

/**
 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUITabControl::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	if ( Pages.Num() > 0 && TabSizeMode == TAST_Fill )
	{
		EUIWidgetFace FirstFace, SecondFace;
		EUIOrientation Orientation;
		switch( TabDockFace )
		{
		case UIFACE_Top:
		case UIFACE_Bottom:

			FirstFace = UIFACE_Left;
			SecondFace = UIFACE_Right;
			Orientation = UIORIENT_Horizontal;
			break;

		case UIFACE_Left:
		case UIFACE_Right:
			FirstFace = UIFACE_Top;
			SecondFace = UIFACE_Bottom;
			Orientation = UIORIENT_Vertical;
			break;

		default:
			FirstFace = UIFACE_MAX;
			SecondFace = UIFACE_MAX;
			Orientation = UIORIENT_MAX;
			checkf(false, TEXT("Invalid value for TabDockFace (UIFACE_MAX) in '%s'"), *GetWidgetPathName());
			break;
		}

		if ( Face == FirstFace || Face == SecondFace )
		{
			//@todo ronp - do I need to override AddDockingNode to ensure that these faces are added first?
			if ( HasPositionBeenResolved(FirstFace) && HasPositionBeenResolved(SecondFace) )
			{
				// get the size of the area available to render the buttons
				const FLOAT BoundingRegionSize = Position.GetBoundsExtent(this, Orientation, EVALPOS_PixelViewport);

				// determine how wide each button should be to fill the available space
				FLOAT TargetButtonSize = BoundingRegionSize / Pages.Num();

				// this is the smallest size a button can be
				FLOAT MinButtonSize = TargetButtonSize;

				// now check to see if any buttons are larget that TargetButtonWidth.  If so, it indicates that there isn't enough
				// space to fit all buttons evenly
				for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
				{
					UUITabButton* TabButton = Pages(PageIndex)->TabButton;
					if ( TabButton->StringRenderComponent != NULL && TabButton->StringRenderComponent->ValueString != NULL )
					{
						UUIString* CaptionString = TabButton->StringRenderComponent->ValueString;
						if ( CaptionString != NULL )
						{
							MinButtonSize = Max<FLOAT>(MinButtonSize, CaptionString->StringExtent[Orientation]);
						}
					}
				}

				// this will be where each button's right/bottom face will fall, in absolute pixels
				FLOAT CurrentButtonPosition = Position.GetPositionValue(this, FirstFace, EVALPOS_PixelViewport);

				// resize all buttons so that they are that size.
				//@fixme ronp - handle the case where MinButtonSize > TargetButtonWidth
				for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
				{
					UUITabButton* TabButton = Pages(PageIndex)->TabButton;

					// set the position and width of the button to the desired values.  Prevent SetPositionValue from triggering
					// another scene update (which will cause a flicker since the button would be rendered this frame
					// using its previously resolved position, but the next frame will use this new value); this requires
					// us to also update the button's RenderBounds for that face as well
					TabButton->Position.SetPositionValue(TabButton, CurrentButtonPosition, FirstFace, EVALPOS_PixelViewport, FALSE);
					TabButton->Position.SetPositionValue(TabButton, TargetButtonSize, SecondFace, EVALPOS_PixelOwner, FALSE);

					TabButton->RenderBounds[FirstFace] = CurrentButtonPosition;
					TabButton->RenderBounds[SecondFace] = CurrentButtonPosition + TargetButtonSize;

					// advance the button face location by the size of the button
					CurrentButtonPosition += TargetButtonSize;
				}
			}
		}
	}
}

/**
 * Called when a style reference is resolved successfully.  Applies the TabButtonCaptionStyle and TabButtonBackgroundStyle
 * to the tab buttons.
 *
 * @param	ResolvedStyle			the style resolved by the style reference
 * @param	StyleProperty			the name of the style reference property that was resolved.
 * @param	ArrayIndex				the array index of the style reference that was resolved.  should only be >0 for style reference arrays.
 * @param	bInvalidateStyleData	if TRUE, the resolved style is different than the style that was previously resolved by this style reference.
 */
void UUITabControl::OnStyleResolved( UUIStyle* ResolvedStyle, const FStyleReferenceId& StylePropertyId, INT ArrayIndex, UBOOL bInvalidateStyleData )
{
	Super::OnStyleResolved(ResolvedStyle,StylePropertyId,ArrayIndex,bInvalidateStyleData);

	FString StylePropertyName = StylePropertyId.GetStyleReferenceName();
	if ( StylePropertyName == TEXT("TabButtonBackgroundStyle") || StylePropertyName == TEXT("TabButtonCaptionStyle") )
	{
		for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
		{
			UUITabPage* Page = Pages(PageIndex);
			if ( Page != NULL && Page->TabButton != NULL )
			{
				Page->TabButton->SetWidgetStyle(ResolvedStyle, StylePropertyId, ArrayIndex);
			}
		}
	}
}

/* === UUIScreenObject interface === */
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
void UUITabControl::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	// Because we manage the styles for the tab buttons, and the buttons' styles are stored in their components (which are style subscribers),
	// we need to initialize the StyleSubscribers arrays for the buttons prior to calling Super::Initialize() so that the first time
	// OnResolveStyles is called, the buttons will be ready to receive those styles.
	for ( INT PageIndex = 0; PageIndex < Pages.Num(); PageIndex++ )
	{
		UUITabPage* Page = Pages(PageIndex);
		if ( Page != NULL && Page->TabButton != NULL )
		{
			Page->TabButton->InitializeStyleSubscribers();
		}
	}

	Super::Initialize(inOwnerScene, inOwner);
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUITabControl::GetSupportedUIActionKeyNames( TArray<FName>& out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddUniqueItem(UIKEY_NextPage);
	out_KeyNames.AddUniqueItem(UIKEY_PreviousPage);
}

/**
 * Assigns values to the links which are used for navigating through this widget using the keyboard.  Sets the first and
 * last focus targets for this widget as well as the next/prev focus targets for all children of this widget.
 *
 * This version clears the navigation links between the tab buttons so that
 */
void UUITabControl::RebuildKeyboardNavigationLinks()
{
	Super::RebuildKeyboardNavigationLinks();
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
UBOOL UUITabControl::SetFocus( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	UUITabButton* TargetedTab = FindTargetedTab(PlayerIndex);
	if ( bAllowPagePreviews && TargetedTab != NULL && IsVisible() && CanAcceptFocus(PlayerIndex) && (Sender == NULL || Sender == GetParent()) )
	{
		if ( TargetedTab == GetLastFocusedControl(PlayerIndex) )
		{
			// when returning focus to a tab control that has a targeted tab, we always want the tab page's first control to receive focus,
			// not whichever control was last focused (the default behavior)
			bResult = TargetedTab->FocusFirstControl(this, PlayerIndex);
		}
		else
		{
			GainFocus(NULL, PlayerIndex);

			// if we currently have a targeted tab, we won't propagate focus anywhere; we'll just make ourselves the focused control
			// so that left/right will switch which tab is targeted
			eventActivatePage(TargetedTab->TabPage, PlayerIndex, FALSE);
			bResult = TRUE;
		}
	}
	else
	{
		bResult = Super::SetFocus(Sender, PlayerIndex);
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
UBOOL UUITabControl::FocusFirstControl( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	UUITabButton* TargetedTab = FindTargetedTab(PlayerIndex);
	if ( bAllowPagePreviews && TargetedTab != NULL )
	{
		if ( TargetedTab == GetLastFocusedControl(PlayerIndex) )
		{
			// when returning focus to a tab control that has a targeted tab, we always want the tab page's first control to receive focus,
			// not whichever control was last focused (the default behavior)
			TargetedTab->DeactivateStateByClass(UUIState_TargetedTab::StaticClass(), PlayerIndex);
			bResult = TargetedTab->FocusFirstControl(this, PlayerIndex);
		}
		else
		{
			GainFocus(NULL, PlayerIndex);

			// if we currently have a targeted tab, we won't propagate focus anywhere; we'll just make ourselves the focused control
			// so that left/right will switch which tab is targeted
			eventActivatePage(TargetedTab->TabPage, PlayerIndex, FALSE);
			bResult = TRUE;
		}
	}
	else if ( ActivePage != NULL )
	{
		bResult = ActivePage->FocusFirstControl(NULL, PlayerIndex);
	}
	else
	{
		bResult = Super::FocusFirstControl(Sender, PlayerIndex);
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
UBOOL UUITabControl::FocusLastControl( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	UBOOL bResult = FALSE;

	UUITabButton* TargetedTab = FindTargetedTab(PlayerIndex);
	if ( bAllowPagePreviews && TargetedTab != NULL )
	{
		if ( TargetedTab == GetLastFocusedControl(PlayerIndex) )
		{
			// when returning focus to a tab control that has a targeted tab, we always want the tab page's first control to receive focus,
			// not whichever control was last focused (the default behavior)
			TargetedTab->DeactivateStateByClass(UUIState_TargetedTab::StaticClass(), PlayerIndex);
			bResult = TargetedTab->FocusLastControl(this, PlayerIndex);
		}
		else
		{
			GainFocus(NULL, PlayerIndex);

			// if we currently have a targeted tab, we won't propagate focus anywhere; we'll just make ourselves the focused control
			// so that left/right will switch which tab is targeted
			eventActivatePage(TargetedTab->TabPage, PlayerIndex, FALSE);
			bResult = TRUE;
		}
	}
	else if ( ActivePage != NULL )
	{
		bResult = ActivePage->FocusLastControl(NULL, PlayerIndex);
	}
	else
	{
		bResult = Super::FocusLastControl(Sender, PlayerIndex);
	}

	return bResult;
}

/**
 * Sets focus to the next control in the tab order (relative to Sender) for widget.  If Sender is the last control in
 * the tab order, propagates the call upwards to this widget's parent widget.
 *
 * @param	Sender			the widget to use as the base for determining which control to focus next
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 *
 * @return	TRUE if we successfully set focus to the next control in tab order.  FALSE if Sender was the last eligible
 *			child of this widget or we couldn't otherwise set focus to another control.
 */
UBOOL UUITabControl::NextControl( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	tracef(TEXT("UUITabControl::NextControl  Sender:%s  Focused:%s"), *Sender->GetName(), *GetFocusedControl(PlayerIndex)->GetName());
	UBOOL bResult = FALSE;

	UUITabButton* TabButtonSender = Cast<UUITabButton>(Sender);

	// if the sender is one of this tab control's tab buttons, it means that the currently focused control
	// is the last control in the currently active page and the focus chain is attempting to set focus to the
	// next page's first control.  We don't allow this because in the tab control, navigation between pages can
	// only happen when ActivatePage is called.  Instead, what we do is make the tab control itself the overall
	// focused control so that the user can use the arrow keys to move between tab buttons.
	if ( TabButtonSender == NULL || TabButtonSender->GetOwner() != this )
	{
		bResult = Super::NextControl(Sender, PlayerIndex);
	}
	else
	{
		if ( !bAllowPagePreviews )
		{
			bResult = Super::NextControl(this, PlayerIndex);
		}
		else
		{
			GainFocus(NULL, PlayerIndex);

			// make the tab button the targeted tab button
			TabButtonSender->ActivateStateByClass(UUIState_TargetedTab::StaticClass(), PlayerIndex);
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Sets focus to the previous control in the tab order (relative to Sender) for widget.  If Sender is the first control in
 * the tab order, propagates the call upwards to this widget's parent widget.
 *
 * @param	Sender			the widget to use as the base for determining which control to focus next
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 *
 * @return	TRUE if we successfully set focus to the previous control in tab order.  FALSE if Sender was the first eligible
 *			child of this widget or we couldn't otherwise set focus to another control.
 */
UBOOL UUITabControl::PrevControl( UUIScreenObject* Sender, INT PlayerIndex/*=0*/ )
{
	tracef(TEXT("UUITabControl::PrevControl  Sender:%s  Focused:%s"), *Sender->GetName(), *GetFocusedControl(PlayerIndex)->GetName());
	UBOOL bResult = FALSE;

	UUITabButton* TabButtonSender = Cast<UUITabButton>(Sender);

	// if the sender is one of this tab control's tab buttons, it means that the currently focused control
	// is the first control in the currently active page and the focus chain is attempting to set focus to the
	// previous page's last control.  We don't allow this because in the tab control, navigation between pages can
	// only happen when ActivatePage is called.  Instead, what we do is make the tab control itself the overall
	// focused control so that the user can use the arrow keys to move between tab buttons.
	if ( TabButtonSender == NULL || TabButtonSender->GetOwner() != this )
	{
		bResult = Super::PrevControl(Sender, PlayerIndex);
	}
	else
	{
		if ( !bAllowPagePreviews )
		{
			bResult = Super::PrevControl(this, PlayerIndex);
		}
		else
		{
			GainFocus(NULL, PlayerIndex);

			// make the tab button the targeted tab button
			TabButtonSender->ActivateStateByClass(UUIState_TargetedTab::StaticClass(), PlayerIndex);
			bResult = TRUE;
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
UBOOL UUITabControl::NavigateFocus( UUIScreenObject* Sender, BYTE Direction, INT PlayerIndex/*=0*/ )
{
	tracef(TEXT("UUITabControl::NavigateFocus  Sender:%s  Direction:%s  Focused:%s"), *Sender->GetName(), *GetDockFaceText(Direction), *GetFocusedControl(PlayerIndex)->GetName());
	UBOOL bResult = FALSE;

	UUITabButton* TabButtonSender = Cast<UUITabButton>(Sender);

	// if the sender is one of this tab control's tab buttons, it means that the currently focused control
	// is the first or last control in the currently active page and the focus chain is attempting to set focus to the
	// the nearest sibling of that tab button.  We don't allow this because in the tab control, navigation between pages can
	// only happen when ActivatePage is called.  Instead, what we do is make the tab control itself the overall
	// focused control so that the user can use the arrow keys to move between tab buttons.
	if ( TabButtonSender == NULL || TabButtonSender->GetOwner() != this )
	{
		bResult = Super::NavigateFocus(Sender, Direction, PlayerIndex);
	}
	else
	{
		if ( !bAllowPagePreviews )
		{
			bResult = Super::NavigateFocus(this, Direction, PlayerIndex);
		}
		else
		{
			GainFocus(NULL, PlayerIndex);

			// make the tab button the targeted tab button
			TabButtonSender->ActivateStateByClass(UUIState_TargetedTab::StaticClass(), PlayerIndex);
			bResult = TRUE;
		}
	}

	return bResult;
}


/**
 * Called when the scene receives a notification that the viewport has been resized.  Propagated down to all children.
 *
 * This version requests a layout update on the tab control.
 *
 * @param	OldViewportSize		the previous size of the viewport
 * @param	NewViewportSize		the new size of the viewport
 */
void UUITabControl::NotifyResolutionChanged( const FVector2D& OldViewportSize, const FVector2D& NewViewportSize )
{
	RequestLayoutUpdate();

	Super::NotifyResolutionChanged(OldViewportSize, NewViewportSize);
}

/**
 * Handles input events for this widget.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUITabControl::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	if ( EventParms.InputAliasName == UIKEY_NextPage || EventParms.InputAliasName == UIKEY_PreviousPage )
	{
		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Repeat )
		{
			if ( EventParms.InputAliasName == UIKEY_NextPage )
			{
				eventActivateNextPage(EventParms.PlayerIndex);
			}
			else
			{
				eventActivatePreviousPage(EventParms.PlayerIndex);
			}
		}

		bResult = TRUE;
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
}

/* === UObject interface === */
/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUITabControl::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);
}

/**
 * Called when a property value from a member struct or array has been changed in the editor.
 */
void UUITabControl::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PostEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* ModifiedMemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( ModifiedMemberProperty != NULL )
		{
			FName PropertyName = ModifiedMemberProperty->GetFName();
			if ( PropertyName == TEXT("TabDockFace") || PropertyName == TEXT("TabButtonSize") || PropertyName == TEXT("TabButtonPadding") || PropertyName == TEXT("Pages") )
			{
				RequestLayoutUpdate();
			}
			else if ( PropertyName == TEXT("TabSizeMode") )
			{
				RequestLayoutUpdate();

				// if the user changed the way tabs are autosized, we'll need to reapply formatting to the tab's captions
				RequestFormattingUpdate();
			}
		}
	}
}

/* ==========================================================================================================
	UITabButton
========================================================================================================== */
/* === UUIScreenObject interface === */
/**
 * Called when this widget is created.  Copies the style data from the owning tab control into this button's
 * string and image rendering components, then calls InitializeStyleSubscribers.  This is necessary because tab control
 * manages the styles for tab buttons - initialization of the style data is handled by the tab control for existing tab
 * buttons, but for new tab buttons being added to the tab control we need to perform this step ourselves.
 */
void UUITabButton::Created( UUIScreenObject* Creator )
{
	Super::Created(Creator);

	UUITabControl* TabControlCreator = Cast<UUITabControl>(Creator);
	if ( TabControlCreator != NULL )
	{
		// first, initialize the list of style subscribers so that SetWidgetStyle finds the correct resolver
		InitializeStyleSubscribers();

		// next, copy the style data from the tab control into the string and image rendering components.
		if ( BackgroundImageComponent != NULL )
		{
			// the style reference may be NULL if it has never been resolved - this might happen when instancing a
			// UIPrefab which contains a tab control and several existing buttons.
			UUIStyle* BackgroundStyle = TabControlCreator->TabButtonBackgroundStyle.GetResolvedStyle();
			if ( BackgroundStyle != NULL )
			{
				FStyleReferenceId BackgroundStyleId(TEXT("TabButtonBackgroundStyle"), FindFieldWithFlag<UProperty, CLASS_IsAUProperty>(BackgroundImageComponent->GetClass(), TEXT("ImageStyle")));
				SetWidgetStyle(BackgroundStyle, BackgroundStyleId);
			}
		}

		// now the caption rendering component.
		if ( StringRenderComponent != NULL )
		{
			UUIStyle* CaptionStyle = TabControlCreator->TabButtonCaptionStyle.GetResolvedStyle();
			if ( CaptionStyle != NULL )
			{
				FStyleReferenceId CaptionStyleId(TEXT("TabButtonCaptionStyle"), FindFieldWithFlag<UProperty, CLASS_IsAUProperty>(StringRenderComponent->GetClass(), TEXT("StringStyle")));
				SetWidgetStyle(CaptionStyle, CaptionStyleId);
			}
		}
	}
}

/**
 * Determines whether this page can be activated.  Calls the IsActivationAllowed delegate to provide other objects
 * a chance to veto the activation of this button.
 *
 * Child classes which override this method should call Super::CanActivateButton() FIRST and only check additional
 * conditions if the return value is true.
 *
 * @param	PlayerIndex	the index [into the Engine.GamePlayers array] for the player that wishes to activate this page.
 *
 * @return	TRUE if this button is allowed to become the active tab button.
 */
UBOOL UUITabButton::CanActivateButton( INT PlayerIndex )
{
	UBOOL bResult = FALSE;

	if ( GIsGame && IsEnabled(PlayerIndex) && TabPage != NULL )
	{
		if ( DELEGATE_IS_SET(IsActivationAllowed) )
		{
			bResult = delegateIsActivationAllowed(this, PlayerIndex);
		}
		else
		{
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Returns TRUE if this widget has a UIState_TargetedTab object in its StateStack
 *
 * @param	StateIndex	if specified, will be set to the index of the last state in the list of active states that
 *						has the class specified
 */
UBOOL UUITabButton::IsTargeted( INT PlayerIndex/*=0*/, INT* StateIndex/*=NULL*/ ) const
{
	return HasActiveStateOfClass(UUIState_TargetedTab::StaticClass(),PlayerIndex,StateIndex);
}
void UUITabButton::execIsTargeted( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT_OPTX(PlayerIndex,GetBestPlayerIndex());
	P_GET_INT_OPTX_REF(StateIndex,0);
	P_FINISH;
	*(UBOOL*)Result=IsTargeted(PlayerIndex,pStateIndex);
}

/* ==========================================================================================================
	UITabPage
========================================================================================================== */
/**
 * Returns the tab control that contains this tab page, or NULL if it's not part of a tab control.
 */
UUITabControl* UUITabPage::GetOwnerTabControl() const
{
	UUITabControl* Result=NULL;

	// TabPage's Owner should be the TabButton.  TabButton's Owner should be the TabControl, so we should be able
	// to find it simply by iterating up the owner chain
	for ( UUIObject* NextOwner = GetOwner(); NextOwner && Result == NULL; NextOwner = NextOwner->GetOwner() )
	{
		Result = Cast<UUITabControl>(NextOwner);
	}
	if ( Result == NULL )
	{
		// but for some reason that didn't work - try the same thing but start with our tabButton (in case the TabButton isn't
		// our Owner for some reason)
		if ( TabButton != NULL && TabButton != GetOwner() )
		{
			for ( UUIObject* NextOwner = TabButton->GetOwner(); NextOwner && Result == NULL; NextOwner = NextOwner->GetOwner() )
			{
				Result = Cast<UUITabControl>(NextOwner);
			}
		}

		if ( Result == NULL && GetOuter() != GetOwner() )
		{
			// OK, last ditch effort.  a widget's Owner is always the Outer as well, so now we iterate up the Outer chain
			for ( UUIObject* NextOwner = Cast<UUIObject>(GetOuter()); NextOwner && Result == NULL; NextOwner = Cast<UUIObject>(NextOwner->GetOuter()) )
			{
				Result = Cast<UUITabControl>(NextOwner);
			}
		}
	}
	return Result;
}

/* === UIScreenObject interface === */
/**
 * Called when this widget is created.
 */
void UUITabPage::Created( UUIScreenObject* Creator )
{
	UUITabButton* TabButtonCreator = Cast<UUITabButton>(Creator);
	if ( TabButtonCreator != NULL )
	{
		//@fixme ronp - verify that we don't have a TabButton at this point!  we shouldn't, since it's transient now
		TabButton = TabButtonCreator;
	}

	Super::Created(Creator);
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
void UUITabPage::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	UUITabButton* TabButtonOwner = Cast<UUITabButton>(inOwner);
	if ( TabButtonOwner != NULL )
	{
		//@fixme ronp - verify that we don't have a TabButton at this point!  we shouldn't, since it's transient now
		TabButton = TabButtonOwner;
	}

	// Since UITabPage used to be a child of UIScrollFrame, we need to clear the NotifyPositionChanged delegate for
	// any children that still have that delegate pointing to OnChildRepositioned.  OnChildRepositioned is a method in
	// UIScrollFrame that doesn't exist in UIPanel
	TArray<UUIObject*> TabPageChildren;
	GetChildren(TabPageChildren, TRUE);

	for ( INT ChildIndex = 0; ChildIndex < TabPageChildren.Num(); ChildIndex++ )
	{
		UUIObject* Child = TabPageChildren(ChildIndex);
		if ( OBJ_DELEGATE_IS_SET(Child,NotifyPositionChanged) )
		{
			FScriptDelegate& Delegate = Child->__NotifyPositionChanged__Delegate;
			if ( Delegate.FunctionName == TEXT("OnChildRepositioned") && Delegate.Object == this )
			{
				Delegate.FunctionName = NAME_None;
				Delegate.Object = NULL;
			}
		}
	}

	Super::Initialize(inOwnerScene, inOwner);
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
void UUITabPage::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=INDEX_NONE*/ )
{
	switch( BindingIndex )
	{
	case UCONST_TOOLTIP_BINDING_INDEX:
		if ( TabButton != NULL )
		{
			TabButton->SetDataStoreBinding(MarkupText, BindingIndex);
		}
		break;

	case UCONST_DESCRIPTION_DATABINDING_INDEX:
		if ( PageDescription.MarkupString != MarkupText )
		{
			Modify();
			PageDescription.MarkupString = MarkupText;
		}
		break;

	case UCONST_CAPTION_DATABINDING_INDEX:
	case INDEX_NONE:
		if ( TabButton != NULL )
		{
			TabButton->SetDataStoreBinding(MarkupText, INDEX_NONE);
		}

		if ( ButtonCaption.MarkupString != MarkupText )
		{
			Modify();
			ButtonCaption.MarkupString = MarkupText;
		}
	    break;

	default:
		if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
		{
			SetDefaultDataBinding(MarkupText, BindingIndex);
		}
	}

	RefreshSubscriberValue(BindingIndex);
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
FString UUITabPage::GetDataStoreBinding(INT BindingIndex/*=INDEX_NONE*/) const
{
	FString Result;

	switch( BindingIndex )
	{
	case UCONST_DESCRIPTION_DATABINDING_INDEX:
		Result = PageDescription.MarkupString;
	    break;

	case UCONST_CAPTION_DATABINDING_INDEX:
	case INDEX_NONE:
		Result = ButtonCaption.MarkupString;
	    break;

	default:
		if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
		{
			Result = GetDefaultDataBinding(BindingIndex);
		}
		break;
	}

	return Result;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUITabPage::RefreshSubscriberValue(INT BindingIndex/*=INDEX_NONE*/)
{
	UBOOL bResult = FALSE;

	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( TabButton != NULL )
	{
		TabButton->SetDataStoreBinding(ButtonCaption.MarkupString, INDEX_NONE);
	}
	
	// nothing to do yet...

	return TRUE;//bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUITabPage::GetBoundDataStores(TArray<UUIDataStore*>& out_BoundDataStores)
{
	GetDefaultDataStores(out_BoundDataStores);
	if ( ButtonCaption )
	{
		out_BoundDataStores.AddUniqueItem(*ButtonCaption);
	}
	if ( ButtonToolTip )
	{
		out_BoundDataStores.AddUniqueItem(*ButtonToolTip);
	}
	if ( PageDescription )
	{
		out_BoundDataStores.AddUniqueItem(*PageDescription);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUITabPage::ClearBoundDataStores()
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
	UUIScrollFrame
========================================================================================================== */
/* === UUIScrollFrame interface === */
/**
 * Changes the background image for this slider, creating the wrapper UITexture if necessary.
 *
 * @param	NewBarImage		the new surface to use for the slider's background image
 */
void UUIScrollFrame::SetBackgroundImage( USurface* NewBackgroundImage )
{
	if ( StaticBackgroundImage != NULL )
	{
		StaticBackgroundImage->SetImage(NewBackgroundImage);
	}
}

/**
 * Sets the flag indicating that the scrollbars need to be re-resolved.
 */
void UUIScrollFrame::RefreshScrollbars( UBOOL bImmediately/*=FALSE*/ )
{
	bRefreshScrollbars = TRUE;
	if ( bImmediately && GetNumResolvedFaces() != 0 )
	{
		ResolveScrollbars();
	}
}

/**
 * Sets the flag indicating that the client region needs to be recalculated and triggers a scene update.
 *
 * @param	bImmediately	specify TRUE to immediately call CalculateClientRegion instead of triggering a scene update.
 */
void UUIScrollFrame::ReapplyFormatting( UBOOL bImmediately/*=FALSE*/ )
{
	bRecalculateClientRegion = TRUE;
	if ( bImmediately )
	{
		CalculateClientRegion();
	}
	else
	{
		RequestSceneUpdate(FALSE, TRUE);
	}
}

/**
 * Determines the size of the region necessary to contain all children of this widget, and resizes this
 * UIScrollClientPanel accordingly.
 *
 * @return	the size of the client region (in pixels) after calculation.
 */
void UUIScrollFrame::CalculateClientRegion( FVector2D* RegionSize/*=NULL*/ )
{
	FLOAT FrameX, FrameX2, FrameY, FrameY2;
	GetPositionExtents(FrameX, FrameX2, FrameY, FrameY2, TRUE);

	const FLOAT ClientRegionOffsetX = HorizontalClientRegion.GetValue(this) * ClientRegionPosition.X;
	const FLOAT ClientRegionOffsetY = VerticalClientRegion.GetValue(this) * ClientRegionPosition.Y;

	FLOAT MinX=FrameX, MaxX=FrameX2, MinY=FrameY, MaxY=FrameY2;
	FLOAT SizeX = 0.f, SizeY = 0.f;

	// we don't want to count the scrollbars.
	TArray<UUIObject*> ExclusionSet;
	ExclusionSet.AddItem(ScrollbarHorizontal);
	ExclusionSet.AddItem(ScrollbarVertical);

	// since children don't necessarily have to be inside the bounds of their parents, we'll need to consider all children inside this container
	TArray<UUIObject*> AllChildren;
	GetChildren(AllChildren, TRUE, &ExclusionSet);

	if ( AllChildren.Num() > 0 )
	{
		for ( INT ChildIndex = 0; ChildIndex < AllChildren.Num(); ChildIndex++ )
		{
			UUIObject* Child = AllChildren(ChildIndex);

			// find the extrema bounds across all children of this 
			FLOAT ChildMinX, ChildMaxX, ChildMinY, ChildMaxY;
			Child->GetPositionExtents(ChildMinX, ChildMaxX, ChildMinY, ChildMaxY, TRUE);

			MinX = Min(MinX, ChildMinX);
			MaxX = Max(MaxX, ChildMaxX);
			MinY = Min(MinY, ChildMinY);
			MaxY = Max(MaxY, ChildMaxY);
		}

		SizeX = MaxX - MinX;
		SizeY = MaxY - MinY;
	}

	// update the size of the client region.
	HorizontalClientRegion.SetValue(this, SizeX);
	VerticalClientRegion.SetValue(this, SizeY);

	// though the client region doesn't move, the value of ClientRegionPosition has probably changed
	// since it is the ratio of the size of the region to the offset from the frame
	ClientRegionPosition.X = SizeX != 0 ? ClientRegionOffsetX / SizeX : 0.f;
	ClientRegionPosition.Y = SizeY != 0 ? ClientRegionOffsetY / SizeY : 0.f;

	// Reset the scrollbars.
	RefreshScrollbars();

	// Resolved...
 	bRecalculateClientRegion = FALSE;
	if ( RegionSize != NULL )
	{
		RegionSize->X = SizeX;
		RegionSize->Y = SizeY;
	}
}

/**
 * Enables and initializes the scrollbars that need to be visible
 */
void UUIScrollFrame::ResolveScrollbars()
{
	const FLOAT FrameX = GetPosition(UIFACE_Left, EVALPOS_PixelViewport);
	const FLOAT FrameY = GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
	const FLOAT ClientX = GetClientRegionPosition(UIORIENT_Horizontal);
	const FLOAT ClientY = GetClientRegionPosition(UIORIENT_Vertical);
	const FLOAT ClientExtentX = HorizontalClientRegion.GetValue(this);
	const FLOAT ClientExtentY = VerticalClientRegion.GetValue(this);

	const UBOOL bHorzVisible = ClientExtentX > GetBounds(UIORIENT_Horizontal, EVALPOS_PixelViewport);
	const UBOOL bVertVisible = ClientExtentY > GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport);

	ScrollbarHorizontal->eventSetVisibility(bHorzVisible);
	ScrollbarVertical->eventSetVisibility(bVertVisible);

	ScrollbarHorizontal->EnableCornerPadding(bVertVisible);
	ScrollbarVertical->EnableCornerPadding(bHorzVisible);

	const FLOAT HorizontalBarWidth	= bHorzVisible ? ScrollbarHorizontal->GetScrollZoneWidth() : 0.f;
	const FLOAT VerticalBarWidth	= bVertVisible ? ScrollbarVertical->GetScrollZoneWidth() : 0.f;

	const FLOAT FrameExtentX = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelViewport) - HorizontalBarWidth;
	const FLOAT FrameExtentY = GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport) - VerticalBarWidth;

	const FLOAT MaxHiddenRegionX = ClientExtentX - FrameExtentX;
	ScrollbarHorizontal->SetMarkerSize( ClientExtentX != 0 ? FrameExtentX / ClientExtentX : 0.f );
	ScrollbarHorizontal->SetMarkerPosition( ClientExtentX != 0 ? (FrameX - ClientX) / MaxHiddenRegionX : 0.f );

	if ( bHorzVisible )
	{
		// now set the NudgeMultiplier such that the number of clicks it takes to go from one side to the other is proportional
		// to the size of the "hidden" region
		const FLOAT VisibilityRatio = MaxHiddenRegionX / FrameExtentX;
		ScrollbarHorizontal->NudgeMultiplier = Clamp(MaxHiddenRegionX * VisibilityRatio, 5.f, 50.f);
	}
	
	const FLOAT MaxHiddenRegionY = ClientExtentY - FrameExtentY;
	ScrollbarVertical->SetMarkerSize( ClientExtentY != 0 ? FrameExtentY / ClientExtentY : 0.f );
	ScrollbarVertical->SetMarkerPosition( ClientExtentY != 0 ? (FrameY - ClientY) / MaxHiddenRegionY : 0.f );
	if ( bHorzVisible )
	{
		// now set the NudgeMultiplier such that the number of clicks it takes to go from one side to the other is proportional
		// to the size of the "hidden" region
		const FLOAT VisibilityRatio = MaxHiddenRegionY / FrameExtentY;

		ScrollbarVertical->NudgeMultiplier = Clamp(MaxHiddenRegionY * VisibilityRatio, 5.f, 50.f);
	}

	bRefreshScrollbars = FALSE;
}

/**
 * Scrolls all of the child widgets by the specified amount in the specified direction.
 *
 * @param	Sender			the scrollbar that generated the event.
 * @param	PositionChange	indicates the amount that the scrollbar has travelled.
 * @param	bPositionMaxed	indicates that the scrollbar's marker has reached its farthest available position,
 *                          used to achieve pixel exact scrolling
 */
UBOOL UUIScrollFrame::ScrollRegion( UUIScrollbar* Sender, FLOAT PositionChange, UBOOL bPositionMaxed/*=FALSE*/ )
{
	UBOOL bResult = FALSE;
	
	if ( Sender != NULL )
	{
		if ( Sender == ScrollbarHorizontal )
		{
			if ( !bPositionMaxed )
			{
				const FLOAT PixelChange = PositionChange * ScrollbarHorizontal->GetNudgeValue();
				const FLOAT ScrollRegionExtent = ScrollbarHorizontal->GetScrollZoneExtent();
				const FLOAT PercentChange = ScrollRegionExtent != 0.f ? PixelChange / ScrollRegionExtent : 0.f;

				ClientRegionPosition.X = Clamp(ClientRegionPosition.X + PercentChange, 0.f, 1.f);
			}
			else if ( PositionChange < 0 )
			{
				ClientRegionPosition.X = 0.f;
			}
			else
			{
				const FLOAT FrameExtentX = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelViewport) - (ScrollbarVertical->IsVisible() ? ScrollbarVertical->GetScrollZoneWidth() : 0.f);
				const FLOAT ClientRegionExtent = HorizontalClientRegion.GetValue(this);
				const FLOAT MaxHiddenPixels = ClientRegionExtent - FrameExtentX;

				ClientRegionPosition.X = ClientRegionExtent != 0 ? MaxHiddenPixels / ClientRegionExtent : 0.f;
			}
			bResult = TRUE;
		}
		else if ( Sender == ScrollbarVertical )
		{
			if ( !bPositionMaxed )
			{
				const FLOAT PixelChange = PositionChange * ScrollbarVertical->GetNudgeValue();
				const FLOAT ScrollRegionExtent = ScrollbarVertical->GetScrollZoneExtent();
				const FLOAT PercentChange = ScrollRegionExtent != 0.f ? PixelChange / ScrollRegionExtent : 0.f;
				ClientRegionPosition.Y = Clamp(ClientRegionPosition.Y + PercentChange, 0.f, 1.f);
			}
			else if ( PositionChange < 0 )
			{
				ClientRegionPosition.Y = 0.f;
			}
			else
			{
				const FLOAT FrameExtentY = GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport) - (ScrollbarHorizontal->IsVisible() ? ScrollbarHorizontal->GetScrollZoneWidth() : 0.f);
				const FLOAT ClientRegionExtent = VerticalClientRegion.GetValue(this);
				const FLOAT MaxHiddenPixels = ClientRegionExtent - FrameExtentY;

				ClientRegionPosition.Y = ClientRegionExtent != 0 ? MaxHiddenPixels / ClientRegionExtent : 0.f;
			}

			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Changes the position of the client region and synchronizes the scrollbars to the new position.
 *
 * @param	Orientation		specify UIORIENT_Horizontal to set the position of the left side; specify
 *							UIORIENT_Vertical to set the position of the top side.
 * @param	NewPosition		the position to move the client region to, in pixels.
 *
 * @return	TRUE if the client region was moved successfully.
 */
UBOOL UUIScrollFrame::SetClientRegionPosition( /*EUIOrientation*/BYTE Orientation, FLOAT NewPosition )
{
	UBOOL bResult = FALSE;

	if ( Orientation < UIORIENT_MAX )
	{
		FLOAT MinX=0.f, MinY=0.f, MaxX=0.f, MaxY=0.f;
		GetClipRegion(MinX, MinY, MaxX, MaxY);

		const FVector2D VisibleRegionSize(MaxX - MinX, MaxY - MinY);
		const FLOAT ClientRegionSize = GetClientRegionSize(Orientation);

		NewPosition = Clamp(NewPosition, VisibleRegionSize[Orientation] - ClientRegionSize, 0.f) / -ClientRegionSize;
		if ( Abs(NewPosition - ClientRegionPosition[Orientation]) > DELTA )
		{
			ClientRegionPosition[Orientation] = NewPosition;
			RefreshScrollbars(TRUE);
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Changes the position of the client region and synchronizes the scrollbars to the new position.
 *
 * @param	NewPosition		the position to move the client region to, in pixels.
 *
 * @return	TRUE if the client region was moved successfully.
 */
UBOOL UUIScrollFrame::SetClientRegionPositionVector( FVector2D NewPosition )
{
	UBOOL bResult = FALSE;

	FLOAT MinX=0.f, MinY=0.f, MaxX=0.f, MaxY=0.f;
	GetClipRegion(MinX, MinY, MaxX, MaxY);

	const FVector2D VisibleRegionSize(MaxX - MinX, MaxY - MinY);
	const FVector2D ClientRegionSize(GetClientRegionSizeVector());
	const FVector2D MinValue(VisibleRegionSize - ClientRegionSize);

	// normalize the new position; if the client region is smaller than the visible region for either orientation, 
	// negate the value for that orientation
	if ( ClientRegionSize.X <= VisibleRegionSize.X )
	{
		NewPosition.X = 0.f;
	}
	if ( ClientRegionSize.Y <= VisibleRegionSize.Y )
	{
		NewPosition.Y = 0.f;
	}

	NewPosition.X = Clamp(NewPosition.X, MinValue.X, 0.f) / -ClientRegionSize.X;
	NewPosition.Y = Clamp(NewPosition.Y, MinValue.Y, 0.f) / -ClientRegionSize.Y;
	if ( !ClientRegionPosition.Equals(NewPosition, DELTA) )
	{
		ClientRegionPosition = NewPosition;
		RefreshScrollbars(TRUE);
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Gets the position of either the left or top side of the client region.
 *
 * @param	Orientation		specify UIORIENT_Horizontal to retrieve the position of the left side; specify
 *							UIORIENT_Vertical to retrieve the position of the top side.
 *
 * @return	the position of the client region, in canvas coordinates relative to 0,0.
 */
FLOAT UUIScrollFrame::GetClientRegionPosition( /*EUIOrientation*/BYTE Orientation ) const
{
	FLOAT Result = 0.f;
	if ( Orientation < UIORIENT_MAX )
	{
		if ( Orientation == UIORIENT_Horizontal )
		{
			const FLOAT FrameX = GetPosition(UIFACE_Left, EVALPOS_PixelViewport);
			const FLOAT OffsetX = HorizontalClientRegion.GetValue(this) * ClientRegionPosition.X;
			Result = FrameX - OffsetX;
		}
		else if ( Orientation == UIORIENT_Vertical )
		{
			const FLOAT FrameY = GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
			const FLOAT OffsetY = VerticalClientRegion.GetValue(this) * ClientRegionPosition.Y;
			Result = FrameY - OffsetY;
		}
	}

	return Result;
}

/**
 * Returns the size of a single orientation of the client region, in pixels.
 *
 * @param	Orientation		specify UIORIENT_Horizontal to retrieve the width of the client region or UIORIENT_Vertical
 *							to get the height of the client region.
 *
 * @return	the width or height of the client region, in pixels.
 */
FLOAT UUIScrollFrame::GetClientRegionSize( /*EUIOrientation*/BYTE Orientation ) const
{
	FLOAT Result=0.f;
	if ( Orientation < UIORIENT_MAX )
	{
		if ( Orientation == UIORIENT_Horizontal )
		{
			Result = HorizontalClientRegion.GetValue(this);
		}
		else if ( Orientation == UIORIENT_Vertical )
		{
			Result = VerticalClientRegion.GetValue(this);
		}
	}
	return Result;
}

/**
 * Gets the position of the upper-left corner of the client region.
 *
 * @return	the position of the client region, in canvas coordinates relative to 0,0.
 */
FVector2D UUIScrollFrame::GetClientRegionPositionVector() const
{
	const FLOAT FrameX = GetPosition(UIFACE_Left, EVALPOS_PixelViewport);
	const FLOAT FrameY = GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
	const FLOAT OffsetX = HorizontalClientRegion.GetValue(this) * ClientRegionPosition.X;
	const FLOAT OffsetY = VerticalClientRegion.GetValue(this) * ClientRegionPosition.Y;
	return FVector2D(
		FrameX - OffsetX,
		FrameY - OffsetY
		);
}

/**
 * Returns the size of the client region, in pixels.
 */
FVector2D UUIScrollFrame::GetClientRegionSizeVector() const
{
	return FVector2D(HorizontalClientRegion.GetValue(this), VerticalClientRegion.GetValue(this));
}

/**
 * Returns a vector containing the size of the region (in pixels) available for rendering inside this scrollframe,
 * taking account whether the scrollbars are visible.
 */
void UUIScrollFrame::GetClipRegion( FLOAT& MinX, FLOAT& MinY, FLOAT& MaxX, FLOAT& MaxY ) const
{
	GetPositionExtents(MinX, MaxX, MinY, MaxY, FALSE);
	if ( ScrollbarVertical != NULL && ScrollbarVertical->IsVisible() )
	{
		MaxX -= ScrollbarVertical->GetScrollZoneWidth();
	}

	if ( ScrollbarHorizontal != NULL && ScrollbarHorizontal->IsVisible() )
	{
		MaxY -= ScrollbarHorizontal->GetScrollZoneWidth();
	}
}

/**
 * Returns the percentage of the client region that is visible within the scrollframe's bounds.
 *
 * @param	Orientation		specifies whether to return the vertical or horizontal percentage.
 *
 * @return	a value from 0.0 to 1.0 representing the percentage of the client region that can be visible at once.
 */
FLOAT UUIScrollFrame::GetVisibleRegionPercentage( /*EUIOrientation*/BYTE Orientation ) const
{
	FLOAT Result=0.f;
	if ( Orientation < UIORIENT_MAX )
	{
		const FLOAT FrameExtentX = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelViewport);
		const FLOAT FrameExtentY = GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport);
		const FLOAT ClientExtentX = HorizontalClientRegion.GetValue(this);
		const FLOAT ClientExtentY = VerticalClientRegion.GetValue(this);
		const FLOAT HorizontalBarWidth	= ClientExtentX > FrameExtentX ? ScrollbarHorizontal->GetScrollZoneWidth() : 0.f;
		const FLOAT VerticalBarWidth	= ClientExtentY > FrameExtentY ? ScrollbarVertical->GetScrollZoneWidth() : 0.f;

		if ( Orientation == UIORIENT_Horizontal )
		{
			if ( ClientExtentX != 0 )
			{
				Result = (FrameExtentX - VerticalBarWidth) / ClientExtentX;
			}
		}
		else if ( Orientation == UIORIENT_Vertical )
		{
			if ( ClientExtentY != 0 )
			{
				Result = (FrameExtentY - HorizontalBarWidth) / ClientExtentY;
			}
		}
	}
	return Result;
}

/* === UIObject interface === */
/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
 */
void UUIScrollFrame::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	VALIDATE_COMPONENT(StaticBackgroundImage);
	AddStyleSubscriber(StaticBackgroundImage);
}

/**
 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUIScrollFrame::ResolveFacePosition( EUIWidgetFace Face )
{
	if ( bRecalculateClientRegion && GetNumResolvedFaces() == 0 )
	{
		CalculateClientRegion();
	}

	Super::ResolveFacePosition(Face);

	if ( GetNumResolvedFaces() == UIFACE_MAX && bRefreshScrollbars )
	{
 		ResolveScrollbars();
	}
}

/* === UUIScreenObject interface === */
/**
 * Initializes the buttons and creates the background image.
 *
 * @param	inOwnerScene	the scene to add this widget to.
 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
 *							is being added to the scene's list of children.
 */
void UUIScrollFrame::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner/*=NULL*/ )
{
	Super::Initialize( inOwnerScene, inOwner );

	TArray<UUIScrollbar*> ScrollbarChildren;
	if ( ContainsObjectOfClass<UUIObject>(Children, UUIScrollbar::StaticClass(), FALSE, (TArray<UUIObject*>*)&ScrollbarChildren) )
	{
		for ( INT ScrollbarIndex = 0; ScrollbarIndex < ScrollbarChildren.Num(); ScrollbarIndex++ )
		{
			UUIScrollbar* Scrollbar = ScrollbarChildren(ScrollbarIndex);
			if ( Scrollbar->GetOuter() == this )
			{
				if ( Scrollbar->ScrollbarOrientation == UIORIENT_Horizontal )
				{
					ScrollbarHorizontal = Scrollbar;
				}

				else if ( Scrollbar->ScrollbarOrientation == UIORIENT_Vertical )
				{
					ScrollbarVertical = Scrollbar;
				}
			}
		}
	}
	
	UUIScrollFrame* ScrollFrameArch = GetArchetype<UUIScrollFrame>();
	UBOOL bCreateScrollbarVert = ScrollbarVertical == NULL || ScrollbarVertical->GetOuter() != this;
	if ( !bCreateScrollbarVert && ScrollbarVertical->GetArchetype() != ScrollFrameArch->ScrollbarVertical )
	{
		// remove the old scrollbar
		RemoveChild(ScrollbarVertical);
		// move it into the transient package so that we have no conflicts
		ScrollbarVertical->Rename(NULL, GetTransientPackage(), REN_ForceNoResetLoaders);
		ScrollbarVertical = NULL;
		// create a new one
		bCreateScrollbarVert = TRUE;
	}
	UBOOL bCreateScrollbarHorz = ScrollbarHorizontal == NULL || ScrollbarHorizontal->GetOuter() != this;
	if ( !bCreateScrollbarHorz && ScrollbarHorizontal->GetArchetype() != ScrollFrameArch->ScrollbarHorizontal )
	{
		// remove the old scrollbar
		RemoveChild(ScrollbarHorizontal);
		// move it into the transient package so that we have no conflicts
		ScrollbarHorizontal->Rename(NULL, GetTransientPackage(), REN_ForceNoResetLoaders);
		ScrollbarHorizontal = NULL;
		// create a new one
		bCreateScrollbarHorz = TRUE;
	}

	{
		// create internal controls
		if ( bCreateScrollbarVert )
		{
			ScrollbarVertical = Cast<UUIScrollbar>(CreateWidget(this, UUIScrollbar::StaticClass(), ScrollFrameArch->ScrollbarVertical));
			InsertChild( ScrollbarVertical );
		}
		if ( bCreateScrollbarHorz )
		{
			ScrollbarHorizontal = Cast<UUIScrollbar>(CreateWidget(this, UUIScrollbar::StaticClass(), ScrollFrameArch->ScrollbarHorizontal));
			InsertChild( ScrollbarHorizontal );
		}
	}
}

#if 0
// only need to enable these two methods if we're going to allow the scrollframe itself to become the focused
// control (similar to UITabControl), so that the user can scroll the client region using the arrow keys for example)

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUIScrollFrame::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{

}

/**
 * Handles input events for this scrollframe widget.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUIScrollFrame::ProcessInputKey( const struct FSubscribedInputEventParameters& EventParms )
{
	return Super::ProcessInputKey( EventParms );
}
#endif

/**
 * Render this scroll frame.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUIScrollFrame::Render_Widget( FCanvas* Canvas )
{
	if ( StaticBackgroundImage != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
		);

		StaticBackgroundImage->RenderComponent( Canvas, Parameters );
	}
}
/**
 * Routes rendering calls to children of this screen object.
 *
 * This version sets a clip mask on the canvas while the children are being rendered.
 *
 * @param	Canvas	the canvas to use for rendering
 */
void UUIScrollFrame::Render_Children( FCanvas* Canvas )
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

	GetClipRegion(FrameBounds[UIFACE_Left], FrameBounds[UIFACE_Top], FrameBounds[UIFACE_Right], FrameBounds[UIFACE_Bottom]);

	const FVector2D ClientRegionPos(GetClientRegionPositionVector());
	FTranslationMatrix ClientRegionTransform(FVector(ClientRegionPos,0));
	for ( INT i = 0; i < RenderList.Num(); i++ )
	{
		UUIObject* Child = RenderList(i);

		// apply the widget's rotation
		if ( Child->IsVisible() )
		{
			UBOOL bRenderChild=FALSE;
			if ( Child != ScrollbarHorizontal && Child != ScrollbarVertical )
			{
				// get the extents for this child widget
				FLOAT MinX=0.f, MaxX=0.f, MinY=0.f, MaxY=0.f;
				Child->GetPositionExtents(MinX, MaxX, MinY, MaxY, TRUE);

				// don't render this child if it's completely outside this panel's visible area
				if (MinX - DELTA <= FrameBounds[UIFACE_Right] + DELTA && MinY - DELTA <= FrameBounds[UIFACE_Bottom] + DELTA
				&&	MaxX + DELTA >= FrameBounds[UIFACE_Left] - DELTA && MaxY + DELTA >= FrameBounds[UIFACE_Top] - DELTA )
				{
					// set the clip mask on the canvas
					//@todo ronp/samz
					//Canvas->SetCanvasMask(FrameBounds);

					// apply the widget's transform matrix combined with the scrollregion offset translation
					Canvas->PushRelativeTransform(ClientRegionTransform * Child->GenerateTransformMatrix(FALSE));
					bRenderChild = TRUE;
				}
			}
			else
			{
				// apply the widget's transform matrix 
				Canvas->PushRelativeTransform(Child->GenerateTransformMatrix(FALSE));
				bRenderChild = TRUE;
			}

			if ( !bRenderChild )
			{
				continue;
			}


			// use the widget's ZDepth as the sorting key for the canvas
			Canvas->PushDepthSortKey(appCeil(Child->ZDepth));

			// add this widget to the scene's render stack
			OwnerScene->RenderStack.Push(Child);

			// now render the child
			Render_Child(Canvas, Child);

			// restore the previous sort key
			Canvas->PopDepthSortKey();

			// restore the previous transform
			Canvas->PopTransform();

			// clear the clip mask
			//@todo ronp/samz
			//Canvas->ClearCanvasMask();
		}
	}

	// restore the previous global fade value
	Canvas->AlphaModulate = CurrentAlphaModulation;
}

/**
 * Insert a widget as a child of this one.  This version routes the call to the ClientPanel if the widget is not
 * eligible to be a child of this scroll frame.
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
INT UUIScrollFrame::InsertChild( UUIObject* NewChild, INT InsertIndex/*=INDEX_NONE*/, UBOOL bRenameExisting/*=TRUE*/ )
{
	if ( NewChild == ScrollbarHorizontal || NewChild == ScrollbarVertical )
	{
		// Scrollbars should always be rendered last
		//@todo ronp zdepth support - might need to do additional work (such as ensuring that the scrollbars' ZDepth is always lower than any other
		// children in the scrollframe)
		InsertIndex = INDEX_NONE;
	}
	else
	{
		// all other children must be inserted into the Children array before the scrollbars
		InsertIndex = Max(0, Min(InsertIndex, Min(Children.FindItemIndex(ScrollbarHorizontal), Children.FindItemIndex(ScrollbarVertical))));
	}

	return Super::InsertChild(NewChild, InsertIndex, bRenameExisting);
}

/**
 * Called immediately after a child has been added to this screen object.
 *
 * @param	WidgetOwner		the screen object that the NewChild was added as a child for
 * @param	NewChild		the widget that was added
 */
void UUIScrollFrame::NotifyAddedChild( UUIScreenObject* WidgetOwner, UUIObject* NewChild )
{
	Super::NotifyAddedChild( WidgetOwner, NewChild );

	// ignore the adding of our own children
	if ( NewChild != ScrollbarVertical && NewChild != ScrollbarHorizontal )
	{
		// Recalculate the region extent since adding a widget could have affected it.
		ReapplyFormatting();
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
void UUIScrollFrame::NotifyRemovedChild( UUIScreenObject* WidgetOwner, UUIObject* OldChild, TArray<UUIObject*>* ExclusionSet/*=NULL*/ )
{
	Super::NotifyRemovedChild( WidgetOwner, OldChild, ExclusionSet );

	if ( OldChild != ScrollbarVertical && OldChild != ScrollbarHorizontal )
	{
		// Recalculate the region extent since removing a widget could have affected it.
		ReapplyFormatting();
	}
}

/**
 * Called when a property is modified that could potentially affect the widget's position onscreen.
 */
void UUIScrollFrame::RefreshPosition()
{	
	RefreshScrollbars();

	Super::RefreshPosition();
}

/**
 * Called to globally update the formatting of all UIStrings.
 */
void UUIScrollFrame::RefreshFormatting()
{
	Super::RefreshFormatting();

	// Recalculate the region extent since the movement of the frame could have affected it.
	bRefreshScrollbars = TRUE;
}

/**
 * Called when the scene receives a notification that the viewport has been resized.  Propagated down to all children.
 *
 * @param	OldViewportSize		the previous size of the viewport
 * @param	NewViewportSize		the new size of the viewport
 */
void UUIScrollFrame::NotifyResolutionChanged( const FVector2D& OldViewportSize, const FVector2D& NewViewportSize )
{
	RefreshFormatting();
	ReapplyFormatting();
	Super::NotifyResolutionChanged(OldViewportSize, NewViewportSize);
}

/**
 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
 */
void UUIScrollFrame::PreEditChange( FEditPropertyChain& PropertyThatChanged )
{
	Super::PreEditChange(PropertyThatChanged);

	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("StaticBackgroundImage") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty && StaticBackgroundImage != NULL )
				{
					// the user either cleared the value of the BackgroundImageComponent (which should never happen since
					// we use the 'noclear' keyword on the property declaration), or is assigning a new value to the BackgroundImageComponent.
					// Unsubscribe the current component from our list of style resolvers.
					RemoveStyleSubscriber(StaticBackgroundImage);
				}
			}
		}
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIScrollFrame::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("StaticBackgroundImage") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();

				// if the value of the BackgroundImageComponent itself was changed
				if ( MemberProperty == ModifiedProperty )
				{
					if ( StaticBackgroundImage != NULL )
					{
						UUIComp_DrawImage* ComponentTemplate = GetArchetype<UUIScrollFrame>()->StaticBackgroundImage;
						if ( ComponentTemplate != NULL )
						{
							StaticBackgroundImage->StyleResolverTag = ComponentTemplate->StyleResolverTag;
						}
						else
						{
							StaticBackgroundImage->StyleResolverTag = TEXT("Background Image Style");
						}

						// user created a new background image component - add it to the list of style subscribers
						AddStyleSubscriber(StaticBackgroundImage);

						// now initialize the component's image
						StaticBackgroundImage->SetImage(StaticBackgroundImage->GetImage());
					}
				}
				else if ( StaticBackgroundImage != NULL )
				{
					// a property of the ImageComponent was changed
					if ( ModifiedProperty->GetFName() == TEXT("ImageRef") && StaticBackgroundImage->GetImage() != NULL )
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
 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated Background,
 * BackgroundCoordinates, and PrimaryStyle properties over to the BackgroundImageComponent.
 */
void UUIScrollFrame::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerVersion() < VER_FIXED_UISCROLLBARS )
	{
		if ( GIsEditor )
		{
			if ( ScrollbarVertical != NULL )
			{
				RemoveChild(ScrollbarVertical);
				ScrollbarVertical = NULL;
			}

			if ( ScrollbarHorizontal != NULL )
			{
				RemoveChild(ScrollbarHorizontal);
				ScrollbarHorizontal = NULL;
			}
		}
	}
}



// EOF







