/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __RENDERMODIFIER_FOCUSCHAINHANDLES_H__
#define __RENDERMODIFIER_FOCUSCHAINHANDLES_H__

#include "RenderModifier_SelectedWidgetsOutline.h"

/**
 * This class renders the overlay lines and focus chain handles for a selected widget.
 */
class FRenderModifier_FocusChainHandles : public FRenderModifier_SelectedWidgetsOutline
{
public:
	/**
	 * Constructor
	 *
	 * @param	InEditor	the editor window that holds the container to render the outline for
	 */
	FRenderModifier_FocusChainHandles( class WxUIEditorBase* InEditor );

	/**
	 * Renders the outline for the top-level object of a UI editor window.
	 */
	virtual void Render( FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas );

	/** Sets the selected dock face for highlighting. */
	virtual void SetSelectedDockFace(EUIWidgetFace InFace);

private:
	/** 
	 * Renders a focus chain link for ths specified face.
	 * 
	 * @param Widget				Widget to render the docking handle for.
	 * @param InFace				Which face we are drawing the handle for.
	 * @param RI					Rendering interface to use for drawing the handle.
	 * @param Start					Starting position for the link.
	 */
	void RenderLink( UUIObject* Widget, EUIWidgetFace InFace, FCanvas* Canvas, const FVector& InStart);

	/** 
	 * Renders a focus chain handle for the specified widget.
	 * 
	 * @param Widget	Widget to render the focus chain handle for.
	 * @param Face		Which face we are drawing the handle for.
	 * @param RI		Rendering interface to use for drawing the handle.
	 */
	virtual void RenderFocusChainHandle( UUIObject* Widget, EUIWidgetFace InFace, FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas );

};


#endif

