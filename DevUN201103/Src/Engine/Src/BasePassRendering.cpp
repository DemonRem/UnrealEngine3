/*=============================================================================
	BasePassRendering.cpp: Base pass rendering implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/** Whether to render dominant lights in the base pass, which is significantly faster than using an extra light pass. */
#if XBOX || PS3
    // Currently only enabled on Xbox and PS3, PC SM3 doesn't have enough interpolators.
	const UBOOL GOnePassDominantLight = TRUE;
#else 
	const UBOOL GOnePassDominantLight = FALSE;
#endif

/** Whether to use deferred shading, where the base pass outputs G buffer attributes, and lighting passes fetch these attributes and do shading based on them. */
UBOOL GAllowDeferredShading = TRUE;

/** Returns TRUE if the given material and primitive can be lit in a deferred pass. */
UBOOL MeshSupportsDeferredLighting(const FMaterial* Material, const FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	const UBOOL bSupportsDeferredLighting = 
		// Only Phong supported for now
		Material->GetLightingModel() == MLM_Phong
		// Deferred passes don't output the MRT values that subsurface scattering needs
		&& !Material->HasSubsurfaceScattering()
		// Only render deferred if any of the channels supported by the channel mask are set
		&& PrimitiveSceneInfo->LightingChannels.GetDeferredShadingChannelMask() > 0;

	return bSupportsDeferredLighting;
}

/** Whether to replace lightmap textures with solid colors to visualize the mip-levels. */
UBOOL GVisualizeMipLevels = FALSE;

#if WITH_D3D11_TESSELLATION
// Typedef is necessary because the C preprocessor thinks the comma in the template parameter list is a comma in the macro parameter list.
// BasePass Vertex Shader needs to include hull and domain shaders for tessellation, these only compile for D3D11
#define IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,FogDensityPolicyType) \
	typedef TBasePassVertexShader<LightMapPolicyType,FogDensityPolicyType> TBasePassVertexShader##LightMapPolicyType##FogDensityPolicyType; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassVertexShader##LightMapPolicyType##FogDensityPolicyType,TEXT("BasePassVertexShader"),TEXT("Main"),SF_Vertex,0,0); \
	typedef TBasePassHullShader<LightMapPolicyType,FogDensityPolicyType> TBasePassHullShader##LightMapPolicyType##FogDensityPolicyType; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassHullShader##LightMapPolicyType##FogDensityPolicyType,TEXT("BasePassTessellationShaders"),TEXT("MainHull"),SF_Hull,0,0); \
	typedef TBasePassDomainShader<LightMapPolicyType,FogDensityPolicyType> TBasePassDomainShader##LightMapPolicyType##FogDensityPolicyType; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassDomainShader##LightMapPolicyType##FogDensityPolicyType,TEXT("BasePassTessellationShaders"),TEXT("MainDomain"),SF_Domain,0,0); 
#else

#define IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,FogDensityPolicyType) \
	typedef TBasePassVertexShader<LightMapPolicyType,FogDensityPolicyType> TBasePassVertexShader##LightMapPolicyType##FogDensityPolicyType; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassVertexShader##LightMapPolicyType##FogDensityPolicyType,TEXT("BasePassVertexShader"),TEXT("Main"),SF_Vertex,0,0); 
#endif

#define IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,bEnableSkyLight,SkyLightShaderName) \
	typedef TBasePassPixelShader<LightMapPolicyType,bEnableSkyLight> TBasePassPixelShader##LightMapPolicyType##SkyLightShaderName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TBasePassPixelShader##LightMapPolicyType##SkyLightShaderName,TEXT("BasePassPixelShader"),TEXT("Main"),SF_Pixel,VER_FIXED_TRANSLUCENT_SHADOW_FILTERING,0);

// Implement a vertex shader for each supported combination of affecting fog primitives
// These are for forward per-vertex fogging of translucency, opaque materials will always use FNoDensityPolicy
#define IMPLEMENT_BASEPASS_LIGHTMAPPED_VERTEXONLY_TYPE(LightMapPolicyType) \
	IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,FNoDensityPolicy); \
	IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,FConstantDensityPolicy); \
	IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,FLinearHalfspaceDensityPolicy); \
	IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,FSphereDensityPolicy); \
	IMPLEMENT_BASEPASS_VERTEXSHADER_TYPE(LightMapPolicyType,FConeDensityPolicy);

