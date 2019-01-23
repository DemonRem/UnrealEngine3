/*=============================================================================
	TessellationRendering.h: Tessellation rendering definitions.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __TESSELLATIONRENDERING_H__
#define __TESSELLATIONRENDERING_H__

#if WITH_D3D11_TESSELLATION

class FTessellationMaterialPolicy
{
public:

	/**
	 * This method is used to set any salient defines to turn on/off specific code related to tessellation and the particular tessellation policy
	 */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, EMaterialTessellationMode TessellationMode, FShaderCompilerEnvironment& OutEnvironment)
	{
		if (Platform != SP_PCD3D_SM5 || TessellationMode == MTM_NoTessellation)
		{
			OutEnvironment.Definitions.Set(TEXT("USING_SM5_TESSELATION"),TEXT("0"));
			return;
		}

		OutEnvironment.Definitions.Set(TEXT("USING_SM5_TESSELATION"),TEXT("1"));

		if (TessellationMode == MTM_FlatTessellation)
		{
			OutEnvironment.Definitions.Set(TEXT("TESSELLATION_TYPE_FLAT"),TEXT("1"));
		}
		else if (TessellationMode == MTM_PNTriangles)
		{
			OutEnvironment.Definitions.Set(TEXT("TESSELLATION_TYPE_PNTRIANGLES"),TEXT("1"));
		}
	}

	/**
	 * This ShouldCache is used to determine is Hull/Domain shaders are required for the usage context.  Dictates whether the HS/DS will be compiled and cached.
	 */
	static UBOOL ShouldCache(EShaderFrequency ShaderFrequency, EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		if (ShaderFrequency == SF_Vertex || ShaderFrequency == SF_Pixel)
		{ 
			// always cache VS or PS
			return TRUE;	
		}
		
		// after this only testing HS or DS shaders
		if (Platform != SP_PCD3D_SM5)
		{
			// HS/DS only supported in SM5
			return FALSE;	
		}

		if (VertexFactoryType && !VertexFactoryType->SupportsTessellationShaders())
		{
			// VF can opt out of tessellation
			return FALSE;	
		}

		if (!Material || Material->GetD3D11TessellationMode() == MTM_NoTessellation) 
		{
			// Material controls use of tessellation
			return FALSE;	
		}

		return TRUE;
	}
};

/** Base Hull shader for drawing policy rendering */
class FBaseHullShader : public FShader
{
	DECLARE_SHADER_TYPE(FBaseHullShader,MeshMaterial);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		if (!FTessellationMaterialPolicy::ShouldCache(SF_Hull,Platform,Material,VertexFactoryType))
		{
			return FALSE;
		}

		return TRUE;
	}

	FBaseHullShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		MaterialParameters.Bind(Initializer.ParameterMap);
	}

	FBaseHullShader() {}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
		Ar << MaterialParameters;
		return bShaderHasOutdatedParameters;
	}
	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView& View)
	{
		FMaterialRenderContext MaterialRenderContext(MaterialRenderProxy, View.Family->CurrentWorldTime, View.Family->CurrentRealTime, &View);
		MaterialParameters.Set(this,MaterialRenderContext);
	}
	void SetMesh(const FPrimitiveSceneInfo* PrimitiveSceneInfo,const FMeshElement& Mesh,const FSceneView& View)
	{
		MaterialParameters.SetMesh(this,PrimitiveSceneInfo,Mesh,View);
	}
	virtual UBOOL IsUniformExpressionSetValid(const FUniformExpressionSet& UniformExpressionSet) const 
	{ 
		return MaterialParameters.IsUniformExpressionSetValid(UniformExpressionSet); 
	}

private:
	FMaterialHullShaderParameters MaterialParameters;
};

/** Base Domain shader for drawing policy rendering */
class FBaseDomainShader : public FShader
{
	DECLARE_SHADER_TYPE(FBaseDomainShader,MeshMaterial);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		if (!FTessellationMaterialPolicy::ShouldCache(SF_Domain,Platform,Material,VertexFactoryType))
		{
			return FALSE;
		}

		return TRUE;		
	}

	FBaseDomainShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		MaterialParameters.Bind(Initializer.ParameterMap);
	}

	FBaseDomainShader() {}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
		Ar << MaterialParameters;
		return bShaderHasOutdatedParameters;
	}
	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView& View)
	{
		FMaterialRenderContext MaterialRenderContext(MaterialRenderProxy, View.Family->CurrentWorldTime, View.Family->CurrentRealTime, &View);
		MaterialParameters.Set(this,MaterialRenderContext);
	}
	void SetMesh(const FPrimitiveSceneInfo* PrimitiveSceneInfo,const FMeshElement& Mesh,const FSceneView& View)
	{
		MaterialParameters.SetMesh(this,PrimitiveSceneInfo,Mesh,View);
	}
	virtual UBOOL IsUniformExpressionSetValid(const FUniformExpressionSet& UniformExpressionSet) const 
	{ 
		return MaterialParameters.IsUniformExpressionSetValid(UniformExpressionSet); 
	}

private:
	FMaterialDomainShaderParameters MaterialParameters;
};

#endif //#if WITH_D3D11_TESSELLATION

#endif
