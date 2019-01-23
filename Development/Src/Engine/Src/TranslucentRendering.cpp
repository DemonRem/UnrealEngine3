/*=============================================================================
	TranslucentRendering.cpp: Translucent rendering implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"

/**
* Pixel shader used for combining LDR translucency with scene color when floating point blending isn't supported
*/
class FLDRTranslucencyCombinePixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FLDRTranslucencyCombinePixelShader,Global);

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return Platform == SP_PCD3D_SM2;
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
	}

	/** Default constructor. */
	FLDRTranslucencyCombinePixelShader() {}

public:

	FShaderParameter LDRTranslucencyTextureParameter;
	FSceneTextureShaderParameters SceneTextureParameters;

	/** Initialization constructor. */
	FLDRTranslucencyCombinePixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer.ParameterMap);
		LDRTranslucencyTextureParameter.Bind(Initializer.ParameterMap,TEXT("LDRTranslucencyTexture"),TRUE);
	}

	// FShader interface.
	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << SceneTextureParameters;
		Ar << LDRTranslucencyTextureParameter;
	}
};

IMPLEMENT_SHADER_TYPE(,FLDRTranslucencyCombinePixelShader,TEXT("LDRTranslucencyCombinePixelShader"),TEXT("Main"),SF_Pixel,0,0);

IMPLEMENT_SHADER_TYPE(,FLDRTranslucencyCombineVertexShader,TEXT("LDRTranslucencyCombineVertexShader"),TEXT("Main"),SF_Vertex,0,0);

FGlobalBoundShaderStateRHIRef FSceneRenderer::LDRCombineBoundShaderState;

extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

/** The parameters used to draw a translucent mesh. */
class FDrawTranslucentMeshAction
{
public:

	FCommandContextRHI* Context;
	const FViewInfo& View;
	UBOOL bBackFace;
	UBOOL bPreFog;
	FHitProxyId HitProxyId;

	/** Initialization constructor. */
	FDrawTranslucentMeshAction(
		FCommandContextRHI* InContext,
		const FViewInfo& InView,
		const UBOOL bInBackFace,
		const UBOOL bInPreFog,
		const FHitProxyId InHitProxyId
		):
		Context(InContext),
		View(InView),
		bBackFace(bInBackFace),
		bPreFog(bInPreFog),
		HitProxyId(InHitProxyId)
	{}

	/** Draws the translucent mesh with a specific light-map type, and fog volume type */
	template<typename LightMapPolicyType,typename FogDensityPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData,
		const typename FogDensityPolicyType::ElementDataType& FogDensityElementData
		) const
	{
		const UBOOL bIsLitMaterial = Parameters.LightingModel != MLM_Unlit;

		TBasePassDrawingPolicy<LightMapPolicyType,FogDensityPolicyType> DrawingPolicy(
			Parameters.Mesh.VertexFactory,
			Parameters.Mesh.MaterialRenderProxy,
			LightMapPolicy,
			Parameters.PrimitiveSceneInfo && Parameters.PrimitiveSceneInfo->HasDynamicSkyLighting() && bIsLitMaterial,
			View.Family->ShowFlags & SHOW_ShaderComplexity
			);
		DrawingPolicy.DrawShared(
			Context,
			&View,
			DrawingPolicy.CreateBoundShaderState(Parameters.Mesh.GetDynamicVertexStride())
			);
		DrawingPolicy.SetMeshRenderState(
			Context,
			Parameters.PrimitiveSceneInfo,
			Parameters.Mesh,
			bBackFace,
			typename TBasePassDrawingPolicy<LightMapPolicyType,FogDensityPolicyType>::ElementDataType(
				LightMapElementData,
				FogDensityElementData
				)
			);
		DrawingPolicy.DrawMesh(Context,Parameters.Mesh);
	}
};

/**
 * Render a dynamic mesh using a translucent draw policy
 * @return TRUE if the mesh rendered
 */
