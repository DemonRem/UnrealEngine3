/*=============================================================================
	DistortionRendering.cpp: Distortion rendering implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

//** distortion accumulate shader type implementations */
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TDistortionMeshVertexShader<FDistortMeshAccumulatePolicy>,TEXT("DistortAccumulateVertexShader"),TEXT("Main"),SF_Vertex,0,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TDistortionMeshPixelShader<FDistortMeshAccumulatePolicy>,TEXT("DistortAccumulatePixelShader"),TEXT("Main"),SF_Pixel,0,0);
/** distortion apply screen shader type implementations */
IMPLEMENT_SHADER_TYPE(,FDistortionApplyScreenVertexShader,TEXT("DistortApplyScreenVertexShader"),TEXT("Main"),SF_Vertex,0,0);
IMPLEMENT_SHADER_TYPE(,FDistortionApplyScreenPixelShader,TEXT("DistortApplyScreenPixelShader"),TEXT("Main"),SF_Pixel,VER_DISTORTION_AND_DEPTHONLY_RECOMPILE,0);

/*-----------------------------------------------------------------------------
TDistortionMeshDrawingPolicy
-----------------------------------------------------------------------------*/

/**
* Constructor
* @param InIndexBuffer - index buffer for rendering
* @param InVertexFactory - vertex factory for rendering
* @param InMaterialRenderProxy - material instance for rendering
*/
template<class DistortMeshPolicy> 
TDistortionMeshDrawingPolicy<DistortMeshPolicy>::TDistortionMeshDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy
	)
:	FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy)
{
	const FMaterialShaderMap* MaterialShaderIndex = InMaterialRenderProxy->GetMaterial()->GetShaderMap();
	const FMeshMaterialShaderMap* MeshShaderIndex = MaterialShaderIndex->GetMeshShaderMap(InVertexFactory->GetType());
	// get cached shaders
	VertexShader = MeshShaderIndex->GetShader<TDistortionMeshVertexShader<DistortMeshPolicy> >();
	PixelShader = MeshShaderIndex->GetShader<TDistortionMeshPixelShader<DistortMeshPolicy> >();
}

/**
* Match two draw policies
* @param Other - draw policy to compare
* @return TRUE if the draw policies are a match
*/
template<class DistortMeshPolicy>
UBOOL TDistortionMeshDrawingPolicy<DistortMeshPolicy>::Matches(
	const TDistortionMeshDrawingPolicy& Other
	) const
{
	return FMeshDrawingPolicy::Matches(Other) &&
		VertexShader == Other.VertexShader &&
		PixelShader == Other.PixelShader;
}

/**
* Executes the draw commands which can be shared between any meshes using this drawer.
* @param CI - The command interface to execute the draw commands on.
* @param View - The view of the scene being drawn.
*/
template<class DistortMeshPolicy>
void TDistortionMeshDrawingPolicy<DistortMeshPolicy>::DrawShared(
	FCommandContextRHI* Context,
	const FSceneView* View,
	FBoundShaderStateRHIParamRef BoundShaderState
	) const
{
	// Set the translucent shader parameters for the material instance
	VertexShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View);
	PixelShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View);
	
	// Set shared mesh resources
	FMeshDrawingPolicy::DrawShared(Context,View);

	// Set the actual shader & vertex declaration state
	RHISetBoundShaderState(Context, BoundShaderState);
}

/** 
* Create bound shader state using the vertex decl from the mesh draw policy
* as well as the shaders needed to draw the mesh
* @param DynamicStride - optional stride for dynamic vertex data
* @return new bound shader state object
*/
template<class DistortMeshPolicy>
FBoundShaderStateRHIRef TDistortionMeshDrawingPolicy<DistortMeshPolicy>::CreateBoundShaderState(DWORD DynamicStride)
{
	FVertexDeclarationRHIParamRef VertexDeclaration;
	DWORD StreamStrides[MaxVertexElementCount];

	FMeshDrawingPolicy::GetVertexDeclarationInfo(VertexDeclaration, StreamStrides);
	if (DynamicStride)
	{
		StreamStrides[0] = DynamicStride;
	}

	return RHICreateBoundShaderState(VertexDeclaration, StreamStrides, VertexShader->GetVertexShader(), PixelShader->GetPixelShader());
}

