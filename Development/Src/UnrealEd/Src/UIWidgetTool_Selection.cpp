/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnObjectEditor.h"
#include "UnUIEditor.h"
#include "ScopedTransaction.h"

#include "MouseTool_DockLinkCreate.h"

#include "UIWidgetTool_Selection.h"
#include "ScopedObjectStateChange.h"

/* ==========================================================================================================
WxSelectionToolProxy
========================================================================================================== */
BEGIN_EVENT_TABLE(WxSelectionToolProxy, wxWindow)
	EVT_MENU_RANGE(IDM_UI_DOCKTARGET_BEGIN, IDM_UI_DOCKTARGET_END, WxSelectionToolProxy::OnContext_ConnectDockLink)
	EVT_MENU(IDM_UI_BREAKDOCKLINK, WxSelectionToolProxy::OnContext_BreakDockLink)
	EVT_MENU(IDM_UI_BREAKALLDOCKLINKS, WxSelectionToolProxy::OnContext_BreakAllDockLinks)
	EVT_MENU(IDM_UI_DOCKINGEDITOR, WxSelectionToolProxy::OnContext_DockingEditor)
END_EVENT_TABLE()

WxSelectionToolProxy::WxSelectionToolProxy(wxWindow* InParent, FUIWidgetTool_Selection* InParentTool, class WxUIEditorBase* InEditor) : 
wxWindow(InParent, wxID_ANY),
ParentTool(InParentTool),
UIEditor(InEditor)
{

}

/**
 * Breaks the dock link starting from the handle that was right clicked.
 */
void WxSelectionToolProxy::OnContext_BreakDockLink(wxCommandEvent& Event )
{
	FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_BreakDockLink")) );

	
	WxUIHandleContextMenu* Menu = wxDynamicCast(Event.GetEventObject(), WxUIHandleContextMenu);
	UUIObject* Widget = Menu->ActiveWidget;
	EUIWidgetFace DockFace = Menu->ActiveFace;

	FScopedObjectStateChange DockingChangeNotification(Widget);

	Menu->ActiveWidget->SetDockTarget(DockFace, NULL, UIFACE_MAX);
	UIEditor->RefreshPositionPanelValues();
	UIEditor->RefreshDockingPanelValues();
}


/**
 * Breaks the dock link starting from the handle that was right clicked.
 */
void WxSelectionToolProxy::OnContext_BreakAllDockLinks(wxCommandEvent& Event )
{
	FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_BreakDockLink")) );
	WxUIHandleContextMenu* Menu = wxDynamicCast(Event.GetEventObject(), WxUIHandleContextMenu);
	UUIObject* Widget = Menu->ActiveWidget;

	FScopedObjectStateChange NavTargetChangeNotification(Widget);
	Menu->ActiveWidget->SetDockTarget(UIFACE_Top, NULL, UIFACE_MAX);
	Menu->ActiveWidget->SetDockTarget(UIFACE_Bottom, NULL, UIFACE_MAX);
	Menu->ActiveWidget->SetDockTarget(UIFACE_Left, NULL, UIFACE_MAX);
	Menu->ActiveWidget->SetDockTarget(UIFACE_Right, NULL, UIFACE_MAX);
	UIEditor->RefreshPositionPanelValues();
	UIEditor->RefreshDockingPanelValues();
}


/**
 * Connects the selected dock link.
 */
void WxSelectionToolProxy::OnContext_ConnectDockLink( wxCommandEvent& Event )
{
	// Get the HandleIdx from the event's ID, and then make sure it is still a valid index into the sorted dock handle array.
	// If it is, set the dock target for the selected widget using that info.

	const INT HandleIdx = Event.GetId() - IDM_UI_DOCKTARGET_BEGIN;
	const TArray<FRenderModifier_TargetWidgetOutline::FWidgetDockHandle*>* Handles = ParentTool->TargetWidgetOutline.GetCloseDockHandles();
	const UBOOL bValidId = HandleIdx < Handles->Num();
	if( bValidId == TRUE)
	{
		const FRenderModifier_TargetWidgetOutline::FWidgetDockHandle* Handle = (*Handles)(HandleIdx);
		UUIObject* SelectedWidget = ParentTool->SelectedWidgetsOutline.GetSelectedWidget();
		const EUIWidgetFace SelectedFace = ParentTool->SelectedWidgetsOutline.GetSelectedDockHandle();
		UUIScreenObject* TargetWidget = Handle->Widget;
		const EUIWidgetFace TargetFace = Handle->Face;

		if(SelectedWidget != NULL && SelectedFace != UIFACE_MAX)
		{
			FScopedTransaction ScopedTransaction( *LocalizeUI(TEXT("UIEditor_MenuItem_ConnectDockLink")) );
			FScopedObjectStateChange NavTargetChangeNotification(SelectedWidget);

			if(TargetWidget == NULL && TargetFace != UIFACE_MAX)
			{
				TargetWidget = UIEditor->OwnerContainer;
			}


			SelectedWidget->SetDockTarget(SelectedFace, TargetWidget, TargetFace);
			SelectedWidget->UpdateScene();
			UIEditor->RefreshPositionPanelValues();
			UIEditor->RefreshDockingPanelValues();
		}
	}
}

