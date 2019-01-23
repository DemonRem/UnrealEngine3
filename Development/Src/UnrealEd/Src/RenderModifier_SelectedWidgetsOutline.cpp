/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"

#include "EngineUIPrivateClasses.h"
#include "UnrealEdPrivateClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineUISequenceClasses.h"

#include "UnLinkedObjDrawUtils.h"
#include "UnObjectEditor.h"
#include "UnUIEditor.h"
#include "RenderModifier_SelectedWidgetsOutline.h"


FRenderModifier_SelectedWidgetsOutline::FRenderModifier_SelectedWidgetsOutline( class WxUIEditorBase* InEditor ) : 
FUIEditor_RenderModifier(InEditor),
SelectedWidget(NULL),
SelectedDockHandle(UIFACE_MAX)
{

}

void FRenderModifier_SelectedWidgetsOutline::Render( FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas )
{
	TArray<UUIObject*> SelectedWidgets;
	UIEditor->GetSelectedWidgets(SelectedWidgets);

	const INT NumWidgets = SelectedWidgets.Num();

	// Loop through all selected widgets and render a selection box around them,
	// and their docking handles.
	for(INT WidgetIdx = 0; WidgetIdx < NumWidgets; WidgetIdx++)
	{
		UUIObject* Widget = SelectedWidgets(WidgetIdx);

		// Render a box outline around this widget because it is selected.
		FColor BorderColor(64,64,255);

		FVector Expand(2 / ObjectVC->Zoom2D,2 / ObjectVC->Zoom2D,2 / ObjectVC->Zoom2D);
		FVector Start, End;

		FLOAT MinX = 0.0f, MaxX = 0.0f, MinY = 0.0f, MaxY = 0.0f;

		Widget->GetPositionExtents( MinX, MaxX, MinY, MaxY );

		// Top Line.
		DrawTile(Canvas,MinX - Expand.X, MinY - Expand.Y, Expand.X * 2 + MaxX - MinX, Expand.Y * 2 + MinY - MinY, 0.0f, 0.0f, 0.0f, 0.0f, BorderColor);
		// Bottom Line.
		DrawTile(Canvas,MinX - Expand.X, MaxY - Expand.Y, Expand.X * 2 + MaxX - MinX, Expand.Y * 2 + MaxY - MaxY, 0.0f, 0.0f, 0.0f, 0.0f, BorderColor);
		// Left Line.
		DrawTile(Canvas,MinX - Expand.X, MinY - Expand.Y, Expand.X * 2 + MinX - MinX, Expand.Y * 2 + MaxY - MinY, 0.0f, 0.0f, 0.0f, 0.0f, BorderColor);
		// Right Line.
		DrawTile(Canvas,MaxX - Expand.X, MinY - Expand.Y, Expand.X * 2 + MaxX - MaxX, Expand.Y * 2 + MaxY - MinY, 0.0f, 0.0f, 0.0f, 0.0f, BorderColor);


		// Draw docking handles for this object.
		if(UIEditor->EditorOptions->bShowDockHandles == TRUE && !Widget->IsPrivateBehaviorSet( UCONST_PRIVATE_NotDockable ))
		{
			RenderDockHandle(Widget, UIFACE_Top, ObjectVC, Canvas);
			RenderDockHandle(Widget, UIFACE_Bottom, ObjectVC, Canvas);
			RenderDockHandle(Widget, UIFACE_Left, ObjectVC, Canvas);
			RenderDockHandle(Widget, UIFACE_Right, ObjectVC, Canvas);
		}
	}
}

void FRenderModifier_SelectedWidgetsOutline::SetSelectedWidget(UUIObject* InWidget)
{
	SelectedWidget = InWidget;
}

/** Sets the selected dock face for highlighting. */
void FRenderModifier_SelectedWidgetsOutline::SetSelectedDockFace(EUIWidgetFace InFace)
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

			const UUIObject* TargetWidget = SelectedWidget->DockTargets.GetDockTarget(InFace);
			const EUIWidgetFace TargetFace = SelectedWidget->DockTargets.GetDockFace(InFace);

			UIEditor->GetDockHandleString(TargetWidget, TargetFace,DockString);
			TempString = FString::Printf(*LocalizeUI(TEXT("UIEditor_Target")), *DockString);

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

