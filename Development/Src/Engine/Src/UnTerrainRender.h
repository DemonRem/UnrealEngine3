/*=============================================================================
	UnTerrainRender.h: Definitions and inline code for rendering TerrainComponet
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef TERRAIN_RENDER_HEADER
#define TERRAIN_RENDER_HEADER

#include "FoliageRendering.h"
#include "ScenePrivate.h"
#include "GenericOctree.h"

#if __INTEL_BYTE_ORDER__ || PS3
	#define	UBYTE4_BYTEORDER_XYZW		1
#else
	#define	UBYTE4_BYTEORDER_XYZW		0
#endif

// Forward declarations.
class FDecalInteraction;
class FDecalState;
class FDynamicTerrainData;
class FTerrainComponentSceneProxy;
struct FTerrainObject;
class UTerrainComponent;

/**
 *	Flags identifying morphing setup.
 */
enum ETerrainMorphing
{
	/** No morphing is applied to this terrain.			*/
	ETMORPH_Disabled	= 0x00,
	/** Morph the terrain height.						*/
	ETMORPH_Height		= 0x01,
	/** Morph the terrain gradients.					*/
	ETMORPH_Gradient	= 0x02,
	/** Morph both the terrain height and gradients.	*/
	ETMORPH_Full		= ETMORPH_Height | ETMORPH_Gradient
};

/**
 *	FTerrainVertex
 *	The vertex structure used for terrain. 
 */
struct FTerrainVertex
{
#if UBYTE4_BYTEORDER_XYZW
	BYTE	X,
			Y,
			Z_LOBYTE,
			Z_HIBYTE;
#else
	BYTE	Z_HIBYTE,
			Z_LOBYTE,
			Y,
			X;
#endif
	FLOAT	Displacement;
	SWORD	GradientX,
			GradientY;
};

/**
 *	FTerrainMorphingVertex
 *	Vertex structure used when morphing terrain.
 *	Contains the transitional Height values
 */
struct FTerrainMorphingVertex : FTerrainVertex
{
#if UBYTE4_BYTEORDER_XYZW
	BYTE	TESS_DATA_INDEX_LO,
			TESS_DATA_INDEX_HI,
			Z_TRANS_LOBYTE,
			Z_TRANS_HIBYTE;
#else
	BYTE	Z_TRANS_HIBYTE,
			Z_TRANS_LOBYTE,
			TESS_DATA_INDEX_HI,
			TESS_DATA_INDEX_LO;
#endif
};

/**
 *	FTerrainFullMorphingVertex
 *	Vertex structure used when morphing terrain.
 *	Contains the transitional Height and Gradient values
 */
struct FTerrainFullMorphingVertex : FTerrainMorphingVertex
{
	SWORD	TransGradientX,
			TransGradientY;
};

//
//	FTerrainVertexBuffer
//
struct FTerrainVertexBuffer: FVertexBuffer
{
	/**
	 * Constructor.
	 * @param InMeshRenderData pointer to parent structure
	 */
	FTerrainVertexBuffer(const FTerrainObject* InTerrainObject, const UTerrainComponent* InComponent, INT InMaxTessellation, UBOOL bInIsDynamic = FALSE) :
		  bIsDynamic(bInIsDynamic)
		, TerrainObject(InTerrainObject)
		, Component(InComponent)
		, MaxTessellation(InMaxTessellation)
		, MaxVertexCount(0)
		, CurrentTessellation(-1)
		, VertexCount(0)
		, bRepackRequired(bInIsDynamic)
		, bIsCollisionLevel(FALSE)
		, MorphingFlags(ETMORPH_Disabled)
	{
		if (InComponent)
		{
			ATerrain* Terrain = InComponent->GetTerrain();
			if (Terrain)
			{
				if (Terrain->bMorphingEnabled)
				{
					MorphingFlags = ETMORPH_Height;
					if (Terrain->bMorphingGradientsEnabled)
					{
						MorphingFlags = ETMORPH_Full;
					}
				}
			}
		}
	}

