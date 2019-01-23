/*=============================================================================
	DecalRendering.cpp: High-level decal rendering implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/** 
 * Iterates over the sorted list of sorted decal prims and draws them.
 *
 * @param	Context							Command context.
 * @param	View							The Current view used to draw items.
 * @param	DPGIndex						The current DPG used to draw items.
 * @param	bTranslucentReceiverPass		TRUE during the decal pass for translucent receivers, FALSE for opaque receivers.
 * @param	bPreFog							TRUE if the draw call is occurring before fog has been rendered.
 * @return									TRUE if anything was drawn, FALSE otherwise.
 */
UBOOL FDecalPrimSet::Draw(FCommandContextRHI* Context,
						  const FViewInfo* View,
						  UINT DPGIndex,
						  UBOOL bTranslucentReceiverPass,
						  UBOOL bPreFog)
{
	UBOOL bDirty = FALSE;
	if( Prims.Num() )
	{
		TDynamicPrimitiveDrawer<FBasePassOpaqueDrawingPolicyFactory> BasePassDrawer(
			Context,
			View,
			DPGIndex,
			FBasePassOpaqueDrawingPolicyFactory::ContextType(),
			bPreFog
			);

		TDynamicPrimitiveDrawer<FTranslucencyDrawingPolicyFactory> TranslucencyDrawer(
			Context,
			View,
			DPGIndex,
			FTranslucencyDrawingPolicyFactory::ContextType(),
			bPreFog
			);

		// Draw sorted scene prims
		for( INT PrimIdx = 0 ; PrimIdx < Prims.Num() ; ++PrimIdx )
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = Prims(PrimIdx);

			// Check that the primitive hasn't been occluded.
			if( View->PrimitiveVisibilityMap( PrimitiveSceneInfo->Id ) )
			{
				const UBOOL bHasTranslucentRelevance = View->PrimitiveViewRelevanceMap(PrimitiveSceneInfo->Id).bTranslucentRelevance;
				if((bTranslucentReceiverPass && bHasTranslucentRelevance) || (!bTranslucentReceiverPass && !bHasTranslucentRelevance))
				{
					BasePassDrawer.SetPrimitive( PrimitiveSceneInfo );
					TranslucencyDrawer.SetPrimitive( PrimitiveSceneInfo );

					PrimitiveSceneInfo->Proxy->DrawDecalElements(
						Context,
						&BasePassDrawer,
						&TranslucencyDrawer,
						View,
						DPGIndex,
						bTranslucentReceiverPass
						);
				}
			}
		}

		// Mark dirty if something was rendered.
		bDirty |= BasePassDrawer.IsDirty();
		bDirty |= TranslucencyDrawer.IsDirty();
	}
	return bDirty;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FSceneRenderer::RenderTranslucentDecals
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** 
 * Renders the scene's decals.
 *
 * @param	DPGIndex					Current DPG used to draw items.
 * @param	bTranslucentReceiverPass	TRUE during the decal pass for translucent receivers, FALSE for opaque receivers.
 * @param	bPreFog						TRUE if the draw call is occurring before fog has been rendered.
 * @return								TRUE if anything was drawn.
 */
UBOOL FSceneRenderer::RenderDecals(UINT DPGIndex, UBOOL bTranslucentReceiverPass, UBOOL bPreFog)
{
	SCOPED_DRAW_EVENT(EventDecals)(DEC_SCENE_ITEMS,TEXT("RenderDecals"));

	UBOOL bRenderDecals = FALSE;
	for( INT ViewIndex = 0 ; ViewIndex < Views.Num() ; ++ViewIndex )
	{
		const FViewInfo& View = Views(ViewIndex);
		if( View.DecalPrimSet[DPGIndex].NumPrims() > 0 )
		{
			bRenderDecals = TRUE;
			break;
		}
	}

	UBOOL bDirty = FALSE;
	if( bRenderDecals )
	{
		RHISetDepthState( GlobalContext, TStaticDepthState<FALSE,CF_LessEqual>::GetRHI() );

		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);		

			// Set the viewport to match the view size.
			FViewInfo& View = Views(ViewIndex);
			RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
			RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

			// Render the decals.
			bDirty |= View.DecalPrimSet[DPGIndex].Draw( GlobalContext, &View, DPGIndex, bTranslucentReceiverPass, bPreFog );
		}
	}
	return bDirty;
}
