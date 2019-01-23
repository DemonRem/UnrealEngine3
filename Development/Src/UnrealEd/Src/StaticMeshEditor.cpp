/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "Factories.h"
#include "MouseDeltaTracker.h"
#include "EnginePhysicsClasses.h"
#include "ConvexDecompTool.h"
#include "StaticMeshEditor.h"
#include "..\..\Launch\Resources\resource.h"
#include "MeshUtils.h"
#include "Properties.h"
#include "DlgGenericComboEntry.h"
#include "DlgGenerateUVOptions.h"

IMPLEMENT_CLASS(UStaticMeshEditorComponent);

/**
 * Versioning Info for the Docking Parent layout file.
 */
namespace
{
	static const TCHAR* DockingParent_Name = TEXT("StaticMeshEditor");
	static const INT DockingParent_Version = 0;		//Needs to be incremented every time a new dock window is added or removed from this docking parent.
}


static const FLOAT	LightRotSpeed = 40.0f;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxStaticMeshEditMenu
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class WxStaticMeshEditMenu : public wxMenuBar
{
public:
	WxStaticMeshEditMenu()
	{
		// File Menu
		wxMenu* FileMenu = new wxMenu();
		FileMenu->Append( IDM_SME_IMPORTMESHLOD, *LocalizeUnrealEd("ImportMeshLOD"), TEXT("") );
		FileMenu->Append( IDM_SME_REMOVELOD, *LocalizeUnrealEd("RemoveLOD"), TEXT("") );
		FileMenu->Append( IDM_SME_GENERATELOD, *LocalizeUnrealEd("GenerateLOD"), TEXT("") );
		FileMenu->Append( IDM_SME_GENERATEUVS, *LocalizeUnrealEd("GenerateUVs"), TEXT("") );

		// View menu
		wxMenu* ViewMenu = new wxMenu();
		ViewMenu->AppendCheckItem( ID_SHOW_OPEN_EDGES, *LocalizeUnrealEd("OpenEdges"), TEXT("") );
		ViewMenu->AppendCheckItem( ID_SHOW_UV_OVERLAY, *LocalizeUnrealEd("UVOverlay"), TEXT("") );
		ViewMenu->AppendCheckItem( ID_SHOW_WIREFRAME, *LocalizeUnrealEd("Wireframe"), TEXT("") );
		ViewMenu->AppendCheckItem( ID_SHOW_BOUNDS, *LocalizeUnrealEd("Bounds"), TEXT("") );
		ViewMenu->AppendCheckItem( IDMN_COLLISION, *LocalizeUnrealEd("Collision"), TEXT("") );
		ViewMenu->AppendSeparator();
		ViewMenu->AppendCheckItem( ID_LOCK_CAMERA, *LocalizeUnrealEd("LockCamera"), TEXT("") );

		// Tool menu
		wxMenu* ToolMenu = new wxMenu();
		ToolMenu->Append( ID_SAVE_THUMBNAIL, *LocalizeUnrealEd("SaveThumbnailAngle"), TEXT("") );

		// Collision menu
		wxMenu* CollisionMenu = new wxMenu();
		CollisionMenu->Append( IDMN_SME_COLLISION_6DOP, *LocalizeUnrealEd("6DOP"), TEXT("") );
		CollisionMenu->Append( IDMN_SME_COLLISION_10DOPX, *LocalizeUnrealEd("10DOPX"), TEXT("") );
		CollisionMenu->Append( IDMN_SME_COLLISION_10DOPY, *LocalizeUnrealEd("10DOPY"), TEXT("") );
		CollisionMenu->Append( IDMN_SME_COLLISION_10DOPZ, *LocalizeUnrealEd("10DOPZ"), TEXT("") );
		CollisionMenu->Append( IDMN_SME_COLLISION_18DOP, *LocalizeUnrealEd("18DOP"), TEXT("") );
		CollisionMenu->Append( IDMN_SME_COLLISION_26DOP, *LocalizeUnrealEd("26DOP"), TEXT("") );
		CollisionMenu->Append( IDMN_SME_COLLISION_SPHERE, *LocalizeUnrealEd("SphereSimplifiedCollision"), TEXT("") );
		CollisionMenu->Append( IDMN_SME_COLLISION_DECOMP, *LocalizeUnrealEd("AutoConvexCollision"), TEXT("") );
	    CollisionMenu->Append( IDMN_SME_COLLISION_REMOVE, *LocalizeUnrealEd("RemoveCollision"), TEXT("") );

		Append( FileMenu, *LocalizeUnrealEd("MeshMenu") );
		Append( ViewMenu, *LocalizeUnrealEd("View") );
		Append( ToolMenu, *LocalizeUnrealEd("Tool") );
		Append( CollisionMenu, *LocalizeUnrealEd("Collision") );
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxStaticMeshEditorBar
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class WxStaticMeshEditorBar : public wxToolBar
{
public:
	WxStaticMeshEditorBar( wxWindow* InParent, wxWindowID InID )
		: wxToolBar( InParent, InID, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxTB_FLAT | wxTB_NODIVIDER )
	{
		// Bitmaps

		OpenEdgesB.Load( TEXT("OpenEdges") );
		UVOverlayB.Load( TEXT("UVOverlay") );
		WireframeB.Load( TEXT("Wireframe") );
		BoundsB.Load( TEXT("Bounds") );
		CollisionB.Load( TEXT("Collision") );
		LockB.Load( TEXT("Lock") );
		CameraB.Load( TEXT("Camera") );

		LODAutoB.Load(TEXT("AnimTree_LOD_Auto"));
		LODBaseB.Load(TEXT("AnimTree_LOD_Base"));
		LOD1B.Load(TEXT("AnimTree_LOD_1"));
		LOD2B.Load(TEXT("AnimTree_LOD_2"));
		LOD3B.Load(TEXT("AnimTree_LOD_3"));

		// Set up the ToolBar

		AddCheckTool( ID_SHOW_OPEN_EDGES, TEXT(""), OpenEdgesB, OpenEdgesB, *LocalizeUnrealEd("ToolTip_68") );
		AddCheckTool( ID_SHOW_UV_OVERLAY, TEXT(""), UVOverlayB, UVOverlayB, *LocalizeUnrealEd("ToolTip_UVOverlay") );
		AddCheckTool( ID_SHOW_WIREFRAME, TEXT(""), WireframeB, WireframeB, *LocalizeUnrealEd("ToolTip_69") );
		AddCheckTool( ID_SHOW_BOUNDS, TEXT(""), BoundsB, BoundsB, *LocalizeUnrealEd("ToolTip_70") );
		AddCheckTool( IDMN_COLLISION, TEXT(""), CollisionB, CollisionB, *LocalizeUnrealEd("ToolTip_71") );
		AddSeparator();
		AddCheckTool( ID_LOCK_CAMERA, TEXT(""), LockB, LockB, *LocalizeUnrealEd("ToolTip_72") );
		AddSeparator();
		AddTool( ID_SAVE_THUMBNAIL, TEXT(""), CameraB, *LocalizeUnrealEd("ToolTip_73") );


		AddSeparator();
		wxStaticText* LODLabel = new wxStaticText(this, -1 ,*LocalizeUnrealEd("LODLabel") );
		AddControl(LODLabel);
		AddRadioTool(IDM_SME_LOD_AUTO, *LocalizeUnrealEd("SetLODAuto"), LODAutoB, wxNullBitmap, *LocalizeUnrealEd("SetLODAuto") );
		AddRadioTool(IDM_SME_LOD_BASE, *LocalizeUnrealEd("ForceLODBaseMesh"), LODBaseB, wxNullBitmap, *LocalizeUnrealEd("ForceLODBaseMesh") );
		AddRadioTool(IDM_SME_LOD_1, *LocalizeUnrealEd("ForceLOD1"), LOD1B, wxNullBitmap, *LocalizeUnrealEd("ForceLOD1") );
		AddRadioTool(IDM_SME_LOD_2, *LocalizeUnrealEd("ForceLOD2"), LOD2B, wxNullBitmap, *LocalizeUnrealEd("ForceLOD2") );
		AddRadioTool(IDM_SME_LOD_3, *LocalizeUnrealEd("ForceLOD3"), LOD3B, wxNullBitmap, *LocalizeUnrealEd("ForceLOD3") );
		ToggleTool(IDM_SME_LOD_AUTO, true);

		Realize();
	}

private:
	WxMaskedBitmap OpenEdgesB, UVOverlayB, WireframeB, BoundsB, CollisionB, LockB, CameraB;
	WxMaskedBitmap LODAutoB, LODBaseB, LOD1B, LOD2B, LOD3B;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	FStaticMeshEditorViewportClient
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FStaticMeshEditorViewportClient: public FEditorLevelViewportClient, private FPreviewScene
{
	class WxStaticMeshEditor*	StaticMeshEditor;
	UStaticMeshEditorComponent*	StaticMeshComponent;
	UEditorComponent*			EditorComponent;

	// Used for when the viewport is rotating around a locked position

	UBOOL bLock;
	FRotator LockRot;
	FVector LockLocation;

	// Constructor.

	FStaticMeshEditorViewportClient( class WxStaticMeshEditor* InStaticMeshEditor );

	// FEditorLevelViewportClient interface.
	virtual FSceneInterface* GetScene() { return FPreviewScene::GetScene(); }
	virtual FLinearColor GetBackgroundColor() { return FColor(64,64,64); }

	virtual void Draw(FViewport* Viewport,FCanvas* Canvas);
	virtual void DrawUVs(FViewport* Viewport,FCanvas* Canvas);
	virtual UBOOL InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed = 1.f,UBOOL bGamepad=FALSE);
	virtual UBOOL InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime, UBOOL bGamepad=FALSE);

	virtual void Serialize(FArchive& Ar) { Ar << Input << (FPreviewScene&)*this; }
};


FStaticMeshEditorViewportClient::FStaticMeshEditorViewportClient( WxStaticMeshEditor* InStaticMeshEditor )
	:	StaticMeshEditor(InStaticMeshEditor)
	,	bLock( TRUE )
{
	EditorComponent = ConstructObject<UEditorComponent>(UEditorComponent::StaticClass());
	EditorComponent->bDrawPivot = FALSE;
	EditorComponent->bDrawGrid = FALSE;
	FPreviewScene::AddComponent(EditorComponent,FMatrix::Identity);

	StaticMeshComponent = ConstructObject<UStaticMeshEditorComponent>(UStaticMeshEditorComponent::StaticClass());
	StaticMeshComponent->StaticMeshEditor = InStaticMeshEditor;
	StaticMeshComponent->StaticMesh = InStaticMeshEditor->StaticMesh;
	StaticMeshComponent->bUsePrecomputedShadows = FALSE;
	FPreviewScene::AddComponent(StaticMeshComponent,FMatrix::Identity);

	ViewLocation = -FVector(0,StaticMeshEditor->StaticMesh->Bounds.SphereRadius / (75.0f * (FLOAT)PI / 360.0f),0);
	ViewRotation = FRotator(0,16384,0);

	LockLocation = FVector(0,StaticMeshEditor->StaticMesh->ThumbnailDistance,0);
	LockRot = StaticMeshEditor->StaticMesh->ThumbnailAngle;

	const FMatrix LockMatrix = FRotationMatrix( FRotator(0,LockRot.Yaw,0) ) * FRotationMatrix( FRotator(0,0,LockRot.Pitch) ) * FTranslationMatrix( LockLocation );
	StaticMeshComponent->ConditionalUpdateTransform(FTranslationMatrix(-StaticMeshEditor->StaticMesh->Bounds.Origin) * LockMatrix);

	bLock = TRUE;
	bDrawAxes = FALSE;
	NearPlane = 1.0f;
	bAllowMayaCam = FALSE;//TRUE;
	SetViewLocationForOrbiting( FVector(0.f,0.f,0.f) );
}

void FStaticMeshEditorViewportClient::Draw(FViewport* Viewport,FCanvas* Canvas)
{
	FEditorLevelViewportClient::Draw(Viewport,Canvas);

	DrawShadowedString(Canvas,
		6,
		6,
		*FString::Printf(*LocalizeUnrealEd("Triangles_F"),StaticMeshEditor->NumTriangles),
		GEngine->SmallFont,
		FLinearColor::White
		);

	DrawShadowedString(Canvas,
		6,
		24,
		*FString::Printf(*LocalizeUnrealEd("UVChannels_F"),StaticMeshEditor->NumUVChannels),
		GEngine->SmallFont,
		FLinearColor::White
		);

	// Show the number of collision primitives if we are drawing collision.
	if((ShowFlags & SHOW_Collision) && StaticMeshEditor->StaticMesh->BodySetup)
	{
		DrawShadowedString(Canvas,
			6,
			42,
			*FString::Printf(*LocalizeUnrealEd("NumPrimitives_F"),StaticMeshEditor->StaticMesh->BodySetup->AggGeom.GetElementCount()),
			GEngine->SmallFont,
			FLinearColor::White
			);
	}

	FStaticMeshEditorViewportClient* vp = (FStaticMeshEditorViewportClient*)Viewport->GetClient();
	if( vp->bLock )
	{
		FRotator DrawRotation( vp->LockRot.Pitch, -(vp->LockRot.Yaw - 16384), vp->LockRot.Roll );
		vp->DrawAxes( Viewport, Canvas, &DrawRotation );
	}

	if (StaticMeshEditor->DrawUVOverlay)
	{
		DrawUVs(Viewport, Canvas);
	}
}

void FStaticMeshEditorViewportClient::DrawUVs(FViewport* Viewport,FCanvas* Canvas)
{
	//use the overriden LOD level
	UINT LODLevel = Clamp(StaticMeshComponent->ForcedLodModel - 1, 0, StaticMeshEditor->StaticMesh->LODModels.Num() - 1);

	//draw a string showing what UV channel and LOD is being displayed
	DrawShadowedString(Canvas,
		6,
		60,
		*FString::Printf(*LocalizeUnrealEd("UVOverlay_F"),StaticMeshEditor->StaticMesh->LightMapCoordinateIndex,LODLevel),
		GEngine->SmallFont,
		FLinearColor::White
		);

	FStaticMeshRenderData* RenderData = &StaticMeshEditor->StaticMesh->LODModels(LODLevel);
	if (RenderData && UINT(StaticMeshEditor->StaticMesh->LightMapCoordinateIndex) < RenderData->VertexBuffer.GetNumTexCoords())
	{
		//calculate scaling
		const UINT BorderWidth = 5;
		const UINT MinY = 96 + BorderWidth;
		const UINT MinX = BorderWidth;
		const FVector2D Origin(MinX, MinY);
		const UINT SizeOfSquare = Min(Viewport->GetSizeX() - MinX, Viewport->GetSizeY() - MinY) - BorderWidth;

		//draw texture border
		DrawLine2D(Canvas, Origin, FVector2D(MinX + SizeOfSquare, MinY), FLinearColor::White);
		DrawLine2D(Canvas, FVector2D(MinX + SizeOfSquare, MinY), FVector2D(MinX + SizeOfSquare, MinY + SizeOfSquare), FLinearColor::White);
		DrawLine2D(Canvas, FVector2D(MinX + SizeOfSquare, MinY + SizeOfSquare), FVector2D(MinX, MinY + SizeOfSquare), FLinearColor::White);
		DrawLine2D(Canvas, FVector2D(MinX, MinY + SizeOfSquare), Origin, FLinearColor::White);   

		//draw triangles
		WORD* Indices = (WORD*)RenderData->IndexBuffer.Indices.GetData();
		UINT NumIndices = RenderData->IndexBuffer.Indices.Num();
		for (UINT i = 0; i < NumIndices - 2; i += 3)
		{
			FVector2D UV1(RenderData->VertexBuffer.VertexUV(Indices[i], StaticMeshEditor->StaticMesh->LightMapCoordinateIndex)); 
			FVector2D UV2(RenderData->VertexBuffer.VertexUV(Indices[i + 1], StaticMeshEditor->StaticMesh->LightMapCoordinateIndex)); 
			FVector2D UV3(RenderData->VertexBuffer.VertexUV(Indices[i + 2], StaticMeshEditor->StaticMesh->LightMapCoordinateIndex)); 

			UV1 = (UV1 - FVector2D(floorf(UV1.X), floorf(UV1.Y))) * SizeOfSquare + Origin;
			UV2 = (UV2 - FVector2D(floorf(UV2.X), floorf(UV2.Y))) * SizeOfSquare + Origin;
			UV3 = (UV3 - FVector2D(floorf(UV3.X), floorf(UV3.Y))) * SizeOfSquare + Origin;

			DrawLine2D(Canvas, UV1, UV2, FLinearColor::Black);
			DrawLine2D(Canvas, UV2, UV3, FLinearColor::Black);
			DrawLine2D(Canvas, UV3, UV1, FLinearColor::Black);
		}
	}
}

UBOOL FStaticMeshEditorViewportClient::InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed,UBOOL Gamepad)
{
	// Capture the mouse input if one of the mouse buttons are held down.
	Viewport->CaptureMouseInput( Viewport->KeyState(KEY_LeftMouseButton) || Viewport->KeyState(KEY_MiddleMouseButton) || Viewport->KeyState(KEY_RightMouseButton) );

	if( Event == IE_Pressed) 
	{
		if( Key == KEY_LeftMouseButton || Key == KEY_RightMouseButton )
		{
			const INT HitX = Viewport->GetMouseX();
			const INT HitY = Viewport->GetMouseY();
			MouseDeltaTracker->StartTracking( this, HitX, HitY );
		}
	}
	else if( Event == IE_Released )
	{
		if( Key == KEY_LeftMouseButton || Key == KEY_RightMouseButton )
		{
			MouseDeltaTracker->EndTracking( this );
		}
	}

	// Handle viewport screenshot.
	InputTakeScreenshot( Viewport, Key, Event );

	return TRUE;
	//return FEditorLevelViewportClient::InputKey( Viewport, ControllerId, Key, Event, AmountDepressed );
}

