/*=============================================================================
	EmissiveRendering.h: Emissive rendering definitions.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "LightMapRendering.h"

/**
 * A vertex shader for rendering the emissive color, and light-mapped/ambient lighting of a mesh.
 */
template<class LightMapPolicyType>
class TEmissiveVertexShader : public FShader, public LightMapPolicyType::VertexParametersType
{
	DECLARE_SHADER_TYPE(TEmissiveVertexShader,MeshMaterial);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return	!IsTranslucentBlendMode(Material->GetBlendMode()) &&
				LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType);
	}

	TEmissiveVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer),
		VertexFactoryParameters(Initializer.VertexFactoryType,Initializer.ParameterMap)
	{
		LightMapPolicyType::VertexParametersType::Bind(Initializer.ParameterMap);
	}
	TEmissiveVertexShader() {}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("NUM_LIGHTMAP_COEFFICIENTS"),*FString::Printf(TEXT("%u"),NUM_LIGHTMAP_COEFFICIENTS));
		LightMapPolicyType::ModifyCompilationEnvironment(OutEnvironment);
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		LightMapPolicyType::VertexParametersType::Serialize(Ar);
		Ar << VertexFactoryParameters;
	}

	void SetParameters(FCommandContextRHI* Context,const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView* View)
	{
		VertexFactoryParameters.Set(Context,this,VertexFactory,View);
	}

	void SetLocalTransforms(FCommandContextRHI* Context,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal)
	{
		VertexFactoryParameters.SetLocalTransforms(Context,this,LocalToWorld,WorldToLocal);
	}

private:
	FVertexFactoryParameterRef VertexFactoryParameters;
};

/**
 * A pixel shader for rendering the emissive color, and light-mapped/ambient lighting of a mesh.
 */
template<class LightMapPolicyType>
class TEmissivePixelShader : public FShader, public LightMapPolicyType::PixelParametersType
{
	DECLARE_SHADER_TYPE(TEmissivePixelShader,MeshMaterial);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return	!IsTranslucentBlendMode(Material->GetBlendMode()) &&
				LightMapPolicyType::ShouldCache(Platform,Material,VertexFactoryType);
	}

	TEmissivePixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		LightMapPolicyType::PixelParametersType::Bind(Initializer.ParameterMap);
		MaterialParameters.Bind(Initializer.Material,Initializer.ParameterMap);
		AmbientColorAndSkyFactorParameter.Bind(Initializer.ParameterMap,TEXT("AmbientColorAndSkyFactor"),TRUE);
		UpperSkyColorParameter.Bind(Initializer.ParameterMap,TEXT("UpperSkyColor"),TRUE);
		LowerSkyColorParameter.Bind(Initializer.ParameterMap,TEXT("LowerSkyColor"),TRUE);
	}
	TEmissivePixelShader() {}

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("NUM_LIGHTMAP_COEFFICIENTS"),*FString::Printf(TEXT("%u"),NUM_LIGHTMAP_COEFFICIENTS));
		LightMapPolicyType::ModifyCompilationEnvironment(OutEnvironment);
	}

	void SetParameters(FCommandContextRHI* Context,const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView* View)
	{
		FMaterialRenderContext MaterialRenderContext(MaterialRenderProxy, View->Family->CurrentWorldTime, View->Family->CurrentRealTime, View);
		MaterialParameters.Set(Context,this,MaterialRenderContext);
		SetPixelShaderValue(
			Context,
			GetPixelShader(),
			AmbientColorAndSkyFactorParameter,
			(View->Family->ShowFlags & SHOW_Lighting) ? FLinearColor(0,0,0,1) : FLinearColor(1,1,1,0)
			);
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

/**
 * Draws the emissive color and the light-map of a mesh.
 */
template<class LightMapPolicyType>
class TEmissiveDrawingPolicy : public FMeshDrawingPolicy
{
public:

	typedef typename LightMapPolicyType::ElementDataType ElementDataType;

	TEmissiveDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		LightMapPolicyType InLightMapPolicy
		):
		FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy),
		LightMapPolicy(InLightMapPolicy)
	{
		const FMaterialShaderMap* MaterialShaderIndex = InMaterialRenderProxy->GetMaterial()->GetShaderMap();
		const FMeshMaterialShaderMap* MeshShaderIndex = MaterialShaderIndex->GetMeshShaderMap(InVertexFactory->GetType());
		VertexShader = MeshShaderIndex->GetShader<TEmissiveVertexShader<LightMapPolicyType> >();
		PixelShader = MeshShaderIndex->GetShader<TEmissivePixelShader<LightMapPolicyType> >();
	}

	// FMeshDrawingPolicy interface.

	virtual UBOOL Matches(const TEmissiveDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other) &&
			VertexShader == Other.VertexShader &&
			PixelShader == Other.PixelShader &&
			LightMapPolicy == Other.LightMapPolicy;
	}

	virtual void DrawShared(FCommandContextRHI* Context,const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const
	{
		// Set the emissive shader parameters for the material.
		VertexShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View);
		PixelShader->SetParameters(Context,VertexFactory,MaterialRenderProxy,View);

		// Set the light-map policy.
		LightMapPolicy.Set(Context,VertexShader,PixelShader,PixelShader,VertexFactory,MaterialRenderProxy,View);

		// Set the actual shader & vertex declaration state
		RHISetBoundShaderState(Context, BoundShaderState);
	}

	/** 
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @param DynamicStride - optional stride for dynamic vertex data
	* @return new bound shader state object
	*/
	virtual FBoundShaderStateRHIRef CreateBoundShaderState(DWORD DynamicStride = 0)
	{
		FVertexDeclarationRHIParamRef VertexDeclaration;
		DWORD StreamStrides[MaxVertexElementCount];

		LightMapPolicy.GetVertexDeclarationInfo(VertexDeclaration, StreamStrides, VertexFactory);
		if (DynamicStride)
		{
			StreamStrides[0] = DynamicStride;
		}

		return RHICreateBoundShaderState(VertexDeclaration, StreamStrides, VertexShader->GetVertexShader(), PixelShader->GetPixelShader());
	}

	virtual void SetMeshRenderState(
		FCommandContextRHI* Context,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		const ElementDataType& ElementData
		) const
	{
		// Set the light-map policy's mesh-specific settings.
		LightMapPolicy.SetMesh(Context,VertexShader,PixelShader,VertexShader,PixelShader,VertexFactory,MaterialRenderProxy,ElementData);

		VertexShader->SetLocalTransforms(Context,Mesh.LocalToWorld,Mesh.WorldToLocal);
		PixelShader->SetLocalTransforms(Context,Mesh.MaterialRenderProxy,Mesh.LocalToWorld,bBackFace);
		if(PrimitiveSceneInfo)
		{
			PixelShader->SetSkyColor(Context,PrimitiveSceneInfo->UpperSkyLightColor,PrimitiveSceneInfo->LowerSkyLightColor);
		}
		else
		{
			PixelShader->SetSkyColor(Context,FLinearColor::Black,FLinearColor::Black);
		}
		FMeshDrawingPolicy::SetMeshRenderState(Context,PrimitiveSceneInfo,Mesh,bBackFace,FMeshDrawingPolicy::ElementDataType());
	}

	friend INT Compare(const TEmissiveDrawingPolicy& A,const TEmissiveDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
		COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		return Compare(A.LightMapPolicy,B.LightMapPolicy);
	}

