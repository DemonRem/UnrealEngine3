/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "UnrealEd.h"
#include "MouseDeltaTracker.h"
#include "TwoDeeShapeEditor.h"
#include "Properties.h"

static INT GDefaultDetailLevel = 5;

enum ESegType {
	ST_LINEAR	= 0,
	ST_BEZIER	= 1
};

struct H2DSEVectorProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(H2DSEVectorProxy,HHitProxy);

	F2DSEVector* Vector;

	H2DSEVectorProxy( F2DSEVector* InVector ):
	HHitProxy(HPP_UI),
		Vector(InVector)
	{}

	virtual EMouseCursor GetMouseCursor()
	{
		return MC_Cross;
	}
};

struct H2DSEControlPointProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(H2DSEControlPointProxy,HHitProxy);

	F2DSEVector* ControlPoint;

	H2DSEControlPointProxy( F2DSEVector* InControlPoint ):
	HHitProxy(HPP_UI),
		ControlPoint(InControlPoint)
	{}

	virtual EMouseCursor GetMouseCursor()
	{
		return MC_Cross;
	}
};

struct H2DSEShapeHandleProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(H2DSEShapeHandleProxy,HHitProxy);

	F2DSEVector* ShapeHandle;

	H2DSEShapeHandleProxy( F2DSEVector* InVector ):
	HHitProxy(HPP_UI),
		ShapeHandle(InVector)
	{}

	virtual EMouseCursor GetMouseCursor()
	{
		return MC_Cross;
	}
};

struct H2DSEOriginProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(H2DSEOriginProxy,HHitProxy);

	F2DSEVector* Vector;

	H2DSEOriginProxy( F2DSEVector* InVector ):
	HHitProxy(HPP_UI),
		Vector(InVector)
	{}

	virtual EMouseCursor GetMouseCursor()
	{
		return MC_Cross;
	}
};


struct H2DSESegmentProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(H2DSESegmentProxy,HHitProxy);

	FSegment* Segment;

	H2DSESegmentProxy( FSegment* InSegment ):
	HHitProxy(HPP_UI),
		Segment(InSegment)
	{}

	virtual EMouseCursor GetMouseCursor()
	{
		return MC_Cross;
	}
};

/*-----------------------------------------------------------------------------
	F2DSEVector
-----------------------------------------------------------------------------*/

F2DSEVector::F2DSEVector()
{
	X = Y = Z = 0;
	bSelected = 0;
}

F2DSEVector::F2DSEVector( FLOAT x, FLOAT y, FLOAT z)
	: FVector( x, y, z )
{
	bSelected = 0;
}

F2DSEVector::~F2DSEVector()
{
}

void F2DSEVector::SelectToggle()
{
	bSelected = !bSelected;
}

void F2DSEVector::Select( UBOOL bSel )
{
	bSelected = bSel;
}

UBOOL F2DSEVector::IsSelected( void )
{
	return bSelected;
}

/*-----------------------------------------------------------------------------
	FSegment
-----------------------------------------------------------------------------*/

FSegment::FSegment()
{
	SegType = ST_LINEAR;
	DetailLevel = 10;
	bSelected = 0;
}

FSegment::FSegment( F2DSEVector vtx1, F2DSEVector vtx2 )
{
	Vertex[0] = vtx1;
	Vertex[1] = vtx2;
	SegType = ST_LINEAR;
	DetailLevel = 10;
	bSelected = 0;
}

FSegment::FSegment( FVector vtx1, FVector vtx2 )
{
	Vertex[0] = vtx1;
	Vertex[1] = vtx2;
	SegType = ST_LINEAR;
	DetailLevel = 10;
	bSelected = 0;
}

FSegment::~FSegment()
{
}

FVector FSegment::GetHalfwayPoint()
{
	return Vertex[0] + ((Vertex[1] - Vertex[0]) / 2.0);
}

void FSegment::GenerateControlPoint()
{
	FVector Dir = Vertex[1] - Vertex[0];
	INT Size = Dir.Size();
	Dir.Normalize();

	ControlPoint[0] = Vertex[0] + (Dir * (Size / 3.0f));
	ControlPoint[1] = Vertex[1] - (Dir * (Size / 3.0f));

}

void FSegment::SetSegType( INT InType )
{
	if( InType == SegType ) return;
	SegType = InType;
	if( InType == ST_BEZIER )
	{
		GenerateControlPoint();
		DetailLevel = GDefaultDetailLevel;
	}
}

UBOOL FSegment::IsSelected()
{
	return bSelected;
}

void FSegment::GetBezierPoints(TArray<FVector>& BezierPoints)
{
	FVector Points[4];
	Points[0] = Vertex[0];
	Points[1] = ControlPoint[0];
	Points[2] = ControlPoint[1];
	Points[3] = Vertex[1];

	EvaluateBezier( Points, DetailLevel+1, BezierPoints );
}

/*-----------------------------------------------------------------------------
	FShape
-----------------------------------------------------------------------------*/

FShape::FShape()
{
}

FShape::~FShape()
{
}

void FShape::ComputeHandlePosition()
{
	Handle = F2DSEVector(0,0,0);
	for( INT seg = 0 ; seg < Segments.Num() ; ++seg )
		Handle += Segments(seg).Vertex[0];
	Handle /= Segments.Num();
}

// Breaks the shape down into convex polys.

void FShape::Breakdown( F2DSEVector InOrigin, FPolyBreaker* InBreaker )
{
	Verts.Empty();
	for( INT seg = 1 ; seg < Segments.Num() ; ++seg )
		Segments(seg).bUsed = 0;

	// Use up the first segment before going into the loop.
	if( Segments(0).SegType == ST_BEZIER )
	{
		TArray<FVector> BezierPoints;
		Segments(0).GetBezierPoints( BezierPoints );
		for( INT bz = BezierPoints.Num() - 1 ; bz > 0 ; --bz )
		{
			Verts(Verts.Add()) = BezierPoints(bz) - InOrigin;
		}
	}
	else
	{
		Verts(Verts.Add()) = Segments(0).Vertex[1] - InOrigin;
	}
	Segments(0).bUsed = 1;

	F2DSEVector Match = Segments(0).Vertex[0];

	for( INT seg = 0 ; seg < Segments.Num() ; ++seg )
	{
		if( !Segments(seg).bUsed
				&& Segments(seg).Vertex[1] == Match )
		{
			if( Segments(seg).SegType == ST_BEZIER )
			{
				TArray<FVector> BezierPoints;
				Segments(seg).GetBezierPoints( BezierPoints );
				for( INT bz = BezierPoints.Num() - 1 ; bz > 0 ; --bz )
				{
					Verts(Verts.Add()) = BezierPoints(bz) - InOrigin;
				}
			}
			else
			{
				Verts(Verts.Add()) = Segments(seg).Vertex[1] - InOrigin;
			}

			Segments(seg).bUsed = 1;
			Match = Segments(seg).Vertex[0];

			seg = 0;
		}
	}

	// Reverse the vertex winding -- this seems to make things work more reliably.
	TArray<FVector> Reverse;
	for( INT x = Verts.Num()-1 ; x > -1 ; --x )
		new(Reverse)FVector(Verts(x));

	InBreaker->Process( &Reverse, FVector(-1,0,0) );

}

//
//	UTwoDeeShapeEditorComponent
//

class UTwoDeeShapeEditorComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UTwoDeeShapeEditorComponent,UPrimitiveComponent,0,UnrealEd);

	class WxTwoDeeShapeEditor*	TwoDeeShapeEditor;
};

IMPLEMENT_CLASS(UTwoDeeShapeEditorComponent);

struct FTwoDeeShapeEditorViewportClient: public FEditorLevelViewportClient, FPreviewScene
{
	class WxTwoDeeShapeEditor*		TwoDeeShapeEditor;
	UTwoDeeShapeEditorComponent*	TwoDeeShapeComponent;
	UEditorComponent*				EditorComponent;

	// Used for when the viewport is rotating around a locked position

	UBOOL bLock;
	FRotator LockRot;
	FVector LockLocation;

	// Constructor.

	FTwoDeeShapeEditorViewportClient( class WxTwoDeeShapeEditor* InTwoDeeShapeEditor );

	// FEditorLevelViewportClient interface.

