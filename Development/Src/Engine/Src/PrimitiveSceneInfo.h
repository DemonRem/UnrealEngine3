/*=============================================================================
	PrimitiveSceneInfo.h: Primitive scene info definitions.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __PRIMITIVESCENEINFO_H__
#define __PRIMITIVESCENEINFO_H__

#include "GenericOctree.h"

/** The information needed to determine whether a primitive is visible. */
class FPrimitiveSceneInfoCompact
{
public:

	FPrimitiveSceneInfo* PrimitiveSceneInfo;
	FPrimitiveSceneProxy* Proxy;
	UPrimitiveComponent* Component;
	ULightEnvironmentComponent* LightEnvironment;
	FBoxSphereBounds Bounds;
	FLOAT MinDrawDistanceSquared;
	FLOAT MaxDrawDistanceSquared;
	FLightingChannelContainer LightingChannels;

#if USE_MASSIVE_LOD
	// the primitive that will replace this one. NEVER dereference this pointer, it's only for looking up in the prim->replacement info map
	UPrimitiveComponent* ReplacementPrimitiveMapKey;

	/** The square of the distance that at which MassiveLOD switches between high and low LOD */
	FLOAT MassiveLODDistanceSquared;
#endif

	/** Used for precomputed visibility */
	INT VisibilityId;

	BITFIELD bAllowApproximateOcclusion : 1;
	BITFIELD bFirstFrameOcclusion : 1;
	BITFIELD bAcceptsLights : 1;
	BITFIELD bHasViewDependentDPG : 1;
	BITFIELD bShouldCullModulatedShadows : 1;
	BITFIELD bCastDynamicShadow : 1;
	BITFIELD bLightEnvironmentForceNonCompositeDynamicLights : 1;
	BITFIELD bIgnoreNearPlaneIntersection : 1;
	BITFIELD StaticDepthPriorityGroup : UCONST_SDPG_NumBits;
	BITFIELD bHasCustomOcclusionBounds : 1;

#if USE_MASSIVE_LOD
	/** Has this primitive been added to a parent? Only used for descendency purposes */
	STAT(BITFIELD bHasBeenAdded : 1);
	/** List of primitives that are children of this one (this is their LOD parent). Must be indirect so we can keep pointers to the items (FPathToCompact) */
	TArray<FPrimitiveSceneInfoCompact*> ChildPrimitives;
	/** Tracks the number of children, grandchildren, etc */
	STAT(INT NumberOfDescendents);

	/**
	 * Adds the given primitive to our list of children
	 */
	inline void AddChildPrimitive(FPrimitiveSceneInfoCompact* Child)
	{
		// add the child to the list
		ChildPrimitives.AddItem(Child);
		STAT(UpdateDescendentCounts(Child, TRUE));
	}

	/**
	 * Removes the given primitive from our list of children
	 */
	inline void RemoveChildPrimitive(FPrimitiveSceneInfoCompact* Child)
	{
		INT NumRemoved = ChildPrimitives.RemoveItem(Child);
#if STATS
		if (NumRemoved > 0)
		{
			UpdateDescendentCounts(Child, FALSE);
		}
#endif
	}

	/**
	 * Finds any pending children for the given primitive, and adds them as children to us
	 *
	 * @param PrimitiveKey The key used to find our children in the pending child primitive map
	 */
	void AddPendingChildren(UPrimitiveComponent* PrimitiveKey);

#if STATS
	/**
	 * Updates the descendent counts of compact infos, going up the hierarchy
	 */
	void UpdateDescendentCounts(FPrimitiveSceneInfoCompact* Child, UBOOL bChildWasAdded);
#endif

#endif // USE_MASSIVE_LOD

	/** Initializes the compact scene info from the primitive's full scene info. */
	void Init(FPrimitiveSceneInfo* InPrimitiveSceneInfo);

	/** Default constructor. */
	FPrimitiveSceneInfoCompact():
		PrimitiveSceneInfo(NULL),
		Proxy(NULL),
		Component(NULL),
		LightEnvironment(NULL)
	{}

	/** Initialization constructor. */
	FPrimitiveSceneInfoCompact(FPrimitiveSceneInfo* InPrimitiveSceneInfo)
	{
		Init(InPrimitiveSceneInfo);
	}
};

