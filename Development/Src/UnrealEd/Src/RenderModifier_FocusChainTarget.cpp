/*=============================================================================
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/


#include "UnrealEd.h"
#include "UnObjectEditor.h"
#include "UnUIEditor.h"
#include "UnLinkedObjDrawUtils.h"

#include "RenderModifier_FocusChainTarget.h"

/**
 * Constructor
 *
 * @param	InEditor	The UI editor that this rendermodifier belongs to.
 */
FRenderModifier_FocusChainTarget::FRenderModifier_FocusChainTarget( class WxUIEditorBase* InEditor ) : 
FUIEditor_RenderModifier(InEditor),
SelectedWidget(NULL)
{

}


/**
 * Sets the list of widgets that we currently have as targets (ie under the mouse cursor)
 * @param Widgets	Currently targeted widgets
 */
void FRenderModifier_FocusChainTarget::SetTargetWidgets(TArray<UUIObject*> &Widgets)
{
	TargetWidgets = Widgets;

	if(TargetWidgets.Num())
	{
		// Generate a tooltip with the names of all of the widgets the mouse is currently over.
		TArray<FString> TempStringList;
		const INT NumHitWidgets = TargetWidgets.Num();
		for(INT WidgetIdx = 0; WidgetIdx < NumHitWidgets; WidgetIdx++)
		{
			const UUIObject* Widget = TargetWidgets(WidgetIdx);
			TempStringList.AddItem(*Widget->GetName());
		}

		UIEditor->TargetTooltip->SetStringList(TempStringList);
	}
	else
	{
		UIEditor->TargetTooltip->ClearStringList();
	}

}

/** Clears the selected dock face and dock widget. */
void FRenderModifier_FocusChainTarget::ClearSelections()
{
	SelectedWidget = NULL;
	TargetWidgets.Empty();

	UIEditor->TargetTooltip->ClearStringList();
}


/**
 * Method for serializing UObject references that must be kept around through GC's.
 *
 * @param Ar The archive to serialize with
 */
void FRenderModifier_FocusChainTarget::Serialize(FArchive& Ar)
{
	Ar << SelectedWidget << TargetWidgets;
}

/**
 * Renders an outline around all of our target widgets.
 */
void FRenderModifier_FocusChainTarget::Render( FEditorObjectViewportClient* ObjectVC, FCanvas* Canvas )
{
	INT NumWidgets = TargetWidgets.Num();

	for(INT WidgetIdx = 0; WidgetIdx < NumWidgets; WidgetIdx++)
	{
		UUIObject* Widget = TargetWidgets(WidgetIdx);

		// Render a box outline around this widget because it is selected.
		FColor BorderColor(255,255,64);

		FVector Expand(2 / ObjectVC->Zoom2D,2 / ObjectVC->Zoom2D,2 / ObjectVC->Zoom2D);
		FVector Start, End;

		// Top Line.
		Start.X = Widget->RenderBounds[UIFACE_Left];
		Start.Y = Widget->RenderBounds[UIFACE_Top];

		End.X = Widget->RenderBounds[UIFACE_Right];
		End.Y = Widget->RenderBounds[UIFACE_Top];

		DrawTile(Canvas,Start.X - Expand.X, Start.Y - Expand.Y, Expand.X * 2 + End.X - Start.X, Expand.Y * 2 + End.Y - Start.Y, 0.0f, 0.0f, 0.0f, 0.0f, BorderColor);

		// Bottom Line.
		Start.X = Widget->RenderBounds[UIFACE_Left];
		Start.Y = Widget->RenderBounds[UIFACE_Bottom];

		End.X = Widget->RenderBounds[UIFACE_Right];
		End.Y = Widget->RenderBounds[UIFACE_Bottom];

		DrawTile(Canvas,Start.X - Expand.X, Start.Y - Expand.Y, Expand.X * 2 + End.X - Start.X, Expand.Y * 2 + End.Y - Start.Y, 0.0f, 0.0f, 0.0f, 0.0f, BorderColor);


		// Left Line.
		Start.X = Widget->RenderBounds[UIFACE_Left];
		Start.Y = Widget->RenderBounds[UIFACE_Top];

		End.X = Widget->RenderBounds[UIFACE_Left];
		End.Y = Widget->RenderBounds[UIFACE_Bottom];

		DrawTile(Canvas,Start.X - Expand.X, Start.Y - Expand.Y, Expand.X * 2 + End.X - Start.X, Expand.Y * 2 + End.Y - Start.Y, 0.0f, 0.0f, 0.0f, 0.0f, BorderColor);

		// Right Line.
		Start.X = Widget->RenderBounds[UIFACE_Right];
		Start.Y = Widget->RenderBounds[UIFACE_Top];

		End.X = Widget->RenderBounds[UIFACE_Right];
		End.Y = Widget->RenderBounds[UIFACE_Bottom];

		DrawTile(Canvas,Start.X - Expand.X, Start.Y - Expand.Y, Expand.X * 2 + End.X - Start.X, Expand.Y * 2 + End.Y - Start.Y, 0.0f, 0.0f, 0.0f, 0.0f, BorderColor);
	}
}
