/*=============================================================================
	MaterialShader.h: Material shader definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/** The minimum package version to load material pixel shaders with. */
#define VER_MIN_MATERIAL_PIXELSHADER	VER_MATERIAL_GAMMA_CORRECTION

/** Same as VER_MIN_MATERIAL_PIXELSHADER, but for the licensee package version. */
#define LICENSEE_VER_MIN_MATERIAL_PIXELSHADER	0

/** The minimum package version to load material vertex shaders with. */
#define VER_MIN_MATERIAL_VERTEXSHADER	VER_AUGUST_XDK_UPGRADE

/** Same as VER_MIN_MATERIAL_VERTEXSHADER, but for the licensee package version. */
#define LICENSEE_VER_MIN_MATERIAL_VERTEXSHADER	0

/** Set to true to get a list of what shadertypes are compiled for each vertex factory and material. */
#define DUMP_MATERIAL_SHADER_STATS	0

/** A macro to implement material shaders which checks the package version for VER_MIN_MATERIAL_*SHADER and LICENSEE_VER_MIN_MATERIAL_*SHADER. */
#define IMPLEMENT_MATERIAL_SHADER_TYPE(TemplatePrefix,ShaderClass,SourceFilename,FunctionName,Frequency,MinPackageVersion,MinLicenseePackageVersion) \
	IMPLEMENT_SHADER_TYPE( \
		TemplatePrefix, \
		ShaderClass, \
		SourceFilename, \
		FunctionName, \
		Frequency, \
		Max(MinPackageVersion,Frequency == SF_Pixel ? Max(VER_MIN_COMPILEDMATERIAL, VER_MIN_MATERIAL_PIXELSHADER) : Max(VER_MIN_COMPILEDMATERIAL, VER_MIN_MATERIAL_VERTEXSHADER)), \
		Max(MinLicenseePackageVersion,Frequency == SF_Pixel ? Max(LICENSEE_VER_MIN_COMPILEDMATERIAL, LICENSEE_VER_MIN_MATERIAL_PIXELSHADER) : Max(LICENSEE_VER_MIN_COMPILEDMATERIAL, LICENSEE_VER_MIN_MATERIAL_VERTEXSHADER)) \
		);

/** Converts an EMaterialLightingModel to a string description. */
extern FString GetLightingModelString(EMaterialLightingModel LightingModel);

/** Converts an EBlendMode to a string description. */
extern FString GetBlendModeString(EBlendMode BlendMode);

/** Called for every material shader to update the appropriate stats. */
extern void UpdateMaterialShaderCompilingStats(const FMaterial* Material);

/**
* Holds the information for a static switch parameter
*/
class FStaticSwitchParameter
{
public:
	FName ParameterName;
	UBOOL Value;
	UBOOL bOverride;
	FGuid ExpressionGUID;

	FStaticSwitchParameter() :
		ParameterName(TEXT("None")),
		Value(FALSE),
		bOverride(FALSE),
		ExpressionGUID(0,0,0,0)
	{ }

	FStaticSwitchParameter(FName InName, UBOOL InValue, UBOOL InOverride, FGuid InGuid) :
		ParameterName(InName),
		Value(InValue),
		bOverride(InOverride),
		ExpressionGUID(InGuid)
	{ }

	friend FArchive& operator<<(FArchive& Ar,FStaticSwitchParameter& P)
	{
		Ar << P.ParameterName << P.Value << P.bOverride;
		if (Ar.Ver() >= VER_MATERIAL_FALLBACKS)
		{
			Ar << P.ExpressionGUID;
		}
		return Ar;
	}
};

/**
* Holds the information for a static component mask parameter
*/
class FStaticComponentMaskParameter
{
public:
	FName ParameterName;
	UBOOL R, G, B, A;
	UBOOL bOverride;
	FGuid ExpressionGUID;

