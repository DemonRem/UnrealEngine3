/*=============================================================================
	UnSkeletalRenderGPUSkin.cpp: GPU skinned skeletal mesh rendering code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "EnginePhysicsClasses.h"
#include "GPUSkinVertexFactory.h"
#include "UnSkeletalRenderGPUSkin.h"
#include "UnDecalRenderData.h"

#if XBOX
#include "UnSkeletalRenderGPUXe.h"
#endif

#if !__INTEL_BYTE_ORDER__
// BYTE[] elements in LOD.VertexBufferGPUSkin have been swapped for VET_UBYTE4 vertex stream use
static const INT RigidBoneIdx=3;
#else
static const INT RigidBoneIdx=0;
#endif

/*-----------------------------------------------------------------------------
FGPUSkinVertexBuffer
-----------------------------------------------------------------------------*/
void FGPUSkinVertexBuffer::InitRHI()
{
	// Create the buffer rendering resource
	UINT Size = Vertices.GetResourceDataSize();
	if( Size > 0 )
	{
		VertexBufferRHI = RHICreateVertexBuffer(Size,&Vertices,FALSE);
	}
}

/**
* Serializer for this class
* @param Ar - archive to serialize to
* @param B - data to serialize
*/
FArchive& operator<<(FArchive& Ar,FGPUSkinVertexBuffer& B)
{
	if( Ar.IsSaving() && (GCookingTarget & UE3::PLATFORM_Console) && !B.bInflucencesByteSwapped )
	{
		// swap order for InfluenceBones/InfluenceWeights byte entries
		for( INT VertIdx=0; VertIdx < B.Vertices.Num(); VertIdx++ )
		{
			FSoftSkinVertex& Vert = B.Vertices(VertIdx);
			for( INT i=0; i < MAX_INFLUENCES/2; i++ )
			{
				Exchange(Vert.InfluenceBones[i],Vert.InfluenceBones[MAX_INFLUENCES-1-i]);
				Exchange(Vert.InfluenceWeights[i],Vert.InfluenceWeights[MAX_INFLUENCES-1-i]);
			}
		}
		B.bInflucencesByteSwapped=TRUE;
	}

    B.Vertices.BulkSerialize(Ar);
	return Ar;
}

/*-----------------------------------------------------------------------------
FMorphVertexBuffer
-----------------------------------------------------------------------------*/

/** 
* Initialize the dynamic RHI for this rendering resource 
*/
void FMorphVertexBuffer::InitDynamicRHI()
{
	// LOD of the skel mesh is used to find number of vertices in buffer
	FStaticLODModel& LodModel = SkelMesh->LODModels(LODIdx);

	// Create the buffer rendering resource
	UINT Size = LodModel.NumVertices * sizeof(FMorphGPUSkinVertex);
	VertexBufferRHI = RHICreateVertexBuffer(Size,NULL,TRUE);

	// Lock the buffer.
	FMorphGPUSkinVertex* Buffer = (FMorphGPUSkinVertex*) RHILockVertexBuffer(VertexBufferRHI,0,Size);

	// zero all deltas (NOTE: DeltaTangentZ is FPackedNormal, so we can't just appMemzero)
	for (UINT VertIndex=0; VertIndex < LodModel.NumVertices; ++VertIndex)
	{
		Buffer[VertIndex].DeltaPosition = FVector(0,0,0);
		Buffer[VertIndex].DeltaTangentZ = FVector(0,0,0);
	}

	// Unlock the buffer.
	RHIUnlockVertexBuffer(VertexBufferRHI);
}

/** 
* Release the dynamic RHI for this rendering resource 
*/
void FMorphVertexBuffer::ReleaseDynamicRHI()
{
	VertexBufferRHI.Release();
}

/*-----------------------------------------------------------------------------
FSkeletalMeshObjectGPUSkin
-----------------------------------------------------------------------------*/

/** 
 * Constructor 
 * @param	InSkeletalMeshComponent - skeletal mesh primitive we want to render 
 */
FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectGPUSkin(USkeletalMeshComponent* InSkeletalMeshComponent) 
:	FSkeletalMeshObject(InSkeletalMeshComponent)
,	CachedShadowLOD(INDEX_NONE)
,	DynamicData(NULL)
,	bMorphResourcesInitialized(FALSE)
{
	// create LODs to match the base mesh
	LODs.Empty(SkeletalMesh->LODModels.Num());
	for( INT LODIndex=0;LODIndex < SkeletalMesh->LODModels.Num();LODIndex++ )
	{
		new(LODs) FSkeletalMeshObjectLOD(SkeletalMesh,LODIndex,bDecalFactoriesEnabled);
	}

	InitResources();
}

/** 
 * Destructor 
 */
FSkeletalMeshObjectGPUSkin::~FSkeletalMeshObjectGPUSkin()
{
	delete DynamicData;
}

/** 
 * Initialize rendering resources for each LOD. 
 */
void FSkeletalMeshObjectGPUSkin::InitResources()
{
	for( INT LODIndex=0;LODIndex < LODs.Num();LODIndex++ )
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs(LODIndex);
		SkelLOD.InitResources(bUseLocalVertexFactory);
	}
}

/** 
 * Release rendering resources for each LOD.
 */
void FSkeletalMeshObjectGPUSkin::ReleaseResources()
{
	for( INT LODIndex=0;LODIndex < LODs.Num();LODIndex++ )
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs(LODIndex);
		SkelLOD.ReleaseResources();
	}
	// also release morph resources
	ReleaseMorphResources();
}

/** 
* Initialize morph rendering resources for each LOD 
*/
void FSkeletalMeshObjectGPUSkin::InitMorphResources()
{
	if( bMorphResourcesInitialized )
	{
		// release first if already initialized
		ReleaseMorphResources();
	}

	for( INT LODIndex=0;LODIndex < LODs.Num();LODIndex++ )
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs(LODIndex);
		// init any morph vertex buffers for each LOD
		SkelLOD.InitMorphResources();
	}
	bMorphResourcesInitialized = TRUE;
}

