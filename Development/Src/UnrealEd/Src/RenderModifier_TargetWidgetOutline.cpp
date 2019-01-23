/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/


#include "UnrealEd.h"
#include "UnObjectEditor.h"
#include "UnUIEditor.h"
#include "UnLinkedObjDrawUtils.h"

#include "RenderModifier_TargetWidgetOutline.h"

/**
* Constructor
*
* @param	InEditor	the editor window that holds the container to render the outline for
*/
FRenderModifier_TargetWidgetOutline::FRenderModifier_TargetWidgetOutline( class WxUIEditorBase* InEditor ) : 
FRenderModifier_SelectedWidgetsOutline(InEditor)
{

}

FRenderModifier_TargetWidgetOutline::~FRenderModifier_TargetWidgetOutline()
{
	FreeSortedDockHandleList();
}


/** 
 * Generates a list of all of the dock handles in the scene and sorts them by X position. 
 *
 * @param SourceWidget	The widget we are connecting a dock link from.
 * @param SourceFace	The face we are connecting from.
 */
void FRenderModifier_TargetWidgetOutline::GenerateSortedDockHandleList(UUIObject* SourceWidget, EUIWidgetFace SourceFace)
{
	FreeSortedDockHandleList();

	// Add dock handles for the scene.
	{
		UUIScene* Scene = Cast<UUIScene>(UIEditor->OwnerContainer);
		FWidgetDockHandles* WidgetDockHandles = new FWidgetDockHandles;

		WidgetDockHandles->bVisible = FALSE;
		WidgetDockHandles->Widget = NULL;

		// Add a item for each dock handle, each dock handle checks to see if it is a valid target.
		// If not, it wont be added to the sorted dock handles list and will also have its validtarget flag set to false.
		const EUIWidgetFace Faces[4] = {UIFACE_Top, UIFACE_Bottom, UIFACE_Left, UIFACE_Right};

		for(INT DockFaceIdx = 0; DockFaceIdx < 4; DockFaceIdx++)
		{
			WidgetDockHandles->DockHandles[DockFaceIdx].Widget = NULL;
			WidgetDockHandles->DockHandles[DockFaceIdx].Face = Faces[DockFaceIdx];
			WidgetDockHandles->DockHandles[DockFaceIdx].bValidTarget = TRUE;

			if(WidgetDockHandles->DockHandles[DockFaceIdx].bValidTarget == TRUE)
			{
				GetDockHandlePosition(Scene, Faces[DockFaceIdx], FDockingConstants::HandleRadius, 0, WidgetDockHandles->DockHandles[DockFaceIdx].Position);

				SortedDockHandles.AddItem(&WidgetDockHandles->DockHandles[DockFaceIdx]);
			}
		}

		SceneWidgetDockHandles.AddItem(WidgetDockHandles);
	}

	// Add dock handles for all of the scene's widgets.
	TArray<UUIObject*> Widgets = UIEditor->OwnerContainer->GetChildren( TRUE );

	for( INT DockIdx = 0; DockIdx < Widgets.Num(); DockIdx++ )
	{
		FWidgetDockHandles* WidgetDockHandles = new FWidgetDockHandles;

		WidgetDockHandles->bVisible = FALSE;
		WidgetDockHandles->Widget = Widgets(DockIdx);

		// Add a item for each dock handle, each dock handle checks to see if it is a valid target.
		// If not, it wont be added to the sorted dock handles list and will also have its validtarget flag set to false.
		const EUIWidgetFace Faces[4] = {UIFACE_Top, UIFACE_Bottom, UIFACE_Left, UIFACE_Right};

		for(INT DockFaceIdx = 0; DockFaceIdx < 4; DockFaceIdx++)
		{
			WidgetDockHandles->DockHandles[DockFaceIdx].Widget = WidgetDockHandles->Widget;
			WidgetDockHandles->DockHandles[DockFaceIdx].Face = Faces[DockFaceIdx];
			WidgetDockHandles->DockHandles[DockFaceIdx].bValidTarget = SourceWidget->IsValidDockTarget(SourceFace, WidgetDockHandles->DockHandles[DockFaceIdx].Widget, WidgetDockHandles->DockHandles[DockFaceIdx].Face);

			if(WidgetDockHandles->DockHandles[DockFaceIdx].bValidTarget == TRUE)
			{
				GetDockHandlePosition(WidgetDockHandles->Widget, Faces[DockFaceIdx], FDockingConstants::HandleRadius, 0, WidgetDockHandles->DockHandles[DockFaceIdx].Position);

				SortedDockHandles.AddItem(&WidgetDockHandles->DockHandles[DockFaceIdx]);
			}
		}
	
		SceneWidgetDockHandles.AddItem(WidgetDockHandles);
	}


	Sort<FWidgetDockHandle*,FRenderModifier_TargetWidgetOutline::FWidgetDockHandle>(&SortedDockHandles(0),SortedDockHandles.Num());
}