	// FRenderResource interface.
	virtual void InitRHI();

	/** 
	 * Initialize the dynamic RHI for this rendering resource 
	 */
	virtual void InitDynamicRHI();

	/** 
	 * Release the dynamic RHI for this rendering resource 
	 */
	virtual void ReleaseDynamicRHI();

	virtual FString GetFriendlyName() const
	{
		return TEXT("Terrain component vertices");
	}

	inline INT GetMaxTessellation()		{	return MaxTessellation;		}
	inline INT GetMaxVertexCount()		{	return MaxVertexCount;		}
	inline INT GetCurrentTessellation()	{	return CurrentTessellation;	}
	inline INT GetVertexCount()			{	return VertexCount;			}
	inline UBOOL GetRepackRequired()	{	return bRepackRequired;		}
	inline void ClearRepackRequired()	{	bRepackRequired = FALSE;	}

	virtual void SetCurrentTessellation(INT InCurrentTessellation)
	{
		CurrentTessellation = Clamp<INT>(InCurrentTessellation, 0, MaxTessellation);
	}

	virtual UBOOL FillData(INT TessellationLevel);

	void SetIsCollisionLevel(UBOOL bIsCollision)
	{
		bIsCollisionLevel = bIsCollision;
	}

private:
	/** Flag indicating it is dynamic						*/
	UBOOL				bIsDynamic;
	/** The owner terrain object							*/
	const FTerrainObject*		TerrainObject;
	/** The 'owner' component								*/
	const UTerrainComponent*	Component;
	/** The maximum tessellation to create vertices for		*/
	INT					MaxTessellation;
	/** The maximum number of vertices in the buffer		*/
	INT					MaxVertexCount;
	/** The maximum tessellation to create vertices for		*/
	INT					CurrentTessellation;
	/** The number of vertices in the buffer				*/
	INT					VertexCount;
	/** A repack is required								*/
	UBOOL				bRepackRequired;
	/** Flag indicating it is for rendering the collision	*/
	UBOOL				bIsCollisionLevel;
	/** Flag indicating it is for morphing terrain			*/
	BYTE				MorphingFlags;
};


//
//	FTerrainFullVertexBuffer
//
struct FTerrainFullVertexBuffer : FTerrainVertexBuffer
{
	/**
	 * Constructor.
	 * @param InMeshRenderData pointer to parent structure
	 */
	FTerrainFullVertexBuffer(const FTerrainObject* InTerrainObject, const UTerrainComponent* InComponent, INT InMaxTessellation) :
		  FTerrainVertexBuffer(InTerrainObject, InComponent, InMaxTessellation, FALSE)
	{
	}

	// FRenderResource interface.
	virtual void InitRHI()
	{
		FTerrainVertexBuffer::InitRHI();
	}

	/** 
	 * Initialize the dynamic RHI for this rendering resource 
	 */
	virtual void InitDynamicRHI()
	{
		// Do NOTHING for the FullVertexBuffer
	}

	/** 
	 * Release the dynamic RHI for this rendering resource 
	 */
	virtual void ReleaseDynamicRHI()
	{
		// Do NOTHING for the FullVertexBuffer
	}

	virtual FString GetFriendlyName() const
	{
		return TEXT("Terrain FULL component vertices");
	}

	virtual void SetCurrentTessellation(INT InCurrentTessellation)
	{
		// Do NOTHING for the FullVertexBuffer
	}

	virtual UBOOL FillData(INT TessellationLevel)
	{
		return FTerrainVertexBuffer::FillData(TessellationLevel);
	}
};

// Forward declarations.
struct FTerrainIndexBuffer;
struct TerrainTessellationIndexBufferType;
struct TerrainDecalTessellationIndexBufferType;

