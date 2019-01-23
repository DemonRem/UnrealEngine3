/*=============================================================================
	MaterialShader.h: Material shader definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

//
// Globals
//
TDynamicMap<FStaticParameterSet,FMaterialShaderMap*> FMaterialShaderMap::GIdToMaterialShaderMap[SP_NumPlatforms];

/** Converts an EMaterialLightingModel to a string description. */
FString GetLightingModelString(EMaterialLightingModel LightingModel)
{
	FString LightingModelName;
	switch(LightingModel)
	{
		case MLM_Phong: LightingModelName = TEXT("MLM_Phong"); break;
		case MLM_NonDirectional: LightingModelName = TEXT("MLM_NonDirectional"); break;
		case MLM_Unlit: LightingModelName = TEXT("MLM_Unlit"); break;
		case MLM_SHPRT: LightingModelName = TEXT("MLM_SHPRT"); break;
		case MLM_Custom: LightingModelName = TEXT("MLM_Custom"); break;
		default: LightingModelName = TEXT("Unknown"); break;
	}
	return LightingModelName;
}

/** Converts an EBlendMode to a string description. */
FString GetBlendModeString(EBlendMode BlendMode)
{
	FString BlendModeName;
	switch(BlendMode)
	{
		case BLEND_Opaque: BlendModeName = TEXT("BLEND_Opaque"); break;
		case BLEND_Masked: BlendModeName = TEXT("BLEND_Masked"); break;
		case BLEND_Translucent: BlendModeName = TEXT("BLEND_Translucent"); break;
		case BLEND_Additive: BlendModeName = TEXT("BLEND_Additive"); break;
		case BLEND_Modulate: BlendModeName = TEXT("BLEND_Modulate"); break;
		default: BlendModeName = TEXT("Unknown"); break;
	}
	return BlendModeName;
}

/** Called for every material shader to update the appropriate stats. */
void UpdateMaterialShaderCompilingStats(const FMaterial* Material)
{
	INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumTotalMaterialShaders,1);

	switch(Material->GetBlendMode())
	{
		case BLEND_Opaque: INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumOpaqueMaterialShaders,1); break;
		case BLEND_Masked: INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumMaskedMaterialShaders,1); break;
		default: INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumTransparentMaterialShaders,1); break;
	}

	switch(Material->GetLightingModel())
	{
		case MLM_Phong: INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumLitMaterialShaders,1); break;
		case MLM_Unlit: INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumUnlitMaterialShaders,1); break;
		default: break;
	};

	if (Material->IsSpecialEngineMaterial())
	{
		INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumSpecialMaterialShaders,1);
	}
	if (Material->IsTerrainMaterial())
	{
		INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumTerrainMaterialShaders,1);
	}
	if (Material->IsDecalMaterial())
	{
		INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumDecalMaterialShaders,1);
	}
	if (Material->IsUsedWithParticleSystem())
	{
		INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumParticleMaterialShaders,1);
	}
	if (Material->IsUsedWithSkeletalMesh())
	{
		INC_DWORD_STAT_BY(STAT_ShaderCompiling_NumSkinnedMaterialShaders,1);
	}
}

/** 
* Indicates whether two static parameter sets are unequal.  This takes into account parameter override settings.
* 
* @param ReferenceSet	The set to compare against
* @return				TRUE if the sets are not equal
*/
UBOOL FStaticParameterSet::ShouldMarkDirty(const FStaticParameterSet * ReferenceSet)
{
	if (ReferenceSet->StaticSwitchParameters.Num() != StaticSwitchParameters.Num()
		|| ReferenceSet->StaticComponentMaskParameters.Num() != StaticComponentMaskParameters.Num())
	{
		return TRUE;
	}

	UBOOL bMarkDirty = FALSE;

	//switch parameters
	for (INT RefParamIndex = 0;RefParamIndex < ReferenceSet->StaticSwitchParameters.Num();RefParamIndex++)
	{
		const FStaticSwitchParameter * ReferenceSwitchParameter = &ReferenceSet->StaticSwitchParameters(RefParamIndex);
		for (INT ParamIndex = 0;ParamIndex < StaticSwitchParameters.Num();ParamIndex++)
		{
			FStaticSwitchParameter * SwitchParameter = &StaticSwitchParameters(ParamIndex);
			if (SwitchParameter->ParameterName == ReferenceSwitchParameter->ParameterName
				&& SwitchParameter->ExpressionGUID == ReferenceSwitchParameter->ExpressionGUID)
			{
				SwitchParameter->bOverride = ReferenceSwitchParameter->bOverride;
				if (SwitchParameter->Value != ReferenceSwitchParameter->Value)
				{
					bMarkDirty = TRUE;
					break;
				}
			}
		}
	}

	//component mask parameters
	for (INT RefParamIndex = 0;RefParamIndex < ReferenceSet->StaticComponentMaskParameters.Num();RefParamIndex++)
	{
		const FStaticComponentMaskParameter * ReferenceComponentMaskParameter = &ReferenceSet->StaticComponentMaskParameters(RefParamIndex);
		for (INT ParamIndex = 0;ParamIndex < StaticComponentMaskParameters.Num();ParamIndex++)
		{
			FStaticComponentMaskParameter * ComponentMaskParameter = &StaticComponentMaskParameters(ParamIndex);
			if (ComponentMaskParameter->ParameterName == ReferenceComponentMaskParameter->ParameterName
				&& ComponentMaskParameter->ExpressionGUID == ReferenceComponentMaskParameter->ExpressionGUID)
			{
				ComponentMaskParameter->bOverride = ReferenceComponentMaskParameter->bOverride;
				if (ComponentMaskParameter->R != ReferenceComponentMaskParameter->R
					|| ComponentMaskParameter->G != ReferenceComponentMaskParameter->G
					|| ComponentMaskParameter->B != ReferenceComponentMaskParameter->B
					|| ComponentMaskParameter->A != ReferenceComponentMaskParameter->A)
				{
					bMarkDirty = TRUE;
					break;
				}
			}
		}
	}

	return bMarkDirty;
}