	virtual FSceneInterface* GetScene() { return FPreviewScene::GetScene(); }
	virtual void ProcessClick(FSceneView* View,HHitProxy* HitProxy,FName Key,EInputEvent Event,UINT HitX,UINT HitY);
	virtual void Draw(FViewport* Viewport,FCanvas* Canvas);
	virtual UBOOL InputKey(FViewport* Viewport, INT ControllerId, FName Key, EInputEvent Event,FLOAT AmountDepressed,UBOOL bGamepad=FALSE);
	virtual UBOOL InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime);

	void DrawSegments( const FSceneView* View,FPrimitiveDrawInterface* PDI );
	void DrawVertices( const FSceneView* View,FPrimitiveDrawInterface* PDI );

	virtual void Serialize(FArchive& Ar) { Ar << Input << (FPreviewScene&)*this; }
};

//
//	FTwoDeeShapeEditorViewportClient::FTwoDeeShapeEditorViewportClient
//

FTwoDeeShapeEditorViewportClient::FTwoDeeShapeEditorViewportClient( WxTwoDeeShapeEditor* InTwoDeeShapeEditor )
	: TwoDeeShapeEditor(InTwoDeeShapeEditor)
{
	// No postprocess 
	ShowFlags			&= ~SHOW_PostProcess;

	// Create EditorComponent for drawing grid, axes etc.
	EditorComponent = ConstructObject<UEditorComponent>(UEditorComponent::StaticClass());
	EditorComponent->bDrawPivot = false;
	EditorComponent->bDrawWorldBox = false;
	EditorComponent->bDrawKillZ = false;
	EditorComponent->PerspectiveGridSize = HALF_WORLD_MAX1;
	FPreviewScene::AddComponent(EditorComponent,FMatrix::Identity);

	TwoDeeShapeComponent = ConstructObject<UTwoDeeShapeEditorComponent>(UTwoDeeShapeEditorComponent::StaticClass());
	TwoDeeShapeComponent->TwoDeeShapeEditor = InTwoDeeShapeEditor;
	FPreviewScene::AddComponent(TwoDeeShapeComponent,FMatrix::Identity);
}

void FTwoDeeShapeEditorViewportClient::ProcessClick(FSceneView* View,HHitProxy* HitProxy,FName Key,EInputEvent Event,UINT HitX,UINT HitY)
{
	FViewportClick	Click(View,this,Key,Event,HitX,HitY);

	if(HitProxy )
	{
		if( HitProxy->IsA(H2DSESegmentProxy::StaticGetType()))
		{
			if( Event == IE_DoubleClick )
			{
				TwoDeeShapeEditor->SelectNone();
				((H2DSESegmentProxy*)HitProxy)->Segment->bSelected = 1;
				TwoDeeShapeEditor->SplitEdges();
			}
			else
			{
				if( Click.IsControlDown() )
				{
					((H2DSESegmentProxy*)HitProxy)->Segment->bSelected = !((H2DSESegmentProxy*)HitProxy)->Segment->bSelected;
				}
				else
				{
					TwoDeeShapeEditor->SelectNone();
					((H2DSESegmentProxy*)HitProxy)->Segment->bSelected = 1;
				}
			}
		}
		else if( HitProxy->IsA(H2DSEOriginProxy::StaticGetType()))
		{
			if( Click.IsControlDown() )
			{
				((H2DSEOriginProxy*)HitProxy)->Vector->bSelected = !((H2DSEOriginProxy*)HitProxy)->Vector->bSelected;
			}
			else
			{
				TwoDeeShapeEditor->SelectNone();
				((H2DSEOriginProxy*)HitProxy)->Vector->bSelected = 1;
			}
		}
		else if( HitProxy->IsA(H2DSEShapeHandleProxy::StaticGetType()))
		{
			if( Click.IsControlDown() )
			{
				((H2DSEShapeHandleProxy*)HitProxy)->ShapeHandle->bSelected = !((H2DSEShapeHandleProxy*)HitProxy)->ShapeHandle->bSelected;
			}
			else
			{
				TwoDeeShapeEditor->SelectNone();
				((H2DSEShapeHandleProxy*)HitProxy)->ShapeHandle->bSelected = 1;
			}
		}
		else if( HitProxy->IsA(H2DSEVectorProxy::StaticGetType()))
		{
			if( Key == KEY_RightMouseButton )
			{
				FVector Old = FVector( 0, ((H2DSEVectorProxy*)HitProxy)->Vector->Y, ((H2DSEVectorProxy*)HitProxy)->Vector->Z );
				FVector New = Old.GridSnap( GUnrealEd->Constraints.GetGridSize() );
				FVector Diff = New - Old;
				TwoDeeShapeEditor->ApplyDelta( Diff );
			}
			else
			{
				if( Click.IsControlDown() )
				{
					((H2DSEVectorProxy*)HitProxy)->Vector->bSelected = !((H2DSEVectorProxy*)HitProxy)->Vector->bSelected;
				}
				else
				{
					TwoDeeShapeEditor->SelectNone();
					((H2DSEVectorProxy*)HitProxy)->Vector->bSelected = 1;
				}
			}
		}
		else if( HitProxy->IsA(H2DSEControlPointProxy::StaticGetType()))
		{
			if( Key == KEY_RightMouseButton )
			{
				FVector Old = FVector( 0, ((H2DSEVectorProxy*)HitProxy)->Vector->Y, ((H2DSEVectorProxy*)HitProxy)->Vector->Z );
				FVector New = Old.GridSnap( GUnrealEd->Constraints.GetGridSize() );
				FVector Diff = New - Old;
				TwoDeeShapeEditor->ApplyDelta( Diff );
			}
			else
			{
				if( Click.IsControlDown() )
				{
					((H2DSEControlPointProxy*)HitProxy)->ControlPoint->bSelected = !((H2DSEControlPointProxy*)HitProxy)->ControlPoint->bSelected;
				}
				else
				{
					TwoDeeShapeEditor->SelectNone();
					((H2DSEControlPointProxy*)HitProxy)->ControlPoint->bSelected = 1;
				}
			}
		}
	}
	else
	{
		if( !Click.IsControlDown() )
		{
			TwoDeeShapeEditor->SelectNone();
		}
	}
}

//
//	FTwoDeeShapeEditorViewportClient::Draw
//

void FTwoDeeShapeEditorViewportClient::Draw(FViewport* Viewport,FCanvas* Canvas)
{
	FEditorLevelViewportClient::Draw(Viewport,Canvas);
}

void FTwoDeeShapeEditorViewportClient::DrawSegments( const FSceneView* View,FPrimitiveDrawInterface* PDI )
{
	for( INT shape = 0 ; shape < TwoDeeShapeEditor->Shapes.Num() ; ++shape )
	{
		for( INT seg = 0 ; seg < TwoDeeShapeEditor->Shapes(shape).Segments.Num() ; ++seg )
		{
			FSegment* segment = &TwoDeeShapeEditor->Shapes(shape).Segments(seg);
			FColor clr(255,255,255);

			if( segment->IsSelected() )
			{
				clr = FColor(255,0,0);
			}

			switch( segment->SegType )
			{
				case ST_LINEAR:
					PDI->SetHitProxy( new H2DSESegmentProxy(segment) );
					PDI->DrawLine( segment->Vertex[0], segment->Vertex[1], clr, SDPG_Foreground );
					PDI->SetHitProxy( NULL );
					break;

				case ST_BEZIER:

					// Generate list of vertices for bezier curve and render them as a line.
					TArray<FVector> BezierPoints;
					segment->GetBezierPoints( BezierPoints );

					PDI->SetHitProxy( new H2DSESegmentProxy(segment) );
					for( INT bz = 0 ; bz < BezierPoints.Num() - 1 ; ++bz )
					{
						PDI->DrawLine( BezierPoints(bz), BezierPoints(bz+1), clr, SDPG_Foreground );
					}
					PDI->SetHitProxy( NULL );

					PDI->SetHitProxy( new H2DSEControlPointProxy(&segment->ControlPoint[0]) );
					PDI->DrawLine( segment->Vertex[0], segment->ControlPoint[0], segment->ControlPoint[0].IsSelected() ? FColor(255,0,0) : FColor(0,255,0), SDPG_Foreground );
					PDI->SetHitProxy( NULL );

					PDI->SetHitProxy( new H2DSEControlPointProxy(&segment->ControlPoint[1]) );
					PDI->DrawLine( segment->Vertex[1], segment->ControlPoint[1], segment->ControlPoint[1].IsSelected() ? FColor(255,0,0) : FColor(0,255,0), SDPG_Foreground );
					PDI->SetHitProxy( NULL );

					break;
			}
		}
	}
}

