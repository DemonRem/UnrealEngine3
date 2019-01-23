/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/


#include "UnrealEd.h"
#include "UnObjectEditor.h"
#include "UnUIEditor.h"
#include "UnLinkedObjDrawUtils.h"

#include "RenderModifier_FocusChainHandles.h"

// Docking Constants
namespace FFocusChainConstants
{
	static const INT HandleRadius = 8;
	static const INT HandleGap = 8;
	static const INT HandleNumSides = 16;
};


/**
 * Constructor
 *
 * @param	InEditor	the editor window that holds the container to render the outline for
 */
FRenderModifier_FocusChainHandles::FRenderModifier_FocusChainHandles( class WxUIEditorBase* InEditor ) : 
FRenderModifier_SelectedWidgetsOutline(InEditor)
{

}



/** Sets the selected dock face for highlighting. */
void FRenderModifier_FocusChainHandles::SetSelectedDockFace(EUIWidgetFace InFace)
{
	// Only update if we are changing state.
	// If we changed to a blank face, then clear the tooltip. Otherwise, update the tooltip with some information
	// about the currently highlighted dock handle.
	if(SelectedDockHandle != InFace)
	{
		if(InFace != UIFACE_MAX && SelectedWidget != NULL)
		{
			FString DockString;
			FString TempString;
			TArray<FString> TempStringArray;

			UIEditor->GetDockHandleString(SelectedWidget, InFace,DockString);
			TempString = FString::Printf(*LocalizeUI(TEXT("UIEditor_Source")), *DockString);
			TempStringArray.AddItem(TempString);

			const UUIObject* TargetWidget = SelectedWidget->GetNavigationTarget(InFace);
			if(TargetWidget == NULL)
			{
				TempString = FString::Printf(*LocalizeUI(TEXT("UIEditor_Target")), *LocalizeUI(TEXT("UIEditor_Face_None")));
			}
			else
			{
				TempString = FString::Printf(*LocalizeUI(TEXT("UIEditor_Target")), *TargetWidget->GetName());
			}
			
		
			TempStringArray.AddItem(TempString);

			UIEditor->TargetTooltip->SetStringList(TempStringArray);
		}
		else
		{
			UIEditor->TargetTooltip->ClearStringList();
		}

		SelectedDockHandle = InFace;
	}
}

/**
 * Renders the outline for the top-level object of a UI editor window.
 */
void FRenderModifier_FocusChainHandles::Render( FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas )
{
	// Loop through all selected widgets and render their focus chain handles.
	TArray<UUIObject*> SelectedWidgets;
	UIEditor->GetSelectedWidgets(SelectedWidgets);

	const INT NumWidgets = SelectedWidgets.Num();

	for(INT WidgetIdx = 0; WidgetIdx < NumWidgets; WidgetIdx++)
	{
		UUIObject* Widget = SelectedWidgets(WidgetIdx);

		if( !Widget->IsPrivateBehaviorSet( UCONST_PRIVATE_NotFocusable ) )
		{
			RenderFocusChainHandle(Widget, UIFACE_Top, ObjectVC, Canvas);
			RenderFocusChainHandle(Widget, UIFACE_Bottom, ObjectVC, Canvas);
			RenderFocusChainHandle(Widget, UIFACE_Left, ObjectVC, Canvas);
			RenderFocusChainHandle(Widget, UIFACE_Right, ObjectVC, Canvas);
		}
	}
}

