/*=============================================================================
UnStaticMeshRender.cpp: Static mesh rendering code.
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineMeshClasses.h"
#include "EngineProcBuildingClasses.h"
#include "LocalVertexFactoryShaderParms.h"
#include "UnStaticMeshLight.h"


IMPLEMENT_CLASS(UInstancedStaticMeshComponent);

enum EInstancingStats
{
	STAT_LoadedInstances = STAT_InstancingFirstStat,
	STAT_AttachedInstances,
};

DECLARE_STATS_GROUP(TEXT("Instancing"),STATGROUP_Instancing);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Loaded instances"),STAT_LoadedInstances,STATGROUP_Instancing);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Attached instances"),STAT_AttachedInstances,STATGROUP_Instancing);

TSet<AActor*> UInstancedStaticMeshComponent::ActorsWithInstancedComponents;
static const INT InstancedStaticMeshMaxTexCoord = MAX_TEXCOORDS;

/*-----------------------------------------------------------------------------
	FStaticMeshInstanceData
-----------------------------------------------------------------------------*/

/** The implementation of the static mesh instance data storage type. */
class FStaticMeshInstanceData :
	public FStaticMeshVertexDataInterface,
	public TResourceArray<FVector4,VERTEXBUFFER_ALIGNMENT>
{
public:

	typedef TResourceArray<FVector4,VERTEXBUFFER_ALIGNMENT> ArrayType;

	/**
	 * Constructor
	 * @param InNeedsCPUAccess - TRUE if resource array data should be CPU accessible
	 */
	FStaticMeshInstanceData(UBOOL InNeedsCPUAccess=FALSE)
		:	TResourceArray<FVector4,VERTEXBUFFER_ALIGNMENT>(InNeedsCPUAccess)
	{
	}

	/**
	 * Resizes the vertex data buffer, discarding any data which no longer fits.
	 * @param NumVertices - The number of vertices to allocate the buffer for.
	 */
	virtual void ResizeBuffer(UINT NumInstances)
	{
		checkf(0, TEXT("ArrayType::Add is not supported on all platforms"));
	}

	virtual UINT GetStride() const
	{
		const UINT VectorsPerInstance = 7;
		return sizeof(FVector4) * VectorsPerInstance;
	}
	virtual BYTE* GetDataPointer()
	{
		return (BYTE*)&(*this)(0);
	}
	virtual FResourceArrayInterface* GetResourceArray()
	{
		return this;
	}
	virtual void Serialize(FArchive& Ar)
	{
		TResourceArray<FVector4,VERTEXBUFFER_ALIGNMENT>::BulkSerialize(Ar);
	}

	void Set(const TArray<FVector4>& RawData)
	{
		*((ArrayType*)this) = RawData;
	}
};


/*-----------------------------------------------------------------------------
	FStaticMeshInstanceBuffer
-----------------------------------------------------------------------------*/


/** A vertex buffer of positions. */
class FStaticMeshInstanceBuffer : public FVertexBuffer
{
public:

	/** Default constructor. */
	FStaticMeshInstanceBuffer();

	/** Destructor. */
	~FStaticMeshInstanceBuffer();

	/** Delete existing resources */
	void CleanUp();

	/**
	 * Initializes the buffer with the given matrices.
	 * @param OwnerLocalToWorld The transform for the owning component
	 * @param InData The per-instance data from the component
	 * @param InInstancingRandomSeed Random number seed for this instanced static mesh buffer
	 */
	void Init(const FMatrix& OwnerLocalToWorld, const TArray<FInstancedStaticMeshInstanceData>& InData, INT InInstancingRandomSeed);

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar, FStaticMeshInstanceBuffer& VertexBuffer);

	/**
	 * Specialized assignment operator, only used when importing LOD's. 
	 */
	void operator=(const FStaticMeshInstanceBuffer &Other);

	// Other accessors.
	FORCEINLINE UINT GetStride() const
	{
		return Stride;
	}
	FORCEINLINE UINT GetNumInstances() const
	{
		return NumInstances;
	}

	// FRenderResource interface.
	virtual void InitRHI();
	virtual FString GetFriendlyName() const { return TEXT("Static-mesh instances"); }

private:

	/** The vertex data storage type */
	FStaticMeshInstanceData* InstanceData;

	/** The cached vertex stride. */
	UINT Stride;

	/** The cached number of instances. */
	UINT NumInstances;

	/** Allocates the vertex data storage type. */
	void AllocateData();
};


FStaticMeshInstanceBuffer::FStaticMeshInstanceBuffer():
	InstanceData(NULL)
{
}

FStaticMeshInstanceBuffer::~FStaticMeshInstanceBuffer()
{
	CleanUp();
}

/** Delete existing resources */
void FStaticMeshInstanceBuffer::CleanUp()
{
	if (InstanceData)
	{
		delete InstanceData;
		InstanceData = NULL;
	}
}

/**
 * Initializes the buffer with the given matrices.
 * @param OwnerLocalToWorld The transform for the owning component
 * @param InData The per-instance data from the component
 * @param InInstancingRandomSeed Random number seed for this instanced static mesh buffer
 */
void FStaticMeshInstanceBuffer::Init(const FMatrix& OwnerLocalToWorld, const TArray<FInstancedStaticMeshInstanceData>& InData, INT InInstancingRandomSeed )
{
	NumInstances = InData.Num();

	// Allocate the vertex data storage type.
	AllocateData();

	// PS3 only allows assignment currently, so we make a TArray of the right type, then assign it
	TArray<FVector4> RawData;
	check( GetStride() % sizeof(FVector4) == 0 );
	RawData.Add(NumInstances * GetStride() / sizeof(FVector));

	// @todo: Make LD-customizable per component?
	const FLOAT RandomInstanceIDBase = 0.0f;
	const FLOAT RandomInstanceIDRange = 1.0f;

	// Setup our random number generator such that random values are generated consistently for any
	// given instance index between reattaches
	FRandomStream RandomStream( InInstancingRandomSeed );

	UINT CurVecIndex = 0;
	for (UINT InstanceIndex = 0; InstanceIndex < NumInstances; InstanceIndex++)
	{
		const FInstancedStaticMeshInstanceData& Instance = InData(InstanceIndex);

		// X, Y	: Shadow map UV bias
		//	  Z : Per-instance random ID
		//	  W : Currently unused! (0.0)
		const FLOAT RandomInstanceID = RandomInstanceIDBase + RandomStream.GetFraction() * RandomInstanceIDRange;
		RawData(CurVecIndex++) = FVector4( Instance.ShadowmapUVBias.X, Instance.ShadowmapUVBias.Y, RandomInstanceID, 0.0f );

		// Grab the instance -> local matrix.  Every mesh instance has it's own transformation into
		// the actor's coordinate space.
		const FMatrix& InstanceToLocal = Instance.Transform;

		// Create an instance -> world transform by combining the instance -> local transform with the
		// local -> world transform
		const FMatrix InstanceToWorld = InstanceToLocal * OwnerLocalToWorld;

		// Instance to world transform matrix
		{
			const FMatrix Transpose = InstanceToWorld.Transpose();
			RawData(CurVecIndex++) = FVector4(Transpose.M[0][0], Transpose.M[0][1], Transpose.M[0][2], Transpose.M[0][3]);
			RawData(CurVecIndex++) = FVector4(Transpose.M[1][0], Transpose.M[1][1], Transpose.M[1][2], Transpose.M[1][3]);
			RawData(CurVecIndex++) = FVector4(Transpose.M[2][0], Transpose.M[2][1], Transpose.M[2][2], Transpose.M[2][3]);
		}

		// World to instance rotation matrix (3x3)
		{
 			// Invert the instance -> world matrix
			const FMatrix WorldToInstance = InstanceToWorld.Inverse();

			// hide the offset (bias) of the lightmap in the matrix's w
			const FMatrix Transpose = WorldToInstance.Transpose();
			RawData(CurVecIndex++) = FVector4(Transpose.M[0][0], Transpose.M[0][1], Transpose.M[0][2], Instance.LightmapUVBias.X);
			RawData(CurVecIndex++) = FVector4(Transpose.M[1][0], Transpose.M[1][1], Transpose.M[1][2], Instance.LightmapUVBias.Y);
			RawData(CurVecIndex++) = FVector4(Transpose.M[2][0], Transpose.M[2][1], Transpose.M[2][2], 0.0f);
		}
	}

	// Allocate the vertex data buffer.
	InstanceData->Set(RawData);
}

