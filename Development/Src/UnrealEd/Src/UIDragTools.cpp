/*=============================================================================
	UIDragTools.cpp: Class implementation for UI editor mouse drag tools
	Copyright 2006-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/* ==========================================================================================================
	Includes
========================================================================================================== */
#include "UnrealEd.h"
#include "UnrealEdPrivateClasses.h"
#include "UnObjectEditor.h"
#include "EngineUIPrivateClasses.h"

#include "UnUIEditor.h"
#include "UIWidgetTool.h"
#include "UIDragTools.h"
#include "UnLinkedObjDrawUtils.h"

#include "ScopedTransaction.h"
#include "ScopedObjectStateChange.h"

/* ==========================================================================================================
	"Interactive Place Widget" Drag Tool - places a widget into the scene using the mouse to initialize its
	position and size
========================================================================================================== */

static const INT MouseDragCreateThreshold = 4;

/** Constructor */
FMouseTool_ObjectDragCreate::FMouseTool_ObjectDragCreate(FEditorObjectViewportClient* InObjectVC, WxUIEditorBase* InUIEditor)
: FMouseTool(InObjectVC), UIEditor(InUIEditor), DragWidget(NULL)
{
	SetGridOrigin(InUIEditor->GetGridOrigin());
	SetGridSize(InUIEditor->GetGridSize());
	SetUseSnapping(InUIEditor->GetUseSnapping());
}

/** Destructor */
FMouseTool_ObjectDragCreate::~FMouseTool_ObjectDragCreate()
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
void FMouseTool_ObjectDragCreate::StartDrag(FEditorLevelViewportClient* InViewportClient, const FVector& InStart)
{
	FMouseTool::StartDrag(InViewportClient, InStart);

	DragWidget = NULL;
}

/**
 * Ends the mouse drag movement.  Called when the user releases the mouse button while this drag tool is active.
 */
void FMouseTool_ObjectDragCreate::EndDrag()
{
	DragWidget = NULL;
}


/**
* Called when the user moves the mouse while this viewport is not capturing mouse input (i.e. not dragging).
*/
void FMouseTool_ObjectDragCreate::MouseMove(FViewport* Viewport, INT X, INT Y)
{
	FMouseTool::MouseMove(Viewport, X, Y);

	// Generate start and end positions based on which way they dragged the mouse from the start position.
	// This way they can create a widget using their start click location as a 'pivot'.
	FVector StartPos;
	FVector EndPos;

	// In order to correct for gutter positioning, we offset the position of the created widget.
	// @todo: This offset code is here because SetPosition is a bit buggy when it comes to gutter sizes right now.
	INT Offset;

	if(bUseSnapping)
	{
		const INT NumBoxes = (INT)appTrunc(UIEditor->EditorOptions->ViewportGutterSize / (FLOAT)GridSize + 0.5f);
		Offset = NumBoxes * GridSize;
	}
	else
	{
		Offset = UIEditor->EditorOptions->ViewportGutterSize;
	}


	if(End.X < Start.X)
	{
		StartPos.X = End.X;
		EndPos.X = Start.X;
	}
	else
	{
		StartPos.X = Start.X;
		EndPos.X = End.X;
	}

	if(End.Y < Start.Y)
	{
		StartPos.Y = End.Y;
		EndPos.Y = Start.Y;
	}
	else
	{
		StartPos.Y = Start.Y;
		EndPos.Y = End.Y;
	}
	
	StartPos.X -= Offset;
	StartPos.Y -= Offset;
	EndPos.X -= Offset;
	EndPos.Y -= Offset;

	// See if we need to create a widget first, we wait until the mouse has traveled a certain distance before creating a widget.
	if(DragWidget == NULL && (End-Start).Size() > MouseDragCreateThreshold)
	{
		INT WidgetIdx;
		const UBOOL bIsValidWidget = UIEditor->MainToolBar->GetWidgetIDFromToolID(UIEditor->EditorOptions->ToolMode, WidgetIdx);

		if( bIsValidWidget )
		{
			// In order to correct for gutter positioning, we offset the position of the created widget.
			// @todo: This offset code is here because SetPosition is a bit buggy when it comes to gutter sizes right now.
			INT Offset;

			if(bUseSnapping)
			{
				const INT NumBoxes = (INT)appTrunc(UIEditor->EditorOptions->ViewportGutterSize / (FLOAT)GridSize + 0.5f);
				Offset = NumBoxes * GridSize;
			}
			else
			{
				Offset = UIEditor->EditorOptions->ViewportGutterSize;
			}


			DragWidget = UIEditor->CreateWidget(WidgetIdx, Start.X - Offset, Start.Y - Offset, 1, 1);
		}
		else
		{
			DragWidget = NULL;
		}
	}

	// Resize widget to reflect our new mouse end position.
	if(DragWidget)
	{
		DragWidget->SetPosition( StartPos.X, UIFACE_Left, EVALPOS_PixelViewport);
		DragWidget->SetPosition( StartPos.Y, UIFACE_Top, EVALPOS_PixelViewport);
		DragWidget->SetPosition( EndPos.X, UIFACE_Right, EVALPOS_PixelViewport);
		DragWidget->SetPosition( EndPos.Y, UIFACE_Bottom, EVALPOS_PixelViewport);

		UIEditor->PositionPanel->RefreshControls();
	}
}

