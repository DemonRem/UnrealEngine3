/*=============================================================================
	UnSkeletalRenderCPUSkin.cpp: CPU skinned skeletal mesh rendering code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "EnginePhysicsClasses.h"
#include "UnSkeletalRenderCPUSkin.h"
#include "UnNovodexSupport.h"

#if XBOX
	#include "UnSkeletalRenderCPUXe.h"
	#define USING_COOKED_DATA 1
#elif PS3
	#define USING_COOKED_DATA 1
#else
	#define USING_COOKED_DATA 0
#endif


#if USING_COOKED_DATA
	// BYTE[] elements in LOD.VertexBufferGPUSkin have been swapped for VET_UBYTE4 vertex stream use
	static const INT RigidBoneIdx = 3;
	#define INFLUENCE_0		3
	#define INFLUENCE_1		2
	#define INFLUENCE_2		1
	#define INFLUENCE_3		0
#else
	static const INT RigidBoneIdx = 0;
	#define INFLUENCE_0		0
	#define INFLUENCE_1		1
	#define INFLUENCE_2		2
	#define INFLUENCE_3		3
#endif

/** optimized skinning */
static FORCEINLINE void SkinVertices( FFinalSkinVertex* DestVertex, FMatrix* ReferenceToLocal, INT LODIndex, FStaticLODModel& LOD, TArray<FActiveMorph>& ActiveMorphs );

/*-----------------------------------------------------------------------------
	FFinalSkinVertexBuffer
-----------------------------------------------------------------------------*/

/** 
 * Initialize the dynamic RHI for this rendering resource 
 */
void FFinalSkinVertexBuffer::InitDynamicRHI()
{
	// all the vertex data for a single LOD of the skel mesh
	FStaticLODModel& LodModel = SkelMesh->LODModels(LODIdx);

	// Create the buffer rendering resource
	
	UINT Size;

	if(SkelMesh->bEnableClothTearing && (SkelMesh->ClothWeldingMap.Num() == 0))
	{
		/*
		We need to reserve extra vertices at the end of the buffer to accomadate vertices generated
		due to cloth tearing.
		*/
		Size = (LodModel.NumVertices + SkelMesh->ClothTearReserve) * sizeof(FFinalSkinVertex);
	}
	else
	{
		Size = LodModel.NumVertices * sizeof(FFinalSkinVertex);
	}

	VertexBufferRHI = RHICreateVertexBuffer(Size,NULL,TRUE);

	// Lock the buffer.
	void* Buffer = RHILockVertexBuffer(VertexBufferRHI,0,Size);

	// Initialize the vertex data
	// All chunks are combined into one (rigid first, soft next)
	check(LodModel.VertexBufferGPUSkin.Vertices.Num() == LodModel.NumVertices);
	FSoftSkinVertex* SrcVertex = (FSoftSkinVertex*)LodModel.VertexBufferGPUSkin.Vertices.GetData();
	FFinalSkinVertex* DestVertex = (FFinalSkinVertex*)Buffer;
	for( UINT VertexIdx=0; VertexIdx < LodModel.NumVertices; VertexIdx++ )
	{
		DestVertex->Position = SrcVertex->Position;
		DestVertex->TangentX = SrcVertex->TangentX;
		// w component of TangentZ should already have sign of the tangent basis determinant
		DestVertex->TangentZ = SrcVertex->TangentZ;

		DestVertex->U = SrcVertex->U;
		DestVertex->V = SrcVertex->V;

		SrcVertex++;
		DestVertex++;
	}

	// Unlock the buffer.
	RHIUnlockVertexBuffer(VertexBufferRHI);
}

void FFinalSkinVertexBuffer::ReleaseDynamicRHI()
{
	VertexBufferRHI.Release();
}

/*-----------------------------------------------------------------------------
	FFinalDynamicIndexBuffer
-----------------------------------------------------------------------------*/

/** 
 * Initialize the dynamic RHI for this rendering resource 
 */
void FFinalDynamicIndexBuffer::InitDynamicRHI()
{
	// all the vertex data for a single LOD of the skel mesh
	FStaticLODModel& LodModel = SkelMesh->LODModels(LODIdx);

	// Create the buffer rendering resource
	
	UINT Size = LodModel.IndexBuffer.Indices.Num();
	
	if(!SkelMesh->bEnableClothTearing || Size == 0 || (SkelMesh->ClothWeldingMap.Num() != 0))
	{
		return;
	}

	IndexBufferRHI = RHICreateIndexBuffer(sizeof(WORD), Size * sizeof(WORD), NULL, TRUE);

	// Lock the buffer.
	void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Size * sizeof(WORD));
	WORD* BufferWords = (WORD *)Buffer;

	// Initialize the index data
	// Copy the index data from the LodModel.

	for(UINT i=0; i<Size; i++)
	{
		BufferWords[i] = LodModel.IndexBuffer.Indices(i);
	}

	// Unlock the buffer.
	RHIUnlockIndexBuffer(IndexBufferRHI);
}

void FFinalDynamicIndexBuffer::ReleaseDynamicRHI()
{
	IndexBufferRHI.Release();
}

/*-----------------------------------------------------------------------------
	FSkeletalMeshObjectCPUSkin
-----------------------------------------------------------------------------*/

/** 
 * Constructor 
 * @param	InSkeletalMeshComponent - skeletal mesh primitive we want to render 
 */
FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectCPUSkin(USkeletalMeshComponent* InSkeletalMeshComponent) 
:	FSkeletalMeshObject(InSkeletalMeshComponent)
,	DynamicData(NULL)
,	CachedVertexLOD(INDEX_NONE)
,	CachedShadowLOD(INDEX_NONE)
{
	// create LODs to match the base mesh
	for( INT LODIndex=0;LODIndex < SkeletalMesh->LODModels.Num();LODIndex++ )
	{
		new(LODs) FSkeletalMeshObjectLOD(SkeletalMesh,LODIndex);
	}

	InitResources();
}

/** 
 * Destructor 
 */
FSkeletalMeshObjectCPUSkin::~FSkeletalMeshObjectCPUSkin()
{
	delete DynamicData;
}

/** 
 * Initialize rendering resources for each LOD. 
 * Blocks until init completes on rendering thread
 */
void FSkeletalMeshObjectCPUSkin::InitResources()
{
	for( INT LODIndex=0;LODIndex < LODs.Num();LODIndex++ )
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs(LODIndex);
		SkelLOD.InitResources();
	}
}

/** 
 * Release rendering resources for each LOD.
 * Blocks until release completes on rendering thread
 */
void FSkeletalMeshObjectCPUSkin::ReleaseResources()
{
	for( INT LODIndex=0;LODIndex < LODs.Num();LODIndex++ )
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs(LODIndex);
		SkelLOD.ReleaseResources();
	}
}

