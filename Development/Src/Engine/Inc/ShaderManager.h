/*=============================================================================
	ShaderManager.h: Shader manager definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Forward declarations.
class FShaderType;

extern EShaderPlatform GRHIShaderPlatform;

/** The minimum package version which stores valid compiled shaders.  This can be used to force a recompile of all shaders. */
#define VER_MIN_SHADER	VER_OCT06_DXSDK_DOWNGRADE

/** Same as VER_MIN_SHADER, but for the licensee package version. */
#define LICENSEE_VER_MIN_SHADER	0

/** Controls discarding of shaders whose source .usf file has changed since the shader was compiled. */
extern UBOOL GAutoReloadChangedShaders;

/**
 * A shader parameter's register binding.
 */
class FShaderParameter
{
public:
	FShaderParameter(): NumRegisters(0) {}
	void Bind(const FShaderParameterMap& ParameterMap,const TCHAR* ParameterName,UBOOL bIsOptional = FALSE);
	friend FArchive& operator<<(FArchive& Ar,FShaderParameter& P);
	UBOOL IsBound() const { return NumRegisters > 0; }
	UINT GetBaseRegisterIndex() const { return BaseRegisterIndex; }
	UINT GetNumRegisters() const { return NumRegisters; }
private:
	WORD BaseRegisterIndex;
	WORD NumRegisters;
};

/**
 * Sets the value of a vertex shader parameter.
 * A template parameter specified the type of the parameter value.
 */
template<class ParameterType>
void SetVertexShaderValue(
	FCommandContextRHI* Context,
	FVertexShaderRHIParamRef VertexShader,
	const FShaderParameter& Parameter,
	const ParameterType& Value,
	UINT ElementIndex = 0
	)
{
	if(Parameter.IsBound())
	{
		enum { RegisterSize = sizeof(FLOAT) * 4 };
		enum { NumRegistersPerElement = (sizeof(ParameterType) + RegisterSize - 1) / RegisterSize };
		check((ElementIndex + 1) * NumRegistersPerElement <= Parameter.GetNumRegisters());
#if PS3
		// @GEMINI_TODO: Remove this when precompiler no longer needs it
		extern UBOOL HACK_ParamIsMatrix;
		HACK_ParamIsMatrix = NumRegistersPerElement > 2;
#endif
		RHISetVertexShaderParameter(
			Context,
			VertexShader,
			Parameter.GetBaseRegisterIndex() + ElementIndex * NumRegistersPerElement,
			NumRegistersPerElement,
			(FLOAT*)&Value
			);
	}
}

/**
 * Sets the value of a pixel shader parameter.
 * A template parameter specified the type of the parameter value.
 */
template<class ParameterType>
void SetPixelShaderValue(
	FCommandContextRHI* Context,
	FPixelShaderRHIParamRef PixelShader,
	const FShaderParameter& Parameter,
	const ParameterType& Value,
	UINT ElementIndex = 0
	)
{
	if(Parameter.IsBound())
	{
		enum { RegisterSize = sizeof(FLOAT) * 4 };
		enum { NumRegistersPerElement = (sizeof(ParameterType) + RegisterSize - 1) / RegisterSize };
		check((ElementIndex + 1) * NumRegistersPerElement <= Parameter.GetNumRegisters());
		RHISetPixelShaderParameter(
			Context,
			PixelShader,
			Parameter.GetBaseRegisterIndex() + ElementIndex * NumRegistersPerElement,
			NumRegistersPerElement,
			(FLOAT*)&Value
			);
	}
}

#if PS3
/**
 * Sets (and swaps R/B) a Linear Color parameter
 */
template<>
void SetPixelShaderValue(
	FCommandContextRHI* Context,
	FPixelShaderRHIParamRef PixelShader,
	const FShaderParameter& Parameter,
	const FLinearColor& Value,
	UINT ElementIndex
	);
#endif

/**
 * Sets the value of a vertex shader parameter array.
 * A template parameter specified the type of the parameter value.
 */
template<class ParameterType>
void SetVertexShaderValues(
	FCommandContextRHI* Context,
	FVertexShaderRHIParamRef VertexShader,
	const FShaderParameter& Parameter,
	const ParameterType* Values,
	UINT NumElements,
	UINT BaseElementIndex = 0
	)
{
	if(Parameter.IsBound())
	{
		enum { RegisterSize = sizeof(FLOAT) * 4 };
		enum { NumRegistersPerElement = (sizeof(ParameterType) + RegisterSize - 1) / RegisterSize };
		check((BaseElementIndex + NumElements) * NumRegistersPerElement <= Parameter.GetNumRegisters());
#if PS3
		extern UBOOL HACK_ParamIsMatrix;
		HACK_ParamIsMatrix = NumRegistersPerElement > 2;
#endif
		RHISetVertexShaderParameter(
			Context,
			VertexShader,
			Parameter.GetBaseRegisterIndex() + BaseElementIndex * NumRegistersPerElement,
			NumElements * NumRegistersPerElement,
			(FLOAT*)Values
			);
	}
}

