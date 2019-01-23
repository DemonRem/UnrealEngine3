/*=============================================================================
	SceneRendering.h: Scene rendering definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/** An association between a hit proxy and a mesh. */
class FHitProxyMeshPair : public FMeshElement
{
public:
	FHitProxyId HitProxyId;

	/** Initialization constructor. */
	FHitProxyMeshPair(const FMeshElement& InMesh,FHitProxyId InHitProxyId):
		FMeshElement(InMesh),
		HitProxyId(InHitProxyId)
	{}
};

/** Information about a visible light. */
class FVisibleLightInfo
{
public:

	FVisibleLightInfo()
		:bInViewFrustum(FALSE)
	{}

	/** true if this light in the view frustum (dir/sky lights always are). */
	UBOOL bInViewFrustum;

	/** Check whether the Light has visible lit primitives in any DPG */
	UBOOL HasVisibleLitPrimitives() const 
	{ 
		for (UINT DPGIndex=0;DPGIndex<SDPG_MAX_SceneRender;++DPGIndex)
		{
			if (DPGInfo[DPGIndex].bHasVisibleLitPrimitives) 
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	/** Information about a visible light in a specific DPG */
	class FDPGInfo
	{
	public:
		FDPGInfo()
			:bHasVisibleLitPrimitives(FALSE)
		{}

		/** The dynamic primitives which are both visible and affected by this light. */
		TArray<FPrimitiveSceneInfo*> VisibleDynamicLitPrimitives;

		/** The primitives which are visible, affected by this light and receiving lit decals. */
		TArray<FPrimitiveSceneInfo*> VisibleLitDecalPrimitives;

		/** Whether the light has any visible lit primitives (static or dynamic) in the DPG */
		UBOOL bHasVisibleLitPrimitives;
	};

	/** Information about the light in each DPG. */
	FDPGInfo DPGInfo[SDPG_MAX_SceneRender];
};

/** 
* Set of sorted translucent scene prims  
*/
class FTranslucentPrimSet
{
public:

	/** 
	* Iterate over the sorted list of prims and draw them
	* @param Context - command context
	* @param View - current view used to draw items
	* @param DPGIndex - current DPG used to draw items
	* @param bPreFog - TRUE if the draw call is occurring before fog has been rendered.
	* @return TRUE if anything was drawn
	*/
	UBOOL Draw(FCommandContextRHI* Context,const class FViewInfo* View,UINT DPGIndex,UBOOL bPreFog);

	/**
	* Add a new primitive to the list of sorted prims
	* @param PrimitiveSceneInfo - primitive info to add. Origin of bounds is used for sort.
	* @param ViewInfo - used to transform bounds to view space
	* @param bUsesSceneColor - primitive samples from scene color
	*/
	void AddScenePrimitive(FPrimitiveSceneInfo* PrimitivieSceneInfo,const FViewInfo& ViewInfo, UBOOL bUsesSceneColor=FALSE);

	/**
	* Sort any primitives that were added to the set back-to-front
	*/
	void SortPrimitives();

	/** 
	* @return number of prims to render
	*/
	INT NumPrims() const
	{
		return SortedPrims.Num() + SortedSceneColorPrims.Num();
	}

private:
	/** contains a scene prim and its sort key */
	struct FSortedPrim
	{
		FSortedPrim(FPrimitiveSceneInfo* InPrimitiveSceneInfo,FLOAT InSortKey,INT InSortPriority)
			:	PrimitiveSceneInfo(InPrimitiveSceneInfo)
			,	SortPriority(InSortPriority)
			,	SortKey(InSortKey)
		{

		}

		FPrimitiveSceneInfo* PrimitiveSceneInfo;
		INT SortPriority;
		FLOAT SortKey;
	};
	/** list of sorted translucent primitives */
	TArray<FSortedPrim> SortedPrims;
	/** list of sorted translucent primitives that use the scene color. These are drawn after all other translucent prims */
	TArray<FSortedPrim> SortedSceneColorPrims;
	/** sortkey compare class */
	IMPLEMENT_COMPARE_CONSTREF( FSortedPrim,TranslucentRender,
	{ 
		if (A.SortPriority == B.SortPriority)
		{
			// sort normally from back to front
			return A.SortKey <= B.SortKey ? 1 : -1;
		}

		// else lower sort priorities should render first
		return A.SortPriority > B.SortPriority ? 1 : -1; 
	} )
};

/** 
 * Set of primitives with decal scene relevance.
 */
class FDecalPrimSet
{
public:
	/** 
	 * Iterates over the sorted list of sorted decal prims and draws them.
	 *
	 * @param	Context							Command context.
	 * @param	View							The Current view used to draw items.
	 * @param	DPGIndex						The current DPG used to draw items.
	 * @param	bTranslucentReceiverPass		TRUE during the decal pass for translucent receivers, FALSE for opaque receivers.
	 * @param	bPreFog							TRUE if the draw call is occurring before fog has been rendered.
	 * @return									TRUE if anything was drawn, FALSE otherwise.
	 */
	UBOOL Draw(FCommandContextRHI* Context,const class FViewInfo* View,UINT DPGIndex,UBOOL bTranslucentReceiverPass, UBOOL bPreFog);

	/**
	* Adds a new primitive to the list of sorted prims
	* @param	PrimitiveSceneInfo		The primitive info to add.
	*/
	void AddScenePrimitive(const FPrimitiveSceneInfo* PrimitivieSceneInfo)
	{
		Prims.AddItem( PrimitivieSceneInfo );
	}

	/**
	 * @param		PrimitiveIndex		The index of the primitive to return.
	 * @return							The primitive at the specified index.
	 */
	const FPrimitiveSceneInfo* GetScenePrimitive(INT PrimitiveIndex) const
	{
		return Prims( PrimitiveIndex );
	}

	/** 
	 * @return		The number of primitives in this set.
	 */
	INT NumPrims() const
	{
		return Prims.Num();
	}

private:
	/** List of primitives with decal relevance. */
	TArray<const FPrimitiveSceneInfo*> Prims;
};

/** MotionBlur parameters */
struct FMotionBlurParameters
{
	FMotionBlurParameters()
		:	VelocityScale( 1.0f )
		,	MaxVelocity( 1.0f )
		,	bFullMotionBlur( TRUE )
		,	RotationThreshold( 45.0f )
		,	TranslationThreshold( 10000.0f )
	{
	}
	FLOAT VelocityScale;
	FLOAT MaxVelocity;
	UBOOL bFullMotionBlur;
	FLOAT RotationThreshold;
	FLOAT TranslationThreshold;
};

/**
 * Combines consecutive primitives which use the same occlusion query into a single DrawIndexedPrimitive call.
 */
class FOcclusionQueryBatcher
{
public:

	/** The maximum number of consecutive previously occluded primitives which will be combined into a single occlusion query. */
	enum { OccludedPrimitiveQueryBatchSize = 8 };

	/** Initialization constructor. */
	FOcclusionQueryBatcher(class FSceneViewState* ViewState,UINT InMaxBatchedPrimitives);

	/** Destructor. */
	~FOcclusionQueryBatcher();

	/** Renders the current batch and resets the batch state. */
	void Flush(FCommandContextRHI* Context);

	/**
	 * Batches a primitive's occlusion query for rendering.
	 * @param Bounds - The primitive's bounds.
	 */
	FOcclusionQueryRHIParamRef BatchPrimitive(const FBoxSphereBounds& Bounds);

private:

	/** A batched primitive. */
	struct FPrimitive
	{
		FVector Origin;
		FVector Extent;
	};

	/** The pending batches. */
	TArray<FOcclusionQueryRHIRef> BatchOcclusionQueries;

	/** The pending primitives. */
	TArray<FPrimitive> Primitives;

	/** The batch new primitives are being added to. */
	FOcclusionQueryRHIParamRef CurrentBatchOcclusionQuery;

	/** The maximum number of primitives in a batch. */
	const UINT MaxBatchedPrimitives;

	/** The number of primitives in the current batch. */
	UINT NumBatchedPrimitives;

	/** The pool to allocate occlusion queries from. */
	class FOcclusionQueryPool* OcclusionQueryPool;
};

/** A FSceneView with additional state used by the scene renderer. */
class FViewInfo : public FSceneView
{
public:

	/** A map from projected shadow ID to a boolean visibility value. */
	FBitArray ShadowVisibilityMap;

	/** A map from primitive ID to a boolean visibility value. */
	FBitArray PrimitiveVisibilityMap;

	/** A map from primitive ID to the primitive's view relevance. */
	TArray<FPrimitiveViewRelevance> PrimitiveViewRelevanceMap;

	/** A map from static mesh ID to a boolean visibility value. */
	FBitArray StaticMeshVisibilityMap;

	/** A map from static mesh ID to a boolean occluder value. */
	FBitArray StaticMeshOccluderMap;

	/** A map from static mesh ID to a boolean visibility value for velocity rendering. */
	FBitArray StaticMeshVelocityMap;

	/** The dynamic primitives visible in this view. */
	TArray<const FPrimitiveSceneInfo*> VisibleDynamicPrimitives;

	/** The primitives visible in this view that have lit decals. */
	TArray<const FPrimitiveSceneInfo*> VisibleLitDecalPrimitives;

	/** Set of decal primitives for this view - one for each DPG */
	FDecalPrimSet DecalPrimSet[SDPG_MAX_SceneRender];

	/** Set of translucent prims for this view - one for each DPG */
	FTranslucentPrimSet TranslucentPrimSet[SDPG_MAX_SceneRender];

	/** Set of distortion prims for this view - one for each DPG */
	FDistortionPrimSet DistortionPrimSet[SDPG_MAX_SceneRender];
	
	/** A map from light ID to a boolean visibility value. */
	TArray<FVisibleLightInfo> VisibleLightInfos;

	/** The view's batched elements, sorted by DPG. */
	FBatchedElements BatchedViewElements[SDPG_MAX_SceneRender];

	/** The view's mesh elements, sorted by DPG. */
	TIndirectArray<FHitProxyMeshPair> ViewMeshElements[SDPG_MAX_SceneRender];

	/** TRUE if the DPG has at least one mesh in ViewMeshElements[DPGIndex] with a translucent material. */
	BITFIELD bHasTranslucentViewMeshElements : SDPG_MAX_SceneRender;

	/** TRUE if the DPG has at least one mesh in ViewMeshElements[DPGIndex] with a distortion material. */
	BITFIELD bHasDistortionViewMeshElements : SDPG_MAX_SceneRender;

	/** The dynamic resources used by the view elements. */
	TArray<FDynamicPrimitiveResource*> DynamicResources;

	/** fog params for 4 layers of height fog */
	FLOAT FogMinHeight[4];
	FLOAT FogMaxHeight[4];
	FLOAT FogDistanceScale[4];
	FLOAT FogExtinctionDistance[4];
	FLinearColor FogInScattering[4];
	FLOAT FogStartDistance[4];

	/** Whether FSceneRenderer needs to output velocities during pre-pass. */
	UBOOL					bRequiresVelocities;

	/** Whether we should use the occlusion queries or not (useful to ignoring occlusions on the first frame after a camera cut. */
	UBOOL					bIgnoreOcclusionQueries;

	FMatrix					PrevViewProjMatrix;
	FMotionBlurParameters	MotionBlurParameters;

    /** Post process render proxies */
    TIndirectArray<FPostProcessSceneProxy> PostProcessSceneProxies;

	/** An intermediate number of visible static meshes.  Doesn't account for occlusion until after FinishOcclusionQueries is called. */
	INT NumVisibleStaticMeshElements;

	/** An intermediate number of visible dynamic primitives.  Doesn't account for occlusion until after FinishOcclusionQueries is called. */
	INT NumVisibleDynamicPrimitives;

	FOcclusionQueryBatcher IndividualOcclusionQueries;
	FOcclusionQueryBatcher GroupedOcclusionQueries;

	/** 
	* Initialization constructor. Passes all parameters to FSceneView constructor
	*/
	FViewInfo(
		const FSceneViewFamily* InFamily,
		FSceneViewStateInterface* InState,
		FSynchronizedActorVisibilityHistory* InHistory,
		const AActor* InViewActor,
		const UPostProcessChain* InPostProcessChain,
		const FPostProcessSettings* InPostProcessSettings,
		FViewElementDrawer* InDrawer,
		FLOAT InX,
		FLOAT InY,
		FLOAT InSizeX,
		FLOAT InSizeY,
		const FMatrix& InViewMatrix,
		const FMatrix& InProjectionMatrix,
		const FLinearColor& InBackgroundColor,
		const FLinearColor& InOverlayColor,
		const FLinearColor& InColorScale,
		const TArray<FPrimitiveSceneInfo*>& InHiddenPrimitives,
		FLOAT InLODDistanceFactor = 1.0f
		);

	/** 
	* Initialization constructor. 
	* @param InView - copy to init with
	*/
	explicit FViewInfo(const FSceneView* InView);

	/** 
	* Destructor. 
	*/
	~FViewInfo();

	/** 
	* Initializes the dynamic resources used by this view's elements. 
	*/
	void InitDynamicResources();
};


/**
 * Used to hold combined stats for a shadow. In the case of shadow volumes the ShadowResolution remains
 * at INDEX_NONE and the Subjects array has a single entry. In the case of projected shadows the shadows
 * for the preshadow and subject are combined in this stat and so are primitives with a shadow parent.
 */
struct FCombinedShadowStats
{
	/** Array of shadow subjects. The first one is the shadow parent in the case of multiple entries.	*/
	TArray<const FPrimitiveSceneInfo*>	SubjectPrimitives;
	/** Array of preshadow primitives in the case of projected shadows.									*/
	TArray<const FPrimitiveSceneInfo*>	PreShadowPrimitives;
	/** Shadow resolution in the case of projected shadows, INDEX_NONE for shadow volumes.				*/
	INT									ShadowResolution;
	/** Shadow pass number in the case of projected shadows, INDEX_NONE for shadow volumes.				*/
	INT									ShadowPassNumber;

	/**
	 * Default constructor, initializing ShadowResolution for shadow volume case. 
	 */
	FCombinedShadowStats()
	:	ShadowResolution(INDEX_NONE)
	,	ShadowPassNumber(INDEX_NONE)
	{}
};

/**
* Global render state 
*/
class FGlobalSceneRenderState
{
public:
	FGlobalSceneRenderState() : FrameNumber(0) {}
	/** Incremented once per frame before the first scene is being rendered */
	UINT FrameNumber;
};

/**
 * Used as the scope for scene rendering functions.
 * It is initialized in the game thread by FSceneViewFamily::BeginRender, and then passed to the rendering thread.
 * The rendering thread calls Render(), and deletes the scene renderer when it returns.
 */
class FSceneRenderer
{
public:

	/** The scene being rendered. */
	FScene* Scene;

	/** The view family being rendered.  This references the Views array. */
	FSceneViewFamily ViewFamily;

	/** The views being rendered. */
	TArray<FViewInfo> Views;

	/** The visible projected shadows. */
	TIndirectArray<FProjectedShadowInfo> ProjectedShadows;
	
	/** The canvas transform used to render the scene. */
	FMatrix CanvasTransform;

	/** The width in screen pixels of the view family being rendered. */
	UINT FamilySizeX;

	/** The height in screen pixels of the view family being rendered. */
	UINT FamilySizeY;

	/** If a freeze request has been made */
	UBOOL bHasRequestedToggleFreeze;

	/** The global RHI context */
	FCommandContextRHI* GlobalContext;

	/** Initialization constructor. */
	FSceneRenderer(const FSceneViewFamily* InViewFamily,FHitProxyConsumer* HitProxyConsumer,const FMatrix& InCanvasTransform);

	/** Destructor, stringifying stats if stats gathering was enabled. */
	~FSceneRenderer();

	/** Renders the view family. */
	void Render();

	/** Renders only the final post processing for the view */
	void RenderPostProcessOnly();

	/** Render the view family's hit proxies. */
	void RenderHitProxies();

	/** Renders the scene to capture target textures */
	void RenderSceneCaptures();

	/** 
	* Global state shared by all FSceneRender instances 
	* @return global state
	*/
	FGlobalSceneRenderState* GetGlobalSceneRenderState();

private:
	/** Visible shadow casting lights in any DPG. */
	TArray<FLightSceneInfo*> VisibleShadowCastingLightInfos;

	/** Map from light primitive interaction to its combined shadow stats. Only used during stats gathering. */
	TMap<FLightPrimitiveInteraction*,FCombinedShadowStats> InteractionToDynamicShadowStatsMap;

	/** Whether we should be gathering dynamic shadow stats this frame. */
	UBOOL bShouldGatherDynamicShadowStats;

	/** The world time of the previous frame. */
	FLOAT PreviousFrameTime;

	/**
	 * Creates a projected shadow for light-primitive interaction.
	 * @param Interaction - The interaction to create a shadow for.
	 * @param OutPreShadows - If the primitive has a preshadow, CreateProjectedShadow adds it to OutPreShadows.
	 */
	void CreateProjectedShadow(const FLightPrimitiveInteraction* Interaction,TArray<FProjectedShadowInfo*>& OutPreShadows);

	/** Finds the visible dynamic shadows for each view. */
	void InitDynamicShadows();

	/** Determines which primitives are visible for each view. */
	void InitViews();

	/** Initialized the fog constants for each view. */
	void InitFogConstants();

	/**
	 * Renders the scene's prepass and occlusion queries.
	 * If motion blur is enabled, it will also render velocities for dynamic primitives to the velocity buffer and
	 * flag those pixels in the stencil buffer.
	 */
	UBOOL RenderPrePass(UINT DPGIndex,UBOOL bIsOcclusionTesting);

	/** 
	* Renders the scene's base pass 
	*
	* @param DPGIndex - current depth priority group index
	* @param bIsTiledRendering - TRUE if currently within a Begin/End tiled rendering block
	* @return TRUE if anything was rendered
	*/
	UBOOL RenderBasePass(UINT DPGIndex, UBOOL bIsTiledRendering);

	/** Renders the scene's fogging. */
	UBOOL RenderFog(UINT DPGIndex);

	/** bound shader state for combining LDR translucency with scene color */
	static FGlobalBoundShaderStateRHIRef LDRCombineBoundShaderState;

	/** Renders the scene's lighting. */
	UBOOL RenderLights(UINT DPGIndex);

	/** Renders the scene's distortion */
	UBOOL RenderDistortion(UINT DPGIndex);
	
	/** 
	 * Renders the scene's translucency.
	 *
	 * @param	DPGIndex	Current DPG used to draw items.
	 * @return				TRUE if anything was drawn.
	 */
	UBOOL RenderTranslucency(UINT DPGIndex);

	/** 
	* Combines the recently rendered LDR translucency with scene color.
	*
	* @param	Context	 The RHI command context the primitives are being rendered to. 
	*/
	void CombineLDRTranslucency(FCommandContextRHI* Context);

	/** 
	 * Renders the scene's decals.
	 *
	 * @param	DPGIndex					Current DPG used to draw items.
	 * @param	bTranslucentReceiverPass	TRUE during the decal pass for translucent receivers, FALSE for opaque receivers.
	 * @param	bPreFog						TRUE if the draw call is occurring before fog has been rendered.
	 * @return								TRUE if anything was drawn.
	 */
	UBOOL RenderDecals(UINT DPGIndex, UBOOL bTranslucentReceiverPass, UBOOL bPreFog);

	/** Renders the velocities of movable objects for the motion blur effect. */
	void RenderVelocities(UINT DPGIndex);

	/** Renders world-space texture density instead of the normal color. */
	UBOOL RenderTextureDensities(UINT DPGIndex);

	/** Begins the occlusion tests for a view. */
	void BeginOcclusionTests(FViewInfo& View);

	/** bound shader state for occlusion test prims */
	static FBoundShaderStateRHIRef OcclusionTestBoundShaderState;

	/** Renders the post process effects for a view. */
	void RenderPostProcessEffects(UINT DPGIndex);

	/**
	* Finish rendering a view, mapping accumulated shader complexity to a color.
	* @param View - The view to process.
	*/
	void RenderShaderComplexity(const FViewInfo* View);
	/** bound shader state for full-screen shader complexity apply pass */
	static FGlobalBoundShaderStateRHIRef ShaderComplexityBoundShaderState;

	/**
	 * Finish rendering a view, writing the contents to ViewFamily.RenderTarget.
	 * @param View - The view to process.
	*/
	void FinishRenderViewTarget(const FViewInfo* View);
	/** bound shader state for full-screen gamma correction pass */
	static FGlobalBoundShaderStateRHIRef PostProcessBoundShaderState;

	/**
	  * Used by RenderLights to figure out if projected shadows need to be rendered to the attenuation buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @return TRUE if anything needs to be rendered
	  */
	UBOOL CheckForProjectedShadows( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex );

	/**
	  * Used by RenderLights to render projected shadows to the attenuation buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @return TRUE if anything got rendered
	  */
	UBOOL RenderProjectedShadows( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex );

	/**
	  * Used by RenderLights to figure out if shadow volumes need to be rendered to the stencil buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @return TRUE if anything needs to be rendered
	  */
	UBOOL CheckForShadowVolumes( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex );

	/**
	  * Used by RenderLights to render shadow volumes to the attenuation buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @param LightIndex The light's index into FScene::Lights
	  * @return TRUE if anything got rendered
	  */
	UBOOL RenderShadowVolumes( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex );

	/**
	* Used by RenderLights to figure out if shadow volumes need to be rendered to the attenuation buffer.
	*
	* @param LightSceneInfo Represents the current light
	* @return TRUE if anything needs to be rendered
	*/
	UBOOL CheckForShadowVolumeAttenuation( const FLightSceneInfo* LightSceneInfo );

	/**
	* Attenuate the shadowed area of a shadow volume. For use with modulated shadows
	* @param LightSceneInfo - Represents the current light
	* @return TRUE if anything got rendered
	*/
	UBOOL AttenuateShadowVolumes( const FLightSceneInfo* LightSceneInfo );

	/**
	  * Used by RenderLights to figure out if light functions need to be rendered to the attenuation buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @return TRUE if anything got rendered
	  */
	UBOOL CheckForLightFunction( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex );

	/**
	  * Used by RenderLights to render a light function to the attenuation buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @param LightIndex The light's index into FScene::Lights
	  * @return TRUE if anything got rendered
	  */
	UBOOL RenderLightFunction( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex );

	/**
	  * Used by RenderLights to render a light to the scene color buffer.
	  *
	  * @param LightSceneInfo Represents the current light
	  * @param LightIndex The light's index into FScene::Lights
	  * @return TRUE if anything got rendered
	  */
	UBOOL RenderLight( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex );

	/**
	 * Renders all the modulated shadows to the scene color buffer.
	 * @param	DPGIndex					Current DPG used to draw items.
	 * @return TRUE if anything got rendered
	 */
	UBOOL RenderModulatedShadows(UINT DPGIndex);

	/**
	* Clears the scene color depth (stored in alpha channel) to max depth
	* This is needed for depth bias blend materials to show up correctly
	*/
	void ClearSceneColorDepth();

	/** Saves the actor and primitive visibility states for the game thread. */
	void SaveVisibilityState();
};

/**
 * An implementation of the dynamic primitive definition interface to draw the elements passed to it on a given RHI command interface.
 */
template<typename DrawingPolicyFactoryType>
class TDynamicPrimitiveDrawer : public FPrimitiveDrawInterface
{
public:

	/**
	* Init constructor
	*
	* @param InContext - RHI context to process render commands
	* @param InView - view region being rendered
	* @param InDPGIndex - current depth priority group of the scene
	* @param InDrawingContext - contex for the given draw policy type
	* @param InPreFog - rendering is occuring before fog
	* @param bInIsHitTesting - rendering is occuring during hit testing
	*/
	TDynamicPrimitiveDrawer(
		FCommandContextRHI* InContext,
		const FViewInfo* InView,
		UINT InDPGIndex,
		const typename DrawingPolicyFactoryType::ContextType& InDrawingContext,
		UBOOL InPreFog,
		UBOOL bInIsHitTesting = FALSE
		):
		FPrimitiveDrawInterface(InView),
		View(InView),
		Context(InContext),
		DPGIndex(InDPGIndex),
		DrawingContext(InDrawingContext),
		bPreFog(InPreFog),
		bDirty(FALSE),
		bIsHitTesting(bInIsHitTesting)
	{}

	~TDynamicPrimitiveDrawer()
	{
		if(View)
		{
			// Draw the batched elements.
			BatchedElements.Draw(
				Context,
				View->ViewProjectionMatrix,
				appTrunc(View->SizeX),
				appTrunc(View->SizeY),
				(View->Family->ShowFlags & SHOW_HitProxies) != 0
				);
		}

		// Cleanup the dynamic resources.
		for(INT ResourceIndex = 0;ResourceIndex < DynamicResources.Num();ResourceIndex++)
		{
			//release the resources before deleting, they will delete themselves
			DynamicResources(ResourceIndex)->ReleasePrimitiveResource();
		}
	}

	void SetPrimitive(const FPrimitiveSceneInfo* NewPrimitiveSceneInfo)
	{
		PrimitiveSceneInfo = NewPrimitiveSceneInfo;
		HitProxyId = PrimitiveSceneInfo->DefaultDynamicHitProxyId;
	}

	// FPrimitiveDrawInterface interface.

	virtual UBOOL IsHitTesting()
	{
		return bIsHitTesting;
	}

	virtual void SetHitProxy(HHitProxy* HitProxy)
	{
		if(HitProxy)
		{
			// Only allow hit proxies from CreateHitProxies.
			checkMsg(PrimitiveSceneInfo->HitProxies.FindItemIndex(HitProxy) != INDEX_NONE,"Hit proxy used in DrawDynamicElements which wasn't created in CreateHitProxies");
			HitProxyId = HitProxy->Id;
		}
		else
		{
			HitProxyId = FHitProxyId();
		}
	}

	virtual void RegisterDynamicResource(FDynamicPrimitiveResource* DynamicResource)
	{
		// Add the dynamic resource to the list of resources to cleanup on destruction.
		DynamicResources.AddItem(DynamicResource);

		// Initialize the dynamic resource immediately.
		DynamicResource->InitPrimitiveResource();
	}

	virtual UBOOL IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy) const
	{
		return DrawingPolicyFactoryType::IsMaterialIgnored(MaterialRenderProxy);
	}

	virtual void DrawMesh(const FMeshElement& Mesh)
	{
		if( Mesh.DepthPriorityGroup == DPGIndex )
		{
			const FMaterial* Material = Mesh.MaterialRenderProxy->GetMaterial();
			const UBOOL bIsTwoSided = Material->IsTwoSided();
			const EMaterialLightingModel LightingModel = Material->GetLightingModel();
			const UBOOL bNeedsBackfacePass = (LightingModel != MLM_NonDirectional) && (LightingModel != MLM_Unlit);
			const UINT NumBackfacePasses = (bIsTwoSided && bNeedsBackfacePass) ? 2 : 1;
			for(UINT bBackFace = 0;bBackFace < NumBackfacePasses;bBackFace++)
			{
				bDirty |= DrawingPolicyFactoryType::DrawDynamicMesh(
					Context,
					View,
					DrawingContext,
					Mesh,
					bBackFace,
					bPreFog,
					PrimitiveSceneInfo,
					HitProxyId
					);
			}
		}
	}

	virtual void DrawSprite(
		const FVector& Position,
		FLOAT SizeX,
		FLOAT SizeY,
		const FTexture* Sprite,
		const FLinearColor& Color,
		BYTE DepthPriorityGroup
		)
	{
		if(DepthPriorityGroup == DPGIndex && DrawingPolicyFactoryType::bAllowSimpleElements)
		{
			BatchedElements.AddSprite(Position,SizeX,SizeY,Sprite,Color,HitProxyId);
			bDirty = TRUE;
		}
	}

	virtual void DrawLine(
		const FVector& Start,
		const FVector& End,
		const FLinearColor& Color,
		BYTE DepthPriorityGroup
		)
	{
		if(DepthPriorityGroup == DPGIndex && DrawingPolicyFactoryType::bAllowSimpleElements)
		{
			BatchedElements.AddLine(Start,End,Color,HitProxyId);
			bDirty = TRUE;
		}
	}

	virtual void DrawPoint(
		const FVector& Position,
		const FLinearColor& Color,
		FLOAT PointSize,
		BYTE DepthPriorityGroup
		)
	{
		if(DepthPriorityGroup == DPGIndex && DrawingPolicyFactoryType::bAllowSimpleElements)
		{
			BatchedElements.AddPoint(Position,PointSize,Color,HitProxyId);
			bDirty = TRUE;
		}
	}

	// Accessors.
	/**
	 * @return		TRUE if fog has not yet been rendered.
	 */
	UBOOL IsPreFog() const
	{
		return bPreFog;
	}

	/**
	 * @return		TRUE if any elements have been rendered by this drawer.
	 */
	UBOOL IsDirty() const
	{
		return bDirty;
	}

private:
	/** The view which is being rendered. */
	const FViewInfo* const View;

	/** The RHI command context the primitives are being rendered to. */
	FCommandContextRHI* Context;

	/** The DPG which is being rendered. */
	const UINT DPGIndex;

	/** The drawing context passed to the drawing policy for the mesh elements rendered by this drawer. */
	typename DrawingPolicyFactoryType::ContextType DrawingContext;

	/** The primitive being rendered. */
	const FPrimitiveSceneInfo* PrimitiveSceneInfo;

	/** The current hit proxy ID being rendered. */
	FHitProxyId HitProxyId;

	/** The batched simple elements. */
	FBatchedElements BatchedElements;

	/** The dynamic resources which have been registered with this drawer. */
	TArray<FDynamicPrimitiveResource*> DynamicResources;

	/** TRUE if fog has not yet been rendered. */
	BITFIELD bPreFog : 1;

	/** Tracks whether any elements have been rendered by this drawer. */
	BITFIELD bDirty : 1;

	/** TRUE if hit proxies are being drawn. */
	BITFIELD bIsHitTesting : 1;
};

/**
 * Draws a view's elements with the specified drawing policy factory type.
 * @param Context - The RHI command interface to execute the draw commands on.
 * @param View - The view to draw the meshes for.
 * @param DrawingContext - The drawing policy type specific context for the drawing.
 * @param DPGIndex - The depth priority group to draw the elements from.
 * @param bPreFog - TRUE if the draw call is occurring before fog has been rendered.
 */
template<class DrawingPolicyFactoryType>
UBOOL DrawViewElements(
	FCommandContextRHI* Context,
	const FViewInfo* View,
	const typename DrawingPolicyFactoryType::ContextType& DrawingContext,
	UINT DPGIndex,
	UBOOL bPreFog
	)
{
	// Draw the view's mesh elements.
	for(INT MeshIndex = 0;MeshIndex < View->ViewMeshElements[DPGIndex].Num();MeshIndex++)
	{
		const FHitProxyMeshPair& Mesh = View->ViewMeshElements[DPGIndex](MeshIndex);
		const UBOOL bIsTwoSided = Mesh.MaterialRenderProxy->GetMaterial()->IsTwoSided();
		const UBOOL bIsNonDirectionalLighting = Mesh.MaterialRenderProxy->GetMaterial()->GetLightingModel() == MLM_NonDirectional;
		const UINT NumBackfacePasses = (bIsTwoSided && !bIsNonDirectionalLighting) ? 2 : 1;
		for(UINT bBackFace = 0;bBackFace < NumBackfacePasses;bBackFace++)
		{
			DrawingPolicyFactoryType::DrawDynamicMesh(
				Context,
				View,
				DrawingContext,
				Mesh,
				bBackFace,
				bPreFog,
				NULL,
				Mesh.HitProxyId
				);
		}
	}

	return View->ViewMeshElements[DPGIndex].Num() != 0;
}

/**
 * Draws a given set of dynamic primitives to a RHI command interface, using the specified drawing policy type.
 * @param Context - The RHI command interface to execute the draw commands on.
 * @param View - The view to draw the meshes for.
 * @param DrawingContext - The drawing policy type specific context for the drawing.
 * @param DPGIndex - The depth priority group to draw the elements from.
 * @param bPreFog - TRUE if the draw call is occurring before fog has been rendered.
 */
template<class DrawingPolicyFactoryType>
UBOOL DrawDynamicPrimitiveSet(
	FCommandContextRHI* Context,
	const FViewInfo* View,
	const typename DrawingPolicyFactoryType::ContextType& DrawingContext,
	UINT DPGIndex,
	UBOOL bPreFog,
	UBOOL bIsHitTesting = FALSE
	)
{
	// Draw the view's elements.
	UBOOL bDrewViewElements = DrawViewElements<DrawingPolicyFactoryType>(Context,View,DrawingContext,DPGIndex,bPreFog);

	// Draw the elements of each dynamic primitive.
	TDynamicPrimitiveDrawer<DrawingPolicyFactoryType> Drawer(Context,View,DPGIndex,DrawingContext,bPreFog,bIsHitTesting);
	for(INT PrimitiveIndex = 0;PrimitiveIndex < View->VisibleDynamicPrimitives.Num();PrimitiveIndex++)
	{
		const FPrimitiveSceneInfo* PrimitiveSceneInfo = View->VisibleDynamicPrimitives(PrimitiveIndex);

		if(!View->PrimitiveVisibilityMap(PrimitiveSceneInfo->Id))
		{
			continue;
		}

		if(!View->PrimitiveViewRelevanceMap(PrimitiveSceneInfo->Id).GetDPG(DPGIndex))
		{
			continue;
		}

		Drawer.SetPrimitive(PrimitiveSceneInfo);
		PrimitiveSceneInfo->Proxy->DrawDynamicElements(
			&Drawer,
			View,
			DPGIndex
			);
	}

	return bDrewViewElements || Drawer.IsDirty();
}

/** A primitive draw interface which adds the drawn elements to the view's batched elements. */
class FViewElementPDI : public FPrimitiveDrawInterface
{
public:

	FViewElementPDI(FViewInfo* InViewInfo,FHitProxyConsumer* InHitProxyConsumer):
		FPrimitiveDrawInterface(InViewInfo),
		ViewInfo(InViewInfo),
		HitProxyConsumer(InHitProxyConsumer)
	{}

	virtual UBOOL IsHitTesting()
	{
		return HitProxyConsumer != NULL;
	}
	virtual void SetHitProxy(HHitProxy* HitProxy)
	{
		// Change the current hit proxy.
		CurrentHitProxy = HitProxy;

		if(HitProxyConsumer && HitProxy)
		{
			// Notify the hit proxy consumer of the new hit proxy.
			HitProxyConsumer->AddHitProxy(HitProxy);
		}
	}

	virtual void RegisterDynamicResource(FDynamicPrimitiveResource* DynamicResource)
	{
		ViewInfo->DynamicResources.AddItem(DynamicResource);
	}

	virtual void DrawSprite(
		const FVector& Position,
		FLOAT SizeX,
		FLOAT SizeY,
		const FTexture* Sprite,
		const FLinearColor& Color,
		BYTE DepthPriorityGroup
		)
	{
		ViewInfo->BatchedViewElements[DepthPriorityGroup].AddSprite(
			Position,
			SizeX,
			SizeY,
			Sprite,
			Color,
			CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId()
			);
	}