protected:
	TEmissiveVertexShader<LightMapPolicyType>* VertexShader;
	TEmissivePixelShader<LightMapPolicyType>* PixelShader;

	LightMapPolicyType LightMapPolicy;
};


/**
* Draws the shader complexity from the emissive shader for a mesh, this is used by the shader complexity viewmode.
*/
template<class LightMapPolicyType>
class TShaderComplexityEmissiveDrawingPolicy : public TEmissiveDrawingPolicy<LightMapPolicyType>
{
public:

	typedef typename LightMapPolicyType::ElementDataType ElementDataType;

	TShaderComplexityEmissiveDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		LightMapPolicyType InLightMapPolicy
		):
	TEmissiveDrawingPolicy<LightMapPolicyType>(InVertexFactory,InMaterialRenderProxy,InLightMapPolicy)
	{
		//find the complexity pixel shader in the default material
		FMaterialRenderProxy* DefaultMaterialInstance = GEngine->DefaultMaterial->GetRenderProxy(FALSE);
		const FMaterialShaderMap* MaterialShaderIndex = DefaultMaterialInstance->GetMaterial()->GetShaderMap();
		const FMeshMaterialShaderMap* MeshShaderIndex = MaterialShaderIndex->GetMeshShaderMap(InVertexFactory->GetType());
		ShaderComplexityAccumulatePixelShader = MeshShaderIndex->GetShader<FShaderComplexityAccumulatePixelShader>();
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

		TEmissiveDrawingPolicy<LightMapPolicyType>::LightMapPolicy.GetVertexDeclarationInfo(VertexDeclaration, StreamStrides, TEmissiveDrawingPolicy<LightMapPolicyType>::VertexFactory);
		if (DynamicStride)
		{
			StreamStrides[0] = DynamicStride;
		}

		return RHICreateBoundShaderState(VertexDeclaration, StreamStrides, TEmissiveDrawingPolicy<LightMapPolicyType>::VertexShader->GetVertexShader(), ShaderComplexityAccumulatePixelShader->GetPixelShader());
	}

	void SetMeshRenderState(
		FCommandContextRHI* Context,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		const ElementDataType& ElementData
		) const
	{
		TEmissiveDrawingPolicy<LightMapPolicyType>::SetMeshRenderState(Context, PrimitiveSceneInfo, Mesh, bBackFace, ElementData);

		//set these parameters after all TEmissiveDrawingPolicy pixel shader parameters so they won't get overwritten
		ShaderComplexityAccumulatePixelShader->SetParameters(
			Context, 
			TEmissiveDrawingPolicy<LightMapPolicyType>::MaterialRenderProxy, 
			TEmissiveDrawingPolicy<LightMapPolicyType>::VertexShader->GetNumInstructions(), 
			TEmissiveDrawingPolicy<LightMapPolicyType>::PixelShader->GetNumInstructions());
	}

private:

	/** Pixel shader used to output shader complexity */
	FShaderComplexityAccumulatePixelShader* ShaderComplexityAccumulatePixelShader;
};


/**
 * A drawing policy factory for the emissive drawing policy.
 */
class FEmissiveDrawingPolicyFactory
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
};
