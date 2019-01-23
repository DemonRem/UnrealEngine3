/*=============================================================================
	PrimitiveComponent.h: Primitive component definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Forward declarations.
class FDecalInteraction;
class FDecalRenderData;
class FPrimitiveSceneInfo;
class UDecalComponent;

/**
 * Encapsulates the data which is mirrored to render a primitive parallel to the game thread.
 */
class FPrimitiveSceneProxy
{
public:

	/** optional flags used for DrawDynamicElements */
	enum EDrawDynamicElementFlags
	{		
		/** mesh elements with dynamic data can be drawn */
		DontAllowDynamicMeshElementData = 1<<0,
		/** mesh elements without dynamic data can be drawn */
		DontAllowStaticMeshElementData = 1<<1
	};

	/** Initialization constructor. */
	FPrimitiveSceneProxy(const UPrimitiveComponent* InComponent);

	/** Virtual destructor. */
	virtual ~FPrimitiveSceneProxy();

	/**
	 * Creates the hit proxies are used when DrawDynamicElements is called.
	 * Called in the game thread.
	 * @param OutHitProxies - Hit proxes which are created should be added to this array.
	 * @return The hit proxy to use by default for elements drawn by DrawDynamicElements.
	 */
	virtual HHitProxy* CreateHitProxies(const UPrimitiveComponent* Component,TArray<TRefCountPtr<HHitProxy> >& OutHitProxies);

