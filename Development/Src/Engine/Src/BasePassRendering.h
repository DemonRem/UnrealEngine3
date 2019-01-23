/*=============================================================================
	BasePassRendering.h: Base pass rendering definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "LightMapRendering.h"

/**
 * The base shader type for vertex shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 */
template<typename LightMapPolicyType,typename FogDensityPolicyType>
class TBasePassVertexShader : public FShader, public LightMapPolicyType::VertexParametersType
{
	DECLARE_SHADER_TYPE(TBasePassVertexShader,MeshMaterial);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		// Opaque and modulated materials shouldn't apply fog volumes in their base pass.
		const EBlendMode BlendMode = Material->GetBlendMode();
		const UBOOL bUseFogVolume = IsTranslucentBlendMode(BlendMode) && BlendMode != BLEND_Modulate;
		const UBOOL bIsFogVolumeShader = FogDensityPolicyType::DensityFunctionType != FVDF_None;
		return	(bUseFogVolume || !bIsFogVolumeShader) &&
				FogDensityPolicyType::ShouldCache(Platform,Material,VertexFactoryType) && 
				LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		LightMapPolicyType::ModifyCompilationEnvironment(OutEnvironment);
		FogDensityPolicyType::ModifyCompilationEnvironment(OutEnvironment);
	}

	/** Initialization constructor. */
	TBasePassVertexShader(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer),
		VertexFactoryParameters(Initializer.VertexFactoryType,Initializer.ParameterMap)
	{
		LightMapPolicyType::VertexParametersType::Bind(Initializer.ParameterMap);

		HeightFogParameters.Bind(Initializer.ParameterMap);
		FogVolumeParameters.Bind(Initializer.ParameterMap);
	}
	TBasePassVertexShader() {}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		LightMapPolicyType::VertexParametersType::Serialize(Ar);
		Ar << VertexFactoryParameters;
		Ar << HeightFogParameters;
		Ar << FogVolumeParameters;
	}

	void SetParameters(
		FCommandContextRHI* Context,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView* View
		)
	{
		VertexFactoryParameters.Set(Context,this,VertexFactory,View);
		HeightFogParameters.Set(Context, MaterialRenderProxy, View, this);
	}

	void SetFogVolumeParameters(
		FCommandContextRHI* Context,
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		typename FogDensityPolicyType::ElementDataType FogVolumeElementData
		)
	{
		FogVolumeParameters.Set(Context, MaterialRenderProxy, this, FogVolumeElementData);
	}

	void SetLocalTransforms(FCommandContextRHI* Context,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal)
	{
		VertexFactoryParameters.SetLocalTransforms(Context,this,LocalToWorld,WorldToLocal);
	}

private:
	FVertexFactoryParameterRef VertexFactoryParameters;

	/** The parameters needed to calculate the fog contribution from height fog layers. */
	FHeightFogVertexShaderParameters HeightFogParameters;

	/** The parameters needed to calculate the fog contribution from an intersecting fog volume. */
	typename FogDensityPolicyType::VertexShaderParametersType FogVolumeParameters;
};

/**
 * The base type for pixel shaders that render the emissive color, and light-mapped/ambient lighting of a mesh.
 * The base type is shared between the versions with and without sky light.
 */