/** 
* Release morph rendering resources for each LOD. 
*/
void FSkeletalMeshObjectGPUSkin::ReleaseMorphResources()
{
	for( INT LODIndex=0;LODIndex < LODs.Num();LODIndex++ )
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs(LODIndex);
		// release morph vertex buffers and factories if they were created
		SkelLOD.ReleaseMorphResources();
	}
	bMorphResourcesInitialized = FALSE;
}

/**
* Called by the game thread for any dynamic data updates for this skel mesh object
* @param	LODIndex - lod level to update
* @param	InSkeletalMeshComponen - parent prim component doing the updating
* @param	ActiveMorphs - morph targets to blend with during skinning
*/
void FSkeletalMeshObjectGPUSkin::Update(INT LODIndex,USkeletalMeshComponent* InSkeletalMeshComponent,const TArray<FActiveMorph>& ActiveMorphs)
{
	// make sure morph data has been initialized for each LOD
	if( !bMorphResourcesInitialized && ActiveMorphs.Num() > 0 )
	{
		// initialized on-the-fly in order to avoid creating extra vertex streams for each skel mesh instance
		InitMorphResources();		
	}

	// create the new dynamic data for use by the rendering thread
	// this data is only deleted when another update is sent
	FDynamicSkelMeshObjectData* NewDynamicData = new FDynamicSkelMeshObjectDataGPUSkin(InSkeletalMeshComponent,LODIndex,ActiveMorphs);


	// queue a call to update this data
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		SkelMeshObjectUpdateDataCommand,
		FSkeletalMeshObject*, MeshObject, this,
		FDynamicSkelMeshObjectData*, NewDynamicData, NewDynamicData,
	{
		MeshObject->UpdateDynamicData_RenderThread(NewDynamicData);
	}
	);
}

/**
 * Called by the rendering thread to update the current dynamic data
 * @param	InDynamicData - data that was created by the game thread for use by the rendering thread
 */
void FSkeletalMeshObjectGPUSkin::UpdateDynamicData_RenderThread(FDynamicSkelMeshObjectData* InDynamicData)
{
	UBOOL bMorphNeedsUpdate=FALSE;
	// figure out if the morphing vertex buffer needs to be updated. compare old vs new active morphs
	bMorphNeedsUpdate = DynamicData ? !DynamicData->ActiveMorphTargetsEqual(((FDynamicSkelMeshObjectDataGPUSkin*)InDynamicData)->ActiveMorphs) : TRUE;

	// we should be done with the old data at this point
	delete DynamicData;
	// update with new data
	DynamicData = (FDynamicSkelMeshObjectDataGPUSkin*)InDynamicData;
	checkSlow(DynamicData);

	FSkeletalMeshObjectLOD& LOD = LODs(DynamicData->LODIndex);
	FStaticLODModel& LODModel = SkeletalMesh->LODModels(DynamicData->LODIndex);

	checkSlow( DynamicData->NumWeightedActiveMorphs == 0 || (LOD.MorphVertexFactories.Num()==LODModel.Chunks.Num()) );
	if (DynamicData->NumWeightedActiveMorphs > 0 || LOD.VertexFactories.Num() > 0)
	{
		for( INT ChunkIdx=0; ChunkIdx < LODModel.Chunks.Num(); ChunkIdx++ )
		{
			FSkelMeshChunk& Chunk = LODModel.Chunks(ChunkIdx);
			FGPUSkinVertexFactory::ShaderDataType& ShaderData = DynamicData->NumWeightedActiveMorphs > 0 ? LOD.MorphVertexFactories(ChunkIdx).GetShaderData() : LOD.VertexFactories(ChunkIdx).GetShaderData();

			// update bone matrix shader data for the vertex factory of each chunk
			TArray<FSkinMatrix3x4>& ChunkMatrices = ShaderData.BoneMatrices;
			ChunkMatrices.Reset();
			for( INT BoneIdx=0; BoneIdx < Chunk.BoneMap.Num(); BoneIdx++ )
			{
				INT MatricesToAdd = BoneIdx - ChunkMatrices.Num() + 1;
				if ( MatricesToAdd > 0 )
				{
					ChunkMatrices.Add( MatricesToAdd );
				}
				FSkinMatrix3x4& BoneMat = ChunkMatrices(BoneIdx);
				BoneMat.SetMatrixTranspose(DynamicData->ReferenceToLocal(Chunk.BoneMap(BoneIdx)));
			}

			// update the max number of influences for the vertex factory
			ShaderData.SetMaxBoneInfluences(Chunk.MaxBoneInfluences);
		}
	}

	// Decal factories
	if ( bDecalFactoriesEnabled )
	{
		checkSlow( DynamicData->NumWeightedActiveMorphs == 0 || (LOD.MorphDecalVertexFactories.Num()==LODModel.Chunks.Num()) );
		if (DynamicData->NumWeightedActiveMorphs > 0 || LOD.DecalVertexFactories.Num() > 0)
		{
			for( INT ChunkIdx=0; ChunkIdx < LODModel.Chunks.Num(); ChunkIdx++ )
			{
				FSkelMeshChunk& Chunk = LODModel.Chunks(ChunkIdx);
				FGPUSkinVertexFactory::ShaderDataType& ShaderData = DynamicData->NumWeightedActiveMorphs > 0 ? LOD.MorphDecalVertexFactories(ChunkIdx).GetShaderData() : LOD.DecalVertexFactories(ChunkIdx).GetShaderData();

				// update bone matrix shader data for the vertex factory of each chunk
				TArray<FSkinMatrix3x4>& ChunkMatrices = ShaderData.BoneMatrices;
				ChunkMatrices.Reset();
				for( INT BoneIdx=0; BoneIdx < Chunk.BoneMap.Num(); BoneIdx++ )
				{
					INT MatricesToAdd = BoneIdx - ChunkMatrices.Num() + 1;
					if ( MatricesToAdd > 0 )
					{
						ChunkMatrices.Add( MatricesToAdd );
					}
					FSkinMatrix3x4& BoneMat = ChunkMatrices(BoneIdx);
					BoneMat.SetMatrixTranspose(DynamicData->ReferenceToLocal(Chunk.BoneMap(BoneIdx)));
				}

				// update the max number of influences for the vertex factory
				ShaderData.SetMaxBoneInfluences(Chunk.MaxBoneInfluences);
			}
		}
	}

	// Flag the vertex cache as dirty, so potential shadow volumes can update it
	CachedShadowLOD = INDEX_NONE;

	// only update if the morph data changed and there are weighted morph targets
	if( bMorphNeedsUpdate &&
		DynamicData->NumWeightedActiveMorphs > 0 )
	{
		// update the morph data for the lod
		LOD.UpdateMorphVertexBuffer( DynamicData->ActiveMorphs );
	}
}