/**
* Called by the game thread for any dynamic data updates for this skel mesh object
* @param	LODIndex - lod level to update
* @param	InSkeletalMeshComponen - parent prim component doing the updating
* @param	ActiveMorphs - morph targets to blend with during skinning
*/
void FSkeletalMeshObjectCPUSkin::Update(INT LODIndex,USkeletalMeshComponent* InSkeletalMeshComponent,const TArray<FActiveMorph>& ActiveMorphs)
{
	// create the new dynamic data for use by the rendering thread
	// this data is only deleted when another update is sent
	FDynamicSkelMeshObjectData* NewDynamicData = new FDynamicSkelMeshObjectDataCPUSkin(InSkeletalMeshComponent,LODIndex,ActiveMorphs);
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
void FSkeletalMeshObjectCPUSkin::UpdateDynamicData_RenderThread(FDynamicSkelMeshObjectData* InDynamicData)
{
	// we should be done with the old data at this point
	delete DynamicData;
	// update with new data
	DynamicData = (FDynamicSkelMeshObjectDataCPUSkin*)InDynamicData;	
	check(DynamicData);

	// update vertices using the new data
	CacheVertices(DynamicData->LODIndex,TRUE,FALSE,TRUE);
}

/**
 * Adds a decal interaction to the primitive.  This is called in the rendering thread by the skeletal mesh proxy's AddDecalInteraction_RenderingThread.
 */
void FSkeletalMeshObjectCPUSkin::AddDecalInteraction_RenderingThread(const FDecalInteraction& DecalInteraction)
{
	for( INT LODIndex = 0 ; LODIndex < LODs.Num() ; ++LODIndex )
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs(LODIndex);
		SkelLOD.AddDecalInteraction_RenderingThread( DecalInteraction );
	}
}

/**
 * Removes a decal interaction from the primitive.  This is called in the rendering thread by the skeletal mesh proxy's RemoveDecalInteraction_RenderingThread.
 */
void FSkeletalMeshObjectCPUSkin::RemoveDecalInteraction_RenderingThread(UDecalComponent* DecalComponent)
{
	for( INT LODIndex = 0 ; LODIndex < LODs.Num() ; ++LODIndex )
	{
		FSkeletalMeshObjectLOD& SkelLOD = LODs(LODIndex);
		SkelLOD.RemoveDecalInteraction_RenderingThread( DecalComponent );
	}
}

static UBOOL ComputeTangent(FVector &t,
							const FVector &p0, const FVector2D &c0,
							const FVector &p1, const FVector2D &c1,
							const FVector &p2, const FVector2D &c2)
{
  const FLOAT epsilon = 0.0001f;
  UBOOL   Ret = FALSE;
  FVector dp1 = p1 - p0;
  FVector dp2 = p2 - p0;
  FLOAT   du1 = c1.X - c0.X;
  FLOAT   dv1 = c1.Y - c0.Y;
  if(Abs(dv1) < epsilon && Abs(du1) >= epsilon)
  {
	t = dp1 / du1;
	Ret = TRUE;
  }
  else
  {
	  FLOAT du2 = c2.X - c0.X;
	  FLOAT dv2 = c2.Y - c0.Y;
	  FLOAT det = dv1*du2 - dv2*du1;
	  if(Abs(det) >= epsilon)
	  {
		t = (dp2*dv1-dp1*dv2)/det;
		Ret = TRUE;
	  }
  }
  return Ret;
}

static inline INT ClothIdxToGraphics(INT ClothIndex, const TArray<INT>& ClothToGraphicsVertMap, INT LODNumVertices)
{
	if(ClothIndex < ClothToGraphicsVertMap.Num() )
	{
		return ClothToGraphicsVertMap(ClothIndex);
	}
	else
	{
		return (ClothIndex - ClothToGraphicsVertMap.Num()) + LODNumVertices;
	}
}

/**
 * Re-skin cached vertices for an LOD and update the vertex buffer. Note that this
 * function is called from the render thread!
 * @param	LODIndex - index to LODs
 * @param	bForce - force update even if LOD index hasn't changed
 * @param	bUpdateShadowVertices - whether to update the shadow volumes vertices
 * @param	bUpdateDecalVertices - whether to update the decal vertices
 */
