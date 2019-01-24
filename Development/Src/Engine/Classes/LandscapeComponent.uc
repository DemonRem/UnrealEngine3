/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
class LandscapeComponent extends PrimitiveComponent
	native(Terrain);

var const int						SectionBaseX,
									SectionBaseY,

									ComponentSizeQuads,		// Total number of quads for this component
									SubsectionSizeQuads,	// Number of quads for a subsection of the component. SubsectionSizeQuads+1 must be a power of two.
									NumSubsections;			// Number of subsections in X or Y axis

var MaterialInstanceConstant		MaterialInstance;

/** Stores information about which weightmap texture and channel each layer is stored */
struct native WeightmapLayerAllocationInfo
{
	var Name LayerName;
	var byte WeightmapTextureIndex;
	var byte WeightmapTextureChannel;

	structcpptext
	{
		FWeightmapLayerAllocationInfo(FName InLayerName)
		:	LayerName(InLayerName)
		,	WeightmapTextureIndex(255)	// Indicates an invalid allocation
		,	WeightmapTextureChannel(255)
		{}
	}
};

/** List of layers, and the weightmap and channel they are stored */
var private const array<WeightmapLayerAllocationInfo> WeightmapLayerAllocations;

/** Weightmap texture reference */
var private const array<Texture2D> WeightmapTextures;

/** UV offset to component's weightmap data from component local coordinates*/
var Vector4 WeightmapScaleBias;

/** U or V offset into the weightmap for the first subsection, in texture UV space */
var float WeightmapSubsectionOffset;

/** UV offset to Heightmap data from component local coordinates */
var Vector4 HeightmapScaleBias;

/** U or V offset into the heightmap for the first subsection, in texture UV space */
var float HeightmapSubsectionOffset;

/** UV offset for layer texturing to match adjacent components */
var Vector2D LayerUVPan;

/** Heightmap texture reference */
var private const Texture2D HeightmapTexture;

/** Cached bounds, created at heightmap update time */
var const BoxSphereBounds CachedBoxSphereBounds;

/**
 *	The resolution to cache lighting at, in texels/patch.
 *	A separate shadow-map is used for each terrain component, which is
 *	(SectionSizeQuads * StaticLightingResolution + 1) pixels on a side.
 */
var int StaticLightingResolution;

/** Unique ID for this component, used for caching during distributed lighting */
var private const editoronly Guid LightingGuid;

/** Array of shadow maps for this component. */
var private const array<ShadowMap2D> ShadowMaps;
/**	INTERNAL: Array of lights that don't apply to the terrain component.		*/
var const array<Guid>		IrrelevantLights;

/** Reference to the texture lightmap resource. */
var native private const LightMapRef LightMap;

/** Pointer to data shared with the render therad, used by the editor tools */
var private native pointer EditToolRenderData{struct FLandscapeEditToolRenderData};

/** Heightfield mipmap used to generate collision */
var const int CollisionMipLevel;

enum ETerrainComponentNeighbors
{
	TCN_NW,
	TCN_N,
	TCN_NE,
	TCN_W,
	TCN_E,
	TCN_SW,
	TCN_S,
	TCN_SE,
};

/* Eight neigboring TerrainComponents, or NULL if none */
var editoronly const LandscapeComponent Neighbors[8];

cpptext
{
	// UObject interface
	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray );
	virtual void Serialize(FArchive& Ar);
	virtual void BeginDestroy();
#if WITH_EDITOR
	virtual void PostLoad();
	virtual void PostEditImport();

	// Register ourselves with the actor.
	void SetupActor();
#endif

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const;
	virtual void UpdateBounds();
	void SetParentToWorld(const FMatrix& ParentToWorld);

#if WITH_EDITOR
	virtual void GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options);
	virtual UBOOL GetLightMapResolution( INT& Width, INT& Height ) const;
	virtual void GetStaticTriangles(FPrimitiveTriangleDefinitionInterface* PTDI) const;
