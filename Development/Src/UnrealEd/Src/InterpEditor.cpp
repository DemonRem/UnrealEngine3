/*=============================================================================
	InterpEditor.cpp: Interpolation editing
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "CurveEd.h"
#include "EngineSequenceClasses.h"
#include "EngineInterpolationClasses.h"
#include "InterpEditor.h"
#include "UnLinkedObjDrawUtils.h"
#include "Properties.h"
#include "GenericBrowser.h"

IMPLEMENT_CLASS(UInterpEdOptions);


static const INT LabelWidth = 160;
static const FLOAT ZoomIncrement = 1.2f;

static const FColor PositionMarkerLineColor(255, 222, 206);
static const FColor LoopRegionFillColor(80,255,80,24);
static const FColor Track3DSelectedColor(255,255,0);

/*-----------------------------------------------------------------------------
	UInterpEdTransBuffer / FInterpEdTransaction
-----------------------------------------------------------------------------*/

void UInterpEdTransBuffer::BeginSpecial(const TCHAR* SessionName)
{
	CheckState();
	if( ActiveCount++==0 )
	{
		// Cancel redo buffer.
		//debugf(TEXT("BeginTrans %s"), SessionName);
		if( UndoCount )
		{
			UndoBuffer.Remove( UndoBuffer.Num()-UndoCount, UndoCount );
		}

		UndoCount = 0;

		// Purge previous transactions if too much data occupied.
		while( GetUndoSize() > MaxMemory )
		{
			UndoBuffer.Remove( 0 );
		}

		// Begin a new transaction.
		GUndo = new(UndoBuffer)FInterpEdTransaction( SessionName, 1 );
	}
	CheckState();
}

void UInterpEdTransBuffer::EndSpecial()
{
	CheckState();
	check(ActiveCount>=1);
	if( --ActiveCount==0 )
	{
		GUndo = NULL;
	}
	CheckState();
}

void FInterpEdTransaction::SaveObject( UObject* Object )
{
	check(Object);

	if( Object->IsA( USeqAct_Interp::StaticClass() ) ||
		Object->IsA( UInterpData::StaticClass() ) ||
		Object->IsA( UInterpGroup::StaticClass() ) ||
		Object->IsA( UInterpTrack::StaticClass() ) ||
		Object->IsA( UInterpGroupInst::StaticClass() ) ||
		Object->IsA( UInterpTrackInst::StaticClass() ) ||
		Object->IsA( UInterpEdOptions::StaticClass() ) )
	{
		// Save the object.
		new( Records )FObjectRecord( this, Object, NULL, 0, 0, 0, 0, NULL, NULL );
	}
}

void FInterpEdTransaction::SaveArray( UObject* Object, FArray* Array, INT Index, INT Count, INT Oper, INT ElementSize, STRUCT_AR Serializer, STRUCT_DTOR Destructor )
{
	// Never want this.
}

IMPLEMENT_CLASS(UInterpEdTransBuffer);

/*-----------------------------------------------------------------------------
 FInterpEdViewportClient
-----------------------------------------------------------------------------*/

FInterpEdViewportClient::FInterpEdViewportClient( class WxInterpEd* InInterpEd )
{
	InterpEd = InInterpEd;

	// Set initial zoom range
	ViewStartTime = 0.f;
	ViewEndTime = InterpEd->IData->InterpLength;

	OldMouseX = 0;
	OldMouseY = 0;

	DistanceDragged = 0;

	BoxStartX = 0;
	BoxStartY = 0;
	BoxEndX = 0;
	BoxEndY = 0;

	PixelsPerSec = 1.f;
	TrackViewSizeX = 0;

	bDrawSnappingLine = false;
	SnappingLinePosition = 0.f;

	bPanning = false;
	bMouseDown = false;
	bGrabbingHandle = false;
	bBoxSelecting = false;
	bTransactionBegun = false;
	bNavigating = false;
	bGrabbingMarker	= false;

	DragObject = NULL;

	SetRealtime( false );
}

FInterpEdViewportClient::~FInterpEdViewportClient()
{

}

