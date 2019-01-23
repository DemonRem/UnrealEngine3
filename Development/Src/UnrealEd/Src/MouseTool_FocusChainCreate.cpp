/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnObjectEditor.h"
#include "UnUIEditor.h"
#include "UnLinkedObjDrawUtils.h"
#include "ScopedTransaction.h"

#include "MouseTool_FocusChainCreate.h"
#include "UIWidgetTool_FocusChain.h"
#include "ScopedObjectStateChange.h"

/** Constructor */
FMouseTool_FocusChainCreate::FMouseTool_FocusChainCreate( FEditorObjectViewportClient* InObjectVC, WxUIEditorBase* InUIEditor, class FUIWidgetTool_FocusChain* InParentWidgetTool ) : 
FMouseTool(InObjectVC),
UIEditor(InUIEditor),
DragWidget(NULL),
DragFace(UIFACE_MAX),
ParentWidgetTool(InParentWidgetTool)
{

}

/** Destructor */
FMouseTool_FocusChainCreate::~FMouseTool_FocusChainCreate()
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
void FMouseTool_FocusChainCreate::StartDrag(FEditorLevelViewportClient* InViewportClient, const FVector& InStart)
{
	// Disable traditional mouse capturing, we will handle input ourselves.
	InViewportClient->Viewport->CaptureMouseInput( FALSE );
	InViewportClient->Viewport->LockMouseToWindow( FALSE );

	// Store some information about our start position and reset drag variables.
	ViewportClient = InViewportClient;
	Start = End = EndWk = InStart;


	DragWidget = ParentWidgetTool->RenderModifier.GetSelectedWidget();


	// Generate a list of all of the possible dock targets and sort them by X position.
	if(DragWidget == NULL)
	{
		DragFace = UIFACE_MAX;
	}
	else
	{
		const INT DockHandleRadius = FDockingConstants::HandleRadius;
		const INT DockHandleGap = FDockingConstants::HandleGap;
		DragFace = ParentWidgetTool->RenderModifier.GetSelectedDockHandle();

		// Set the spline's starting position to the center of the handle.
		ParentWidgetTool->RenderModifier.GetHandlePosition(DragWidget->RenderBounds, DragFace, DockHandleRadius, DockHandleGap, SplineStart);
	}
}

/**
 * Ends the mouse drag movement.  Called when the user releases the mouse button while this drag tool is active.
 */
void FMouseTool_FocusChainCreate::EndDrag()
{
	// If there is a selected widget and a target widget, we will create a focus chain link to the widget.
	const UUIObject* TargetWidget = ParentWidgetTool->TargetRenderModifier.GetSelectedWidget();
	
	const UBOOL bWidgetSelected = DragWidget != NULL;
	const UBOOL bDragFaceSelected = DragFace != UIFACE_MAX;
	const UBOOL bTargetWidgetSelected = TargetWidget != NULL;

	if( bWidgetSelected == TRUE && bDragFaceSelected == TRUE && bTargetWidgetSelected == TRUE )
	{
		TArray<UUIObject*>& TargetWidgets = ParentWidgetTool->TargetRenderModifier.GetTargetWidgets();
		
		// If only one widget was a possible selection then just set the focus chain link to that widget.
		// otherwise, display a popup menu of possible choices and let the user pick which widget to attach to.
		const INT NumTargetWidgets = TargetWidgets.Num();
		if(NumTargetWidgets == 1)
		{
			{
				FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_ConnectFocusChainLink")) );
				FScopedObjectStateChange FocusChainChainNotification(DragWidget);

				DragWidget->SetForcedNavigationTarget(DragFace,const_cast<UUIObject*>(TargetWidget));
			}
		}
		else
		{
			// Create the choose target context menu by specifying the number of options.
			// Then loop through and set the label for each of the options to generated string.
			WxUIDockTargetContextMenu menu(ParentWidgetTool->MenuProxy);
			menu.Create( NumTargetWidgets );

			for(INT WidgetIdx = 0; WidgetIdx < NumTargetWidgets; WidgetIdx++)
			{
				const UUIObject* Widget = TargetWidgets(WidgetIdx);

				FString WidgetName = FString::Printf( *LocalizeUI(TEXT("UIEditor_MenuItem_ConnectFocusChainLinkTo")), *Widget->GetName());
				menu.SetMenuItemLabel(WidgetIdx, *WidgetName);
				
			}

			FTrackPopupMenu tpm( ParentWidgetTool->MenuProxy, &menu );
			tpm.Show();
		}
	}

	// Make sure to free the sorted dock handle list so we do not have extra UObject references lying around.
	ParentWidgetTool->TargetRenderModifier.ClearSelections();
	ParentWidgetTool->RenderModifier.ClearSelections();

	// Update the property window to reflect our new focus link changes.
	UIEditor->UpdatePropertyWindow();

	DragWidget = NULL;
	DragFace = UIFACE_MAX;
}

/**
 *	Callback allowing the drag tool to render some data to the viewport.
 */
void FMouseTool_FocusChainCreate::Render(FCanvas* Canvas)
{
	// Render a spline from the starting position to our mouse cursor.
	
	const FVector2D StartDir(1,0);
	const FVector2D EndDir(1,0);
	const FColor SplineColor(0,255,255);
	const FIntPoint SplineEnd(End.X, End.Y);
	const FLOAT Tension = (SplineEnd - SplineStart).Size();

	FLinkedObjDrawUtils::DrawSpline(Canvas, SplineStart, StartDir * 0, SplineEnd, EndDir * 0, SplineColor, FALSE);
}

