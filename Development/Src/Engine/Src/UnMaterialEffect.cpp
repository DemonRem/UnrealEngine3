/*=============================================================================
	UMaterialEffect.cpp: Material post process effect implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "TileRendering.h"
#include "EngineMaterialClasses.h"

IMPLEMENT_CLASS(UMaterialEffect);

/*-----------------------------------------------------------------------------
FMaterialPostProcessSceneProxy
-----------------------------------------------------------------------------*/

/**
* Render-side data and rendering functionality for a UMaterialEffect
*/
class FMaterialPostProcessSceneProxy : public FPostProcessSceneProxy
{
public:
	/** 
	 * Initialization constructor. 
	 * @param InEffect - material post process effect to mirror in this proxy
	 */
	FMaterialPostProcessSceneProxy(const UMaterialEffect* InEffect,const FPostProcessSettings* WorldSettings)
	:	FPostProcessSceneProxy(InEffect)
	{
		UMaterial* Material = InEffect->Material ? InEffect->Material->GetMaterial() : NULL;
		
		// Check whether applied material is not unlit as it's a waste of GPU cycles.
		if( Material && Material->LightingModel != MLM_Unlit )
		{
			UBOOL bFixUpMaterial = FALSE;
		
			if( GIsEditor )
			{
				// Prompt user whether to fix up the material under the hood.
				bFixUpMaterial = appMsgf( AMT_YesNo, TEXT("%s assigned to %s via %s uses lighting mode other than MLM_Unlit. Do you want this changed?"), 
									*Material->GetFullName(),
									*InEffect->GetFullName(),
									*InEffect->Material->GetFullName() );
				// Fix up lighting model, recompile and mark package dirty.
				if( bFixUpMaterial )
				{
					Material->LightingModel = MLM_Unlit;
					Material->PostEditChange(NULL);
					Material->MarkPackageDirty();
				}
			}

			// Material has been fixed up, use it.
			if( bFixUpMaterial )
			{
				MaterialRenderProxy = InEffect->Material->GetRenderProxy(FALSE);
			}
			// Material hasn't been fixed up, use default emissive material and log warning.
			else
			{
				warnf(TEXT("%s assigned to %s via %s uses lighting mode other than MLM_Unlit."),
					*Material->GetFullName(),
					*InEffect->GetFullName(),
					*InEffect->Material->GetFullName() );
				MaterialRenderProxy = GEngine->EmissiveTexturedMaterial->GetRenderProxy(FALSE);
			}
		}
		else
		{
			MaterialRenderProxy = InEffect->Material ? InEffect->Material->GetRenderProxy(FALSE) : GEngine->EmissiveTexturedMaterial->GetRenderProxy(FALSE);
		}

		if(WorldSettings)
		{
			// The material need to be a material instance constant.
			UMaterialInstanceConstant* MaterialInterface = Cast<UMaterialInstanceConstant>(InEffect->Material);
			if( MaterialInterface )
			{
				// Propagate settings.
				MaterialInterface->SetScalarParameterValue( NAME_Desaturation, WorldSettings->Scene_Desaturation );
				const FVector& HighLights	= WorldSettings->Scene_HighLights;
				MaterialInterface->SetVectorParameterValue( NAME_HighLights, FLinearColor( HighLights.X, HighLights.Y, HighLights.Z, 0.f ) );
				const FVector& MidTones	= WorldSettings->Scene_MidTones;
				MaterialInterface->SetVectorParameterValue( NAME_MidTones, FLinearColor( MidTones.X, MidTones.Y, MidTones.Z, 0.f ) );
				const FVector& Shadows		= WorldSettings->Scene_Shadows;
				MaterialInterface->SetVectorParameterValue( NAME_Shadows, FLinearColor( Shadows.X, Shadows.Y, Shadows.Z, 0.f ) );
			}
		}
	}

	/**
	 * Render the post process effect
	 * Called by the rendering thread during scene rendering
	 * @param InDepthPriorityGroup - scene DPG currently being rendered
	 * @param View - current view
	 * @param CanvasTransform - same canvas transform used to render the scene
	 * @return TRUE if anything was rendered
	 */
	UBOOL Render(FCommandContextRHI* Context, UINT InDepthPriorityGroup,FViewInfo& View,const FMatrix& CanvasTransform)
	{
		SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("MaterialEffect"));

		UBOOL bDirty=TRUE;
		const FMaterial* Material = MaterialRenderProxy->GetMaterial();
		check(Material);

		// distortion materials are not handled
		if( Material->IsDistorted() )
		{
			bDirty = FALSE;
			return bDirty;
		}

		if (View.bUseLDRSceneColor) // Using 32-bit (LDR) surface
		{
			if (FinalEffectInGroup) // Final effect in chain, so render to the view's output render target
			{
				RHISetRenderTarget(Context,View.Family->RenderTarget->GetRenderTargetSurface(), FSurfaceRHIRef()); // Render to the final render target
			}
			else
			{
				GSceneRenderTargets.BeginRenderingSceneColorLDR();
			}
		}
		else // Using 64-bit (HDR) surface
		{
			GSceneRenderTargets.BeginRenderingSceneColor(); // Start rendering to the scene color buffer.
		}

		// viewport to match view size
		RHISetViewport(Context,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);

		// Turn off alpha-writes, since we sometimes store the Z-values there.
		RHISetColorWriteMask(Context, CW_RGB);

		FTileRenderer TileRenderer;
		// draw a full-view tile (the TileRenderer uses SizeX, not RenderTargetSizeX)
		check(View.SizeX == View.RenderTargetSizeX);
		TileRenderer.DrawTile(Context, View, MaterialRenderProxy);

		// Turn on alpha-writes again.
		RHISetColorWriteMask(Context, CW_RGBA);

		// Resolve the scene color buffer.  Note that nothing needs to be done if we are writing directly to the view's render target
		if (View.bUseLDRSceneColor)
		{
			if (!FinalEffectInGroup)
			{
				GSceneRenderTargets.FinishRenderingSceneColorLDR();
			}
		}
		else
		{
			GSceneRenderTargets.FinishRenderingSceneColor();
		}

		return bDirty;
	}

private:
	/** Material instance used by the effect */
	const FMaterialRenderProxy* MaterialRenderProxy;
};

/*-----------------------------------------------------------------------------
UMaterialEffect
-----------------------------------------------------------------------------*/

/**
* Creates a proxy to represent the render info for a post process effect
* @return The proxy object.
*/
FPostProcessSceneProxy* UMaterialEffect::CreateSceneProxy(const FPostProcessSettings* WorldSettings)
{
	if( Material && (!WorldSettings || WorldSettings->bEnableSceneEffect))
	{
		return new FMaterialPostProcessSceneProxy(this,WorldSettings);
	}
	else
	{
		return NULL;
	}
}
