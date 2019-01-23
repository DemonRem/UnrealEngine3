/*=============================================================================
	SceneCore.h: Core scene definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Forward declarations.
class FStaticMesh;
class FScene;
class FPrimitiveSceneInfo;
class FLightSceneInfo;

/** The type of dynamic shadow which results from a light-primitive interaction. */
enum EDynamicShadowType
{
	DST_None = 0,
	DST_Volume = 1,
	DST_Projected = 2,
	DST_NumBits = 2
};

/**
 * An interaction between a light and a primitive.
 */
class FLightPrimitiveInteraction
{
public:

	~FLightPrimitiveInteraction();

	/** Creates an interaction for a light-primitive pair. */
	static void Create(FLightSceneInfo* LightSceneInfo,FPrimitiveSceneInfo* PrimitiveSceneInfo);

	// Accessors.
	UBOOL HasShadow() const { return bCastShadow; }
	UBOOL IsLightMapped() const { return bLightMapped; }
	FLightSceneInfo *	GetLight() const { return LightSceneInfo; }
	EDynamicShadowType GetDynamicShadowType() const { return (EDynamicShadowType)DynamicShadowType; }
	INT GetLightId() const { return LightId; }
	FPrimitiveSceneInfo* GetPrimitiveSceneInfo() const { return PrimitiveSceneInfo; }
	FLightPrimitiveInteraction* GetNextPrimitive() const { return NextPrimitive; }
	FLightPrimitiveInteraction* GetNextLight() const { return NextLight; }

	/** Hash function required for TMap support */
	friend DWORD GetTypeHash( const FLightPrimitiveInteraction* Interaction )
	{
		return (DWORD)Interaction->LightId;
	}

private:

	/** The index into Scene->Lights of the light which affects the primitive. */
	INT LightId;

	/** The light which affects the primitive. */
	FLightSceneInfo* LightSceneInfo;

	/** The primitive which is affected by the light. */
	FPrimitiveSceneInfo* PrimitiveSceneInfo;

	/** True if the primitive casts a shadow from the light. */
	BITFIELD bCastShadow : 1;

	/** True if the primitive has a light-map containing the light. */
	BITFIELD bLightMapped : 1;

	/** True if the interaction is an uncached static lighting interaction. */
	BITFIELD bUncachedStaticLighting : 1;

	/** The type of dynamic shadow used for this interaction. */
	BITFIELD DynamicShadowType : DST_NumBits;

	/** A pointer to the NextPrimitive member of the previous interaction in the light's interaction list. */
	FLightPrimitiveInteraction** PrevPrimitiveLink;

	/** The next interaction in the light's interaction list. */
	FLightPrimitiveInteraction* NextPrimitive;

	/** A pointer to the NextLight member of the previous interaction in the primitive's interaction list. */
	FLightPrimitiveInteraction** PrevLightLink;

	/** The next interaction in the primitive's interaction list. */
	FLightPrimitiveInteraction* NextLight;
	
	/** Initialization constructor. */
	FLightPrimitiveInteraction(FLightSceneInfo* InLightSceneInfo,FPrimitiveSceneInfo* InPrimitiveSceneInfo,UBOOL bNoStaticShadowing,UBOOL bInLightMapped);

	/** Hide default constructor. */
	FLightPrimitiveInteraction();
};

/** Information about the primitives and lights in a light environment. */
class FLightEnvironmentSceneInfo
{
public:

	/** The light environment component that this scene info is for. */
	ULightEnvironmentComponent* Component;

	/** The index of the light environment scene info in FScene::LightEnvironments. */
	INT Id;

	/** The primitives attached to the light environment. */
	TArray<FPrimitiveSceneInfo*> AttachedPrimitives;

	/** The lights attached to the light environment. */
	TArray<FLightSceneInfo*> AttachedLights;

	/** The light components that affect the light environment. */
	TMap<const ULightComponent*,UBOOL> Lights;

	/** Initialization constructor. */
	FLightEnvironmentSceneInfo(ULightEnvironmentComponent* InComponent):
		Component(InComponent),
		Id(INDEX_NONE)
	{}
};

/**
 * The information used to a render a primitive.  This is the rendering thread's mirror of the game thread's UPrimitiveComponent.
 */
class FPrimitiveSceneInfo : public FDeferredCleanupInterface
{
public:

	/** The scene the primitive is in. */
	FScene* Scene;

	/** The render proxy for the primitive. */
	FPrimitiveSceneProxy* Proxy;

	/** The UPrimitiveComponent this scene info is for. */
	UPrimitiveComponent* Component;

	/** The actor which owns the PrimitiveComponent. */
	AActor* Owner;

	/** The primitive's static meshes. */
	TIndirectArray<FStaticMesh> StaticMeshes;

	/** The index of the primitive in Scene->Primitives. */
	INT Id;

	/** The translucency sort priority */
	INT TranslucencySortPriority;

	/** True if the primitive will cache static shadowing. */
	BITFIELD bStaticShadowing : 1;

	/** True if the primitive casts dynamic shadows. */
	BITFIELD bCastDynamicShadow : 1;
	
	/** True if the primitive casts static shadows. */
	BITFIELD bCastStaticShadow : 1;

	/** True if the primitive casts shadows even when hidden. */
	BITFIELD bCastHiddenShadow : 1;

	/** True if the primitive receives lighting. */
	BITFIELD bAcceptsLights : 1;

	/** True if the primitive should be affected by dynamic lights. */
	BITFIELD bAcceptsDynamicLights : 1;

	/** True if the primitive should only be affected by lights in the same level. */
	BITFIELD bSelfContainedLighting : 1;