	/**
	 * Draws the primitive's static elements.  This is called from the game thread once when the scene proxy is created.
	 * The static elements will only be rendered if GetViewRelevance declares static relevance.
	 * Called in the game thread.
	 * @param PDI - The interface which receives the primitive elements.
	 */
	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) {}

	/**
	 * Draws the primitive's dynamic elements.  This is called from the rendering thread for each frame of each view.
	 * The dynamic elements will only be rendered if GetViewRelevance declares dynamic relevance.
	 * Called in the rendering thread.
	 * @param PDI - The interface which receives the primitive elements.
	 * @param View - The view which is being rendered.
	 * @param InDepthPriorityGroup - The DPG which is being rendered.
	 * @param Flags - optional set of flags from EDrawDynamicElementFlags
	 */
	virtual void DrawDynamicElements(
		FPrimitiveDrawInterface* PDI,
		const FSceneView* View,
		UINT InDepthPriorityGroup,
		DWORD Flags=0
		) {}

	virtual void InitLitDecalFlags(UINT InDepthPriorityGroup) {}

	/**
	 * Adds a decal interaction to the primitive.  This is called in the rendering thread by AddDecalInteraction_GameThread.
	 */
	virtual void AddDecalInteraction_RenderingThread(const FDecalInteraction& DecalInteraction);

	/**
	 * Adds a decal interaction to the primitive.  This simply sends a message to the rendering thread to call AddDecalInteraction_RenderingThread.
	 * This is called in the game thread as new decal interactions are created.
	 */
	void AddDecalInteraction_GameThread(const FDecalInteraction& DecalInteraction);

	/**
	 * Removes a decal interaction from the primitive.  This is called in the rendering thread by RemoveDecalInteraction_GameThread.
	 */
	virtual void RemoveDecalInteraction_RenderingThread(UDecalComponent* DecalComponent);

	/**
	* Removes a decal interaction from the primitive.  This simply sends a message to the rendering thread to call RemoveDecalInteraction_RenderingThread.
	* This is called in the game thread when a decal is detached from a primitive which has been added to the scene.
	*/
	void RemoveDecalInteraction_GameThread(UDecalComponent* DecalComponent);

	/**
	 * Draws the primitive's lit decal elements.  This is called from the rendering thread for each frame of each view.
	 * The dynamic elements will only be rendered if GetViewRelevance declares dynamic relevance.
	 * Called in the rendering thread.
	 *
	 * @param	Context					The RHI command context to which the primitives are being rendered.
	 * @param	PDI						The interface which receives the primitive elements.
	 * @param	View					The view which is being rendered.
	 * @param	InDepthPriorityGroup		The DPG which is being rendered.
	 * @param	bDrawingDynamicLights	TRUE if drawing dynamic lights, FALSE if drawing static lights.
	 */
	virtual void DrawLitDecalElements(
		FCommandContextRHI* Context,
		FPrimitiveDrawInterface* PDI,
		const FSceneView* View,
		UINT InDepthPriorityGroup,
		UBOOL bDrawingDynamicLights
		) {}

	/**
	 * Draws the primitive's decal elements.  This is called from the rendering thread for each frame of each view.
	 * The dynamic elements will only be rendered if GetViewRelevance declares decal relevance.
	 * Called in the rendering thread.
	 *
	 * @param	Context							The RHI command context to which the primitives are being rendered.
	 * @param	OpaquePDI						The interface which receives the opaque primitive elements.
	 * @param	TranslucentPDI					The interface which receives the translucent primitive elements.
	 * @param	View							The view which is being rendered.
	 * @param	InDepthPriorityGroup				The DPG which is being rendered.
	 * @param	bTranslucentReceiverPass		TRUE during the decal pass for translucent receivers, FALSE for opaque receivers.
	 */
	virtual void DrawDecalElements(
		FCommandContextRHI* Context,
		FPrimitiveDrawInterface* OpaquePDI,
		FPrimitiveDrawInterface* TranslucentPDI,
		const FSceneView* View,
		UINT InDepthPriorityGroup,
		UBOOL bTranslucentReceiverPass
		) {}

	/**
	 * Draws the primitive's shadow volumes.  This is called from the rendering thread,
	 * in the FSceneRenderer::RenderLights phase.
	 * @param SVDI - The interface which performs the actual rendering of a shadow volume.
	 * @param View - The view which is being rendered.
	 * @param Light - The light for which shadows should be drawn.
	 * @param DPGIndex - The depth priority group the light is being drawn for.
	 */
	virtual void DrawShadowVolumes(FShadowVolumeDrawInterface* SVDI,const FSceneView* View,const FLightSceneInfo* Light,UINT DPGIndex) {}

	/**
	 * Removes potentially cached shadow volume data for the passed in light.
	 *
	 * @param Light		The light for which cached shadow volume data will be removed.
	 */
	virtual void RemoveCachedShadowVolumeData( const FLightSceneInfo* Light ) {}

	/**
	 * Determines the relevance of this primitive's elements to the given view.
	 * Called in the rendering thread.
	 * @param View - The view to determine relevance for.
	 * @return The relevance of the primitive's elements to the view.
	 */
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View);

	/**
	* Called during FSceneRenderer::InitViews for view processing on scene proxies before rendering them
	* Only called for primitives that are visible and have bDynamicRelevance
	* @param View - The view the proxy will process.
	* @param FrameNumber - The frame number of this frame, incremented once per rendering loop.
	*/
	virtual void PreRenderView(const FSceneView* View, INT FrameNumber) {}

	/**
	 *	Determines the relevance of this primitive's elements to the given light.
	 *	@param	LightSceneInfo			The light to determine relevance for
	 *	@param	bDynamic (output)		The light is dynamic for this primitive
	 *	@param	bRelevant (output)		The light is relevant for this primitive
	 *	@param	bLightMapped (output)	The light is light mapped for this primitive
	 */
	virtual void GetLightRelevance(FLightSceneInfo* LightSceneInfo, UBOOL& bDynamic, UBOOL& bRelevant, UBOOL& bLightMapped)
	{
		// Determine the lights relevance to the primitive.
		bDynamic = TRUE;
		bRelevant = TRUE;
		bLightMapped = FALSE;
	}

	/**
	 * @return		TRUE if the primitive has decals with lit materials that should be rendered in the given view.
	 */
	UBOOL HasLitDecals(const FSceneView* View) const;

	/**
	 *	Called when the rendering thread adds the proxy to the scene.
	 *	This function allows for generating renderer-side resources.
	 *	Called in the rendering thread.
	 */
	virtual UBOOL CreateRenderThreadResources()
	{
		return TRUE;
	}

	/**
	 * Called by the rendering thread to notify the proxy when a light is no longer
	 * associated with the proxy, so that it can clean up any cached resources.
	 * @param Light - The light to be removed.
	 */
	virtual void OnDetachLight(const FLightSceneInfo* Light)
	{
	}

	/**
	 * Called to notify the proxy when its transform has been updated.
	 * Called in the thread that owns the proxy; game or rendering.
	 */
	virtual void OnTransformChanged()
	{
	}

	/**
	 * Updates the primitive proxy's cached transforms, and calls OnUpdateTransform to notify it of the change.
	 * Called in the thread that owns the proxy; game or rendering.
	 * @param InLocalToWorld - The new local to world transform of the primitive.
	 * @param InLocalToWorldDeterminant - The new local to world transform's determinant.
	 */
	void SetTransform(const FMatrix& InLocalToWorld,FLOAT InLocalToWorldDeterminant);

	/**
	 * Returns whether the owning actor is movable or not.
	 * @return TRUE if the owning actor is movable
	 */
	UBOOL IsMovable()
	{
		return bMovable;
	}

	/**
	 * Checks if the primitive is owned by the given actor.
	 * @param Actor - The actor to check for ownership.
	 * @return TRUE if the primitive is owned by the given actor.
	 */
	UBOOL IsOwnedBy(const AActor* Actor) const
	{
		return Owners.FindItemIndex(Actor) != INDEX_NONE;
	}

	/** @return TRUE if the primitive is in different DPGs depending on view. */
	UBOOL HasViewDependentDPG() const
	{
		return bUseViewOwnerDepthPriorityGroup;
	}

	/**
	 * Determines the DPG to render the primitive in regardless of view.
	 * Should only be called if HasViewDependentDPG()==TRUE.
	 */
	BYTE GetStaticDepthPriorityGroup() const
	{
		check(!HasViewDependentDPG());
		return DepthPriorityGroup;
	}

	/**
	 * Determines the DPG to render the primitive in for the given view.
	 * May be called regardless of the result of HasViewDependentDPG.
	 * @param View - The view to determine the primitive's DPG for.
	 * @return The DPG the primitive should be rendered in for the given view.
	 */
	BYTE GetDepthPriorityGroup(const FSceneView* View) const
	{
		return (bUseViewOwnerDepthPriorityGroup && IsOwnedBy(View->ViewActor)) ?
			ViewOwnerDepthPriorityGroup :
			DepthPriorityGroup;
	}

	/** @return The local to world transform of the primitive. */
	const FMatrix& GetLocalToWorld() const
	{
		return LocalToWorld;
	}

	/** Every derived class should override these functions */
	virtual EMemoryStats GetMemoryStatType( void ) const = 0;
	virtual DWORD GetMemoryFootprint( void ) const = 0;
	DWORD GetAllocatedSize( void ) const { return( Owners.GetAllocatedSize() ); }

