/*=============================================================================
	LocalVertexFactory.h: Local vertex factory definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * A vertex factory which simply transforms explicit vertex attributes from local to world space.
 */
class FLocalVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FLocalVertexFactory);
public:

	struct DataType : public FVertexFactory::DataType
	{
		/** The stream to read the vertex position from. */
		FVertexStreamComponent PositionComponent;

		/** The streams to read the tangent basis from. */
		FVertexStreamComponent TangentBasisComponents[2];

		/** The streams to read the texture coordinates from. */
		TStaticArray<FVertexStreamComponent,MAX_TEXCOORDS> TextureCoordinates;

		/** The stream to read the shadow map texture coordinates from. */
		FVertexStreamComponent ShadowMapCoordinateComponent;

		/** The stream to read the vertex color from. */
		FVertexStreamComponent ColorComponent;
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
