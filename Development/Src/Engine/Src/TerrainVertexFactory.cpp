/*=============================================================================
	TerrainVertexFactory.cpp: Terrain vertex factory implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnTerrain.h"
#include "UnTerrainRender.h"

/** Vertex factory with vertex stream components for terrain vertices */
// FRenderResource interface.
void FTerrainVertexFactory::InitRHI()
{
	// list of declaration items
	FVertexDeclarationElementList Elements;

	// position decls
	Elements.AddItem(AccessStreamComponent(Data.PositionComponent, VEU_Position));
	// displacement
	Elements.AddItem(AccessStreamComponent(Data.DisplacementComponent, VEU_BlendWeight));
	// gradients
	Elements.AddItem(AccessStreamComponent(Data.GradientComponent, VEU_Tangent));

	// create the actual device decls
	//@todo.SAS. Include shadow map and light map
	InitDeclaration(Elements,FVertexFactory::DataType(),FALSE,FALSE);
}

/**
 *	Initialize the component streams.
 *	
 *	@param	Buffer	Pointer to the vertex buffer that will hold the data streams.
 *	@param	Stride	The stride of the provided vertex buffer.
 */
UBOOL FTerrainVertexFactory::InitComponentStreams(FTerrainVertexBuffer* Buffer)
{
	INT Stride = sizeof(FTerrainVertex);
	// update vertex factory components and sync it
	TSetResourceDataContext<FTerrainVertexFactory> VertexFactoryData(this);
	// position
	VertexFactoryData->PositionComponent = FVertexStreamComponent(
#if UBYTE4_BYTEORDER_XYZW
		Buffer, STRUCT_OFFSET(FTerrainVertex, X), Stride, VET_UByte4);
#else
		Buffer, STRUCT_OFFSET(FTerrainVertex, Z_HIBYTE), Stride, VET_UByte4);
#endif
	// displacement
	VertexFactoryData->DisplacementComponent = FVertexStreamComponent(
		Buffer, STRUCT_OFFSET(FTerrainVertex, Displacement), Stride, VET_Float1);
	// gradients
	VertexFactoryData->GradientComponent = FVertexStreamComponent(
		Buffer, STRUCT_OFFSET(FTerrainVertex, GradientX), Stride, VET_Short2);

	// copy it
	VertexFactoryData.Commit();

	return TRUE;
}

