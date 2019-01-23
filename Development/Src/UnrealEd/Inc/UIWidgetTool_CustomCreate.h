/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __UIWIDGETTOOL_CUSTOMCREATE_H__
#define __UIWIDGETTOOL_CUSTOMCREATE_H__

#include "UIWidgetTool.h"
#include "UIWidgetTool_Selection.h"


/**
 *	UI Editor Widget Tool, this is the widget creation tool.
 */
class FUIWidgetTool_CustomCreate : public FUIWidgetTool_Selection
{
public:

	FUIWidgetTool_CustomCreate(class WxUIEditorBase* InEditor, INT InWidgetIdx);

	/** Called when this tool is 'activated'. */
	virtual void EnterToolMode();

	/** Called when this tool is 'deactivated'. */
	virtual void ExitToolMode();

	/** Creates a mouse tool for the current widget mode. */
	virtual FMouseTool* CreateMouseTool() const;

	/**
	 * Check a key event received by the viewport.
	 * If the viewport client uses the event, it should return true to consume it.
	 * @param	Viewport - The viewport which the key event is from.
	 * @param	ControllerId - The controller which the key event is from.
	 * @param	Key - The name of the key which an event occured for.
	 * @param	Event - The type of event which occured.
	 * @param	AmountDepressed - For analog keys, the depression percent.
	 * @return	True to consume the key event, false to pass it on.
	 */
	virtual UBOOL InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed);

	/**
	 * Called when the user clicks anywhere in the viewport.
	 *
	 * @param	HitProxy	the hitproxy that was clicked on (may be null if no hit proxies were clicked on)
	 * @param	Click		contains data about this mouse click
	 * @return				Whether or not we consumed the click.
	 */
	virtual UBOOL ProcessClick (HHitProxy* HitProxy, const FObjectViewportClick& Click);

	/**
	 * Called when the user moves the mouse while this viewport is capturing mouse input (i.e. dragging)
	 */
	virtual void InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime);

	/**
	 * Called when the user moves the mouse while this viewport is not capturing mouse input (i.e. not dragging).
	 */
	virtual void MouseMove(FViewport* Viewport, INT X, INT Y);

protected:

	/** Widget index, this is the widget type we will be creating. */
	INT WidgetIdx;
};



#endif