/** 
 * Generates a string that describes the current drag operation to the user.
 * 
 * This version blanks out the string because we do not want to display any info.
 *
 * @param OutString		Storage class for the generated string, should have the current delta in it in some way.
 */
void FMouseTool_ObjectDragCreate::GetDeltaStatusString(FString &OutString) const
{
	FVector Delta = End-Start;

	OutString = FString::Printf(*LocalizeUnrealEd("ObjectEditor_MouseDelta_SizeF"), Delta.X, Delta.Y);
}

/* ==========================================================================================================
	"Resize List Columns" drag tool - interactively resizes columns in a UIList using the mouse.
========================================================================================================== */
/** Constructor */
FMouseTool_ResizeListCell::FMouseTool_ResizeListCell( FEditorObjectViewportClient* InObjectVC, WxUIEditorBase* InUIEditor, UUIList* InSelectedList, INT InResizeCell )
: FMouseTool(InObjectVC), UIEditor(InUIEditor), CellResizePropagator(NULL), SelectedList(InSelectedList), ResizeCell(InResizeCell)
{
	SetGridOrigin(InUIEditor->GetGridOrigin());
	SetGridSize(InUIEditor->GetGridSize());
	SetUseSnapping(FALSE);
}

/** Destructor */
FMouseTool_ResizeListCell::~FMouseTool_ResizeListCell()
{
	ObjectVC = NULL;
	SelectedList = NULL;
}

/**
 * Begins resizing the 
 *
 * @param	InViewportClient	the viewport client to draw the selection box in
 * @param	Start				Where the mouse was when the drag started
 */
void FMouseTool_ResizeListCell::StartDrag(FEditorLevelViewportClient* InViewportClient, const FVector& InStart)
{
	UIEditor->BeginTransaction(*LocalizeUI(TEXT("TransResizeCell")));
	CellResizePropagator = new FScopedObjectStateChange(SelectedList);

	FMouseTool::StartDrag(InViewportClient, InStart);
}

/**
 * Ends the mouse drag movement.  Called when the user releases the mouse button while this drag tool is active.
 */
void FMouseTool_ResizeListCell::EndDrag()
{
	FMouseTool::EndDrag();

	delete CellResizePropagator;
	CellResizePropagator = NULL;

	UIEditor->EndTransaction();
}

/**
 * Called when the user moves the mouse while this viewport is not capturing mouse input (i.e. not dragging).
 */
void FMouseTool_ResizeListCell::MouseMove(FViewport* Viewport, INT X, INT Y)
{
	FMouseTool::MouseMove(Viewport, X, Y);

	if ( SelectedList != NULL && SelectedList->CellDataComponent != NULL )
	{
		FUIListElementCellTemplate& SchemaCell = SelectedList->CellDataComponent->ElementSchema.Cells(ResizeCell);

		FVector TransformedMousePos = SelectedList->PixelToCanvas(FVector2D(X, Y));

		FLOAT PreviousValue = SchemaCell.CellSize.GetValue(SelectedList);
		FLOAT NewValue = (SelectedList->CellLinkType == LINKED_Columns ? TransformedMousePos.X : TransformedMousePos.Y) - SchemaCell.CellPosition;

		SchemaCell.CellSize.SetValue(SelectedList, NewValue);
		if ( PreviousValue != NewValue )
		{
			SelectedList->CellDataComponent->ReapplyFormatting();
		}
	}
}

