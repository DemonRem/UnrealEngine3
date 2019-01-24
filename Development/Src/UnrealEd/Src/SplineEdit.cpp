/*=============================================================================
	SplineEdit.cpp
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "EngineSplineClasses.h"
#include "ScopedTransaction.h"
#include "SplineEdit.h"

/** Util to get the forst two selected SplineActors */
static void GetFirstTwoSelectedSplineActors(ASplineActor* &SplineA, ASplineActor* &SplineB)
{
	SplineA = NULL;
	SplineB = NULL;
	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		ASplineActor* Spline = Cast<ASplineActor>( *It );
		if(Spline)
		{
			if(!SplineA)
			{
				SplineA = Spline;
			}
			else if(!SplineB)
			{
				SplineB = Spline;
				break; // have our 2 splines, exit now
			}
		}
	}
}

/** Util to break a spline, given the hit proxy that was clicked on */
void SplineBreakGivenProxy(HSplineProxy* SplineProxy)
{
	USplineComponent* SplineComp = SplineProxy->SplineComp;

	// Find the SplineActor that owns this component, and the SplineActor it connects to.
	ASplineActor* SplineActor = Cast<ASplineActor>(SplineComp->GetOwner());
	ASplineActor* TargetActor = NULL;
	if(SplineActor)
	{
		TargetActor = SplineActor->FindTargetForComponent(SplineComp);
	}

	// If we have a source and target, and holding alt, break connection
	if(SplineActor && TargetActor)
	{
		const FScopedTransaction Transaction( *LocalizeUnrealEd("Break Spline") );

		SplineActor->Modify();
		SplineActor->BreakConnectionTo(TargetActor);
	}
}

/*------------------------------------------------------------------------------
FEdModeSpline
------------------------------------------------------------------------------*/

FEdModeSpline::FEdModeSpline() : FEdMode()
{
	ID = EM_Spline;
	Desc = TEXT("Spline Editing");

	ModSplineActor = NULL;
	bModArrive = FALSE;

	TangentHandleScale = 0.5f;
}

FEdModeSpline::~FEdModeSpline()
{

}

void FEdModeSpline::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

	UBOOL bHitTesting = PDI->IsHitTesting();
	for ( FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It )
	{
		ASplineActor *Spline = Cast<ASplineActor>(*It);
		if (Spline != NULL && Spline->IsSelected())
		{
			// Handles
			FVector LeaveHandlePos = Spline->Location + (Spline->GetWorldSpaceTangent() * TangentHandleScale);
			FVector ArriveHandlePos = Spline->Location - (Spline->GetWorldSpaceTangent() * TangentHandleScale);

			// Line
			PDI->DrawLine(LeaveHandlePos, ArriveHandlePos, FColor(255,255,255), SDPG_Foreground);

			// Leave handle
			if(bHitTesting) PDI->SetHitProxy( new HSplineHandleProxy(Spline, FALSE) );
			PDI->DrawPoint(LeaveHandlePos, FColor(255,255,255), 5.f, SDPG_Foreground);
			if(bHitTesting) PDI->SetHitProxy( NULL );

			// Arrive handle		
			if(bHitTesting) PDI->SetHitProxy( new HSplineHandleProxy(Spline, TRUE) );
			PDI->DrawPoint(ArriveHandlePos, FColor(255,255,255), 5.f, SDPG_Foreground);
			if(bHitTesting) PDI->SetHitProxy( NULL );

						
			// Let the actor draw extra info when selected
			Spline->EditModeSelectedDraw(View, Viewport, PDI);
		}
	}
}



