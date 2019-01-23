/*=============================================================================
	UnStaticMeshBuild.cpp: Static mesh building.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

#include "UnMeshBuild.h"

#if !CONSOLE
//
//	PointsEqual
//
inline UBOOL PointsEqual(const FVector& V1,const FVector& V2)
{
	if(Abs(V1.X - V2.X) > THRESH_POINTS_ARE_SAME * 4.0f)
		return 0;

	if(Abs(V1.Y - V2.Y) > THRESH_POINTS_ARE_SAME * 4.0f)
		return 0;

	if(Abs(V1.Z - V2.Z) > THRESH_POINTS_ARE_SAME * 4.0f)
		return 0;

	return 1;
}

//
//	UVsEqual
//

inline UBOOL UVsEqual(const FVector2D& UV1,const FVector2D& UV2)
{
	if(Abs(UV1.X - UV2.X) > (1.0f / 1024.0f))
		return 0;

	if(Abs(UV1.Y - UV2.Y) > (1.0f / 1024.0f))
		return 0;

	return 1;
}

//
// FanFace - Smoothing group interpretation helper structure.
//

struct FanFace
{
	INT FaceIndex;
	INT LinkedVertexIndex;
	UBOOL Filled;		
};

//
//	FindVertexIndex
//

INT FindVertexIndex(FVector Position,FPackedNormal TangentX,FPackedNormal TangentY,FPackedNormal TangentZ,FColor Color,const FVector2D* UVs,INT NumUVs,TArray<FStaticMeshBuildVertex>& Vertices)
{
	// Find any identical vertices already in the vertex buffer.

	INT	VertexBufferIndex = INDEX_NONE;

	for(INT VertexIndex = 0;VertexIndex < Vertices.Num();VertexIndex++)
	{
		// Compare vertex position and normal.

		FStaticMeshBuildVertex* CompareVertex = &Vertices(VertexIndex);

		if(!PointsEqual(CompareVertex->Position,Position))
			continue;

		if(!(CompareVertex->TangentX == TangentX))
			continue;

		if(!(CompareVertex->TangentY == TangentY))
			continue;

		if(!(CompareVertex->TangentZ == TangentZ))
			continue;

		if(!(CompareVertex->Color == Color))
			continue;

		// Compare vertex UVs.
		UBOOL UVsMatch = TRUE;
		for(INT UVIndex = 0;UVIndex < Min<INT>(NumUVs,MAX_TEXCOORDS);UVIndex++)
		{
			if(!UVsEqual(CompareVertex->UVs[UVIndex],UVs[UVIndex]))
			{
				UVsMatch = FALSE;
				break;
			}
		}
		if(!UVsMatch)
			continue;

		// The vertex matches!

		VertexBufferIndex = VertexIndex;
		break;
	}

	// If there is no identical vertex already in the vertex buffer...

	if(VertexBufferIndex == INDEX_NONE)
	{
		// Add a new vertex to the vertex buffers.

		FStaticMeshBuildVertex Vertex;

		Vertex.Position = Position;
		Vertex.TangentX = TangentX;
		Vertex.TangentY = TangentY;
		Vertex.TangentZ = TangentZ;
		Vertex.Color = Color;

		for(INT UVIndex = 0;UVIndex < Min<INT>(NumUVs,MAX_TEXCOORDS);UVIndex++)
		{
			Vertex.UVs[UVIndex] = UVs[UVIndex];
		}

		for(INT UVIndex = NumUVs;UVIndex < MAX_TEXCOORDS;UVIndex++)
		{
			Vertex.UVs[UVIndex] = FVector2D(0,0);
		}

		VertexBufferIndex = Vertices.AddItem(Vertex);
	}

	return VertexBufferIndex;

}

//
//	ClassifyTriangleVertices
//

ESplitType ClassifyTriangleVertices(FPlane Plane,FVector* Vertices)
{
	ESplitType	Classification = SP_Coplanar;

	for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
	{
		FLOAT	Dist = Plane.PlaneDot(Vertices[VertexIndex]);

		if(Dist < -0.0001f)
		{
			if(Classification == SP_Front)
				Classification = SP_Split;
			else if(Classification != SP_Split)
				Classification = SP_Back;
		}
		else if(Dist >= 0.0001f)
		{
			if(Classification == SP_Back)
				Classification = SP_Split;
			else if(Classification != SP_Split)
				Classification = SP_Front;
		}
	}

	return Classification;
}
#endif

//
//	UStaticMesh::Build
//

void UStaticMesh::Build()
{
#if !CONSOLE
	GWarn->BeginSlowTask(*FString::Printf(TEXT("(%s) Building"),*GetPathName()),1);

	// Detach all instances of this static mesh from the scene.
	FStaticMeshComponentReattachContext	ComponentReattachContext(this);

	// Release the static mesh's resources.
	ReleaseResources();

	// Flush the resource release commands to the rendering thread to ensure that the build doesn't occur while a resource is still
	// allocated, and potentially accessing the UStaticMesh.
	ReleaseResourcesFence.Wait();

	// Mark the parent package as dirty.
	MarkPackageDirty();

	TArray<FkDOPBuildCollisionTriangle<WORD> > kDOPBuildTriangles;
	check(LODModels.Num());
	for(INT i=0;i<LODModels.Num();i++)
	{
		// NOTE: Only building kdop for LOD 0
		LODModels(i).Build(kDOPBuildTriangles,(i==0),this);
	}

	// Calculate the bounding box.

	FBox	BoundingBox(0);

	for(UINT VertexIndex = 0;VertexIndex < LODModels(0).NumVertices;VertexIndex++)
	{
		BoundingBox += LODModels(0).PositionVertexBuffer.VertexPosition(VertexIndex);
	}
	BoundingBox.GetCenterAndExtents(Bounds.Origin,Bounds.BoxExtent);

	// Calculate the bounding sphere, using the center of the bounding box as the origin.

	Bounds.SphereRadius = 0.0f;
	for(UINT VertexIndex = 0;VertexIndex < LODModels(0).NumVertices;VertexIndex++)
	{
		Bounds.SphereRadius = Max((LODModels(0).PositionVertexBuffer.VertexPosition(VertexIndex) - Bounds.Origin).Size(),Bounds.SphereRadius);
	}
	kDOPTree.Build(kDOPBuildTriangles);

	// Reinitialize the static mesh's resources.
	InitResources();

	GWarn->EndSlowTask();
#else
	appErrorf(TEXT("UStaticMesh::Build should not be called on a console"));
#endif
}


/**
* Fill an array with triangles which will be used to build a KDOP tree
* @param kDOPBuildTriangles - the array to fill
*/