UBOOL FStaticMeshEditorViewportClient::InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime, UBOOL bGamepad)
{
	// See if we are holding down the 'rotate light' key
	const UBOOL bLightMoveDown = Viewport->KeyState( KEY_L );

	// If so, use mouse movement to rotate light direction,
	if( bLightMoveDown )
	{
		// Look at which axis is being dragged and by how much
		const FLOAT DragX = (Key == KEY_MouseX) ? Delta : 0.f;
		const FLOAT DragY = (Key == KEY_MouseY) ? Delta : 0.f;

		FRotator LightDir = GetLightDirection();

		LightDir.Yaw += -DragX * LightRotSpeed;
		LightDir.Pitch += -DragY * LightRotSpeed;

		SetLightDirection( LightDir );
	}
	// If we are not moving light, use the MouseDeltaTracker to update camera.
	else
	{
		if( (Key == KEY_MouseX || Key == KEY_MouseY) && Delta != 0.0f )
		{
			const UBOOL LeftMouseButton = Viewport->KeyState(KEY_LeftMouseButton);
			const UBOOL MiddleMouseButton = Viewport->KeyState(KEY_MiddleMouseButton);
			const UBOOL RightMouseButton = Viewport->KeyState(KEY_RightMouseButton);

			MouseDeltaTracker->AddDelta( this, Key, Delta, 0 );
			const FVector DragDelta = MouseDeltaTracker->GetDelta();

			if( !DragDelta.IsZero() )
			{
				GEditor->MouseMovement += DragDelta;

				if ( bAllowMayaCam && GEditor->bUseMayaCameraControls ) 
				{
					if ( bLock )
					{
						//StaticMeshEditor->LockCamera( FALSE ); // unlock 
					}
					FVector Drag;
					FRotator Rot;
					InputAxisMayaCam( Viewport, DragDelta, Drag, Rot );
					if ( bLock )
					{
						LockRot += FRotator( Rot.Pitch, -Rot.Yaw, Rot.Roll );
						LockLocation.Y -= Drag.Y;

						const FMatrix LockMatrix = FRotationMatrix( FRotator(0,LockRot.Yaw,0) ) * FRotationMatrix( FRotator(0,0,LockRot.Pitch) ) * FTranslationMatrix( LockLocation );
						StaticMeshComponent->ConditionalUpdateTransform(FTranslationMatrix(-StaticMeshEditor->StaticMesh->Bounds.Origin) * LockMatrix);
					}
				}
				else
				{
					// Convert the movement delta into drag/rotation deltas

					FVector Drag;
					FRotator Rot;
					FVector Scale;
					MouseDeltaTracker->ConvertMovementDeltaToDragRot( this, DragDelta, Drag, Rot, Scale );

					if( bLock )
					{
						LockRot += FRotator( Rot.Pitch, -Rot.Yaw, Rot.Roll );
						LockLocation.Y -= Drag.Y;

						const FMatrix LockMatrix = FRotationMatrix( FRotator(0,LockRot.Yaw,0) ) * FRotationMatrix( FRotator(0,0,LockRot.Pitch) ) * FTranslationMatrix( LockLocation );
						StaticMeshComponent->ConditionalUpdateTransform(FTranslationMatrix(-StaticMeshEditor->StaticMesh->Bounds.Origin) * LockMatrix);
					}
					else
					{
						MoveViewportCamera( Drag, Rot );
					}
				}

				MouseDeltaTracker->ReduceBy( DragDelta );
			}
		}
	}

	Viewport->Invalidate();

	return TRUE;
}


