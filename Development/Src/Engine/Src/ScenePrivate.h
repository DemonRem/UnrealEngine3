/*=============================================================================
	ScenePrivate.h: Private scene manager definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __SCENEPRIVATE_H__
#define __SCENEPRIVATE_H__

// Forward declarations.
class FScene;

/** max DPG for scene rendering */
enum { SDPG_MAX_SceneRender = SDPG_PostProcess };

// Globals for scene rendering

// defined in SceneCore.cpp
extern UBOOL GInvertCullMode;

// Dependencies.
#include "BatchedElements.h"
#include "SceneRenderTargets.h"
#include "SceneCore.h"
#include "DepthRendering.h"
#include "SceneHitProxyRendering.h"
#include "ShaderComplexityRendering.h"
#include "FogRendering.h"
#include "FogVolumeRendering.h"
#include "BasePassRendering.h"
#include "ShadowRendering.h"
#include "ShadowVolumeRendering.h"
#include "VSMShadowRendering.h"
#include "BranchingPCFShadowRendering.h"
#include "DistortionRendering.h"
#include "SceneRendering.h"
#include "TranslucentRendering.h"
#include "VelocityRendering.h"
#include "TextureDensityRendering.h"

/** Holds information about a single primitive's occlusion. */
class FPrimitiveOcclusionHistory
{
public:

	/** The primitive the occlusion information is about. */
	const UPrimitiveComponent* Primitive;

	/** The occlusion query which contains the primitive's pending occlusion results. */
	FOcclusionQueryRHIRef PendingOcclusionQuery;

	/** The last time the primitive was visible. */
	FLOAT LastVisibleTime;

	/** The last time the primitive was in the view frustum. */
	FLOAT LastConsideredTime;

	/** 
	 *	The pixels that were rendered the last time the primitive was drawn.
	 *	It is the ratio of pixels unoccluded to the resolution of the scene.
	 */
	FLOAT LastPixelsPercentage;

	/** Initialization constructor. */
	FPrimitiveOcclusionHistory(const UPrimitiveComponent* InPrimitive):
		Primitive(InPrimitive),
		LastVisibleTime(0.0f),
		LastConsideredTime(0.0f),
		LastPixelsPercentage(0.0f)
	{}

	/** Defines how the hash set indexes the FPrimitiveOcclusionHistory objects. */
	struct KeyFuncs
	{
		typedef const UPrimitiveComponent* KeyType;
		typedef const UPrimitiveComponent* KeyInitType;

		static KeyInitType GetSetKey(const FPrimitiveOcclusionHistory& Element)
		{
			return Element.Primitive;
		}

		static UBOOL Matches(KeyInitType A,KeyInitType B)
		{
			return A == B;
		}

		static DWORD GetTypeHash(KeyInitType Key)
		{
			return PointerHash(Key);
		}
	};
};

/**
 * A pool of occlusion queries which are allocated individually, and returned to the pool as a group.
 */
class FOcclusionQueryPool
{
public:

	/** Releases all the occlusion queries in the pool. */
	void Release();

	/** Allocates an occlusion query from the pool. */
	FOcclusionQueryRHIParamRef Allocate();

	/** Resets all occlusion queries allocated from the pool to the freed state. */
	void Reset();

	virtual ~FOcclusionQueryPool();

private:

	TArray<FOcclusionQueryRHIRef> OcclusionQueries;
	INT NextFreeQueryIndex;
};

/**
 * The scene manager's private implementation of persistent view state.
 * This class is associated with a particular camera across multiple frames by the game thread.
 * The game thread calls AllocateViewState to create an instance of this private implementation.
 */
class FSceneViewState : public FSceneViewStateInterface, public FDeferredCleanupInterface, public FRenderResource
{
public:

    class FProjectedShadowKey
    {
	public:

        FORCEINLINE UBOOL operator == (const FProjectedShadowKey &Other) const
        {
            return (Primitive == Other.Primitive && Light == Other.Light);
        }

        FProjectedShadowKey(const UPrimitiveComponent* InPrimitive,const ULightComponent* InLight):
            Primitive(InPrimitive),
            Light(InLight)
        {
		}

