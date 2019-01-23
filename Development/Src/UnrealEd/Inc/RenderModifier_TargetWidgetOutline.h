/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __RENDERMODIFIER_TARGETWIDGETOUTLINE_H__
#define __RENDERMODIFIER_TARGETWIDGETOUTLINE_H__

#include "RenderModifier_SelectedWidgetsOutline.h"

/**
 * This class renders the overlay lines and dock handles for a dock link target widget.
 */
class FRenderModifier_TargetWidgetOutline : public FRenderModifier_SelectedWidgetsOutline
{
public:
	struct FWidgetDockHandle
	{
		UUIObject* Widget;
		EUIWidgetFace Face;
		FIntPoint Position;
		UBOOL bValidTarget;

		// Used for sorting
		static INT Compare(FWidgetDockHandle* A, FWidgetDockHandle* B)
		{
			if(A->Position.X < B->Position.X)
			{
				return -1;
			}
			else if( A->Position.X > B->Position.X )
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
	};

	struct FWidgetDockHandles
	{
		/** Redundant pointer to the Widget so we can quickly toggle visible of all of its 4 dock handles. */
		UUIObject* Widget;
		
		/** Widget Dock Handles */
		FWidgetDockHandle DockHandles[4];

		/** Whether or not this widget's dock handles are visible. */
		UBOOL bVisible;
	};

	/**
	* Constructor
	*
	* @param	InEditor	the editor window that holds the container to render the outline for
	*/
	FRenderModifier_TargetWidgetOutline( class WxUIEditorBase* InEditor );
	~FRenderModifier_TargetWidgetOutline();

	/** 
	 * Generates a list of all of the dock handles in the scene and sorts them by X position. 
	 *
	 * @param SourceWidget	The widget we are connecting a dock link from.
	 * @param SourceFace	The face we are connecting from.
	 */
	void GenerateSortedDockHandleList(UUIObject* SourceWidget, EUIWidgetFace SourceFace);

	/** Deallocates the sorted dock handle list. */
	void FreeSortedDockHandleList();

	/**
	 *	@return Returns the list of sorted dock handles for the scene.
	 */
	const TArray<FWidgetDockHandle*>* GetSortedDockHandleList() const
	{
		return &SortedDockHandles;
	}

	/**
	 * Generates a list of visible dock handles from the widgets passed in.  It combines the handles when they are too close to each other.
	 *
	 * @param Widgets A list of widgets to use for generating visible dock handles.
	 */
	void CalculateVisibleDockHandles(TArray<UUIObject*> &Widgets);

	/** Toggles all of the dock handles to not be visible */
	void ClearVisibleDockHandles();

	/**
	 *	Calculates dock handles which are close to the currently selected widget dock handle.
	 *  @return			The number of dock handles in the specified radius.
	 */
	TArray<FRenderModifier_TargetWidgetOutline::FWidgetDockHandle*>* CalculateCloseDockHandles();

	/**
	 * @return Returns the last generated list of close dock handles (CalculateCloseDockHandles must be called prior to this).
	 */
	TArray<FRenderModifier_TargetWidgetOutline::FWidgetDockHandle*>* GetCloseDockHandles()
	{
		return &CloseHandles;
	}

	/** Sets the selected dock face for highlighting. */
	virtual void SetSelectedDockFace(EUIWidgetFace InFace);

	/** Clears the selected dock face and dock widget. */
	virtual void ClearSelections();

	/**
	* Method for serializing UObject references that must be kept around through GC's.
	*
	* @param Ar The archive to serialize with
	*/
	virtual void Serialize(FArchive& Ar);

	/**
	* Renders all of the visible target dock handles.
	*/
	virtual void Render( FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas );

protected:

	/** 
	 * Renders a docking handle for the specified widget.
	 * 
	 * @param Widget	Widget to render the docking handle for.
	 * @param Face		Which face we are drawing the handle for.
	 * @param RI		Rendering interface to use for drawing the handle.
	 */
	virtual void RenderDockHandle( const FWidgetDockHandle& DockHandle, FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas );

	TArray<FWidgetDockHandles*> SceneWidgetDockHandles;	
	TArray<FWidgetDockHandle*> SortedDockHandles;	
	TArray<FWidgetDockHandle*> CloseHandles;
};

#endif
