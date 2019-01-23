/*=============================================================================
	EmissiveRendering.cpp: Emissive rendering implementation.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TEmissiveVertexShader<FNoLightMapPolicy>,TEXT("EmissiveVertexShader"),TEXT("Main"),SF_Vertex,0,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TEmissiveVertexShader<FVertexLightMapPolicy>,TEXT("EmissiveVertexShader"),TEXT("Main"),SF_Vertex,VER_LIGHTMAP_PS3_BYTEORDER_FIX,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TEmissiveVertexShader<FLightMapTexturePolicy>,TEXT("EmissiveVertexShader"),TEXT("Main"),SF_Vertex,0,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TEmissivePixelShader<FNoLightMapPolicy>,TEXT("EmissivePixelShader"),TEXT("Main"),SF_Pixel,VER_SKYLIGHT_LOWERHEMISPHERE_SHADER_RECOMPILE,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TEmissivePixelShader<FVertexLightMapPolicy>,TEXT("EmissivePixelShader"),TEXT("Main"),SF_Pixel,VER_SKYLIGHT_LOWERHEMISPHERE_SHADER_RECOMPILE,0);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TEmissivePixelShader<FLightMapTexturePolicy>,TEXT("EmissivePixelShader"),TEXT("Main"),SF_Pixel,VER_SKYLIGHT_LOWERHEMISPHERE_SHADER_RECOMPILE,0);

void FEmissiveDrawingPolicyFactory::AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh,ContextType)
{
	if(!StaticMesh->IsTranslucent() && !StaticMesh->IsDistortion())
	{
		// Check for a cached light-map.
		const UBOOL bIsLitMaterial = StaticMesh->MaterialRenderProxy->GetMaterial()->GetLightingModel() != MLM_Unlit;
		const FLightMapInteraction LightMapInteraction = 
			(StaticMesh->LCI && bIsLitMaterial) ?
				StaticMesh->LCI->GetLightMapInteraction() :
				FLightMapInteraction();

		if(LightMapInteraction.GetType() == LMIT_Vertex)
		{
			// Add the static mesh to the vertex light-map emissive draw list.
			Scene->DPGs[StaticMesh->DepthPriorityGroup].EmissiveVertexLightMapDrawList.AddMesh(
				StaticMesh,
				TEmissiveDrawingPolicy<FVertexLightMapPolicy>::ElementDataType(LightMapInteraction),
				TEmissiveDrawingPolicy<FVertexLightMapPolicy>(
					StaticMesh->VertexFactory,
					StaticMesh->MaterialRenderProxy,
					FVertexLightMapPolicy()
					)
				);
		}
		else if(LightMapInteraction.GetType() == LMIT_Texture)
		{
			// Add the static mesh to the light-map texture emissive draw list.
			Scene->DPGs[StaticMesh->DepthPriorityGroup].EmissiveLightMapTextureDrawList.AddMesh(
				StaticMesh,
				TEmissiveDrawingPolicy<FLightMapTexturePolicy>::ElementDataType(LightMapInteraction),
				TEmissiveDrawingPolicy<FLightMapTexturePolicy>(
					StaticMesh->VertexFactory,
					StaticMesh->MaterialRenderProxy,
					FLightMapTexturePolicy(LightMapInteraction)
					)
				);
		}
		else
		{
			// Add the static mesh to the no light-map emissive draw list.
			Scene->DPGs[StaticMesh->DepthPriorityGroup].EmissiveNoLightMapDrawList.AddMesh(
				StaticMesh,
				TEmissiveDrawingPolicy<FNoLightMapPolicy>::ElementDataType(),
				TEmissiveDrawingPolicy<FNoLightMapPolicy>(
					StaticMesh->VertexFactory,
					StaticMesh->MaterialRenderProxy,
					FNoLightMapPolicy()
					)
				);
		}
	}
}

UBOOL FEmissiveDrawingPolicyFactory::DrawDynamicMesh(
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
	if(!Mesh.IsTranslucent() && !Mesh.IsDistortion())
	{
		// Check for a cached light-map.
		const UBOOL bIsLitMaterial =
			Mesh.MaterialRenderProxy->GetMaterial()->GetLightingModel() != MLM_Unlit &&
			(View->Family->ShowFlags & SHOW_Lighting);

		const FLightMapInteraction LightMapInteraction = 
			(Mesh.LCI && bIsLitMaterial) ?
				Mesh.LCI->GetLightMapInteraction() :
				FLightMapInteraction();

		const UBOOL bRenderShaderComplexity = View->Family->ShowFlags & SHOW_ShaderComplexity;

		if(LightMapInteraction.GetType() == LMIT_Vertex)
		{
			if (bRenderShaderComplexity)
			{
				// Draw the mesh using the vertex light-map.
				TShaderComplexityEmissiveDrawingPolicy<FVertexLightMapPolicy> DrawingPolicy(
					Mesh.VertexFactory,
					Mesh.MaterialRenderProxy,
					FVertexLightMapPolicy()
					);
				DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()));
				DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,LightMapInteraction);
				DrawingPolicy.DrawMesh(Context,Mesh);
			}
			else
			{
				// Draw the mesh using the vertex light-map.
				TEmissiveDrawingPolicy<FVertexLightMapPolicy> DrawingPolicy(
					Mesh.VertexFactory,
					Mesh.MaterialRenderProxy,
					FVertexLightMapPolicy()
					);
				DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()));
				DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,LightMapInteraction);
				DrawingPolicy.DrawMesh(Context,Mesh);
			}
		}
		else if(LightMapInteraction.GetType() == LMIT_Texture)
		{
			if (bRenderShaderComplexity)
			{
				// Draw the mesh using the light-map texture.
				TShaderComplexityEmissiveDrawingPolicy<FLightMapTexturePolicy> DrawingPolicy(
					Mesh.VertexFactory,
					Mesh.MaterialRenderProxy,
					FLightMapTexturePolicy(LightMapInteraction)
					);
				DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()));
				DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,LightMapInteraction);
				DrawingPolicy.DrawMesh(Context,Mesh);
			}
			else
			{
				// Draw the mesh using the light-map texture.
				TEmissiveDrawingPolicy<FLightMapTexturePolicy> DrawingPolicy(
					Mesh.VertexFactory,
					Mesh.MaterialRenderProxy,
					FLightMapTexturePolicy(LightMapInteraction)
					);
				DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()));
				DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,LightMapInteraction);
				DrawingPolicy.DrawMesh(Context,Mesh);
			}
		}
		else
		{
			if (bRenderShaderComplexity)
			{
				// Draw the mesh using no light-map.
				TShaderComplexityEmissiveDrawingPolicy<FNoLightMapPolicy> DrawingPolicy(
					Mesh.VertexFactory,
					Mesh.MaterialRenderProxy,
					FNoLightMapPolicy()
					);
				DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()));
				DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,FNoLightMapPolicy::ElementDataType());
				DrawingPolicy.DrawMesh(Context,Mesh);
			}
			else
			{
				// Draw the mesh using no light-map.
				TEmissiveDrawingPolicy<FNoLightMapPolicy> DrawingPolicy(
					Mesh.VertexFactory,
					Mesh.MaterialRenderProxy,
					FNoLightMapPolicy()
					);
				DrawingPolicy.DrawShared(Context,View,DrawingPolicy.CreateBoundShaderState(Mesh.GetDynamicVertexStride()));
				DrawingPolicy.SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,FNoLightMapPolicy::ElementDataType());
				DrawingPolicy.DrawMesh(Context,Mesh);
			}
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