#endif
	virtual void InvalidateLightingCache();
	/**
	 *	Requests whether the component will use texture, vertex or no lightmaps.
	 *
	 *	@return	ELightMapInteractionType		The type of lightmap interaction the component will use.
	 */
	virtual ELightMapInteractionType GetStaticLightingType() const	{ return LMIT_Texture;	}
	virtual void GetStreamingTextureInfo(TArray<FStreamingTexturePrimitiveInfo>& OutStreamingTextures) const;

	// ULandscapeComponent Interface

	/** Return's the landscape actor associated with this component. */
	class ALandscape* GetLandscapeActor() const
	{
		return CastChecked<ALandscape>(GetOuter());
	}

#if WITH_EDITOR
	/** Initialize the landscape component */
	void Init(INT InBaseX,INT InBaseY,INT InComponentSizeQuads, INT InNumSubsections,INT InSubsectionSizeQuads);

	/**
	 * Recalculate cached bounds using height values.
	 */
	void UpdateCachedBounds();

	/**
	 * Update the MaterialInstance parameters to match the layer and weightmaps for this component
	 * Creates the MaterialInstance if it doesn't exist.
	 */
	void UpdateMaterialInstances();

	/**
	 * Generate mipmaps for height and tangent data.
	 * @param HeightmapTextureMipData - array of pointers to the locked mip data. 
	 *           This should only include the mips that are generated directly from this component's data
	 *           ie where each subsection has at least 2 vertices.
	 */
	void GenerateHeightmapMips(TArray<FColor*>& HeightmapTextureMipData, INT ComponentX1=0, INT ComponentY1=0, INT ComponentX2=MAXINT, INT ComponentY2=MAXINT,struct FLandscapeTextureDataInfo* TextureDataInfo=NULL);

	/**
	 * Generates empty mipmaps for weightmap
	 */
	static void ULandscapeComponent::CreateEmptyWeightmapMips(UTexture2D* WeightmapTexture);

	/**
	 * Generate mipmaps for weightmap
	 * Assumes all weightmaps are unique to this component.
	 * @param WeightmapTextureBaseMipData: array of pointers to each of the weightmaps' base locked mip data.
	 */
	static void GenerateWeightmapMips(INT InNumSubsections, INT InSubsectionSizeQuads, UTexture2D* WeightmapTexture, FColor* BaseMipData);

	/**
	 * Update mipmaps for existing weightmap texture
	 */
	static void UpdateWeightmapMips(INT InNumSubsections, INT InSubsectionSizeQuads, UTexture2D* WeightmapTexture, TArray<FColor*>& WeightmapTextureMipData, INT ComponentX1=0, INT ComponentY1=0, INT ComponentX2=MAXINT, INT ComponentY2=MAXINT, struct FLandscapeTextureDataInfo* TextureDataInfo=NULL);

	/**
	 * Creates or updates collision component
	 * @param HeightmapTextureMipData: heightmap data
	 * @param ComponentX1, ComponentY1, ComponentX2, ComponentY2: region to update
	 * @param Whether to update bounds from render component.
	 */
	void UpdateCollisionComponent(FColor* HeightmapTextureMipData, INT ComponentX1=0, INT ComponentY1=0, INT ComponentX2=MAXINT, INT ComponentY2=MAXINT, UBOOL bUpdateBounds=FALSE);

	/**
	 * Creates weightmaps for this component for the layers specified in the WeightmapLayerAllocations array
	 */
	void ReallocateWeightmaps(struct FLandscapeEditDataInterface* DataInterface=NULL);

	/**
	 * Generate a key for this component's layer allocations to use with MaterialInstanceConstantMap.
	 */
	FString GetLayerAllocationKey() const;
	FString GetLayerDebugColorKey(INT& R, INT& G, INT& B) const;

#endif

	friend class FLandscapeComponentSceneProxy;
	friend struct FLandscapeComponentDataInterface;
}

defaultproperties
{
	LightingChannels=(Static=TRUE,bInitialized=TRUE)
	CollideActors=TRUE
	BlockActors=TRUE
	BlockZeroExtent=TRUE
	BlockNonZeroExtent=TRUE
	BlockRigidBody=TRUE
	CastShadow=TRUE
	bAcceptsLights=TRUE
	bAcceptsDecals=TRUE
	bAcceptsStaticDecals=TRUE
	bUsePrecomputedShadows=TRUE
	bForceDirectLightMap=TRUE
	bUseAsOccluder=TRUE
	bAllowCullDistanceVolume=FALSE
	CollisionMipLevel=0
}
