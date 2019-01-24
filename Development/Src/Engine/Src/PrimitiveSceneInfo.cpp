/*=============================================================================
	PrimitiveSceneInfo.cpp: Primitive scene info implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

#include "EngineParticleClasses.h"

#if USE_MASSIVE_LOD

/** Global map of primitive components to their Compact primitive info to set up child relationships */
TMap<UPrimitiveComponent*, FPathToCompact> FPrimitiveSceneInfo::PrimitiveToCompactMap;

/** If the children primitives are attached before the parent, they have no compact info to attach to, so store it here temporarily */
TMultiMap<UPrimitiveComponent*, FPrimitiveSceneInfoCompact*> FPrimitiveSceneInfo::PendingChildPrimitiveMap;

/**
* Retrieves the compact info represented by the path
*/
class FPrimitiveSceneInfoCompact& FPathToCompact::GetCompact(FScenePrimitiveOctree& Octree)
{
	return DirectReference ? *DirectReference : Octree.GetElementById(OctreeId);
}

#endif


/** An implementation of FStaticPrimitiveDrawInterface that stores the drawn elements for the rendering thread to use. */
class FBatchingSPDI : public FStaticPrimitiveDrawInterface
{
public:

	// Constructor.
	FBatchingSPDI(FPrimitiveSceneInfo* InPrimitiveSceneInfo):
		PrimitiveSceneInfo(InPrimitiveSceneInfo)
	{}

	// FStaticPrimitiveDrawInterface.
	virtual void SetHitProxy(HHitProxy* HitProxy)
	{
		CurrentHitProxy = HitProxy;

		if(HitProxy)
		{
			// Only use static scene primitive hit proxies in the editor.
			if(GIsEditor)
			{
				// Keep a reference to the hit proxy from the FPrimitiveSceneInfo, to ensure it isn't deleted while the static mesh still
				// uses its id.
				PrimitiveSceneInfo->HitProxies.AddItem(HitProxy);
			}
		}
	}
	virtual void DrawMesh(
		const FMeshElement& Mesh,
		FLOAT MinDrawDistance,
		FLOAT MaxDrawDistance
		)
	{
		check(Mesh.NumPrimitives > 0);
		check(Mesh.VertexFactory);
		FStaticMesh* StaticMesh = new(PrimitiveSceneInfo->StaticMeshes) FStaticMesh(
			PrimitiveSceneInfo,
			Mesh,
			Square(Max(0.0f,MinDrawDistance)),
			Square(Max(0.0f,MaxDrawDistance)),
			CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId()
			);
	}

private:
	FPrimitiveSceneInfo* PrimitiveSceneInfo;
	TRefCountPtr<HHitProxy> CurrentHitProxy;
};

void FPrimitiveSceneInfoCompact::Init(FPrimitiveSceneInfo* InPrimitiveSceneInfo)
{
	PrimitiveSceneInfo = InPrimitiveSceneInfo;
	Proxy = PrimitiveSceneInfo->Proxy;
	Component = PrimitiveSceneInfo->Component;
	LightEnvironment = PrimitiveSceneInfo->LightEnvironment;
	Bounds = PrimitiveSceneInfo->Bounds;
	MinDrawDistanceSquared = Square(PrimitiveSceneInfo->MinDrawDistance);
	MaxDrawDistanceSquared = Square(PrimitiveSceneInfo->MaxDrawDistance);
	LightingChannels = PrimitiveSceneInfo->LightingChannels;
#if USE_MASSIVE_LOD
	ReplacementPrimitiveMapKey = PrimitiveSceneInfo->ReplacementPrimitiveMapKey;
	MassiveLODDistanceSquared = Square(PrimitiveSceneInfo->MassiveLODDistance);
	STAT(NumberOfDescendents = 0);
	STAT(bHasBeenAdded = FALSE);
#endif

	VisibilityId = PrimitiveSceneInfo->VisibilityId;

	bAllowApproximateOcclusion = PrimitiveSceneInfo->bAllowApproximateOcclusion;
	bFirstFrameOcclusion = PrimitiveSceneInfo->bFirstFrameOcclusion;
	bAcceptsLights = PrimitiveSceneInfo->bAcceptsLights;
	bHasViewDependentDPG = Proxy->HasViewDependentDPG();
	bShouldCullModulatedShadows = Component->ShouldCullModulatedShadows();
	bCastDynamicShadow = PrimitiveSceneInfo->bCastDynamicShadow;
	bLightEnvironmentForceNonCompositeDynamicLights = PrimitiveSceneInfo->bLightEnvironmentForceNonCompositeDynamicLights;
	bIgnoreNearPlaneIntersection = PrimitiveSceneInfo->bIgnoreNearPlaneIntersection;
	
	StaticDepthPriorityGroup = bHasViewDependentDPG ? 0 : Proxy->GetStaticDepthPriorityGroup();

	bHasCustomOcclusionBounds = PrimitiveSceneInfo->bHasCustomOcclusionBounds;
}