void FTwoDeeShapeEditorViewportClient::DrawVertices( const FSceneView* View,FPrimitiveDrawInterface* PDI )
{
	FVector Mid(0,0,0);
#if GEMINI_TODO
	FLOAT Scale = Context.View->Project( Mid ).W * ( 4 / (FLOAT)Context.SizeX / Context.View->ProjectionMatrix.M[0][0] );
	float Sz = 4.f * Scale;
	FColor clr;

	for( INT shape = 0 ; shape < TwoDeeShapeEditor->Shapes.Num() ; ++shape )
	{
		clr = FColor(255,128,0);
		if( TwoDeeShapeEditor->Shapes(shape).Handle.bSelected )
		{
			clr = FColor(255,0,0);
		}
		PDI->SetHitProxy( new H2DSEShapeHandleProxy(&TwoDeeShapeEditor->Shapes(shape).Handle) );
		PDI->DrawSprite( TwoDeeShapeEditor->Shapes(shape).Handle, Sz, Sz, GWorld->GetWorldInfo()->BSPVertex, clr, SDPG_Foreground );
		PDI->SetHitProxy( NULL );

		for( INT seg = 0 ; seg < TwoDeeShapeEditor->Shapes(shape).Segments.Num() ; ++seg )
		{
			FSegment* segment = &TwoDeeShapeEditor->Shapes(shape).Segments(seg);

			clr = FColor(255,255,255);
			if( segment->Vertex[0].bSelected )
			{
				clr = FColor(255,0,0);
			}
			PDI->SetHitProxy( new H2DSEVectorProxy(&segment->Vertex[0]) );
			PDI->DrawSprite( segment->Vertex[0], Sz, Sz, GWorld->GetWorldInfo()->BSPVertex, clr, SDPG_Foreground );
			PDI->SetHitProxy( NULL );

			clr = FColor(255,255,255);
			if( segment->Vertex[1].bSelected )
			{
				clr = FColor(255,0,0);
			}
			PDI->SetHitProxy( new H2DSEVectorProxy(&segment->Vertex[1]) );
			PDI->DrawSprite( segment->Vertex[1], Sz, Sz, GWorld->GetWorldInfo()->BSPVertex, clr, SDPG_Foreground );
			PDI->SetHitProxy( NULL );

			if( segment->SegType == ST_BEZIER )
			{
				clr = FColor(0,255,0);
				if( segment->ControlPoint[0].bSelected )
				{
					clr = FColor(255,0,0);
				}
				PDI->SetHitProxy( new H2DSEControlPointProxy(&segment->ControlPoint[0]) );
				PDI->DrawSprite( segment->ControlPoint[0], Sz, Sz, GWorld->GetWorldInfo()->BSPVertex, clr, SDPG_Foreground );
				PDI->SetHitProxy( NULL );

				clr = FColor(0,255,0);
				if( segment->ControlPoint[1].bSelected )
				{
					clr = FColor(255,0,0);
				}
				PDI->SetHitProxy( new H2DSEControlPointProxy(&segment->ControlPoint[1]) );
				PDI->DrawSprite( segment->ControlPoint[1], Sz, Sz, GWorld->GetWorldInfo()->BSPVertex, clr, SDPG_Foreground );
				PDI->SetHitProxy( NULL );
			}
		}
	}

	clr = FColor(0,0,255);
	if( TwoDeeShapeEditor->Origin.bSelected )
	{
		clr = FColor(255,0,0);
	}
	PDI->SetHitProxy( new H2DSEOriginProxy(&TwoDeeShapeEditor->Origin) );
	PDI->DrawSprite( TwoDeeShapeEditor->Origin, Sz, Sz, GWorld->GetWorldInfo()->BSPVertex, clr, SDPG_Foreground );
	PDI->SetHitProxy( NULL );
#endif
}

UBOOL FTwoDeeShapeEditorViewportClient::InputKey(FViewport* Viewport, INT ControllerId, FName Key, EInputEvent Event,FLOAT AmountDepressed,UBOOL Gamepad)
{
	UBOOL bCtrlDown = Input->IsCtrlPressed();
	UBOOL bShiftDown = Input->IsShiftPressed();
	UBOOL bAltDown = Input->IsAltPressed();

	if( Event == IE_Pressed )
	{
		if( Key == KEY_I && bCtrlDown )
		{
			TwoDeeShapeEditor->SplitEdges();
			return TRUE;
		}
		if( Key == KEY_Delete )
		{
			TwoDeeShapeEditor->Delete();
			return TRUE;
		}
		if( Key == KEY_B )
		{
			wxCommandEvent DummyEvent;
			TwoDeeShapeEditor->OnEditBezier( DummyEvent );
			return TRUE;
		}
		if( Key == KEY_L )
		{
			wxCommandEvent DummyEvent;
			TwoDeeShapeEditor->OnEditLinear( DummyEvent );
			return TRUE;
		}
	}

	return FEditorLevelViewportClient::InputKey( Viewport, ControllerId, Key, Event, AmountDepressed );
}

UBOOL FTwoDeeShapeEditorViewportClient::InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime)
{
	// Get some useful info about buttons being held down
	UBOOL bCtrlDown = Input->IsCtrlPressed();
	UBOOL bShiftDown = Input->IsShiftPressed();
	UBOOL bAltDown = Input->IsAltPressed();

	MouseDeltaTracker->AddDelta( this, Key, Delta, 0 );
	const FVector DragDelta = MouseDeltaTracker->GetDeltaSnapped();

	GEditor->MouseMovement += DragDelta;

	if( !DragDelta.IsZero() )
	{
		// Convert the movement delta into drag/rotation deltas
		FVector Drag;
		FRotator Rot;
		FVector Scale;
		MouseDeltaTracker->ConvertMovementDeltaToDragRot( this, DragDelta, Drag, Rot, Scale );

		if( TwoDeeShapeEditor->HasSelection() && bCtrlDown && !bShiftDown && !bAltDown )
		{
				TwoDeeShapeEditor->ApplyDelta( Drag );
		}
		else
		{
			MoveViewportCamera( Drag, Rot );
		}
		MouseDeltaTracker->ReduceBy( DragDelta );
	}

	Viewport->Invalidate();

	return TRUE;
}

/*-----------------------------------------------------------------------------
	wxTwoDeeShapEditorPrimitives
-----------------------------------------------------------------------------*/

/**
* Window that sits on the right side of the splitter, containing the options for creating
* brush primitives from the 2D shapes.
*/

class wxTwoDeeShapEditorPrimitives : public wxWindow
{
public:
	DECLARE_DYNAMIC_CLASS(wxTwoDeeShapEditorPrimitives);

	wxTwoDeeShapEditorPrimitives() {}
	wxTwoDeeShapEditorPrimitives( wxWindow* parent, wxWindowID id );
	virtual ~wxTwoDeeShapEditorPrimitives();

	void ResetPrimitiveTypeCombo();

	wxComboBox* PrimitiveTypeCombo;
	wxButton* BuildButton;
	WxPropertyWindow* PropertyWindow;

	INT PrimType;

	WxTwoDeeShapeEditor* TwoDeeShapeEditor;

	void OnSize( wxSizeEvent& In );
	void OnSelChange( wxCommandEvent& In );
	void OnBuild( wxCommandEvent& In );

	DECLARE_EVENT_TABLE();
};

IMPLEMENT_DYNAMIC_CLASS(wxTwoDeeShapEditorPrimitives,wxWindow);

BEGIN_EVENT_TABLE( wxTwoDeeShapEditorPrimitives, wxWindow )
	EVT_SIZE( wxTwoDeeShapEditorPrimitives::OnSize )
	EVT_COMBOBOX(ID_2DSE_PrimitiveType, wxTwoDeeShapEditorPrimitives::OnSelChange)
	EVT_BUTTON( IDPB_Build, wxTwoDeeShapEditorPrimitives::OnBuild )
END_EVENT_TABLE()