/** A logical exclusive or function. */
static UBOOL XOR(UBOOL A,UBOOL B)
{
	return (A && !B) || (!A && B);
}

/**
* Sets the render states for drawing a mesh.
* @param Context - command context
* @param PrimitiveSceneInfo - The primitive drawing the dynamic mesh.  If this is a view element, this will be NULL.
* @param Mesh - mesh element with data needed for rendering
* @param ElementData - context specific data for mesh rendering
*/
template<class DistortMeshPolicy>
void TDistortionMeshDrawingPolicy<DistortMeshPolicy>::SetMeshRenderState(
	FCommandContextRHI* Context,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	const ElementDataType& ElementData
	) const
{
	// Set transforms
	VertexShader->SetLocalTransforms(Context,Mesh.LocalToWorld,Mesh.WorldToLocal);
	PixelShader->SetLocalTransforms(Context,Mesh.MaterialRenderProxy,Mesh.LocalToWorld,bBackFace);
	
	// Set rasterizer state.
	const FRasterizerStateInitializerRHI Initializer = {
		(Mesh.bWireframe || IsWireframe()) ? FM_Wireframe : FM_Solid,
		IsTwoSided() ? CM_None : (XOR(bBackFace, Mesh.ReverseCulling||GInvertCullMode) ? CM_CCW : CM_CW),
		Mesh.DepthBias,
		Mesh.SlopeScaleDepthBias
	};
	RHISetRasterizerStateImmediate(Context, Initializer);
}

/*-----------------------------------------------------------------------------
TDistortionMeshDrawingPolicyFactory
-----------------------------------------------------------------------------*/

/**
* Render a dynamic mesh using a distortion mesh draw policy 
* @return TRUE if the mesh rendered
*/
template<class DistortMeshPolicy>
UBOOL TDistortionMeshDrawingPolicyFactory<DistortMeshPolicy>::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FSceneView* View,
	ContextType DrawingContext,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId
	)
{
	if(Mesh.IsDistortion() && !bBackFace)
	{
		// draw dynamic mesh element using distortion mesh policy
		TDistortionMeshDrawingPolicy<DistortMeshPolicy> DrawingPolicy(
			Mesh.VertexFactory,
			Mesh.MaterialRenderProxy
			);
		DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()));
		DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,typename TDistortionMeshDrawingPolicy<DistortMeshPolicy>::ElementDataType());
		DrawingPolicy.DrawMesh(Context,Mesh);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
* Render a dynamic mesh using a distortion mesh draw policy 
* @return TRUE if the mesh rendered
*/
template<class DistortMeshPolicy>
UBOOL TDistortionMeshDrawingPolicyFactory<DistortMeshPolicy>::DrawStaticMesh(
	FCommandContextRHI* Context,
	const FSceneView* View,
	ContextType DrawingContext,
	const FStaticMesh& StaticMesh,
	UBOOL bBackFace,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId
	)
{
	if(StaticMesh.IsDistortion())
	{
		// draw static mesh element using distortion mesh policy
		TDistortionMeshDrawingPolicy<DistortMeshPolicy> DrawingPolicy(
			StaticMesh.VertexFactory,
			StaticMesh.MaterialRenderProxy
			);
		DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState());
		DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,StaticMesh,bBackFace,typename TDistortionMeshDrawingPolicy<DistortMeshPolicy>::ElementDataType());
		DrawingPolicy.DrawMesh(Context,StaticMesh);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