/** Shader parameters for use with FTerrainVertexFactory */
class FTerrainVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	/**
	 * Bind shader constants by name
	 * @param	ParameterMap - mapping of named shader constants to indices
	 */
	virtual void Bind(const FShaderParameterMap& ParameterMap)
	{
		LocalToWorldParameter.Bind(ParameterMap,TEXT("LocalToWorld"));
		WorldToLocalParameter.Bind(ParameterMap,TEXT("WorldToLocal"),TRUE);
		LocalToViewParameter.Bind(ParameterMap,TEXT("LocalToView"),TRUE);
		ShadowCoordinateScaleBiasParameter.Bind(ParameterMap,TEXT("ShadowCoordinateScaleBias"),TRUE);
		TessellationInterpolation_Parameter.Bind(ParameterMap,TEXT("TessellationInterpolation"), TRUE);
		InvMaxTessLevel_ZScale_Parameter.Bind(ParameterMap,TEXT("InvMaxTesselationLevel_ZScale"), TRUE);
		InvTerrainSize_SectionBase_Parameter.Bind(ParameterMap,TEXT("InvTerrainSize_SectionBase"), TRUE);
		LightMapCoordScaleBiasParameter.Bind(ParameterMap,TEXT("LightMapCoordinateScaleBias"), TRUE);
		TessellationDistanceScaleParameter.Bind(ParameterMap,TEXT("TessellationDistanceScale"), TRUE);
		TessInterpDistanceValuesParameter.Bind(ParameterMap,TEXT("TessInterpDistanceValues"),TRUE);
	}

	/**
	 * Serialize shader params to an archive
	 * @param	Ar - archive to serialize to
	 */
	virtual void Serialize(FArchive& Ar)
	{
		Ar << LocalToWorldParameter;
		Ar << WorldToLocalParameter;
		if (Ar.Ver() >= VER_TERRAIN_MORPHING_OPTION)
		{
			Ar << LocalToViewParameter;
		}
		Ar << ShadowCoordinateScaleBiasParameter;
		if (Ar.Ver() >= VER_TERRAIN_MORPHING_OPTION)
		{
			Ar << TessellationInterpolation_Parameter;
		}
		Ar << InvMaxTessLevel_ZScale_Parameter;
		Ar << InvTerrainSize_SectionBase_Parameter;
		Ar << LightMapCoordScaleBiasParameter;
		if (Ar.Ver() >= VER_TERRAIN_MORPHING_OPTION)
		{
			Ar << TessellationDistanceScaleParameter;
		}
		if (Ar.Ver() >= VER_PS3_MORPH_TERRAIN)
		{
			Ar << TessInterpDistanceValuesParameter;
		}
	}

	/**
	 * Set any shader data specific to this vertex factory
	 */
	virtual void Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const
	{
		FTerrainVertexFactory* TerrainVF = (FTerrainVertexFactory*)VertexFactory;
		FTerrainObject* TerrainObject = TerrainVF->GetTerrainObject();

		if (LocalToViewParameter.IsBound())
		{
			FMatrix Value = TerrainObject->GetLocalToWorld() * View->ViewMatrix;
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), LocalToViewParameter, Value);
		}

		if (TessellationInterpolation_Parameter.IsBound())
		{
			FLOAT Value = 1.0f;
			SetVertexShaderValue(Context,VertexShader->GetVertexShader(),TessellationInterpolation_Parameter,Value);
		}

		if (InvMaxTessLevel_ZScale_Parameter.IsBound())
		{
			FVector2D Value(
					1.0f, //1.0f / (FLOAT)(TerrainVF->GetTessellationLevel())
					TerrainObject->GetTerrainHeightScale()
				);
			SetVertexShaderValue(Context,VertexShader->GetVertexShader(),InvMaxTessLevel_ZScale_Parameter,Value);
		}
		if (InvTerrainSize_SectionBase_Parameter.IsBound())
		{
			FVector4 Value;
			if (GPlatformNeedsPowerOfTwoTextures) // power of two
			{
				Value.X = 1.0f / (1 << appCeilLogTwo(TerrainObject->GetNumVerticesX()));
				Value.Y = 1.0f / (1 << appCeilLogTwo(TerrainObject->GetNumVerticesY()));
			}
			else
			{
				Value.X = 1.0f / TerrainObject->GetNumVerticesX();
				Value.Y = 1.0f / TerrainObject->GetNumVerticesY();
			}
			Value.Z = TerrainObject->GetComponentSectionBaseX();
			Value.W = TerrainObject->GetComponentSectionBaseY();
			SetVertexShaderValue(Context,VertexShader->GetVertexShader(),InvTerrainSize_SectionBase_Parameter,Value);
		}
		if (LightMapCoordScaleBiasParameter.IsBound())
		{
			FVector4 Value;

			INT LightMapRes = TerrainObject->GetLightMapResolution();
			Value.X = (FLOAT)LightMapRes / ((FLOAT)TerrainObject->GetComponentTrueSectionSizeX() * (FLOAT)LightMapRes + 1.0f);
			Value.Y = (FLOAT)LightMapRes / ((FLOAT)TerrainObject->GetComponentTrueSectionSizeY() * (FLOAT)LightMapRes + 1.0f);
			Value.Z = 0.0f;
			Value.W = 0.0f;
			SetVertexShaderValue(Context,VertexShader->GetVertexShader(),LightMapCoordScaleBiasParameter,Value);
		}
		if (ShadowCoordinateScaleBiasParameter.IsBound())
		{
			FVector4 Value(
				TerrainObject->GetShadowCoordinateScaleX(),
				TerrainObject->GetShadowCoordinateScaleY(),
				TerrainObject->GetShadowCoordinateBiasY(),
				TerrainObject->GetShadowCoordinateBiasX()
				);
			SetVertexShaderValue(Context,VertexShader->GetVertexShader(),ShadowCoordinateScaleBiasParameter,Value);
		}
		if (TessellationDistanceScaleParameter.IsBound())
		{
			FVector4 Value(TerrainObject->GetTessellationDistanceScale(), 0.0f, 0.0f, 0.0f);
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), TessellationDistanceScaleParameter, Value);
		}
		if (TessInterpDistanceValuesParameter.IsBound())
		{
			static const FLOAT TessInterpDistanceValues[][4] =
			{
				{     0.0f,    -1.0f, 0.0f, 0.0f },		// Highest tessellation level - NEVER goes away
				{ 16384.0f, 16384.0f, 0.0f, 0.0f },		// 1 level  up (16384 .. 32768)
				{  8192.0f,  8192.0f, 0.0f, 0.0f },		// 2 levels up ( 8192 .. 16384)
				{  4096.0f,  4096.0f, 0.0f, 0.0f },		// 3 levels up ( 4096 ..  8192)
				{     0.0f,  4096.0f, 0.0f, 0.0f }		// 4 levels up (    0 ..  4096)
			};
			SetVertexShaderValues(Context, VertexShader->GetVertexShader(), TessInterpDistanceValuesParameter, TessInterpDistanceValues, 5);
		}
	}

	/**
	 * 
	 */
	virtual void SetLocalTransforms(FCommandContextRHI* Context,FShader* VertexShader,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal) const
	{
		SetVertexShaderValue(Context,VertexShader->GetVertexShader(),LocalToWorldParameter,LocalToWorld);
		SetVertexShaderValues(Context,VertexShader->GetVertexShader(),WorldToLocalParameter,(FVector4*)&WorldToLocal,3);
	}