		friend FORCEINLINE DWORD GetTypeHash(const FSceneViewState::FProjectedShadowKey& Key)
		{
			return PointerHash(Key.Light,PointerHash(Key.Primitive));
		}

	private:
		const UPrimitiveComponent* Primitive;
        const ULightComponent* Light;
    };

    class FPrimitiveComponentKey
    {
	public:

        FORCEINLINE UBOOL operator == (const FPrimitiveComponentKey &Other)
        {
            return (Primitive == Other.Primitive);
        }

        FPrimitiveComponentKey(const UPrimitiveComponent* InPrimitive):
            Primitive(InPrimitive)
        {
        }

		friend FORCEINLINE DWORD GetTypeHash(const FSceneViewState::FPrimitiveComponentKey& Key)
		{
			return PointerHash(Key.Primitive);
		}

	private:
		
        const UPrimitiveComponent* Primitive;
    };

    TMap<FProjectedShadowKey,FOcclusionQueryRHIRef> ShadowOcclusionQueryMap;

	/** The view's occlusion query pool. */
	FOcclusionQueryPool OcclusionQueryPool;

	/** Parameter to keep track of previous frame. Managed by the rendering thread. */
	FMatrix		PendingPrevProjMatrix;
	FMatrix		PrevProjMatrix;
	FMatrix		PendingPrevViewMatrix;
	FMatrix		PrevViewMatrix;
	FVector		PendingPrevViewOrigin;
	FVector		PrevViewOrigin;
	FLOAT		LastRenderTime;
	FLOAT		MotionBlurTimeScale;

	/** Used by states that have IsViewParent() == TRUE to store primitives for child states. */
	THashSet<const UPrimitiveComponent*> ParentPrimitives;

#if !FINAL_RELEASE
	/** Are we currently in the state of freezing rendering? (1 frame where we gather what was rendered) */
	UBOOL bIsFreezing;

	/** Is rendering currently frozen? */
	UBOOL bIsFrozen;

	/** The set of primitives that were rendered the frame that we froze rendering */
	THashSet<const UPrimitiveComponent*> FrozenPrimitives;
#endif

	/** Default constructor. */
    FSceneViewState()
    {
		LastRenderTime = 0.0f;
		MotionBlurTimeScale = 1.0f;
		PendingPrevProjMatrix.SetIdentity();
		PrevProjMatrix.SetIdentity();
		PendingPrevViewMatrix.SetIdentity();
		PrevViewMatrix.SetIdentity();
		PendingPrevViewOrigin = FVector(0,0,0);
		PrevViewOrigin = FVector(0,0,0);
#if !FINAL_RELEASE
		bIsFreezing = FALSE;
		bIsFrozen = FALSE;
#endif

		// Register this object as a resource, so it will receive device reset notifications.
		BeginInitResource(this);
    }

	/**
	 * Determines whether a primitive has been occluded or non-visible for a minimum amount of time.
	 * @param Primitive - The primitive.
	 * @param MinTime - The earliest time to check in the occlusion history.
	 * @return TRUE if the primitive has been occluded the whole time since MinTime.
	 */
	UBOOL WasPrimitivePreviouslyOccluded(const UPrimitiveComponent* Primitive,FLOAT MinTime) const;

	/**
	 * Cleans out old entries from the primitive occlusion history, and resets unused pending occlusion queries.
	 * @param MinHistoryTime - The occlusion history for any primitives which have been visible and unoccluded since
	 *							this time will be kept.  The occlusion history for any primitives which haven't been
	 *							visible and unoccluded since this time will be discarded.
	 * @param MinQueryTime - The pending occlusion queries older than this will be discarded.
	 */
	void TrimOcclusionHistory(FLOAT MinHistoryTime,FLOAT MinQueryTime);

	/**
	 * Checks whether a primitive is occluded this frame.  Also updates the occlusion history for the primitive.
	 * @param CompactPrimitiveSceneInfo - The compact scene info for the primitive to check the occlusion state for.
	 * @param View - The frame's view corresponding to this view state.
	 * @param CurrentRealTime - The current frame's real time.
	 */
	UBOOL UpdatePrimitiveOcclusion(const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo,FViewInfo& View,FLOAT CurrentRealTime);