UBOOL FTranslucencyDrawingPolicyFactory::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FViewInfo* View,
	ContextType DrawingContext,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId
	)
{
	UBOOL bDirty = FALSE;

	// Determine the mesh's material and blend mode.
	const FMaterial* Material = Mesh.MaterialRenderProxy->GetMaterial();
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Only render translucent materials.
	if(IsTranslucentBlendMode(BlendMode))
	{
		// Decals render pre fog, non-decals render post-fog.
		const UBOOL bIsDecalMaterial = Material->IsDecalMaterial();

		// Handle the fog volume case.
		if (Material->IsUsedWithFogVolumes())
		{
			bDirty = RenderFogVolume(Context, View, Mesh, bBackFace, bPreFog, PrimitiveSceneInfo, HitProxyId);
		}
		else
		{
			UBOOL bDrawMesh = FALSE;

			// If we're drawing before fog, only render decals.
			if ( !bPreFog || bIsDecalMaterial )
			{
				// Draw decals before fog if floating point blending is not supported
				if (!CanBlendWithFPRenderTarget(GRHIShaderPlatform) && bIsDecalMaterial && bPreFog)
				{
					//resolve the scene color surface to the texture so that it can be used for blending in the shader
					GSceneRenderTargets.FinishRenderingSceneColor();
					GSceneRenderTargets.BeginRenderingSceneColor();

					//set states that are reset during the resolve
					RHISetDepthState( Context, TStaticDepthState<FALSE,CF_LessEqual>::GetRHI() );
					RHISetViewport(Context,View->RenderTargetX,View->RenderTargetY,0.0f,View->RenderTargetX + View->RenderTargetSizeX,View->RenderTargetY + View->RenderTargetSizeY,1.0f);

					bDrawMesh = TRUE;
				}
				else
				{
					// Draw non-decals (post-fog), or decals pre-fog, or decals post-fog if the blend policy allows it.
					const UBOOL bMaterialCanBeRenderedPostFog = (BlendMode != BLEND_Modulate);
					if ( !bIsDecalMaterial || ( bPreFog || bMaterialCanBeRenderedPostFog ) )
					{
						bDrawMesh = TRUE;
					}
				}
			}

			if(bDrawMesh)
			{
				ProcessBasePassMesh(
					FProcessBasePassMeshParameters(
						Mesh,
						Material,
						PrimitiveSceneInfo
						),
					FDrawTranslucentMeshAction(
						Context,
						*View,
						bBackFace,
						bPreFog,
						HitProxyId
						)
					);
				bDirty = TRUE;
			}
		}
	}
	return bDirty;
}

/**
 * Render a static mesh using a translucent draw policy
 * @return TRUE if the mesh rendered
 */
UBOOL FTranslucencyDrawingPolicyFactory::DrawStaticMesh(
	FCommandContextRHI* Context,
	const FViewInfo* View,
	ContextType DrawingContext,
	const FStaticMesh& StaticMesh,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId
	)
{
	UBOOL bDirty = FALSE;

	const FMaterial* Material = StaticMesh.MaterialRenderProxy->GetMaterial();
	const UBOOL bIsTwoSided = Material->IsTwoSided();
	const EMaterialLightingModel LightingModel = Material->GetLightingModel();
	const UBOOL bNeedsBackfacePass = (LightingModel != MLM_NonDirectional) && (LightingModel != MLM_Unlit);
	const UINT NumBackfacePasses = (bIsTwoSided && bNeedsBackfacePass) ? 2 : 1;
	for(UINT bBackFace = 0;bBackFace < NumBackfacePasses;bBackFace++)
	{
		bDirty |= DrawDynamicMesh(
			Context,
			View,
			DrawingContext,
			StaticMesh,
			bBackFace,
			bPreFog,
			PrimitiveSceneInfo,
			HitProxyId
			);
	}

	return bDirty;
}

/*-----------------------------------------------------------------------------
FTranslucentPrimSet
-----------------------------------------------------------------------------*/

/**
* Iterate over the sorted list of prims and draw them
* @param Context - command context
* @param ViewInfo - current view used to draw items
* @param DPGIndex - current DPG used to draw items
* @param bPreFog - TRUE if the draw call is occurring before fog has been rendered.
* @return TRUE if anything was drawn
*/
UBOOL FTranslucentPrimSet::Draw(FCommandContextRHI* Context,
		  const FViewInfo* View,
		  UINT DPGIndex,
		  UBOOL bPreFog
		  )
{
	UBOOL bDirty=FALSE;

	// Draw the view's mesh elements with the translucent drawing policy.
	bDirty |= DrawViewElements<FTranslucencyDrawingPolicyFactory>(Context,View,FTranslucencyDrawingPolicyFactory::ContextType(),DPGIndex,bPreFog);

	// Draw normal translucency first, then translucent materials that read from SceneColor.
	for(INT Phase = 0;Phase < 2;Phase++)
	{
		const TArray<FSortedPrim>& PhaseSortedPrimitives =
			Phase == 0 ?
				SortedPrims :
				SortedSceneColorPrims;

		if( PhaseSortedPrimitives.Num() )
		{
			if( Phase == 1 && bDirty )
			{
				// if any translucent prims were rendered then resolve scene color texture
				//@todo.SAS/SZ-Determine why this breaks emitters on PC...
#if CONSOLE
				GSceneRenderTargets.ResolveSceneColor();
#endif
			}

			// For drawing scene prims with dynamic relevance.
			TDynamicPrimitiveDrawer<FTranslucencyDrawingPolicyFactory> Drawer(
				Context,
				View,
				DPGIndex,
				FTranslucencyDrawingPolicyFactory::ContextType(),
				bPreFog
				);

			// Draw sorted scene prims
			for( INT PrimIdx=0; PrimIdx < PhaseSortedPrimitives.Num(); PrimIdx++ )
			{
				FPrimitiveSceneInfo* PrimitiveSceneInfo = PhaseSortedPrimitives(PrimIdx).PrimitiveSceneInfo;

				// Check that the primitive hasn't been occluded.
				if(View->PrimitiveVisibilityMap(PrimitiveSceneInfo->Id))
				{
					const FPrimitiveViewRelevance& ViewRelevance = View->PrimitiveViewRelevanceMap(PrimitiveSceneInfo->Id);

					// Render dynamic scene prim
					if( ViewRelevance.bDynamicRelevance )
					{
						Drawer.SetPrimitive(PrimitiveSceneInfo);
						PrimitiveSceneInfo->Proxy->DrawDynamicElements(
							&Drawer,
							View,
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
								bDirty |= FTranslucencyDrawingPolicyFactory::DrawStaticMesh(
									Context,
									View,
									FTranslucencyDrawingPolicyFactory::ContextType(),
									StaticMesh,
									bPreFog,
									PrimitiveSceneInfo,
									StaticMesh.HitProxyId
									);
							}
						}
					}
				}
			}
			// Mark dirty if dynamic drawer rendered
			bDirty |= Drawer.IsDirty();
		}
	}

	return bDirty;
}

