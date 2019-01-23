/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnObjectEditor.h"
#include "UnUIEditor.h"
#include "ScopedTransaction.h"

#include "MouseTool_FocusChainCreate.h"

#include "UIWidgetTool_FocusChain.h"
#include "ScopedObjectStateChange.h"

/* ==========================================================================================================
WxFocusChainToolProxy
========================================================================================================== */
BEGIN_EVENT_TABLE(WxFocusChainToolProxy, wxWindow)
	EVT_MENU_RANGE(IDM_UI_DOCKTARGET_BEGIN, IDM_UI_DOCKTARGET_END, WxFocusChainToolProxy::OnContext_ConnectFocusChainLink)
	
	EVT_MENU(IDM_UI_SETFOCUSCHAINTONULL, WxFocusChainToolProxy::OnContext_SetFocusChainToNull)
	EVT_MENU(IDM_UI_SETALLFOCUSCHAINTONULL, WxFocusChainToolProxy::OnContext_SetAllFocusChainsToNull)

	EVT_MENU(IDM_UI_BREAKFOCUSCHAINLINK, WxFocusChainToolProxy::OnContext_BreakFocusChainLink)
	EVT_MENU(IDM_UI_BREAKALLFOCUSCHAINLINKS, WxFocusChainToolProxy::OnContext_BreakAllFocusChainLinks)
END_EVENT_TABLE()

WxFocusChainToolProxy::WxFocusChainToolProxy(wxWindow* InParent, FUIWidgetTool_FocusChain* InParentTool) : 
wxWindow(InParent, wxID_ANY),
ParentTool(InParentTool)
{

}

/**
 * Breaks the focus chain link starting from the handle that was right clicked.
 */
void WxFocusChainToolProxy::OnContext_BreakFocusChainLink(wxCommandEvent& Event)
{
	FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_BreakFocusChainLink")) );
	WxUIHandleContextMenu* Menu = wxDynamicCast(Event.GetEventObject(), WxUIHandleContextMenu);

	UUIObject* Widget = Menu->ActiveWidget;
	EUIWidgetFace Face = Menu->ActiveFace;

	FScopedObjectStateChange NavTargetChangeNotification(Widget);
	Widget->SetForcedNavigationTarget(Face, NULL);
}

/**
 * Breaks all of the focus chain links connected to the current object.
 */
void WxFocusChainToolProxy::OnContext_BreakAllFocusChainLinks(wxCommandEvent& Event)
{
	
	{
		FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_BreakFocusChainLink")) );
		WxUIHandleContextMenu* Menu = wxDynamicCast(Event.GetEventObject(), WxUIHandleContextMenu);

		UUIObject* Widget = Menu->ActiveWidget;

		FScopedObjectStateChange NavTargetChangeNotification(Widget);
		Widget->SetForcedNavigationTarget(UIFACE_Top, NULL, FALSE);
		Widget->SetForcedNavigationTarget(UIFACE_Bottom, NULL, FALSE);
		Widget->SetForcedNavigationTarget(UIFACE_Left, NULL, FALSE);
		Widget->SetForcedNavigationTarget(UIFACE_Right, NULL, FALSE);
	}
}

/**
 * Sets a focus chain to NULL.
 */
void WxFocusChainToolProxy::OnContext_SetFocusChainToNull(wxCommandEvent& Event)
{
	FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_BreakFocusChainLink")) );
	WxUIHandleContextMenu* Menu = wxDynamicCast(Event.GetEventObject(), WxUIHandleContextMenu);

	UUIObject* Widget = Menu->ActiveWidget;
	EUIWidgetFace Face = Menu->ActiveFace;

	FScopedObjectStateChange NavTargetChangeNotification(Widget);
	Widget->SetForcedNavigationTarget(Face, NULL, TRUE);
}

/**
 * Sets all focus chains to NULL.
 */