/** Serializer. */
FArchive& operator<<(FArchive& Ar,FStaticMeshInstanceBuffer& InstanceBuffer)
{
	Ar << InstanceBuffer.Stride << InstanceBuffer.NumInstances;

	if(Ar.IsLoading())
	{
		// Allocate the vertex data storage type.
		InstanceBuffer.AllocateData();
	}

	// Serialize the vertex data.
	InstanceBuffer.InstanceData->Serialize(Ar);

	return Ar;
}

/**
 * Specialized assignment operator, only used when importing LOD's.  
 */
void FStaticMeshInstanceBuffer::operator=(const FStaticMeshInstanceBuffer &Other)
{
	checkf(0, TEXT("Unexpected assignment call"));
}

void FStaticMeshInstanceBuffer::InitRHI()
{
	check(InstanceData);
	FResourceArrayInterface* ResourceArray = InstanceData->GetResourceArray();
	if(ResourceArray->GetResourceDataSize())
	{
		// Create the vertex buffer.
		VertexBufferRHI = RHICreateVertexBuffer(ResourceArray->GetResourceDataSize(),ResourceArray,RUF_Static);
	}
}

void FStaticMeshInstanceBuffer::AllocateData()
{
	// Clear any old VertexData before allocating.
	CleanUp();

	const UBOOL bNeedsCPUAccess=FALSE;
	InstanceData = new FStaticMeshInstanceData(bNeedsCPUAccess);
	// Calculate the vertex stride.
	Stride = InstanceData->GetStride();
}



/*-----------------------------------------------------------------------------
	FInstancedStaticMeshVertexFactory
-----------------------------------------------------------------------------*/

/**
 * A vertex factory for instanced static meshes
 */
struct FInstancedStaticMeshVertexFactory : public FLocalVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FInstancedStaticMeshVertexFactory);
public:
	struct DataType : public FLocalVertexFactory::DataType
	{
		/** The stream to read shadow map bias (and random instance ID) from. */
		FVertexStreamComponent InstancedShadowMapBiasComponent;

		/** The stream to read the mesh transform from. */
		FVertexStreamComponent InstancedTransformComponent[3];

		/** The stream to read the inverse transform, as well as the Lightmap Bias in 0/1 */
		FVertexStreamComponent InstancedInverseTransformComponent[3];
	};

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	/**
	 * Modify compile environment to enable instancing
	 * @param OutEnvironment - shader compile environment to modify
	 */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.Definitions.Set(TEXT("USE_INSTANCING"),TEXT("1"));
	}

	/**
	 * An implementation of the interface used by TSynchronizedResource to update the resource with new data from the game thread.
	 */
	void SetData(const DataType& InData)
	{
		Data = InData;
		UpdateRHI();
	}

	/**
	 * Copy the data from another vertex factory
	 * @param Other - factory to copy from
	 */
	void Copy(const FInstancedStaticMeshVertexFactory& Other);

	// FRenderResource interface.
	virtual void InitRHI();

private:
	DataType Data;
};



/**
 * Should we cache the material's shadertype on this platform with this vertex factory? 
 */
UBOOL FInstancedStaticMeshVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return (Material->IsUsedWithInstancedMeshes() || Material->IsSpecialEngineMaterial())
			&& FLocalVertexFactory::ShouldCache(Platform, Material, ShaderType)
			&& !Material->IsUsedWithDecals(); 
}


/**
 * Copy the data from another vertex factory
 * @param Other - factory to copy from
 */
void FInstancedStaticMeshVertexFactory::Copy(const FInstancedStaticMeshVertexFactory& Other)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FInstancedStaticMeshVertexFactoryCopyData,
		FInstancedStaticMeshVertexFactory*,VertexFactory,this,
		const DataType*,DataCopy,&Other.Data,
	{
		VertexFactory->Data = *DataCopy;
	});
	BeginUpdateResourceRHI(this);
}

void FInstancedStaticMeshVertexFactory::InitRHI()
{
	// If the vertex buffer containing position is not the same vertex buffer containing the rest of the data,
	// then initialize PositionStream and PositionDeclaration.
	if(Data.PositionComponent.VertexBuffer != Data.TangentBasisComponents[0].VertexBuffer)
	{
		FVertexDeclarationElementList PositionOnlyStreamElements;
		PositionOnlyStreamElements.AddItem(AccessPositionStreamComponent(Data.PositionComponent,VEU_Position));

		// toss in the instanced location stream
		PositionOnlyStreamElements.AddItem(AccessPositionStreamComponent(Data.InstancedTransformComponent[0],VEU_TextureCoordinate, 4));
		PositionOnlyStreamElements.AddItem(AccessPositionStreamComponent(Data.InstancedTransformComponent[1],VEU_TextureCoordinate, 5));
		PositionOnlyStreamElements.AddItem(AccessPositionStreamComponent(Data.InstancedTransformComponent[2],VEU_TextureCoordinate, 6));
		InitPositionDeclaration(PositionOnlyStreamElements);
	}

	FVertexDeclarationElementList Elements;
	if(Data.PositionComponent.VertexBuffer != NULL)
	{
		Elements.AddItem(AccessStreamComponent(Data.PositionComponent,VEU_Position));
	}

	// only tangent,normal are used by the stream. the binormal is derived in the shader
	EVertexElementUsage TangentBasisUsages[2] = { VEU_Tangent, VEU_Normal };
	for(INT AxisIndex = 0;AxisIndex < 2;AxisIndex++)
	{
		if(Data.TangentBasisComponents[AxisIndex].VertexBuffer != NULL)
		{
			Elements.AddItem(AccessStreamComponent(Data.TangentBasisComponents[AxisIndex],TangentBasisUsages[AxisIndex]));
		}
	}

	// @todo: Vertex color not supported with instanced static meshes (we need the stream input for shadow maps)
	/*
	if(Data.ColorComponent.VertexBuffer)
	{
		Elements.AddItem(AccessStreamComponent(Data.ColorComponent,VEU_Color,1));
	}
	else
	{
		//If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
		//This wastes 4 bytes of bandwidth per vertex, but prevents having to compile out twice the number of vertex factories.
		FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color);
		Elements.AddItem(AccessStreamComponent(NullColorComponent,VEU_Color,1));
	}*/

	if(Data.TextureCoordinates.Num())
	{
		for(UINT CoordinateIndex = 0;CoordinateIndex < Data.TextureCoordinates.Num();CoordinateIndex++)
		{
			Elements.AddItem(AccessStreamComponent(
				Data.TextureCoordinates(CoordinateIndex),
				VEU_TextureCoordinate,
				CoordinateIndex
				));
		}

		for(UINT CoordinateIndex = Data.TextureCoordinates.Num();CoordinateIndex < InstancedStaticMeshMaxTexCoord;CoordinateIndex++)
		{
			Elements.AddItem(AccessStreamComponent(
				Data.TextureCoordinates(Data.TextureCoordinates.Num() - 1),
				VEU_TextureCoordinate,
				CoordinateIndex
				));
		}
	}

	if(Data.ShadowMapCoordinateComponent.VertexBuffer)
	{
		Elements.AddItem(AccessStreamComponent(Data.ShadowMapCoordinateComponent,VEU_Color));
	}
	else if(Data.TextureCoordinates.Num())
	{
		Elements.AddItem(AccessStreamComponent(Data.TextureCoordinates(0),VEU_Color));
	}

	// toss in the instanced location stream
	Elements.AddItem(AccessStreamComponent(Data.InstancedShadowMapBiasComponent,VEU_Color, 1));
	Elements.AddItem(AccessStreamComponent(Data.InstancedTransformComponent[0],VEU_TextureCoordinate, 4));
	Elements.AddItem(AccessStreamComponent(Data.InstancedTransformComponent[1],VEU_TextureCoordinate, 5));
	Elements.AddItem(AccessStreamComponent(Data.InstancedTransformComponent[2],VEU_TextureCoordinate, 6));
 	Elements.AddItem(AccessStreamComponent(Data.InstancedInverseTransformComponent[0],VEU_TextureCoordinate, 7));
 	Elements.AddItem(AccessStreamComponent(Data.InstancedInverseTransformComponent[1],VEU_BlendIndices, 0));
 	Elements.AddItem(AccessStreamComponent(Data.InstancedInverseTransformComponent[2],VEU_Binormal, 0));

	// we don't need per-vertex shadow or lightmap rendering
	InitDeclaration(Elements,Data, FALSE, FALSE, FALSE);
}