private:
	INT	TessellationLevel;
	FShaderParameter LocalToWorldParameter;
	FShaderParameter WorldToLocalParameter;
	FShaderParameter LocalToViewParameter;
	FShaderParameter ShadowCoordinateScaleBiasParameter;
	FShaderParameter TessellationInterpolation_Parameter;
	FShaderParameter InvMaxTessLevel_ZScale_Parameter;
	FShaderParameter InvTerrainSize_SectionBase_Parameter;
	FShaderParameter LightMapCoordScaleBiasParameter;
	FShaderParameter TessellationDistanceScaleParameter;
	FShaderParameter TessInterpDistanceValuesParameter;
};

/** bind terrain vertex factory to its shader file and its shader parameters */
IMPLEMENT_VERTEX_FACTORY_TYPE(FTerrainVertexFactory, FTerrainVertexFactoryShaderParameters, "TerrainVertexFactory", TRUE, TRUE, VER_PS3_MORPH_TERRAIN,0);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FTerrainDecalVertexFactory
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FTerrainDecalVertexFactoryShaderParameters : public FTerrainVertexFactoryShaderParameters
{
public:
	typedef FTerrainVertexFactoryShaderParameters Super;

	/**
	 * Bind shader constants by name
	 * @param	ParameterMap - mapping of named shader constants to indices
	 */
	virtual void Bind(const FShaderParameterMap& ParameterMap)
	{
		Super::Bind( ParameterMap );
		WorldToDecalParameter.Bind( ParameterMap, TEXT("WorldToDecal"), TRUE );
		DecalLocationParameter.Bind( ParameterMap, TEXT("DecalLocation"), TRUE );
		DecalOffsetParameter.Bind( ParameterMap, TEXT("DecalOffset"), TRUE );
		DecalLocalBinormal.Bind( ParameterMap, TEXT("DecalLocalBinormal"), TRUE );
		DecalLocalTangent.Bind( ParameterMap, TEXT("DecalLocalTangent"), TRUE );
	}

	/**
	* Serialize shader params to an archive
	* @param	Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar)
	{
		Super::Serialize( Ar );
		Ar << WorldToDecalParameter;
		Ar << DecalLocationParameter;
		Ar << DecalOffsetParameter;
		if ( Ar.Ver() >= VER_TERRAIN_DECAL_TANGENTS )
		{
			Ar << DecalLocalBinormal;
			Ar << DecalLocalTangent;
		}
	}

	/**
	 * Set any shader data specific to this vertex factory
	 */
	virtual void Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const
	{
		Super::Set( Context, VertexShader, VertexFactory, View );

		FTerrainVertexFactory* TerrainVF = (FTerrainVertexFactory*)VertexFactory;
		FTerrainDecalVertexFactoryBase* DecalBase = TerrainVF->CastTFTerrainDecalVertexFactoryBase();
		if (DecalBase)
		{
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), WorldToDecalParameter, DecalBase->GetDecalMatrix() );
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), DecalLocationParameter, DecalBase->GetDecalLocation() );
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), DecalOffsetParameter, DecalBase->GetDecalOffset() );
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), DecalLocalBinormal, DecalBase->GetDecalLocalBinormal() );
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), DecalLocalTangent, DecalBase->GetDecalLocalTangent() );
		}
	}

private:
	FShaderParameter WorldToDecalParameter;
	FShaderParameter DecalLocationParameter;
	FShaderParameter DecalOffsetParameter;
	FShaderParameter DecalLocalBinormal;
	FShaderParameter DecalLocalTangent;
};

/**
 * Should we cache the material's shader type on this platform with this vertex factory? 
 */
UBOOL FTerrainDecalVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	// Only compile decal materials and special engine materials for a terrain decal vertex factory.
	// The special engine materials must be compiled for the terrain decal vertex factory because they are used with it for wireframe, etc.
	if ( Material->IsDecalMaterial() || Material->IsSpecialEngineMaterial() )
	{
		if ( !appStrstr(ShaderType->GetName(),TEXT("VertexLightMapPolicy")) )
		{
			return TRUE;
		}
	}
	return FALSE;
}

