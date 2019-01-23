/*=============================================================================
	LightingCache.h: Lighting cache definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __LIGHTING_CACHE_H__
#define __LIGHTING_CACHE_H__

#include "GenericOctree.h"

/** A lighting cache. */
class FLightingCache
{
public:

	/** The irradiance for a single static lighting vertex. */
	class FRecord
	{
	public:

		/** The static lighting vertex the irradiance record was computed for. */
		FStaticLightingVertex Vertex;

		/** The radius around the vertex that the record ir relevant to. */
		FLOAT Radius;

		/** The lighting incident on an infinitely small surface at WorldPosition facing along WorldNormal. */
		FLightSample Lighting;

		/** Initialization constructor. */
		FRecord(const FStaticLightingVertex& InVertex,FLOAT InRadius,const FLightSample& InLighting):
			Vertex(InVertex),
			Radius(InRadius),
			Lighting(InLighting)
		{}

		/** The octree semantics for irradiance records. */
		struct OctreeSemantics
		{
			enum { MaxElementsPerNode = 4 };
			enum { MaxNodeDepth = 12 };

			static FBox GetBoundingBox(const FRecord& LightingRecord)
			{
				return FBox(
					LightingRecord.Vertex.WorldPosition - FVector(1,1,1) * LightingRecord.Radius,
					LightingRecord.Vertex.WorldPosition + FVector(1,1,1) * LightingRecord.Radius
					);
			}
		};
	};

	/** Adds a lighting record to the cache. */
	void AddRecord(const FRecord& Record);

	/**
	 * Interpolates nearby lighting records for a vertex.
	 * @param Vertex - The vertex to interpolate the lighting for.
	 * @param OutLighting - If TRUE is returned, contains the blended lighting records that were found near the point.
	 * @return TRUE if nearby records were found with enough relevance to interpolate this point's lighting.
	 */
	UBOOL InterpolateLighting(const FStaticLightingVertex& Vertex,FLightSample& OutLighting) const;

	/** Initialization constructor. */
	FLightingCache(const FBox& InBoundingBox);

private:

	/** The type of lighting cache octree nodes. */
	typedef TOctree<FRecord,FRecord::OctreeSemantics> LightingOctreeType;

	/** The lighting cache octree. */
	LightingOctreeType Octree;
};

#endif