class FInstancedStaticMeshVertexFactoryShaderParameters : public FLocalVertexFactoryShaderParameters
{
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		FLocalVertexFactoryShaderParameters::Bind(ParameterMap);
		InstancedViewTranslationParameter.Bind(ParameterMap,TEXT("InstancedViewTranslation"));
		InstancingParameters.Bind(ParameterMap,TEXT("InstancingParameters"),TRUE);
	}

	void SetMesh(FShader* VertexShader,const FMeshElement& Mesh,const FSceneView& View) const
	{
		SetVertexShaderValue(
			VertexShader->GetVertexShader(),
			InstancedViewTranslationParameter,
			View.PreViewTranslation
			);

		if ( InstancingParameters.IsBound() )
		{
			INT NumVerticesInInstancedBuffer = Mesh.MaxVertexIndex + 1;
			FVector4 Parameters( FLOAT(NumVerticesInInstancedBuffer), 1.0f/FLOAT(NumVerticesInInstancedBuffer), 0.0f, 0.0f );
			SetVertexShaderValue( VertexShader->GetVertexShader(), InstancingParameters, Parameters );
		}

		if (LocalToWorldRotDeterminantFlipParameter.IsBound())
		{
			// Used to flip the normal direction if LocalToWorldRotDeterminant is negative.  
			// This prevents non-uniform negative scaling from making vectors transformed with CalcTangentToWorld pointing in the wrong quadrant.
			FLOAT LocalToWorldRotDeterminant = Mesh.LocalToWorld.RotDeterminant();
			SetVertexShaderValue(
				VertexShader->GetVertexShader(),
				LocalToWorldRotDeterminantFlipParameter,
				appFloatSelect(LocalToWorldRotDeterminant, 1, -1)
				);
		}

//		SetVertexShaderValue(VertexShader->GetVertexShader(),WorldToLocalParameter,Mesh.WorldToLocal);
	}

	void Serialize(FArchive& Ar)
	{
		FLocalVertexFactoryShaderParameters::Serialize(Ar);
		Ar << InstancedViewTranslationParameter;
		Ar << InstancingParameters;
	}

private:
	FShaderParameter InstancedViewTranslationParameter;
	FShaderParameter InstancingParameters;
};


IMPLEMENT_VERTEX_FACTORY_TYPE(FInstancedStaticMeshVertexFactory,FInstancedStaticMeshVertexFactoryShaderParameters,"LocalVertexFactory",TRUE,TRUE,TRUE,TRUE, VER_XBOXINSTANCING,0);




/*-----------------------------------------------------------------------------
	FInstancedStaticMeshRenderData
-----------------------------------------------------------------------------*/

class FInstancedStaticMeshRenderData
{
public:

	FInstancedStaticMeshRenderData(UInstancedStaticMeshComponent* InComponent)
	  : Component(InComponent)
	  , StaticMeshRenderData(Component->StaticMesh->LODModels(0))
	{
		// initialize the instance buffer from the component's instances
		InstanceBuffer.Init(Component->LocalToWorld, Component->PerInstanceSMData, InComponent->InstancingRandomSeed );
		InitResources();
	}

	~FInstancedStaticMeshRenderData()
	{
		ReleaseResources();
	}

	void InitResources()
	{
		BeginInitResource(&InstanceBuffer);

		// Initialize the static mesh's vertex factory.
		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			CallInitStaticMeshVertexFactory,
			FInstancedStaticMeshVertexFactory*,VertexFactory,&VertexFactory,
			FInstancedStaticMeshRenderData*,InstancedRenderData,this,
			UStaticMesh*,Parent,Component->StaticMesh,
		{
			InitStaticMeshVertexFactory( VertexFactory, InstancedRenderData, Parent );
		});

		BeginInitResource(&VertexFactory);
	}

	void ReleaseResources()
	{
		InstanceBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}

	static void InitStaticMeshVertexFactory(
		FInstancedStaticMeshVertexFactory* VertexFactory,
		FInstancedStaticMeshRenderData* InstancedRenderData,
		UStaticMesh* Parent);

	/** Source component */
	UInstancedStaticMeshComponent* Component;

	/** Instance buffer */
	FStaticMeshInstanceBuffer InstanceBuffer;

	/** Vertex factory */
	FInstancedStaticMeshVertexFactory VertexFactory;

	/** The static mesh render data for a single mesh. */
	const FStaticMeshRenderData& StaticMeshRenderData;
};

void FInstancedStaticMeshRenderData::InitStaticMeshVertexFactory(
		FInstancedStaticMeshVertexFactory* VertexFactory,
		FInstancedStaticMeshRenderData* InstancedRenderData,
		UStaticMesh* Parent)
	{
			const FStaticMeshRenderData* RenderData = &InstancedRenderData->StaticMeshRenderData;

			FInstancedStaticMeshVertexFactory::DataType Data;
			Data.PositionComponent = FVertexStreamComponent(
				&RenderData->PositionVertexBuffer,
				STRUCT_OFFSET(FPositionVertex,Position),
				RenderData->PositionVertexBuffer.GetStride(),
				VET_Float3
				);
			Data.TangentBasisComponents[0] = FVertexStreamComponent(
				&RenderData->VertexBuffer,
				STRUCT_OFFSET(FStaticMeshFullVertex,TangentX),
				RenderData->VertexBuffer.GetStride(),
				VET_PackedNormal
				);
			Data.TangentBasisComponents[1] = FVertexStreamComponent(
				&RenderData->VertexBuffer,
				STRUCT_OFFSET(FStaticMeshFullVertex,TangentZ),
				RenderData->VertexBuffer.GetStride(),
				VET_PackedNormal
				);

			// @todo: Vertex color not supported with instanced static meshes (we need the stream input for shadow maps)
			/*
			if( RenderData->ColorVertexBuffer.GetNumVertices() > 0 )
			{
				Data.ColorComponent = FVertexStreamComponent(
					&RenderData->ColorVertexBuffer,
					0,	// Struct offset to color
					RenderData->ColorVertexBuffer.GetStride(),
					VET_Color
					);
			}
			*/

			Data.TextureCoordinates.Empty();

			if( !RenderData->VertexBuffer.GetUseFullPrecisionUVs() )
			{
				for(UINT UVIndex = 0;UVIndex < RenderData->VertexBuffer.GetNumTexCoords();UVIndex++)
				{
					Data.TextureCoordinates.AddItem(FVertexStreamComponent(
						&RenderData->VertexBuffer,
						STRUCT_OFFSET(TStaticMeshFullVertexFloat16UVs<InstancedStaticMeshMaxTexCoord>,UVs) + sizeof(FVector2DHalf) * UVIndex,
						RenderData->VertexBuffer.GetStride(),
						VET_Half2
						));
				}
				if(	Parent->LightMapCoordinateIndex >= 0 && (UINT)Parent->LightMapCoordinateIndex < RenderData->VertexBuffer.GetNumTexCoords())
				{
					Data.ShadowMapCoordinateComponent = FVertexStreamComponent(
						&RenderData->VertexBuffer,
						STRUCT_OFFSET(TStaticMeshFullVertexFloat16UVs<InstancedStaticMeshMaxTexCoord>,UVs) + sizeof(FVector2DHalf) * Parent->LightMapCoordinateIndex,
						RenderData->VertexBuffer.GetStride(),
						VET_Half2
						);
				}
			}
			else
			{
				for(UINT UVIndex = 0;UVIndex < RenderData->VertexBuffer.GetNumTexCoords();UVIndex++)
				{
					Data.TextureCoordinates.AddItem(FVertexStreamComponent(
						&RenderData->VertexBuffer,
						STRUCT_OFFSET(TStaticMeshFullVertexFloat32UVs<InstancedStaticMeshMaxTexCoord>,UVs) + sizeof(FVector2D) * UVIndex,
						RenderData->VertexBuffer.GetStride(),
						VET_Float2
						));
				}

				if(	Parent->LightMapCoordinateIndex >= 0 && (UINT)Parent->LightMapCoordinateIndex < RenderData->VertexBuffer.GetNumTexCoords())
				{
					Data.ShadowMapCoordinateComponent = FVertexStreamComponent(
						&RenderData->VertexBuffer,
						STRUCT_OFFSET(TStaticMeshFullVertexFloat32UVs<InstancedStaticMeshMaxTexCoord>,UVs) + sizeof(FVector2D) * Parent->LightMapCoordinateIndex,
						RenderData->VertexBuffer.GetStride(),
						VET_Float2
						);
				}
			}	

			
			// Shadow map bias (and random instance ID)
			INT CurInstanceBufferOffset = 0;
			Data.InstancedShadowMapBiasComponent = FVertexStreamComponent(
				&InstancedRenderData->InstanceBuffer,
				CurInstanceBufferOffset, 
				InstancedRenderData->InstanceBuffer.GetStride(),
				VET_Float4,
				TRUE
				);
			CurInstanceBufferOffset += sizeof(FLOAT) * 4;

			for (INT MatrixRow = 0; MatrixRow < 3; MatrixRow++)
			{
				Data.InstancedTransformComponent[MatrixRow] = FVertexStreamComponent(
					&InstancedRenderData->InstanceBuffer,
					CurInstanceBufferOffset, 
					InstancedRenderData->InstanceBuffer.GetStride(),
					VET_Float4,
					TRUE
					);
				CurInstanceBufferOffset += sizeof(FLOAT) * 4;
			}

			for (INT MatrixRow = 0; MatrixRow < 3; MatrixRow++)
			{
				Data.InstancedInverseTransformComponent[MatrixRow] = FVertexStreamComponent(
					&InstancedRenderData->InstanceBuffer,
					CurInstanceBufferOffset, 
					InstancedRenderData->InstanceBuffer.GetStride(),
					VET_Float4,
					TRUE
					);
				CurInstanceBufferOffset += sizeof(FLOAT) * 4;
			}

			Data.NumVerticesPerInstance = RenderData->PositionVertexBuffer.GetNumVertices();
			Data.NumInstances = InstancedRenderData->InstanceBuffer.GetNumInstances();

			VertexFactory->SetData(Data);
	}