void FSkeletalMeshObjectCPUSkin::CacheVertices(INT LODIndex, UBOOL bForce, UBOOL bUpdateShadowVertices, UBOOL bUpdateDecalVertices) const
{
	// Source skel mesh and static lod model
	FStaticLODModel& LOD = SkeletalMesh->LODModels(LODIndex);

	// Get the destination mesh LOD.
	const FSkeletalMeshObjectLOD& MeshLOD = LODs(LODIndex);

	// only recache if lod changed
	if ( (LODIndex != CachedVertexLOD || bForce) &&
		DynamicData && 
		IsValidRef(MeshLOD.VertexBuffer.VertexBufferRHI) )
	{
		// bone matrices
		FMatrix* ReferenceToLocal = &DynamicData->ReferenceToLocal(0);

		INT CachedFinalVerticesNum = LOD.NumVertices;
		if(SkeletalMesh->bEnableClothTearing && (SkeletalMesh->ClothWeldingMap.Num() == 0))
		{
			CachedFinalVerticesNum += SkeletalMesh->ClothTearReserve;
		}

		CachedFinalVertices.Empty(CachedFinalVerticesNum);
		CachedFinalVertices.Add(CachedFinalVerticesNum);

		// final cached verts
		FFinalSkinVertex* DestVertex = &CachedFinalVertices(0);

		{
			SCOPE_CYCLE_COUNTER(STAT_SkinningTime);
			// do actual skinning
			SkinVertices( DestVertex, ReferenceToLocal, DynamicData->LODIndex, LOD, DynamicData->ActiveMorphs );
		}

#if !NX_DISABLE_CLOTH
	
		// Update graphics verts from physics info if we are running cloth
		if( DynamicData->ClothPosData.Num() > 0 && LODIndex == 0 && DynamicData->ClothBlendWeight > 0.f)
		{
			SCOPE_CYCLE_COUNTER(STAT_UpdateClothVertsTime);

			check(SkeletalMesh->NumFreeClothVerts <= SkeletalMesh->ClothToGraphicsVertMap.Num());
			check(DynamicData->ClothPosData.Num() == DynamicData->ClothNormalData.Num());
			//check(SkeletalMesh->NumFreeClothVerts <= DynamicData->ClothMeshVerts.Num());

			TArray<INT>     &ClothIndexData   = DynamicData->ClothIndexData;
			TArray<FVector> &ClothTangentData = CachedClothTangents;

			INT NumVertices;
			INT NumIndices = DynamicData->ClothIndexData.Num();
	
			if(SkeletalMesh->bEnableClothTearing && (SkeletalMesh->ClothWeldingMap.Num() == 0))
			{
				NumVertices = DynamicData->ActualClothPosDataNum;
			}
			else
			{
				NumVertices = DynamicData->ClothPosData.Num();
			}

			ClothTangentData.Reset();
			ClothTangentData.AddZeroed(NumVertices);

			for(INT i=0; i<NumIndices; i+=3)
			{
				INT A  = DynamicData->ClothIndexData(i+0);
				INT B  = DynamicData->ClothIndexData(i+1);
				INT C  = DynamicData->ClothIndexData(i+2);
				const FVector  &APos = DynamicData->ClothPosData(A);
				const FVector  &BPos = DynamicData->ClothPosData(B);
				const FVector  &CPos = DynamicData->ClothPosData(C);
				const FVector2D AUV(DestVertex[A].U, DestVertex[A].V);
				const FVector2D BUV(DestVertex[B].U, DestVertex[B].V);
				const FVector2D CUV(DestVertex[C].U, DestVertex[C].V);
				FVector Tangent;
				if(ComputeTangent(Tangent, APos, AUV, BPos, BUV, CPos, CUV))
				{
					ClothTangentData(A) += Tangent;
					ClothTangentData(B) += Tangent;
					ClothTangentData(C) += Tangent;
				}
			}
			
			INT NumSkinnedClothVerts = SkeletalMesh->NumFreeClothVerts;
			if(SkeletalMesh->bEnableClothTearing)
			{
				NumSkinnedClothVerts = NumVertices;
			}

			for(INT i=0; i<NumSkinnedClothVerts; i++)
			{
				// Find the index of the graphics vertex that corresponds to this cloth vertex
				INT GraphicsIndex = ClothIdxToGraphics(i, SkeletalMesh->ClothToGraphicsVertMap, LOD.NumVertices);			
				check((GraphicsIndex >= 0) && (GraphicsIndex < CachedFinalVerticesNum));

				//Is this a new vertex created due to tearing...
				if(i >= SkeletalMesh->ClothToGraphicsVertMap.Num())
				{
					//Copy across texture coords for reserve vertices...
					//We track back over parents until we find the original.

					INT OrigIndex = DynamicData->ClothParentIndices(i);
					while(OrigIndex >= SkeletalMesh->ClothToGraphicsVertMap.Num())
					{
						OrigIndex = DynamicData->ClothParentIndices(OrigIndex);
					}

					INT OrigGraphicsIndex = SkeletalMesh->ClothToGraphicsVertMap(OrigIndex);

					DestVertex[GraphicsIndex].U = DestVertex[OrigGraphicsIndex].U;
					DestVertex[GraphicsIndex].V = DestVertex[OrigGraphicsIndex].V;
				}

				
				check(GraphicsIndex < CachedFinalVertices.Num());

				// Transform into local space
				FVector LocalClothPos = DynamicData->WorldToLocal.TransformFVector( P2UScale * DynamicData->ClothPosData(i) );
				
				// Blend between cloth and skinning
				DestVertex[GraphicsIndex].Position = Lerp<FVector>(DestVertex[GraphicsIndex].Position, LocalClothPos, DynamicData->ClothBlendWeight);

				// Transform normal.
				FVector TangentZ  = DynamicData->WorldToLocal.TransformNormal( DynamicData->ClothNormalData(i) );
				DestVertex[GraphicsIndex].TangentZ = TangentZ;
				
				// Generate contiguous tangent vectors. Slower but works without artifacts.
				FVector TangentX = DynamicData->WorldToLocal.TransformNormal(ClothTangentData(i)).SafeNormal();
				FVector TangentY = TangentZ ^ TangentX;
				TangentX = TangentY ^ TangentZ;
				DestVertex[GraphicsIndex].TangentX = TangentX;

				// store sign of determinant in TangentZ.W
				DestVertex[GraphicsIndex].TangentZ.Vector.W = GetBasisDeterminantSign(TangentX,TangentY,TangentZ) < 0 ? 0 : 255;
			}
		}
#endif //!NX_DISABLE_CLOTH

		// set lod level currently cached
		CachedVertexLOD = LODIndex;

		// set shadow LOD to none, so it can be updated
		CachedShadowLOD = INDEX_NONE;

		// copy to the vertex buffer
		if(SkeletalMesh->bEnableClothTearing && (SkeletalMesh->ClothWeldingMap.Num() == 0))
		{
			check(LOD.NumVertices <= (UINT)CachedFinalVertices.Num());
			MeshLOD.UpdateFinalSkinVertexBuffer( &CachedFinalVertices(0), CachedFinalVerticesNum * sizeof(FFinalSkinVertex) );

			//Generate dynamic index buffer, start with existing index data, followed by cloth index data.

			//Update dynamic index buffer(TODO: refactor into another function)
			if(IsValidRef(MeshLOD.DynamicIndexBuffer.IndexBufferRHI))
			{
				const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
				const INT LODModelIndexNum = LODModel.IndexBuffer.Indices.Num();

				/* Note: we assume that the index buffer is CPU accessible and there is not a large 
				 * penalty for readback. This is true due to CPU decals already
				*/

				//Manual copy due to lack of appropriate assignment operator.
				TArray<INT> RemappedIndices;

				RemappedIndices.Empty(LODModel.IndexBuffer.Indices.Num());
				RemappedIndices.Add(LODModel.IndexBuffer.Indices.Num());

				for(INT i=0; i<RemappedIndices.Num(); i++)
				{
					RemappedIndices(i) = LODModel.IndexBuffer.Indices(i);
				}

				for(INT i=0; i<DynamicData->ClothIndexData.Num(); i+=3)
				{
					INT ClothIndex0 = DynamicData->ClothIndexData(i + 0);
					INT ClothIndex1 = DynamicData->ClothIndexData(i + 1);
					INT ClothIndex2 = DynamicData->ClothIndexData(i + 2);

					INT NewClothIndex0 = ClothIndex0;
					INT NewClothIndex1 = ClothIndex1;
					INT NewClothIndex2 = ClothIndex2;

					//Chase back all the indices to the original triangle using the parent data.
					while(ClothIndex0 >= SkeletalMesh->ClothToGraphicsVertMap.Num())
					{
						ClothIndex0 = DynamicData->ClothParentIndices(ClothIndex0);
					}

					while(ClothIndex1 >= SkeletalMesh->ClothToGraphicsVertMap.Num())
					{
						ClothIndex1 = DynamicData->ClothParentIndices(ClothIndex1);
					}
					
					while(ClothIndex2 >= SkeletalMesh->ClothToGraphicsVertMap.Num())
					{
						ClothIndex2 = DynamicData->ClothParentIndices(ClothIndex2);
					}

					//Lookup the graphics tri associated in the mesh
					//For some reason tris are stored with reverse winding in PhysX.
					QWORD i0 = ClothIndex2; 
					QWORD i1 = ClothIndex1;
					QWORD i2 = ClothIndex0;
				
					//Try all 3 permutations of the tri in case PhysX rotated the tri.
					QWORD PackedTri0 = i0 + (i1 << 16) + (i2 << 32);
					INT *Val = SkeletalMesh->ClothTornTriMap.Find(PackedTri0);

					if(Val == NULL)
					{
						QWORD PackedTri1 = i1 + (i2 << 16) + (i0 << 32);
						Val = SkeletalMesh->ClothTornTriMap.Find(PackedTri1);

						if(Val == NULL)
						{
							QWORD PackedTri2 = i2 + (i0 << 16) + (i1 << 32);
							Val = SkeletalMesh->ClothTornTriMap.Find(PackedTri2);
						}
					}

					check(Val != NULL);

					if(Val != NULL)
					{
						INT GraphicsTriOffset = *Val;
						//Update graphics indices to new indices.

						RemappedIndices(GraphicsTriOffset + 0) = 
							ClothIdxToGraphics(NewClothIndex0, SkeletalMesh->ClothToGraphicsVertMap, LOD.NumVertices);

						RemappedIndices(GraphicsTriOffset + 1) = 
							ClothIdxToGraphics(NewClothIndex1, SkeletalMesh->ClothToGraphicsVertMap, LOD.NumVertices);
						
						RemappedIndices(GraphicsTriOffset + 2) = 
							ClothIdxToGraphics(NewClothIndex2, SkeletalMesh->ClothToGraphicsVertMap, LOD.NumVertices);
					}
				}


				
				INT TotalNumIndices  = LODModelIndexNum;
				WORD* Buffer = (WORD *)RHILockIndexBuffer(MeshLOD.DynamicIndexBuffer.IndexBufferRHI,0, TotalNumIndices * sizeof(WORD));
				
				//Copy static mesh indices into the dynamic index buffer.
				for(INT i=0; i<LODModelIndexNum; i++)
				{
					check(RemappedIndices(i) < 0xffFF);
					Buffer[i] = (WORD)RemappedIndices(i);
				}

				RHIUnlockIndexBuffer(MeshLOD.DynamicIndexBuffer.IndexBufferRHI);
			}
		}
		else
		{
			check(LOD.NumVertices == CachedFinalVertices.Num());
			MeshLOD.UpdateFinalSkinVertexBuffer( &CachedFinalVertices(0), LOD.NumVertices * sizeof(FFinalSkinVertex) );
		}
	}

	// Also, copy to the shadow vertex buffer if necessary
	if ( bUpdateShadowVertices && CachedShadowLOD != LODIndex )
	{
		check(LOD.NumVertices == CachedFinalVertices.Num());
		MeshLOD.UpdateShadowVertexBuffer( &CachedFinalVertices(0), LOD.NumVertices );
		CachedShadowLOD = LODIndex;
	}
}

