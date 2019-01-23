/*=============================================================================
	ShaderManager.cpp: Shader manager implementation
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UShaderCache);

// local and reference shader cache type IDs.

enum EShaderCacheType
{
	SC_Local				= 0,
	SC_Reference			= 1,

	SC_NumShaderCacheTypes	= 2,
};

/** Global shader caches. Not handled by GC and code associating objects is responsible for adding them to the root set. */
UShaderCache* GShaderCaches[SC_NumShaderCacheTypes][SP_NumPlatforms];

UBOOL GSerializingLocalShaderCache = FALSE;

/** Controls discarding of shaders whose source .usf file has changed since the shader was compiled. */
UBOOL GAutoReloadChangedShaders = FALSE;

/** The global shader map. */
TShaderMap<FGlobalShaderType>* GGlobalShaderMap[SP_NumPlatforms];

/** The shader file cache, used to minimize shader file reads */
TMap<FFilename, FString> GShaderFileCache;

/** The shader file CRC cache, used to minimize loading and CRC'ing shader files */
TMap<FString, DWORD> GShaderCRCCache;

IMPLEMENT_SHADER_TYPE(,FNULLPixelShader,TEXT("NULLPixelShader"),TEXT("Main"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(,FOneColorVertexShader,TEXT("OneColorShader"),TEXT("MainVertexShader"),SF_Vertex,0,0);
IMPLEMENT_SHADER_TYPE(,FOneColorPixelShader,TEXT("OneColorShader"),TEXT("MainPixelShader"),SF_Pixel,0,0);

UBOOL FShaderParameterMap::FindParameterAllocation(const TCHAR* ParameterName,WORD& OutBaseRegisterIndex,WORD& OutNumRegisters) const
{
	const FParameterAllocation* Allocation = ParameterMap.Find(ParameterName);
	if(Allocation)
	{
		OutBaseRegisterIndex = Allocation->BaseRegisterIndex;
		OutNumRegisters = Allocation->NumRegisters;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void FShaderParameterMap::AddParameterAllocation(const TCHAR* ParameterName,WORD BaseRegisterIndex,WORD NumRegisters)
{
	FParameterAllocation Allocation;
	Allocation.BaseRegisterIndex = BaseRegisterIndex;
	Allocation.NumRegisters = NumRegisters;
	ParameterMap.Set(ParameterName,Allocation);
}

void FShaderParameter::Bind(const FShaderParameterMap& ParameterMap,const TCHAR* ParameterName,UBOOL bIsOptional)
{
	if(!ParameterMap.FindParameterAllocation(ParameterName,BaseRegisterIndex,NumRegisters) && !bIsOptional)
	{
		// Error: Wasn't able to bind a non-optional parameter.
		appErrorf(TEXT("Failure to bind non-optional shader parameter %s"),ParameterName);
	}
}

FArchive& operator<<(FArchive& Ar,FShaderParameter& P)
{
	return Ar << P.BaseRegisterIndex << P.NumRegisters;
}

TLinkedList<FShaderType*>*& FShaderType::GetTypeList()
{
	static TLinkedList<FShaderType*>* TypeList = NULL;
	return TypeList;
}

/**
* @return The global shader name to type map
*/
TMap<FName, FShaderType*>& FShaderType::GetNameToTypeMap()
{
	static TMap<FName, FShaderType*> NameToTypeMap;
	return NameToTypeMap;
}

/**
* Gets a list of FShaderTypes whose source file no longer matches what that type was compiled with
*/
void FShaderType::GetOutdatedTypes(TArray<FShaderType*>& OutdatedShaderTypes)
{
	for(TLinkedList<FShaderType*>::TIterator It(GetTypeList()); It; It.Next())
	{
		//find the CRC that the shader type was compiled with
		const DWORD* ShaderTypeCRC = UShaderCache::GetShaderTypeCRC(*It, GRHIShaderPlatform);
		//calculate the current CRC
		DWORD CurrentCRC = It->GetSourceCRC();

		//if they are not equal, the current type is outdated
		if (ShaderTypeCRC != NULL && CurrentCRC != *ShaderTypeCRC)
		{
			warnf(TEXT("ShaderType %s is outdated"), It->GetName());
			OutdatedShaderTypes.Push(*It);
			//remove the type's crc from the cache, since we know it is outdated now
			//this is necessary since the shader may not be compiled before the next CRC check and it will just be reported as outdated again
			UShaderCache::RemoveShaderTypeCRC(*It, GRHIShaderPlatform);
		}
	}
}

FArchive& operator<<(FArchive& Ar,FShaderType*& Ref)
{
	if(Ar.IsSaving())
	{
		FName FactoryName = Ref ? FName(Ref->Name) : NAME_None;
		Ar << FactoryName;
	}
	else if(Ar.IsLoading())
	{
		FName FactoryName = NAME_None;
		Ar << FactoryName;
		
		Ref = NULL;

		if(FactoryName != NAME_None)
		{
			// look for the shader type in the global name to type map
			FShaderType** ShaderType = FShaderType::GetNameToTypeMap().Find(FactoryName);
			if (ShaderType)
			{
				// if we found it, use it
				Ref = *ShaderType;
			}
		}
	}
	return Ar;
}

void FShaderType::RegisterShader(FShader* Shader)
{
	ShaderIdMap.Set(Shader->GetId(),Shader);
	Shader->CodeMapRef = ShaderCodeMap.Add(Shader);
}

void FShaderType::DeregisterShader(FShader* Shader)
{
	ShaderIdMap.Remove(Shader->GetId());
	ShaderCodeMap.Remove(Shader->CodeMapRef);
}

FShader* FShaderType::FindShaderByOutput(const FShaderCompilerOutput& Output) const
{
	FShader*const* CodeMapRef = ShaderCodeMap.Find(FShaderKey(Output.Code, Output.ParameterMap));
	return CodeMapRef ? *CodeMapRef : NULL;
}

FShader* FShaderType::FindShaderById(const FGuid& Id) const
{
	return ShaderIdMap.FindRef(Id);
}

FShader* FShaderType::ConstructForDeserialization() const
{
	return (*ConstructSerializedRef)();
}

/**
* Calculates a CRC based on this shader type's source code and includes
*/
DWORD FShaderType::GetSourceCRC()
{
	return GetShaderFileCRC(GetShaderFilename());
}

UBOOL FShaderType::CompileShader(EShaderPlatform Platform,const FShaderCompilerEnvironment& InEnvironment,FShaderCompilerOutput& Output,UBOOL bSilent)
{
	// Allow the shader type to modify its compile environment.
	FShaderCompilerEnvironment Environment = InEnvironment;
	(*ModifyCompilationEnvironmentRef)(Environment);

	// Construct shader target for the shader type's frequency and the specified platform.
	FShaderTarget Target;
	Target.Platform = Platform;
	Target.Frequency = Frequency;

	// Compile the shader environment passed in with the shader type's source code.
	return ::CompileShader(
		SourceFilename,
		FunctionName,
		Target,
		Environment,
		Output,
		bSilent
		);
}

FShader* FGlobalShaderType::CompileShader(EShaderPlatform Platform,TArray<FString>& OutErrors)
{
	// Construct the shader environment.
	FShaderCompilerEnvironment Environment;

	// Compile the shader code.
	FShaderCompilerOutput Output;

	//calculate the CRC for this global shader type and store it in the shader cache.
	//this will be used to determine when global shader files have changed.
	DWORD CurrentCRC = GetSourceCRC();
	UShaderCache::SetShaderTypeCRC(this, CurrentCRC, Platform);

	STAT(DOUBLE GlobalShaderCompilingTime = 0);
	{
		SCOPE_SECONDS_COUNTER(GlobalShaderCompilingTime);
		if(!FShaderType::CompileShader(Platform,Environment,Output))
		{
			OutErrors = Output.Errors;
			return NULL;
		}
	}
	INC_FLOAT_STAT_BY(STAT_ShaderCompiling_GlobalShaders,(FLOAT)GlobalShaderCompilingTime);
	
	// Check for shaders with identical compiled code.
	FShader* Shader = FindShaderByOutput(Output);
	if(!Shader)
	{
		// Create the shader.
		Shader = (*ConstructCompiledRef)(CompiledShaderInitializerType(this,Output));
	}
	return Shader;
}

/**
* Calculates a CRC based on this shader type's source code and includes
*/
DWORD FGlobalShaderType::GetSourceCRC()
{
	return FShaderType::GetSourceCRC();
}

FShader::FShader(const FGlobalShaderType::CompiledShaderInitializerType& Initializer):
	Key(Initializer.Code, Initializer.ParameterMap),
	Target(Initializer.Target),
	Type(Initializer.Type),
	NumRefs(0),
	NumInstructions(Initializer.NumInstructions)
{
	Id = appCreateGuid();
	Type->RegisterShader(this);

	INC_DWORD_STAT_BY((Target.Frequency == SF_Vertex) ? STAT_VertexShaderMemory : STAT_PixelShaderMemory, Key.Code.Num());
}

FShader::~FShader()
{
	DEC_DWORD_STAT_BY((Target.Frequency == SF_Vertex) ? STAT_VertexShaderMemory : STAT_PixelShaderMemory, Key.Code.Num());
}

/**
* @return the shader's vertex shader
*/
const FVertexShaderRHIRef& FShader::GetVertexShader() 
{ 
	// If the shader resource hasn't been initialized yet, initialize it.
	// In game, shaders are always initialized on load, so this check isn't necessary.
	if(GIsEditor && !IsInitialized())
	{
		Init();
	}

	return VertexShader; 
}

/**
* @return the shader's pixel shader
*/
const FPixelShaderRHIRef& FShader::GetPixelShader() 
{ 
	// If the shader resource hasn't been initialized yet, initialize it.
	// In game, shaders are always initialized on load, so this check isn't necessary.
	if(GIsEditor && !IsInitialized())
	{
		Init();
	}

	return PixelShader; 
}

void FShader::Serialize(FArchive& Ar)
{
	BYTE TargetPlatform = Target.Platform;
	BYTE TargetFrequency = Target.Frequency;
	Ar << TargetPlatform << TargetFrequency;
	Target.Platform = TargetPlatform;
	Target.Frequency = TargetFrequency;

	if (Ar.Ver() < VER_PARAMETER_MAP_COMPARISON)
	{
		Ar << Key.Code;
		Key.ParameterMapCRC = 0;
	}
	else
	{
		Ar << Key.Code;
		Ar << Key.ParameterMapCRC;
	}

	Ar << Id << Type;
	if(Ar.IsLoading())
	{
		Type->RegisterShader(this);
	}

	if(Ar.Ver() >= VER_SHADER_NUMINSTRUCTIONS)
	{
		Ar << NumInstructions;
	}

	INC_DWORD_STAT_BY((Target.Frequency == SF_Vertex) ? STAT_VertexShaderMemory : STAT_PixelShaderMemory, Key.Code.Num());
}

void FShader::InitRHI()
{
	// we can't have this called on the wrong platform's shaders
	if (Target.Platform != GRHIShaderPlatform)
	{
#if CONSOLE
		appErrorf( TEXT("FShader::Init got platform %i but expected %i"), (INT) Target.Platform, (INT) GRHIShaderPlatform );
#endif
		return;
	}
	check(Key.Code.Num());
	if(Target.Frequency == SF_Vertex)
	{
		VertexShader = RHICreateVertexShader(Key.Code);
	}
	else if(Target.Frequency == SF_Pixel)
	{
		PixelShader = RHICreatePixelShader(Key.Code);
	}

#if CONSOLE
	// Memory has been duplicated at this point.
	DEC_DWORD_STAT_BY((Target.Frequency == SF_Vertex) ? STAT_VertexShaderMemory : STAT_PixelShaderMemory, Key.Code.Num());
	Key.Code.Empty();
#endif
}

void FShader::ReleaseRHI()
{
	VertexShader.Release();
	PixelShader.Release();
}

void FShader::AddRef()
{
	++NumRefs;
}

void FShader::RemoveRef()
{
	if(--NumRefs == 0)
	{
		// Deregister the shader now to eliminate references to it by the type's ShaderIdMap and ShaderCodeMap.
		Type->DeregisterShader(this);

		// Send a release message to the rendering thread when the shader loses its last reference.
		BeginReleaseResource(this);

		BeginCleanup(this);
	}
}

void FShader::FinishCleanup()
{
	delete this;
}

FArchive& operator<<(FArchive& Ar,FShader*& Ref)
{
	if(Ar.IsSaving())
	{
		if(Ref)
		{
			// Serialize the shader's ID and type.
			FGuid ShaderId = Ref->GetId();
			FShaderType* ShaderType = Ref->GetType();
			Ar << ShaderId << ShaderType;
		}
		else
		{
			FGuid ShaderId(0,0,0,0);
			FShaderType* ShaderType = NULL;
			Ar << ShaderId << ShaderType;
		}
	}
	else if(Ar.IsLoading())
	{
		// Deserialize the shader's ID and type.
		FGuid ShaderId;
		FShaderType* ShaderType = NULL;
		Ar << ShaderId << ShaderType;

		Ref = NULL;

		if(ShaderType)
		{
			// Find the shader using the ID and type.
			Ref = ShaderType->FindShaderById(ShaderId);
		}
	}

	return Ar;
}

/**
 * Adds this to the list of global bound shader states 
 */
FGlobalBoundShaderStateRHIRef::FGlobalBoundShaderStateRHIRef() :
	ShaderRecompileGroup(SRG_GLOBAL_MISC)
{
}

/**
 * Initializes a global bound shader state with a vanilla bound shader state and required information.
 */
void FGlobalBoundShaderStateRHIRef::Init(
	const FBoundShaderStateRHIRef& InBoundShaderState, 
	EShaderRecompileGroup InShaderRecompileGroup, 
	FGlobalShaderType* InGlobalVertexShaderType, 
	FGlobalShaderType* InGlobalPixelShaderType)
{
	BoundShaderState = InBoundShaderState;
	ShaderRecompileGroup = InShaderRecompileGroup;
	
	//make sure that we are only accessing these maps from the rendering thread
	check(IsInRenderingThread());

	//add a shaderType to global bound shader state map so that we can iterate over all
	//global bound shader states and look them up by shaderType
	GetTypeToBoundStateMap().Set(InGlobalVertexShaderType, this);
	GetTypeToBoundStateMap().Set(InGlobalPixelShaderType, this);
}

/**
 * Removes this from the list of global bound shader states 
 */
FGlobalBoundShaderStateRHIRef::~FGlobalBoundShaderStateRHIRef()
{
	TMap<FGlobalShaderType*, FGlobalBoundShaderStateRHIRef*>& TypeToBoundStateMap = GetTypeToBoundStateMap();

	//find and remove one of the entries (either the vertex or pixel shader type)
	FGlobalShaderType** GlobalShaderType = TypeToBoundStateMap.FindKey(this);
	if (GlobalShaderType && *GlobalShaderType)
	{
		TypeToBoundStateMap.Remove(*GlobalShaderType);

		//find and remove the second entry
		GlobalShaderType = TypeToBoundStateMap.FindKey(this);
		if (GlobalShaderType && *GlobalShaderType)
		{
			TypeToBoundStateMap.Remove(*GlobalShaderType);
		}
	}
}

/**
 * A map used to find global bound shader states by global shader type and keep track of loaded global bound shader states
 */
TMap<FGlobalShaderType*, FGlobalBoundShaderStateRHIRef*>& FGlobalBoundShaderStateRHIRef::GetTypeToBoundStateMap()
{
	static TMap<FGlobalShaderType*, FGlobalBoundShaderStateRHIRef*> TypeToBoundStateMap;
	return TypeToBoundStateMap;
}

/**
 * Constructor.
 *
 * @param	InPlatform	Platform this shader cache is for.
 */
UShaderCache::UShaderCache( EShaderPlatform InPlatform )
:	Platform( InPlatform )
{}

/**
 * Flushes the shader map for a material from the cache.
 * @param StaticParameters - The static parameter set identifying the material
 * @param Platform Platform to flush.
 */
void UShaderCache::FlushId(const FStaticParameterSet& StaticParameters, EShaderPlatform Platform)
{
	UShaderCache* ShaderCache = GShaderCaches[SC_Local][Platform];
	if (ShaderCache)
	{
		// Remove the shaders cached for this material ID from the shader cache.
		ShaderCache->MaterialShaderMap.Remove(StaticParameters);
		// make sure the reference in the map is removed
		ShaderCache->MaterialShaderMap.Shrink();
		ShaderCache->bDirty = TRUE;
	}
}

/**
* Flushes the shader map for a material from all caches.
* @param StaticParameters - The static parameter set identifying the material
*/
void UShaderCache::FlushId(const class FStaticParameterSet& StaticParameters)
{
	for( INT PlatformIndex=0; PlatformIndex<SP_NumPlatforms; PlatformIndex++ )
	{
		UShaderCache::FlushId( StaticParameters, (EShaderPlatform)PlatformIndex );
	}
}

/**
 * Adds a material shader map to the cache fragment.
 */
void UShaderCache::AddMaterialShaderMap(FMaterialShaderMap* InMaterialShaderMap)
{
	MaterialShaderMap.Set(InMaterialShaderMap->GetMaterialId(),InMaterialShaderMap);
	bDirty = TRUE;
}

void UShaderCache::FinishDestroy()
{
	for( INT TypeIndex=0; TypeIndex<SC_NumShaderCacheTypes; TypeIndex++ )
	{
		for( INT PlatformIndex=0; PlatformIndex<SP_NumPlatforms; PlatformIndex++ )
		{
			if( GShaderCaches[TypeIndex][PlatformIndex] == this )
			{
				// The shader cache is a root object, but it will still be purged on exit.  Make sure there isn't a dangling reference to it.
				GShaderCaches[TypeIndex][PlatformIndex] = NULL;
			}
		}
	}

	Super::FinishDestroy();
}

/**
 * Finds a CRC stored for the given shaderType on the given platform
 */
const DWORD* UShaderCache::GetShaderTypeCRC(FShaderType* LookupShaderType, EShaderPlatform Platform)
{
	for( INT TypeIndex=0; TypeIndex<SC_NumShaderCacheTypes; TypeIndex++ )
	{
		UShaderCache* ShaderCache = GShaderCaches[TypeIndex][Platform];
		if (ShaderCache)
		{
			const DWORD* ShaderTypeCRC = ShaderCache->ShaderTypeCRCMap.Find(LookupShaderType);
			//return the first one found, so that local shader cache entries will override ref shader cache entries
			if (ShaderTypeCRC)
			{
				return ShaderTypeCRC;
			}
		}
	}
	return NULL;
}

/**
 * Finds a CRC stored for the given vertexFactoryType on the given platform
 */
const DWORD* UShaderCache::GetVertexFactoryTypeCRC(FVertexFactoryType* LookupVFType, EShaderPlatform Platform)
{
	for( INT TypeIndex=0; TypeIndex<SC_NumShaderCacheTypes; TypeIndex++ )
	{
		UShaderCache* ShaderCache = GShaderCaches[TypeIndex][Platform];
		if (ShaderCache)
		{
			const DWORD* VFTypeCRC = ShaderCache->VertexFactoryTypeCRCMap.Find(LookupVFType);
			//return the first one found, so that local shader cache entries will override ref shader cache entries
			if (VFTypeCRC)
			{
				return VFTypeCRC;
			}
		}
	}
	return NULL;
}

/**
 * Sets a CRC for the given shaderType on the given platform
 */
void UShaderCache::SetShaderTypeCRC(FShaderType* InShaderType, DWORD InCRC, EShaderPlatform Platform)
{
	UShaderCache* ShaderCache = GShaderCaches[SC_Local][Platform];
	if (ShaderCache)
	{
		ShaderCache->ShaderTypeCRCMap.Set(InShaderType, InCRC);
	}
}

/**
 * Sets a CRC for the given vertexFactoryType on the given platform
 */
void UShaderCache::SetVertexFactoryTypeCRC(FVertexFactoryType* InVertexFactoryType, DWORD InCRC, EShaderPlatform Platform)
{
	UShaderCache* ShaderCache = GShaderCaches[SC_Local][Platform];
	if (ShaderCache)
	{
		ShaderCache->VertexFactoryTypeCRCMap.Set(InVertexFactoryType, InCRC);
	}
}

/**
 * Removes a FShaderType from the CRC map of all shader caches of the given platform.
 */
void UShaderCache::RemoveShaderTypeCRC(FShaderType* InShaderType, EShaderPlatform Platform)
{
	for( INT TypeIndex=0; TypeIndex<SC_NumShaderCacheTypes; TypeIndex++ )
	{
		UShaderCache* ShaderCache = GShaderCaches[TypeIndex][Platform];
		if (ShaderCache)
		{
			ShaderCache->ShaderTypeCRCMap.Remove(InShaderType);
		}
	}
}

/**
 * Removes a FVertexFactoryType from the CRC map of all shader caches of the given platform.
 */
void UShaderCache::RemoveVertexFactoryTypeCRC(FVertexFactoryType* InVertexFactoryType, EShaderPlatform Platform)
{
	for( INT TypeIndex=0; TypeIndex<SC_NumShaderCacheTypes; TypeIndex++ )
	{
		UShaderCache* ShaderCache = GShaderCaches[TypeIndex][Platform];
		if (ShaderCache)
		{
			ShaderCache->VertexFactoryTypeCRCMap.Remove(InVertexFactoryType);
		}
	}
}


/**
 * Copies over CRC entries from the ref cache if necessary,
 * only called on the local shader cache before saving.
 */
void UShaderCache::CompleteCRCMaps()
{
	if (GShaderCaches[SC_Reference][Platform])
	{
		//copy over the shaderType to CRC entries from the ref to the local shader cache
		//this is needed because CRC entries are only generated for the cache that the shader was compiled for
		//but shaders can be put into caches that they were not compiled for, and each shaderType will no longer have a CRC entry in that cache
		//(local cache is always saved with all loaded shaders, not just the ones that needed a recompile)
		for(TLinkedList<FShaderType*>::TIterator It(FShaderType::GetTypeList()); It; It.Next())
		{
			const DWORD* RefShaderTypeCRC = GShaderCaches[SC_Reference][Platform]->ShaderTypeCRCMap.Find(*It);
			const DWORD* LocalShaderTypeCRC = ShaderTypeCRCMap.Find(*It);
			//if the shader type's CRC was found in the ref cache but not in the local, copy it over
			if (RefShaderTypeCRC && !LocalShaderTypeCRC)
			{
				ShaderTypeCRCMap.Set(*It, *RefShaderTypeCRC);
			}
		}

		for(TLinkedList<FVertexFactoryType*>::TIterator It(FVertexFactoryType::GetTypeList()); It; It.Next())
		{
			const DWORD* RefVFTypeCRC = GShaderCaches[SC_Reference][Platform]->VertexFactoryTypeCRCMap.Find(*It);
			const DWORD* LocalVFTypeCRC = VertexFactoryTypeCRCMap.Find(*It);
			//if the vertex factory type's CRC was found in the ref cache but not in the local, copy it over
			if (RefVFTypeCRC && !LocalVFTypeCRC)
			{
				VertexFactoryTypeCRCMap.Set(*It, *RefVFTypeCRC);
			}
		}
	}
}

IMPLEMENT_COMPARE_CONSTREF(FStaticParameterSet,SortMaterialsByStaticParamSet,
{
	for ( INT i = 0; i < 4; i++ )
	{
		if ( A.BaseMaterialId[i] > B.BaseMaterialId[i] )
		{
			return 1;
		}
		else if ( A.BaseMaterialId[i] < B.BaseMaterialId[i] )
		{
			return -1;
		}
	}

	if (A.StaticSwitchParameters.Num() > B.StaticSwitchParameters.Num())
	{
		return 1;
	}
	else if (A.StaticSwitchParameters.Num() < B.StaticSwitchParameters.Num())
	{
		return -1;
	}

	for (INT SwitchIndex = 0; SwitchIndex < A.StaticSwitchParameters.Num(); SwitchIndex++)
	{
		const FStaticSwitchParameter &SwitchA = A.StaticSwitchParameters(SwitchIndex);
		const FStaticSwitchParameter &SwitchB = B.StaticSwitchParameters(SwitchIndex);

		if (SwitchA.ParameterName.ToString() != SwitchB.ParameterName.ToString())
		{
			return (SwitchA.ParameterName.ToString() > SwitchB.ParameterName.ToString()) * 2 - 1;
		} 

		if (SwitchA.Value != SwitchB.Value) { return (SwitchA.Value > SwitchB.Value) * 2 - 1; } 
	}

	if (A.StaticComponentMaskParameters.Num() > B.StaticComponentMaskParameters.Num())
	{
		return 1;
	}
	else if (A.StaticComponentMaskParameters.Num() < B.StaticComponentMaskParameters.Num())
	{
		return -1;
	}

	for (INT MaskIndex = 0; MaskIndex < A.StaticComponentMaskParameters.Num(); MaskIndex++)
	{
		const FStaticComponentMaskParameter &MaskA = A.StaticComponentMaskParameters(MaskIndex);
		const FStaticComponentMaskParameter &MaskB = B.StaticComponentMaskParameters(MaskIndex);

		if (MaskA.ParameterName.ToString() != MaskB.ParameterName.ToString())
		{
			return (MaskA.ParameterName.ToString() > MaskB.ParameterName.ToString()) * 2 - 1;
		} 

		if (MaskA.R != MaskB.R) { return (MaskA.R > MaskB.R) * 2 - 1; } 
		if (MaskA.G != MaskB.G) { return (MaskA.G > MaskB.G) * 2 - 1; } 
		if (MaskA.B != MaskB.B) { return (MaskA.B > MaskB.B) * 2 - 1; } 
		if (MaskA.A != MaskB.A) { return (MaskA.A > MaskB.A) * 2 - 1; } 
	}

	return 0;
})

/**
 * Presave function. Gets called once before an object gets serialized for saving.  Sorts the MaterialShaderMap
 * maps so that shader cache serialization is deterministic.
 */
void UShaderCache::PreSave()
{
	Super::PreSave();
	MaterialShaderMap.KeySort<COMPARE_CONSTREF_CLASS(FStaticParameterSet,SortMaterialsByStaticParamSet)>();
}

void UShaderCache::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << Platform;

	if(Ar.IsSaving())
	{
		// the reference shader should never be saved (when it's being created, it's actually the local shader cache for that run of the commandlet)
//		check(this != GShaderCaches[SC_Reference][Platform]);

		// Find the shaders used by materials in the cache.
		TMap<FGuid,FShader*> Shaders;
	
		for( TDynamicMap<FStaticParameterSet,TRefCountPtr<FMaterialShaderMap> >::TIterator MaterialIt(MaterialShaderMap); MaterialIt; ++MaterialIt )
		{
			MaterialIt.Value()->GetShaderList(Shaders);
		}
		GetGlobalShaderMap((EShaderPlatform)Platform)->GetShaderList(Shaders);
	
		CompleteCRCMaps();
		Ar << ShaderTypeCRCMap;
		Ar << VertexFactoryTypeCRCMap;

		// Save the shaders in the cache.
		INT NumShaders = Shaders.Num();
		Ar << NumShaders;
		for(TMap<FGuid,FShader*>::TIterator ShaderIt(Shaders);ShaderIt;++ShaderIt)
		{
			FShader* Shader = ShaderIt.Value();

			// Serialize the shader type and ID separately, so at load time we can see whether this is a redundant shader without fully
			// deserializing it.
			FShaderType* ShaderType = Shader->GetType();
			FGuid ShaderId = Shader->GetId();
			Ar << ShaderType << ShaderId;

			// Write a placeholder value for the skip offset.
			INT SkipOffset = Ar.Tell();
			Ar << SkipOffset;

			// Serialize the shader.
			Shader->Serialize(Ar);

			// Write the actual offset of the end of the shader data over the placeholder value written above.
			INT EndOffset = Ar.Tell();
			Ar.Seek(SkipOffset);
			Ar << EndOffset;
			Ar.Seek(EndOffset);
		}

		// Save the material shader indices in the cache.
		INT NumMaterialShaderMaps = MaterialShaderMap.Num();
		Ar << NumMaterialShaderMaps;
		for( TDynamicMap<FStaticParameterSet,TRefCountPtr<FMaterialShaderMap> >::TIterator MaterialIt(MaterialShaderMap); MaterialIt; ++MaterialIt )
		{
			// Serialize the material static parameter set separate, so at load time we can see whether this is a redundant material shader map without fully
			// deserializing it.
			FStaticParameterSet StaticParameters = MaterialIt.Key();
			StaticParameters.Serialize(Ar);

			// Write a placeholder value for the skip offset.
			INT SkipOffset = Ar.Tell();
			Ar << SkipOffset;

			// Serialize the material shader map.
			MaterialIt.Value()->Serialize(Ar);

			// Write the actual offset of the end of the material shader map data over the placeholder value written above.
			INT EndOffset = Ar.Tell();
			Ar.Seek(SkipOffset);
			Ar << EndOffset;
			Ar.Seek(EndOffset);
		}

		// Save the global shader map
		check( ARRAY_COUNT(GGlobalShaderMap) == SP_NumPlatforms );
		GetGlobalShaderMap((EShaderPlatform)Platform)->Serialize(Ar);

		// Mark the cache as not dirty.
		bDirty = FALSE;
	}
	else if(Ar.IsLoading())
	{
		if (Ar.Ver() >= VER_SHADER_CRC_CHECKING)
		{
			Ar << ShaderTypeCRCMap;
			Ar << VertexFactoryTypeCRCMap;
		}

		INT NumShaders			= 0;
		INT NumRedundantShaders = 0;
		INT NumLegacyShaders	= 0;
		TArray<FString> OutdatedShaderTypes;

		Ar << NumShaders;

		// Load the shaders in the cache.
		for( INT ShaderIndex=0; ShaderIndex<NumShaders; ShaderIndex++ )
		{
			// Deserialize the shader type and shader ID.
			FShaderType* ShaderType = NULL;
			FGuid ShaderId;
			Ar << ShaderType << ShaderId;

			// Deserialize the offset of the next shader.
			INT SkipOffset = 0;
			Ar << SkipOffset;

			if(!ShaderType)
			{
				// If the shader type doesn't exist anymore, skip the shader.
				Ar.Seek(SkipOffset);
				NumLegacyShaders++;
			}
			else
			{
				DWORD SavedCRC = 0;
				DWORD CurrentCRC = 0;

				if (GAutoReloadChangedShaders)
				{
					//calculate the source file's CRC to see if it has changed since it was last compiled
					CurrentCRC = ShaderType->GetSourceCRC();
					const DWORD* ShaderTypeCRC = ShaderTypeCRCMap.Find(ShaderType);
					if (ShaderTypeCRC)
					{
						SavedCRC = *ShaderTypeCRC;
					}
					else
					{
						//the CRC was not saved for this type.  
						SavedCRC = CurrentCRC;
					}
				}

				FShader* Shader = ShaderType->FindShaderById(ShaderId);
				if(Shader)
				{
					// If a shader with the same type and ID is already resident, skip this shader.
					Ar.Seek(SkipOffset);
					NumRedundantShaders++;
				}
				else if(GAutoReloadChangedShaders && SavedCRC != CurrentCRC)
				{
					// If the shader has changed since it was last compiled, skip it.
					Ar.Seek(SkipOffset);
					NumLegacyShaders++;
					OutdatedShaderTypes.AddUniqueItem(FString(ShaderType->GetName()));
				}
				else if(Ar.Ver() < ShaderType->GetMinPackageVersion() || Ar.LicenseeVer() < ShaderType->GetMinLicenseePackageVersion())
				{
					// If the shader type's serialization is compatible with the version the shader was saved in, skip it.
					Ar.Seek(SkipOffset);
					NumLegacyShaders++;
				}
				else
				{
					// Create a new instance of the shader type.
					Shader = ShaderType->ConstructForDeserialization();

					// Deserialize the shader into the new instance.
					Shader->Serialize(Ar);

					//If this check fails and VER_LATEST_ENGINE was changed locally and the local shader cache was saved with it, then the local shader cache should be deleted.
					//Otherwise, if the shader's serialization was changed (usually shader parameters or vertex factory parameters) 
					//and the shader file was not changed, so the CRC check passed, then the shader's MinPackageVersion needs to be incremented.
					checkf(Ar.Tell() == SkipOffset, 
						TEXT("Deserialized the wrong amount for shader %s!  \n")
						TEXT("  Shader cache file [%s] \n.")
						TEXT("	This can happen if VER_LATEST_ENGINE was changed locally, so the local shader cache needs to be deleted, \n")
						TEXT("	or if the shader's serialization was changed without bumping its appropriate version.  Expected archive position %i, got position %i"), 
						*GetPathName(),
						ShaderType->GetName(), 
						SkipOffset, 
						Ar.Tell()
						);
				}
			}
		}

		TArray<FString> OutdatedVFTypes;
		INT NumMaterialShaderMaps = 0;	
		Ar << NumMaterialShaderMaps;

		// Load the material shader indices in the cache.
		for( INT MaterialIndex=0; MaterialIndex<NumMaterialShaderMaps; MaterialIndex++ )
		{
			// Deserialize the material ID.
			FGuid OldId(0,0,0,0);
			if (Ar.Ver() < VER_STATIC_MATERIAL_PARAMETERS)
			{
				Ar << OldId;
			}

			FStaticParameterSet StaticParameters(OldId);
			if (Ar.Ver() >= VER_STATIC_MATERIAL_PARAMETERS)
			{
				StaticParameters.Serialize(Ar);
			}

			// Deserialize the offset of the next material.
			INT SkipOffset = 0;
			Ar << SkipOffset;

			FMaterialShaderMap* MaterialShaderIndex = FMaterialShaderMap::FindId(StaticParameters,(EShaderPlatform)Platform);
			if(MaterialShaderIndex)
			{
				// If a shader with the same type and ID is already resident, skip this shader.
				Ar.Seek(SkipOffset);
			}
			else
			{
				// Deserialize the material shader map.
				MaterialShaderIndex = new FMaterialShaderMap();
				MaterialShaderIndex->Serialize(Ar);

				if (GAutoReloadChangedShaders)
				{
					//go through each vertex factory type
					for(TLinkedList<FVertexFactoryType*>::TIterator It(FVertexFactoryType::GetTypeList()); It; It.Next())
					{
						const DWORD* VFTypeCRC = VertexFactoryTypeCRCMap.Find(*It);
						DWORD CurrentCRC = It->GetSourceCRC();

						//if the CRC that the type was compiled with doesn't match the current CRC, flush the type
						if (VFTypeCRC != NULL && CurrentCRC != *VFTypeCRC)
						{
							MaterialShaderIndex->FlushShadersByVertexFactoryType(*It);
							OutdatedVFTypes.AddUniqueItem(FString(It->GetName()));
						}
					}
				}
			}

			// Add a reference to the shader from the cache. This ensures that the shader isn't deleted between 
			// the cache being deserialized and PostLoad being called on materials in the same package.
			MaterialShaderMap.Set(StaticParameters,MaterialShaderIndex);	
		}

		// Load the global shader map, but ONLY in the local shader cache
		TShaderMap<FGlobalShaderType>* NewGlobalShaderMap = new TShaderMap<FGlobalShaderType>();
		NewGlobalShaderMap->Serialize(Ar);

		// Only load once in case of seekfree loading on consoles...
		if( (GUseSeekFreeLoading && CONSOLE && GGlobalShaderMap[Platform] == NULL) 
		// ... and only serialize global shaders from local shader cache if we're using normal loading or seekfree loading on PC
		|| ((!GUseSeekFreeLoading || !CONSOLE) && GSerializingLocalShaderCache)  )
		{
			delete GGlobalShaderMap[Platform];
			GGlobalShaderMap[Platform] = NewGlobalShaderMap;
			if( GUseSeekFreeLoading && CONSOLE)
			{
				// Initialize global shaders now. They are only loaded once on consoles.
				GGlobalShaderMap[Platform]->BeginInit();
			}
		}
		else
		{
			delete NewGlobalShaderMap;
		}

		//Log which types were skipped/flushed because their source files have changed
		if (GAutoReloadChangedShaders && GSerializingLocalShaderCache)
		{
			INT NumOutdatedShaderTypes = OutdatedShaderTypes.Num();
			if (NumOutdatedShaderTypes > 0)
			{
				debugf(TEXT("Skipped %i outdated FShaderTypes:"), NumOutdatedShaderTypes);
				for (INT TypeIndex = 0; TypeIndex < NumOutdatedShaderTypes; TypeIndex++)
				{	
					debugf(TEXT("	%s"), *OutdatedShaderTypes(TypeIndex));
				}
			}
			
			INT NumOutdatedVFTypes = OutdatedVFTypes.Num();
			if (NumOutdatedVFTypes > 0)
			{
				debugf(TEXT("Skipped %i outdated FVertexFactoryTypes:"), NumOutdatedVFTypes);
				for (INT TypeIndex = 0; TypeIndex < NumOutdatedVFTypes; TypeIndex++)
				{	
					debugf(TEXT("	%s"), *OutdatedVFTypes(TypeIndex));
				}
			}
		}

		// Log some cache stats.
		debugf(
			TEXT("Loaded shader cache %s: %u shaders(%u legacy, %u redundant), %u materials"),
			*GetPathName(),
			NumShaders,
			NumLegacyShaders,
			NumRedundantShaders,
			NumMaterialShaderMaps
			);
	}

	if( Ar.IsCountingMemory() )
	{
		MaterialShaderMap.CountBytes( Ar );
		for( TDynamicMap<FStaticParameterSet,TRefCountPtr<class FMaterialShaderMap> >::TIterator It(MaterialShaderMap); It; ++It )
		{
			It.Value()->Serialize( Ar );
		}
	}
}

/**
 * Identifies temporary shader files that should never be CRC'ed since their contents will change over time.
 * Files excluded here need to be handled manually elsewhere or changes to them will not be detected by shaders that include them.
 */
UBOOL IsTempShaderFile(const FString &Filename)
{
	//Material.usf is filled out by the material compiler from MaterialTemplate.usf and the CRC check is handled by FMeshMaterialShaderType::GetSourceCRC()
	if (Filename == TEXT("Material.usf")
		|| Filename == TEXT("VertexFactory.usf"))
	{
		return TRUE;
	}
	return FALSE;
}

/**
 * Populates IncludeFilenames with the include filenames from FileContents
 */
void GetShaderIncludes(const FString& FileContents, TArray<FString> &IncludeFilenames)
{
	//avoid an infinite loop with a 0 length string
	check(FileContents.Len() > 0);

	//find the first include directive
	TCHAR* IncludeBegin = appStrstr(*FileContents, TEXT("#include "));

	UINT SearchCount = 0;
	const UINT MaxSearchCount = 20;
	//keep searching for includes as long as we are finding new ones and haven't exceeded the fixed limit
	while (IncludeBegin != NULL && SearchCount < MaxSearchCount)
	{
		//find the first double quotation after the include directive
		TCHAR* IncludeFilenameBegin = appStrstr(IncludeBegin, TEXT("\""));
		//find the trailing double quotation
		TCHAR* IncludeFilenameEnd = appStrstr(IncludeFilenameBegin + 1, TEXT("\""));
		//construct a string between the double quotations
		FString ExtractedIncludeFilename((INT)(IncludeFilenameEnd - IncludeFilenameBegin - 1), IncludeFilenameBegin + 1);
		//don't add temporary shader files
		if (!IsTempShaderFile(ExtractedIncludeFilename))
		{
			new (IncludeFilenames) FString(ExtractedIncludeFilename);
		}
		
		//find the next include directive
		IncludeBegin = appStrstr(IncludeFilenameEnd + 1, TEXT("#include "));
		SearchCount++;
	}
}

/**
* Recursively CRC's a shader file and its includes.
*/
DWORD RecursivelyGetShaderFileCRC(const TCHAR* Filename)
{
	DWORD* CachedCRC = GShaderCRCCache.Find(Filename);

	//if a CRC for this filename has been cached, use that
	if (CachedCRC)
	{
		return *CachedCRC;
	}

	//load the source file and CRC it
	FString FileContents = LoadShaderSourceFile(Filename);
	DWORD ComputedCRC = appStrCrcCaps(*FileContents);
	TArray<FString> IncludeFilenames;

	//get the list of includes this file contains
	GetShaderIncludes(FileContents, IncludeFilenames);

	//recursively CRC each include and xor it with this file's CRC
	for (INT IncludeIndex = 0; IncludeIndex < IncludeFilenames.Num(); IncludeIndex++)
	{
		ComputedCRC ^= GetShaderFileCRC(*IncludeFilenames(IncludeIndex));
	}

	//update the CRC cache
	GShaderCRCCache.Set(*FString(Filename), ComputedCRC);
	return ComputedCRC;
}

/**
 * Calculates a CRC for the given filename if it does not already exist in the CRC cache.
 * This handles nested includes through recursion.
 */
DWORD GetShaderFileCRC(const TCHAR* Filename)
{
	DWORD FileCRC;
	STAT(DOUBLE CRCTime = 0);
	{
		SCOPE_SECONDS_COUNTER(CRCTime);
		FileCRC = RecursivelyGetShaderFileCRC(Filename);
	}
	INC_FLOAT_STAT_BY(STAT_ShaderCompiling_CRCingShaderFiles,(FLOAT)CRCTime);
	return FileCRC;
}

/**
 * Flushes the shader file and CRC cache.
 */
void FlushShaderFileCache()
{
	GShaderCRCCache.Empty();
	GShaderFileCache.Empty();
}

/**
 * Loads the shader file with the given name.
 * @return The contents of the shader file.
 */
FString LoadShaderSourceFile(const TCHAR* Filename)
{
	FString	FileContents;
	STAT(DOUBLE ShaderFileLoadingTime = 0);
	{
		SCOPE_SECONDS_COUNTER(ShaderFileLoadingTime);

		// Load the specified file from the System/Shaders directory.
		FFilename ShaderFilename = FString(appBaseDir()) * appShaderDir() * Filename;

		if (ShaderFilename.GetExtension() != TEXT("usf"))
		{
			ShaderFilename += TEXT(".usf");
		}

		FString* CachedFile = GShaderFileCache.Find(ShaderFilename);

		//if this file has already been loaded and cached, use that
		if (CachedFile)
		{
			FileContents = *CachedFile;
		}
		else
		{
			if( !appLoadFileToString(FileContents, *ShaderFilename))
			{
				appErrorf(TEXT("Couldn't load shader file \'%s\'"),Filename);
			}

			//update the shader file cache
			GShaderFileCache.Set(ShaderFilename, *FileContents);
		}
	}

	INC_FLOAT_STAT_BY(STAT_ShaderCompiling_LoadingShaderFiles,(FLOAT)ShaderFileLoadingTime);
	return FileContents;
}

/**
 * Dumps shader stats to the log.
 */
void DumpShaderStats()
{
	// Iterate over all shader types and log stats.
	INT TotalShaderCount	= 0;
	INT TotalTypeCount		= 0;
	debugf(TEXT("Shader dump:"));
	for( TLinkedList<FShaderType*>::TIterator It(FShaderType::GetTypeList()); It; It.Next() )
	{
		const FShaderType* Type = *It;
		debugf(TEXT("%7i shaders of type '%s'"), Type->GetNumShaders(), Type->GetName());
		TotalShaderCount += Type->GetNumShaders();
		TotalTypeCount++;
	}
	debugf(TEXT("%i shaders spread over %i types"),TotalShaderCount,TotalTypeCount);
}

FString GetShaderCacheFilename(const TCHAR* Prefix, EShaderPlatform Platform)
{
	// find the first entry in the Paths array that contains the GameDir (so we don't find an ..\Engine path)
	for (INT PathIndex = 0; PathIndex < GSys->Paths.Num(); PathIndex++)
	{
		// does the path contain GameDir? (..\ExampleGame\)
		if (GSys->Paths(PathIndex).InStr(appGameDir(), FALSE, TRUE) != -1)
		{
			// if so, use it
			return FString::Printf(TEXT("%s%s%sShaderCache-%s.upk"), 
				*GSys->Paths(PathIndex),
				PATH_SEPARATOR,
				Prefix,
				ShaderPlatformToText(Platform));
		}
	}

	checkf(FALSE, TEXT("When making the ShaderCache filename, failed to find a GSys->Path containing %s"), *appGameDir());
	return TEXT("");
}

FString GetLocalShaderCacheFilename( EShaderPlatform Platform )
{
	return GetShaderCacheFilename(TEXT("Local"), Platform);
}

FString GetReferenceShaderCacheFilename( EShaderPlatform Platform )
{
	return GetShaderCacheFilename(TEXT("Ref"), Platform);
}

/**
 * Loads the reference and local shader caches for the passed in platform
 *
 * @param	Platform	Platform to load shader caches for.
 */
static void LoadShaderCaches( EShaderPlatform Platform )
{
	//initialize GRHIShaderPlatform based on the current system's capabilities
	RHIInitializeShaderPlatform();

	//get the option to skip shaders whose source files have changed since they were compiled
	GConfig->GetBool( TEXT("DevOptions.Shaders"), TEXT("AutoReloadChangedShaders"), GAutoReloadChangedShaders, GEngineIni );

	// If we're building the reference shader cache, only load the reference shader cache.
	const UBOOL bLoadOnlyReferenceShaderCache = ParseParam(appCmdLine(), TEXT("refcache"));

	for( INT TypeIndex=0; TypeIndex<SC_NumShaderCacheTypes; TypeIndex++ )
	{
		UShaderCache*& ShaderCacheRef = GShaderCaches[TypeIndex][Platform];
		//check for reentry
		check(ShaderCacheRef == NULL);

		// If desired, skip loading the local shader cache.
		if(TypeIndex == SC_Local && bLoadOnlyReferenceShaderCache)
		{
			continue;
		}

		// mark that we are serializing the local shader cache, so we can load the global shaders
		GSerializingLocalShaderCache = (TypeIndex == SC_Local);
		
		// by default, we have no shadercache
		ShaderCacheRef = NULL;

		// only look for the shader cache object if the package exists (avoids a throw inside the code)
		FFilename PackageName = (TypeIndex == SC_Local ? GetLocalShaderCacheFilename(Platform) : GetReferenceShaderCacheFilename(Platform));
		FString Filename;
		if (GPackageFileCache->FindPackageFile(*PackageName, NULL, Filename))
		{
			// if another instance is writing the shader cache while we are reading, then opening will fail, and 
			// that's not good, so retry until we can open it
#if !CONSOLE
			// this "lock" will make sure that another process can't be writing to the package before we actually 
			// read from it with LoadPackage, etc below
			FArchive* ReaderLock = NULL;
			// try to open the shader cache for 
			DOUBLE StartTime = appSeconds();
			const DOUBLE ShaderRetryDelaySeconds = 15.0;

			// try until we can read the file, or until ShaderRetryDelaySeconds has passed
			while (ReaderLock == NULL && appSeconds() - StartTime < ShaderRetryDelaySeconds)
			{
				ReaderLock = GFileManager->CreateFileReader(*Filename);
				if(!ReaderLock)
				{
					// delay a bit
					appSleep(1.0f);
				}
			}
#endif

			UBOOL bSkipLoading = FALSE;
			if( TypeIndex == SC_Local && (CONSOLE || !GUseSeekFreeLoading))
			{
				// This function is being called during script compilation, which is why we need to use LOAD_FindIfFail.
				UObject::BeginLoad();
				ULinkerLoad* Linker = UObject::GetPackageLinker( NULL, *Filename, LOAD_NoWarn | LOAD_FindIfFail, NULL, NULL );
				UObject::EndLoad();
				// Skip loading the local shader cache if it was built with an old version.
				const INT MaxVersionDifference = 10;
				// Skip loading the local shader cache if the version # is less than a given threshold
				const INT MinShaderCacheVersion = VER_REMOVE_BINORMAL_TANGENT_VECTOR;
				if( Linker && 
					((GEngineVersion - Linker->Summary.EngineVersion) > MaxVersionDifference ||
					Linker->Summary.EngineVersion < MinShaderCacheVersion) )
				{
					bSkipLoading = TRUE;
				}
			}

			// Skip loading the shader cache if wanted.
			if( !bSkipLoading )
			{
#if !CONSOLE
				// if we are seekfree loading and we are in here, we are a PC game which doesn't cook the shadercaches into each package, 
				// but we can't call LoadPackage normally because we are going to be inside loading Engine.u at this point, so the 
				// ResetLoaders at the end of the LoadPackage (in the seekfree case) will fail because the objects in the shader cache 
				// package won't get EndLoad called on them because we are already inside an EndLoad (for Engine.u).
				// Instead, we use LoadObject, and we will ResetLoaders on the package later in the startup process. We need to disable 
				// seekfree temporarily because LoadObject doesn't work in the seekfree case if the object isn't already loaded.
				if (GUseSeekFreeLoading)
				{
					// unset GUseSeekFreeLoading so that the LoadObject will work
					GUseSeekFreeLoading = FALSE;
					ShaderCacheRef = LoadObject<UShaderCache>(NULL, *(PackageName.GetBaseFilename() + TEXT(".CacheObject")), NULL, LOAD_NoWarn, NULL );
					GUseSeekFreeLoading = TRUE;
				}
				else
#endif
				{
					// This function is being called during script compilation, which is why we need to use LOAD_FindIfFail.
					UPackage* ShaderCachePackage = UObject::LoadPackage( NULL, *Filename, LOAD_NoWarn | LOAD_FindIfFail );
					if( ShaderCachePackage )
					{
						ShaderCacheRef = FindObject<UShaderCache>( ShaderCachePackage, TEXT("CacheObject") );
					}
				}
			}
#if !CONSOLE
			delete ReaderLock;
#endif
		}

		if(!ShaderCacheRef)
		{
			// if we didn't find the local shader cache, create it. if we don't find the refshadercache, that's okay, just leave it be
			if (TypeIndex == SC_Local || bLoadOnlyReferenceShaderCache)
			{
				// If the local shader cache couldn't be loaded, create an empty cache.
				FString LocalShaderCacheName = FString(TEXT("LocalShaderCache-")) + ShaderPlatformToText(Platform);
				ShaderCacheRef = new(UObject::CreatePackage(NULL,*LocalShaderCacheName),TEXT("CacheObject")) UShaderCache(Platform);
				ShaderCacheRef->MarkPackageDirty(FALSE);
			}
		}
		// if we found it, make sure it's loaded
		else
		{
			// if this function was inside a BeginLoad()/EndLoad(), then the LoadObject above didn't actually serialize it, this will
			ShaderCacheRef->GetLinker()->Preload(ShaderCacheRef);
		}

		if (ShaderCacheRef)
		{
			// make sure it's not GC'd
			ShaderCacheRef->AddToRoot();
		}

		GSerializingLocalShaderCache = FALSE;
	}

	// If we only loaded the reference shader cache, use it in place of the local shader cache.
	if(bLoadOnlyReferenceShaderCache)
	{
		GShaderCaches[SC_Local][Platform] = GShaderCaches[SC_Reference][Platform];
	}

	// Ensure that the global shader map contains all global shader types.
	VerifyGlobalShaders();
}

/**
 * Makes sure all global shaders are loaded and/or compiled for the passed in platform.
 *
 * @param	Platform	Platform to verify global shaders for
 */
void VerifyGlobalShaders(EShaderPlatform Platform)
{
	// Ensure local shader cache for this platform has been loaded if present.
	GetLocalShaderCache(Platform);

	// Ensure that the global shader map contains all global shader types.
	TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(Platform);
	for(TLinkedList<FShaderType*>::TIterator ShaderTypeIt(FShaderType::GetTypeList());ShaderTypeIt;ShaderTypeIt.Next())
	{
		FGlobalShaderType* GlobalShaderType = ShaderTypeIt->GetGlobalShaderType();
		if(GlobalShaderType && GlobalShaderType->ShouldCache(Platform))
		{
			if(!GlobalShaderMap->HasShader(GlobalShaderType))
			{
				// Compile this global shader type.
				TArray<FString> ShaderErrors;
				FShader* Shader = GlobalShaderType->CompileShader(Platform,ShaderErrors);
				if(Shader)
				{
					// Add the new global shader instance to the global shader map.
					// This will cause FShader::AddRef to be called, which will cause BeginInitResource(Shader) to be called.
					GlobalShaderMap->AddShader(GlobalShaderType,Shader);

					// make sure the shader cache is saved out
					GetLocalShaderCache(Platform)->MarkDirty();
				}
				else
				{
					appErrorf(TEXT("Failed to compile global shader %s"), GlobalShaderType->GetName());
				}
			}
		}
	}
	GGlobalShaderMap[Platform]->BeginInit();
}

/**
 * Returns the local shader cache for the passed in platform
 *
 * @param	Platform	Platform to return local shader cache for.
 * @return	The local shader cache for the passed in platform
 */
UShaderCache* GetLocalShaderCache( EShaderPlatform Platform )
{
	if( !GShaderCaches[SC_Local][Platform] )
	{
		LoadShaderCaches( Platform );
	}

	return GShaderCaches[SC_Local][Platform];
}

/**
 * Saves the local shader cache for the passed in platform.
 *
 * @param	Platform	Platform to save shader cache for.
 * @param	OverrideCacheFilename If non-NULL, then the shader cache will be saved to the given path
 */
void SaveLocalShaderCache(EShaderPlatform Platform, const TCHAR* OverrideCacheFilename)
{
#if !SHIPPING_PC_GAME
	// Only save the shader cache for the first instance running.
	if( GIsFirstInstance )
	{
#endif

		UShaderCache* ShaderCache = GShaderCaches[SC_Local][Platform];
		if( ShaderCache && ShaderCache->IsDirty())
		{
			// Reset the LinkerLoads for all shader caches, since we may be saving the local shader cache over the refshadercache file.
			for(INT TypeIndex = 0;TypeIndex < SC_NumShaderCacheTypes;TypeIndex++)
			{
				if(GShaderCaches[TypeIndex][Platform])
				{
					UObject::ResetLoaders(GShaderCaches[TypeIndex][Platform]);
				}
			}

			UPackage* ShaderCachePackage = ShaderCache->GetOutermost();
			// The shader cache isn't network serializable
			ShaderCachePackage->PackageFlags |= PKG_ServerSideOnly;

			// allow this operation to fail unless we are overriding the cache filename (which implies a special case that shouldn't be skipped)
			if (OverrideCacheFilename)
			{
				UObject::SavePackage(ShaderCachePackage, ShaderCache, 0, OverrideCacheFilename);
			}
			else
			{
				UObject::SavePackage(ShaderCachePackage, ShaderCache, 0, *GetLocalShaderCacheFilename(Platform), GWarn, NULL, FALSE, TRUE, SAVE_NoError);
			}

			// mark it as clean, as its been saved!
			ShaderCache->MarkClean();
		}

#if !SHIPPING_PC_GAME
	}
	else
	{
		// Only warn once.
		static UBOOL bAlreadyWarned = FALSE;
		if( !bAlreadyWarned )
		{
			bAlreadyWarned = TRUE;
			debugf( NAME_Warning, TEXT("Skipping saving the shader cache as another instance of the game is running.") );
		}
	}
#endif
}

/**
 * Saves all local shader caches.
 */
extern void SaveLocalShaderCaches()
{
	for( INT PlatformIndex=0; PlatformIndex<SP_NumPlatforms; PlatformIndex++ )
	{
		SaveLocalShaderCache( (EShaderPlatform)PlatformIndex );
	}
}

TShaderMap<FGlobalShaderType>* GetGlobalShaderMap(EShaderPlatform Platform)
{
	if(!GGlobalShaderMap[Platform])
	{
		GGlobalShaderMap[Platform] = new TShaderMap<FGlobalShaderType>();
	}
	return GGlobalShaderMap[Platform];
}

/**
 * Forces a recompile of the global shaders.
 */
void RecompileGlobalShaders()
{
	if( !GUseSeekFreeLoading )
	{
		// Flush pending accesses to the existing global shaders.
		FlushRenderingCommands();

		GetGlobalShaderMap(GRHIShaderPlatform)->Empty();

		TArray<FGlobalBoundShaderStateRHIRef*> GlobalBoundShaderStates;
		FGlobalBoundShaderStateRHIRef::GetTypeToBoundStateMap().GenerateValueArray(GlobalBoundShaderStates);
		//invalidate global bound shader states so they will be created with the new shaders the next time they are set (in SetGlobalBoundShaderState)
		for (INT TypeIndex = 0; TypeIndex < GlobalBoundShaderStates.Num(); TypeIndex++)
		{
			FGlobalBoundShaderStateRHIRef* CurrentGlobalBoundShaderState = GlobalBoundShaderStates(TypeIndex);
			if (CurrentGlobalBoundShaderState)
			{
				CurrentGlobalBoundShaderState->Invalidate();
			}
		}

		VerifyGlobalShaders(GRHIShaderPlatform);
	}
}

/**
 * Recompiles the specified global shader types, and flushes their bound shader states.
 */
void RecompileGlobalShaders(const TArray<FShaderType*>& OutdatedShaderTypes)
{
	if( !GUseSeekFreeLoading )
	{
		// Flush pending accesses to the existing global shaders.
		FlushRenderingCommands();

		TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(GRHIShaderPlatform);

		for (INT TypeIndex = 0; TypeIndex < OutdatedShaderTypes.Num(); TypeIndex++)
		{
			FGlobalShaderType* CurrentGlobalShaderType = OutdatedShaderTypes(TypeIndex)->GetGlobalShaderType();
			if (CurrentGlobalShaderType)
			{
				debugf(TEXT("Flushing Global Shader %s"), CurrentGlobalShaderType->GetName());
				GlobalShaderMap->RemoveShaderType(CurrentGlobalShaderType);
				FGlobalBoundShaderStateRHIRef* CurrentBoundShaderState = FGlobalBoundShaderStateRHIRef::GetTypeToBoundStateMap().FindRef(CurrentGlobalShaderType);
				if (CurrentBoundShaderState)
				{
					debugf(TEXT("Invalidating Bound Shader State"));
					CurrentBoundShaderState->Invalidate();
				}
			}
		}

		VerifyGlobalShaders(GRHIShaderPlatform);
	}
}

/**
 * Forces a recompile of only the specified group of global shaders. 
 * Also invalidates global bound shader states so that the new shaders will be used.
 */
void RecompileGlobalShaderGroup(EShaderRecompileGroup FlushGroup)
{
	if( !GUseSeekFreeLoading )
	{
		// Flush pending accesses to the existing global shaders.
		FlushRenderingCommands();

		TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(GRHIShaderPlatform);
		TMap<FGuid,FShader*> GlobalShaderList;
		GlobalShaderMap->GetShaderList(GlobalShaderList);
		for(TMap<FGuid,FShader*>::TIterator ShaderIt(GlobalShaderList);ShaderIt;++ShaderIt)
		{
			FShader* CurrentGlobalShader = ShaderIt.Value();
			if (CurrentGlobalShader->GetRecompileGroup() == FlushGroup)
			{
				FShaderType* CurrentShaderType = CurrentGlobalShader->GetType();
				FGlobalShaderType* CurrentGlobalShaderType = CurrentShaderType->GetGlobalShaderType();
				check(CurrentGlobalShaderType);
				debugf(TEXT("Flushing Global Shader %s"), CurrentGlobalShaderType->GetName());
				GlobalShaderMap->RemoveShaderType(CurrentGlobalShaderType);
			}
		}

		debugf(TEXT("Flushing Global Bound Shader States..."));

		TArray<FGlobalBoundShaderStateRHIRef*> GlobalBoundShaderStates;
		FGlobalBoundShaderStateRHIRef::GetTypeToBoundStateMap().GenerateValueArray(GlobalBoundShaderStates);
		//invalidate global bound shader states so they will be created with the new shaders the next time they are set (in SetGlobalBoundShaderState)
		for (INT TypeIndex = 0; TypeIndex < GlobalBoundShaderStates.Num(); TypeIndex++)
		{
			FGlobalBoundShaderStateRHIRef* CurrentGlobalBoundShaderState = GlobalBoundShaderStates(TypeIndex);
			if (CurrentGlobalBoundShaderState && CurrentGlobalBoundShaderState->ShaderRecompileGroup == FlushGroup)
			{
				CurrentGlobalBoundShaderState->Invalidate();
			}
		}

		VerifyGlobalShaders(GRHIShaderPlatform);
	}
}

#if PS3
/**
 * Set a Linear Color parameter (used to swaps Red/Blue too)
 */
// @GEMINI_TODO: Make sure this is never needed, then remove
template<>
void SetPixelShaderValue(
	FCommandContextRHI* Context,
	FPixelShaderRHIParamRef PixelShader,
	const FShaderParameter& Parameter,
	const FLinearColor& Value,
	UINT ElementIndex
	)
{
	if(Parameter.IsBound())
	{
		check((ElementIndex + 1) <= Parameter.GetNumRegisters());
		FLinearColor Temp(Value.R, Value.G, Value.B, Value.A);
		RHISetPixelShaderParameter(
			Context,
			PixelShader,
			Parameter.GetBaseRegisterIndex() + ElementIndex,
			1,
			(FLOAT*)&Temp
			);
	}
}
#endif

//
FShaderType* FindShaderTypeByName(const TCHAR* ShaderTypeName)
{
	for(TLinkedList<FShaderType*>::TIterator ShaderTypeIt(FShaderType::GetTypeList());ShaderTypeIt;ShaderTypeIt.Next())
	{
		if(!appStricmp(ShaderTypeIt->GetName(),ShaderTypeName))
		{
			return *ShaderTypeIt;
		}
	}
	return NULL;
}

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
void SetGlobalBoundShaderState(
   FCommandContextRHI* Context,
   FGlobalBoundShaderStateRHIRef &GlobalBoundShaderState,
   FVertexDeclarationRHIParamRef VertexDeclaration,
   FShader* VertexShader,
   FShader* PixelShader,
   UINT Stride)
{
	if( !IsValidRef(GlobalBoundShaderState.BoundShaderState))
	{
		DWORD Strides[MaxVertexElementCount];
		appMemzero(Strides, sizeof(Strides));
		Strides[0] = Stride;

		GlobalBoundShaderState.Init(
			RHICreateBoundShaderState(VertexDeclaration, Strides, VertexShader->GetVertexShader(), PixelShader->GetPixelShader()),
			PixelShader->GetRecompileGroup(),
			VertexShader->GetType()->GetGlobalShaderType(),
			PixelShader->GetType()->GetGlobalShaderType());
	}
	RHISetBoundShaderState(Context, GlobalBoundShaderState.BoundShaderState);
}