/** 
* Tests this set against another for equality, disregarding override settings.
* 
* @param ReferenceSet	The set to compare against
* @return				TRUE if the sets are equal
*/
UBOOL FStaticParameterSet::operator==(const FStaticParameterSet &ReferenceSet)
{
	if (BaseMaterialId == ReferenceSet.BaseMaterialId)
	{
		if (StaticSwitchParameters.Num() == ReferenceSet.StaticSwitchParameters.Num()
			&& StaticComponentMaskParameters.Num() == ReferenceSet.StaticComponentMaskParameters.Num())
		{
			for (INT SwitchIndex = 0; SwitchIndex < StaticSwitchParameters.Num(); SwitchIndex++)
			{
				if (StaticSwitchParameters(SwitchIndex).ParameterName != ReferenceSet.StaticSwitchParameters(SwitchIndex).ParameterName
					|| StaticSwitchParameters(SwitchIndex).ExpressionGUID != ReferenceSet.StaticSwitchParameters(SwitchIndex).ExpressionGUID
					|| StaticSwitchParameters(SwitchIndex).Value != ReferenceSet.StaticSwitchParameters(SwitchIndex).Value)
				{
					return FALSE;
				}
			}

			for (INT ComponentMaskIndex = 0; ComponentMaskIndex < StaticComponentMaskParameters.Num(); ComponentMaskIndex++)
			{
				if (StaticComponentMaskParameters(ComponentMaskIndex).ParameterName != ReferenceSet.StaticComponentMaskParameters(ComponentMaskIndex).ParameterName
					|| StaticComponentMaskParameters(ComponentMaskIndex).ExpressionGUID != ReferenceSet.StaticComponentMaskParameters(ComponentMaskIndex).ExpressionGUID
					|| StaticComponentMaskParameters(ComponentMaskIndex).R != ReferenceSet.StaticComponentMaskParameters(ComponentMaskIndex).R
					|| StaticComponentMaskParameters(ComponentMaskIndex).G != ReferenceSet.StaticComponentMaskParameters(ComponentMaskIndex).G
					|| StaticComponentMaskParameters(ComponentMaskIndex).B != ReferenceSet.StaticComponentMaskParameters(ComponentMaskIndex).B
					|| StaticComponentMaskParameters(ComponentMaskIndex).A != ReferenceSet.StaticComponentMaskParameters(ComponentMaskIndex).A)
				{
					return FALSE;
				}
			}

			return TRUE;
		}
	}
	return FALSE;
}

