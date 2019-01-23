/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __UIWIDGETTOOL_FOCUSCHAIN_H__
#define __UIWIDGETTOOL_FOCUSCHAIN_H__

#include "UIWidgetTool.h"
#include "RenderModifier_FocusChainHandles.h"
#include "RenderModifier_FocusChainTarget.h"

class WxFocusChainToolProxy : public wxWindow
{
public:
	WxFocusChainToolProxy(wxWindow* InParent, class FUIWidgetTool_FocusChain* InParentTool);

protected:
	/**
	 * Breaks the focus chain link starting from the handle that was right clicked.
	 */
	void OnContext_BreakFocusChainLink(wxCommandEvent& Event);

	/**
	 * Breaks all of the focus chain links connected to the current object.
	 */
	void OnContext_BreakAllFocusChainLinks(wxCommandEvent& Event);

	/**
	 * Sets a focus chain to NULL.
	 */
	void OnContext_SetFocusChainToNull(wxCommandEvent& Event);
	
	/**
 	 * Sets all focus chains to NULL.
	 */
	void WxFocusChainToolProxy::OnContext_SetAllFocusChainsToNull(wxCommandEvent& Event);

	/**
	 * Connects the selected focus chain link.
	 */
	void OnContext_ConnectFocusChainLink( wxCommandEvent& Event );

	/** Tool that spawned this menu. */
	class FUIWidgetTool_FocusChain* ParentTool;

	DECLARE_EVENT_TABLE()
};


/**
 *	UI Editor Widget Tool, this is the default manipulate widget tool.
 */
class FUIWidgetTool_FocusChain : public FUIWidgetTool, public FSerializableObject
{
public:

	FUIWidgetTool_FocusChain(class WxUIEditorBase* InEditor);

	/** Called when this tool is 'activated'. */
	virtual void EnterToolMode();

	/** Called when this tool is 'deactivated'. */
	virtual void ExitToolMode();

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


	/**
	 * Method for serializing UObject references that must be kept around through GC's.
	 *
	 * @param Ar The archive to serialize with
	 */
	virtual void Serialize(FArchive& Ar);


	/** Render modifier that draws the focus chain handles for selected widgets. */
	class FRenderModifier_FocusChainHandles RenderModifier;

	/** Render modifier that displays which widgets the focus chain mouse tool is targeting. */
	class FRenderModifier_FocusChainTarget TargetRenderModifier;

	/** Proxy to receive wx menu events. */
	WxFocusChainToolProxy* MenuProxy;

protected:
	/**
	 * Called when the user right-clicks on a dock handle in the viewport, shows a context menu for dock handles.
	 */
	void ShowFocusChainHandleContextMenu();
};



#endif