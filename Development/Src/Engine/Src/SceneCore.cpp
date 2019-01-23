/*=============================================================================
	SceneCore.cpp: Core scene implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/** 
 * Globally inverts the cull mode. 
 * Useful when rendering the scene with an inverted view 
 */
UBOOL GInvertCullMode = FALSE;

#if STATS
/** Global counter of draw events, incremented when issued and reset by code updating STATS. */
DWORD FDrawEvent::Counter;
#endif

/*-----------------------------------------------------------------------------
	FLightPrimitiveInteraction
-----------------------------------------------------------------------------*/

void FLightPrimitiveInteraction::Create(FLightSceneInfo* LightSceneInfo,FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	// Attach the light to the primitive's static meshes.
	UBOOL bDynamic = TRUE;
	UBOOL bRelevant = FALSE;
	UBOOL bLightMapped = TRUE;

	// Determine the light's relevance to the primitive.
	check(PrimitiveSceneInfo->Proxy);
	PrimitiveSceneInfo->Proxy->GetLightRelevance(LightSceneInfo, bDynamic, bRelevant, bLightMapped);

	if(bRelevant)
	{
		// Attach the light to the primitive's static meshes.
		for(INT ElementIndex = 0;ElementIndex < PrimitiveSceneInfo->StaticMeshes.Num();ElementIndex++)
		{
			FMeshLightingDrawingPolicyFactory::AddStaticMesh(
				PrimitiveSceneInfo->Scene,
				&PrimitiveSceneInfo->StaticMeshes(ElementIndex),
				LightSceneInfo
				);
		}

		// Create the light interaction.
		FLightPrimitiveInteraction* Interaction = new FLightPrimitiveInteraction(LightSceneInfo,PrimitiveSceneInfo,bDynamic,bLightMapped);

		// Attach the light to the primitive.
		LightSceneInfo->AttachPrimitive(*Interaction);
	}
}

FLightPrimitiveInteraction::FLightPrimitiveInteraction(
	FLightSceneInfo* InLightSceneInfo,
	FPrimitiveSceneInfo* InPrimitiveSceneInfo,
	UBOOL bNoStaticShadowing,
	UBOOL bInLightMapped
	):
	LightId(InLightSceneInfo->Id),
	LightSceneInfo(InLightSceneInfo),
	PrimitiveSceneInfo(InPrimitiveSceneInfo),
	bLightMapped(bInLightMapped),
	bUncachedStaticLighting(FALSE),
	DynamicShadowType(DST_None)
{
	// Determine whether this light-primitive interaction produces a shadow.
	if(PrimitiveSceneInfo->bStaticShadowing)
	{
		bCastShadow = LightSceneInfo->bCastStaticShadow && PrimitiveSceneInfo->bCastStaticShadow;
	}
	else
	{
		bCastShadow = LightSceneInfo->bCastDynamicShadow && PrimitiveSceneInfo->bCastDynamicShadow;
	}

	if(bCastShadow && bNoStaticShadowing)
	{
		// Determine the type of dynamic shadow produced by this light.
		if(PrimitiveSceneInfo->bStaticShadowing)
		{
			if(LightSceneInfo->bStaticShadowing && PrimitiveSceneInfo->bCastStaticShadow)
			{
				// The primitive and the light are unmovable; this is a case which will have static shadowing
				// once the level is rebuilt.  Use shadow volumes in the mean-time.
				DynamicShadowType = DST_Volume;

				// Update the game thread's counter of number of uncached static lighting interactions.
				bUncachedStaticLighting = TRUE;
				appInterlockedIncrement(&PrimitiveSceneInfo->Scene->NumUncachedStaticLightingInteractions);
			}
			else if(PrimitiveSceneInfo->bCastDynamicShadow)
			{
				// The light is movable, and the primitive is set to cast dynamic shadows.
				DynamicShadowType = DST_Volume;
			}
		}
		else
		{
			if(!LightSceneInfo->bProjectedShadows && PrimitiveSceneInfo->bCastDynamicShadow)
			{
				// The primitive is dynamic and set to cast dynamic shadows.  The light is set to not use projected
				// shadows for dynamic primitives, so a shadow volume will be used.
				DynamicShadowType = DST_Volume;
			}
			else
			{
				DynamicShadowType = DST_Projected;
			}
		}
	}
	
	// keep track of shadow volume primitive interactions
	if (DynamicShadowType == DST_Volume)
	{
		++ LightSceneInfo->NumShadowVolumeInteractions;
	}

	// Add the interaction to the light's interaction list.
	if(bNoStaticShadowing)
	{
		PrevPrimitiveLink = &LightSceneInfo->DynamicPrimitiveList;
	}
	else
	{
		PrevPrimitiveLink = &LightSceneInfo->StaticPrimitiveList;
	}
	NextPrimitive = *PrevPrimitiveLink;
	if(*PrevPrimitiveLink)
	{
		(*PrevPrimitiveLink)->PrevPrimitiveLink = &NextPrimitive;
	}
	*PrevPrimitiveLink = this;

	// Add the interaction to the primitive's interaction list.
	PrevLightLink = &PrimitiveSceneInfo->LightList;
	NextLight = *PrevLightLink;
	if(*PrevLightLink)
	{
		(*PrevLightLink)->PrevLightLink = &NextLight;
	}
	*PrevLightLink = this;
}