/**
 * Sets the value of a pixel shader parameter array.
 * A template parameter specified the type of the parameter value.
 */
template<class ParameterType>
void SetPixelShaderValues(
	FCommandContextRHI* Context,
	FPixelShaderRHIParamRef PixelShader,
	const FShaderParameter& Parameter,
	const ParameterType* Values,
	UINT NumElements,
	UINT BaseElementIndex = 0
	)
{
	if(Parameter.IsBound())
	{
		enum { RegisterSize = sizeof(FLOAT) * 4 };
		enum { NumRegistersPerElement = (sizeof(ParameterType) + RegisterSize - 1) / RegisterSize };
		check((BaseElementIndex + NumElements) * NumRegistersPerElement <= Parameter.GetNumRegisters());
		RHISetPixelShaderParameter(
			Context,
			PixelShader,
			Parameter.GetBaseRegisterIndex() + BaseElementIndex * NumRegistersPerElement,
			NumElements * NumRegistersPerElement,
			(FLOAT*)Values
			);
	}
}

/**
 * Sets the value of a shader texture parameter.
 */
FORCEINLINE void SetTextureParameter(
	FCommandContextRHI* Context,
	FPixelShaderRHIParamRef PixelShader,
	const FShaderParameter& Parameter,
	const FTexture* Texture,
	UINT ElementIndex = 0
	)
{
	if(Parameter.IsBound())
	{
		check(ElementIndex < Parameter.GetNumRegisters());
		Texture->LastRenderTime = GCurrentTime;
		RHISetSamplerState(Context,PixelShader,Parameter.GetBaseRegisterIndex() + ElementIndex,Texture->SamplerStateRHI,Texture->TextureRHI);
	}
}

/**
 * Sets the value of a shader texture parameter.
 */
FORCEINLINE void SetTextureParameter(
	FCommandContextRHI* Context,
	FPixelShaderRHIParamRef PixelShader,
	const FShaderParameter& Parameter,
	FSamplerStateRHIParamRef SamplerStateRHI,
	FTextureRHIParamRef TextureRHI,
	UINT ElementIndex = 0
	)
{
	if(Parameter.IsBound())
	{
		check(ElementIndex < Parameter.GetNumRegisters());
		RHISetSamplerState(Context,PixelShader,Parameter.GetBaseRegisterIndex() + ElementIndex,SamplerStateRHI,TextureRHI);
	}
}

/**
 * Sets the value of a vertex shader scalar parameter.
 */
inline void SetVertexShaderValue(
	FCommandContextRHI* Context,
	FVertexShaderRHIParamRef VertexShader,
	const FShaderParameter& Parameter,
	FLOAT Value,
	UINT ElementIndex = 0
	)
{
	if(Parameter.IsBound())
	{
		check(ElementIndex < Parameter.GetNumRegisters());
		FLOAT ValueVector[4] = { Value, Value, Value, Value };
		RHISetVertexShaderParameter(Context,VertexShader,Parameter.GetBaseRegisterIndex() + ElementIndex,1,ValueVector);
	}
}

/**
 * Sets the value of a pixel shader scalar parameter.
 */
inline void SetPixelShaderValue(
	FCommandContextRHI* Context,
	FPixelShaderRHIParamRef PixelShader,
	const FShaderParameter& Parameter,
	FLOAT Value,
	UINT ElementIndex = 0
	)
{
	if(Parameter.IsBound())
	{
		check(ElementIndex < Parameter.GetNumRegisters());
		FLOAT ValueVector[4] = { Value, Value, Value, Value };
		RHISetPixelShaderParameter(Context,PixelShader,Parameter.GetBaseRegisterIndex() + ElementIndex,1,ValueVector);
	}
}

/**
 *	Shader recompile groups are used to minimize shader iteration time. 
 *  Shaders return their group from their GetRecompileGroup() member function.
 *  Currently only global shaders make use of these groups.
 */