void FRenderModifier_TargetWidgetOutline::FreeSortedDockHandleList()
{
	
	INT WidgetCount = SceneWidgetDockHandles.Num();


	for( INT WidgetIdx = 0; WidgetIdx < WidgetCount; WidgetIdx++ )
	{
		FWidgetDockHandles* WidgetHandles = SceneWidgetDockHandles(WidgetIdx);

		delete WidgetHandles;
	}

	SceneWidgetDockHandles.Empty();
	SortedDockHandles.Empty();
}


void FRenderModifier_TargetWidgetOutline::CalculateVisibleDockHandles(TArray<UUIObject*> &Widgets)
{

	for(INT WidgetHandlesIdx = 0; WidgetHandlesIdx < SceneWidgetDockHandles.Num(); WidgetHandlesIdx++)
	{
		FWidgetDockHandles* WidgetHandles = SceneWidgetDockHandles(WidgetHandlesIdx);
		UBOOL bWidgetMatch = FALSE;

		for( INT WidgetIdx = 0; WidgetIdx < Widgets.Num(); WidgetIdx++ )
		{
			UUIObject* Widget = Widgets(WidgetIdx);

			bWidgetMatch = (WidgetHandles->Widget == Widget);

			if( bWidgetMatch == TRUE)
			{
				break;
			}
		}

		WidgetHandles->bVisible = bWidgetMatch;
	}


}

/** Sets the selected dock face for highlighting. */
void FRenderModifier_TargetWidgetOutline::SetSelectedDockFace(EUIWidgetFace InFace)
{
	// Only update when we actually change state because it is an expensive operation to calculate
	// close dock handles.
	if(SelectedDockHandle != InFace)
	{
		SelectedDockHandle = InFace;

		if(SelectedDockHandle != UIFACE_MAX)
		{
			// Calculate all handles within the proximity of the handle we selected, then build a list of all of their names.
			TArray<FString> TempStringList;
			const TArray<FRenderModifier_TargetWidgetOutline::FWidgetDockHandle*>* CloseHandles = CalculateCloseDockHandles();
			const INT NumHandles = CloseHandles->Num();
			for(INT HandleIdx = 0; HandleIdx < NumHandles; HandleIdx++)
			{
				const FWidgetDockHandle* Handle = (*CloseHandles)(HandleIdx);

				FString TempString;
				UIEditor->GetDockHandleString(Handle->Widget, Handle->Face, TempString);

				TempStringList.AddItem(TempString);

			}

			UIEditor->TargetTooltip->SetStringList(TempStringList);
		}
		else
		{
			UIEditor->TargetTooltip->ClearStringList();
		}
	}
	
}


/** Clears the selected dock face and dock widget. */
void FRenderModifier_TargetWidgetOutline::ClearSelections()
{
	ClearVisibleDockHandles();
	SelectedWidget = NULL;
	SelectedDockHandle = UIFACE_MAX;

	UIEditor->TargetTooltip->ClearStringList();
}

/** Toggles all of the dock handles to not be visible */
void FRenderModifier_TargetWidgetOutline::ClearVisibleDockHandles()
{
	for(INT WidgetHandlesIdx = 0; WidgetHandlesIdx < SceneWidgetDockHandles.Num(); WidgetHandlesIdx++)
	{
		FWidgetDockHandles* WidgetHandles = SceneWidgetDockHandles(WidgetHandlesIdx);

		WidgetHandles->bVisible = FALSE;
	}
}
/**
 *	Calculates dock handles which are close to the currently selected widget dock handle.
 *  @return			The number of dock handles in the specified radius.
 */