template<typename LightMapPolicyType>
class TBasePassPixelShaderBaseType : public FShader, public LightMapPolicyType::PixelParametersType
{
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType,UBOOL bEnableSkyLight)
	{
		return LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType,bEnableSkyLight);
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		LightMapPolicyType::ModifyCompilationEnvironment(OutEnvironment);
	}

	/** Initialization constructor. */
	TBasePassPixelShaderBaseType(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		LightMapPolicyType::PixelParametersType::Bind(Initializer.ParameterMap);
		MaterialParameters.Bind(Initializer.Material,Initializer.ParameterMap);
		AmbientColorAndSkyFactorParameter.Bind(Initializer.ParameterMap,TEXT("AmbientColorAndSkyFactor"),TRUE);
		UpperSkyColorParameter.Bind(Initializer.ParameterMap,TEXT("UpperSkyColor"),TRUE);
		LowerSkyColorParameter.Bind(Initializer.ParameterMap,TEXT("LowerSkyColor"),TRUE);
	}
	TBasePassPixelShaderBaseType() {}

	void SetParameters(FCommandContextRHI* Context,const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView* View)
	{
		FMaterialRenderContext MaterialRenderContext(MaterialRenderProxy, View->Family->CurrentWorldTime, View->Family->CurrentRealTime, View);
		MaterialParameters.Set(Context,this,MaterialRenderContext);

		if(AmbientColorAndSkyFactorParameter.IsBound())
		{
			// Draw the surface unlit if it's an unlit view, or it's a lit material without a light-map.
			const FMaterial* Material = MaterialRenderProxy->GetMaterial();
			const UBOOL bIsTranslucentLitMaterial = IsTranslucentBlendMode(Material->GetBlendMode()) && Material->GetLightingModel() != MLM_Unlit;
			const UBOOL bIsUnlitView = !(View->Family->ShowFlags & SHOW_Lighting);
			const UBOOL bDrawSurfaceUnlit = bIsUnlitView || (bIsTranslucentLitMaterial && LightMapPolicyType::bDrawLitTranslucencyUnlit);
			SetPixelShaderValue(
				Context,
				GetPixelShader(),
				AmbientColorAndSkyFactorParameter,
				bDrawSurfaceUnlit ? FLinearColor(1,1,1,0) : FLinearColor(0,0,0,1)
				);
		}
	}

	void SetLocalTransforms(FCommandContextRHI* Context,const FMaterialRenderProxy* MaterialRenderProxy,const FMatrix& LocalToWorld,UBOOL bBackFace)
	{
		MaterialParameters.SetLocalTransforms(Context,this,MaterialRenderProxy,LocalToWorld,bBackFace);
	}

	void SetSkyColor(FCommandContextRHI* Context,const FLinearColor& UpperSkyColor,const FLinearColor& LowerSkyColor)
	{
		SetPixelShaderValue(Context,GetPixelShader(),UpperSkyColorParameter,UpperSkyColor);
		SetPixelShaderValue(Context,GetPixelShader(),LowerSkyColorParameter,LowerSkyColor);
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		LightMapPolicyType::PixelParametersType::Serialize(Ar);
		Ar << MaterialParameters;
		Ar << AmbientColorAndSkyFactorParameter;
		Ar << UpperSkyColorParameter;
		Ar << LowerSkyColorParameter;
	}

private:
	FMaterialPixelShaderParameters MaterialParameters;
	FShaderParameter AmbientColorAndSkyFactorParameter;
	FShaderParameter UpperSkyColorParameter;
	FShaderParameter LowerSkyColorParameter;
};

/** The concrete base pass pixel shader type, parameterized by whether sky lighting is needed. */
template<typename LightMapPolicyType,UBOOL bEnableSkyLight>
class TBasePassPixelShader : public TBasePassPixelShaderBaseType<LightMapPolicyType>
{
	DECLARE_SHADER_TYPE(TBasePassPixelShader,MeshMaterial);
public:
	
	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		//don't compile skylight versions if the material is unlit
		const UBOOL bCacheShaders = !bEnableSkyLight || (Material->GetLightingModel() != MLM_Unlit);
		return bCacheShaders && 
			TBasePassPixelShaderBaseType<LightMapPolicyType>::ShouldCache(Platform, Material, VertexFactoryType, bEnableSkyLight);
	}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		TBasePassPixelShaderBaseType<LightMapPolicyType>::ModifyCompilationEnvironment(OutEnvironment);
		OutEnvironment.Definitions.Set(TEXT("ENABLE_SKY_LIGHT"),bEnableSkyLight ? TEXT("1") : TEXT("0"));
	}
	
	/** Initialization constructor. */
	TBasePassPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		TBasePassPixelShaderBaseType<LightMapPolicyType>(Initializer)
	{}

	/** Default constructor. */
	TBasePassPixelShader() {}
};

/**
 * Draws the emissive color and the light-map of a mesh.
 */
template<typename LightMapPolicyType,typename FogDensityPolicyType>
class TBasePassDrawingPolicy : public FMeshDrawingPolicy
{
public:

	/** The data the drawing policy uses for each mesh element. */
	class ElementDataType
	{
	public:

		/** The element's light-map data. */
		typename LightMapPolicyType::ElementDataType LightMapElementData;

		/** The element's fog volume data. */
		typename FogDensityPolicyType::ElementDataType FogVolumeElementData;

		/** Default constructor. */
		ElementDataType()
		{}

		/** Initialization constructor. */
		ElementDataType(
			const typename LightMapPolicyType::ElementDataType& InLightMapElementData,
			const typename FogDensityPolicyType::ElementDataType& InFogVolumeElementData
			):
			LightMapElementData(InLightMapElementData),
			FogVolumeElementData(InFogVolumeElementData)
		{}
	};