enum EShaderRecompileGroup
{
	SRG_GLOBAL_MISC_SHADOW,
	SRG_GLOBAL_BPCF_SHADOW_LOW,
	SRG_GLOBAL_BPCF_SHADOW_MEDIUM_HIGH,
	SRG_GLOBAL_MISC,
	SRG_MISC
};

class FShaderKey
{
public:

	FShaderKey() : 
		ParameterMapCRC(0)
	{}

	FShaderKey(const TArray<BYTE>& InCode, const FShaderParameterMap& InParameterMap) : 
		Code(InCode)
	{
		ParameterMapCRC = InParameterMap.GetCRC();
	}

	TArray<BYTE> Code;
	DWORD ParameterMapCRC;
};

/**
 * A compiled shader and its parameter bindings.
 */
class FShader : public FRenderResource, public FDeferredCleanupInterface
{
	friend class FShaderType;
public:

	struct CompiledShaderInitializerType
	{
		FShaderType* Type;
		FShaderTarget Target;
		const TArray<BYTE>& Code;
		const FShaderParameterMap& ParameterMap;
		UINT NumInstructions;
		CompiledShaderInitializerType(
			FShaderType* InType,
			const FShaderCompilerOutput& CompilerOutput
			):
			Type(InType),
			Target(CompilerOutput.Target),
			Code(CompilerOutput.Code),
			ParameterMap(CompilerOutput.ParameterMap),
			NumInstructions(CompilerOutput.NumInstructions)
		{}
	};

	FShader(): Type(NULL), NumRefs(0) {}
	FShader(const CompiledShaderInitializerType& Initializer);
	virtual ~FShader();

	/**
	 * Can be overridden by FShader subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment) {}

	// Serializer.
	virtual void Serialize(FArchive& Ar);

	// FRenderResource interface.
	virtual void InitRHI();
	virtual void ReleaseRHI();

	// Reference counting.
	void AddRef();
	void RemoveRef();
	virtual void FinishCleanup();

	/**
	* @return the shader's vertex shader
	*/
	const FVertexShaderRHIRef& GetVertexShader();
	/**
	* @return the shader's vertex shader
	*/
	const FPixelShaderRHIRef& GetPixelShader();

	/**
	* @return the shader's recompile group
	*/
	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_MISC;
	}

	// Accessors.
	const FGuid& GetId() const { return Id; }
	FShaderType* GetType() const { return Type; }
	const TArray<BYTE>& GetCode() const { return Key.Code; }
	const FShaderKey& GetKey() const { return Key; }
	UINT GetNumInstructions() const { return NumInstructions; }

	/**
	 * Serializes a shader reference by GUID.
	 */
	friend FArchive& operator<<(FArchive& Ar,FShader*& Ref);

private:

	FShaderKey Key;

	FShaderTarget Target;
	FVertexShaderRHIRef VertexShader;
	FPixelShaderRHIRef PixelShader;

	/** The shader type. */
	FShaderType* Type;

	/** A unique identifier for the shader. */
	FGuid Id;

	/** The number of references to this shader. */
	mutable UINT NumRefs;

	/** The reference from the shader code map to this shader. */
	FShader** CodeMapRef;

	/** The number of instructions the shader takes to execute. */
	UINT NumInstructions;
};

/**
 * An object which is used to serialize/deserialize, compile, and cache a particular shader class.
 */
class FShaderType
{
public:

	typedef class FShader* (*ConstructSerializedType)();
	typedef void (*ModifyCompilationEnvironmentType)(FShaderCompilerEnvironment&);

	/**
	* @return The global shader factory list.
	*/
	static TLinkedList<FShaderType*>*& GetTypeList();

	/**
	* @return The global shader name to type map
	*/
	static TMap<FName, FShaderType*>& GetNameToTypeMap();

	/**
	* Gets a list of FShaderTypes whose source file no longer matches what that type was compiled with
	*/
	static void GetOutdatedTypes(TArray<FShaderType*>& OutdatedShaderTypes);

	/**
	 * Minimal initialization constructor.
	 */
	FShaderType(
		const TCHAR* InName,
		const TCHAR* InSourceFilename,
		const TCHAR* InFunctionName,
		DWORD InFrequency,
		INT InMinPackageVersion,
		INT InMinLicenseePackageVersion,
		ConstructSerializedType InConstructSerializedRef,
		ModifyCompilationEnvironmentType InModifyCompilationEnvironmentRef
		):
		Name(InName),
		SourceFilename(InSourceFilename),
		FunctionName(InFunctionName),
		Frequency(InFrequency),
		MinPackageVersion(InMinPackageVersion),
		MinLicenseePackageVersion(InMinLicenseePackageVersion),
		ConstructSerializedRef(InConstructSerializedRef),
		ModifyCompilationEnvironmentRef(InModifyCompilationEnvironmentRef)
	{
		//make sure the name is shorter than the maximum serializable length
		check(appStrlen(InName) < NAME_SIZE);

		// register this shader type
		(new TLinkedList<FShaderType*>(this))->Link(GetTypeList());
		GetNameToTypeMap().Set(FName(InName), this);

		// Assign the vertex factory type the next unassigned hash index.
		static DWORD NextHashIndex = 0;
		HashIndex = NextHashIndex++;
	}

