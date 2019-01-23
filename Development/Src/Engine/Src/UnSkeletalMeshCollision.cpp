/*=============================================================================
	UnSkeletalMeshCollision.cpp: Skeletal mesh collision code
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EnginePhysicsClasses.h"

#if WITH_NOVODEX
#include "UnNovodexSupport.h"
#endif // WITH_NOVODEX

UBOOL USkeletalMeshComponent::LineCheck(
                FCheckResult &Result,
                const FVector& End,
                const FVector& Start,
                const FVector& Extent,
				DWORD TraceFlags)
{
	UBOOL Retval = FALSE;

	// Line check always fail if no SkeletalMesh.
	if(!SkeletalMesh)
	{
		return Retval;
	}

	UBOOL bZeroExtent = Extent.IsZero();
	UBOOL bWantSimpleCheck = (SkeletalMesh->bUseSimpleBoxCollision && !bZeroExtent) || (SkeletalMesh->bUseSimpleLineCollision && bZeroExtent);


	// If we want to use per-poly collision information (works for specified rigid parts).
	// If no bones are specified for per-poly, always fall back to using simplified collision.
	if(!bWantSimpleCheck || ((TraceFlags & TRACE_ComplexCollision) && SkeletalMesh->PerPolyCollisionBones.Num() > 0))
	{
		check(SkeletalMesh->PerPolyCollisionBones.Num() == SkeletalMesh->PerPolyBoneKDOPs.Num());

		UBOOL bHit = FALSE;
		Result.Time = 1.f;
		for(INT i=0; i<SkeletalMesh->PerPolyBoneKDOPs.Num(); i++)
		{
			FName BoneName = SkeletalMesh->PerPolyCollisionBones(i);
			INT BoneIndex = SkeletalMesh->MatchRefBone(BoneName);
			if(BoneIndex != INDEX_NONE)
			{
				FSkelMeshCollisionDataProvider Provider(this, SkeletalMesh, BoneIndex, i);
				FCheckResult TempResult(1.f);
				UBOOL bTraceHit = FALSE;

				if(bZeroExtent)
				{
					TkDOPLineCollisionCheck<FSkelMeshCollisionDataProvider,WORD> kDOPCheck(Start,End,TraceFlags,Provider,&TempResult);
					// Do the line check
					bTraceHit = SkeletalMesh->PerPolyBoneKDOPs(i).KDOPTree.LineCheck(kDOPCheck);
					if(bTraceHit)
					{
						TempResult.Normal = kDOPCheck.GetHitNormal();
					}
				}
				else
				{
					TkDOPBoxCollisionCheck<FSkelMeshCollisionDataProvider,WORD> kDOPCheck(Start,End,Extent,TraceFlags,Provider,&TempResult);
					// Do the swept-box check
					bTraceHit = SkeletalMesh->PerPolyBoneKDOPs(i).KDOPTree.BoxCheck(kDOPCheck);
					if(bTraceHit)
					{
						TempResult.Normal = kDOPCheck.GetHitNormal();
					}
				}

				// If the result is closer than our best so far, keep this result
				if(TempResult.Time < Result.Time)
				{
					Result = TempResult;
					Result.BoneName = BoneName;
					bHit = TRUE;
				}
			} 
		}

		// If we got at least one hit, fill in the other details and return FALSE (
		if(bHit)
		{
			//FVector TrueLocation = Start + (End - Start) * Result.Time;

			Result.Actor = Owner;
			Result.Component = this;
			Result.Time = Clamp(Result.Time - Clamp(0.1f,0.1f / (End - Start).Size(),4.0f / (End - Start).Size()),0.0f,1.0f);
			Result.Location = Start + (End - Start) * Result.Time;
			Retval = FALSE;

			//GWorld->PersistentLineBatcher->DrawLine(Start, TrueLocation, FColor(0,255,255), SDPG_World);
			//GWorld->PersistentLineBatcher->DrawLine(FVector(0,0,0), TrueLocation, FColor(255,0,0), SDPG_World);
		}
		else
		{
			Retval = TRUE;
		}
	}
	else
	{
		// The PhysicsAsset provides the simplified collision geometry for a skeletal mesh
		if( PhysicsAsset != NULL )
		{
			Retval = PhysicsAsset->LineCheck( Result, this, Start, End, Extent );

			// If we hit and it's an extent trace (eg player movement) - pull back the hit location in the same way as static meshes.
			if(!Retval && !Extent.IsZero())
			{
				Result.Time = Clamp(Result.Time - Clamp(0.1f,0.1f / (End - Start).Size(),4.0f / (End - Start).Size()),0.0f,1.0f);
				Result.Location = Start + (End - Start) * Result.Time;
			}
		}
		else
		{
			Retval = TRUE;
		}
	}

#if WITH_NOVODEX && !NX_DISABLE_CLOTH
	if(ClothSim && bEnableClothSimulation && SkeletalMesh && SkeletalMesh->bEnableClothLineChecks)
	{
		FCheckResult TempResult;

		if(!ClothLineCheck(this, TempResult, End, Start, Extent, TraceFlags))
		{//hit
			if( Retval || (TempResult.Time < Result.Time) )
			{
				Result = TempResult;
				Retval = FALSE;
			}
		}
	}
#endif

	return Retval;
}


UBOOL USkeletalMeshComponent::PointCheck(FCheckResult& Result, const FVector& Location, const FVector& Extent, DWORD TraceFlags)
{
	UBOOL bHit = FALSE;

	if(PhysicsAsset)
	{
		bHit = !PhysicsAsset->PointCheck( Result, this, Location, Extent );
	}

#if WITH_NOVODEX && !NX_DISABLE_CLOTH
	if(!bHit)
	{
		if(ClothSim && bEnableClothSimulation && SkeletalMesh && SkeletalMesh->bEnableClothLineChecks)
		{
			bHit = !ClothPointCheck(Result, this, Location, Extent);
		}
	}
#endif

	return !bHit;
}


/** Re-generate the per-poly collision data in the PerPolyBoneKDOPs array, based on names in the PerPolyCollisionBones array. */
void USkeletalMesh::UpdatePerPolyKDOPs()
{
	// Empty and resize array of kdops.
	PerPolyBoneKDOPs.Empty();
	PerPolyBoneKDOPs.AddZeroed(PerPolyCollisionBones.Num());

	// Build vertex buffer with _all_ verts in skeletal mesh.
	FStaticLODModel* LODModel = &LODModels(0);
	TArray<FVector>	AllVerts;
	for(INT ChunkIndex = 0;ChunkIndex < LODModel->Chunks.Num();ChunkIndex++)
	{
		FSkelMeshChunk& Chunk = LODModel->Chunks(ChunkIndex);

		if(Chunk.GetNumRigidVertices() > 0)
		{
			FSoftSkinVertex* SrcRigidVertex = &LODModel->VertexBufferGPUSkin.Vertices(Chunk.GetRigidVertexBufferIndex());
			for(INT i=0; i<Chunk.GetNumRigidVertices(); i++,SrcRigidVertex++)
			{
				AllVerts.AddItem( SrcRigidVertex->Position );
			}
		}

		if(Chunk.GetNumSoftVertices() > 0)
		{
			FSoftSkinVertex* SrcSoftVertex = &LODModel->VertexBufferGPUSkin.Vertices(Chunk.GetSoftVertexBufferIndex());
			for(INT i=0; i<Chunk.GetNumSoftVertices(); i++,SrcSoftVertex++)
			{
				AllVerts.AddItem( SrcSoftVertex->Position );
			}
		}
	}

	// Iterate over each bone we want collision for.
	if(PerPolyCollisionBones.Num() > 0)
	{
		debugf(TEXT("Building per-poly collision kDOP trees for '%s'"), *GetPathName());
	}

	for(INT i=0; i<PerPolyCollisionBones.Num(); i++)
	{
		FPerPolyBoneCollisionData& Data = PerPolyBoneKDOPs(i);

		INT BoneIndex = MatchRefBone(PerPolyCollisionBones(i));
		if(BoneIndex != INDEX_NONE)
		{
			// Get the transform from mesh space to bone space
			const FMatrix& MeshToBone = RefBasesInvMatrix(BoneIndex);

			// Verts to use for collision for this bone
			TArray<INT> CollisionToGraphicsVertMap;
			// Current position in AllVerts
			INT VertIndex = 0;
			for(INT ChunkIndex = 0;ChunkIndex < LODModel->Chunks.Num();ChunkIndex++)
			{
				FSkelMeshChunk& Chunk = LODModel->Chunks(ChunkIndex);

				// Only consider rigidly weighted verts for this kind of collision
				for(INT i=0; i<Chunk.RigidVertices.Num(); i++)
				{
					FRigidSkinVertex& RV = Chunk.RigidVertices(i);
					if( Chunk.BoneMap(RV.Bone) == BoneIndex )
					{
						CollisionToGraphicsVertMap.AddItem(VertIndex);
					}

					VertIndex++;
				}

				// Wind on VertIndex over all the soft verts in this chunk.
				VertIndex += Chunk.SoftVertices.Num();
			}

			// For all the verts that are rigidly weighted to this bone, transform them into 'bone space'
			Data.CollisionVerts.Add(CollisionToGraphicsVertMap.Num());
			for(INT VertIdx=0; VertIdx<CollisionToGraphicsVertMap.Num(); VertIdx++)
			{
				Data.CollisionVerts(VertIdx) = MeshToBone.TransformFVector( AllVerts(CollisionToGraphicsVertMap(VertIdx)) );
			}

			// Now make triangles for building kDOP
			// Find all triangles where all 3 verts are rigidly weighted to bone (ie all three verts are in CollisionToGraphicsVertMap)
			TArray<FkDOPBuildCollisionTriangle<WORD> > KDOPTriangles;
			for(INT j=0; j<LODModel->IndexBuffer.Indices.Num(); j+=3)
			{
				WORD Index0 = LODModel->IndexBuffer.Indices(j+0);
				WORD Index1 = LODModel->IndexBuffer.Indices(j+1);
				WORD Index2 = LODModel->IndexBuffer.Indices(j+2);

				INT CollisionIndex0 = CollisionToGraphicsVertMap.FindItemIndex(Index0);
				INT CollisionIndex1 = CollisionToGraphicsVertMap.FindItemIndex(Index1);
				INT CollisionIndex2 = CollisionToGraphicsVertMap.FindItemIndex(Index2);

				if(	CollisionIndex0 != INDEX_NONE && 
					CollisionIndex1 != INDEX_NONE && 
					CollisionIndex2 != INDEX_NONE )
				{
					// Build a new kDOP collision triangle
					new (KDOPTriangles) FkDOPBuildCollisionTriangle<WORD>(CollisionIndex0, CollisionIndex1, CollisionIndex2,
						0,
						Data.CollisionVerts(CollisionIndex0), Data.CollisionVerts(CollisionIndex1), Data.CollisionVerts(CollisionIndex2));
				}
			}

			Data.KDOPTree.Build(KDOPTriangles);
			debugf(TEXT("--- Bone: %s  Tris: %d"), *PerPolyCollisionBones(i).ToString(), Data.KDOPTree.Triangles.Num());
		}
	}
}

UBOOL USkeletalMeshComponent::GetBonesWithinRadius( const FVector& Origin, FLOAT Radius, DWORD TraceFlags, TArray<FName>& out_Bones )
{
	if( !SkeletalMesh )
		return FALSE;

	FLOAT RadiusSq = Radius*Radius;

	// Transform the Origin into mesh local space so we don't have to transform the (mesh local) bone locations
	FVector TestLocation = LocalToWorld.Inverse().TransformFVector(Origin);
	for( INT Idx = 0; Idx < SpaceBases.Num(); Idx++ )
	{
		FLOAT DistSquared = (TestLocation - SpaceBases(Idx).GetOrigin()).SizeSquared();
		if( DistSquared <= RadiusSq )
		{
			out_Bones.AddItem( SkeletalMesh->RefSkeleton(Idx).Name );
		}
	}

	return (out_Bones.Num()>0);
}