UBOOL FInterpEdViewportClient::InputKey(FViewport* Viewport, INT ControllerId, FName Key, EInputEvent Event,FLOAT /*AmountDepressed*/,UBOOL /*Gamepad*/)
{
	//Viewport->CaptureMouseInput( Viewport->KeyState(KEY_LeftMouseButton) || Viewport->KeyState(KEY_RightMouseButton) );

	const UBOOL bCtrlDown = Viewport->KeyState(KEY_LeftControl) || Viewport->KeyState(KEY_RightControl);
	const UBOOL bShiftDown = Viewport->KeyState(KEY_LeftShift) || Viewport->KeyState(KEY_RightShift);
	const UBOOL bAltDown = Viewport->KeyState(KEY_LeftAlt) || Viewport->KeyState(KEY_RightAlt);

	const INT HitX = Viewport->GetMouseX();
	const INT HitY = Viewport->GetMouseY();

	if( Key == KEY_LeftMouseButton )
	{
		switch( Event )
		{
		case IE_Pressed:
			{
				if(DragObject == NULL)
				{
					HHitProxy*	HitResult = Viewport->GetHitProxy(HitX,HitY);

					if(HitResult)
					{
						if(HitResult->IsA(HInterpEdGroupTitle::StaticGetType()))
						{
							UInterpGroup* Group = ((HInterpTrackKeypointProxy*)HitResult)->Group;

							InterpEd->SetActiveTrack(Group, INDEX_NONE);

							InterpEd->ClearKeySelection();
						}
						else if(HitResult->IsA(HInterpEdGroupCollapseBtn::StaticGetType()))
						{
							UInterpGroup* Group = ((HInterpTrackKeypointProxy*)HitResult)->Group;

							InterpEd->SetActiveTrack(Group, INDEX_NONE);
							Group->bCollapsed = !Group->bCollapsed;

							InterpEd->ClearKeySelection();
						}
						else if(HitResult->IsA(HInterpEdGroupLockCamBtn::StaticGetType()))
						{
							UInterpGroup* Group = ((HInterpEdGroupLockCamBtn*)HitResult)->Group;

							if(Group == InterpEd->CamViewGroup)
							{
								InterpEd->LockCamToGroup(NULL);
							}
							else
							{
								InterpEd->LockCamToGroup(Group);
							}
						}
						else if(HitResult->IsA(HInterpEdTrackTitle::StaticGetType()))
						{
							UInterpGroup* Group = ((HInterpTrackKeypointProxy*)HitResult)->Group;
							const INT TrackIndex = ((HInterpEdTrackTitle*)HitResult)->TrackIndex;

							InterpEd->SetActiveTrack(Group, TrackIndex);

							InterpEd->ClearKeySelection();
						}
						else if(HitResult->IsA(HInterpEdTrackGraphPropBtn::StaticGetType()))
						{
							UInterpGroup* Group = ((HInterpEdTrackGraphPropBtn*)HitResult)->Group;
							const INT TrackIndex = ((HInterpEdTrackGraphPropBtn*)HitResult)->TrackIndex;

							InterpEd->AddTrackToCurveEd(Group, TrackIndex);
						}
						else if(HitResult->IsA(HInterpEdEventDirBtn::StaticGetType()))
						{
							UInterpGroup* Group = ((HInterpEdEventDirBtn*)HitResult)->Group;
							const INT TrackIndex = ((HInterpEdEventDirBtn*)HitResult)->TrackIndex;
							EInterpEdEventDirection Dir = ((HInterpEdEventDirBtn*)HitResult)->Dir;

							UInterpTrackEvent* EventTrack = CastChecked<UInterpTrackEvent>( Group->InterpTracks(TrackIndex) );

							if(Dir == IED_Forward)
							{
								EventTrack->bFireEventsWhenForwards = !EventTrack->bFireEventsWhenForwards;
							}
							else
							{
								EventTrack->bFireEventsWhenBackwards = !EventTrack->bFireEventsWhenBackwards;
							}
						}
						else if(HitResult->IsA(HInterpTrackKeypointProxy::StaticGetType()))
						{
							UInterpGroup* Group = ((HInterpTrackKeypointProxy*)HitResult)->Group;
							const INT TrackIndex = ((HInterpTrackKeypointProxy*)HitResult)->TrackIndex;
							const INT KeyIndex = ((HInterpTrackKeypointProxy*)HitResult)->KeyIndex;

							if(!bCtrlDown)
							{
								InterpEd->ClearKeySelection();
								InterpEd->AddKeyToSelection(Group, TrackIndex, KeyIndex, !bShiftDown);
								if ( bShiftDown )
								{
									InterpEd->SetActiveTrack( Group, TrackIndex );
								}
							}
						}
						else if(HitResult->IsA(HInterpEdTrackBkg::StaticGetType()))
						{
							InterpEd->SetActiveTrack(NULL, INDEX_NONE);
						}
						else if(HitResult->IsA(HInterpEdTimelineBkg::StaticGetType()))
						{
							const FLOAT NewTime = ViewStartTime + ((HitX - LabelWidth) / PixelsPerSec);

							// When jumping to location by clicking, stop playback.
							InterpEd->Interp->Stop();
							SetRealtime( false );

							// Move to clicked on location
							InterpEd->SetInterpPosition(NewTime);

							// Act as if we grabbed the handle as well.
							bGrabbingHandle = true;
						}
						else if(HitResult->IsA(HInterpEdNavigator::StaticGetType()))
						{
							// Center view on position we clicked on in the navigator.
							const FLOAT JumpToTime = ((HitX - LabelWidth)/NavPixelsPerSecond);
							const FLOAT ViewWindow = (ViewEndTime - ViewStartTime);

							ViewStartTime = JumpToTime - (0.5f * ViewWindow);
							ViewEndTime = JumpToTime + (0.5f * ViewWindow);
							InterpEd->SyncCurveEdView();

							bNavigating = true;
						}
						else if(HitResult->IsA(HInterpEdMarker::StaticGetType()))
						{
							GrabbedMarkerType = ((HInterpEdMarker*)HitResult)->Type;

							InterpEd->BeginMoveMarker();
							bGrabbingMarker = true;
						}
						else if(HitResult->IsA(HInterpEdTab::StaticGetType()))
						{
							InterpEd->SetSelectedFilter(((HInterpEdTab*)HitResult)->Filter);

							Viewport->Invalidate();	
						}
						else if(HitResult->IsA(HInterpEdTrackDisableTrackBtn::StaticGetType()))
						{
							HInterpEdTrackDisableTrackBtn* TrackProxy = ((HInterpEdTrackDisableTrackBtn*)HitResult);

							if(TrackProxy->Group != NULL)
							{
								if(TrackProxy->TrackIndex != INDEX_NONE)
								{
									UInterpTrack* Track = TrackProxy->Group->InterpTracks(TrackProxy->TrackIndex);
									Track->bDisableTrack = !Track->bDisableTrack;

									// Update the preview
									InterpEd->Interp->UpdateInterp(InterpEd->Interp->Position, TRUE, TRUE);
									InterpEd->UpdateCameraToGroup();
								}
							}
						}
						else if(HitResult->IsA(HInterpEdInputInterface::StaticGetType()))
						{
							HInterpEdInputInterface* Proxy = ((HInterpEdInputInterface*)HitResult);

							DragObject = Proxy->ClickedObject;
							DragData = Proxy->InputData;
							DragData.PixelsPerSec = PixelsPerSec;
							DragData.MouseStart = FIntPoint(HitX, HitY);
							DragData.bCtrlDown = bCtrlDown;
							DragData.bAltDown = bAltDown;
							DragData.bShiftDown = bShiftDown;
							Proxy->ClickedObject->BeginDrag(DragData);
						}
					}
					else
					{
						if(bCtrlDown && bAltDown)
						{
							BoxStartX = BoxEndX = HitX;
							BoxStartY = BoxEndY = HitY;

							bBoxSelecting = true;
						}
						else
						{
							bPanning = true;
						}
					}

					Viewport->LockMouseToWindow(true);

					bMouseDown = true;
					OldMouseX = HitX;
					OldMouseY = HitY;
					DistanceDragged = 0;
				}
			}
			break;
		case IE_DoubleClick:
			{
				HHitProxy*	HitResult = Viewport->GetHitProxy(HitX,HitY);

				if(HitResult)
				{
					if(HitResult->IsA(HInterpEdGroupTitle::StaticGetType()))
					{
						UInterpGroup* Group = ((HInterpTrackKeypointProxy*)HitResult)->Group;

						Group->bCollapsed = !Group->bCollapsed;
					}
				}
			}
			break;
		case IE_Released:
			{
				if(bBoxSelecting)
				{
					const INT MinX = Min(BoxStartX, BoxEndX);
					const INT MinY = Min(BoxStartY, BoxEndY);
					const INT MaxX = Max(BoxStartX, BoxEndX) + 1;
					const INT MaxY = Max(BoxStartY, BoxEndY) + 1;
					const INT TestSizeX = MaxX - MinX;
					const INT TestSizeY = MaxY - MinY;

					// Find how much (in time) 1.5 pixels represents on the screen.
					const FLOAT PixelTime = 1.5f/PixelsPerSec;

					// We read back the hit proxy map for the required region.
					TArray<HHitProxy*> ProxyMap;
					Viewport->GetHitProxyMap((UINT)MinX, (UINT)MinY, (UINT)MaxX, (UINT)MaxY, ProxyMap);

					TArray<FInterpEdSelKey>	NewSelection;

					// Find any keypoint hit proxies in the region - add the keypoint to selection.
					for(INT Y=0; Y < TestSizeY; Y++)
					{
						for(INT X=0; X < TestSizeX; X++)
						{
							HHitProxy* HitProxy = ProxyMap(Y * TestSizeX + X);

							if(HitProxy && HitProxy->IsA(HInterpTrackKeypointProxy::StaticGetType()))
							{
								UInterpGroup* Group = ((HInterpTrackKeypointProxy*)HitProxy)->Group;
								const INT TrackIndex = ((HInterpTrackKeypointProxy*)HitProxy)->TrackIndex;
								const INT KeyIndex = ((HInterpTrackKeypointProxy*)HitProxy)->KeyIndex;

								// Because AddKeyToSelection might invalidate the display, we just remember all the keys here and process them together afterwards.
								NewSelection.AddUniqueItem( FInterpEdSelKey(Group, TrackIndex, KeyIndex) );

								// Slight hack here. We select any other keys on the same track which are within 1.5 pixels of this one.
								UInterpTrack* Track = Group->InterpTracks(TrackIndex);
								const FLOAT SelKeyTime = Track->GetKeyframeTime(KeyIndex);

								for(INT i=0; i<Track->GetNumKeyframes(); i++)
								{
									const FLOAT KeyTime = Track->GetKeyframeTime(i);
									if( Abs(KeyTime - SelKeyTime) < PixelTime )
									{
										NewSelection.AddUniqueItem( FInterpEdSelKey(Group, TrackIndex, i) );
									}
								}
							}
						}
					}

					if(!bShiftDown)
					{
						InterpEd->ClearKeySelection();
					}

					for(INT i=0; i<NewSelection.Num(); i++)
					{
						InterpEd->AddKeyToSelection( NewSelection(i).Group, NewSelection(i).TrackIndex, NewSelection(i).KeyIndex, false );
					}
				}
				else if(DragObject)
				{
					HHitProxy*	HitResult = Viewport->GetHitProxy(HitX,HitY);

					if(HitResult)
					{
						if(HitResult->IsA(HInterpEdInputInterface::StaticGetType()))
						{
							HInterpEdInputInterface* Proxy = ((HInterpEdInputInterface*)HitResult);
							
							//@todo: Do dropping.
						}
					}

					DragData.PixelsPerSec = PixelsPerSec;
					DragData.MouseCurrent = FIntPoint(HitX, HitY);
					DragObject->EndDrag(DragData);
					DragObject = NULL;
				}
				else if(DistanceDragged < 4)
				{
					HHitProxy*	HitResult = Viewport->GetHitProxy(HitX,HitY);

					// If mouse didn't really move since last time, and we released over empty space, deselect everything.
					if(!HitResult)
					{
						InterpEd->ClearKeySelection();
					}
					else if(bCtrlDown && HitResult->IsA(HInterpTrackKeypointProxy::StaticGetType()))
					{
						UInterpGroup* Group = ((HInterpTrackKeypointProxy*)HitResult)->Group;
						const INT TrackIndex = ((HInterpTrackKeypointProxy*)HitResult)->TrackIndex;
						const INT KeyIndex = ((HInterpTrackKeypointProxy*)HitResult)->KeyIndex;

						const UBOOL bAlreadySelected = InterpEd->KeyIsInSelection(Group, TrackIndex, KeyIndex);
						if(bAlreadySelected)
						{
							InterpEd->RemoveKeyFromSelection(Group, TrackIndex, KeyIndex);
						}
						else
						{
							InterpEd->AddKeyToSelection(Group, TrackIndex, KeyIndex, !bShiftDown);
							if ( bShiftDown )
							{
								InterpEd->SetActiveTrack( Group, TrackIndex );
							}
						}
					}
				}

				if(bTransactionBegun)
				{
					InterpEd->EndMoveSelectedKeys();
					bTransactionBegun = false;
				}

				if(bGrabbingMarker)
				{
					InterpEd->EndMoveMarker();
					bGrabbingMarker = false;
				}

				Viewport->LockMouseToWindow(false);

				DistanceDragged = 0;

				bPanning = false;
				bMouseDown = false;
				bGrabbingHandle = false;
				bNavigating = false;
				bBoxSelecting = false;
			}
			break;
		}
	}
	else if( Key == KEY_RightMouseButton )
	{
		switch( Event )
		{
		case IE_Pressed:
			{
				const INT HitX = Viewport->GetMouseX();
				const INT HitY = Viewport->GetMouseY();
				HHitProxy*	HitResult = Viewport->GetHitProxy(HitX,HitY);

				if(HitResult)
				{
					wxMenu* Menu = InterpEd->CreateContextMenu( Viewport, HitResult );
					if(Menu)
					{
						FTrackPopupMenu tpm( InterpEd, Menu );
						tpm.Show();
						delete Menu;
					}
				}
			}
			break;

		case IE_Released:
			{
				
			}
			break;
		}
	}
	
	if(Event == IE_Pressed)
	{
		if(Key == KEY_MouseScrollDown)
		{
			InterpEd->ZoomView(ZoomIncrement);
		}
		else if(Key == KEY_MouseScrollUp)
		{
			InterpEd->ZoomView(1.f/ZoomIncrement);
		}

		// Handle hotkey bindings.
		UUnrealEdOptions* UnrealEdOptions = GUnrealEd->GetUnrealEdOptions();

		if(UnrealEdOptions)
		{
			FString Cmd = UnrealEdOptions->GetExecCommand(Key, bAltDown, bCtrlDown, bShiftDown, TEXT("Matinee"));

			if(Cmd.Len())
			{
				Exec(*Cmd);
			}
		}
	}

	// Handle viewport screenshot.
	InputTakeScreenshot( Viewport, Key, Event );

	return TRUE;
}