	/**
	 * Checks whether a shadow is occluded this frame.
	 * @param Primitive - The shadow subject.
	 * @param Light - The shadow source.
	 */
	UBOOL IsShadowOccluded(const UPrimitiveComponent* Primitive,const ULightComponent* Light) const;

	/**
	 *	Retrieves the percentage of the views render target the primitive touched last time it was rendered.
	 *
	 *	@param	Primitive				The primitive of interest.
	 *	@param	OutCoveragePercentage	REFERENCE: The screen coverate percentage. (OUTPUT)
	 *	@return	UBOOL					TRUE if the primitive was found and the results are valid, FALSE is not
	 */
	UBOOL GetPrimitiveCoveragePercentage(const UPrimitiveComponent* Primitive, FLOAT& OutCoveragePercentage);

    // FRenderResource interface.
    virtual void ReleaseDynamicRHI()
    {
        ShadowOcclusionQueryMap.Reset();
		PrimitiveOcclusionHistorySet.Empty();
		OcclusionQueryPool.Release();
    }

	// FSceneViewStateInterface
	virtual void Destroy()
	{
		check(IsInGameThread());

		// Release the occlusion query data.
		BeginReleaseResource(this);

		// Defer deletion of the view state until the rendering thread is done with it.
		BeginCleanup(this);
	}
	
	// FDeferredCleanupInterface
	virtual void FinishCleanup()
	{
		delete this;
	}

private:

	/** Information about visibility/occlusion states in past frames for individual primitives. */
	THashSet<FPrimitiveOcclusionHistory,FPrimitiveOcclusionHistory::KeyFuncs,1> PrimitiveOcclusionHistorySet;
};

class FDepthPriorityGroup
{
public:

	// various static draw lists for this DPG

	/** position-ony depth draw list */
	TStaticMeshDrawList<FPositionOnlyDepthDrawingPolicy> PositionOnlyDepthDrawList;
	/** depth draw list */
	TStaticMeshDrawList<FDepthDrawingPolicy> DepthDrawList;
	/** Base pass draw list - no light map */
	TStaticMeshDrawList<TBasePassDrawingPolicy<FNoLightMapPolicy,FNoDensityPolicy> > BasePassNoLightMapDrawList;
	/** Base pass draw list - directional vertex light maps */
	TStaticMeshDrawList<TBasePassDrawingPolicy<FDirectionalVertexLightMapPolicy,FNoDensityPolicy> > BasePassDirectionalVertexLightMapDrawList;
	/** Base pass draw list - simple vertex light maps */
	TStaticMeshDrawList<TBasePassDrawingPolicy<FSimpleVertexLightMapPolicy,FNoDensityPolicy> > BasePassSimpleVertexLightMapDrawList;
	/** Base pass draw list - directional texture light maps */
	TStaticMeshDrawList<TBasePassDrawingPolicy<FDirectionalLightMapTexturePolicy,FNoDensityPolicy> > BasePassDirectionalLightMapTextureDrawList;
	/** Base pass draw list - simple texture light maps */
	TStaticMeshDrawList<TBasePassDrawingPolicy<FSimpleLightMapTexturePolicy,FNoDensityPolicy> > BasePassSimpleLightMapTextureDrawList;
	/** hit proxy draw list */
	TStaticMeshDrawList<FHitProxyDrawingPolicy> HitProxyDrawList;
	/** draw list for motion blur velocities */
	TStaticMeshDrawList<FVelocityDrawingPolicy> VelocityDrawList;

	/** Maps a light-map type to the appropriate base pass draw list. */
	template<typename LightMapPolicyType>
	TStaticMeshDrawList<TBasePassDrawingPolicy<LightMapPolicyType,FNoDensityPolicy> >& GetBasePassDrawList();
};

class FScene : public FSceneInterface
{
public:

	/** An optional world associated with the level. */
	UWorld* World;

	/** The draw lists for the scene, sorted by DPG and translucency layer. */
	FDepthPriorityGroup DPGs[SDPG_MAX_SceneRender];