/*-----------------------------------------------------------------------------
	WxStaticMeshEditor
-----------------------------------------------------------------------------*/

IMPLEMENT_DYNAMIC_CLASS(WxStaticMeshEditor,wxFrame);

BEGIN_EVENT_TABLE( WxStaticMeshEditor, wxFrame )
	EVT_SIZE( WxStaticMeshEditor::OnSize )
	EVT_PAINT( WxStaticMeshEditor::OnPaint )
	EVT_UPDATE_UI( ID_SHOW_OPEN_EDGES, WxStaticMeshEditor::UI_ShowEdges )
	EVT_UPDATE_UI( ID_SHOW_UV_OVERLAY, WxStaticMeshEditor::UI_ShowUVOverlay )
	EVT_UPDATE_UI( ID_SHOW_WIREFRAME, WxStaticMeshEditor::UI_ShowWireframe )
	EVT_UPDATE_UI( ID_SHOW_BOUNDS, WxStaticMeshEditor::UI_ShowBounds )
	EVT_UPDATE_UI( IDMN_COLLISION, WxStaticMeshEditor::UI_ShowCollision )
	EVT_UPDATE_UI( ID_LOCK_CAMERA, WxStaticMeshEditor::UI_LockCamera )
	EVT_MENU( ID_SHOW_OPEN_EDGES, WxStaticMeshEditor::OnShowEdges )
	EVT_MENU( ID_SHOW_UV_OVERLAY, WxStaticMeshEditor::OnShowUVOverlay )
	EVT_MENU( ID_SHOW_WIREFRAME, WxStaticMeshEditor::OnShowWireframe )
	EVT_MENU( ID_SHOW_BOUNDS, WxStaticMeshEditor::OnShowBounds )
	EVT_MENU( IDMN_COLLISION, WxStaticMeshEditor::OnShowCollision )
	EVT_MENU( ID_LOCK_CAMERA, WxStaticMeshEditor::OnLockCamera )
	EVT_MENU( ID_SAVE_THUMBNAIL, WxStaticMeshEditor::OnSaveThumbnailAngle )
	EVT_MENU( IDMN_SME_COLLISION_6DOP, WxStaticMeshEditor::OnCollision6DOP )
	EVT_MENU( IDMN_SME_COLLISION_10DOPX, WxStaticMeshEditor::OnCollision10DOPX )
	EVT_MENU( IDMN_SME_COLLISION_10DOPY, WxStaticMeshEditor::OnCollision10DOPY )
	EVT_MENU( IDMN_SME_COLLISION_10DOPZ, WxStaticMeshEditor::OnCollision10DOPZ )
	EVT_MENU( IDMN_SME_COLLISION_18DOP, WxStaticMeshEditor::OnCollision18DOP )
	EVT_MENU( IDMN_SME_COLLISION_26DOP, WxStaticMeshEditor::OnCollision26DOP )
	EVT_MENU( IDMN_SME_COLLISION_SPHERE, WxStaticMeshEditor::OnCollisionSphere )
	EVT_MENU( IDM_SME_LOD_AUTO, WxStaticMeshEditor::OnForceLODLevel )
	EVT_MENU( IDM_SME_LOD_BASE, WxStaticMeshEditor::OnForceLODLevel )
	EVT_MENU( IDM_SME_LOD_1, WxStaticMeshEditor::OnForceLODLevel )
	EVT_MENU( IDM_SME_LOD_2, WxStaticMeshEditor::OnForceLODLevel )
	EVT_MENU( IDM_SME_LOD_3, WxStaticMeshEditor::OnForceLODLevel )
	EVT_MENU( IDM_SME_IMPORTMESHLOD, WxStaticMeshEditor::OnImportMeshLOD )
	EVT_MENU( IDM_SME_REMOVELOD,  WxStaticMeshEditor::OnRemoveLOD )
	EVT_MENU( IDM_SME_GENERATELOD,  WxStaticMeshEditor::OnGenerateLOD )
	EVT_MENU( IDM_SME_GENERATEUVS,  WxStaticMeshEditor::OnGenerateUVs )
	EVT_MENU( IDMN_SME_COLLISION_REMOVE, WxStaticMeshEditor::OnCollisionRemove )
	EVT_MENU( IDMN_SME_COLLISION_DECOMP, WxStaticMeshEditor::OnCollisionConvexDecomp )