protected:

	/** Pointer back to the PrimitiveSceneInfo that owns this Proxy. */
	class FPrimitiveSceneInfo *	PrimitiveSceneInfo;
	friend class FPrimitiveSceneInfo;

	/** The primitive's local to world transform. */
	FMatrix LocalToWorld;

	/** The determinant of the local to world transform. */
	FLOAT LocalToWorldDeterminant;
public:
	/** The decals which interact with the primitive. */
	TArray<FDecalInteraction*> Decals;
protected:
	/** @return True if the primitive is visible in the given View. */
	UBOOL IsShown(const FSceneView* View) const;
	/** @return True if the primitive is casting a shadow. */
	UBOOL IsShadowCast(const FSceneView* View) const;

	/** @return True if the primitive has decals which should be rendered in the given view. */
	UBOOL HasRelevantDecals(const FSceneView* View) const;

private:

	BITFIELD bHiddenGame : 1;
	BITFIELD bHiddenEditor : 1;
	BITFIELD bIsNavigationPoint : 1;
	BITFIELD bOnlyOwnerSee : 1;
	BITFIELD bOwnerNoSee : 1;
	BITFIELD bMovable : 1;

	/** TRUE if ViewOwnerDepthPriorityGroup should be used. */
	BITFIELD bUseViewOwnerDepthPriorityGroup : 1;

	/** DPG this prim belongs to. */
	BITFIELD DepthPriorityGroup : UCONST_SDPG_NumBits;

	/** DPG this primitive is rendered in when viewed by its owner. */
	BITFIELD ViewOwnerDepthPriorityGroup : UCONST_SDPG_NumBits;

	TArray<const AActor*> Owners;

	FLOAT CullDistance;
};

/** Information about a vertex of a primitive's triangle. */
struct FPrimitiveTriangleVertex
{
	FVector WorldPosition;
	FVector WorldTangentX;
	FVector WorldTangentY;
	FVector WorldTangentZ;
};

/** An interface to some consumer of the primitive's triangles. */
class FPrimitiveTriangleDefinitionInterface
{
public:

	/**
	 * Defines a triangle.
	 * @param Vertex0 - The triangle's first vertex.
	 * @param Vertex1 - The triangle's second vertex.
	 * @param Vertex2 - The triangle's third vertex.
	 * @param StaticLighting - The triangle's static lighting information.
	 */
	virtual void DefineTriangle(
		const FPrimitiveTriangleVertex& Vertex0,
		const FPrimitiveTriangleVertex& Vertex1,
		const FPrimitiveTriangleVertex& Vertex2
		) = 0;
};

//
// FForceApplicator - Callback interface to apply a force field to a component(eg Rigid Body, Cloth, Particle System etc)
// Used by AddForceField.
//

class FForceApplicator
{
public:
	
	/**
	Called to compute a number of forces resulting from a force field.

	@param Positions Array of input positions (in world space)
	@param PositionStride Number of bytes between consecutive position vectors
	@param Velocities Array of inpute velocities
	@param VelocityStride Number of bytes between consecutive velocity vectors
	@param OutForce Output array of force vectors, computed from position/velocity, forces are added to the existing value
	@param OutForceStride Number of bytes between consectutiive output force vectors
	@param Count Number of forces to compute
	@param Scale factor to apply to positions before use(ie positions may not be in unreal units)
	@param PositionBoundingBox Bounding box for the positions passed to the call

	@return TRUE if any force is non zero.
	*/

	virtual UBOOL ComputeForce(
		FVector* Positions, INT PositionStride, FLOAT PositionScale,
		FVector* Velocities, INT VelocityStride, FLOAT VelocityScale,
		FVector* OutForce, INT OutForceStride, FLOAT OutForceScale,
		FVector* OutTorque, INT OutTorqueStride, FLOAT OutTorqueScale,
		INT Count, const FBox& PositionBoundingBox) = 0;
};

//
//	UPrimitiveComponent
//

class UPrimitiveComponent : public UActorComponent
{
	DECLARE_ABSTRACT_CLASS(UPrimitiveComponent,UActorComponent,CLASS_NoExport,Engine);
public:

	static INT CurrentTag;

	/** The primitive's scene info. */
	class FPrimitiveSceneInfo* SceneInfo;

	/** A fence to track when the primitive is detached from the scene in the rendering thread. */
	FRenderCommandFence DetachFence;