UBOOL FEdModeSpline::InputKey(FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, FName Key, EInputEvent Event)
{
	const UBOOL bAltDown = Viewport->KeyState( KEY_LeftAlt ) || Viewport->KeyState( KEY_RightAlt );

	if(Key == KEY_LeftMouseButton)
	{
		// Press mouse button
		if(Event == IE_Pressed)
		{		
			// See if we clicked on a spline handle..
			INT HitX = ViewportClient->Viewport->GetMouseX();
			INT HitY = ViewportClient->Viewport->GetMouseY();
			HHitProxy*	HitProxy = ViewportClient->Viewport->GetHitProxy(HitX, HitY);
			if(HitProxy)
			{			
				// If left clicking on a spline handle proxy
				if( HitProxy->IsA(HSplineHandleProxy::StaticGetType()) )
				{
					// save info about what we grabbed
					HSplineHandleProxy* SplineHandleProxy = (HSplineHandleProxy*)HitProxy;
					ModSplineActor = SplineHandleProxy->SplineActor;
					bModArrive = SplineHandleProxy->bArrive;
				}		
			}
		}
		// Release mouse button
		else if(Event == IE_Released)
		{
			// End any handle-tweaking
			if(ModSplineActor)
			{
				ModSplineActor->UpdateConnectedSplineComponents(TRUE);			
				ModSplineActor = NULL;
			}
	
			bModArrive = FALSE;
		}	
	}
	else if(Key == KEY_Period)
	{
		if(Event == IE_Pressed)
		{
			// Check we have two SplineActor selected 
			ASplineActor* SplineA = NULL;
			ASplineActor* SplineB = NULL;
			GetFirstTwoSelectedSplineActors(SplineA, SplineB);
			if(SplineA && SplineB)
			{
				GEditor->SplineConnect();
				ViewportClient->Invalidate();
				return TRUE;			
			}
		}
	}

	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
}

UBOOL FEdModeSpline::InputDelta(FEditorLevelViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale)
{
	if(ModSplineActor)
	{
		FVector InputDeltaDrag( InDrag );

		// scale to compensate for handle length
		InputDeltaDrag /= TangentHandleScale;


		// We seem to need this - not sure why...
		if ( InViewportClient->ViewportType == LVT_OrthoXY )
		{
			InputDeltaDrag.X = InDrag.Y;
			InputDeltaDrag.Y = -InDrag.X;
		}
		// Make it a bit more sensitive in perspective
		else if(InViewportClient->ViewportType == LVT_Perspective)
		{
			InputDeltaDrag *= 2.f;
		}

		//if we're using inverted panning
		if ((InViewportClient->ViewportType != LVT_Perspective) && InViewportClient->ShouldUseMoveCanvasMovement())
		{
			InputDeltaDrag = -InputDeltaDrag;
		}

		// Invert movement if grabbing arrive handle
		if(bModArrive)
		{
			InputDeltaDrag *= -1.f;
		}

		// Update thie actors tangent
		FVector LocalDeltaDrag = ModSplineActor->LocalToWorld().InverseTransformNormal(InputDeltaDrag);
		ModSplineActor->SplineActorTangent += LocalDeltaDrag;

		// And propagate to spline component
		ModSplineActor->UpdateConnectedSplineComponents(FALSE);

		return TRUE;
	}

	return FEdMode::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
}

/** Stop widget moving when adjusting handle */
UBOOL FEdModeSpline::AllowWidgetMove()
{
	if(ModSplineActor)
	{
		return FALSE;
	}
	else
	{
		return TRUE;	
	}
}

/** Called when actors duplicated, to link them into current spline  */
void FEdModeSpline::ActorsDuplicatedNotify(TArray<AActor*>& PreDuplicateSelection, TArray<AActor*>& PostDuplicateSelection, UBOOL bOffsetLocations)
{
	// If offsetting locations, this isn't an alt-drag - do nothing
	if(bOffsetLocations)
	{
		return;
	}

	INT NumActors = Min(PreDuplicateSelection.Num(), PostDuplicateSelection.Num());
	for(INT i=0; i<NumActors; i++)
	{
		ASplineActor* SourceSA = Cast<ASplineActor>(PreDuplicateSelection(i));
		ASplineActor* DestSA = Cast<ASplineActor>(PostDuplicateSelection(i));
		// Only allow connection between SplineActors in the same level
		if(SourceSA && DestSA && (SourceSA->GetLevel() == DestSA->GetLevel()))
		{
			// Clear all pointers from source node
			SourceSA->BreakAllConnectionsFrom();

			// Clear all connections on newly created node
			DestSA->BreakAllConnections();
			
			// Connect source point to destination			
			SourceSA->AddConnectionTo(DestSA);
			
			// Special case for loft actors - grab mesh from previous point and assign
			ASplineLoftActor* SourceSpline = Cast<ASplineLoftActor>(SourceSA);
			if(SourceSpline && (SourceSpline->LinksFrom.Num() > 0))
			{
				ASplineLoftActor* PrevSpline = Cast<ASplineLoftActor>(SourceSpline->LinksFrom(0));
				if(PrevSpline)
				{
					SourceSpline->DeformMesh = PrevSpline->DeformMesh;
				}
			}
			
		}	
	}
}