TArray<FRenderModifier_TargetWidgetOutline::FWidgetDockHandle*>* FRenderModifier_TargetWidgetOutline::CalculateCloseDockHandles()
{
	if(SelectedDockHandle != UIFACE_MAX)
	{
		FIntPoint SelectedHandlePosition;
		GetDockHandlePosition(SelectedWidget, SelectedDockHandle, FDockingConstants::HandleRadius, 0, SelectedHandlePosition);

		const INT TotalHandles = SortedDockHandles.Num();
		const FLOAT SearchRadius = (FDockingConstants::HandleRadius * 1.5f);
		const FLOAT DistanceThreshold = SearchRadius * SearchRadius;
		const FLOAT MinX = (INT)SelectedHandlePosition.X - SearchRadius;
		const FLOAT MaxX = (INT)SelectedHandlePosition.X + SearchRadius;

		CloseHandles.Empty();

		// Generate a list of docking handles within our selection threshold.  The list of handles is sorted by X position,
		// so we can exit early if the X position of a handle is greater than our maximum X.
		for(INT HandleIdx = 0; HandleIdx < TotalHandles; HandleIdx++)
		{
			FRenderModifier_TargetWidgetOutline::FWidgetDockHandle* Handle = SortedDockHandles(HandleIdx);

			if(Handle->Position.X > MaxX)
			{
				break;
			}

			if(Handle->Position.X > MinX)
			{
				FLOAT DistX = Handle->Position.X - SelectedHandlePosition.X; 
				FLOAT DistY = Handle->Position.Y - SelectedHandlePosition.Y;
				FLOAT Dist = DistX * DistX + DistY * DistY;

				if(Dist < DistanceThreshold)
				{
					CloseHandles.AddItem(Handle);
				}
			}
		}
	}
	else
	{
		CloseHandles.Empty();
	}
	

	return &CloseHandles;
}


/**
 * Method for serializing UObject references that must be kept around through GC's.
 *
 * @param Ar The archive to serialize with
 */
void FRenderModifier_TargetWidgetOutline::Serialize(FArchive& Ar)
{
	INT NumHandles = SceneWidgetDockHandles.Num();

	for(INT HandleIdx = 0; HandleIdx < NumHandles; HandleIdx++)
	{
		FWidgetDockHandles* WidgetDockHandles = SceneWidgetDockHandles(HandleIdx);

		Ar << WidgetDockHandles->Widget;
	}

	Ar << SelectedWidget;


	FRenderModifier_SelectedWidgetsOutline::Serialize(Ar);
}

/**
 * Renders all of the visible dock handles.
 */
void FRenderModifier_TargetWidgetOutline::Render( FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas )
{
	INT NumHandles = SceneWidgetDockHandles.Num();

	for(INT HandleIdx = 0; HandleIdx < NumHandles; HandleIdx++)
	{
		const FWidgetDockHandles* WidgetDockHandles = SceneWidgetDockHandles(HandleIdx);

		if(WidgetDockHandles->bVisible == TRUE)
		{
			for( INT DockIdx = 0; DockIdx < 4; DockIdx++)
			{

				// Only draw the dock handle if it is a valid target for our dock link.
				if(WidgetDockHandles->DockHandles[DockIdx].bValidTarget == TRUE)
				{
					RenderDockHandle(WidgetDockHandles->DockHandles[DockIdx], ObjectVC, Canvas);
				}
			}
		}
	}
}


/** 
 * Renders a docking handle for this widget, it uses a different color to let the user know its a target widget.
 * 
 * @param Widget	Widget to render the docking handle for.
 * @param Face		Which face we are drawing the handle for.
 * @param RI		Rendering interface to use for drawing the handle.
 */
void FRenderModifier_TargetWidgetOutline::RenderDockHandle(const FWidgetDockHandle& DockHandle, FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas )
{
	const FColor DockHandlerColor(255,32,0);
	const FColor DockHandlerHighlightColor(255,255,0);
	const FColor DockHandlerBorderColor(0,0,0);
	const FLOAT Zoom = ObjectVC->Zoom2D < 1.0f ? 1.0f : ObjectVC->Zoom2D;
	const FLOAT DockHandleRadius = FDockingConstants::HandleRadius / Zoom;
	const INT NumSides = FDockingConstants::HandleNumSides;

	Canvas->SetHitProxy( new HUIDockHandleHitProxy(DockHandle.Widget, DockHandle.Face) );
	{
		// Draw the dock handle circle, if the mouse is over the dock handle, then draw it in a different color.
		const UBOOL bHandleHighlighted = (SelectedDockHandle == DockHandle.Face && SelectedWidget == DockHandle.Widget);

		FLinkedObjDrawUtils::DrawNGon(Canvas, DockHandle.Position, DockHandlerBorderColor, NumSides, DockHandleRadius+1);

		if(bHandleHighlighted == TRUE)
		{
			FLinkedObjDrawUtils::DrawNGon(Canvas, DockHandle.Position, DockHandlerHighlightColor, NumSides, DockHandleRadius);
		}
		else
		{
			FLinkedObjDrawUtils::DrawNGon(Canvas, DockHandle.Position, DockHandlerColor, NumSides, DockHandleRadius);
		}

	}
	Canvas->SetHitProxy( NULL );
}