FLightPrimitiveInteraction::~FLightPrimitiveInteraction()
{
	check(IsInRenderingThread());

	/*
	 * Notify the scene proxy that the scene light is no longer
	 * associated with the proxy.
	 */
	if ( PrimitiveSceneInfo->Proxy )
	{
		PrimitiveSceneInfo->Proxy->OnDetachLight( LightSceneInfo );
	}

	// Update the game thread's counter of number of uncached static lighting interactions.
	if(bUncachedStaticLighting)
	{
		appInterlockedDecrement(&PrimitiveSceneInfo->Scene->NumUncachedStaticLightingInteractions);
	}

	// keep track of shadow volume primitive interactions
	if (DynamicShadowType == DST_Volume)
	{
		check(0 < LightSceneInfo->NumShadowVolumeInteractions);
		-- LightSceneInfo->NumShadowVolumeInteractions;
	}

	// Detach the light from the primitive.
	LightSceneInfo->DetachPrimitive(*this);

	// Remove the interaction from the light's interaction list.
	if(NextPrimitive)
	{
		NextPrimitive->PrevPrimitiveLink = PrevPrimitiveLink;
	}
	*PrevPrimitiveLink = NextPrimitive;

	// Remove the interaction from the primitive's interaction list.
	if(NextLight)
	{
		NextLight->PrevLightLink = PrevLightLink;
	}
	*PrevLightLink = NextLight;
}

/*-----------------------------------------------------------------------------
FPrimitiveSceneInfo
-----------------------------------------------------------------------------*/
#include "EngineParticleClasses.h"