// X and Y here are the new screen position of the cursor.
void FInterpEdViewportClient::MouseMove(FViewport* Viewport, INT X, INT Y)
{
	UBOOL bCtrlDown = Viewport->KeyState(KEY_LeftControl) || Viewport->KeyState(KEY_RightControl);

	INT DeltaX = OldMouseX - X;
	INT DeltaY = OldMouseY - Y;

	if(bMouseDown)
	{
		DistanceDragged += ( Abs<INT>(DeltaX) + Abs<INT>(DeltaY) );
	}

	OldMouseX = X;
	OldMouseY = Y;


	if(bMouseDown)
	{
		if(DragObject != NULL)
		{
			DragData.PixelsPerSec = PixelsPerSec;
			DragData.MouseCurrent = FIntPoint(X, Y);
			DragObject->ObjectDragged(DragData);
		}
		else if(bGrabbingHandle)
		{
			FLOAT NewTime = ViewStartTime + ((X - LabelWidth) / PixelsPerSec);

			InterpEd->SetInterpPosition(NewTime);
		}
		else if(bBoxSelecting)
		{
			BoxEndX = X;
			BoxEndY = Y;
		}
		else if( bCtrlDown && InterpEd->Opt->SelectedKeys.Num() > 0 )
		{
			if(DistanceDragged > 4)
			{
				if(!bTransactionBegun)
				{
					InterpEd->BeginMoveSelectedKeys();
					bTransactionBegun = true;
				}

				FLOAT DeltaTime = -DeltaX / PixelsPerSec;
				InterpEd->MoveSelectedKeys(DeltaTime);
			}
		}
		else if(bNavigating)
		{
			FLOAT DeltaTime = -DeltaX / NavPixelsPerSecond;
			ViewStartTime += DeltaTime;
			ViewEndTime += DeltaTime;
			InterpEd->SyncCurveEdView();
		}
		else if(bGrabbingMarker)
		{
			FLOAT DeltaTime = -DeltaX / PixelsPerSec;
			UnsnappedMarkerPos += DeltaTime;

			if(GrabbedMarkerType == ISM_SeqEnd)
			{
				InterpEd->SetInterpEnd( InterpEd->SnapTime(UnsnappedMarkerPos, false) );
			}
			else if(GrabbedMarkerType == ISM_LoopStart || GrabbedMarkerType == ISM_LoopEnd)
			{
				InterpEd->MoveLoopMarker( InterpEd->SnapTime(UnsnappedMarkerPos, false), GrabbedMarkerType == ISM_LoopStart );
			}
		}
		else if(bPanning)
		{
			FLOAT DeltaTime = -DeltaX / PixelsPerSec;
			ViewStartTime -= DeltaTime;
			ViewEndTime -= DeltaTime;
			InterpEd->SyncCurveEdView();
		}
	}
}

UBOOL FInterpEdViewportClient::InputAxis(FViewport* Viewport, INT ControllerId, FName Key, FLOAT Delta, FLOAT DeltaTime, UBOOL bGamepad)
{
	return FALSE;
}

EMouseCursor FInterpEdViewportClient::GetCursor(FViewport* Viewport,INT X,INT Y)
{
	EMouseCursor Result = MC_Cross;

	if(DragObject==NULL)
	{
		HHitProxy*	HitProxy = Viewport->GetHitProxy(X,Y);
		

		if(HitProxy)
		{
			Result = HitProxy->GetMouseCursor();
		}
	}
	else
	{
		Result = MC_NoChange;
	}

	return Result;
}

void FInterpEdViewportClient::Tick(FLOAT DeltaSeconds)
{
	InterpEd->TickInterp(DeltaSeconds);

	// If curve editor is shown - sync us with it.
	if(InterpEd->CurveEd->IsShown())
	{
		ViewStartTime = InterpEd->CurveEd->StartIn;
		ViewEndTime = InterpEd->CurveEd->EndIn;
	}

	if(bNavigating || bPanning)
	{
		const INT ScrollBorderSize = 20;
		const FLOAT	ScrollBorderSpeed = 500.f;
		const INT PosX = Viewport->GetMouseX();
		const INT PosY = Viewport->GetMouseY();
		const INT SizeX = Viewport->GetSizeX();
		const INT SizeY = Viewport->GetSizeY();

		FLOAT DeltaTime = Clamp(DeltaSeconds, 0.01f, 1.0f);

		if(PosX < ScrollBorderSize)
		{
			ScrollAccum.X += (1.f - ((FLOAT)PosX/(FLOAT)ScrollBorderSize)) * ScrollBorderSpeed * DeltaTime;
		}
		else if(PosX > SizeX - ScrollBorderSize)
		{
			ScrollAccum.X -= ((FLOAT)(PosX - (SizeX - ScrollBorderSize))/(FLOAT)ScrollBorderSize) * ScrollBorderSpeed * DeltaTime;
		}
		else
		{
			ScrollAccum.X = 0.f;
		}

		// Apply integer part of ScrollAccum to the curve editor view position.
		const INT DeltaX = appFloor(ScrollAccum.X);
		ScrollAccum.X -= DeltaX;

		if(bNavigating)
		{
			DeltaTime = -DeltaX / NavPixelsPerSecond;
			ViewStartTime += DeltaTime;
			ViewEndTime += DeltaTime;

			InterpEd->SyncCurveEdView();
		}
		else
		{
			DeltaTime = -DeltaX / PixelsPerSec;
			ViewStartTime -= DeltaTime;
			ViewEndTime -= DeltaTime;
			InterpEd->SyncCurveEdView();
		}
	}

	Viewport->Invalidate();
}


void FInterpEdViewportClient::Serialize(FArchive& Ar) 
{ 
	Ar << Input; 

	// Drag object may be a instance of UObject, so serialize it if it is.
	if(DragObject && DragObject->GetUObject())
	{
		UObject* DragUObject = DragObject->GetUObject();
		Ar << DragUObject;
	}
}