	/** If this is True, this primitive will be used to occlusion cull other primitives. */
	BITFIELD bUseAsOccluder:1;

	/** If this is True, this primitive doesn't need exact occlusion info. */
	BITFIELD bAllowApproximateOcclusion : 1;

	/** If this is TRUE, this primitive's static meshes need to be updated before it can be rendered. */
	BITFIELD bNeedsStaticMeshUpdate : 1;

	/** The primitive's bounds. */
	FBoxSphereBounds Bounds;

	/** The primitive's cull distance. */
	FLOAT CullDistance;

	/** The hit proxies used by the primitive. */
	TArray<TRefCountPtr<HHitProxy> > HitProxies;

	/** The ID of the hit proxy which is used to represent the primitive's dynamic elements. */
	FHitProxyId DefaultDynamicHitProxyId;

	/** The light channels which this primitive is affected by. */
	const FLightingChannelContainer LightingChannels;

	/** The scene info for the light environment that the primitive is in. */
	FLightEnvironmentSceneInfo* LightEnvironmentSceneInfo;

	/** The name of the level the primitive is in. */
	FName LevelName;

	/** The list of lights affecting this primitive. */
	FLightPrimitiveInteraction* LightList;

	/** The aggregate light color of the upper sky light hemispheres affecting this primitive. */
	FLinearColor UpperSkyLightColor;

	/** The aggregate light color of the lower sky light hemispheres affecting this primitive. */
	FLinearColor LowerSkyLightColor;

	/** A primitive which this primitive is grouped with for projected shadows. */
	FPrimitiveSceneInfo* ShadowParent;

	/** The first primitive which is part of this primitive's projected shadow group. */
	FPrimitiveSceneInfo* FirstShadowChild;

	/** The next primitive which is part of the ShadowParent's projected shadow group. */
	FPrimitiveSceneInfo* NextShadowChild;

	/** The fog volume this primitive is intersecting with. */
	FFogVolumeDensitySceneInfo* FogVolumeSceneInfo;

	/** Last render time in seconds since level started play. */
	FLOAT LastRenderTime;

	/** Initialization constructor. */
	FPrimitiveSceneInfo(UPrimitiveComponent* InPrimitive,FPrimitiveSceneProxy* InProxy,FScene* InScene);

	/** Adds the primitive to the scene. */
	void AddToScene();

	/** Removes the primitive from the scene. */
	void RemoveFromScene();

	/** Updates the primitive's static meshes in the scene. */
	void ConditionalUpdateStaticMeshes();

	/** Sets a flag to update the primitive's static meshes before it is next rendered. */
	void BeginDeferredUpdateStaticMeshes();

	/** Links the primitive to its shadow parent. */
	void LinkShadowParent();

	/** Unlinks the primitive from its shadow parent. */
	void UnlinkShadowParent();

	// FDeferredCleanupInterface
	virtual void FinishCleanup();

	/** Size this class uses in bytes */
	UINT GetMemoryFootprint( void ) { return( sizeof( *this ) + HitProxies.GetAllocatedSize() + StaticMeshes.GetAllocatedSize() ); }

	/** Determines whether the primitive has dynamic sky lighting. */
	UBOOL HasDynamicSkyLighting() const
	{
		return (!UpperSkyLightColor.Equals(FLinearColor::Black) || !LowerSkyLightColor.Equals(FLinearColor::Black))
			//only use dynamic sky lighting with directional lightmaps
			&& GSystemSettings.bAllowDirectionalLightMaps;
	}
};

/** The information needed to determine whether a primitive is visible. */
class FPrimitiveSceneInfoCompact
{
public:

	FPrimitiveSceneInfo* PrimitiveSceneInfo;
	FPrimitiveSceneProxy* Proxy;
	UPrimitiveComponent* Component;
	FBoxSphereBounds Bounds;
	FLOAT CullDistance;
	BITFIELD bAllowApproximateOcclusion : 1;
	BITFIELD bAcceptsLights : 1;

	/** Initializes the compact scene info from the primitive's full scene info. */
	void Init(FPrimitiveSceneInfo* InPrimitiveSceneInfo)
	{
		PrimitiveSceneInfo = InPrimitiveSceneInfo;
		Proxy = PrimitiveSceneInfo->Proxy;
		Component = PrimitiveSceneInfo->Component;
		Bounds = PrimitiveSceneInfo->Bounds;
		CullDistance = PrimitiveSceneInfo->CullDistance;
		bAllowApproximateOcclusion = PrimitiveSceneInfo->bAllowApproximateOcclusion;
		bAcceptsLights = PrimitiveSceneInfo->bAcceptsLights;
	}

	/** Default constructor. */
	FPrimitiveSceneInfoCompact():
		PrimitiveSceneInfo(NULL),
		Proxy(NULL),
		Component(NULL)
	{}

	/** Initialization constructor. */
	FPrimitiveSceneInfoCompact(FPrimitiveSceneInfo* InPrimitiveSceneInfo)
	{
		Init(InPrimitiveSceneInfo);
	}
};

/**
* The information used to a render a scene capture.  This is the rendering thread's mirror of the game thread's USceneCaptureComponent.
*/
class FCaptureSceneInfo
{
public:

	/** The capture probe for the component. */
	FSceneCaptureProbe* SceneCaptureProbe;

	/** The USceneCaptureComponent this scene info is for.  It is not safe to dereference this pointer.  */
	const USceneCaptureComponent* Component;

	/** The index of the info in Scene->SceneCaptures. */
	INT Id;

	/** Scene currently using this capture info */
	FScene* Scene;

