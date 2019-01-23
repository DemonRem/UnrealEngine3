/*=============================================================================
	UnSkeletalMesh.cpp: Unreal skeletal mesh and animation implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnFaceFXSupport.h"
#include "EngineAnimClasses.h"
#include "EnginePhysicsClasses.h"
#include "ScenePrivate.h"
#include "UnDecalRenderData.h"
#include "GPUSkinVertexFactory.h"

#if WITH_FACEFX
using namespace OC3Ent;
using namespace Face;
#endif

IMPLEMENT_CLASS(USkeletalMesh);
IMPLEMENT_CLASS(USkeletalMeshSocket);
IMPLEMENT_CLASS(ASkeletalMeshActor);
IMPLEMENT_CLASS(ASkeletalMeshActorMAT);

/*-----------------------------------------------------------------------------
	FSoftSkinVertex
-----------------------------------------------------------------------------*/

/**
* Serializer
*
* @param Ar - archive to serialize with
* @param V - vertex to serialize
* @return archive that was used
*/
FArchive& operator<<(FArchive& Ar,FSoftSkinVertex& V)
{
	Ar << V.Position;
	Ar << V.TangentX << V.TangentY << V.TangentZ;

	// store the sign of the determinant in TangentZ.W
	if( Ar.Ver() < VER_REMOVE_BINORMAL_TANGENT_VECTOR )
	{
		V.TangentZ.Vector.W = GetBasisDeterminantSignByte( V.TangentX, V.TangentY, V.TangentZ );
	}

	Ar << V.U << V.V;

	if( Ar.Ver() < VER_USE_UMA_RESOURCE_ARRAY_MESH_DATA )
	{
		// legacy serialize
		for(UINT InfluenceIndex = 0;InfluenceIndex < MAX_INFLUENCES;InfluenceIndex++)
		{
			Ar << V.InfluenceBones[InfluenceIndex];
			Ar << V.InfluenceWeights[InfluenceIndex];
		}
	}
	else
	{			
		// serialize bone and weight BYTE arrays in order
		// this is required when serializing as bulk data memory (see TArray::BulkSerialize notes)
		for(UINT InfluenceIndex = 0;InfluenceIndex < MAX_INFLUENCES;InfluenceIndex++)
		{
			Ar << V.InfluenceBones[InfluenceIndex];
		}
		for(UINT InfluenceIndex = 0;InfluenceIndex < MAX_INFLUENCES;InfluenceIndex++)
		{
			Ar << V.InfluenceWeights[InfluenceIndex];
		}
	}
	return Ar;
}

/*-----------------------------------------------------------------------------
	FRigidSkinVertex
-----------------------------------------------------------------------------*/

/**
* Serializer
*
* @param Ar - archive to serialize with
* @param V - vertex to serialize
* @return archive that was used
*/
FArchive& operator<<(FArchive& Ar,FRigidSkinVertex& V)
{
	Ar << V.Position;
	Ar << V.TangentX << V.TangentY << V.TangentZ;

	// store the sign of the determinant in TangentZ.W
	if( Ar.Ver() < VER_REMOVE_BINORMAL_TANGENT_VECTOR )
	{
		V.TangentZ.Vector.W = GetBasisDeterminantSignByte( V.TangentX, V.TangentY, V.TangentZ );
	}

	Ar << V.U << V.V << V.Bone;
	return Ar;
}

/*-----------------------------------------------------------------------------
	FStaticLODModel
-----------------------------------------------------------------------------*/

/**
* Special serialize function passing the owning UObject along as required by FUnytpedBulkData
* serialization.
*
* @param	Ar		Archive to serialize with
* @param	Owner	UObject this structure is serialized within
*/
void FStaticLODModel::Serialize( FArchive& Ar, UObject* Owner )
{
	Ar << Sections;
	Ar << IndexBuffer;
	Ar << ShadowIndices << ActiveBoneIndices << ShadowTriangleDoubleSided;
	Ar << Chunks;
	Ar << Size;
	Ar << NumVertices;
	Ar << Edges;
	Ar << RequiredBones;
	if( Ar.Ver() < VER_REPLACED_LAZY_ARRAY_WITH_UNTYPED_BULK_DATA )
	{
		RawPointIndices.SerializeLikeLazyArray( Ar, Owner );
	}
	else
	{
		RawPointIndices.Serialize( Ar, Owner );
	}

	if( Ar.Ver() < VER_USE_UMA_RESOURCE_ARRAY_MESH_DATA &&
		Ar.IsLoading() )
	{
        // build vertex buffers for legacy load
		BuildVertexBuffers();
	}
	else
	{	
		// load/save vertex buffer 
		Ar << VertexBufferGPUSkin;
	}
}

/**
* Initialize the LOD's render resources.
*
* @param Parent Parent mesh
*/
void FStaticLODModel::InitResources(class USkeletalMesh* Parent)
{
	check(Parent);
	INC_DWORD_STAT_BY( STAT_SkeletalMeshIndexMemory, IndexBuffer.Indices.Num() * 2 );
	BeginInitResource(&IndexBuffer);

	if( !Parent->IsCPUSkinned() )
	{
		INC_DWORD_STAT_BY( STAT_SkeletalMeshVertexMemory, VertexBufferGPUSkin.Vertices.GetResourceDataSize() );
        BeginInitResource(&VertexBufferGPUSkin);
	}
}

/**
* Releases the LOD's render resources.
*/
void FStaticLODModel::ReleaseResources()
{
	INC_DWORD_STAT_BY( STAT_SkeletalMeshIndexMemory, IndexBuffer.Indices.Num() * 2 );
	DEC_DWORD_STAT_BY( STAT_SkeletalMeshVertexMemory, VertexBufferGPUSkin.Vertices.GetResourceDataSize() );
	BeginReleaseResource(&IndexBuffer);
	BeginReleaseResource(&VertexBufferGPUSkin);
}

/**
* Utility function for returning total number of faces in this LOD. 
*/
INT FStaticLODModel::GetTotalFaces()
{
	INT TotalFaces = 0;
	for(INT i=0; i<Sections.Num(); i++)
	{
		TotalFaces += Sections(i).NumTriangles;
	}

	return TotalFaces;
}

/**
* Initialize postion and tangent vertex buffers from skel mesh chunks
*/
void FStaticLODModel::BuildVertexBuffers()
{
    VertexBufferGPUSkin.Vertices.Empty(NumVertices);
	VertexBufferGPUSkin.Vertices.Add(NumVertices);
        
	// Initialize the vertex data
	// All chunks are combined into one (rigid first, soft next)
	FSoftSkinVertex* DestVertex = (FSoftSkinVertex*)VertexBufferGPUSkin.Vertices.GetData();
	for(INT ChunkIndex = 0;ChunkIndex < Chunks.Num();ChunkIndex++)
	{
		const FSkelMeshChunk& Chunk = Chunks(ChunkIndex);
		check(Chunk.NumRigidVertices == Chunk.RigidVertices.Num());
		check(Chunk.NumSoftVertices == Chunk.SoftVertices.Num());
		for(INT VertexIndex = 0;VertexIndex < Chunk.RigidVertices.Num();VertexIndex++)
		{
			const FRigidSkinVertex& SourceVertex = Chunk.RigidVertices(VertexIndex);
			DestVertex->Position = SourceVertex.Position;
			DestVertex->TangentX = SourceVertex.TangentX;
			DestVertex->TangentY = SourceVertex.TangentY;
			DestVertex->TangentZ = SourceVertex.TangentZ;
			// store the sign of the determinant in TangentZ.W
			DestVertex->TangentZ.Vector.W = GetBasisDeterminantSignByte( SourceVertex.TangentX, SourceVertex.TangentY, SourceVertex.TangentZ );

			DestVertex->U = SourceVertex.U;
			DestVertex->V = SourceVertex.V;
			DestVertex->InfluenceBones[0] = SourceVertex.Bone;
			DestVertex->InfluenceWeights[0] = 255;
			for(INT InfluenceIndex = 1;InfluenceIndex < 4;InfluenceIndex++)
			{
				DestVertex->InfluenceBones[InfluenceIndex] = 0;
				DestVertex->InfluenceWeights[InfluenceIndex] = 0;
			}
			DestVertex++;
		}
		appMemcpy(DestVertex,&Chunk.SoftVertices(0),Chunk.SoftVertices.Num() * sizeof(FSoftSkinVertex));
		DestVertex += Chunk.SoftVertices.Num();
	}
}

/*-----------------------------------------------------------------------------
USkeletalMesh
-----------------------------------------------------------------------------*/

/**
* Calculate max # of bone influences used by this skel mesh chunk
*/
void FSkelMeshChunk::CalcMaxBoneInfluences()
{
	// if we only have rigid verts then there is only one bone
	MaxBoneInfluences = 1;
	// iterate over all the soft vertices for this chunk and find max # of bones used
	for( INT VertIdx=0; VertIdx < SoftVertices.Num(); VertIdx++ )
	{
		FSoftSkinVertex& SoftVert = SoftVertices(VertIdx);

		// calc # of bones used by this soft skinned vertex
		INT BonesUsed=0;
		for( INT InfluenceIdx=0; InfluenceIdx < MAX_INFLUENCES; InfluenceIdx++ )
		{
			if( SoftVert.InfluenceWeights[InfluenceIdx] > 0 )
			{
				BonesUsed++;
			}
		}
		// reorder bones so that there aren't any unused influence entries within the [0,BonesUsed] range
		for( INT InfluenceIdx=0; InfluenceIdx < BonesUsed; InfluenceIdx++ )
		{
			if( SoftVert.InfluenceWeights[InfluenceIdx] == 0 )
			{
                for( INT ExchangeIdx=InfluenceIdx+1; ExchangeIdx < MAX_INFLUENCES; ExchangeIdx++ )
				{
                    if( SoftVert.InfluenceWeights[ExchangeIdx] != 0 )
					{
						Exchange(SoftVert.InfluenceWeights[InfluenceIdx],SoftVert.InfluenceWeights[ExchangeIdx]);
						Exchange(SoftVert.InfluenceBones[InfluenceIdx],SoftVert.InfluenceBones[ExchangeIdx]);
						break;
					}
				}
			}
		}

		// maintain max bones used
		MaxBoneInfluences = Max(MaxBoneInfluences,BonesUsed);			
	}
}