	virtual void DrawLine(
		const FVector& Start,
		const FVector& End,
		const FLinearColor& Color,
		BYTE DepthPriorityGroup
		)
	{
		ViewInfo->BatchedViewElements[DepthPriorityGroup].AddLine(Start,End,Color,CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId());
	}

	virtual void DrawPoint(
		const FVector& Position,
		const FLinearColor& Color,
		FLOAT PointSize,
		BYTE DepthPriorityGroup
		)
	{
		ViewInfo->BatchedViewElements[DepthPriorityGroup].AddPoint(
			Position,
			PointSize,
			Color,
			CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId()
			);
	}

	virtual void DrawMesh(const FMeshElement& Mesh)
	{
		const UINT DepthPriorityGroup = Mesh.DepthPriorityGroup < SDPG_MAX_SceneRender ? Mesh.DepthPriorityGroup : SDPG_World;

		// Keep track of view mesh elements whether that have transluceny or distortion.
		ViewInfo->bHasTranslucentViewMeshElements |= (Mesh.IsTranslucent() << DepthPriorityGroup);
		ViewInfo->bHasDistortionViewMeshElements |= (Mesh.IsDistortion() << DepthPriorityGroup);

		new(ViewInfo->ViewMeshElements[DepthPriorityGroup]) FHitProxyMeshPair(
			Mesh,
			CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId()
			);
	}

private:
	FViewInfo* ViewInfo;
	TRefCountPtr<HHitProxy> CurrentHitProxy;
	FHitProxyConsumer* HitProxyConsumer;
};