/**
* Update the contents of the morph target vertex buffer by accumulating all 
* delta positions and delta normals from the set of active morph targets
* @param ActiveMorphs - morph targets to accumulate. assumed to be weighted and have valid targets
*/
void FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectLOD::UpdateMorphVertexBuffer( const TArray<FActiveMorph>& ActiveMorphs )
{
	if( IsValidRef(MorphVertexBuffer.VertexBufferRHI) )
	{
		// LOD of the skel mesh is used to find number of vertices in buffer
		FStaticLODModel& LodModel = SkelMesh->LODModels(LODIndex);
		UINT Size = LodModel.NumVertices * sizeof(FMorphGPUSkinVertex);

		// Lock the buffer.
		FMorphGPUSkinVertex* Buffer = (FMorphGPUSkinVertex*)RHILockVertexBuffer(MorphVertexBuffer.VertexBufferRHI,0,Size);

		// zero all deltas (NOTE: DeltaTangentZ is FPackedNormal, so we can't just appMemzero)
		for (UINT VertIndex=0; VertIndex < LodModel.NumVertices; ++VertIndex)
		{
			Buffer[VertIndex].DeltaPosition = FVector(0,0,0);
			Buffer[VertIndex].DeltaTangentZ = FVector(0,0,0);
		}

		// iterate over all active morph targets and accumulate their vertex deltas
		for( INT MorphIdx=0; MorphIdx < ActiveMorphs.Num(); MorphIdx++ )
		{
			const FActiveMorph& Morph = ActiveMorphs(MorphIdx);
			checkSlow(Morph.Target && Morph.Target->MorphLODModels.IsValidIndex(LODIndex) && Morph.Target->MorphLODModels(LODIndex).Vertices.Num());
			checkSlow(Morph.Weight >= MinMorphBlendWeight && Morph.Weight <= MaxMorphBlendWeight);
			const FMorphTargetLODModel& MorphLODModel = Morph.Target->MorphLODModels(LODIndex);

			FLOAT ClampedMorphWeight = Min(Morph.Weight,1.0f);

			// iterate over the vertices that this lod model has changed
			for( INT MorphVertIdx=0; MorphVertIdx < MorphLODModel.Vertices.Num(); MorphVertIdx++ )
			{
				const FMorphTargetVertex& MorphVertex = MorphLODModel.Vertices(MorphVertIdx);
				checkSlow(MorphVertex.SourceIdx < LodModel.NumVertices);
				FMorphGPUSkinVertex& DestVertex = Buffer[MorphVertex.SourceIdx];

				DestVertex.DeltaPosition += MorphVertex.PositionDelta * Morph.Weight;
				DestVertex.DeltaTangentZ = FVector(DestVertex.DeltaTangentZ) + FVector(MorphVertex.TangentZDelta) * ClampedMorphWeight;
			}
		}

		// Unlock the buffer.
		RHIUnlockVertexBuffer(MorphVertexBuffer.VertexBufferRHI);
	}
}

/**
 * @param	LODIndex - each LOD has its own vertex data
 * @param	ChunkIdx - index for current mesh chunk
 * @return	vertex factory for rendering the LOD
 */
const FVertexFactory* FSkeletalMeshObjectGPUSkin::GetVertexFactory(INT LODIndex,INT ChunkIdx) const
{
	checkSlow( LODs.IsValidIndex(LODIndex) );
	checkSlow( DynamicData );

	if( DynamicData->NumWeightedActiveMorphs > 0 )
	{
		// use the morph enabled vertex factory if any active morphs are set
		return &LODs(LODIndex).MorphVertexFactories(ChunkIdx);
	}
	else if ( bUseLocalVertexFactory )
	{
		// use the local gpu vertex factory (when bForceRefpose is true)
		return LODs(LODIndex).LocalVertexFactory.GetOwnedPointer();
	}
	else
	{
		// use the default gpu skin vertex factory
		return &LODs(LODIndex).VertexFactories(ChunkIdx);
	}
}


/**
 * @param	LODIndex - each LOD has its own vertex data
 * @param	ChunkIdx - index for current mesh chunk
 * @return	Decal vertex factory for rendering the LOD
 */
FSkelMeshDecalVertexFactoryBase* FSkeletalMeshObjectGPUSkin::GetDecalVertexFactory(INT LODIndex,INT ChunkIdx,const FDecalInteraction* Decal)
{
	checkSlow( bDecalFactoriesEnabled );
	checkSlow( LODs.IsValidIndex(LODIndex) );

	if( DynamicData->NumWeightedActiveMorphs > 0 )
	{
		// use the morph enabled decal vertex factory if any active morphs are set
		return &LODs(LODIndex).MorphDecalVertexFactories(ChunkIdx);
	}
	else if ( LODs(LODIndex).LocalDecalVertexFactory )
	{
		// use the local decal vertex factory (when bForceRefpose is true)
		return LODs(LODIndex).LocalDecalVertexFactory.GetOwnedPointer();
	}
	else
	{
		// use the default gpu skin decal vertex factory
		return &LODs(LODIndex).DecalVertexFactories(ChunkIdx);
	}
}

/**
 * Get the shadow vertex factory for an LOD
 * @param	LODIndex - each LOD has its own vertex data
 * @return	Vertex factory for rendering the shadow volume for the LOD
 */
const FLocalShadowVertexFactory& FSkeletalMeshObjectGPUSkin::GetShadowVertexFactory( INT LODIndex ) const
{
	checkSlow( LODs.IsValidIndex(LODIndex) );
	return LODs(LODIndex).ShadowVertexBuffer.GetVertexFactory();
}

/** 
 * Init rendering resources for this LOD 
 */
void FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectLOD::InitResources(UBOOL bUseLocalVertexFactory)
{
	check(SkelMesh);
	check(SkelMesh->LODModels.IsValidIndex(LODIndex));

	// vertex buffer for each lod has already been created when skelmesh was loaded
	FStaticLODModel& LODModel = SkelMesh->LODModels(LODIndex);

	// one for each chunk
	VertexFactories.Empty();
	LocalVertexFactory.Reset();
	if (bUseLocalVertexFactory)
	{
		// only need one local vertex factory because this is no unique data per chunk when using the local factory.
		LocalVertexFactory.Reset(new FLocalVertexFactory());

		// update vertex factory components and sync it
		TSetResourceDataContext<FLocalVertexFactory> VertexFactoryData(LocalVertexFactory.GetOwnedPointer());
		// position
		VertexFactoryData->PositionComponent = FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,Position),sizeof(FSoftSkinVertex),VET_Float3);
		// tangents
		VertexFactoryData->TangentBasisComponents[0] = FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentX),sizeof(FSoftSkinVertex),VET_PackedNormal);
		VertexFactoryData->TangentBasisComponents[1] = FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentZ),sizeof(FSoftSkinVertex),VET_PackedNormal);
		// uvs
		VertexFactoryData->TextureCoordinates.AddItem(FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,U),sizeof(FSoftSkinVertex),VET_Float2));

		// copy it
		VertexFactoryData.Commit();
		// init rendering resource	
		BeginInitResource(LocalVertexFactory.GetOwnedPointer());
	}
	else
	{
		VertexFactories.Empty(LODModel.Chunks.Num());
		for( INT FactoryIdx=0; FactoryIdx < LODModel.Chunks.Num(); FactoryIdx++ )
		{
			FGPUSkinVertexFactory* VertexFactory = new(VertexFactories) FGPUSkinVertexFactory();

			// update vertex factory components and sync it
			TSetResourceDataContext<FGPUSkinVertexFactory> VertexFactoryData(VertexFactory);
			// position
			VertexFactoryData->PositionComponent = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,Position),sizeof(FSoftSkinVertex),VET_Float3);
			// tangents
			VertexFactoryData->TangentBasisComponents[0] = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentX),sizeof(FSoftSkinVertex),VET_PackedNormal);
			VertexFactoryData->TangentBasisComponents[1] = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentY),sizeof(FSoftSkinVertex),VET_PackedNormal);
			VertexFactoryData->TangentBasisComponents[2] = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentZ),sizeof(FSoftSkinVertex),VET_PackedNormal);
			// uvs
			VertexFactoryData->TextureCoordinates.AddItem(FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,U),sizeof(FSoftSkinVertex),VET_Float2));
			// bone indices
			VertexFactoryData->BoneIndices = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,InfluenceBones),sizeof(FSoftSkinVertex),VET_UByte4);
			// bone weights
			VertexFactoryData->BoneWeights = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,InfluenceWeights),sizeof(FSoftSkinVertex),VET_UByte4N);

			// copy it
			VertexFactoryData.Commit();
			// init rendering resource	
			BeginInitResource(VertexFactory);
		}
	}

	// Decals
	if ( bDecalFactoriesEnabled )
	{
		DecalVertexFactories.Empty();
		LocalDecalVertexFactory.Reset();
		if (bUseLocalVertexFactory)
		{
			// only need one local vertex factory because this is no unique data per chunk when using the local factory.
			LocalDecalVertexFactory.Reset(new FLocalDecalVertexFactory());

			// update vertex factory components and sync it
			TSetResourceDataContext<FLocalDecalVertexFactory> DecalVertexFactoryData(LocalDecalVertexFactory.GetOwnedPointer());
			// position
			DecalVertexFactoryData->PositionComponent = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,Position),sizeof(FSoftSkinVertex),VET_Float3);
			// tangents
			DecalVertexFactoryData->TangentBasisComponents[0] = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentX),sizeof(FSoftSkinVertex),VET_PackedNormal);
			DecalVertexFactoryData->TangentBasisComponents[1] = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentZ),sizeof(FSoftSkinVertex),VET_PackedNormal);
			// uvs
			DecalVertexFactoryData->TextureCoordinates.AddItem(FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,U),sizeof(FSoftSkinVertex),VET_Float2));

			// copy it
			DecalVertexFactoryData.Commit();
			// init rendering resource	
			BeginInitResource(LocalDecalVertexFactory.GetOwnedPointer());
		}
		else
		{
			DecalVertexFactories.Empty(LODModel.Chunks.Num());
			for( INT FactoryIdx=0; FactoryIdx < LODModel.Chunks.Num(); FactoryIdx++ )
			{
				FGPUSkinDecalVertexFactory* DecalVertexFactory = new(DecalVertexFactories) FGPUSkinDecalVertexFactory();

				// update vertex factory components and sync it
				TSetResourceDataContext<FGPUSkinDecalVertexFactory> DecalVertexFactoryData(DecalVertexFactory);
				// position
				DecalVertexFactoryData->PositionComponent = FVertexStreamComponent(
					&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,Position),sizeof(FSoftSkinVertex),VET_Float3);
				// tangents
				DecalVertexFactoryData->TangentBasisComponents[0] = FVertexStreamComponent(
					&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentX),sizeof(FSoftSkinVertex),VET_PackedNormal);
				DecalVertexFactoryData->TangentBasisComponents[1] = FVertexStreamComponent(
					&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentY),sizeof(FSoftSkinVertex),VET_PackedNormal);
				DecalVertexFactoryData->TangentBasisComponents[2] = FVertexStreamComponent(
					&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentZ),sizeof(FSoftSkinVertex),VET_PackedNormal);
				// uvs
				DecalVertexFactoryData->TextureCoordinates.AddItem(FVertexStreamComponent(
					&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,U),sizeof(FSoftSkinVertex),VET_Float2));
				// bone indices
				DecalVertexFactoryData->BoneIndices = FVertexStreamComponent(
					&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,InfluenceBones),sizeof(FSoftSkinVertex),VET_UByte4);
				// bone weights
				DecalVertexFactoryData->BoneWeights = FVertexStreamComponent(
					&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,InfluenceWeights),sizeof(FSoftSkinVertex),VET_UByte4N);

				// copy it
				DecalVertexFactoryData.Commit();
				// init rendering resource	
				BeginInitResource(DecalVertexFactory);
			}
		}
	}
	// End Decals

	if( UEngine::ShadowVolumesAllowed() )
	{
		// Initialize the shadow vertex buffer and its factory.
		BeginInitResource(&ShadowVertexBuffer);
	}
}