/**
 * @param	LODIndex - each LOD has its own vertex data
 * @param	ChunkIdx - not used
 * @return	vertex factory for rendering the LOD
 */
const FVertexFactory* FSkeletalMeshObjectCPUSkin::GetVertexFactory(INT LODIndex,INT /*ChunkIdx*/) const
{
	check( LODs.IsValidIndex(LODIndex) );
	return &LODs(LODIndex).VertexFactory;
}

/**
 * @return		Vertex factory for rendering the specified decal at the specified LOD.
 */
FSkelMeshDecalVertexFactoryBase* FSkeletalMeshObjectCPUSkin::GetDecalVertexFactory(INT LODIndex,INT ChunkIdx,const FDecalInteraction* Decal)
{
	check( bDecalFactoriesEnabled );
	FSkelMeshDecalVertexFactoryBase* DecalVertexFactory = LODs(LODIndex).GetDecalVertexFactory( Decal->Decal );
	return DecalVertexFactory;
}

/**
 * Get the shadow vertex factory for an LOD
 * @param	LODIndex - each LOD has its own vertex data
 * @return	Vertex factory for rendering the shadow volume for the LOD
 */
const FLocalShadowVertexFactory& FSkeletalMeshObjectCPUSkin::GetShadowVertexFactory( INT LODIndex ) const
{
	check( LODs.IsValidIndex(LODIndex) );
	return LODs(LODIndex).ShadowVertexBuffer.GetVertexFactory();
}

/** 
 * Init rendering resources for this LOD 
 */
void FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::InitResources()
{
	// upload vertex buffer
	BeginInitResource(&VertexBuffer);

	// update vertex factory components and sync it
	TSetResourceDataContext<FLocalVertexFactory> VertexFactoryData(&VertexFactory);

	// position
	VertexFactoryData->PositionComponent = FVertexStreamComponent(
		&VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,Position),sizeof(FFinalSkinVertex),VET_Float3);
	// tangents
	VertexFactoryData->TangentBasisComponents[0] = FVertexStreamComponent(
		&VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,TangentX),sizeof(FFinalSkinVertex),VET_PackedNormal);
	VertexFactoryData->TangentBasisComponents[1] = FVertexStreamComponent(
		&VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,TangentZ),sizeof(FFinalSkinVertex),VET_PackedNormal);
	// uvs
	VertexFactoryData->TextureCoordinates.AddItem(FVertexStreamComponent(
		&VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,U),sizeof(FFinalSkinVertex),VET_Float2));

	// copy it
	VertexFactoryData.Commit();
	BeginInitResource(&VertexFactory);

	if( UEngine::ShadowVolumesAllowed() )
	{
		// Initialize the shadow vertex buffer and its factory.
		BeginInitResource(&ShadowVertexBuffer);
	}

	// Initialize resources for decals drawn at this LOD.
	for ( INT DecalIndex = 0 ; DecalIndex < Decals.Num() ; ++DecalIndex )
	{
		FDecalLOD& Decal = Decals(DecalIndex);
		Decal.InitResources_GameThread( this );
	}
	BeginInitResource(&DynamicIndexBuffer);

	bResourcesInitialized = TRUE;
}

void FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::FDecalLOD::InitResources_GameThread(FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD* LODObject)
{
	TSetResourceDataContext<FLocalDecalVertexFactory> VertexFactoryData(&DecalVertexFactory);

	// position
	VertexFactoryData->PositionComponent = FVertexStreamComponent(
		&LODObject->VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,Position),sizeof(FFinalSkinVertex),VET_Float3);
	// tangents
	VertexFactoryData->TangentBasisComponents[0] = FVertexStreamComponent(
		&LODObject->VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,TangentX),sizeof(FFinalSkinVertex),VET_PackedNormal);
	VertexFactoryData->TangentBasisComponents[1] = FVertexStreamComponent(
		&LODObject->VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,TangentZ),sizeof(FFinalSkinVertex),VET_PackedNormal);
	// uvs
	VertexFactoryData->TextureCoordinates.AddItem(FVertexStreamComponent(
		&LODObject->VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,U),sizeof(FFinalSkinVertex),VET_Float2));

	// copy it
	VertexFactoryData.Commit();
	BeginInitResource(&DecalVertexFactory);
}

void FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::FDecalLOD::InitResources_RenderingThread(FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD* LODObject)
{
	FLocalDecalVertexFactory::DataType Data;

	// position
	Data.PositionComponent = FVertexStreamComponent(
		&LODObject->VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,Position),sizeof(FFinalSkinVertex),VET_Float3);
	// tangents
	Data.TangentBasisComponents[0] = FVertexStreamComponent(
		&LODObject->VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,TangentX),sizeof(FFinalSkinVertex),VET_PackedNormal);
	Data.TangentBasisComponents[1] = FVertexStreamComponent(
		&LODObject->VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,TangentZ),sizeof(FFinalSkinVertex),VET_PackedNormal);
	// uvs
	Data.TextureCoordinates.AddItem(FVertexStreamComponent(
		&LODObject->VertexBuffer,STRUCT_OFFSET(FFinalSkinVertex,U),sizeof(FFinalSkinVertex),VET_Float2));

	// copy it
	DecalVertexFactory.SetData(Data);
	DecalVertexFactory.Init();
}

/** 
 * Release rendering resources for this LOD 
 */
void FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::ReleaseResources()
{	
	BeginReleaseResource(&VertexFactory);
	BeginReleaseResource(&VertexBuffer);

	// Release the shadow vertex buffer and its factory.
	BeginReleaseResource(&ShadowVertexBuffer);

	BeginReleaseResource(&DynamicIndexBuffer);

	// Release resources for decals drawn at this LOD.
	for ( INT DecalIndex = 0 ; DecalIndex < Decals.Num() ; ++DecalIndex )
	{
		FDecalLOD& Decal = Decals(DecalIndex);
		Decal.ReleaseResources_GameThread();
	}

	bResourcesInitialized = FALSE;
}

/** 
 * Update the contents of the vertex buffer with new data
 * @param	NewVertices - array of new vertex data
 * @param	Size - size of new vertex data aray 
 */
void FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::UpdateFinalSkinVertexBuffer(void* NewVertices, DWORD Size) const
{
	void* Buffer = RHILockVertexBuffer(VertexBuffer.VertexBufferRHI,0,Size);
	appMemcpy(Buffer,NewVertices,Size);
	RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
}