/*-----------------------------------------------------------------------------
	USkeletalMesh
-----------------------------------------------------------------------------*/

/**
* Initialize the mesh's render resources.
*/
void USkeletalMesh::InitResources()
{
	// initialize resources for each lod
	for( INT LODIndex = 0;LODIndex < LODModels.Num();LODIndex++ )
	{
		LODModels(LODIndex).InitResources(this);
	}
}

/**
* Releases the mesh's render resources.
*/
void USkeletalMesh::ReleaseResources()
{
	// release resources for each lod
	for( INT LODIndex = 0;LODIndex < LODModels.Num();LODIndex++ )
	{
		LODModels(LODIndex).ReleaseResources();
	}

	// insert a fence to signal when these commands completed
	ReleaseResourcesFence.BeginFence();
}

/**
 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
 *
 * @return size of resource as to be displayed to artists/ LDs in the Editor.
 */
INT USkeletalMesh::GetResourceSize()
{
	FArchiveCountMem CountBytesSize( this );
	INT ResourceSize = CountBytesSize.GetNum();
	return ResourceSize;
}

/**
 * Operator for MemCount only, so it only serializes the arrays that needs to be counted.
 */
FArchive &operator<<( FArchive& Ar, FSkeletalMeshLODInfo& I )
{
	Ar << I.LODMaterialMap;
	Ar << I.bEnableShadowCasting;
	return Ar;
}

/** 
* Some property has changed so cleanup resources
*
* @param	PropertyAboutToChange - property that is changing
*/
void USkeletalMesh::PreEditChange(UProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	// Release the mesh's render resources.
	ReleaseResources();

	// Flush the resource release commands to the rendering thread to ensure that the edit change doesn't occur while a resource is still
	// allocated, and potentially accessing the mesh data.
	ReleaseResourcesFence.Wait();
}

/** 
* Some property has changed so recreate resources
*
* @param	PropertyAboutToChange - property that changed
*/
void USkeletalMesh::PostEditChange(UProperty* PropertyThatChanged)
{
	// rebuild vertex buffers
	for( INT LODIndex = 0;LODIndex < LODModels.Num();LODIndex++ )
	{
		LODModels(LODIndex).BuildVertexBuffers();
	}

	// Reinitialize the mesh's render resources.
	InitResources();

	// Rebuild any per-poly collision data we want.
	UpdatePerPolyKDOPs();

	// invalidate the components that use the mesh 
	// when switching between cpu/gpu skinning
	if( GIsEditor &&
		PropertyThatChanged &&
		PropertyThatChanged->GetFName() == FName(TEXT("bForceCPUSkinning")) )
	{
		TArray<AActor*> ActorsToUpdate;
		for( TObjectIterator<USkeletalMeshComponent> It; It; ++It )
		{
			USkeletalMeshComponent* MeshComponent = *It;
			if( MeshComponent && 
				MeshComponent->SkeletalMesh == this )
			{
				FComponentReattachContext ReattachContext(MeshComponent);
			}
		}
	}	

	Super::PostEditChange(PropertyThatChanged);
}

/** 
* This object wants to be destroyed so cleanup the rendering resources 
*/
void USkeletalMesh::BeginDestroy()
{
	Super::BeginDestroy();

	// Release the mesh's render resources.
	ReleaseResources();
}

/**
* Check for asynchronous resource cleanup completion
*
* @return	TRUE if the rendering resources have been released
*/
UBOOL USkeletalMesh::IsReadyForFinishDestroy()
{
	// see if we have hit the resource flush fence
	return ReleaseResourcesFence.GetNumPendingFences() == 0;
}

/** 
* Serialize 
*/
void USkeletalMesh::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	Ar << Bounds;
	Ar << Materials;
	Ar << Origin << RotOrigin;
	Ar << RefSkeleton;			// Reference skeleton.
	Ar << SkeletalDepth;		// How many bones beep the heirarchy goes.		
	LODModels.Serialize( Ar, this );

	// make sure we're counting properly
	if (!Ar.IsLoading() && !Ar.IsSaving())
	{
		Ar << RefBasesInvMatrix;
        SkelMirrorTable.CountBytes(Ar); // this doesn't have an << operator, but it doesn't have any sub arrays, so we're cool
		Ar << Sockets;
		Ar << LODInfo;
/* // @todo: count mem for cloth pointers, etc (icky FPointer arrays)
		Ar << ClothMesh << ClothMeshScale << ClothToGraphicsVertMap << ClothIndexBuffer << ClothBones;
*/
	}

	if( Ar.Ver() >= VER_ADD_SKELMESH_NAMEINDEXMAP )
	{
		Ar << NameIndexMap;
	}

	if( Ar.IsCountingMemory() )
	{
		//TODO: Count this member when calculating memory used
		// Ar << ReleaseResourcesFence;
	}

	if( Ar.Ver() >= VER_SKELMESH_BONE_KDOP )
	{
		Ar << PerPolyBoneKDOPs;
	}

#if !CONSOLE
	// Strip away loaded Editor-only data if we're a client and never care about saving.
	if( Ar.IsLoading() && GIsClient && !GIsEditor && !GIsUCC )
	{
		// Console platform is not a mistake, this ensures that as much as possible will be tossed.
		StripData( UE3::PLATFORM_Console );
	}
#endif
}

/** 
* Postload 
*/
void USkeletalMesh::PostLoad()
{
	Super::PostLoad();

	// If LODInfo is missing - create array of correct size.
	if( LODInfo.Num() != LODModels.Num() )
	{
		LODInfo.Empty(LODModels.Num());
		LODInfo.AddZeroed(LODModels.Num());

		for(INT i=0; i<LODInfo.Num(); i++)
		{
			LODInfo(i).LODHysteresis = 0.02f;

			// Presize the per-section shadow array.
			LODInfo(i).bEnableShadowCasting.Empty( LODModels(i).Sections.Num() );
			for ( INT SectionIndex = 0 ; SectionIndex < LODModels(i).Sections.Num() ; ++SectionIndex )
			{
				LODInfo(i).bEnableShadowCasting.AddItem( TRUE );
			}
		}
	}

	// Pre-size each LOD's per-section bEnableShadowCasting array.
	if( GetLinker() && GetLinker()->Ver() < VER_SHADOW_CASTING_PER_MESH_ELEMENT )
	{
		for( INT LODIndex = 0 ; LODIndex < LODInfo.Num() ; ++LODIndex )
		{
			if ( LODInfo(LODIndex).bEnableShadowCasting.Num() != LODModels(LODIndex).Sections.Num() )
			{
				LODInfo(LODIndex).bEnableShadowCasting.Empty( LODModels(LODIndex).Sections.Num() );
				for ( INT SectionIndex = 0 ; SectionIndex < LODModels(LODIndex).Sections.Num() ; ++SectionIndex )
				{
					LODInfo(LODIndex).bEnableShadowCasting.AddItem( TRUE );
				}
			}
		}
	}

	// For version before we added RequiredBones, create the array
	if( GetLinker() && GetLinker()->Ver() < 206 )
	{
		// Throw away LODs imported before this version.
		check(LODInfo.Num() == LODModels.Num());
		if(LODInfo.Num() > 1)
		{
			LODInfo.Remove(1, LODInfo.Num()-1);
			LODModels.Remove(1, LODModels.Num()-1);
		}

		// Make sure the per-section shadow array is sized correctly.
		FStaticLODModel& LODModel = LODModels(0);
		if ( LODInfo(0).bEnableShadowCasting.Num() != LODModel.Sections.Num() )
		{
			LODInfo(0).bEnableShadowCasting.Empty( LODModel.Sections.Num() );
			for ( INT SectionIndex = 0 ; SectionIndex < LODModel.Sections.Num() ; ++SectionIndex )
			{
				LODInfo(0).bEnableShadowCasting.AddItem( TRUE );
			}
		}

		// RequiredBones for base model includes all bones.
		LODModel.RequiredBones.Add(RefSkeleton.Num());
		for(INT i=0; i<LODModel.RequiredBones.Num(); i++)
		{
			LODModel.RequiredBones(i) = i;
		}
	}

	if( !UEngine::ShadowVolumesAllowed() )
	{
		RemoveShadowVolumeData();
	}

	// initialize rendering resources
	InitResources();

	CalculateInvRefMatrices();

	// If the name->index is empty - build now (for old content)
	if(NameIndexMap.Num() == 0)
	{
		InitNameIndexMap();
	}
}

void USkeletalMesh::PreSave()
{
	Super::PreSave();

	// Make sure collision data is up to date. Don't execute on default objects!
	// We also don't do this during cooking, because the chunk verts we need get stripped out.
	if(!IsTemplate() && !GIsCooking)
	{
		UpdatePerPolyKDOPs();
	}
}

