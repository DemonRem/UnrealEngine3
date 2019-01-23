/*=============================================================================
	VertexFactory.h: Vertex factory definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Constants.
enum { MAX_TEXCOORDS = 4 };

// Forward declarations.
class FShader;

/**
 * A typed data source for a vertex factory which streams data from a vertex buffer.
 */
struct FVertexStreamComponent
{
	/** The vertex buffer to stream data from.  If null, no data can be read from this stream. */
	const FVertexBuffer* VertexBuffer;

	/** The offset of the data, relative to the beginning of each element in the vertex buffer. */
	BYTE Offset;

	/** The stride of the data. */
	BYTE Stride;

	/** The type of the data read from this stream. */
	BYTE Type;

	/** TRUE if the stream should be indexed by instance index instead of vertex index. */
	UBOOL bUseInstanceIndex;

	/**
	 * Initializes the data stream to null.
	 */
	FVertexStreamComponent():
		VertexBuffer(NULL),
		Offset(0),
		Stride(0),
		Type(VET_None),
		bUseInstanceIndex(FALSE)
	{}

	/**
	 * Minimal initialization constructor.
	 */
	FVertexStreamComponent(const FVertexBuffer* InVertexBuffer,UINT InOffset,UINT InStride,UINT InType,UBOOL bInUseInstanceIndex = FALSE):
		VertexBuffer(InVertexBuffer),
		Offset(InOffset),
		Stride(InStride),
		Type(InType),
		bUseInstanceIndex(bInUseInstanceIndex)
	{}
};

/**
 * A macro which initializes a FVertexStreamComponent to read a member from a struct.
 */
#define STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,VertexType,Member,MemberType) \
	FVertexStreamComponent(VertexBuffer,STRUCT_OFFSET(VertexType,Member),sizeof(VertexType),MemberType)

/**
 * An interface to the parameter bindings for the vertex factory used by a shader.
 */
class FVertexFactoryShaderParameters
{
public:
	virtual ~FVertexFactoryShaderParameters() {}
	virtual void Bind(const class FShaderParameterMap& ParameterMap) = 0;
	virtual void Serialize(FArchive& Ar) = 0;
	virtual void Set(FCommandContextRHI* Context,FShader* VertexShader,const class FVertexFactory* VertexFactory,const FSceneView* View) const = 0;
	virtual void SetLocalTransforms(FCommandContextRHI* Context,FShader* VertexShader,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal) const = 0;
};

/**
 * An object used to represent the type of a vertex factory.
 */
class FVertexFactoryType
{
public:

	typedef FVertexFactoryShaderParameters* (*ConstructParametersType)();
	typedef UBOOL (*ShouldCacheType)(EShaderPlatform, const class FMaterial*, const class FShaderType*);
	typedef void (*ModifyCompilationEnvironmentType)(FShaderCompilerEnvironment&);

	/**
	 * @return The global shader factory list.
	 */
	static TLinkedList<FVertexFactoryType*>*& GetTypeList();

	/**
	 * Gets a list of FVertexFactoryType whose source file no longer matches what that type was compiled with
	 */
	static void GetOutdatedTypes(TArray<FVertexFactoryType*>& OutdatedVFTypes);

	/**
	 * Finds a FVertexFactoryType by name.
	 */
	static FVertexFactoryType* GetVFByName(const FString& VFName);

	/**
	 * Minimal initialization constructor.
	 * @param bInUsedWithMaterials - True if the vertex factory will be rendered in combination with a material.
	 * @param bInSupportsStaticLighting - True if the vertex factory will be rendered with static lighting.
	 */
	FVertexFactoryType(
		const TCHAR* InName,
		const TCHAR* InShaderFilename,
		UBOOL bInUsedWithMaterials,
		UBOOL bInSupportsStaticLighting,
		ConstructParametersType InConstructParameters,
		ShouldCacheType InShouldCache,
		ModifyCompilationEnvironmentType InModifyCompilationEnvironment,
		INT InMinPackageVersion,
		INT InMinLicenseePackageVersion
		);