	FStaticComponentMaskParameter() :
		ParameterName(TEXT("None")),
		R(FALSE),
		G(FALSE),
		B(FALSE),
		A(FALSE),
		bOverride(FALSE),
		ExpressionGUID(0,0,0,0)
	{ }

	FStaticComponentMaskParameter(FName InName, UBOOL InR, UBOOL InG, UBOOL InB, UBOOL InA, UBOOL InOverride, FGuid InGuid) :
		ParameterName(InName),
		R(InR),
		G(InG),
		B(InB),
		A(InA),
		bOverride(InOverride),
		ExpressionGUID(InGuid)
	{ }

	friend FArchive& operator<<(FArchive& Ar,FStaticComponentMaskParameter& P)
	{
		Ar << P.ParameterName << P.R << P.G << P.B << P.A << P.bOverride;
		if (Ar.Ver() >= VER_MATERIAL_FALLBACKS)
		{
			Ar << P.ExpressionGUID;
		}
		return Ar;
	}
};

/**
* Contains all the information needed to identify a FMaterialShaderMap.  
*/
class FStaticParameterSet
{
public:
	/** 
	* The Id of the base material's FMaterialResource.  This, along with the static parameters,
	* are used to identify identical shader maps so that material instances with static permutations can share shader maps.
	*/
	FGuid BaseMaterialId;

	/** An array of static switch parameters in this set */
	TArray<FStaticSwitchParameter> StaticSwitchParameters;

	/** An array of static component mask parameters in this set */
	TArray<FStaticComponentMaskParameter> StaticComponentMaskParameters;

	FStaticParameterSet() :
		BaseMaterialId(0, 0, 0, 0)
	{
	}

	FStaticParameterSet(FGuid InBaseId) :
		BaseMaterialId(InBaseId)
	{
	}

	/** 
	* Checks if this set contains any parameters
	* 
	* @return	TRUE if this set has no parameters
	*/
	UBOOL IsEmpty()
	{
		return StaticSwitchParameters.Num() == 0 && StaticComponentMaskParameters.Num() == 0;
	}

	void Serialize(FArchive& Ar)
	{
		Ar << BaseMaterialId << StaticSwitchParameters << StaticComponentMaskParameters;
	}

	/** 
	* Indicates whether this set is equal to another, copying override settings.
	* 
	* @param ReferenceSet	The set to compare against
	* @return				TRUE if the sets are not equal
	*/
	UBOOL ShouldMarkDirty(const FStaticParameterSet * ReferenceSet);

	friend DWORD GetTypeHash(const FStaticParameterSet &Ref)
	{
		return Ref.BaseMaterialId.A;
	}

	/** 
	* Tests this set against another for equality, disregarding override settings.
	* 
	* @param ReferenceSet	The set to compare against
	* @return				TRUE if the sets are equal
	*/
	UBOOL operator==(const FStaticParameterSet &ReferenceSet);
};

/**
 * An encapsulation of the material parameters for a shader.
 */
class FMaterialPixelShaderParameters
{
public:

	void Bind(const FMaterial* Material,const FShaderParameterMap& ParameterMap);
	void Set(FCommandContextRHI* Context,FShader* PixelShader,const FMaterialRenderContext& MaterialRenderContext) const;
	
	/**
	* Set local transforms for rendering a material with a single mesh
	* @param Context - command context
	* @param FMaterialRenderProxy - material used for the shader
	* @param LocalToWorld - l2w for rendering a single mesh
	* @param bBackFace - True if the backfaces of a two-sided material are being rendered.
	*/
	void SetLocalTransforms(
		FCommandContextRHI* Context,
		FShader* PixelShader,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMatrix& LocalToWorld,
		UBOOL bBackFace
		) const;

