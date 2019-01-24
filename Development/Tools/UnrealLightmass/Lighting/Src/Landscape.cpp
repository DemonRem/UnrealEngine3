/*=============================================================================
	Landscape.cpp: Static lighting Landscape mesh/mapping implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "stdafx.h"
#include "Importer.h"
#include "Landscape.h"

namespace Lightmass
{
	// Accessors from FLandscapeDataInterface
	void FLandscapeStaticLightingMesh::VertexIndexToXY(INT VertexIndex, INT& OutX, INT& OutY) const
	{
		OutX = VertexIndex % (ComponentSizeQuads + 2*ExpandQuadsX + 1);
		OutY = VertexIndex / (ComponentSizeQuads + 2*ExpandQuadsY + 1);
	}

	void FLandscapeStaticLightingMesh::QuadIndexToXY(INT QuadIndex, INT& OutX, INT& OutY) const
	{
		OutX = QuadIndex % (ComponentSizeQuads + 2*ExpandQuadsX);
		OutY = QuadIndex / (ComponentSizeQuads + 2*ExpandQuadsY);
	}

	const FColor* FLandscapeStaticLightingMesh::GetHeightData( INT LocalX, INT LocalY ) const
	{
		return &HeightMap(LocalX + LocalY * (ComponentSizeQuads + 1 + 2*ExpandQuadsX) );
	}

	void FLandscapeStaticLightingMesh::GetWorldPositionTangents( INT LocalX, INT LocalY, FVector4& WorldPos, FVector4& WorldTangentX, FVector4& WorldTangentY, FVector4& WorldTangentZ ) const
	{
		// clamping LocalX, LocalY
		INT ClampedLocalX = LocalX-ExpandQuadsX; //Clamp(LocalX-ExpandQuadsX, 0, ComponentSizeQuads);
		INT ClampedLocalY = LocalY-ExpandQuadsY; //Clamp(LocalY-ExpandQuadsY, 0, ComponentSizeQuads);

		const FColor* Data = GetHeightData( LocalX, LocalY );
		//const FColor* Data2 = GetHeightData( ClampedLocalX+ExpandQuadsX, ClampedLocalY+ExpandQuadsY );

		WorldTangentZ.X = 2.f * (FLOAT)Data->B / 255.f - 1.f;
		WorldTangentZ.Y = 2.f * (FLOAT)Data->A / 255.f - 1.f;
		WorldTangentZ.Z = appSqrt(1.f - (Square(WorldTangentZ.X)+Square(WorldTangentZ.Y)));
		WorldTangentZ = WorldTangentZ.SafeNormal();
		WorldTangentX = FVector4(WorldTangentZ.Z, 0.f, -WorldTangentZ.X).SafeNormal();
		//WorldTangentY = FVector4(0.f, WorldTangentZ.Z, -WorldTangentZ.Y);
		WorldTangentY = (WorldTangentZ ^ WorldTangentX).SafeNormal();

		// Assume there is no rotation, so we don't need to do any LocalToWorld.
		WORD Height = (Data->R << 8) + Data->G;
		//WORD Height = (Data2->R << 8) + Data2->G;

		WorldPos = LocalToWorld.TransformFVector( FVector4( ClampedLocalX, ClampedLocalY, ((FLOAT)Height - 32768.f) * LANDSCAPE_ZSCALE ) );
		//WorldPos = LocalToWorld.TransformFVector( FVector4( LocalX-ExpandQuadsX, LocalY-ExpandQuadsY, ((FLOAT)Height - 32768.f) * LANDSCAPE_ZSCALE ) );
		//debugf(TEXT("%d, %d, %d, %d, %d, %d, X:%f, Y:%f, Z:%f "), SectionBaseX+LocalX-ExpandQuadsX, SectionBaseY+LocalY-ExpandQuadsY, ClampedLocalX, ClampedLocalY, SectionBaseX, SectionBaseY, WorldPos.X, WorldPos.Y, WorldPos.Z);
	}

	void FLandscapeStaticLightingMesh::GetTriangleIndices(INT QuadIndex,INT TriNum,INT& OutI0,INT& OutI1,INT& OutI2) const
	{
		INT QuadX, QuadY;
		QuadIndexToXY( QuadIndex, QuadX, QuadY );
		switch(TriNum)
		{
		case 0:
			OutI0 = (QuadX + 0) + (QuadY + 0) * (ComponentSizeQuads+1 + 2*ExpandQuadsX);
			OutI1 = (QuadX + 1) + (QuadY + 1) * (ComponentSizeQuads+1 + 2*ExpandQuadsX);
			OutI2 = (QuadX + 1) + (QuadY + 0) * (ComponentSizeQuads+1 + 2*ExpandQuadsX);
			break;
		case 1:
			OutI0 = (QuadX + 0) + (QuadY + 0) * (ComponentSizeQuads+1 + 2*ExpandQuadsX);
			OutI1 = (QuadX + 0) + (QuadY + 1) * (ComponentSizeQuads+1 + 2*ExpandQuadsX);
			OutI2 = (QuadX + 1) + (QuadY + 1) * (ComponentSizeQuads+1 + 2*ExpandQuadsX);
			break;
		}
	}

	// from FStaticLightMesh....
	void FLandscapeStaticLightingMesh::GetStaticLightingVertex(INT VertexIndex, FStaticLightingVertex& OutVertex) const
	{	
		INT X, Y;
		VertexIndexToXY(VertexIndex, X, Y);
		GetWorldPositionTangents(X, Y, OutVertex.WorldPosition, OutVertex.WorldTangentX, OutVertex.WorldTangentY, OutVertex.WorldTangentZ);

		INT LightmapUVIndex = 1;
		
		INT LocalX = Clamp<INT>(X-ExpandQuadsX, 0, ComponentSizeQuads);
		INT LocalY = Clamp<INT>(Y-ExpandQuadsY, 0, ComponentSizeQuads);

		OutVertex.TextureCoordinates[0] = FVector2D((FLOAT)X / (FLOAT)(ComponentSizeQuads), (FLOAT)Y / (FLOAT)(ComponentSizeQuads));

		OutVertex.TextureCoordinates[LightmapUVIndex].X = (FLOAT)(X * StaticLightingResolution + 0.5f) * LightMapRatio / (FLOAT)((2*ExpandQuadsX + ComponentSizeQuads + 1) * StaticLightingResolution); //SizeX;
		OutVertex.TextureCoordinates[LightmapUVIndex].Y = (FLOAT)(Y * StaticLightingResolution + 0.5f) * LightMapRatio / (FLOAT)((2*ExpandQuadsY + ComponentSizeQuads + 1) * StaticLightingResolution); //SizeY;
	}

	// FStaticLightingMesh interface.
	void FLandscapeStaticLightingMesh::GetTriangle(INT TriangleIndex,FStaticLightingVertex& OutV0,FStaticLightingVertex& OutV1,FStaticLightingVertex& OutV2,INT& ElementIndex) const
	{
		INT I0, I1, I2;
		GetTriangleIndices(TriangleIndex,I0, I1, I2);
		GetStaticLightingVertex(I0,OutV0);
		GetStaticLightingVertex(I1,OutV1);
		GetStaticLightingVertex(I2,OutV2);
		ElementIndex = 0;
	}

	void FLandscapeStaticLightingMesh::GetTriangleIndices(INT TriangleIndex,INT& OutI0,INT& OutI1,INT& OutI2) const
	{
		INT QuadIndex = TriangleIndex >> 1;
		INT QuadTriIndex = TriangleIndex & 1;

		GetTriangleIndices(QuadIndex, QuadTriIndex, OutI0, OutI1, OutI2);
	}

	void FLandscapeStaticLightingMesh::Import( class FLightmassImporter& Importer )
	{
		// import super class
		FStaticLightingMesh::Import( Importer );
		Importer.ImportData((FLandscapeStaticLightingMeshData*) this);

		// we have the guid for the mesh, now hook it up to the actual static mesh
		INT ReadSize = Square(ComponentSizeQuads + 2*ExpandQuadsX + 1);
		checkf(ReadSize > 0, TEXT("Failed to import Landscape Heightmap data!"));
		Importer.ImportArray(HeightMap, ReadSize);
		check(HeightMap.Num() == ReadSize);
	}

	void FLandscapeStaticLightingTextureMapping::Import( class FLightmassImporter& Importer )
	{
		FStaticLightingTextureMapping::Import(Importer);

		// Can't use the FStaticLightingMapping Import functionality for this
		// as it only looks in the StaticMeshInstances map...
		Mesh = Importer.GetLandscapeMeshInstances().FindRef(Guid);
		check(Mesh);
	}

} //namespace Lightmass