//
//	FTerrainObject
//
struct FTerrainObject : public FDeferredCleanupInterface
{
public:
	FTerrainObject(UTerrainComponent* InTerrainComponent, INT InMaxTessellation) :
		  bIsInitialized(FALSE)
		, bIsDeadInGameThread(FALSE)
		, bIsShowingCollision(FALSE)
		, MorphingFlags(ETMORPH_Disabled)
	    , TerrainComponent(InTerrainComponent)
		, TessellationLevels(NULL)
		, VertexFactory(NULL)
		, DecalVertexFactory(NULL)
		, VertexBuffer(NULL)
		, FullVertexBuffer(NULL)
		, FullIndexBuffer(NULL)
		, CollisionVertexFactory(NULL)
		, CollisionVertexBuffer(NULL)
		, CollisionSmoothIndexBuffer(NULL)
	{
		check(TerrainComponent);
		ATerrain* Terrain = TerrainComponent->GetTerrain();
		if (Terrain)
		{
			bIsShowingCollision = Terrain->bShowingCollision;
			if (Terrain->bMorphingEnabled)
			{
				MorphingFlags = ETMORPH_Height;
				if (Terrain->bMorphingGradientsEnabled)
				{
					MorphingFlags = ETMORPH_Full;
				}
			}
		}
		Init();
	}
	
	virtual ~FTerrainObject();

	void Init();

	virtual void InitResources();
	virtual void ReleaseResources();
	virtual void Update();
	virtual const FVertexFactory* GetVertexFactory() const;

	//@todo.SAS. Remove this! START
	UBOOL GetIsDeadInGameThread()
	{
		return bIsDeadInGameThread;
	}
	void SetIsDeadInGameThread(UBOOL bInIsDeadInGameThread)
	{
		bIsDeadInGameThread = bInIsDeadInGameThread;
	}
	//@todo.SAS. Remove this! STOP

#if 1	//@todo. Remove these as we depend on the component anyway!
	inline INT		GetComponentSectionSizeX() const		{	return ComponentSectionSizeX;		}
	inline INT		GetComponentSectionSizeY() const		{	return ComponentSectionSizeY;		}
	inline INT		GetComponentSectionBaseX() const		{	return ComponentSectionBaseX;		}
	inline INT		GetComponentSectionBaseY() const		{	return ComponentSectionBaseY;		}
	inline INT		GetComponentTrueSectionSizeX() const	{	return ComponentTrueSectionSizeX;	}
	inline INT		GetComponentTrueSectionSizeY() const	{	return ComponentTrueSectionSizeY;	}
#endif	//#if 1	//@todo. Remove these as we depend on the component anyway!
	inline INT		GetNumVerticesX() const					{	return NumVerticesX;				}
	inline INT		GetNumVerticesY() const					{	return NumVerticesY;				}
	inline INT		GetMaxTessellationLevel() const			{	return MaxTessellationLevel;		}
	inline FLOAT	GetTerrainHeightScale() const			{	return TerrainHeightScale;			}
	inline FLOAT	GetTessellationDistanceScale() const	{	return TessellationDistanceScale;	}
	inline BYTE		GetTessellationLevel(INT Index) const	{	return TessellationLevels[Index];	}
	inline INT		GetLightMapResolution() const			{	return LightMapResolution;			}
	inline FLOAT	GetShadowCoordinateScaleX() const		{	return ShadowCoordinateScale.X;		}
	inline FLOAT	GetShadowCoordinateScaleY() const		{	return ShadowCoordinateScale.Y;		}
	inline FLOAT	GetShadowCoordinateBiasX() const		{	return ShadowCoordinateBias.X;		}
	inline FLOAT	GetShadowCoordinateBiasY() const		{	return ShadowCoordinateBias.Y;		}
	inline FMatrix&	GetLocalToWorld() const				
	{
		check(TerrainComponent);
		return TerrainComponent->LocalToWorld;
	}

	inline void		SetShadowCoordinateScale(const FVector2D& InShadowCoordinateScale)
	{
		ShadowCoordinateScale = InShadowCoordinateScale;
	}
	inline void		SetShadowCoordinateBias(const FVector2D& InShadowCoordinateBias)
	{
		ShadowCoordinateBias = InShadowCoordinateBias;
	}