/** The type of the octree used by FScene to find primitives. */
typedef TOctree<FPrimitiveSceneInfoCompact,struct FPrimitiveOctreeSemantics> FScenePrimitiveOctree;



#if USE_MASSIVE_LOD
/**
* Representation of how to get a FPSICompact - either via OctreeId (because it's in the octree)
* or as a direct pointer to a FPSICompact (because it's a child of an octree entry)
*/
struct FPathToCompact
{
	/** ID to get to the root (parentmost) octree element */
	FOctreeElementId OctreeId;

	class FPrimitiveSceneInfoCompact* DirectReference;

	/** Construct with a direct reference */
	FPathToCompact(class FPrimitiveSceneInfoCompact* InDirectReference)
		: DirectReference(InDirectReference)
	{
	}

	/** Construct with an octree ID*/
	FPathToCompact(const FOctreeElementId& InOctreeId)
		: OctreeId(InOctreeId)
		, DirectReference(NULL)
	{
	}

	/**
	* Retrieves the compact info represented by the path
	*/
	class FPrimitiveSceneInfoCompact& GetCompact(FScenePrimitiveOctree& Octree);
};

#endif

/**
 * The information used to a render a primitive.  This is the rendering thread's mirror of the game thread's UPrimitiveComponent.
 */
class FPrimitiveSceneInfo : public FDeferredCleanupInterface
{
public:

	/** The render proxy for the primitive. */
	FPrimitiveSceneProxy* Proxy;

	/** The UPrimitiveComponent this scene info is for. */
	UPrimitiveComponent* Component;

	/** The actor which owns the PrimitiveComponent. */
	AActor* Owner;

	/** The primitive's static meshes. */
	TIndirectArray<class FStaticMesh> StaticMeshes;

	/** The index of the primitive in Scene->Primitives. */
	INT Id;

	/** The identifier for the primitive in Scene->PrimitiveOctree. */
	FOctreeElementId OctreeId;

	/** The translucency sort priority */
	SWORD TranslucencySortPriority;

	/** The number of dominant lights affecting this primitive. */
	WORD NumAffectingDominantLights;

	/** Used for precomputed visibility */
	INT VisibilityId;

	/** True if the primitive will cache static shadowing. */
	BITFIELD bStaticShadowing : 1;

	/** True if the primitive casts dynamic shadows. */
	BITFIELD bCastDynamicShadow : 1;
	
	/** True if the primitive only self shadows and does not cast shadows on other primitives. */
	BITFIELD bSelfShadowOnly : 1;

	/** 
	 * Optimization for objects which don't need to receive dynamic dominant light shadows. 
	 * This is useful for objects which eat up a lot of GPU time and are heavily texture bound yet never receive noticeable shadows from dominant lights like trees.
	 */
	BITFIELD bAcceptsDynamicDominantLightShadows : 1;

	/** True if the primitive casts static shadows. */
	BITFIELD bCastStaticShadow : 1;

	/** True if the primitive casts shadows even when hidden. */
	BITFIELD bCastHiddenShadow : 1;

	/** Whether this primitive should cast dynamic shadows as if it were a two sided material. */
	BITFIELD bCastShadowAsTwoSided : 1;

	/** Whether to allow a preshadow (the static environment casting dynamic shadows on dynamic objects) onto this object. */
	BITFIELD bAllowPreShadow : 1;

	/** True if the primitive receives lighting. */
	BITFIELD bAcceptsLights : 1;

	/** True if the primitive should be affected by dynamic lights. */
	BITFIELD bAcceptsDynamicLights : 1;

	/** 
	 * If TRUE, lit translucency using this light environment will render in one pass, 
	 * Which is cheaper and ensures correct blending but approximates lighting using one directional light and all other lights in an unshadowed SH environment.
	 * If FALSE, lit translucency will render in multiple passes which uses more shader instructions and results in incorrect blending.
	 * Both settings still work correctly with bAllowDynamicShadowsOnTranslucency.
	 */
	BITFIELD bUseOnePassLightingOnTranslucency : 1;

	/** True if the primitive should only be affected by lights in the same level. */
	BITFIELD bSelfContainedLighting : 1;