	/** Initialization constructor. */
	TBasePassDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		LightMapPolicyType InLightMapPolicy,
		UBOOL bInEnableSkyLight,
		UBOOL bOverrideWithShaderComplexity = FALSE
		):
		FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy,bOverrideWithShaderComplexity),
		LightMapPolicy(InLightMapPolicy),
		bEnableSkyLight(bInEnableSkyLight)
	{
		const FMaterialShaderMap* MaterialShaderIndex = InMaterialRenderProxy->GetMaterial()->GetShaderMap();
		const FMeshMaterialShaderMap* MeshShaderIndex = MaterialShaderIndex->GetMeshShaderMap(InVertexFactory->GetType());
		
		VertexShader = MeshShaderIndex->GetShader<TBasePassVertexShader<LightMapPolicyType,FogDensityPolicyType> >();

		// Find the appropriate shaders based on whether sky lighting is needed.
		if(bEnableSkyLight)
		{
			PixelShader = MeshShaderIndex->GetShader<TBasePassPixelShader<LightMapPolicyType,TRUE> >();
		}
		else
		{
			PixelShader = MeshShaderIndex->GetShader<TBasePassPixelShader<LightMapPolicyType,FALSE> >();
		}
	}

	// FMeshDrawingPolicy interface.

	UBOOL Matches(const TBasePassDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) &&
			VertexShader == Other.VertexShader &&
			PixelShader == Other.PixelShader &&
			LightMapPolicy == Other.LightMapPolicy;
	}

	void DrawShared(FCommandContextRHI* Context,const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const
	{
		// Set the base pass shader parameters for the material.
		VertexShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View);

#if !FINAL_RELEASE
		if (bOverrideWithShaderComplexity)
		{
			//if we are in the translucent pass then override the blend mode, otherwise keep the opaque mode
			if (IsTranslucentBlendMode(MaterialRenderProxy->GetMaterial()->GetBlendMode()))
			{
				if (GEngine->bUsePixelShaderComplexity && GEngine->bUseAdditiveComplexity)
				{
					//add complexity to existing
					RHISetBlendState(Context,TStaticBlendState<BO_Add,BF_One,BF_One,BO_Add,BF_Zero,BF_One>::GetRHI());
				}
				else
				{
					//overwrite existing complexity
					RHISetBlendState(Context,TStaticBlendState<>::GetRHI());
				}
			}

			TShaderMapRef<FShaderComplexityAccumulatePixelShader> ShaderComplexityPixelShader(GetGlobalShaderMap());
			ShaderComplexityPixelShader->SetParameters(Context,VertexShader->GetNumInstructions(),PixelShader->GetNumInstructions());
		}
		else
#endif
		{
			PixelShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View);

			switch(MaterialRenderProxy->GetMaterial()->GetBlendMode())
			{
			default:
			case BLEND_Opaque:
			case BLEND_Masked:
				RHISetBlendState(Context,TStaticBlendState<>::GetRHI());
				break;
			case BLEND_Translucent:
				RHISetBlendState(
					Context,
					CanBlendWithFPRenderTarget(GRHIShaderPlatform) ?
						// Blend with the existing scene color.
						TStaticBlendState<BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_Zero,BF_One>::GetRHI() :
						// Blend with the existing translucent color in the LDR buffer, and modulate the scene color transmission factor.
						TStaticBlendState<BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_Zero,BF_InverseSourceAlpha>::GetRHI()
					);
				break;
			case BLEND_Additive:
				RHISetBlendState(
					Context,
					// Modulate with the source alpha and add to the existing scene color.
					TStaticBlendState<BO_Add,BF_SourceAlpha,BF_One,BO_Add,BF_Zero,BF_One>::GetRHI()
					);
				break;
			case BLEND_Modulate:
				RHISetBlendState(
					Context,
					// Modulate with the existing scene color.
					TStaticBlendState<BO_Add,BF_DestColor,BF_Zero,BO_Add,BF_Zero,BF_One>::GetRHI()
					);
				break;
			};
		}

		// Set the light-map policy.
		LightMapPolicy.Set(Context,VertexShader,bOverrideWithShaderComplexity ? NULL : PixelShader,PixelShader,VertexFactory,MaterialRenderProxy,View);

		// Set the actual shader & vertex declaration state
		RHISetBoundShaderState(Context, BoundShaderState);
	}

	/** 
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @param DynamicStride - optional stride for dynamic vertex data
	* @return new bound shader state object
	*/
	FBoundShaderStateRHIRef CreateBoundShaderState(DWORD DynamicStride = 0)
	{
		FVertexDeclarationRHIParamRef VertexDeclaration;
		DWORD StreamStrides[MaxVertexElementCount];

		LightMapPolicy.GetVertexDeclarationInfo(VertexDeclaration, StreamStrides, VertexFactory);
		if (DynamicStride)
		{
			StreamStrides[0] = DynamicStride;
		}

		FPixelShaderRHIParamRef PixelShaderRHIRef = PixelShader->GetPixelShader();

#if !FINAL_RELEASE
		if (bOverrideWithShaderComplexity)
		{
			TShaderMapRef<FShaderComplexityAccumulatePixelShader> ShaderComplexityAccumulatePixelShader(GetGlobalShaderMap());
			PixelShaderRHIRef = ShaderComplexityAccumulatePixelShader->GetPixelShader();
		}
#endif

		return RHICreateBoundShaderState(VertexDeclaration, StreamStrides, VertexShader->GetVertexShader(), PixelShaderRHIRef);
	}

	void SetMeshRenderState(
		FCommandContextRHI* Context,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		const ElementDataType& ElementData
		) const
	{
		// Set the fog volume parameters.
		VertexShader->SetFogVolumeParameters(Context,VertexFactory,MaterialRenderProxy,ElementData.FogVolumeElementData);

		VertexShader->SetLocalTransforms(Context,Mesh.LocalToWorld,Mesh.WorldToLocal);

		// Set the light-map policy's mesh-specific settings.
		LightMapPolicy.SetMesh(Context,VertexShader,bOverrideWithShaderComplexity ? NULL : PixelShader,VertexShader,PixelShader,VertexFactory,MaterialRenderProxy,ElementData.LightMapElementData);

#if !FINAL_RELEASE
		//don't set the draw policies' pixel shader parameters if the shader complexity viewmode is enabled
		//since they will overwrite the FShaderComplexityAccumulatePixelShader parameters
		if(!bOverrideWithShaderComplexity)
#endif
		{
			PixelShader->SetLocalTransforms(Context,Mesh.MaterialRenderProxy,Mesh.LocalToWorld,bBackFace);

			if(bEnableSkyLight)
			{
				FLinearColor UpperSkyLightColor = FLinearColor::Black;
				FLinearColor LowerSkyLightColor = FLinearColor::Black;
				if(PrimitiveSceneInfo)
				{
					UpperSkyLightColor = PrimitiveSceneInfo->UpperSkyLightColor;
					LowerSkyLightColor = PrimitiveSceneInfo->LowerSkyLightColor;
				}
				PixelShader->SetSkyColor(Context,UpperSkyLightColor,LowerSkyLightColor);
			}
		}

		FMeshDrawingPolicy::SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,FMeshDrawingPolicy::ElementDataType());
	}

	friend INT Compare(const TBasePassDrawingPolicy& A,const TBasePassDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
		COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		return Compare(A.LightMapPolicy,B.LightMapPolicy);
	}