	/** Called by FTerrainComponentSceneProxy; repacks vertex and index buffers as needed. */
	UBOOL UpdateResources_RenderingThread(INT TessellationLevel, UBOOL bRepackRequired, TArray<FDecalInteraction*>& ProxyDecals);

	// FDeferredCleanupInterface
	virtual void FinishCleanup()
	{
		delete this;
	}

	ATerrain* GetTerrain()
	{
		return TerrainComponent->GetTerrain();
	}

	const ATerrain* GetTerrain() const
	{
		return TerrainComponent->GetTerrain();
	}

	/** Adds a decal interaction to the game object. */
	void AddDecalInteraction_RenderingThread(FDecalInteraction& DecalInteraction, UINT ProxyMaxTesellation);

	// allow access to mesh component
	friend class FDynamicTerrainData;
	friend class FTerrainComponentSceneProxy;
	friend struct FTerrainIndexBuffer;
	template<typename TerrainQuadRelevance> friend struct FTerrainTessellationIndexBuffer;
	friend struct FTerrainDetessellationIndexBuffer;

protected:
	/** Set to TRUE in InitResources() and FALSE in ReleaseResources()	*/
	UBOOL					bIsInitialized;
	/** Debugging flag...												*/
	UBOOL					bIsDeadInGameThread;
	/** Showing collision flag...										*/
	UBOOL					bIsShowingCollision;
	/** Morphing is enabled flag...										*/
	BYTE					MorphingFlags;
	/** The owner component												*/
	UTerrainComponent*		TerrainComponent;

	/** The component section size and base (may not need these...)		*/
#if 1	//@todo. Remove these as we depend on the component anyway!
	INT						ComponentSectionSizeX;
	INT						ComponentSectionSizeY;
	INT						ComponentSectionBaseX;
	INT						ComponentSectionBaseY;
	INT						ComponentTrueSectionSizeX;
	INT						ComponentTrueSectionSizeY;
#endif	//#if 1	//@todo. Remove these as we depend on the component anyway!
	INT						NumVerticesX;
	INT						NumVerticesY;
	/** The maximum tessellation level of the terrain					*/
	INT						MaxTessellationLevel;
	/** The minimum tessellation level of the terrain					*/
	INT						MinTessellationLevel;
	/** The editor-desired tessellation level to display at				*/
	INT						EditorTessellationLevel;
	FLOAT					TerrainHeightScale;
	FLOAT					TessellationDistanceScale;
	INT						LightMapResolution;
	FVector2D				ShadowCoordinateScale;
	FVector2D				ShadowCoordinateBias;
	/** Copy of the ATerrain::NumPatchesX. */
	INT						NumPatchesX;
	/** Copy of the ATerrain::NumPatchesY. */
	INT						NumPatchesY;

	/** The TessellationLevels arrays (per-batch)						*/
	BYTE*								TessellationLevels;

	/** The vertex factory												*/
	FTerrainVertexFactory*				VertexFactory;
	/** The decal vertex factory										*/
	FTerrainDecalVertexFactoryBase*		DecalVertexFactory;
	/** The vertex buffer containing the vertices for the component		*/
	FTerrainVertexBuffer*				VertexBuffer;
	/** The index buffers for each batch material						*/
	TerrainTessellationIndexBufferType*	SmoothIndexBuffer;
	/** The material resources for each batch							*/
	TArray<FMaterialRenderProxy*>			BatchMaterialResources;

	/** For rendering at full-patch (lowest tessellation)				*/
	FTerrainVertexFactory				FullVertexFactory;
	FTerrainDecalVertexFactory			FullDecalVertexFactory;
	FTerrainFullVertexBuffer*			FullVertexBuffer;
	FTerrainIndexBuffer*				FullIndexBuffer;
	FMaterialRenderProxy*					FullMaterialResource;

	// For rendering the collision...
	/** The vertex factory												*/
	FTerrainVertexFactory*				CollisionVertexFactory;
	/** The vertex buffer containing the vertices for the component		*/
	FTerrainVertexBuffer*				CollisionVertexBuffer;
	/** The index buffers for each batch material						*/
	TerrainTessellationIndexBufferType*	CollisionSmoothIndexBuffer;