	FLOAT LocalToWorldDeterminant;
	FMatrix LocalToWorld;
	INT MotionBlurInfoIndex;

	TArray<FDecalInteraction*> DecalList;

	INT Tag;

	UPrimitiveComponent* ShadowParent;

	/** Keeps track of which fog component this primitive is using. */
	class UFogVolumeDensityComponent* FogVolumeComponent;

	FBoxSphereBounds Bounds;

	/** The lighting environment to take the primitive's lighting from. */
	class ULightEnvironmentComponent* LightEnvironment;

	/** Cull distance exposed to LDs. The real cull distance is the min (disregarding 0) of this and volumes affecting this object. */
	FLOAT LDCullDistance;

	/**
	 * The distance to cull this primitive at.  A CullDistance of 0 indicates that the primitive should not be culled by distance.
	 * The distance calculation uses the center of the primitive's bounding sphere.
	 */
	FLOAT CachedCullDistance;

	/** The scene depth priority group to draw the primitive in. */
	BYTE DepthPriorityGroup;

	/** The scene depth priority group to draw the primitive in, if it's being viewed by its owner. */
	BYTE ViewOwnerDepthPriorityGroup;

	/** If detail mode is >= system detail mode, primitive won't be rendered. */
	BYTE DetailMode;
	
	/** Scalar controlling the amount of motion blur to be applied when object moves */
	FLOAT	MotionBlurScale;

	/** True if the primitive should be rendered using ViewOwnerDepthPriorityGroup if viewed by its owner. */
	BITFIELD	bUseViewOwnerDepthPriorityGroup:1 GCC_BITFIELD_MAGIC;

	/** Whether to accept cull distance volumes to modify cached cull distance. */
	BITFIELD	bAllowCullDistanceVolume:1;

	BITFIELD	HiddenGame:1;
	BITFIELD	HiddenEditor:1;

	/** If this is True, this component won't be visible when the view actor is the component's owner, directly or indirectly. */
	BITFIELD	bOwnerNoSee:1;

	/** If this is True, this component will only be visible when the view actor is the component's owner, directly or indirectly. */
	BITFIELD	bOnlyOwnerSee:1;

	/** If this is True, this primitive will be used to occlusion cull other primitives. */
	BITFIELD	bUseAsOccluder:1;

	/** If this is True, this component doesn't need exact occlusion info. */
	BITFIELD	bAllowApproximateOcclusion:1;

	/** If TRUE, forces mips for textures used by this component to be resident when this component's level is loaded. */
	BITFIELD     bForceMipStreaming:1;

	/** If TRUE, this primitive accepts decals.*/
	BITFIELD	bAcceptsDecals:1;

	/** If TRUE, this primitive accepts decals spawned during gameplay.  Effectively FALSE if bAcceptsDecals is FALSE. */
	BITFIELD	bAcceptsDecalsDuringGameplay:1;

	BITFIELD	bIsRefreshingDecals:1;

	/** If TRUE, this primitive accepts foliage. */
	BITFIELD	bAcceptsFoliage:1;

	/** 
	* Translucent objects with a lower sort priority draw before objects with a higher priority.
	* Translucent objects with the same priority are rendered from back-to-front based on their bounds origin.
	*
	* Ignored if the object is not translucent.
	* The default priority is zero. 
	**/
	INT TranslucencySortPriority;

	// Lighting flags

	BITFIELD	CastShadow:1;

	/** If true, forces all static lights to use light-maps for direct lighting on this primitive, regardless of the light's UseDirectLightMap property. */
	BITFIELD	bForceDirectLightMap : 1;
	
	/** If true, primitive casts dynamic shadows. */
	BITFIELD	bCastDynamicShadow : 1;
	/** If TRUE, primitive will cast shadows even if bHidden is TRUE. */
	BITFIELD	bCastHiddenShadow:1;
	
	BITFIELD	bAcceptsLights:1;
	
	/** Whether this primitives accepts dynamic lights */
	BITFIELD	bAcceptsDynamicLights:1;

	/** Lighting channels controlling light/ primitive interaction. Only allows interaction if at least one channel is shared */
	FLightingChannelContainer	LightingChannels;

	/** Whether the primitive supports/ allows static shadowing */
	BITFIELD	bUsePrecomputedShadows:1;

	// Collision flags.

	BITFIELD	CollideActors:1;
	BITFIELD	BlockActors:1;
	BITFIELD	BlockZeroExtent:1;
	BITFIELD	BlockNonZeroExtent:1;
	BITFIELD	BlockRigidBody:1;

	/** 
	 *	Disables collision between any rigid body physics in this Component and pawn bodies. 
	 *	DEPRECATED! Use RBChannel/RBCollideWithChannels instead now
	 */
	BITFIELD	RigidBodyIgnorePawns:1;

	/** Enum indicating what type of object this should be considered for rigid body collision. */
	BYTE		RBChannel GCC_BITFIELD_MAGIC;

	/** Types of objects that this physics objects will collide with. */
	FRBCollisionChannelContainer RBCollideWithChannels;

	BITFIELD	bNotifyRigidBodyCollision:1;

	// Novodex fluids
	BITFIELD	bFluidDrain:1;
	BITFIELD	bFluidTwoWay:1;