/** Clears the selected dock face and dock widget. */
void FRenderModifier_SelectedWidgetsOutline::ClearSelections()
{
	SelectedWidget = NULL;
	SelectedDockHandle = UIFACE_MAX;
	UIEditor->TargetTooltip->ClearStringList();
}

/**
* Method for serializing UObject references that must be kept around through GC's.
*
* @param Ar The archive to serialize with
*/
void FRenderModifier_SelectedWidgetsOutline::Serialize(FArchive& Ar)
{
	Ar << SelectedWidget;
}

/** 
* Renders a docking handle for the specified widget.
* 
* @param Widget	Widget to render the docking handle for.
* @param Face		Which face we are drawing the handle for.
* @param RI		Rendering interface to use for drawing the handle.
*/
void FRenderModifier_SelectedWidgetsOutline::RenderDockHandle( UUIObject* Widget, EUIWidgetFace InFace, FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas )
{
	const FColor DockHandlerColor(255,128,0);
	const FColor DockHandlerHighlightColor(255,255,0);
	const FColor DockHandlerBorderColor(0,0,0);
	const FLOAT Zoom = ObjectVC->Zoom2D < 1.0f ? 1.0f : ObjectVC->Zoom2D;
	const FLOAT DockHandleRadius = FDockingConstants::HandleRadius / Zoom;
	const FLOAT DockHandleGap = FDockingConstants::HandleGap / Zoom;
	const INT NumSides = FDockingConstants::HandleNumSides;

	// Get a position for this dock handle.
	FIntPoint DockPosition;
	FLOAT RenderBounds[4];
	Widget->GetPositionExtents( RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Right], RenderBounds[UIFACE_Top], RenderBounds[UIFACE_Bottom] );

	GetHandlePosition(RenderBounds, InFace, DockHandleRadius, DockHandleGap, DockPosition);
	
	//Render the dock link for this object.
	RenderDockLink(Widget, InFace, Canvas, DockPosition, DockHandleRadius, DockHandleGap);

	Canvas->SetHitProxy( new HUIDockHandleHitProxy(Widget, InFace) );
	{
		// Draw the dock handle circle, if the mouse is over the dock handle, then draw it in a different color.
		const UBOOL bHandleHighlighted = (SelectedWidget == Widget && SelectedDockHandle == InFace);

		FLinkedObjDrawUtils::DrawNGon(Canvas, DockPosition, DockHandlerBorderColor, NumSides, DockHandleRadius + (1 / Zoom));

		if(bHandleHighlighted == TRUE)
		{
			FLinkedObjDrawUtils::DrawNGon(Canvas, DockPosition, DockHandlerHighlightColor, NumSides, DockHandleRadius);
		}
		else
		{
			FLinkedObjDrawUtils::DrawNGon(Canvas, DockPosition, DockHandlerColor, NumSides, DockHandleRadius);
		}

		//Draw a X on top of the handle if the widget is docked.
		const UBOOL bWidgetDocked = (Widget->DockTargets.IsDocked(InFace));
		if( bWidgetDocked == TRUE )
		{
			UTexture2D* tex = GApp->MaterialEditor_Delete;
			DrawTile(Canvas,DockPosition.X - DockHandleRadius / 2,DockPosition.Y - DockHandleRadius / 2, DockHandleRadius, DockHandleRadius, 0.0f,0.0f,1.0f,1.0f, FLinearColor::White, tex->Resource );
		}
	}
	Canvas->SetHitProxy( NULL );
}


/** 
 * Generates a dock handle position for a object.
 * 
 * @param Widget			Widget we are getting a dock handle position for.
 * @param InFace			Which face we are getting a position for.
 * @param HandleRadius		Radius of the handle.
 * @param HandleGap			Gap between the handle and the object's edge.
 * @param OutPosition		Position of the dock handle.
 */