/** 
 * Release rendering resources for this LOD 
 */
void FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectLOD::ReleaseResources()
{	
	for( INT FactoryIdx=0; FactoryIdx < VertexFactories.Num(); FactoryIdx++ )
	{
		FGPUSkinVertexFactory& VertexFactory = VertexFactories(FactoryIdx);
		BeginReleaseResource(&VertexFactory);
	}
	if (LocalVertexFactory)
	{
		BeginReleaseResource(LocalVertexFactory.GetOwnedPointer());
	}

	// Decals
	for( INT FactoryIdx=0; FactoryIdx < DecalVertexFactories.Num(); FactoryIdx++ )
	{
		FGPUSkinDecalVertexFactory& DecalVertexFactory = DecalVertexFactories(FactoryIdx);
		BeginReleaseResource(&DecalVertexFactory);
	}
	if (LocalDecalVertexFactory)
	{
		BeginReleaseResource(LocalDecalVertexFactory.GetOwnedPointer());
	}
	// End Decals

	// Release the shadow vertex buffer and its factory.
	BeginReleaseResource(&ShadowVertexBuffer);
}

/** 
* Init rendering resources for the morph stream of this LOD
*/
void FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectLOD::InitMorphResources()
{
	check(SkelMesh);
	check(SkelMesh->LODModels.IsValidIndex(LODIndex));

	// vertex buffer for each lod has already been created when skelmesh was loaded
	FStaticLODModel& LODModel = SkelMesh->LODModels(LODIndex);

	// init the delta vertex buffer for this LOD
	BeginInitResource(&MorphVertexBuffer);

	// one for each chunk
	MorphVertexFactories.Empty(LODModel.Chunks.Num());
	for( INT FactoryIdx=0; FactoryIdx < LODModel.Chunks.Num(); FactoryIdx++ )
	{
		FGPUSkinMorphVertexFactory* MorphVertexFactory = new(MorphVertexFactories) FGPUSkinMorphVertexFactory();

		// update vertex factory components and sync it
		TSetResourceDataContext<FGPUSkinMorphVertexFactory> MorphVertexFactoryData(MorphVertexFactory);
		// position
		MorphVertexFactoryData->PositionComponent = FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,Position),sizeof(FSoftSkinVertex),VET_Float3);
		// tangents
		MorphVertexFactoryData->TangentBasisComponents[0] = FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentX),sizeof(FSoftSkinVertex),VET_PackedNormal);
		MorphVertexFactoryData->TangentBasisComponents[1] = FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentY),sizeof(FSoftSkinVertex),VET_PackedNormal);
		MorphVertexFactoryData->TangentBasisComponents[2] = FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentZ),sizeof(FSoftSkinVertex),VET_PackedNormal);
		// uvs
		MorphVertexFactoryData->TextureCoordinates.AddItem(FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,U),sizeof(FSoftSkinVertex),VET_Float2));
		// bone indices
		MorphVertexFactoryData->BoneIndices = FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,InfluenceBones),sizeof(FSoftSkinVertex),VET_UByte4);
		// bone weights
		MorphVertexFactoryData->BoneWeights = FVertexStreamComponent(
			&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,InfluenceWeights),sizeof(FSoftSkinVertex),VET_UByte4N);

		// delta positions
		MorphVertexFactoryData->DeltaPositionComponent = FVertexStreamComponent(
			&MorphVertexBuffer,STRUCT_OFFSET(FMorphGPUSkinVertex,DeltaPosition),sizeof(FMorphGPUSkinVertex),VET_Float3);
		// delta normals
		MorphVertexFactoryData->DeltaTangentZComponent = FVertexStreamComponent(
			&MorphVertexBuffer,STRUCT_OFFSET(FMorphGPUSkinVertex,DeltaTangentZ),sizeof(FMorphGPUSkinVertex),VET_PackedNormal);

		// copy it
		MorphVertexFactoryData.Commit();
		// init rendering resource	
		BeginInitResource(MorphVertexFactory);
	}

	// Decals
	if ( bDecalFactoriesEnabled )
	{
		MorphDecalVertexFactories.Empty(LODModel.Chunks.Num());
		for( INT FactoryIdx=0; FactoryIdx < LODModel.Chunks.Num(); FactoryIdx++ )
		{
			FGPUSkinMorphDecalVertexFactory* MorphDecalVertexFactory = new(MorphDecalVertexFactories) FGPUSkinMorphDecalVertexFactory();

			// update vertex factory components and sync it
			TSetResourceDataContext<FGPUSkinMorphDecalVertexFactory> MorphDecalVertexFactoryData(MorphDecalVertexFactory);
			// position
			MorphDecalVertexFactoryData->PositionComponent = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,Position),sizeof(FSoftSkinVertex),VET_Float3);
			// tangents
			MorphDecalVertexFactoryData->TangentBasisComponents[0] = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentX),sizeof(FSoftSkinVertex),VET_PackedNormal);
			MorphDecalVertexFactoryData->TangentBasisComponents[1] = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentY),sizeof(FSoftSkinVertex),VET_PackedNormal);
			MorphDecalVertexFactoryData->TangentBasisComponents[2] = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,TangentZ),sizeof(FSoftSkinVertex),VET_PackedNormal);
			// uvs
			MorphDecalVertexFactoryData->TextureCoordinates.AddItem(FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,U),sizeof(FSoftSkinVertex),VET_Float2));
			// bone indices
			MorphDecalVertexFactoryData->BoneIndices = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,InfluenceBones),sizeof(FSoftSkinVertex),VET_UByte4);
			// bone weights
			MorphDecalVertexFactoryData->BoneWeights = FVertexStreamComponent(
				&LODModel.VertexBufferGPUSkin,STRUCT_OFFSET(FSoftSkinVertex,InfluenceWeights),sizeof(FSoftSkinVertex),VET_UByte4N);

			// delta positions
			MorphDecalVertexFactoryData->DeltaPositionComponent = FVertexStreamComponent(
				&MorphVertexBuffer,STRUCT_OFFSET(FMorphGPUSkinVertex,DeltaPosition),sizeof(FMorphGPUSkinVertex),VET_Float3);
			// delta normals
			MorphDecalVertexFactoryData->DeltaTangentZComponent = FVertexStreamComponent(
				&MorphVertexBuffer,STRUCT_OFFSET(FMorphGPUSkinVertex,DeltaTangentZ),sizeof(FMorphGPUSkinVertex),VET_PackedNormal);

			// copy it
			MorphDecalVertexFactoryData.Commit();
			// init rendering resource	
			BeginInitResource(MorphDecalVertexFactory);
		}
	}
	// End Decals
}