	/** 
	* Constructor 
	* @param InComponent - mirrored scene capture component requesting the capture
	* @param InSceneCaptureProbe - new probe for capturing the scene
	*/
	FCaptureSceneInfo(USceneCaptureComponent* InComponent,FSceneCaptureProbe* InSceneCaptureProbe);

	/** 
	* Destructor
	*/
	~FCaptureSceneInfo();

	/**
	* Capture the scene
	* @param SceneRenderer - original scene renderer so that we can match certain view settings
	*/
	void CaptureScene(class FSceneRenderer* SceneRenderer);

	/**
	* Add this capture scene info to a scene 
	* @param InScene - scene to add to
	*/
	void AddToScene(class FScene* InScene);

	/**
	* Remove this capture scene info from a scene 
	* @param InScene - scene to remove from
	*/
	void RemoveFromScene(class FScene* InScene);
};

/**
 * A mesh which is defined by a primitive at scene segment construction time and never changed.
 * Lights are attached and detached as the segment containing the mesh is added or removed from a scene.
 */
class FStaticMesh : public FMeshElement
{
public:

	/**
	 * An interface to a draw list's reference to this static mesh.
	 * used to remove the static mesh from the draw list without knowing the draw list type.
	 */
	class FDrawListElementLink : public FRefCountedObject
	{
	public:

		virtual void Remove() = 0;
	};

	/** The squared minimum distance to draw the primitive at. */
	FLOAT MinDrawDistanceSquared;

	/** The squared maximum distance to draw the primitive at. */
	FLOAT MaxDrawDistanceSquared;

	/** The render info for the primitive which created this mesh. */
	FPrimitiveSceneInfo* PrimitiveSceneInfo;

	/** The ID of the hit proxy which represents this static mesh. */
	FHitProxyId HitProxyId;

	/** The index of the mesh in the scene's static meshes array. */
	INT Id;

	// Constructor/destructor.
	FStaticMesh(
		FPrimitiveSceneInfo* InPrimitiveSceneInfo,
		const FMeshElement& InMesh,
		FLOAT InMinDrawDistanceSquared,
		FLOAT InMaxDrawDistanceSquared,
		FHitProxyId InHitProxyId
		):
		FMeshElement(InMesh),
		MinDrawDistanceSquared(InMinDrawDistanceSquared),
		MaxDrawDistanceSquared(InMaxDrawDistanceSquared),
		PrimitiveSceneInfo(InPrimitiveSceneInfo),
		HitProxyId(InHitProxyId),
		Id(INDEX_NONE)
	{
		// If the static mesh is in an invalid DPG, move it to the world DPG.
        if(DepthPriorityGroup >= SDPG_MAX_SceneRender)
		{
			DepthPriorityGroup = SDPG_World;
		}
	}
	~FStaticMesh();

	/** Adds a link from the mesh to its entry in a draw list. */
	void LinkDrawList(FDrawListElementLink* Link);

	/** Removes a link from the mesh to its entry in a draw list. */
	void UnlinkDrawList(FDrawListElementLink* Link);

	/** Adds the static mesh to the appropriate draw lists in a scene. */
	void AddToDrawLists(FScene* Scene);

	/** Removes the static mesh from all draw lists. */
	void RemoveFromDrawLists();

private:
	/** Links to the draw lists this mesh is an element of. */
	TArray<TRefCountPtr<FDrawListElementLink> > DrawListLinks;

	/** Private copy constructor. */
	FStaticMesh(const FStaticMesh& InStaticMesh):
		FMeshElement(InStaticMesh),
		MinDrawDistanceSquared(InStaticMesh.MinDrawDistanceSquared),
		MaxDrawDistanceSquared(InStaticMesh.MaxDrawDistanceSquared),
		PrimitiveSceneInfo(InStaticMesh.PrimitiveSceneInfo),
		HitProxyId(InStaticMesh.HitProxyId),
		Id(InStaticMesh.Id)
	{}
};

/**
 * A macro to compare members of two drawing policies(A and B), and return based on the result.
 * If the members are the same, the macro continues execution rather than returning to the caller.
 */
#define COMPAREDRAWINGPOLICYMEMBERS(MemberName) \
	if(A.MemberName < B.MemberName) { return -1; } \
	else if(A.MemberName > B.MemberName) { return +1; }

/**
 * The base mesh drawing policy.  Subclasses are used to draw meshes with type-specific context variables.
 * May be used either simply as a helper to render a dynamic mesh, or as a static instance shared between
 * similar meshs.
 */
class FMeshDrawingPolicy
{
public:

	struct ElementDataType {};

	FMeshDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		UBOOL bOverrideWithShaderComplexity = FALSE);

	DWORD GetTypeHash() const
	{
		return PointerHash(VertexFactory);
	}

	UBOOL Matches(const FMeshDrawingPolicy& OtherDrawer) const
	{
		return
			VertexFactory == OtherDrawer.VertexFactory &&
			MaterialRenderProxy == OtherDrawer.MaterialRenderProxy &&
			bIsTwoSidedMaterial == OtherDrawer.bIsTwoSidedMaterial && 
			bIsWireframeMaterial == OtherDrawer.bIsWireframeMaterial;
	}

	/**
	 * Sets the render states for drawing a mesh.
	 * @param PrimitiveSceneInfo - The primitive drawing the dynamic mesh.  If this is a view element, this will be NULL.
	 */
	void SetMeshRenderState(
		FCommandContextRHI* Context,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		const ElementDataType& ElementData
		) const;

	/**
	 * Executes the draw commands for a mesh.
	 */
	virtual void DrawMesh(FCommandContextRHI* Context,const FMeshElement& Mesh) const;

	/**
	 * Executes the draw commands which can be shared between any meshes using this drawer.
	 * @param CI - The command interface to execute the draw commands on.
	 * @param View - The view of the scene being drawn.
	 */
	void DrawShared(FCommandContextRHI* Context,const FSceneView* View) const;

	/**
	* Get the decl and stream strides for this mesh policy type and vertexfactory
	* @param VertexDeclaration - output decl 
	* @param StreamStrides - output array of vertex stream strides 
	*/
	void GetVertexDeclarationInfo(FVertexDeclarationRHIParamRef &VertexDeclaration, DWORD *StreamStrides) const;

	friend INT Compare(const FMeshDrawingPolicy& A,const FMeshDrawingPolicy& B)
	{
		COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
		COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
		COMPAREDRAWINGPOLICYMEMBERS(bIsWireframeMaterial);
		return 0;
	}

	// Accessors.
	UBOOL IsTwoSided() const
	{
		return bIsTwoSidedMaterial;
	}
	UBOOL IsWireframe() const
	{
		return bIsWireframeMaterial;
	}
	UBOOL NeedsBackfacePass() const
	{
		return bNeedsBackfacePass;
	}

protected:
	const FVertexFactory* VertexFactory;
	const FMaterialRenderProxy* MaterialRenderProxy;
	const BITFIELD bIsTwoSidedMaterial : 1;
	const BITFIELD bIsWireframeMaterial : 1;
	const BITFIELD bNeedsBackfacePass : 1;
	const BITFIELD bOverrideWithShaderComplexity : 1;
};


DWORD GetTypeHash(const FBoundShaderStateRHIRef &Key);


/**
 * A set of static meshs, each associated with a mesh drawing policy of a particular type.
 * @param DrawingPolicyType - The drawing policy type used to draw mesh in this draw list.
 * @param HashSize - The number of buckets to use in the drawing policy hash.
 */
template<typename DrawingPolicyType>
class TStaticMeshDrawList
{
public:
	typedef typename DrawingPolicyType::ElementDataType ElementPolicyDataType;

private:

	/**
	 * A handle to an element in the draw list.  Used by FStaticMesh to keep track of draw lists containing the mesh.
	 */
	class FElementHandle : public FStaticMesh::FDrawListElementLink
	{
	public:

		/**
		 * Minimal initialization constructor.
		 */
		FElementHandle(void* InDrawingPolicyLink,INT InElementIndex):
			DrawingPolicyLink(InDrawingPolicyLink),
			ElementIndex(InElementIndex)
		{
		}

		// FAbstractDrawListElementLink interface.
		virtual void Remove();

	private:
		// due to gcc3 template limitations, we make this a void* and typecase it to an FDrawngPolicyLink* in 
		// the Remove function that is now defined after the template class
		void* DrawingPolicyLink;
		// @todo gcc4: if gcc4 has better templates this can go back to FDrawingPolicyLink
//		FDrawingPolicyLink* DrawingPolicyLink;
		INT ElementIndex;
	};

	/**
	 * An element in the draw list.
	 */
	struct FElement
	{
		INT MeshId;
		FStaticMesh* Mesh;

		TRefCountPtr<FElementHandle> Handle;

#if RHI_SUPPORTS_COMMAND_LISTS
		FCommandListRHI DrawCommands;
#else
		ElementPolicyDataType PolicyData;
#endif

		/** Default constructor. */
		FElement():
			MeshId(INDEX_NONE),
			Mesh(NULL)
		{}

		/** Minimal initialization constructor. */
		FElement(FStaticMesh* InMesh,const ElementPolicyDataType& InPolicyData,void* DrawingPolicyLink,INT ElementIndex):
			MeshId(InMesh->Id),
			Mesh(InMesh),
			Handle(new FElementHandle(DrawingPolicyLink,ElementIndex)),
#if !RHI_SUPPORTS_COMMAND_LISTS
			PolicyData(InPolicyData)
#else
            DrawCommands()
#endif
		{
#if RHI_SUPPORTS_COMMAND_LISTS
			// If the draw list stores command lists, build the command list for the element now.
			DrawCommands.Init();
			FCommandContextRHI* ElementRecordingContext = DrawCommands.BeginRecording(DrawCommands.GetBaseAddress());
			for(INT bBackFace = 0;bBackFace < (DrawingPolicyLink->DrawingPolicy.NeedsBackfacePass() ? 2 : 1);bBackFace++)
			{
				DrawingPolicyLink->DrawingPolicy.SetMeshRenderState(
					ElementRecordingContext,
					Mesh->PrimitiveSceneInfo,
					*Mesh,
					bBackFace,
					InPolicyData
					);
				DrawingPolicyLink->DrawingPolicy.DrawMesh(Context,*Mesh);
			}
			DrawCommands.EndRecording();
#endif
		}

		/**
		 * Destructor.
		 */
		~FElement()
		{
			if(Mesh)
			{
				Mesh->UnlinkDrawList(Handle);
			}
		}
	};

	/**
	 * A set of draw list elements with the same drawing policy.
	 */
	struct FDrawingPolicyLink
	{
		TStaticMeshDrawList* DrawList;
		DrawingPolicyType DrawingPolicy;
		TSparseArray<FElement> Elements;

		FBoundShaderStateRHIRef BoundShaderState;

		FDrawingPolicyLink(TStaticMeshDrawList* InDrawList,const DrawingPolicyType& InDrawingPolicy):
			DrawList(InDrawList),
			DrawingPolicy(InDrawingPolicy)
		{
			BoundShaderState = DrawingPolicy.CreateBoundShaderState();
		}
	};