void FMaterialPixelShaderParameters::Bind(const FMaterial* Material,const FShaderParameterMap& ParameterMap)
{
	// Bind uniform scalar expression parameters.
	for(INT ParameterIndex = 0;ParameterIndex < Material->UniformScalarExpressions.Num();ParameterIndex++)
	{
		FShaderParameter ShaderParameter;
		FString ParameterName = FString::Printf(TEXT("UniformScalar_%u"),ParameterIndex);
		ShaderParameter.Bind(ParameterMap,*ParameterName,TRUE);
		if(ShaderParameter.IsBound())
		{
			FUniformParameter* UniformParameter = new(UniformParameters) FUniformParameter;
			UniformParameter->Type = MCT_Float;
			UniformParameter->Index = ParameterIndex;
			UniformParameter->ShaderParameter = ShaderParameter;
		}
	}

	// Bind uniform vector expression parameters.
	for(INT ParameterIndex = 0;ParameterIndex < Material->UniformVectorExpressions.Num();ParameterIndex++)
	{
		FShaderParameter ShaderParameter;
		FString ParameterName = FString::Printf(TEXT("UniformVector_%u"),ParameterIndex);
		ShaderParameter.Bind(ParameterMap,*ParameterName,TRUE);
		if(ShaderParameter.IsBound())
		{
			FUniformParameter* UniformParameter = new(UniformParameters) FUniformParameter;
			UniformParameter->Type = MCT_Float4;
			UniformParameter->Index = ParameterIndex;
			UniformParameter->ShaderParameter = ShaderParameter;
		}
	}
	
	// Bind uniform 2D texture parameters.
	for(INT ParameterIndex = 0;ParameterIndex < Material->Uniform2DTextureExpressions.Num();ParameterIndex++)
	{
		FShaderParameter ShaderParameter;
		FString ParameterName = FString::Printf(TEXT("Texture2D_%u"),ParameterIndex);
		ShaderParameter.Bind(ParameterMap,*ParameterName,TRUE);
		if(ShaderParameter.IsBound())
		{
			FUniformParameter* UniformParameter = new(UniformParameters) FUniformParameter;
			UniformParameter->Type = MCT_Texture2D;
			UniformParameter->Index = ParameterIndex;
			UniformParameter->ShaderParameter = ShaderParameter;
		}
	}

	// Bind uniform cube texture parameters.
	for(INT ParameterIndex = 0;ParameterIndex < Material->UniformCubeTextureExpressions.Num();ParameterIndex++)
	{
		FShaderParameter ShaderParameter;
		FString ParameterName = FString::Printf(TEXT("TextureCube_%u"),ParameterIndex);
		ShaderParameter.Bind(ParameterMap,*ParameterName,TRUE);
		if(ShaderParameter.IsBound())
		{
			FUniformParameter* UniformParameter = new(UniformParameters) FUniformParameter;
			UniformParameter->Type = MCT_TextureCube;
			UniformParameter->Index = ParameterIndex;
			UniformParameter->ShaderParameter = ShaderParameter;
		}
	}

	// only used if Material has a Transform expression 
	LocalToWorldParameter.Bind(ParameterMap,TEXT("LocalToWorldMatrix"),TRUE);
	WorldToViewParameter.Bind(ParameterMap,TEXT("WorldToViewMatrix"),TRUE);
	WorldProjectionInvParameter.Bind(ParameterMap,TEXT("WorldProjectionInvMatrix"),TRUE);
	CameraWorldPositionParameter.Bind(ParameterMap,TEXT("CameraWorldPos"),TRUE);

	SceneTextureParameters.Bind(ParameterMap);

	// Only used for two-sided materials.
	TwoSidedSignParameter.Bind(ParameterMap,TEXT("TwoSidedSign"),TRUE);
	// Only used when material needs gamma correction
	InvGammaParameter.Bind(ParameterMap,TEXT("MatInverseGamma"),TRUE);
}