/** 
* Renders a focus chain handle for the specified widget.
* 
* @param Widget	Widget to render the focus chain handle for.
* @param Face		Which face we are drawing the handle for.
* @param RI		Rendering interface to use for drawing the handle.
*/
void FRenderModifier_FocusChainHandles::RenderFocusChainHandle( UUIObject* Widget, EUIWidgetFace InFace, FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas )
{
	const FColor FocusChainHandlerColor(128,0,128);
	const FColor FocusChainHandlerHighlightColor(192,0,192);
	const FColor FocusChainHandlerBorderColor(0,0,0);
	const FLOAT Zoom = ObjectVC->Zoom2D < 1.0f ? 1.0f : ObjectVC->Zoom2D;
	const FLOAT FocusChainHandleRadius = FFocusChainConstants::HandleRadius / Zoom;
	const FLOAT FocusChainHandleGap = FFocusChainConstants::HandleGap / Zoom;

	// Get a position for the focus chain handle.
	FIntPoint HandlePosition;
	GetHandlePosition(Widget->RenderBounds, InFace, FocusChainHandleRadius, FocusChainHandleGap, HandlePosition);

	//Render the dock link for this object.
	RenderLink(Widget, InFace, Canvas, HandlePosition);

	Canvas->SetHitProxy( new HUIFocusChainHandleHitProxy(Widget, InFace) );
	{
		// Draw the dock handle circle, if the mouse is over the dock handle, then draw it in a different color.
		const UBOOL bHandleHighlighted = (SelectedWidget == Widget && SelectedDockHandle == InFace);
		FColor DockColor;

		if(bHandleHighlighted == TRUE)
		{
			DockColor = FocusChainHandlerHighlightColor;
		}
		else
		{
			DockColor = FocusChainHandlerColor;
		}

		FVector2D LogicLeft(HandlePosition.X - FocusChainHandleRadius, HandlePosition.Y);
		FVector2D LogicTop(HandlePosition.X, HandlePosition.Y - FocusChainHandleRadius);
		FVector2D LogicRight(HandlePosition.X + FocusChainHandleRadius, HandlePosition.Y);
		FVector2D LogicBottom(HandlePosition.X, HandlePosition.Y + FocusChainHandleRadius);

		// Draw Triangle Borders
		const FLOAT BorderSize = 2 / Zoom;
		DrawTriangle2D( Canvas,
			FVector2D(LogicLeft.X - BorderSize,		LogicLeft.Y), FVector2D(0,0),
			FVector2D(LogicTop.X,					LogicTop.Y - BorderSize), FVector2D(0,0),
			FVector2D(LogicRight.X + BorderSize,	LogicRight.Y), FVector2D(0,0),
			FocusChainHandlerBorderColor );

		DrawTriangle2D( Canvas,
			FVector2D(LogicLeft.X - BorderSize,		LogicLeft.Y), FVector2D(0,0),
			FVector2D(LogicBottom.X,				LogicBottom.Y + BorderSize), FVector2D(0,0),
			FVector2D(LogicRight.X + BorderSize,	LogicRight.Y), FVector2D(0,0),
			FocusChainHandlerBorderColor );

		DrawTriangle2D( Canvas,
			FVector2D(LogicLeft.X,	LogicLeft.Y), FVector2D(0,0),
			FVector2D(LogicTop.X,	LogicTop.Y), FVector2D(0,0),
			FVector2D(LogicRight.X,	LogicRight.Y), FVector2D(0,0),
			DockColor );

		DrawTriangle2D( Canvas,
			FVector2D(LogicLeft.X,		LogicLeft.Y), FVector2D(0,0),
			FVector2D(LogicBottom.X,	LogicBottom.Y), FVector2D(0,0),
			FVector2D(LogicRight.X,		LogicRight.Y), FVector2D(0,0),
			DockColor );

		//Draw a X on top of the handle if the widget focus link is connected.
		const UBOOL bWidgetHandleConnected = Widget->GetNavigationTarget(InFace) != NULL;
		if( bWidgetHandleConnected == TRUE )
		{
			UTexture2D* tex = GApp->MaterialEditor_Delete;
			DrawTile(Canvas,HandlePosition.X - FocusChainHandleRadius / 2,HandlePosition.Y - FocusChainHandleRadius / 2, FocusChainHandleRadius, FocusChainHandleRadius, 0.0f,0.0f,1.0f,1.0f, FLinearColor::White, tex->Resource );
		}
	}
	Canvas->SetHitProxy( NULL );
}


/** 
 * Renders a focus chain link for the specified face.
 * 
 * @param Widget				Widget to render the docking handle for.
 * @param InFace				Which face we are drawing the handle for.
 * @param RI					Rendering interface to use for drawing the handle.
 * @param Start					Starting position for the link.
 */
void FRenderModifier_FocusChainHandles::RenderLink( UUIObject* Widget, EUIWidgetFace InFace, FCanvas* Canvas, const FVector& InStart)
{
	// Make sure that we have a focus target for this face.
	const UUIObject* TargetWidget = Widget->GetNavigationTarget(InFace);
	if(TargetWidget != NULL)
	{
		FIntPoint End;

		End.X = appTrunc((TargetWidget->RenderBounds[UIFACE_Right] + TargetWidget->RenderBounds[UIFACE_Left]) * 0.5f);
		End.Y = appTrunc((TargetWidget->RenderBounds[UIFACE_Top] + TargetWidget->RenderBounds[UIFACE_Bottom]) * 0.5f);

		// Calculate start arrow direction.
		FVector2D StartDir(0,0);

		switch(InFace)
		{
		case UIFACE_Bottom: case UIFACE_Top:
			if(InStart.Y < End.Y)
			{
				StartDir[1] = 1;
			}
			else
			{
				StartDir[1] = -1;
			}
			
			break;

		case UIFACE_Right: case UIFACE_Left:
			if(InStart.X < End.X)
			{
				StartDir[0] = 1;
			}
			else
			{
				StartDir[0] = -1;
			}
			break;

		default:
			return;
		}


		// Finally render the spline.
		const FIntPoint Start(appTrunc(InStart.X), appTrunc(InStart.Y));
		const FColor LineColor(0,255,255);
		const FLOAT Tension = (Start-End).Size();
		FLinkedObjDrawUtils::DrawSpline(Canvas, Start, Tension * StartDir, End, Tension * StartDir, LineColor, true);
	}
}