	/**
	 * Registers a shader for lookup by ID or code.
	 */
	void RegisterShader(FShader* Shader);

	/**
	 * Removes a shader from the ID and code lookup maps.
	 */
	void DeregisterShader(FShader* Shader);

	/**
	 * Finds a shader of this type with the specified output.
	 * @return NULL if no shader with the specified code was found.
	 */
	FShader* FindShaderByOutput(const FShaderCompilerOutput& Output) const;

	/**
	 * Finds a shader of this type by ID.
	 * @return NULL if no shader with the specified ID was found.
	 */
	FShader* FindShaderById(const FGuid& Id) const;

	/**
	 * Constructs a new instance of the shader type for deserialization.
	 */
	FShader* ConstructForDeserialization() const;

	// Accessors.
	DWORD GetFrequency() const 
	{ 
		return Frequency; 
	}
	INT GetMinPackageVersion() const 
	{ 
		return MinPackageVersion; 
	}
	INT GetMinLicenseePackageVersion() const
	{
		return MinLicenseePackageVersion;
	}
	const TCHAR* GetName() const 
	{ 
		return Name; 
	}
	const TCHAR* GetShaderFilename() const 
	{ 
		return SourceFilename; 
	}
	/** 
	 * Returns the number of shaders of this type.
	 *
	 * @return number of shaders in shader code map
	 */
	INT GetNumShaders() const
	{
		return ShaderCodeMap.Num();
	}

	/**
	 * Serializes a shader type reference by name.
	 */
	friend FArchive& operator<<(FArchive& Ar,FShaderType*& Ref);
	
	// Hash function.
	friend DWORD GetTypeHash(FShaderType* Ref)
	{
		return Ref ? Ref->HashIndex : 0;
	}

	/**
	* Calculates a CRC based on this shader type's source code and includes
	*/
	virtual DWORD GetSourceCRC();

	// Dynamic casts.
	virtual class FGlobalShaderType* GetGlobalShaderType() { return NULL; }
	virtual class FMaterialShaderType* GetMaterialShaderType() { return NULL; }
	virtual class FMeshMaterialShaderType* GetMeshMaterialShaderType() { return NULL; }

protected:

	/**
	 * Compiles a shader with the shader type's compilation parameters, using the provided shader environment.
	 * @param Compiler - The shader compiler to use.
	 * @param Environment - The environment to compile the shader in.
	 * @param Output - Contains the compiler output.
	 * @return True if the compilation succeeded.
	 */
	UBOOL CompileShader(EShaderPlatform Platform,const FShaderCompilerEnvironment& Environment,FShaderCompilerOutput& Output,UBOOL bSilent = FALSE);

private:
	DWORD HashIndex;
	const TCHAR* Name;
	const TCHAR* SourceFilename;
	const TCHAR* FunctionName;
	DWORD Frequency;
	INT MinPackageVersion;
	INT MinLicenseePackageVersion;

	ConstructSerializedType ConstructSerializedRef;
	ModifyCompilationEnvironmentType ModifyCompilationEnvironmentRef;

	/** A map from shader ID to shader.  A shader will be removed from it when deleted, so this doesn't need to use a TRefCountPtr. */
	TDynamicMap<FGuid,FShader*> ShaderIdMap;

	/**
	 * Functions to extract the shader code from a FShader* as a key for THashSet..
	 */
	struct FShaderCodeKeyFuncs
	{
		typedef FShaderKey KeyType;

		static const KeyType& GetSetKey(FShader* Shader)
		{
			return Shader->GetKey();
		}

		static UBOOL Matches(const KeyType& A, const KeyType& B)
		{
			return A.ParameterMapCRC == B.ParameterMapCRC && A.Code == B.Code;
		}

		static DWORD GetTypeHash(const KeyType& ShaderKey)
		{
			return appMemCrc(&ShaderKey.Code(0),ShaderKey.Code.Num()) ^ ShaderKey.ParameterMapCRC;
		}
	};

