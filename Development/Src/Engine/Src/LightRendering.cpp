/*=============================================================================
	LightRendering.cpp: Light rendering implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/**
 * Helper function to encapsulate the logic for whether a light should be rendered,
 * which is needed in more than one place.
 * 
 */
static UBOOL ShouldRenderLight(const FVisibleLightInfo& VisibleLightInfo, UINT DPGIndex)
{
	// only render the light if it has visible primitives and is in the view frustum,
	// since only lights whose volumes are visible will contribute to the visible primitives.
	return VisibleLightInfo.DPGInfo[DPGIndex].bHasVisibleLitPrimitives && VisibleLightInfo.bInViewFrustum;
}

FLightSceneInfo::FLightSceneInfo(const ULightComponent* Component)
	: LightComponent(Component)
	, LightGuid(Component->LightGuid)
	, LightmapGuid(Component->LightmapGuid)
	, WorldToLight(Component->WorldToLight)
	, LightToWorld(Component->LightToWorld)
	, Position(Component->GetPosition())
	, Color(FLinearColor(Component->LightColor) * Component->Brightness)
	, LightingChannels(Component->LightingChannels)
	, StaticPrimitiveList(NULL)
	, DynamicPrimitiveList(NULL)
	, NumShadowVolumeInteractions(0)
	, Id(INDEX_NONE)
	, bProjectedShadows(Component->HasProjectedShadowing())
	, bStaticLighting(Component->HasStaticShadowing())
	, bStaticShadowing(Component->HasStaticShadowing())
	, bCastDynamicShadow(Component->CastShadows && Component->CastDynamicShadows)
	, bCastStaticShadow(Component->CastShadows && Component->CastStaticShadows)
	, bOnlyAffectSameAndSpecifiedLevels(Component->bOnlyAffectSameAndSpecifiedLevels)
	, bUseVolumes(Component->bUseVolumes)
	, bAffectsDefaultLightEnvironment(Component->bAffectsDefaultLightEnvironment)
	, LightType(Component->GetLightType())
	, LightShadowMode(Component->LightShadowMode)
	, ShadowProjectionTechnique(Component->ShadowProjectionTechnique)
	, ShadowFilterQuality(Component->ShadowFilterQuality)
	, MinShadowResolution(Component->MinShadowResolution)
	, MaxShadowResolution(Component->MaxShadowResolution)
	, LevelName(Component->GetOutermost()->GetFName())
	, OtherLevelsToAffect(Component->OtherLevelsToAffect)
	, ExclusionConvexVolumes(Component->ExclusionConvexVolumes)
	, InclusionConvexVolumes(Component->InclusionConvexVolumes)
	, ModShadowColor(Component->ModShadowColor)
	, ModShadowFadeoutTime(Component->ModShadowFadeoutTime)
	, ModShadowFadeoutExponent(Component->ModShadowFadeoutExponent)
#if !FINAL_RELEASE
	, LightComponentStr(Component->GetOwner() ? Component->GetOwner()->GetName() : Component->GetName())
#endif
{
	if ( Component->Function && Component->Function->SourceMaterial && Component->Function->SourceMaterial->GetMaterial()->bUsedAsLightFunction )
	{
		LightFunctionScale = Component->Function->Scale;
		LightFunction = Component->Function->SourceMaterial->GetRenderProxy(FALSE);
	}
	else
	{
		LightFunction = NULL;
	}
}

void FLightSceneInfo::Detach()
{
	check(IsInRenderingThread());

	while(StaticPrimitiveList)
	{
		delete StaticPrimitiveList;
	};
	while(DynamicPrimitiveList)
	{
		delete DynamicPrimitiveList;
	};
	NumShadowVolumeInteractions = 0;
}

UBOOL FLightSceneInfo::AffectsLightEnvironment(const FLightEnvironmentSceneInfo* LightEnvironmentSceneInfo) const
{
	if(!LightEnvironmentSceneInfo && !bAffectsDefaultLightEnvironment)
	{
		return FALSE;
	}
	
	if(LightEnvironmentSceneInfo && !LightEnvironmentSceneInfo->Lights.Find(LightComponent))
	{
		return FALSE;
	}

	return TRUE;
}