wxTwoDeeShapEditorPrimitives::wxTwoDeeShapEditorPrimitives( wxWindow* Parent, wxWindowID id )
	: wxWindow( Parent, id )
{
	BuildButton = new wxButton( this, IDPB_Build, *LocalizeUnrealEd("Build") );
	PropertyWindow = new WxPropertyWindow;
	PropertyWindow->Create( this, NULL );

	ResetPrimitiveTypeCombo();
}

void wxTwoDeeShapEditorPrimitives::ResetPrimitiveTypeCombo()
{
	// No need to delete, wx will do it for us.
	PrimitiveTypeCombo = new WxComboBox( this, ID_2DSE_PrimitiveType, TEXT(""), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );

	PrimitiveTypeCombo->Append( *LocalizeUnrealEd("Extrude"), *GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPEREXTRUDE ) );
	PrimitiveTypeCombo->Append( *LocalizeUnrealEd("ExtrudeToBevel"), *GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPEREXTRUDETOBEVEL ) );
	PrimitiveTypeCombo->Append( *LocalizeUnrealEd("ExtrudeToPoint"), *GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPEREXTRUDETOPOINT ) );
	PrimitiveTypeCombo->Append( *LocalizeUnrealEd("Revolve"), *GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPERREVOLVE ) );
	PrimitiveTypeCombo->Append( *LocalizeUnrealEd("Sheet"), *GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPERSHEET ) );

	PrimType = 0;
	PrimitiveTypeCombo->SetSelection( PrimType );

	PropertyWindow->SetObject( *GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPEREXTRUDE ), 1, 1, 0 );
}

wxTwoDeeShapEditorPrimitives::~wxTwoDeeShapEditorPrimitives()
{
	PropertyWindow->SetObject( NULL, 0,0,0 );
	delete PropertyWindow;
}

void wxTwoDeeShapEditorPrimitives::OnSize( wxSizeEvent& In )
{
	wxRect rc = GetClientRect();

	PrimitiveTypeCombo->SetSize( rc.GetLeft(),rc.GetTop(),rc.GetWidth()-70-4,24 );
	BuildButton->SetSize( rc.GetRight()-70,rc.GetTop(),70,24 );
	PropertyWindow->SetSize( rc.GetLeft(),rc.GetTop()+24+4,rc.GetWidth(),rc.GetHeight()-24-4-4 );
}

void wxTwoDeeShapEditorPrimitives::OnSelChange( wxCommandEvent& In )
{
	UObject* obj = (UObject*)PrimitiveTypeCombo->GetClientData( PrimitiveTypeCombo->GetSelection() );
	PropertyWindow->SetObject( obj, 1, 1, 0 );
}

void wxTwoDeeShapEditorPrimitives::OnBuild( wxCommandEvent& In )
{
	INT sel = PrimitiveTypeCombo->GetSelection();

	switch( sel )
	{
		case 0:
		{
			UOptions2DShaperExtrude* Proxy = (UOptions2DShaperExtrude*)*GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPEREXTRUDE );
			TwoDeeShapeEditor->ProcessExtrude( Proxy->Depth );
			TwoDeeShapeEditor->RotateBuilderBrush( Proxy->Axis );
		}
		break;

		case 1:
		{
			UOptions2DShaperExtrudeToBevel* Proxy = (UOptions2DShaperExtrudeToBevel*)*GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPEREXTRUDETOBEVEL );
			TwoDeeShapeEditor->ProcessExtrudeToBevel( Proxy->Height, Proxy->CapHeight );
			TwoDeeShapeEditor->RotateBuilderBrush( Proxy->Axis );
		}
		break;

		case 2:
		{
			UOptions2DShaperExtrudeToPoint* Proxy = (UOptions2DShaperExtrudeToPoint*)*GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPEREXTRUDETOPOINT );
			TwoDeeShapeEditor->ProcessExtrudeToPoint( Proxy->Depth );
			TwoDeeShapeEditor->RotateBuilderBrush( Proxy->Axis );
		}
		break;

		case 3:
		{
			UOptions2DShaperRevolve* Proxy = (UOptions2DShaperRevolve*)*GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPERREVOLVE );
			TwoDeeShapeEditor->ProcessRevolve( Proxy->SidesPer360, Proxy->Sides );
			TwoDeeShapeEditor->RotateBuilderBrush( Proxy->Axis );
		}
		break;

		case 4:
		{
			UOptions2DShaperSheet* Proxy = (UOptions2DShaperSheet*)*GApp->EditorFrame->FindOptionProxy( OPTIONS_2DSHAPERSHEET );
			TwoDeeShapeEditor->ProcessSheet();
			TwoDeeShapeEditor->RotateBuilderBrush( Proxy->Axis );
		}
		break;

	}
}

/*-----------------------------------------------------------------------------
	WxTwoDeeShapeEditorBar.
-----------------------------------------------------------------------------*/

class WxTwoDeeShapeEditorBar : public wxToolBar
{
public:
	WxTwoDeeShapeEditorBar( wxWindow* InParent, wxWindowID InID )
		: wxToolBar( InParent, InID, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxNO_BORDER | wxTB_3DBUTTONS )
	{
		OpenB.Load( TEXT("Open") );
		SaveB.Load( TEXT("Save") );

		AddTool( IDM_OPEN, TEXT(""), OpenB, *LocalizeUnrealEd("ToolTip_74") );
		AddTool( IDM_SAVE, TEXT(""), SaveB, *LocalizeUnrealEd("ToolTip_75") );

		Realize();
	}

private:
	WxMaskedBitmap OpenB, SaveB;
};

/*-----------------------------------------------------------------------------
	WxTwoDeeShapeEditorMenu.
-----------------------------------------------------------------------------*/

class WxTwoDeeShapeEditorMenu : public wxMenuBar
{
public:
	WxTwoDeeShapeEditorMenu()
	{
		wxMenu* FileMenu = new wxMenu();
		wxMenu* EditMenu = new wxMenu();
		wxMenu* ShapeMenu = new wxMenu();

		// File menu
		{
			FileMenu->Append( IDM_NEW, *LocalizeUnrealEd("New"), *LocalizeUnrealEd("ToolTip_154") );
			FileMenu->Append( IDM_OPEN, *LocalizeUnrealEd("OpenE"), *LocalizeUnrealEd("ToolTip_155") );
			FileMenu->AppendSeparator();
			FileMenu->Append( IDM_SAVE, *LocalizeUnrealEd("Save"), *LocalizeUnrealEd("ToolTip_156") );
			FileMenu->Append( IDM_SAVE_AS, *LocalizeUnrealEd("SaveAsE"), *LocalizeUnrealEd("ToolTip_157") );
		}

		// Edit menu
		{
			EditMenu->Append( IDMN_2DSE_Delete, *LocalizeUnrealEd("Delete"), TEXT("") );
			EditMenu->AppendSeparator();
			EditMenu->Append( IDMN_2DSE_SplitEdges, *LocalizeUnrealEd("SplitEdges"), TEXT("") );
			EditMenu->AppendSeparator();
			EditMenu->Append( IDMN_2DSE_Bezier, *LocalizeUnrealEd("Bezier"), TEXT("") );
			EditMenu->Append( IDMN_2DSE_Linear, *LocalizeUnrealEd("Linear"), TEXT("") );
		}

		// Shape menu
		{
			ShapeMenu->Append( IDMN_2DSE_NewShape, *LocalizeUnrealEd("New"), TEXT("") );
		}

		Append( FileMenu, *LocalizeUnrealEd("File") );
		Append( EditMenu, *LocalizeUnrealEd("Edit") );
		Append( ShapeMenu, *LocalizeUnrealEd("Shape") );
	}
};

/*-----------------------------------------------------------------------------
	WxTwoDeeShapeEditor
-----------------------------------------------------------------------------*/

IMPLEMENT_DYNAMIC_CLASS(WxTwoDeeShapeEditor,wxFrame);

