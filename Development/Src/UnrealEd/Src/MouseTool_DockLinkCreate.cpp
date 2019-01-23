/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnObjectEditor.h"
#include "UnUIEditor.h"
#include "UnLinkedObjDrawUtils.h"
#include "ScopedTransaction.h"

#include "MouseTool_DockLinkCreate.h"
#include "UIWidgetTool_Selection.h"
#include "ScopedObjectStateChange.h"

/** Constructor */
FMouseTool_DockLinkCreate::FMouseTool_DockLinkCreate( FEditorObjectViewportClient* InObjectVC, WxUIEditorBase* InUIEditor, class FUIWidgetTool_Selection* InParentWidgetTool ) : 
FMouseTool(InObjectVC),
UIEditor(InUIEditor),
DragWidget(NULL),
DockFace(UIFACE_MAX),
ParentWidgetTool(InParentWidgetTool)
{

}

/** Destructor */
FMouseTool_DockLinkCreate::~FMouseTool_DockLinkCreate()
{
	ObjectVC = NULL;
	DragWidget = NULL;
	UIEditor = NULL;
}


/**
 * Creates a instance of a widget based on the current tool mode and starts the drag process.
 *
 * @param	InViewportClient	the viewport client to draw the selection box in
 * @param	Start				Where the mouse was when the drag started
 */
void FMouseTool_DockLinkCreate::StartDrag(FEditorLevelViewportClient* InViewportClient, const FVector& InStart)
{
	// Disable traditional mouse capturing, we will handle input ourselves.
	InViewportClient->Viewport->CaptureMouseInput( FALSE );
	InViewportClient->Viewport->LockMouseToWindow( FALSE );

	// Store some information about our start position and reset drag variables.
	ViewportClient = InViewportClient;
	Start = End = EndWk = InStart;

	DragWidget = ParentWidgetTool->SelectedWidgetsOutline.GetSelectedWidget();

	// Generate a list of all of the possible dock targets and sort them by X position.
	ParentWidgetTool->TargetWidgetOutline.GenerateSortedDockHandleList(DragWidget, ParentWidgetTool->SelectedWidgetsOutline.GetSelectedDockHandle());

	if(DragWidget == NULL)
	{
		DockFace = UIFACE_MAX;
	}
	else
	{
		const INT DockHandleRadius = FDockingConstants::HandleRadius;
		const INT DockHandleGap = FDockingConstants::HandleGap;
		DockFace = ParentWidgetTool->SelectedWidgetsOutline.GetSelectedDockHandle();

		FLOAT RenderBounds[4];
		DragWidget->GetPositionExtents( RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Right], RenderBounds[UIFACE_Top], RenderBounds[UIFACE_Bottom] );

		// Set the spline's starting position to the center of the handle.
		ParentWidgetTool->SelectedWidgetsOutline.GetHandlePosition(RenderBounds, DockFace, DockHandleRadius, DockHandleGap, SplineStart);
	}
}

/**
 * Ends the mouse drag movement.  Called when the user releases the mouse button while this drag tool is active.
 */
void FMouseTool_DockLinkCreate::EndDrag()
{
	// If there is a selected widget and a target widget and face, we will create a docking link.
	UUIScreenObject* TargetWidget = ParentWidgetTool->TargetWidgetOutline.GetSelectedWidget();
	EUIWidgetFace TargetFace = ParentWidgetTool->TargetWidgetOutline.GetSelectedDockHandle();

	const UBOOL bWidgetSelected = DragWidget != NULL;
	const UBOOL bDockFaceSelected = DockFace != UIFACE_MAX;
	const UBOOL bTargetFaceSelected = TargetFace != UIFACE_MAX;

	if( bWidgetSelected == TRUE && bDockFaceSelected == TRUE && bTargetFaceSelected == TRUE )
	{
		TArray<FRenderModifier_TargetWidgetOutline::FWidgetDockHandle*>* CloseHandles = ParentWidgetTool->TargetWidgetOutline.CalculateCloseDockHandles();
		
		// If only one handle was a possible selection then just set the dock target to that handle.
		// otherwise, display a popup menu of possible choices and let the user pick which handle to attach to.
		const INT NumCloseHandles = CloseHandles->Num();

		if(NumCloseHandles == 1)
		{
			{
				FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_ConnectDockLink")) );

				if(TargetWidget == NULL)
				{
					TargetWidget = UIEditor->OwnerContainer;
				}

				FScopedObjectStateChange DockingChangeNotifier(DragWidget);

				DragWidget->SetDockTarget(DockFace, TargetWidget, TargetFace);
				DragWidget->UpdateScene();
				UIEditor->RefreshPositionPanelValues();
				UIEditor->RefreshDockingPanelValues();
			}
		}
		else
		{
			// Create the choose target context menu by specifying the number of options.
			// Then loop through and set the label for each of the options to generated string.
			WxUIDockTargetContextMenu menu(ParentWidgetTool->MenuProxy);
			menu.Create( NumCloseHandles );

			for(INT HandleIdx = 0; HandleIdx < NumCloseHandles; HandleIdx++)
			{
				FRenderModifier_TargetWidgetOutline::FWidgetDockHandle* Handle = (*CloseHandles)(HandleIdx);
				FString HandleName;
				UIEditor->GetDockHandleString(Handle->Widget, Handle->Face, HandleName);
				
				HandleName = FString::Printf( *LocalizeUI(TEXT("UIEditor_MenuItem_ConnectDockLinkTo")), *HandleName);
				menu.SetMenuItemLabel(HandleIdx, *HandleName);
				
			}

			FTrackPopupMenu tpm( ParentWidgetTool->MenuProxy, &menu );
			tpm.Show();
		}
	}

	// Make sure to free the sorted dock handle list so we do not have extra UObject references lying around.
	ParentWidgetTool->TargetWidgetOutline.FreeSortedDockHandleList();
	ParentWidgetTool->TargetWidgetOutline.ClearSelections();
	ParentWidgetTool->SelectedWidgetsOutline.ClearSelections();

	DragWidget = NULL;
	DockFace = UIFACE_MAX;
}

/**
 *	Callback allowing the drag tool to render some data to the viewport.
 */
void FMouseTool_DockLinkCreate::Render(FCanvas* Canvas)
{
	// Render a spline from the starting position to our mouse cursor.
	
	const FVector2D StartDir(1,0);
	const FVector2D EndDir(1,0);
	const FColor SplineColor(0,255,255);
	const FIntPoint SplineEnd(End.X, End.Y);
	const FLOAT Tension = (SplineEnd - SplineStart).Size();

	FLinkedObjDrawUtils::DrawSpline(Canvas, SplineStart, StartDir * 0, SplineEnd, EndDir * 0, SplineColor, FALSE);
}