void FStaticMeshRenderData::GetKDOPTriangles(TArray<FkDOPBuildCollisionTriangle<WORD> >& kDOPBuildTriangles) 
{
	for(INT TriangleIndex = 0; TriangleIndex < IndexBuffer.Indices.Num(); TriangleIndex += 3)
	{
		UINT IndexOne = IndexBuffer.Indices(TriangleIndex);
		UINT IndexTwo = IndexBuffer.Indices(TriangleIndex + 1);
		UINT IndexThree = IndexBuffer.Indices(TriangleIndex + 2);

		//add a triangle to the array
		new (kDOPBuildTriangles) FkDOPBuildCollisionTriangle<WORD>(IndexOne,
			IndexTwo, IndexThree, 0,
			PositionVertexBuffer.VertexPosition(IndexOne), PositionVertexBuffer.VertexPosition(IndexTwo), PositionVertexBuffer.VertexPosition(IndexThree));
	}
}

/**
* Build rendering data from a raw triangle stream
* @param kDOPBuildTriangles output collision tree. A dummy can be passed if you do not specify BuildKDop as TRUE
* @param Whether to build and return a kdop tree from the mesh data
* @param Parent Parent mesh
*/
void FStaticMeshRenderData::Build(TArray<FkDOPBuildCollisionTriangle<WORD> >& kDOPBuildTriangles, UBOOL BuildKDop, class UStaticMesh* Parent)
{
#if !CONSOLE
	check(Parent);
	// Load raw data.
	FStaticMeshTriangle* RawTriangleData = (FStaticMeshTriangle*) RawTriangles.Lock(LOCK_READ_ONLY);

	// Clear old data.
	TArray<FStaticMeshBuildVertex> Vertices;
	IndexBuffer.Indices.Empty();
	WireframeIndexBuffer.Indices.Empty();
	Edges.Empty();
	ShadowTriangleDoubleSided.Empty();
	VertexBuffer.CleanUp();
	PositionVertexBuffer.CleanUp();
	ShadowExtrusionVertexBuffer.CleanUp();

	// force 32 bit floats if needed
	VertexBuffer.SetUseFullPrecisionUVs(Parent->UseFullPrecisionUVs);	

	// Calculate triangle normals.

	TArray<FVector>	TriangleTangentX(RawTriangles.GetElementCount());
	TArray<FVector>	TriangleTangentY(RawTriangles.GetElementCount());
	TArray<FVector>	TriangleTangentZ(RawTriangles.GetElementCount());

	for(INT TriangleIndex = 0;TriangleIndex < RawTriangles.GetElementCount();TriangleIndex++)
	{
		FStaticMeshTriangle*	Triangle = &RawTriangleData[TriangleIndex];
		INT						UVIndex = 0;
		FVector					TriangleNormal = FPlane(
											Triangle->Vertices[2],
											Triangle->Vertices[1],
											Triangle->Vertices[0]
											);

		FVector	P1 = Triangle->Vertices[0],
				P2 = Triangle->Vertices[1],
				P3 = Triangle->Vertices[2];
		FMatrix	ParameterToLocal(
			FPlane(	P2.X - P1.X,	P2.Y - P1.Y,	P2.Z - P1.Z,	0	),
			FPlane(	P3.X - P1.X,	P3.Y - P1.Y,	P3.Z - P1.Z,	0	),
			FPlane(	P1.X,			P1.Y,			P1.Z,			0	),
			FPlane(	0,				0,				0,				1	)
			);

		FVector2D	T1 = Triangle->UVs[0][UVIndex],
					T2 = Triangle->UVs[1][UVIndex],
					T3 = Triangle->UVs[2][UVIndex];
		FMatrix		ParameterToTexture(
			FPlane(	T2.X - T1.X,	T2.Y - T1.Y,	0,	0	),
			FPlane(	T3.X - T1.X,	T3.Y - T1.Y,	0,	0	),
			FPlane(	T1.X,			T1.Y,			1,	0	),
			FPlane(	0,				0,				0,	1	)
			);

		const FMatrix TextureToLocal = ParameterToTexture.InverseSafe() * ParameterToLocal;

		TriangleTangentX(TriangleIndex) = TextureToLocal.TransformNormal(FVector(1,0,0)).SafeNormal();
		TriangleTangentY(TriangleIndex) = TextureToLocal.TransformNormal(FVector(0,1,0)).SafeNormal();
		TriangleTangentZ(TriangleIndex) = TriangleNormal;

		CreateOrthonormalBasis(
			TriangleTangentX(TriangleIndex),
			TriangleTangentY(TriangleIndex),
			TriangleTangentZ(TriangleIndex)
			);
	}

	// Initialize material index buffers.

	TArray< FRawStaticIndexBuffer<TRUE> >	ElementIndexBuffers;

	for(INT ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
	{
		new(ElementIndexBuffers) FRawStaticIndexBuffer<TRUE>();
	}

	// Determine the number of texture coordinates/vertex used by the static mesh.
	UINT NumTexCoords = 1;
	for(INT TriangleIndex = 0;TriangleIndex < RawTriangles.GetElementCount();TriangleIndex++)
	{
		const FStaticMeshTriangle* Triangle = &RawTriangleData[TriangleIndex];
		NumTexCoords = Clamp<UINT>(Triangle->NumUVs,NumTexCoords,MAX_TEXCOORDS);
	}

	// Process each triangle.
    INT	NumDegenerates = 0; 
	for(INT TriangleIndex = 0;TriangleIndex < RawTriangles.GetElementCount();TriangleIndex++)
	{
		const FStaticMeshTriangle* Triangle = &RawTriangleData[TriangleIndex];
		FStaticMeshElement& Element = Elements(Triangle->MaterialIndex);

        if( PointsEqual(Triangle->Vertices[0],Triangle->Vertices[1])
		    ||	PointsEqual(Triangle->Vertices[0],Triangle->Vertices[2])
		    ||	PointsEqual(Triangle->Vertices[1],Triangle->Vertices[2])
			|| TriangleTangentZ(TriangleIndex).IsZero()
		)
		{
            NumDegenerates++;
			continue;
		}

		// Calculate smooth vertex normals.

		FVector	VertexTangentX[3],
				VertexTangentY[3],
				VertexTangentZ[3];

		for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
		{
			VertexTangentX[VertexIndex] = FVector(0,0,0);
			VertexTangentY[VertexIndex] = FVector(0,0,0);
			VertexTangentZ[VertexIndex] = FVector(0,0,0);
		}

		FLOAT	Determinant = FTriple(
					TriangleTangentX(TriangleIndex),
					TriangleTangentY(TriangleIndex),
					TriangleTangentZ(TriangleIndex)
					);
		
		// Determine contributing faces for correct smoothing group behaviour  according to the orthodox Max interpretation of smoothing groups.    O(n^2)      - EDN

		TArray<FanFace> RelevantFacesForVertex[3];

		for(INT OtherTriangleIndex = 0;OtherTriangleIndex < RawTriangles.GetElementCount();OtherTriangleIndex++)
		{
			for(INT OurVertexIndex = 0; OurVertexIndex < 3; OurVertexIndex++)
			{		
				const FStaticMeshTriangle* OtherTriangle = &RawTriangleData[OtherTriangleIndex];
				FanFace NewFanFace;
				INT CommonIndexCount = 0;				
				// Check for vertices in common.
				if(TriangleIndex == OtherTriangleIndex)
				{
					CommonIndexCount = 3;		
					NewFanFace.LinkedVertexIndex = OurVertexIndex;
				}
				else
				{
					// Check matching vertices against main vertex .
					for(INT OtherVertexIndex=0; OtherVertexIndex<3; OtherVertexIndex++)
					{
						if( PointsEqual(Triangle->Vertices[OurVertexIndex], OtherTriangle->Vertices[OtherVertexIndex]) )
						{
							CommonIndexCount++;
							NewFanFace.LinkedVertexIndex = OtherVertexIndex;
						}
					}
				}
				//Add if connected by at least one point. Smoothing matches are considered later.
				if(CommonIndexCount > 0)
				{ 					
					NewFanFace.FaceIndex = OtherTriangleIndex;
					NewFanFace.Filled = ( OtherTriangleIndex == TriangleIndex ); // Starter face for smoothing floodfill.
					RelevantFacesForVertex[OurVertexIndex].AddItem( NewFanFace );
				}
			}
		}

		// Find true relevance of faces for a vertex normal by traversing smoothing-group-compatible connected triangle fans around common vertices.

		for(INT VertexIndex = 0; VertexIndex < 3; VertexIndex++)
		{
			INT NewConnections = 1;
			while( NewConnections )
			{
				NewConnections = 0;
				for( INT OtherFaceIdx=0; OtherFaceIdx < RelevantFacesForVertex[VertexIndex].Num(); OtherFaceIdx++ )
				{															
					// The vertex' own face is initially the only face with  .Filled == true.
					if( RelevantFacesForVertex[VertexIndex]( OtherFaceIdx ).Filled )  
					{				
						const FStaticMeshTriangle* OtherTriangle = &RawTriangleData[ RelevantFacesForVertex[VertexIndex](OtherFaceIdx).FaceIndex ];
						for( INT MoreFaceIdx = 0; MoreFaceIdx < RelevantFacesForVertex[VertexIndex].Num(); MoreFaceIdx ++ )
						{								
							if( ! RelevantFacesForVertex[VertexIndex]( MoreFaceIdx).Filled )
							{
								const FStaticMeshTriangle* FreshTriangle = &RawTriangleData[ RelevantFacesForVertex[VertexIndex](MoreFaceIdx).FaceIndex ];
								if( ( FreshTriangle->SmoothingMask &  OtherTriangle->SmoothingMask ) &&  ( MoreFaceIdx != OtherFaceIdx) )
								{				
									INT CommonVertices = 0;
									for(INT OtherVertexIndex = 0; OtherVertexIndex < 3; OtherVertexIndex++)
									{											
										for(INT OrigVertexIndex = 0; OrigVertexIndex < 3; OrigVertexIndex++)
										{
											if( PointsEqual ( FreshTriangle->Vertices[OrigVertexIndex],  OtherTriangle->Vertices[OtherVertexIndex]  )	)
											{
												CommonVertices++;
											}
										}										
									}
									// Flood fill faces with more than one common vertices which must be touching edges.
									if( CommonVertices > 1)
									{
										RelevantFacesForVertex[VertexIndex]( MoreFaceIdx).Filled = true;
										NewConnections++;
									}								
								}
							}
						}
					}
				}
			} 
		}

		// Vertex normal construction.

		for(INT VertexIndex = 0; VertexIndex < 3; VertexIndex++)
		{
			for(INT RelevantFaceIdx = 0; RelevantFaceIdx < RelevantFacesForVertex[VertexIndex].Num(); RelevantFaceIdx++)
			{
				if( RelevantFacesForVertex[VertexIndex](RelevantFaceIdx).Filled )
				{
					INT OtherTriangleIndex = RelevantFacesForVertex[VertexIndex]( RelevantFaceIdx).FaceIndex;
					INT OtherVertexIndex	= RelevantFacesForVertex[VertexIndex]( RelevantFaceIdx).LinkedVertexIndex;

					const FStaticMeshTriangle*	OtherTriangle = &RawTriangleData[OtherTriangleIndex];
					FLOAT OtherDeterminant = FTriple(
						TriangleTangentX(OtherTriangleIndex),
						TriangleTangentY(OtherTriangleIndex),
						TriangleTangentZ(OtherTriangleIndex)
						);

					if( Determinant * OtherDeterminant > 0.0f && UVsEqual(Triangle->UVs[VertexIndex][0],OtherTriangle->UVs[OtherVertexIndex][0]) )
					{
						VertexTangentX[VertexIndex] += TriangleTangentX(OtherTriangleIndex);
						VertexTangentY[VertexIndex] += TriangleTangentY(OtherTriangleIndex);
					}
					VertexTangentZ[VertexIndex] += TriangleTangentZ(OtherTriangleIndex);
				}
			}
		}


		// Normalization.

		for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
		{
			VertexTangentX[VertexIndex].Normalize();
			VertexTangentY[VertexIndex].Normalize();
			VertexTangentZ[VertexIndex].Normalize();

			VertexTangentY[VertexIndex] -= VertexTangentX[VertexIndex] * (VertexTangentX[VertexIndex] | VertexTangentY[VertexIndex]);
			VertexTangentY[VertexIndex].Normalize();

			VertexTangentX[VertexIndex] -= VertexTangentZ[VertexIndex] * (VertexTangentZ[VertexIndex] | VertexTangentX[VertexIndex]);
			VertexTangentX[VertexIndex].Normalize();
			VertexTangentY[VertexIndex] -= VertexTangentZ[VertexIndex] * (VertexTangentZ[VertexIndex] | VertexTangentY[VertexIndex]);
			VertexTangentY[VertexIndex].Normalize();
		}

		// Index the triangle's vertices.

		INT	VertexIndices[3];

		for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
			VertexIndices[VertexIndex] = FindVertexIndex(
											Triangle->Vertices[VertexIndex],
											VertexTangentX[VertexIndex],
											VertexTangentY[VertexIndex],
											VertexTangentZ[VertexIndex],
											Triangle->Colors[VertexIndex],
											Triangle->UVs[VertexIndex],
											Triangle->NumUVs,
											Vertices
											);

		// Reject degenerate triangles.

		if(VertexIndices[0] == VertexIndices[1] || VertexIndices[1] == VertexIndices[2] || VertexIndices[0] == VertexIndices[2])
			continue;

		// Put the indices in the material index buffer.

		for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
			ElementIndexBuffers(Triangle->MaterialIndex).Indices.AddItem(VertexIndices[VertexIndex]);

		if(Element.EnableCollision && BuildKDop)
		{
			// Build a new kDOP collision triangle
			new (kDOPBuildTriangles) FkDOPBuildCollisionTriangle<WORD>(VertexIndices[0],
				VertexIndices[1],VertexIndices[2],Triangle->MaterialIndex,
				Triangle->Vertices[0],Triangle->Vertices[1],Triangle->Vertices[2]);
		}
	}

	// Initialize the vertex buffer.
	NumVertices = Vertices.Num();
	VertexBuffer.Init(Vertices,NumTexCoords);
	PositionVertexBuffer.Init(Vertices);
	ShadowExtrusionVertexBuffer.Init(Vertices);

	if(NumDegenerates)
	{
    	debugf(TEXT("StaticMesh had %i degenerates"), NumDegenerates );
	}

	// Build a cache optimized triangle list for each material and copy it into the shared index buffer.

	for(INT ElementIndex = 0;ElementIndex < ElementIndexBuffers.Num();ElementIndex++)
	{
		FStaticMeshElement& Element = Elements(ElementIndex);
		FRawStaticIndexBuffer<TRUE>& ElementIndexBuffer = ElementIndexBuffers(ElementIndex);
		UINT FirstIndex = IndexBuffer.Indices.Num();

		if(ElementIndexBuffer.Indices.Num())
		{
			ElementIndexBuffer.CacheOptimize();

			WORD*	DestPtr = &IndexBuffer.Indices(IndexBuffer.Indices.Add(ElementIndexBuffer.Indices.Num()));
			WORD*	SrcPtr = &ElementIndexBuffer.Indices(0);

			Element.FirstIndex = FirstIndex;
			Element.NumTriangles = ElementIndexBuffer.Indices.Num() / 3;
			Element.MinVertexIndex = *SrcPtr;
			Element.MaxVertexIndex = *SrcPtr;

			for(INT Index = 0;Index < ElementIndexBuffer.Indices.Num();Index++)
			{
				Element.MinVertexIndex = Min<UINT>(*SrcPtr,Element.MinVertexIndex);
				Element.MaxVertexIndex = Max<UINT>(*SrcPtr,Element.MaxVertexIndex);

				*DestPtr++ = *SrcPtr++;
			}
		}
		else
		{
			Element.FirstIndex = 0;
			Element.NumTriangles = 0;
			Element.MinVertexIndex = 0;
			Element.MaxVertexIndex = 0;
		}
	}

	// Build a list of wireframe edges in the static mesh.
//	FEdgeBuilder(IndexBuffer.Indices,Vertices,Edges).FindEdges();
	FStaticMeshEdgeBuilder(IndexBuffer.Indices,Vertices,Edges).FindEdges();

	// Pre-size the wireframe indices array to avoid extra memcpys
	WireframeIndexBuffer.Indices.Empty(2 * Edges.Num());

	for(INT EdgeIndex = 0;EdgeIndex < Edges.Num();EdgeIndex++)
	{
		FMeshEdge&	Edge = Edges(EdgeIndex);

		WireframeIndexBuffer.Indices.AddItem(Edge.Vertices[0]);
		WireframeIndexBuffer.Indices.AddItem(Edge.Vertices[1]);
	}

	// Find triangles that aren't part of a closed mesh.

	TArray<INT>	SeparateTriangles;

	while(1)
	{
		INT	InitialSeparate = SeparateTriangles.Num();
		for(INT EdgeIndex = 0;EdgeIndex < Edges.Num();EdgeIndex++)
		{
			FMeshEdge&	Edge = Edges(EdgeIndex);
			if(Edge.Faces[1] == INDEX_NONE || SeparateTriangles.FindItemIndex(Edge.Faces[1]) != INDEX_NONE)
			{
				SeparateTriangles.AddUniqueItem(Edge.Faces[0]);
			}
			else if(SeparateTriangles.FindItemIndex(Edge.Faces[0]) != INDEX_NONE)
			{
				SeparateTriangles.AddUniqueItem(Edge.Faces[1]);
			}
		}
		if(SeparateTriangles.Num() == InitialSeparate)
		{
			break;
		}
	};

	ShadowTriangleDoubleSided.AddZeroed(IndexBuffer.Indices.Num() / 3);

	for(UINT TriangleIndex = 0;TriangleIndex < (UINT)SeparateTriangles.Num();TriangleIndex++)
	{
		ShadowTriangleDoubleSided(SeparateTriangles(TriangleIndex)) = 1;
	}

	RawTriangles.Unlock();
#else
	appErrorf(TEXT("FStaticMeshRenderData::Build should not be called on a console"));
#endif
}

IMPLEMENT_COMPARE_CONSTREF( FLOAT, UnStaticMeshBuild, { return (B - A) > 0 ? 1 : -1 ; } )


/**
 * Returns the scale dependent texture factor used by the texture streaming code.	
 *
 * @param RequestedUVIndex UVIndex to look at
 * @return scale dependent texture factor
 */
FLOAT UStaticMesh::GetStreamingTextureFactor( INT RequestedUVIndex )
{
	FLOAT StreamingTextureFactor = 0.f;
#if !CONSOLE

	FStaticMeshRenderData& StaticMeshRenderData = LODModels(0);

	FStaticMeshTriangle* RawTriangleData = (FStaticMeshTriangle*) StaticMeshRenderData.RawTriangles.Lock(LOCK_READ_ONLY);

	if( StaticMeshRenderData.RawTriangles.GetElementCount() )
	{
		TArray<FLOAT> TexelRatios;
		TexelRatios.Empty( StaticMeshRenderData.RawTriangles.GetElementCount() );
		
		// Figure out Unreal unit per texel ratios.
		for(INT TriangleIndex=0; TriangleIndex<StaticMeshRenderData.RawTriangles.GetElementCount(); TriangleIndex++ )
		{
			const FStaticMeshTriangle& Triangle = RawTriangleData[TriangleIndex];

			FLOAT	L1	= (Triangle.Vertices[0] - Triangle.Vertices[1]).Size(),
					L2	= (Triangle.Vertices[0] - Triangle.Vertices[2]).Size();

			INT	UVIndex = Min( Triangle.NumUVs - 1, RequestedUVIndex );

			FLOAT	T1	= (Triangle.UVs[0][UVIndex] - Triangle.UVs[1][UVIndex]).Size(),
					T2	= (Triangle.UVs[0][UVIndex] - Triangle.UVs[2][UVIndex]).Size();
		
			if( Abs(T1 * T2) > Square(SMALL_NUMBER) )
			{
				TexelRatios.AddItem( Max( L1 / T1, L2 / T2 ) );
			}
		}

		if( TexelRatios.Num() )
		{
			// Disregard upper 75% of texel ratios.
			Sort<USE_COMPARE_CONSTREF(FLOAT,UnStaticMeshBuild)>( &TexelRatios(0), TexelRatios.Num() );
			StreamingTextureFactor = TexelRatios( appTrunc(TexelRatios.Num() * 0.75f) );
		}
	}

	StaticMeshRenderData.RawTriangles.Unlock();
#else
	appErrorf(TEXT("UStaticMesh::GetStreamingTextureFactor should not be called on a console"));
#endif
	return StreamingTextureFactor;
}