/** 
 * Update the contents of the vertex buffer with new data
 * @param	NewVertices - array of new vertex data
 * @param	NumVertices - Number of vertices
 */
void FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::UpdateShadowVertexBuffer( const FFinalSkinVertex* NewVertices, UINT NumVertices ) const
{
	ShadowVertexBuffer.UpdateVertices( NewVertices, NumVertices, sizeof(FFinalSkinVertex) );
}

/** Creates resources for drawing the specified decal at this LOD. */
void FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::AddDecalInteraction_RenderingThread(const FDecalInteraction& DecalInteraction)
{
	checkSlow( FindDecalObjectIndex(DecalInteraction.Decal) == INDEX_NONE );

	const INT DecalIndex = Decals.AddItem( DecalInteraction.Decal );
	if ( bResourcesInitialized )
	{
		FDecalLOD& Decal = Decals(DecalIndex);
		Decal.InitResources_RenderingThread( this );
	}
}

/** Releases resources for the specified decal at this LOD. */
void FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::RemoveDecalInteraction_RenderingThread(UDecalComponent* DecalComponent)
{
	const INT DecalIndex = FindDecalObjectIndex( DecalComponent );
	if ( bResourcesInitialized )
	{
		FDecalLOD& Decal = Decals(DecalIndex);
		Decal.ReleaseResources_RenderingThread();
	}
	Decals.Remove(DecalIndex);
}

/**
 * @return		The vertex factory associated with the specified decal at this LOD.
 */
FLocalDecalVertexFactory* FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::GetDecalVertexFactory(const UDecalComponent* DecalComponent)
{
	const INT DecalIndex = FindDecalObjectIndex( DecalComponent );
	FDecalLOD& Decal = Decals(DecalIndex);
	return &(Decal.DecalVertexFactory);
}

/**
 * @return		The index into the decal objects list for the specified decal, or INDEX_NONE if none found.
 */
INT FSkeletalMeshObjectCPUSkin::FSkeletalMeshObjectLOD::FindDecalObjectIndex(const UDecalComponent* DecalComponent) const
{
	for ( INT DecalIndex = 0 ; DecalIndex < Decals.Num() ; ++DecalIndex )
	{
		const FDecalLOD& Decal = Decals(DecalIndex);
		if ( Decal.DecalComponent == DecalComponent )
		{
			return DecalIndex;
		}
	}
	return INDEX_NONE;
}

/**
 * Compute the distance of a light from the triangle planes.
 */
void FSkeletalMeshObjectCPUSkin::GetPlaneDots(FLOAT* OutPlaneDots,const FVector4& LightPosition,INT LODIndex) const
{
	const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
#if XBOX
	// Use VMX optimized version to calculate the plane dots
	GetPlaneDotsXbox(LODModel.ShadowIndices.Num() / 3,LightPosition,
		(FFinalSkinVertex*)CachedFinalVertices.GetData(),
		(WORD*)LODModel.ShadowIndices.GetData(),OutPlaneDots);
#else
	for(INT TriangleIndex = 0;TriangleIndex < LODModel.ShadowIndices.Num() / 3;TriangleIndex++)
	{
		const FVector& V1 = CachedFinalVertices(LODModel.ShadowIndices(TriangleIndex * 3 + 0)).Position;
		const FVector& V2 = CachedFinalVertices(LODModel.ShadowIndices(TriangleIndex * 3 + 1)).Position;
		const FVector& V3 = CachedFinalVertices(LODModel.ShadowIndices(TriangleIndex * 3 + 2)).Position;
		*OutPlaneDots++ = ((V2-V3) ^ (V1-V3)) | (FVector(LightPosition) - V1 * LightPosition.W);
	}
#endif
}

/** 
 *	Get the array of component-space bone transforms. 
 *	Not safe to hold this point between frames, because it exists in dynamic data passed from main thread.
 */
TArray<FMatrix>* FSkeletalMeshObjectCPUSkin::GetSpaceBases() const
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
 * Transforms the decal from refpose space to local space, in preparation for application
 * to post-skinned (ie local space) verts on the GPU.
 */
void FSkeletalMeshObjectCPUSkin::TransformDecalState(const FDecalState& DecalState,
													 FMatrix& OutDecalMatrix,
													 FVector& OutDecalLocation,
													 FVector2D& OutDecalOffset)
{
	OutDecalMatrix = DecalState.WorldTexCoordMtx;
	OutDecalLocation = DecalState.HitLocation;
	OutDecalOffset = FVector2D(DecalState.OffsetX, DecalState.OffsetY);

	if ( DynamicData )
	{
		if ( DecalState.HitBoneIndex != INDEX_NONE )
		{
			const FMatrix& RefToLocal = DynamicData->ReferenceToLocal(DecalState.HitBoneIndex);
			OutDecalLocation = RefToLocal.TransformFVector4( DecalState.HitLocation );
			const FVector LocTangent = RefToLocal.TransformNormal( DecalState.HitTangent );
			const FVector LocBinormal = RefToLocal.TransformNormal( DecalState.HitBinormal );
			const FVector LocNormal = RefToLocal.TransformNormal( DecalState.HitNormal );
			OutDecalMatrix = FMatrix( /*TileX**/LocTangent/DecalState.Width,
									/*TileY**/LocBinormal/DecalState.Height,
									LocNormal,
									FVector(0.f,0.f,0.f) ).Transpose();
		}
	}
}

/*-----------------------------------------------------------------------------
FDynamicSkelMeshObjectDataCPUSkin
-----------------------------------------------------------------------------*/

/**
* Constructor - Called on Game Thread
* Updates the ReferenceToLocal matrices using the new dynamic data.
* @param	InSkelMeshComponent - parent skel mesh component
* @param	InLODIndex - each lod has its own bone map 
* @param	InActiveMorphs - morph targets to blend with during skinning
* @param	InClothBlendWeight - amount to blend in cloth weight.
*/
FDynamicSkelMeshObjectDataCPUSkin::FDynamicSkelMeshObjectDataCPUSkin(
	USkeletalMeshComponent* InSkelMeshComponent,
	INT InLODIndex,
	const TArray<FActiveMorph>& InActiveMorphs
	)
