/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __RENDERMODIFIER_FOCUSCHAINTARGET_H__
#define __RENDERMODIFIER_FOCUSCHAINTARGET_H__

#include "RenderModifier_SelectedWidgetsOutline.h"

/**
 * This class renders the overlay lines and dock handles for a dock link target widget.
 */
class FRenderModifier_FocusChainTarget : public FUIEditor_RenderModifier
{
public:

	/**
	 * Constructor
	 *
	 * @param	InEditor	the editor window that holds the container to render the outline for
	 */
	FRenderModifier_FocusChainTarget( class WxUIEditorBase* InEditor );


	/**
	 * Sets the list of widgets that we currently have as targets (ie under the mouse cursor)
	 * @param Widgets	Currently targeted widgets
	 */
	void SetTargetWidgets(TArray<UUIObject*> &Widgets);

	/** Sets the selected widget for highlighting. */
	void SetSelectedWidget(UUIObject* InWidget)
	{
		SelectedWidget = InWidget;
	}

	/** @return Returns the top-most widget that is currently under the mouse cursor. */
	const UUIObject* GetSelectedWidget() const
	{
		return SelectedWidget;
	}

	/** @return Returns all of the widgets that are currently under the mouse cursor. */
	TArray<UUIObject*>& GetTargetWidgets()
	{
		return TargetWidgets;
	}

	/** Clears the selected widgets. */
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

	/** Current object directly under the mouse cursor (The hitproxy object). */
	UUIObject* SelectedWidget;

	/** All objects under the mouse cursor. */
	TArray<UUIObject*> TargetWidgets;
};

#endif
