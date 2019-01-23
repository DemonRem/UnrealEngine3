/*=============================================================================
	StaticLightingVertexMapping.cpp: Static lighting vertex mapping implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "StaticLightingPrivate.h"

// Don't compile the static lighting system on consoles.
#if !CONSOLE

/** The maximum number of shadow samples per triangle. */
#define MAX_SHADOW_SAMPLES_PER_TRIANGLE	32

/**
 * Caches the vertices of a mesh.
 * @param Mesh - The mesh to cache vertices from.
 * @param OutVertices - Upon return, contains the meshes vertices.
 */
static void CacheVertices(const FStaticLightingMesh* Mesh,TArray<FStaticLightingVertex>& OutVertices,TMultiMap<FVector,INT>& OutVertexPositionMap)
{
	OutVertices.Empty(Mesh->NumVertices);
	OutVertices.Add(Mesh->NumVertices);

	for(INT TriangleIndex = 0;TriangleIndex < Mesh->NumTriangles;TriangleIndex++)
	{
		// Query the mesh for the triangle's vertices.
		FStaticLightingVertex V0;
		FStaticLightingVertex V1;
		FStaticLightingVertex V2;
		Mesh->GetTriangle(TriangleIndex,V0,V1,V2);
		INT I0 = 0;
		INT I1 = 0;
		INT I2 = 0;
		Mesh->GetTriangleIndices(TriangleIndex,I0,I1,I2);

		// Cache the vertices by vertex index.
		OutVertices(I0) = V0;
		OutVertices(I1) = V1;
		OutVertices(I2) = V2;

		// Also map the vertices by position.
		OutVertexPositionMap.AddUnique(V0.WorldPosition,I0);
		OutVertexPositionMap.AddUnique(V1.WorldPosition,I1);
		OutVertexPositionMap.AddUnique(V2.WorldPosition,I2);
	}
}

/**
 * Interpolates a triangle's vertices to find the attributes of a point in the triangle.
 * @param V0 - The triangle's first vertex.
 * @param V1 - The triangle's second vertex.
 * @param V2 - The triangle's third vertex.
 * @param S - The barycentric coordinates of the point on the triangle to derive.
 * @param T - The barycentric coordinates of the point on the triangle to derive.
 * @param U - The barycentric coordinates of the point on the triangle to derive.
 * @return A vertex representing the specified point on the triangle/
 */
static FStaticLightingVertex InterpolateTrianglePoint(const FStaticLightingVertex& V0,const FStaticLightingVertex& V1,const FStaticLightingVertex& V2,FLOAT S,FLOAT T,FLOAT U)
{
	FStaticLightingVertex Result;
	Result.WorldPosition =	S * V0.WorldPosition +		T * V1.WorldPosition +	U * V2.WorldPosition;
	Result.WorldTangentX =	S * V0.WorldTangentX +		T * V1.WorldTangentX +	U * V2.WorldTangentX;
	Result.WorldTangentY =	S * V0.WorldTangentY +		T * V1.WorldTangentY +	U * V2.WorldTangentY;
	Result.WorldTangentZ =	S * V0.WorldTangentZ +		T * V1.WorldTangentZ +	U * V2.WorldTangentZ;
	return Result;
}