FPrimitiveSceneInfo::FPrimitiveSceneInfo(UPrimitiveComponent* InComponent,FPrimitiveSceneProxy* InProxy,FScene* InScene):
	Scene(InScene),
	Proxy(InProxy),
	Component(InComponent),
	Owner(InComponent->GetOwner()),
	Id(INDEX_NONE),
	TranslucencySortPriority(InComponent->TranslucencySortPriority),
	bStaticShadowing(InComponent->HasStaticShadowing()),
	bCastDynamicShadow(InComponent->bCastDynamicShadow && InComponent->CastShadow),
	bCastStaticShadow(InComponent->CastShadow),
	bCastHiddenShadow(InComponent->bCastHiddenShadow),
	bAcceptsLights(InComponent->bAcceptsLights),
	bAcceptsDynamicLights(InComponent->bAcceptsDynamicLights),
	bSelfContainedLighting((InComponent->GetOutermost()->PackageFlags & PKG_SelfContainedLighting) != 0),
	bUseAsOccluder(InComponent->bUseAsOccluder),
	bAllowApproximateOcclusion(InComponent->bAllowApproximateOcclusion),
	bNeedsStaticMeshUpdate(FALSE),
	Bounds(InComponent->Bounds),
	CullDistance(InComponent->CachedCullDistance),
	LightingChannels(InComponent->LightingChannels),
	LightEnvironmentSceneInfo(
		(InComponent->LightEnvironment && InComponent->LightEnvironment->bEnabled) ?
			InComponent->LightEnvironment->SceneInfo :
			NULL
		),
	LevelName(InComponent->GetOutermost()->GetFName()),
	LightList(NULL),
	UpperSkyLightColor(FLinearColor::Black),
	LowerSkyLightColor(FLinearColor::Black),
	ShadowParent(InComponent->ShadowParent ? InComponent->ShadowParent->SceneInfo : NULL),
	FirstShadowChild(NULL),
	NextShadowChild(NULL),
	FogVolumeSceneInfo(NULL),
	LastRenderTime(0.0f)
{
	check(Component);
	check(Proxy);

	InComponent->SceneInfo = this;
	Proxy->PrimitiveSceneInfo = this;

	// Replace 0 culldistance with WORLD_MAX so we don't have to add any special case handling later.
	if( CullDistance == 0 )
	{
		CullDistance = WORLD_MAX;
	}

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
	}

	/**
	 * An implementation of FStaticPrimitiveDrawInterface that stores the drawn elements for the rendering thread to use.
	 */
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
			FStaticMesh* StaticMesh = new(PrimitiveSceneInfo->StaticMeshes) FStaticMesh(
				PrimitiveSceneInfo,
				Mesh,
				Square(MinDrawDistance),
				Square(MaxDrawDistance),
				CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId()
				);
		}

	private:
		FPrimitiveSceneInfo* PrimitiveSceneInfo;
		TRefCountPtr<HHitProxy> CurrentHitProxy;
	};

	// Cache the primitive's static mesh elements.
	FBatchingSPDI BatchingSPDI(this);
	BatchingSPDI.SetHitProxy(DefaultHitProxy);
	Proxy->DrawStaticElements(&BatchingSPDI);
	StaticMeshes.Shrink();
}

void FPrimitiveSceneInfo::AddToScene()
{
	check(IsInRenderingThread());

	for(INT MeshIndex = 0;MeshIndex < StaticMeshes.Num();MeshIndex++)
	{
		// Add the static mesh to the scene's static mesh list.
		TSparseArrayAllocationInfo<FStaticMesh*> SceneArrayAllocation = Scene->StaticMeshes.Add();
		Scene->StaticMeshes(SceneArrayAllocation.Index) = &StaticMeshes(MeshIndex);
		StaticMeshes(MeshIndex).Id = SceneArrayAllocation.Index;

		// Add the static mesh to the appropriate draw lists.
		StaticMeshes(MeshIndex).AddToDrawLists(Scene);
	}

	// Construct a compact scene info to pass to the light attachment code.
	FPrimitiveSceneInfoCompact CompactPrimitiveSceneInfo(this);

	if(LightEnvironmentSceneInfo)
	{
		// For primitives in a light environment, only attach it to lights which are in the same light environment.
		for(INT LightIndex = 0;LightIndex < LightEnvironmentSceneInfo->AttachedLights.Num();LightIndex++)
		{
			FLightSceneInfo* LightSceneInfo = LightEnvironmentSceneInfo->AttachedLights(LightIndex);
			if(LightSceneInfo->AffectsPrimitive(CompactPrimitiveSceneInfo))
			{
				FLightPrimitiveInteraction::Create(LightSceneInfo,this);
			}
		}
	}
	else
	{	
		// For primitives which aren't in a light environment, check for interactions with all lights that have matching lighting channels.
		for(TDynamicMap<FLightingChannelContainer,TArray<FLightSceneInfo*> >::TConstIterator LightingChannelIt(Scene->LightingChannelLightGroupMap);
			LightingChannelIt;++
			LightingChannelIt
			)
		{
			if(LightingChannelIt.Key().OverlapsWith(LightingChannels))
			{
				for(INT LightIndex = 0;LightIndex < LightingChannelIt.Value().Num();LightIndex++)
				{
					FLightSceneInfo* LightSceneInfo = LightingChannelIt.Value()(LightIndex);
					if(LightSceneInfo->AffectsPrimitive(CompactPrimitiveSceneInfo))
					{
						FLightPrimitiveInteraction::Create(LightSceneInfo,this);
					}
				}
			}
		}
	}
}