#if USE_MASSIVE_LOD


/**
 * Finds any pending children for the given primitive, and adds them as children to us
 *
 * @param PrimitiveKey The key used to find our children in the pending child primitive map
 */
void FPrimitiveSceneInfoCompact::AddPendingChildren(UPrimitiveComponent* PrimitiveKey)
{
	// if any children were added before we were, add them now as our children (query directly into the child array for speed)
	TArray<FPrimitiveSceneInfoCompact*> PendingChildren;
	FPrimitiveSceneInfo::PendingChildPrimitiveMap.MultiFind(PrimitiveKey, ChildPrimitives);
	FPrimitiveSceneInfo::PendingChildPrimitiveMap.RemoveKey(PrimitiveKey);

#if STATS
	// update stats based on all the children just added
	for (INT ChildIndex = 0; ChildIndex < ChildPrimitives.Num(); ChildIndex++)
	{
		UpdateDescendentCounts(ChildPrimitives(ChildIndex), TRUE);
	}
#endif

}

#if STATS
/**
 * Updates the descendent counts of compact infos, going up the hierarchy
 * @param Child The primitive that was added or removed as a child from this compact info
 * @bChildWasAdded If TRUE, the child was added, so descendent counts increase, otherwise decrease
 */
void FPrimitiveSceneInfoCompact::UpdateDescendentCounts(FPrimitiveSceneInfoCompact* Child, UBOOL bChildWasAdded)
{
	// track if this guy has been added to the parent, and been counted
	bHasBeenAdded = bChildWasAdded;

	// keep descendent counts up to date, going up the parenting
	FPrimitiveSceneInfoCompact* Parent = this;

	// account for the prim we are adding/removing, as well as it's descendents
	INT AmountToModify = bChildWasAdded ? (Child->NumberOfDescendents + 1) : -(Child->NumberOfDescendents + 1);

	while (Parent)
	{
		Parent->NumberOfDescendents += AmountToModify;

		// walk up the chain
		FPrimitiveSceneInfoCompact* NewParent = NULL;
		if (Parent->ReplacementPrimitiveMapKey)
		{
			FPathToCompact* ParentPath = FPrimitiveSceneInfo::PrimitiveToCompactMap.Find(Parent->ReplacementPrimitiveMapKey);
			if (ParentPath)
			{
				NewParent = &ParentPath->GetCompact(PrimitiveSceneInfo->Scene->PrimitiveOctree);

				// due to pending adds, we don't want to go up the chain if the parent hasn't already been added, because
				// otherwise, it will be added again, and double counted
				if (!NewParent->bHasBeenAdded)
				{
					NewParent = NULL;
				}
			}
		}

		Parent = NewParent;
	}
}

#endif // STATS

#endif // USE_MASSIVE_LOD


