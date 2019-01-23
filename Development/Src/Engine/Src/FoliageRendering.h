/*=============================================================================
	FoliageRendering.h: Foliage rendering definitions.
	Copyright 2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __FOLIAGERENDERING_H__
#define __FOLIAGERENDERING_H__

// Include dependencies.
#include "LevelUtils.h"
#include "EngineFoliageClasses.h"
#include "ScenePrivate.h"
#include "FoliageVertexFactory.h"

// Forward declarations.
class FFoliageRenderResources;
class FFoliageClonedRenderResources;
class FFoliageInstancedRenderResources;

/** 
 * Data for a foliage instance that stores directional static lighting.
 */
struct FDirectionalFoliageInstance : public FFoliageInstanceBase
{
	FColor StaticLighting[NUM_DIRECTIONAL_LIGHTMAP_COEF];
	static const UINT NumCoefficients = NUM_DIRECTIONAL_LIGHTMAP_COEF;
	static const UINT CoefficientOffset = 0;
};

/** 
* Data for a foliage instance that stores simple static lighting.
*/
struct FSimpleFoliageInstance : public FFoliageInstanceBase
{
	FColor StaticLighting[NUM_SIMPLE_LIGHTMAP_COEF];
	static const UINT NumCoefficients = NUM_SIMPLE_LIGHTMAP_COEF;
	static const UINT CoefficientOffset = SIMPLE_LIGHTMAP_COEF_INDEX;
};

/** An index buffer containing the triangles of a foliage instance list. */
class FFoliageIndexBuffer : public FIndexBuffer
{
public:

	/** Initialization constructor. */
	FFoliageIndexBuffer(const FStaticMeshRenderData& InRenderData):
		RenderData(InRenderData),
		MaxInstances(0)
	{}

	/** Sets the maximum number of instances that will be drawn with this index buffer. */
	void SetMaxInstances(INT InMaxInstances);

	// FRenderResource interface.
	virtual void InitRHI();

private:

	/** The foliage mesh's render data. */
	const FStaticMeshRenderData& RenderData;
	
	/** The maximum number of instances the index buffer will be used to render. */
	INT MaxInstances;
};

/** The base implementation of render resources for a foliage component. */
template<typename FoliageRenderDataPolicy>
class TFoliageRenderData : public FoliageRenderDataPolicy
{
public:

	typedef typename FoliageRenderDataPolicy::InstanceType InstanceType;
	typedef typename FoliageRenderDataPolicy::InstanceReferenceType InstanceReferenceType;

	/** A vertex buffer containing foliage instance data. */
	class FFoliageInstanceBuffer: public FVertexBuffer
	{
	public:

		/** Initialization constructor. */
		FFoliageInstanceBuffer(const TFoliageRenderData& InRenderResources):
			RenderResources(InRenderResources)
		{}

		// FRenderResource interface.
		virtual void ReleaseDynamicRHI()
		{
			VertexBufferRHI.Release();
		}

		virtual FString GetFriendlyName() const { return TEXT("Foliage instances"); }

		/**
		 * Writes the instances for a specific view point to the vertex buffer.
		 * @param VisibleFoliageInstances - The indices of the foliage instances which are visible in the view.
		 * @param View - The view to write the instances for.
		 */
		void WriteInstances(const TArray<InstanceReferenceType>& VisibleFoliageInstances,const FSceneView* View)
		{
			SCOPE_CYCLE_COUNTER(STAT_TerrainFoliageTime);

			// Compute the size of the visible vertices.
			const UINT VisibleSize = sizeof(InstanceType) * VisibleFoliageInstances.Num();

			if(VisibleSize > 0)
			{
				// Create the RHI vertex buffer.
				VertexBufferRHI = RHICreateVertexBuffer(VisibleSize,NULL,TRUE);

				// Lock the vertex buffer.
				void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI,0,VisibleSize);
				InstanceType* DestInstance = (InstanceType*)VertexBufferData;

				// Get the list of the wind sources in the scene.
				const TArray<FWindSourceSceneProxy*>& WindSources = View->Family->Scene->GetWindSources_RenderThread();

				// Generate the instance data.
				for(INT VisibleInstanceIndex = 0;VisibleInstanceIndex < VisibleFoliageInstances.Num();VisibleInstanceIndex++)
				{
					// Generate the instance data.
					FLOAT WindScale = 1.0f;
					InstanceType LocalInstance;
					RenderResources.GetInstance(
						VisibleFoliageInstances(VisibleInstanceIndex),
						View,
						LocalInstance,
						WindScale
						);

					// Apply the wind skew for this instance.
					FVector WindSkew(0,0,0);
					for(INT SourceIndex = 0;SourceIndex < WindSources.Num();SourceIndex++)
					{
						WindSkew += WindSources(SourceIndex)->GetWindSkew(LocalInstance.Location,View->Family->CurrentWorldTime);
					}
					LocalInstance.ZAxis += WindSkew * WindScale;

					// Write the instance to the buffer.
					*DestInstance++ = LocalInstance;
				}

				// Unlock the vertex buffer.
				RHIUnlockVertexBuffer(VertexBufferRHI);
			}
		}

	private:

		const TFoliageRenderData& RenderResources;
	}; 

	/** Initialization constructor. */
	TFoliageRenderData(const FoliageRenderDataPolicy& InRenderPolicy):
		FoliageRenderDataPolicy(InRenderPolicy),
		InstanceBuffer(*this),
		IndexBuffer(InRenderPolicy.GetMeshRenderData())
	{}

	/** Destructor. */
	~TFoliageRenderData()
	{
		InstanceBuffer.Release();
		FoliageVertexFactory.Release();
		IndexBuffer.Release();
	}

	/** Initialized the vertex factory for a specific number of instances. */
	void InitResources(UINT NumInstances)
	{
		// Initialize the instance buffer.
		InstanceBuffer.Init();

		// Initialize the index buffer.
		IndexBuffer.SetMaxInstances(FoliageRenderDataPolicy::GetMaxInstances());
		IndexBuffer.Init();

		const FStaticMeshRenderData& FoliageMeshRenderData = FoliageRenderDataPolicy::GetMeshRenderData();
		FFoliageVertexFactory::DataType FoliageVertexFactoryData;

		FoliageVertexFactoryData.PositionComponent = FVertexStreamComponent(
			&FoliageMeshRenderData.PositionVertexBuffer,
			STRUCT_OFFSET(FPositionVertex,Position),
			FoliageMeshRenderData.PositionVertexBuffer.GetStride(),
			VET_Float3
			);
		FoliageVertexFactoryData.TangentBasisComponents[0] = FVertexStreamComponent(
			&FoliageMeshRenderData.VertexBuffer,
			STRUCT_OFFSET(FStaticMeshFullVertex,TangentX),
			FoliageMeshRenderData.VertexBuffer.GetStride(),
			VET_PackedNormal
			);
		FoliageVertexFactoryData.TangentBasisComponents[1] = FVertexStreamComponent(
			&FoliageMeshRenderData.VertexBuffer,
			STRUCT_OFFSET(FStaticMeshFullVertex,TangentZ),
			FoliageMeshRenderData.VertexBuffer.GetStride(),
			VET_PackedNormal
			);
		if( !FoliageMeshRenderData.VertexBuffer.GetUseFullPrecisionUVs() )
		{
			FoliageVertexFactoryData.TextureCoordinateComponent = FVertexStreamComponent(
				&FoliageMeshRenderData.VertexBuffer,
				STRUCT_OFFSET(TStaticMeshFullVertexFloat16UVs<MAX_TEXCOORDS>,UVs[0]),
				FoliageMeshRenderData.VertexBuffer.GetStride(),
				VET_Half2
				);
		}
		else
		{
			FoliageVertexFactoryData.TextureCoordinateComponent = FVertexStreamComponent(
				&FoliageMeshRenderData.VertexBuffer,
				STRUCT_OFFSET(TStaticMeshFullVertexFloat32UVs<MAX_TEXCOORDS>,UVs[0]),
				FoliageMeshRenderData.VertexBuffer.GetStride(),
				VET_Float2
				);
		}
		
		FoliageVertexFactoryData.InstanceOffsetComponent = FVertexStreamComponent(
			&InstanceBuffer,
			STRUCT_OFFSET(InstanceType,Location),
			sizeof(InstanceType),
			VET_Float3,
			TRUE
			);
		FoliageVertexFactoryData.InstanceAxisComponents[0] = FVertexStreamComponent(
			&InstanceBuffer,
			STRUCT_OFFSET(InstanceType,XAxis),
			sizeof(InstanceType),
			VET_Float3,
			TRUE
			);
		FoliageVertexFactoryData.InstanceAxisComponents[1] = FVertexStreamComponent(
			&InstanceBuffer,
			STRUCT_OFFSET(InstanceType,YAxis),
			sizeof(InstanceType),
			VET_Float3,
			TRUE
			);
		FoliageVertexFactoryData.InstanceAxisComponents[2] = FVertexStreamComponent(
			&InstanceBuffer,
			STRUCT_OFFSET(InstanceType,ZAxis),
			sizeof(InstanceType),
			VET_Float3,
			TRUE
			);

		FoliageVertexFactoryData.NumVerticesPerInstance = FoliageMeshRenderData.NumVertices;
		FoliageVertexFactoryData.NumInstances = NumInstances;

		// Allow the render data policy to initialize the vertex factory's static lighting components.
		InitVertexFactoryStaticLighting(&InstanceBuffer,FoliageVertexFactoryData);

		FoliageVertexFactory.SetData(FoliageVertexFactoryData);
		FoliageVertexFactory.Init();
	}

	/**
	 * Updates the foliage instances for a new viewpoint.
	 * @param VisibleFoliageInstances - The indices of the foliage instances which are visible in the view.
	 * @param View - The view to write the instances for.
	 */
	void UpdateInstances(const TArray<InstanceReferenceType>& VisibleFoliageInstances,const FSceneView* View)
	{
		InitResources(VisibleFoliageInstances.Num());
		InstanceBuffer.WriteInstances(VisibleFoliageInstances,View);
	}

	/** Accesses the vertex factory used to render the foliage. */
	const FVertexFactory* GetVertexFactory()
	{
		return &FoliageVertexFactory;
	}

	/** Accesses the vertex buffer containing the foliage static lighting. */
	const FVertexBuffer* GetStaticLightingVertexBuffer() const
	{
		return &InstanceBuffer;
	}

	/** Accesses the index buffer containing the foliage mesh indices. */
	const FIndexBuffer* GetIndexBuffer() const
	{
		return &IndexBuffer;
	}

private:
	
	/** The vertex buffer used to hold the foliage instance data. */
	FFoliageInstanceBuffer InstanceBuffer;

	/** The vertex factory used to render the component's foliage instances. */
	FFoliageVertexFactory FoliageVertexFactory;

	/** The index buffer used to hold the component's foliage instance indices. */
	FFoliageIndexBuffer IndexBuffer;
};

#endif