void USkeletalMesh::FinishDestroy()
{
	ClearClothMeshCache();
	Super::FinishDestroy();
}

/**
 * Used by various commandlets to purge Editor only data from the object.
 * 
 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
 */
void USkeletalMesh::StripData(UE3::EPlatformType TargetPlatform)
{
	Super::StripData(TargetPlatform);	

	// Strip out unwanted LOD levels based on current platform.
	if( LODModels.Num() )
	{
		check( LODModels.Num() == LODInfo.Num() );
		
		// Determine LOD bias to use.
		INT PlatformLODBias = 0;
		switch( TargetPlatform )
		{
		case UE3::PLATFORM_Xenon:
			PlatformLODBias = LODBiasXbox360;
			break;
		case UE3::PLATFORM_PS3:
			PlatformLODBias = LODBiasPS3;
			break;
		case UE3::PLATFORM_Windows:
		default:
			PlatformLODBias = LODBiasPC;
			break;
		}

		// Ensure valid range.
		PlatformLODBias = Clamp(PlatformLODBias,0,LODModels.Num()-1);

		// Release resources for LOD before destroying it implicitly by removing from array.
		for( INT LODIndex=0; LODIndex<PlatformLODBias; LODIndex++ )
		{
			LODModels(LODIndex).ReleaseResources();
		}

		// Remove LOD models and info structs.
		if( PlatformLODBias )
		{
			LODModels.Remove( 0, PlatformLODBias );	
			LODInfo.Remove( 0, PlatformLODBias );
		}
	}

	// @todo: in theory the data is used by the Editor, but not really required
	if( TRUE ) // TargetPlatform & UE3::PLATFORM_Console )
	{
		for( INT LODIdx=0; LODIdx < LODModels.Num(); LODIdx++ )
		{
			FStaticLODModel& LODModel = LODModels(LODIdx);

			LODModel.RawPointIndices.RemoveBulkData();

			for( INT ChunkIdx=0; ChunkIdx < LODModel.Chunks.Num(); ChunkIdx++ )
			{
				FSkelMeshChunk& Chunk = LODModel.Chunks(ChunkIdx);
				// stored in vertex buffer
				Chunk.RigidVertices.Empty();
				Chunk.SoftVertices.Empty();
			}
		}
	}

	// remove shadow volume data based on engine config setting
	if( !UEngine::ShadowVolumesAllowed() )
	{
		RemoveShadowVolumeData();
	}
}

/** Clear and create the NameIndexMap of bone name to bone index. */
void USkeletalMesh::InitNameIndexMap()
{
	// Start by clearing the current map.
	NameIndexMap.Empty();

	// Then iterate over each bone, adding the name and bone index.
	for(INT i=0; i<RefSkeleton.Num(); i++)
	{
		FName BoneName = RefSkeleton(i).Name;
		if(BoneName != NAME_None)
		{
			NameIndexMap.Set(BoneName, i);
		}
	}
}

UBOOL USkeletalMesh::IsCPUSkinned() const
{
	// Only use GPU skinning if all the chunks in the mesh use fewer than MAX_GPUSKIN_BONES simultaneous bones.
	UBOOL bUseCPUSkinning = bForceCPUSkinning;

	if (!bUseCPUSkinning && LODModels.Num())
	{
		for (INT ChunkIndex = 0; ChunkIndex < LODModels(0).Chunks.Num(); ChunkIndex++)
		{
			if (LODModels(0).Chunks(ChunkIndex).BoneMap.Num() > MAX_GPUSKIN_BONES)
			{
				debugf(TEXT("'%s' has too many bones (%d) for GPU skinning"), *GetFullName(), LODModels(0).Chunks(ChunkIndex).BoneMap.Num());
				bUseCPUSkinning = TRUE;
				break;
			}
		}
	}

	return bUseCPUSkinning;
}

/**
 * Match up startbone by name. Also allows the attachment tag aliases.
 * Pretty much the same as USkeletalMeshComponent::MatchRefBone
 */
INT USkeletalMesh::MatchRefBone(FName StartBoneName) const
{
	INT BoneIndex = INDEX_NONE;
	if( StartBoneName != NAME_None )
	{
		const INT* IndexPtr = NameIndexMap.Find(StartBoneName);
		if(IndexPtr)
		{
			BoneIndex = *IndexPtr;
		}
	}
	return BoneIndex;
}

/** 
 *	Find a socket object in this SkeletalMesh by name. 
 *	Entering NAME_None will return NULL. If there are multiple sockets with the same name, will return the first one.
 */
USkeletalMeshSocket* USkeletalMesh::FindSocket(FName InSocketName)
{
	if(InSocketName == NAME_None)
	{
		return NULL;
	}

	for(INT i=0; i<Sockets.Num(); i++)
	{
		USkeletalMeshSocket* Socket = Sockets(i);
		if(Socket && Socket->SocketName == InSocketName)
		{
			return Socket;
		}
	}

	return NULL;
}

/** 
* GetRefPoseMatrix 
*/
FMatrix USkeletalMesh::GetRefPoseMatrix( INT BoneIndex ) const
{
	check( BoneIndex >= 0 && BoneIndex < RefSkeleton.Num() );
	return FQuatRotationTranslationMatrix( RefSkeleton(BoneIndex).BonePos.Orientation, RefSkeleton(BoneIndex).BonePos.Position );
}

/** 
* Removes all vertex data needed for shadow volume rendering 
*/
void USkeletalMesh::RemoveShadowVolumeData()
{
#if !CONSOLE
	for( INT LODIdx=0; LODIdx < LODModels.Num(); LODIdx++ )
	{
		FStaticLODModel& LODModel = LODModels(LODIdx);
		LODModel.Edges.Empty();
		LODModel.ShadowIndices.Empty();
		LODModel.ShadowTriangleDoubleSided.Empty();
	}
#endif
}

/*-----------------------------------------------------------------------------
USkeletalMeshSocket
-----------------------------------------------------------------------------*/

/** 
* Utility that returns the current matrix for this socket. Returns false if socket was not valid (bone not found etc) 
*/
UBOOL USkeletalMeshSocket::GetSocketMatrix(FMatrix& OutMatrix, class USkeletalMeshComponent* SkelComp)
{
	INT BoneIndex = SkelComp->MatchRefBone(BoneName);
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneMatrix = SkelComp->GetBoneMatrix(BoneIndex);
		FRotationTranslationMatrix RelSocketMatrix( RelativeRotation, RelativeLocation );
		OutMatrix = RelSocketMatrix * BoneMatrix;
		return true;
	}

	return false;
}

/*-----------------------------------------------------------------------------
	ASkeletalMeshActor
-----------------------------------------------------------------------------*/

/** 
* PreviewBeginAnimControl 
*/
void ASkeletalMeshActor::PreviewBeginAnimControl(TArray<class UAnimSet*>& InAnimSets)
{
	SkeletalMeshComponent->AnimSets.Empty();
	SkeletalMeshComponent->AnimSets.Add(InAnimSets.Num());

	for(INT i=0; i<InAnimSets.Num(); i++)
	{
		SkeletalMeshComponent->AnimSets(i) = InAnimSets(i);
	}

	SkeletalMeshComponent->InitAnimTree();
}