END_EVENT_TABLE()

WxStaticMeshEditor::WxStaticMeshEditor() : 
FDockingParent(this)
{}

WxStaticMeshEditor::WxStaticMeshEditor( wxWindow* Parent, wxWindowID id, UStaticMesh* InStaticMesh ) :	
        wxFrame( Parent, id, TEXT(""), wxDefaultPosition, wxDefaultSize, wxFRAME_FLOAT_ON_PARENT | wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR )
	,   FDockingParent(this)
	,	StaticMesh( InStaticMesh )
	,	DrawOpenEdges(FALSE)
	,	DrawUVOverlay(FALSE)
	,	DecompOptions(NULL)
{
	// Set the static mesh editor window title to include the static mesh being edited.
	SetTitle( *FString::Printf( *LocalizeUnrealEd("StaticMeshEditorCaption_F"), *StaticMesh->GetPathName() ) );

	FStaticMeshRenderData* LODModel = &StaticMesh->LODModels(0);
	check(LODModel);

	// Create property window
	PropertyWindow = new WxPropertyWindow;
	PropertyWindow->Create( this, NULL );
	PropertyWindow->SetObject( StaticMesh, 1,1,0 );
	
	// Create viewport.
	ViewportHolder = new WxViewportHolder( this, -1, 0 );
	ViewportClient = new FStaticMeshEditorViewportClient( this );
	ViewportClient->Viewport = GEngine->Client->CreateWindowChildViewport( ViewportClient, (HWND)ViewportHolder->GetHandle() );
	ViewportClient->Viewport->CaptureJoystickInput( FALSE );
	ViewportHolder->SetViewport( ViewportClient->Viewport );
	ViewportHolder->Show();

	FWindowUtil::LoadPosSize( TEXT("StaticMeshEditor"), this, 64,64,800,450 );

	// Add docking windows.
	{
		AddDockingWindow(PropertyWindow, FDockingParent::DH_Right, *FString::Printf(*LocalizeUnrealEd("PropertiesCaption_F"), *StaticMesh->GetPathName()), *LocalizeUnrealEd("Properties"));
		SetDockHostSize(FDockingParent::DH_Right, 300);

		wxPane* PreviewPane = new wxPane( this );
		{
			PreviewPane->ShowHeader(false);
			PreviewPane->ShowCloseButton( false );
			PreviewPane->SetClient(ViewportHolder);
		}
		LayoutManager->SetLayout( wxDWF_SPLITTER_BORDERS, PreviewPane );

		// Try to load a existing layout for the docking windows.
		LoadDockingLayout();
	}

	ToolBar = new WxStaticMeshEditorBar( this, -1 );
	SetToolBar( ToolBar );

	MenuBar = new WxStaticMeshEditMenu();
	AppendDockingMenu(MenuBar);
	SetMenuBar( MenuBar );

	UpdateToolbars();

	// Reset view position to something reasonable
	LockCamera( ViewportClient->bLock );
}