void FMaterialPixelShaderParameters::Set(FCommandContextRHI* Context,FShader* PixelShader,const FMaterialRenderContext& MaterialRenderContext) const
{
	// Set the uniform parameters.
	const FMaterial* Material = MaterialRenderContext.MaterialRenderProxy->GetMaterial();
	for(INT ParameterIndex = 0;ParameterIndex < UniformParameters.Num();ParameterIndex++)
	{
		const FUniformParameter& UniformParameter = UniformParameters(ParameterIndex);
		switch(UniformParameter.Type)
		{
		case MCT_Float:
		{
			FLinearColor Value;
			if (!Material || (UniformParameter.Index >= Material->UniformScalarExpressions.Num()))
			{
#if DO_CHECK
				static UBOOL s_bWarnedOnce_Scalar = FALSE;
				if (!s_bWarnedOnce_Scalar)
				{
					warnf(TEXT("Material accessing scalar beyond ScalarExpressions size (%d/%d) - %s"), 
						UniformParameter.Index, Material ? Material->UniformScalarExpressions.Num() : 0,
						Material ? *Material->GetFriendlyName() : TEXT("NO MATERIAL!!!"));
					s_bWarnedOnce_Scalar = TRUE;
				}
#endif	//#if DO_CHECK
				Value = FLinearColor(0.0f, 0.0f, 0.0f);
			}
			else
			{
				Material->UniformScalarExpressions(UniformParameter.Index)->GetNumberValue(MaterialRenderContext,Value);
			}
			SetPixelShaderValue(Context,PixelShader->GetPixelShader(),UniformParameter.ShaderParameter,Value.R);
			break;
		}
		case MCT_Float4:
		{
			FLinearColor Value;			
			if (!Material || (UniformParameter.Index >= Material->UniformVectorExpressions.Num()))
			{
#if DO_CHECK
				static UBOOL s_bWarnedOnce_Vector = FALSE;
				if (!s_bWarnedOnce_Vector)
				{
					warnf(TEXT("Material accessing vector beyond VectorExpressions size (%d/%d) - %s"), 
						UniformParameter.Index, Material ? Material->UniformVectorExpressions.Num() : 0,
						Material ? *Material->GetFriendlyName() : TEXT("NO MATERIAL!!!"));
					s_bWarnedOnce_Vector = TRUE;
				}
#endif	//#if DO_CHECK
				Value = FLinearColor(5.0f, 0.0f, 5.0f);
			}
			else
			{
				Material->UniformVectorExpressions(UniformParameter.Index)->GetNumberValue(MaterialRenderContext,Value);
			}
			SetPixelShaderValue(Context,PixelShader->GetPixelShader(),UniformParameter.ShaderParameter,Value);
			break;
		}
		case MCT_Texture2D:
		{
			const FTexture* Value = NULL;
			if (!Material || (UniformParameter.Index >= Material->Uniform2DTextureExpressions.Num()))
			{
#if DO_CHECK
				static UBOOL s_bWarnedOnce_Texture = FALSE;
				if (!s_bWarnedOnce_Texture)
				{
					warnf(TEXT("Material accessing texture beyond TextureExpressions size (%d/%d) - %s"), 
						UniformParameter.Index, Material ? Material->Uniform2DTextureExpressions.Num() : 0, 
						Material ? *Material->GetFriendlyName() : TEXT("NO MATERIAL!!!"));
					s_bWarnedOnce_Texture = TRUE;
				}
#endif	//#if DO_CHECK
				Value = GWhiteTexture;
			}
			if (!Value)
			{
				Material->Uniform2DTextureExpressions(UniformParameter.Index)->GetTextureValue(MaterialRenderContext,&Value);
			}
			if(!Value)
			{
				Value = GWhiteTexture;
			}
			SetTextureParameter(Context,PixelShader->GetPixelShader(),UniformParameter.ShaderParameter,Value);
			break;
		}
		case MCT_TextureCube:
		{
			const FTexture* Value = NULL;
			if (!Material || (UniformParameter.Index >= Material->UniformCubeTextureExpressions.Num()))
			{
#if DO_CHECK
				static UBOOL s_bWarnedOnce_Cube = FALSE;
				if (!s_bWarnedOnce_Cube)
				{
					warnf(TEXT("Material accessing cube texture beyond CubeTextureExpressions size (%d/%d)  - %s"), 
						UniformParameter.Index, Material ? Material->UniformCubeTextureExpressions.Num() : 0, 
						Material ? *Material->GetFriendlyName() : TEXT("NO MATERIAL!!!"));
					s_bWarnedOnce_Cube = TRUE;
				}
#endif	//#if DO_CHECK
				Value = GWhiteTextureCube;
			}
			if (!Value)
			{
				Material->UniformCubeTextureExpressions(UniformParameter.Index)->GetTextureValue(MaterialRenderContext,&Value);
			}
			if(!Value)
			{
				Value = GWhiteTextureCube;
			}
			SetTextureParameter(Context,PixelShader->GetPixelShader(),UniformParameter.ShaderParameter,Value);
			break;
		}
		};
	}

	if( Material != NULL )
	{
		checkSlow(MaterialRenderContext.View);
		checkSlow(MaterialRenderContext.View->Family);

		// set view matrix for use by view space Transform expressions
		if (Material->GetTransformsUsed() & UsedCoord_View)
		{
			SetPixelShaderValues(Context,PixelShader->GetPixelShader(),WorldToViewParameter,(FVector4*)&MaterialRenderContext.View->ViewMatrix,3);
		}

		//only set as many registers as were bound, since the compiler may have optimized some out
		// set the inverse projection transform
		SetPixelShaderValues(
			Context,
			PixelShader->GetPixelShader(),
			WorldProjectionInvParameter,
			(FVector4*)&(MaterialRenderContext.View->InvViewProjectionMatrix),
			WorldProjectionInvParameter.GetNumRegisters()
			);

		// set the camera world position transform
		SetPixelShaderValue(Context,PixelShader->GetPixelShader(),CameraWorldPositionParameter,MaterialRenderContext.View->ViewOrigin);

		if( Material->IsUsedWithGammaCorrection() )
		{			
			// set inverse gamma shader constant
			checkSlow(MaterialRenderContext.View->Family->GammaCorrection > 0.0f );
			SetPixelShaderValue( Context, PixelShader->GetPixelShader(), InvGammaParameter, 1.0f / MaterialRenderContext.View->Family->GammaCorrection );
		}
	}

	SceneTextureParameters.Set(Context,MaterialRenderContext.View,PixelShader);
}