void WxSelectionToolProxy::OnContext_DockingEditor( wxCommandEvent& Event )
{
	// Forward this event to our parent.
	Event.SetEventObject(GetParent());

	GetParent()->ProcessEvent(Event);
}

/** 
 * FUIWidgetTool_Selection
 */

FUIWidgetTool_Selection::FUIWidgetTool_Selection(class WxUIEditorBase* InEditor) : 
FUIWidgetTool(InEditor),
SelectedWidgetsOutline(InEditor),
TargetWidgetOutline(InEditor),
MenuProxy(NULL)
{
	
}

/**
 * Method for serializing UObject references that must be kept around through GC's.
 * 
 * @param Ar The archive to serialize with
 */
void FUIWidgetTool_Selection::Serialize(FArchive& Ar)
{
	SelectedWidgetsOutline.Serialize(Ar);
	TargetWidgetOutline.Serialize(Ar);
}


/** Called when this tool is 'activated'. */
void FUIWidgetTool_Selection::EnterToolMode()
{
	// Add render modifiers.
	UIEditor->ObjectVC->AddRenderModifier(&TargetWidgetOutline,0);
	UIEditor->ObjectVC->AddRenderModifier(&SelectedWidgetsOutline,0);

	// Add a proxy to receive wx events
	MenuProxy = new WxSelectionToolProxy(UIEditor, this, UIEditor);
}

/** Called when this tool is 'deactivated'. */
void FUIWidgetTool_Selection::ExitToolMode()
{
	// Remove render modifiers.
	UIEditor->ObjectVC->RemoveRenderModifier(&TargetWidgetOutline);
	UIEditor->ObjectVC->RemoveRenderModifier(&SelectedWidgetsOutline);


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
UBOOL FUIWidgetTool_Selection::InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed)
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

	return bConsumeInput;
}

/**
 * Called when the user clicks anywhere in the viewport.
 *
 * @param	HitProxy	the hitproxy that was clicked on (may be null if no hit proxies were clicked on)
 * @param	Click		contains data about this mouse click
 * @return				Whether or not we consumed the click.
 */
UBOOL FUIWidgetTool_Selection::ProcessClick (HHitProxy* HitProxy, const FObjectViewportClick& Click)
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
void FUIWidgetTool_Selection::InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime)
{
	TargetWidgetOutline.ClearSelections();
}

/**
 * Called when the user moves the mouse while this viewport is not capturing mouse input (i.e. not dragging).
 */