WxStaticMeshEditor::~WxStaticMeshEditor()
{
	FWindowUtil::SavePosSize( TEXT("StaticMeshEditor"), this );

	SaveDockingLayout();

	GEngine->Client->CloseViewport( ViewportClient->Viewport );
	ViewportClient->Viewport = NULL;
	delete ViewportClient;

	PropertyWindow->SetObject( NULL, 0,0,0 );
	delete PropertyWindow;
}

/**
 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
 *  @return A string representing a name to use for this docking parent.
 */
const TCHAR* WxStaticMeshEditor::GetDockingParentName() const
{
	return DockingParent_Name;
}

/**
 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
 */
const INT WxStaticMeshEditor::GetDockingParentVersion() const
{
	return DockingParent_Version;
}

void WxStaticMeshEditor::OnSize( wxSizeEvent& In )
{
	wxFrame::OnSize(In);
}

void WxStaticMeshEditor::Serialize(FArchive& Ar)
{
	Ar << StaticMesh;
	check(ViewportClient);
	ViewportClient->Serialize(Ar);
}

void WxStaticMeshEditor::OnPaint( wxPaintEvent& In )
{
	wxPaintDC dc( this );
	ViewportClient->Viewport->Invalidate();
}

void WxStaticMeshEditor::UI_ShowEdges( wxUpdateUIEvent& In )
{
	const bool bCheckStatus = DrawOpenEdges == TRUE;
	In.Check( bCheckStatus );
}

void WxStaticMeshEditor::UI_ShowUVOverlay( wxUpdateUIEvent& In )
{
	const bool bCheckStatus = DrawUVOverlay == TRUE;
	In.Check( bCheckStatus );
}

void WxStaticMeshEditor::UI_ShowWireframe( wxUpdateUIEvent& In )
{
	const bool bCheckStatus = (ViewportClient->ShowFlags & SHOW_ViewMode_Mask) == SHOW_ViewMode_Wireframe;
	In.Check( bCheckStatus );
}

void WxStaticMeshEditor::UI_ShowBounds( wxUpdateUIEvent& In )
{
	const bool bCheckStatus = ViewportClient->ShowFlags & SHOW_Bounds ? true : false;
	In.Check( bCheckStatus );
}

void WxStaticMeshEditor::UI_ShowCollision( wxUpdateUIEvent& In )
{
	const bool bCheckStatus = ViewportClient->ShowFlags & SHOW_Collision ? true : false;
	In.Check( bCheckStatus );
}

void WxStaticMeshEditor::UI_LockCamera( wxUpdateUIEvent& In )
{
	const bool bCheckStatus = ViewportClient->bLock == TRUE;
	In.Check( bCheckStatus );
}

void WxStaticMeshEditor::OnShowEdges( wxCommandEvent& In )
{
	DrawOpenEdges = !DrawOpenEdges;
	ViewportClient->Viewport->Invalidate();
}

void WxStaticMeshEditor::OnShowUVOverlay( wxCommandEvent& In )
{
	DrawUVOverlay = !DrawUVOverlay;
	ViewportClient->Viewport->Invalidate();
}

void WxStaticMeshEditor::OnShowWireframe( wxCommandEvent& In )
{
	if((ViewportClient->ShowFlags & SHOW_ViewMode_Mask) == SHOW_ViewMode_Wireframe)
	{
		ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
		ViewportClient->ShowFlags |= SHOW_ViewMode_Lit;
	}
	else
	{
		ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
		ViewportClient->ShowFlags |= SHOW_ViewMode_Wireframe;
	}
	ViewportClient->Viewport->Invalidate();
}

void WxStaticMeshEditor::OnShowBounds( wxCommandEvent& In )
{
	ViewportClient->ShowFlags ^= SHOW_Bounds;
	ViewportClient->Viewport->Invalidate();
}

void WxStaticMeshEditor::OnShowCollision( wxCommandEvent& In )
{
	ViewportClient->ShowFlags ^= SHOW_Collision;
	ViewportClient->Viewport->Invalidate();
}