//////////////////////////////////////////////////////////////////////////
// UEditorEngine functions



/** Break all connections to all selected SplineActors */
void UEditorEngine::SplineBreakAll()
{
	const FScopedTransaction Transaction( *LocalizeUnrealEd("SplineBreakAll") );

	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		ASplineActor* Spline = Cast<ASplineActor>( *It );
		if(Spline)
		{
			Spline->BreakAllConnections(); // 
		}
	}
}

/** Create connection between first 2 selected SplineActors (or flip connection if already connected) */
void UEditorEngine::SplineConnect()
{
	const FScopedTransaction Transaction( *LocalizeUnrealEd("SplineConnect") );

	ASplineActor* SplineA = NULL;
	ASplineActor* SplineB = NULL;
	GetFirstTwoSelectedSplineActors(SplineA, SplineB);

	if(SplineA && SplineB)
	{
		// If they are connected, flip connection
		if(SplineA->IsConnectedTo(SplineB,FALSE))
		{
			SplineA->BreakConnectionTo(SplineB);
			SplineB->AddConnectionTo(SplineA);
		}
		else if(SplineB->IsConnectedTo(SplineA,FALSE))
		{
			SplineB->BreakConnectionTo(SplineA);
			SplineA->AddConnectionTo(SplineB);
		}
		// If they are not, connect them
		else
		{
			SplineA->AddConnectionTo(SplineB);
		}	
	}
}

/** Break any connections between selected SplineActors */
void UEditorEngine::SplineBreak()
{
	const FScopedTransaction Transaction( *LocalizeUnrealEd("SplineBreak") );

	// TODO: Handle more than 2!
	ASplineActor* SplineA = NULL;
	ASplineActor* SplineB = NULL;
	GetFirstTwoSelectedSplineActors(SplineA, SplineB);

	if(SplineA && SplineB)
	{
		// If they are connected, break
		if(SplineA->IsConnectedTo(SplineB,FALSE))
		{
			SplineA->BreakConnectionTo(SplineB);
		}
		else if(SplineB->IsConnectedTo(SplineA,FALSE))
		{
			SplineB->BreakConnectionTo(SplineA);
		}
	}
}

/** Util that reverses direction of all splines between selected SplineActors */
void UEditorEngine::SplineReverseAllDirections()
{
	const FScopedTransaction Transaction( *LocalizeUnrealEd("SplineReverseAllDirections") );


	// Struct for storing one connection
	struct FSplineConnection
	{
		ASplineActor* FromSpline;
		ASplineActor* ToSpline;
	};

	// First create array of all selected splines
	TArray<ASplineActor*> SelectedSplineActors;
	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		ASplineActor* Spline = Cast<ASplineActor>( *It );
		if(Spline)
		{
			Spline->Modify();
			SelectedSplineActors.AddItem(Spline);
		}
	}
	
	// Now find all connections and save them off
	TArray<FSplineConnection> CurrentConnections;
	// For each selected spline..
	for (INT i=0; i<SelectedSplineActors.Num(); i++)
	{
		ASplineActor* ThisSpline = SelectedSplineActors(i);
		// ..for each connection
		for(INT ConnIdx=0; ConnIdx<ThisSpline->Connections.Num(); ConnIdx++)
		{
			ASplineActor* ConnSpline = ThisSpline->Connections(ConnIdx).ConnectTo;
			// If it connects to something else in the selection set, remember it.
			if(ConnSpline && SelectedSplineActors.ContainsItem(ConnSpline))
			{
				INT NewIdx = CurrentConnections.AddZeroed();
				CurrentConnections(NewIdx).FromSpline = ThisSpline;
				CurrentConnections(NewIdx).ToSpline = ConnSpline;				
			}
		}		
	}
	
	// Flip all tangent dirs
	for(INT i=0; i<SelectedSplineActors.Num(); i++)
	{
		SelectedSplineActors(i)->SplineActorTangent = -1.f * SelectedSplineActors(i)->SplineActorTangent;
	}
	
	// Then for all connections, break the current one and reverse
	for(INT i=0; i<CurrentConnections.Num(); i++)
	{
		CurrentConnections(i).FromSpline->BreakConnectionTo(CurrentConnections(i).ToSpline);
		CurrentConnections(i).ToSpline->AddConnectionTo(CurrentConnections(i).FromSpline);
	}	
}