void WxFocusChainToolProxy::OnContext_SetAllFocusChainsToNull(wxCommandEvent& Event)
{
	{
		FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_BreakFocusChainLink")) );
		WxUIHandleContextMenu* Menu = wxDynamicCast(Event.GetEventObject(), WxUIHandleContextMenu);

		UUIObject* Widget = Menu->ActiveWidget;

		FScopedObjectStateChange NavTargetChangeNotification(Widget);
		Widget->SetForcedNavigationTarget(UIFACE_Top, NULL, TRUE);
		Widget->SetForcedNavigationTarget(UIFACE_Left, NULL, TRUE);
		Widget->SetForcedNavigationTarget(UIFACE_Right, NULL, TRUE);
		Widget->SetForcedNavigationTarget(UIFACE_Bottom, NULL, TRUE);
	}
}

/**
 * Connects the selected dock link.
 */
void WxFocusChainToolProxy::OnContext_ConnectFocusChainLink(wxCommandEvent& Event)
{
	// Get the HandleIdx from the event's ID, and then make sure it is still a valid index into the sorted dock handle array.
	// If it is, set the dock target for the selected widget using that info.
	const INT WidgetIdx = Event.GetId() - IDM_UI_DOCKTARGET_BEGIN;
	const TArray<UUIObject*>& TargetWidgets = ParentTool->TargetRenderModifier.GetTargetWidgets();
	const UBOOL bValidId = WidgetIdx < TargetWidgets.Num();
	if( bValidId == TRUE)
	{
		const UUIObject* Widget = TargetWidgets(WidgetIdx);
		UUIObject* SelectedWidget = ParentTool->RenderModifier.GetSelectedWidget();
		const EUIWidgetFace SelectedFace = ParentTool->RenderModifier.GetSelectedDockHandle();

		if(SelectedWidget != NULL && SelectedFace != UIFACE_MAX)
		{
			FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_ConnectFocusChainLink")) );
			FScopedObjectStateChange NavTargetChangeNotification(SelectedWidget);
			SelectedWidget->SetForcedNavigationTarget(SelectedFace,const_cast<UUIObject*>(Widget));
	    }
    }
}




/* ==========================================================================================================
FUIWidgetTool_FocusChain
========================================================================================================== */
FUIWidgetTool_FocusChain::FUIWidgetTool_FocusChain(class WxUIEditorBase* InEditor) : 
FUIWidgetTool(InEditor),
RenderModifier(InEditor),
TargetRenderModifier(InEditor)
{

}


/** Called when this tool is 'activated'. */
void FUIWidgetTool_FocusChain::EnterToolMode()
{
	// This order matters, the target must be inserted before the normal modifier so that it is drawn on top.
	UIEditor->ObjectVC->AddRenderModifier(&TargetRenderModifier, 0);
	UIEditor->ObjectVC->AddRenderModifier(&RenderModifier, 0);

	// Add a proxy to receive wx events
	MenuProxy = new WxFocusChainToolProxy(UIEditor, this);
}

/** Called when this tool is 'deactivated'. */
void FUIWidgetTool_FocusChain::ExitToolMode()
{
	UIEditor->ObjectVC->RemoveRenderModifier(&TargetRenderModifier);
	UIEditor->ObjectVC->RemoveRenderModifier(&RenderModifier);

	// We need to remove the menu proxy from its parent so we can be deleted safely.
	if(MenuProxy != NULL)
	{
		MenuProxy->Destroy();
		MenuProxy = NULL;
	}
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
UBOOL FUIWidgetTool_FocusChain::InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed)
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
				if(HitProxy->IsA(HUIFocusChainHandleHitProxy::StaticGetType()) == TRUE && RenderModifier.GetSelectedWidget() != NULL)
				{
					HUIFocusChainHandleHitProxy* DockHandleProxy = (HUIFocusChainHandleHitProxy*)(HitProxy);

					UIEditor->ObjectVC->MouseTool= new FMouseTool_FocusChainCreate(UIEditor->ObjectVC, UIEditor, this);
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

	return bConsumeInput;
}

/**
 * Called when the user clicks anywhere in the viewport.
 *
 * @param	HitProxy	the hitproxy that was clicked on (may be null if no hit proxies were clicked on)
 * @param	Click		contains data about this mouse click
 * @return				Whether or not we consumed the click.
 */