void WxStaticMeshEditor::LockCamera(UBOOL bInLock)
{
	ViewportClient->bLock = bInLock;
	ViewportClient->Viewport->Invalidate();

	// Only reset view location/rotation when the user turns ON locking.
	if( ViewportClient->bLock )
	{
		ViewportClient->ViewLocation = -FVector(0,StaticMesh->Bounds.SphereRadius / (75.0f * (FLOAT)PI / 360.0f),0);
		ViewportClient->ViewRotation = FRotator(0,16384,0);

		ViewportClient->LockLocation = FVector(0,0,0);
		ViewportClient->LockRot = StaticMesh->ThumbnailAngle;

		FMatrix LockMatrix = FRotationMatrix( FRotator(0,ViewportClient->LockRot.Yaw,0) ) * FRotationMatrix( FRotator(0,0,ViewportClient->LockRot.Pitch) );
		ViewportClient->StaticMeshComponent->ConditionalUpdateTransform(FTranslationMatrix(-StaticMesh->Bounds.Origin) * LockMatrix);
	}
	else
	{
		ViewportClient->LockLocation = FVector(0,0,0);
		ViewportClient->LockRot = FRotator(0,0,0);
	}
}

void WxStaticMeshEditor::OnLockCamera( wxCommandEvent& In )
{
	if ( ViewportClient->bAllowMayaCam && GEditor->bUseMayaCameraControls )
	{
		return;
	}

	// Toggle camera locking.
	LockCamera( !ViewportClient->bLock );
}

void WxStaticMeshEditor::OnSaveThumbnailAngle( wxCommandEvent& In )
{
	ViewportClient->StaticMeshComponent->StaticMesh->ThumbnailAngle = ViewportClient->LockRot;
	ViewportClient->StaticMeshComponent->StaticMesh->ThumbnailDistance = ViewportClient->LockLocation.Y;
	ViewportClient->StaticMeshComponent->StaticMesh->MarkPackageDirty();
	GCallbackEvent->Send(CALLBACK_RefreshEditor_GenericBrowser);
}

void WxStaticMeshEditor::OnCollision6DOP( wxCommandEvent& In )
{
	GenerateKDop(KDopDir6,6);
}

void WxStaticMeshEditor::OnCollision10DOPX( wxCommandEvent& In )
{
	GenerateKDop(KDopDir10X,10);
}

void WxStaticMeshEditor::OnCollision10DOPY( wxCommandEvent& In )
{
	GenerateKDop(KDopDir10Y,10);
}

void WxStaticMeshEditor::OnCollision10DOPZ( wxCommandEvent& In )
{
	GenerateKDop(KDopDir10Z,10);
}

void WxStaticMeshEditor::OnCollision18DOP( wxCommandEvent& In )
{
	GenerateKDop(KDopDir18,18);
}

void WxStaticMeshEditor::OnCollision26DOP( wxCommandEvent& In )
{
	GenerateKDop(KDopDir26,26);
}

void WxStaticMeshEditor::OnCollisionSphere( wxCommandEvent& In )
{
	GenerateSphere();
}

/**
 * Handles the user selection of the remove collision option. Uses the
 * common routine for clearing the collision
 *
 * @param In the command event to handle
 */
void WxStaticMeshEditor::OnCollisionRemove( wxCommandEvent& In )
{
	RemoveCollision();
}

/** When menu item is selected, pop up options dialog for convex decomposition util */
void WxStaticMeshEditor::OnCollisionConvexDecomp( wxCommandEvent& In )
{
	if(!DecompOptions)
	{
		DecompOptions = new WxConvexDecompOptions( this, this );
		DecompOptions->Show();
		ViewportClient->ShowFlags |= SHOW_Collision;
	}
}

/** This is called when Apply is pressed in the dialog. Does the actual processing. */
void WxStaticMeshEditor::DoDecomp(INT Depth, INT MaxVerts, FLOAT CollapseThresh)
{
	// Check we have a selected StaticMesh
	if(StaticMesh)
	{
		// Make vertex buffer
		INT NumVerts = StaticMesh->LODModels(0).NumVertices;
		TArray<FVector> Verts;
		for(INT i=0; i<NumVerts; i++)
		{
			FVector Vert = StaticMesh->LODModels(0).PositionVertexBuffer.VertexPosition(i);
			Verts.AddItem(Vert);
		}
	
		// Make index buffer
		INT NumIndices = StaticMesh->LODModels(0).IndexBuffer.Indices.Num();
		TArray<INT> Indices;
		for(INT i=0; i<NumIndices; i++)
		{
			INT Index = StaticMesh->LODModels(0).IndexBuffer.Indices(i);
			Indices.AddItem(Index);
		}

		// Make sure rendering is done - so we are not changing data being used by collision drawing.
		FlushRenderingCommands();

		// Get the BodySetup we are going to put the collision into
		URB_BodySetup* bs = StaticMesh->BodySetup;
		if(bs)
		{
			bs->AggGeom.EmptyElements();
			bs->ClearShapeCache();
		}
		else
		{
			// Otherwise, create one here.
			StaticMesh->BodySetup = ConstructObject<URB_BodySetup>(URB_BodySetup::StaticClass(), StaticMesh);
			bs = StaticMesh->BodySetup;
		}

		// Run actual util to do the work
		DecomposeMeshToHulls(&(bs->AggGeom), Verts, Indices, Depth, 0.1f, CollapseThresh, MaxVerts);		

		// Mark mesh as dirty
		StaticMesh->MarkPackageDirty();

		// Update screen.
		ViewportClient->Viewport->Invalidate();
		PropertyWindow->SetObject( StaticMesh, 1,1,0 );
	}
}

/** When options window is closed - clear pointer. */
void WxStaticMeshEditor::DecompOptionsClosed()
{
	DecompOptions = NULL;
	ViewportClient->ShowFlags &= ~SHOW_Collision;
}


/**
 * Clears the collision data for the static mesh
 */
void WxStaticMeshEditor::RemoveCollision(void)
{
	// If we have a collision model for this staticmesh, ask if we want to replace it.
	if (StaticMesh->BodySetup != NULL)
	{
		UBOOL bShouldReplace = appMsgf(AMT_YesNo, *LocalizeUnrealEd("RemoveCollisionPrompt"));
		if (bShouldReplace == TRUE)
		{
			StaticMesh->BodySetup = NULL;
			// Mark staticmesh as dirty, to help make sure it gets saved.
			StaticMesh->MarkPackageDirty();
			// Update views/property windows
			ViewportClient->Viewport->Invalidate();
			PropertyWindow->SetObject( StaticMesh, 1,1,0 );
		}
	}
}