/**
* Set local transforms for rendering a material with a single mesh
* @param Context - command context
* @param MaterialRenderContext - material specific info for setting the shader
* @param LocalToWorld - l2w for rendering a single mesh
*/
void FMaterialPixelShaderParameters::SetLocalTransforms(
	FCommandContextRHI* Context,
	FShader* PixelShader,
	const FMaterialRenderProxy* MaterialRenderProxy,
	const FMatrix& LocalToWorld,
	UBOOL bBackFace
	) const
{
	const FMaterial* Material = MaterialRenderProxy->GetMaterial();
	// set world matrix for use by world/view space Transform expressions
	if( Material && 
		(Material->GetTransformsUsed() & UsedCoord_World) ||
		(Material->GetTransformsUsed() & UsedCoord_View) )
	{
		SetPixelShaderValues(Context,PixelShader->GetPixelShader(),LocalToWorldParameter,(FVector4*)&LocalToWorld,3);
	}
	// Set the two-sided sign parameter.
	SetPixelShaderValue(Context,PixelShader->GetPixelShader(),TwoSidedSignParameter,bBackFace ? -1.0f : +1.0f);
}

FArchive& operator<<(FArchive& Ar,FMaterialPixelShaderParameters& Parameters)
{
	check(Ar.Ver() >= VER_MIN_MATERIAL_PIXELSHADER);

	Ar << Parameters.UniformParameters;
	Ar << Parameters.LocalToWorldParameter;
	Ar << Parameters.WorldToViewParameter;
	Ar << Parameters.WorldProjectionInvParameter;
	Ar << Parameters.CameraWorldPositionParameter;
	Ar << Parameters.SceneTextureParameters;
	Ar << Parameters.TwoSidedSignParameter;
	Ar << Parameters.InvGammaParameter;

	return Ar;
}

/**
* Finds a FMaterialShaderType by name.
*/
FMaterialShaderType* FMaterialShaderType::GetTypeByName(const FString& TypeName)
{
	for(TLinkedList<FShaderType*>::TIterator It(FShaderType::GetTypeList()); It; It.Next())
	{
		FString CurrentTypeName = FString(It->GetName());
		FMaterialShaderType* CurrentType = It->GetMaterialShaderType();
		if (CurrentType && CurrentTypeName == TypeName)
		{
			return CurrentType;
		}
	}
	return NULL;
}

FShader* FMaterialShaderType::CompileShader(
	EShaderPlatform Platform,
	const FMaterial* Material,
	const TCHAR* MaterialShaderCode,
	TArray<FString>& OutErrors
	)
{
	// Construct the shader environment.
	FShaderCompilerEnvironment Environment;
	Environment.IncludeFiles.Set(TEXT("Material.usf"),MaterialShaderCode);

	switch(Material->GetBlendMode())
	{
	case BLEND_Opaque: Environment.Definitions.Set(TEXT("MATERIALBLENDING_SOLID"),TEXT("1")); break;
	case BLEND_Masked: Environment.Definitions.Set(TEXT("MATERIALBLENDING_MASKED"),TEXT("1")); break;
	case BLEND_Translucent: Environment.Definitions.Set(TEXT("MATERIALBLENDING_TRANSLUCENT"),TEXT("1")); break;
	case BLEND_Additive: Environment.Definitions.Set(TEXT("MATERIALBLENDING_ADDITIVE"),TEXT("1")); break;
	case BLEND_Modulate: Environment.Definitions.Set(TEXT("MATERIALBLENDING_MODULATE"),TEXT("1")); break;
	default: appErrorf(TEXT("Unknown material blend mode: %u"),(INT)Material->GetBlendMode());
	}

	Environment.Definitions.Set(TEXT("MATERIAL_TWOSIDED"),Material->IsTwoSided() ? TEXT("1") : TEXT("0"));

	switch(Material->GetLightingModel())
	{
	case MLM_SHPRT: // For backward compatibility, treat the deprecated SHPRT lighting model as Phong.
	case MLM_Phong: Environment.Definitions.Set(TEXT("MATERIAL_LIGHTINGMODEL_PHONG"),TEXT("1")); break;
	case MLM_NonDirectional: Environment.Definitions.Set(TEXT("MATERIAL_LIGHTINGMODEL_NONDIRECTIONAL"),TEXT("1")); break;
	case MLM_Unlit: Environment.Definitions.Set(TEXT("MATERIAL_LIGHTINGMODEL_UNLIT"),TEXT("1")); break;
	case MLM_Custom: Environment.Definitions.Set(TEXT("MATERIAL_LIGHTINGMODEL_CUSTOM"),TEXT("1")); break;
	default: appErrorf(TEXT("Unknown material lighting model: %u"),(INT)Material->GetLightingModel());
	};

	if( Material->GetTransformsUsed() != UsedCoord_None )
	{
		// only use WORLD_COORDS code if a Transform expression was used by the material
		Environment.Definitions.Set(TEXT("WORLD_COORDS"),TEXT("1"));
	}

	if( Material->IsUsedWithGammaCorrection() )
	{
		// only use USE_GAMMA_CORRECTION code when enabled
		Environment.Definitions.Set(TEXT("MATERIAL_USE_GAMMA_CORRECTION"),TEXT("1"));
	}

	//calculate the CRC for this type and store it in the shader cache.
	//this will be used to determine when shader files have changed.
	DWORD CurrentCRC = GetSourceCRC();
	UShaderCache::SetShaderTypeCRC(this, CurrentCRC, Platform);

	//update material shader stats
	UpdateMaterialShaderCompilingStats(Material);

	// Compile the shader code.
	FShaderCompilerOutput Output;
	if(!FShaderType::CompileShader(Platform,Environment,Output))
	{
		OutErrors = Output.Errors;
		return NULL;
	}

	// Check for shaders with identical compiled code.
	FShader* Shader = FindShaderByOutput(Output);
	if(!Shader)
	{
		// Create the shader.
		Shader = (*ConstructCompiledRef)(CompiledShaderInitializerType(this,Output,Material));
	}
	return Shader;
}