	// Accessors.
	const TCHAR* GetName() const { return Name; }
	FName GetFName() const { return TypeName; }
	const TCHAR* GetShaderFilename() const { return ShaderFilename; }
	FVertexFactoryShaderParameters* CreateShaderParameters() const { return (*ConstructParameters)(); }
	UBOOL IsUsedWithMaterials() const { return bUsedWithMaterials; }
	UBOOL SupportsStaticLighting() const { return bSupportsStaticLighting; }
	INT GetMinPackageVersion() const { return MinPackageVersion; }
	INT GetMinLicenseePackageVersion() const { return MinLicenseePackageVersion; }

	// Hash function.
	friend DWORD GetTypeHash(FVertexFactoryType* Type)
	{ 
		return Type ? Type->HashIndex : 0;
	}

	/**
	 * Calculates a CRC based on this vertex factory type's source code and includes
	 */
	virtual DWORD GetSourceCRC();

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		return (*ShouldCacheRef)(Platform, Material, ShaderType); 
	}

	/**
	* Calls the function ptr for the shader type on the given environment
	* @param Environment - shader compile environment to modify
	*/
	void ModifyCompilationEnvironment(FShaderCompilerEnvironment& Environment)
	{
		(*ModifyCompilationEnvironmentRef)( Environment );
	}

private:
	DWORD HashIndex;
	const TCHAR* Name;
	const TCHAR* ShaderFilename;
	FName TypeName;
	BITFIELD bUsedWithMaterials : 1;
	BITFIELD bSupportsStaticLighting : 1;
	ConstructParametersType ConstructParameters;
	ShouldCacheType ShouldCacheRef;
	ModifyCompilationEnvironmentType ModifyCompilationEnvironmentRef;
	/**
	 *	The minimum version the package needs to be for this vertex factor to be valid.
	 *	Otherwise, toss them at load-time.
	 */
	INT MinPackageVersion;
	/** The minimum licensee package version the shader cache's package needs to load shaders compiled for the vertex factory. */
	INT MinLicenseePackageVersion;
};

/**
 * Serializes a reference to a vertex factory type.
 */
extern FArchive& operator<<(FArchive& Ar,FVertexFactoryType*& TypeRef);

/**
 * Find the vertex factory type with the given name.
 * @return NULL if no vertex factory type matched, otherwise a vertex factory type with a matching name.
 */
extern FVertexFactoryType* FindVertexFactoryType(FName TypeName);

/**
 * A macro for declaring a new vertex factory type, for use in the vertex factory class's definition body.
 */
#define DECLARE_VERTEX_FACTORY_TYPE(FactoryClass) \
	public: \
	static FVertexFactoryType StaticType; \
	virtual FVertexFactoryType* GetType() const { return &StaticType; }

/**
 * A macro for implementing the static vertex factory type object, and specifying parameters used by the type.
 * @param bUsedWithMaterials - True if the vertex factory will be rendered in combination with a material.
 * @param bSupportsStaticLighting - True if the vertex factory will be rendered with static lighting.
 */