/** 
* PreviewSetAnimPosition 
*/
void ASkeletalMeshActor::PreviewSetAnimPosition(FName SlotName, INT ChannelIndex, FName InAnimSeqName, FLOAT InPosition, UBOOL bLooping)
{
	UAnimNodeSequence* SeqNode = Cast<UAnimNodeSequence>(SkeletalMeshComponent->Animations);

	// Do nothing if no anim tree or its not an AnimNodeSequence
	if(!SeqNode)
	{
		return;
	}

	if(SeqNode->AnimSeqName != InAnimSeqName)
	{
		SeqNode->SetAnim(InAnimSeqName);
	}

	SeqNode->bLooping = bLooping;
	SeqNode->SetPosition(InPosition, false);

	// Update space bases so new animation position has an effect.
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

/** 
* PreviewSetAnimWeights 
*/
void ASkeletalMeshActor::PreviewSetAnimWeights(TArray<FAnimSlotInfo>& SlotInfos)
{

}

/** 
* PreviewFinishAnimControl 
*/
void ASkeletalMeshActor::PreviewFinishAnimControl()
{
	UAnimNodeSequence* SeqNode = Cast<UAnimNodeSequence>(SkeletalMeshComponent->Animations);

	// Do nothing if no anim tree or its not an AnimNodeSequence
	if(!SeqNode)
	{
		return;
	}

	SeqNode->SetAnim(NAME_None);

	// Update space bases to reset it back to ref pose
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();

	// Clear the AnimSets array.
	SkeletalMeshComponent->AnimSets.Empty();
}

/** Function used to control FaceFX animation in the editor (Matinee). */
void ASkeletalMeshActor::PreviewUpdateFaceFX(UBOOL bForceAnim, const FString& GroupName, const FString& SeqName, FLOAT InPosition)
{
	//debugf( TEXT("GroupName: %s  SeqName: %s  InPos: %f"), *GroupName, *SeqName, InPosition );

	check(SkeletalMeshComponent);

	// Scrubbing case
	if(bForceAnim)
	{
#if WITH_FACEFX
		if(SkeletalMeshComponent->FaceFXActorInstance)
		{
			// FaceFX animations start at a neagative time, where zero time is where the sound begins ie. a pre-amble before the audio starts.
			// Because Matinee just think of things as starting at zero, we need to determine this start offset to place ourselves at the 
			// correct point in the FaceFX animation.

			// Get the FxActor
			FxActor* fActor = SkeletalMeshComponent->FaceFXActorInstance->GetActor();

			// Find the Group by name
			FxSize GroupIndex = fActor->FindAnimGroup(TCHAR_TO_ANSI(*GroupName));
			if(FxInvalidIndex != GroupIndex)
			{
				FxAnimGroup& fGroup = fActor->GetAnimGroup(GroupIndex);

				// Find the animation by name
				FxSize SeqIndex = fGroup.FindAnim(TCHAR_TO_ANSI(*SeqName));
				if(FxInvalidIndex != SeqIndex)
				{
					const FxAnim& fAnim = fGroup.GetAnim(SeqIndex);

					// Get the offset (will be 0.0 or negative)
					FLOAT StartOffset = fAnim.GetStartTime();

					// Force FaceFX to a particular point
					SkeletalMeshComponent->FaceFXActorInstance->ForceTick(TCHAR_TO_ANSI(*SeqName), TCHAR_TO_ANSI(*GroupName), InPosition + StartOffset);

					// Update the skeleton, morphs etc.
					// The FALSE here stops us from calling regular Tick on the FaceFXActorInstance, and clobbering the ForceTick we just did.
					SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
					SkeletalMeshComponent->ConditionalUpdateTransform();
				}
			}
		}
#endif
	}
	// Playback in Matinee case
	else
	{
		SkeletalMeshComponent->UpdateSkelPose();
		SkeletalMeshComponent->ConditionalUpdateTransform();
	}
}

/** Used by matinee playback to start a FaceFX animation playing. */
void ASkeletalMeshActor::PreviewActorPlayFaceFX(const FString& GroupName, const FString& SeqName)
{
	check(SkeletalMeshComponent);
	// FaceFXAnimSetRef set to NULL because matinee mounts sets during initialization
	SkeletalMeshComponent->PlayFaceFXAnim(NULL, SeqName, GroupName);
}

/** Used by matinee to stop current FaceFX animation playing. */
void ASkeletalMeshActor::PreviewActorStopFaceFX()
{
	check(SkeletalMeshComponent);
	SkeletalMeshComponent->StopFaceFXAnim();
}

/** Used in Matinee to get the AudioComponent we should play facial animation audio on. */
UAudioComponent* ASkeletalMeshActor::PreviewGetFaceFXAudioComponent()
{
	return FacialAudioComp;
}

/** Get the UFaceFXAsset that is currently being used by this Actor when playing facial animations. */
UFaceFXAsset* ASkeletalMeshActor::PreviewGetActorFaceFXAsset()
{
	check(SkeletalMeshComponent);
	if(SkeletalMeshComponent->SkeletalMesh)
	{
		return SkeletalMeshComponent->SkeletalMesh->FaceFXAsset;
	}

	return NULL;
}

/** Check SkeletalMeshActor for errors. */
void ASkeletalMeshActor::CheckForErrors()
{
	Super::CheckForErrors();

	if(SkeletalMeshComponent && SkeletalMeshComponent->SkeletalMesh && !SkeletalMeshComponent->PhysicsAsset)
	{
		GWarn->MapCheck_Add(MCTYPE_WARNING, this, TEXT("SkeletalMeshActor has no PhysicsAsset assigned."), MCACTION_NONE, TEXT("SkelMeshActorNoPhysAsset"));
	}
}


/*-----------------------------------------------------------------------------
	ASkeletalMeshActorMAT
-----------------------------------------------------------------------------*/

/** 
* Start AnimControl. Add required AnimSets. 
*/
void ASkeletalMeshActorMAT::MAT_BeginAnimControl(const TArray<class UAnimSet*>& InAnimSets)
{
	// Set the AnimSets array to what is passed in.
	SkeletalMeshComponent->AnimSets.Empty();
	SkeletalMeshComponent->AnimSets = InAnimSets;

	// Cache slot anim nodes, so we don't have to iterate through all anim nodes in the tree every time.
	SlotNodes.Empty();
	if( SkeletalMeshComponent->Animations )
	{
		TArray<UAnimNode*> Nodes;
		SkeletalMeshComponent->Animations->GetNodesByClass(Nodes, UAnimNodeSlot::StaticClass());

		for(INT i=0; i<Nodes.Num(); i++)
		{
			UAnimNodeSlot* SlotNode = Cast<UAnimNodeSlot>(Nodes(i));
			if( SlotNode )
			{
				SlotNodes.AddItem(SlotNode);
			}
		}
	}
	else
	{
		debugf(TEXT("MAT_BeginAnimControl, no AnimTree, couldn't cache slots!") );
	}
}


/** 
* Update AnimTree from track info 
*/
void ASkeletalMeshActorMAT::MAT_SetAnimPosition(FName SlotName, INT ChannelIndex, FName InAnimSeqName, FLOAT InPosition, UBOOL bFireNotifies, UBOOL bLooping)
{
	// Forward animation positions to slots. They will forward to relevant channels.
	for(INT i=0; i<SlotNodes.Num(); i++)
	{
		UAnimNodeSlot* SlotNode = SlotNodes(i);
		if( SlotNode && SlotNode->NodeName == SlotName )
		{	
			SlotNode->MAT_SetAnimPosition(ChannelIndex, InAnimSeqName, InPosition, bFireNotifies, bLooping);
		}
	}
}

/** 
* Update AnimTree from track weights 
*/
void ASkeletalMeshActorMAT::MAT_SetAnimWeights(const TArray<FAnimSlotInfo>& SlotInfos)
{
#if 0
	debugf( TEXT("-- SET ANIM WEIGHTS ---") );
	for(INT i=0; i<SlotInfos.Num(); i++)
	{
		debugf( TEXT("SLOT: %s"), *(SlotInfos(i).SlotName) );
		for(INT j=0; j<SlotInfos(i).ChannelWeights.Num(); j++)
		{
			debugf( TEXT("   CHANNEL %d: %1.3f"), j, SlotInfos(i).ChannelWeights(j) );
		}
	}
#endif

	// Forward channel weights to relevant slot(s)
	for(INT SlotInfoIdx=0; SlotInfoIdx<SlotInfos.Num(); SlotInfoIdx++)
	{
		const FAnimSlotInfo& SlotInfo = SlotInfos(SlotInfoIdx);

		for(INT SlotIdx=0; SlotIdx<SlotNodes.Num(); SlotIdx++)
		{
			UAnimNodeSlot* SlotNode = SlotNodes(SlotIdx);
			if( SlotNode && SlotNode->NodeName == SlotInfo.SlotName )
			{	
				SlotNode->MAT_SetAnimWeights(SlotInfo);
			}
		}
	}

}

/** 
* End AnimControl. Release required AnimSets 
*/
void ASkeletalMeshActorMAT::MAT_FinishAnimControl()
{
	// Empty the AnimSets array.
	SkeletalMeshComponent->AnimSets.Empty();

	// Empty SlotNodes cached references
	SlotNodes.Empty();
}

void ASkeletalMeshActorMAT::MAT_SetMorphWeight(FName MorphNodeName, FLOAT MorphWeight)
{
	if(SkeletalMeshComponent)
	{
		UMorphNodeWeight* WeightNode = Cast<UMorphNodeWeight>(SkeletalMeshComponent->FindMorphNode(MorphNodeName));
		if(WeightNode)
		{
			WeightNode->SetNodeWeight(MorphWeight);
		}
	}
}

void ASkeletalMeshActorMAT::MAT_SetSkelControlScale(FName SkelControlName, FLOAT Scale)
{
	if(SkeletalMeshComponent)
	{
		USkelControlBase* Control = SkeletalMeshComponent->FindSkelControl(SkelControlName);
		if(Control)
		{
			Control->BoneScale = Scale;
		}
	}
}

/* This is where we add information on slots and channels to the array that was passed in. */
void ASkeletalMeshActorMAT::GetAnimControlSlotDesc(TArray<struct FAnimSlotDesc>& OutSlotDescs)
{
	if( !SkeletalMeshComponent->Animations )
	{
		// fail
		appMsgf(AMT_OK, TEXT("SkeletalMeshActorMAT has no AnimTree Instance."));
		return;
	}

	// Find all AnimSlotNodes in the AnimTree
	for(INT i=0; i<SlotNodes.Num(); i++)
	{
		// Number of channels available on this slot node.
		const INT NumChannels = SlotNodes(i)->Children.Num() - 1;

		if( SlotNodes(i)->NodeName != NAME_None && NumChannels > 0 )
		{
			// Add a new element
			const INT Index = OutSlotDescs.Add(1);
			OutSlotDescs(Index).SlotName	= SlotNodes(i)->NodeName;
			OutSlotDescs(Index).NumChannels	= NumChannels;
		}
	}
}


/** 
* PreviewBeginAnimControl 
*/
void ASkeletalMeshActorMAT::PreviewBeginAnimControl(TArray<class UAnimSet*>& InAnimSets)
{
	// We need an AnimTree in Matinee in the editor to preview the animations, so instance one now if we don't have one.
	// This function can get called multiple times, but this is safe - will only instance the first time.
	if( !SkeletalMeshComponent->Animations && SkeletalMeshComponent->AnimTreeTemplate )
	{
		SkeletalMeshComponent->Animations = SkeletalMeshComponent->AnimTreeTemplate->CopyAnimTree(SkeletalMeshComponent);
	}

	// Add in AnimSets
	MAT_BeginAnimControl(InAnimSets);

	// Initialize tree
	SkeletalMeshComponent->InitAnimTree();
}

/** 
* PreviewSetAnimPosition 
*/
void ASkeletalMeshActorMAT::PreviewSetAnimPosition(FName SlotName, INT ChannelIndex, FName InAnimSeqName, FLOAT InPosition, UBOOL bLooping)
{
	MAT_SetAnimPosition(SlotName, ChannelIndex, InAnimSeqName, InPosition, FALSE, bLooping);

	// Update space bases so new animation position has an effect.
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

/** 
* PreviewSetAnimWeights 
*/
void ASkeletalMeshActorMAT::PreviewSetAnimWeights(TArray<FAnimSlotInfo>& SlotInfos)
{
	MAT_SetAnimWeights(SlotInfos);
}

/** 
* PreviewFinishAnimControl 
*/
void ASkeletalMeshActorMAT::PreviewFinishAnimControl()
{
	MAT_FinishAnimControl();

	// When done in MAtinee in the editor, drop the AnimTree instance.
	SkeletalMeshComponent->Animations = NULL;

	// Update space bases to reset it back to ref pose
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

void ASkeletalMeshActorMAT::PreviewSetMorphWeight(FName MorphNodeName, FLOAT MorphWeight)
{
	MAT_SetMorphWeight(MorphNodeName, MorphWeight);
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

void ASkeletalMeshActorMAT::PreviewSetSkelControlScale(FName SkelControlName, FLOAT Scale)
{
	MAT_SetSkelControlScale(SkelControlName, Scale);
	SkeletalMeshComponent->UpdateSkelPose(0.f, FALSE);
	SkeletalMeshComponent->ConditionalUpdateTransform();
}

/*-----------------------------------------------------------------------------
FSkeletalMeshSceneProxy
-----------------------------------------------------------------------------*/
#include "LevelUtils.h"
#include "UnSkeletalRender.h"

// Needed for FSkeletalMeshSceneProxy::DrawShadowVolumes().
#include "ScenePrivate.h"

/** 
 * Constructor. 
 * @param	Component - skeletal mesh primitive being added
 */
FSkeletalMeshSceneProxy::FSkeletalMeshSceneProxy(const USkeletalMeshComponent* Component)
 		:	FPrimitiveSceneProxy(Component)
		,	Owner(Component->GetOwner())
		,	SkeletalMesh(Component->SkeletalMesh)
		,	MeshObject(Component->MeshObject)
 		,	PhysicsAsset(Component->PhysicsAsset)
 		,	LevelColor(255,255,255)
		,	PropertyColor(255,255,255)
 		,	bCastShadow(Component->CastShadow)
 		,	bSelected(Component->IsOwnerSelected())
 		,	bShouldCollide(Component->ShouldCollide())
 		,	bDisplayBones(Component->bDisplayBones)
		,	bForceWireframe(Component->bForceWireframe)
		,	MaterialViewRelevance(Component->GetMaterialViewRelevance())
{
	check(MeshObject);
	check(SkeletalMesh);		  

	// setup materials and performance classification for each LOD.
	LODSections.AddZeroed(SkeletalMesh->LODModels.Num());
	for(INT i=0; i<SkeletalMesh->LODModels.Num(); i++)
	{
		const FStaticLODModel& LODModel = SkeletalMesh->LODModels(i);
		const FSkeletalMeshLODInfo& Info = SkeletalMesh->LODInfo(i);
		check( Info.bEnableShadowCasting.Num() == LODModel.Sections.Num() );

		FLODSectionElements& LODSection = LODSections(i);

		// Presize the array
		LODSection.SectionElements.Empty( LODModel.Sections.Num() );
		for(INT SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
		{
			const FSkelMeshSection& Section = LODModel.Sections(SectionIndex);

			// If we are at a dropped LOD, route material index through the LODMaterialMap in the LODInfo struct.
			INT UseMaterialIndex = Section.MaterialIndex;			
			if(i > 0)
			{
				if(Section.MaterialIndex < Info.LODMaterialMap.Num())
				{
					UseMaterialIndex = Info.LODMaterialMap(Section.MaterialIndex);
					UseMaterialIndex = ::Clamp( UseMaterialIndex, 0, SkeletalMesh->Materials.Num() );
				}
			}

            // If the material is NULL, or isn't flagged for use with skeletal meshes, it will be replaced by the default material.
			UMaterialInterface* Material = Component->GetMaterial(UseMaterialIndex);
			LODSection.SectionElements.AddItem(
				FSectionElementInfo(
					(Material && Material->UseWithSkeletalMesh()) ? Material : GEngine->DefaultMaterial,
					bCastShadow && Info.bEnableShadowCasting( SectionIndex )
					));
		}
	}

	// Try to find a color for level coloration.
	if( Owner )
	{
		ULevel* Level = Owner->GetLevel();
		ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel( Level );
		if ( LevelStreaming )
		{
			LevelColor = LevelStreaming->DrawColor;
		}
	}

	// Get a color for property coloration
	GEngine->GetPropertyColorationColor( (UObject*)Component, PropertyColor );
}

// FPrimitiveSceneProxy interface.

/**
 * Adds a decal interaction to the primitive.  This is called in the rendering thread by AddDecalInteraction_GameThread.
 */
void FSkeletalMeshSceneProxy::AddDecalInteraction_RenderingThread(const FDecalInteraction& DecalInteraction)
{
	FPrimitiveSceneProxy::AddDecalInteraction_RenderingThread( DecalInteraction );
	if ( MeshObject )
	{
		MeshObject->AddDecalInteraction_RenderingThread( DecalInteraction );
	}
}

/**
 * Removes a decal interaction from the primitive.  This is called in the rendering thread by RemoveDecalInteraction_GameThread.
 */
void FSkeletalMeshSceneProxy::RemoveDecalInteraction_RenderingThread(UDecalComponent* DecalComponent)
{
	FPrimitiveSceneProxy::RemoveDecalInteraction_RenderingThread( DecalComponent );
	if ( MeshObject )
	{
		MeshObject->RemoveDecalInteraction_RenderingThread( DecalComponent );
	}
}

/** Update the LOD level we want for this skeletal mesh (back on the game thread). */
void FSkeletalMeshSceneProxy::PreRenderView(const FSceneView* View, INT FrameNumber)
{
	if(MeshObject)
	{
		MeshObject->UpdateMinDesiredLODLevel(View, PrimitiveSceneInfo->Bounds, FrameNumber);
	}
}


/** 
* Draw the scene proxy as a dynamic element
*
* @param	PDI - draw interface to render to
* @param	View - current view
* @param	DPGIndex - current depth priority 
* @param	Flags - optional set of flags from EDrawDynamicElementFlags
*/
void FSkeletalMeshSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags)
{
	if( !MeshObject )
	{
		return;
	}		  

	const INT LODIndex = MeshObject->GetLOD();
	check(LODIndex < SkeletalMesh->LODModels.Num());
	const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
	const FLODSectionElements& LODSection = LODSections(LODIndex);

	// Determine the DPG the primitive should be drawn in for this view.
	if (GetDepthPriorityGroup(View) == DPGIndex)
	{
		if(IsCollisionView(View))
		{
			//JTODO: Hook up drawing physics asset under correct circumstances
		}
		else
		{
			check(SkeletalMesh->LODInfo.Num() == SkeletalMesh->LODModels.Num());
			check(LODSection.SectionElements.Num()==LODModel.Sections.Num());

			const UBOOL bIsCPUSkinned = SkeletalMesh->IsCPUSkinned();

			for(INT SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
			{
				const FSkelMeshSection& Section = LODModel.Sections(SectionIndex);
				const FSkelMeshChunk& Chunk = LODModel.Chunks(Section.ChunkIndex);
				const FSectionElementInfo& SectionElementInfo = LODSection.SectionElements(SectionIndex);

				FMeshElement Mesh;		
				Mesh.bWireframe |= bForceWireframe;

				const FIndexBuffer* DynamicIndexBuffer = MeshObject->GetDynamicIndexBuffer(LODIndex);
				//Tearing is only enabled when we are not welding the mesh.
				if(SkeletalMesh->bEnableClothTearing && (DynamicIndexBuffer != NULL) && (SkeletalMesh->ClothWeldingMap.Num() == 0))
				{
					Mesh.IndexBuffer = DynamicIndexBuffer;
				}
				else
				{
					Mesh.IndexBuffer = &LODModel.IndexBuffer;
				}

				Mesh.VertexFactory = MeshObject->GetVertexFactory(LODIndex,Section.ChunkIndex);
				Mesh.DynamicVertexData = NULL;
				Mesh.MaterialRenderProxy = SectionElementInfo.Material->GetRenderProxy(bSelected);
				Mesh.LCI = NULL;
				GetWorldMatrices( View, Mesh.LocalToWorld, Mesh.WorldToLocal );
				Mesh.FirstIndex = Section.BaseIndex;
				Mesh.NumPrimitives = Section.NumTriangles;
				Mesh.MinVertexIndex = Chunk.BaseVertexIndex;
				Mesh.MaxVertexIndex = LODModel.NumVertices - 1;
				Mesh.UseDynamicData = FALSE;
				Mesh.ReverseCulling = (LocalToWorldDeterminant < 0.0f);
				Mesh.CastShadow = SectionElementInfo.bEnableShadowCasting;
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)DPGIndex;

				const INT NumVertices = Chunk.NumRigidVertices + Chunk.NumSoftVertices;
				INC_DWORD_STAT_BY(STAT_GPUSkinVertices,(DWORD)(bIsCPUSkinned ? 0 : NumVertices));
				INC_DWORD_STAT_BY(STAT_SkelMeshTriangles,Mesh.NumPrimitives);

				DrawRichMesh(
					PDI,
					Mesh,
					FLinearColor::White,
					LevelColor,
					PropertyColor,
					PrimitiveSceneInfo,
					bSelected
					);
			}
		}
	}

	// debug drawing
	TArray<FMatrix>* BoneSpaceBases = MeshObject->GetSpaceBases();

	if( bDisplayBones )
	{
		if(BoneSpaceBases)
		{
			DebugDrawBones(PDI,View,*BoneSpaceBases, LODModel, FColor(230,230,255));
		}
	}
	if( PhysicsAsset )
	{
		DebugDrawPhysicsAsset(PDI,View);
	}

	// Draw per-poly collision data.
	if((View->Family->ShowFlags & SHOW_Collision) && BoneSpaceBases)
	{
		DebugDrawPerPolyCollision(PDI, *BoneSpaceBases);
	}

	// Draw bounds
	if( (View->Family->ShowFlags & SHOW_Bounds) && (View->Family->ShowFlags & SHOW_SkeletalMeshes) && (GIsGame || !Owner || Owner->IsSelected()) )
	{
		DebugDrawBounds(PDI,View);
	}
}

IMPLEMENT_COMPARE_CONSTPOINTER( FDecalInteraction, UnSkeletalMeshRender,
{
	return (A->DecalState.SortOrder <= B->DecalState.SortOrder) ? -1 : 1;
} );

/**
 * Draws the primitive's lit decal elements.  This is called from the rendering thread for each frame of each view.
 * The dynamic elements will only be rendered if GetViewRelevance declares dynamic relevance.
 * Called in the rendering thread.
 *
 * @param	Context					The RHI command context to which the primitives are being rendered.
 * @param	PDI						The interface which receives the primitive elements.
 * @param	View					The view which is being rendered.
 * @param	DepthPriorityGroup		The DPG which is being rendered.
 * @param	bDrawingDynamicLights	TRUE if drawing dynamic lights, FALSE if drawing static lights.
 */
void FSkeletalMeshSceneProxy::DrawLitDecalElements(FCommandContextRHI* Context,
												   FPrimitiveDrawInterface* PDI,
												   const FSceneView* View,
												   UINT DepthPriorityGroup,
												   UBOOL bDrawingDynamicLights)
{
	if( !MeshObject || !bDrawingDynamicLights )
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_DecalRenderTime);
	checkSlow( View->Family->ShowFlags & SHOW_Decals );

	const INT LODIndex = MeshObject->GetLOD();
	check(LODIndex < SkeletalMesh->LODModels.Num());
	const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
	const FLODSectionElements& LODSection = LODSections(LODIndex);

	// Determine the DPG the primitive should be drawn in for this view.
	if (GetViewRelevance(View).GetDPG(DepthPriorityGroup) == TRUE)
	{
		// Compute the set of decals in this DPG.
		TArray<FDecalInteraction*> DPGDecals;
		for ( INT DecalIndex = 0 ; DecalIndex < Decals.Num() ; ++DecalIndex )
		{
			FDecalInteraction* Interaction = Decals(DecalIndex);
			if ( DepthPriorityGroup == Interaction->DecalState.DepthPriorityGroup )
			{
				checkSlow( Interaction->DecalState.DecalMaterial );
				const UMaterial* DecalMaterial = Interaction->DecalState.DecalMaterial->GetMaterial();
				if ( DecalMaterial->LightingModel != MLM_Unlit )
				{
					DPGDecals.AddItem( Interaction );
				}
			}
		}

		if ( DPGDecals.Num() > 0 )
		{
			// Sort and render all decals.
			Sort<USE_COMPARE_CONSTPOINTER(FDecalInteraction,UnSkeletalMeshRender)>( DPGDecals.GetTypedData(), DPGDecals.Num() );

			check(SkeletalMesh->LODInfo.Num() == SkeletalMesh->LODModels.Num());
			check(LODSection.SectionElements.Num()==LODModel.Sections.Num());

			for ( INT DecalIndex = 0 ; DecalIndex < DPGDecals.Num() ; ++DecalIndex )
			{
				const FDecalInteraction* Decal	= DPGDecals(DecalIndex);
				const FDecalState& DecalState	= Decal->DecalState;

				// Compute a scissor rect by clipping the decal frustum vertices to the screen.
				// Don't draw the decal if the frustum projects off the screen.
				FVector2D MinCorner;
				FVector2D MaxCorner;
				if ( TRUE )//DecalState.QuadToClippedScreenSpaceAABB( View, MinCorner, MaxCorner ) )
				{
					/*
					// Don't override the light's scissor rect
					if (!bDrawingDynamicLights)
					{
						// Set the decal scissor rect.
						RHISetScissorRect( Context, TRUE, appTrunc(MinCorner.X), appTrunc(MinCorner.Y), appTrunc(MaxCorner.X), appTrunc(MaxCorner.Y) );
					}
					*/

					FMatrix DecalMatrix;
					FVector DecalLocation;
					FVector2D DecalOffset;
					MeshObject->TransformDecalState( DecalState, DecalMatrix, DecalLocation, DecalOffset );

					for(INT SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
					{
						RHISetBlendState(Context,TStaticBlendState<>::GetRHI());

						const FSkelMeshSection& Section = LODModel.Sections(SectionIndex);
						const FSkelMeshChunk& Chunk = LODModel.Chunks(Section.ChunkIndex);
						const FSectionElementInfo& SectionElementInfo = LODSection.SectionElements(SectionIndex);

						FMeshElement Mesh;		
						Mesh.bWireframe |= bForceWireframe;
						Mesh.IndexBuffer = &LODModel.IndexBuffer;

						FSkelMeshDecalVertexFactoryBase* DecalVertexFactory = MeshObject->GetDecalVertexFactory(LODIndex,Section.ChunkIndex,Decal);
						DecalVertexFactory->SetDecalMatrix( DecalMatrix );
						DecalVertexFactory->SetDecalLocation( DecalLocation );
						DecalVertexFactory->SetDecalOffset( DecalOffset );
						Mesh.VertexFactory = DecalVertexFactory->CastToFVertexFactory();

						Mesh.DynamicVertexData = NULL;
						Mesh.MaterialRenderProxy = DecalState.DecalMaterial->GetRenderProxy(FALSE);
						Mesh.LCI = NULL;
						GetWorldMatrices( View, Mesh.LocalToWorld, Mesh.WorldToLocal );
						Mesh.FirstIndex = Section.BaseIndex;
						Mesh.NumPrimitives = Section.NumTriangles;
						Mesh.MinVertexIndex = Chunk.BaseVertexIndex;
						Mesh.MaxVertexIndex = LODModel.NumVertices - 1;
						Mesh.UseDynamicData = FALSE;
						Mesh.ReverseCulling = (LocalToWorldDeterminant < 0.0f);
						Mesh.CastShadow = SectionElementInfo.bEnableShadowCasting;
						Mesh.Type = PT_TriangleList;
						Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)DepthPriorityGroup;

						INC_DWORD_STAT_BY(STAT_DecalTriangles,Mesh.NumPrimitives);
						INC_DWORD_STAT(STAT_DecalDrawCalls);
						DrawRichMesh(
							PDI,
							Mesh,
							FLinearColor(0.5f,1.0f,0.5f),
							LevelColor,
							PropertyColor,
							PrimitiveSceneInfo,
							FALSE
							);
					}
				}
				
				/*
				if (!bDrawingDynamicLights)
				{
					// Restore the scissor rect.
					RHISetScissorRect( Context, FALSE, 0, 0, 0, 0 );
				}
				*/
			}
		}
	}
}

/**
 * Draws the primitive's decal elements.  This is called from the rendering thread for each frame of each view.
 * The dynamic elements will only be rendered if GetViewRelevance declares decal relevance.
 * Called in the rendering thread.
 *
 * @param	Context							The RHI command context to which the primitives are being rendered.
 * @param	OpaquePDI						The interface which receives the opaque primitive elements.
 * @param	TranslucentPDI					The interface which receives the translucent primitive elements.
 * @param	View							The view which is being rendered.
 * @param	DepthPriorityGroup				The DPG which is being rendered.
 * @param	bTranslucentReceiverPass		TRUE during the decal pass for translucent receivers, FALSE for opaque receivers.
 */
void FSkeletalMeshSceneProxy::DrawDecalElements(FCommandContextRHI* Context, 
												FPrimitiveDrawInterface* OpaquePDI, 
												FPrimitiveDrawInterface* TranslucentPDI, 
												const FSceneView* View, 
												UINT DepthPriorityGroup, 
												UBOOL bTranslucentReceiverPass)
{
	if( !MeshObject )
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_DecalRenderTime);
	checkSlow( View->Family->ShowFlags & SHOW_Decals );

	if ( (!bTranslucentReceiverPass && MaterialViewRelevance.bTranslucency) || (bTranslucentReceiverPass && !MaterialViewRelevance.bTranslucency) )
	{
		return;
	}

	const INT LODIndex = MeshObject->GetLOD();
	check(LODIndex < SkeletalMesh->LODModels.Num());
	const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
	const FLODSectionElements& LODSection = LODSections(LODIndex);

	// Determine the DPG the primitive should be drawn in for this view.
	if (GetViewRelevance(View).GetDPG(DepthPriorityGroup) == TRUE)
	{
		// Compute the set of decals in this DPG.
		TArray<FDecalInteraction*> DPGDecals;
		for ( INT DecalIndex = 0 ; DecalIndex < Decals.Num() ; ++DecalIndex )
		{
			FDecalInteraction* Interaction = Decals(DecalIndex);
			if ( DepthPriorityGroup == Interaction->DecalState.DepthPriorityGroup )
			{
				checkSlow( Interaction->DecalState.DecalMaterial );
				const UMaterial* DecalMaterial = Interaction->DecalState.DecalMaterial->GetMaterial();
				if ( DecalMaterial->LightingModel == MLM_Unlit )
				{
					DPGDecals.AddItem( Interaction );
				}
			}
		}

		if ( DPGDecals.Num() > 0 )
		{
			// Sort and render all decals.
			Sort<USE_COMPARE_CONSTPOINTER(FDecalInteraction,UnSkeletalMeshRender)>( DPGDecals.GetTypedData(), DPGDecals.Num() );

			check(SkeletalMesh->LODInfo.Num() == SkeletalMesh->LODModels.Num());
			check(LODSection.SectionElements.Num()==LODModel.Sections.Num());

			for ( INT DecalIndex = 0 ; DecalIndex < DPGDecals.Num() ; ++DecalIndex )
			{
				const FDecalInteraction* Decal	= DPGDecals(DecalIndex);
				const FDecalState& DecalState	= Decal->DecalState;

				// Compute a scissor rect by clipping the decal frustum vertices to the screen.
				// Don't draw the decal if the frustum projects off the screen.
				FVector2D MinCorner;
				FVector2D MaxCorner;
				if ( TRUE ) //DecalState.QuadToClippedScreenSpaceAABB( View, MinCorner, MaxCorner ) )
				{
					// Set the decal scissor rect.
					//RHISetScissorRect( Context, TRUE, appTrunc(MinCorner.X), appTrunc(MinCorner.Y), appTrunc(MaxCorner.X), appTrunc(MaxCorner.Y) );

					FPrimitiveDrawInterface* PDI;
					if ( Decal->DecalState.MaterialViewRelevance.bTranslucency )
					{
						PDI = TranslucentPDI;
					}
					else
					{
						RHISetBlendState( Context, TStaticBlendState<>::GetRHI() );
						PDI = OpaquePDI;
					}

					FMatrix DecalMatrix;
					FVector DecalLocation;
					FVector2D DecalOffset;
					MeshObject->TransformDecalState( DecalState, DecalMatrix, DecalLocation, DecalOffset );

					for(INT SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
					{
						const FSkelMeshSection& Section = LODModel.Sections(SectionIndex);
						const FSkelMeshChunk& Chunk = LODModel.Chunks(Section.ChunkIndex);
						const FSectionElementInfo& SectionElementInfo = LODSection.SectionElements(SectionIndex);

						FMeshElement Mesh;		
						Mesh.bWireframe |= bForceWireframe;
						Mesh.IndexBuffer = &LODModel.IndexBuffer;

						FSkelMeshDecalVertexFactoryBase* DecalVertexFactory = MeshObject->GetDecalVertexFactory(LODIndex,Section.ChunkIndex,Decal);
						DecalVertexFactory->SetDecalMatrix( DecalMatrix );
						DecalVertexFactory->SetDecalLocation( DecalLocation );
						DecalVertexFactory->SetDecalOffset( DecalOffset );
						Mesh.VertexFactory = DecalVertexFactory->CastToFVertexFactory();

						Mesh.DynamicVertexData = NULL;
						Mesh.MaterialRenderProxy = DecalState.DecalMaterial->GetRenderProxy(FALSE);
						Mesh.LCI = NULL;
						GetWorldMatrices( View, Mesh.LocalToWorld, Mesh.WorldToLocal );
						Mesh.FirstIndex = Section.BaseIndex;
						Mesh.NumPrimitives = Section.NumTriangles;
						Mesh.MinVertexIndex = Chunk.BaseVertexIndex;
						Mesh.MaxVertexIndex = LODModel.NumVertices - 1;
						Mesh.UseDynamicData = FALSE;
						Mesh.ReverseCulling = (LocalToWorldDeterminant < 0.0f);
						Mesh.CastShadow = SectionElementInfo.bEnableShadowCasting;
						Mesh.Type = PT_TriangleList;
						Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)DepthPriorityGroup;

						INC_DWORD_STAT_BY(STAT_DecalTriangles,Mesh.NumPrimitives);
						INC_DWORD_STAT(STAT_DecalDrawCalls);
						DrawRichMesh(
							PDI,
							Mesh,
							FLinearColor(0.5f,1.0f,0.5f),
							LevelColor,
							PropertyColor,
							PrimitiveSceneInfo,
							FALSE
							);
					}
				}

				// Restore the scissor rect.
				//RHISetScissorRect( Context, FALSE, 0, 0, 0, 0 );
			}
		}
	}
}

/**
 * Draws the primitive's shadow volumes.  This is called from the rendering thread,
 * during the FSceneRenderer::RenderLights phase.
 * @param SVDI - The interface which performs the actual rendering of a shadow volume.
 * @param View - The view which is being rendered.
 * @param Light - The light for which shadows should be drawn.
 */
void FSkeletalMeshSceneProxy::DrawShadowVolumes(FShadowVolumeDrawInterface* SVDI,const FSceneView* View,const FLightSceneInfo* Light,UINT DPGIndex)
{
	checkSlow(UEngine::ShadowVolumesAllowed());

	// Determine the DPG the primitive should be drawn in for this view.
	if (!MeshObject || (GetViewRelevance(View).GetDPG(DPGIndex) == FALSE))
	{
		return;
	}

	INT LODIndex = MeshObject->GetLOD();
	check(LODIndex < SkeletalMesh->LODModels.Num());

	check(SkeletalMesh->LODInfo.Num() == SkeletalMesh->LODModels.Num());
	const FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);
	const FLODSectionElements& LODSection = LODSections(LODIndex);
	check(LODSection.SectionElements.Num() == LODModel.Sections.Num());
	if( LODModel.ShadowIndices.Num() == 0 ||
		LODModel.Edges.Num() == 0 )
	{
		return;
	}

	// Make sure the vertices have been CPU skinned and copied into a FShadowVertexBuffer.
    MeshObject->CacheVertices(LODIndex, TRUE, TRUE, FALSE);

	FMatrix LocalToWorld, WorldToLocal;
	GetWorldMatrices( View, LocalToWorld, WorldToLocal );

	// Find the homogenous light position in local space.
	FVector4 LightPosition = WorldToLocal.TransformFVector4(Light->GetPosition());

	// Find the orientation of the triangles relative to the light position.
	FLOAT*	PlaneDots = new FLOAT[LODModel.ShadowIndices.Num() / 3];
	MeshObject->GetPlaneDots(PlaneDots,LightPosition,LODIndex);

	// Extrude a shadow volume.
	FShadowIndexBuffer	IndexBuffer;
	WORD				FirstExtrudedVertex = LODModel.NumVertices;

	IndexBuffer.Indices.Empty(LODModel.ShadowIndices.Num() * 2);

	for(INT TriangleIndex = 0;TriangleIndex < LODModel.ShadowIndices.Num() / 3;TriangleIndex++)
	{
		if(IsNegativeFloat(PlaneDots[TriangleIndex]))
		{
			const WORD*	TriangleIndices = &LODModel.ShadowIndices(TriangleIndex * 3);

			IndexBuffer.AddFace(
				FirstExtrudedVertex + TriangleIndices[0],
				FirstExtrudedVertex + TriangleIndices[1],
				FirstExtrudedVertex + TriangleIndices[2]
				);

			IndexBuffer.AddFace(
				TriangleIndices[2],
				TriangleIndices[1],
				TriangleIndices[0]
				);
		}
	}

	for(UINT EdgeIndex = 0;EdgeIndex < (UINT)LODModel.Edges.Num();EdgeIndex++)
	{
		const FMeshEdge& Edge = LODModel.Edges(EdgeIndex);
		if(	(Edge.Faces[1] == INDEX_NONE && IsNegativeFloat(PlaneDots[Edge.Faces[0]])) ||
			(Edge.Faces[1] != INDEX_NONE && IsNegativeFloat(PlaneDots[Edge.Faces[0]]) != IsNegativeFloat(PlaneDots[Edge.Faces[1]])))
		{
			IndexBuffer.AddEdge(
				Edge.Vertices[IsNegativeFloat(PlaneDots[Edge.Faces[0]]) ? 1 : 0],
				Edge.Vertices[IsNegativeFloat(PlaneDots[Edge.Faces[0]]) ? 0 : 1],
				FirstExtrudedVertex
				);
		}
	}

	delete [] PlaneDots;

	// create the shadow volume index buffer
	DWORD Size = IndexBuffer.Indices.Num() * sizeof(WORD);
	FIndexBufferRHIRef RHIIndexBuffer = RHICreateIndexBuffer( sizeof(WORD), Size, NULL, FALSE /*TRUE*/ );

	// Initialize the buffer.
	void* Buffer = RHILockIndexBuffer(RHIIndexBuffer,0,Size);
	appMemcpy(Buffer,&IndexBuffer.Indices(0),Size);
	RHIUnlockIndexBuffer(RHIIndexBuffer);

	SVDI->DrawShadowVolume( RHIIndexBuffer, MeshObject->GetShadowVertexFactory(LODIndex), LocalToWorld, 0, IndexBuffer.Indices.Num()/3, 0, LODModel.NumVertices * 2 - 1 );
}

/**
 * Returns the world transform to use for drawing.
 * @param View - Current view
 * @param OutLocalToWorld - Will contain the local-to-world transform when the function returns.
 * @param OutWorldToLocal - Will contain the world-to-local transform when the function returns.
 */
void FSkeletalMeshSceneProxy::GetWorldMatrices( const FSceneView* View, FMatrix& OutLocalToWorld, FMatrix& OutWorldToLocal )
{
	OutLocalToWorld = LocalToWorld;
	OutWorldToLocal = LocalToWorld.Inverse();
}

/**
 * Relevance is always dynamic for skel meshes unless they are disabled
 */
FPrimitiveViewRelevance FSkeletalMeshSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	if (View->Family->ShowFlags & SHOW_SkeletalMeshes)
	{
		if(IsShown(View))
		{
			Result.bDynamicRelevance = TRUE;
			Result.SetDPG(GetDepthPriorityGroup(View),TRUE);

			// only add to foreground DPG for debug rendering
			if( bDisplayBones ||
				View->Family->ShowFlags & (SHOW_Bounds|SHOW_Collision) )
			{
				Result.SetDPG(SDPG_Foreground,TRUE);
			}		
			Result.bDecalRelevance = HasRelevantDecals(View);
		}
		if (IsShadowCast(View))
		{
			Result.bShadowRelevance = TRUE;
		}
		MaterialViewRelevance.SetPrimitiveViewRelevance(Result);
	}
	return Result;
}