/*-----------------------------------------------------------------------------
	FInstancedStaticMeshSceneProxy
-----------------------------------------------------------------------------*/


class FInstancedStaticMeshSceneProxyInstanceData
{

public: 

	/** Cached instance -> World transform */
	FMatrix InstanceToWorld;
};


class FInstancedStaticMeshSceneProxy : public FStaticMeshSceneProxy
{
public:

	FInstancedStaticMeshSceneProxy(UInstancedStaticMeshComponent* InComponent):
	  FStaticMeshSceneProxy(InComponent),
		  StaticMeshRenderData(InComponent->StaticMesh->LODModels(0)),
		  InstancedRenderData(InComponent),
		  Component(InComponent)
	{
		// Copy and cache any per-instance data that we need
		if( InComponent->PerInstanceSMData.Num() > 0 )
		{
			PerInstanceSMData.Add( InComponent->PerInstanceSMData.Num() );
			for( INT CurInstanceIndex = 0; CurInstanceIndex < InComponent->PerInstanceSMData.Num(); ++CurInstanceIndex )
			{
				const FInstancedStaticMeshInstanceData& CurInstanceComponentData = InComponent->PerInstanceSMData( CurInstanceIndex );
				FInstancedStaticMeshSceneProxyInstanceData& CurInstanceSceneProxyData = PerInstanceSMData( CurInstanceIndex );

				// Cache instance -> world transform
				CurInstanceSceneProxyData.InstanceToWorld = CurInstanceComponentData.Transform * InComponent->LocalToWorld;
			}
		}

		// make sure all the materials are okay to be rendered as an instanced mesh
		for (INT LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			FStaticMeshSceneProxy::FLODInfo& LODInfo = LODs(LODIndex);
			for (INT ElementIndex = 0; ElementIndex < LODInfo.Elements.Num(); ElementIndex++)
			{
				FStaticMeshSceneProxy::FLODInfo::FElementInfo& Element = LODInfo.Elements(ElementIndex);
				if (!Element.Material->CheckMaterialUsage(MATUSAGE_InstancedMeshes))
				{
					Element.Material = GEngine->DefaultMaterial;
				}
			}
		}
	}

	// FPrimitiveSceneProxy interface.
	
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		if(View->Family->ShowFlags & SHOW_InstancedStaticMeshes)
		{
			Result = FStaticMeshSceneProxy::GetViewRelevance(View);
		}
		return Result;
	}

	/** Sets up a FMeshElement for a specific LOD and element. */
	virtual UBOOL GetMeshElement(INT LODIndex,INT ElementIndex,INT FragmentIndex,BYTE InDepthPriorityGroup,const FMatrix& WorldToLocal,FMeshElement& OutMeshElement, const UBOOL bUseSelectedMaterial, const UBOOL bUseHoveredMaterial) const;

	/** Sets up a wireframe FMeshElement for a specific LOD. */
	virtual UBOOL GetWireframeMeshElement(INT LODIndex, const FMaterialRenderProxy* WireframeRenderProxy, BYTE InDepthPriorityGroup, const FMatrix& WorldToLocal, FMeshElement& OutMeshElement) const;

	/**
	 * Returns whether or not this component is instanced.
	 * The base implementation returns FALSE.  You should override this method in derived classes.
	 *
	 * @return	TRUE if this component represents multiple instances of a primitive.
	 */
	virtual UBOOL IsInstanced() const
	{
		return TRUE;
	}

	/**
	 * For instanced components, returns the number of instances.
	 * The base implementation returns zero.  You should override this method in derived classes.
	 *
	 * @return	Number of instances
	 */
	virtual INT GetInstanceCount() const
	{
		return PerInstanceSMData.Num();
	}

	/**
	 * For instanced components, returns the Local -> World transform for the specific instance number.
	 * If the function is called on non-instanced components, the component's LocalToWorld will be returned.
	 * You should override this method in derived classes that support instancing.
	 *
	 * @param	InInstanceIndex	The index of the instance to return the Local -> World transform for
	 *
	 * @return	Number of instances
	 */
	virtual const FMatrix& GetInstanceLocalToWorld( INT InInstanceIndex ) const
	{
		return PerInstanceSMData( InInstanceIndex ).InstanceToWorld;
	}

	/**
	* Instanced meshes do not support LODs at this time, so always return 0
	*
	* @param	View The view to check for
	*
	* @return	Index of the LOD to use
	*/
	INT GetLOD(const FSceneView* View) const
	{
		return 0;
	}

private:

	/** The static mesh render data */
	const FStaticMeshRenderData& StaticMeshRenderData;

	/** Array of per-instance static mesh rendering data for the scene proxy */
	TArray< FInstancedStaticMeshSceneProxyInstanceData > PerInstanceSMData;

	/** Per component render data */
	FInstancedStaticMeshRenderData InstancedRenderData;

	/** The component. */
	UInstancedStaticMeshComponent* Component;
};


/** Sets up a FMeshElement for a specific LOD and element. */
UBOOL FInstancedStaticMeshSceneProxy::GetMeshElement(INT LODIndex,INT ElementIndex,INT FragmentIndex,BYTE InDepthPriorityGroup,const FMatrix& WorldToLocal,FMeshElement& OutMeshElement, const UBOOL bUseSelectedMaterial, const UBOOL bUseHoveredMaterial) const
{
	checkf(FragmentIndex == 0, TEXT("Geting instanced static mesh element with invalid params [%d, %d, %d]"), LODIndex, ElementIndex, FragmentIndex);

	if (LODIndex == 0 && FStaticMeshSceneProxy::GetMeshElement(LODIndex, ElementIndex, FragmentIndex, InDepthPriorityGroup, WorldToLocal, OutMeshElement, bUseSelectedMaterial, bUseHoveredMaterial))
	{
		OutMeshElement.VertexFactory = &InstancedRenderData.VertexFactory;
		return TRUE;
	}
	return FALSE;
};