:	LODIndex(InLODIndex)
,	ActiveMorphs(InActiveMorphs)
,	ClothBlendWeight(1.0)
{
	UpdateRefToLocalMatrices( ReferenceToLocal, InSkelMeshComponent, LODIndex );
	MeshSpaceBases = InSkelMeshComponent->SpaceBases;

#if !NX_DISABLE_CLOTH
	WorldToLocal = InSkelMeshComponent->LocalToWorld.Inverse();

	// Copy cloth vertex information to rendering thread.
	if(InSkelMeshComponent->ClothSim && !InSkelMeshComponent->bClothFrozen && InSkelMeshComponent->ClothMeshPosData.Num() > 0 && LODIndex == 0)
	{
		if (InSkelMeshComponent->SkeletalMesh->ClothWeldingMap.Num() > 0)
		{
			// Last step of Welding
			check(InSkelMeshComponent->ClothMeshPosData.Num() == InSkelMeshComponent->ClothMeshNormalData.Num());
			check(InSkelMeshComponent->ClothMeshPosData.Num() == InSkelMeshComponent->SkeletalMesh->ClothWeldingMap.Num());

			TArray<INT>& weldingMap = InSkelMeshComponent->SkeletalMesh->ClothWeldingMap;
			for (INT i = 0; i < InSkelMeshComponent->ClothMeshPosData.Num(); i++)
			{
				INT welded = weldingMap(i);
				InSkelMeshComponent->ClothMeshPosData(i) = InSkelMeshComponent->ClothMeshWeldedPosData(welded);
				InSkelMeshComponent->ClothMeshNormalData(i) = InSkelMeshComponent->ClothMeshWeldedNormalData(welded);
			}
		}
		INT NumClothVerts = InSkelMeshComponent->ClothMeshPosData.Num();

		// TODO: ClothPosData etc. arrays are NULL every time this code is executed, they must be emptied somewhere
		// This causes an alloc each frame for each cloth
		ClothPosData = InSkelMeshComponent->ClothMeshPosData;
		ClothNormalData = InSkelMeshComponent->ClothMeshNormalData;

		ClothBlendWeight = InSkelMeshComponent->ClothBlendWeight;

		ClothIndexData = InSkelMeshComponent->ClothMeshIndexData;
		
		ActualClothPosDataNum = InSkelMeshComponent->NumClothMeshVerts;
		ClothParentIndices = InSkelMeshComponent->ClothMeshParentData;

	}
	else
	{
		ActualClothPosDataNum = 0;
	}

#endif
}

/*-----------------------------------------------------------------------------
	FSkeletalMeshObjectCPUSkin - morph target blending implementation
-----------------------------------------------------------------------------*/

/**
 * Since the vertices in the active morphs are sorted based on the index of the base mesh vert
 * that they affect we keep track of the next valid morph vertex to apply
 *
 * @param	OutMorphVertIndices		[out] Llist of vertex indices that need a morph target blend
 * @return							number of active morphs that are valid
 */
static UINT GetMorphVertexIndices(const TArray<FActiveMorph>& ActiveMorphs, INT LODIndex, TArray<INT>& OutMorphVertIndices)
{
	UINT NumValidMorphs=0;

	for( INT MorphIdx=0; MorphIdx < ActiveMorphs.Num(); MorphIdx++ )
	{
		const FActiveMorph& ActiveMorph = ActiveMorphs(MorphIdx);
		if( ActiveMorph.Target &&
			ActiveMorph.Weight >= MinMorphBlendWeight && 
			ActiveMorph.Weight <= MaxMorphBlendWeight &&				
			ActiveMorph.Target->MorphLODModels.IsValidIndex(LODIndex) &&
			ActiveMorph.Target->MorphLODModels(LODIndex).Vertices.Num() )
		{
			// start at the first vertex since they affect base mesh verts in ascending order
			OutMorphVertIndices.AddItem(0);
			NumValidMorphs++;
		}
		else
		{
			// invalidate the indices for any invalid morph models
			OutMorphVertIndices.AddItem(INDEX_NONE);
		}			
	}
	return NumValidMorphs;
}

/** 
* Derive the tanget/binormal using the new normal and the base tangent vectors for a vertex 
*/
template<typename VertexType>
FORCEINLINE void RebuildTangentBasis( VertexType& DestVertex )
{
	// derive the new tangent by orthonormalizing the new normal against
	// the base tangent vector (assuming these are normalized)
	FVector Tangent( DestVertex.TangentX );
	FVector Normal( DestVertex.TangentZ );
	Tangent = Tangent - ((Tangent | Normal) * Normal);
	Tangent.Normalize();
	DestVertex.TangentX = Tangent;

	// store the sign of the determinant in TangentZ.W
	DestVertex.TangentZ.Vector.W = GetBasisDeterminantSign( Tangent, Normal ^ Tangent, Normal ) < 0 ? 0 : 255;	
}

/**
 * Applies the vertex deltas to a vertex.
 */
template<typename VertexType>
FORCEINLINE void ApplyMorphBlend( VertexType& DestVertex, const FMorphTargetVertex& SrcMorph, FLOAT Weight )
{
	// add position offset
	DestVertex.Position += SrcMorph.PositionDelta * Weight;
	// add normal offset. can only apply normal deltas up to a weight of 1
	DestVertex.TangentZ = FVector(FVector(DestVertex.TangentZ) + FVector(SrcMorph.TangentZDelta) * Min(Weight,1.0f)).UnsafeNormal();
} 

static const FVector VecZero(0,0,0);

/**
 * Blends the source vertex with all the active morph targets.
 */
template<typename VertexType>
FORCEINLINE void UpdateMorphedVertex( TArray<FActiveMorph>& ActiveMorphs, VertexType& MorphedVertex, VertexType& SrcVertex, INT CurBaseVertIdx, INT LODIndex, TArray<INT>& MorphVertIndices )
{
	// copy the source first
	MorphedVertex = SrcVertex;
	// iterate over all active morphs
	for( INT MorphIdx=0; MorphIdx < MorphVertIndices.Num(); MorphIdx++ )
	{
		// get the next affected vertex index
		INT NextMorphVertIdx = MorphVertIndices(MorphIdx);
		// get the lod model (assumed valid)
		FMorphTargetLODModel& MorphModel = ActiveMorphs(MorphIdx).Target->MorphLODModels(LODIndex);
		// if the current base vertex matches the next affected morph index 
		if( MorphModel.Vertices.IsValidIndex(NextMorphVertIdx) &&
			MorphModel.Vertices(NextMorphVertIdx).SourceIdx == CurBaseVertIdx )
		{
			ApplyMorphBlend( MorphedVertex, MorphModel.Vertices(NextMorphVertIdx), ActiveMorphs(MorphIdx).Weight );
			MorphVertIndices(MorphIdx) += 1;
		}
	}
	
	// rebuild orthonormal tangents
	RebuildTangentBasis( MorphedVertex );
}


#pragma warning(push)
#pragma warning(disable : 4730) //mixing _m64 and floating point expressions may result in incorrect code

/*-----------------------------------------------------------------------------
	FSkeletalMeshObjectCPUSkin - optimized skinning code
-----------------------------------------------------------------------------*/

#pragma warning(push)
#pragma warning(disable : 4730) //mixing _m64 and floating point expressions may result in incorrect code

/*-----------------------------------------------------------------------------
FSkeletalMeshObjectCPUSkin - optimized skinning code
-----------------------------------------------------------------------------*/

static const VectorRegister		VECTOR_PACK_127_5		= { 127.5f, 127.5f, 127.5f, 0.f };
static const VectorRegister		VECTOR4_PACK_127_5		= { 127.5f, 127.5f, 127.5f, 127.5f };

static const VectorRegister		VECTOR_INV_127_5		= { 1.f / 127.5f, 1.f / 127.5f, 1.f / 127.5f, 0.f };
static const VectorRegister		VECTOR4_INV_127_5		= { 1.f / 127.5f, 1.f / 127.5f, 1.f / 127.5f, 1.f / 127.5f };

static const VectorRegister		VECTOR_UNPACK_MINUS_1	= { -1.f, -1.f, -1.f, 0.f };
static const VectorRegister		VECTOR4_UNPACK_MINUS_1	= { -1.f, -1.f, -1.f, -1.f };

static const VectorRegister		VECTOR_INV_255			= { 1.f/255.f, 1.f/255.f, 1.f/255.f, 1.f/255.f };
static const VectorRegister		VECTOR_0001				= { 0.f, 0.f, 0.f, 1.f };