	/** If this is True, this primitive will be used to occlusion cull other primitives. */
	BITFIELD bUseAsOccluder:1;

	/** If this is True, this primitive doesn't need exact occlusion info. */
	BITFIELD bAllowApproximateOcclusion : 1;

	/** If this is True, the component will return 'occluded' for the first frame. */
	BITFIELD bFirstFrameOcclusion:1;

	/** Whether to ignore the near plane intersection test before occlusion querying. */
	BITFIELD bIgnoreNearPlaneIntersection : 1;

	/** If this is True, this primitive can be selected in the editor. */
	BITFIELD bSelectable : 1;

	/** If this is TRUE, this primitive's static meshes need to be updated before it can be rendered. */
	BITFIELD bNeedsStaticMeshUpdate : 1;

	/** 
	* If TRUE, the primitive backfaces won't allow for modulated shadows to be cast on them. 
	* If FALSE, could help performance since the mesh doesn't have to be drawn again to cull the backface shadows 
	*/
	BITFIELD	bCullModulatedShadowOnBackfaces:1;
	/** 
	* If TRUE, the emissive areas of the primitive won't allow for modulated shadows to be cast on them. 
	* If FALSE, could help performance since the mesh doesn't have to be drawn again to cull the emissive areas in shadow
	*/
	BITFIELD	bCullModulatedShadowOnEmissive:1;

	/**
	* Controls whether ambient occlusion should be allowed on or from this primitive, only has an effect on movable primitives.
	* Note that setting this flag to FALSE will negatively impact performance.
	*/
	BITFIELD	bAllowAmbientOcclusion:1;

	/** Whether to render SHLightSceneInfo in the BasePass instead of as a separate pass after modulated shadows. */
	BITFIELD	bRenderSHLightInBasePass : 1;

	/** Whether motionblur is enabled for this primitive or not. */
	BITFIELD	bEnableMotionBlur : 1;

	/** The value of the Component->LightEnvironment->bForceNonCompositeDynamicLights; or TRUE if it has no light environment. */
	BITFIELD	bLightEnvironmentForceNonCompositeDynamicLights : 1;

	/** If TRUE, the primitive has a custom occlusion bounds that should be used */
	BITFIELD	bHasCustomOcclusionBounds : 1;

	/** Determines whether or not we allow shadowing fading.  Some objects (especially in cinematics) having the shadow fade/pop out looks really bad. **/
	BITFIELD bAllowShadowFade : 1;

	/** Whether any dominant light is allowed to affect the primitive. */
	BITFIELD bAllowDominantLightInfluence : 1;

	/** 
	 * Whether lit translucency is allowed to receive dynamic shadows from the static environment.
	 * When FALSE, bTranslucencyShadowed will be used for cheaper on/off shadowing.
	 */
	BITFIELD bAllowDynamicShadowsOnTranslucency : 1;

	/** Contains the shadow factor used on translucency when bAllowDynamicShadowsOnTranslucency is FALSE. */
	BITFIELD bTranslucencyShadowed : 1;

	/** Environment shadow factor used when previewing unbuilt lighting on this primitive. */
	BYTE PreviewEnvironmentShadowing;

	/** The primitive's bounds. */
	FBoxSphereBounds Bounds;

	/** The primitive's cull distance. */
	FLOAT MaxDrawDistance;

	/** The primitive's minimum cull distance. */
	FLOAT MinDrawDistance;

#if USE_MASSIVE_LOD
	/** The primitive's MassiveLOD high/low swap distance. */
	FLOAT MassiveLODDistance;
#endif

	/** The hit proxies used by the primitive. */
	TArray<TRefCountPtr<HHitProxy> > HitProxies;

	/** The ID of the hit proxy which is used to represent the primitive's dynamic elements. */
	FHitProxyId DefaultDynamicHitProxyId;

	/** The light channels which this primitive is affected by. */
	const FLightingChannelContainer LightingChannels;

	/** The light environment which the primitive is in. */
	ULightEnvironmentComponent* LightEnvironment;

	/** The single dominant light that is allowed to affect this primitive, if any.  This is ignored if bAllowDominantLightInfluence == FALSE. */
	const ULightComponent* AffectingDominantLight;