	void RepackDecalIndexBuffers_RenderingThread(INT TessellationLevel, INT MaxTessellation, TArray<FDecalInteraction*>& Decals);
	void ReinitDecalResources_RenderThread();
};

//
//	FTerrainIndexBuffer
//
struct FTerrainIndexBuffer: FIndexBuffer
{
	const FTerrainObject* TerrainObject;
	INT	SectionSizeX;
	INT	SectionSizeY;
	INT NumVisibleTriangles;

	// Constructor.
	FTerrainIndexBuffer(const FTerrainObject* InTerrainObject) :
		  TerrainObject(InTerrainObject)
		, SectionSizeX(InTerrainObject->GetComponentSectionSizeX())
		, SectionSizeY(InTerrainObject->GetComponentSectionSizeY())
		, NumVisibleTriangles(INDEX_NONE)
	{
	}

	// FRenderResource interface.
	virtual void InitRHI();

	virtual FString GetFriendlyName() const
	{
		return TEXT("Terrain component indices (full batch)");
	}
};

/** An instance of a terrain foliage mesh. */
struct FTerrainFoliageInstance
{
	FVector Location;
    FVector XAxis;
    FVector YAxis;
    FVector ZAxis;
	FVector2D StaticLightingTexCoord;
	FLOAT BoundingRadius;

	/** The octree semantics for foliage instances. */
	struct OctreeSemantics
	{
		enum { MaxElementsPerNode = 33 };
		enum { MaxNodeDepth = 5 };

		static FBox GetBoundingBox(const FTerrainFoliageInstance& FoliageInstance)
		{
			return FBox(
				FoliageInstance.Location - FVector(1,1,1) * FoliageInstance.BoundingRadius,
				FoliageInstance.Location + FVector(1,1,1) * FoliageInstance.BoundingRadius
				);
		}
	};
};

/** Implements the hooks used by the common foliage rendering code to build the render data for a TerrainComponent's foliage. */
class FTerrainFoliageRenderDataPolicy
{
public:

	typedef FTerrainFoliageInstance InstanceType;
	typedef const FTerrainFoliageInstance* InstanceReferenceType;

	/** The time since the foliage was last rendered. */
	FLOAT TimeSinceLastRendered;

	/** Initialization constructor. */
	FTerrainFoliageRenderDataPolicy(const FTerrainFoliageMesh* InMesh,const FBox& InComponentBoundingBox):
		TimeSinceLastRendered(0.0f),
		Mesh(InMesh),
		InstanceOctree(InComponentBoundingBox),
		NumInstances(0),
		StaticMeshRenderData(InMesh->StaticMesh->LODModels(0)),
		MinTransitionRadiusSquared(Square(InMesh->MinTransitionRadius)),
		InvTransitionSize(1.0f / Max(InMesh->MaxDrawRadius - InMesh->MinTransitionRadius,DELTA))
	{
	}

	/**
	* Creates the foliage instances for this mesh+component pair.  Fills in the Instances and Vertices arrays.
	* @param Mesh - The mesh being instances.
	* @param Component - The terrain component which the mesh is being instanced on.
	* @param WeightedMaterial - The terrain weightmap which controls this foliage mesh's density.
	*/
	void InitInstances(const UTerrainComponent* Component,const FTerrainWeightedMaterial& WeightedMaterial);

	/**
	 * Computes the set of instances which are visible in a specific view.
	 * @param View - The view to find visible instances for.
	 * @param OutVisibleFoliageInstances - Upon return, contains the foliage instance indices which are visible in the view.
	 */
	void GetVisibleFoliageInstances(const FSceneView* View,TArray<const FTerrainFoliageInstance*>& OutVisibleFoliageInstances);