/** Sets up a wireframe FMeshElement for a specific LOD. */
UBOOL FInstancedStaticMeshSceneProxy::GetWireframeMeshElement(INT LODIndex, const FMaterialRenderProxy* WireframeRenderProxy, BYTE InDepthPriorityGroup, const FMatrix& WorldToLocal, FMeshElement& OutMeshElement) const
{
	if (LODIndex == 0 && FStaticMeshSceneProxy::GetWireframeMeshElement(LODIndex, WireframeRenderProxy, InDepthPriorityGroup, WorldToLocal, OutMeshElement))
	{
		OutMeshElement.VertexFactory = &InstancedRenderData.VertexFactory;
		return TRUE;
	}
	return FALSE;
}

/*-----------------------------------------------------------------------------
	FInstancedStaticMeshStaticLightingMesh
-----------------------------------------------------------------------------*/

/**
 * A static lighting mesh class that transforms the points by the per-instance transform of an 
 * InstancedStaticMeshComponent
 */
class FInstancedStaticMeshStaticLightingMesh : public FStaticMeshStaticLightingMesh
{
public:

	/** Initialization constructor. */
	FInstancedStaticMeshStaticLightingMesh(const UInstancedStaticMeshComponent* InPrimitive, INT InstanceIndex, const TArray<ULightComponent*>& InRelevantLights)
		: FStaticMeshStaticLightingMesh(InPrimitive, 0, InRelevantLights)
	{
		// override the local to world to combine the per instance transform with the component's standard transform
		SetLocalToWorld(InPrimitive->PerInstanceSMData(InstanceIndex).Transform * InPrimitive->LocalToWorld);
	}
};



/*-----------------------------------------------------------------------------
	FInstancedStaticMeshStaticLightingTextureMapping
-----------------------------------------------------------------------------*/


/** Represents a static mesh primitive with texture mapped static lighting. */
class FInstancedStaticMeshStaticLightingTextureMapping : public FStaticMeshStaticLightingTextureMapping
{
public:

	/** Initialization constructor. */
	FInstancedStaticMeshStaticLightingTextureMapping(UInstancedStaticMeshComponent* InPrimitive,INT InInstanceIndex,FStaticLightingMesh* InMesh,INT InSizeX,INT InSizeY,INT InTextureCoordinateIndex,UBOOL bPerformFullQualityRebuild)
		: FStaticMeshStaticLightingTextureMapping(InPrimitive, 0, InMesh, InSizeX, InSizeY, InTextureCoordinateIndex, bPerformFullQualityRebuild)
		, InstanceIndex(InInstanceIndex), LightMapData(NULL), QuantizedData(NULL)
		, bComplete(FALSE)
	{
	}

	// FStaticLightingTextureMapping interface
	virtual void Apply(FLightMapData2D* InLightMapData,const TMap<ULightComponent*,FShadowMapData2D*>& InShadowMapData, FQuantizedLightmapData* InQuantizedData)
	{
		check(bComplete == FALSE);

		UInstancedStaticMeshComponent* InstancedComponent = Cast<UInstancedStaticMeshComponent>(Primitive);
		// note that a mapping was completed
		InstancedComponent->NumPendingLightmaps--;

		// Save the static lighting until all of the component's static lighting has been built.
		LightMapData = InLightMapData;
		ShadowMapData = InShadowMapData;
		QuantizedData = InQuantizedData;
		bComplete = TRUE;

		if (InstancedComponent->NumPendingLightmaps == 0)
		{
			InstancedComponent->ApplyAllMappings();
		}
	}

private:

	friend class UInstancedStaticMeshComponent;

	/** The instance of the primitive this mapping represents. */
	const INT InstanceIndex;

	/** Static lighting data */
	FLightMapData2D* LightMapData;
	TMap<ULightComponent*,FShadowMapData2D*> ShadowMapData;

	/** Quantized light map data */
	FQuantizedLightmapData* QuantizedData;

	/** Has this mapping already been completed? */
	UBOOL bComplete;
};


/*-----------------------------------------------------------------------------
	UInstancedStaticMeshComponent
-----------------------------------------------------------------------------*/


FPrimitiveSceneProxy* UInstancedStaticMeshComponent::CreateSceneProxy()
{
	// Verify that the mesh is valid before using it.
	const UBOOL bMeshIsValid = 
		// make sure we have instances
		PerInstanceSMData.Num() > 0 &&
		// make sure we have an actual staticmesh
		StaticMesh &&
		StaticMesh->LODModels(0).NumVertices > 0 && 
		StaticMesh->LODModels(0).IndexBuffer.Indices.Num() > 0 && 
		// You really can't use hardware instancing on the consoles with multiple elements because they share the same index buffer. 
		// @todo: Level error or something to let LDs know this
		1;//StaticMesh->LODModels(0).Elements.Num() == 1;

	if(bMeshIsValid)
	{
		// If we don't have a random seed for this instanced static mesh component yet, then go ahead and
		// generate one now.  This will be saved with the static mesh component and used for future generation
		// of random numbers for this component's instances. (Used by the PerInstanceRandom material expression)
		while( InstancingRandomSeed == 0 )
		{
			InstancingRandomSeed = appRand();
		}

		return ::new FInstancedStaticMeshSceneProxy(this);
	}
	else
	{
		return NULL;
	}
}

void UInstancedStaticMeshComponent::UpdateBounds()
{
	if(StaticMesh && PerInstanceSMData.Num() > 0)
	{
		// Graphics bounds.
		Bounds = StaticMesh->Bounds.TransformBy(PerInstanceSMData(0).Transform * LocalToWorld);

 		for (INT InstanceIndex = 1; InstanceIndex < PerInstanceSMData.Num(); InstanceIndex++)
 		{
 			Bounds = Bounds + StaticMesh->Bounds.TransformBy(PerInstanceSMData(InstanceIndex).Transform * LocalToWorld);
 		}

		// Takes into account that the static mesh collision code nudges collisions out by up to 1 unit.
		Bounds.BoxExtent += FVector(1,1,1);
		Bounds.SphereRadius += 1.0f;
	}
	else
	{
		Super::UpdateBounds();
	}
}

void UInstancedStaticMeshComponent::GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options)
{
	// toss any stale mapping data
	CachedMappings.Empty();

	if( StaticMesh && HasStaticShadowing() && bAcceptsLights )
	{

		if( !Options.bOnlyBuildSelected || GetOwner()->IsSelected() )
		{
			// If we aren't building only selected actors or we are but the actor is selected,
			// remember the actor as an actor with instanced components that need processing after lighting
			ActorsWithInstancedComponents.Add(GetOwner());
		}

		// create static lighting for LOD 0 (we only support one lod)
		INT		LightMapWidth	= 0;
		INT		LightMapHeight	= 0;
		GetLightMapResolution( LightMapWidth, LightMapHeight );

		// Create a static lighting mesh for the LOD.
		for (INT InstanceIndex = 0; InstanceIndex < PerInstanceSMData.Num(); InstanceIndex++)
		{
			FInstancedStaticMeshStaticLightingMesh* StaticLightingMesh = new FInstancedStaticMeshStaticLightingMesh(this, InstanceIndex, InRelevantLights);
			OutPrimitiveInfo.Meshes.AddItem(StaticLightingMesh);

			FInstancedStaticMeshStaticLightingTextureMapping* InstancedMapping = new FInstancedStaticMeshStaticLightingTextureMapping(
				this, InstanceIndex, StaticLightingMesh, LightMapWidth, LightMapHeight, StaticMesh->LightMapCoordinateIndex, TRUE);

			// add the mapping to the lighting info and the local list of mappings for delayed applying
			OutPrimitiveInfo.Mappings.AddItem(InstancedMapping);

			// cache the mapping in the list of all mappings for this component
			INT Index = CachedMappings.AddZeroed();
			CachedMappings(Index).Mapping = InstancedMapping;
		}
	}

	// reset the component's pending lightmap count (since we need to requantize all the mapping for 1 lightmap scale)
	NumPendingLightmaps = CachedMappings.Num();
}

