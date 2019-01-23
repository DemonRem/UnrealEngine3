/*=============================================================================
	TerrainVertexFactory.h: Terrain vertex factory definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
struct FTerrainObject;

// Forward declaration of the terrain vertex buffer...
struct FTerrainVertexBuffer;

#include "DecalVertexFactory.h"

class FTerrainDecalVertexFactoryBase;

/** Vertex factory with vertex stream components for terrain vertices */
class FTerrainVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FTerrainVertexFactory);

public:
	struct DataType
	{
		/** The stream to read the vertex position from.		*/
		FVertexStreamComponent PositionComponent;
		/** The stream to read the vertex displacement from.	*/
		FVertexStreamComponent DisplacementComponent;
		/** The stream to read the vertex gradients from.		*/
		FVertexStreamComponent GradientComponent;
	};

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		// only compile terrain materials for terrain vertex factory
		// The special engine materials must be compiled for the terrain vertex factory because they are used with it for wireframe, etc.
		return Material->IsTerrainMaterial() || Material->IsSpecialEngineMaterial();
	}

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("DISABLE_LIGHTMAP_SPECULAR"),TEXT("1"));
	}

	/**
	 * An implementation of the interface used by TSynchronizedResource to 
	 * update the resource with new data from the game thread.
	 * @param	InData - new stream component data
	 */
	void SetData(const DataType& InData)
	{
		Data = InData;
		UpdateRHI();
	}

	/** accessor */
	void SetTerrainObject(FTerrainObject* InTerrainObject)
	{
		TerrainObject = InTerrainObject;
	}

	FTerrainObject* GetTerrainObject()
	{
		return TerrainObject;
	}

	INT GetTessellationLevel()
	{
		return TessellationLevel;
	}

	void SetTessellationLevel(INT InTessellationLevel)
	{
		TessellationLevel = InTessellationLevel;
	}

	// FRenderResource interface.
	virtual void InitRHI();

	/**
	 *	Initialize the component streams.
	 *	
	 *	@param	Buffer	Pointer to the vertex buffer that will hold the data streams.
	 *	@param	Stride	The stride of the provided vertex buffer.
	 */
	virtual UBOOL InitComponentStreams(FTerrainVertexBuffer* Buffer);

	virtual FTerrainDecalVertexFactoryBase* CastTFTerrainDecalVertexFactoryBase()
	{
		return NULL;
	}

private:
	/** stream component data bound to this vertex factory */
	DataType Data;  

	FTerrainObject* TerrainObject;
	INT TessellationLevel;
};

class FTerrainDecalVertexFactoryBase : public FDecalVertexFactoryBase
{
public:
	virtual ~FTerrainDecalVertexFactoryBase()
	{
	}

	virtual FTerrainVertexFactory* CastToFTerrainVertexFactory()=0;
};

/** Decal vertex factory with vertex stream components for terrain vertices */
class FTerrainDecalVertexFactory : public FTerrainDecalVertexFactoryBase, public FTerrainVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FTerrainDecalVertexFactory);

public:
	/**
	 * Should we cache the material's shader type on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("TERRAIN_DECAL_ENABLED"),TEXT("1"));
	}

	virtual FVertexFactory* CastToFVertexFactory()
	{
		return static_cast<FTerrainDecalVertexFactory*>( this );
		//return static_cast<FTerrainVertexFactory*>( this );
	}

	virtual FTerrainVertexFactory* CastToFTerrainVertexFactory()
	{
		return static_cast<FTerrainDecalVertexFactory*>( this );
	}

	virtual FTerrainDecalVertexFactoryBase* CastTFTerrainDecalVertexFactoryBase()
	{
		return (FTerrainDecalVertexFactoryBase*)(static_cast<FTerrainDecalVertexFactory*>( this ));
	}
};

/** Vertex factory with vertex stream components for terrain morphing vertices */
class FTerrainMorphVertexFactory : public FTerrainVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FTerrainMorphVertexFactory);