// Implement a pixel shader type for skylights and one without, and one vertex shader that will be shared between them
#define IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(LightMapPolicyType) \
	IMPLEMENT_BASEPASS_LIGHTMAPPED_VERTEXONLY_TYPE(LightMapPolicyType) \
	IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,FALSE,NoSkyLight); \
	IMPLEMENT_BASEPASS_PIXELSHADER_TYPE(LightMapPolicyType,TRUE,SkyLight);

// Implement shader types per lightmap policy
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FNoLightMapPolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FDirectionalVertexLightMapPolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FSimpleVertexLightMapPolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FDirectionalLightMapTexturePolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FSimpleLightMapTexturePolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FDirectionalLightLightMapPolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FDynamicallyShadowedMultiTypeLightLightMapPolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FSHLightAndMultiTypeLightMapPolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FSHLightLightMapPolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FShadowedDynamicLightDirectionalVertexLightMapPolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FShadowedDynamicLightDirectionalLightMapTexturePolicy); 
IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE(FDistanceFieldShadowedDynamicLightDirectionalLightMapTexturePolicy);

/** The action used to draw a base pass static mesh element. */
class FDrawBasePassStaticMeshAction
{
public:

	FScene* Scene;
	FStaticMesh* StaticMesh;

	/** Initialization constructor. */
	FDrawBasePassStaticMeshAction(FScene* InScene,FStaticMesh* InStaticMesh):
		Scene(InScene),
		StaticMesh(InStaticMesh)
	{}

	ESceneDepthPriorityGroup GetDPG(const FProcessBasePassMeshParameters& Parameters) const
	{
		return (ESceneDepthPriorityGroup)Parameters.PrimitiveSceneInfo->Proxy->GetStaticDepthPriorityGroup();
	}

	UBOOL ShouldReceiveDominantShadows(const FProcessBasePassMeshParameters& Parameters) const
	{
		// Only allow receiving dynamic shadows from the dominant light if bUseAsOccluder == TRUE, 
		// Since dominant light shadows are projected using depths from the depth only pass, which bUseAsOccluder controls.
		return GOnePassDominantLight && Parameters.PrimitiveSceneInfo->bAcceptsDynamicDominantLightShadows && Parameters.PrimitiveSceneInfo->bUseAsOccluder;
	}

	UBOOL ShouldOverrideDynamicShadowsOnTranslucency(const FProcessBasePassMeshParameters& Parameters) const
	{
		return FALSE;
	}

	UBOOL UseTranslucencyLightAttenuation(const FProcessBasePassMeshParameters& Parameters) const
	{
		return FALSE;
	}

	const FLightSceneInfo* GetTranslucencyMergedDynamicLightInfo() const
	{ 
		return NULL;
	}

	const FSHVectorRGB* GetTranslucencyCompositedDynamicLighting() const 
	{ 
		return NULL; 
	}

	const FProjectedShadowInfo* GetTranslucentPreShadow() const { return NULL; }