/**
 * @return	Returns which mouse cursor to display while this tool is active.
 */
EMouseCursor FMouseTool_ResizeListCell::GetCursor() const
{
	if ( SelectedList != NULL )
	{
		return SelectedList->CellLinkType == LINKED_Columns ? MC_SizeLeftRight : MC_SizeUpDown;
	}
	
	return FMouseTool::GetCursor();
}

/** 
 * Generates a string that describes the current drag operation to the user.
 * 
 * @param OutString		Storage class for the generated string, should have the current delta in it in some way.
 */
void FMouseTool_ResizeListCell::GetDeltaStatusString(FString &OutString) const
{
	FVector Delta = End - Start;
	OutString = FString::Printf(*LocalizeUnrealEd("ObjectEditor_MouseDelta_SizeF"), Delta.X, Delta.Y);
}


HUICellBoundaryHitProxy::HUICellBoundaryHitProxy( UUIList* InSelectedList, INT InResizeCell )
: HObject(InSelectedList), SelectedList(InSelectedList), ResizeCell(InResizeCell)
{}

/**
 * Changes the cursor to the resize cursor
 */
EMouseCursor HUICellBoundaryHitProxy::GetMouseCursor()
{
	if ( SelectedList != NULL )
	{
		return SelectedList->CellLinkType == LINKED_Columns ? MC_SizeLeftRight : MC_SizeUpDown;
	}

	return HObject::GetMouseCursor();
}

/* ==========================================================================================================
	"Create Navigation Link" Drag Tool - creates a manual navigation link between two widgets
========================================================================================================== */
/** Constructor */
FMouseTool_FocusChainCreate::FMouseTool_FocusChainCreate( FEditorObjectViewportClient* InObjectVC, WxUIEditorBase* InUIEditor, FUIWidgetTool_FocusChain* InParentWidgetTool )
:  FMouseTool(InObjectVC), UIEditor(InUIEditor),
DragWidget(NULL), DragFace(UIFACE_MAX), ParentWidgetTool(InParentWidgetTool)
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
		FLOAT RenderBounds[4];
		DragWidget->GetPositionExtents( RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Right], RenderBounds[UIFACE_Top], RenderBounds[UIFACE_Bottom], FALSE );
		ParentWidgetTool->RenderModifier.GetHandlePosition(RenderBounds, DragFace, DockHandleRadius, DockHandleGap, SplineStart);

		FVector TransformedSplineStart = DragWidget->Project(FVector(SplineStart.X, SplineStart.Y, 0.f));
		SplineStart.X = appRound(TransformedSplineStart.X);
		SplineStart.Y = appRound(TransformedSplineStart.Y);
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

	Canvas->PushAbsoluteTransform(FScaleMatrix(UIEditor->ObjectVC->Zoom2D) * FTranslationMatrix(FVector(UIEditor->ObjectVC->Origin2D,0)));
	FLinkedObjDrawUtils::DrawSpline(Canvas, SplineStart, StartDir * 0, SplineEnd, EndDir * 0, SplineColor, FALSE);
	Canvas->PopTransform();
}

/* ==========================================================================================================
	"Create Docking Link" Drag Tool - creates a docking link between two widgets
========================================================================================================== */
/** Constructor */
FMouseTool_DockLinkCreate::FMouseTool_DockLinkCreate( FEditorObjectViewportClient* InObjectVC, WxUIEditorBase* InUIEditor, FUIWidgetTool_Selection* InParentWidgetTool )
:  FMouseTool(InObjectVC), UIEditor(InUIEditor),
DragWidget(NULL), DockFace(UIFACE_MAX), ParentWidgetTool(InParentWidgetTool)
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

		FVector TransformedSplineStart = DragWidget->Project(FVector(SplineStart.X, SplineStart.Y, 0.f));

		SplineStart.X = appRound(TransformedSplineStart.X);
		SplineStart.Y = appRound(TransformedSplineStart.Y);
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
					TargetWidget = UIEditor->OwnerScene;
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





// EOL




