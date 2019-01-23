/*=============================================================================
	TranslucentRendering.cpp: Translucent rendering implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "TileRendering.h"

/** FTileRenderer globals */
UBOOL FTileRenderer::bInitialized = FALSE;
TGlobalResource<FLocalVertexFactory> FTileRenderer::VertexFactory;
TGlobalResource<FMaterialTileVertexBuffer> FTileRenderer::VertexBuffer;
FMeshElement FTileRenderer::Mesh;
FMatrix FTileRenderer::SaveViewProjectionMatrix = FMatrix::Identity;


FTileRenderer::FTileRenderer()
{
	// if the static data was never initialized, do it now
	if (!bInitialized)
	{
		bInitialized = TRUE;

		FLocalVertexFactory::DataType Data;
		// position
		Data.PositionComponent = FVertexStreamComponent(
			&VertexBuffer,STRUCT_OFFSET(FMaterialTileVertex,Position),sizeof(FMaterialTileVertex),VET_Float3);
		// tangents
		Data.TangentBasisComponents[0] = FVertexStreamComponent(
			&VertexBuffer,STRUCT_OFFSET(FMaterialTileVertex,TangentX),sizeof(FMaterialTileVertex),VET_PackedNormal);
		Data.TangentBasisComponents[1] = FVertexStreamComponent(
			&VertexBuffer,STRUCT_OFFSET(FMaterialTileVertex,TangentZ),sizeof(FMaterialTileVertex),VET_PackedNormal);
		// color
		Data.ColorComponent = FVertexStreamComponent(
			&VertexBuffer,STRUCT_OFFSET(FMaterialTileVertex,Color),sizeof(FMaterialTileVertex),VET_Color);
		// UVs
		Data.TextureCoordinates.AddItem(FVertexStreamComponent(
			&VertexBuffer,STRUCT_OFFSET(FMaterialTileVertex,U),sizeof(FMaterialTileVertex),VET_Float2));

        // update the data
		VertexFactory.SetData(Data);

		Mesh.VertexFactory = &VertexFactory;
		Mesh.DynamicVertexStride = sizeof(FMaterialTileVertex);
		Mesh.WorldToLocal = FMatrix::Identity;
		Mesh.FirstIndex = 0;
		Mesh.NumPrimitives = 2;
		Mesh.MinVertexIndex = 0;
		Mesh.MaxVertexIndex = 3;
		Mesh.ReverseCulling = FALSE;
		Mesh.UseDynamicData = TRUE;
		Mesh.Type = PT_TriangleFan;
		Mesh.DepthPriorityGroup = SDPG_Foreground;
	}
}

void FTileRenderer::DrawTile(FCommandContextRHI* Context, const class FViewInfo& View, const FMaterialRenderProxy* MaterialRenderProxy)
{
	// update the FMeshElement
	Mesh.UseDynamicData = FALSE;
	Mesh.MaterialRenderProxy = MaterialRenderProxy;

	// full screen render, just use the identity matrix for LocalToWorld
	PrepareShaders(Context, View, MaterialRenderProxy, FMatrix::Identity, TRUE);
}

void FTileRenderer::DrawTile(FCommandContextRHI* Context, const class FViewInfo& View, const FMaterialRenderProxy* MaterialRenderProxy, FLOAT X, FLOAT Y, FLOAT SizeX, FLOAT SizeY, UBOOL bIsHitTesting, const FHitProxyId HitProxyId)
{
	// @GEMINI_TODO: Fix my matrices below :(
DrawTile(Context, View, MaterialRenderProxy, X, Y, SizeX, SizeY, 0, 0, 1, 1, bIsHitTesting, HitProxyId);
return;
	// since we have 0..1 UVs, we can use the pre-made vertex factory data
#if GEMINI_TODO
	// force the fully dynamic path for hit testing (which has the hit testing code)
	if (bIsHitTesting)
	{
		DrawTile(View, MaterialRenderProxy, X, Y, SizeX, SizeY, 0, 0, 1, 1, bIsHitTesting, HitProxyId);
		return;
	}

	// update the FMeshElement
	Mesh.UseDynamicData = FALSE;
	Mesh.MaterialRenderProxy = MaterialRenderProxy;

	// @GEMINI_TODO: Cache these inversions?
	FLOAT InvScreenX = 1.0f / View.SizeX;
 	FLOAT InvScreenY = 1.0f / View.SizeY;

	PrepareShaders(
		Context,
		View, 
		MaterialRenderProxy,
		FMatrix(
			FPlane(InvScreenX * SizeX, 0.0f, 0.0f, 0.0f),
			FPlane(0.0f, InvScreenY * SizeY, 0.0f, 0.0f),
			FPlane(0.0f, 0.0f, 1.0f, 0.0f),
			FPlane(InvScreenX * (X * 0.5f), InvScreenY * (Y * 0.5f), 0.0f, 1.0f)),
		TRUE,
		bIsHitTesting,
		HitProxyId);

#endif
}