	/** Draws the translucent mesh with a specific light-map type, and fog volume type */
	template<typename LightMapPolicyType,typename FogDensityPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData,
		const typename FogDensityPolicyType::ElementDataType& FogDensityElementData
		) const
	{
		FDepthPriorityGroup::EBasePassDrawListType DrawType = FDepthPriorityGroup::EBasePass_Default;		
 
		if( StaticMesh->IsDecal() )
		{
			// handle decal case by adding to the decal base pass draw lists
			if( StaticMesh->IsTranslucent() )
			{
				// transparent decals rendered in the base pass are handled separately
				DrawType = FDepthPriorityGroup::EBasePass_Decals_Translucent;
			}
			else
			{
				DrawType = FDepthPriorityGroup::EBasePass_Decals;
			}
		}
		else if (StaticMesh->IsMasked())
		{
			DrawType = FDepthPriorityGroup::EBasePass_Masked;	
		}

		// Find the appropriate draw list for the static mesh based on the light-map policy type.
		TStaticMeshDrawList<TBasePassDrawingPolicy<LightMapPolicyType,FNoDensityPolicy> >& DrawList =
			Scene->DPGs[StaticMesh->DepthPriorityGroup].GetBasePassDrawList<LightMapPolicyType>(DrawType);

		const UBOOL bUsePreviewSkyLight = GIsEditor && LightMapPolicyType::bAllowPreviewSkyLight && Parameters.PrimitiveSceneInfo && Parameters.PrimitiveSceneInfo->bStaticShadowing;
		// Add the static mesh to the draw list.
		DrawList.AddMesh(
			StaticMesh,
			typename TBasePassDrawingPolicy<LightMapPolicyType,FNoDensityPolicy>::ElementDataType(LightMapElementData,FNoDensityPolicy::ElementDataType()),
			TBasePassDrawingPolicy<LightMapPolicyType,FNoDensityPolicy>(
			StaticMesh->VertexFactory,
			StaticMesh->MaterialRenderProxy,
			LightMapPolicy,
			Parameters.BlendMode,
			Parameters.LightingModel != MLM_Unlit && (StaticMesh->PrimitiveSceneInfo->HasDynamicSkyLighting() || bUsePreviewSkyLight)
			)
			);
	}
};

void FBasePassOpaqueDrawingPolicyFactory::AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh,ContextType)
{
	// Determine the mesh's material and blend mode.
	const FMaterial* Material = StaticMesh->MaterialRenderProxy->GetMaterial();
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Only draw opaque, non-distorted materials.
	if( (!IsTranslucentBlendMode(BlendMode) && BlendMode != BLEND_SoftMasked && !Material->IsDistorted()) ||
		// allow for decals to batch using translucent materials when rendered on opaque meshes
		StaticMesh->IsDecal() )
	{
		ProcessBasePassMesh(
			FProcessBasePassMeshParameters(
				*StaticMesh,
				Material,
				StaticMesh->PrimitiveSceneInfo,
				FALSE
				),
			FDrawBasePassStaticMeshAction(Scene,StaticMesh)
			);
	}
}

/** The action used to draw a base pass dynamic mesh element. */
class FDrawBasePassDynamicMeshAction
{
public:

	const FSceneView& View;
	UBOOL bBackFace;
	UBOOL bPreFog;
	FHitProxyId HitProxyId;

	/** Initialization constructor. */
	FDrawBasePassDynamicMeshAction(
		const FSceneView& InView,
		const UBOOL bInBackFace,
		const FHitProxyId InHitProxyId
		):
		View(InView),
		bBackFace(bInBackFace),
		HitProxyId(InHitProxyId)
	{}

	ESceneDepthPriorityGroup GetDPG(const FProcessBasePassMeshParameters& Parameters) const
	{
		return (ESceneDepthPriorityGroup)Parameters.PrimitiveSceneInfo->Proxy->GetDepthPriorityGroup(&View);
	}

	UBOOL ShouldReceiveDominantShadows(const FProcessBasePassMeshParameters& Parameters) const
	{
		// Only allow receiving dynamic shadows from the dominant light if bUseAsOccluder == TRUE, 
		// Since dominant light shadows are projected using depths from the depth only pass, which bUseAsOccluder controls.
		return GOnePassDominantLight && Parameters.PrimitiveSceneInfo->bAcceptsDynamicDominantLightShadows && Parameters.PrimitiveSceneInfo->bUseAsOccluder;
	}

	UBOOL ShouldOverrideDynamicShadowsOnTranslucency(const FProcessBasePassMeshParameters& Parameters) const
	{
		return FALSE;
	}

	UBOOL UseTranslucencyLightAttenuation(const FProcessBasePassMeshParameters& Parameters) const
	{
		return FALSE;
	}

	const FLightSceneInfo* GetTranslucencyMergedDynamicLightInfo() const
	{
		return NULL;
	}

	const FSHVectorRGB* GetTranslucencyCompositedDynamicLighting() const 
	{ 
		return NULL; 
	}

	const FProjectedShadowInfo* GetTranslucentPreShadow() const { return NULL; }