/**
* Calculates a CRC based on this shader type's source code and includes
*/
DWORD FMaterialShaderType::GetSourceCRC()
{
	DWORD SourceCRC = FShaderType::GetSourceCRC();
	//manually include MaterialTemplate.usf since it is included as "Material.usf"
	SourceCRC ^= GetShaderFileCRC(TEXT("MaterialTemplate"));
	return SourceCRC;
}

/**
* Finds the shader map for a material.
* @param StaticParameterSet - The static parameter set identifying the shader map
* @param Platform - The platform to lookup for
* @return NULL if no cached shader map was found.
*/
FMaterialShaderMap* FMaterialShaderMap::FindId(const FStaticParameterSet& StaticParameterSet, EShaderPlatform Platform)
{
	return GIdToMaterialShaderMap[Platform].FindRef(StaticParameterSet);
}

FMaterialShaderMap::~FMaterialShaderMap()
{
	GIdToMaterialShaderMap[Platform].Remove(StaticParameters);
}

/**
* Compiles the shaders for a material and caches them in this shader map.
* @param Material - The material to compile shaders for.
* @param InStaticParameters - the set of static parameters to compile for
* @param MaterialShaderCode - The shader code for Material.
* @param Platform - The platform to compile to
* @param OutErrors - Upon compilation failure, OutErrors contains a list of the errors which occured.
* @return True if the compilation succeeded.
*/
UBOOL FMaterialShaderMap::Compile(const FMaterial* Material, const FStaticParameterSet* InStaticParameters, const TCHAR* MaterialShaderCode,EShaderPlatform InPlatform,TArray<FString>& OutErrors,UBOOL bSilent)
{
#if CONSOLE
	appErrorf( TEXT("Trying to compile %s at run-time, which is not supported on consoles!"), *Material->GetFriendlyName() );
	return FALSE;
#else
	UBOOL bSuccess = TRUE;

	// Store the material name for debugging purposes.
	FriendlyName = Material->GetFriendlyName();

#if DUMP_MATERIAL_SHADER_STATS
	warnf(TEXT("	Compiling %s:	%s, %s, Special=%u, StaticLit=%u, Particle=%u, Terrain=%u, Decal=%u, Skinned=%u, LensFlare=%u, GammaCorrected=%u"), 
		*FriendlyName, *GetLightingModelString(Material->GetLightingModel()), *GetBlendModeString(Material->GetBlendMode()), 
		Material->IsSpecialEngineMaterial(), Material->IsUsedWithStaticLighting(), (UBOOL)Material->IsUsedWithParticleSystem(), 
		Material->IsTerrainMaterial(), Material->IsDecalMaterial(), Material->IsUsedWithSkeletalMesh(), 
		Material->IsUsedWithLensFlare(),Material->IsUsedWithGammaCorrection());
#endif

	// Iterate over all vertex factory types.
	for(TLinkedList<FVertexFactoryType*>::TIterator VertexFactoryTypeIt(FVertexFactoryType::GetTypeList());VertexFactoryTypeIt;VertexFactoryTypeIt.Next())
	{
		FVertexFactoryType* VertexFactoryType = *VertexFactoryTypeIt;
		check(VertexFactoryType);

		if(VertexFactoryType->IsUsedWithMaterials())
		{
			FMeshMaterialShaderMap* MeshShaderMap = NULL;

			// look for existing map for this vertex factory type
			for (INT ShaderMapIndex = 0; ShaderMapIndex < MeshShaderMaps.Num(); ShaderMapIndex++)
			{
				if (MeshShaderMaps(ShaderMapIndex).GetVertexFactoryType() == VertexFactoryType)
				{
					MeshShaderMap = &MeshShaderMaps(ShaderMapIndex);
					break;
				}
			}

			if (MeshShaderMap == NULL)
			{
				// Create a new mesh material shader map.
				MeshShaderMap = new(MeshShaderMaps) FMeshMaterialShaderMap;
			}

			// Compile all mesh material shaders for this material and vertex factory type combo, and cache them in the shader map.
			if(!MeshShaderMap->Compile(Material,MaterialShaderCode,VertexFactoryType,InPlatform,OutErrors,bSilent))
			{
				bSuccess = FALSE;
				break;
			}
		}
	}

	// Iterate over all material shader types.
	for(TLinkedList<FShaderType*>::TIterator ShaderTypeIt(FShaderType::GetTypeList());ShaderTypeIt;ShaderTypeIt.Next())
	{
		FMaterialShaderType* ShaderType = ShaderTypeIt->GetMaterialShaderType();
		if (ShaderType && 
			ShaderType->ShouldCache(InPlatform,Material) && 
			Material->ShouldCache(InPlatform, ShaderType, NULL)
			)
		{
			// Compile this material shader for this material.
			TArray<FString> ShaderErrors;

			// only compile the shader if we don't already have it
			if (!HasShader(ShaderType))
			{
			    FShader* Shader = ShaderType->CompileShader(InPlatform,Material,MaterialShaderCode,ShaderErrors);
			    if(Shader)
			    {
				    AddShader(ShaderType,Shader);
			    }
			    else
			    {
				    for(INT ErrorIndex = 0;ErrorIndex < ShaderErrors.Num();ErrorIndex++)
				    {
					    OutErrors.AddUniqueItem(ShaderErrors(ErrorIndex));
				    }
				    bSuccess = FALSE;
					break;
			    }
			}
		}
	}

	// Reinitialize the vertex factory map.
	InitVertexFactoryMap();

	// Register this shader map in the global map with the material's GUID.
	MaterialId = Material->GetId();
	StaticParameters = *InStaticParameters;
	Platform = InPlatform;

	GIdToMaterialShaderMap[Platform].Set(StaticParameters,this);

	// Add the persistent shaders to the local shader cache.
	if (Material->IsPersistent())
	{
		GetLocalShaderCache(Platform)->AddMaterialShaderMap(this);
	}

	return bSuccess;
#endif
}

