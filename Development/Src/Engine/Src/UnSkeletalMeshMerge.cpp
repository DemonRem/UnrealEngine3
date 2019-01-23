/*=============================================================================
	UnSkeletalMeshMerge.cpp: Unreal skeletal mesh merging implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnSkeletalMeshMerge.h"

/*-----------------------------------------------------------------------------
	FSkeletalMeshMerge
-----------------------------------------------------------------------------*/

/**
* Constructor
* @param InMergeMesh - destination mesh to merge to
* @param InSrcMeshList - array of source meshes to merge
* @param InForceSectionMapping - optional array to map sections from the source meshes to merged section entries
*/
FSkeletalMeshMerge::FSkeletalMeshMerge(USkeletalMesh* InMergeMesh, 
									   const TArray<USkeletalMesh*>& InSrcMeshList, 
									   const TArray<FSkelMeshMergeSectionMapping>& InForceSectionMapping)
:	MergeMesh(InMergeMesh)
,	SrcMeshList(InSrcMeshList)
,	ForceSectionMapping(InForceSectionMapping)
{
	check(MergeMesh);
}

/** Util for finding index of bone in skeleton. */
INT FindBoneIndex(const TArray<FMeshBone>& RefSkeleton, const FName BoneName)
{
	for(INT i=0; i<RefSkeleton.Num(); i++)
	{
		if(RefSkeleton(i).Name == BoneName)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

/**
* Merge/Composite the list of source meshes onto the merge one
* The MergeMesh is reinitialized 
* @return TRUE if succeeded
*/
UBOOL FSkeletalMeshMerge::DoMerge()
{
	UBOOL Result=TRUE;

	if( MergeMesh->LODModels.Num() > 0 )
	{
		// Release the mesh's render resources.
		MergeMesh->ReleaseResources();
		// This will block for the resources to be released on the render thread
		MergeMesh->ReleaseResourcesFence.Wait();
	}

	// find common max num of LODs available in the list of source meshes
	INT MaxNumLODs = INDEX_NONE;
	UBOOL bMaxNumLODsInit=TRUE;
	NewRefSkeleton.Empty();

	SrcMeshInfo.Empty();
	SrcMeshInfo.AddZeroed(SrcMeshList.Num());
	for( INT MeshIdx=0; MeshIdx < SrcMeshList.Num(); MeshIdx++ )
	{
		USkeletalMesh* SrcMesh = SrcMeshList(MeshIdx);
		if( SrcMesh )
		{
			if( bMaxNumLODsInit )
			{
				// initialize
				MaxNumLODs = SrcMesh->LODInfo.Num();
				bMaxNumLODsInit=FALSE;
			}
			else
			{
				// keep track of the smallest number of LODs 
				MaxNumLODs = Max<INT>( MaxNumLODs, SrcMesh->LODInfo.Num() );
			}

			// Initialise new RefSkeleton with first supplied mesh.
			if(NewRefSkeleton.Num() == 0)
			{
				NewRefSkeleton = SrcMesh->RefSkeleton;
			}
			// For subsequent ones, add any missing bones.
			else
			{			
				for(INT i=0; i<SrcMesh->RefSkeleton.Num(); i++)
				{
					FName SrcBoneName = SrcMesh->RefSkeleton(i).Name;
					INT DestBoneIndex = FindBoneIndex(NewRefSkeleton, SrcBoneName);
					// If it's not present, we add it just after its parent.
					if(DestBoneIndex == INDEX_NONE)
					{
						check(i != 0); // Must have at least root shared between each mesh!
						INT ParentBoneIndex = SrcMesh->RefSkeleton(i).ParentIndex;
						FName ParentBoneName = SrcMesh->RefSkeleton(ParentBoneIndex).Name;
						INT DestParentIndex = FindBoneIndex(NewRefSkeleton, ParentBoneName);
						check(DestParentIndex != INDEX_NONE); // Shouldn't be any way we are missing the parent - parents are always before children				
						INT NewBoneIndex = DestParentIndex+1;

						NewRefSkeleton.InsertItem(SrcMesh->RefSkeleton(i), NewBoneIndex);
						NewRefSkeleton(NewBoneIndex).ParentIndex = DestParentIndex;

						// Now we need to fix all the parent indices that pointed to bones after this in the array
						// These must be after this point in the array.
						for(INT j=NewBoneIndex+1; j<NewRefSkeleton.Num(); j++)
						{
							if(NewRefSkeleton(j).ParentIndex >= NewBoneIndex)
							{
								NewRefSkeleton(j).ParentIndex += 1;
							}
						}
					}
				}
			}
		}
	}

#if 0
	for(INT i=0; i<NewRefSkeleton.Num(); i++)
	{
		FMeshBone& MeshBone = NewRefSkeleton(i);

		INT Depth = 0;
		INT TmpBoneIndex = i;
		while( TmpBoneIndex != 0 )
		{
			TmpBoneIndex = NewRefSkeleton(TmpBoneIndex).ParentIndex;
			Depth++;
		}

		FString LeadingSpace;
		for(INT j=0; j<Depth; j++)
		{
			LeadingSpace += TEXT(" ");
		}

		if( i==0 )
		{
			debugf( TEXT("%3d: %s%s\r\n"), i, *LeadingSpace, *MeshBone.Name.ToString());
		}
		else
		{
			debugf( TEXT("%3d: %s%s (ParentBoneID: %d)\r\n"), i, *LeadingSpace, *MeshBone.Name.ToString(), MeshBone.ParentIndex );
		}
	}
#endif

	// Now new RefSkeleton is complete, create mapping from each input mesh bones to bones in the destination mesh.
	for( INT MeshIdx=0; MeshIdx < SrcMeshList.Num(); MeshIdx++ )
	{
		USkeletalMesh* SrcMesh = SrcMeshList(MeshIdx);
		if(SrcMesh)
		{
			FMergeMeshInfo& MeshInfo = SrcMeshInfo(MeshIdx);
			MeshInfo.SrcToDestRefSkeletonMap.Add(SrcMesh->RefSkeleton.Num());

			for(INT i=0; i<SrcMesh->RefSkeleton.Num(); i++)
			{
				FName SrcBoneName = SrcMesh->RefSkeleton(i).Name;
				INT DestBoneIndex = FindBoneIndex(NewRefSkeleton, SrcBoneName);
				check(DestBoneIndex != INDEX_NONE); // All bones should be here now!
				MeshInfo.SrcToDestRefSkeletonMap(i) = DestBoneIndex;
			}
		}
	}

	// If things are going ok so far...
	if(Result)
	{
		if( MaxNumLODs == INDEX_NONE )
		{
			debugf( NAME_Warning, TEXT("FSkeletalMeshMerge: Invalid source mesh list") );
			Result = FALSE;
		}
		else
		{
			// initialize merge mesh
			MergeMesh->LODInfo.Empty(MaxNumLODs);
			MergeMesh->LODModels.Empty(MaxNumLODs);
			MergeMesh->Materials.Empty();

			// process each LOD for the new merged mesh
			for( INT LODIdx=0; LODIdx < MaxNumLODs; LODIdx++ )
			{
				GenerateLODModel( LODIdx );
			}
			// update the merge skel mesh entries
			if( !ProcessMergeMesh() )
			{
				Result = FALSE;
			}

			// Reinitialize the mesh's render resources.
			MergeMesh->InitResources();
		}
	}

	return Result;
}

/**
* Merge a bonemap with an existing bonemap and keep track of remapping
* (a bonemap is a list of indices of bones in the USkeletalMesh::RefSkeleton array)
* @param MergedBoneMap - out merged bonemap
* @param BoneMapToMergedBoneMap - out of mapping from original bonemap to new merged bonemap 
* @param BoneMap - input bonemap to merge
*/
void FSkeletalMeshMerge::MergeBoneMap( TArray<WORD>& MergedBoneMap, TArray<WORD>& BoneMapToMergedBoneMap, const TArray<WORD>& BoneMap )
{
	BoneMapToMergedBoneMap.Add( BoneMap.Num() );
	for( INT IdxB=0; IdxB < BoneMap.Num(); IdxB++ )
	{
		BoneMapToMergedBoneMap(IdxB) = MergedBoneMap.AddUniqueItem( BoneMap(IdxB) );
	}
}

static void BoneMapToNewRefSkel(const TArray<WORD>& InBoneMap, const TArray<INT>& SrcToDestRefSkeletonMap, TArray<WORD>& OutBoneMap)
{
	OutBoneMap.Empty();
	OutBoneMap.Add(InBoneMap.Num());

	for(INT i=0; i<InBoneMap.Num(); i++)
	{
		check(InBoneMap(i) < SrcToDestRefSkeletonMap.Num());
		OutBoneMap(i) = SrcToDestRefSkeletonMap(InBoneMap(i));
	}
}

/**
* Generate the list of sections that need to be created along with info needed to merge sections
* @param NewSectionArray - out array to populate
* @param LODIdx - current LOD to process
*/
void FSkeletalMeshMerge::GenerateNewSectionArray( TArray<FNewSectionInfo>& NewSectionArray, INT LODIdx )
{
	NewSectionArray.Empty();
	for( INT MeshIdx=0; MeshIdx < SrcMeshList.Num(); MeshIdx++ )
	{
		// source mesh
		USkeletalMesh* SrcMesh = SrcMeshList(MeshIdx);

		if( SrcMesh )
		{
			INT SourceLODIdx = Min(LODIdx, SrcMesh->LODModels.Num()-1);
			FStaticLODModel& SrcLODModel = SrcMesh->LODModels(SourceLODIdx);
			FSkeletalMeshLODInfo& SrcLODInfo = SrcMesh->LODInfo(SourceLODIdx);

			// iterate over each section of this LOD
			for( INT SectionIdx=0; SectionIdx < SrcLODModel.Sections.Num(); SectionIdx++ )
			{
				INT MaterialId = -1;
				// check for the optional list of material ids corresponding to the list of src meshes
				// if the id is valid (not -1) it is used to find an existing section entry to merge with
				if( ForceSectionMapping.Num() == SrcMeshList.Num() &&
					ForceSectionMapping.IsValidIndex(MeshIdx) &&
					ForceSectionMapping(MeshIdx).SectionIDs.IsValidIndex(SectionIdx) )
				{
					MaterialId = ForceSectionMapping(MeshIdx).SectionIDs(SectionIdx);
				}

				FSkelMeshSection& Section = SrcLODModel.Sections(SectionIdx);
				FSkelMeshChunk& Chunk = SrcLODModel.Chunks(Section.ChunkIndex);

				// Convert Chunk.BoneMap from src to dest bone indices
				TArray<WORD> DestChunkBoneMap;
				BoneMapToNewRefSkel(Chunk.BoneMap, SrcMeshInfo(MeshIdx).SrcToDestRefSkeletonMap, DestChunkBoneMap);


				// get the material for this section
				INT MaterialIndex = Section.MaterialIndex;
				// use the remapping of material indices for all LODs besides the base LOD 
				if( LODIdx > 0 && 
					SrcLODInfo.LODMaterialMap.IsValidIndex(Section.MaterialIndex) )
				{
					MaterialIndex = Clamp<INT>( SrcLODInfo.LODMaterialMap(Section.MaterialIndex), 0, SrcMesh->Materials.Num() );
				}
				UMaterialInterface* Material = SrcMesh->Materials(MaterialIndex);

				// see if there is an existing entry in the array of new sections that matches its material
				// if there is a match then the source section can be added to its list of sections to merge 
				INT FoundIdx = INDEX_NONE;
				for( INT Idx=0; Idx < NewSectionArray.Num(); Idx++ )
				{
					FNewSectionInfo& NewSectionInfo = NewSectionArray(Idx);
					// check for a matching material or a matching material index id if it is valid
					if( (MaterialId == -1 && Material == NewSectionInfo.Material) ||
						(MaterialId != -1 && MaterialId == NewSectionInfo.MaterialId) )
					{
						check(NewSectionInfo.MergeSections.Num());

						// merge the bonemap from the source section with the existing merged bonemap
						TArray<WORD> TempMergedBoneMap(NewSectionInfo.MergedBoneMap);
						TArray<WORD> TempBoneMapToMergedBoneMap;									
						MergeBoneMap(TempMergedBoneMap,TempBoneMapToMergedBoneMap,DestChunkBoneMap);

						// check to see if the newly merged bonemap is still within the bone limit for GPU skinning
						if( TempMergedBoneMap.Num() <= MAX_GPUSKIN_BONES )
						{
							// add the source section as a new merge entry
							FMergeSectionInfo& MergeSectionInfo = *new(NewSectionInfo.MergeSections) FMergeSectionInfo(
								SrcMesh,
								&SrcLODModel.Sections(SectionIdx),
								&SrcLODModel.Chunks(Section.ChunkIndex)
								);
							// keep track of remapping for the existing chunk's bonemap 
							// so that the bone matrix indices can be updated for the vertices
							MergeSectionInfo.BoneMapToMergedBoneMap = TempBoneMapToMergedBoneMap;

							// use the updated bonemap for this new section
							NewSectionInfo.MergedBoneMap = TempMergedBoneMap;

							// keep track of the entry that was found
							FoundIdx = Idx;
							break;
						}
					}
				}

				// new section entries will be created if the material for the source section was not found 
				// or merging it with an existing entry would go over the bone limit for GPU skinning
				if( FoundIdx == INDEX_NONE )
				{
					// create a new section entry
					FNewSectionInfo& NewSectionInfo = *new(NewSectionArray) FNewSectionInfo(Material,MaterialId);
					// initialize the merged bonemap to simply use the original chunk bonemap
					NewSectionInfo.MergedBoneMap = DestChunkBoneMap;
					// add a new merge section entry
					FMergeSectionInfo& MergeSectionInfo = *new(NewSectionInfo.MergeSections) FMergeSectionInfo(
						SrcMesh,
						&SrcLODModel.Sections(SectionIdx),
						&SrcLODModel.Chunks(Section.ChunkIndex));
					// since merged bonemap == chunk.bonemap then remapping is just pass-through
					MergeSectionInfo.BoneMapToMergedBoneMap.Empty( DestChunkBoneMap.Num() );
					for( INT i=0; i < DestChunkBoneMap.Num(); i++ )
					{
						MergeSectionInfo.BoneMapToMergedBoneMap.AddItem((WORD)i);
					}
				}
			}
		}
	}
}

IMPLEMENT_COMPARE_CONSTREF( BYTE, UnSkeletalMeshMerge, { return (A - B); } )

/**
* Creates a new LOD model and adds the new merged sections to it. Modifies the MergedMesh.
* @param LODIdx - current LOD to process
*/
void FSkeletalMeshMerge::GenerateLODModel( INT LODIdx )
{
	// add the new LOD model entry
	FStaticLODModel& MergeLODModel = *new(MergeMesh->LODModels) FStaticLODModel;
	MergeLODModel.NumVertices = 0;
	MergeLODModel.Size = 0;
	// add the new LOD info entry
	FSkeletalMeshLODInfo& MergeLODInfo = *new(MergeMesh->LODInfo) FSkeletalMeshLODInfo;
	MergeLODInfo.DisplayFactor = MergeLODInfo.LODHysteresis = MAX_FLT;

	// generate an array with info about new sections that need to be created
	TArray<FNewSectionInfo> NewSectionArray;
	GenerateNewSectionArray( NewSectionArray, LODIdx );

	MergeLODInfo.bEnableShadowCasting.Empty( NewSectionArray.Num() );

	// merged vertex buffer
	TArray<FSoftSkinVertex> MergedVertexBuffer;
	// merged index buffer
	TArray<WORD> MergedIndexBuffer;

	for( INT CreateIdx=0; CreateIdx < NewSectionArray.Num(); CreateIdx++ )
	{
		FNewSectionInfo& NewSectionInfo = NewSectionArray(CreateIdx);

		// ActiveBoneIndices contains all the bones used by the verts from all the sections of this LOD model
		// Add the bones used by this new section
		for( INT Idx=0; Idx < NewSectionInfo.MergedBoneMap.Num(); Idx++ )
		{
			MergeLODModel.ActiveBoneIndices.AddUniqueItem( NewSectionInfo.MergedBoneMap(Idx) );
		}

		// add the new chunk entry 
		FSkelMeshChunk& Chunk = *new(MergeLODModel.Chunks) FSkelMeshChunk;

		// set the new bonemap from the merged sections
		// these are the bones that will be used by this new section
		Chunk.BoneMap = NewSectionInfo.MergedBoneMap;

		// init vert totals
		Chunk.NumRigidVertices = 0;
		Chunk.NumSoftVertices = 0;
		// keep track of the current base vertex for this section in the merged vertex buffer
		Chunk.BaseVertexIndex = MergedVertexBuffer.Num();

		// add the new section entry
		FSkelMeshSection& Section = *new(MergeLODModel.Sections) FSkelMeshSection;
		// The current implementation of skeletal mesh merging can't do more than two sections.
		// Therefore, we cannot e.g. create new sections based on the source section shadow casting.
		MergeLODInfo.bEnableShadowCasting.AddItem( TRUE );

		// find existing material index
		check(MergeMesh->Materials.Num() == MaterialIds.Num());
		INT MatIndex;
		if(NewSectionInfo.MaterialId == -1)
		{
			MatIndex = MergeMesh->Materials.FindItemIndex(NewSectionInfo.Material);
		}
		else
		{
			MatIndex = MaterialIds.FindItemIndex(NewSectionInfo.MaterialId);
		}

		// if it doesn't exist, make new entry
		if(MatIndex == INDEX_NONE)
		{
			MergeMesh->Materials.AddItem(NewSectionInfo.Material);
			MaterialIds.AddItem(NewSectionInfo.MaterialId);
			Section.MaterialIndex = MergeMesh->Materials.Num()-1;
		}
		else
		{
			Section.MaterialIndex = MatIndex;
		}
		
		// init tri totals
		Section.NumTriangles = 0;
		// one-to-one mapping of sections to chunks
		Section.ChunkIndex = MergeLODModel.Chunks.Num() - 1;
		// keep track of the current base index for this section in the merged index buffer
		Section.BaseIndex = MergedIndexBuffer.Num();

		// iterate over all of the sections that need to be merged together
		for( INT MergeIdx=0; MergeIdx < NewSectionInfo.MergeSections.Num(); MergeIdx++ )
		{
			FMergeSectionInfo& MergeSectionInfo = NewSectionInfo.MergeSections(MergeIdx);
			INT SourceLODIdx = Min(LODIdx, MergeSectionInfo.SkelMesh->LODModels.Num()-1);

			// get the source skel LOD info from this merge entry
			const FSkeletalMeshLODInfo& SrcLODInfo = MergeSectionInfo.SkelMesh->LODInfo(SourceLODIdx);

			// keep track of the lowest LOD displayfactor and hysterisis
			MergeLODInfo.DisplayFactor = Min<FLOAT>(MergeLODInfo.DisplayFactor,SrcLODInfo.DisplayFactor);
			MergeLODInfo.LODHysteresis = Min<FLOAT>(MergeLODInfo.LODHysteresis,SrcLODInfo.LODHysteresis);

			// get the source skel LOD model from this merge entry
			const FStaticLODModel& SrcLODModel = MergeSectionInfo.SkelMesh->LODModels(SourceLODIdx);

			// add required bones from this source model entry to the merge model entry
			for( INT Idx=0; Idx < SrcLODModel.RequiredBones.Num(); Idx++ )
			{
				FName SrcLODBoneName = MergeSectionInfo.SkelMesh->RefSkeleton( SrcLODModel.RequiredBones(Idx) ).Name;
				INT MergeBoneIndex = FindBoneIndex(NewRefSkeleton, SrcLODBoneName);
				check(MergeBoneIndex != INDEX_NONE);
				MergeLODModel.RequiredBones.AddUniqueItem(MergeBoneIndex);
			}

			// keep track of the max number of influences used by the vertices of the chunk
			Chunk.MaxBoneInfluences = Max<INT>( Chunk.MaxBoneInfluences, MergeSectionInfo.Chunk->MaxBoneInfluences );
			// update rigid/smooth verts totals
			Chunk.NumRigidVertices += MergeSectionInfo.Chunk->NumRigidVertices;
			Chunk.NumSoftVertices += MergeSectionInfo.Chunk->NumSoftVertices;

			// update total number of vertices 
			INT NumTotalVertices = MergeSectionInfo.Chunk->NumRigidVertices + MergeSectionInfo.Chunk->NumSoftVertices;
			MergeLODModel.NumVertices += NumTotalVertices;					

			// add the vertices from the original source mesh to the merged vertex buffer					
			INT MaxVertIdx = Min<INT>( 
				MergeSectionInfo.Chunk->BaseVertexIndex + NumTotalVertices, 
				SrcLODModel.VertexBufferGPUSkin.Vertices.Num() 
				);
			// keep track of the current base vertex index before adding any new vertices
			// this will be needed to remap the index buffer values to the new range
			INT CurrentBaseVertexIndex = MergedVertexBuffer.Num();
			for( INT VertIdx=MergeSectionInfo.Chunk->BaseVertexIndex; VertIdx < MaxVertIdx; VertIdx++ )
			{
				// source vertex
				const FSoftSkinVertex& SrcVert = SrcLODModel.VertexBufferGPUSkin.Vertices(VertIdx);
				// add the new vertex
				FSoftSkinVertex& DestVert = MergedVertexBuffer(MergedVertexBuffer.AddItem(SrcVert));

				// remap the bone index used by this vertex to match the mergedbonemap 
				for( INT Idx=0; Idx < MAX_INFLUENCES; Idx++ )
				{
					if( DestVert.InfluenceWeights > 0 )
					{
						checkSlow(MergeSectionInfo.BoneMapToMergedBoneMap.IsValidIndex(DestVert.InfluenceBones[Idx]));
						DestVert.InfluenceBones[Idx] = (BYTE)MergeSectionInfo.BoneMapToMergedBoneMap(DestVert.InfluenceBones[Idx]);
					}
				}
			}

			// update total number of triangles
			Section.NumTriangles += MergeSectionInfo.Section->NumTriangles;

			// add the indices from the original source mesh to the merged index buffer					
			INT MaxIndexIdx = Min<INT>( 
				MergeSectionInfo.Section->BaseIndex + MergeSectionInfo.Section->NumTriangles * 3, 
				SrcLODModel.IndexBuffer.Indices.Num() 
				);
			for( INT IndexIdx=MergeSectionInfo.Section->BaseIndex; IndexIdx < MaxIndexIdx; IndexIdx++ )
			{
				WORD SrcIndex = SrcLODModel.IndexBuffer.Indices(IndexIdx);

				// add offset to each index to match the new entries in the merged vertex buffer
				checkSlow(SrcIndex >= MergeSectionInfo.Chunk->BaseVertexIndex);	
				WORD DstIndex = SrcIndex - MergeSectionInfo.Chunk->BaseVertexIndex + CurrentBaseVertexIndex;
				checkSlow(DstIndex < MergedVertexBuffer.Num());

				// add the new index to the merged vertex buffer
				MergedIndexBuffer.AddItem(DstIndex);
			}
		}
	}

	check( MergeLODInfo.bEnableShadowCasting.Num() == MergeLODModel.Sections.Num() );

	// sort required bone array in strictly increasing order
	Sort<USE_COMPARE_CONSTREF(BYTE, UnSkeletalMeshMerge)>( &MergeLODModel.RequiredBones(0), MergeLODModel.RequiredBones.Num() );

	// copy the new vertices and indices to the vertex buffer for the new model
	MergeLODModel.VertexBufferGPUSkin.Vertices = MergedVertexBuffer;
	MergeLODModel.IndexBuffer.Indices = MergedIndexBuffer;
}

/**
* (Re)initialize and merge skeletal mesh info from the list of source meshes to the merge mesh
* @return TRUE if succeeded
*/
UBOOL FSkeletalMeshMerge::ProcessMergeMesh()
{
	UBOOL Result=TRUE;
	
	// copy settings and bone info from src meshes
	UBOOL bNeedsInit=TRUE;

	// Assign new ref skeleton, and compute quick lookup map
	MergeMesh->RefSkeleton = NewRefSkeleton;
	MergeMesh->InitNameIndexMap();

	MergeMesh->SkelMirrorTable.Empty();

	for( INT MeshIdx=0; MeshIdx < SrcMeshList.Num(); MeshIdx++ )
	{
		USkeletalMesh* SrcMesh = SrcMeshList(MeshIdx);
		if( SrcMesh )
		{
			if( bNeedsInit )
			{
				MergeMesh->RefBasesInvMatrix = SrcMesh->RefBasesInvMatrix;

				// initialize the merged mesh with the first src mesh entry used
				MergeMesh->Bounds = SrcMesh->Bounds;
				MergeMesh->Origin = SrcMesh->Origin;
				MergeMesh->RotOrigin = SrcMesh->RotOrigin;
				MergeMesh->SkeletalDepth = SrcMesh->SkeletalDepth;
				MergeMesh->SkelMirrorAxis = SrcMesh->SkelMirrorAxis;
				MergeMesh->SkelMirrorFlipAxis = SrcMesh->SkelMirrorFlipAxis;

				MergeMesh->Sockets.AddZeroed(SrcMesh->Sockets.Num());
				for( INT SocketIdx=0; SocketIdx < SrcMesh->Sockets.Num(); SocketIdx++ )
				{
					USkeletalMeshSocket* DupSocket = CastChecked<USkeletalMeshSocket>(UObject::StaticDuplicateObject( SrcMesh->Sockets(SocketIdx), SrcMesh->Sockets(SocketIdx), MergeMesh, TEXT("None") ));
					MergeMesh->Sockets(SocketIdx) = DupSocket;
				}

				MergeMesh->SkeletalDepth = SrcMesh->SkeletalDepth;
				// only initialize once
				bNeedsInit = FALSE;
			}
			else
			{
				// merge sockets USkeletalMesh::Sockets
				for( INT SocketIdx=0; SocketIdx < SrcMesh->Sockets.Num(); SocketIdx++ )
				{
					USkeletalMeshSocket* DupSocket = CastChecked<USkeletalMeshSocket>(UObject::StaticDuplicateObject( SrcMesh->Sockets(SocketIdx), SrcMesh->Sockets(SocketIdx), MergeMesh, TEXT("None") ));
					MergeMesh->Sockets.AddUniqueItem( DupSocket );
				}

				// add bounds
				MergeMesh->Bounds = MergeMesh->Bounds + SrcMesh->Bounds;

				// keep track of the max bone hierarchy
				MergeMesh->SkeletalDepth = Max(MergeMesh->SkeletalDepth,SrcMesh->SkeletalDepth);
			}			
		}
	}

	// Rebuild inverse ref pose matrices.
	MergeMesh->RefBasesInvMatrix.Empty();
	MergeMesh->CalculateInvRefMatrices();

	return Result;
}