/** Util for getting LOD index currently used by this SceneProxy. */
INT FSkeletalMeshSceneProxy::GetCurrentLODIndex()
{
	if(MeshObject)
	{
		return MeshObject->GetLOD();
	}
	else
	{
		return 0;
	}
}

/** 
 * Render bones for debug display
 */
void FSkeletalMeshSceneProxy::DebugDrawBones(FPrimitiveDrawInterface* PDI,const FSceneView* View, const TArray<FMatrix>& InSpaceBases, const class FStaticLODModel& LODModel, const FColor& LineColor)
{
	FMatrix LocalToWorld, WorldToLocal;
	GetWorldMatrices( View, LocalToWorld, WorldToLocal );

	TArray<FMatrix> WorldBases;
	WorldBases.Add( InSpaceBases.Num() );
	for(INT i=0; i<LODModel.RequiredBones.Num(); i++)
	{
		INT BoneIndex = LODModel.RequiredBones(i);
		check(BoneIndex < InSpaceBases.Num());
		// transform bone mats to world space
		WorldBases(BoneIndex) = InSpaceBases(BoneIndex) * LocalToWorld;
		if( BoneIndex == 0 )
		{
			PDI->DrawLine(WorldBases(BoneIndex).GetOrigin(), LocalToWorld.GetOrigin(), FColor(255, 0, 255), SDPG_Foreground);
		}
		else
		{
			INT ParentIdx = SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;
			PDI->DrawLine(WorldBases(BoneIndex).GetOrigin(), WorldBases(ParentIdx).GetOrigin(), LineColor, SDPG_Foreground);
		}
		// Display colored coordinate system axes for each joint.			  
		// Red = X
		FVector XAxis =  WorldBases(BoneIndex).TransformNormal( FVector(1.0f,0.0f,0.0f));
		XAxis.Normalize();
		PDI->DrawLine(  WorldBases(BoneIndex).GetOrigin(),  WorldBases(BoneIndex).GetOrigin() + XAxis * 3.75f, FColor( 255, 80, 80), SDPG_Foreground );			
		// Green = Y
		FVector YAxis =  WorldBases(BoneIndex).TransformNormal( FVector(0.0f,1.0f,0.0f));
		YAxis.Normalize();
		PDI->DrawLine(  WorldBases(BoneIndex).GetOrigin(),  WorldBases(BoneIndex).GetOrigin() + YAxis * 3.75f, FColor( 80, 255, 80), SDPG_Foreground ); 
		// Blue = Z
		FVector ZAxis =  WorldBases(BoneIndex).TransformNormal( FVector(0.0f,0.0f,1.0f));
		ZAxis.Normalize();
		PDI->DrawLine(  WorldBases(BoneIndex).GetOrigin(),  WorldBases(BoneIndex).GetOrigin() + ZAxis * 3.75f, FColor( 80, 80, 255), SDPG_Foreground ); 
	}
}