UBOOL FUIWidgetTool_FocusChain::ProcessClick (HHitProxy* HitProxy, const FObjectViewportClick& Click)
{
	UBOOL bHandledClick = FALSE;

	if( Click.GetKey() == KEY_RightMouseButton)
	{
		// See if they right clicked on a widget dock handle, if so, display the dock handle
		// context menu.
		if(HitProxy != NULL)
		{
			if(HitProxy->IsA(HUIFocusChainHandleHitProxy::StaticGetType()) == TRUE)
			{
				HUIFocusChainHandleHitProxy* DockHandleProxy = (HUIFocusChainHandleHitProxy*)(HitProxy);

				ShowFocusChainHandleContextMenu();
				bHandledClick = TRUE;
			}
		}
	}

	return bHandledClick;
}

/**
 * Called when the user moves the mouse while this viewport is capturing mouse input (i.e. dragging)
 */
void FUIWidgetTool_FocusChain::InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime)
{
	TargetRenderModifier.ClearSelections();
}

/**
 * Called when the user moves the mouse while this viewport is not capturing mouse input (i.e. not dragging).
 */
void FUIWidgetTool_FocusChain::MouseMove(FViewport* Viewport, INT X, INT Y)
{
	if(UIEditor->ObjectVC->MouseTool != NULL)
	{
		// If we are dragging, we keep track of a 'target' widget and face for the mouse tools.
		UBOOL bClearTarget = TRUE;

		HHitProxy* HitProxy = Viewport->GetHitProxy(X, Y);

		if(HitProxy != NULL)
		{
			if(HitProxy->IsA(HObject::StaticGetType()) == TRUE)
			{
				HUIHitProxy* UIProxy = (HUIHitProxy*)HitProxy;
				UUIObject* Widget = Cast<UUIObject>(UIProxy->UIObject);
				
				const UBOOL bHasSelectedWidget = RenderModifier.GetSelectedWidget() != NULL;
				const UBOOL bHasSelectedFace = RenderModifier.GetSelectedDockHandle() != UIFACE_MAX;
				if(Widget != NULL && bHasSelectedWidget && bHasSelectedFace)
				{
					// Build a list of widgets currently under the mouse cursor, we will add this to our 
					// target widget outline container.
					const UUIObject* SelectedWidget = RenderModifier.GetSelectedWidget();

					FLOAT ViewportX, ViewportY;
					UIEditor->ObjectVC->GetViewportPositionFromMousePosition(X, Y, ViewportX, ViewportY);

					const FVector2D Point(ViewportX, ViewportY);
					TArray<UUIObject*> Children = UIEditor->OwnerContainer->GetChildren( TRUE );
					TArray<UUIObject*> HitWidgets;

					for(INT ChildIdx = 0; ChildIdx < Children.Num(); ChildIdx++)
					{
						UUIObject* ChildWidget = Children(ChildIdx);
						const UBOOL bUnderMouse = ChildWidget->ContainsPoint(Point);

						// See if this widget is under the mouse, not the selected widget and it can receive focus.
						if( bUnderMouse && SelectedWidget != ChildWidget && !ChildWidget->IsPrivateBehaviorSet( UCONST_PRIVATE_NotFocusable ) )
						{
							HitWidgets.AddItem(ChildWidget);
						}
					}

					
					// Make sure we have atleast one target widget under us that isn't the selected widget.
					if(HitWidgets.Num() > 0)
					{	
							
						TargetRenderModifier.SetTargetWidgets(HitWidgets);
						TargetRenderModifier.SetSelectedWidget(Widget);

						bClearTarget = FALSE;
					}
					
				}
			}
		}

		// Clear the target widget if nothing was moused over.
		if( bClearTarget == TRUE )
		{	
			TargetRenderModifier.ClearSelections();
		}

	}
	else
	{
		// Use hitproxy's to see if we are over any dock handles, if we are then set the
		// render modifier state to reflect the fact that the handle is highlighted.
		HHitProxy* HitProxy = Viewport->GetHitProxy(X, Y);
		UBOOL bClearSelections = TRUE;

		if(HitProxy != NULL)
		{
			if(HitProxy->IsA(HUIFocusChainHandleHitProxy::StaticGetType()) == TRUE)
			{
				HUIFocusChainHandleHitProxy* DockHandleProxy = (HUIFocusChainHandleHitProxy*)(HitProxy);

				RenderModifier.SetSelectedWidget(DockHandleProxy->UIObject);
				RenderModifier.SetSelectedDockFace(DockHandleProxy->DockFace);

				bClearSelections = FALSE;
			}
		}

		if(bClearSelections == TRUE)
		{
			RenderModifier.ClearSelections();
		}
	}
}