FPrimitiveSceneInfo::FPrimitiveSceneInfo(UPrimitiveComponent* InComponent,FPrimitiveSceneProxy* InProxy,FScene* InScene):
	Proxy(InProxy),
	Component(InComponent),
	Owner(InComponent->GetOwner()),
	Id(INDEX_NONE),
	TranslucencySortPriority(Clamp(InComponent->TranslucencySortPriority, SHRT_MIN, SHRT_MAX)),
	NumAffectingDominantLights(0),
	VisibilityId(InComponent->VisibilityId),
	bStaticShadowing(InComponent->HasStaticShadowing()),
	// Disable dynamic shadow casting if the primitive only casts indirect shadows, since dynamic shadows are always shadowing direct lighting
	bCastDynamicShadow(InComponent->bCastDynamicShadow && InComponent->CastShadow && !InComponent->GetShadowIndirectOnly()),
	bSelfShadowOnly(InComponent->bSelfShadowOnly),
	bAcceptsDynamicDominantLightShadows(InComponent->bAcceptsDynamicDominantLightShadows),
	bCastStaticShadow(InComponent->CastShadow && InComponent->bCastStaticShadow),
	bCastHiddenShadow(InComponent->bCastHiddenShadow),
	bCastShadowAsTwoSided(InComponent->bCastShadowAsTwoSided),
	bAllowPreShadow((InComponent->LightEnvironment && InComponent->LightEnvironment->IsEnabled()) ?
		InComponent->LightEnvironment->AllowPreShadow() :
		FALSE),
	bAcceptsLights(InComponent->bAcceptsLights),
	bAcceptsDynamicLights(InComponent->bAcceptsDynamicLights),
	bUseOnePassLightingOnTranslucency(InComponent->bUseOnePassLightingOnTranslucency && !InComponent->HasStaticShadowing()),
	bSelfContainedLighting((InComponent->GetOutermost()->PackageFlags & PKG_SelfContainedLighting) != 0),
	bUseAsOccluder(InComponent->bUseAsOccluder),
	bAllowApproximateOcclusion(InComponent->bAllowApproximateOcclusion),
	bFirstFrameOcclusion(InComponent->bFirstFrameOcclusion),
	bIgnoreNearPlaneIntersection(InComponent->bIgnoreNearPlaneIntersection),
	bSelectable(InComponent->bSelectable),
	bNeedsStaticMeshUpdate(FALSE),
	bCullModulatedShadowOnBackfaces(InComponent->bCullModulatedShadowOnBackfaces),
	bCullModulatedShadowOnEmissive(InComponent->bCullModulatedShadowOnEmissive),
	bAllowAmbientOcclusion(InComponent->bAllowAmbientOcclusion),
	bRenderSHLightInBasePass(FALSE),
	bEnableMotionBlur(InComponent->MotionBlurScale > 0.5f),
	bLightEnvironmentForceNonCompositeDynamicLights(InComponent->LightEnvironment ? InComponent->LightEnvironment->bForceNonCompositeDynamicLights : TRUE),
	bHasCustomOcclusionBounds(InProxy->HasCustomOcclusionBounds()),
	bAllowShadowFade(InComponent->bAllowShadowFade),
	bAllowDominantLightInfluence(
		!InComponent->LightEnvironment 
		|| !InComponent->LightEnvironment->IsEnabled() 
		|| InComponent->LightEnvironment->GetAffectingDominantLight() != NULL),
	bAllowDynamicShadowsOnTranslucency((InComponent->LightEnvironment && InComponent->LightEnvironment->IsEnabled()) ?
		InComponent->LightEnvironment->AllowDynamicShadowsOnTranslucency() :
		FALSE),
	bTranslucencyShadowed((InComponent->LightEnvironment && InComponent->LightEnvironment->IsEnabled()) ?
		InComponent->LightEnvironment->IsTranslucencyShadowed() :
		FALSE),
	PreviewEnvironmentShadowing(InComponent->PreviewEnvironmentShadowing),
	Bounds(InComponent->Bounds),
	MaxDrawDistance(InComponent->CachedMaxDrawDistance),
	MinDrawDistance(InComponent->MinDrawDistance),
	LightingChannels(InComponent->LightingChannels),
	LightEnvironment(
		(InComponent->LightEnvironment && InComponent->LightEnvironment->IsEnabled()) ?
			InComponent->LightEnvironment :
			NULL
		),
	AffectingDominantLight(
		(InComponent->LightEnvironment && InComponent->LightEnvironment->IsEnabled()) ?
			InComponent->LightEnvironment->GetAffectingDominantLight() :
			NULL
		),
	OverrideLightComponent(InComponent->OverrideLightComponent),
	LevelName(InComponent->GetOutermost()->GetFName()),
	LightList(NULL),
	UpperSkyLightColor(FLinearColor::Black),
	LowerSkyLightColor(FLinearColor::Black),
	DynamicLightSceneInfo(NULL),
	SHLightSceneInfo(NULL),
	DominantShadowFactor(
		(InComponent->LightEnvironment && InComponent->LightEnvironment->IsEnabled()) ?
			InComponent->LightEnvironment->GetDominantShadowFactor() :
			1.0f
		),
	BrightestDominantLightSceneInfo(NULL),
	ShadowParent(InComponent->ShadowParent),
	FogVolumeSceneInfo(NULL),
	LastRenderTime(-FLT_MAX),
	LastVisibilityChangeTime(0.0f),
	Scene(InScene)
{
	check(Component);
	check(Proxy);

	InComponent->SceneInfo = this;
	Proxy->PrimitiveSceneInfo = this;

	// if the primitive is shadow parented then use cull mode of its parent
	if( ShadowParent )
	{
		bCullModulatedShadowOnBackfaces = ShadowParent->bCullModulatedShadowOnBackfaces;
		bCullModulatedShadowOnEmissive = ShadowParent->bCullModulatedShadowOnEmissive;
	}

	// Replace 0 MaxDrawDistance with FLT_MAX so we don't have to add any special case handling later.
	if( MaxDrawDistance == 0 )
	{
		MaxDrawDistance = FLT_MAX;
	}

#if USE_MASSIVE_LOD
	// cache the distance where MassiveLOD swaps
	MassiveLODDistance = InComponent->MassiveLODDistance;

	// Cache the replacement primitive pointer for looking up in maps, etc (never dereferenced)
	ReplacementPrimitiveMapKey = InComponent->ReplacementPrimitive;

#endif

	// Only create hit proxies in the Editor as that's where they are used.
	HHitProxy* DefaultHitProxy = NULL;
	if(GIsEditor)
	{
		// Create a dynamic hit proxy for the primitive. 
		DefaultHitProxy = Proxy->CreateHitProxies(Component,HitProxies);
		if( DefaultHitProxy )
		{
			DefaultDynamicHitProxyId = DefaultHitProxy->Id;
		}

#if USE_MASSIVE_LOD
		// record in the world that we are actively using MassiveLOD, so extra cleanup can happen in the editor
		if (ReplacementPrimitiveMapKey)
		{
			GWorld->bEditorHasMassiveLOD = TRUE;
		}
#endif
	}

	// Cache the primitive's static mesh elements.
	FBatchingSPDI BatchingSPDI(this);
	BatchingSPDI.SetHitProxy(DefaultHitProxy);
	Proxy->DrawStaticElements(&BatchingSPDI);
	StaticMeshes.Shrink();
}