/**
 * Refreshes the instance buffer with the current contents of the component's PerInstanceSMData
 */
void UInstancedStaticMeshComponent::UpdateInstances()
{
	FComponentReattachContext Reattach(this);
}

/**
 * After all lighting mappings come back
 */
void UInstancedStaticMeshComponent::ApplyAllMappings()
{
	// Calculate the range of each coefficient in this light-map.
	FLOAT MaxCoefficient[NUM_STORED_LIGHTMAP_COEF][3];
	for(INT CoefficientIndex = 0;CoefficientIndex < NUM_STORED_LIGHTMAP_COEF;CoefficientIndex++)
	{
		for(INT ColorIndex = 0;ColorIndex < 3;ColorIndex++)
		{
			MaxCoefficient[CoefficientIndex][ColorIndex] = 0;
		}
	}

	// first, we need to find the max scale for all mappings, and that will be the scale across all instances of this component
	for (INT MappingIndex = 0; MappingIndex < CachedMappings.Num(); MappingIndex++)
	{
		FInstancedStaticMeshStaticLightingTextureMapping* Mapping = CachedMappings(MappingIndex).Mapping;

		// if we need to quantize, calc the max
		if (Mapping->QuantizedData)
		{
			for(INT CoefficientIndex = 0;CoefficientIndex < NUM_STORED_LIGHTMAP_COEF;CoefficientIndex++)
			{
				for(INT ColorIndex = 0;ColorIndex < 3;ColorIndex++)
				{
					MaxCoefficient[CoefficientIndex][ColorIndex] = Max<FLOAT>(
						Mapping->QuantizedData->Scale[CoefficientIndex][ColorIndex],
						MaxCoefficient[CoefficientIndex][ColorIndex]);
				}
			}

		}
		else
		{
			for(UINT Y = 0;Y < Mapping->LightMapData->GetSizeY();Y++)
			{
				for(UINT X = 0;X < Mapping->LightMapData->GetSizeX();X++)
				{
					const FLightSample& SourceSample = (*Mapping->LightMapData)(X,Y);
					if(SourceSample.bIsMapped)
					{
						for(INT CoefficientIndex = 0;CoefficientIndex < NUM_STORED_LIGHTMAP_COEF;CoefficientIndex++)
						{
							for(INT ColorIndex = 0;ColorIndex < 3;ColorIndex++)
							{
								MaxCoefficient[CoefficientIndex][ColorIndex] = Max<FLOAT>(
									SourceSample.Coefficients[CoefficientIndex][ColorIndex],
									MaxCoefficient[CoefficientIndex][ColorIndex]);
							}
						}
					}
				}
			}
		}
		
	}

	TArray<FGuid> AllRelevantLights;
	TArray<FGuid> AllLights;

	// generate the final lightmaps for all the mappings for this component
	for (INT MappingIndex = 0; MappingIndex < CachedMappings.Num(); MappingIndex++)
	{
		FInstancedStaticMeshStaticLightingTextureMapping* Mapping = CachedMappings(MappingIndex).Mapping;

		// Determine the material to use for grouping the light-maps and shadow-maps.
		UMaterialInterface* const Material = GetNumElements() == 1 ? GetMaterial(0) : NULL;

		// Ensure LODData has enough entries in it, free not required.
		SetLODDataCount(Mapping->LODIndex + 1, StaticMesh->LODModels.Num());

		FStaticMeshComponentLODInfo& ComponentLODInfo = LODData(Mapping->LODIndex);

		const ELightMapPaddingType PaddingType = GAllowLightmapPadding ? LMPT_NormalPadding : LMPT_NoPadding;

		// Create a light-map for the primitive.
		FInstancedLightMap2D* NewLightMap = FInstancedLightMap2D::AllocateLightMap(
			// we know the Primitive is Instanced because of the constructor for this class
			this,
			Mapping->InstanceIndex,
			Mapping->LightMapData,
			Mapping->QuantizedData,
			Material,
			Bounds,
			PaddingType,
			LMF_Streamed,
			MaxCoefficient
			);

		// only apply one lightmap to the component
		if (Mapping->InstanceIndex == 0)
		{
			ComponentLODInfo.LightMap = NewLightMap;
			ComponentLODInfo.ShadowVertexBuffers.Empty();
			ComponentLODInfo.ShadowMaps.Empty();
		}
		else
		{
			// we need the base lightmap to contain all of the lights used by all lightmaps in the component
			for (INT LightIndex = 0; LightIndex < NewLightMap->LightGuids.Num(); LightIndex++)
			{
				ComponentLODInfo.LightMap->LightGuids.AddUniqueItem(NewLightMap->LightGuids(LightIndex));
			}
		}

		CachedMappings(Mapping->InstanceIndex).LightMap = NewLightMap;

		// Create the shadow-maps for the primitive.
		for(TMap<ULightComponent*,FShadowMapData2D*>::TConstIterator ShadowMapDataIt(Mapping->ShadowMapData);ShadowMapDataIt;++ShadowMapDataIt)
		{
			UShadowMap2D* NewShadowMap = new(Owner) UShadowMap2D(
				this,
				*ShadowMapDataIt.Value(),
				ShadowMapDataIt.Key()->LightGuid,
				Material,
				Bounds,
				PaddingType,
				SMF_Streamed,
				Mapping->InstanceIndex );

			ComponentLODInfo.ShadowMaps.AddItem( NewShadowMap );
			delete ShadowMapDataIt.Value();
		}

		// Build the list of statically irrelevant lights.
		// TODO: This should be stored per LOD.
		TArray< FGuid > MappingIrrelevantLights;
		for(INT LightIndex = 0;LightIndex < Mapping->Mesh->RelevantLights.Num();LightIndex++)
		{
			const ULightComponent* Light = Mapping->Mesh->RelevantLights(LightIndex);

			// Check if the light is stored in the light-map.
			const UBOOL bIsInLightMap = ComponentLODInfo.LightMap && ComponentLODInfo.LightMap->LightGuids.ContainsItem(Light->LightmapGuid);

			// Check if the light is stored in the shadow-map.
			UBOOL bIsInShadowMap = FALSE;
			for(INT ShadowMapIndex = 0;ShadowMapIndex < ComponentLODInfo.ShadowMaps.Num();ShadowMapIndex++)
			{
				if(ComponentLODInfo.ShadowMaps(ShadowMapIndex)->GetLightGuid() == Light->LightGuid)
				{
					bIsInShadowMap = TRUE;
					break;
				}
			}

			// Add the light to the statically irrelevant light list if it is in the potentially relevant light list, but didn't contribute to the light-map or a shadow-map.
 			if(!bIsInLightMap && !bIsInShadowMap)
 			{	
 				MappingIrrelevantLights.AddUniqueItem(Light->LightGuid);
 			}

			// Add the light to the statically irrelevant light list if it is in the potentially relevant light list, but didn't contribute to the light-map or a shadow-map.
			if(bIsInLightMap || bIsInShadowMap)
			{	
				AllRelevantLights.AddUniqueItem(Light->LightGuid);
			}
			AllLights.AddUniqueItem(Light->LightGuid);
		}
	}


	// find which lights were not relevant for any mapping, and add those to the Irrelevant lights list
	IrrelevantLights.Empty();
	for (INT LightIndex = 0; LightIndex < AllLights.Num(); LightIndex++)
	{
		const FGuid& CurLightGuid = AllLights( LightIndex );

		// @todo: IrrelevantLights needs to be computed properly for final component set
		//		if (AllRelevantLights.FindItemIndex( CurLightGuid ) == INDEX_NONE)
		{
			IrrelevantLights.AddUniqueItem( CurLightGuid );
		}
	}
}



/**
 * Structure that maps a component to it's lighting/instancing specific data which must be the same
 * between all instances that are bound to that component.
 */