	/**
	 * Accesses an instance for a specific view.  The instance is scaled according to the view.
	 * @param InstanceReference - A reference to the instance to access.
	 * @param View - The view to access the instance for.
	 * @param OutInstance - Upon return, contains the instance data.
	 * @param OutWindScale - Upon return, contains the amount to scale the wind skew by.
	 */
	void GetInstance(InstanceReferenceType InstanceReference,const FSceneView* View,InstanceType& OutInstance,FLOAT& OutWindScale) const;

	/**
	 * Sets up the foliage vertex factory for the instance's static lighting data.
	 * @param InstanceBuffer - The vertex buffer containing the instance data.
	 * @param OutData - Upon return, the static lighting components are set for the terrain foliage rendering.
	 */
	void InitVertexFactoryStaticLighting(FVertexBuffer* InstanceBuffer,FFoliageVertexFactory::DataType& OutData);

	/** @return The maximum number of instances which will be rendered. */
	INT GetMaxInstances() const
	{
		return NumInstances;
	}

	/** @return The static mesh render data for a single foliage mesh. */
	const FStaticMeshRenderData& GetMeshRenderData() const
	{
		return StaticMeshRenderData;
	}

private:

	/** The mesh that is being instanced. */
	const FTerrainFoliageMesh* Mesh;

	/** The type of the octree which holds the component's foliage instances. */
	typedef TOctree<FTerrainFoliageInstance,FTerrainFoliageInstance::OctreeSemantics> InstanceOctreeType;

	/** An octree holding the component's foliage instances. */
	InstanceOctreeType InstanceOctree;

	/** The total number of instances. */
	INT NumInstances;

	/** The static mesh render data for a single foliage mesh. */
	const FStaticMeshRenderData& StaticMeshRenderData;

	/** The squared distance that foliage instances begin to scale down at. */
	const FLOAT MinTransitionRadiusSquared;

	/** The inverse difference between the distance where foliage instances begin to scale down, and the distance where they disappear. */
	const FLOAT InvTransitionSize;
};

inline DWORD GetTypeHash(const FTerrainFoliageMesh* Mesh)
{
	return PointerHash(Mesh);
}

//
//	FTerrainComponentSceneProxy
//
class FTerrainComponentSceneProxy : public FPrimitiveSceneProxy, public FTickableObject
{
private:
	class FTerrainComponentInfo : public FLightCacheInterface
	{
	public:

		/** Initialization constructor. */
		FTerrainComponentInfo(const UTerrainComponent& Component)
		{
			// Build the static light interaction map.
			for (INT LightIndex = 0; LightIndex < Component.IrrelevantLights.Num(); LightIndex++)
			{
				StaticLightInteractionMap.Set(Component.IrrelevantLights(LightIndex), FLightInteraction::Irrelevant());
			}
			
			LightMap = Component.LightMap;
			if (LightMap)
			{
				for (INT LightIndex = 0; LightIndex < LightMap->LightGuids.Num(); LightIndex++)
				{
					StaticLightInteractionMap.Set(LightMap->LightGuids(LightIndex), FLightInteraction::LightMap());
				}
			}

			for (INT LightIndex = 0; LightIndex < Component.ShadowMaps.Num(); LightIndex++)
			{
				UShadowMap2D* ShadowMap = Component.ShadowMaps(LightIndex);
				if (ShadowMap && ShadowMap->IsValid())
				{
					StaticLightInteractionMap.Set(
						ShadowMap->GetLightGuid(),
						FLightInteraction::ShadowMap2D(
							ShadowMap->GetTexture(),
							ShadowMap->GetCoordinateScale(),
							ShadowMap->GetCoordinateBias()
							)
						);

					Component.TerrainObject->SetShadowCoordinateBias(ShadowMap->GetCoordinateBias());
					Component.TerrainObject->SetShadowCoordinateScale(ShadowMap->GetCoordinateScale());
				}
			}
		}

		// FLightCacheInterface.
		virtual FLightInteraction GetInteraction(const FLightSceneInfo* LightSceneInfo) const
		{
			// Check for a static light interaction.
			const FLightInteraction* Interaction = StaticLightInteractionMap.Find(LightSceneInfo->LightmapGuid);
			if (!Interaction)
			{
				Interaction = StaticLightInteractionMap.Find(LightSceneInfo->LightGuid);
			}
			return Interaction ? *Interaction : FLightInteraction::Uncached();
		}