UBOOL FLightSceneInfo::AffectsPrimitive(const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo) const
{
	// Prefetch the full primitive scene info while culling the primitive by bounds.
	PREFETCH(CompactPrimitiveSceneInfo.PrimitiveSceneInfo);

	// Check against the bounds first, since the contents of PrimitiveSceneInfo may not be in the cache yet.
	if(!AffectsBounds(CompactPrimitiveSceneInfo.Bounds))
	{
		return FALSE;
	}

	if(!CompactPrimitiveSceneInfo.bAcceptsLights)
	{
		return FALSE;
	}

	const FPrimitiveSceneInfo* PrimitiveSceneInfo = CompactPrimitiveSceneInfo.PrimitiveSceneInfo;

	if(!AffectsLightEnvironment(PrimitiveSceneInfo->LightEnvironmentSceneInfo))
	{
		return FALSE;
	}

	if( !LightingChannels.OverlapsWith( PrimitiveSceneInfo->LightingChannels ) )
	{
		return FALSE;
	}

	if( !PrimitiveSceneInfo->bAcceptsDynamicLights && !bStaticShadowing )
	{
		return FALSE;
	}

	// Check whether the package the primitive resides in is set to have self contained lighting.
	if(PrimitiveSceneInfo->bSelfContainedLighting && bStaticShadowing)
	{
		// Only allow interaction in the case of self contained lighting if both light and primitive are in the 
		// the same package/ level, aka share an outermost.
		if(LevelName != PrimitiveSceneInfo->LevelName)
		{
			return FALSE;
		}
	}

	if( bOnlyAffectSameAndSpecifiedLevels )
	{
		// Retrieve the FName of the light's and primtive's level.
		FName LightLevelName			= LevelName;
		FName PrimitiveLevelName		= PrimitiveSceneInfo->LevelName;
		UBOOL bShouldAffectPrimitive	= FALSE;

		// Check whether the light's level matches the primitives.
		if( LightLevelName == PrimitiveLevelName )
		{
			bShouldAffectPrimitive = TRUE;
		}
		// Check whether the primitve's level is in the OtherLevelsToAffect array.
		else
		{
			// Iterate over all levels and look for match.
			for( INT LevelIndex=0; LevelIndex<OtherLevelsToAffect.Num(); LevelIndex++ )
			{
				const FName& OtherLevelName = OtherLevelsToAffect(LevelIndex);
				if( OtherLevelName == PrimitiveLevelName )
				{
					// Match found, early out.
					bShouldAffectPrimitive = TRUE;
					break;
				}
			}
		}

		// Light should not affect primitive, early out.
		if( !bShouldAffectPrimitive )
		{
			return FALSE;
		}
	}

	// Use inclusion/ exclusion volumes to determine whether primitive is affected by light.
	if( bUseVolumes )
	{
		// Exclusion volumes have higher priority. Return FALSE if primitive intersects or is
		// encompassed by any exclusion volume.
		for( INT VolumeIndex=0; VolumeIndex<ExclusionConvexVolumes.Num(); VolumeIndex++ )
		{
			const FConvexVolume& ExclusionConvexVolume	= ExclusionConvexVolumes(VolumeIndex);
			if( ExclusionConvexVolume.IntersectBox( PrimitiveSceneInfo->Bounds.Origin, PrimitiveSceneInfo->Bounds.BoxExtent ) )
			{
				return FALSE;	
			}
		}

		// Check whether primitive intersects or is encompassed by any inclusion volume.
		UBOOL bIntersectsInclusionVolume = FALSE;
		for( INT VolumeIndex=0; VolumeIndex<InclusionConvexVolumes.Num(); VolumeIndex++ )
		{
			const FConvexVolume& InclusionConvexVolume	= InclusionConvexVolumes(VolumeIndex);
			if( InclusionConvexVolume.IntersectBox( PrimitiveSceneInfo->Bounds.Origin, PrimitiveSceneInfo->Bounds.BoxExtent ) )
			{
				bIntersectsInclusionVolume = TRUE;
				break;
			}
		}
		// No interaction unless there is intersection or there are no inclusion volumes, in which case
		// we assume that the whole world is included unless explicitly excluded.
		if( !bIntersectsInclusionVolume && InclusionConvexVolumes.Num() )
		{
			return FALSE;
		}
	}

	return TRUE;
}

ELightInteractionType FMeshLightingDrawingPolicyFactory::AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh,FLightSceneInfo* Light)
{
	if(!StaticMesh->IsTranslucent() && !StaticMesh->IsDistortion())
	{
		// Don't draw the light on the mesh if it's unlit.
		const FMaterial* const Material = StaticMesh->MaterialRenderProxy->GetMaterial();
		if(Material->GetLightingModel() != MLM_Unlit)
		{
			return Light->GetDPGInfo(StaticMesh->DepthPriorityGroup)->AttachStaticMesh(Light,StaticMesh);
		}
	}

	return LIT_CachedIrrelevant;
}