FPrimitiveSceneInfo::~FPrimitiveSceneInfo()
{
	check(!OctreeId.IsValidId());
}

void FPrimitiveSceneInfo::AddToScene()
{
	check(IsInRenderingThread());

	for(INT MeshIndex = 0;MeshIndex < StaticMeshes.Num();MeshIndex++)
	{
		// Add the static mesh to the scene's static mesh list.
		FSparseArrayAllocationInfo SceneArrayAllocation = Scene->StaticMeshes.Add();
		Scene->StaticMeshes(SceneArrayAllocation.Index) = &StaticMeshes(MeshIndex);
		StaticMeshes(MeshIndex).Id = SceneArrayAllocation.Index;

		// Add the static mesh to the appropriate draw lists.
		StaticMeshes(MeshIndex).AddToDrawLists(Scene);
	}

	// create potential storage for our compact info (may not be used in the ReplacementPrimitiveMapKey case)
	FPrimitiveSceneInfoCompact LocalCompactPrimitiveSceneInfo;
	FPrimitiveSceneInfoCompact* CompactPrimitiveSceneInfo = &LocalCompactPrimitiveSceneInfo;

#if USE_MASSIVE_LOD

	// if we have parent, then we add ourselves to the parent, NOT the octree
	if (ReplacementPrimitiveMapKey)
	{
		// if we are being added as a child, we need to allocate "permanent" (won't be realloced) storage for the compact info
		CompactPrimitiveSceneInfo = new FPrimitiveSceneInfoCompact(this);

		// look in the pending map for children of this primitive, based on the component key
		CompactPrimitiveSceneInfo->AddPendingChildren(Component);

		// look to see if our parent exists yet
		FPathToCompact* PathToParent = PrimitiveToCompactMap.Find(ReplacementPrimitiveMapKey);
		if (PathToParent)
		{
			// if it does exist, add us as a child
			FPrimitiveSceneInfoCompact& CompactParent = PathToParent->GetCompact(Scene->PrimitiveOctree);
			CompactParent.AddChildPrimitive(CompactPrimitiveSceneInfo);
		}
		else
		{
			// if the parent wasn't attached yet, store us in the pending map
			PendingChildPrimitiveMap.Add(ReplacementPrimitiveMapKey, CompactPrimitiveSceneInfo);
		}

		// and add us to the PrimitiveToCompact map for tracking
		PrimitiveToCompactMap.Set(Component, FPathToCompact(CompactPrimitiveSceneInfo));
	}
	else
#endif
	{
		// if we are being added directly to the Octree, initialize the temp compact scene info,
		// and let the Octree make a copy of it
		LocalCompactPrimitiveSceneInfo.Init(this);

#if USE_MASSIVE_LOD
		// look in the pending map for children of this primitive, based on the component key
		LocalCompactPrimitiveSceneInfo.AddPendingChildren(Component);
#endif

		// Add the primitive to the octree.
		check(!OctreeId.IsValidId());
		Scene->PrimitiveOctree.AddElement(LocalCompactPrimitiveSceneInfo);
		check(OctreeId.IsValidId());

#if USE_MASSIVE_LOD
		// and add us to the PrimitiveToCompact map for tracking
		PrimitiveToCompactMap.Set(Component, FPathToCompact(OctreeId));
#endif
	}

	if(bAcceptsLights)
	{
		if(LightEnvironment)
		{
			// For primitives in a light environment, attach it to lights which are in the same light environment.
			FLightEnvironmentSceneInfo& LightEnvironmentSceneInfo = Scene->GetLightEnvironmentSceneInfo(LightEnvironment);
			for(INT LightIndex = 0;LightIndex < LightEnvironmentSceneInfo.Lights.Num();LightIndex++)
			{
				FLightSceneInfo* LightSceneInfo = LightEnvironmentSceneInfo.Lights(LightIndex);
				if(FLightSceneInfoCompact(LightSceneInfo).AffectsPrimitive(*CompactPrimitiveSceneInfo))
				{
					FLightPrimitiveInteraction::Create(LightSceneInfo,this);
				}
			}
		}

		// Find lights that affect the primitive in the light octree.		
		{
			FMemMark MemStackMark(GRenderingThreadMemStack);

			for(FSceneLightOctree::TConstElementBoxIterator<SceneRenderingAllocator> LightIt(Scene->LightOctree,Bounds.GetBox());
				LightIt.HasPendingElements();
				LightIt.Advance())
			{
				const FLightSceneInfoCompact& LightSceneInfoCompact = LightIt.GetCurrentElement();

				if(LightSceneInfoCompact.AffectsPrimitive(*CompactPrimitiveSceneInfo))
				{
					FLightPrimitiveInteraction::Create(LightSceneInfoCompact.LightSceneInfo,this);
				}
			}
		}
	}
}