		virtual FLightMapInteraction GetLightMapInteraction() const
		{
			return LightMap ? LightMap->GetInteraction() : FLightMapInteraction();
		}

	private:
		/** A map from persistent light IDs to information about the light's interaction with the model element. */
		TMap<FGuid,FLightInteraction> StaticLightInteractionMap;

		/** The light-map used by the element. */
		const FLightMap* LightMap;
	};

	/** */
	struct FTerrainBatchInfo
	{
		FTerrainBatchInfo(UTerrainComponent* Component, INT BatchIndex);
		~FTerrainBatchInfo();

        FMaterialRenderProxy* MaterialRenderProxy;
		UBOOL bIsTerrainMaterialResourceInstance;
		TArray<UTexture2D*> WeightMaps;
	};

	struct FTerrainMaterialInfo
	{
		FTerrainMaterialInfo(UTerrainComponent* Component);
		~FTerrainMaterialInfo();
		
		TArray<FTerrainBatchInfo*> BatchInfoArray;
		FTerrainComponentInfo* ComponentLightInfo;
	};

public:
	/** Initialization constructor. */
	FTerrainComponentSceneProxy(UTerrainComponent* Component);
	~FTerrainComponentSceneProxy();

	virtual void InitLitDecalFlags(UINT InDepthPriorityGroup);

	/**
	 * Adds a decal interaction to the primitive.  This is called in the rendering thread by AddDecalInteraction_GameThread.
	 */
	virtual void AddDecalInteraction_RenderingThread(const FDecalInteraction& DecalInteraction);

	// FPrimitiveSceneProxy interface.
	
