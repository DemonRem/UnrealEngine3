/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __MOUSETOOL_DOCKLINKCREATE_H__
#define __MOUSETOOL_DOCKLINKCREATE_H__

/**
 * This class allows the user to drag a link from a dock handle to another widget's dock handle to create a 
 * docking relationship.
 */
class FMouseTool_DockLinkCreate : public FMouseTool
{
public:
	/** Constructor */
	FMouseTool_DockLinkCreate( FEditorObjectViewportClient* InObjectVC, WxUIEditorBase* InUIEditor, class FUIWidgetTool_Selection* InParentWidgetTool );

	/** Destructor */
	virtual ~FMouseTool_DockLinkCreate();

	/**
	 * Creates a instance of a widget based on the current tool mode and starts the drag process.
	 *
	 * @param	InViewportClient	the viewport client to draw the selection box in
	 * @param	Start				Where the mouse was when the drag started
	 */
	virtual void StartDrag(FEditorLevelViewportClient* InViewportClient, const FVector& InStart);

	/**
	 * Ends the mouse drag movement.  Called when the user releases the mouse button while this drag tool is active.
	 */
	virtual void EndDrag();

	/**
	 *	Callback allowing the drag tool to render some data to the viewport.
	 */
	virtual void Render(FCanvas* Canvas);

	/** 
	 * Generates a string that describes the current drag operation to the user.
	 * 
	 * This version blanks out the string because we do not want to display any info.
	 *
	 * @param OutString		Storage class for the generated string, should have the current delta in it in some way.
	 */
	virtual void GetDeltaStatusString(FString &OutString) const
	{
		OutString = TEXT("");
	}
protected:

	/** pointer to the UI editor object so we can create widgets */
	WxUIEditorBase* UIEditor;

	/** Widget we are currently dragging. */
	UUIObject* DragWidget;

	/** Dock face that we are dragging from. */
	EUIWidgetFace DockFace;

	/** Starting position of the link spline. */
	FIntPoint SplineStart;

	/** Current target widget for the link. */
	UUIObject* TargetWidget;

	/** Current target widget dock face for the link. */
	EUIWidgetFace TargetFace;

	class FUIWidgetTool_Selection* ParentWidgetTool;
};



#endif