	BITFIELD	bIgnoreRadialImpulse:1;
	BITFIELD	bIgnoreRadialForce:1;

	/** Disables the influence from ALL types of force fields. */
	BITFIELD	bIgnoreForceField:1;

	
	/** Place into a NxCompartment that will run in parallel with the primary scene's physics with potentially different simulation parameters.
	 *  If double buffering is enabled in the WorldInfo then physics will run in parallel with the entire game for this component. */
	BITFIELD	bUseCompartment:1;	// hardware scene support

	BITFIELD	AlwaysLoadOnClient:1;
	BITFIELD	AlwaysLoadOnServer:1;

	BITFIELD	bIgnoreHiddenActorsMembership:1;

	BITFIELD							bWasSNFiltered:1;
	TArrayNoInit<class FOctreeNode*>	OctreeNodes;
	
	class UPhysicalMaterial*	PhysMaterialOverride;
	class URB_BodyInstance*		BodyInstance;

	/** 
	 *	Used for creating one-way physics interactions (via constraints or contacts) 
	 *	Groups with lower RBDominanceGroup push around higher values in a 'one way' fashion. Must be <32.
	 */
	BYTE		RBDominanceGroup;

	// Copied from TransformComponent
	FMatrix CachedParentToWorld;
	FVector		Translation;
	FRotator	Rotation;
	FLOAT		Scale;
	FVector		Scale3D;
	BITFIELD	AbsoluteTranslation:1;
	BITFIELD	AbsoluteRotation:1;
	BITFIELD	AbsoluteScale:1;

	/** Last time the component was submitted for rendering (called FScene::AddPrimitive). */
	FLOAT		LastSubmitTime;

	/** Last render time in seconds since level started play. Updated to WorldInfo->TimeSeconds so float is sufficient. */
	FLOAT		LastRenderTime;

	/** if > 0, the script RigidBodyCollision() event will be called on our Owner when a physics collision involving
	 * this PrimitiveComponent occurs and the relative velocity is greater than or equal to this
	 */
	FLOAT ScriptRigidBodyCollisionThreshold;

	// Should this Component be in the Octree for collision
	UBOOL ShouldCollide() const;

	void AttachDecal(class UDecalComponent* Decal, class FDecalRenderData* RenderData, const class FDecalState* DecalState);
	void DetachDecal(class UDecalComponent* Decal);

	/** Creates the render data for a decal on this primitive. */
	virtual class FDecalRenderData* GenerateDecalRenderData(class FDecalState* Decal) const;

	/**
	 * Returns True if a primitive cannot move or be destroyed during gameplay, and can thus cast and receive static shadowing.
	 */
	UBOOL HasStaticShadowing() const;

	/**
	 * Returns whether this primitive only uses unlit materials.
	 *
	 * @return TRUE if only unlit materials are used for rendering, false otherwise.
	 */
	virtual UBOOL UsesOnlyUnlitMaterials() const;

	/**
	 * Returns the lightmap resolution used for this primivite instnace in the case of it supporting texture light/ shadow maps.
	 * 0 if not supported or no static shadowing.
	 *
	 * @param Width		[out]	Width of light/shadow map
	 * @param Height	[out]	Height of light/shadow map
	 */
	virtual void GetLightMapResolution( INT& Width, INT& Height ) const;

	/**
	 * Returns the light and shadow map memory for this primite in its out variables.
	 *
	 * Shadow map memory usage is per light whereof lightmap data is independent of number of lights, assuming at least one.
	 *
	 * @param [out] LightMapMemoryUsage		Memory usage in bytes for light map (either texel or vertex) data
	 * @param [out]	ShadowMapMemoryUsage	Memory usage in bytes for shadow map (either texel or vertex) data
	 */
	virtual void GetLightAndShadowMapMemoryUsage( INT& LightMapMemoryUsage, INT& ShadowMapMemoryUsage ) const;

	/**
	 * Validates the lighting channels and makes adjustments as appropriate.
	 */
	void ValidateLightingChannels();

	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();

	/**
	 * Sets Bounds.  Default behavior is a bounding box/sphere the size of the world.
	 */
	virtual void UpdateBounds();