	/** A map from shader code to shader. */
	THashSet<FShader*,FShaderCodeKeyFuncs> ShaderCodeMap;
};

/**
 * A shader meta type for the simplest shaders; shaders which are not material or vertex factory linked.
 * There should only a single instance of each simple shader type.
 */
class FGlobalShaderType : public FShaderType
{
public:

	typedef FShader::CompiledShaderInitializerType CompiledShaderInitializerType;
	typedef FShader* (*ConstructCompiledType)(const CompiledShaderInitializerType&);
	typedef UBOOL (*ShouldCacheType)(EShaderPlatform);

	FGlobalShaderType(
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
	 * @param OutErrors - Upon compilation failure, OutErrors contains a list of the errors which occured.
	 * @return NULL if the compilation failed.
	 */
	FShader* CompileShader(EShaderPlatform Platform,TArray<FString>& OutErrors);

	/**
	 * Checks if the shader type should be cached for a particular platform.
	 * @param Platform - The platform to check.
	 * @return True if this shader type should be cached.
	 */
	UBOOL ShouldCache(EShaderPlatform Platform) const
	{
		return (*ShouldCacheRef)(Platform);
	}

	/**
	* Calculates a CRC based on this shader type's source code and includes
	*/
	virtual DWORD GetSourceCRC();

	// Dynamic casting.
	virtual FGlobalShaderType* GetGlobalShaderType() { return this; }

private:
	ConstructCompiledType ConstructCompiledRef;
	ShouldCacheType ShouldCacheRef;
};

/**
 * A macro to declare a new shader type.  This should be called in the class body of the new shader type.
 * @param ShaderClass - The name of the class representing an instance of the shader type.
 * @param ShaderMetaTypeShortcut - The shortcut for the shader meta type: simple, material, meshmaterial, etc.  The shader meta type
 *	controls 
 */
#define DECLARE_SHADER_TYPE(ShaderClass,ShaderMetaTypeShortcut) \
	public: \
	typedef F##ShaderMetaTypeShortcut##ShaderType ShaderMetaType; \
	static ShaderMetaType StaticType; \
	static FShader* ConstructSerializedInstance() { return new ShaderClass(); } \
	static FShader* ConstructCompiledInstance(const ShaderMetaType::CompiledShaderInitializerType& Initializer) \
	{ return new ShaderClass(Initializer); }

/**
 * A macro to implement a shader type.
 */
#define IMPLEMENT_SHADER_TYPE(TemplatePrefix,ShaderClass,SourceFilename,FunctionName,Frequency,MinPackageVersion,MinLicenseePackageVersion) \
	TemplatePrefix \
	ShaderClass::ShaderMetaType ShaderClass::StaticType( \
		TEXT(#ShaderClass), \
		SourceFilename, \
		FunctionName, \
		Frequency, \
		Max(VER_MIN_SHADER,MinPackageVersion), \
		Max(LICENSEE_VER_MIN_SHADER,MinLicenseePackageVersion), \
		ShaderClass::ConstructSerializedInstance, \
		ShaderClass::ConstructCompiledInstance, \
		ShaderClass::ModifyCompilationEnvironment, \
		ShaderClass::ShouldCache \
		);

/**
 * FGlobalShader
 * 
 * Global shaders derive from this class to set their default recompile group as a global one
 */
class FGlobalShader : public FShader
{
	DECLARE_SHADER_TYPE(FGlobalShader,Global);
public:
	FGlobalShader() : FShader() {}

	FGlobalShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FShader(Initializer) {}

	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_GLOBAL_MISC;
	}
};

/**
 * FGlobalBoundShaderStateRHIRef
 * 
 * Encapsulates the information that a global bound shader state requires
 */
class FGlobalBoundShaderStateRHIRef
{
public:
	/**
	 * The group that the bound shader state's pixel shader belongs to.  When that group is recompiled, 
	 * this bound shader state will be invalidated to propogate the new pixel shader. This is set in 
	 * SetGlobalBoundShaderState().  
	 */
	EShaderRecompileGroup ShaderRecompileGroup;
	FBoundShaderStateRHIRef BoundShaderState;

	FGlobalBoundShaderStateRHIRef();

	/**
	 * Initializes a global bound shader state with a vanilla bound shader state and required information.
	 */
	void Init(
		const FBoundShaderStateRHIRef& InBoundShaderState, 
		EShaderRecompileGroup InShaderRecompileGroup, 
		FGlobalShaderType* InGlobalVertexShaderType, 
		FGlobalShaderType* InGlobalPixelShaderType);