	/** 
	* Draw the scene proxy as a dynamic element
	*
	* @param	PDI - draw interface to render to
	* @param	View - current view
	* @param	DPGIndex - current depth priority 
	* @param	Flags - optional set of flags from EDrawDynamicElementFlags
	*/
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags);

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
	 * @param	bEmissivePass					TRUE if the draw call is occurring after the emissive pass, before lighting.
	 */
	virtual void DrawDecalElements(FCommandContextRHI* Context, FPrimitiveDrawInterface* OpaquePDI, FPrimitiveDrawInterface* TranslucentPDI, const FSceneView* View, UINT DepthPriorityGroup, UBOOL bTranslucentReceiverPass);

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
	virtual void DrawLitDecalElements(
		FCommandContextRHI* Context,
		FPrimitiveDrawInterface* PDI,
		const FSceneView* View,
		UINT DepthPriorityGroup,
		UBOOL bDrawingDynamicLights
		);

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		const EShowFlags ShowFlags = View->Family->ShowFlags;
		if (TerrainObject != NULL)
		{
			if(ShowFlags & SHOW_Terrain)
			{
				if (IsShown(View))
				{
					Result.bDynamicRelevance = TRUE;
					Result.SetDPG(GetDepthPriorityGroup(View),TRUE);
					Result.bDecalRelevance = HasRelevantDecals(View);

					if (TerrainObject->bIsShowingCollision == TRUE)
					{
						Result.bTranslucentRelevance = TRUE;
					}

					//@todo.SAS. Kick off the terrain tessellation in a separate here
				}
				if (IsShadowCast(View))
				{
					Result.bShadowRelevance = TRUE;
				}
				if(ShowFlags & SHOW_Foliage)
				{
					FoliageMaterialViewRelevance.SetPrimitiveViewRelevance(Result);

					// Set opaque relevance manually to account for the terrain's material.
					Result.bOpaqueRelevance = TRUE;
				}
			}
		}
		return Result;
	}

	/**
	 *	Determines the relevance of this primitive's elements to the given light.
	 *	@param	LightSceneInfo			The light to determine relevance for
	 *	@param	bDynamic (output)		The light is dynamic for this primitive
	 *	@param	bRelevant (output)		The light is relevant for this primitive
	 *	@param	bLightMapped (output)	The light is light mapped for this primitive
	 */
	virtual void GetLightRelevance(FLightSceneInfo* LightSceneInfo, UBOOL& bDynamic, UBOOL& bRelevant, UBOOL& bLightMapped)
	{
		// Attach the light to the primitive's static meshes.
		bDynamic = TRUE;
		bRelevant = FALSE;
		bLightMapped = TRUE;

		if (CurrentMaterialInfo)
		{
			if (CurrentMaterialInfo->ComponentLightInfo)
			{
				ELightInteractionType InteractionType = CurrentMaterialInfo->ComponentLightInfo->GetInteraction(LightSceneInfo).GetType();
				if(InteractionType != LIT_CachedIrrelevant)
				{
					bRelevant = TRUE;
				}
				if(InteractionType != LIT_CachedLightMap && InteractionType != LIT_CachedIrrelevant)
				{
					bLightMapped = FALSE;
				}
				if(InteractionType != LIT_Uncached)
				{
					bDynamic = FALSE;
				}
			}
		}
		else
		{
			bRelevant = TRUE;
			bLightMapped = FALSE;
		}
	}

	/**
	 *	Called when the rendering thread adds the proxy to the scene.
	 *	This function allows for generating renderer-side resources.
	 */
	virtual UBOOL CreateRenderThreadResources();

	/**
	 *	Called when the rendering thread removes the dynamic data from the scene.
	 */
	virtual UBOOL ReleaseRenderThreadResources();

	// FTickableObject interface.
	virtual void Tick( FLOAT DeltaTime );
	virtual UBOOL IsTickable()
	{
		return TRUE;
	}

	void UpdateData(UTerrainComponent* Component);
	void UpdateData_RenderThread(FTerrainMaterialInfo* NewMaterialInfo);

	virtual EMemoryStats GetMemoryStatType( void ) const { return( STAT_GameToRendererMallocOther ); }
	virtual DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	DWORD GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

	/** @return			Cached value of MaxTessellationLevel, as computed in DrawDynamicElements. */
	UINT GetMaxTessellation() const
	{
		return MaxTessellation;
	}

protected:
	AActor* GetOwner();

private:
	AActor* Owner;
	UTerrainComponent* ComponentOwner;

	FTerrainObject* TerrainObject;

	UBOOL bSelected;

	FColor LevelColor;
	FColor PropertyColor;

	FLOAT CullDistance;

	BITFIELD bCastShadow : 1;

	FColoredMaterialRenderProxy SelectedWireframeMaterialInstance;
	FColoredMaterialRenderProxy DeselectedWireframeMaterialInstance;

	FTerrainMaterialInfo*	CurrentMaterialInfo;

	/** Cache of MaxTessellationLevel, as computed in DrawDynamicElements. */
	UINT MaxTessellation;

	/** Array of meshes, one for each batch material.  Populated by DrawDynamicElements. */
	TArray<FMeshElement> Meshes;

	/** The component's foliage instances. */
	TMap<const FTerrainFoliageMesh*,TFoliageRenderData<FTerrainFoliageRenderDataPolicy>*> FoliageRenderDataSet;

	/** The component's foliage mesh materials. */
	TMap<const FTerrainFoliageMesh*,const UMaterialInterface*> FoliageMeshToMaterialMap;

	/** The foliage materials' view relevance. */
	FMaterialViewRelevance FoliageMaterialViewRelevance;

	/** Draws the component's foliage. */
	void DrawFoliage(FPrimitiveDrawInterface* PDI,const FSceneView* View, UINT DPGIndex);

	/**
	 * Check for foliage that hasn't been rendered recently, and free its data.
	 * @param DeltaTime - The time since the last update.
	 */
	void TickFoliage(FLOAT DeltaTime);

	/** Releases all foliage render data. */
	void ReleaseFoliageRenderData();
};

#endif	//#ifndef TERRAIN_RENDER_HEADER