	/**
	 * Requests the information about the component that the static lighting system needs.
	 * @param OutPrimitiveInfo - Upon return, contains the component's static lighting information.
	 * @param InRelevantLights - The lights relevant to the primitive.
	 * @param InOptions - The options for the static lighting build.
	 */
	virtual void GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options) {}

	/**
	 * Gets the primitive's static triangles.
	 * @param PTDI - An implementation of the triangle definition interface.
	 */
	virtual void GetStaticTriangles(FPrimitiveTriangleDefinitionInterface* PTDI) const {}

	// Collision tests.

	virtual UBOOL PointCheck(FCheckResult& Result,const FVector& Location,const FVector& Extent,DWORD TraceFlags) { return 1; }
	virtual UBOOL LineCheck(FCheckResult& Result,const FVector& End,const FVector& Start,const FVector& Extent,DWORD TraceFlags) { return 1; }

	FVector GetOrigin() const
	{
		return LocalToWorld.GetOrigin();
	}

	/** Removes any scaling from the LocalToWorld matrix and returns it, along with the overall scaling. */
	void GetTransformAndScale(FMatrix& OutTransform, FVector& OutScale);

	// Rigid-Body Physics

	virtual void InitComponentRBPhys(UBOOL bFixed);
	virtual void SetComponentRBFixed(UBOOL bFixed);
	virtual void TermComponentRBPhys(FRBPhysScene* InScene);

	/** Return the BodySetup to use for this PrimitiveComponent (single body case) */
	virtual class URB_BodySetup* GetRBBodySetup() { return NULL; }

	/** Returns any pre-cooked convex mesh data associated with this PrimitiveComponent. Used by InitBody. */
	virtual FKCachedConvexData* GetCachedPhysConvexData(const FVector& InScale3D) { return NULL; }
	class URB_BodySetup* FindRBBodySetup();
	void AddImpulse(FVector Impulse, FVector Position = FVector(0,0,0), FName BoneName = NAME_None, UBOOL bVelChange=false);
	virtual void AddRadialImpulse(const FVector& Origin, FLOAT Radius, FLOAT Strength, BYTE Falloff, UBOOL bVelChange=false);
	void AddForce(FVector Force, FVector Position = FVector(0,0,0), FName BoneName = NAME_None);
	virtual void AddRadialForce(const FVector& Origin, FLOAT Radius, FLOAT Strength, BYTE Falloff);

	/** Applies the affect of a force field to this primitive component. */
	virtual void AddForceField(FForceApplicator* Applicator, const FBox& FieldBoundingBox, UBOOL bApplyToCloth, UBOOL bApplyToRigidBody);

	virtual void SetRBLinearVelocity(const FVector& NewVel, UBOOL bAddToCurrent=false);
	virtual void SetRBAngularVelocity(const FVector& NewAngVel, UBOOL bAddToCurrent=false);
	virtual void SetRBPosition(const FVector& NewPos, FName BoneName = NAME_None);
	virtual void SetRBRotation(const FRotator& NewRot, FName BoneName = NAME_None);
	virtual void WakeRigidBody( FName BoneName = NAME_None );
	virtual void PutRigidBodyToSleep(FName BoneName = NAME_None);
	UBOOL RigidBodyIsAwake( FName BoneName = NAME_None );
	virtual void SetBlockRigidBody(UBOOL bNewBlockRigidBody);
	void SetRBCollidesWithChannel(ERBCollisionChannel Channel, UBOOL bNewCollides);
	void SetRBChannel(ERBCollisionChannel Channel);
	virtual void SetNotifyRigidBodyCollision(UBOOL bNewNotifyRigidBodyCollision);
	
	/** 
	 *	Used for creating one-way physics interactions.
	 *	@see RBDominanceGroup
	 */
	virtual void SetRBDominanceGroup(BYTE InDomGroup);

	/** 
	 *	Changes the current PhysMaterialOverride for this component. 
	 *	Note that if physics is already running on this component, this will _not_ alter its mass/inertia etc, it will only change its 
	 *	surface properties like friction and the damping.
	 */
	virtual void SetPhysMaterialOverride(UPhysicalMaterial* NewPhysMaterial);

	/** returns the physics RB_BodyInstance for the root body of this component (if any) */
	virtual URB_BodyInstance* GetRootBodyInstance();

	virtual void UpdateRBKinematicData();

	// Copied from TransformComponent
	virtual void SetTransformedToWorld();

	DECLARE_FUNCTION(execAddImpulse);
	DECLARE_FUNCTION(execAddRadialImpulse);
	DECLARE_FUNCTION(execAddForce);
	DECLARE_FUNCTION(execAddRadialForce);
	DECLARE_FUNCTION(execSetRBLinearVelocity);
	DECLARE_FUNCTION(execSetRBAngularVelocity);
	DECLARE_FUNCTION(execSetRBPosition);
	DECLARE_FUNCTION(execSetRBRotation);
	DECLARE_FUNCTION(execWakeRigidBody);
	DECLARE_FUNCTION(execPutRigidBodyToSleep);
	DECLARE_FUNCTION(execRigidBodyIsAwake);
	DECLARE_FUNCTION(execSetBlockRigidBody);
	DECLARE_FUNCTION(execSetRBCollidesWithChannel);
	DECLARE_FUNCTION(execSetRBChannel);
	DECLARE_FUNCTION(execSetNotifyRigidBodyCollision);
	DECLARE_FUNCTION(execSetPhysMaterialOverride);
	DECLARE_FUNCTION(execSetRBDominanceGroup);

	DECLARE_FUNCTION(execSetHidden);
	DECLARE_FUNCTION(execSetOwnerNoSee);
	DECLARE_FUNCTION(execSetOnlyOwnerSee);
	DECLARE_FUNCTION(execSetShadowParent);
	DECLARE_FUNCTION(execSetLightEnvironment);
	DECLARE_FUNCTION(execSetCullDistance);
	DECLARE_FUNCTION(execSetLightingChannels);
	DECLARE_FUNCTION(execSetDepthPriorityGroup);
	DECLARE_FUNCTION(execSetViewOwnerDepthPriorityGroup);
	DECLARE_FUNCTION(execSetTraceBlocking);
	DECLARE_FUNCTION(execSetActorCollision);
	DECLARE_FUNCTION(execGetRootBodyInstance);

	// Copied from TransformComponent
	DECLARE_FUNCTION(execSetTranslation);
	DECLARE_FUNCTION(execSetRotation);
	DECLARE_FUNCTION(execSetScale);
	DECLARE_FUNCTION(execSetScale3D);
	DECLARE_FUNCTION(execSetAbsolute);