/** bind terrain decal vertex factory to its shader file and its shader parameters */
IMPLEMENT_VERTEX_FACTORY_TYPE(FTerrainDecalVertexFactory, FTerrainDecalVertexFactoryShaderParameters, "TerrainVertexFactory", TRUE, TRUE, VER_TERRAIN_DECAL_TANGENTS,0);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FTerrainMorphVertexFactory
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FRenderResource interface.
void FTerrainMorphVertexFactory::InitRHI()
{
	// list of declaration items
	FVertexDeclarationElementList Elements;

	// position decls
	Elements.AddItem(AccessStreamComponent(Data.PositionComponent, VEU_Position));
	// displacement
	Elements.AddItem(AccessStreamComponent(Data.DisplacementComponent, VEU_BlendWeight));
	// gradients
	Elements.AddItem(AccessStreamComponent(Data.GradientComponent, VEU_Tangent));
	// height transitions
	Elements.AddItem(AccessStreamComponent(Data.HeightTransitionComponent, VEU_Normal));

	// create the actual device decls
	//@todo.SAS. Include shadow map and light map
	InitDeclaration(Elements,FVertexFactory::DataType(),FALSE,FALSE);
}

/**
 *	Initialize the component streams.
 *	
 *	@param	Buffer	Pointer to the vertex buffer that will hold the data streams.
 *	@param	Stride	The stride of the provided vertex buffer.
 */
UBOOL FTerrainMorphVertexFactory::InitComponentStreams(FTerrainVertexBuffer* Buffer)
{
	INT Stride = sizeof(FTerrainMorphingVertex);

	// update vertex factory components and sync it
	TSetResourceDataContext<FTerrainMorphVertexFactory> VertexFactoryData(this);
	// position
	VertexFactoryData->PositionComponent = FVertexStreamComponent(
#if UBYTE4_BYTEORDER_XYZW
		Buffer, STRUCT_OFFSET(FTerrainVertex, X), Stride, VET_UByte4);
#else
		Buffer, STRUCT_OFFSET(FTerrainVertex, Z_HIBYTE), Stride, VET_UByte4);
#endif
	// displacement
	VertexFactoryData->DisplacementComponent = FVertexStreamComponent(
		Buffer, STRUCT_OFFSET(FTerrainVertex, Displacement), Stride, VET_Float1);
	// gradients
	VertexFactoryData->GradientComponent = FVertexStreamComponent(
		Buffer, STRUCT_OFFSET(FTerrainVertex, GradientX), Stride, VET_Short2);
	// Transitions
	VertexFactoryData->HeightTransitionComponent = FVertexStreamComponent(
#if UBYTE4_BYTEORDER_XYZW
		Buffer, STRUCT_OFFSET(FTerrainMorphingVertex, TESS_DATA_INDEX_LO), Stride, VET_UByte4);
#else	//#if UBYTE4_BYTEORDER_XYZW
		Buffer, STRUCT_OFFSET(FTerrainMorphingVertex, Z_TRANS_HIBYTE), Stride, VET_UByte4);
#endif	//#if UBYTE4_BYTEORDER_XYZW

	// copy it
	VertexFactoryData.Commit();

	return TRUE;
}

/** bind terrain vertex factory to its shader file and its shader parameters */
IMPLEMENT_VERTEX_FACTORY_TYPE(FTerrainMorphVertexFactory, FTerrainVertexFactoryShaderParameters, "TerrainVertexFactory", TRUE, TRUE, VER_PS3_MORPH_TERRAIN,0);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FTerrainMorphDecalVertexFactory
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Should we cache the material's shader type on this platform with this vertex factory? 
 */
UBOOL FTerrainMorphDecalVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	// Only compile decal materials and special engine materials for a terrain decal vertex factory.
	// The special engine materials must be compiled for the terrain decal vertex factory because they are used with it for wireframe, etc.
	if ( Material->IsDecalMaterial() || Material->IsSpecialEngineMaterial() )
	{
		if ( !appStrstr(ShaderType->GetName(),TEXT("VertexLightMapPolicy")) )
		{
			return TRUE;
		}
	}
	return FALSE;
}

