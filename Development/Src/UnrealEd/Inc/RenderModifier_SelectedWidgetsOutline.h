/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __RENDERMODIFIER_SELECTEDWIDGETSOUTLINE_H__
#define __RENDERMODIFIER_SELECTEDWIDGETSOUTLINE_H__

// Docking Handle Constants
namespace FDockingConstants
{
	static const INT HandleRadius = 8;
	static const INT HandleGap = 8;
	static const INT HandleNumSides = 16;
};

/**
 * This class renders the overlay lines and dock handles for selected widgets. 
 */
class FRenderModifier_SelectedWidgetsOutline : public FUIEditor_RenderModifier
{
public:
	/**
	 * Constructor
	 *
	 * @param	InEditor	the editor window that holds the container to render the outline for
	 */
	FRenderModifier_SelectedWidgetsOutline( class WxUIEditorBase* InEditor );

	/**
	 * Renders the outline for the top-level object of a UI editor window.
	 */
	virtual void Render( FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas );

	/** Sets the selected widget for highlighting. */
	void SetSelectedWidget(UUIObject* InWidget);

	/** Sets the selected dock face for highlighting. */
	virtual void SetSelectedDockFace(EUIWidgetFace InFace);

	/** Clears the selected dock face and dock widget. */
	virtual void ClearSelections();
	/**
	 *	@return Returns the currently selected(highlighted) widget, if any.
	 */
	UUIObject* GetSelectedWidget() const
	{
		return SelectedWidget;
	}

	/**
	 *	@return Returns the currently selected(highlighted) dock handle.
	 */
	EUIWidgetFace GetSelectedDockHandle() const
	{
		return SelectedDockHandle;
	}

	/**
	 * Method for serializing UObject references that must be kept around through GC's.
	 *
	 * @param Ar The archive to serialize with
	 */
	virtual void Serialize(FArchive& Ar);

	/** 
	 * Generates a dock handle position for a object.
	 * 
	 * @param Widget			Widget we are getting a dock handle position for.
	 * @param InFace			Which face we are getting a position for.
	 * @param HandleRadius		Radius of the handle.
	 * @param HandleGap			Gap between the handle and the object's edge.
	 * @param OutPosition	Position of the dock handle.
	 */
	void GetDockHandlePosition(UUIObject* Widget, EUIWidgetFace InFace, INT HandleRadius, INT HandleGap, FIntPoint& OutPosition);

	/** 
	 * Generates a dock handle position for a scene.
	 * 
	 * @param Scene				Scene we are getting a dock handle position for.
	 * @param InFace			Which face we are getting a position for.
	 * @param HandleRadius		Radius of the handle.
	 * @param HandleGap			Gap between the handle and the object's edge.
	 * @param OutPosition		Position of the dock handle.
	 */
	void GetDockHandlePosition(UUIScene* Scene, EUIWidgetFace InFace, INT HandleRadius, INT HandleGap, FIntPoint& OutPosition);

	/** 
	 * Generates a handle position for a object.
	 * 
	 * @param RenderBounds	Render bounds of the object we are getting a dock handle for.
	 * @param InFace		Which face we are getting a position for.
	 * @param HandleRadius	Radius of the handle.
	 * @param HandleGap		Gap between the handle and the object's edge.
	 * @param OutPosition	Position of the dock handle.
	 */
	void GetHandlePosition(const FLOAT* RenderBounds, EUIWidgetFace InFace, INT HandleRadius, INT HandleGap, FIntPoint& OutPosition);

protected:

	/** 
	 * Renders a docking handle for the specified widget.
	 * 
	 * @param Widget	Widget to render the docking handle for.
	 * @param Face		Which face we are drawing the handle for.
	 * @param RI		Rendering interface to use for drawing the handle.
	 */
	virtual void RenderDockHandle( UUIObject* Widget, EUIWidgetFace InFace, FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas );



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
	void RenderDockLink( UUIObject* Widget, EUIWidgetFace InFace, FCanvas* Canvas, const FVector& Start, INT DockHandleRadius, INT DockHandleGap );

	/** Member variable to store which widget is currently moused over. */
	UUIObject* SelectedWidget;

	/** Member variable to store which dock handle is currently moused over. */
	EUIWidgetFace SelectedDockHandle;
};



#endif