BEGIN_EVENT_TABLE( WxTwoDeeShapeEditor, wxFrame )
	EVT_SIZE( WxTwoDeeShapeEditor::OnSize )
	EVT_PAINT( WxTwoDeeShapeEditor::OnPaint )
	EVT_MENU( IDMN_2DSE_Delete, WxTwoDeeShapeEditor::OnEditDelete )
	EVT_MENU( IDMN_2DSE_SplitEdges, WxTwoDeeShapeEditor::OnEditSplitEdges )
	EVT_MENU( IDMN_2DSE_Bezier, WxTwoDeeShapeEditor::OnEditBezier )
	EVT_MENU( IDMN_2DSE_Linear, WxTwoDeeShapeEditor::OnEditLinear )
	EVT_MENU( IDMN_2DSE_NewShape, WxTwoDeeShapeEditor::OnShapeNew )
	EVT_MENU( IDM_NEW, WxTwoDeeShapeEditor::OnFileNew )
	EVT_MENU( IDM_OPEN, WxTwoDeeShapeEditor::OnFileOpen )
	EVT_MENU( IDM_SAVE, WxTwoDeeShapeEditor::OnFileSave )
	EVT_MENU( IDM_SAVE_AS, WxTwoDeeShapeEditor::OnFileSaveAs )
END_EVENT_TABLE()

WxTwoDeeShapeEditor::WxTwoDeeShapeEditor()
{}

WxTwoDeeShapeEditor::WxTwoDeeShapeEditor( wxWindow* Parent, wxWindowID id )
	: wxFrame( Parent, id, *LocalizeUnrealEd("2DShapeEditor"), wxDefaultPosition, wxDefaultSize, wxFRAME_FLOAT_ON_PARENT | wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR )
{
	SplitterWnd = new wxSplitterWindow( this, ID_SPLITTERWINDOW, wxDefaultPosition, wxSize(100, 100), wxSP_3D|wxSP_3DBORDER|wxSP_FULLSASH );

	// Primitive window
	PrimitiveWindow = NULL;
	ResetPrimitiveWindow();

	// Create viewport.

	ViewportHolder = new WxViewportHolder( SplitterWnd, -1, 0 );
	ViewportClient = new FTwoDeeShapeEditorViewportClient( this );
	ViewportClient->Viewport = GEngine->Client->CreateWindowChildViewport( ViewportClient, (HWND)ViewportHolder->GetHandle() );
	ViewportClient->Viewport->CaptureJoystickInput(false);
	ViewportHolder->SetViewport( ViewportClient->Viewport );
	ViewportClient->ViewportType = LVT_OrthoYZ;
	ViewportClient->ShowFlags = (SHOW_DefaultEditor & ~SHOW_ViewMode_Mask) | SHOW_ViewMode_Wireframe;
	ViewportHolder->Show();

	FWindowUtil::LoadPosSize( TEXT("TwoDeeShapeEditor"), this, 64,64,800,800 );

	wxRect rc = GetClientRect();

	SplitterWnd->SplitVertically( ViewportHolder, PrimitiveWindow, rc.GetWidth() - 350 );

	ToolBar = new WxTwoDeeShapeEditorBar( this, -1 );
	SetToolBar( ToolBar );

	MenuBar = new WxTwoDeeShapeEditorMenu();
	SetMenuBar( MenuBar );

	New();
}

WxTwoDeeShapeEditor::~WxTwoDeeShapeEditor()
{
	FWindowUtil::SavePosSize( TEXT("TwoDeeShapeEditor"), this );

	GEngine->Client->CloseViewport( ViewportClient->Viewport );
	ViewportClient->Viewport = NULL;
	delete ViewportClient;
}

/**
 * Must be called on each map change to update the shape editors GWorld-dependent objects.
 */
void WxTwoDeeShapeEditor::ResetPrimitiveWindow()
{
	if ( !PrimitiveWindow )
	{
		PrimitiveWindow = new wxTwoDeeShapEditorPrimitives( SplitterWnd, -1 );
		PrimitiveWindow->TwoDeeShapeEditor = this;
	}
	else
	{
		PrimitiveWindow->ResetPrimitiveTypeCombo();
	}
}

void WxTwoDeeShapeEditor::OnSize( wxSizeEvent& In )
{
	wxPoint origin = GetClientAreaOrigin();
	wxRect rc = GetClientRect();
	rc.y -= origin.y;

	SplitterWnd->SetSize( rc );
}

void WxTwoDeeShapeEditor::Serialize(FArchive& Ar)
{
	check(ViewportClient);
	ViewportClient->Serialize(Ar);
}

void WxTwoDeeShapeEditor::OnPaint( wxPaintEvent& In )
{
	wxPaintDC dc( this );

	ViewportClient->Viewport->Invalidate();
}

/**
 * Clears out the shapes list and resets the editor to the start up state.
 */
void WxTwoDeeShapeEditor::New()
{
	Shapes.Empty();
	InsertNewShape();

	Filename = TEXT("");

	ViewportClient->Viewport->Invalidate();
}

/**
 * Inserts a new shape into the world.
 */
void WxTwoDeeShapeEditor::InsertNewShape()
{
	//F2DSEVector A(-128,-128,0), B(-128,128,0), C(128,128,0), D(128,-128,0);
	F2DSEVector A(0,-128,-128), B(0,128,-128), C(0,128,128), D(0,-128,128);

	new(Shapes)FShape();
	FShape* shape = &Shapes(Shapes.Num()-1);

	new(shape->Segments)FSegment(A, B);
	new(shape->Segments)FSegment(B, C);
	new(shape->Segments)FSegment(C, D);
	new(shape->Segments)FSegment(D, A);

	ComputeHandlePositions();
}

/**
 * Computes handle positions for selected shapes.
 *
 * @param	InAlwaysCompute		If 1, all handle positions are computed, selected or not
 */
void WxTwoDeeShapeEditor::ComputeHandlePositions( UBOOL InAlwaysCompute )
{
	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		if( !Shapes(shape).Handle.IsSelected() || InAlwaysCompute )
		{
			Shapes(shape).ComputeHandlePosition();
		}
	}

	ViewportClient->Viewport->Invalidate();
}

/**
 * Removes selection status from everything.
 */
void WxTwoDeeShapeEditor::SelectNone()
{
	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		Shapes(shape).Handle.Select(0);
		for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
		{
			Shapes(shape).Segments(seg).bSelected = 0;
			Shapes(shape).Segments(seg).Vertex[0].Select(0);
			Shapes(shape).Segments(seg).Vertex[1].Select(0);
			Shapes(shape).Segments(seg).ControlPoint[0].Select(0);
			Shapes(shape).Segments(seg).ControlPoint[1].Select(0);
		}
	}
	
	Origin.Select(0);
}

/**
 * Checks to see if anything is selected.
 *
 * @return	1 if there are selections.
 */
UBOOL WxTwoDeeShapeEditor::HasSelection()
{
	if( Origin.IsSelected() )
	{
		return 1;
	}

	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		if( Shapes(shape).Handle.IsSelected() )
		{
			return 1;
		}

		for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
		{
			if( Shapes(shape).Segments(seg).bSelected
				|| Shapes(shape).Segments(seg).Vertex[0].IsSelected()
				|| Shapes(shape).Segments(seg).Vertex[1].IsSelected()
				|| Shapes(shape).Segments(seg).ControlPoint[0].IsSelected()
				|| Shapes(shape).Segments(seg).ControlPoint[1].IsSelected() )
			{
				return 1;
			}
		}
	}

	return 0;
}

/**
 * Applies a movement vector to all selected items.
 *
 * @param	InDelta		The movement to apply
 */
void WxTwoDeeShapeEditor::ApplyDelta( FVector InDelta )
{
	InDelta = -InDelta;
	if( Origin.IsSelected() )
	{
		Origin -= InDelta;
	}

	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		if( Shapes(shape).Handle.IsSelected() )
		{
			Shapes(shape).Handle -= InDelta;

			for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
			{
				FSegment* segment = &Shapes(shape).Segments(seg);

				ApplyDeltaToVertex( &Shapes(shape), segment->Vertex[0], InDelta );
			}
		}
		else
		{
			for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
			{
				FSegment* segment = &Shapes(shape).Segments(seg);

				if( segment->bSelected )
				{
					ApplyDeltaToVertex( &Shapes(shape), segment->Vertex[0], InDelta );
					ApplyDeltaToVertex( &Shapes(shape), segment->Vertex[1], InDelta );
					segment->ControlPoint[0] -= InDelta;
					segment->ControlPoint[1] -= InDelta;
				}
				else
				{
					if( segment->Vertex[0].IsSelected() )
					{
						ApplyDeltaToVertex( &Shapes(shape), segment->Vertex[0], InDelta );
					}

					if( segment->Vertex[1].IsSelected() )
					{
						ApplyDeltaToVertex( &Shapes(shape), segment->Vertex[1], InDelta );
					}

					if( segment->ControlPoint[0].IsSelected() )
					{
						segment->ControlPoint[0] -= InDelta;
					}

					if( segment->ControlPoint[1].IsSelected() )
					{
						segment->ControlPoint[1] -= InDelta;
					}
				}
			}
		}
	}

	ComputeHandlePositions();
}