/** 
* Release rendering resources for the morph stream of this LOD
*/
void FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectLOD::ReleaseMorphResources()
{
	for( INT FactoryIdx=0; FactoryIdx < MorphVertexFactories.Num(); FactoryIdx++ )
	{
		FGPUSkinMorphVertexFactory& MorphVertexFactory = MorphVertexFactories(FactoryIdx);
		BeginReleaseResource(&MorphVertexFactory);
	}

	// Decals
	for( INT FactoryIdx=0; FactoryIdx < MorphDecalVertexFactories.Num(); FactoryIdx++ )
	{
		FGPUSkinMorphDecalVertexFactory& MorphDecalVertexFactory = MorphDecalVertexFactories(FactoryIdx);
		BeginReleaseResource(&MorphDecalVertexFactory);
	}
	// End Decals

	// release the delta vertex buffer
	BeginReleaseResource(&MorphVertexBuffer);
}

/** 
 * Update the contents of the vertex buffer with new data
 * @param	NewVertices - array of new vertex data
 * @param	NumVertices - Number of vertices
 */
void FSkeletalMeshObjectGPUSkin::FSkeletalMeshObjectLOD::UpdateShadowVertexBuffer( const FVector* NewVertices, UINT NumVertices ) const
{
	ShadowVertexBuffer.UpdateVertices( NewVertices, NumVertices, sizeof(FVector) );
}

/**
 * Compute the distance of a light from the triangle planes.
 */
void FSkeletalMeshObjectGPUSkin::GetPlaneDots(FLOAT* OutPlaneDots,const FVector4& LightPosition,INT LODIndex) const
{
	const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
#if XBOX
	// Use VMX optimized version to calculate the plane dots
	GetPlaneDotsXbox(LODModel.ShadowIndices.Num() / 3,LightPosition,
		(FVector*)CachedVertices.GetData(),
		(WORD*)LODModel.ShadowIndices.GetData(),OutPlaneDots);
#else
	for(INT TriangleIndex = 0;TriangleIndex < LODModel.ShadowIndices.Num() / 3;TriangleIndex++)
	{
		const FVector& V1 = CachedVertices(LODModel.ShadowIndices(TriangleIndex * 3 + 0));
		const FVector& V2 = CachedVertices(LODModel.ShadowIndices(TriangleIndex * 3 + 1));
		const FVector& V3 = CachedVertices(LODModel.ShadowIndices(TriangleIndex * 3 + 2));
		*OutPlaneDots++ = ((V2-V3) ^ (V1-V3)) | (FVector(LightPosition) - V1 * LightPosition.W);
	}
#endif
}

/**
 * Re-skin cached vertices for an LOD and update the vertex buffer. Note that this
 * function is called from the render thread!
 * @param	LODIndex - index to LODs
 * @param	bForce - force update even if LOD index hasn't changed
 * @param	bUpdateShadowVertices - whether to update the shadow volumes vertices
 * @param	bUpdateDecalVertices - whether to update the decal vertices
 */
void FSkeletalMeshObjectGPUSkin::CacheVertices(INT LODIndex, UBOOL bForce, UBOOL bUpdateShadowVertices, UBOOL bUpdateDecalVertices) const
{
	// Get the destination mesh LOD.
	const FSkeletalMeshObjectLOD& MeshLOD = LODs(LODIndex);

	// source skel mesh and static lod model
	FStaticLODModel& LOD = SkeletalMesh->LODModels(LODIndex);

	// only recache if lod changed and we need it for a shadow volume
	if ( (LODIndex != CachedShadowLOD || bForce) && 
		bUpdateShadowVertices &&
		DynamicData && 
		IsValidRef(MeshLOD.ShadowVertexBuffer.VertexBufferRHI) )
	{
		// bone matrices
		const TArray<FMatrix>& ReferenceToLocal = DynamicData->ReferenceToLocal;

		// final cached verts
		CachedVertices.Empty(LOD.NumVertices);
		CachedVertices.Add(LOD.NumVertices);
		FVector* DestVertex = &CachedVertices(0);

		// Do the actual skinning
		//@todo: skin straight to the shadow vertex buffer?
		SkinVertices( DestVertex, ReferenceToLocal, LOD );

		// copy to the shadow vertex buffer
		check(LOD.NumVertices == CachedVertices.Num());
		MeshLOD.UpdateShadowVertexBuffer( &CachedVertices(0), LOD.NumVertices );

		CachedShadowLOD = LODIndex;
	}
}

static const VectorRegister		VECTOR_INV_255			= { 1.f/255.f, 1.f/255.f, 1.f/255.f, 1.f/255.f };