void FPrimitiveSceneInfo::RemoveFromScene()
{
	check(IsInRenderingThread());

	// Detach all lights.
	while(LightList)
	{
		delete LightList;
	};

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
	}
}

void FPrimitiveSceneInfo::BeginDeferredUpdateStaticMeshes()
{
	// Set a flag which causes InitViews to update the static meshes the next time the primitive is visible.
	bNeedsStaticMeshUpdate = TRUE;
}

void FPrimitiveSceneInfo::LinkShadowParent()
{
	if(ShadowParent)
	{
		NextShadowChild = ShadowParent->FirstShadowChild;
		ShadowParent->FirstShadowChild = this;
	}
}

void FPrimitiveSceneInfo::UnlinkShadowParent()
{
	// Remove the primitive from its shadow parent's linked list of children.
	if(ShadowParent)
	{
		FPrimitiveSceneInfo** ShadowChildPtr = &ShadowParent->FirstShadowChild;
		while(*ShadowChildPtr && *ShadowChildPtr != this)
			ShadowChildPtr = &(*ShadowChildPtr)->NextShadowChild;
		*ShadowChildPtr = NextShadowChild;
		NextShadowChild = NULL;
	}

	// Disassociate shadow children.
	FPrimitiveSceneInfo** ShadowChildPtr = &FirstShadowChild;
	while(*ShadowChildPtr)
	{
		FPrimitiveSceneInfo** PrevShadowChildLink = ShadowChildPtr;
		(*PrevShadowChildLink)->ShadowParent = NULL;
		ShadowChildPtr = &(*ShadowChildPtr)->NextShadowChild;
		*PrevShadowChildLink = NULL;
	};
}

//
void FPrimitiveSceneInfo::FinishCleanup()
{
	delete this;
}

/*-----------------------------------------------------------------------------
FCaptureSceneInfo
-----------------------------------------------------------------------------*/

/** 
* Constructor 
* @param InComponent - mirrored scene capture component requesting the capture
* @param InSceneCaptureProbe - new probe for capturing the scene
*/
FCaptureSceneInfo::FCaptureSceneInfo(USceneCaptureComponent* InComponent,FSceneCaptureProbe* InSceneCaptureProbe)
:	SceneCaptureProbe(InSceneCaptureProbe)
,	Component(InComponent)
,	Id(INDEX_NONE)
,	Scene(NULL)
{
	check(Component);
	check(SceneCaptureProbe);

	InComponent->CaptureInfo = this;	
}

/** 
* Destructor
*/
FCaptureSceneInfo::~FCaptureSceneInfo()
{
	RemoveFromScene(Scene);
	delete SceneCaptureProbe;
}

/**
* Capture the scene
* @param SceneRenderer - original scene renderer so that we can match certain view settings
*/
void FCaptureSceneInfo::CaptureScene(class FSceneRenderer* SceneRenderer)
{
	if( SceneCaptureProbe )
	{
        SceneCaptureProbe->CaptureScene(SceneRenderer);
	}
}

/**
* Add this capture scene info to a scene 
* @param InScene - scene to add to
*/
void FCaptureSceneInfo::AddToScene(class FScene* InScene)
{
	check(InScene);

	// can only be active in a single scene
	RemoveFromScene(Scene);
	
	// add it to the scene and keep track of Id
	Scene = InScene;
	Id = Scene->SceneCaptures.AddItem(this);
	
}

/**
* Remove this capture scene info from a scene 
* @param InScene - scene to remove from
*/
void FCaptureSceneInfo::RemoveFromScene(class FScene* /*InScene*/)
{
	if( Scene &&
		Id != INDEX_NONE )
	{
		Scene->SceneCaptures.Remove(Id);
		Scene = NULL;
	}
}