void WxTwoDeeShapeEditor::ApplyDeltaToVertex( FShape* InShape, F2DSEVector InVertex, FVector InDelta )
{
	for( INT seg = 0 ; seg < InShape->Segments.Num() ; ++seg )
	{
		FSegment* segment = &InShape->Segments(seg);

		if( segment->Vertex[0] == InVertex )
		{
			segment->Vertex[0] -= InDelta;
			segment->ControlPoint[0] -= InDelta;
		}
		if( segment->Vertex[1] == InVertex )
		{
			segment->Vertex[1] -= InDelta;
			segment->ControlPoint[1] -= InDelta;
		}
		if( segment->ControlPoint[0] == InVertex )
		{
			segment->ControlPoint[0] -= InDelta;
		}
		if( segment->ControlPoint[1] == InVertex )
		{
			segment->ControlPoint[1] -= InDelta;
		}
	}
}

/**
 * Splits all selected sides in half.
 */
void WxTwoDeeShapeEditor::SplitEdges()
{
	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
		{
			if( Shapes(shape).Segments(seg).IsSelected() )
			{
				// Create a new segment half the size of this one, starting from the middle and extending
				// to the second vertex.

				FVector HalfWay = Shapes(shape).Segments(seg).GetHalfwayPoint();
				new(Shapes(shape).Segments)FSegment( HalfWay, Shapes(shape).Segments(seg).Vertex[1] );

				// Move the original segments ending point to the halfway point.

				Shapes(shape).Segments(seg).Vertex[1] = HalfWay;
			}
		}
	}

	ComputeHandlePositions();
	ViewportClient->Viewport->Invalidate();
}

void WxTwoDeeShapeEditor::DeleteSegment( FShape* InShape, INT InSegIdx )
{
	F2DSEVector v0 = InShape->Segments(InSegIdx).Vertex[0];
	F2DSEVector v1 = InShape->Segments(InSegIdx).Vertex[1];

	InShape->Segments.Remove(InSegIdx);

	for( INT s = 0 ; s < InShape->Segments.Num() ; ++s )
	{
		if( InShape->Segments(s).Vertex[1] == v0 )
		{
			InShape->Segments(s).Vertex[1] = v1;
		}
	}
}

void WxTwoDeeShapeEditor::Delete()
{
	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		if( Shapes(shape).Handle.IsSelected() )
		{
			Shapes.Remove(shape);
			shape = -1;
		}
		else
		{
			if( Shapes(shape).Segments.Num() > 3 )
			{
				for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
				{
					if( Shapes(shape).Segments(seg).IsSelected() )
					{
						DeleteSegment( &Shapes(shape), seg );
					}
					else
					{
						if( Shapes(shape).Segments(seg).Vertex[0].IsSelected() )
						{
							DeleteSegment( &Shapes(shape), seg );
						}
						else if( Shapes(shape).Segments(seg).Vertex[1].IsSelected() )
						{
							DeleteSegment( &Shapes(shape), seg );
						}
					}
				}
			}
		}
	}
			
	ComputeHandlePositions();
	ViewportClient->Viewport->Invalidate();
}

void WxTwoDeeShapeEditor::OnEditDelete( wxCommandEvent& In )
{
	Delete();
}

void WxTwoDeeShapeEditor::OnEditSplitEdges( wxCommandEvent& In )
{
	SplitEdges();
}

void WxTwoDeeShapeEditor::OnEditBezier( wxCommandEvent& In )
{
	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
		{
			if( Shapes(shape).Segments(seg).IsSelected() )
			{
				Shapes(shape).Segments(seg).SetSegType( ST_BEZIER );
			}
		}
	}

	ViewportClient->Viewport->Invalidate();
}

void WxTwoDeeShapeEditor::OnEditLinear( wxCommandEvent& In )
{
	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
		{
			if( Shapes(shape).Segments(seg).IsSelected() )
			{
				Shapes(shape).Segments(seg).SetSegType( ST_LINEAR );
			}
		}
	}

	ViewportClient->Viewport->Invalidate();
}

void WxTwoDeeShapeEditor::OnShapeNew( wxCommandEvent& In )
{
	InsertNewShape();

	ViewportClient->Viewport->Invalidate();
}

void WxTwoDeeShapeEditor::RotateBuilderBrush( INT Axis )
{
	// Rotate the builder brush to match the axis requested.
	FRotator Rot;
	switch(Axis)
	{
		case 0:		Rot = FRotator(0,16384,16384);	break;
		case 1:		Rot = FRotator(0,0,16384);		break;
		case 2:		Rot = FRotator(0,0,0);			break;
	};
	FMatrix RotMatrix = FRotationMatrix( Rot );

	ABrush* Brush = GWorld->GetBrush();
	for( INT poly = 0 ; poly < Brush->Brush->Polys->Element.Num() ; ++poly )
	{
		FPoly* Poly = &(Brush->Brush->Polys->Element(poly));

		for( INT vtx = 0 ; vtx < Poly->Vertices.Num() ; ++vtx )
		{
			Poly->Vertices(vtx) = Brush->PrePivot + RotMatrix.TransformNormal( Poly->Vertices(vtx) - Brush->PrePivot );
		}
		Poly->Base = Poly->Vertices(0);

		Poly->TextureU = FVector(0,0,0);
		Poly->TextureV = FVector(0,0,0);
	}

	Brush->ClearComponents();
	Brush->ConditionalUpdateComponents();

	GUnrealEd->RedrawLevelEditingViewports();
}

void WxTwoDeeShapeEditor::ProcessSheet()
{
	FString Cmd;
	Cmd += TEXT("BRUSH SET\n\n");

	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		FPolyBreaker breaker;
		Shapes(shape).Breakdown(Origin, &breaker);

		for( INT poly = 0 ; poly < breaker.FinalPolys.Num() ; ++poly )
		{
			Cmd += TEXT("Begin Polygon Flags=8\n");
			for( INT vtx = breaker.FinalPolys(poly).Vertices.Num()-1 ; vtx > -1 ; --vtx )
			//for( INT vtx = 0 ; vtx < breaker.FinalPolys(poly).NumVertices ; ++vtx )
			{
				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					breaker.FinalPolys(poly).Vertices(vtx).X,
					breaker.FinalPolys(poly).Vertices(vtx).Y,
					breaker.FinalPolys(poly).Vertices(vtx).Z );
			}
			Cmd += TEXT("End Polygon\n");
		}
	}

	GUnrealEd->Exec( *Cmd );
}

void WxTwoDeeShapeEditor::ProcessExtrude(INT Depth)
{
	FString Cmd;
	Cmd += TEXT("BRUSH SET\n\n");

	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		FPolyBreaker breaker;
		Shapes(shape).Breakdown(Origin, &breaker);

		for( INT poly = 0 ; poly < breaker.FinalPolys.Num() ; ++poly )
		{
			Cmd += TEXT("Begin Polygon Flags=0\n");
			for( INT vtx = breaker.FinalPolys(poly).Vertices.Num()-1 ; vtx > -1 ; --vtx )
			{
				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					Depth / 2.0f,
					breaker.FinalPolys(poly).Vertices(vtx).Y,
					breaker.FinalPolys(poly).Vertices(vtx).Z
					);
			}
			Cmd += TEXT("End Polygon\n");

			Cmd += TEXT("Begin Polygon Flags=0\n");
			for( INT vtx = 0 ; vtx < breaker.FinalPolys(poly).Vertices.Num() ; ++vtx )
			{
				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					-(Depth / 2.0f),
					breaker.FinalPolys(poly).Vertices(vtx).Y,
					breaker.FinalPolys(poly).Vertices(vtx).Z
					);
			}
			Cmd += TEXT("End Polygon\n");
		}

		// Sides ...
		//
		for( INT vtx = 0 ; vtx < Shapes(shape).Verts.Num() ; ++vtx )
		{
			Cmd += TEXT("Begin Polygon Flags=0\n");

			FVector* pvtxPrev = &Shapes(shape).Verts( (vtx ? vtx - 1 : Shapes(shape).Verts.Num() - 1 ) );
			FVector* pvtx = &Shapes(shape).Verts(vtx);

			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				Depth / 2.0f, pvtx->Y, pvtx->Z );

			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				-(Depth / 2.0f), pvtx->Y, pvtx->Z );

			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				-(Depth / 2.0f), pvtxPrev->Y, pvtxPrev->Z );

			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					Depth / 2.0f, pvtxPrev->Y, pvtxPrev->Z );

			Cmd += TEXT("End Polygon\n");
		}
	}

	GUnrealEd->Exec( *Cmd );
}