public:
	struct DataType : public FTerrainVertexFactory::DataType
	{
		/** The stream to read the height and gradient transitions from.	*/
		FVertexStreamComponent HeightTransitionComponent;		// TessDataIndexLo, TessDataIndexHi, ZLo, ZHi
	};

	/**
	 * An implementation of the interface used by TSynchronizedResource to 
	 * update the resource with new data from the game thread.
	 * @param	InData - new stream component data
	 */
	void SetData(const DataType& InData)
	{
		Data = InData;
		UpdateRHI();
	}

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("TERRAIN_MORPHING_ENABLED"),TEXT("1"));
		OutEnvironment.Definitions.Set(TEXT("DISABLE_LIGHTMAP_SPECULAR"),TEXT("1"));
	}

	// FRenderResource interface.
	virtual void InitRHI();

	/**
	 *	Initialize the component streams.
	 *	
	 *	@param	Buffer	Pointer to the vertex buffer that will hold the data streams.
	 *	@param	Stride	The stride of the provided vertex buffer.
	 */
	virtual UBOOL InitComponentStreams(FTerrainVertexBuffer* Buffer);

private:
	/** stream component data bound to this vertex factory */
	DataType Data;  
};

/** Decal vertex factory with vertex stream components for terrain morphing vertices */
class FTerrainMorphDecalVertexFactory : public FTerrainDecalVertexFactoryBase, public FTerrainMorphVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FTerrainMorphDecalVertexFactory);

public:
	/**
	 * Should we cache the material's shader type on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("TERRAIN_DECAL_ENABLED"),TEXT("1"));
		OutEnvironment.Definitions.Set(TEXT("TERRAIN_MORPHING_ENABLED"),TEXT("1"));
	}

	virtual FVertexFactory* CastToFVertexFactory()
	{
		return static_cast<FTerrainMorphDecalVertexFactory*>( this );
	}

	virtual FTerrainVertexFactory* CastToFTerrainVertexFactory()
	{
		return static_cast<FTerrainMorphDecalVertexFactory*>( this );
	}

	virtual FTerrainDecalVertexFactoryBase* CastTFTerrainDecalVertexFactoryBase()
	{
		return (FTerrainDecalVertexFactoryBase*)(static_cast<FTerrainMorphDecalVertexFactory*>( this ));
	}
};

/** Vertex factory with vertex stream components for terrain morphing vertices */
class FTerrainFullMorphVertexFactory : public FTerrainMorphVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FTerrainFullMorphVertexFactory);

public:
	struct DataType : public FTerrainMorphVertexFactory::DataType
	{
		/** The stream to read the height and gradient transitions from.	*/
		FVertexStreamComponent GradientTransitionComponent;		// XGrad, YGrad
	};

	/**
	 * An implementation of the interface used by TSynchronizedResource to 
	 * update the resource with new data from the game thread.
	 * @param	InData - new stream component data
	 */
	void SetData(const DataType& InData)
	{
		Data = InData;
		UpdateRHI();
	}

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("TERRAIN_MORPHING_ENABLED"),TEXT("1"));
		OutEnvironment.Definitions.Set(TEXT("TERRAIN_MORPHING_GRADIENTS"),TEXT("1"));
		OutEnvironment.Definitions.Set(TEXT("DISABLE_LIGHTMAP_SPECULAR"),TEXT("1"));
	}

	// FRenderResource interface.
	virtual void InitRHI();

	/**
	 *	Initialize the component streams.
	 *	
	 *	@param	Buffer	Pointer to the vertex buffer that will hold the data streams.
	 *	@param	Stride	The stride of the provided vertex buffer.
	 */
	virtual UBOOL InitComponentStreams(FTerrainVertexBuffer* Buffer);

private:
	/** stream component data bound to this vertex factory */
	DataType Data;  
};

/** Decal vertex factory with vertex stream components for terrain full morphing vertices */
class FTerrainFullMorphDecalVertexFactory : public FTerrainDecalVertexFactoryBase, public FTerrainFullMorphVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FTerrainFullMorphDecalVertexFactory);

public:
	/**
	 * Should we cache the material's shader type on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("TERRAIN_DECAL_ENABLED"),TEXT("1"));
		OutEnvironment.Definitions.Set(TEXT("TERRAIN_MORPHING_ENABLED"),TEXT("1"));
		OutEnvironment.Definitions.Set(TEXT("TERRAIN_MORPHING_GRADIENTS"),TEXT("1"));
	}

	virtual FVertexFactory* CastToFVertexFactory()
	{
		return static_cast<FTerrainFullMorphDecalVertexFactory*>( this );
	}

	virtual FTerrainVertexFactory* CastToFTerrainVertexFactory()
	{
		return static_cast<FTerrainFullMorphDecalVertexFactory*>( this );
	}

	virtual FTerrainDecalVertexFactoryBase* CastTFTerrainDecalVertexFactoryBase()
	{
		return (FTerrainDecalVertexFactoryBase*)(static_cast<FTerrainFullMorphDecalVertexFactory*>( this ));
	}
};