	/** If specified, only OverrideLightComponent can affect the primitive. */
	const ULightComponent* OverrideLightComponent;

	/** The name of the level the primitive is in. */
	FName LevelName;

	/** The list of lights affecting this primitive. */
	class FLightPrimitiveInteraction* LightList;

	/** The aggregate light color of the upper sky light hemispheres affecting this primitive. */
	FLinearColor UpperSkyLightColor;

	/** The aggregate light color of the lower sky light hemispheres affecting this primitive. */
	FLinearColor LowerSkyLightColor;

	/** The dynamic light to be rendered in the base pass, if any. */
	const class FLightSceneInfo* DynamicLightSceneInfo;

	/** The spherical harmonic light to be rendered in the base pass, if any. */
	const class FLightSceneInfo* SHLightSceneInfo;

	/** Shadowing factor applied to BrightestDominantLightSceneInfo. */
	FLOAT DominantShadowFactor;

	/** The brightest dominant light affecting this primitive. */
	const class FLightSceneInfo* BrightestDominantLightSceneInfo;

	/** A primitive which this primitive is grouped with for projected shadows. */
	UPrimitiveComponent* ShadowParent;

	/** The fog volume this primitive is intersecting with. */
	class FFogVolumeDensitySceneInfo* FogVolumeSceneInfo;

	/** Last render time in seconds since level started play. */
	FLOAT LastRenderTime;

	/** Last time that the primitive became visible in seconds since level started play. */
	FLOAT LastVisibilityChangeTime;

	/** The scene the primitive is in. */
	FScene* Scene;

	// the primitive that will replace this one. NEVER dereference this pointer, it's only for looking up in the prim->replacement info map
	UPrimitiveComponent* ReplacementPrimitiveMapKey;

#if USE_MASSIVE_LOD
	/** The min draw distance of the replacement primitive, or -1.0 if the replacement component wasn't attached yet */
	FLOAT ReplacementPrimitiveMinDrawDistance;

	/** Global map of primitive components to their Compact primitive info to set up child relationships */
	static TMap<UPrimitiveComponent*, FPathToCompact> PrimitiveToCompactMap;

	/** If the children primitives are attached before the parent, they have no compact info to attach to, so store it here temporarily */
	static TMultiMap<UPrimitiveComponent*, FPrimitiveSceneInfoCompact*> PendingChildPrimitiveMap;

#endif // USE_MASSIVE_LOD



	/** Initialization constructor. */
	FPrimitiveSceneInfo(UPrimitiveComponent* InPrimitive,FPrimitiveSceneProxy* InProxy,FScene* InScene);

	/** Destructor. */
	~FPrimitiveSceneInfo();

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
	UINT GetMemoryFootprint();

	/** Determines whether the primitive has dynamic sky lighting. */
	UBOOL HasDynamicSkyLighting() const
	{
		return (!UpperSkyLightColor.Equals(FLinearColor::Black) || !LowerSkyLightColor.Equals(FLinearColor::Black));
	}

	/** Returns the scene's preview skylight color */
	FLinearColor GetPreviewSkyLightColor() const;
};

/** Defines how the primitive is stored in the scene's primitive octree. */
struct FPrimitiveOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static const FBoxSphereBounds& GetBoundingBox(const FPrimitiveSceneInfoCompact& PrimitiveSceneInfoCompact)
	{
		return PrimitiveSceneInfoCompact.Bounds;
	}

	FORCEINLINE static UBOOL AreElementsEqual(const FPrimitiveSceneInfoCompact& A,const FPrimitiveSceneInfoCompact& B)
	{
		return A.PrimitiveSceneInfo == B.PrimitiveSceneInfo;
	}

	FORCEINLINE static void SetElementId(const FPrimitiveSceneInfoCompact& Element,FOctreeElementId Id)
	{
		Element.PrimitiveSceneInfo->OctreeId = Id;

#if USE_MASSIVE_LOD
		// update the primitive to compact map with the new id
		FPrimitiveSceneInfo::PrimitiveToCompactMap.Set(Element.PrimitiveSceneInfo->Component, FPathToCompact(Id));
#endif
	}
};

#endif