void WxTwoDeeShapeEditor::ProcessExtrudeToPoint(INT Depth)
{
	FString Cmd;
	Cmd += TEXT("BRUSH SET\n\n");

	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		FPolyBreaker breaker;
		Shapes(shape).Breakdown(Origin, &breaker);

		for( INT poly = 0 ; poly < breaker.FinalPolys.Num() ; ++poly )
		{
			Cmd += TEXT("Begin Polygon Flags=0\n");
			for( INT vtx = 0 ; vtx < breaker.FinalPolys(poly).Vertices.Num() ; ++vtx )
			//for( INT vtx = breaker.FinalPolys(poly).NumVertices-1 ; vtx > -1 ; --vtx )
			{
				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					0.0f,
					breaker.FinalPolys(poly).Vertices(vtx).Y,
					breaker.FinalPolys(poly).Vertices(vtx).Z );
			}
			Cmd += TEXT("End Polygon\n");
		}

		// Sides ...
		//
		for( INT vtx = 0 ; vtx < Shapes(shape).Verts.Num() ; ++vtx )
		{
			Cmd += TEXT("Begin Polygon Flags=0\n");

			FVector* pvtxPrev = &( Shapes(shape).Verts( (vtx ? vtx - 1 : Shapes(shape).Verts.Num() - 1 ) ) );
			FVector* pvtx = &( Shapes(shape).Verts(vtx) );

			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				pvtxPrev->X, pvtxPrev->Y, pvtxPrev->Z );
			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				(FLOAT)Depth, 0.0f, 0.0f );
			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				pvtx->X, pvtx->Y, pvtx->Z );

			Cmd += TEXT("End Polygon\n");
		}
	}

	GUnrealEd->Exec( *Cmd );
}

void WxTwoDeeShapeEditor::ProcessExtrudeToBevel(INT Depth, INT CapHeight)
{
	const FLOAT Dist = 1.0f - (CapHeight / (FLOAT)Depth);

	FString Cmd;
	Cmd += TEXT("BRUSH SET\n\n");

	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		FPolyBreaker breaker;
		Shapes(shape).Breakdown(Origin, &breaker);

		for( INT poly = 0 ; poly < breaker.FinalPolys.Num() ; ++poly )
		{
			Cmd += TEXT("Begin Polygon Flags=0\n");
			for( INT vtx = 0 ; vtx < breaker.FinalPolys(poly).Vertices.Num() ; ++vtx )
			{
				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					0.0f,
					breaker.FinalPolys(poly).Vertices(vtx).Y,
					breaker.FinalPolys(poly).Vertices(vtx).Z );
			}
			Cmd += TEXT("End Polygon\n");

			Cmd += TEXT("Begin Polygon Flags=0\n");
			for( INT vtx = breaker.FinalPolys(poly).Vertices.Num()-1 ; vtx > -1 ; --vtx )
			{
				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					(FLOAT)CapHeight,
					breaker.FinalPolys(poly).Vertices(vtx).Y * Dist,
					breaker.FinalPolys(poly).Vertices(vtx).Z * Dist );
			}
			Cmd += TEXT("End Polygon\n");
		}

		// Sides ...
		//
		for( INT vtx = 0 ; vtx < Shapes(shape).Verts.Num() ; ++vtx )
		{
			Cmd += TEXT("Begin Polygon Flags=0\n");

			FVector* pvtxPrev = &( Shapes(shape).Verts( (vtx ? vtx - 1 : Shapes(shape).Verts.Num() - 1 ) ) );
			FVector* pvtx = &( Shapes(shape).Verts(vtx) );

			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				pvtxPrev->X, pvtxPrev->Y, pvtxPrev->Z );
			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				(FLOAT)CapHeight, pvtxPrev->Y * Dist, pvtxPrev->Z * Dist );
			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				(FLOAT)CapHeight, pvtx->Y * Dist, pvtx->Z * Dist );
			Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
				pvtx->X, pvtx->Y, pvtx->Z );

			Cmd += TEXT("End Polygon\n");
		}
	}

	GUnrealEd->Exec( *Cmd );
}

void WxTwoDeeShapeEditor::ProcessRevolve( INT TotalSides, INT Sides )
{
	UBOOL bPositive, bNegative;

	// Make sure the origin is totally to the left or right of the shape.
	bPositive = bNegative = FALSE;
	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
		{
			for( INT vtx = 0 ; vtx < 2 ; ++vtx )
			{
				if( Origin.Y > Shapes(shape).Segments(seg).Vertex[vtx].Y )
				{
					bPositive = TRUE;
				}
				if( Origin.Y < Shapes(shape).Segments(seg).Vertex[vtx].Y )
				{
					bNegative = TRUE;
				}
			}
		}
	}

	if( bPositive && bNegative )
	{
		appMsgf( AMT_OK, TEXT("Origin must be completely to the left or right side of the shape.") );
		return;
	}

	// When revolving from the left side, we have to flip the polys around.
	const UBOOL bFromLeftSide = ( bNegative && !bPositive );

	FString Cmd;
	Cmd += TEXT("BRUSH SET\n\n");

	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		FPolyBreaker breaker;
		Shapes(shape).Breakdown(Origin, &breaker);

		if( Sides != TotalSides )	// Don't make end caps for a complete revolve
		{
			for( INT poly = 0 ; poly < breaker.FinalPolys.Num() ; ++poly )
			{
				if( bFromLeftSide )
				{
					Cmd += TEXT("Begin Polygon Flags=0\n");
					for( INT vtx = breaker.FinalPolys(poly).Vertices.Num()-1 ; vtx > -1 ; --vtx )
					{
						Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
							breaker.FinalPolys(poly).Vertices(vtx).X,
							breaker.FinalPolys(poly).Vertices(vtx).Y,
							breaker.FinalPolys(poly).Vertices(vtx).Z );
					}
					Cmd += TEXT("End Polygon\n");

					FRotator Rotation( 0, (65536.0f / TotalSides) * Sides, 0 );
					FRotationMatrix RotMatrix( Rotation );
					Cmd += TEXT("Begin Polygon Flags=0\n");
					for( INT vtx = 0 ; vtx < breaker.FinalPolys(poly).Vertices.Num() ; ++vtx )
					{
						FVector NewVtx = RotMatrix.TransformNormal( breaker.FinalPolys(poly).Vertices(vtx) );

						Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
							NewVtx.X, NewVtx.Y, NewVtx.Z );
					}
					Cmd += TEXT("End Polygon\n");
				}
				else
				{
					Cmd += TEXT("Begin Polygon Flags=0\n");
					for( INT vtx = 0 ; vtx < breaker.FinalPolys(poly).Vertices.Num() ; ++vtx )
					{
						Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
							breaker.FinalPolys(poly).Vertices(vtx).X,
							breaker.FinalPolys(poly).Vertices(vtx).Y,
							breaker.FinalPolys(poly).Vertices(vtx).Z );
					}
					Cmd += TEXT("End Polygon\n");

					FRotator Rotation( 0, (65536.0f / TotalSides) * Sides, 0 );
					FMatrix RotMatrix = FRotationMatrix( Rotation );

					Cmd += TEXT("Begin Polygon Flags=0\n");
					for( INT vtx = breaker.FinalPolys(poly).Vertices.Num()-1 ; vtx > -1 ; --vtx )
					{
						FVector NewVtx = RotMatrix.TransformNormal( breaker.FinalPolys(poly).Vertices(vtx) );

						Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
							NewVtx.X, NewVtx.Y, NewVtx.Z );
					}
					Cmd += TEXT("End Polygon\n");
				}
			}
		}

		// Sides ...
		//
		for( INT side = 0 ; side < Sides ; ++side )
		{
			const FRotator	StepRotation( 0, (65536.0f / TotalSides) * side, 0 );
			const FRotator	StepRotation2( 0, (65536.0f / TotalSides) * (side+1), 0 );
			const FRotationMatrix	RotMatrix1( StepRotation );
			const FRotationMatrix	RotMatrix2( StepRotation2 );

			for( INT vi = 0 ; vi < Shapes(shape).Verts.Num() ; ++vi )
			{
				Cmd += TEXT("Begin Polygon Flags=0\n");

				FVector *pvtx, *pvtxPrev;

				pvtxPrev = &( Shapes(shape).Verts( (vi ? vi - 1 : Shapes(shape).Verts.Num() - 1 ) ) );
				pvtx = &( Shapes(shape).Verts(vi) );

				if( bFromLeftSide )
				{
					Exchange( pvtxPrev, pvtx );
				}

				const FVector vtxPrev	= RotMatrix1.TransformNormal( *pvtxPrev );
				const FVector vtx		= RotMatrix1.TransformNormal( *pvtx );
				const FVector vtxPrev2	= RotMatrix2.TransformNormal( *pvtxPrev );
				const FVector vtx2		= RotMatrix2.TransformNormal( *pvtx );

				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					vtx.X, vtx.Y, vtx.Z );
				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					vtxPrev.X, vtxPrev.Y, vtxPrev.Z );
				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					vtxPrev2.X, vtxPrev2.Y, vtxPrev2.Z );
				Cmd += *FString::Printf(TEXT("Vertex   X=%1.1f, Y=%1.1f, Z=%1.1f\n"),
					vtx2.X, vtx2.Y, vtx2.Z );

				Cmd += TEXT("End Polygon\n");
			}
		}
	}

	GUnrealEd->Exec( *Cmd );
}