	void Invalidate()
	{
		BoundShaderState.Invalidate();
	}

	virtual ~FGlobalBoundShaderStateRHIRef();

	/**
	* A map used to find global bound shader states by global shader type and keep track of loaded global bound shader states
	*/
	static TMap<FGlobalShaderType*, FGlobalBoundShaderStateRHIRef*>& GetTypeToBoundStateMap();
};

typedef const FGlobalBoundShaderStateRHIRef& FGlobalBoundShaderStateRHIParamRef;

/**
 * A collection of shaders of different types, but the same meta type.
 */
template<typename ShaderMetaType>
class TShaderMap
{
public:

	/** Finds the shader with the given type.  Asserts on failure. */
	template<typename ShaderType>
	ShaderType* GetShader() const
	{
		const TRefCountPtr<FShader>* ShaderRef = Shaders.Find(&ShaderType::StaticType);
		checkf(ShaderRef != NULL && *ShaderRef != NULL, TEXT("Failed to find shader type %s"), ShaderType::StaticType.GetName());
		return (ShaderType*)(FShader*)*ShaderRef;
	}

	/** Finds the shader with the given type.  May return NULL. */
	FShader* GetShader(FShaderType* ShaderType) const
	{
		const TRefCountPtr<FShader>* ShaderRef = Shaders.Find(ShaderType);
		return ShaderRef ? (FShader*)*ShaderRef : NULL;
	}

	/** Finds the shader with the given type.  Asserts on failure. */
	UBOOL HasShader(ShaderMetaType* Type) const
	{
		const TRefCountPtr<FShader>* ShaderRef = Shaders.Find(Type);
		return ShaderRef != NULL && *ShaderRef != NULL;
	}

	void Serialize(FArchive& Ar)
	{
		// Serialize the shader references by factory and GUID.
		Ar << Shaders;
	}

	void AddShader(ShaderMetaType* Type,FShader* Shader)
	{
		Shaders.Set(Type,Shader);
	}

	/**
	 * Remvoes the shader of the given type from the shader map
	 * @param Type Shader type to remove the entry for 
	 */
	void RemoveShaderType(ShaderMetaType* Type)
	{
		Shaders.Remove(Type);
	}

	/**
	 * Builds a list of the shaders in a shader map.
	 */
	void GetShaderList(TMap<FGuid,FShader*>& OutShaders) const
	{
		for(TMap<FShaderType*,TRefCountPtr<FShader> >::TConstIterator ShaderIt(Shaders);ShaderIt;++ShaderIt)
		{
			if(ShaderIt.Value())
			{
				OutShaders.Set(ShaderIt.Value()->GetId(),ShaderIt.Value());
			}
		}
	}
	
	/**
	 * Begins initializing the shaders used by the shader map.
	 */
	void BeginInit()
	{
		for(TMap<FShaderType*,TRefCountPtr<FShader> >::TConstIterator ShaderIt(Shaders);ShaderIt;++ShaderIt)
		{
			if(ShaderIt.Value())
			{
				BeginInitResource(ShaderIt.Value());
			}
		}
	}

	/**
	 *	IsEmpty - Returns TRUE if the map is empty
	 */
	UBOOL IsEmpty()
	{
		return ((Shaders.Num() == 0) ? TRUE : FALSE);
	}

	/**
	 *	Empty - clears out all shaders held in the map
	 */
	void Empty()
	{
		Shaders.Empty();
	}

private:
	TMap<FShaderType*,TRefCountPtr<FShader> > Shaders;
};

/**
 * A reference which is initialized with the requested shader type from a shader map.
 */
template<typename ShaderType>
class TShaderMapRef
{
public:
	TShaderMapRef(const TShaderMap<typename ShaderType::ShaderMetaType>* ShaderIndex):
	 Shader(ShaderIndex->template GetShader<ShaderType>()) // gcc3 needs the template quantifier so it knows the < is not a less-than
	{}
	ShaderType* operator->() const
	{
		return Shader;
	}
	ShaderType* operator*() const
	{
		return Shader;
	}
private:
	ShaderType* Shader;
};

/**
 * A collection of persistent shaders.
 */
class UShaderCache : public UObject
{
	DECLARE_CLASS(UShaderCache,UObject,CLASS_Intrinsic,Engine);
private:
	/** Default constructor, private by design */
	UShaderCache() 
	{
	}

public:

	/**
	 * Constructor.
	 *
	 * @param	Platform	Platform this shader cache is for.
	 */
	UShaderCache( EShaderPlatform Platform );