UBOOL FMeshLightingDrawingPolicyFactory::DrawDynamicMesh(
	FCommandContextRHI* Context,
	const FSceneView* View,
	const FLightSceneInfo* Light,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId
	)
{
	if(!Mesh.IsTranslucent() && !Mesh.IsDistortion())
	{
		// Don't draw the light on the mesh if it's unlit.
		const FMaterial* const Material = Mesh.MaterialRenderProxy->GetMaterial();
		if(!IsTranslucentBlendMode(Material->GetBlendMode()) && Material->GetLightingModel() != MLM_Unlit)
		{
			// Draw the light's effect on the primitive.
			return Light->GetDPGInfo(Mesh.DepthPriorityGroup)->DrawDynamicMesh(
				Context,
				View,
				Light,
				Mesh,
				bBackFace,
				bPreFog,
				PrimitiveSceneInfo,
				HitProxyId
				);
		}
	}

	return FALSE;
}


UBOOL FSceneRenderer::RenderLights(UINT DPGIndex)
{
	UBOOL bSceneColorDirty = FALSE;
	UBOOL bStencilBufferDirty = FALSE;	// The stencil buffer should've been cleared to 0 already

	// Draw each light.
	for(TSparseArray<FLightSceneInfo*>::TConstIterator LightIt(Scene->Lights);LightIt;++LightIt)
	{
		const FLightSceneInfo* LightSceneInfo = *LightIt;
		const INT LightId = LightIt.GetIndex();

		const UBOOL bIsLightBlack =
			Square(LightSceneInfo->Color.R) < DELTA &&
			Square(LightSceneInfo->Color.G) < DELTA &&
			Square(LightSceneInfo->Color.B) < DELTA;

		// Nothing to do for black lights as modulated shadows are rendered independently.
		if( bIsLightBlack )
		{
			continue;
		}

		// Check if the light is visible in any of the views.
		UBOOL bLightIsVisibleInAnyView = FALSE;
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			if (ShouldRenderLight(Views(ViewIndex).VisibleLightInfos(LightId), DPGIndex))
			{
				bLightIsVisibleInAnyView = TRUE;
				break;
			}
		}

		if(bLightIsVisibleInAnyView)
		{
			UBOOL bUseAttenuationBuffer = FALSE;
			UBOOL bUseStencilBuffer = FALSE;
			UBOOL bSavedRawSceneColor = FALSE;

			SCOPED_DRAW_EVENT(EventLightPass)(DEC_SCENE_ITEMS,TEXT("Light Pass %s"),LightSceneInfo->GetLightName());

			// Only clear the stencil buffer if it has been written to since the last clear.
			if(bStencilBufferDirty)
			{
				GSceneRenderTargets.BeginRenderingShadowVolumes();
				RHIClear(GlobalContext,FALSE,FLinearColor::Black,FALSE,0,TRUE,0);
				GSceneRenderTargets.FinishRenderingShadowVolumes();

				bStencilBufferDirty = FALSE;
			}

			// Check for a need to use the attenuation buffer
			const UBOOL bDrawShadows =	(ViewFamily.ShowFlags & SHOW_DynamicShadows) 
									&&	GSystemSettings.bAllowDynamicShadows
									&&	LightSceneInfo->LightShadowMode != LightShadow_Modulate;
			if(bDrawShadows)
			{
				// Render non-modulated projected shadows to the attenuation buffer.
				bUseAttenuationBuffer |= CheckForProjectedShadows( LightSceneInfo, DPGIndex );

				// Render shadow volumes to the scene depth stencil buffer.
				bUseStencilBuffer |= CheckForShadowVolumes( LightSceneInfo, DPGIndex );
			}

			// Render light function to the attenuation buffer.
			bUseAttenuationBuffer |= CheckForLightFunction( LightSceneInfo, DPGIndex );

			if (bUseAttenuationBuffer)
			{
				if (bSceneColorDirty)
				{
					// Save the color buffer
					GSceneRenderTargets.SaveSceneColorRaw();
					bSavedRawSceneColor = TRUE;
				}

				// Clear the light attenuation surface to white.
				GSceneRenderTargets.BeginRenderingLightAttenuation();
				RHIClear(GlobalContext,TRUE,FLinearColor::White,FALSE,0,FALSE,0);
				
				if(bDrawShadows)
				{
					// Render non-modulated projected shadows to the attenuation buffer.
					RenderProjectedShadows( LightSceneInfo, DPGIndex );
				}
			}

			if (bUseStencilBuffer && bDrawShadows)
			{
				// Render shadow volumes to the scene depth stencil buffer.
				bStencilBufferDirty |= RenderShadowVolumes( LightSceneInfo, DPGIndex );
			}
	
			if (bUseAttenuationBuffer)
			{
				// Render light function to the attenuation buffer.
				RenderLightFunction( LightSceneInfo, DPGIndex );

				// Resolve light attenuation buffer
				GSceneRenderTargets.FinishRenderingLightAttenuation();
			}

			GSceneRenderTargets.SetLightAttenuationMode(bUseAttenuationBuffer);

			// Rendering the light pass to the scene color buffer.

			if( bSavedRawSceneColor )
			{
				GSceneRenderTargets.RestoreSceneColorRaw();
				GSceneRenderTargets.BeginRenderingSceneColor(FALSE);
			}
			else
			{
				GSceneRenderTargets.BeginRenderingSceneColor(bUseAttenuationBuffer);
			}

			// Render the light to the scene color buffer, conditionally using the attenuation buffer or a 1x1 white texture as input 
			bSceneColorDirty |= RenderLight( LightSceneInfo, DPGIndex );

			// Do not resolve to scene color texture, this is done lazily,
			// unless the light contribution was manually blended, in which case the next lighting pass will need 
			// this pass's lighting in the scene color texture.
			GSceneRenderTargets.FinishRenderingSceneColor(!CanBlendWithFPRenderTarget(GRHIShaderPlatform));
			
		}
	}

	// Restore the default mode
	GSceneRenderTargets.SetLightAttenuationMode(TRUE);

	return bSceneColorDirty;
}