	/**
	 * Functions to extract the drawing policy from FDrawingPolicyLink as a key for THashSet.
	 */
	struct FDrawingPolicyKeyFuncs
	{
		typedef DrawingPolicyType KeyType;

		static const DrawingPolicyType& GetSetKey(const FDrawingPolicyLink& Link)
		{
			return Link.DrawingPolicy;
		}

		static UBOOL Matches(const DrawingPolicyType& A,const DrawingPolicyType& B)
		{
			return A.Matches(B);
		}

		static DWORD GetTypeHash(const DrawingPolicyType& DrawingPolicy)
		{
			return DrawingPolicy.GetTypeHash();
		}
	};


	/**
	* Submits the draw calls.  Used by Draw() after the mesh has been determined to be visible
	* @param Context - The Context to the execute the draw commands in.
	* @param View - The view of the meshes to render.
	* @param Element - The mesh element
	* @param DrawingPolicyLink - the drawing policy link
	* @param bDrawnShared - determines whether to draw shared 
	*/

	void SubmitDrawCall(FCommandContextRHI* Context, const FSceneView* View, const FElement& Element, const FDrawingPolicyLink* DrawingPolicyLink, UBOOL &bDrawnShared) const
	{
		if(!bDrawnShared)
		{
			DrawingPolicyLink->DrawingPolicy.DrawShared(Context,View,DrawingPolicyLink->BoundShaderState);
			bDrawnShared = TRUE;
		}

#if RHI_SUPPORTS_COMMAND_LISTS
		RHICommandListCall(Context,Element.DrawCommands,Element.DrawCommands.GetBaseAddress());
#else
		for(INT bBackFace = 0;bBackFace < (DrawingPolicyLink->DrawingPolicy.NeedsBackfacePass() ? 2 : 1);bBackFace++)
		{
			DrawingPolicyLink->DrawingPolicy.SetMeshRenderState(
				Context,
				Element.Mesh->PrimitiveSceneInfo,
				*Element.Mesh,
				bBackFace,
				Element.PolicyData
				);
			DrawingPolicyLink->DrawingPolicy.DrawMesh(Context,*Element.Mesh);
		}
#endif
	}

public:

	/**
	 * Adds a mesh to the draw list.
	 * @param Mesh - The mesh to add.
	 * @param PolicyData - The drawing policy data for the mesh.
	 * @param InDrawingPolicy - The drawing policy to use to draw the mesh.
	 */
	void AddMesh(
		FStaticMesh* Mesh,
		const ElementPolicyDataType& PolicyData,
		const DrawingPolicyType& InDrawingPolicy
		)
	{
		FDrawingPolicyLink* DrawingPolicyLink = DrawingPolicySet.Find(InDrawingPolicy);

		if(!DrawingPolicyLink)
		{
			DrawingPolicyLink = DrawingPolicySet.Add(FDrawingPolicyLink(this,InDrawingPolicy));

			// Insert the drawing policy into the ordered drawing policy list.
			INT MinIndex = 0;
			INT MaxIndex = OrderedDrawingPolicies.Num() - 1;
			while(MinIndex < MaxIndex)
			{
				INT PivotIndex = (MaxIndex + MinIndex) / 2;
				INT CompareResult = Compare(OrderedDrawingPolicies(PivotIndex)->DrawingPolicy,DrawingPolicyLink->DrawingPolicy);
				if(CompareResult < 0)
				{
					MinIndex = PivotIndex + 1;
				}
				else if(CompareResult > 0)
				{
					MaxIndex = PivotIndex;
				}
				else
				{
					MinIndex = MaxIndex = PivotIndex;
				}
			};
			check(MinIndex >= MaxIndex);
			OrderedDrawingPolicies.InsertItem(DrawingPolicyLink,MinIndex);
		}

		TSparseArrayAllocationInfo<FElement> ElementAllocation = DrawingPolicyLink->Elements.Add();
		FElement* Element = new(ElementAllocation) FElement(Mesh, PolicyData, DrawingPolicyLink, ElementAllocation.Index);

		Mesh->LinkDrawList(Element->Handle);
	}

	/**
	 * Draws all elements of the draw list.
	 * @param CI - The command interface to the execute the draw commands on.
	 * @param View - The view of the meshes to render.
	 * @return True if any static meshes were drawn.
	 */
	UBOOL DrawAll(FCommandContextRHI* Context, const FSceneView* View) const
	{
		UBOOL bDirty = FALSE;
		for(INT PolicyIndex = 0;PolicyIndex < OrderedDrawingPolicies.Num();PolicyIndex++)
		{
			const FDrawingPolicyLink* DrawingPolicyLink = OrderedDrawingPolicies(PolicyIndex);
			UBOOL bDrawnShared = FALSE;

			for(typename TSparseArray<FElement>::TConstIterator ElementIt(DrawingPolicyLink->Elements); ElementIt; ++ElementIt)
			{
				const FElement& Element = *ElementIt;
				FLOAT DistanceSquared = 0.0f;
				if(View->ViewOrigin.W > 0.0f)
				{
					DistanceSquared = (Element.Mesh->PrimitiveSceneInfo->Bounds.Origin - View->ViewOrigin).SizeSquared();
				}

				//cull the mesh if it is not in range
				if(DistanceSquared >= Element.Mesh->MinDrawDistanceSquared && DistanceSquared < Element.Mesh->MaxDrawDistanceSquared)
				{
					SubmitDrawCall(Context, View, Element, DrawingPolicyLink, bDrawnShared);
					bDirty = TRUE;
				}
			}
		}
		return bDirty;
	}