/**
* Apply scale/bias to packed normal byte values and store result in register
* Only first 3 components are copied. W component is always 0
* 
* @param PackedNormal - source vector packed with byte components
* @return vector register with unpacked float values
*/
static FORCEINLINE VectorRegister Unpack3( const DWORD *PackedNormal )
{
#if XBOX
	return VectorMultiplyAdd( VectorLoadByte4Reverse(PackedNormal), VECTOR_INV_127_5, VECTOR_UNPACK_MINUS_1 );
#else
	return VectorMultiplyAdd( VectorLoadByte4(PackedNormal), VECTOR_INV_127_5, VECTOR_UNPACK_MINUS_1 );
#endif
}

/**
* Apply scale/bias to float register values and store results in memory as packed byte values 
* Only first 3 components are copied. W component is always 0
* 
* @param Normal - source vector register with floats
* @param PackedNormal - destination vector packed with byte components
*/
static FORCEINLINE void Pack3( VectorRegister Normal, DWORD *PackedNormal )
{
	Normal = VectorMultiplyAdd(Normal, VECTOR_PACK_127_5, VECTOR_PACK_127_5);
#if XBOX
	// Need to reverse the order on Xbox.
	Normal = VectorSwizzle( Normal, 3, 2, 1, 0 );
#endif
	VectorStoreByte4( Normal, PackedNormal );
}

/**
* Apply scale/bias to packed normal byte values and store result in register
* All 4 components are copied. 
* 
* @param PackedNormal - source vector packed with byte components
* @return vector register with unpacked float values
*/
static FORCEINLINE VectorRegister Unpack4( const DWORD *PackedNormal )
{
#if XBOX
	return VectorMultiplyAdd( VectorLoadByte4Reverse(PackedNormal), VECTOR4_INV_127_5, VECTOR4_UNPACK_MINUS_1 );
#else
	return VectorMultiplyAdd( VectorLoadByte4(PackedNormal), VECTOR4_INV_127_5, VECTOR4_UNPACK_MINUS_1 );
#endif
}

/**
* Apply scale/bias to float register values and store results in memory as packed byte values 
* All 4 components are copied. 
* 
* @param Normal - source vector register with floats
* @param PackedNormal - destination vector packed with byte components
*/
static FORCEINLINE void Pack4( VectorRegister Normal, DWORD *PackedNormal )
{
	Normal = VectorMultiplyAdd(Normal, VECTOR4_PACK_127_5, VECTOR4_PACK_127_5);
#if XBOX
	// Need to reverse the order on Xbox.
	Normal = VectorSwizzle( Normal, 3, 2, 1, 0 );
#endif
	VectorStoreByte4( Normal, PackedNormal );
}