void FPrimitiveSceneInfo::RemoveFromScene()
{
	check(IsInRenderingThread());

	// implicit linked list. The destruction will update this "head" pointer to the next item in the list.
	while(LightList)
	{
		FLightPrimitiveInteraction::Destroy(LightList);
	}
	
#if USE_MASSIVE_LOD
	// find the path to ourself
	FPathToCompact* PathToSelf = PrimitiveToCompactMap.Find(Component);
	check(PathToSelf);

	// get the compact info of the parent
	FPrimitiveSceneInfoCompact& CompactSelf = PathToSelf->GetCompact(Scene->PrimitiveOctree);
	check(CompactSelf.Component == Component);

	// remove ourself from the primitive to compact map
	PrimitiveToCompactMap.RemoveKey(Component);

	// now, all of our children must be put into the pending list, so they can be readded to the parent if
	// it gets reattached
	for (INT ChildIndex = 0; ChildIndex < CompactSelf.ChildPrimitives.Num(); ChildIndex++)
	{
		PendingChildPrimitiveMap.Add(Component, CompactSelf.ChildPrimitives(ChildIndex));
	}

	// remove the info from its parent
	if (ReplacementPrimitiveMapKey)
	{
		// find the path to the parent component
		FPathToCompact* PathToParent = PrimitiveToCompactMap.Find(ReplacementPrimitiveMapKey);

		// have we been added (the common case)
		if (PathToParent)
		{
			// get the compact info of the parent
			FPrimitiveSceneInfoCompact& CompactParent = PathToParent->GetCompact(Scene->PrimitiveOctree);

			// remove us from our parent
			CompactParent.RemoveChildPrimitive(&CompactSelf);
		}
		// if the parent wasn't found, then it's probably in the pending list, so remove ourself from there
		else
		{
			PendingChildPrimitiveMap.RemovePair(ReplacementPrimitiveMapKey, &CompactSelf);
		}

		// we need to free the memory for the compact, because in this case it was new'ed in AddToScene
		delete &CompactSelf;
	}
	else
#endif
	{
		// Remove the primitive from the octree.
		check(OctreeId.IsValidId());
		check(Scene->PrimitiveOctree.GetElementById(OctreeId).PrimitiveSceneInfo == this);
		Scene->PrimitiveOctree.RemoveElement(OctreeId);
		OctreeId = FOctreeElementId();
	}

	// Remove static meshes from the scene.
	StaticMeshes.Empty();
}