void WxStaticMeshEditor::GenerateKDop(const FVector* Directions,UINT NumDirections)
{
	TArray<FVector>	DirArray;

	for(UINT DirectionIndex = 0;DirectionIndex < NumDirections;DirectionIndex++)
		DirArray.AddItem(Directions[DirectionIndex]);

	GenerateKDopAsCollisionModel( StaticMesh, DirArray );

	ViewportClient->Viewport->Invalidate();
	PropertyWindow->SetObject( StaticMesh, 1,1,0 );
}

void WxStaticMeshEditor::GenerateSphere()
{
	GenerateSphereAsKarmaCollision( StaticMesh );

	ViewportClient->Viewport->Invalidate();
	PropertyWindow->SetObject( StaticMesh, 1,1,0 );
}

/** Handler for forcing the rendering of the preview model to use a particular LOD. */
void WxStaticMeshEditor::OnForceLODLevel( wxCommandEvent& In)
{
	if(In.GetId() == IDM_SME_LOD_AUTO)
	{
		UpdateLODStats(0);
		ViewportClient->StaticMeshComponent->ForcedLodModel = 0;
	}
	else if(In.GetId() == IDM_SME_LOD_BASE)
	{
		UpdateLODStats(0);
		ViewportClient->StaticMeshComponent->ForcedLodModel = 1;
	}
	else if(In.GetId() == IDM_SME_LOD_1)
	{
		UpdateLODStats(1);
		ViewportClient->StaticMeshComponent->ForcedLodModel = 2;
	}
	else if(In.GetId() == IDM_SME_LOD_2)
	{
		UpdateLODStats(2);
		ViewportClient->StaticMeshComponent->ForcedLodModel = 3;
	}
	else if(In.GetId() == IDM_SME_LOD_3)
	{
		UpdateLODStats(3);
		ViewportClient->StaticMeshComponent->ForcedLodModel = 4;
	}

	//MenuBar->Check( In.GetId(), true );
	ToolBar->ToggleTool( In.GetId(), true );
	{FComponentReattachContext ReattachContext(ViewportClient->StaticMeshComponent);}
	ViewportClient->Viewport->Invalidate();
}

/** Handler for removing a particular LOD from the SkeletalMesh. */
void WxStaticMeshEditor::OnRemoveLOD(wxCommandEvent &In)
{
	if( StaticMesh->LODModels.Num() == 1 )
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("NoLODToRemove") );
		return;
	}

	// Now display combo to choose which LOD to remove.
	TArray<FString> LODStrings;
	LODStrings.AddZeroed( StaticMesh->LODModels.Num()-1 );
	for(INT i=0; i<StaticMesh->LODModels.Num()-1; i++)
	{
		LODStrings(i) = FString::Printf( TEXT("%d"), i+1 );
	}

	// pop up dialog
	WxDlgGenericComboEntry dlg;
	if( dlg.ShowModal( TEXT("ChooseLODLevel"), TEXT("LODLevel:"), LODStrings, 0, TRUE ) == wxID_OK )
	{
		check( StaticMesh->LODInfo.Num() == StaticMesh->LODModels.Num() );

		// If its a valid LOD, kill it.
		INT DesiredLOD = dlg.GetComboBox().GetSelection() + 1;
		if( DesiredLOD > 0 && DesiredLOD < StaticMesh->LODModels.Num() )
		{
			StaticMesh->LODModels.Remove(DesiredLOD);
			StaticMesh->LODInfo.Remove(DesiredLOD);

			// Set the forced LOD to Auto.
			ViewportClient->StaticMeshComponent->ForcedLodModel = 0;
			ToolBar->ToggleTool( IDM_SME_LOD_AUTO, true );
			UpdateToolbars();
		}
	}
}


/** Event for generating an LOD */
void WxStaticMeshEditor::OnGenerateLOD( wxCommandEvent& In )
{
	INT NumLODs = StaticMesh->LODModels.Num()+1;
	if(NumLODs > 3)
	{
		NumLODs = 3;
	}

	// Combo options to choose mesh to generate LOD for
	TArray<FString> LODStrings;
	LODStrings.AddZeroed( NumLODs );
	for(INT i=0; i<NumLODs; i++)
	{
		LODStrings(i) = FString::Printf( TEXT("%d"), i+1 );
	}

	WxDlgGenericComboEntry dlg;	
	if( dlg.ShowModal( TEXT("ChooseLODLevel"), TEXT("LODLevel:"), LODStrings, 0, TRUE ) == wxID_OK )
	{
		const INT DesiredLOD = dlg.GetComboBox().GetSelection()+1;

		// Dialog to select target triangle count
		WxDlgGenericStringEntry dlg2;

		FStaticMeshRenderData& BaseLOD = StaticMesh->LODModels(0);

		FString NumFacesString = appItoa(BaseLOD.IndexBuffer.Indices.Num() / 6);
		if( dlg2.ShowModal(TEXT("GenerateLOD"), TEXT("LODTriCount"), *NumFacesString) == wxID_OK )
		{
			const FString Tris = dlg2.GetEnteredString();
			const INT NumTris = appAtoi(*Tris);

			if (GenerateLOD(StaticMesh,DesiredLOD,NumTris))
			{
				UpdateToolbars();
				appMsgf( AMT_OK, *LocalizeUnrealEd("LODGenerationSuccessful") );
			}
		}
	}
}


/** Event for generating UVs */
void WxStaticMeshEditor::OnGenerateUVs( wxCommandEvent& In )
{
	//an entry for each LOD
	TArray<FString> LODStrings;
	//the number of texture coordinates each LOD has 
	//used for updating the tex coord combo box when a different LOD is chosen
	TArray<INT> LODNumTexcoords;
	LODStrings.AddZeroed(StaticMesh->LODModels.Num());
	LODNumTexcoords.AddZeroed(StaticMesh->LODModels.Num());
	for(INT i = 0; i < StaticMesh->LODModels.Num(); i++)
	{
		//populate the arrays
		LODStrings(i) = FString::Printf( TEXT("%d"), i );
		LODNumTexcoords(i) = StaticMesh->LODModels(i).VertexBuffer.GetNumTexCoords();
	}

	//create a dialog that exposes the options needed to generate UV's
	WxDlgGenerateUVOptions dlg(this, LODStrings, LODNumTexcoords, 0.9f);
	if (dlg.ShowModal() == wxID_OK)
	{
		UINT NumGeneratedPatches = 0;
		//call the utility helper with the user supplied parameters
		if (GenerateUVs(StaticMesh, dlg.ChosenLODIndex, dlg.ChosenTexIndex, dlg.MaxStretch, NumGeneratedPatches))
		{
			//update toolbar states
			UpdateToolbars();
			appMsgf( AMT_OK, *FString::Printf(*LocalizeUnrealEd("UVGenerationSuccessful"), NumGeneratedPatches, dlg.MaxStretch));
		}
	}
}