	/** Draws the translucent mesh with a specific light-map type, fog volume type, and shader complexity predicate. */
	template<typename LightMapPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData
		) const
	{
		const UBOOL bIsLitMaterial = Parameters.LightingModel != MLM_Unlit;

#if !FINAL_RELEASE
		// When rendering masked materials in the shader complexity viewmode, 
		// We want to overwrite complexity for the pixels which get depths written,
		// And accumulate complexity for pixels which get killed due to the opacity mask being below the clip value.
		// This is accomplished by forcing the masked materials to render depths in the depth only pass, 
		// Then rendering in the base pass with additive complexity blending, depth tests on, and depth writes off.
		if ((View.Family->ShowFlags & SHOW_ShaderComplexity) != 0)
		{
			RHISetDepthState(TStaticDepthState<FALSE,CF_LessEqual>::GetRHI());
		}
#endif

		const UBOOL bUsePreviewSkyLight = GIsEditor && LightMapPolicyType::bAllowPreviewSkyLight && Parameters.PrimitiveSceneInfo && Parameters.PrimitiveSceneInfo->bStaticShadowing;
		TBasePassDrawingPolicy<LightMapPolicyType,FNoDensityPolicy> DrawingPolicy(
			Parameters.Mesh.VertexFactory,
			Parameters.Mesh.MaterialRenderProxy,
			LightMapPolicy,
			Parameters.BlendMode,
			(Parameters.PrimitiveSceneInfo && (Parameters.PrimitiveSceneInfo->HasDynamicSkyLighting() || bUsePreviewSkyLight)) && bIsLitMaterial,
			(View.Family->ShowFlags & SHOW_ShaderComplexity) != 0
			);
		DrawingPolicy.DrawShared(
			&View,
			DrawingPolicy.CreateBoundShaderState(Parameters.Mesh.GetDynamicVertexStride())
			);
		DrawingPolicy.SetMeshRenderState(
			View,
			Parameters.PrimitiveSceneInfo,
			Parameters.Mesh,
			bBackFace,
			typename TBasePassDrawingPolicy<LightMapPolicyType,FNoDensityPolicy>::ElementDataType(
				LightMapElementData,
				FNoDensityPolicy::ElementDataType()
				)
			);
		DrawingPolicy.DrawMesh(Parameters.Mesh);

#if !FINAL_RELEASE
		if ((View.Family->ShowFlags & SHOW_ShaderComplexity) != 0)
		{
			RHISetDepthState(TStaticDepthState<TRUE,CF_LessEqual>::GetRHI());
		}
#endif
	}

	/** Draws the translucent mesh with a specific light-map type, and fog volume type */
	template<typename LightMapPolicyType,typename FogDensityPolicyType>
	void Process(
		const FProcessBasePassMeshParameters& Parameters,
		const LightMapPolicyType& LightMapPolicy,
		const typename LightMapPolicyType::ElementDataType& LightMapElementData,
		const typename FogDensityPolicyType::ElementDataType& FogDensityElementData
		) const
	{
		if((View.Family->ShowFlags & SHOW_Lighting) != 0)
		{
			Process<LightMapPolicyType>(Parameters,LightMapPolicy,LightMapElementData);
		}
		else
		{
			Process<FNoLightMapPolicy>(Parameters,FNoLightMapPolicy(),FNoLightMapPolicy::ElementDataType());
		}
	}
};

UBOOL FBasePassOpaqueDrawingPolicyFactory::DrawDynamicMesh(
	const FSceneView& View,
	ContextType DrawingContext,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	UBOOL bPreFog,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	FHitProxyId HitProxyId
	)
{
	// Determine the mesh's material and blend mode.
	const FMaterial* Material = Mesh.MaterialRenderProxy->GetMaterial();
	const EBlendMode BlendMode = Material->GetBlendMode();

	// Only draw opaque, non-distorted materials.
	if(!IsTranslucentBlendMode(BlendMode) && !Material->IsDistorted())
	{
		ProcessBasePassMesh(
			FProcessBasePassMeshParameters(
				Mesh,
				Material,
				PrimitiveSceneInfo,
				!bPreFog
				),
			FDrawBasePassDynamicMeshAction(
				View,
				bBackFace,
				HitProxyId
				)
			);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