/** optimized skinning */
FORCEINLINE void FSkeletalMeshObjectGPUSkin::SkinVertices( FVector* DestVertex, const TArray<FMatrix>& ReferenceToLocal, FStaticLODModel& LOD ) const
{
	//@todo sz - handle morph targets when updating vertices for shadow volumes

	DWORD StatusRegister = VectorGetControlRegister();
	VectorSetControlRegister( StatusRegister | VECTOR_ROUND_TOWARD_ZERO );

	// For each chunk in the LOD
	for (INT ChunkIndex = 0; ChunkIndex < LOD.Chunks.Num(); ChunkIndex++)
	{
		const FSkelMeshChunk& Chunk = LOD.Chunks(ChunkIndex);
		// For reading the verts
		FSoftSkinVertex* RESTRICT SrcRigidVertex = &LOD.VertexBufferGPUSkin.Vertices(Chunk.GetRigidVertexBufferIndex());
		// For each rigid vert transform from reference pose to local space
		for (INT VertexIndex = 0; VertexIndex < Chunk.GetNumRigidVertices(); VertexIndex++)
		{
			const FMatrix& RefToLocal = ReferenceToLocal(Chunk.BoneMap(SrcRigidVertex->InfluenceBones[RigidBoneIdx]));
			// Load VMX registers
			VectorRegister Position = VectorLoadFloat3( &SrcRigidVertex->Position );
			VectorRegister M3 = VectorLoadAligned( &RefToLocal.M[3][0] );
			VectorRegister M2 = VectorLoadAligned( &RefToLocal.M[2][0] );
			VectorRegister M1 = VectorLoadAligned( &RefToLocal.M[1][0] );
			VectorRegister M0 = VectorLoadAligned( &RefToLocal.M[0][0] );
			// Move to next source
			SrcRigidVertex++;
			// Splat position (transformed position starts as Z splat)
			VectorRegister TransformedPosition = VectorReplicate(Position, 2);
			VectorRegister SplatY = VectorReplicate(Position, 1);
			VectorRegister SplatX = VectorReplicate(Position, 0);
			// Now transform
			TransformedPosition = VectorMultiplyAdd(M2,TransformedPosition,M3);
			TransformedPosition = VectorMultiplyAdd(SplatY,M1,TransformedPosition);
			TransformedPosition = VectorMultiplyAdd(SplatX,M0,TransformedPosition);
			// Write out 3 part vector
			VectorStoreFloat3( TransformedPosition, DestVertex );
			// Move to next dest
			DestVertex++;
		}
		// Source vertices for transformation
		FSoftSkinVertex* RESTRICT SrcSoftVertex = (Chunk.GetNumSoftVertices() > 0) ? &LOD.VertexBufferGPUSkin.Vertices(Chunk.GetSoftVertexBufferIndex()) : NULL;
		// Transform each skinned vert
		for (INT VertexIndex = 0; VertexIndex < Chunk.GetNumSoftVertices(); VertexIndex++)
		{
			// Load VMX registers
			VectorRegister Position = VectorLoadFloat3( &SrcSoftVertex->Position );
			VectorRegister WeightVector = VectorMultiply( VectorLoadByte4(SrcSoftVertex->InfluenceWeights), VECTOR_INV_255 );
			VectorResetFloatRegisters();	// Must call this after VectorLoadByte4 in order to use scalar floats again.

			// Zero the out going position
			VectorRegister TransformedPosition = VectorZero();
			// Influence 0
			{
				// Get the matrix for this influence
				const FMatrix& RefToLocal = ReferenceToLocal(Chunk.BoneMap(SrcSoftVertex->InfluenceBones[0]));
				// Load the matrix
				VectorRegister M3 = VectorLoadAligned(&RefToLocal.M[3][0]);
				VectorRegister M2 = VectorLoadAligned(&RefToLocal.M[2][0]);
				VectorRegister M1 = VectorLoadAligned(&RefToLocal.M[1][0]);
				VectorRegister M0 = VectorLoadAligned(&RefToLocal.M[0][0]);
				// Splat position (influenced position starts as Z splat)
				VectorRegister InflPosition = VectorReplicate(Position, 2);
				VectorRegister SplatY = VectorReplicate(Position, 1);
				VectorRegister SplatX = VectorReplicate(Position, 0);
				// Splat the weight
				VectorRegister Weight = VectorReplicate(WeightVector, 0);
				// Now transform
				InflPosition = VectorMultiplyAdd(M2,InflPosition,M3);
				InflPosition = VectorMultiplyAdd(SplatY,M1,InflPosition);
				InflPosition = VectorMultiplyAdd(SplatX,M0,InflPosition);
				// Multipy by the weight and accumulate the results
				TransformedPosition = VectorMultiplyAdd(Weight,InflPosition,TransformedPosition);
			}
			// Influence 1
			{
				// Get the matrix for this influence
				const FMatrix& RefToLocal = ReferenceToLocal(Chunk.BoneMap(SrcSoftVertex->InfluenceBones[1]));
				// Load the matrix
				VectorRegister M3 = VectorLoadAligned(&RefToLocal.M[3][0]);
				VectorRegister M2 = VectorLoadAligned(&RefToLocal.M[2][0]);
				VectorRegister M1 = VectorLoadAligned(&RefToLocal.M[1][0]);
				VectorRegister M0 = VectorLoadAligned(&RefToLocal.M[0][0]);
				// Splat position (influenced position starts as Z splat)
				VectorRegister InflPosition = VectorReplicate(Position, 2);
				VectorRegister SplatY = VectorReplicate(Position, 1);
				VectorRegister SplatX = VectorReplicate(Position, 0);
				// Splat the weight
				VectorRegister Weight = VectorReplicate(WeightVector, 1);
				// Now transform
				InflPosition = VectorMultiplyAdd(M2,InflPosition,M3);
				InflPosition = VectorMultiplyAdd(SplatY,M1,InflPosition);
				InflPosition = VectorMultiplyAdd(SplatX,M0,InflPosition);
				// Multipy by the weight and accumulate the results
				TransformedPosition = VectorMultiplyAdd(Weight,InflPosition,TransformedPosition);
			}
			// Influence 2
			{
				// Get the matrix for this influence
				const FMatrix& RefToLocal = ReferenceToLocal(Chunk.BoneMap(SrcSoftVertex->InfluenceBones[2]));
				// Load the matrix
				VectorRegister M3 = VectorLoadAligned(&RefToLocal.M[3][0]);
				VectorRegister M2 = VectorLoadAligned(&RefToLocal.M[2][0]);
				VectorRegister M1 = VectorLoadAligned(&RefToLocal.M[1][0]);
				VectorRegister M0 = VectorLoadAligned(&RefToLocal.M[0][0]);
				// Splat position (influenced position starts as Z splat)
				VectorRegister InflPosition = VectorReplicate(Position, 2);
				VectorRegister SplatY = VectorReplicate(Position, 1);
				VectorRegister SplatX = VectorReplicate(Position, 0);
				// Splat the weight
				VectorRegister Weight = VectorReplicate(WeightVector, 2);
				// Now transform
				InflPosition = VectorMultiplyAdd(M2,InflPosition,M3);
				InflPosition = VectorMultiplyAdd(SplatY,M1,InflPosition);
				InflPosition = VectorMultiplyAdd(SplatX,M0,InflPosition);
				// Multipy by the weight and accumulate the results
				TransformedPosition = VectorMultiplyAdd(Weight,InflPosition,TransformedPosition);
			}
			// Influence 3
			{
				// Get the matrix for this influence
				const FMatrix& RefToLocal = ReferenceToLocal(Chunk.BoneMap(SrcSoftVertex->InfluenceBones[3]));
				// Load the matrix
				VectorRegister M3 = VectorLoadAligned(&RefToLocal.M[3][0]);
				VectorRegister M2 = VectorLoadAligned(&RefToLocal.M[2][0]);
				VectorRegister M1 = VectorLoadAligned(&RefToLocal.M[1][0]);
				VectorRegister M0 = VectorLoadAligned(&RefToLocal.M[0][0]);
				// Splat position (influenced position starts as Z splat)
				VectorRegister InflPosition = VectorReplicate(Position, 2);
				VectorRegister SplatY = VectorReplicate(Position, 1);
				VectorRegister SplatX = VectorReplicate(Position, 0);
				// Splat the weight
				VectorRegister Weight = VectorReplicate(WeightVector, 3);
				// Now transform
				InflPosition = VectorMultiplyAdd(M2,InflPosition,M3);
				InflPosition = VectorMultiplyAdd(SplatY,M1,InflPosition);
				InflPosition = VectorMultiplyAdd(SplatX,M0,InflPosition);
				// Multipy by the weight and accumulate the results
				TransformedPosition = VectorMultiplyAdd(Weight,InflPosition,TransformedPosition);
			}
			// Move to next source vert
			SrcSoftVertex++;
			// Write out 3 part vector
			VectorStoreFloat3( TransformedPosition, DestVertex );
			// Move to next dest
			DestVertex++;
		}
	}
	VectorSetControlRegister( StatusRegister );
}