	/** The primitives in the scene. */
	TSparseArray<FPrimitiveSceneInfoCompact> Primitives;

	/** The scene capture probes for rendering the scene to texture targets */
	TSparseArray<FCaptureSceneInfo*> SceneCaptures;

	/** The lights in the scene. */
	TSparseArray<FLightSceneInfo*> Lights;

	/** The static meshes in the scene. */
	TSparseArray<FStaticMesh*> StaticMeshes;

	/** The fog components in the scene. */
	TArray<FHeightFogSceneInfo> Fogs;

	/** The wind sources in the scene. */
	TArray<class FWindSourceSceneProxy*> WindSources;

	/** Maps a primitive component to the fog volume density information associated with it. */
	TDynamicMap<const UPrimitiveComponent*, FFogVolumeDensitySceneInfo*> FogVolumes;

	/** The light environments in the scene. */
	TSparseArray<FLightEnvironmentSceneInfo*> LightEnvironments;

	/** The light which aren't in light environments, grouped by lighting channels. */
	TDynamicMap<FLightingChannelContainer,TArray<FLightSceneInfo*> > LightingChannelLightGroupMap;

	/** The set of lights which have been added since the last time the scene was rendered, and haven't been attached to primitives yet. */
	TMap<FLightSceneInfo*,UBOOL> PendingLightAttachments;

	/** Indicates this scene always allows audio playback. */
	UBOOL bAlwaysAllowAudioPlayback;

	/** Indicates whether this scene requires hit proxy rendering. */
	UBOOL bRequiresHitProxies;

	/** Set by the rendering thread to signal to the game thread that the scene needs a static lighting build. */
	volatile mutable INT NumUncachedStaticLightingInteractions;

	/** The motion blur info entries for the frame */
	TArray<FMotionBlurInfo> MotionBlurInfoArray;

	/** Default constructor. */
	FScene():
		NumUncachedStaticLightingInteractions(0)
	{}

	/** Commits all pending light attachments. */
	void CommitPendingLightAttachments();

	// FSceneInterface interface.
	virtual void AddPrimitive(UPrimitiveComponent* Primitive);
	virtual void RemovePrimitive(UPrimitiveComponent* Primitive);
	virtual void UpdatePrimitiveTransform(UPrimitiveComponent* Primitive);
	virtual void AddLight(ULightComponent* Light);
	virtual void RemoveLight(ULightComponent* Light);
	virtual void UpdateLightTransform(ULightComponent* Light);
	virtual void UpdateLightColorAndBrightness(ULightComponent* Light);
	virtual void AddLightEnvironment(const ULightEnvironmentComponent* LightEnvironment);
	virtual void RemoveLightEnvironment(const ULightEnvironmentComponent* LightEnvironment);
	virtual void AddHeightFog(UHeightFogComponent* FogComponent);
	virtual void RemoveHeightFog(UHeightFogComponent* FogComponent);
	virtual void AddWindSource(UWindDirectionalSourceComponent* WindComponent);
	virtual void RemoveWindSource(UWindDirectionalSourceComponent* WindComponent);
	virtual const TArray<FWindSourceSceneProxy*>& GetWindSources_RenderThread() const;

	/**
	 * Adds a default FFogVolumeConstantDensitySceneInfo to UPrimitiveComponent pair to the Scene's FogVolumes map.
	 */
	virtual void AddFogVolume(const UPrimitiveComponent* VolumeComponent);

	/**
	 * Adds a FFogVolumeDensitySceneInfo to UPrimitiveComponent pair to the Scene's FogVolumes map.
	 */
	virtual void AddFogVolume(class FFogVolumeDensitySceneInfo* FogVolumeSceneInfo, const UPrimitiveComponent* VolumeComponent);

	/**
	 * Removes an entry by UPrimitiveComponent from the Scene's FogVolumes map.
	 */
	virtual void RemoveFogVolume(const UPrimitiveComponent* VolumeComponent);

