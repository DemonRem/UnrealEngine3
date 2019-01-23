/*=============================================================================
	Landscape.h: Static lighting Landscape mesh/mapping definitions.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#pragma once

#define LANDSCAPE_ZSCALE	(1.0f/128.0f)

namespace Lightmass
{
	/** Represents the triangles of a Landscape primitive to the static lighting system. */
	class FLandscapeStaticLightingMesh : public FStaticLightingMesh, public FLandscapeStaticLightingMeshData
	{
	public:	
		void GetStaticLightingVertex(INT VertexIndex, FStaticLightingVertex& OutVertex) const;
		// FStaticLightingMesh interface.
		virtual void GetTriangle(INT TriangleIndex,FStaticLightingVertex& OutV0,FStaticLightingVertex& OutV1,FStaticLightingVertex& OutV2,INT& ElementIndex) const;
		virtual void GetTriangleIndices(INT TriangleIndex,INT& OutI0,INT& OutI1,INT& OutI2) const;

		virtual void Import( class FLightmassImporter& Importer );

		// Accessors from FLandscapeDataInterface
		void			VertexIndexToXY(INT VertexIndex, INT& OutX, INT& OutY) const;
		void			QuadIndexToXY(INT QuadIndex, INT& OutX, INT& OutY) const;
		const FColor*	GetHeightData( INT LocalX, INT LocalY ) const;
//		FVector4		GetWorldVertex( INT LocalX, INT LocalY ) const;
//		FVector4		GetWorldVertex( INT VertexIndex ) const;
		void			GetWorldPositionTangents( INT LocalX, INT LocalY, FVector4& WorldPos, FVector4& WorldTangentX, FVector4& WorldTangentY, FVector4& WorldTangentZ ) const;
		void			GetWorldPositionTangents( INT VertexIndex, FVector4& WorldPos, FVector4& WorldTangentX, FVector4& WorldTangentY, FVector4& WorldTangentZ ) const;
		void			GetTriangleIndices(INT QuadX,INT QuadY,INT TriNum,INT& OutI0,INT& OutI1,INT& OutI2) const;
		void			GetTriangleIndices(INT QuadIndex,INT TriNum,INT& OutI0,INT& OutI1,INT& OutI2) const;

	private:
		TArray<FColor> HeightMap;	
	};

	/** Represents a landscape primitive with texture mapped static lighting. */
	class FLandscapeStaticLightingTextureMapping : public FStaticLightingTextureMapping
	{
	public:
		virtual void Import( class FLightmassImporter& Importer );
	};

} //namespace Lightmass