// @debug
UBOOL GDumpIsComplete=FALSE;
UBOOL FMaterialShaderMap::IsComplete(const FMaterial* Material) const
{
	UBOOL bIsComplete = TRUE;

	// Iterate over all vertex factory types.
	for(TLinkedList<FVertexFactoryType*>::TIterator VertexFactoryTypeIt(FVertexFactoryType::GetTypeList());VertexFactoryTypeIt;VertexFactoryTypeIt.Next())
	{
		FVertexFactoryType* VertexFactoryType = *VertexFactoryTypeIt;

		if(VertexFactoryType->IsUsedWithMaterials())
		{
			// Find the shaders for this vertex factory type.
			const FMeshMaterialShaderMap* MeshShaderMap = GetMeshShaderMap(VertexFactoryType);
			if(!MeshShaderMap || !MeshShaderMap->IsComplete(Platform,Material,VertexFactoryType))
			{
				// @debug
				if (GDumpIsComplete && !MeshShaderMap) 
				{
					debugf(TEXT("Don't have VF %s"), VertexFactoryType->GetName());
				}
				bIsComplete = FALSE;
				break;
			}
		}
	}

	// Iterate over all material shader types.
	for(TLinkedList<FShaderType*>::TIterator ShaderTypeIt(FShaderType::GetTypeList());ShaderTypeIt;ShaderTypeIt.Next())
	{
		// Find this shader type in the material's shader map.
		FMaterialShaderType* ShaderType = ShaderTypeIt->GetMaterialShaderType();
		if (ShaderType && 
			ShaderType->ShouldCache(Platform,Material) && 
			Material->ShouldCache(Platform, ShaderType, NULL) &&
			!HasShader(ShaderType)
			)
		{
			// @debug
			if (GDumpIsComplete)
			{
				debugf(TEXT("Don't have Shader %s"), ShaderType->GetName());
			}
			bIsComplete = FALSE;
			break;
		}
	}

	return bIsComplete;
}