void FRenderModifier_SelectedWidgetsOutline::GetDockHandlePosition(UUIObject* Widget, EUIWidgetFace InFace, INT HandleRadius, INT HandleGap, FIntPoint& OutPosition)
{
	if(Widget == NULL)
	{
		UUIScene* Scene = Cast<UUIScene>(UIEditor->OwnerContainer);
		if(Scene != NULL)
		{
			GetDockHandlePosition(Scene, InFace, HandleRadius, HandleGap, OutPosition);
		}
	}
	else
	{
		FLOAT RenderBounds[4];
		Widget->GetPositionExtents( RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Right], RenderBounds[UIFACE_Top], RenderBounds[UIFACE_Bottom] );

		GetHandlePosition(RenderBounds, InFace, HandleRadius, HandleGap, OutPosition);
	}	
}

/** 
 * Generates a dock handle position for a scene.
 * 
 * @param Scene				Scene we are getting a dock handle position for.
 * @param InFace			Which face we are getting a position for.
 * @param HandleRadius	Radius of the handle.
 * @param HandleGap		Gap between the handle and the object's edge.
 * @param OutPosition		Position of the dock handle.
 */
void FRenderModifier_SelectedWidgetsOutline::GetDockHandlePosition(UUIScene* Scene, EUIWidgetFace InFace, INT HandleRadius, INT HandleGap, FIntPoint& OutPosition)
{
	FLOAT RenderBounds[4];

	for(INT FaceIdx=0; FaceIdx < 4; FaceIdx++)
	{
		RenderBounds[FaceIdx] = Scene->GetPosition((EUIWidgetFace)FaceIdx, EVALPOS_PixelViewport, FALSE);
	}
	
	GetHandlePosition(RenderBounds, InFace, HandleRadius, HandleGap, OutPosition);
}


/** 
 * Generates a handle position for a object.
 * 
 * @param RenderBounds	Render bounds of the object we are getting a dock handle for.
 * @param InFace		Which face we are getting a position for.
 * @param HandleRadius	Radius of the handle.
 * @param HandleGap		Gap between the handle and the object's edge.
 * @param OutPosition	Position of the dock handle.
 */
void FRenderModifier_SelectedWidgetsOutline::GetHandlePosition(const FLOAT* RenderBounds, EUIWidgetFace InFace, INT HandleRadius, INT HandleGap, FIntPoint& OutPosition)
{
	switch(InFace)
	{
	case UIFACE_Top:
		{


			OutPosition.X = (RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left]) / 2 + RenderBounds[UIFACE_Left];
			OutPosition.Y = RenderBounds[UIFACE_Top] + HandleRadius + HandleGap;

			// Make sure that we can clearly see all of the dock handles for a selected widget.
			FLOAT Distance = Abs(RenderBounds[UIFACE_Top] - RenderBounds[UIFACE_Bottom]);

			const FLOAT DistThreshold = (HandleRadius*2.0f + HandleGap*2.0f) * 2.0f;
			if(Distance < DistThreshold)
			{
				OutPosition.Y -= (DistThreshold - Distance) / 2;
			}
		}

		break;

	case UIFACE_Bottom:
		{
			OutPosition.X = (RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left]) / 2 + RenderBounds[UIFACE_Left];
			OutPosition.Y = RenderBounds[UIFACE_Bottom] - HandleRadius - HandleGap;

			// Make sure that we can clearly see all of the dock handles for a selected widget.
			FLOAT Distance = Abs(RenderBounds[UIFACE_Top] - RenderBounds[UIFACE_Bottom]);
			const FLOAT DistThreshold = (HandleRadius*2.0f + HandleGap*2.0f) * 2.0f;
			if(Distance < DistThreshold)
			{
				OutPosition.Y += (DistThreshold - Distance) / 2;
			}
		}
		break;

	case UIFACE_Left:
		{
			OutPosition.X = RenderBounds[UIFACE_Left] + HandleRadius + HandleGap;
			OutPosition.Y = (RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]) / 2 + RenderBounds[UIFACE_Top];

			// Make sure that we can clearly see all of the dock handles for a selected widget.
			FLOAT Distance = Abs(RenderBounds[UIFACE_Left] - RenderBounds[UIFACE_Right]);
			const FLOAT DistThreshold = (HandleRadius*2.0f + HandleGap*2.0f) * 2.0f;
			if(Distance < DistThreshold)
			{
				OutPosition.X -= (DistThreshold - Distance) / 2;
			}
		}

		break;

	case UIFACE_Right:
		{
			OutPosition.X = RenderBounds[UIFACE_Right] - HandleRadius - HandleGap;
			OutPosition.Y = (RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]) / 2 + RenderBounds[UIFACE_Top];

			// Make sure that we can clearly see all of the dock handles for a selected widget.
			FLOAT Distance = Abs(RenderBounds[UIFACE_Left] - RenderBounds[UIFACE_Right]);
			const FLOAT DistThreshold = (HandleRadius*2.0f + HandleGap*2.0f) * 2.0f;
			if(Distance < DistThreshold)
			{
				OutPosition.X += (DistThreshold - Distance) / 2;
			}
		}
		break;

	default:
		break;
	}
}