/**
* Add a new primitive to the list of sorted prims
* @param PrimitiveSceneInfo - primitive info to add. Origin of bounds is used for sort.
* @param ViewInfo - used to transform bounds to view space
* @param bUsesSceneColor - primitive samples from scene color
*/
void FTranslucentPrimSet::AddScenePrimitive(FPrimitiveSceneInfo* PrimitiveSceneInfo,const FViewInfo& ViewInfo, UBOOL bUsesSceneColor)
{
	SCOPE_CYCLE_COUNTER(STAT_TranslucencySetupTime);

	FLOAT SortKey=0.f;
	FFogVolumeDensitySceneInfo** FogDensityInfoRef = PrimitiveSceneInfo->Scene->FogVolumes.Find(PrimitiveSceneInfo->Component);
	UBOOL bIsFogVolume = FogDensityInfoRef != NULL;
	if (bIsFogVolume)
	{
		//sort by view space depth + primitive radius, so that intersecting translucent objects are drawn first,
		//which is needed for fogging the translucent object.
		SortKey = ViewInfo.ViewMatrix.TransformFVector(PrimitiveSceneInfo->Bounds.Origin).Z + 0.7f * PrimitiveSceneInfo->Bounds.SphereRadius;
	}
	else
	{
		//sort based on view space depth
		SortKey = ViewInfo.ViewMatrix.TransformFVector(PrimitiveSceneInfo->Bounds.Origin).Z;

		PrimitiveSceneInfo->FogVolumeSceneInfo = NULL;
		FLOAT LargestFogVolumeRadius = 0.0f;
		//find the largest fog volume this translucent object is intersecting with
		for( TDynamicMap<const UPrimitiveComponent*, FFogVolumeDensitySceneInfo*>::TIterator FogVolumeIt(PrimitiveSceneInfo->Scene->FogVolumes); FogVolumeIt; ++FogVolumeIt )
		{
			const UPrimitiveComponent* FogVolumePrimComponent = FogVolumeIt.Key();
			if (FogVolumePrimComponent)
			{
				const FLOAT FogVolumeRadius = FogVolumePrimComponent->Bounds.SphereRadius;
				const FLOAT TranslucentObjectRadius = PrimitiveSceneInfo->Bounds.SphereRadius;
				if (FogVolumeRadius > LargestFogVolumeRadius)
				{
					const FLOAT DistSquared = (FogVolumePrimComponent->Bounds.Origin - PrimitiveSceneInfo->Bounds.Origin).SizeSquared();
					if (DistSquared < FogVolumeRadius * FogVolumeRadius + TranslucentObjectRadius * TranslucentObjectRadius)
					{
						LargestFogVolumeRadius = FogVolumeRadius;
						PrimitiveSceneInfo->FogVolumeSceneInfo = FogVolumeIt.Value();
					}
				}
			}
		}
	}

	if( bUsesSceneColor )
	{
		// add to list of translucent prims that use scene color
		new(SortedSceneColorPrims) FSortedPrim(PrimitiveSceneInfo,SortKey,PrimitiveSceneInfo->TranslucencySortPriority);
	}
	else
	{
		// add to list of translucent prims
		new(SortedPrims) FSortedPrim(PrimitiveSceneInfo,SortKey,PrimitiveSceneInfo->TranslucencySortPriority);
	}
}

