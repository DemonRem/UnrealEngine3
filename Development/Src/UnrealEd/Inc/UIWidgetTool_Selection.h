/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __UIWIDGETTOOL_SELECTION_H__
#define __UIWIDGETTOOL_SELECTION_H__

#include "RenderModifier_SelectedWidgetsOutline.h"
#include "RenderModifier_TargetWidgetOutline.h"

#include "UIWidgetTool.h"


class WxSelectionToolProxy : public wxWindow
{
public:
	WxSelectionToolProxy(wxWindow* InParent, class FUIWidgetTool_Selection* InParentTool, class WxUIEditorBase* InEditor);

protected:
	/**
	 * Breaks the dock link starting from the handle that was right clicked.
	 */
	void OnContext_BreakDockLink( wxCommandEvent& Event );

	/**
	 * Breaks all of the dock links for the widget that was right clicked.
	 */
	void OnContext_BreakAllDockLinks( wxCommandEvent& Event );

	/**
	 * Connects the selected dock link.
	 */
	void OnContext_ConnectDockLink( wxCommandEvent& Event );

	/**
	 * Shows the docking editor.
	 */
	void OnContext_DockingEditor( wxCommandEvent& Event );

	/** Tool that spawned this menu. */
	class FUIWidgetTool_Selection* ParentTool;

	/** UIEditor that owns the parent tool. */
	class WxUIEditorBase* UIEditor;

	DECLARE_EVENT_TABLE()
};

/**
 *	UI Editor Widget Tool, this is the default manipulate widget tool.
 */
class FUIWidgetTool_Selection : public FUIWidgetTool, public FSerializableObject
{
public:

	FUIWidgetTool_Selection(class WxUIEditorBase* InEditor);
	virtual ~FUIWidgetTool_Selection() {}

	/**
	 * Method for serializing UObject references that must be kept around through GC's.
	 * 
	 * @param Ar The archive to serialize with
	 */
	virtual void Serialize(FArchive& Ar);

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

	/** displays outlines and docking handles around all selected widgets. */
	class FRenderModifier_SelectedWidgetsOutline	SelectedWidgetsOutline;

	/** Displays docking handles and an outline for a target widget. */
	class FRenderModifier_TargetWidgetOutline		TargetWidgetOutline;

	/** Proxy to handle receiving wx events. */
	class WxSelectionToolProxy* MenuProxy;

protected:
	/**
	 * Called when the user right-clicks on a dock handle in the viewport, shows a context menu for dock handles.
	 */
	void ShowDockHandleContextMenu();
};



#endif