/** 
 *	Get the array of component-space bone transforms. 
 *	Not safe to hold this point between frames, because it exists in dynamic data passed from main thread.
 */
TArray<FMatrix>* FSkeletalMeshObjectGPUSkin::GetSpaceBases() const
{
	if(DynamicData)
	{
		return &(DynamicData->MeshSpaceBases);
	}
	else
	{
		return NULL;
	}
}

/**
 * Allows derived types to transform decal state into a space that's appropriate for the given skinning algorithm.
 */
void FSkeletalMeshObjectGPUSkin::TransformDecalState(const FDecalState& DecalState,
													 FMatrix& OutDecalMatrix,
													 FVector& OutDecalLocation,
													 FVector2D& OutDecalOffset)
{
	// The decal is already in the 'reference pose' space; just pass values along.
	OutDecalMatrix = DecalState.WorldTexCoordMtx;
	OutDecalLocation = DecalState.HitLocation;
	OutDecalOffset = FVector2D(DecalState.OffsetX, DecalState.OffsetY);
}

/*-----------------------------------------------------------------------------
FDynamicSkelMeshObjectDataGPUSkin
-----------------------------------------------------------------------------*/

/**
* Constructor
* Updates the ReferenceToLocal matrices using the new dynamic data.
* @param	InSkelMeshComponent - parent skel mesh component
* @param	InLODIndex - each lod has its own bone map 
* @param	InActiveMorphs - morph targets active for the mesh
*/
FDynamicSkelMeshObjectDataGPUSkin::FDynamicSkelMeshObjectDataGPUSkin(
	USkeletalMeshComponent* InSkelMeshComponent,
	INT InLODIndex,
	const TArray<FActiveMorph>& InActiveMorphs
	)
	:	LODIndex(InLODIndex)
	,	ActiveMorphs(InActiveMorphs)
	,	NumWeightedActiveMorphs(0)
{
	UpdateRefToLocalMatrices( ReferenceToLocal, InSkelMeshComponent, LODIndex );
	MeshSpaceBases = InSkelMeshComponent->SpaceBases;

	// find number of morphs that are currently weighted and will affect the mesh
	for( INT MorphIdx=0; MorphIdx < ActiveMorphs.Num(); MorphIdx++ )
	{
		const FActiveMorph& Morph = ActiveMorphs(MorphIdx);
		if( Morph.Weight >= MinMorphBlendWeight &&
			Morph.Weight <= MaxMorphBlendWeight &&
			Morph.Target &&
			Morph.Target->MorphLODModels.IsValidIndex(LODIndex) &&
			Morph.Target->MorphLODModels(LODIndex).Vertices.Num() )
		{
			NumWeightedActiveMorphs++;
		}
	}
}

/**
* Compare the given set of active morph targets with the current list to check if different
* @param CompareActiveMorphs - array of morph targets to compare
* @return TRUE if boths sets of active morph targets are equal
*/
UBOOL FDynamicSkelMeshObjectDataGPUSkin::ActiveMorphTargetsEqual( const TArray<FActiveMorph>& CompareActiveMorphs )
{
	UBOOL Result=TRUE;
	if( CompareActiveMorphs.Num() == ActiveMorphs.Num() )
	{
		for( INT MorphIdx=0; MorphIdx < ActiveMorphs.Num(); MorphIdx++ )
		{
			const FActiveMorph& Morph = ActiveMorphs(MorphIdx);
			const FActiveMorph& CompMorph = CompareActiveMorphs(MorphIdx);
			const FLOAT MorphWeightThreshold = 0.001f;

			if( Morph.Target != CompMorph.Target ||
				Abs(Morph.Weight - CompMorph.Weight) >= MorphWeightThreshold )
			{
				Result=FALSE;
				break;
			}
		}
	}
	else
	{
		Result = FALSE;
	}
	return Result;
}