void FMaterialShaderMap::GetShaderList(TMap<FGuid,FShader*>& OutShaders) const
{
	TShaderMap<FMaterialShaderType>::GetShaderList(OutShaders);
	for(INT Index = 0;Index < MeshShaderMaps.Num();Index++)
	{
		MeshShaderMaps(Index).GetShaderList(OutShaders);
	}
}

void FMaterialShaderMap::BeginInit()
{
	TShaderMap<FMaterialShaderType>::BeginInit();
	for(INT MapIndex = 0;MapIndex < MeshShaderMaps.Num();MapIndex++)
	{
		MeshShaderMaps(MapIndex).BeginInit();
	}
}

/**
 * Removes all entries in the cache with exceptions based on a shader type
 * @param ShaderType - The shader type to flush or keep (depending on second param)
 * @param bFlushAllButShaderType - TRUE if all shaders EXCEPT the given type should be flush. FALSE will flush ONLY the given shader type
 */
void FMaterialShaderMap::FlushShadersByShaderType(FShaderType* ShaderType, UBOOL bFlushAllButShaderType)
{
	// flush from all the vertex factory shader maps
	for(INT Index = 0;Index < MeshShaderMaps.Num();Index++)
	{
		MeshShaderMaps(Index).FlushShadersByShaderType(ShaderType, bFlushAllButShaderType);
	}

	// flush if flushing all but the given type, go over other shader types and remove them
	if (bFlushAllButShaderType)
	{
		// flush from this shader map
		for(TLinkedList<FShaderType*>::TIterator ShaderTypeIt(FShaderType::GetTypeList());ShaderTypeIt;ShaderTypeIt.Next())
		{
			if (ShaderType != *ShaderTypeIt && ShaderTypeIt->GetMaterialShaderType())
			{
				RemoveShaderType(ShaderTypeIt->GetMaterialShaderType());
			}
		}
	}
	// otherwise just remove this type
	else if (ShaderType->GetMaterialShaderType())
	{
		RemoveShaderType(ShaderType->GetMaterialShaderType());	
	}
}

/**
 * Removes all entries in the cache with exceptions based on a vertex factory type
 * @param ShaderType - The shader type to flush or keep (depending on second param)
 * @param bFlushAllButVertexFactoryType - TRUE if all shaders EXCEPT the given type should be flush. FALSE will flush ONLY the given vertex factory type
 */
void FMaterialShaderMap::FlushShadersByVertexFactoryType(FVertexFactoryType* VertexFactoryType, UBOOL bFlushAllButVertexFactoryType)
{
	for(INT Index = 0;Index < MeshShaderMaps.Num();Index++)
	{
		FVertexFactoryType* VFType = MeshShaderMaps(Index).GetVertexFactoryType();
		// determine if this shaders vertex factory type should be flushed
		if ((bFlushAllButVertexFactoryType && VFType != VertexFactoryType) ||
			(!bFlushAllButVertexFactoryType && VFType == VertexFactoryType))
		{
			// remove the shader map
			MeshShaderMaps.Remove(Index);
			// fix up the counter
			Index--;
		}
	}

	// reset the vertex factory map to remove references to the removed maps
	InitVertexFactoryMap();
}

void FMaterialShaderMap::Serialize(FArchive& Ar)
{
	TShaderMap<FMaterialShaderType>::Serialize(Ar);
	Ar << MeshShaderMaps;
	Ar << MaterialId;
	Ar << FriendlyName;

	if (Ar.Ver() >= VER_STATIC_MATERIAL_PARAMETERS)
	{
		StaticParameters.Serialize(Ar);
	}
	else
	{
		StaticParameters.BaseMaterialId = MaterialId;
	}

	// serialize the platform enum as a BYTE
	INT TempPlatform = (INT)Platform;
	Ar << TempPlatform;
	Platform = (EShaderPlatform)TempPlatform;

	if(Ar.IsLoading())
	{
		// When loading, reinitialize VertexFactoryShaderMap from the new contents of VertexFactoryShaders.
		InitVertexFactoryMap();

		// Register this shader map in the global map with the material's ID.
		GIdToMaterialShaderMap[Platform].Set(StaticParameters,this);
	}
}

const FMeshMaterialShaderMap* FMaterialShaderMap::GetMeshShaderMap(FVertexFactoryType* VertexFactoryType) const
{
	FMeshMaterialShaderMap* MeshShaderIndex = VertexFactoryMap.FindRef(VertexFactoryType);
	if(!MeshShaderIndex)
	{
		return NULL;
	}
	return MeshShaderIndex;
}

void FMaterialShaderMap::InitVertexFactoryMap()
{
	VertexFactoryMap.Empty();
	for(INT Index = 0;Index < MeshShaderMaps.Num();Index++)
	{
		VertexFactoryMap.Set(MeshShaderMaps(Index).GetVertexFactoryType(),&MeshShaderMaps(Index));
	}
}