	/**
	 * Draws only the static meshes which are in the visibility map.
	 * @param CI - The command interface to the execute the draw commands on.
	 * @param View - The view of the meshes to render.
	 * @param StaticMeshVisibilityMap - An map from FStaticMesh::Id to visibility state.
	 * @return True if any static meshes were drawn.
	 */
	UBOOL DrawVisible(FCommandContextRHI* Context, const FSceneView* View, const FBitArray& StaticMeshVisibilityMap) const
	{
		UBOOL bDirty = FALSE;
		for(INT PolicyIndex = 0;PolicyIndex < OrderedDrawingPolicies.Num();PolicyIndex++)
		{
			const FDrawingPolicyLink* DrawingPolicyLink = OrderedDrawingPolicies(PolicyIndex);
			UBOOL bDrawnShared = FALSE;

			for(typename TSparseArray<FElement>::TConstIterator ElementIt(DrawingPolicyLink->Elements); ElementIt; ++ElementIt)
			{
				const FElement& Element = *ElementIt;

				// Check if the element is visible.
				if(StaticMeshVisibilityMap(Element.MeshId))
				{
					INC_DWORD_STAT_BY(STAT_StaticMeshTriangles,Element.Mesh->NumPrimitives);
					SubmitDrawCall(Context, View, Element, DrawingPolicyLink, bDrawnShared);
					bDirty = TRUE;
				}
			}
		}
		return bDirty;
	}

	/**
	 * @return total number of meshes in all draw policies
	 */
	INT NumMeshes() const
	{
		INT TotalMeshes=0;
		for(INT PolicyIndex = 0;PolicyIndex < OrderedDrawingPolicies.Num();PolicyIndex++)
		{
			TotalMeshes += OrderedDrawingPolicies(PolicyIndex)->Elements.Num();
		}
		return TotalMeshes;
	}

private:
	/** All drawing policies in the draw list, in rendering order. */
    TArray<FDrawingPolicyLink*> OrderedDrawingPolicies;

	/** All drawing policy element sets in the draw list, hashed by drawing policy. */
	THashSet<FDrawingPolicyLink,FDrawingPolicyKeyFuncs> DrawingPolicySet;
};

// Moved this from the template above because of circular references that gcc3 was choking on
template<typename DrawingPolicyType>
void TStaticMeshDrawList<DrawingPolicyType>::FElementHandle::Remove()
{
	// Make a copy of the drawing policy link pointer on the stack, since we're about to delete this.
	FDrawingPolicyLink* LocalDrawingPolicyLink = (FDrawingPolicyLink*)DrawingPolicyLink;

	// Unlink the mesh from this draw list.
	LocalDrawingPolicyLink->Elements(ElementIndex).Mesh->UnlinkDrawList(this);
	LocalDrawingPolicyLink->Elements(ElementIndex).Mesh = NULL;

	// Remove this element from the drawing policy's element list.
	LocalDrawingPolicyLink->Elements.Remove(ElementIndex);

	if(!LocalDrawingPolicyLink->Elements.Num())
	{
		// Make a copy of the draw list pointer on the stack, since we're about to delete LocalDrawingPolicyLink.
		TStaticMeshDrawList* LocalDrawList = LocalDrawingPolicyLink->DrawList;

		// If this was the last element for the drawing policy, remove the drawing policy from the draw list.
		LocalDrawList->DrawingPolicySet.Remove(LocalDrawingPolicyLink);
		LocalDrawList->OrderedDrawingPolicies.RemoveItem(LocalDrawingPolicyLink);
	}
}

/** An interface to the information about a light's effect on a scene's DPG. */
class FLightSceneDPGInfoInterface
{
public:

	virtual UBOOL DrawStaticMeshes(
		FCommandContextRHI* Context,
		const FSceneView* View,
		const FBitArray& StaticMeshVisibilityMap
		) const = 0;

	virtual ELightInteractionType AttachStaticMesh(const FLightSceneInfo* LightSceneInfo,FStaticMesh* Mesh) = 0;
	
	virtual UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FSceneView* View,
		const FLightSceneInfo* LightSceneInfo,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId
		) const = 0;
};

/**
 * The information used to render a light.  This is the rendering thread's mirror of the game thread's ULightComponent.
 */
class FLightSceneInfo
{
public:
    /** The light component. */
    const ULightComponent* LightComponent;

	/** The light's persistent shadowing GUID. */
	const FGuid LightGuid;

	/** The light's persistent lighting GUID. */
	const FGuid LightmapGuid;

	/** A transform from world space into light space. */
	const FMatrix WorldToLight;

	/** A transform from light space into world space. */
	const FMatrix LightToWorld;

	/** The homogenous position of the light. */
	const FVector4 Position;

	/** The light color. */
	FLinearColor Color;

	/** The light channels which this light affects. */
	const FLightingChannelContainer LightingChannels;

	/** The list of static primitives affected by the light. */
	FLightPrimitiveInteraction* StaticPrimitiveList;

	/** The list of dynamic primitives affected by the light. */
	FLightPrimitiveInteraction* DynamicPrimitiveList;

	/** Number of dynamic primitive interactions that use shadow volumes */
	UINT NumShadowVolumeInteractions;

	/** The index of the primitive in Scene->Lights. */
	INT Id;

	/** Light function parameters. */
	FVector	LightFunctionScale;
	const FMaterialRenderProxy* LightFunction;
	/** Bound shader state for this light's light function. This is mutable because it is cached on first use, possibly when const */
	mutable FBoundShaderStateRHIRef LightFunctionBoundShaderState; 

	/** True if the light will cast projected shadows from dynamic primitives. */
	const BITFIELD bProjectedShadows : 1;