void WxStaticMeshEditor::OnImportMeshLOD( wxCommandEvent& In )
{
	WxFileDialog ImportFileDialog( this, 
		*LocalizeUnrealEd("ImportMeshLOD"), 
		*(GApp->LastDir[LD_GENERIC_IMPORT]),
		TEXT(""),
		TEXT("ASE files|*.ase"),
		wxOPEN | wxFILE_MUST_EXIST | wxHIDE_READONLY,
		wxDefaultPosition);

	if( ImportFileDialog.ShowModal() == wxID_OK )
	{
		wxArrayString ImportFilePaths;
		ImportFileDialog.GetPaths(ImportFilePaths);

		if(ImportFilePaths.Count() == 0)
		{
			appMsgf( AMT_OK, *LocalizeUnrealEd("NoFileSelectedForLOD") );
		}
		else if(ImportFilePaths.Count() > 1)
		{
			appMsgf( AMT_OK, *LocalizeUnrealEd("MultipleFilesSelectedForLOD") );
		}
		else
		{
			// Now display combo to choose which LOD to import this mesh as.
			TArray<FString> LODStrings;
			LODStrings.AddZeroed( 3 );
			for(INT i=0; i<3; i++)
			{
				LODStrings(i) = FString::Printf( TEXT("%d"), i+1 );
			}

			WxDlgGenericComboEntry dlg;
			if( dlg.ShowModal( TEXT("ChooseLODLevel"), TEXT("LODLevel:"), LODStrings, 0, TRUE ) == wxID_OK )
			{
				INT DesiredLOD = dlg.GetComboBox().GetSelection()+1;
				// If the LOD we want
				if( DesiredLOD > 0  )
				{
					FFilename Filename = (const TCHAR*)ImportFilePaths[0];
					GApp->LastDir[LD_GENERIC_IMPORT] = Filename.GetPath(); // Save path as default for next time.

					// Load the data from the file into a byte array.
					FString Data;
					if( appLoadFileToString( Data, *Filename ) )
					{
						const TCHAR* Ptr = *Data;

						debugf(TEXT("LOD %d loading (%x)"),DesiredLOD,(INT)&DesiredLOD);

						// Use the StaticMeshFactory to load this StaticMesh into a temporary StaticMesh.
						UStaticMeshFactory* StaticMeshFact = new UStaticMeshFactory();
						UStaticMesh* TempStaticMesh = (UStaticMesh*)StaticMeshFact->FactoryCreateText( 
							UStaticMesh::StaticClass(), UObject::GetTransientPackage(), NAME_None, 0, NULL, TEXT("ASE"), Ptr, Ptr+Data.Len(), GWarn );

						debugf(TEXT("LOD %d loaded"),DesiredLOD);

						// Extract data to LOD
						if(TempStaticMesh)
						{
							// Detach all instances of the static mesh from the scene while we're merging the LOD data.
							FComponentReattachContext ReattachContext(ViewportClient->StaticMeshComponent);

							// Overwriting existing LOD
							if(DesiredLOD < StaticMesh->LODModels.Num())
							{
								StaticMesh->LODModels(DesiredLOD).ReleaseResources();
								StaticMesh->LODModels(DesiredLOD).PositionVertexBuffer.CleanUp();
								StaticMesh->LODModels(DesiredLOD).VertexBuffer.CleanUp();
								StaticMesh->LODModels(DesiredLOD) = TempStaticMesh->LODModels(0);
							}
							// Adding new LOD
							else
							{
								// Add dummy LODs if the LOD being inserted is not the next one in the array
								while(StaticMesh->LODModels.Num() < DesiredLOD)
								{
									StaticMesh->LODModels.AddRawItem(0); 
								}

								// Add dummy LODs if the LOD being inserted is not the next one in the array
								while(StaticMesh->LODInfo.Num() <= DesiredLOD)
								{
									StaticMesh->LODInfo.AddZeroed();
								}

								StaticMesh->LODInfo(DesiredLOD) = FStaticMeshLODInfo();

								FStaticMeshRenderData* Data = new(StaticMesh->LODModels) FStaticMeshRenderData();
								*Data = TempStaticMesh->LODModels(0);
							}
							// Rebuild the static mesh.
							StaticMesh->Build();
						}

						// Update mesh component
						StaticMesh->MarkPackageDirty();
						UpdateToolbars();
						appMsgf( AMT_OK, *LocalizeUnrealEd("LODImportSuccessful"), DesiredLOD );
					}
				}
			}
		}

	}
}

/**
* Updates the UI
*/
void WxStaticMeshEditor::UpdateToolbars()
{
	if(StaticMesh)
	{
		UpdateLODStats(0);

		// Update LOD info with the materials
		check(StaticMesh->LODInfo.Num() == StaticMesh->LODModels.Num());
		for( INT LODIndex = 0; LODIndex < StaticMesh->LODModels.Num(); LODIndex++)
		{
			FStaticMeshRenderData& LODData = StaticMesh->LODModels(LODIndex);
			FStaticMeshLODInfo& LODInfo = StaticMesh->LODInfo(LODIndex);

			LODInfo.Elements.Empty();
			LODInfo.Elements.AddZeroed( LODData.Elements.Num() );

			for(INT MatIndex = 0; MatIndex < LODData.Elements.Num(); MatIndex++)
			{
				LODInfo.Elements(MatIndex).Material = LODData.Elements(MatIndex).Material;
				LODInfo.Elements(MatIndex).bEnableCollision = LODData.Elements(MatIndex).EnableCollision;

			}
		}

		PropertyWindow->Refresh();

		ToolBar->EnableTool( IDM_SME_LOD_1, StaticMesh->LODModels.Num() > 1 );
		ToolBar->EnableTool( IDM_SME_LOD_2, StaticMesh->LODModels.Num() > 2 );
		ToolBar->EnableTool( IDM_SME_LOD_3, StaticMesh->LODModels.Num() > 3 );
	}
}


/** 
* Updates NumTriangles, and NumUVChannels for the given LOD 
*/
void WxStaticMeshEditor::UpdateLODStats(INT CurrentLOD) {

	check(CurrentLOD >= 0 && CurrentLOD < StaticMesh->LODModels.Num());
	FStaticMeshRenderData* LODModel = &StaticMesh->LODModels(CurrentLOD);
	check(LODModel);

	NumTriangles = LODModel->IndexBuffer.Indices.Num() / 3;

	NumUVChannels = LODModel->VertexBuffer.GetNumTexCoords();
}

