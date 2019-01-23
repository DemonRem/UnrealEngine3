/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "DragTool_BoxSelect.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FDragTool_BoxSelect
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
* Ends a mouse drag behavior (the user has let go of the mouse button).
*/
void FDragTool_BoxSelect::EndDrag()
{
	// If the user is selecting, but isn't hold down SHIFT, remove all current selections.
	if( bLeftMouseButtonDown && !bShiftDown )
	{
		switch( GEditorModeTools().GetCurrentModeID() )
		{
		case EM_Geometry:
			((FEdModeGeometry*)GEditorModeTools().GetCurrentMode())->SelectNone();
			break;

		default:
			GEditor->SelectNone( TRUE, TRUE );
			break;
		}
	}

	// Create a bounding box based on the start/end points (normalizes the points).
	FBox SelBBox(0);
	SelBBox += Start;
	SelBBox += End;

	switch(ViewportClient->ViewportType)
	{
	case LVT_OrthoXY:
		SelBBox.Min.Z = -WORLD_MAX;
		SelBBox.Max.Z = WORLD_MAX;
		break;
	case LVT_OrthoXZ:
		SelBBox.Min.Y = -WORLD_MAX;
		SelBBox.Max.Y = WORLD_MAX;
		break;
	case LVT_OrthoYZ:
		SelBBox.Min.X = -WORLD_MAX;
		SelBBox.Max.X = WORLD_MAX;
		break;
	}

	if ( GEditorModeTools().GetCurrentModeID() == EM_Geometry )
	{
		// We're in geometry mode -- let geometry mode handle the box selection.
		GEditorModeTools().GetCurrentMode()->BoxSelect( SelBBox, bLeftMouseButtonDown );
	}
	else
	{
		// Select all actors that are within the selection box area.  Be aware that certain modes do special processing below.	
		UBOOL bSelectionChanged = FALSE;
		const DOUBLE StartTime = appSeconds();
		for( FActorIterator It; It; ++It )
		{
			AActor* Actor = *It;
			UBOOL bActorHitByBox = FALSE;

			// Never drag-select hidden actors or builder brushes.
			if( !Actor->IsHiddenEd() && !Actor->IsABuilderBrush() )
			{
				// Skeletal meshes.
				APawn* Pawn = Cast<APawn>( Actor );
				if( Pawn && Pawn->Mesh )
				{
					if( ViewportClient->ComponentIsTouchingSelectionBox( Actor, Pawn->Mesh, SelBBox ) )
					{
						GEditor->SelectActor( Actor, TRUE, ViewportClient, FALSE );
						bActorHitByBox = TRUE;
					}
				}
				else
				{
					// Iterate over all actor components, selecting out primitive components
					for( INT ComponentIndex = 0 ; ComponentIndex < Actor->Components.Num() ; ++ComponentIndex )
					{
						UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>( Actor->Components(ComponentIndex) );
						if ( PrimitiveComponent && !PrimitiveComponent->HiddenEditor )
						{
							if ( ViewportClient->ComponentIsTouchingSelectionBox( Actor, PrimitiveComponent, SelBBox ) )
							{
								bActorHitByBox = TRUE;
								break;
							}
						}
					}
				}
			}

			// Select the actor if we need to
			if( bActorHitByBox )
			{
				GEditor->SelectActor( Actor, bLeftMouseButtonDown, ViewportClient, FALSE );
				bSelectionChanged = TRUE;
			}
		}
		const DOUBLE EndTime = appSeconds() - StartTime;
		debugf(TEXT("FDragTool_BoxSelect::EndDrag()  %.1f msec"), EndTime*1000.);
		if ( bSelectionChanged )
		{
			GEditor->NoteSelectionChange();
		}
	}

	// Clean up.
	FDragTool::EndDrag();
}

void FDragTool_BoxSelect::Render3D(const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	DrawWireBox( PDI, FBox( Start, End ), FColor(255,0,0), SDPG_Foreground );
}