/** Util to test a route from one selected spline node to another */
void UEditorEngine::SplineTestRoute()
{
	ASplineActor* SplineA = NULL;
	ASplineActor* SplineB = NULL;
	GetFirstTwoSelectedSplineActors(SplineA, SplineB);
	
	if(SplineA && SplineB)
	{
		TArray<ASplineActor*> Route;
		UBOOL bSuccess = SplineA->FindSplinePathTo(SplineB, Route);
		if(bSuccess)
		{
			// Worked, so draw route!
			GWorld->PersistentLineBatcher->BatchedLines.Empty();
			
			for(INT SplineIdx=1; SplineIdx<Route.Num(); SplineIdx++)
			{
				check( Route(SplineIdx-1) );
				check( Route(SplineIdx) );
				GWorld->PersistentLineBatcher->DrawLine( Route(SplineIdx)->Location, Route(SplineIdx-1)->Location, FColor(10,200,30), SDPG_World );
			}
		}
		else
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("SplineTestRouteFailed"));
		}
	}
}

/** Set tangents on the selected two points to be straight and even. */
void UEditorEngine::SplineStraightTangents()
{
	ASplineActor* SplineStart = NULL;
	ASplineActor* SplineEnd = NULL;
	GetFirstTwoSelectedSplineActors(SplineStart, SplineEnd);
	
	if(SplineStart && SplineEnd)
	{
		// If wrong way around, swap
		if(SplineEnd->IsConnectedTo(SplineStart,FALSE))
		{
			Swap(SplineStart, SplineEnd);
		}
		// If not connected at all, do nothing and bail
		else if(!SplineStart->IsConnectedTo(SplineEnd,FALSE))
		{
			appMsgf(AMT_OK, TEXT("Spline actors are not connected."));
			return;
		}
		
		const FScopedTransaction Transaction( *LocalizeUnrealEd("SplineStraightTangents") );
		
		// At this point SplineStart connects to SplineEnd
		
		// Direct vector between them becomes the tangent for each end
		FVector Delta = SplineEnd->Location - SplineStart->Location;
		
		// Transform into local space and assign to each actor
		
		SplineStart->Modify();
		SplineStart->SplineActorTangent = FRotationMatrix(SplineStart->Rotation).InverseTransformNormal( Delta );		
		SplineStart->UpdateConnectedSplineComponents(TRUE);
		
		SplineEnd->Modify();
		SplineEnd->SplineActorTangent = FRotationMatrix(SplineEnd->Rotation).InverseTransformNormal( Delta );		
		SplineEnd->UpdateConnectedSplineComponents(TRUE);
	}
}

/** Select all nodes on the same splines as selected set */
void UEditorEngine::SplineSelectAllNodes()
{
	// Array of all spline actors connected to selected set
	TArray<ASplineActor*> AllConnectedActors;
	
	// Iterate over all selected actors
	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		ASplineActor* Spline = Cast<ASplineActor>( *It );
		if(Spline)
		{
			TArray<ASplineActor*> Connected;
			Spline->GetAllConnectedSplineActors(Connected);
			
			for(INT i=0; i<Connected.Num(); i++)
			{
				if(Connected(i))
				{
					AllConnectedActors.AddUniqueItem(Connected(i));				
				}
			}			
		}
	}
	
	// Now change selection to what we found
	
	SelectNone( FALSE, TRUE );

	for(INT ActorIdx=0; ActorIdx<AllConnectedActors.Num(); ActorIdx++)
	{
		SelectActor( AllConnectedActors(ActorIdx), TRUE, NULL, FALSE );
	}
	
	NoteSelectionChange();
}