protected:
	TBasePassVertexShader<LightMapPolicyType,FogDensityPolicyType>* VertexShader;
	TBasePassPixelShaderBaseType<LightMapPolicyType>* PixelShader;

	LightMapPolicyType LightMapPolicy;

	BITFIELD bEnableSkyLight : 1;
};

/**
 * A drawing policy factory for the base pass drawing policy.
 */
class FBasePassOpaqueDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = TRUE };
	struct ContextType {};

	static void AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh,ContextType DrawingContext = ContextType());
	static UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FSceneView* View,
		ContextType DrawingContext,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId
		);
	static UBOOL IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
	{
		// Ignore non-opaque materials in the opaque base pass.
		return MaterialRenderProxy && IsTranslucentBlendMode(MaterialRenderProxy->GetMaterial()->GetBlendMode());
	}
};

/** The parameters used to process a base pass mesh. */
class FProcessBasePassMeshParameters
{
public:

	const FMeshElement& Mesh;
	const FMaterial* Material;
	const FPrimitiveSceneInfo* PrimitiveSceneInfo;
	EBlendMode BlendMode;
	EMaterialLightingModel LightingModel;

	/** Initialization constructor. */
	FProcessBasePassMeshParameters(
		const FMeshElement& InMesh,
		const FMaterial* InMaterial,
		const FPrimitiveSceneInfo* InPrimitiveSceneInfo
		):
		Mesh(InMesh),
		Material(InMaterial),
		PrimitiveSceneInfo(InPrimitiveSceneInfo),
		BlendMode(InMaterial->GetBlendMode()),
		LightingModel(InMaterial->GetLightingModel())
	{
	}
};