	friend FArchive& operator<<(FArchive& Ar,FMaterialPixelShaderParameters& Parameters);

private:
	struct FUniformParameter
	{
		BYTE Type;
		INT Index;
		FShaderParameter ShaderParameter;
		friend FArchive& operator<<(FArchive& Ar,FUniformParameter& P)
		{
			return Ar << P.Type << P.Index << P.ShaderParameter;
		}
	};
	TArray<FUniformParameter> UniformParameters;
	/** matrix parameter for materials with a world transform */
	FShaderParameter LocalToWorldParameter;
	/** matrix parameter for materials with a view transform */
	FShaderParameter WorldToViewParameter;
	/** matrix parameter for materials with a world position transform */
	FShaderParameter WorldProjectionInvParameter;
	/** world-space camera position */
	FShaderParameter CameraWorldPositionParameter;
	/** The scene texture parameters. */
	FSceneTextureShaderParameters SceneTextureParameters;
	/** Parameter indicating whether the front-side or the back-side of a two-sided material is being rendered. */
	FShaderParameter TwoSidedSignParameter;
	/** Inverse gamma parameter. Only used when USE_GAMMA_CORRECTION 1 */
	FShaderParameter InvGammaParameter;
};

/**
 * A shader meta type for material-linked shaders.
 */
class FMaterialShaderType : public FShaderType
{
public:

	/**
	 * Finds a FMaterialShaderType by name.
	 */
	static FMaterialShaderType* GetTypeByName(const FString& TypeName);

	struct CompiledShaderInitializerType : FGlobalShaderType::CompiledShaderInitializerType
	{
		const FMaterial* Material;
		CompiledShaderInitializerType(
			FShaderType* InType,
			const FShaderCompilerOutput& CompilerOutput,
			const FMaterial* InMaterial
			):
			FGlobalShaderType::CompiledShaderInitializerType(InType,CompilerOutput),
			Material(InMaterial)
		{}
	};
	typedef FShader* (*ConstructCompiledType)(const CompiledShaderInitializerType&);
	typedef UBOOL (*ShouldCacheType)(EShaderPlatform,const FMaterial*);

	FMaterialShaderType(
		const TCHAR* InName,
		const TCHAR* InSourceFilename,
		const TCHAR* InFunctionName,
		DWORD InFrequency,
		INT InMinPackageVersion,
		INT InMinLicenseePackageVersion,
		ConstructSerializedType InConstructSerializedRef,
		ConstructCompiledType InConstructCompiledRef,
		ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef,
		ShouldCacheType InShouldCacheRef
		):
		FShaderType(InName,InSourceFilename,InFunctionName,InFrequency,InMinPackageVersion,InMinLicenseePackageVersion,InConstructSerializedRef,InModifyCompilationEnvironmentRef),
		ConstructCompiledRef(InConstructCompiledRef),
		ShouldCacheRef(InShouldCacheRef)
	{}

	/**
	 * Compiles a shader of this type.  After compiling the shader, either returns an equivalent existing shader of this type, or constructs
	 * a new instance.
	 * @param Compiler - The shader compiler to use.
	 * @param Material - The material to link the shader with.
	 * @param MaterialShaderCode - The shader code for the material.
	 * @param OutErrors - Upon compilation failure, OutErrors contains a list of the errors which occured.
	 * @return NULL if the compilation failed.
	 */
	FShader* CompileShader(
		EShaderPlatform Platform,
		const FMaterial* Material,
		const TCHAR* MaterialShaderCode,
		TArray<FString>& OutErrors
		);

	/**
	 * Checks if the shader type should be cached for a particular platform and material.
	 * @param Platform - The platform to check.
	 * @param Material - The material to check.
	 * @return True if this shader type should be cached.
	 */
	UBOOL ShouldCache(EShaderPlatform Platform,const FMaterial* Material) const
	{
		return (*ShouldCacheRef)(Platform,Material);
	}

	virtual DWORD GetSourceCRC();

	// Dynamic casting.
	virtual FMaterialShaderType* GetMaterialShaderType() { return this; }

private:
	ConstructCompiledType ConstructCompiledRef;
	ShouldCacheType ShouldCacheRef;
};

/**
 * The set of material shaders for a single material.
 */