/** 
 * Renders a docking link for ths specified face.
 * 
 * @param Widget					Widget to render the docking handle for.
 * @param InFace					Which face we are drawing the handle for.
 * @param RI						Rendering interface to use for drawing the handle.
 * @param Start					Starting position for the link.
 * @param DockHandleRadius		Radius for the docking handles.
 * @param DockHandleGap			Gap between the handles and the edge of the widget.
 */
void FRenderModifier_SelectedWidgetsOutline::RenderDockLink( UUIObject* Widget, EUIWidgetFace InFace, FCanvas* Canvas, const FVector& InStart, INT DockHandleRadius, INT DockHandleGap )
{
	// Make sure that we have a dock target for this face.
	const EUIWidgetFace TargetFace = Widget->DockTargets.GetDockFace(InFace);
	UUIObject* TargetWidget = Widget->DockTargets.GetDockTarget(InFace);

	if( Widget->DockTargets.IsDocked(InFace) )
	{
		// Calculate end position for the link and the end arrow direction.
		FIntPoint End;
		GetDockHandlePosition(TargetWidget, TargetFace, DockHandleRadius, DockHandleGap, End);

		// Calculate end arrow direction.
		const INT MinDist = 7;
		FVector2D StartDir(0,0);
		const FIntPoint Start(InStart.X, InStart.Y);
		const FIntPoint Diff(End-Start);
		FVector2D EndDir(0,0);
		FLOAT Tension = (Start-End).Size();
		

		switch(TargetFace)
		{
		case UIFACE_Bottom:
			EndDir[1] = 1;

			if(Abs<INT>(Diff.Y) < MinDist)
			{
				Tension *= 1.5f;
			}

			break;

		case UIFACE_Top:
			EndDir[1] = -1;

			if(Abs<INT>(Diff.Y) < MinDist)
			{
				Tension *= 1.5f;
			}

			break;

		case UIFACE_Right: 
			EndDir[0] = 1;

			if(Abs<INT>(Diff.X) < MinDist)
			{
				Tension *= 1.5f;
			}

			break;

		case UIFACE_Left:
			EndDir[0] = -1;

			if(Abs<INT>(Diff.X) < MinDist)
			{
				Tension *= 1.5f;
			}
			
			break;

		default:
			return;
		}

		/** @todo: Leave this code here please. - ASM
			switch(TargetFace)
			{
			case UIFACE_Bottom: case UIFACE_Top:
				if(Diff.Y > MinDist)
				{
					EndDir[1] = -1;
				}
				else if(Diff.Y < -MinDist)
				{
					EndDir[1] = 1;
				}
				else
				{
					if(Diff.X > MinDist)
					{
						EndDir[0] = -1;
					}
					else
					{
						EndDir[0] = 1;
					}
				}
				break;

			case UIFACE_Right: case UIFACE_Left:
				if(Diff.X > MinDist)
				{
					EndDir[0] = -1;
				}
				else if(Diff.X < -MinDist)
				{
					EndDir[0] = 1;
				}
				else
				{
					if(Diff.Y > MinDist)
					{
						EndDir[1] = -1;
					}
					else
					{
						EndDir[1] = 1;
					}
				}
				break;

			default:
				return;
			}
		*/



		// Finally render the spline.
		const FColor LineColor(0,255,255);
		FLinkedObjDrawUtils::DrawSpline(Canvas, Start, StartDir, End, -Tension * EndDir, LineColor, TRUE, FALSE);
	}
}