#define IMPLEMENT_VERTEX_FACTORY_TYPE(FactoryClass,ShaderParametersType,ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,InMinPackageVersion,InMinLicenseePackageVersion) \
	FVertexFactoryShaderParameters* Construct##FactoryClass##ShaderParameters() { return new ShaderParametersType(); } \
	FVertexFactoryType FactoryClass::StaticType( \
		TEXT(#FactoryClass), \
		TEXT(ShaderFilename), \
		bUsedWithMaterials, \
		bSupportsStaticLighting, \
		Construct##FactoryClass##ShaderParameters, \
		FactoryClass::ShouldCache, \
		FactoryClass::ModifyCompilationEnvironment, \
		InMinPackageVersion, \
		InMinLicenseePackageVersion \
		);

/**
 * Encapsulates a vertex data source which can be linked into a vertex shader.
 */
class FVertexFactory : public FRenderResource
{
public:

	struct DataType
	{
		struct
		{
			/** The offset of the vertex light-map stream. */
			BYTE Offset;

			/** Stride of the vertex light-map stream if it stores directional samples. */
			BYTE DirectionalStride;

			/** Stride of the vertex light-map stream if it stores simple samples. */
			BYTE SimpleStride;

			/** TRUE if the vertex light-map stream should be indexed by instance index instead of vertex index. */
			UBOOL bUseInstanceIndex;
		} LightMapStream;

		/** The number of vertices in an instance */
		UINT NumVerticesPerInstance;

		/** The number of instances. */
		UINT NumInstances;

		/** Initialization constructor. */
		DataType();
	};

	/**
	 * @return The vertex factory's type.
	 */
	virtual FVertexFactoryType* GetType() const = 0;

	/**
	 * Activates the vertex factory.
	 */
	void Set(FCommandContextRHI* Context) const;

	/**
	* Sets the position stream as the current stream source.
	*/
	void SetPositionStream(FCommandContextRHI* Context) const;

	/**
	 * Activates the vertex factory for rendering a vertex shadow-map.
	 */
	void SetVertexShadowMap(FCommandContextRHI* Context,const FVertexBuffer* ShadowMapVertexBuffer) const;

	/**
	 * Activates the vertex factory for rendering a vertex light-map.
	 */
	void SetVertexLightMap(FCommandContextRHI* Context,const FVertexBuffer* LightMapVertexBuffer,UBOOL bUseDirectionalLightMap) const;

	/**
	* Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	*/
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment) {}

	// FRenderResource interface.
	virtual void ReleaseRHI();

	// Accessors.
	FVertexDeclarationRHIRef& GetDeclaration() { return Declaration; }
	void SetDeclaration(FVertexDeclarationRHIRef& NewDeclaration) { Declaration = NewDeclaration; }

	const FVertexDeclarationRHIRef& GetDeclaration() const { return Declaration; }
	const FVertexDeclarationRHIRef& GetPositionDeclaration() const { return PositionDeclaration; }
	const FVertexDeclarationRHIRef& GetVertexShadowMapDeclaration() const { return VertexShadowMapDeclaration; }
	const FVertexDeclarationRHIRef& GetVertexLightMapDeclaration( UBOOL bUseDirectionalLightMap ) const 
	{ 
		return bUseDirectionalLightMap ? DirectionalVertexLightMapDeclaration : SimpleVertexLightMapDeclaration; 
	}

	INT GetNumVerticesPerInstance() const { return Data.NumVerticesPerInstance; }
	virtual UBOOL IsGPUSkinned() const { return FALSE; }

	/** Indicates whether the vertex factory supports a position-only stream. */
	UBOOL SupportsPositionOnlyStream() const { return PositionStream.Num(); }

	/**
	* Fill in array of strides from this factory's vertex streams without shadow/light maps
	* @param OutStreamStrides - output array of # MaxVertexElementCount stream strides to fill
	*/
	INT GetStreamStrides(DWORD *OutStreamStrides, UBOOL bPadWithZeroes=TRUE) const;
	/**
	* Fill in array of strides from this factory's position only vertex streams
	* @param OutStreamStrides - output array of # MaxVertexElementCount stream strides to fill
	*/
	void GetPositionStreamStride(DWORD *OutStreamStrides) const;
	/**
	* Fill in array of strides from this factory's vertex streams when using shadow-maps
	* @param OutStreamStrides - output array of # MaxVertexElementCount stream strides to fill
	*/
	void GetVertexShadowMapStreamStrides(DWORD *OutStreamStrides) const;
	/**
	* Fill in array of strides from this factory's vertex streams when using light-maps
	* @param OutStreamStrides - output array of # MaxVertexElementCount stream strides to fill
	*/
	void GetVertexLightMapStreamStrides(DWORD *OutStreamStrides,UBOOL bUseDirectionalLightMap) const;

protected:

	/**
	 * Creates a vertex element for a vertex stream components.  Adds a unique stream index for the vertex buffer used by the component.
	 * @param Component - The vertex stream component.
	 * @param Usage - The vertex element usage semantics.
	 * @param UsageIndex - the vertex element usage index.
	 * @return The vertex element which corresponds to Component.
	 */
	FVertexElement AccessStreamComponent(const FVertexStreamComponent& Component,BYTE Usage,BYTE UsageIndex = 0);

	/**
	 * Creates a vertex element for a position vertex stream component.  Adds a unique position stream index for the vertex buffer used by the component.
	 * @param Component - The position vertex stream component.
	 * @param Usage - The vertex element usage semantics.
	 * @param UsageIndex - the vertex element usage index.
	 * @return The vertex element which corresponds to Component.
	 */
	FVertexElement AccessPositionStreamComponent(const FVertexStreamComponent& Component,BYTE Usage,BYTE UsageIndex = 0);

	/**
	 * Initializes the vertex declaration.
	 * @param Elements - The elements of the vertex declaration.
	 */
	void InitDeclaration(
//TTP:51684
		//old code
		//const FVertexDeclarationElementList& Elements, 
		//need to remove "const" to patch the declarator for FX card
		FVertexDeclarationElementList& Elements, 
//TTP:51684
		const DataType& Data, 
		UBOOL bCreateShadowMapDeclaration = TRUE, 
		UBOOL bCreateDirectionalLightMapDeclaration = TRUE, 
		UBOOL bCreateSimpleLightMapDeclaration = TRUE);

	/**
	 * Initializes the position-only vertex declaration.
	 * @param Elements - The elements of the vertex declaration.
	 */
	void InitPositionDeclaration(const FVertexDeclarationElementList& Elements);

private:
	struct FVertexStream
	{
		const FVertexBuffer* VertexBuffer;
		UINT Stride;
		UBOOL bUseInstanceIndex;

		friend UBOOL operator==(const FVertexStream& A,const FVertexStream& B)
		{
			return A.VertexBuffer == B.VertexBuffer && A.Stride == B.Stride && A.bUseInstanceIndex == B.bUseInstanceIndex;
		}
	};

	/** The vertex streams used to render the factory. */
	TStaticArray<FVertexStream,MaxVertexElementCount> Streams;

	/** The position only vertex stream used to render the factory during depth only passes. */
	TStaticArray<FVertexStream,MaxVertexElementCount> PositionStream;

	/** The RHI vertex declaration used to render the factory normally. */
	FVertexDeclarationRHIRef Declaration;

	/** The RHI vertex declaration used to render the factory during depth only passes. */
	FVertexDeclarationRHIRef PositionDeclaration;

	/** The RHI vertex declaration used to render the factory with a vertex shadow-map. */
	FVertexDeclarationRHIRef VertexShadowMapDeclaration;

	/** The RHI vertex declaration used to render the factory with a directional vertex light-map. */
	FVertexDeclarationRHIRef DirectionalVertexLightMapDeclaration;

	/** The RHI vertex declaration used to render the factory with a simple vertex light-map. */
	FVertexDeclarationRHIRef SimpleVertexLightMapDeclaration;

	/** The vertex factory's data. */
	DataType Data;
};