/** Exec handler */
void FInterpEdViewportClient::Exec(const TCHAR* Cmd)
{
	const TCHAR* Str = Cmd;

	if(ParseCommand(&Str, TEXT("MATINEE")))
	{
		if(ParseCommand(&Str, TEXT("Undo")))
		{
			InterpEd->InterpEdUndo();
		}
		else if(ParseCommand(&Str, TEXT("Redo")))
		{
			InterpEd->InterpEdRedo();
		}
		else if(ParseCommand(&Str, TEXT("Cut")))
		{
			InterpEd->CopySelectedGroupOrTrack(TRUE);
		}
		else if(ParseCommand(&Str, TEXT("Copy")))
		{
			InterpEd->CopySelectedGroupOrTrack(FALSE);
		}
		else if(ParseCommand(&Str, TEXT("Paste")))
		{
			InterpEd->PasteSelectedGroupOrTrack();
		}
		else if(ParseCommand(&Str, TEXT("Play")))
		{
			InterpEd->StartPlaying(FALSE);
		}
		else if(ParseCommand(&Str, TEXT("Stop")))
		{
			InterpEd->StopPlaying();
		}
		else if(ParseCommand(&Str, TEXT("Rewind")))
		{
			InterpEd->SetInterpPosition(0.f);
		}
		else if(ParseCommand(&Str, TEXT("TogglePlayPause")))
		{
			if(InterpEd->Interp->bIsPlaying)
			{
				InterpEd->StopPlaying();
			}
			else
			{
				InterpEd->StartPlaying(FALSE);
			}
		}
		else if(ParseCommand(&Str, TEXT("DeleteSelection")))
		{
			InterpEd->DeleteSelectedKeys(TRUE);
		}
		else if(ParseCommand(&Str, TEXT("MarkInSection")))
		{
			InterpEd->MoveLoopMarker(InterpEd->Interp->Position, TRUE);
		}
		else if(ParseCommand(&Str, TEXT("MarkOutSection")))
		{
			InterpEd->MoveLoopMarker(InterpEd->Interp->Position, FALSE);
		}
		else if(ParseCommand(&Str, TEXT("CropAnimationBeginning")))
		{
			InterpEd->CropAnimKey(TRUE);
		}
		else if(ParseCommand(&Str, TEXT("CropAnimationEnd")))
		{
			InterpEd->CropAnimKey(FALSE);
		}
		else if(ParseCommand(&Str, TEXT("IncrementPosition")))
		{
			InterpEd->IncrementSelection();
		}
		else if(ParseCommand(&Str, TEXT("DecrementPosition")))
		{
			InterpEd->DecrementSelection();
		}
		else if(ParseCommand(&Str, TEXT("MoveToNextKey")))
		{
			InterpEd->SelectNextKey();
		}
		else if(ParseCommand(&Str, TEXT("MoveToPrevKey")))
		{
			InterpEd->SelectPreviousKey();
		}
		else if(ParseCommand(&Str, TEXT("SplitAnimKey")))
		{
			InterpEd->SplitAnimKey();
		}
		else if(ParseCommand(&Str, TEXT("ToggleSnap")))
		{
			InterpEd->SetSnapEnabled(!InterpEd->bSnapEnabled);
		}
		else if(ParseCommand(&Str, TEXT("MoveActiveUp")))
		{
			InterpEd->MoveActiveUp();
		}
		else if(ParseCommand(&Str, TEXT("MoveActiveDown")))
		{
			InterpEd->MoveActiveDown();
		}
		else if(ParseCommand(&Str, TEXT("AddKey")))
		{
			InterpEd->AddKey();
		}
		else if(ParseCommand(&Str, TEXT("DuplicateSelectedKeys")) )
		{
			InterpEd->DuplicateSelectedKeys();
		}
		else if(ParseCommand(&Str, TEXT("ViewFitSequence")) )
		{
			InterpEd->ViewFitSequence();
		}
		else if(ParseCommand(&Str, TEXT("ViewFitLoop")) )
		{
			InterpEd->ViewFitLoop();
		}
		else if(ParseCommand(&Str, TEXT("ViewFitLoopSequence")) )
		{
			InterpEd->ViewFitLoopSequence();
		}
		else if(ParseCommand(&Str, TEXT("ChangeKeyInterpModeAUTO")) )
		{
			InterpEd->ChangeKeyInterpMode(CIM_CurveAuto);
		}
		else if(ParseCommand(&Str, TEXT("ChangeKeyInterpModeUSER")) )
		{
			InterpEd->ChangeKeyInterpMode(CIM_CurveUser);
		}
		else if(ParseCommand(&Str, TEXT("ChangeKeyInterpModeBREAK")) )
		{
			InterpEd->ChangeKeyInterpMode(CIM_CurveBreak);
		}
		else if(ParseCommand(&Str, TEXT("ChangeKeyInterpModeLINEAR")) )
		{
			InterpEd->ChangeKeyInterpMode(CIM_Linear);
		}
		else if(ParseCommand(&Str, TEXT("ChangeKeyInterpModeCONSTANT")) )
		{
			InterpEd->ChangeKeyInterpMode(CIM_Constant);
		}
	}
}

/*-----------------------------------------------------------------------------
 WxInterpEdVCHolder
 -----------------------------------------------------------------------------*/


BEGIN_EVENT_TABLE( WxInterpEdVCHolder, wxWindow )
	EVT_SIZE( WxInterpEdVCHolder::OnSize )
END_EVENT_TABLE()