/*-----------------------------------------------------------------------------
FStaticMesh
-----------------------------------------------------------------------------*/

void FStaticMesh::LinkDrawList(FStaticMesh::FDrawListElementLink* Link)
{
	check(IsInRenderingThread());
	check(!DrawListLinks.ContainsItem(Link));
	DrawListLinks.AddItem(Link);
}

void FStaticMesh::UnlinkDrawList(FStaticMesh::FDrawListElementLink* Link)
{
	check(IsInRenderingThread());
	verify(DrawListLinks.RemoveItem(Link) == 1);
}

void FStaticMesh::AddToDrawLists(FScene* Scene)
{
	// not all platforms need this
	const UBOOL bRequiresHitProxies = Scene->RequiresHitProxies();
	if ( bRequiresHitProxies )
	{
		// Add the static mesh to the DPG's hit proxy draw list.
		FHitProxyDrawingPolicyFactory::AddStaticMesh(Scene,this);
	}

	if(!IsTranslucent())
	{
		if(DepthPriorityGroup == SDPG_World)
		{
			// Add the static mesh to the depth-only draw list.
			FDepthDrawingPolicyFactory::AddStaticMesh(Scene,this);

			if ( !PrimitiveSceneInfo->bStaticShadowing )
			{
				FVelocityDrawingPolicyFactory::AddStaticMesh(Scene,this);
			}
		}

		// Add the static mesh to the DPG's base pass draw list.
		FBasePassOpaqueDrawingPolicyFactory::AddStaticMesh(Scene,this);
	}

	// Add the static mesh to the light draw lists for the primitive's light interactions.
	for(FLightPrimitiveInteraction* LightInteraction = PrimitiveSceneInfo->LightList;LightInteraction;LightInteraction = LightInteraction->GetNextLight())
	{
		FMeshLightingDrawingPolicyFactory::AddStaticMesh(
			Scene,
			this,
			LightInteraction->GetLight()
			);
	}
}

void FStaticMesh::RemoveFromDrawLists()
{
	// Remove the mesh from all draw lists.
	while(DrawListLinks.Num())
	{
		FStaticMesh::FDrawListElementLink* Link = DrawListLinks(0);
		const INT OriginalNumLinks = DrawListLinks.Num();
		// This will call UnlinkDrawList.
		Link->Remove();
		check(DrawListLinks.Num() == OriginalNumLinks - 1);
		if(DrawListLinks.Num())
		{
			check(DrawListLinks(0) != Link);
		}
	}
}

FStaticMesh::~FStaticMesh()
{
	// Remove this static mesh from the scene's list.
	PrimitiveSceneInfo->Scene->StaticMeshes.Remove(Id);
	RemoveFromDrawLists();
}

/*-----------------------------------------------------------------------------
FMeshDrawingPolicy
-----------------------------------------------------------------------------*/

FMeshDrawingPolicy::FMeshDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy,
	UBOOL bInOverrideWithShaderComplexity
	):
	VertexFactory(InVertexFactory),
	MaterialRenderProxy(InMaterialRenderProxy),
	bIsTwoSidedMaterial(InMaterialRenderProxy->GetMaterial()->IsTwoSided()),
	bIsWireframeMaterial(InMaterialRenderProxy->GetMaterial()->IsWireframe()),
	bNeedsBackfacePass(
		InMaterialRenderProxy->GetMaterial()->IsTwoSided() &&
		(InMaterialRenderProxy->GetMaterial()->GetLightingModel() != MLM_NonDirectional) && 
		(InMaterialRenderProxy->GetMaterial()->GetLightingModel() != MLM_Unlit)
		),
	//convert from signed UBOOL to unsigned BITFIELD
	bOverrideWithShaderComplexity(bInOverrideWithShaderComplexity != FALSE)
{
}

/** A logical exclusive or function. */
static UBOOL XOR(UBOOL A,UBOOL B)
{
	return (A && !B) || (!A && B);
}