/**
 * Used by RenderLights to render a light to the scene color buffer.
 *
 * @param LightSceneInfo Represents the current light
 * @param LightIndex The light's index into FScene::Lights
 * @return TRUE if anything got rendered
 */
UBOOL FSceneRenderer::RenderLight( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex )
{
	SCOPE_CYCLE_COUNTER(STAT_LightingDrawTime);
	SCOPED_DRAW_EVENT(EventRenderLight)(DEC_SCENE_ITEMS,TEXT("Light"));

#if XBOX
	// Add an additional -0.0001f depth bias, to avoid z-fighting.
	// The Xbox shader compiler can generate slightly different vertex positions.
	extern FLOAT GDepthBiasOffset;
	GDepthBiasOffset = -0.0001f;
#endif

	UBOOL bDirty = FALSE;
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

		const FViewInfo& View = Views(ViewIndex);
		const FVisibleLightInfo& VisibleLightInfo = View.VisibleLightInfos(LightSceneInfo->Id);

		if( ShouldRenderLight(VisibleLightInfo, DPGIndex) )
		{
			// Set the device viewport for the view.
			RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
			RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

			// Set the light's scissor rectangle.
			LightSceneInfo->SetScissorRect(GlobalContext,&View);

			if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
			{
				// Additive blending, depth tests, no depth writes.
				RHISetBlendState(GlobalContext,TStaticBlendState<BO_Add,BF_One,BF_One,BO_Add,BF_Zero,BF_One>::GetRHI());
			}
			else
			{
				// Blending will be done in the lighting shader
				RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());
			}
			
			RHISetDepthState(GlobalContext,TStaticDepthState<FALSE,CF_LessEqual>::GetRHI());
			RHISetStencilState(GlobalContext,TStaticStencilState<TRUE,CF_Equal>::GetRHI());

			// Draw the light's effect on static meshes.
			bDirty |= LightSceneInfo->GetDPGInfo(DPGIndex)->DrawStaticMeshes(GlobalContext,&View,View.StaticMeshVisibilityMap);

			// Draw the light's effect on dynamic meshes.
			{
				TDynamicPrimitiveDrawer<FMeshLightingDrawingPolicyFactory> Drawer(GlobalContext,&View,DPGIndex,LightSceneInfo,TRUE);
				for(INT PrimitiveIndex = 0;PrimitiveIndex < VisibleLightInfo.DPGInfo[DPGIndex].VisibleDynamicLitPrimitives.Num();PrimitiveIndex++)
				{
					const FPrimitiveSceneInfo* PrimitiveSceneInfo = VisibleLightInfo.DPGInfo[DPGIndex].VisibleDynamicLitPrimitives(PrimitiveIndex);
					if(View.PrimitiveVisibilityMap(PrimitiveSceneInfo->Id))
					{
						Drawer.SetPrimitive(PrimitiveSceneInfo);
						PrimitiveSceneInfo->Proxy->DrawDynamicElements(&Drawer,&View,DPGIndex);
					}
				}

				// Render decals for visible primitives affected by this light.
				for(INT PrimitiveIndex = 0;PrimitiveIndex < VisibleLightInfo.DPGInfo[DPGIndex].VisibleLitDecalPrimitives.Num();PrimitiveIndex++)
				{
					const FPrimitiveSceneInfo* PrimitiveSceneInfo = VisibleLightInfo.DPGInfo[DPGIndex].VisibleLitDecalPrimitives(PrimitiveIndex);
					if(View.PrimitiveVisibilityMap(PrimitiveSceneInfo->Id))
					{
						Drawer.SetPrimitive(PrimitiveSceneInfo);
						PrimitiveSceneInfo->Proxy->DrawLitDecalElements(GlobalContext,&Drawer,&View,DPGIndex,TRUE);
					}
				}
				bDirty |= Drawer.IsDirty();
			}

			// Reset the scissor rectangle and stencil state.
			RHISetScissorRect(GlobalContext,FALSE,0,0,0,0);
			RHISetDepthBoundsTest(GlobalContext, FALSE, 0.0f, 1.0f);
			RHISetStencilState(GlobalContext,TStaticStencilState<>::GetRHI());
		}
	}