/** 
 * Render physics asset for debug display
 */
void FSkeletalMeshSceneProxy::DebugDrawPhysicsAsset(FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	FVector TotalScale(1.f);
	if( Owner )
	{
		TotalScale = Owner->DrawScale * Owner->DrawScale3D;
	}

	// Only valid if scaling if uniform.
	if( TotalScale.IsUniform() )
	{
		FMatrix LocalToWorld, WorldToLocal;
		GetWorldMatrices( View, LocalToWorld, WorldToLocal );

		TArray<FMatrix>* BoneSpaceBases = MeshObject->GetSpaceBases();
		if(BoneSpaceBases)
		{
			const EShowFlags ShowFlags = View->Family->ShowFlags;
			check(PhysicsAsset);
			if( (ShowFlags & SHOW_Collision) && bShouldCollide )
			{
				PhysicsAsset->DrawCollision(PDI, SkeletalMesh, *BoneSpaceBases, LocalToWorld, TotalScale.X);
			}
			if( ShowFlags & SHOW_Constraints )
			{
				PhysicsAsset->DrawConstraints(PDI, SkeletalMesh, *BoneSpaceBases, LocalToWorld, TotalScale.X);
			}
		}
	}


}

/** Render any per-poly collision data for tri's rigidly weighted to bones. */
void FSkeletalMeshSceneProxy::DebugDrawPerPolyCollision(FPrimitiveDrawInterface* PDI, const TArray<FMatrix>& InSpaceBases)
{
	check(SkeletalMesh->PerPolyCollisionBones.Num() == SkeletalMesh->PerPolyBoneKDOPs.Num());
	// Iterate over each bone with per-poly collision data
	for(INT BoneIdx=0; BoneIdx<SkeletalMesh->PerPolyBoneKDOPs.Num(); BoneIdx++)
	{
		// Pick a different colour for each bone.
		FColor BoneColor = DebugUtilColor[BoneIdx%NUM_DEBUG_UTIL_COLORS];
		const FPerPolyBoneCollisionData& Data = SkeletalMesh->PerPolyBoneKDOPs(BoneIdx);
		INT BoneIndex = SkeletalMesh->MatchRefBone(SkeletalMesh->PerPolyCollisionBones(BoneIdx));
		if(BoneIndex != INDEX_NONE)
		{
			// Calculate transform from bone-space to world-space
			FMatrix BoneToWorld = InSpaceBases(BoneIndex) * LocalToWorld;
			// Iterate over each triangle
			for(INT TriIndex=0; TriIndex<Data.KDOPTree.Triangles.Num(); TriIndex++)
			{
				const FkDOPCollisionTriangle<WORD>& Tri = Data.KDOPTree.Triangles(TriIndex);

				// TRansform verts into world space
				FVector WorldVert1 = BoneToWorld.TransformFVector( Data.CollisionVerts(Tri.v1) );
				FVector WorldVert2 = BoneToWorld.TransformFVector( Data.CollisionVerts(Tri.v2) );
				FVector WorldVert3 = BoneToWorld.TransformFVector( Data.CollisionVerts(Tri.v3) );

				// Draw wire triangle
				PDI->DrawLine( WorldVert1, WorldVert2, BoneColor, SDPG_World);
				PDI->DrawLine( WorldVert2, WorldVert3, BoneColor, SDPG_World);
				PDI->DrawLine( WorldVert3, WorldVert1, BoneColor, SDPG_World);
			}
		}
	}
}