/**
 * Method for serializing UObject references that must be kept around through GC's.
 *
 * @param Ar The archive to serialize with
 */
void FUIWidgetTool_FocusChain::Serialize(FArchive& Ar)
{
	RenderModifier.Serialize(Ar);
	TargetRenderModifier.Serialize(Ar);
}

/**
 * Called when the user right-clicks on a dock handle in the viewport, shows a context menu for dock handles.
 */
void FUIWidgetTool_FocusChain::ShowFocusChainHandleContextMenu()
{
	UUIObject* SelectedWidget = RenderModifier.GetSelectedWidget();
	EUIWidgetFace SelectedFace = RenderModifier.GetSelectedDockHandle();

	const UBOOL bValidWidget = (SelectedWidget != NULL);
	const UBOOL bValidFace = (SelectedFace != UIFACE_MAX);

	if(bValidWidget && bValidFace)
	{
		UIEditor->SynchronizeSelectedWidgets(UIEditor);

		const UUIObject* TargetWidget = SelectedWidget->GetNavigationTarget(SelectedFace, NAVLINK_Manual);
		const UUIObject* AutomaticWidget = SelectedWidget->GetNavigationTarget(SelectedFace, NAVLINK_Automatic);
		const UUIObject* MaxTargetWidget = SelectedWidget->GetNavigationTarget(SelectedFace, NAVLINK_MAX);
		const UBOOL bOverrideTarget = (AutomaticWidget != NULL && MaxTargetWidget == NULL);
		WxUIHandleContextMenu menu(MenuProxy);

		menu.Create( SelectedWidget, SelectedFace );
		
		menu.Append( IDM_UI_SETFOCUSCHAINTONULL, *LocalizeUI(TEXT("UIEditor_MenuItem_SetFocusChainToNull")) );
		menu.Enable( IDM_UI_SETFOCUSCHAINTONULL, bOverrideTarget == FALSE);
		menu.Append( IDM_UI_SETALLFOCUSCHAINTONULL, *LocalizeUI(TEXT("UIEditor_MenuItem_SetAllFocusChainToNull")) );
		menu.AppendSeparator();

		menu.Append( IDM_UI_BREAKFOCUSCHAINLINK, *LocalizeUI(TEXT("UIEditor_MenuItem_BreakFocusChainLink")) );

		// If we do not have a focus chain link on this focus chain handle, then disable the break focus chain link item.
		// Otherwise, enable the item and generate a label for the menu item that says which widget/face we are breaking the link to.
	

		if(TargetWidget == NULL && bOverrideTarget == FALSE)
		{
			menu.SetLabel(IDM_UI_BREAKFOCUSCHAINLINK, *LocalizeUI(TEXT("UIEditor_MenuItem_BreakFocusChainLink")));
			menu.Enable(IDM_UI_BREAKFOCUSCHAINLINK, false);
		}
		else
		{
			FString MenuLabel = FString::Printf(*LocalizeUI(TEXT("UIEditor_MenuItem_BreakFocusChainLinkTo")), *TargetWidget->GetName());
			menu.SetLabel(IDM_UI_BREAKFOCUSCHAINLINK, *MenuLabel);
			menu.Enable(IDM_UI_BREAKFOCUSCHAINLINK, true);
		}

		menu.Append( IDM_UI_BREAKALLFOCUSCHAINLINKS, *LocalizeUI(TEXT("UIEditor_MenuItem_BreakAllFocusChainLinks")) );


		FTrackPopupMenu tpm( MenuProxy, &menu );
		tpm.Show();
	}
}