void FUIWidgetTool_Selection::MouseMove(FViewport* Viewport, INT X, INT Y)
{
	if(UIEditor->ObjectVC->MouseTool != NULL)
	{
		// If we are dragging, we keep track of a 'target' widget and face for the mouse tools.
		UBOOL bClearTarget = TRUE;

		HHitProxy* HitProxy = Viewport->GetHitProxy(X, Y);

		if(HitProxy != NULL)
		{
			// We first check to see if we have moused over a widget.  
			if(HitProxy->IsA(HUIDockHandleHitProxy::StaticGetType()) == TRUE)
			{
				HUIDockHandleHitProxy* DockHandleProxy = (HUIDockHandleHitProxy*)(HitProxy);
				UUIObject* Widget = DockHandleProxy->UIObject;

				const UUIObject* SelectedWidget = SelectedWidgetsOutline.GetSelectedWidget();
				const EUIWidgetFace DockHandle = DockHandleProxy->DockFace;

				// Make sure we aren't setting the target widget to the same widget as the selected widget.
				if(SelectedWidget != NULL && SelectedWidget != Widget && DockHandle != UIFACE_MAX)
				{	
					TargetWidgetOutline.SetSelectedWidget(Widget);
					TargetWidgetOutline.SetSelectedDockFace(DockHandle);

					bClearTarget = FALSE;
				}
			}
		}

		// See if we moused over the scene or any widgets...
		if(bClearTarget == TRUE)
		{
			// Build a list of widgets currently under the mouse cursor, we will add this to our 
			// target widget outline container.
			const UUIObject* SelectedWidget = SelectedWidgetsOutline.GetSelectedWidget();

			FLOAT ViewportX, ViewportY;
			UIEditor->ObjectVC->GetViewportPositionFromMousePosition(X, Y, ViewportX, ViewportY);

			const FVector2D Point(ViewportX, ViewportY);
			TArray<UUIObject*> Children = UIEditor->OwnerContainer->GetChildren( TRUE );
			TArray<UUIObject*> HitWidgets;

			for(INT ChildIdx = 0; ChildIdx < Children.Num(); ChildIdx++)
			{
				UUIObject* ChildWidget = Children(ChildIdx);
				const UBOOL bUnderMouse = ChildWidget->ContainsPoint(Point);

				if( bUnderMouse && SelectedWidget != ChildWidget )
				{
					HitWidgets.AddItem(ChildWidget);
				}
			}

			// Check to see if the user is moused over the scene extents.
			if(UIEditor->OwnerContainer->ContainsPoint(Point) == TRUE)
			{
				HitWidgets.AddUniqueItem(NULL);
			}

			// Make sure we have atleast one target widget under us that isn't the selected widget.
			if(HitWidgets.Num() > 0)
			{	
				TargetWidgetOutline.CalculateVisibleDockHandles(HitWidgets);

				// Clear the dock handle member because we are not on one.
				TargetWidgetOutline.SetSelectedWidget(NULL);
				TargetWidgetOutline.SetSelectedDockFace(UIFACE_MAX);	

				bClearTarget = FALSE;
			}
		}

		// Clear the target widget if nothing was moused over.
		if( bClearTarget == TRUE )
		{	
			TargetWidgetOutline.ClearSelections();
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
			if(HitProxy->IsA(HUIDockHandleHitProxy::StaticGetType()) == TRUE)
			{
				HUIDockHandleHitProxy* DockHandleProxy = (HUIDockHandleHitProxy*)(HitProxy);

				SelectedWidgetsOutline.SetSelectedWidget(DockHandleProxy->UIObject);
				SelectedWidgetsOutline.SetSelectedDockFace(DockHandleProxy->DockFace);

				bClearSelections = FALSE;
			}
		}

		if(bClearSelections == TRUE)
		{
			SelectedWidgetsOutline.ClearSelections();
		}
	}
}

/**
 * Called when the user right-clicks on a dock handle in the viewport, shows a context menu for dock handles.
 */
void FUIWidgetTool_Selection::ShowDockHandleContextMenu( )
{
	UUIObject* SelectedWidget = SelectedWidgetsOutline.GetSelectedWidget();
	EUIWidgetFace SelectedFace = SelectedWidgetsOutline.GetSelectedDockHandle();

	const UBOOL bValidWidget = (SelectedWidget != NULL);
	const UBOOL bValidFace = (SelectedFace != UIFACE_MAX);

	if(bValidWidget && bValidFace)
	{
		UIEditor->SynchronizeSelectedWidgets(UIEditor);

		WxUIHandleContextMenu menu(MenuProxy);
		menu.Create( SelectedWidget, SelectedFace );
		menu.Append( IDM_UI_BREAKDOCKLINK, *LocalizeUI(TEXT("UIEditor_MenuItem_BreakDockLink")) );
		menu.Append( IDM_UI_BREAKALLDOCKLINKS, *LocalizeUI(TEXT("UIEditor_MenuItem_BreakAllDockLinks")) );
		menu.AppendSeparator();
		menu.Append( IDM_UI_DOCKINGEDITOR, *LocalizeUI(TEXT("UIEditor_MenuItem_DockingEditor")) );

		// If we do not have a dock link on this dock handle, then disable the break dock link item.
		// Otherwise, enable the item and generate a label for the menu item that says which widget/face we are breaking the link to.
		UUIObject* TargetWidget = SelectedWidget->DockTargets.GetDockTarget(SelectedFace);
		EUIWidgetFace TargetFace = SelectedWidget->DockTargets.GetDockFace(SelectedFace);

		if(!SelectedWidget->DockTargets.IsDocked(SelectedFace))
		{
			menu.SetLabel(IDM_UI_BREAKDOCKLINK, *LocalizeUI(TEXT("UIEditor_MenuItem_BreakDockLink")));
			menu.Enable(IDM_UI_BREAKDOCKLINK, false);
		}
		else
		{
			FString DockLabel;

			UIEditor->GetDockHandleString(TargetWidget, TargetFace, DockLabel);

			FString MenuLabel = FString::Printf(*LocalizeUI(TEXT("UIEditor_MenuItem_BreakDockLinkTo")), *DockLabel);
			menu.SetLabel(IDM_UI_BREAKDOCKLINK, *MenuLabel);
			menu.Enable(IDM_UI_BREAKDOCKLINK, true);
		}

		FTrackPopupMenu tpm( MenuProxy, &menu );
		tpm.Show();
	}
}
