/*=============================================================================
	FoliageVertexFactory.h: Foliage vertex factory definition.
	Copyright 2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_FOLIAGEVERTEXFACTORY
#define _INC_FOLIAGEVERTEXFACTORY

/** The shader parameters for the foliage vertex factory. */
class FFoliageVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:

	// FVertexFactoryShaderParameters interface.
	virtual void Bind(const FShaderParameterMap& ParameterMap);
	virtual void Serialize(FArchive& Ar);
	virtual void Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const;
	virtual void SetLocalTransforms(FCommandContextRHI* Context,FShader* VertexShader,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal) const;

private:

	FShaderParameter InvNumVerticesPerInstanceParameter;
	FShaderParameter NumVerticesPerInstanceParameter;
};

/**
 * A vertex factory used to render instanced foliage meshes.
 */
class FFoliageVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FFoliageVertexFactory);
public:

	struct DataType : public FVertexFactory::DataType
	{
		/** The stream to read the vertex position from. */
		FVertexStreamComponent PositionComponent;

		/** The streams to read the tangent basis from. */
		FVertexStreamComponent TangentBasisComponents[3];

		/** The streams to read the texture coordinates from. */
		FVertexStreamComponent TextureCoordinateComponent;

		/** The stream to read the shadow map texture coordinates from. */
		FVertexStreamComponent ShadowMapCoordinateComponent;

		/** The stream to read the instance offset from. */
		FVertexStreamComponent InstanceOffsetComponent;

		/** The stream to read the instance axes from. */
		FVertexStreamComponent InstanceAxisComponents[3];
	};

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	/**
	 * An implementation of the interface used by TSynchronizedResource to update the resource with new data from the game thread.
	 */
	void SetData(const DataType& InData)
	{
		Data = InData;
		UpdateRHI();
	}

	// FRenderResource interface.
	virtual void InitRHI();

private:
	DataType Data;
};

#endif