#if WITH_NOVODEX
	virtual class NxActor* GetNxActor(FName BoneName = NAME_None);

	/** Utility for getting all physics bodies contained within this component. */
	virtual void GetAllNxActors(TArray<class NxActor*>& OutActors);
#endif // WITH_NOVODEX

	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy()
	{
		return NULL;
	}

	/**
	 * Determines whether the proxy for this primitive type needs to be recreated whenever the primitive moves.
	 * @return TRUE to recreate the proxy when UpdateTransform is called.
	 */
	virtual UBOOL ShouldRecreateProxyOnUpdateTransform() const
	{
		return FALSE;
	}

protected:
	/** @name UActorComponent interface. */
	//@{
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);
	virtual void Attach();
	virtual void UpdateTransform();
	virtual void Detach();
	//@}

	/** Internal function that updates physics objects to match the RBChannel/RBCollidesWithChannel info. */
	virtual void UpdatePhysicsToRBChannels();
public:

	// UObject interface.

	virtual void Serialize(FArchive& Ar);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void PostLoad();
	virtual UBOOL IsReadyForFinishDestroy();
	virtual UBOOL NeedsLoadForClient() const;
	virtual UBOOL NeedsLoadForServer() const;

	virtual void SetOwnerNoSee(UBOOL bNewOwnerNoSee);
	virtual void SetOnlyOwnerSee(UBOOL bNewOnlyOwnerSee);

	virtual void SetHiddenGame(UBOOL NewHidden);

	/**
	 *	Sets the HiddenEditor flag and reattaches the component as necessary.
	 *
	 *	@param	NewHidden		New Value fo the HiddenEditor flag.
	 */
	virtual void SetHiddenEditor(UBOOL NewHidden);
	virtual void SetShadowParent(UPrimitiveComponent* NewShadowParent);
	virtual void SetLightEnvironment(ULightEnvironmentComponent* NewLightEnvironment);
	virtual void SetCullDistance(FLOAT NewCullDistance);
	virtual void SetLightingChannels(FLightingChannelContainer NewLightingChannels);
	virtual void SetDepthPriorityGroup(ESceneDepthPriorityGroup NewDepthPriorityGroup);
	virtual void SetViewOwnerDepthPriorityGroup(
		UBOOL bNewUseViewOwnerDepthPriorityGroup,
		ESceneDepthPriorityGroup NewViewOwnerDepthPriorityGroup
		);

	/**
	 * Default constructor, generates a GUID for the primitive.
	 */
	UPrimitiveComponent();
};

//
//	UMeshComponent
//

class UMeshComponent : public UPrimitiveComponent
{
	DECLARE_ABSTRACT_CLASS(UMeshComponent,UPrimitiveComponent,CLASS_NoExport,Engine);
public:

	TArrayNoInit<UMaterialInterface*>	Materials;

	/** @return The total number of elements in the mesh. */
	virtual INT GetNumElements() const PURE_VIRTUAL(UMeshComponent::GetNumElements,return 0;);

	/** Accesses the material applied to a specific material index. */
	virtual UMaterialInterface* GetMaterial(INT ElementIndex) const;

	/** Sets the material applied to a material index. */
	void SetMaterial(INT ElementIndex,UMaterialInterface* Material);

	/** Accesses the scene relevance information for the materials applied to the mesh. */
	FMaterialViewRelevance GetMaterialViewRelevance() const;

	// UnrealScript interface.
	DECLARE_FUNCTION(execGetMaterial);
	DECLARE_FUNCTION(execSetMaterial);
	DECLARE_FUNCTION(execGetNumElements);
};

//
//	USpriteComponent
//

class USpriteComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(USpriteComponent,UPrimitiveComponent,CLASS_NoExport,Engine);
public:
	
	UTexture2D*	Sprite;
	BITFIELD	bIsScreenSizeScaled:1;
	FLOAT		ScreenSize;

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();
};

//
//	UBrushComponent
//

class UBrushComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UBrushComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	UModel*						Brush;

	/** Simplified collision data for the mesh. */
	FKAggregateGeom				BrushAggGeom;

	/** Physics engine shapes created for this BrushComponent. */
	class NxActorDesc*			BrushPhysDesc;

	/** Cached brush convex-mesh data for use with the physics engine. */
	FKCachedConvexData			CachedPhysBrushData;

	/** 
	 *	Indicates version that CachedPhysBrushData was created at. 
	 *	Compared against CurrentCachedPhysDataVersion.
	 */
	INT							CachedPhysBrushDataVersion;

	// UObject interface.
	virtual void Serialize( FArchive& Ar );
	virtual void PostLoad();
	virtual void PreSave();
	virtual void FinishDestroy();

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();

	virtual void InitComponentRBPhys(UBOOL bFixed);

	virtual UBOOL PointCheck(FCheckResult& Result,const FVector& Location,const FVector& Extent,DWORD TraceFlags);
	virtual UBOOL LineCheck(FCheckResult& Result,const FVector& End,const FVector& Start,const FVector& Extent,DWORD TraceFlags);

	virtual void UpdateBounds();

	// UBrushComponent interface.
	virtual UBOOL IsValidComponent() const;

	// UBrushComponent interface

	/** Create the BrushAggGeom collection-of-convex-primitives from the Brush UModel data. */
	void BuildSimpleBrushCollision();

	/** Build cached convex data for physics engine. */
	void BuildPhysBrushData();
};

/**
 * Breaks a set of brushes down into a set of convex volumes.  The convex volumes are in world-space.
 * @param Brushes			The brushes to enumerate the convex volumes from.
 * @param OutConvexVolumes	Upon return, contains the convex volumes which compose the input brushes.
 */
extern void GetConvexVolumesFromBrushes(const TArray<ABrush*>& Brushes,TArray<FConvexVolume>& OutConvexVolumes);

//
//	UCylinderComponent
//

class UCylinderComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UCylinderComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	FLOAT	CollisionHeight;
	FLOAT	CollisionRadius;

	// UPrimitiveComponent interface.

	virtual UBOOL PointCheck(FCheckResult& Result,const FVector& Location,const FVector& Extent,DWORD TraceFlags);
	virtual UBOOL LineCheck(FCheckResult& Result,const FVector& End,const FVector& Start,const FVector& Extent,DWORD TraceFlags);

	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();

	// UCylinderComponent interface.

	void SetCylinderSize(FLOAT NewRadius, FLOAT NewHeight);

	// Native script functions.

	DECLARE_FUNCTION(execSetCylinderSize);
};

//
//	UArrowComponent
//

class UArrowComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UArrowComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	FColor	ArrowColor;
	FLOAT	ArrowSize;

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();
};

//
//	UDrawSphereComponent
//

class UDrawSphereComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UDrawSphereComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	FColor				SphereColor;
	UMaterialInterface*	SphereMaterial;
	FLOAT				SphereRadius;
	INT					SphereSides;
	BITFIELD			bDrawWireSphere:1;
	BITFIELD			bDrawLitSphere:1;

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();
};

//
//	UDrawCylinderComponent
//

class UDrawCylinderComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UDrawCylinderComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	FColor						CylinderColor;
	class UMaterialInstance*	CylinderMaterial;
	FLOAT						CylinderRadius;
	FLOAT						CylinderTopRadius;
	FLOAT						CylinderHeight;
	FLOAT						CylinderHeightOffset;
	INT							CylinderSides;
	BITFIELD					bDrawWireCylinder:1;
	BITFIELD					bDrawLitCylinder:1;

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();
};

//
//	UDrawBoxComponent
//

class UDrawBoxComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UDrawBoxComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	FColor				BoxColor;
	UMaterialInstance*	BoxMaterial;
	FVector				BoxExtent;
	BITFIELD			bDrawWireBox:1;
	BITFIELD			bDrawLitBox:1;

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();
};

//
//	UDrawBoxComponent
//

class UDrawCapsuleComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UDrawCapsuleComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	FColor				CapsuleColor;
	UMaterialInstance*	CapsuleMaterial;
	float				CapsuleHeight;
	float				CapsuleRadius;
	BITFIELD			bDrawWireCapsule:1;
	BITFIELD			bDrawLitCapsule:1;

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();
};

//
//	UDrawFrustumComponent
//

class UDrawFrustumComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UDrawFrustumComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	FColor			FrustumColor;
	FLOAT			FrustumAngle;
	FLOAT			FrustumAspectRatio;
	FLOAT			FrustumStartDist;
	FLOAT			FrustumEndDist;
	/** optional texture to show on the near plane */
	UTexture*		Texture;

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();
};

class UDrawLightRadiusComponent : public UDrawSphereComponent
{
	DECLARE_CLASS(UDrawLightRadiusComponent,UDrawSphereComponent,CLASS_NoExport,Engine);

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();
};

//
//	UCameraConeComponent
//

class UCameraConeComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UCameraConeComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	// UPrimitiveComponent interface.
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void UpdateBounds();
};


/**
 *	Utility component for drawing a textured quad face. 
 *  Origin is at the component location, frustum points down position X axis.
 */
class UDrawQuadComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UDrawQuadComponent,UPrimitiveComponent,CLASS_NoExport,Engine);

	/** Texture source to draw on quad face */
	UTexture*	Texture;
	/** Width of quad face */
	FLOAT		Width;
	/** Height of quad face */
	FLOAT		Height;

	// UPrimitiveComponent interface.

	virtual void Render(const FSceneView* View,class FPrimitiveDrawInterface* PDI);
	virtual void UpdateBounds();
};