struct FComponentInstanceSharingData
{
	/** The component that is associated (owns) this data */
	UInstancedStaticMeshComponent* Component;

	/** Light map texture */
	UTexture* LightMapTexture;

	/** Shadow map texture (or NULL if no shadow map) */
	UTexture* ShadowMapTexture;


	FComponentInstanceSharingData()
		: Component( NULL ),
		  LightMapTexture( NULL ),
		  ShadowMapTexture( NULL )
	{
	}
};


/**
 * Helper struct to hold information about what components use what lightmap textures
 */
struct FComponentInstancedLightmapData
{
	/** List of all original components and their original instances containing */
	TMap<UInstancedStaticMeshComponent*, TArray<FInstancedStaticMeshInstanceData> > ComponentInstances;

	/** List of new components */
	TArray< FComponentInstanceSharingData > SharingData;
};

/**
 * Struct that controls what we use to determine compatible components
 */
struct FValidCombination
{
	/** An optional key for marking components as compatible (eg proc buildings only allow meshes on a single face to join) */
	INT JoinKey;

	/** Different meshes are never compatible */
	UStaticMesh* Mesh;

	friend UBOOL operator==(const FValidCombination& A, const FValidCombination& B)
	{
		return A.JoinKey == B.JoinKey && A.Mesh == B.Mesh;
	}

	friend DWORD GetTypeHash(const FValidCombination& Combo)
	{
		return (DWORD)(PTRINT)Combo.Mesh * Combo.JoinKey;
	}
};

/**
 * Split up/join components into multiple components based on their staticmesh and ComponentJoinKey 
 *
 * @param bWasLightingSuccessful If TRUE, lighting should be applied, etc. If not, just clean up
 * @param bIgnoreTextureForBatching - If TRUE, this is for recombining all components back together prior to lighting
 */
void UInstancedStaticMeshComponent::ResolveInstancedLightmaps(UBOOL bWasLightingSuccessful, UBOOL bIgnoreTextureForBatching)
{
	// cache existing information
	for (TSet<AActor*>::TIterator ActorIt(ActorsWithInstancedComponents); ActorIt; ++ActorIt) 
	{
		AActor* Actor = *ActorIt;
		ResolveInstancedLightmapsForActor(Actor, bWasLightingSuccessful, bIgnoreTextureForBatching);
	}

	// more cleanup
	ActorsWithInstancedComponents.Empty();
}

/**
 * Split up/join components into multiple components based on their staticmesh and ComponentJoinKey 
 *
 * @param Actor
 * @param bWasLightingSuccessful If TRUE, lighting should be applied, etc. If not, just clean up
 * @param bIgnoreTextureForBatching - If TRUE, this is for recombining all components back together prior to lighting
 */