	/**
	 * Retrieves the lights interacting with the passed in primitive and adds them to the out array.
	 *
	 * @param	Primitive				Primitive to retrieve interacting lights for
	 * @param	RelevantLights	[out]	Array of lights interacting with primitive
	 */
	virtual void GetRelevantLights( UPrimitiveComponent* Primitive, TArray<const ULightComponent*>* RelevantLights ) const;

	/**
	 * Create the scene capture info for a capture component and add it to the scene
	 * @param CaptureComponent - component to add to the scene 
	 */
	virtual void AddSceneCapture(USceneCaptureComponent* CaptureComponent);

	/**
	 * Remove the scene capture info for a capture component from the scene
	 * @param CaptureComponent - component to remove from the scene 
	 */
	virtual void RemoveSceneCapture(USceneCaptureComponent* CaptureComponent);

	virtual void Release();
	virtual UWorld* GetWorld() const { return World; }

	/** Indicates if sounds in this should be allowed to play. */
	virtual UBOOL AllowAudioPlayback();

	/**
	 * @return		TRUE if hit proxies should be rendered in this scene.
	 */
	virtual UBOOL RequiresHitProxies() const;

	/**
	* Return the scene to be used for rendering
	*/
	virtual FSceneInterface* GetRenderScene()
	{
		return this;
	}

	/** 
	 *	Set the primitives motion blur info
	 * 
	 *	@param PrimitiveSceneInfo	The primitive to add
	 *	@param	bRemoving			TRUE if the primitive is being removed
	 */
	virtual void AddPrimitiveMotionBlur(FPrimitiveSceneInfo* PrimitiveSceneInfo, UBOOL bRemoving);

	/**
	 *	Clear out the motion blur info
	 */
	virtual void ClearMotionBlurInfo();

	/** 
	 *	Get the primitives motion blur info
	 * 
	 *	@param	PrimitiveSceneInfo	The primitive to retrieve the motion blur info for
	 *	@param	MotionBlurInfo		The pointer to set to the motion blur info for the primitive (OUTPUT)
	 *
	 *	@return	UBOOL				TRUE if the primitive info was found and set
	 */
	virtual UBOOL GetPrimitiveMotionBlurInfo(const FPrimitiveSceneInfo* PrimitiveSceneInfo, const FMotionBlurInfo*& MBInfo);

private:

	/**
	 * Retrieves the lights interacting with the passed in primitive and adds them to the out array.
	 * Render thread version of function.
	 * @param	Primitive				Primitive to retrieve interacting lights for
	 * @param	RelevantLights	[out]	Array of lights interacting with primitive
	 */
	void GetRelevantLights_RenderThread( UPrimitiveComponent* Primitive, TArray<const ULightComponent*>* RelevantLights ) const;

	/**
	 * Adds a primitive to the scene.  Called in the rendering thread by AddPrimitive.
	 * @param PrimitiveSceneInfo - The primitive being added.
	 */
	void AddPrimitiveSceneInfo_RenderThread(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	/**
	 * Removes a primitive from the scene.  Called in the rendering thread by RemovePrimitive.
	 * @param PrimitiveSceneInfo - The primitive being removed.
	 */
	void RemovePrimitiveSceneInfo_RenderThread(FPrimitiveSceneInfo* PrimitiveSceneInfo);

	/**
	 * Adds a light to the scene.  Called in the rendering thread by AddLight.
	 * @param LightSceneInfo - The light being added.
	 */
	void AddLightSceneInfo_RenderThread(FLightSceneInfo* LightSceneInfo);

	/**
	 * Removes a light from the scene.  Called in the rendering thread by RemoveLight.
	 * @param LightSceneInfo - The light being removed.
	 */
	void RemoveLightSceneInfo_RenderThread(FLightSceneInfo* LightSceneInfo);
};

/** The scene update stats. */
enum
{
	STAT_AddScenePrimitiveRenderThreadTime = STAT_SceneUpdateFirstStat,
	STAT_AddScenePrimitiveGameThreadTime,
	STAT_AddSceneLightTime,
	STAT_RemoveScenePrimitiveTime,
	STAT_RemoveSceneLightTime,
};

#endif // __SCENEPRIVATE_H__