/**
 * An encapsulation of the vertex factory parameters for a shader.
 * Handles serialization and binding of the vertex factory parameters for the shader's vertex factory type.
 */
class FVertexFactoryParameterRef
{
public:

	FVertexFactoryParameterRef():
		Parameters(NULL),
		VertexFactoryType(NULL)
	{}

	FVertexFactoryParameterRef(FVertexFactoryType* InVertexFactoryType,const FShaderParameterMap& ParameterMap):
		Parameters(NULL),
		VertexFactoryType(InVertexFactoryType)
	{
		Parameters = VertexFactoryType->CreateShaderParameters();
		if(Parameters)
		{
			Parameters->Bind(ParameterMap);
		}
	}

	~FVertexFactoryParameterRef()
	{
		delete Parameters;
	}

	void Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const
	{
		if(Parameters)
		{
			Parameters->Set(Context,VertexShader,VertexFactory,View);
		}
	}

	void SetLocalTransforms(FCommandContextRHI* Context,FShader* VertexShader,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal) const
	{
		if(Parameters)
		{
			Parameters->SetLocalTransforms(Context,VertexShader,LocalToWorld,WorldToLocal);
		}
	}

	friend FArchive& operator<<(FArchive& Ar,FVertexFactoryParameterRef& Ref);

private:
	FVertexFactoryShaderParameters* Parameters;
	FVertexFactoryType* VertexFactoryType;
};