/** 
	* Render bounds for debug display
	*/
void FSkeletalMeshSceneProxy::DebugDrawBounds(FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	// Draw bounding wireframe box.
	DrawWireBox(PDI, PrimitiveSceneInfo->Bounds.GetBox(), FColor(72,72,255),SDPG_Foreground);
	// Draw bounding spheres
	DrawCircle(PDI, PrimitiveSceneInfo->Bounds.Origin,FVector(1,0,0),FVector(0,1,0),FColor(255,255,0),PrimitiveSceneInfo->Bounds.SphereRadius,32,SDPG_Foreground);
	DrawCircle(PDI, PrimitiveSceneInfo->Bounds.Origin,FVector(1,0,0),FVector(0,0,1),FColor(255,255,0),PrimitiveSceneInfo->Bounds.SphereRadius,32,SDPG_Foreground);
	DrawCircle(PDI, PrimitiveSceneInfo->Bounds.Origin,FVector(0,1,0),FVector(0,0,1),FColor(255,255,0),PrimitiveSceneInfo->Bounds.SphereRadius,32,SDPG_Foreground);
}

/** Create the scene proxy needed for rendering a skeletal mesh */
FPrimitiveSceneProxy* USkeletalMeshComponent::CreateSceneProxy()
{
	FSkeletalMeshSceneProxy* Result = NULL;

	// only create a scene proxy for rendering if
	// properly initialized
	if( SkeletalMesh && 
		SkeletalMesh->LODModels.IsValidIndex(PredictedLODLevel) &&
		!bHideSkin &&
		MeshObject )
	{
		Result = ::new FSkeletalMeshSceneProxy(this);
	}

	return Result;
}


/*-----------------------------------------------------------------------------
	The end.
-----------------------------------------------------------------------------*/