void FPrimitiveSceneInfo::ConditionalUpdateStaticMeshes()
{
	if(bNeedsStaticMeshUpdate)
	{
		bNeedsStaticMeshUpdate = FALSE;

		// Remove the primitive's static meshes from the draw lists they're currently in, and readd them to the appropriate draw lists.
		for(INT MeshIndex = 0;MeshIndex < StaticMeshes.Num();MeshIndex++)
		{
			StaticMeshes(MeshIndex).RemoveFromDrawLists();
			StaticMeshes(MeshIndex).AddToDrawLists(Scene);
		}

		// Also re-add the primitive's decal static mesh elements to draw lists
		for(INT DecalIdx = 0; DecalIdx < Proxy->Decals.Num(); DecalIdx++)
		{
			FDecalInteraction* Decal = Proxy->Decals(DecalIdx);
			if(Decal && Decal->DecalStaticMesh)
			{
				Decal->DecalStaticMesh->RemoveFromDrawLists();
				Decal->DecalStaticMesh->AddToDrawLists(Scene);
			}
		}
	}
}

void FPrimitiveSceneInfo::BeginDeferredUpdateStaticMeshes()
{
	// Set a flag which causes InitViews to update the static meshes the next time the primitive is visible.
	bNeedsStaticMeshUpdate = TRUE;
}

void FPrimitiveSceneInfo::LinkShadowParent()
{
	// Add the primitive to its shadow group.
	if(ShadowParent)
	{
		FShadowGroupSceneInfo* ShadowGroup = Scene->ShadowGroups.Find(ShadowParent);
		if(!ShadowGroup)
		{
			// If this is the first primitive attached that uses this shadow parent, create a new shadow group.
			ShadowGroup = &Scene->ShadowGroups.Set(ShadowParent,FShadowGroupSceneInfo());
		}
		ShadowGroup->Primitives.AddItem(this);
	}
}

void FPrimitiveSceneInfo::UnlinkShadowParent()
{
	// Remove the primitive from its shadow group.
	if(ShadowParent)
	{
		FShadowGroupSceneInfo* ShadowGroup = Scene->ShadowGroups.Find(ShadowParent);
		check(ShadowGroup);
		ShadowGroup->Primitives.RemoveItemSwap(this);

		if(!ShadowGroup->Primitives.Num())
		{
			// If this was the last primitive attached that uses this shadow parent, free the shadow group.
			Scene->ShadowGroups.Remove(ShadowParent);
		}
	}
}

void FPrimitiveSceneInfo::FinishCleanup()
{
	delete this;
}

UINT FPrimitiveSceneInfo::GetMemoryFootprint()
{
	return( sizeof( *this ) + HitProxies.GetAllocatedSize() + StaticMeshes.GetAllocatedSize() );
}

FLinearColor FPrimitiveSceneInfo::GetPreviewSkyLightColor() const
{
	return Scene->PreviewSkyLightColor * Min(2.0f * Square(PreviewEnvironmentShadowing / 255.0f), 1.0f);
}
