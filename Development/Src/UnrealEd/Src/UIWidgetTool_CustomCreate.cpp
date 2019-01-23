/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnObjectEditor.h"
#include "UnUIEditor.h"

#include "MouseTool_DockLinkCreate.h"
#include "RenderModifier_SelectedWidgetsOutline.h"
#include "UIWidgetTool_CustomCreate.h"



FUIWidgetTool_CustomCreate::FUIWidgetTool_CustomCreate(class WxUIEditorBase* InEditor, INT InWidgetIdx) : 
FUIWidgetTool_Selection(InEditor),
WidgetIdx(InWidgetIdx)
{

}

/** Called when this tool is 'activated'. */
void FUIWidgetTool_CustomCreate::EnterToolMode()
{
	FUIWidgetTool_Selection::EnterToolMode();
}

/** Called when this tool is 'deactivated'. */
void FUIWidgetTool_CustomCreate::ExitToolMode()
{
	FUIWidgetTool_Selection::ExitToolMode();
}


FMouseTool* FUIWidgetTool_CustomCreate::CreateMouseTool() const
{
	FMouseTool* MouseTool = NULL;
	FEditorObjectViewportClient* ViewportClient = UIEditor->ObjectVC;
	const UBOOL bNoModifiers = (!ViewportClient->Input->IsAltPressed() && !ViewportClient->Input->IsCtrlPressed() && !ViewportClient->Input->IsShiftPressed());
	const UBOOL bLeftMouseButtonOnly = (ViewportClient->Viewport->KeyState(KEY_LeftMouseButton) == TRUE) && (ViewportClient->Viewport->KeyState(KEY_RightMouseButton) == FALSE);

	if( bLeftMouseButtonOnly == TRUE && bNoModifiers == TRUE)
	{
		MouseTool =  new FMouseTool_ObjectDragCreate(ViewportClient, UIEditor);
	}

	return MouseTool;
}

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
UBOOL FUIWidgetTool_CustomCreate::InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed)
{
	UBOOL bConsumeInput = FALSE;

	// See if we are interacting with a docking widget handle, if so we handle all input here.
	if( Key == KEY_LeftMouseButton )
	{
		if(Event == IE_Pressed)
		{
			const INT MouseX = Viewport->GetMouseX();
			const INT MouseY = Viewport->GetMouseY();

			// See if they clicked on a dock handle, if so, let start the link dragging process.
			HHitProxy* HitProxy = Viewport->GetHitProxy(MouseX, MouseY);

			if(HitProxy != NULL)
			{
				if(HitProxy->IsA(HUIDockHandleHitProxy::StaticGetType()) == TRUE && SelectedWidgetsOutline.GetSelectedWidget() != NULL)
				{
					HUIDockHandleHitProxy* DockHandleProxy = (HUIDockHandleHitProxy*)(HitProxy);

					UIEditor->ObjectVC->MouseTool= new FMouseTool_DockLinkCreate(UIEditor->ObjectVC, UIEditor, this);

					UIEditor->TargetTooltip->ClearStringList();

					// Tell the MouseTool to start dragging.
					FLOAT HitX;
					FLOAT HitY;
					UIEditor->ObjectVC->GetViewportPositionFromMousePosition(MouseX, MouseY, HitX, HitY);
					UIEditor->ObjectVC->MouseTool->StartDrag(UIEditor->ObjectVC, FVector(HitX, HitY, 0));

					bConsumeInput = TRUE;
				}
			}
		}
	}

	if(bConsumeInput == FALSE)
	{
		bConsumeInput = FUIWidgetTool_Selection::InputKey(Viewport, ControllerId, Key, Event, AmountDepressed);
	}

	return bConsumeInput;
}

/**
 * Called when the user clicks anywhere in the viewport.
 *
 * @param	HitProxy	the hitproxy that was clicked on (may be null if no hit proxies were clicked on)
 * @param	Click		contains data about this mouse click
 * @return				Whether or not we consumed the click.
 */
UBOOL FUIWidgetTool_CustomCreate::ProcessClick (HHitProxy* HitProxy, const FObjectViewportClick& Click)
{
	UBOOL bHandledClick = FALSE;


	if( Click.GetKey() == KEY_RightMouseButton)
	{
		// See if they right clicked on a widget dock handle, if so, display the dock handle
		// context menu.
		if(HitProxy != NULL)
		{
			if(HitProxy->IsA(HUIDockHandleHitProxy::StaticGetType()) == TRUE)
			{
				HUIDockHandleHitProxy* DockHandleProxy = (HUIDockHandleHitProxy*)(HitProxy);

				ShowDockHandleContextMenu();
				bHandledClick = TRUE;
			}
		}
	}

	return bHandledClick;
}

/**
 * Called when the user moves the mouse while this viewport is capturing mouse input (i.e. dragging)
 */
void FUIWidgetTool_CustomCreate::InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime)
{
	TargetWidgetOutline.ClearSelections();
}

/**
 * Called when the user moves the mouse while this viewport is not capturing mouse input (i.e. not dragging).
 */
void FUIWidgetTool_CustomCreate::MouseMove(FViewport* Viewport, INT X, INT Y)
{
	FUIWidgetTool_Selection::MouseMove(Viewport, X, Y);
}