	/** True if primitives will cache static lighting for the light. */
	const BITFIELD bStaticLighting : 1;

	/** True if primitives will cache static shadowing for the light. */
	const BITFIELD bStaticShadowing : 1;

	/** True if the light casts dynamic shadows. */
	const BITFIELD bCastDynamicShadow : 1;
	
	/** True if the light casts static shadows. */
	const BITFIELD bCastStaticShadow : 1;

	/** Whether to only affect primitives that are in the same level/ share the same  GetOutermost() or are in the set of additionally specified ones. */
	const BITFIELD bOnlyAffectSameAndSpecifiedLevels : 1;

	/** True if the light's exclusion and inclusion volumes should be used to determine primitive relevance. */
	const BITFIELD bUseVolumes : 1;

	/** Whether the light affects primitives in the default light environment. */
	const BITFIELD bAffectsDefaultLightEnvironment : 1;

	/** The light type (ELightComponentType) */
	const BYTE LightType;

	/** Type of shadowing to apply for the light (ELightShadowMode) */
	const BYTE LightShadowMode;

	/** Type of shadow projection to use for this light */
	const BYTE ShadowProjectionTechnique;

	/** Quality of shadow buffer filtering to use for this light's shadows */
	const BYTE ShadowFilterQuality;

	/** 
	* override for min dimensions (in texels) allowed for rendering shadow subject depths.
	* 0 defaults to Engine.MinShadowResolution
	*/
	const INT MinShadowResolution;
	
	/** 
	* override for max square dimensions (in texels) allowed for rendering shadow subject depths 
	* 0 defaults to Engine.MaxShadowResolution
	*/
	const INT MaxShadowResolution;

	/** The name of the level the light is in. */
	FName LevelName;

	/** Array of other levels to affect if bOnlyAffectSameAndSpecifiedLevels is TRUE, own level always implicitly part of array. */
	TArray<FName> OtherLevelsToAffect;

	/** The light's exclusion volumes. */
	TArray<FConvexVolume> ExclusionConvexVolumes;

	/** The light's inclusion volumes. */
	TArray<FConvexVolume> InclusionConvexVolumes;

	/** Shadow color for modulating entire scene */
	const FLinearColor ModShadowColor;

	/** Time since the subject was last visible at which the mod shadow will fade out completely.  */
	FLOAT ModShadowFadeoutTime;

	/** Exponent that controls mod shadow fadeout curve. */
	FLOAT ModShadowFadeoutExponent;

	/** The light environments that the light affects. */
	TArray<FLightEnvironmentSceneInfo*> LightEnvironments;

#if !FINAL_RELEASE
	FString LightComponentStr;
	const TCHAR* GetLightName() const { return *LightComponentStr; }
#else
	const TCHAR* GetLightName() const { return TEXT(""); }
#endif

	// Accessors.
	FVector GetDirection() const { return FVector(WorldToLight.M[0][2],WorldToLight.M[1][2],WorldToLight.M[2][2]); }
	FVector GetOrigin() const { return LightToWorld.GetOrigin(); }
	FVector4 GetPosition() const { return Position; }

	/** @return radius of the light or 0 if no radius */
	virtual FLOAT GetRadius() const { return 0.0f; }
	
	/** Initialization constructor. */
	FLightSceneInfo(const ULightComponent* InLight);
	virtual ~FLightSceneInfo() {}

	/** Detaches the light from the scene. */
	void Detach();

	/**
	 * Tests whether this light affects a specific light environment's primitives.
	 * @param LightEnvironmentSceneInfo - The scene info for the light environment to test.  NULL indicates the default light environment.
	 * @return TRUE if the light may affect the light environment's primitives.
	 */
	UBOOL AffectsLightEnvironment(const FLightEnvironmentSceneInfo* LightEnvironmentSceneInfo) const;

	/**
	 * Tests whether this light affects the given primitive.  This checks both the primitive and light settings for light relevance
	 * and also calls AffectsBounds.
	 * @param CompactPrimitiveSceneInfo - The primitive to test.
	 * @return True if the light affects the primitive.
	 */
	UBOOL AffectsPrimitive(const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo) const;

	/**
	 * Tests whether the light affects the given bounding volume.
	 * @param Bounds - The bounding volume to test.
	 * @return True if the light affects the bounding volume
	 */
	virtual UBOOL AffectsBounds(const FBoxSphereBounds& Bounds) const
	{
		return TRUE;
	}

	virtual void DetachPrimitive(const FLightPrimitiveInteraction& Interaction) {}
	virtual void AttachPrimitive(const FLightPrimitiveInteraction& Interaction) {}

	/**
	 * Sets up a projected shadow initializer for the given subject.
	 * @param SubjectBoundingSphere - The bounding sphere of the subject.
	 * @param OutInitializer - Upon successful return, contains the initialization parameters for the shadow.
	 * @return True if a projected shadow should be cast by this subject-light pair.
	 */
	virtual UBOOL GetProjectedShadowInitializer(const FSphere& SubjectBoundingSphere,class FProjectedShadowInitializer& OutInitializer) const
	{
		return FALSE;
	}

	virtual void SetScissorRect(FCommandContextRHI* Context,const FSceneView* View) const
	{
	}

	/**
	 * Returns a pointer to the light type's DPG info object for the given DPG.
	 * @param DPGIndex - The index of the DPG to get the info object for.
	 * @return The DPG info interface.
	 */
	virtual const FLightSceneDPGInfoInterface* GetDPGInfo(UINT DPGIndex) const = 0;
	virtual FLightSceneDPGInfoInterface* GetDPGInfo(UINT DPGIndex) = 0;