class FMaterialShaderMap : public TShaderMap<FMaterialShaderType>, public FRefCountedObject
{
public:

	/**
	 * Finds the shader map for a material.
	 * @param StaticParameterSet - The static parameter set identifying the shader map
	 * @param Platform - The platform to lookup for
	 * @return NULL if no cached shader map was found.
	 */
	static FMaterialShaderMap* FindId(const FStaticParameterSet& StaticParameterSet, EShaderPlatform Platform);

	FMaterialShaderMap()
	{}

	// Destructor.
	~FMaterialShaderMap();

	/**
	 * Compiles the shaders for a material and caches them in this shader map.
	 * @param Material - The material to compile shaders for.
	 * @param InStaticParameters - the set of static parameters to compile for
	 * @param MaterialShaderCode - The shader code for Material.
	 * @param Platform - The platform to compile to
	 * @param OutErrors - Upon compilation failure, OutErrors contains a list of the errors which occured.
	 * @return True if the compilation succeeded.
	 */
	UBOOL Compile(const FMaterial* Material,const FStaticParameterSet* InStaticParameters, const TCHAR* MaterialShaderCode,EShaderPlatform Platform,TArray<FString>& OutErrors,UBOOL bSilent = FALSE);

	/**
	 * Checks whether the material shader map is missing any shader types necessary for the given material.
	 * @param Material - The material which is checked.
	 * @return True if the shader map has all of the shader types necessary.
	 */
	UBOOL IsComplete(const FMaterial* Material) const;

	/**
	 * Builds a list of the shaders in a shader map.
	 */
	void GetShaderList(TMap<FGuid,FShader*>& OutShaders) const;

	/**
	 * Begins initializing the shaders used by the material shader map.
	 */
	void BeginInit();

	/**
	 * Removes all entries in the cache with exceptions based on a shader type
	 * @param ShaderType - The shader type to flush or keep (depending on second param)
	 * @param bFlushAllButShaderType - TRUE if all shaders EXCEPT the given type should be flush. FALSE will flush ONLY the given shader type
	 */
	void FlushShadersByShaderType(FShaderType* ShaderType, UBOOL bFlushAllButShaderType=FALSE);

	/**
	 * Removes all entries in the cache with exceptions based on a vertex factory type
	 * @param ShaderType - The shader type to flush or keep (depending on second param)
	 * @param bFlushAllButVertexFactoryType - TRUE if all shaders EXCEPT the given type should be flush. FALSE will flush ONLY the given vertex factory type
	 */
	void FlushShadersByVertexFactoryType(FVertexFactoryType* VertexFactoryType, UBOOL bFlushAllButVertexFactoryType=FALSE);

	// Serializer.
	void Serialize(FArchive& Ar);

	// Accessors.
	const class FMeshMaterialShaderMap* GetMeshShaderMap(FVertexFactoryType* VertexFactoryType) const;
	const FStaticParameterSet& GetMaterialId() const { return StaticParameters; }

private:

	/** A global map from a material's static parameter set to any shader map cached for that material. */
	static TDynamicMap<FStaticParameterSet,FMaterialShaderMap*> GIdToMaterialShaderMap[SP_NumPlatforms];

	/** The material's cached shaders for vertex factory type dependent shaders. */
	TIndirectArray<class FMeshMaterialShaderMap> MeshShaderMaps;

	/** The persistent GUID of this material shader map. */
	FGuid MaterialId;

	/** The material's user friendly name, typically the object name. */
	FString FriendlyName;

	/** The platform this shader map was compiled with */
	EShaderPlatform Platform;

	/** The static parameter set that this shader map was compiled with */
	FStaticParameterSet StaticParameters;

	/** A map from vertex factory type to the material's cached shaders for that vertex factory type. */
	TMap<FVertexFactoryType*,class FMeshMaterialShaderMap*> VertexFactoryMap;

	/**
	 * Initializes VertexFactoryMap from the contents of MeshShaderMaps.
	 */
	void InitVertexFactoryMap();
};