FORCEINLINE void SkinVertices( FFinalSkinVertex* DestVertex, FMatrix* ReferenceToLocal, INT LODIndex, FStaticLODModel& LOD, TArray<FActiveMorph>& ActiveMorphs )
{
	DWORD StatusRegister = VectorGetControlRegister();
	VectorSetControlRegister( StatusRegister | VECTOR_ROUND_TOWARD_ZERO );

	TArray<INT> MorphVertIndices;
	UINT NumValidMorphs = GetMorphVertexIndices(ActiveMorphs,LODIndex,MorphVertIndices);

	// Prefetch all matrices
	for ( INT MatrixIndex=0; MatrixIndex < 75; MatrixIndex+=2 )
	{
		PREFETCH( ReferenceToLocal + MatrixIndex );
	}

	INT CurBaseVertIdx = 0;
	for(INT ChunkIndex = 0;ChunkIndex < LOD.Chunks.Num();ChunkIndex++)
	{
		FSkelMeshChunk& Chunk = LOD.Chunks(ChunkIndex);

		// Prefetch all bone indices
		WORD* BoneMap = Chunk.BoneMap.GetTypedData();
		PREFETCH( BoneMap );
		PREFETCH( BoneMap + 64 );
		PREFETCH( BoneMap + 128 );
		PREFETCH( BoneMap + 192 );

		INC_DWORD_STAT_BY(STAT_CPUSkinVertices,Chunk.GetNumRigidVertices());
		INC_DWORD_STAT_BY(STAT_CPUSkinVertices,Chunk.GetNumSoftVertices());

		FSoftSkinVertex* SrcRigidVertex = NULL;
		if (Chunk.GetNumRigidVertices() > 0)
		{
			SrcRigidVertex = &LOD.VertexBufferGPUSkin.Vertices(Chunk.GetRigidVertexBufferIndex());
			PREFETCH( SrcRigidVertex );		// Prefetch first vertex
		}

		for(INT VertexIndex = 0;VertexIndex < Chunk.GetNumRigidVertices();VertexIndex++,SrcRigidVertex++,DestVertex++)
		{
			PREFETCH( ((BYTE*)SrcRigidVertex) + 128 );	// Prefetch next vertices
			FSoftSkinVertex* MorphedVertex = SrcRigidVertex;
			static FSoftSkinVertex VertexCopy;			
			if( NumValidMorphs ) 
			{
				MorphedVertex = &VertexCopy;
				UpdateMorphedVertex( ActiveMorphs, *MorphedVertex, *SrcRigidVertex, CurBaseVertIdx, LODIndex, MorphVertIndices );
			}

			VectorRegister SrcNormals[3];
			VectorRegister DstNormals[3];
			SrcNormals[0] = VectorLoadFloat3_W1( &MorphedVertex->Position );
			SrcNormals[1] = Unpack3( &MorphedVertex->TangentX.Vector.Packed );
			SrcNormals[2] = Unpack4( &MorphedVertex->TangentZ.Vector.Packed );
			VectorResetFloatRegisters(); // Need to call this to be able to use regular floating point registers again after Unpack().

			FMatrix &BoneMatrix = ReferenceToLocal[BoneMap[MorphedVertex->InfluenceBones[RigidBoneIdx]]];
			VectorRegister M00	= VectorLoadAligned( &BoneMatrix.M[0][0] );
			VectorRegister M10	= VectorLoadAligned( &BoneMatrix.M[1][0] );
			VectorRegister M20	= VectorLoadAligned( &BoneMatrix.M[2][0] );
			VectorRegister M30	= VectorLoadAligned( &BoneMatrix.M[3][0] );

			VectorRegister N_xxxx = VectorReplicate( SrcNormals[0], 0 );
			VectorRegister N_yyyy = VectorReplicate( SrcNormals[0], 1 );
			VectorRegister N_zzzz = VectorReplicate( SrcNormals[0], 2 );
			DstNormals[0] = VectorMultiplyAdd( N_xxxx, M00, VectorMultiplyAdd( N_yyyy, M10, VectorMultiplyAdd( N_zzzz, M20, M30 ) ) );

			N_xxxx = VectorReplicate( SrcNormals[1], 0 );
			N_yyyy = VectorReplicate( SrcNormals[1], 1 );
			N_zzzz = VectorReplicate( SrcNormals[1], 2 );
			DstNormals[1] = VectorMultiplyAdd( N_xxxx, M00, VectorMultiplyAdd( N_yyyy, M10, VectorMultiply( N_zzzz, M20 ) ) );

			N_xxxx = VectorReplicate( SrcNormals[2], 0 );
			N_yyyy = VectorReplicate( SrcNormals[2], 1 );
			N_zzzz = VectorReplicate( SrcNormals[2], 2 );
			DstNormals[2] = VectorMultiplyAdd( N_xxxx, M00, VectorMultiplyAdd( N_yyyy, M10, VectorMultiply( N_zzzz, M20 ) ) );

			// carry over the W component (sign of basis determinant) 
			DstNormals[2] = VectorMultiplyAdd( VECTOR_0001, SrcNormals[2], DstNormals[2] );

			// Write to 16-byte aligned memory:
			VectorStore( DstNormals[0], &DestVertex->Position );
			Pack3( DstNormals[1], &DestVertex->TangentX.Vector.Packed );
			Pack4( DstNormals[2], &DestVertex->TangentZ.Vector.Packed );
			VectorResetFloatRegisters(); // Need to call this to be able to use regular floating point registers again after Pack().
			DestVertex->U = MorphedVertex->U;
			DestVertex->V = MorphedVertex->V;
			CurBaseVertIdx++;
		}

		FSoftSkinVertex* SrcSoftVertex = NULL;
		if (Chunk.GetNumSoftVertices() > 0)
		{
			SrcSoftVertex = &LOD.VertexBufferGPUSkin.Vertices(Chunk.GetSoftVertexBufferIndex());
			PREFETCH( SrcSoftVertex );	// Prefetch first vertex
		}
		for(INT VertexIndex = 0;VertexIndex < Chunk.GetNumSoftVertices();VertexIndex++,SrcSoftVertex++,DestVertex++)
		{
			PREFETCH( ((BYTE*)SrcSoftVertex) + 128 );	// Prefetch next vertices
			FSoftSkinVertex* MorphedVertex = SrcSoftVertex;
			static FSoftSkinVertex VertexCopy;			
			if( NumValidMorphs ) 
			{
				MorphedVertex = &VertexCopy;
				UpdateMorphedVertex( ActiveMorphs, *MorphedVertex, *SrcSoftVertex, CurBaseVertIdx, LODIndex, MorphVertIndices );
			}

			static VectorRegister	SrcNormals[3];
			VectorRegister			DstNormals[3];

			SrcNormals[0] = VectorLoadFloat3_W1( &MorphedVertex->Position );
			SrcNormals[1] = Unpack3( &MorphedVertex->TangentX.Vector.Packed );
			SrcNormals[2] = Unpack4( &MorphedVertex->TangentZ.Vector.Packed );
			VectorRegister Weights = VectorMultiply( VectorLoadByte4(MorphedVertex->InfluenceWeights), VECTOR_INV_255 );
			VectorResetFloatRegisters(); // Need to call this to be able to use regular floating point registers again after Unpack and VectorLoadByte4.

			FMatrix &BoneMatrix0 = ReferenceToLocal[BoneMap[MorphedVertex->InfluenceBones[INFLUENCE_0]]];
			VectorRegister Weight0 = VectorReplicate( Weights, INFLUENCE_0 );
			VectorRegister M00	= VectorMultiply( VectorLoadAligned( &BoneMatrix0.M[0][0] ), Weight0 );
			VectorRegister M10	= VectorMultiply( VectorLoadAligned( &BoneMatrix0.M[1][0] ), Weight0 );
			VectorRegister M20	= VectorMultiply( VectorLoadAligned( &BoneMatrix0.M[2][0] ), Weight0 );
			VectorRegister M30	= VectorMultiply( VectorLoadAligned( &BoneMatrix0.M[3][0] ), Weight0 );

			if ( Chunk.MaxBoneInfluences > 1 )
			{
				FMatrix &BoneMatrix1 = ReferenceToLocal[BoneMap[MorphedVertex->InfluenceBones[INFLUENCE_1]]];
				VectorRegister Weight1 = VectorReplicate( Weights, INFLUENCE_1 );
				M00	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix1.M[0][0] ), Weight1, M00 );
				M10	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix1.M[1][0] ), Weight1, M10 );
				M20	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix1.M[2][0] ), Weight1, M20 );
				M30	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix1.M[3][0] ), Weight1, M30 );

				if ( Chunk.MaxBoneInfluences > 2 )
				{
					FMatrix &BoneMatrix2 = ReferenceToLocal[BoneMap[MorphedVertex->InfluenceBones[INFLUENCE_2]]];
					VectorRegister Weight2 = VectorReplicate( Weights, INFLUENCE_2 );
					M00	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix2.M[0][0] ), Weight2, M00 );
					M10	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix2.M[1][0] ), Weight2, M10 );
					M20	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix2.M[2][0] ), Weight2, M20 );
					M30	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix2.M[3][0] ), Weight2, M30 );

					if ( Chunk.MaxBoneInfluences > 3 )
					{
						FMatrix &BoneMatrix3 = ReferenceToLocal[BoneMap[MorphedVertex->InfluenceBones[INFLUENCE_3]]];
						VectorRegister Weight3 = VectorReplicate( Weights, INFLUENCE_3 );
						M00	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix3.M[0][0] ), Weight3, M00 );
						M10	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix3.M[1][0] ), Weight3, M10 );
						M20	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix3.M[2][0] ), Weight3, M20 );
						M30	= VectorMultiplyAdd( VectorLoadAligned( &BoneMatrix3.M[3][0] ), Weight3, M30 );
					}
				}
			}

			VectorRegister N_xxxx = VectorReplicate( SrcNormals[0], 0 );
			VectorRegister N_yyyy = VectorReplicate( SrcNormals[0], 1 );
			VectorRegister N_zzzz = VectorReplicate( SrcNormals[0], 2 );
			DstNormals[0] = VectorMultiplyAdd( N_xxxx, M00, VectorMultiplyAdd( N_yyyy, M10, VectorMultiplyAdd( N_zzzz, M20, M30 ) ) );

			N_xxxx = VectorReplicate( SrcNormals[1], 0 );
			N_yyyy = VectorReplicate( SrcNormals[1], 1 );
			N_zzzz = VectorReplicate( SrcNormals[1], 2 );
			DstNormals[1] = VectorMultiplyAdd( N_xxxx, M00, VectorMultiplyAdd( N_yyyy, M10, VectorMultiply( N_zzzz, M20 ) ) );

			N_xxxx = VectorReplicate( SrcNormals[2], 0 );
			N_yyyy = VectorReplicate( SrcNormals[2], 1 );
			N_zzzz = VectorReplicate( SrcNormals[2], 2 );
			DstNormals[2] = VectorMultiplyAdd( N_xxxx, M00, VectorMultiplyAdd( N_yyyy, M10, VectorMultiply( N_zzzz, M20 ) ) );

			// carry over the W component (sign of basis determinant) 
			DstNormals[2] = VectorMultiplyAdd( VECTOR_0001, SrcNormals[2], DstNormals[2] );

			// Write to 16-byte aligned memory:
			VectorStore( DstNormals[0], &DestVertex->Position );
			Pack3( DstNormals[1], &DestVertex->TangentX.Vector.Packed );
			Pack4( DstNormals[2], &DestVertex->TangentZ.Vector.Packed );
			VectorResetFloatRegisters(); // Need to call this to be able to use regular floating point registers again after Pack().
			DestVertex->U = MorphedVertex->U;
			DestVertex->V = MorphedVertex->V;
			CurBaseVertIdx++;
		}
	}

	VectorSetControlRegister( StatusRegister );
}

#pragma warning(pop)