	/**
	* @return modulated shadow projection pixel shader for this light type
	*/
	virtual class FShadowProjectionPixelShaderInterface* GetModShadowProjPixelShader() const = 0;
	
	/**
	* @return Branching PCF modulated shadow projection pixel shader for this light type
	*/
	virtual class FBranchingPCFProjectionPixelShaderInterface* GetBranchingPCFModProjPixelShader() const = 0;

	/**
	* @return modulated shadow projection pixel shader for this light type
	*/
	virtual class FModShadowVolumePixelShader* GetModShadowVolumeShader() const = 0;

	/**
	* @return modulated shadow projection bound shader state for this light type
	*/
	virtual FGlobalBoundShaderStateRHIRef* GetModShadowProjBoundShaderState() const = 0;
	/** Bound shader state for this light's modulated shadow projection. This is mutable because it is cached on first use, possibly when const */
	mutable FGlobalBoundShaderStateRHIRef ModShadowProjBoundShaderState;

#if SUPPORTS_VSM
	/**
	* @return VSM modulated shadow projection pixel shader for this light type
	*/
	virtual class FVSMModProjectionPixelShader* GetVSMModProjPixelShader() const = 0;
	/**
	* @return VSM modulated shadow projection bound shader state for this light type
	*/
	virtual FBoundShaderStateRHIParamRef GetVSMModProjBoundShaderState() const = 0;
	/** Bound shader state for this light's VSM modulated shadow projection. This is mutable because it is cached on first use, possibly when const */
	mutable FBoundShaderStateRHIRef VSMModProjBoundShaderState;
#endif //#if SUPPORTS_VSM

	/**
	* @return PCF Branching modulated shadow projection bound shader state for this light type
	*/
	virtual FGlobalBoundShaderStateRHIRef* GetBranchingPCFModProjBoundShaderState() const = 0;
	/** Bound shader state for this light's PCF Branching modulated shadow projection. This is mutable because it is cached on first use, possibly when const */
	mutable FGlobalBoundShaderStateRHIRef ModBranchingPCFLowQualityBoundShaderState;
	mutable FGlobalBoundShaderStateRHIRef ModBranchingPCFMediumQualityBoundShaderState;
	mutable FGlobalBoundShaderStateRHIRef ModBranchingPCFHighQualityBoundShaderState;

	/**
	* @return modulated shadow projection bound shader state for this light type
	*/
	virtual FBoundShaderStateRHIParamRef GetModShadowVolumeBoundShaderState() const = 0;
	/** Bound shader state for this light's light modulated shadow volume. This is mutable because it is cached on first use, possibly when const */
	mutable FBoundShaderStateRHIRef ModShadowVolumeBoundShaderState;

	/** Hash function. */
	friend DWORD GetTypeHash(const FLightSceneInfo* LightSceneInfo)
	{
		return (DWORD)LightSceneInfo->Id;
	}
};

/**
 * A drawing policy factory for the lighting drawing policy.
 */
class FMeshLightingDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = FALSE };
	typedef const FLightSceneInfo* ContextType;

	static ELightInteractionType AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh,FLightSceneInfo* Light);
	static UBOOL DrawDynamicMesh(
		FCommandContextRHI* Context,
		const FSceneView* View,
		const FLightSceneInfo* Light,
		const FMeshElement& Mesh,
		UBOOL bBackFace,
		UBOOL bPreFog,
		const FPrimitiveSceneInfo* PrimitiveSceneInfo,
		FHitProxyId HitProxyId
		);
	static UBOOL IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
	{
		return MaterialRenderProxy && MaterialRenderProxy->GetMaterial()->GetLightingModel() != MLM_Unlit;
	}
};

/** The properties of a height fog layer which are used for rendering. */
class FHeightFogSceneInfo
{
public:

	/** The fog component the scene info is for. */
	const UHeightFogComponent* Component;
	/** z-height for the fog plane - updated by the owning actor */
	FLOAT Height;
	/** affects the scale for the fog layer's thickness */
	FLOAT Density;
	/** Fog color to blend with the scene */
	FLinearColor LightColor;
	/** The distance at which light passing through the fog is 100% extinguished. */
	FLOAT ExtinctionDistance;
	/** distance at which fog starts affecting the scene */
	FLOAT StartDistance;

	/** Initialization constructor. */
	FHeightFogSceneInfo(const UHeightFogComponent* InComponent);
};

IMPLEMENT_COMPARE_CONSTREF(FHeightFogSceneInfo,SceneCore,{ return A.Height < B.Height ? +1 : (A.Height > B.Height ? -1 : 0); });

/**
 * The scene rendering stats.
 */
enum ESceneRenderingStats
{
	STAT_OcclusionQueryTime = STAT_SceneRenderingFirstStat,
	STAT_InitViewsTime,
	STAT_DynamicShadowSetupTime,
	STAT_TranslucencySetupTime,
	STAT_TotalGPUFrameTime,
	STAT_TotalSceneRenderingTime,

	STAT_DepthDrawTime,
	STAT_BasePassDrawTime,
	STAT_LightingDrawTime,
	STAT_ProjectedShadowDrawTime,
	STAT_ModulatedShadowDrawTime,
	STAT_LightFunctionDrawTime,
	STAT_ShadowVolumeDrawTime,
	STAT_TranslucencyDrawTime,

	STAT_ProjectedShadows,
	STAT_CulledPrimitives,
	STAT_OccludedPrimitives,
	STAT_OcclusionQueries,
	STAT_VisibleStaticMeshElements,
	STAT_VisibleDynamicPrimitives,
	STAT_DrawEvents,
	STAT_SceneLights,
};