void FTileRenderer::DrawTile(FCommandContextRHI* Context, const class FViewInfo& View, const FMaterialRenderProxy* MaterialRenderProxy, FLOAT X, FLOAT Y, FLOAT SizeX, FLOAT SizeY, FLOAT U, FLOAT V, FLOAT SizeU, FLOAT SizeV, UBOOL bIsHitTesting, const FHitProxyId HitProxyId)
{
if (bIsHitTesting)
{
	// @GEMINI_TODO: the hitproxy drawpolicy should force DefaultMaterial for opaque materials (will need to generate hitproxy shaders for non-opaque materials!)
	MaterialRenderProxy = GEngine->DefaultMaterial->GetRenderProxy(0);
}

	// draw the mesh
#if GEMINI_TODO
	// let RHI allocate memory (needs DrawDymamicMesh to be able to use RHIEndDrawPrimitiveUP for hittesting)
	void* Vertices;
	//RHIBeginDrawPrimitiveUP(Context sizeof(FMaterialTileVertex) * 4, Vertices);
	FMaterialTileVertex* DestVertex = (FMaterialTileVertex*)Vertices;
#else
	FMaterialTileVertex DestVertex[4];
#endif

	// create verts
	DestVertex[0].Initialize(X, Y, U, V);
	DestVertex[1].Initialize(X, Y + SizeY, U, V + SizeV);
	DestVertex[2].Initialize(X + SizeX, Y + SizeY, U + SizeU, V + SizeV);
	DestVertex[3].Initialize(X + SizeX, Y, U + SizeU, V);

	// update the FMeshElement
	Mesh.UseDynamicData = TRUE;
	Mesh.DynamicVertexData = DestVertex;
	Mesh.MaterialRenderProxy = MaterialRenderProxy;

	// set shaders and render the mesh
	PrepareShaders(Context, View, MaterialRenderProxy, FMatrix::Identity, FALSE, bIsHitTesting, HitProxyId);
}




void FTileRenderer::PrepareShaders(FCommandContextRHI* Context, const class FViewInfo& View, const FMaterialRenderProxy* MaterialRenderProxy, const FMatrix& LocalToWorld, UBOOL bUseIdentityViewProjection, UBOOL bIsHitTesting, const FHitProxyId HitProxyId)
{
	const FMaterial* Material = MaterialRenderProxy->GetMaterial();

	// disable depth test & writes
	RHISetDepthState(Context,TStaticDepthState<FALSE,CF_Always>::GetRHI());		
	// default cull mode
	RHISetRasterizerState(Context,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());

	//get the blend mode of the material
	const EBlendMode MaterialBlendMode = Material->GetBlendMode();

	if (bUseIdentityViewProjection)
	{
		SaveViewProjectionMatrix = const_cast<FViewInfo&>(View).ViewProjectionMatrix;
		const_cast<FViewInfo&>(View).ViewProjectionMatrix = FMatrix::Identity;
	}

	// set view shader constants
	RHISetViewParameters( Context, &View, View.ViewProjectionMatrix, View.ViewOrigin );

	// set the LocalToWorld matrix
	Mesh.LocalToWorld = LocalToWorld;

	// handle translucent material blend modes
	if (IsTranslucentBlendMode(MaterialBlendMode))
	{
		FTranslucencyDrawingPolicyFactory::DrawDynamicMesh(Context, &View, FTranslucencyDrawingPolicyFactory::ContextType(), Mesh, FALSE, FALSE, NULL, HitProxyId);
	}
	// handle opaque material
	else
	{
		// make sure we are doing opaque drawing
		// @GEMINI_TODO: Is this set in the below DrawDynamicMesh calls?
		RHISetBlendState(Context, TStaticBlendState<>::GetRHI());

		// draw the mesh
		if (bIsHitTesting)
		{
			FHitProxyDrawingPolicyFactory::DrawDynamicMesh(Context, &View, FHitProxyDrawingPolicyFactory::ContextType(), Mesh, FALSE, FALSE, NULL, HitProxyId);
		}
		else
		{
			FBasePassOpaqueDrawingPolicyFactory::DrawDynamicMesh(Context, &View, FBasePassOpaqueDrawingPolicyFactory::ContextType(), Mesh, FALSE, FALSE, NULL, HitProxyId);
		}
	}		

	if (bUseIdentityViewProjection)
	{
		const_cast<FViewInfo&>(View).ViewProjectionMatrix = SaveViewProjectionMatrix;
	}
}