template<typename DistortMeshPolicy>
UBOOL TDistortionMeshDrawingPolicyFactory<DistortMeshPolicy>::IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
{
	// Non-distorted materials are ignored when rendering distortion.
	return MaterialRenderProxy && !MaterialRenderProxy->GetMaterial()->IsDistorted();
}

/*-----------------------------------------------------------------------------
FDistortionPrimSet
-----------------------------------------------------------------------------*/

/** 
* Iterate over the distortion prims and draw their accumulated offsets
* @param Context - command context
* @param ViewInfo - current view used to draw items
* @param DPGIndex - current DPG used to draw items
* @return TRUE if anything was drawn
*/
UBOOL FDistortionPrimSet::DrawAccumulatedOffsets(FCommandContextRHI* Context,const class FViewInfo* ViewInfo,UINT DPGIndex)
{
	UBOOL bDirty=FALSE;

	// Draw the view's elements with the translucent drawing policy.
	bDirty |= DrawViewElements<TDistortionMeshDrawingPolicyFactory<FDistortMeshAccumulatePolicy> >(
		Context,
		ViewInfo,
		TDistortionMeshDrawingPolicyFactory<FDistortMeshAccumulatePolicy>::ContextType(),
		DPGIndex,
		FALSE // Distortion is rendered post fog.
		);

	if( Prims.Num() )
	{
		// For drawing scene prims with dynamic relevance.
		TDynamicPrimitiveDrawer<TDistortionMeshDrawingPolicyFactory<FDistortMeshAccumulatePolicy> > Drawer(
			Context,
			ViewInfo,
			DPGIndex,
			TDistortionMeshDrawingPolicyFactory<FDistortMeshAccumulatePolicy>::ContextType(),
			FALSE // Distortion is rendered post fog.
			);

		// Draw sorted scene prims
		for( INT PrimIdx=0; PrimIdx < Prims.Num(); PrimIdx++ )
		{
			FPrimitiveSceneInfo* PrimitiveSceneInfo = Prims(PrimIdx);
			const FPrimitiveViewRelevance& ViewRelevance = ViewInfo->PrimitiveViewRelevanceMap(PrimitiveSceneInfo->Id);

			// Render dynamic scene prim
			if( ViewRelevance.bDynamicRelevance )
			{				
				Drawer.SetPrimitive(PrimitiveSceneInfo);
				PrimitiveSceneInfo->Proxy->DrawDynamicElements(
					&Drawer,
					ViewInfo,
					DPGIndex
					);
			}
			// Render static scene prim
			if( ViewRelevance.bStaticRelevance )
			{
				// Render static meshes from static scene prim
				for( INT StaticMeshIdx=0; StaticMeshIdx < PrimitiveSceneInfo->StaticMeshes.Num(); StaticMeshIdx++ )
				{
					FStaticMesh& StaticMesh = PrimitiveSceneInfo->StaticMeshes(StaticMeshIdx);

					// Only render static mesh elements using translucent materials
					if( StaticMesh.IsTranslucent() )
					{
						bDirty |= TDistortionMeshDrawingPolicyFactory<FDistortMeshAccumulatePolicy>::DrawStaticMesh(
							Context,
							ViewInfo,
							TDistortionMeshDrawingPolicyFactory<FDistortMeshAccumulatePolicy>::ContextType(),
							StaticMesh,
							FALSE,
							PrimitiveSceneInfo,
							StaticMesh.HitProxyId
							);
					}
				}
			}
		}
		// Mark dirty if dynamic drawer rendered
		bDirty |= Drawer.IsDirty();
	}
	return bDirty;
}

FGlobalBoundShaderStateRHIRef FDistortionPrimSet::ApplyScreenBoundShaderState;