//
void FMeshDrawingPolicy::SetMeshRenderState(
	FCommandContextRHI* Context,
	const FPrimitiveSceneInfo* PrimitiveSceneInfo,
	const FMeshElement& Mesh,
	UBOOL bBackFace,
	const ElementDataType& ElementData
	) const
{
	const FRasterizerStateInitializerRHI Initializer = {
		(Mesh.bWireframe || IsWireframe()) ? FM_Wireframe : FM_Solid,
		(IsTwoSided() && !NeedsBackfacePass()) ? CM_None :
			(XOR(bBackFace, Mesh.ReverseCulling||GInvertCullMode) ? CM_CCW : CM_CW),
		Mesh.DepthBias,
		Mesh.SlopeScaleDepthBias
	};
	RHISetRasterizerStateImmediate(Context, Initializer);
}

//
void FMeshDrawingPolicy::DrawMesh(FCommandContextRHI* Context,const FMeshElement& Mesh) const
{
#if !FINAL_RELEASE
	extern UBOOL GShowMaterialDrawEvents;
	if ( GShowMaterialDrawEvents )
	{
		SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS, *Mesh.MaterialRenderProxy->GetMaterial()->GetFriendlyName());
	}
#endif

	if (Mesh.UseDynamicData)
	{
		if (Mesh.ParticleType == PET_None)
		{
			check(Mesh.DynamicVertexData);

			if (Mesh.DynamicIndexData)
			{
				//@todo.SAS. This path has not been tested yet!
				RHIDrawIndexedPrimitiveUP(
					Context,
					Mesh.Type,
					Mesh.MinVertexIndex,
					Mesh.MaxVertexIndex - Mesh.MinVertexIndex + 1,
					Mesh.NumPrimitives,
					Mesh.DynamicIndexData,
					Mesh.DynamicIndexStride,
					Mesh.DynamicVertexData,
					Mesh.DynamicVertexStride
					);
			}
			else
			{
				RHIDrawPrimitiveUP(
					Context,
					Mesh.Type,
					Mesh.NumPrimitives,
					Mesh.DynamicVertexData,
					Mesh.DynamicVertexStride
					);
			}
		}
		else
		if (Mesh.ParticleType == PET_Sprite)
		{
			RHIDrawSpriteParticles(
				Context, 
				Mesh
				);
		}
		else
		if (Mesh.ParticleType == PET_SubUV)
		{
			RHIDrawSubUVParticles(
				Context, 
				Mesh
				);
		}
	}
	else
	{
		if(Mesh.IndexBuffer)
		{
			RHIDrawIndexedPrimitive(
				Context,
				Mesh.IndexBuffer->IndexBufferRHI,
				Mesh.Type,
				0,
				Mesh.MinVertexIndex,
				Mesh.MaxVertexIndex - Mesh.MinVertexIndex + 1,
				Mesh.FirstIndex,
				Mesh.NumPrimitives
				);
		}
		else
		{
			RHIDrawPrimitive(
				Context,
				Mesh.Type,
				Mesh.FirstIndex,
				Mesh.NumPrimitives
				);
		}
	}
}

void FMeshDrawingPolicy::DrawShared(FCommandContextRHI* Context,const FSceneView* View) const
{
	check(VertexFactory);
	VertexFactory->Set(Context);
}

/**
* Get the decl and stream strides for this mesh policy type and vertexfactory
* @param VertexDeclaration - output decl 
* @param StreamStrides - output array of vertex stream strides 
*/
void FMeshDrawingPolicy::GetVertexDeclarationInfo(FVertexDeclarationRHIParamRef &VertexDeclaration, DWORD *StreamStrides) const
{
	check(VertexFactory);
	VertexFactory->GetStreamStrides(StreamStrides);
	VertexDeclaration = VertexFactory->GetDeclaration();
}

/** Initialization constructor. */
FHeightFogSceneInfo::FHeightFogSceneInfo(const UHeightFogComponent* InComponent):
	Component(InComponent),
	Height(InComponent->Height),
	Density(InComponent->Density),
	LightColor(FLinearColor(InComponent->LightColor) * InComponent->LightBrightness),
	ExtinctionDistance(InComponent->ExtinctionDistance),
	StartDistance(InComponent->StartDistance)
{}