/** bind terrain decal vertex factory to its shader file and its shader parameters */
IMPLEMENT_VERTEX_FACTORY_TYPE(FTerrainMorphDecalVertexFactory, FTerrainDecalVertexFactoryShaderParameters, "TerrainVertexFactory", TRUE, TRUE, VER_TERRAIN_DECAL_TANGENTS,0);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FTerrainFullMorphVertexFactory
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FRenderResource interface.
void FTerrainFullMorphVertexFactory::InitRHI()
{
	// list of declaration items
	FVertexDeclarationElementList Elements;

	// position decls
	Elements.AddItem(AccessStreamComponent(Data.PositionComponent, VEU_Position));
	// displacement
	Elements.AddItem(AccessStreamComponent(Data.DisplacementComponent, VEU_BlendWeight));
	// gradients
	Elements.AddItem(AccessStreamComponent(Data.GradientComponent, VEU_Tangent));
	// height transitions
	Elements.AddItem(AccessStreamComponent(Data.HeightTransitionComponent, VEU_Normal));
	// gradient transitions
	Elements.AddItem(AccessStreamComponent(Data.GradientTransitionComponent, VEU_Binormal));

	// create the actual device decls
	//@todo.SAS. Include shadow map and light map
	InitDeclaration(Elements,FVertexFactory::DataType(),FALSE,FALSE);
}

/**
 *	Initialize the component streams.
 *	
 *	@param	Buffer	Pointer to the vertex buffer that will hold the data streams.
 *	@param	Stride	The stride of the provided vertex buffer.
 */
UBOOL FTerrainFullMorphVertexFactory::InitComponentStreams(FTerrainVertexBuffer* Buffer)
{
	INT Stride = sizeof(FTerrainFullMorphingVertex);

	// update vertex factory components and sync it
	TSetResourceDataContext<FTerrainFullMorphVertexFactory> VertexFactoryData(this);
	// position
	VertexFactoryData->PositionComponent = FVertexStreamComponent(
#if UBYTE4_BYTEORDER_XYZW
		Buffer, STRUCT_OFFSET(FTerrainVertex, X), Stride, VET_UByte4);
#else
		Buffer, STRUCT_OFFSET(FTerrainVertex, Z_HIBYTE), Stride, VET_UByte4);
#endif
	// displacement
	VertexFactoryData->DisplacementComponent = FVertexStreamComponent(
		Buffer, STRUCT_OFFSET(FTerrainVertex, Displacement), Stride, VET_Float1);
	// gradients
	VertexFactoryData->GradientComponent = FVertexStreamComponent(
		Buffer, STRUCT_OFFSET(FTerrainVertex, GradientX), Stride, VET_Short2);
	// Transitions
	VertexFactoryData->HeightTransitionComponent = FVertexStreamComponent(
#if UBYTE4_BYTEORDER_XYZW
		Buffer, STRUCT_OFFSET(FTerrainFullMorphingVertex, TESS_DATA_INDEX_LO), Stride, VET_UByte4);
#else	//#if UBYTE4_BYTEORDER_XYZW
		Buffer, STRUCT_OFFSET(FTerrainFullMorphingVertex, Z_TRANS_HIBYTE), Stride, VET_UByte4);
#endif	//#if UBYTE4_BYTEORDER_XYZW
	VertexFactoryData->GradientTransitionComponent = FVertexStreamComponent(
		Buffer, STRUCT_OFFSET(FTerrainFullMorphingVertex, TransGradientX), Stride, VET_Short2);

	// copy it
	VertexFactoryData.Commit();

	return TRUE;
}

/** bind terrain vertex factory to its shader file and its shader parameters */
IMPLEMENT_VERTEX_FACTORY_TYPE(FTerrainFullMorphVertexFactory, FTerrainVertexFactoryShaderParameters, "TerrainVertexFactory", TRUE, TRUE, VER_PS3_MORPH_TERRAIN,0);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FTerrainFullMorphDecalVertexFactory
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Should we cache the material's shader type on this platform with this vertex factory? 
 */
UBOOL FTerrainFullMorphDecalVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	// Only compile decal materials and special engine materials for a terrain decal vertex factory.
	// The special engine materials must be compiled for the terrain decal vertex factory because they are used with it for wireframe, etc.
	if ( Material->IsDecalMaterial() || Material->IsSpecialEngineMaterial() )
	{
		if ( !appStrstr(ShaderType->GetName(),TEXT("VertexLightMapPolicy")) )
		{
			return TRUE;
		}
	}
	return FALSE;
}

/** bind terrain decal vertex factory to its shader file and its shader parameters */
IMPLEMENT_VERTEX_FACTORY_TYPE(FTerrainFullMorphDecalVertexFactory, FTerrainDecalVertexFactoryShaderParameters, "TerrainVertexFactory", TRUE, TRUE, VER_TERRAIN_DECAL_TANGENTS,0);