void UInstancedStaticMeshComponent::ResolveInstancedLightmapsForActor(AActor* InActor, UBOOL bWasLightingSuccessful, UBOOL bIgnoreTextureForBatching)
{
	// track which textures are used by which components for each static mesh
	TMap<FValidCombination, FComponentInstancedLightmapData> JoinKeyToLightmapDataMap;

	// Keep track of all light maps so we can destroy light maps that don't end up
	// being referenced by any components afterwards.  This array will contain ref-
	// counted pointers to the light map instances.
	TArray<FLightMapRef> AllLightmaps;

	// cache all components
	TArray<UInstancedStaticMeshComponent*> AllComponents;

	// first collect all the instanced components and the current set of lightmap textures
	for (INT ComponentIndex = 0; ComponentIndex < InActor->Components.Num(); ComponentIndex++)
	{
		UInstancedStaticMeshComponent* Component = Cast<UInstancedStaticMeshComponent>(InActor->Components(ComponentIndex));

		//if we're recombining, go ahead and ignore mappings
		if (Component && Component->StaticMesh && (Component->PerInstanceSMData.Num() > 0) && ((Component->CachedMappings.Num() > 0) || (bIgnoreTextureForBatching)))
		{
			FValidCombination Combo;
			Combo.JoinKey = Component->ComponentJoinKey;
			Combo.Mesh = Component->StaticMesh;

			// track lightmap usage per component for each staticmesh
			FComponentInstancedLightmapData* DataForStaticMesh = JoinKeyToLightmapDataMap.Find(Combo);
			if (DataForStaticMesh == NULL)
			{
				DataForStaticMesh = &JoinKeyToLightmapDataMap.Set(Combo, FComponentInstancedLightmapData());
			}

			// back up the component instances, then clear them out (to be filled in below)
			DataForStaticMesh->ComponentInstances.Set(Component, Component->PerInstanceSMData);
			Component->PerInstanceSMData.Empty();

			// This component will be reassumed to use the light/shadow map and other settings from
			// the component's first instance mapping
			{
				FInstancedLightMap2D* TempLightMap = NULL;
				UTexture2D *TempLightMapTexture = NULL;
				UShadowMap2D *TempShadowMap = NULL;
				if (!bIgnoreTextureForBatching)
				{
					const FInstancedStaticMeshMappingInfo& FirstInstance = Component->CachedMappings( 0 );
					TempLightMap = FirstInstance.LightMap;
					TempLightMapTexture = FirstInstance.LightmapTexture;
					TempShadowMap = FirstInstance.ShadowmapTexture;
				}

				if( Component->LODData.Num() )
				{
					Component->LODData(0).LightMap = TempLightMap;
					Component->LODData(0).ShadowMaps.Reset();
					if( TempShadowMap != NULL )
					{
						Component->LODData(0).ShadowMaps.AddItem( TempShadowMap );
					}
				}

				// Take note that this combination is owned by this component
				FComponentInstanceSharingData NewSharingData;
				NewSharingData.Component = Component;
				NewSharingData.LightMapTexture = TempLightMapTexture;
				if( TempShadowMap != NULL )
				{
					NewSharingData.ShadowMapTexture = TempShadowMap->GetTexture();
				}
				DataForStaticMesh->SharingData.AddItem( NewSharingData );
			}

			// remember this component
			AllComponents.AddItem(Component);
		}
	}

	if (bWasLightingSuccessful)
	{
		// now move instances around between components 
		for (TMap<FValidCombination, FComponentInstancedLightmapData>::TIterator It(JoinKeyToLightmapDataMap); It; ++It)
		{
			FComponentInstancedLightmapData& Data = It.Value();

			// loop over the components that share this join key
			for (TMap<UInstancedStaticMeshComponent*, TArray<FInstancedStaticMeshInstanceData> >::TIterator It2(Data.ComponentInstances); It2; ++It2)
			{
				UInstancedStaticMeshComponent* SourceComponent = It2.Key();
				const TArray<FInstancedStaticMeshInstanceData>& Instances = It2.Value();

				//there either have to be mappings or we're going to recombine
				check((SourceComponent->CachedMappings.Num() > 0) || (bIgnoreTextureForBatching));

				// go over the source instances in this component
				for (INT InstanceIndex = 0; InstanceIndex < Instances.Num(); InstanceIndex++)
				{
					const FInstancedStaticMeshInstanceData& CurInstance = Instances( InstanceIndex );
					FInstancedLightMap2D* TestLightMap = NULL;
					UTexture2D *TestLightMapTexture = NULL;
					UShadowMap2D *TestShadowMap = NULL;
					if (!bIgnoreTextureForBatching)
					{
						const FInstancedStaticMeshMappingInfo& CurInstanceMapping = SourceComponent->CachedMappings( InstanceIndex );
						TestLightMap = CurInstanceMapping.LightMap;
						TestLightMapTexture = CurInstanceMapping.LightmapTexture;
						TestShadowMap = CurInstanceMapping.ShadowmapTexture;

						// remember this instance's lightmap
						AllLightmaps.AddItem(CurInstanceMapping.LightMap);
					}


					// Figure out if we already have a new instanced static mesh component that shares
					// the same light map and shadow map with this instance.
					UInstancedStaticMeshComponent* DestComponent = NULL;
					for( INT ExistingComponentIndex = 0; ExistingComponentIndex < Data.SharingData.Num(); ++ExistingComponentIndex )
					{
						const FComponentInstanceSharingData& SharingData = Data.SharingData( ExistingComponentIndex );
						if( TestLightMapTexture != SharingData.LightMapTexture)
						{
							continue;
						}
						const UBOOL bNeitherHasShadowMap = ( TestShadowMap == NULL && SharingData.ShadowMapTexture == NULL );
						const UBOOL bSameShadowMap = ( TestShadowMap != NULL && TestShadowMap->GetTexture() == SharingData.ShadowMapTexture );
						if( bNeitherHasShadowMap || bSameShadowMap)
						{
							// We have a winner!
							DestComponent = SharingData.Component;
							break;
						}
					}

					// If we didn't find another component that matches our requirements, then we'll need
					// to create a new component (by copying relevant data from our source component.)
					if( DestComponent == NULL )
					{
						DestComponent = ConstructObject<UInstancedStaticMeshComponent>(UInstancedStaticMeshComponent::StaticClass(), InActor);
						check(DestComponent);

//							debugf(TEXT("Making a new component %s for texture %s"), *DestComponent->GetPathName(), *CurInstanceMapping.LightmapTexture->GetPathName());

						// Set mesh
						DestComponent->SetStaticMesh(SourceComponent->StaticMesh);

						// Settings from StaticMeshActor
						DestComponent->bAllowApproximateOcclusion = SourceComponent->bAllowApproximateOcclusion;
						DestComponent->bCastDynamicShadow = SourceComponent->bCastDynamicShadow;
						DestComponent->bForceDirectLightMap = SourceComponent->bForceDirectLightMap;
						DestComponent->bUsePrecomputedShadows = SourceComponent->bUsePrecomputedShadows;
						DestComponent->BlockNonZeroExtent = SourceComponent->BlockNonZeroExtent;
						DestComponent->BlockZeroExtent = SourceComponent->BlockZeroExtent;
						DestComponent->BlockActors = SourceComponent->BlockActors;
						DestComponent->CollideActors = SourceComponent->CollideActors;
						DestComponent->bAllowCullDistanceVolume = SourceComponent->bAllowCullDistanceVolume;
						DestComponent->bOverrideLightMapRes = SourceComponent->bOverrideLightMapRes;
						DestComponent->ReplacementPrimitive = SourceComponent->ReplacementPrimitive;
						DestComponent->LODData = SourceComponent->LODData;
						check(DestComponent->LODData.Num() == 1);
						DestComponent->LODData(0).LightMap = TestLightMap;
						if (SourceComponent->LODData(0).LightMap)
						{
							DestComponent->LODData(0).LightMap->LightGuids = SourceComponent->LODData(0).LightMap->LightGuids;
						}

						DestComponent->LODData(0).ShadowMaps.Reset();
						if( TestShadowMap != NULL )
						{
							DestComponent->LODData(0).ShadowMaps.AddItem( TestShadowMap );
						}

						//Make sure to copy over the ComponentJointKey
						DestComponent->ComponentJoinKey = SourceComponent->ComponentJoinKey;

						// Preserve override materials
						DestComponent->Materials = SourceComponent->Materials;

						DestComponent->IrrelevantLights = SourceComponent->IrrelevantLights;

						// Add to BuildingMeshCompInfos so it gets saved if the outer is a building
						AProcBuilding* Building = Cast<AProcBuilding>(InActor);
						if (Building)
						{
							// find the scope for the source component
							INT OriginalScopeIndex = 0;
							for (INT InfoIndex = 0; InfoIndex < Building->BuildingMeshCompInfos.Num(); InfoIndex++)
							{
								if (Building->BuildingMeshCompInfos(InfoIndex).MeshComp == SourceComponent)
								{
									OriginalScopeIndex = Building->BuildingMeshCompInfos(InfoIndex).TopLevelScopeIndex;
									break;
								}
							}

							INT InfoIndex = Building->BuildingMeshCompInfos.AddZeroed();
							Building->BuildingMeshCompInfos(InfoIndex).MeshComp = DestComponent;
							Building->BuildingMeshCompInfos(InfoIndex).TopLevelScopeIndex = OriginalScopeIndex;		

						}

						// put the new component in the actor
						InActor->AttachComponent(DestComponent);

						// Add this (unique) combination of light/shadow map requirements to our list
						FComponentInstanceSharingData NewSharingData;
						NewSharingData.Component = DestComponent;
						NewSharingData.LightMapTexture = TestLightMapTexture;
						if( TestShadowMap != NULL )
						{
							NewSharingData.ShadowMapTexture = TestShadowMap->GetTexture();
						}
						Data.SharingData.AddItem( NewSharingData );

						// remember this component
						AllComponents.AddItem(DestComponent);
					}

					// copy this instance into whatever component it's going into
					DestComponent->PerInstanceSMData.AddItem( CurInstance );
				}
			}
		}

		for (INT ComponentIndex = 0; ComponentIndex < AllComponents.Num(); ComponentIndex++)
		{
			UInstancedStaticMeshComponent* Component = AllComponents(ComponentIndex);
			
			if (Component->PerInstanceSMData.Num() == 0)
			{
				InActor->DetachComponent(Component);

				// if this is a building, then remove the component from the list of component infos
				AProcBuilding* Building = Cast<AProcBuilding>(InActor);
				if (Building)
				{
					// find the scope for the source component
					for (INT InfoIndex = 0; InfoIndex < Building->BuildingMeshCompInfos.Num(); InfoIndex++)
					{
						if (Building->BuildingMeshCompInfos(InfoIndex).MeshComp == Component)
						{
							Building->BuildingMeshCompInfos.Remove(InfoIndex);
							break;
						}
					}
				}
			}

			// update the lightmap instance data
			Component->UpdateInstances();
		}
	}

	// Destroy unused light maps (ref counted pointers)
	AllLightmaps.Empty();
}

void UInstancedStaticMeshComponent::GetLightAndShadowMapMemoryUsage( INT& LightMapMemoryUsage, INT& ShadowMapMemoryUsage ) const
{
	Super::GetLightAndShadowMapMemoryUsage(LightMapMemoryUsage, ShadowMapMemoryUsage);

	INT NumInstances = PerInstanceSMData.Num();

	// Scale lighting demo by number of instances
	LightMapMemoryUsage *= NumInstances;
	ShadowMapMemoryUsage *= NumInstances;
}

/**
 * Serialize function.
 *
 * @param	Ar	Archive to serialize with
 */
void UInstancedStaticMeshComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if( Ar.Ver() >= VER_BULKSERIALIZE_INSTANCE_DATA )
	{
		PerInstanceSMData.BulkSerialize(Ar);
	}
	else
	{
		// Copy data around for backward compatibility.
		PerInstanceSMData = PerInstanceData_DEPRECATED;
		PerInstanceData_DEPRECATED.Empty();
	}
}

#if STATS
/**
 * Called after all objects referenced by this object have been serialized. Order of PostLoad routed to 
 * multiple objects loaded in one set is not deterministic though ConditionalPostLoad can be forced to
 * ensure an object has been "PostLoad"ed.
 */
void UInstancedStaticMeshComponent::PostLoad()
{
	Super::PostLoad();
	INC_DWORD_STAT_BY(STAT_LoadedInstances,PerInstanceSMData.Num());
}

/**
 * Informs object of pending destruction via GC.
 */
void UInstancedStaticMeshComponent::BeginDestroy()
{
	DEC_DWORD_STAT_BY(STAT_LoadedInstances,PerInstanceSMData.Num());
	Super::BeginDestroy();
}

/**
 * Attaches the component to a ParentToWorld transform, owner and scene.
 * Requires IsValidComponent() == true.
 */
void UInstancedStaticMeshComponent::Attach()
{
	Super::Attach();
	INC_DWORD_STAT_BY(STAT_AttachedInstances,PerInstanceSMData.Num());
}

/**
* Detaches the component from the scene it is in.
* Requires bAttached == true
*
* @param bWillReattach TRUE is passed if Attach will be called immediately afterwards.  This can be used to
*                      preserve state between reattachments.
*/
void UInstancedStaticMeshComponent::Detach( UBOOL bWillReattach )
{
	DEC_DWORD_STAT_BY(STAT_AttachedInstances,PerInstanceSMData.Num());
	Super::Detach(bWillReattach);
}
#endif