#if XBOX
	extern FLOAT GDepthBiasOffset;
	GDepthBiasOffset = 0.0f;
#endif
	return bDirty;
}

UBOOL FSceneRenderer::RenderModulatedShadows(UINT DPGIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_ModulatedShadowDrawTime);
	SCOPED_DRAW_EVENT(EventRenderModShadow)(DEC_SCENE_ITEMS,TEXT("ModShadow"));

	UBOOL bSceneColorDirty = FALSE;
	
	// We just assume that the stencil buffer was left dirty by RenderLights.
	UBOOL bStencilBufferDirty = TRUE;

	// Draw each light.
	for(INT LightIndex=0; LightIndex<VisibleShadowCastingLightInfos.Num(); LightIndex++ )
	{
		const FLightSceneInfo* LightSceneInfo = VisibleShadowCastingLightInfos(LightIndex);
		
		// Only look at lights using modulated shadows.
		if( LightSceneInfo->LightShadowMode == LightShadow_Modulate )
		{
			UBOOL bUseStencilBuffer = CheckForShadowVolumes( LightSceneInfo, DPGIndex );

			// Only clear the stencil buffer if it has been written to since the last clear.
			if(bStencilBufferDirty && bUseStencilBuffer)
			{
				GSceneRenderTargets.BeginRenderingShadowVolumes();
				RHIClear(GlobalContext,FALSE,FLinearColor::Black,FALSE,0,TRUE,0);
				GSceneRenderTargets.FinishRenderingShadowVolumes();

				bStencilBufferDirty = FALSE;
			}

			// Render modulated projected shadows to scene color buffer.
			GSceneRenderTargets.BeginRenderingSceneColor(FALSE);
			bSceneColorDirty |= RenderProjectedShadows( LightSceneInfo, DPGIndex );
			GSceneRenderTargets.FinishRenderingSceneColor(FALSE);

			// Modulate the attenuated shadow volumes with the scene color buffer.
			if(bUseStencilBuffer)
			{
				// Render shadow volumes to the scene depth stencil buffer.
				bStencilBufferDirty |= RenderShadowVolumes(LightSceneInfo,DPGIndex);

				//Set Scene Color as the render target even if no shadow volumes were rendered, 
				//because the current color buffer may have been set to something else in RenderShadowVolumes
				GSceneRenderTargets.BeginRenderingSceneColor(FALSE);

				if(bStencilBufferDirty)
				{
					// Render attenuated shadows to light attenuation target
					bSceneColorDirty |= AttenuateShadowVolumes( LightSceneInfo );
					GSceneRenderTargets.FinishRenderingSceneColor(FALSE);
				}
			}
		}
	}	
	return bSceneColorDirty;
}