	/**
	 * Flushes the shader map for a material from the platform cache.
	 * @param StaticParameters - The static parameter set identifying the material
	 * @param Platform Platform to flush.
	 */
	static void FlushId(const class FStaticParameterSet& StaticParameters, EShaderPlatform Platform);

	/**
	 * Flushes the shader map for a material from all caches.
	 * @param StaticParameters - The static parameter set identifying the material
	 */
	static void FlushId(const class FStaticParameterSet& StaticParameters);

	/**
	 * Finds a CRC stored for the given shaderType on the given platform
	 */
	static const DWORD* GetShaderTypeCRC(FShaderType* LookupShaderType, EShaderPlatform Platform);

	/**
	 * Finds a CRC stored for the given vertexFactoryType on the given platform
	 */
	static const DWORD* GetVertexFactoryTypeCRC(FVertexFactoryType* LookupVFType, EShaderPlatform Platform);

	/**
	 * Sets a CRC for the given shaderType on the given platform
	 */
	static void SetShaderTypeCRC(FShaderType* InShaderType, DWORD InCRC, EShaderPlatform Platform);

	/**
	 * Sets a CRC for the given vertexFactoryType on the given platform
	 */
	static void SetVertexFactoryTypeCRC(FVertexFactoryType* InVertexFactoryType, DWORD InCRC, EShaderPlatform Platform);

	/**
	 * Removes a FShaderType from the CRC map of all shader caches of the given platform.
	 */
	static void RemoveShaderTypeCRC(FShaderType* InShaderType, EShaderPlatform Platform);

	/**
	 * Removes a FVertexFactoryType from the CRC map of all shader caches of the given platform.
	 */
	static void RemoveVertexFactoryTypeCRC(FVertexFactoryType* InVertexFactoryType, EShaderPlatform Platform);


	/**
	 * Adds a material shader map to the cache fragment.
	 *
	 * @param MaterialShaderIndex - The shader map for the material.
	 */
	void AddMaterialShaderMap(class FMaterialShaderMap* MaterialShaderMap);

	// UObject interface.
	virtual void FinishDestroy();

	/**
	 * Presave function. Gets called once before an object gets serialized for saving.  Sorts the MaterialShaderMap
	 * maps so that shader cache serialization is deterministic.
	 */
	virtual void PreSave();
	virtual void Serialize(FArchive& Ar);

	// Accessors.
	UBOOL IsDirty() const 
	{ 
		return bDirty; 
	}

	// set the package as clean
	void MarkClean() 
	{
		bDirty = FALSE; 
	}
	// set the package as dirty
	void MarkDirty() 
	{ 
		bDirty = TRUE; 
	}

private:

	/** Map from shader type to the CRC that the shader type was compiled with */
	TMap<FShaderType*, DWORD> ShaderTypeCRCMap;

	/** Map from vertex factory type to the CRC that the shader type was compiled with */
	TMap<FVertexFactoryType*, DWORD> VertexFactoryTypeCRCMap;

	/** Map from material static parameter set to shader map.			*/
	TDynamicMap<FStaticParameterSet,TRefCountPtr<class FMaterialShaderMap> > MaterialShaderMap;
	/** Whether shader cache has been modified since the last save.	*/
	UBOOL bDirty;
	/** Platform this shader cache is for. */
	BYTE Platform;