void WxTwoDeeShapeEditor::Flip( UBOOL InHoriz )
{
	// Flip the vertices across the origin.
	for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
	{
		for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
		{
			for( INT vertex = 0 ; vertex < 2 ; ++vertex )
			{
				FVector Dist = Shapes(shape).Segments(seg).Vertex[vertex] - Origin;
				FVector CPDist = Shapes(shape).Segments(seg).ControlPoint[vertex] - Origin;

				if( InHoriz )
				{
					Shapes(shape).Segments(seg).Vertex[vertex].X -= (Dist.X * 2);
					Shapes(shape).Segments(seg).ControlPoint[vertex].X -= (CPDist.X * 2);
				}
				else
				{
					Shapes(shape).Segments(seg).Vertex[vertex].Y -= (Dist.Y * 2);
					Shapes(shape).Segments(seg).ControlPoint[vertex].Y -= (CPDist.Y * 2);
				}
			}
			Exchange( Shapes(shape).Segments(seg).Vertex[0], Shapes(shape).Segments(seg).Vertex[1] );
			Exchange( Shapes(shape).Segments(seg).ControlPoint[0], Shapes(shape).Segments(seg).ControlPoint[1] );
		}
	}

	ComputeHandlePositions(1);
	GUnrealEd->RedrawLevelEditingViewports();
}

void WxTwoDeeShapeEditor::OnFileNew( wxCommandEvent& In )
{
	New();
	SetCaption();
}

void WxTwoDeeShapeEditor::OnFileOpen( wxCommandEvent& In )
{
	WxFileDialog dlg( GApp->EditorFrame, *LocalizeUnrealEd("Open2DShape"), TEXT(""), TEXT(""), TEXT("2D Shapes (*.2ds)|*.2ds|All Files|*.*"), wxOPEN | wxFILE_MUST_EXIST | wxHIDE_READONLY );
	if( dlg.ShowModal() == wxID_OK )
	{
		Filename = dlg.GetPath().c_str();
		Load( *Filename );
		SetCaption();

		GApp->LastDir[LD_2DS] = Filename.Left( Filename.InStr( TEXT("\\"), 1 ) );
	}
}

void WxTwoDeeShapeEditor::OnFileSave( wxCommandEvent& In )
{
	if( Filename.Len() > 0 )
	{
		Save( *Filename );
	}
	else
	{
		SaveAs();
	}
}

void WxTwoDeeShapeEditor::OnFileSaveAs( wxCommandEvent& In )
{
	SaveAs();
}

void WxTwoDeeShapeEditor::Load( const TCHAR* Filename )
{
	FArchive* Archive;
	Archive = GFileManager->CreateFileReader( Filename );

	if( Archive )
	{
		Shapes.Empty();

		// Origin

		Archive->Serialize( &Origin.X, sizeof(FLOAT) );
		Archive->Serialize( &Origin.Y, sizeof(FLOAT) );

		// Shapes

		INT NumShapes;
		Archive->Serialize( &NumShapes, sizeof(INT) );

		for( INT shape = 0 ; shape < NumShapes ; ++shape )
		{
			new(Shapes)FShape();

			INT NumSegments;
			Archive->Serialize( &NumSegments, sizeof(INT) );

			for( INT seg = 0 ; seg < NumSegments ; ++seg )
			{
				new(Shapes(shape).Segments)FSegment;
				FSegment* pSeg = &(Shapes(shape).Segments(seg));
				Archive->Serialize( &(pSeg->Vertex[0].X), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->Vertex[0].Y), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->Vertex[0].Z), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->Vertex[1].X), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->Vertex[1].Y), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->Vertex[1].Z), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->ControlPoint[0].X), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->ControlPoint[0].Y), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->ControlPoint[0].Z), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->ControlPoint[1].X), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->ControlPoint[1].Y), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->ControlPoint[1].Z), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->SegType), sizeof(FLOAT) );
				Archive->Serialize( &(pSeg->DetailLevel), sizeof(FLOAT) );
			}
			ComputeHandlePositions();
		}

		Archive->Close();
	}

	GUnrealEd->RedrawLevelEditingViewports();
}

void WxTwoDeeShapeEditor::Save( const TCHAR* Filename )
{
	FArchive* Archive;
	Archive = GFileManager->CreateFileWriter( Filename );

	if( Archive )
	{
		// Origin

		*Archive << Origin.X << Origin.Y;

		// Shapes

		INT Num = Shapes.Num();
		*Archive << Num;

		for( INT shape = 0 ; shape < Shapes.Num() ; ++shape )
		{
			Num = Shapes(shape).Segments.Num();
			*Archive << Num;
			for( INT seg = 0 ; seg < Shapes(shape).Segments.Num() ; ++seg )
			{
				FSegment* pSeg = &(Shapes(shape).Segments(seg));
				*Archive 
					<< pSeg->Vertex[0].X
					<< pSeg->Vertex[0].Y
					<< pSeg->Vertex[0].Z
					<< pSeg->Vertex[1].X
					<< pSeg->Vertex[1].Y
					<< pSeg->Vertex[1].Z
					<< pSeg->ControlPoint[0].X
					<< pSeg->ControlPoint[0].Y
					<< pSeg->ControlPoint[0].Z
					<< pSeg->ControlPoint[1].X
					<< pSeg->ControlPoint[1].Y
					<< pSeg->ControlPoint[1].Z
					<< pSeg->SegType
					<< pSeg->DetailLevel;
			}
		}

		Archive->Close();
	}
}

void WxTwoDeeShapeEditor::SaveAs()
{
	WxFileDialog dlg( GApp->EditorFrame, *LocalizeUnrealEd("SaveAs"), *(GApp->LastDir[LD_2DS]), *Filename.GetCleanFilename(), TEXT("2D Shapes (*.2ds)|*.2ds|All Files|*.*"), wxSAVE | wxHIDE_READONLY | wxOVERWRITE_PROMPT );
	if( dlg.ShowModal() == wxID_OK )
	{
		Filename = dlg.GetPath().c_str();
		Save( *Filename );
	}

	SetCaption();
}

void WxTwoDeeShapeEditor::SetCaption()
{
	SetTitle( *FString::Printf( *LocalizeUnrealEd("2DShapeEditorCaption_F"), *Filename ) );
}