/**
* Sort any primitives that were added to the set back-to-front
*/
void FTranslucentPrimSet::SortPrimitives()
{
	SCOPE_CYCLE_COUNTER(STAT_TranslucencySetupTime);
	// sort prims based on depth
	Sort<USE_COMPARE_CONSTREF(FSortedPrim,TranslucentRender)>(&SortedPrims(0),SortedPrims.Num());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FSceneRenderer::RenderTranslucency
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Renders the scene's translucency.
 *
 * @param	DPGIndex	Current DPG used to draw items.
 * @return				TRUE if anything was drawn.
 */
UBOOL FSceneRenderer::RenderTranslucency(UINT DPGIndex)
{
	SCOPED_DRAW_EVENT(EventTranslucent)(DEC_SCENE_ITEMS,TEXT("Translucency"));

	UBOOL bRender=FALSE;
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views(ViewIndex);
		const UBOOL bViewHasTranslucentViewMeshElements = View.bHasTranslucentViewMeshElements & (1<<DPGIndex);
		if( View.TranslucentPrimSet[DPGIndex].NumPrims() > 0 || bViewHasTranslucentViewMeshElements )
		{
			bRender=TRUE;
			break;
		}
	}

	UBOOL bDirty = FALSE;
	if( bRender )
	{

		if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
		{
			// Use the scene color buffer.
			GSceneRenderTargets.BeginRenderingSceneColor();
		}
		else
		{
			// render to the LDR scene color surface
			GSceneRenderTargets.BeginRenderingSceneColorLDR();
			// initialize the surface to (0,0,0,1).  The alpha channel determines how much scene color will be weighted
			// in during the combine, which starts out at 1 so that all scene color will be used.
			RHIClear(GlobalContext,TRUE,FLinearColor(0.0f, 0.0f, 0.0f, 1.0f),FALSE,0.0f,FALSE,0);
		}

		// Enable depth test, disable depth writes.
		RHISetDepthState( GlobalContext, TStaticDepthState<FALSE,CF_LessEqual>::GetRHI() );

		//reset the fog apply stencil index for this frame
		ResetFogVolumeIndex();

		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

			FViewInfo& View = Views(ViewIndex);
			// viewport to match view size
			RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
			RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

			// draw only translucent prims
			bDirty |= View.TranslucentPrimSet[DPGIndex].Draw(GlobalContext,&View,DPGIndex,FALSE);
		}
	}
	return bDirty;
}

/**
* Combines the recently rendered LDR translucency with scene color.
*
* @param	Context	 The RHI command context the primitives are being rendered to.
*/
void FSceneRenderer::CombineLDRTranslucency(FCommandContextRHI* Context)
{
	SCOPED_DRAW_EVENT(EventCombineLDRTranslucency)(DEC_SCENE_ITEMS,TEXT("CombineLDRTranslucency"));

	// Render to the scene color buffer.
	GSceneRenderTargets.BeginRenderingSceneColor();

	TShaderMapRef<FLDRTranslucencyCombineVertexShader> CombineVertexShader(GetGlobalShaderMap());
	TShaderMapRef<FLDRTranslucencyCombinePixelShader> CombinePixelShader(GetGlobalShaderMap());

	SetGlobalBoundShaderState(Context, LDRCombineBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *CombineVertexShader, *CombinePixelShader, sizeof(FFilterVertex));

	//sample the LDR translucency texture
	SetTextureParameter(
		Context,
		CombinePixelShader->GetPixelShader(),
		CombinePixelShader->LDRTranslucencyTextureParameter,
		TStaticSamplerState<SF_Nearest>::GetRHI(),
		GSceneRenderTargets.GetSceneColorLDRTexture()
		);

	RHISetRasterizerState(GlobalContext,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	//opaque, blending will be done in the shader since there is no floating point blending support on this path
	RHISetBlendState(Context,TStaticBlendState<>::GetRHI());

	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		SCOPED_DRAW_EVENT(CombineEventView)(DEC_SCENE_ITEMS,TEXT("CombineView%d"),ViewIndex);

		FViewInfo& View = Views(ViewIndex);

		RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );
		CombinePixelShader->SceneTextureParameters.Set(Context,&View,*CombinePixelShader);

		const UINT SceneColorBufferSizeX = GSceneRenderTargets.GetBufferSizeX();
		const UINT SceneColorBufferSizeY = GSceneRenderTargets.GetBufferSizeY();

		DrawDenormalizedQuad(
			Context,
			View.RenderTargetX,View.RenderTargetY,
			View.RenderTargetSizeX,View.RenderTargetSizeY,
			1,1,
			View.RenderTargetSizeX,View.RenderTargetSizeY,
			SceneColorBufferSizeX,SceneColorBufferSizeY,
			SceneColorBufferSizeX,SceneColorBufferSizeY
			);
	}

	// Resolve the scene color buffer since it has been dirtied.
	GSceneRenderTargets.FinishRenderingSceneColor();

}