WxInterpEdVCHolder::WxInterpEdVCHolder( wxWindow* InParent, wxWindowID InID, WxInterpEd* InInterpEd )
: wxWindow( InParent, InID )
{
	SetMinSize(wxSize(2, 2));
	// Setup the vertical scroll bar
	// We want this on the LEFT side, so the tracks line up in Matinee
	ScrollBar_Vert = new wxScrollBar(this, IDM_INTERP_VERT_SCROLL_BAR, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
	wxRect rc = GetClientRect();
	wxRect rcSBV = ScrollBar_Vert->GetClientRect();
	// Left side determination
	ScrollBar_Vert->SetSize(0, 0, rcSBV.GetWidth(), rc.GetHeight());
	ScrollBar_Vert->SetThumbPosition(0);
	ScrollBar_Vert->SetScrollbar(0, 80, 2000, 80);

	// Create renderer viewport.
	InterpEdVC = new FInterpEdViewportClient( InInterpEd );
	InterpEdVC->Viewport = GEngine->Client->CreateWindowChildViewport(InterpEdVC, (HWND)GetHandle());
	InterpEdVC->Viewport->CaptureJoystickInput(false);
}

WxInterpEdVCHolder::~WxInterpEdVCHolder()
{
	DestroyViewport();
}

/**
 * Destroys the viewport held by this viewport holder, disassociating it from the engine, etc.  Rentrant.
 */
void WxInterpEdVCHolder::DestroyViewport()
{
	if ( InterpEdVC )
	{
		GEngine->Client->CloseViewport(InterpEdVC->Viewport);
		InterpEdVC->Viewport = NULL;
		delete InterpEdVC;
		InterpEdVC = NULL;
	}
}

/** Adjust the scroll bar size */
void WxInterpEdVCHolder::AdjustScrollBar(wxRect& rcClient)
{
	if(ScrollBar_Vert)
	{
		wxRect rcSBV = ScrollBar_Vert->GetClientRect();
		rcClient.x += rcSBV.GetWidth();
		ScrollBar_Vert->SetSize(0, 0, rcSBV.GetWidth(), rcClient.GetHeight());
	}
}

void WxInterpEdVCHolder::OnSize( wxSizeEvent& In )
{
	wxRect rc = GetClientRect();

	AdjustScrollBar(rc);

	::MoveWindow( (HWND)InterpEdVC->Viewport->GetWindow(), rc.x, rc.y, rc.GetWidth()-ScrollBar_Vert->GetRect().GetWidth(), rc.GetHeight(), 1 );
}


/*-----------------------------------------------------------------------------
 WxInterpEd
 -----------------------------------------------------------------------------*/

/**
 * Versioning Info for the Docking Parent layout file.
 */
namespace
{
	static const TCHAR* DockingParent_Name = TEXT("Matinee");
	static const INT DockingParent_Version = 0;		//Needs to be incremented every time a new dock window is added or removed from this docking parent.
}


UBOOL				WxInterpEd::bInterpTrackClassesInitialized = false;
TArray<UClass*>		WxInterpEd::InterpTrackClasses;

// On init, find all track classes. Will use later on to generate menus.
void WxInterpEd::InitInterpTrackClasses()
{
	if(bInterpTrackClassesInitialized)
		return;

	// Construct list of non-abstract gameplay sequence object classes.
	for(TObjectIterator<UClass> It; It; ++It)
	{
		if( It->IsChildOf(UInterpTrack::StaticClass()) && !(It->ClassFlags & CLASS_Abstract) )
		{
			InterpTrackClasses.AddItem(*It);
		}
	}

	bInterpTrackClassesInitialized = true;
}

BEGIN_EVENT_TABLE( WxInterpEd, wxFrame )
	EVT_SIZE( WxInterpEd::OnSize )
	EVT_CLOSE( WxInterpEd::OnClose )
	EVT_MENU( IDM_INTERP_FILE_EXPORT, WxInterpEd::OnMenuExport )
	EVT_MENU( IDM_INTERP_FILE_IMPORT, WxInterpEd::OnMenuImport )
	EVT_MENU_RANGE( IDM_INTERP_NEW_TRACK_START, IDM_INTERP_NEW_TRACK_END, WxInterpEd::OnContextNewTrack )
	EVT_MENU_RANGE( IDM_INTERP_KEYMODE_LINEAR, IDM_INTERP_KEYMODE_CONSTANT, WxInterpEd::OnContextKeyInterpMode )
	EVT_MENU( IDM_INTERP_EVENTKEY_RENAME, WxInterpEd::OnContextRenameEventKey )
	EVT_MENU( IDM_INTERP_KEY_SETTIME, WxInterpEd::OnContextSetKeyTime )
	EVT_MENU( IDM_INTERP_KEY_SETVALUE, WxInterpEd::OnContextSetValue )
	EVT_MENU( IDM_INTERP_KEY_SETCOLOR, WxInterpEd::OnContextSetColor )
	EVT_MENU( IDM_INTERP_ANIMKEY_LOOP, WxInterpEd::OnSetAnimKeyLooping )
	EVT_MENU( IDM_INTERP_ANIMKEY_NOLOOP, WxInterpEd::OnSetAnimKeyLooping )
	EVT_MENU( IDM_INTERP_ANIMKEY_SETSTARTOFFSET, WxInterpEd::OnSetAnimOffset )
	EVT_MENU( IDM_INTERP_ANIMKEY_SETENDOFFSET, WxInterpEd::OnSetAnimOffset )
	EVT_MENU( IDM_INTERP_ANIMKEY_SETPLAYRATE, WxInterpEd::OnSetAnimPlayRate )
	EVT_MENU( IDM_INTERP_ANIMKEY_TOGGLEREVERSE, WxInterpEd::OnToggleReverseAnim )
	EVT_UPDATE_UI( IDM_INTERP_ANIMKEY_TOGGLEREVERSE, WxInterpEd::OnToggleReverseAnim_UpdateUI )

	EVT_MENU( IDM_INTERP_CAMERA_ANIM_EXPORT, WxInterpEd::OnContextSaveAsCameraAnimation )

	EVT_MENU( IDM_INTERP_SoundKey_SetVolume, WxInterpEd::OnSetSoundVolume )
	EVT_MENU( IDM_INTERP_SoundKey_SetPitch, WxInterpEd::OnSetSoundPitch )
	EVT_MENU( IDM_INTERP_DIRKEY_SETTRANSITIONTIME, WxInterpEd::OnContextDirKeyTransitionTime )
	EVT_MENU( IDM_INTERP_TOGGLEKEY_FLIP, WxInterpEd::OnFlipToggleKey )
	EVT_MENU( IDM_INTERP_GROUP_RENAME, WxInterpEd::OnContextGroupRename )
	EVT_MENU( IDM_INTERP_GROUP_DUPLICATE, WxInterpEd::OnContextNewGroup )
	EVT_MENU( IDM_INTERP_GROUP_DELETE, WxInterpEd::OnContextGroupDelete )
	EVT_MENU( IDM_INTERP_GROUP_CREATETAB, WxInterpEd::OnContextGroupCreateTab )
	EVT_MENU( IDM_INTERP_GROUP_DELETETAB, WxInterpEd::OnContextDeleteGroupTab )
	EVT_MENU( IDM_INTERP_GROUP_REMOVEFROMTAB, WxInterpEd::OnContextGroupRemoveFromTab )
	EVT_MENU_RANGE( IDM_INTERP_GROUP_SENDTOTAB_START, IDM_INTERP_GROUP_SENDTOTAB_END, WxInterpEd::OnContextGroupSendToTab )

	EVT_MENU( IDM_INTERP_TRACK_RENAME, WxInterpEd::OnContextTrackRename )
	EVT_MENU( IDM_INTERP_TRACK_DELETE, WxInterpEd::OnContextTrackDelete )
	EVT_MENU( IDM_INTERP_TOGGLE_CURVEEDITOR, WxInterpEd::OnToggleCurveEd )
	EVT_MENU_RANGE( IDM_INTERP_TRACK_FRAME_WORLD, IDM_INTERP_TRACK_FRAME_RELINITIAL, WxInterpEd::OnContextTrackChangeFrame )
	EVT_MENU( IDM_INTERP_NEW_GROUP, WxInterpEd::OnContextNewGroup )
	EVT_MENU( IDM_INTERP_NEW_DIRECTOR_GROUP, WxInterpEd::OnContextNewGroup )
	EVT_MENU( IDM_INTERP_ADDKEY, WxInterpEd::OnMenuAddKey )
	EVT_MENU( IDM_INTERP_PLAY, WxInterpEd::OnMenuPlay )
	EVT_MENU( IDM_INTERP_PLAYLOOPSECTION, WxInterpEd::OnMenuPlay )
	EVT_MENU( IDM_INTERP_STOP, WxInterpEd::OnMenuStop )
	EVT_MENU_RANGE( IDM_INTERP_SPEED_1, IDM_INTERP_SPEED_100, WxInterpEd::OnChangePlaySpeed )
	
	EVT_MENU( IDM_OPEN_BINDKEYS_DIALOG, WxInterpEd::OnOpenBindKeysDialog )

	EVT_MENU( IDM_INTERP_EDIT_UNDO, WxInterpEd::OnMenuUndo )
	EVT_MENU( IDM_INTERP_EDIT_REDO, WxInterpEd::OnMenuRedo )
	EVT_MENU( IDM_INTERP_EDIT_CUT, WxInterpEd::OnMenuCut )
	EVT_MENU( IDM_INTERP_EDIT_COPY, WxInterpEd::OnMenuCopy )
	EVT_MENU( IDM_INTERP_EDIT_PASTE, WxInterpEd::OnMenuPaste )

	EVT_UPDATE_UI ( IDM_INTERP_EDIT_UNDO, WxInterpEd::OnMenuEdit_UpdateUI )
	EVT_UPDATE_UI ( IDM_INTERP_EDIT_REDO, WxInterpEd::OnMenuEdit_UpdateUI )
	EVT_UPDATE_UI ( IDM_INTERP_EDIT_CUT, WxInterpEd::OnMenuEdit_UpdateUI )
	EVT_UPDATE_UI ( IDM_INTERP_EDIT_COPY, WxInterpEd::OnMenuEdit_UpdateUI )
	EVT_UPDATE_UI ( IDM_INTERP_EDIT_PASTE, WxInterpEd::OnMenuEdit_UpdateUI )

	EVT_MENU( IDM_INTERP_MOVEKEY_SETLOOKUP, WxInterpEd::OnSetMoveKeyLookupGroup )
	EVT_MENU( IDM_INTERP_MOVEKEY_CLEARLOOKUP, WxInterpEd::OnClearMoveKeyLookupGroup )

	EVT_MENU( IDM_INTERP_TOGGLE_SNAP, WxInterpEd::OnToggleSnap )
	EVT_MENU( IDM_INTERP_VIEW_FITSEQUENCE, WxInterpEd::OnViewFitSequence )
	EVT_MENU( IDM_INTERP_VIEW_FITLOOP, WxInterpEd::OnViewFitLoop )
	EVT_MENU( IDM_INTERP_VIEW_FITLOOPSEQUENCE, WxInterpEd::OnViewFitLoopSequence )
	EVT_COMBOBOX( IDM_INTERP_SNAPCOMBO, WxInterpEd::OnChangeSnapSize )
	EVT_MENU( IDM_INTERP_EDIT_INSERTSPACE, WxInterpEd::OnMenuInsertSpace )
	EVT_MENU( IDM_INTERP_EDIT_STRETCHSECTION, WxInterpEd::OnMenuStretchSection )
	EVT_MENU( IDM_INTERP_EDIT_DELETESECTION, WxInterpEd::OnMenuDeleteSection )
	EVT_MENU( IDM_INTERP_EDIT_SELECTINSECTION, WxInterpEd::OnMenuSelectInSection )
	EVT_MENU( IDM_INTERP_EDIT_DUPLICATEKEYS, WxInterpEd::OnMenuDuplicateSelectedKeys )
	EVT_MENU( IDM_INTERP_EDIT_SAVEPATHTIME, WxInterpEd::OnSavePathTime )
	EVT_MENU( IDM_INTERP_EDIT_JUMPTOPATHTIME, WxInterpEd::OnJumpToPathTime )
	EVT_MENU( IDM_INTERP_EDIT_REDUCEKEYS, WxInterpEd::OnMenuReduceKeys )
	EVT_MENU( IDM_INTERP_VIEW_HIDE3DTRACKS, WxInterpEd::OnViewHide3DTracks )
	EVT_MENU( IDM_INTERP_VIEW_ZOOMTOSCUBPOS, WxInterpEd::OnViewZoomToScrubPos )


	EVT_SPLITTER_SASH_POS_CHANGED( IDM_INTERP_GRAPH_SPLITTER, WxInterpEd::OnGraphSplitChangePos )
	EVT_SCROLL( WxInterpEd::OnScroll )
END_EVENT_TABLE()


/** Should NOT open an InterpEd unless InInterp has a valid InterpData attached! */
WxInterpEd::WxInterpEd( wxWindow* InParent, wxWindowID InID, USeqAct_Interp* InInterp )
	:	wxFrame( InParent, InID, *LocalizeUnrealEd("Matinee"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR )
	,	FDockingParent(this)
	,	bClosed( FALSE )
{
	// Make sure we have a list of available track classes
	WxInterpEd::InitInterpTrackClasses();

	ThumbPos_Vert = 0;

	GConfig->GetBool( TEXT("Matinee"), TEXT("Hide3DTracks"), bHide3DTrackView, GEditorIni );
	GConfig->GetBool( TEXT("Matinee"), TEXT("ZoomToScrubPos"), bZoomToScrubPos, GEditorIni );
	GConfig->GetBool( TEXT("Matinee"), TEXT("ShowCurveEd"), bShowCurveEd, GEditorIni );

	// We want to update the Attached array of each Actor, so that we can move attached things then their base moves.
	for( FActorIterator It; It; ++It )
	{
		AActor* Actor = *It;
		if(Actor && !Actor->bDeleteMe)
		{
			Actor->EditorUpdateBase();
		}
	}


	// Create options object.
	Opt = ConstructObject<UInterpEdOptions>( UInterpEdOptions::StaticClass(), INVALID_OBJECT, NAME_None, RF_Transactional );
	check(Opt);

	// Swap out regular UTransactor for our special one
	GEditor->Trans->Reset( *LocalizeUnrealEd("OpenMatinee") );

	NormalTransactor = GEditor->Trans;
	InterpEdTrans = new UInterpEdTransBuffer( 8*1024*1024 );
	GEditor->Trans = InterpEdTrans;

	// Set up pointers to interp objects
	Interp = InInterp;

	// Do all group/track instancing and variable hook-up.
	Interp->InitInterp();

	// Flag this action as 'being edited'
	Interp->bIsBeingEdited = TRUE;

	// Should always find some data.
	check(Interp->InterpData);
	IData = Interp->InterpData;

	// Set the default filter for the data
	if(IData->DefaultFilters.Num())
	{
		SetSelectedFilter(IData->DefaultFilters(0));
	}
	else
	{
		SetSelectedFilter(NULL);
	}

	// Slight hack to ensure interpolation data is transactional.
	Interp->SetFlags( RF_Transactional );
	IData->SetFlags( RF_Transactional );
	for(INT i=0; i<IData->InterpGroups.Num(); i++)
	{
		UInterpGroup* Group = IData->InterpGroups(i);
		Group->SetFlags( RF_Transactional );

		for(INT j=0; j<Group->InterpTracks.Num(); j++)
		{
			Group->InterpTracks(j)->SetFlags( RF_Transactional );
		}
	}

	// For each track let it save the state of the object its going to work on before being changed at all by Matinee.
	for(INT i=0; i<Interp->GroupInst.Num(); i++)
	{
		UInterpGroupInst* GrInst = Interp->GroupInst(i);
		GrInst->SaveGroupActorState();

		AActor* GroupActor = GrInst->GetGroupActor();
		if ( GroupActor )
		{
			// Save off the transforms of the actor and anything directly or indirectly attached to it.
			Interp->SaveActorTransforms( GroupActor );

			if ( GroupActor->bStatic )
			{
				UBOOL bHasTrack = FALSE;
				FString TrackNames;

				for(INT TrackIdx=0; TrackIdx<GrInst->Group->InterpTracks.Num(); TrackIdx++)
				{
					if( GrInst->Group->InterpTracks(TrackIdx)->AllowStaticActors()==FALSE )
					{
						bHasTrack = TRUE;
						TrackNames += GrInst->Group->InterpTracks(TrackIdx)->GetClass()->GetDescription();
						TrackNames += "\n";
					}
				}

				if(bHasTrack)
				{
					// Warn if any groups with dynamic tracks are trying to act on bStatic actors!
					appMsgf( AMT_OK, *LocalizeUnrealEd("GroupOnStaticActor"), *GrInst->Group->GroupName.ToString(), 
						*GroupActor->GetName(), *TrackNames );
				}
			}
		}
	}

	// Set position to the start of the interpolation.
	// Will position objects as the first frame of the sequence.
	Interp->UpdateInterp(0.f, true);

	ActiveGroup = NULL;
	ActiveTrackIndex = INDEX_NONE;

	CamViewGroup = NULL;

	bAdjustingKeyframe = false;
	bLoopingSection = false;
	bDragging3DHandle = false;

	PlaybackSpeed = 1.0f;

	// Update cam frustum colours.
	UpdateCamColours();

	// Create other windows and stuff
	GraphSplitterWnd = new wxSplitterWindow(this, IDM_INTERP_GRAPH_SPLITTER, wxDefaultPosition, wxSize(100, 100), wxSP_3DBORDER|wxSP_FULLSASH );
	GraphSplitterWnd->SetMinimumPaneSize(40);

	PropertyWindow = new WxPropertyWindow;
	PropertyWindow->Create( this, this );

	TrackWindow = new WxInterpEdVCHolder( GraphSplitterWnd, -1, this );
	InterpEdVC = TrackWindow->InterpEdVC;

	// Create new curve editor setup if not already done
	if(!IData->CurveEdSetup)
	{
		IData->CurveEdSetup = ConstructObject<UInterpCurveEdSetup>( UInterpCurveEdSetup::StaticClass(), IData, NAME_None, RF_NotForClient | RF_NotForServer );
	}

	// Create graph editor to work on InterpData's CurveEd setup.
	CurveEd = new WxCurveEditor( GraphSplitterWnd, -1, IData->CurveEdSetup );

	// Register this window with the Curve editor so we will be notified of various things.
	CurveEd->SetNotifyObject(this);

	// Set graph view to match track view.
	SyncCurveEdView();

	PosMarkerColor = PositionMarkerLineColor;
	RegionFillColor = LoopRegionFillColor;

	CurveEd->SetEndMarker(true, IData->InterpLength);
	CurveEd->SetPositionMarker(true, 0.f, PosMarkerColor);
	CurveEd->SetRegionMarker(true, IData->EdSectionStart, IData->EdSectionEnd, RegionFillColor);

    // Initially the curve editor is not visible.
	GraphSplitterWnd->Initialize(TrackWindow);

	// Add docking windows.
	{
		AddDockingWindow(PropertyWindow, FDockingParent::DH_Bottom, *LocalizeUnrealEd("Properties"));

		wxPane* PreviewPane = new wxPane( this );
		{
			PreviewPane->ShowHeader(false);
			PreviewPane->ShowCloseButton( false );
			PreviewPane->SetClient(GraphSplitterWnd);
		}
		LayoutManager->SetLayout( wxDWF_SPLITTER_BORDERS, PreviewPane );

		LoadDockingLayout();
	}

	// Create toolbar
	ToolBar = new WxInterpEdToolBar( this, -1 );
	SetToolBar( ToolBar );

	// Initialise snap settings.
	bSnapToKeys = FALSE;
	bSnapEnabled = FALSE;
	bSnapToFrames = FALSE;	

	// Restore selected snap mode from INI.
	GConfig->GetBool( TEXT("Matinee"), TEXT("SnapEnabled"), bSnapEnabled, GEditorIni );
	INT SelectedSnapMode = 3; // default 0.5 sec
	GConfig->GetInt(TEXT("Matinee"), TEXT("SelectedSnapMode"), SelectedSnapMode, GEditorIni);
	ToolBar->SnapCombo->Select(SelectedSnapMode);	
	wxCommandEvent In;
	In.SetInt(SelectedSnapMode);
	OnChangeSnapSize(In);
	// Update snap button & synchronize with curve editor
	SetSnapEnabled( bSnapEnabled );
	
	// Create menu bar
	MenuBar = new WxInterpEdMenuBar(this);
	AppendDockingMenu(MenuBar);
	SetMenuBar( MenuBar );

	// Will look at current selection to set active track
	ActorSelectionChange();

	// Load gradient texture for bars
	BarGradText = LoadObject<UTexture2D>(NULL, TEXT("EditorMaterials.MatineeGreyGrad"), NULL, LOAD_None, NULL);

	// If there is a Director group in this data, default to locking the camera to it.
	UInterpGroupDirector* DirGroup = IData->FindDirectorGroup();
	if(DirGroup)
	{
		LockCamToGroup(DirGroup);
	}

	for(INT i=0; i<4; i++)
	{
		WxLevelViewportWindow* LevelVC = GApp->EditorFrame->ViewportConfigData->Viewports[i].ViewportWindow;
		if(LevelVC)
		{
			// If there is a director group, set the perspective viewports to realtime automatically.
			if(LevelVC->ViewportType == LVT_Perspective)
			{				
				LevelVC->SetRealtime(TRUE);
			}

			// Turn on 'show camera frustums' flag
			LevelVC->ShowFlags |= SHOW_CamFrustums;
		}
	}

	// Update UI to reflect any change in realtime status
	GCallbackEvent->Send( CALLBACK_UpdateUI );

	// Load the desired window position from .ini file
	FWindowUtil::LoadPosSize(TEXT("Matinee"), this, -1, -1, 800,800);

	// Load a default splitter position for the curve editor.
	const UBOOL bSuccess = GConfig->GetInt(TEXT("Matinee"), TEXT("SplitterPos"), GraphSplitPos, GEditorUserSettingsIni);

	if(bSuccess == FALSE)
	{
		GraphSplitPos = 240;
	}

	GraphSplitterWnd->SetSashPosition(GraphSplitPos);
	
	// Show curve editor.
	if ( bShowCurveEd )
	{
		wxCommandEvent In;
		bShowCurveEd = !bShowCurveEd; // invert for toggle
		OnToggleCurveEd(In);
	}
}

WxInterpEd::~WxInterpEd()
{
	// Save window position/size
	FWindowUtil::SavePosSize(TEXT("Matinee"), this);

	// Save the default splitter position for the curve editor.
	GConfig->SetInt(TEXT("Matinee"), TEXT("SplitterPos"), GraphSplitPos, GEditorUserSettingsIni);

	// Save docking layout.
	SaveDockingLayout();
}

void WxInterpEd::Serialize(FArchive& Ar)
{
	Ar << Interp;
	Ar << IData;
	Ar << NormalTransactor;
	Ar << Opt;

	// Check for non-NULL, as these references will be cleared in OnClose.
	if ( CurveEd )
	{
		CurveEd->CurveEdVC->Serialize(Ar);
	}
	if ( InterpEdVC )
	{
		InterpEdVC->Serialize(Ar);
	}
}

/** Starts playing the current sequence. */
void WxInterpEd::StartPlaying(UBOOL bPlayLoop)
{
	if(Interp->bIsPlaying)
		return;

	bAdjustingKeyframe = FALSE;

	bLoopingSection = bPlayLoop;
	if(bLoopingSection)
	{
		// If looping - jump to start of looping section.
		Interp->UpdateInterp(IData->EdSectionStart, TRUE, TRUE);
	}

	Interp->bReversePlayback = FALSE;
	Interp->bIsPlaying = TRUE;

	InterpEdVC->SetRealtime( TRUE ); // So viewport client will get ticked.
}

/** Stops playing the current sequence. */
void WxInterpEd::StopPlaying()
{
	// If already stopped, pressing stop again winds you back to the beginning.
	if(!Interp->bIsPlaying)
	{
		SetInterpPosition(0.f);
		return;
	}

	// Iterate over each group/track giving it a chance to stop things.
	for(INT i=0; i<Interp->GroupInst.Num(); i++)
	{
		UInterpGroupInst* GrInst = Interp->GroupInst(i);
		UInterpGroup* Group = GrInst->Group;

		check(Group->InterpTracks.Num() == GrInst->TrackInst.Num());
		for(INT j=0; j<Group->InterpTracks.Num(); j++)
		{
			UInterpTrack* Track = Group->InterpTracks(j);
			UInterpTrackInst* TrInst = GrInst->TrackInst(j);

			Track->PreviewStopPlayback(TrInst);
		}
	}

	// Set flag to indicate stopped
	Interp->bIsPlaying = FALSE;

	// Stop viewport being realtime
	InterpEdVC->SetRealtime( FALSE );
}

/** Creates a popup context menu based on the item under the mouse cursor.
* @param	Viewport	FViewport for the FInterpEdViewportClient.
* @param	HitResult	HHitProxy returned by FViewport::GetHitProxy( ).
* @return	A new wxMenu with context-appropriate menu options or NULL if there are no appropriate menu options.
*/
wxMenu	*WxInterpEd::CreateContextMenu( FViewport *Viewport, const HHitProxy *HitResult )
{
	wxMenu	*Menu = NULL;

	if(HitResult->IsA(HInterpEdTrackBkg::StaticGetType()))
	{
		ClearKeySelection();
		SetActiveTrack(NULL, INDEX_NONE);				
		Viewport->Invalidate();

		Menu = new WxMBInterpEdBkgMenu( this );
	}
	else if(HitResult->IsA(HInterpEdGroupTitle::StaticGetType()))
	{
		UInterpGroup* Group = ((HInterpEdGroupTitle*)HitResult)->Group;

		ClearKeySelection();
		SetActiveTrack(Group, INDEX_NONE);
		Viewport->Invalidate();

		Menu = new WxMBInterpEdGroupMenu( this );		
	}
	else if(HitResult->IsA(HInterpEdTrackTitle::StaticGetType()))
	{
		UInterpGroup* Group = ((HInterpEdTrackTitle*)HitResult)->Group;
		const INT TrackIndex = ((HInterpEdTrackTitle*)HitResult)->TrackIndex;

		ClearKeySelection();
		SetActiveTrack(Group, TrackIndex);
		Viewport->Invalidate();

		Menu = new WxMBInterpEdTrackMenu( this );
	}
	else if(HitResult->IsA(HInterpTrackKeypointProxy::StaticGetType()))
	{
		UInterpGroup* Group = ((HInterpTrackKeypointProxy*)HitResult)->Group;
		const INT TrackIndex = ((HInterpTrackKeypointProxy*)HitResult)->TrackIndex;
		const INT KeyIndex = ((HInterpTrackKeypointProxy*)HitResult)->KeyIndex;

		const UBOOL bAlreadySelected = KeyIsInSelection(Group, TrackIndex, KeyIndex);
		if(bAlreadySelected)
		{
			Menu = new WxMBInterpEdKeyMenu( this );
		}
	}
	else if(HitResult->IsA(HInterpEdTab::StaticGetType()))
	{
		UInterpFilter* Filter = ((HInterpEdTab*)HitResult)->Filter;

		SetSelectedFilter(Filter);
		Viewport->Invalidate();

		Menu = new WxMBInterpEdTabMenu( this );
	}

	return Menu;
}

void WxInterpEd::OnSize( wxSizeEvent& In )
{
	In.Skip();
}

void WxInterpEd::OnClose( wxCloseEvent& In )
{
	// Re-instate regular transactor
	check( GEditor->Trans == InterpEdTrans );
	check( NormalTransactor->IsA( UTransBuffer::StaticClass() ) );

	GEditor->Trans->Reset( *LocalizeUnrealEd("ExitMatinee") );
	GEditor->Trans = NormalTransactor;

	// Detach editor camera from any group and clear any previewing stuff.
	LockCamToGroup(NULL);

	// Restore the saved state of any actors we were previewing interpolation on.
	for(INT i=0; i<Interp->GroupInst.Num(); i++)
	{
		// Restore Actors to the state they were in when we opened Matinee.
		Interp->GroupInst(i)->RestoreGroupActorState();

		// Call TermTrackInst, but don't actually delete them. Leave them for GC.
		// Because we don't delete groups/tracks so undo works, we could end up deleting the Outer of a valid object.
		Interp->GroupInst(i)->TermGroupInst(false);

		// Set any manipulated cameras back to default frustum colours.
		ACameraActor* Cam = Cast<ACameraActor>(Interp->GroupInst(i)->GroupActor);
		if(Cam && Cam->DrawFrustum)
		{
			ACameraActor* DefCam = (ACameraActor*)(Cam->GetClass()->GetDefaultActor());
			Cam->DrawFrustum->FrustumColor = DefCam->DrawFrustum->FrustumColor;
		}
	}

	// Movement tracks no longer save/restore relative actor positions.  Instead, the SeqAct_interp
	// stores actor positions/orientations so they can be precisely restored on Matinee close.
	// Note that this must happen before Interp's GroupInst array is emptied.
	Interp->RestoreActorTransforms();

	Interp->GroupInst.Empty();
	Interp->InterpData = NULL;

	// Indicate action is no longer being edited.
	Interp->bIsBeingEdited = FALSE;

	// Reset interpolation to the beginning when quitting.
	Interp->bIsPlaying = FALSE;
	Interp->Position = 0.f;

	bAdjustingKeyframe = FALSE;

	// When they close the window - change the mode away from InterpEdit.
	if( GEditorModeTools().GetCurrentModeID() == EM_InterpEdit )
	{
		FEdModeInterpEdit* InterpEditMode = (FEdModeInterpEdit*)GEditorModeTools().GetCurrentMode();

		// Only change mode if this window closing wasn't instigated by someone changing mode!
		if( !InterpEditMode->bLeavingMode )
		{
			InterpEditMode->InterpEd = NULL;
			GEditorModeTools().SetCurrentMode( EM_Default );
		}
	}

	// Undo any weird settings to editor level viewports.
	for(INT i=0; i<4; i++)
	{
		WxLevelViewportWindow* LevelVC = GApp->EditorFrame->ViewportConfigData->Viewports[i].ViewportWindow;
		if(LevelVC)
		{
			// Turn off realtime when exiting.
			if(LevelVC->ViewportType == LVT_Perspective)
			{				
				LevelVC->SetRealtime(FALSE);
			}

			// Turn off 'show camera frustums' flag.
			LevelVC->ShowFlags &= ~SHOW_CamFrustums;
		}
	}

	// Un-highlight selected track.
	if(ActiveGroup && ActiveTrackIndex != INDEX_NONE)
	{
		UInterpTrack* Track = ActiveGroup->InterpTracks(ActiveTrackIndex);
		IData->CurveEdSetup->ChangeCurveColor(Track, ActiveGroup->GroupColor);
	}

	// Update UI to reflect any change in realtime status
	GCallbackEvent->Send( CALLBACK_UpdateUI );

	// Redraw viewport as well, to show reset state of stuff.
	GCallbackEvent->Send( CALLBACK_RedrawAllViewports );

	// Clear references to serialized members so they won't be serialized in the time
	// between the window closing and wx actually deleting it.
	bClosed = TRUE;
	Interp = NULL;
	IData = NULL;
	NormalTransactor = NULL;
	Opt = NULL;
	CurveEd = NULL;

	// Destroy the viewport, disassociating it from the engine, etc.
	TrackWindow->DestroyViewport();
	InterpEdVC = NULL;

	// Destroy the window.
	this->Destroy();
}


/** Handle scrolling */
void WxInterpEd::OnScroll(wxScrollEvent& In)
{
	wxScrollBar* InScrollBar = wxDynamicCast(In.GetEventObject(), wxScrollBar);
	if (InScrollBar) 
	{
		ThumbPos_Vert = -In.GetPosition();

		if (InterpEdVC->Viewport)
		{
			InterpEdVC->Viewport->Invalidate();
			// Force it to draw so the view change is seen
			InterpEdVC->Viewport->Draw();
		}
	}
}

void WxInterpEd::DrawTracks3D(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	for(INT i=0; i<Interp->GroupInst.Num(); i++)
	{
		UInterpGroupInst* GrInst = Interp->GroupInst(i);
		check( GrInst->Group );
		check( GrInst->TrackInst.Num() == GrInst->Group->InterpTracks.Num() );

		// In 3D viewports, Don't draw path if we are locking the camera to this group.
		//if( !(View->Family->ShowFlags & SHOW_Orthographic) && (Group == CamViewGroup) )
		//	continue;

		for(INT j=0; j<GrInst->TrackInst.Num(); j++)
		{
			UInterpTrackInst* TrInst = GrInst->TrackInst(j);
			UInterpTrack* Track = GrInst->Group->InterpTracks(j);

			//UBOOL bTrackSelected = ((GrInst->Group == ActiveGroup) && (j == ActiveTrackIndex));
			UBOOL bTrackSelected = (GrInst->Group == ActiveGroup);
			FColor TrackColor = bTrackSelected ? Track3DSelectedColor : GrInst->Group->GroupColor;

			Track->Render3DTrack( TrInst, View, PDI, j, TrackColor, Opt->SelectedKeys);
		}
	}
}

void WxInterpEd::DrawModeHUD(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas)
{
	// Show a notification if we are adjusting a particular keyframe.
	if(bAdjustingKeyframe)
	{
		check(Opt->SelectedKeys.Num() == 1);

		FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
		FString AdjustNotify = FString::Printf( *LocalizeUnrealEd("AdjustKey_F"), SelKey.KeyIndex );

		INT XL, YL;
		StringSize(GEngine->SmallFont, XL, YL, *AdjustNotify);
		DrawString(Canvas, 3, Viewport->GetSizeY() - (3 + YL) , *AdjustNotify, GEngine->SmallFont, FLinearColor::White );
	}
}


///////////////////////////////////////////////////////////////////////////////////////
// Properties window NotifyHook stuff

void WxInterpEd::NotifyDestroy( void* Src )
{

}

void WxInterpEd::NotifyPreChange( void* Src, UProperty* PropertyAboutToChange )
{

}

void WxInterpEd::NotifyPostChange( void* Src, UProperty* PropertyThatChanged )
{
	CurveEd->CurveChanged();
	InterpEdVC->Viewport->Invalidate();

	// If we are changing the properties of a Group, propagate changes to the GroupAnimSets array to the Actors being controlled by this group.
	if(ActiveGroup && ActiveTrackIndex == INDEX_NONE && ActiveGroup->HasAnimControlTrack())
	{
		for(INT i=0; i<Interp->GroupInst.Num(); i++)
		{
			if(Interp->GroupInst(i)->Group == ActiveGroup)
			{
				AActor* Actor = Interp->GroupInst(i)->GetGroupActor();
				if(Actor)
				{
					Actor->PreviewBeginAnimControl(ActiveGroup->GroupAnimSets);
				}
			}
		}

		// Update to current position - so changes in AnimSets take affect now.
		SetInterpPosition(Interp->Position);
	}
}

void WxInterpEd::NotifyExec( void* Src, const TCHAR* Cmd )
{
	
}

///////////////////////////////////////////////////////////////////////////////////////
// Curve editor notify stuff

/** Implement Curve Editor notify interface, so we can back up state before changes and support Undo. */
void WxInterpEd::PreEditCurve(TArray<UObject*> CurvesAboutToChange)
{
	InterpEdTrans->BeginSpecial(*LocalizeUnrealEd("CurveEdit"));

	// Call Modify on all tracks with keys selected
	for(INT i=0; i<CurvesAboutToChange.Num(); i++)
	{
		// If this keypoint is from an InterpTrack, call Modify on it to back up its state.
		UInterpTrack* Track = Cast<UInterpTrack>( CurvesAboutToChange(i) );
		if(Track)
		{
			Track->Modify();
		}
	}
}

void WxInterpEd::PostEditCurve()
{
	InterpEdTrans->EndSpecial();
}

void WxInterpEd::MovedKey()
{
	// Update interpolation to the current position - but thing may have changed due to fiddling on the curve display.
	SetInterpPosition(Interp->Position);
}

void WxInterpEd::DesireUndo()
{
	InterpEdUndo();
}

void WxInterpEd::DesireRedo()
{
	InterpEdRedo();
}

///////////////////////////////////////////////////////////////////////////////////////
// FDockingParent Interface

/**
 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
 *  @return A string representing a name to use for this docking parent.
 */
const TCHAR* WxInterpEd::GetDockingParentName() const
{
	return DockingParent_Name;
}

/**
 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
 */
const INT WxInterpEd::GetDockingParentVersion() const
{
	return DockingParent_Version;
}


///////////////////////////////////////////////////////////////////////////////////////
// WxCameraAnimEd editor

WxCameraAnimEd::WxCameraAnimEd( wxWindow* InParent, wxWindowID InID, class USeqAct_Interp* InInterp )
:	WxInterpEd(InParent, InID, InInterp)
{
	SetTitle(*LocalizeUnrealEd("MatineeCamAnimMode"));
}

WxCameraAnimEd::~WxCameraAnimEd()
{

}

/** Creates a popup context menu based on the item under the mouse cursor.
* @param	Viewport	FViewport for the FInterpEdViewportClient.
* @param	HitResult	HHitProxy returned by FViewport::GetHitProxy( ).
* @return	A new wxMenu with context-appropriate menu options or NULL if there are no appropriate menu options.
*/
wxMenu	*WxCameraAnimEd::CreateContextMenu( FViewport *Viewport, const HHitProxy *HitResult )
{
	wxMenu	*Menu = NULL;

	if(HitResult->IsA(HInterpEdTrackBkg::StaticGetType()))
	{
		// no menu, explicitly ignore this case
	}
	else if(HitResult->IsA(HInterpEdGroupTitle::StaticGetType()))
	{
		UInterpGroup* Group = ((HInterpEdGroupTitle*)HitResult)->Group;

		ClearKeySelection();
		SetActiveTrack(Group, INDEX_NONE);
		Viewport->Invalidate();

		Menu = new WxMBCameraAnimEdGroupMenu( this );		
	}
	else 
	{
		// let our parent handle it
		Menu = WxInterpEd::CreateContextMenu(Viewport, HitResult);
	}

	return Menu;
}

void WxCameraAnimEd::OnClose(wxCloseEvent& In)
{
	// delete the attached objects.  this should take care of the temp camera actor.
	TArray<UObject**> ObjectVars;
	Interp->GetObjectVars(ObjectVars);
	for (INT Idx=0; Idx<ObjectVars.Num(); ++Idx)
	{
		AActor* Actor = Cast<AActor>(*(ObjectVars(Idx)));

		// prevents a NULL in the selection
		if (Actor->IsSelected())
		{
			GEditor->GetSelectedActors()->Deselect(Actor);
		}

		GWorld->DestroyActor(Actor);
	}

	// need to destroy all of the temp kismet stuff
	TArray<USequenceObject*> SeqsToDelete;
	{
		SeqsToDelete.AddItem(Interp);

		if (Interp->InterpData)
		{
			// delete everything linked to the temp interp.  this should take care of
			// both the interpdata and the cameraactor's seqvar_object
			for (INT Idx=0; Idx<Interp->VariableLinks.Num(); ++Idx)
			{
				FSeqVarLink* Link = &Interp->VariableLinks(Idx);

				for (INT VarIdx=0; VarIdx<Link->LinkedVariables.Num(); ++VarIdx)
				{
					SeqsToDelete.AddItem(Link->LinkedVariables(VarIdx));
				}
			}
		}
	}
	USequence* RootSeq = Interp->GetRootSequence();
	RootSeq->RemoveObjects(SeqsToDelete);

	// make sure destroyed actors get flushed asap
	GWorld->GetWorldInfo()->ForceGarbageCollection();

	// Fill in AnimLength parameter, in case it changed during editing
	if ( Interp && Interp->InterpData && (Interp->InterpData->InterpGroups.Num() > 0) )
	{
		UCameraAnim* const CamAnim = Cast<UCameraAnim>(Interp->InterpData->InterpGroups(0)->GetOuter());
		if (CamAnim)
		{
			if (CamAnim->AnimLength != Interp->InterpData->InterpLength)
			{
				CamAnim->AnimLength = Interp->InterpData->InterpLength;
				CamAnim->MarkPackageDirty(TRUE);
			}
		}
	}

	// update generic browser to reflect any changes we've made
	WxGenericBrowser* GBrowser = GUnrealEd->GetBrowser<WxGenericBrowser>( TEXT("GenericBrowser") );
	GBrowser->Update();	
	
	// clean up any open property windows, in case one of them points to something we just deleted
	GUnrealEd->UpdatePropertyWindows();

	WxInterpEd::OnClose(In);
}

void WxCameraAnimEd::UpdateCameraToGroup()
{
	// Make sure camera's PP settings get applied additively.
	AActor* ViewedActor = GetViewedActor();
	if(ViewedActor)
	{
		ACameraActor* Cam = Cast<ACameraActor>(ViewedActor);
		ACameraActor const* const DefaultCamActor = ACameraActor::StaticClass()->GetDefaultObject<ACameraActor>();
		FPostProcessSettings const* const DefaultCamActorPPSettings = DefaultCamActor ? &DefaultCamActor->CamOverridePostProcess : NULL;

		// Move any perspective viewports to coincide with moved actor.
		for(INT i=0; i<4; i++)
		{
			WxLevelViewportWindow* LevelVC = GApp->EditorFrame->ViewportConfigData->Viewports[i].ViewportWindow;
			if (LevelVC && (LevelVC->ViewportType == LVT_Perspective) && (Cam != NULL) && (DefaultCamActorPPSettings != NULL) )
			{
				LevelVC->bAdditivePostProcessSettings = TRUE;
				LevelVC->AdditivePostProcessSettings = Cam->CamOverridePostProcess;

				// subtract defaults to leave only the additive portion
				LevelVC->AdditivePostProcessSettings.AddInterpolatable(*DefaultCamActorPPSettings, -1.f);
			}
			else
			{
				LevelVC->bAdditivePostProcessSettings = FALSE;
			}
		}
	}

	WxInterpEd::UpdateCameraToGroup();
}