	/**
	 * Copies over CRC entries from the ref cache if necessary,
	 * only called on the local shader cache before saving.
	 */
	void CompleteCRCMaps();
};

/**
 * Calculates a CRC for the given filename if it does not already exist in the CRC cache.
 * This handles nested includes through recursion.
 */
extern DWORD GetShaderFileCRC(const TCHAR* Filename);

/**
 * Flushes the shader file and CRC cache.
 */
extern void FlushShaderFileCache();

/**
 * Dumps shader stats to the log.
 */
extern void DumpShaderStats();

/**
 * Makes sure all global shaders are loaded and/or compiled for the passed in platform.
 *
 * @param	Platform	Platform to verify global shaders for
 */
extern void VerifyGlobalShaders(EShaderPlatform Platform=GRHIShaderPlatform);

/**
 * Returns the local shader cache for the passed in platform
 *
 * @param	Platform	Platform to return local shader cache for.
 * @return	The local shader cache for the passed in platform
 */
extern UShaderCache* GetLocalShaderCache( EShaderPlatform Platform );

/**
 * Saves the local shader cache for the passed in platform.
 *
 * @param	Platform	Platform to save shader cache for.
 * @param	OverrideCacheFilename If non-NULL, then the shader cache will be saved to the given path
 */
extern void SaveLocalShaderCache( EShaderPlatform Platform, const TCHAR* OverrideCacheFilename=NULL );

/**
 * Saves all local shader caches.
 */
extern void SaveLocalShaderCaches();

extern FString GetLocalShaderCacheFilename( EShaderPlatform Platform );

extern FString GetReferenceShaderCacheFilename( EShaderPlatform Platform );

/**
 * Accesses the global shader map.  This is a global TShaderMap<FGlobalShaderType> which contains an instance of each global shader type.
 *
 * @param Platform Which platform's global shader map to use
 * @return A reference to the global shader map.
 */
extern TShaderMap<FGlobalShaderType>* GetGlobalShaderMap(EShaderPlatform Platform=GRHIShaderPlatform);

/**
 * Forces a recompile of the global shaders.
 */
extern void RecompileGlobalShaders();

/**
* Recompiles the specified global shader types, and flushes their bound shader states.
*/
extern void RecompileGlobalShaders(const TArray<FShaderType*>& OutdatedShaderTypes);

/**
* Forces a recompile of only the specified group of global shaders. 
* Also invalidates global bound shader states so that the new shaders will be used.
*/
extern void RecompileGlobalShaderGroup(EShaderRecompileGroup FlushGroup);

/**
 * Finds the shader type with a given name.
 *
 * @param ShaderTypeName - The name of the shader type to find.
 * @return The shader type, or NULL if none matched.
 */
extern FShaderType* FindShaderTypeByName(const TCHAR* ShaderTypeName);

/**
* SetGlobalBoundShaderState - sets the global bound shader state, also creates and caches it if necessary
*
* @param Context - command buffer context
* @param BoundShaderState - current bound shader state, will be updated if it wasn't a valid ref
* @param VertexDeclaration - the vertex declaration to use in creating the new bound shader state
* @param VertexShader - the vertex shader to use in creating the new bound shader state
* @param PixelShader - the pixel shader to use in creating the new bound shader state
* @param Stride
*/
extern void SetGlobalBoundShaderState(
	FCommandContextRHI* Context,
	FGlobalBoundShaderStateRHIRef &BoundShaderState,
	FVertexDeclarationRHIParamRef VertexDeclaration,
	FShader* VertexShader,
	FShader* PixelShader,
	UINT Stride);

/**
 * An internal dummy pixel shader to use when the user calls RHISetPixelShader(Context,NULL).
 */
class FNULLPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FNULLPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	FNULLPixelShader( )	{ }
	FNULLPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
	}
};

/** Encapsulates scene texture shader parameter bindings. */
class FSceneTextureShaderParameters
{
public:

	/** Binds the parameters using a compiled shader's parameter map. */
	void Bind(const FShaderParameterMap& ParameterMap);

	/** Sets the scene texture parameters for the given view. */
	void Set(FCommandContextRHI* Context,const FSceneView* View,FShader* PixelShader, ESamplerFilter ColorFilter=SF_Nearest) const;

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FSceneTextureShaderParameters& P);

private:
	/** The SceneColorTexture parameter for materials that use SceneColor */
	FShaderParameter SceneColorTextureParameter;
	/** The SceneDepthTexture parameter for materials that use SceneDepth */
	FShaderParameter SceneDepthTextureParameter;
	/** Required parameter for using SceneDepthTexture on certain platforms. */
	FShaderParameter SceneDepthCalcParameter;
	/** Required parameter for using SceneColorTexture. */
	FShaderParameter ScreenPositionScaleBiasParameter;
};

/**
 * Vertex shader for rendering a single, constant color.
 */
class FOneColorVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FOneColorVertexShader,Global);
public:
	FOneColorVertexShader( )	{ }
	FOneColorVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	:	FGlobalShader( Initializer )
	{
	}

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}
};

/**
 * Pixel shader for rendering a single, constant color.
 */
class FOneColorPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FOneColorPixelShader,Global);
public:
	FOneColorPixelShader( )	{ }
	FOneColorPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	:	FGlobalShader( Initializer )
	{
		ColorParameter.Bind( Initializer.ParameterMap, TEXT("DrawColor") );
	}

	// FShader interface.
	virtual void Serialize(FArchive& Ar)
	{
		FGlobalShader::Serialize(Ar);
		Ar << ColorParameter;
	}
	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	/** The parameter to use for setting the draw Color. */
	FShaderParameter ColorParameter;
};