/** Processes a base pass mesh using a known light map policy, and unknown fog density policy. */
template<typename ProcessActionType,typename LightMapPolicyType>
void ProcessBasePassMesh_LightMapped(
	const FProcessBasePassMeshParameters& Parameters,
	const ProcessActionType& Action,
	const LightMapPolicyType& LightMapPolicy,
	const typename LightMapPolicyType::ElementDataType& LightMapElementData
	)
{
	// Don't render fog on opaque or modulated materials and GPU skinned meshes.
	const UBOOL bDisableFog =
		!IsTranslucentBlendMode(Parameters.BlendMode) ||
		(Parameters.BlendMode == BLEND_Modulate) ||
		Parameters.Mesh.VertexFactory->IsGPUSkinned();

	// Determine the density function of the fog volume the primitive is in.
	const EFogVolumeDensityFunction FogVolumeDensityFunction = 
		Parameters.PrimitiveSceneInfo && Parameters.PrimitiveSceneInfo->FogVolumeSceneInfo && !bDisableFog ?
			Parameters.PrimitiveSceneInfo->FogVolumeSceneInfo->GetDensityFunctionType() :
			FVDF_None;

	// Define a macro to handle a specific case of fog volume density function.
	#define HANDLE_FOG_VOLUME_DENSITY_FUNCTION(FogDensityPolicyType,FogDensityElementData) \
		case FogDensityPolicyType::DensityFunctionType: \
			Action.template Process<LightMapPolicyType,FogDensityPolicyType>(Parameters,LightMapPolicy,LightMapElementData,FogDensityElementData); \
			break;

	// Call Action.Process with the appropriate fog volume density policy type.
	switch(FogVolumeDensityFunction)
	{
		HANDLE_FOG_VOLUME_DENSITY_FUNCTION(FConstantDensityPolicy,Parameters.PrimitiveSceneInfo->FogVolumeSceneInfo);
		HANDLE_FOG_VOLUME_DENSITY_FUNCTION(FLinearHalfspaceDensityPolicy,Parameters.PrimitiveSceneInfo->FogVolumeSceneInfo);
		HANDLE_FOG_VOLUME_DENSITY_FUNCTION(FSphereDensityPolicy,Parameters.PrimitiveSceneInfo->FogVolumeSceneInfo);
		HANDLE_FOG_VOLUME_DENSITY_FUNCTION(FConeDensityPolicy,Parameters.PrimitiveSceneInfo->FogVolumeSceneInfo);
		default:
		HANDLE_FOG_VOLUME_DENSITY_FUNCTION(FNoDensityPolicy,FNoDensityPolicy::ElementDataType());
	};

	#undef HANDLE_FOG_VOLUME_DENSITY_FUNCTION
}

/** Processes a base pass mesh using an unknown light map policy, and unknown fog density policy. */
template<typename ProcessActionType>
void ProcessBasePassMesh(
	const FProcessBasePassMeshParameters& Parameters,
	const ProcessActionType& Action
	)
{
	// Check for a cached light-map.
	const UBOOL bIsLitMaterial = Parameters.LightingModel != MLM_Unlit;
	const FLightMapInteraction LightMapInteraction = (Parameters.Mesh.LCI && bIsLitMaterial) ? Parameters.Mesh.LCI->GetLightMapInteraction() : FLightMapInteraction();

	// Define a macro to handle a specific case of light-map type.
	#define HANDLE_LIGHTMAP_TYPE(LightMapInteractionType,DirectionalLightMapPolicyType,SimpleLightMapPolicyType,LightMapPolicyParameters,LightMapElementData) \
		case LightMapInteractionType: \
			if(GSystemSettings.bAllowDirectionalLightMaps) \
			{ \
				ProcessBasePassMesh_LightMapped<ProcessActionType,DirectionalLightMapPolicyType>(Parameters,Action,DirectionalLightMapPolicyType LightMapPolicyParameters,LightMapElementData); \
			} \
			else \
			{ \
				ProcessBasePassMesh_LightMapped<ProcessActionType,SimpleLightMapPolicyType>(Parameters,Action,SimpleLightMapPolicyType LightMapPolicyParameters,LightMapElementData); \
			} \
			break;

	// Call DrawTranslucentMesh_LightMapped with the appropriate light-map policy.
	switch(LightMapInteraction.GetType())
	{
		HANDLE_LIGHTMAP_TYPE(LMIT_Vertex,FDirectionalVertexLightMapPolicy,FSimpleVertexLightMapPolicy,(),LightMapInteraction);
		HANDLE_LIGHTMAP_TYPE(LMIT_Texture,FDirectionalLightMapTexturePolicy,FSimpleLightMapTexturePolicy,(LightMapInteraction),LightMapInteraction);
		default:
		HANDLE_LIGHTMAP_TYPE(LMIT_None,FNoLightMapPolicy,FNoLightMapPolicy,(),FNoLightMapPolicy::ElementDataType());
	};

	#undef HANDLE_LIGHTMAP_TYPE
}