void FStaticLightingSystem::ProcessVertexMapping(FStaticLightingVertexMapping* VertexMapping)
{
	const FStaticLightingMesh* const Mesh = VertexMapping->Mesh;
	FCoherentRayCache CoherentRayCache(Mesh);

	// Cache the mesh's vertex data, and build a map from position to indices of vertices at that position.
	TArray<FStaticLightingVertex> Vertices;
	TMultiMap<FVector,INT> VertexPositionMap;
	CacheVertices(Mesh,Vertices,VertexPositionMap);

	// Allocate shadow-map data.
	TMap<ULightComponent*,FShadowMapData1D*> ShadowMapData;
	TArray<FShadowMapData1D*> ShadowMapDataByLightIndex;
	ShadowMapDataByLightIndex.Empty(Mesh->RelevantLights.Num());
	ShadowMapDataByLightIndex.AddZeroed(Mesh->RelevantLights.Num());
	
	// Allocate light-map data.
	FLightMapData1D* LightMapData = new FLightMapData1D(Mesh->NumVertices);

	// Allocate the vertex sample weight total map.
	TArray<FLOAT> VertexSampleWeightTotals;
	VertexSampleWeightTotals.Empty(Mesh->NumVertices);
	VertexSampleWeightTotals.AddZeroed(Mesh->NumVertices);

	// Preallocate the adjacent vertex lists to avoid reallocating them for every triangle.
	TArray<INT> AdjacentVertexIndices[3];
	const UINT AdjacentVertexReserved = 8;
	for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
	{
		AdjacentVertexIndices[VertexIndex].Empty(AdjacentVertexReserved);
	}

	// Perform two passes over the mapping.
	// The first pass computes direct lighting and primes the area lighting cache.
	// The second pass applies the area lighting cache to the mapping.
	for(INT Pass = 0;Pass < 2;Pass++)
	{
		const UBOOL bApplyDirectLighting = (Pass == 0);
		const UBOOL bApplyAreaLighting = (Pass == 1);
		const UBOOL bComputeVertexWeights = (Pass == 0);

		// Setup a thread-safe random stream with a fixed seed, so the sample points are deterministic.
		FRandomStream RandomStream(0);

		// Allocate a map which keeps track of the vertices which don't have any adjacent triangles that have samples.
		FBitArray DirectlySampledVertexMap(TRUE,Mesh->NumVertices);

		if(!VertexMapping->bSampleVertices)
		{
			// Calculate light visibility for each triangle.
			for(INT TriangleIndex = 0;TriangleIndex < Mesh->NumTriangles;TriangleIndex++)
			{
				// Query the mesh for the triangle's vertex indices.
				INT I0 = 0;
				INT I1 = 0;
				INT I2 = 0;
				Mesh->GetTriangleIndices(TriangleIndex,I0,I1,I2);

				// Lookup the triangle's vertices.
				const FStaticLightingVertex TriangleVertices[3] =
				{
					Vertices(I0),
					Vertices(I1),
					Vertices(I2)
				};

				// Look up the vertices adjacent to this triangle at each vertex.
				for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
				{
					AdjacentVertexIndices[VertexIndex].Empty(AdjacentVertexReserved);
					VertexPositionMap.MultiFind(TriangleVertices[VertexIndex].WorldPosition,AdjacentVertexIndices[VertexIndex]);
				}

				// Compute the triangle's normal.
				const FVector TriangleNormal = (TriangleVertices[2].WorldPosition - TriangleVertices[0].WorldPosition) ^ (TriangleVertices[1].WorldPosition - TriangleVertices[0].WorldPosition);

				// Find the lights which are in front of this triangle.
				const FBitArray TriangleRelevantLightsMask = CullBackfacingLights(Mesh->bTwoSidedMaterial,TriangleVertices[0].WorldPosition,TriangleNormal,Mesh->RelevantLights);

				// Compute the triangle's area.
				const FLOAT TriangleArea = 0.5f * TriangleNormal.Size();

				// Compute the number of samples to use for the triangle, proportional to the triangle area.
				const INT NumSamples = Clamp(appTrunc(TriangleArea * VertexMapping->SampleToAreaRatio),0,MAX_SHADOW_SAMPLES_PER_TRIANGLE);

				// If this triangles samples, mark the adjacent vertices as not needing direct samples.
				if(NumSamples > 0)
				{
					for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
					{
						for(INT AdjacentVertexIndex = 0;AdjacentVertexIndex < AdjacentVertexIndices[VertexIndex].Num();AdjacentVertexIndex++)
						{
							const INT MeshVertexIndex = AdjacentVertexIndices[VertexIndex](AdjacentVertexIndex);
							DirectlySampledVertexMap(MeshVertexIndex) = FALSE;
						}
					}
				}

				// Sample the triangle's lighting.
				for(INT SampleIndex = 0;SampleIndex < NumSamples;SampleIndex++)
				{
					// Choose a uniformly distributed random point on the triangle.
					const FLOAT S = 1.0f - appSqrt(RandomStream.GetFraction());
					const FLOAT T = RandomStream.GetFraction() * (1.0f - S);
					const FLOAT U = 1 - S - T;

					// Index the sample's vertex indices and weights.
					const FLOAT TriangleVertexWeights[3] = { S, T, U };
					const INT TriangleVertexIndices[3] = { I0, I1, I2 };

					// Interpolate the triangle's vertex attributes at the sample point.
					const FStaticLightingVertex SampleVertex = 
						TriangleVertices[0] * S +
						TriangleVertices[1] * T +
						TriangleVertices[2] * U;

					// Compute the sample's area lighting.
					const FLightSample AreaLightSample = CalculatePointAreaLighting(VertexMapping,SampleVertex,CoherentRayCache);

					// Don't apply the area lighting in the first pass, since the area lighting cache hasn't been fully populated yet.
					if(bApplyAreaLighting)
					{
						// Add the sample's area lighting to the adjacent vertex light-maps.
						for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
						{
							for(INT AdjacentVertexIndex = 0;AdjacentVertexIndex < AdjacentVertexIndices[VertexIndex].Num();AdjacentVertexIndex++)
							{
								const INT MeshVertexIndex = AdjacentVertexIndices[VertexIndex](AdjacentVertexIndex);

								// Add the sample's contribution to the vertex's light-map value.
								(*LightMapData)(MeshVertexIndex).AddWeighted(AreaLightSample,TriangleVertexWeights[VertexIndex]);
							}
						}
					}

					if(bApplyDirectLighting)
					{
						// Compute the static shadowing and lighting for this vertex from the relevant lights.
						for(INT LightIndex = 0;LightIndex < Mesh->RelevantLights.Num();LightIndex++)
						{
							ULightComponent* Light = Mesh->RelevantLights(LightIndex);

							// Skip sky lights, since their static lighting is computed above.
							if(Light->IsA(USkyLightComponent::StaticClass()))
							{
								continue;
							}

							if(TriangleRelevantLightsMask(LightIndex))
							{
								// Compute the shadowing of this sample point from the light.
								const UBOOL bIsShadowed = CalculatePointShadowing(VertexMapping,SampleVertex.WorldPosition,Light,CoherentRayCache);

								if(!bIsShadowed)
								{
									// Determine whether to use a shadow-map or the light-map for this light.
									const UBOOL bUseStaticLighting = Light->UseStaticLighting(VertexMapping->bForceDirectLightMap);
									if(bUseStaticLighting)
									{
										// Add the light to the light-map's light list.
										LightMapData->Lights.AddUniqueItem(Light);
									}

									// Accumulate the sample lighting and shadowing at the adjacent vertices.
									for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
									{
										for(INT AdjacentVertexIndex = 0;AdjacentVertexIndex < AdjacentVertexIndices[VertexIndex].Num();AdjacentVertexIndex++)
										{
											const INT MeshVertexIndex = AdjacentVertexIndices[VertexIndex](AdjacentVertexIndex);
											const FStaticLightingVertex& AdjacentVertex = Vertices(MeshVertexIndex);

											if(bUseStaticLighting)
											{
												// Use the adjacent vertex's tangent basis to calculate this sample's contribution to its light-map value.
												FStaticLightingVertex AdjacentSampleVertex = SampleVertex;
												AdjacentSampleVertex.WorldTangentX = AdjacentVertex.WorldTangentX;
												AdjacentSampleVertex.WorldTangentY = AdjacentVertex.WorldTangentY;
												AdjacentSampleVertex.WorldTangentZ = AdjacentVertex.WorldTangentZ;

												// Calculate the sample's contribution to the vertex's light-map value.
												FLightSample LightMapSample = CalculatePointLighting(VertexMapping,AdjacentSampleVertex,Light);

												// Add the sample's contribution to the vertex's light-map value.
												(*LightMapData)(MeshVertexIndex).AddWeighted(LightMapSample,TriangleVertexWeights[VertexIndex]);
											}
											else
											{
												// Lookup the shadow-map used by this light.
												FShadowMapData1D* CurrentLightShadowMapData = ShadowMapDataByLightIndex(LightIndex);
												if(!CurrentLightShadowMapData)
												{
													// If this the first sample unshadowed from this light, create a shadow-map for it.
													CurrentLightShadowMapData = new FShadowMapData1D(Mesh->NumVertices);
													ShadowMapDataByLightIndex(LightIndex) = CurrentLightShadowMapData;
													ShadowMapData.Set(Light,CurrentLightShadowMapData);
												}

												// Accumulate the sample shadowing.
												(*CurrentLightShadowMapData)(MeshVertexIndex) += TriangleVertexWeights[VertexIndex];
											}
										}
									}
								}
							}
						}
					}
					
					if(bComputeVertexWeights)
					{
						// Accumulate the sample's weight for each of the triangle's vertices.
						for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
						{
							for(INT AdjacentVertexIndex = 0;AdjacentVertexIndex < AdjacentVertexIndices[VertexIndex].Num();AdjacentVertexIndex++)
							{
								const INT MeshVertexIndex = AdjacentVertexIndices[VertexIndex](AdjacentVertexIndex);
								VertexSampleWeightTotals(MeshVertexIndex) += TriangleVertexWeights[VertexIndex];
							}
						}
					}
				}
			}
		}

		// Calculate light visibility for vertices which had no samples (if bSampleVertices, or they are from small triangles).
		for(FBitArray::FConstSetBitIterator VertexIt(DirectlySampledVertexMap);VertexIt;++VertexIt)
		{
			const INT VertexIndex = VertexIt.GetIndex();
			const FStaticLightingVertex& SampleVertex = Vertices(VertexIndex);

			// Compute the sample's area lighting.
			const FLightSample AreaLighting = CalculatePointAreaLighting(VertexMapping,SampleVertex,CoherentRayCache);

			// Don't apply the area lighting in the first pass, since the area lighting cache hasn't been fully populated et.
			if(bApplyAreaLighting)
			{
				(*LightMapData)(VertexIndex).AddWeighted(AreaLighting,1.0f);
			}

			if(bApplyDirectLighting)
			{
				// Compute the shadow factors for this sample from the shadow-mapped lights.
				for(INT LightIndex = 0;LightIndex < Mesh->RelevantLights.Num();LightIndex++)
				{
					ULightComponent* Light = Mesh->RelevantLights(LightIndex);

					if(!IsLightBehindSurface(SampleVertex.WorldPosition,SampleVertex.WorldTangentZ,Light))
					{
						// Compute the shadowing of this sample point from the light.
						const UBOOL bIsShadowed = CalculatePointShadowing(VertexMapping,SampleVertex.WorldPosition,Light,CoherentRayCache);
						if(!bIsShadowed)
						{
							// Determine whether to use a shadow-map or the light-map for this light.
							const UBOOL bUseStaticLighting = Light->UseStaticLighting(VertexMapping->bForceDirectLightMap);
							if(bUseStaticLighting)
							{
								// Calculate the light-map sample.
								(*LightMapData)(VertexIndex).AddWeighted(CalculatePointLighting(VertexMapping,SampleVertex,Light),1.0f);
								LightMapData->Lights.AddUniqueItem(Light);
							}
							else
							{
								// Lookup the shadow-map used by this light.
								FShadowMapData1D* CurrentLightShadowMapData = CurrentLightShadowMapData = ShadowMapDataByLightIndex(LightIndex);
								if(!CurrentLightShadowMapData)
								{
									// If this the first sample unshadowed from this light, create a shadow-map for it.
									CurrentLightShadowMapData = new FShadowMapData1D(Mesh->NumVertices);
									ShadowMapDataByLightIndex(LightIndex) = CurrentLightShadowMapData;
									ShadowMapData.Set(Light,CurrentLightShadowMapData);
								}

								// Write the vertex's shadow-map value for this light.
								(*CurrentLightShadowMapData)(VertexIndex) = 1.0f;
							}
						}
					}
				}
			}

			if(bComputeVertexWeights)
			{
				// Set the vertex sample weight.
				VertexSampleWeightTotals(VertexIndex) = 1.0f;
			}
		}
	}

	// Renormalize the super-sampled vertex lighting and shadowing.
	for(INT VertexIndex = 0;VertexIndex < Mesh->NumVertices;VertexIndex++)
	{
		const FLOAT TotalWeight = VertexSampleWeightTotals(VertexIndex);
		if(TotalWeight > DELTA)
		{
			const FLOAT InvTotalWeight = 1.0f / VertexSampleWeightTotals(VertexIndex);

			// Renormalize the vertex's light-map sample.
			FLightSample RenormalizedLightSample;
			RenormalizedLightSample.AddWeighted((*LightMapData)(VertexIndex),InvTotalWeight);
			(*LightMapData)(VertexIndex) = RenormalizedLightSample;

			// Renormalize the vertex's shadow-map samples.
			for(TMap<ULightComponent*,FShadowMapData1D*>::TIterator ShadowMapIt(ShadowMapData);ShadowMapIt;++ShadowMapIt)
			{
				(*ShadowMapIt.Value())(VertexIndex) *= InvTotalWeight;
			}
		}
	}

	// Add the sky lights to the light-map's light list.
	for(INT LightIndex = 0;LightIndex < Mesh->RelevantLights.Num();LightIndex++)
	{
		ULightComponent* Light = Mesh->RelevantLights(LightIndex);
		if(Light->IsA(USkyLightComponent::StaticClass()))
		{
			LightMapData->Lights.AddUniqueItem(Light);
		}
	}

	// Enqueue the static lighting for application in the main thread.
	TList<FVertexMappingStaticLightingData>* StaticLightingLink = new TList<FVertexMappingStaticLightingData>(FVertexMappingStaticLightingData(),NULL);
	StaticLightingLink->Element.Mapping = VertexMapping;
	StaticLightingLink->Element.LightMapData = LightMapData;
	StaticLightingLink->Element.ShadowMaps = ShadowMapData;
	CompleteVertexMappingList.AddElement(StaticLightingLink);
}

#endif