/** 
* Apply distortion using the accumulated offsets as a fullscreen quad
* @param Context - command context
* @param ViewInfo - current view used to draw items
* @param DPGIndex - current DPG used to draw items
* @param CanvasTransform - default canvas transform used by scene rendering
* @return TRUE if anything was drawn
*/
void FDistortionPrimSet::DrawScreenDistort(FCommandContextRHI* Context,const class FViewInfo* ViewInfo,UINT DPGIndex,const FMatrix& CanvasTransform)
{
	TShaderMapRef<FDistortionApplyScreenVertexShader> VertexShader(GetGlobalShaderMap());
	check(*VertexShader);
	VertexShader->SetParameters(Context,FMatrix::Identity);

	TShaderMapRef<FDistortionApplyScreenPixelShader> PixelShader(GetGlobalShaderMap());
	check(*PixelShader);
	PixelShader->SetParameters(Context,GSceneRenderTargets.GetLightAttenuationTexture(),GSceneRenderTargets.GetSceneColorRawTexture());

	SetGlobalBoundShaderState(Context, ApplyScreenBoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, sizeof(FSimpleElementVertex));

	// set fill/blend state
	RHISetRasterizerState(Context,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	RHISetBlendState(Context,TStaticBlendState<>::GetRHI());

	// Buffer size
	const FLOAT BufferSizeX = GSceneRenderTargets.GetBufferSizeX();
	const FLOAT BufferSizeY = GSceneRenderTargets.GetBufferSizeY();
	// view offset
    const FLOAT X = -1.0f + -GPixelCenterOffset / (FLOAT)ViewInfo->RenderTargetSizeX * +2.0f;
	const FLOAT Y = +1.0f + -GPixelCenterOffset / (FLOAT)ViewInfo->RenderTargetSizeY * -2.0f;
	// view size
	const FLOAT SizeX = +2.0f;
	const FLOAT SizeY = -2.0f;
	// UV map render target to full view
	const FLOAT U = (FLOAT)ViewInfo->RenderTargetX / BufferSizeX;
	const FLOAT V = (FLOAT)ViewInfo->RenderTargetY / BufferSizeY;
	const FLOAT SizeU = (FLOAT)ViewInfo->RenderTargetSizeX / BufferSizeX;
	const FLOAT SizeV = (FLOAT)ViewInfo->RenderTargetSizeY / BufferSizeY;
	
	// quad verts
	void* VertPtr=NULL;
	RHIBeginDrawPrimitiveUP(Context, PT_TriangleFan, 2, 4, sizeof(FSimpleElementVertex), VertPtr);
	check(VertPtr);
	FSimpleElementVertex* Vertices = (FSimpleElementVertex*)VertPtr;
	Vertices[0] = FSimpleElementVertex(FVector4(X,			Y,			0,1),FVector2D(U,			V),			FColor(255,255,255),FHitProxyId());
	Vertices[1] = FSimpleElementVertex(FVector4(X + SizeX,	Y,			0,1),FVector2D(U + SizeU,	V),			FColor(255,255,255),FHitProxyId());		
	Vertices[2] = FSimpleElementVertex(FVector4(X + SizeX,	Y + SizeY,	0,1),FVector2D(U + SizeU,	V + SizeV),	FColor(255,255,255),FHitProxyId());
	Vertices[3] = FSimpleElementVertex(FVector4(X,			Y + SizeY,	0,1),FVector2D(U,			V + SizeV),	FColor(255,255,255),FHitProxyId());	
    // Draw the screen quad 
	RHIEndDrawPrimitiveUP(Context);
}

/**
* Add a new primitive to the list of distortion prims
* @param PrimitiveSceneInfo - primitive info to add.
* @param ViewInfo - used to transform bounds to view space
*/
void FDistortionPrimSet::AddScenePrimitive(FPrimitiveSceneInfo* PrimitivieSceneInfo,const FViewInfo& ViewInfo)
{
	Prims.AddItem(PrimitivieSceneInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FSceneRenderer::RenderDistortion
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** 
 * Renders the scene's distortion 
 */
UBOOL FSceneRenderer::RenderDistortion(UINT DPGIndex)
{
	SCOPED_DRAW_EVENT(EventDistortion)(DEC_SCENE_ITEMS,TEXT("Distortion"));

	UBOOL bRender=FALSE;
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views(ViewIndex);
		const UBOOL bViewHasDistortionViewMeshElements = View.bHasDistortionViewMeshElements & (1<<DPGIndex);
		if( View.DistortionPrimSet[DPGIndex].NumPrims() > 0 || bViewHasDistortionViewMeshElements )
		{
			bRender=TRUE;
			break;
		}
	}

	UBOOL bDirty = FALSE;
	// Render accumulated distortion offsets
	if( bRender )
	{
		// Save the color buffer as a raw surface
		GSceneRenderTargets.SaveSceneColorRaw();

		SCOPED_DRAW_EVENT(EventDistortionAccum)(DEC_SCENE_ITEMS,TEXT("Distortion Accum"));

		GSceneRenderTargets.BeginRenderingDistortionAccumulation();
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

			FViewInfo& View = Views(ViewIndex);
			// viewport to match view size
			RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
			RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

#if XBOX
			// clear offsets to 0
			RHIClear(GlobalContext,TRUE,FLinearColor(0,0,0,0),FALSE,0,FALSE,0);
#else
			// clear offsets to 0, stencil to 0
			RHIClear(GlobalContext,TRUE,FLinearColor(0,0,0,0),FALSE,0,TRUE,0);

			// Set stencil to one.
			RHISetStencilState(GlobalContext,TStaticStencilState<
				TRUE,CF_Always,SO_Keep,SO_Keep,SO_Replace,
				FALSE,CF_Always,SO_Keep,SO_Keep,SO_Keep,
				0xff,0xff,1
				>::GetRHI());
#endif

			// enable depth test but disable depth writes
			RHISetDepthState(GlobalContext,TStaticDepthState<FALSE,CF_LessEqual>::GetRHI());
			// additive blending of offsets
			RHISetBlendState(GlobalContext,TStaticBlendState<BO_Add,BF_One,BF_One,BO_Add,BF_One,BF_One>::GetRHI());
			// draw only distortion meshes to accumulate their offsets
			bDirty |= View.DistortionPrimSet[DPGIndex].DrawAccumulatedOffsets(GlobalContext,&View,DPGIndex);
		}

		GSceneRenderTargets.FinishRenderingDistortionAccumulation();
	}


	if( bDirty )
	{
		SCOPED_DRAW_EVENT(EventDistortionApply)(DEC_SCENE_ITEMS,TEXT("Distortion Apply"));

		GSceneRenderTargets.BeginRenderingSceneColorRaw();

		// Apply distortion as a full-screen pass		
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

			FViewInfo& View = Views(ViewIndex);
			// viewport to match view size
			RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);				
			RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

			// disable depth test & writes
			RHISetDepthState(GlobalContext,TStaticDepthState<FALSE,CF_Always>::GetRHI());
			// opaque blend
			RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());	

#if !XBOX
			//pass if non-zero, so only the pixels that accumulated an offset will be rendered
			RHISetStencilState(GlobalContext,TStaticStencilState<
				TRUE,CF_NotEqual,SO_Keep,SO_Keep,SO_Keep,
				FALSE,CF_Always,SO_Keep,SO_Keep,SO_Keep,
				0xff,0xff,0
				>::GetRHI());
#endif

			// draw the full-screen quad
			View.DistortionPrimSet[DPGIndex].DrawScreenDistort(GlobalContext,&View,DPGIndex,CanvasTransform);
		}

		GSceneRenderTargets.FinishRenderingSceneColor(FALSE);
	}
	else if ( bRender )
	{
		// Restore scene color texture as a raw-surface
		GSceneRenderTargets.RestoreSceneColorRaw();
		bDirty = TRUE;
	}

	//restore default stencil state
	RHISetStencilState(GlobalContext,TStaticStencilState<>::GetRHI());

	return bDirty;
}
