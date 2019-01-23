/*=============================================================================
	SceneRendering.cpp: Scene rendering.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineDecalClasses.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"

/*-----------------------------------------------------------------------------
	Globals
-----------------------------------------------------------------------------*/

#if WITH_MOBILE_RHI
	/** Whether or not to use off-screen rendering to render targets */
	extern UBOOL GMobileUseOffscreenRendering;
#endif

/**
	This debug variable is toggled by the 'toggleocclusion' console command.
	Turning it on will still issue all queries but ignore the results, which
	makes it possible to analyze those queries in NvPerfHUD. It will also
	stabilize the succession of draw calls in a paused game.
*/
UBOOL GIgnoreAllOcclusionQueries = FALSE;

/**
	This debug variable is set by the 'FullMotionBlur [N]' console command.
	Setting N to -1 or leaving it blank will make the Motion Blur effect use the
	default setting (whatever the game chooses).
	N = 0 forces FullMotionBlur off.
	N = 1 forces FullMotionBlur on.
 */
INT GMotionBlurFullMotionBlur = -1;

// for documentation see "MotionBlurSoftEdge" console command
FLOAT GMotionBlurSoftEdge = -1;

// for documentation see "MotionBlurQuality" console command
INT GMotionBlurQuality = -1;

// for documentation see "MotionBlurMaxVelocity" console command
FLOAT GMotionBlurMaxVelocity = -1;

// for documentation see "MotionBlurAmount" console command
FLOAT GMotionBlurAmount = -1;

// for documentation see "DOFOcclusionTweak" console command
FLOAT GDOFOcclusionTweak = -1;

// for documentation see "BloomQuality" console command
INT GBloomQuality = -1;

// for documentation see "BloomSize" console command
FLOAT GBloomSize = -1;

// for documentation see "BloomScale" console command
FLOAT GBloomScale = -1;

// for documentation see "ImageGrain" console command
FLOAT GImageGrain = -1;

// This allows to deactivate updating the PreViewTranslation vector (0) for debugging purpose
INT GPreViewTranslation = 1;

/** The minimum projected screen radius for a primitive to be drawn in the depth pass, as a fraction of half the horizontal screen width. */
static const FLOAT DefaultMinScreenRadiusForDepthPrepass = 0.03f;
FLOAT GMinScreenRadiusForDepthPrepassSquared = Square(DefaultMinScreenRadiusForDepthPrepass);

/** 
 * The minimum projected screen radius for a primitive to be drawn in the depth pass when whole scene shadows are active. 
 * This threshold is smaller than MinScreenRadiusForDepthPrepass because culling objects from the depth only pass 
 * will have more obvious artifacts when whole scene shadows are in use.
 */
static const FLOAT DefaultMinScreenRadiusForDepthPrepassWithShadows = 0.02f;
FLOAT GMinScreenRadiusForDepthPrepassWithShadowsSquared = Square(DefaultMinScreenRadiusForDepthPrepassWithShadows);

/** The minimum projected screen radius for a primitive to be drawn in a shadow depth pass, as a fraction of half the horizontal screen width. */
static const FLOAT DefaultMinScreenRadiusForShadowDepth = 0.01f;
FLOAT GMinScreenRadiusForShadowDepthSquared = Square(DefaultMinScreenRadiusForShadowDepth);

/** Threshold below which objects in ortho wireframe views will be culled. */
FLOAT WireframeCullThreshold = 5.0f;

extern FLightMap2D* GDebugSelectedLightmap;
extern UPrimitiveComponent* GDebugSelectedComponent;

// 0=off, >0=texture id, changed by "VisualizeTexture" console command, useful for debugging
INT GVisualizeTexture = 0;
//
FLOAT GVisualizeTextureRGBMul = 1.0f;
//
FLOAT GVisualizeTextureAMul = 0.0f;
// 0=view in left top, 1=whole texture, 2=view in left top with 1 texel border
INT GVisualizeTextureInputMapping = 0;

// bit 1: if 1, saturation mode, if 0, frac mode
INT GVisualizeTextureFlags = 0;

/*-----------------------------------------------------------------------------
	FViewInfo
-----------------------------------------------------------------------------*/

/** 
 * Initialization constructor. Passes all parameters to FSceneView constructor
 */
FViewInfo::FViewInfo(
	const FSceneViewFamily* InFamily,
	FSceneViewStateInterface* InState,
	INT InParentViewIndex,
	const FSceneViewFamily* InParentViewFamily,
	FSynchronizedActorVisibilityHistory* InActorVisibilityHistory,
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
	const TSet<UPrimitiveComponent*>& InHiddenPrimitives,
	FLOAT InLODDistanceFactor
	)
	:	FSceneView(
			InFamily,
			InState,
			InParentViewIndex,
			InParentViewFamily,
			InActorVisibilityHistory,
			InViewActor,
			InPostProcessChain,
			InPostProcessSettings,
			InDrawer,
			InX,
			InY,
			InSizeX,
			InSizeY,
			InViewMatrix,
			InProjectionMatrix,
			InBackgroundColor,
			InOverlayColor,
			InColorScale,
			InHiddenPrimitives,
			InLODDistanceFactor
			)
	,	bRenderExponentialFog( FALSE )
	,	bRequiresVelocities( FALSE )
	,	bRequiresPrevTransforms( FALSE )
	,	bPrevTransformsReset( FALSE )
	,	bIgnoreExistingQueries( FALSE )
	,	bDisableQuerySubmissions( FALSE )
	,	bOneLayerHeightFogRenderedInAO( FALSE )
	,	bDisableDistanceBasedFadeTransitions( FALSE )
	,	NumVisibleStaticMeshElements(0)
	,	NumVisibleDynamicPrimitives(0)
	,	PrecomputedVisibilityData(NULL)
	,	IndividualOcclusionQueries((FSceneViewState*)InState,1)
	,	GroupedOcclusionQueries((FSceneViewState*)InState,FOcclusionQueryBatcher::OccludedPrimitiveQueryBatchSize)
	,	ActorVisibilitySet(NULL)
{
	PrevViewProjMatrix.SetIdentity();
	PrevViewRotationProjMatrix.SetIdentity();
	PrevViewOrigin = FVector(0,0,0);

	// Clear fog constants.
	for(INT LayerIndex = 0; LayerIndex < ARRAY_COUNT(FogDistanceScale); LayerIndex++)
	{
		FogMinHeight[LayerIndex] = FogMaxHeight[LayerIndex] = FogDistanceScale[LayerIndex] = 0.0f;
		FogStartDistance[LayerIndex] = 0.f;
		FogInScattering[LayerIndex] = FLinearColor::Black;
		FogExtinctionDistance[LayerIndex] = FLT_MAX;
	}

	ExponentialFogParameters = FVector(0,0,0);
	ExponentialFogColor = FVector(0,0,0);
	LightInscatteringColor = FVector(0,0,0);
	DominantDirectionalLightDirection = FVector(0,0,0);
	FogMaxOpacity = 1;
}

/** 
* Add FPostProcessSceneProxy and update some internals. 
* @param InProxy can be 0
*/
void FViewInfo::AddPostProcessProxy(FPostProcessSceneProxy* InProxy)
{
	if(InProxy)
	{
		PostProcessSceneProxies.AddItem(InProxy);
		bRequiresVelocities = bRequiresVelocities || InProxy->RequiresVelocities( MotionBlurParams );
		bRequiresPrevTransforms = bRequiresPrevTransforms || InProxy->RequiresPreviousTransforms(*this);
		InProxy->ComputeDOFParams(*this, DepthOfFieldParams);
	}
}

/** 
 * Initialization constructor. 
 * @param InView - copy to init with
 */
FViewInfo::FViewInfo(const FSceneView* InView)
	:	FSceneView(*InView)
	,	bHasTranslucentViewMeshElements( 0 )
	,	bHasDistortionViewMeshElements( 0 )
	,	bRenderExponentialFog( FALSE )
	,	bRequiresVelocities( FALSE )
	,	bRequiresPrevTransforms( FALSE )
	,	bPrevTransformsReset( FALSE )
	,	bIgnoreExistingQueries( FALSE )
	,	bDisableQuerySubmissions( FALSE )
	,	bOneLayerHeightFogRenderedInAO( FALSE )
	,	bDisableDistanceBasedFadeTransitions( FALSE )
	,	bRenderedToDoFBlurBuffer( FALSE )
	,	NumVisibleStaticMeshElements(0)
	,	NumVisibleDynamicPrimitives(0)
	,	PrecomputedVisibilityData(NULL)
	,	IndividualOcclusionQueries((FSceneViewState*)InView->State,1)
	,	GroupedOcclusionQueries((FSceneViewState*)InView->State,FOcclusionQueryBatcher::OccludedPrimitiveQueryBatchSize)
	,	ActorVisibilitySet(NULL)
{
	PrevViewProjMatrix.SetIdentity();
	PrevViewRotationProjMatrix.SetIdentity();
	PrevViewOrigin = FVector(0,0,0);

	// Clear fog constants.
	for(INT LayerIndex = 0; LayerIndex < ARRAY_COUNT(FogDistanceScale); LayerIndex++)
	{
		FogMinHeight[LayerIndex] = FogMaxHeight[LayerIndex] = FogDistanceScale[LayerIndex] = 0.0f;
		FogStartDistance[LayerIndex] = 0.f;
		FogInScattering[LayerIndex] = FLinearColor::Black;
		FogExtinctionDistance[LayerIndex] = FLT_MAX;
	}

	ExponentialFogParameters = FVector(0,0,0);
	ExponentialFogColor = FVector(0,0,0);
	LightInscatteringColor = FVector(0,0,0);
	DominantDirectionalLightDirection = FVector(0,0,0);
	FogMaxOpacity = 1;

	// If there were post-process scene proxies already created, just 
	// use those instead of creating them via the post-process chain.
	if( InView->PostProcessSceneProxies.Num() )
	{
		for( INT ProxyIndex = 0; ProxyIndex < InView->PostProcessSceneProxies.Num(); ProxyIndex++ )
		{
			FPostProcessSceneProxy* PostProcessSceneProxy = InView->PostProcessSceneProxies(ProxyIndex);

			AddPostProcessProxy(PostProcessSceneProxy);
		}
	}
	else if( PostProcessChain )
	{
		// create render proxies for any post process effects in this view
		for( INT EffectIdx=0; EffectIdx < PostProcessChain->Effects.Num(); EffectIdx++ )
		{
			UPostProcessEffect* Effect = PostProcessChain->Effects(EffectIdx);

			// only add a render proxy if the effect is enabled
			if( Effect && Effect->IsShown(InView) )
			{
				FPostProcessSceneProxy* PostProcessSceneProxy = Effect->CreateSceneProxy(
					PostProcessSettings && Effect->bUseWorldSettings ? PostProcessSettings : NULL
					);

				AddPostProcessProxy(PostProcessSceneProxy);
			}
		}
	}

	if( PostProcessSceneProxies.Num() )
	{
		// Mark the final post-processing effect so that we can render it directly to the view render target.
		UINT DPGIndex = SDPG_PostProcess;
		INT FinalIdx = -1;
		for( INT ProxyIdx=0; ProxyIdx < PostProcessSceneProxies.Num(); ProxyIdx++ )
		{
			if( PostProcessSceneProxies(ProxyIdx)->GetDepthPriorityGroup() == DPGIndex
				&& !PostProcessSceneProxies(ProxyIdx)->GetAffectsLightingOnly())
			{
				FinalIdx = ProxyIdx;
				PostProcessSceneProxies(FinalIdx)->TerminatesPostProcessChain( FALSE );
			}
		}
		if (FinalIdx != -1)
		{
			PostProcessSceneProxies(FinalIdx)->TerminatesPostProcessChain( TRUE );
		}
	}
}

/** 
 * Destructor. 
 */
FViewInfo::~FViewInfo()
{
	for(INT ResourceIndex = 0;ResourceIndex < DynamicResources.Num();ResourceIndex++)
	{
		DynamicResources(ResourceIndex)->ReleasePrimitiveResource();
	}

	for(INT PostProcessIndex = 0; PostProcessIndex < PostProcessSceneProxies.Num(); PostProcessIndex++)
	{
		delete PostProcessSceneProxies(PostProcessIndex);
	}
}

/** 
 * Initializes the dynamic resources used by this view's elements. 
 */
void FViewInfo::InitDynamicResources()
{
	for(INT ResourceIndex = 0;ResourceIndex < DynamicResources.Num();ResourceIndex++)
	{
		DynamicResources(ResourceIndex)->InitPrimitiveResource();
	}
}


/*-----------------------------------------------------------------------------
FSceneRenderer
-----------------------------------------------------------------------------*/

FSceneRenderer::FSceneRenderer(const FSceneViewFamily* InViewFamily,FHitProxyConsumer* HitProxyConsumer,const FMatrix& InCanvasTransform,UBOOL bInIsSceneCapture)
:	Scene(InViewFamily->Scene ? InViewFamily->Scene->GetRenderScene() : NULL)
,	ViewFamily(*InViewFamily)
,	CanvasTransform(InCanvasTransform)
,	MinViewDistanceSquaredOverride(0.0f)
,	MaxViewDistanceSquaredOverride(MAX_FLT)
,	FrameNumber(GFrameNumber)
,	bDominantShadowsActive(FALSE)
,	bIsSceneCapture(bInIsSceneCapture)
,	PreviousFrameTime(0)
{
	// Copy the individual views.
	Views.Empty(InViewFamily->Views.Num());
	for(INT ViewIndex = 0;ViewIndex < InViewFamily->Views.Num();ViewIndex++)
	{
#if !FINAL_RELEASE
		for(INT ViewIndex2 = 0;ViewIndex2 < InViewFamily->Views.Num();ViewIndex2++)
		{
			if (ViewIndex != ViewIndex2 && InViewFamily->Views(ViewIndex)->State != NULL)
			{
				// Verify that each view has a unique view state, as the occlusion query mechanism depends on it.
				check(InViewFamily->Views(ViewIndex)->State != InViewFamily->Views(ViewIndex2)->State);
			}
		}
#endif
		// Construct a FViewInfo with the FSceneView properties.
		FViewInfo* ViewInfo = new(Views) FViewInfo(InViewFamily->Views(ViewIndex));
		ViewFamily.Views(ViewIndex) = ViewInfo;
		ViewInfo->Family = &ViewFamily;


#if !CONSOLE
		// Should we allow the user to select translucent primitives?
		ViewInfo->bAllowTranslucentPrimitivesInHitProxy =
			GEngine->AllowSelectTranslucent() ||		// User preference enabled?
			ViewInfo->ViewOrigin.W == 0.0f;				// Is orthographic view?
#endif


		// Batch the view's elements for later rendering.
		if(ViewInfo->Drawer)
		{
			FViewElementPDI ViewElementPDI(ViewInfo,HitProxyConsumer);
			ViewInfo->Drawer->Draw(ViewInfo,&ViewElementPDI);
		}
	}
	
	if(HitProxyConsumer)
	{
		// Set the hit proxies show flag.
		ViewFamily.ShowFlags |= SHOW_HitProxies;
	}

	// Calculate the screen extents of the view family.
	UBOOL bInitializedExtents = FALSE;
	FLOAT MinFamilyX = 0;
	FLOAT MinFamilyY = 0;
	FLOAT MaxFamilyX = 0;
	FLOAT MaxFamilyY = 0;
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FSceneView* View = &Views(ViewIndex);
		if(!bInitializedExtents)
		{
			MinFamilyX = View->X;
			MinFamilyY = View->Y;
			MaxFamilyX = View->X + View->SizeX;
			MaxFamilyY = View->Y + View->SizeY;
			bInitializedExtents = TRUE;
		}
		else
		{
			MinFamilyX = Min(MinFamilyX,View->X);
			MinFamilyY = Min(MinFamilyY,View->Y);
			MaxFamilyX = Max(MaxFamilyX,View->X + View->SizeX);
			MaxFamilyY = Max(MaxFamilyY,View->Y + View->SizeY);
		}
	}
	FamilySizeX = appTrunc(MaxFamilyX - MinFamilyX);
	FamilySizeY = appTrunc(MaxFamilyY - MinFamilyY);

	// Allocate the render target space to the views.
	check(bInitializedExtents);
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		FViewInfo& View = Views(ViewIndex);
		View.RenderTargetX = appTrunc(View.X - MinFamilyX);
		View.RenderTargetY = appTrunc(View.Y - MinFamilyY);
		View.RenderTargetSizeX = Min<INT>(appTrunc(View.SizeX),ViewFamily.RenderTarget->GetSizeX());
		View.RenderTargetSizeY = Min<INT>(appTrunc(View.SizeY),ViewFamily.RenderTarget->GetSizeY());

		// Set the vector used by shaders to convert projection-space coordinates to texture space.
		View.ScreenPositionScaleBias =
			FVector4(
				View.SizeX / GSceneRenderTargets.GetBufferSizeX() / +2.0f,
				View.SizeY / GSceneRenderTargets.GetBufferSizeY() / -2.0f,
				(View.SizeY / 2.0f + GPixelCenterOffset + View.RenderTargetY) / GSceneRenderTargets.GetBufferSizeY(),
				(View.SizeX / 2.0f + GPixelCenterOffset + View.RenderTargetX) / GSceneRenderTargets.GetBufferSizeX()
				);
	}

	// copy off the requests
	// (I apologize for the const_cast, but didn't seem worth refactoring just for the freezerendering command)
	bHasRequestedToggleFreeze = const_cast<FRenderTarget*>(InViewFamily->RenderTarget)->HasToggleFreezeCommand();


#if USE_GXM_RHI
	// No need on the tiled device
	// @todo ngp clean: Should we make this a GRHIUseDepthPass variable or something, for all platforms?
	bUseDepthOnlyPass = FALSE;
#else
	if( GUsingES2RHI )
	{
		bUseDepthOnlyPass = FALSE;
	}
	else
	{
#if CONSOLE
		bUseDepthOnlyPass = TRUE;
#else
		bUseDepthOnlyPass = 
			// Use a depth only pass if we are using full blown directional lightmaps
			// Otherwise base pass pixel shaders will be cheap and there will be little benefit to rendering a depth only pass
			GSystemSettings.bAllowDirectionalLightMaps 
			&& (ViewFamily.ShowFlags & SHOW_Lighting) 
			// When using deferred shading, the base pass is just populating the G Buffers and not doing any heavy shading
			&& (!ShouldUseDeferredShading() || (ViewFamily.ShowFlags & SHOW_ShaderComplexity)) ;
#endif
	}
#endif
}

/**
 * Helper structure for sorted shadow stats.
 */
struct FSortedShadowStats
{
	/** Light/ primitive interaction.												*/
	FLightPrimitiveInteraction* Interaction;
	/** Shadow stat.																*/
	FCombinedShadowStats		ShadowStat;
};

/** Comparison operator used by sort. Sorts by light and then by pass number.	*/
IMPLEMENT_COMPARE_CONSTREF( FSortedShadowStats, SceneRendering,	{ if( B.Interaction->GetLightId() == A.Interaction->GetLightId() ) { return A.ShadowStat.ShadowPassNumber - B.ShadowStat.ShadowPassNumber; } else { return A.Interaction->GetLightId() - B.Interaction->GetLightId(); } } )

/**
 * Destructor, stringifying stats if stats gathering was enabled. 
 */
FSceneRenderer::~FSceneRenderer()
{
	if(Scene)
	{
		// Destruct the projected shadow infos.
		for(TSparseArray<FLightSceneInfoCompact>::TConstIterator LightIt(Scene->Lights);LightIt;++LightIt)
		{
			if( VisibleLightInfos.IsValidIndex(LightIt.GetIndex()) )
			{
				FVisibleLightInfo& VisibleLightInfo = VisibleLightInfos(LightIt.GetIndex());
				for(INT ShadowIndex = 0;ShadowIndex < VisibleLightInfo.MemStackProjectedShadows.Num();ShadowIndex++)
				{
					// FProjectedShadowInfo's in MemStackProjectedShadows were allocated on the rendering thread mem stack, 
					// Their memory will be freed when the stack is freed with no destructor call, so invoke the destructor explicitly
					VisibleLightInfo.MemStackProjectedShadows(ShadowIndex)->~FProjectedShadowInfo();
				}
			}
		}

		for(INT ShadowIndex = 0;ShadowIndex < PlanarReflectionShadows.Num();ShadowIndex++)
		{
			PlanarReflectionShadows(ShadowIndex).~FReflectionPlanarShadowInfo();
		}
	}
}

/**
 * Helper for InitViews to detect large camera movement, in both angle and position.
 */
static bool IsLargeCameraMovement(FSceneView& View, const FMatrix& PrevViewMatrix, const FVector& PrevViewOrigin, FLOAT CameraRotationThreshold, FLOAT CameraTranslationThreshold)
{
	FLOAT RotationThreshold = appCos(CameraRotationThreshold * PI / 180.0f);
	FLOAT ViewRightAngle = View.ViewMatrix.GetColumn(0) | PrevViewMatrix.GetColumn(0);
	FLOAT ViewUpAngle = View.ViewMatrix.GetColumn(1) | PrevViewMatrix.GetColumn(1);
	FLOAT ViewDirectionAngle = View.ViewMatrix.GetColumn(2) | PrevViewMatrix.GetColumn(2);

	FVector Distance = FVector(View.ViewOrigin) - PrevViewOrigin;
	return 
		ViewRightAngle < RotationThreshold ||
		ViewUpAngle < RotationThreshold ||
		ViewDirectionAngle < RotationThreshold ||
		Distance.SizeSquared() > CameraTranslationThreshold * CameraTranslationThreshold;
}

extern UINT GTilingMode;

/**
 * Helper used to set device viewports for Render*Pass
 */
void FSceneRenderer::ViewSetViewport(UINT ViewIndex, UBOOL bTiledRenderingPass, FLOAT ZNear, FLOAT ZFar)
{
	FViewInfo &View = Views(ViewIndex);

	RHISetViewport(View.RenderTargetX,View.RenderTargetY, ZNear,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY, ZFar);
	
	if( GUseTilingCode &&
		bTiledRenderingPass )
	{
		RHIMSAAFixViewport();
	}
}

/**
 * Wrapper for performing occlusion queries on a CompactPrimitiveSceneInfo object
 *
 */
class FCompactPrimitiveSceneInfoOcclusionWrapper
{
protected:
	const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo;
public:
	FCompactPrimitiveSceneInfoOcclusionWrapper(const FPrimitiveSceneInfoCompact& InCompactPrimitiveSceneInfo)
		: CompactPrimitiveSceneInfo(InCompactPrimitiveSceneInfo)
	{
	}

	inline UPrimitiveComponent* GetComponent() const
	{
		return CompactPrimitiveSceneInfo.Component;
	}

	inline INT GetVisibilityId() const
	{
		return CompactPrimitiveSceneInfo.VisibilityId;
	}

	inline UBOOL IgnoresNearPlaneIntersection() const
	{
		return CompactPrimitiveSceneInfo.bIgnoreNearPlaneIntersection;
	}

	inline UBOOL AllowsApproximateOcclusion() const
	{
		return CompactPrimitiveSceneInfo.bAllowApproximateOcclusion;
	}

	inline UBOOL IsOccludable(FViewInfo& View) const
	{
		const UINT DepthPriorityGroup =
			CompactPrimitiveSceneInfo.bHasViewDependentDPG
			? CompactPrimitiveSceneInfo.Proxy->GetDepthPriorityGroup(&View)
			: CompactPrimitiveSceneInfo.StaticDepthPriorityGroup;
		if(DepthPriorityGroup != SDPG_World)
		{
		// Only allow primitives that are in the world DPG to be occluded.
			return FALSE;
		}
		else if(View.Family->ShowFlags & SHOW_Wireframe)
		{
			// Don't bother with occlusion culling in wireframe views.
			return FALSE;
		}
		else
		{
			return CompactPrimitiveSceneInfo.Proxy->RequiresOcclusion(&View);
		}
	}

	inline FLOAT PixelPercentageOnFirstFrame() const
	{
		return CompactPrimitiveSceneInfo.bFirstFrameOcclusion ? 0.0f : GEngine->MaxOcclusionPixelsFraction;
	}

	inline FBoxSphereBounds GetOccluderBounds() const
	{
		return (CompactPrimitiveSceneInfo.bHasCustomOcclusionBounds == FALSE) ? 
			CompactPrimitiveSceneInfo.Bounds : 
		CompactPrimitiveSceneInfo.PrimitiveSceneInfo->Proxy->GetCustomOcclusionBounds();
	}

	/** Whether this primitive relies on occlusion queries for the way it looks, as opposed to just using occlusion for culling. */
	inline UBOOL RequiresOcclusionForCorrectness() const
	{
		return CompactPrimitiveSceneInfo.PrimitiveSceneInfo->Proxy->RequiresOcclusionForCorrectness();
	}
};

/**
 * Performs culling for one primitive, and returns what views it was drawn in
 *
 * @param CompactPrimitiveSceneInfo The primitive to process
 * @param ValidViews Which views to process this primitive for
 * @param Depth Current hierarchy depth (0 for root primitives)
 *
 * @return The views that we want to render the children primitives in
 */
BYTE FSceneRenderer::ProcessPrimitiveCullingInner(const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo, BYTE ValidViews, INT Depth )
{
	UBOOL bNeedsPreRenderView = FALSE;
	TBitArray<> ViewVisibilityMap(FALSE, Views.Num());

	// this will collect which views need to have children of this primitive rendered
	BYTE AddChildrenForViews = 0;


	// we use a moving bit here to avoid (1 << ViewIndex) which will generate microcode on consoles and stall the pipeline
	INT ViewBit = 1;
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++, ViewBit <<= 1)
	{
		FViewInfo& View = Views(ViewIndex);

#if USE_MASSIVE_LOD
		// is this view used for this primitive list
		const UBOOL bHiddenByParent = ((ValidViews & ViewBit) == 0);

		// mark that this is a parent and it was processed for culling/visibility in this view
		// doesn't matter if it will be visible or not
		if (CompactPrimitiveSceneInfo.ChildPrimitives.Num() > 0)
		{
			View.PrimitiveParentProcessedMap(CompactPrimitiveSceneInfo.PrimitiveSceneInfo->Id) = TRUE;
		}
#endif

		FSceneViewState* ViewState = (FSceneViewState*)View.State;
#if !FINAL_RELEASE
		UBOOL bIsVisibilityPredetermined = FALSE;
		if( ViewState )
		{
#if !CONSOLE
			// For visibility child views, check if the primitive was visible in the parent view.
			const FSceneViewState* const ViewParent = (FSceneViewState*)ViewState->GetViewParent();
			if(ViewParent)
			{
				bIsVisibilityPredetermined = TRUE;
				if( !ViewParent->ParentPrimitives.Contains(CompactPrimitiveSceneInfo.Component) )
				{
					continue;
				}
			}
#endif

			// For views with frozen visibility, check if the primitive is in the frozen visibility set.
			if(ViewState->bIsFrozen)
			{
				bIsVisibilityPredetermined = TRUE;
				if( !ViewState->FrozenPrimitives.Contains(CompactPrimitiveSceneInfo.Component) )
				{
					continue;
				}
			}
		}
#endif

		// Distance to camera in perspective viewports.
		FLOAT DistanceSquared = 0.0f;

		CONSOLE_PREFETCH(&CompactPrimitiveSceneInfo.PrimitiveSceneInfo->StaticMeshes);


		// consoles never need to have alternate ViewOrigin coming from an override, and it will always have a W origin
		// since no Ortho views on console
#if CONSOLE
#define ViewOriginForDistance View.ViewOrigin
#else
		// @todo: test for faster way then a reference (would a copy be faster?) remember, we're potentially dealing with 10's of thousands of prims
		const FVector4& ViewOriginForDistance = View.ViewOrigin.W > 0.0f ? View.ViewOrigin : View.OverrideLODViewOrigin;

		// Cull primitives based on distance to camera in perspective viewports and calculate distance, also used later for static
		// mesh elements.
		if (ViewOriginForDistance.W > 0.0f)
#endif
		{	
			// Compute the distance between the view and the primitive.
			// Note: This distance calculation must match up with primitive types that have LODs and use the static draw lists! 
			// (FStaticMeshSceneProxy::GetLOD, FSpeedTreeSceneProxy::ConditionalDrawElement)
#if CULL_DISTANCE_TO_BOUNDS
			DistanceSquared = CompactPrimitiveSceneInfo.Bounds.GetBox().ComputeSquaredDistanceToPoint(ViewOriginForDistance);
#else
			DistanceSquared = (CompactPrimitiveSceneInfo.Bounds.Origin - ViewOriginForDistance).SizeSquared();
#endif

			// Cull the primitive if it is further from the view origin than its max draw distance
			const UBOOL bIsDistanceCulled =
#if !FINAL_RELEASE
				!bIsVisibilityPredetermined &&
#endif
				( DistanceSquared > Min( CompactPrimitiveSceneInfo.MaxDrawDistanceSquared, MaxViewDistanceSquaredOverride ) );

			// Cull the primitive if it is closer to the view origin than its min draw distance
			const UBOOL bIsMinDistanceCulled =
#if !FINAL_RELEASE
				!bIsVisibilityPredetermined &&
#endif
				( DistanceSquared < Max( CompactPrimitiveSceneInfo.MinDrawDistanceSquared, MinViewDistanceSquaredOverride ) );

#if USE_MASSIVE_LOD
			// Cull the primitive if it is closer than its MassiveLOD draw distance, and it has children (otherwise always draw it)
			UBOOL bIsMassiveLODCulled =
#if !FINAL_RELEASE
				!bIsVisibilityPredetermined &&
#endif
#if !CONSOLE
				bPerformMinDistanceChecks && 
#endif
				!View.bForceLowestMassiveLOD &&
				DistanceSquared < CompactPrimitiveSceneInfo.MassiveLODDistanceSquared &&
				CompactPrimitiveSceneInfo.ChildPrimitives.Num();
#endif // MASSIVE_LOD

			// Determine if the primitive should be visible based on a variety of factors
			const UBOOL bShouldBeVisible =
				!bIsDistanceCulled &&
#if USE_MASSIVE_LOD
				!bHiddenByParent &&
				!bIsMassiveLODCulled &&
#endif
				!bIsMinDistanceCulled;


			// Are screen door fades enabled?  If so then we'll update fading state.
			UBOOL bIsStillFading = FALSE;
#if WITH_MOBILE_RHI
			if( !GUsingMobileRHI )		// We don't support screen door fades on mobile platforms yet
#endif
			{
				if( GAllowScreenDoorFade && View.State != NULL )
				{
					// Update distance-based visibility and fading state
					bIsStillFading = UpdatePrimitiveFading( View, Depth, bShouldBeVisible, CompactPrimitiveSceneInfo );
				}
			}


#if USE_MASSIVE_LOD
			// Whether or not we're fading out right now, if the object is within the massive LOD distance
			// we'll opt to draw it's LOD child objects (so they can start to fade in!)
			// Also: always draw children if mindist checks are disabled (SHOW_LOD)
			if ( bIsMassiveLODCulled
#if !CONSOLE
				|| !bPerformMinDistanceChecks 
#endif
				)
			{
				// since this won't be drawn, any LOD children should be attempted to draw in this view
				AddChildrenForViews |= ViewBit;
			}
#endif


			// If we're still fading then make sure the object is still drawn, even if it's beyond the max draw distance
			if( !bIsStillFading )
			{
#if STATS
				// Update stats
				{
					// If a parent object asked for it's children to not be rendered, then we'll bail out here
#if USE_MASSIVE_LOD
					if( !bHiddenByParent )
#endif
					{
						UBOOL bWasMassiveLODCulled = FALSE;
#if USE_MASSIVE_LOD
						// Cull the primitive if it is closer than its MassiveLOD draw distance, and it has children (otherwise always draw it)
						if ( bIsMassiveLODCulled )
						{
							// count MassiveLOD dropped primitives as a MinDrawDistance dropped primitive (almost identical)
							STAT(NumMinDrawDroppedPrimitives++);
							bWasMassiveLODCulled = TRUE;
						}
						else
						{
							STAT(NumLODDroppedPrimitives += CompactPrimitiveSceneInfo.NumberOfDescendents);
						}
#endif // MASSIVE_LOD
						if( !bWasMassiveLODCulled )
						{
							// Cull the primitive if it is further from the view origin than its max draw distance
							if (bIsDistanceCulled)
							{
								STAT(NumMaxDrawDroppedPrimitives++);
							}

							// Cull the primitive if it is further from the view origin than its max draw distance
							else if (bIsMinDistanceCulled)
							{
								STAT(NumMinDrawDroppedPrimitives++);
							}
						}
					}
				}
#endif

				// Cull the primitive if it shouldn't be visible right now
				if( !bShouldBeVisible )
				{
					// Skip to the next primitive!
					continue;
				}
			}
		}
#if !CONSOLE
		else if (View.Family->ShowFlags & SHOW_Wireframe)
		{
			// Cull small objects in wireframe in ortho views
			// This is important for performance in the editor because wireframe disables any kind of occlusion culling
			const UBOOL bDrawInWireframe = Max(View.ProjectionMatrix.M[0][0] * View.SizeX, View.ProjectionMatrix.M[1][1] * View.SizeY) * CompactPrimitiveSceneInfo.Bounds.SphereRadius > WireframeCullThreshold;
			if (!bDrawInWireframe)
			{
				continue;
			}
		}
#endif

		// Skip primitives which are in the view's HiddenPrimitives set.
		if(View.HiddenPrimitives.Find(CompactPrimitiveSceneInfo.Component))
		{
			continue;
		}

		// Check the primitive's bounding box against the view frustum.
		UBOOL bFullyContained;
		if(View.ViewFrustum.IntersectSphere(CompactPrimitiveSceneInfo.Bounds.Origin,CompactPrimitiveSceneInfo.Bounds.SphereRadius,bFullyContained))
		{
			// If not fully contained, try a more aggressive cull based on a tighter bounding volume
			if (bFullyContained || View.ViewFrustum.IntersectBox(CompactPrimitiveSceneInfo.Bounds.Origin,CompactPrimitiveSceneInfo.Bounds.BoxExtent))
			{
				FCompactPrimitiveSceneInfoOcclusionWrapper Primitive(CompactPrimitiveSceneInfo);
				// Check whether the primitive is occluded this frame.
				UBOOL bIsDefinitelyUnoccluded = ViewState ? FALSE : TRUE;
				const UBOOL bIsOccluded = ViewState 
					&& ViewState->UpdatePrimitiveOcclusion(Primitive,View,ViewFamily.CurrentRealTime,bIsSceneCapture,bIsDefinitelyUnoccluded);

				if(!bIsOccluded)
				{
					bNeedsPreRenderView |= ProcessVisible(ViewIndex, ViewVisibilityMap, CompactPrimitiveSceneInfo, DistanceSquared, bIsDefinitelyUnoccluded);
				}
				else
				{
					STAT(NumOccludedPrimitives++);
				}
			}
			else
			{
				STAT(NumCulledPrimitives++);
			}
		}
		else
		{
			STAT(NumCulledPrimitives++);
		}
	}

	// If the primitive's relevance indicated it needed a PreRenderView for any view, call it now.
	if (bNeedsPreRenderView)
	{
		CompactPrimitiveSceneInfo.Proxy->PreRenderView(&ViewFamily, ViewVisibilityMap, FrameNumber);
	}

	return AddChildrenForViews;
}



/**
 * Calls ProcessPrimitiveCullingInner for a list of primitives, and process any children
 *
 * @param Primitives List of primitives to process (along with their children)
 * @param NumPrimitives Number of primitives in Primitives
 * @param ValidViews Views for which to process these primitives
 * @param Depth Current hierarchy depth (0 for root primitives)
 */
void FSceneRenderer::ProcessPrimitiveCulling(const FPrimitiveSceneInfoCompact** Primitives, INT NumPrimitives, BYTE ValidViews, INT Depth )
{
	// loop over every primitive in the list
	for (INT PrimitiveIndex = 0; PrimitiveIndex < NumPrimitives; PrimitiveIndex++)
	{
		const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo = *Primitives[PrimitiveIndex];

		// if this primitive needs is not drawn and needs to draw its children in 1 or more views, track those views here
		// process just this primitive for all views (note that this is FORCELINLINED)
		INT AddChildrenForViews = ProcessPrimitiveCullingInner(CompactPrimitiveSceneInfo, ValidViews, Depth);

#if USE_MASSIVE_LOD
		// if this primitive was too far away in any view, add it's children to the list of primitives that need to 
		// potentially be drawn, for all views this was too far away in
		if (CompactPrimitiveSceneInfo.ChildPrimitives.Num() > 0)
		{
			const FPrimitiveSceneInfoCompact** ChildPrimitiveList = (const FPrimitiveSceneInfoCompact**)CompactPrimitiveSceneInfo.ChildPrimitives.GetTypedData();
			ProcessPrimitiveCulling(ChildPrimitiveList, CompactPrimitiveSceneInfo.ChildPrimitives.Num(), AddChildrenForViews, Depth + 1);
		}
#endif
	}
}



/**
 * Updates screen door fading for a primitive.  Called during the primitive culling phase.
 *
 * @param	View	View the primitive's visibility is being computed for
 * @param	Depth	Hierarchical primitive depth
 * @param	bShouldBeVisible	True if the culling phase has determined the primitive should be visible
 * @param	CompactPrimitiveSceneInfo       The primitive to process
 *
 * @return	True if the primitive is still fading in or out and should be rendered even if culled
 */
UBOOL FSceneRenderer::UpdatePrimitiveFading( FViewInfo& View, const INT Depth, const UBOOL bShouldBeVisible, const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo )
{
	UBOOL bIsStillFading = FALSE;

	FSceneViewState* SceneViewState = static_cast<FSceneViewState*>( View.State );
	checkSlow( SceneViewState != NULL );


	// Can set this to FALSE so hierarchical cross-dissolving LODs don't stop drawing unexpectedly for a few frames
	const UBOOL bAllowFadeInterrupts = TRUE;

	// Should we allow fading transitions at all this frame?  For frames where the camera moved
	// a large distance or where we haven't rendered a view in awhile, it's best to disable
	// fading so users don't see unexpected object transitions.
	const UBOOL bAllowTransitions = !View.bDisableDistanceBasedFadeTransitions;


	// Grab per-view visibility state for this primitive
	FSceneViewPrimitiveVisibilityState* PrimVisState = SceneViewState->PrimitiveVisibilityStates.Find( CompactPrimitiveSceneInfo.Component );
	if( PrimVisState == NULL )
	{
		// Primitive has no visibility state yet for this view.  Go ahead and add an entry for this primitive.
		FSceneViewPrimitiveVisibilityState NewState;
		NewState.bIsVisible = bShouldBeVisible;
		NewState.LODIndex = INDEX_NONE;
		PrimVisState = &SceneViewState->PrimitiveVisibilityStates.Set( CompactPrimitiveSceneInfo.Component, NewState );
	}


	// Grab the view-specific fading state for this component.  If we're not fading yet, we may
	// not even have any state to grab.  That's fine, we'll create it here if we need to!
	FSceneViewPrimitiveFadingState* FadingState = NULL;
	FadingState = SceneViewState->PrimitiveFadingStates.Find( CompactPrimitiveSceneInfo.Component );


	// Has visibility state changed since the last time we checked?
	if( PrimVisState->bIsVisible != bShouldBeVisible )
	{
		UBOOL bUpdateVisibleState = !bAllowTransitions;
		if( bAllowTransitions )
		{
			// Need to kick off a fade, so make sure that we have fading state for that
			if( FadingState == NULL )
			{
				// Primitive is not currently fading.  Start a new fade!
				FSceneViewPrimitiveFadingState NewState;
				NewState.FadingInLODIndex = INDEX_NONE;
				NewState.FadingOutLODIndex = INDEX_NONE;
				NewState.FadeOpacity = PrimVisState->bIsVisible ? 1.0f : 0.0f;
				NewState.TargetFadeOpacity = bShouldBeVisible ? 1.0f : 0.0f;

				// Use Normal screen door for primitives with even depths in the hierarchy, and an
				// Inverse screen door for odd depths
				NewState.ScreenDoorPattern = ( EScreenDoorPattern::Type )( Depth % 2 );

				FadingState = &SceneViewState->PrimitiveFadingStates.Set( CompactPrimitiveSceneInfo.Component, NewState );
				bUpdateVisibleState = TRUE;
			}
			else if( bAllowFadeInterrupts )
			{
				// Update the target opacity for this primitive
				FadingState->TargetFadeOpacity = bShouldBeVisible ? 1.0f : 0.0f;
				bUpdateVisibleState = TRUE;
			}
		}


		if( bUpdateVisibleState )
		{
			// Store the object's per-view "distance-based fading visibility" this frame.  We'll use this
			// information in later frames to decide whether or not we need to fade in or out.
			PrimVisState->bIsVisible = bShouldBeVisible;
		}
	}


	// Are we fading?
	if( FadingState != NULL )
	{
		if( bAllowTransitions )
		{
			// Fade speed in "fades per second" (or, one over fade duration)
			const FLOAT FadeSpeed = 2.0f;

			// Smoothly blend toward the target opacity
			const FLOAT DeltaTime = SceneViewState->LastRenderTimeDelta;
			if( FadingState->FadeOpacity < FadingState->TargetFadeOpacity )
			{
				// Fade in
				FadingState->FadeOpacity += DeltaTime * FadeSpeed;
				if( FadingState->FadeOpacity < FadingState->TargetFadeOpacity )
				{
					bIsStillFading = TRUE;
		}
	}
			else
			{
				// Fade out
				FadingState->FadeOpacity -= DeltaTime * FadeSpeed;
				if( FadingState->FadeOpacity > FadingState->TargetFadeOpacity )
				{
					bIsStillFading = TRUE;
				}
			}
		}

		if( !bIsStillFading )
		{
			// Remove entry in the fading state map for this primitive
			SceneViewState->PrimitiveFadingStates.RemoveKey( CompactPrimitiveSceneInfo.Component );
			FadingState = NULL;
		}
	}

	return bIsStillFading;
}

/** 
 * Kicks off a fade if the new LOD does not match the previous LOD that this primitive was rendered with. 
 * Returns an LOD that should be rendered instead of the new LOD level.
 */
SBYTE FSceneRenderer::UpdatePrimitiveLODUsed( FViewInfo& View, SBYTE NewLODIndex, const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo )
{
	SBYTE LODToRender = NewLODIndex;
	if (NewLODIndex != INDEX_NONE)
	{
		FSceneViewState* SceneViewState = static_cast<FSceneViewState*>( View.State );
		checkSlow( SceneViewState != NULL );

		// Grab per-view visibility state for this primitive
		FSceneViewPrimitiveVisibilityState* PrimVisState = SceneViewState->PrimitiveVisibilityStates.Find( CompactPrimitiveSceneInfo.Component );
		// Must exist because UpdatePrimitiveFading has already been called on this primitive
		check(PrimVisState);

		if (PrimVisState->LODIndex == INDEX_NONE)
		{
			// Update the primitive if it has never been rendered at a valid LOD
			PrimVisState->LODIndex = NewLODIndex;
		}
		// If the new LOD is different from the last LOD rendered with, start a new fade
		else if (NewLODIndex != PrimVisState->LODIndex)
		{		
			FSceneViewPrimitiveFadingState* FadingState = SceneViewState->PrimitiveFadingStates.Find( CompactPrimitiveSceneInfo.Component );

			const UBOOL bIsGame = (View.Family->ShowFlags & SHOW_Game) != 0;
			// Don't interrupt an existing fade
			if (!FadingState &&
				// Only start a fade if the primitive was rendered last frame, otherwise just immediately switch to the new LOD
				// This prevents objects from fading as soon as they come on screen if they transitioned off screen
				// Skip the check in the editor because the times are not dependable even with realtime enabled
				(!bIsGame || CompactPrimitiveSceneInfo.PrimitiveSceneInfo->LastRenderTime > (View.Family->CurrentWorldTime - View.Family->DeltaWorldTime) * (1.0f - DELTA)))
			{
				FSceneViewPrimitiveFadingState NewState;
				NewState.FadingInLODIndex = NewLODIndex;
				NewState.FadingOutLODIndex = PrimVisState->LODIndex;
				NewState.FadeOpacity = 1.0f;
				NewState.TargetFadeOpacity = 0.0f;
				NewState.ScreenDoorPattern = ( EScreenDoorPattern::Type )( NewLODIndex % 2 );
				SceneViewState->PrimitiveFadingStates.Set( CompactPrimitiveSceneInfo.Component, NewState );
				// Return the previous LOD since the transition will not start until next frame for primitives with static relevance.
				LODToRender = PrimVisState->LODIndex;
			}
			PrimVisState->LODIndex = NewLODIndex;
		}
	}
	return LODToRender;
}

/**
 * Performs view frustum culling.
 */
void FSceneRenderer::PerformViewFrustumCulling()
{
	// Cull the scene's primitives against the view frustum.
	STAT(NumOccludedPrimitives = 0);
	STAT(NumCulledPrimitives = 0);
	STAT(NumLODDroppedPrimitives = 0);
	STAT(NumMinDrawDroppedPrimitives = 0);
	STAT(NumMaxDrawDroppedPrimitives = 0);
	SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_PerformViewFrustumCullingTime, !bIsSceneCapture);

	// do we want min distance checks enabled? (they can make LOD parenting difficult if enabled)
#if !CONSOLE
	bPerformMinDistanceChecks = (ViewFamily.ShowFlags & SHOW_LOD) == SHOW_LOD;
#endif

	for(FScenePrimitiveOctree::TConstIterator<SceneRenderingAllocator> PrimitiveOctreeIt(Scene->PrimitiveOctree);
		PrimitiveOctreeIt.HasPendingNodes();
		PrimitiveOctreeIt.Advance())
	{
		const FScenePrimitiveOctree::FNode& PrimitiveOctreeNode = PrimitiveOctreeIt.GetCurrentNode();
		const FOctreeNodeContext& PrimitiveOctreeNodeContext = PrimitiveOctreeIt.GetCurrentContext();

		// Find children of this octree node that may contain relevant primitives.
		FOREACH_OCTREE_CHILD_NODE(ChildRef)
		{
			if(PrimitiveOctreeNode.HasChild(ChildRef))
			{
				// Check that the child node is in the frustum for at least one view.
				const FOctreeNodeContext ChildContext = PrimitiveOctreeNodeContext.GetChildContext(ChildRef);
				for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
				{
					FViewInfo& View = Views(ViewIndex);
					if(View.ViewFrustum.IntersectBox(ChildContext.Bounds.Center,ChildContext.Bounds.Extent))
					{		
						PrimitiveOctreeIt.PushChild(ChildRef);
						break;
					}
				}
			}
		}

		// Find the primitives in this node that are visible.
		for(FScenePrimitiveOctree::ElementConstIt NodePrimitiveIt(PrimitiveOctreeNode.GetElementIt());NodePrimitiveIt;++NodePrimitiveIt)
		{
			const FPrimitiveSceneInfoCompact& OctreeCompactPrimitiveSceneInfo = *NodePrimitiveIt;
			PREFETCH(&OctreeCompactPrimitiveSceneInfo);

			PREFETCH(OctreeCompactPrimitiveSceneInfo.PrimitiveSceneInfo);
			PREFETCH(OctreeCompactPrimitiveSceneInfo.Proxy);

			// for the octree change, we need to process this component and its children
			const FPrimitiveSceneInfoCompact* ComponentPointer = &OctreeCompactPrimitiveSceneInfo;
			// process this primitive and its children for all views
			ProcessPrimitiveCulling(&ComponentPointer, 1, 0xFF, 0);
		}
	}
	if (!bIsSceneCapture)
	{
		INC_DWORD_STAT_BY(STAT_OccludedPrimitives,NumOccludedPrimitives);
		INC_DWORD_STAT_BY(STAT_CulledPrimitives,NumCulledPrimitives);
		INC_DWORD_STAT_BY(STAT_LODDroppedPrimitives,NumLODDroppedPrimitives);
		INC_DWORD_STAT_BY(STAT_MaxDrawDroppedPrimitives,NumMaxDrawDroppedPrimitives);
		INC_DWORD_STAT_BY(STAT_MinDrawDroppedPrimitives,NumMinDrawDroppedPrimitives);
	}
}

/**
 * Initialize scene's views.
 * Check visibility, sort translucent items, etc.
 */
void FSceneRenderer::InitViews()
{
	SCOPED_DRAW_EVENT(InitViews)(DEC_SCENE_ITEMS,TEXT("InitViews"));

	SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_InitViewsTime, !bIsSceneCapture);
	SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_SceneCaptureInitViewsTime, bIsSceneCapture);

	if (GIsEditor)
	{
		for(TSparseArray<FLightSceneInfoCompact>::TIterator LightIt(Scene->Lights);LightIt;++LightIt)
		{
			FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
			LightSceneInfoCompact.LightSceneInfo->BeginSceneEditor();
		}

		// Draw lines to lights affecting this mesh if its selected.
		if (ViewFamily.ShowFlags & SHOW_LightInfluences)
		{
			for (TSparseArray<FPrimitiveSceneInfo*>::TConstIterator It(Scene->Primitives); It; ++It)
			{
				const FPrimitiveSceneInfo* PrimitiveSceneInfo = *It;
				if (PrimitiveSceneInfo->Proxy->IsSelected())
				{
					FLightPrimitiveInteraction *LightList = PrimitiveSceneInfo->LightList;
					while (LightList)
					{
						const FLightSceneInfo* LightSceneInfo = LightList->GetLight();

						// Don't count sky lights, since they're "free".
						if (LightSceneInfo->LightType != LightType_Sky && LightSceneInfo->LightType != LightType_SphericalHarmonic)
						{
							UBOOL bDynamic = TRUE;
							UBOOL bRelevant = FALSE;
							UBOOL bLightMapped = TRUE;
							PrimitiveSceneInfo->Proxy->GetLightRelevance(LightSceneInfo, bDynamic, bRelevant, bLightMapped);

							if (bRelevant)
							{
								// Draw blue for light-mapped lights and orange for dynamic lights
								const FColor LineColor = bLightMapped ? FColor(0,140,255) : FColor(255,140,0);
								for (INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
								{
									FViewInfo& View = Views(ViewIndex);
									FViewElementPDI LightInfluencesPDI(&View,NULL);
									LightInfluencesPDI.DrawLine(PrimitiveSceneInfo->Proxy->GetBounds().Origin, LightSceneInfo->LightToWorld.GetOrigin(), LineColor, SDPG_World);
								}
							}
						}
						LightList = LightList->GetNextLight();
					}
				}
			}
		}
	}

	// Setup motion blur parameters (also check for camera movement thresholds)
	PreviousFrameTime = ViewFamily.CurrentRealTime;
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		FViewInfo& View = Views(ViewIndex);
		View.FrameNumber = FrameNumber;
		FSceneViewState* ViewState = (FSceneViewState*) View.State;
		static UBOOL bEnableTimeScale = TRUE;

		// We can't use LatentOcclusionQueries when doing TiledScreenshot because in that case
		// 1-frame lag = 1-tile lag = clipped geometry, so we turn off occlusion queries
		// Occlusion culling is also disabled for hit proxy rendering.
		extern UBOOL GIsTiledScreenshot;
		const UBOOL bIsHitTesting = (ViewFamily.ShowFlags & SHOW_HitProxies) != 0;
		if (GIsTiledScreenshot || (GGameScreenshotCounter != 0) || GIgnoreAllOcclusionQueries || bIsHitTesting)
		{
			View.bDisableQuerySubmissions = TRUE;
			View.bIgnoreExistingQueries = TRUE;
		}

		if ( ViewState )
		{
			// determine if we are initializing or we should reset the persistent state
			const FLOAT DeltaTime = View.Family->CurrentRealTime - ViewState->LastRenderTime;
			const UBOOL bFirstFrameOrTimeWasReset = DeltaTime < -0.0001f || ViewState->LastRenderTime < 0.0001f;

			// detect conditions where we should reset occlusion queries
			if (bFirstFrameOrTimeWasReset || 
				ViewState->LastRenderTime + GEngine->PrimitiveProbablyVisibleTime < View.Family->CurrentRealTime ||
				IsLargeCameraMovement(
					View, 
				    ViewState->PrevViewMatrixForOcclusionQuery, 
				    ViewState->PrevViewOriginForOcclusionQuery, 
				    GEngine->CameraRotationThreshold, GEngine->CameraTranslationThreshold))
			{
				View.bIgnoreExistingQueries = TRUE;
				View.bDisableDistanceBasedFadeTransitions = TRUE;
			}
			ViewState->PrevViewMatrixForOcclusionQuery = View.ViewMatrix;
			ViewState->PrevViewOriginForOcclusionQuery = View.ViewOrigin;


			// detect conditions where we should reset motion blur 
			if (View.bRequiresPrevTransforms)
			{
				const UBOOL bDoCameraInterpolation = (View.Family->ShowFlags & SHOW_CameraInterpolation) != 0;

				if (bFirstFrameOrTimeWasReset ||
					!bDoCameraInterpolation ||
					IsLargeCameraMovement(View, 
					ViewState->PrevViewMatrix, ViewState->PrevViewOrigin, 
					View.MotionBlurParams.RotationThreshold, 
					View.MotionBlurParams.TranslationThreshold))
				{
					ViewState->PrevProjMatrix				= View.ProjectionMatrix;
					ViewState->PendingPrevProjMatrix		= View.ProjectionMatrix;
					ViewState->PrevViewMatrix				= View.ViewMatrix;
					ViewState->PendingPrevViewMatrix		= View.ViewMatrix;
					ViewState->PrevViewOrigin				= View.ViewOrigin;
					ViewState->PendingPrevViewOrigin		= View.ViewOrigin;
					ViewState->PrevPreViewTranslation		= View.PreViewTranslation;
					ViewState->PendingPrevPreViewTranslation= View.PreViewTranslation;

					// PT: If the motion blur shader is the last shader in the post-processing chain then it is the one that is
					//     adjusting for the viewport offset.  So it is always required and we can't just disable the work the
					//     shader does.  The correct fix would be to disable the effect when we don't need it and to properly mark
					//     the uber-postprocessing effect as the last effect in the chain.

					View.bPrevTransformsReset				= TRUE;
				}
				else
				{
					// New frame detected.
					ViewState->PrevViewMatrix				= ViewState->PendingPrevViewMatrix;
					ViewState->PrevViewOrigin				= ViewState->PendingPrevViewOrigin;
					ViewState->PrevProjMatrix				= ViewState->PendingPrevProjMatrix;
					ViewState->PrevPreViewTranslation		= ViewState->PendingPrevPreViewTranslation;
					ViewState->PendingPrevProjMatrix		= View.ProjectionMatrix;
					ViewState->PendingPrevViewMatrix		= View.ViewMatrix;
					ViewState->PendingPrevViewOrigin		= View.ViewOrigin;
					ViewState->PendingPrevPreViewTranslation= View.PreViewTranslation;
				}
				// we don't use DeltaTime as it can be 0 (in editor) and is computed by subtracting floats (loses precision over time)
				ViewState->MotionBlurTimeScale			= bEnableTimeScale ? (1.0f / (View.Family->DeltaWorldTime * 30.0f)) : 1.0f;

				View.PrevPreViewTranslation	= ViewState->PrevPreViewTranslation;
				View.PrevViewProjMatrix = ViewState->PrevViewMatrix * ViewState->PrevProjMatrix;
				View.PrevViewRotationProjMatrix = ViewState->PrevViewMatrix.RemoveTranslation() * ViewState->PrevProjMatrix;
				View.PrevViewOrigin = ViewState->PrevViewOrigin;
			}

			PreviousFrameTime = ViewState->LastRenderTime;
			ViewState->LastRenderTime = View.Family->CurrentRealTime;
			ViewState->LastRenderTimeDelta = DeltaTime;
		}

		/** Each view starts out rendering to the HDR scene color */
		View.bUseLDRSceneColor = FALSE;
	}

	// Allocate the visible light info.
	if (Scene->Lights.GetMaxIndex() > 0)
	{
		VisibleLightInfos.AddZeroed(Scene->Lights.GetMaxIndex());
	}

	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		FViewInfo& View = Views(ViewIndex);

		// Initialize the view elements' dynamic resources.
		View.InitDynamicResources();

		// Allocate the view's visibility maps.
		View.PrimitiveVisibilityMap.Init(FALSE,Scene->Primitives.GetMaxIndex());
		View.StaticMeshVisibilityMap.Init(FALSE,Scene->StaticMeshes.GetMaxIndex());
		View.StaticMeshOccluderMap.Init(FALSE,Scene->StaticMeshes.GetMaxIndex());
		View.StaticMeshVelocityMap.Init(FALSE,Scene->StaticMeshes.GetMaxIndex());
		View.StaticMeshShadowDepthMap.Init(FALSE,Scene->StaticMeshes.GetMaxIndex());
		View.DecalStaticMeshVisibilityMap.Init(FALSE,Scene->DecalStaticMeshes.GetMaxIndex());

#if USE_MASSIVE_LOD
		View.PrimitiveParentProcessedMap.Init(FALSE,Scene->Primitives.GetMaxIndex());
#endif

		View.VisibleLightInfos.Empty(Scene->Lights.GetMaxIndex());
		for(INT LightIndex = 0;LightIndex < Scene->Lights.GetMaxIndex();LightIndex++)
		{
#if !(defined(_DEBUG)) && !DEBUG_MEMORY_ISSUES
			if( LightIndex+2 < Scene->Lights.GetMaxIndex() )
			{
				if (LightIndex > 2)
				{
					FLUSH_CACHE_LINE(&View.VisibleLightInfos(LightIndex-2));
				}
				CONSOLE_PREFETCH(&View.VisibleLightInfos(LightIndex+2));
				CONSOLE_PREFETCH(&View.VisibleLightInfos(LightIndex+1));
			}
#endif
			new(View.VisibleLightInfos) FVisibleLightViewInfo();
		}

		View.PrimitiveViewRelevanceMap.Empty(Scene->Primitives.GetMaxIndex());
		View.PrimitiveViewRelevanceMap.AddZeroed(Scene->Primitives.GetMaxIndex());
		
		// If this is the visibility-parent of other views, reset its ParentPrimitives list.
		FSceneViewState* ViewState = (FSceneViewState*)View.State;
		const UBOOL bIsParent = ViewState && ViewState->IsViewParent();
		if ( bIsParent )
		{
			ViewState->ParentPrimitives.Empty();
		}

		// Create the view's actor visibility set.
		if(View.ActorVisibilityHistory)
		{
			View.ActorVisibilitySet = new FActorVisibilitySet;
		}

		if (ViewState)
		{	
			SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_DecompressPrecomputedOcclusion, !bIsSceneCapture);
			View.PrecomputedVisibilityData = ViewState->GetPrecomputedVisibilityData(View, Scene);
		}
		else
		{
			View.PrecomputedVisibilityData = NULL;
		}
	}

    PerformViewFrustumCulling();

	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{		
		FViewInfo& View = Views(ViewIndex);

		// sort the translucent primitives
		for( INT DPGIndex=0; DPGIndex < SDPG_MAX_SceneRender; DPGIndex++ )
		{
			View.TranslucentPrimSet[DPGIndex].SortPrimitives();
		}
	}

	// determine visibility of each light
	for(TSparseArray<FLightSceneInfoCompact>::TConstIterator LightIt(Scene->Lights);LightIt;++LightIt)
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		const FLightSceneInfo* LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		// view frustum cull lights in each view
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{		
			FViewInfo& View = Views(ViewIndex);
			FVisibleLightViewInfo& VisibleLightViewInfo = View.VisibleLightInfos(LightIt.GetIndex());

			// check if the light has any visible lit primitives 
			// or uses modulated shadows since they are always rendered
			if( LightSceneInfo->LightShadowMode == LightShadow_Modulate || 
				LightSceneInfo->LightShadowMode == LightShadow_ModulateBetter ||
				VisibleLightViewInfo.HasVisibleLitPrimitives())
			{
				// dir lights are always visible, and point/spot only if in the frustum
				if (LightSceneInfo->LightType == LightType_Point 
					|| LightSceneInfo->LightType == LightType_DominantPoint 
					|| LightSceneInfo->LightType == LightType_Spot
					|| LightSceneInfo->LightType == LightType_DominantSpot)
				{
					if (View.ViewFrustum.IntersectSphere(LightSceneInfo->GetOrigin(), LightSceneInfo->GetRadius()))
					{
						VisibleLightViewInfo.bInViewFrustum = TRUE;
					}
				}
				else
				{
					VisibleLightViewInfo.bInViewFrustum = TRUE;
				}
			}
		}
	}

	if( (ViewFamily.ShowFlags & SHOW_DynamicShadows) && GSystemSettings.RenderThreadSettings.bAllowDynamicShadows )
	{
		// Setup dynamic shadows.
		InitDynamicShadows();
	}

	// Initialize the fog constants.
	InitFogConstants();
}

/** 
* @return text description for each DPG 
*/
const TCHAR* GetSceneDPGName( ESceneDepthPriorityGroup DPG )
{
	switch(DPG)
	{
	case SDPG_UnrealEdBackground:
		return TEXT("UnrealEd Background");
	case SDPG_World:
		return TEXT("World");
	case SDPG_Foreground:
		return TEXT("Foreground");
	case SDPG_UnrealEdForeground:
		return TEXT("UnrealEd Foreground");
	case SDPG_PostProcess:
		return TEXT("PostProcess");
	default:
		return TEXT("Unknown");
	};
}

/** 
* Clears a view. 
*/
void FSceneRenderer::ClearView()
{
	SCOPED_DRAW_EVENT( EventClear )( DEC_SCENE_ITEMS, TEXT( "ClearView" ) );

	GSceneRenderTargets.BeginRenderingSceneColor();

	// Clear the entire viewport to make sure no post process filters in any data from an invalid region
	RHISetViewport( 0, 0, 0.0f, ViewFamily.RenderTarget->GetSizeX(), ViewFamily.RenderTarget->GetSizeY(), 1.0f);

    RHIClear( TRUE, FLinearColor::Black, FALSE, 0, FALSE, 0 );

	// Clear the G Buffer render targets
	GSceneRenderTargets.ClearGBufferTargets();

	// Clear the viewports to their background color
	if( GIsEditor )
	{
		// Clear each viewport to its own background color
		GSceneRenderTargets.BeginRenderingSceneColor();
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_CONDITIONAL_DRAW_EVENT(EventView,Views.Num() > 1)(DEC_SCENE_ITEMS,TEXT("ClearView%d"),ViewIndex);

			FViewInfo& View = Views(ViewIndex);

			// Set the device viewport for the view.
			RHISetViewport(View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);

			// Clear the scene color surface when rendering the first DPG.
			FLinearColor FinalBackgroundColor = ConditionalAdjustForMobileEmulation(&View, View.BackgroundColor);
			RHIClear(TRUE,FinalBackgroundColor,FALSE,0,FALSE,0);
		}

		if (!GSupportsDepthTextures)
		{
			// Clear the depths to max depth so that depth bias blend materials always show up
			ClearSceneColorDepth();
		}
	}
}

// These have been tweaked based on Gears of War scenes with lots of foliage.  
// Be sure to re-tweak these based on your game's scenes, as they are an easy way to save GPU time on Xbox 360.
INT GBasePassMaskedPixelGPRs = 80;
INT GBasePassOpaquePixelGPRs = 80;

FORCEINLINE UBOOL FSceneRenderer::RenderDPGBasePassStaticDataMasked(UINT DPGIndex, FViewInfo& View)
{
	UBOOL bDirty = FALSE;

#if !FINAL_RELEASE
	// When rendering masked materials in the shader complexity viewmode, 
	// We want to overwrite complexity for the pixels which get depths written,
	// And accumulate complexity for pixels which get killed due to the opacity mask being below the clip value.
	// This is accomplished by forcing the masked materials to render depths in the depth only pass, 
	// Then rendering in the base pass with additive complexity blending, depth tests on, and depth writes off.
	if (View.Family->ShowFlags & SHOW_ShaderComplexity)
	{
		RHISetDepthState(TStaticDepthState<FALSE,CF_LessEqual>::GetRHI());
	}
#endif

	RHISetShaderRegisterAllocation(128 - GBasePassMaskedPixelGPRs, GBasePassMaskedPixelGPRs);
	{
		// Draw the scene's base pass draw lists.
		FDepthPriorityGroup::EBasePassDrawListType MaskedDrawType = FDepthPriorityGroup::EBasePass_Masked;
		{
			SCOPED_DRAW_EVENT(EventStaticNoLightmap)(DEC_SCENE_ITEMS,TEXT("StaticMaskedNoLightmap"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassNoLightMapDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticMaskedVertexLightmap)(DEC_SCENE_ITEMS,TEXT("StaticMaskedVertexLightmapped"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassDirectionalVertexLightMapDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSimpleVertexLightMapDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticMaskedShadowedDynamicLightVertexLightmap)(DEC_SCENE_ITEMS,TEXT("StaticMaskedShadowedDLVertexLightmap"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassShadowedDynamicLightDirectionalVertexLightMapDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticMaskedTextureLightmap)(DEC_SCENE_ITEMS,TEXT("StaticMaskedTextureLightmapped"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassDirectionalLightMapTextureDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSimpleLightMapTextureDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticMaskedShadowedDynamicLightTextureLightmap)(DEC_SCENE_ITEMS,TEXT("StaticMaskedShadowedDLTextureLightmap"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassShadowedDynamicLightDirectionalLightMapTextureDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassDistanceFieldShadowedDynamicLightDirectionalLightMapTextureDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticDynamicallyLit)(DEC_SCENE_ITEMS,TEXT("StaticMaskedDynamicallyLit"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassDirectionalLightDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSHLightDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassDynamicallyShadowedDynamicLightDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSHLightAndDynamicLightDrawList[MaskedDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
	}

#if !FINAL_RELEASE
	if (View.Family->ShowFlags & SHOW_ShaderComplexity)
	{
		RHISetDepthState(TStaticDepthState<TRUE,CF_LessEqual>::GetRHI());
	}
#endif

	return bDirty;
}

FORCEINLINE UBOOL FSceneRenderer::RenderDPGBasePassStaticDataDefault(UINT DPGIndex, FViewInfo& View)
{
	UBOOL bDirty = FALSE;

	RHISetShaderRegisterAllocation(128 - GBasePassOpaquePixelGPRs, GBasePassOpaquePixelGPRs);
	{
		FDepthPriorityGroup::EBasePassDrawListType OpaqueDrawType = FDepthPriorityGroup::EBasePass_Default;
		{
			SCOPED_DRAW_EVENT(EventStaticNoLightmap)(DEC_SCENE_ITEMS,TEXT("StaticOpaqueNoLightmap"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassNoLightMapDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticVertexLightmap)(DEC_SCENE_ITEMS,TEXT("StaticOpaqueVertexLightmapped"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassDirectionalVertexLightMapDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSimpleVertexLightMapDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticOpaqueShadowedDynamicLightVertexLightmap)(DEC_SCENE_ITEMS,TEXT("StaticOpaqueShadowedDLVertexLightmap"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassShadowedDynamicLightDirectionalVertexLightMapDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticTextureLightmapped)(DEC_SCENE_ITEMS,TEXT("StaticOpaqueTextureLightmapped"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassDirectionalLightMapTextureDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSimpleLightMapTextureDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticShadowedDynamicLightTextureLightmap)(DEC_SCENE_ITEMS,TEXT("StaticOpaqueShadowedDLTextureLightmap"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassShadowedDynamicLightDirectionalLightMapTextureDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassDistanceFieldShadowedDynamicLightDirectionalLightMapTextureDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
		{
			SCOPED_DRAW_EVENT(EventStaticTextureLightmapped)(DEC_SCENE_ITEMS,TEXT("StaticOpaqueDynamicallyLit"));
			bDirty |= Scene->DPGs[DPGIndex].BasePassDirectionalLightDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSHLightDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassDynamicallyShadowedDynamicLightDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSHLightAndDynamicLightDrawList[OpaqueDrawType].DrawVisible(View,View.StaticMeshVisibilityMap);
		}
	}

	return bDirty;
}

/**
* Renders the basepass for the static data of a given DPG and View.
*
* @return TRUE if anything was rendered to scene color
*/
UBOOL FSceneRenderer::RenderDPGBasePassStaticData(UINT DPGIndex, FViewInfo& View)
{
	UBOOL bDirty = FALSE;
	// render opaque decals
	bDirty |= RenderDecals(View,DPGIndex,FALSE);

	SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_StaticDrawListDrawTime, !bIsSceneCapture);

	// When using a depth-only pass, the default opaque geometry's depths are already
	// in the depth buffer at this point, so rendering masked next will already cull
	// as efficiently as it can, while also increasing the ZCull efficiency when
	// rendering the default opaque geometry afterward.
	if( bUseDepthOnlyPass )
	{
		bDirty |= RenderDPGBasePassStaticDataMasked(DPGIndex, View);
		bDirty |= RenderDPGBasePassStaticDataDefault(DPGIndex, View);
	}
	else
	{
		// Otherwise, in the case where we're not using a depth-only pre-pass, there
		// is an advantage to rendering default opaque first to help cull the more
		// expensive masked geometry.
		bDirty |= RenderDPGBasePassStaticDataDefault(DPGIndex, View);
		bDirty |= RenderDPGBasePassStaticDataMasked(DPGIndex, View);
	}

    return bDirty;
}

/**
* Renders the basepass for the dynamic data of a given DPG and View.
*
* @return TRUE if anything was rendered to scene color
*/

UBOOL FSceneRenderer::RenderDPGBasePassDynamicData(UINT DPGIndex, FViewInfo& View)
{
	UBOOL bDirty=0;

    // Draw the base pass for the view's batched mesh elements.
	bDirty |= DrawViewElements<FBasePassOpaqueDrawingPolicyFactory>(View,FBasePassOpaqueDrawingPolicyFactory::ContextType(),DPGIndex,TRUE);

	// Draw the view's batched simple elements(lines, sprites, etc).
	bDirty |= View.BatchedViewElements[DPGIndex].Draw(View.ViewProjectionMatrix,appTrunc(View.SizeX),appTrunc(View.SizeY),FALSE);

    return bDirty;
}

/**
* Renders the basepass for a given DPG and View.
*
* @param DPGIndex - current depth priority group index
* @param bIsTiledRendering - TRUE if currently within a Begin/End tiled rendering block
* @return TRUE if anything was rendered to scene color
*/


UBOOL FSceneRenderer::RenderDPGBasePass(UINT DPGIndex, FViewInfo& View, UBOOL bIsTiledRendering)
{
	UBOOL bWorldDpg = (DPGIndex == SDPG_World);
	UBOOL bDirty=0;

    // only allow rendering of mesh elements with dynamic data if not rendering in a Begin/End tiling block (only applies to world dpg and if tiling is enabled)
	UBOOL bRenderDynamicData = !bIsTiledRendering || !bWorldDpg || !GUseTilingCode;
	// only allow rendering of mesh elements with static data if within the Begin/End tiling block (only applies to world dpg and if tiling is enabled)
	UBOOL bRenderStaticData = bIsTiledRendering || !bWorldDpg || !GUseTilingCode;

	// in non-tiled MSAA split screen mode, render both dynamic & static data
	if (GUseMSAASplitScreen)
	{
		bRenderDynamicData = bRenderStaticData = TRUE;
	}

    if ( bRenderStaticData )
    {
        // Render the base pass static data
        bDirty |= RenderDPGBasePassStaticData(DPGIndex,View);
    }

	{
		SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_DynamicPrimitiveDrawTime, !bIsSceneCapture);
		SCOPED_DRAW_EVENT(EventDynamic)(DEC_SCENE_ITEMS,TEXT("Dynamic"));

		if( View.VisibleDynamicPrimitives.Num() > 0 )
		{
			// Draw the dynamic non-occluded primitives using a base pass drawing policy.
			TDynamicPrimitiveDrawer<FBasePassOpaqueDrawingPolicyFactory> Drawer(
				&View,DPGIndex,FBasePassOpaqueDrawingPolicyFactory::ContextType(),TRUE);
			for(INT PrimitiveIndex = 0;PrimitiveIndex < View.VisibleDynamicPrimitives.Num();PrimitiveIndex++)
			{
				const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives(PrimitiveIndex);
				const FPrimitiveViewRelevance& PrimitiveViewRelevance = View.PrimitiveViewRelevanceMap(PrimitiveSceneInfo->Id);

				const UBOOL bVisible = View.PrimitiveVisibilityMap(PrimitiveSceneInfo->Id);
				const UBOOL bRelevantDPG = PrimitiveViewRelevance.GetDPG(DPGIndex) != 0;

				// Only draw the primitive if it's visible and relevant in the current DPG
				if( bVisible && bRelevantDPG && 
					// only draw opaque and masked primitives if wireframe is disabled
					(PrimitiveViewRelevance.bOpaqueRelevance || ViewFamily.ShowFlags & SHOW_Wireframe)
					// only draw static prims or dynamic prims based on tiled block (see above)
					&& (bRenderStaticData || bRenderDynamicData && PrimitiveViewRelevance.bUsesDynamicMeshElementData) )
				{
					DWORD Flags = bRenderStaticData ? 0 : FPrimitiveSceneProxy::DontAllowStaticMeshElementData;
					Flags |= bRenderDynamicData ? 0 : FPrimitiveSceneProxy::DontAllowDynamicMeshElementData;

					Drawer.SetPrimitive(PrimitiveSceneInfo);
					PrimitiveSceneInfo->Proxy->DrawDynamicElements(
						&Drawer,
						&View,
						DPGIndex,
						Flags					
						);
				}
			}
			bDirty |= Drawer.IsDirty(); 
		}

		if( bRenderDynamicData )
		{
			bDirty |= RenderDPGBasePassDynamicData(DPGIndex,View);
		}
	}
	RHISetShaderRegisterAllocation(64, 64);

    return bDirty;
}

/** 
* Begins the rendering for a given DPG.
*
* @param DPGIndex - current depth priority group index
* @param bRequiresClear - TRUE if the scene color buffer is to be cleared
* @param bSceneColorDirty - TRUE if the scene color is left dirty
* @param bIsOcclusionTesting - TRUE if occlusion testing is allowed for this rendering pass
* @return TRUE if rendering for this DPG should be skipped
*/
UBOOL FSceneRenderer::RenderDPGBegin(UINT DPGIndex, UBOOL& bRequiresClear, UBOOL& bSceneColorDirty, UBOOL bIsOcclusionTesting)
{
	if( !GUsingMobileRHI )
	{
		// WARNING: if any Xbox360 rendering uses SDPG_UnrealEdBackground, it will not show up with tiling enabled
		// unless you go out of your way to Resolve the scene color and Restore it at the beginning of the tiling pass.
		// SDPG_World and SDPG_Foreground work just fine, however.
		UBOOL bWorldDpg = (DPGIndex == SDPG_World);

		const UBOOL bIsWireframe = (ViewFamily.ShowFlags & SHOW_Wireframe) != 0;

#if XBOX
		// On Xbox, use non-tiling MSAA split screen mode for 2P horizontal; only available if we're already using tiling mode 1
		GUseMSAASplitScreen = (GEngine->GameViewport->GetCurrentSplitscreenType() == eSST_2P_HORIZONTAL) 
			&& GUseTilingCode
			&& (GTilingMode == 1)
			&& bWorldDpg
			&& Views.Num() > 1;

		if (GUseMSAASplitScreen)
		{
			// Assumption in non-tiling MSAA mode: view 0 is the top view
			check(Views(0).RenderTargetY < Views(1).RenderTargetY);
		}
#endif

		if (!GUseMSAASplitScreen)
		{
			if( GUseTilingCode && bWorldDpg )
			{
				RHIMSAAInitPrepass();
			}

			// Find the dominant shadow casting light, if one exists
			bDominantShadowsActive = AreDominantShadowsActive(DPGIndex);

			// Render the whole scene dominant shadow depths before anything else is in EDRAM or HiZ for Xbox 360, since we want the highest resolution possible
			RenderWholeSceneDominantShadowDepth(DPGIndex);

			// Draw the scene pre-pass, populating the scene depth buffer and HiZ
			RenderPrePass(DPGIndex, bIsOcclusionTesting, -1);

			// Render dominant light shadows using the depths from the prepass, so they can be looked up in the basepass
			RenderDominantLightShadowsForBasePass(DPGIndex);
		}

		UBOOL bBasePassDirtiedColor = FALSE;

		// Clear scene color buffer if necessary.
		if ( bRequiresClear )
		{
			ClearView();

			// Only clear once.
			bRequiresClear = FALSE;
		}

		if (!GUseMSAASplitScreen)
		{
			if( GUseTilingCode && bWorldDpg )
			{
				RHIMSAABeginRendering(ViewFamily.bClearScene || bIsWireframe);
			}
			else
			{
				// Begin rendering to scene color
				GSceneRenderTargets.BeginRenderingSceneColor(RTUsage_Default, TRUE, FALSE);
			}
		}

		if(IsPCPlatform(GRHIShaderPlatform))
		{
			// For platforms that store depth in scene color alpha, we render the soft masked
			// materials here, additionally we write to the z buffer.
			bSceneColorDirty |= RenderSoftMaskedDepth(DPGIndex);
		}

		if ( (ViewFamily.ShowFlags & SHOW_TextureDensity) && AllowDebugViewmodes() )
		{
			// Override the base pass with the texture density pass if the viewmode is enabled.
			bBasePassDirtiedColor |= RenderTextureDensities(DPGIndex);
		}
		else if ((ViewFamily.ShowFlags & SHOW_LightMapDensity) 
			&& !(ViewFamily.ShowFlags & SHOW_PostProcess) 
			&& AllowDebugViewmodes())
		{
			// Override the base pass with the lightmap density pass if the viewmode is enabled.
			bBasePassDirtiedColor |= RenderLightMapDensities(DPGIndex,TRUE);
		}
		else
		{
			// Draw the base pass pass for all visible primitives in this DPG.
			// First base pass is for the Begin/End tiling block
			bBasePassDirtiedColor |= RenderBasePass(DPGIndex,TRUE);
		}

		bSceneColorDirty |= bBasePassDirtiedColor;

		if( GUseTilingCode && bWorldDpg )
		{
			if (!GUseMSAASplitScreen)
			{
				RHISetBlendState( TStaticBlendState<>::GetRHI());
				RHIMSAAEndRendering(GSceneRenderTargets.GetSceneDepthTexture(), GSceneRenderTargets.GetSceneColorTexture(), 0);

				// Scenecolor has been resolved now.
				bSceneColorDirty = FALSE;
			}
		}

#if !XBOX
		else
		{
			// For SM3 platforms that store depth in scene color alpha, this resolve is needed so shadow passes can read the scene depth.
			GSceneRenderTargets.FinishRenderingSceneColor(bSceneColorDirty||bWorldDpg, FResolveRect(0, 0, FamilySizeX, FamilySizeY));

			// Scenecolor has been resolved now.
			bSceneColorDirty = FALSE;
		}
#endif

		if( bWorldDpg )
		{
			if( GUseTilingCode )
			{
				// Need to set the render targets before we restore the color and depth data
				GSceneRenderTargets.BeginRenderingSceneColor(RTUsage_Default, TRUE, FALSE);
				RHIRestoreColorDepth(GSceneRenderTargets.GetSceneColorTexture(), GSceneRenderTargets.GetSceneDepthTexture());
			
				if (!GUseMSAASplitScreen)
				{
					// render a second pass for base pass items which should not use MSAA predicated tiling
					// this will include anything which uses dynamic data that needs to allocate space on the command buffer
					// since that could easily cause the static buffer used for tiling to run out of space

					const UBOOL bBasePassDirtiedColorUntiled = RenderBasePass(DPGIndex,FALSE);
					bBasePassDirtiedColor |= bBasePassDirtiedColorUntiled;
					bSceneColorDirty |= bBasePassDirtiedColorUntiled;

					if( bBasePassDirtiedColorUntiled )
					{
						// resolve depth buffer
						GSceneRenderTargets.ResolveSceneDepthTexture(); 
					}				
				}				
			}
			else
			{
				// Scene depth values resolved to scene depth texture
				// only resolve depth values from world dpg
				GSceneRenderTargets.ResolveSceneDepthTexture();
			}
		}

		if (ShouldUseDeferredShading())
		{
			if ((ViewFamily.ShowFlags & SHOW_Lighting))
			{
				// Render post process effects that affect lighting only
				// When using deferred shading, SSAO will modify the diffuse G buffer, so we have to do this before the G buffer resolve
				bSceneColorDirty |= RenderPostProcessEffects(DPGIndex, TRUE);
			}

			// Resolve the GBuffers used for deferred shading
			GSceneRenderTargets.ResolveGBufferSurfaces(FResolveRect(0, 0, FamilySizeX, FamilySizeY));
		}
	}
	else
	{
		// GUsingMobileRHI

		// Draw the base pass pass for all visible primitives in this DPG.
		// First base pass is for the Begin/End tiling block
		bSceneColorDirty |= RenderBasePass(DPGIndex,TRUE);
	}
	
    return TRUE;
}

/** 
* Ends the rendering for a given DPG.
*
* @param DPGIndex - current depth priority group index
* @param bDeferPrePostProcessResolve - TRUE if the pre post process resolve is deferred
* @param bSceneColorDirty - TRUE if the scene color is left dirty
*/
void FSceneRenderer::RenderDPGEnd(UINT DPGIndex, UBOOL bDeferPrePostProcessResolve, UBOOL& bSceneColorDirty, UBOOL bIsOcclusionTesting)
{
	if( !GUsingMobileRHI )
	{
		UBOOL bWorldDpg = (DPGIndex == SDPG_World);

		if(ViewFamily.ShowFlags & SHOW_Lighting)
		{
			{
				SCOPED_DRAW_EVENT(EventShadowedLights)(DEC_SCENE_ITEMS,TEXT("ShadowedLights"));
				// Render the scene lights that are affected by modulated shadows.
				bSceneColorDirty |= RenderLights(DPGIndex,TRUE,bSceneColorDirty);
			}

			if( !(ViewFamily.ShowFlags & SHOW_ShaderComplexity) 
				&& ViewFamily.ShowFlags & SHOW_DynamicShadows
				&& GSystemSettings.RenderThreadSettings.bAllowDynamicShadows )
			{
				// Render the modulated shadows.
				bSceneColorDirty |= RenderModulatedShadows(DPGIndex);
			}

			{
				SCOPED_DRAW_EVENT(EventUnshadowedLights)(DEC_SCENE_ITEMS,TEXT("UnshadowedLights"));
				// Render the scene lights that aren't affected by modulated shadows.
				bSceneColorDirty |= RenderLights(DPGIndex,FALSE,bSceneColorDirty);
			}
		}

		if( ViewFamily.ShowFlags & SHOW_Decals )
		{
			SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_UnlitDecalDrawTime, !bIsSceneCapture);
			GSceneRenderTargets.BeginRenderingSceneColor(RTUsage_Default, FALSE, FALSE);

			for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
			{
				const FViewInfo& View = Views(ViewIndex);

				// Set the device viewport for the view and the global shader params for the view
				RHISetViewport(View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
				RHISetViewParameters(View);

				// render translucent decals 
				bSceneColorDirty |= RenderDecals(View,DPGIndex,TRUE);
			}

			GSceneRenderTargets.FinishRenderingSceneColor(FALSE);
		}

		if(!GOnePassDominantLight)
		{
			// This is one of the two possible positions we want to render all SoftMasked
			// objects (Ambient+Sun, no other lights). Unfortunately here the light attenuation texture
			// with the sun information was already reused for other lights.
			bSceneColorDirty |= RenderSoftMaskedBase(DPGIndex);
		}

		// Update the quarter-sized depth buffer with the current contents of the scene depth texture.
		// This needs to happen after modulated shadows, whose render targets overlap with the small depth surface in EDRAM,
		// And before ambient occlusion and occlusion tests, which make use of the small depth buffer.
		if ( bWorldDpg )
		{
			UpdateDownsampledDepthSurface();
		}

		if ( bIsOcclusionTesting )
		{
			// Issue occlusion queries
			// This is done after the downsampled depth buffer is created so that it can be used for issuing queries
			BeginOcclusionTests();
		}

		if(ViewFamily.ShowFlags & SHOW_Lighting)
		{
			// Render image reflections
			bSceneColorDirty |= RenderImageReflections(DPGIndex);

			// Render subsurface scattering.
			bSceneColorDirty |= RenderSubsurfaceScattering(DPGIndex);

			if (!ShouldUseDeferredShading())
			{
				// Render post process effects that affect lighting only
				bSceneColorDirty |= RenderPostProcessEffects(DPGIndex, TRUE);
			}
		}

		if(ShouldRenderFog(ViewFamily.ShowFlags))
		{
			// Render the scene fog.
			bSceneColorDirty |= RenderFog(DPGIndex);
		}

		if( ViewFamily.ShowFlags & SHOW_UnlitTranslucency )
		{
			// Distortion pass
			bSceneColorDirty |= RenderDistortion(DPGIndex);
		}

		// Only resolve here if scene color is dirty and we don't need to resolve at the end of each DPG
		if(bSceneColorDirty && !bDeferPrePostProcessResolve)
		{
			// Save the color buffer if any uncommitted changes have occurred
			GSceneRenderTargets.ResolveSceneColor(FResolveRect(0, 0, FamilySizeX, FamilySizeY));
			bSceneColorDirty = FALSE;
		}

		if( ViewFamily.ShowFlags & SHOW_UnlitTranslucency )
		{
			SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_TranslucencyDrawTime, !bIsSceneCapture);
			// Translucent pass.
			const UBOOL bTranslucencyDirtiedColor = RenderTranslucency( DPGIndex );

			// If any translucent elements were drawn, render post-fog decals for translucent receivers.
			if( bTranslucencyDirtiedColor )
			{
				// Only need to resolve if a PP effect might need to read from scene color at the end of the DPG.
				if ( !bDeferPrePostProcessResolve )
				{
					// Finish rendering scene color after rendering translucency for this DPG.
					GSceneRenderTargets.FinishRenderingSceneColor( TRUE, FResolveRect(0, 0, FamilySizeX, FamilySizeY) );
				}
			}

			if (!(ViewFamily.ShowFlags & SHOW_Wireframe))
			{
				// Radial blur pass (only for non-velocity blur items)
				const UBOOL bRadialBlurDirtiedColor = RenderRadialBlur(DPGIndex, bSceneColorDirty || (bTranslucencyDirtiedColor && bDeferPrePostProcessResolve));
				if (bRadialBlurDirtiedColor && !bDeferPrePostProcessResolve)
				{
					GSceneRenderTargets.FinishRenderingSceneColor( TRUE, FResolveRect(0, 0, FamilySizeX, FamilySizeY) );
				}
			}		
		}

		if (bWorldDpg)
		{
			SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_PostProcessDrawTime, !bIsSceneCapture);
			RenderLightShafts();
		}

		// Render the velocities of movable objects for the motion blur effect.
		if(bWorldDpg
		&& GSystemSettings.RenderThreadSettings.bAllowMotionBlur
		&& (ViewFamily.ShowFlags & SHOW_MotionBlur))
		{
			RenderVelocities(DPGIndex);
		}

		// post process effects pass for scene DPGs
		RenderPostProcessEffects(DPGIndex);
	}
	else
	{
		// scaled down version for mobile rendering

		if ((ViewFamily.ShowFlags & SHOW_Lighting)
			&& (ViewFamily.ShowFlags & SHOW_DynamicShadows)
			&& GSystemSettings.RenderThreadSettings.bAllowDynamicShadows)
		{
			// Render the modulated shadows.
			RenderModulatedShadows(DPGIndex);
		}

		if( ViewFamily.ShowFlags & SHOW_Decals )
		{
			SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_UnlitDecalDrawTime, !bIsSceneCapture);
			GSceneRenderTargets.BeginRenderingSceneColor(RTUsage_Default, FALSE);

			for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
			{
				const FViewInfo& View = Views(ViewIndex);

				// Set the device viewport for the view and the global shader params for the view
				RHISetViewport(View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
				RHISetViewParameters(View);

				// render translucent decals 
				bSceneColorDirty |= RenderDecals(View,DPGIndex,TRUE);
			}

			GSceneRenderTargets.FinishRenderingSceneColor(FALSE);
		}		

		if( ViewFamily.ShowFlags & SHOW_UnlitTranslucency )
		{
			SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_TranslucencyDrawTime, !bIsSceneCapture);
			// Translucent pass.
			const UBOOL bTranslucencyDirtiedColor = RenderTranslucency( DPGIndex );
		}
	}
}


/** 
* Finishes the view family rendering.
*
* @param bDeferPrePostProcessResolve - TRUE if the pre post process resolve is to be deferred
*/
void FSceneRenderer::RenderFinish(UBOOL bDeferPrePostProcessResolve)
{
	// If all post process effects are in SDPG_PostProcess, then only one resolve is necessary after the scene is rendered.
	if( bDeferPrePostProcessResolve )
	{
		GSceneRenderTargets.ResolveSceneColor(FResolveRect(0, 0, FamilySizeX, FamilySizeY));
	}

	// post process effects pass for post process DPGs
	RenderPostProcessEffects(SDPG_PostProcess);

	// Finish rendering for each view.
	{
		SCOPED_DRAW_EVENT(EventFinish)(DEC_SCENE_ITEMS,TEXT("FinishRendering"));
		
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{	
			SCOPED_CONDITIONAL_DRAW_EVENT(EventView,Views.Num() > 1)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

			FinishRenderViewTarget(&Views(ViewIndex));
		}
	}

#if !FINAL_RELEASE
	{
		SCOPED_DRAW_EVENT(EventFrozenText)(DEC_SCENE_ITEMS,TEXT("FrozenText"));

		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{	
			SCOPED_CONDITIONAL_DRAW_EVENT(EventView,Views.Num() > 1)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);
			FViewInfo& View = Views(ViewIndex);

			// display a message saying we're frozen
			FSceneViewState* ViewState = (FSceneViewState*)View.State;
			if (ViewState && (ViewState->HasViewParent()
				|| ViewState->bIsFrozen
				))
			{
				if ( ViewState->bIsFrozen )
				{
					// this is a helper class for FCanvas to be able to get screen size
					class FRenderTargetFreeze : public FRenderTarget
					{
					public:
						UINT SizeX, SizeY;
						FRenderTargetFreeze(UINT InSizeX, UINT InSizeY)
							: SizeX(InSizeX), SizeY(InSizeY)
						{}
						UINT GetSizeX() const
						{
							return SizeX;
						};
						UINT GetSizeY() const
						{
							return SizeY;
						};
					} TempRenderTarget(Views(ViewIndex).RenderTargetSizeX, Views(ViewIndex).RenderTargetSizeY);

					// create a temporary FCanvas object with the temp render target
					// so it can get the screen size
					FCanvas Canvas(&TempRenderTarget, NULL);
					const FString StateText =
						ViewState->bIsFrozen ?
						Localize(TEXT("ViewportStatus"),TEXT("RenderingFrozenE"),TEXT("Engine"))
						:
						Localize(TEXT("ViewportStatus"),TEXT("OcclusionChild"),TEXT("Engine"));
					DrawShadowedString(&Canvas, 10, 130, *StateText, GEngine->GetSmallFont(), FLinearColor(0.8,1.0,0.2,1.0));
					Canvas.Flush();
				}
			}
		}
	}

	// render texture content to screen, for debugging purpose, activated by console command "VisualizeTexture"
	if(GVisualizeTexture)
	{
		FViewInfo& View = Views(0);

		if(View.IsPerspectiveProjection())
		{
			GSceneRenderTargets.UpdateTexturePoolVisualizer();

			UINT Id = GVisualizeTexture - 1;
			const FTexture2DRHIRef& VisTexture = GSceneRenderTargets.GetRenderTargetTexture((ESceneRenderTargetTypes)Id);

			if(IsValidRef(VisTexture))
			{
				FIntPoint ViewExtent((INT)View.SizeX, (INT)View.SizeY);
				FIntPoint RTExtent(GSceneRenderTargets.GetBufferSizeX(), GSceneRenderTargets.GetBufferSizeY());

				// multiple views means game splitscreen
				// we want the whole texture, not only the view
				if(Views.Num() > 1)
				{
					ViewExtent = RTExtent;
				}

				FVector2D Tex00 = FVector2D(0, 0);
				FVector2D Tex11 = FVector2D(1, 1);

				FIntPoint TexExtent;
				FString Name = GSceneRenderTargets.GetRenderTargetInfo((ESceneRenderTargetTypes)Id, TexExtent);

				if(TexExtent == FIntPoint(0, 0))
				{
					TexExtent = RTExtent;
				}

				switch(GVisualizeTextureInputMapping)
				{
					// view in left top
					case 0:
						Tex11 = FVector2D((FLOAT)ViewExtent.X / RTExtent.X, (FLOAT)ViewExtent.Y / RTExtent.Y);
						break;

					// whole texture
					case 1:
						break;

					// view in left top with 1 texel border
					case 2:
						Tex00 = FVector2D(1.0f / TexExtent.X, 1.0f / TexExtent.Y);
						Tex11 = FVector2D((FLOAT)ViewExtent.X / RTExtent.X, (FLOAT)ViewExtent.Y / RTExtent.Y) - Tex00;
						break;
				}

				UBOOL bUseSaturateInsteadOfFrac = (GVisualizeTextureFlags & 1) != 0;

				// continue rendering to HDR if necessary
				UBOOL RenderToBackbuffer = TRUE;
				if( !View.Family->bResolveScene )
				{
					RenderToBackbuffer = FALSE;
				}

				VisualizeTexture(VisTexture, RenderToBackbuffer, RTExtent, RTExtent, GVisualizeTextureRGBMul, GVisualizeTextureAMul, 0.0f, Tex00, Tex11, 1.0f, bUseSaturateInsteadOfFrac);

				// fix this later: outputting text should require 1-3 lines

				// this is a helper class for FCanvas to be able to get screen size
				class FRenderTargetFreeze : public FRenderTarget
				{
				public:
					UINT SizeX, SizeY;
					FRenderTargetFreeze(UINT InSizeX, UINT InSizeY)
						: SizeX(InSizeX), SizeY(InSizeY)
					{}
					UINT GetSizeX() const
					{
						return SizeX;
					};
					UINT GetSizeY() const
					{
						return SizeY;
					};
				} TempRenderTarget(View.RenderTargetSizeX, View.RenderTargetSizeY);

				FCanvas Canvas(&TempRenderTarget, NULL);

				FLOAT X = 100;
				FLOAT Y = 160;
				FLOAT YStep = 14;

				{
					FString Line = FString::Printf(TEXT("VisualizeTexture: %d(%s %dx%d) RGB*%g+A*%g UV%d"), GVisualizeTexture, *Name, 
						TexExtent.X, TexExtent.Y, 
						GVisualizeTextureRGBMul, GVisualizeTextureAMul, GVisualizeTextureInputMapping);
					DrawShadowedString(&Canvas, X, Y += YStep, *Line, GEngine->GetSmallFont(), FLinearColor(1, 1, 1));
				}
				{
					FString Line = FString::Printf(TEXT("   GetBufferSizeX()=%d GetBufferSizeY()=%d"), RTExtent.X, RTExtent.Y);
					DrawShadowedString(&Canvas, X + 10, Y += YStep, *Line, GEngine->GetSmallFont(), FLinearColor(1, 1, 1));
				}

				for(int ViewId = 0; ViewId < Views.Num(); ++ViewId)
				{
					const FViewInfo& ViewIt = Views(ViewId);
					FString Line = FString::Printf(TEXT("   View #%d: (%g,%g,%g,%g) RT:(%d,%d,%d,%d)"), ViewId + 1,
						ViewIt.X, ViewIt.Y,
						ViewIt.SizeX, ViewIt.SizeY,
						ViewIt.RenderTargetX, ViewIt.RenderTargetY,
						ViewIt.RenderTargetSizeX, ViewIt.RenderTargetSizeY);
					DrawShadowedString(&Canvas, X + 10, Y += YStep, *Line, GEngine->GetSmallFont(), FLinearColor(1, 1, 1));
				}

				X += 40;
				DrawShadowedString(&Canvas, X, Y += YStep, TEXT("Black: 0"), GEngine->GetSmallFont(), FLinearColor(1,1,1));
				DrawShadowedString(&Canvas, X, Y += YStep, TEXT("White: 1"), GEngine->GetSmallFont(), FLinearColor(1,1,1));
				DrawShadowedString(&Canvas, X, Y += YStep, TEXT("Blinking Green: >1"), GEngine->GetSmallFont(), FLinearColor(0,1,0));
				DrawShadowedString(&Canvas, X, Y += YStep, TEXT("Blinking Red: <0"), GEngine->GetSmallFont(), FLinearColor(1,0,0));
				DrawShadowedString(&Canvas, X, Y += YStep, TEXT("Blinking Blue: NAN or Inf"), GEngine->GetSmallFont(), FLinearColor(0,0,1));
				Canvas.Flush();
			}
		}
	}


	// Warning if uber post process is required but not included.
	{
		UBOOL bShowWarning = FALSE;

		for(INT ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FViewInfo &View = Views(ViewIndex);

			if(View.RequiresMotionBlurButHasNoUberPostProcess(View))
			{
				bShowWarning = TRUE;
				break;
			}
		}

		if(bShowWarning)
		{
			for(INT ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{	
				FViewInfo& View = Views(ViewIndex);

				if(!View.IsPerspectiveProjection())
				{
					continue;
				}

				// this is a helper class for FCanvas to be able to get screen size
				class FRenderTargetHelper : public FRenderTarget
				{
				public:
					UINT SizeX, SizeY;
					FRenderTargetHelper(UINT InSizeX, UINT InSizeY)
						: SizeX(InSizeX), SizeY(InSizeY)
					{}
					UINT GetSizeX() const
					{
						return SizeX;
					};
					UINT GetSizeY() const
					{
						return SizeY;
					};
				} TempRenderTarget(Views(ViewIndex).RenderTargetSizeX, Views(ViewIndex).RenderTargetSizeY);

				FCanvas Canvas(&TempRenderTarget, NULL);

				DrawShadowedString(&Canvas, 50, 200, TEXT("Warning: MotionBlur is used without UberPostProcessiong (this is no longer supported)"), GEngine->GetSmallFont(), FLinearColor(1, 0, 0));
				Canvas.Flush();
			}
		}
	}

#endif //!FINAL_RELEASE

	// Save the actor and primitive visibility states for the game thread.
	SaveVisibilityState();

	// Save the post-occlusion visibility stats for the frame and freezing info
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views(ViewIndex);
		if (!bIsSceneCapture)
		{
			INC_DWORD_STAT_BY(STAT_VisibleStaticMeshElements,View.NumVisibleStaticMeshElements);
			INC_DWORD_STAT_BY(STAT_VisibleDynamicPrimitives,View.NumVisibleDynamicPrimitives);
		}

#if !FINAL_RELEASE
		// update freezing info
		FSceneViewState* ViewState = (FSceneViewState*)View.State;
		if (ViewState)
		{
			// if we're finished freezing, now we are frozen
			if (ViewState->bIsFreezing)
			{
				ViewState->bIsFreezing = FALSE;
				ViewState->bIsFrozen = TRUE;
			}

			// handle freeze toggle request
			if (bHasRequestedToggleFreeze)
			{
				// do we want to start freezing?
				if (!ViewState->bIsFrozen)
				{
					ViewState->bIsFrozen = FALSE;
					ViewState->bIsFreezing = TRUE;
					ViewState->FrozenPrimitives.Empty();
				}
				// or stop?
				else
				{
					ViewState->bIsFrozen = FALSE;
					ViewState->bIsFreezing = FALSE;
					ViewState->FrozenPrimitives.Empty();
				}
			}
		}
#endif
	}

#if !FINAL_RELEASE
	// clear the commands
	bHasRequestedToggleFreeze = FALSE;
#endif
}

/** 
* Renders the view family. 
*/
void FSceneRenderer::Render()
{
	if( !GUsingMobileRHI )
	{
		// Allocate the maximum scene render target space for the current view family.
		GSceneRenderTargets.Allocate( ViewFamily.RenderTarget->GetSizeX(), ViewFamily.RenderTarget->GetSizeY() );
		GSceneRenderTargets.BeginFrame();

		// Find the visible primitives.
		InitViews();

		// Update the stereo texture.
		RHIUpdateStereoFixTexture(GSceneRenderTargets.GetStereoFixTexture());

		const UBOOL bIsWireframe = (ViewFamily.ShowFlags & SHOW_Wireframe) != 0;

		// Whether to clear the scene color buffer before rendering color for the first DPG. When using tiling, this 
		// gets cleared later automatically.
		UBOOL bRequiresClear = !GUseTilingCode && (ViewFamily.bClearScene || bIsWireframe);

		UBOOL bDeferPrePostProcessResolve = DeferPrePostProcessResolve();

		UBOOL bSceneColorDirty = TRUE;
		for(UINT DPGIndex = 0;DPGIndex < SDPG_MAX_SceneRender;DPGIndex++)
		{
			// force using occ queries for wireframe if rendering is parented or frozen in the first view
			check(Views.Num());        
    		UBOOL bIsOcclusionAllowed = (DPGIndex == SDPG_World) && !GIgnoreAllOcclusionQueries;
#if FINAL_RELEASE
			const UBOOL bIsViewFrozen = FALSE;
			const UBOOL bHasViewParent = FALSE;
#else
			const UBOOL bIsViewFrozen = Views(0).State && ((FSceneViewState*)Views(0).State)->bIsFrozen;
			const UBOOL bHasViewParent = Views(0).State && ((FSceneViewState*)Views(0).State)->HasViewParent();
#endif
    		const UBOOL bIsOcclusionTesting = bIsOcclusionAllowed && (!bIsWireframe || bIsViewFrozen || bHasViewParent);

			// Skip Editor-only DPGs for game rendering.
			if( GIsGame && (DPGIndex == SDPG_UnrealEdBackground || DPGIndex == SDPG_UnrealEdForeground) )
			{
				continue;
			}

			SCOPED_DRAW_EVENT(EventDPG)(DEC_SCENE_ITEMS,TEXT("DPG %s"),GetSceneDPGName((ESceneDepthPriorityGroup)DPGIndex));

			if (!RenderDPGBegin(DPGIndex, bRequiresClear, bSceneColorDirty, bIsOcclusionTesting))
			{
				continue;
			}

			RenderDPGEnd(DPGIndex, bDeferPrePostProcessResolve, bSceneColorDirty, bIsOcclusionTesting);
		}

		RenderFinish(bDeferPrePostProcessResolve);
	}
	else
	{
#if WITH_MOBILE_RHI
		// special version of Render() for mobile rendering

		// Allocate the maximum scene render target space for the current view family.
		GSceneRenderTargets.Allocate( ViewFamily.RenderTarget->GetSizeX(), ViewFamily.RenderTarget->GetSizeY() );

		// Find the visible primitives.
		InitViews();

		UBOOL bIrrelevantPostProcessResolveSwitch = FALSE;
		UBOOL bIrrelevantSceneColorDirty = FALSE;
		UBOOL bIrrelevantRequiresClear = FALSE;
		UBOOL bIrrelevantIsOccluding = FALSE;

		// for now, only use offscreen render targets for devices with small number of pixels
		if (GMobileUseOffscreenRendering || GMobileUsePostProcess)
		{
			GSceneRenderTargets.BeginRenderingSceneColor(RTUsage_ResolveDepth);
		}

#if WITH_ES2_RHI
		// From ES2RHIDebug.cpp
		extern UBOOL GMobileDebugRender;

		// Initial clear, avoiding color since we touch every pixel
		if( !GMobileDebugRender )
		{
			RHIClear( TRUE, FLinearColor::Black, TRUE, 1.0f, FALSE, 0 );
		}
		else
		{
			// Since we normally avoid the color clear, we need to do it in the visualization case
			RHIClear( TRUE, FLinearColor::White, TRUE, 1.0f, FALSE, 0 );
			extern void SwapBuffers();
			SwapBuffers();
			RHIClear( TRUE, FLinearColor::White, TRUE, 1.0f, FALSE, 0 );
		}
#else
		RHIClear( TRUE, FLinearColor::Black, TRUE, 1.0f, FALSE, 0 );
#endif

		// the only DPGs to render
		ESceneDepthPriorityGroup DPGs[] = { SDPG_World, SDPG_Foreground };

		for( UINT DPGIndex = 0; DPGIndex < ARRAY_COUNT(DPGs); DPGIndex++ )
		{
			SCOPED_DRAW_EVENT(EventDPG)(DEC_SCENE_ITEMS,TEXT("DPG %s"), GetSceneDPGName( DPGs[DPGIndex] ));

#if WITH_ES2_RHI
			extern UBOOL GMobilePrepass;
			if( GMobilePrepass )
			{
				RenderPrePass(SDPG_World, FALSE, -1);
			}
#endif

			if (!RenderDPGBegin( DPGs[ DPGIndex ], bIrrelevantRequiresClear, bIrrelevantSceneColorDirty, FALSE))
			{
				continue;
			}
			RenderDPGEnd( DPGs[ DPGIndex ], bIrrelevantPostProcessResolveSwitch, bIrrelevantSceneColorDirty, bIrrelevantIsOccluding );
		}


		// @todo: Not currently needed as we render screen fades using forward-rendering on mobile
		if( 0 )
		{
			// Draw overlay quad (only for views that currently have non-zero) overlay opacity.  This is only needed
			// on mobile platforms which don't perform the overlay effect in the post-process phase.
			// @todo: If we add overlay to a post-process shader then we can omit this step
			RenderOverlayQuadsIfNeeded();
		}


		// for offscreen rendering, RenderFinish will do the copy of the offscreen buffer to onscreen
		if (GMobileUseOffscreenRendering || GMobileUsePostProcess)
		{
			RHISetMobileSimpleParams(BLEND_Opaque);
			if ( GMobileUsePostProcess )
			{
				SCOPE_CYCLE_COUNTER(STAT_PostProcessDrawTime);
				RenderLightShafts();
			}
			RenderFinish( bIrrelevantPostProcessResolveSwitch );
		}
		else
		{
			// if we don't call RenderFinish, we need to call SaveVisibilityState to avoid a memory leak 
			// of ActorVisibilitySet
			SaveVisibilityState();
		}
#endif	// WITH_ES2_RHI
	}
}


/**
 * Draws a alpha-blended quad over the entire viewport for any views that have overlay opacity > 0
 */
void FSceneRenderer::RenderOverlayQuadsIfNeeded()
{
 	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo* View = &Views( ViewIndex );

		// Any overlay applied to this view?
		if( View->OverlayColor.A > KINDA_SMALL_NUMBER )
		{
			ViewSetViewport(ViewIndex,FALSE);

			// Create a quad
			FBatchedElements BatchedElements;
			INT V00 = BatchedElements.AddVertex(FVector4(-1,-1,0,1),FVector2D(0,0),View->OverlayColor,FHitProxyId());
			INT V10 = BatchedElements.AddVertex(FVector4(1,-1,0,1),FVector2D(1,0),View->OverlayColor,FHitProxyId());
			INT V01 = BatchedElements.AddVertex(FVector4(-1,1,0,1),FVector2D(0,1),View->OverlayColor,FHitProxyId());
			INT V11 = BatchedElements.AddVertex(FVector4(1,1,0,1),FVector2D(1,1),View->OverlayColor,FHitProxyId());

			// Alpha blending, no depth tests or writes, no backface culling.
			RHISetBlendState(TStaticBlendState<BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_Zero,BF_One,CF_Greater,0>::GetRHI());
			RHISetDepthState(TStaticDepthState<FALSE,CF_Always>::GetRHI());
			RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());

			// Draw a quad using the generated vertices.
			BatchedElements.AddTriangle(V00,V10,V11,GWhiteTexture,BLEND_Translucent);
			BatchedElements.AddTriangle(V00,V11,V01,GWhiteTexture,BLEND_Translucent);
			BatchedElements.Draw(
				FMatrix::Identity,
				ViewFamily.RenderTarget->GetSizeX(),
				ViewFamily.RenderTarget->GetSizeY(),
				FALSE
				);
		}
	}
}



/** Renders only the final post processing for the view */
void FSceneRenderer::RenderPostProcessOnly() 
{
	// post process effects passes
	for(UINT DPGIndex = 0;DPGIndex < SDPG_MAX_SceneRender;DPGIndex++)
	{
		RenderPostProcessEffects(DPGIndex);
	}	
	RenderPostProcessEffects(SDPG_PostProcess);

	// Finish rendering for each view.
	{
		SCOPED_DRAW_EVENT(EventFinish)(DEC_SCENE_ITEMS,TEXT("FinishRendering"));
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)		
		{	
			SCOPED_CONDITIONAL_DRAW_EVENT(EventView,Views.Num() > 1)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);
			FinishRenderViewTarget(&Views(ViewIndex));
		}
	}
}

/** Renders the scene's prepass and occlusion queries */
UBOOL FSceneRenderer::RenderPrePass(UINT DPGIndex, UBOOL bIsOcclusionTesting, INT ViewIndex)
{
	SCOPED_DRAW_EVENT(EventPrePass)(DEC_SCENE_ITEMS,TEXT("PrePass"));
	SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_DepthDrawTime, !bIsSceneCapture);

	UBOOL bWorldDpg = (DPGIndex == SDPG_World);
	UBOOL bDirty=0;

	if( !GUseTilingCode || !bWorldDpg )
	{
		GSceneRenderTargets.BeginRenderingPrePass();
	}

	// If no view was specified, loop through all views
	if (ViewIndex == -1)
	{
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			bDirty |= RenderPrePassInner(DPGIndex, bIsOcclusionTesting, ViewIndex);
		}
	}
	else
	{
		bDirty |= RenderPrePassInner(DPGIndex, bIsOcclusionTesting, ViewIndex);
	}

	if( !GUseTilingCode || !bWorldDpg )
	{
		GSceneRenderTargets.FinishRenderingPrePass();
	}

	return bDirty;
}

/** Renders the scene's prepass and occlusion queries */
UBOOL FSceneRenderer::RenderPrePassInner(UINT DPGIndex,UBOOL bIsOcclusionTesting,UINT ViewIndex)
{
	SCOPED_CONDITIONAL_DRAW_EVENT(EventView,Views.Num() > 1)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

	UBOOL bDirty=0;
	FViewInfo &View = Views(ViewIndex);
	UBOOL bWorldDpg = (DPGIndex == SDPG_World);

	RHISetViewParameters(View);

	// Set the device viewport for the view.
	ViewSetViewport(ViewIndex, DPGIndex == SDPG_World);

	//@GEMINI_TODO: the Editor currently relies on the prepass clearing the depth.
	if( GIsEditor || bIsOcclusionTesting || (DPGIndex == SDPG_World) || (DPGIndex == SDPG_Foreground) )
	{
		// Clear the depth buffer as required
		RHIClear(FALSE,FLinearColor::Black,TRUE,1.0f,TRUE,0);
	}

	// Opaque blending, depth tests and writes.
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetDepthState(TStaticDepthState<TRUE,CF_LessEqual>::GetRHI());

	// Draw a depth pass to avoid overdraw in the other passes.
	if(bUseDepthOnlyPass)
	{
		// Write the depths of primitives which were unoccluded the previous frame.
		{
			if (GRHIShaderPlatform != SP_XBOXD3D || !GUseTilingCode)
			{
				// Draw foreground occluders in the world DPG to maximize ZCull efficiency during the world BasePass
				if( bWorldDpg )
				{
					// Set the device viewport for the view.  
					// Set the z range so that the foreground primitives will be in front of everything in the world,
					// Except on xbox where the foreground DPG depths will be resolved and used for shadow receiving, post processing, etc and need to be correct.
					ViewSetViewport(ViewIndex, DPGIndex == SDPG_World, 0.0f, GRHIShaderPlatform == SP_XBOXD3D ? 1.0f : 0.0001f);

					SCOPED_DRAW_EVENT(EventForegroundInWorldPrePass)(DEC_SCENE_ITEMS,TEXT("ForegroundOccluders"));
					bDirty |= RenderDPGPrePass(SDPG_Foreground, View);
				}
			}

			// Set the device viewport for the view, so we only clear our portion of the screen.
			ViewSetViewport(ViewIndex, DPGIndex == SDPG_World);

			// Draw this DPG's occluders
			bDirty |= RenderDPGPrePass(DPGIndex, View);
		}
	}
	return bDirty;
}

/**
 * Renders the prepass for the given DPG and View.
 */
UBOOL FSceneRenderer::RenderDPGPrePass(UINT DPGIndex, FViewInfo& View)
{
	UBOOL bDirty = FALSE;
	// Draw the static occluder primitives using a depth drawing policy.
	{
		// Draw opaque occluders which support a separate position-only vertex buffer to minimize vertex fetch bandwidth,
		// which is often the bottleneck during the depth only pass.
		SCOPED_DRAW_EVENT(EventPosOnly)(DEC_SCENE_ITEMS,TEXT("PosOnly Opaque"));
		bDirty |= Scene->DPGs[DPGIndex].PositionOnlyDepthDrawList.DrawVisible(View,View.StaticMeshOccluderMap);
	}
	{
		// Draw opaque occluders, using double speed z where supported.
		SCOPED_DRAW_EVENT(EventOpaque)(DEC_SCENE_ITEMS,TEXT("Opaque"));
		bDirty |= Scene->DPGs[DPGIndex].DepthDrawList.DrawVisible(View,View.StaticMeshOccluderMap);
	}

	if( !GUsingMobileRHI )
	{
		// Only render masked materials if scene depth needs to be up to date after the prepass
		if (bDominantShadowsActive || GSystemSettings.RenderThreadSettings.bAllowSubsurfaceScattering)
		{
			SCOPED_DRAW_EVENT(EventOpaque)(DEC_SCENE_ITEMS,TEXT("Masked"));
			bDirty |= Scene->DPGs[DPGIndex].MaskedDepthDrawList.DrawVisible(View,View.StaticMeshOccluderMap);
		}

		// Always render what's in the soft masked draw list, regardless of the value of bDominantShadowsActive,
		// Since soft masked objects always need to update the depth buffer to get correct sorting in the base pass.
		{
			SCOPED_DRAW_EVENT(EventOpaque)(DEC_SCENE_ITEMS,TEXT("SoftMasked"));
			bDirty |= Scene->DPGs[DPGIndex].SoftMaskedDepthDrawList.DrawVisible(View,View.StaticMeshOccluderMap);
		}

		const FLOAT ScreenThresholdSq = GetScreenThresholdSq();
		const UBOOL bShowShaderComplexity = (View.Family->ShowFlags & SHOW_ShaderComplexity) != 0;
		// Only render masked materials if scene depth needs to be up to date after the prepass, or if shader complexity is enabled
		const EDepthDrawingMode DepthDrawingMode = 
			(	bDominantShadowsActive
			||	GSystemSettings.RenderThreadSettings.bAllowSubsurfaceScattering
			||	bShowShaderComplexity)
			? DDM_AllOccluders : DDM_NonMaskedOnly;
		// Draw the dynamic occluder primitives using a depth drawing policy.
		TDynamicPrimitiveDrawer<FDepthDrawingPolicyFactory> Drawer(&View,DPGIndex,FDepthDrawingPolicyFactory::ContextType(DepthDrawingMode),TRUE);
		{
			SCOPED_DRAW_EVENT(EventDynamic)(DEC_SCENE_ITEMS,TEXT("Dynamic"));
			for(INT PrimitiveIndex = 0;PrimitiveIndex < View.VisibleDynamicPrimitives.Num();PrimitiveIndex++)
			{
				const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives(PrimitiveIndex);
				const FPrimitiveViewRelevance& PrimitiveViewRelevance = View.PrimitiveViewRelevanceMap(PrimitiveSceneInfo->Id);

				const FLOAT LODFactorDistanceSquared = (PrimitiveSceneInfo->Bounds.Origin - View.ViewOrigin).SizeSquared() * Square(View.LODDistanceFactor);
				if( (PrimitiveSceneInfo->bUseAsOccluder || bShowShaderComplexity) &&
					PrimitiveViewRelevance.GetDPG(DPGIndex) && 
					// Render the primitive if scene depth needs to be up to date after the prepass,
					(bDominantShadowsActive || 
					// Or if shader complexity is enabled,
					bShowShaderComplexity || 
					// Or the primitive takes up enough screen space to be a good occluder.
					Square(PrimitiveSceneInfo->Bounds.SphereRadius) > ScreenThresholdSq * LODFactorDistanceSquared) &&
					// Only draw opaque primitives
					PrimitiveViewRelevance.bOpaqueRelevance
					)
				{
					Drawer.SetPrimitive(PrimitiveSceneInfo);
					PrimitiveSceneInfo->Proxy->DrawDynamicElements(
						&Drawer,
						&View,
						DPGIndex
						);
				}
			}
		}
		bDirty |= Drawer.IsDirty();
	}
	return bDirty;
}

/**
 * Renders the scene's base pass 
 *
 * @param DPGIndex - current depth priority group index
 * @param bIsTiledRendering - TRUE if currently within a Begin/End tiled rendering block
 * @return TRUE if anything was rendered
 */
UBOOL FSceneRenderer::RenderBasePass(UINT DPGIndex, UBOOL bIsTiledRendering)
{
	SCOPED_DRAW_EVENT(EventBasePass)(DEC_SCENE_ITEMS,TEXT("BasePass"));
	SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_BasePassDrawTime, !bIsSceneCapture);
	SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_SceneCaptureBasePassTime, bIsSceneCapture);

	UBOOL bWorldDpg = (DPGIndex == SDPG_World);
	UBOOL bDirty=0;

	// Draw the scene's emissive and light-map color.
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		SCOPED_CONDITIONAL_DRAW_EVENT(EventView,Views.Num() > 1)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);
		FViewInfo& View = Views(ViewIndex);

		// In non tiled MSAA split screen mode, do a per-view Z prepass
		if (GUseMSAASplitScreen)
		{
			RHIMSAABeginRendering(FALSE);
			RenderPrePass(DPGIndex,TRUE,ViewIndex);
		}

		// Opaque blending, depth tests and writes.
		RHISetBlendState(TStaticBlendState<>::GetRHI());
		RHISetDepthState(TStaticDepthState<TRUE,CF_LessEqual>::GetRHI());
 
		// Set the device viewport for the view.
		ViewSetViewport(ViewIndex, DPGIndex == SDPG_World && bIsTiledRendering);

		RHISetViewParameters(View);

        bDirty |= RenderDPGBasePass(DPGIndex,View,bIsTiledRendering);

#if XBOX
		// Render foreground occluders into the depth buffer so that the resolved scene depth texture will have both foreground and world DPG depths, 
		// which is necessary for shadows, PP effects, etc. 
		// This only needs to happen once per frame.
		if( bWorldDpg && (!bIsTiledRendering || GUseMSAASplitScreen) )
		{
			// Set the device viewport for the view
			ViewSetViewport(ViewIndex, TRUE);
			RHISetViewParameters(View);

			SCOPED_DRAW_EVENT(EventForegroundInWorldPrePass)(DEC_SCENE_ITEMS,TEXT("ForegroundOccluders"));
			bDirty |= RenderDPGPrePass(SDPG_Foreground, View);
		}
#endif

		if (GUseMSAASplitScreen && bWorldDpg)
		{
			// In non-tiled MSAA mode, resolve each view
			RHISetBlendState( TStaticBlendState<>::GetRHI());
			RHIMSAAEndRendering(GSceneRenderTargets.GetSceneDepthTexture(), GSceneRenderTargets.GetSceneColorTexture(), ViewIndex);
		}
	}

	// restore color write mask
	RHISetColorWriteMask(CW_RGBA);

	if(GOnePassDominantLight)
	{
		// This is one of the two possible positions we want to render all SoftMasked
		// objects (Ambient+Sun, no other lights). The light attenuation texture is setup for the sun.
		bDirty |= RenderSoftMaskedBase(DPGIndex);
	}

	return bDirty;
}

/** 
* Renders the post process effects for a view. 
* @param DPGIndex - current depth priority group (DPG)
*/
UBOOL FSceneRenderer::RenderPostProcessEffects(UINT DPGIndex, UBOOL bAffectLightingOnly)
{
	SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_PostProcessDrawTime, !bIsSceneCapture);
	SCOPED_DRAW_EVENT(EventPP)(DEC_SCENE_ITEMS,TEXT("PostProcessEffects%s"), bAffectLightingOnly ? TEXT(" LightingOnly") : TEXT(""));

#if USE_PS3_RHI
	// Get the current dual-buffer state of SceneColor.
	FPS3RHISurface::FBufferState BufferState = GSceneRenderTargets.GetSceneColorSurface()->GetBufferState();
#endif

	UBOOL bSetAllocations = FALSE;
	UBOOL bSceneColorDirty = FALSE;
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		SCOPED_CONDITIONAL_DRAW_EVENT(EventView,Views.Num() > 1)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

		FViewInfo& View = Views(ViewIndex);
		RHISetViewParameters(View);

		// Normally, we're going to ping-pong SceneColorLDR between the surface and its resolve-target.
		// But if we're drawing to secondary views (e.g. splitscreen), we must make sure that the final result is drawn to the same
		// memory as view0, so that it will contain the final fullscreen image after all postprocess.
		// To do this, we need to detect if there are an odd number of effects that draw to LDR (on the secondary view), since an
		// even number of ping-pongs will place the final image in the same buffer as view0.
		// On PS3 SceneColorLDR, LightAttenuation and BackBuffer refers to the same memory.
		FSceneColorLDRInfo LDRInfo;
		if ( ViewIndex != 0 )
		{
			LDRInfo.bAdjustPingPong = TRUE;
			for( INT EffectIdx=0; EffectIdx < View.PostProcessSceneProxies.Num(); EffectIdx++ )
			{
				FPostProcessSceneProxy* PPEffectProxy = View.PostProcessSceneProxies(EffectIdx);
			    if( PPEffectProxy
				    && PPEffectProxy->GetDepthPriorityGroup() == DPGIndex 
				    && PPEffectProxy->GetAffectsLightingOnly() == bAffectLightingOnly
					&& PPEffectProxy->MayRenderSceneColorLDR() )
				{
					LDRInfo.NumPingPongsRemaining++;
				}
			}
		}

#if USE_PS3_RHI
		// Reset the dual-buffer state of SceneColor, so that each view reads the same memory (ResolveTarget texture),
		// even if the previous view flipped SceneColor an odd number of times.
		GSceneRenderTargets.GetSceneColorSurface()->SetBufferState( BufferState );
#endif

		// render any custom post process effects
		for( INT EffectIdx=0; EffectIdx < View.PostProcessSceneProxies.Num(); EffectIdx++ )
		{
			FPostProcessSceneProxy* PPEffectProxy = View.PostProcessSceneProxies(EffectIdx);
			if( PPEffectProxy
				&& PPEffectProxy->GetDepthPriorityGroup() == DPGIndex 
				&& PPEffectProxy->GetAffectsLightingOnly() == bAffectLightingOnly)
			{
				if (!bSetAllocations)
				{
					// allocate more GPRs for pixel shaders
					RHISetShaderRegisterAllocation(32, 96);
					bSetAllocations = TRUE;
				}
				// render the effect
				bSceneColorDirty |= PPEffectProxy->Render( Scene,DPGIndex,View,CanvasTransform, LDRInfo);

				// For secondary (split-screen) views, is this an effect that potentially renders to SceneColorLDR?
				if( ViewIndex != 0 && PPEffectProxy->MayRenderSceneColorLDR() )
				{
					LDRInfo.NumPingPongsRemaining--;

					// Have we actually started using SceneColorLDR now?
					if ( View.bUseLDRSceneColor )
					{
						// If so, we don't need any more special handling for this view.
						LDRInfo.bAdjustPingPong = FALSE;
					}
				}
			}
		}
	}

	if (bSetAllocations)
	{
		// restore default GPR allocation
		RHISetShaderRegisterAllocation(64, 64);
	}
	return bSceneColorDirty;
}

/** A simple pixel shader used on PC to read scene depth from scene color alpha and write it to a downsized depth buffer. */
class FDownsampleSceneDepthPixelShader : public FShader
{
	DECLARE_SHADER_TYPE(FDownsampleSceneDepthPixelShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform)
	{ 
		return IsPCPlatform(Platform);
	}

	FDownsampleSceneDepthPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer.ParameterMap);
		ProjectionScaleBiasParameter.Bind(Initializer.ParameterMap,TEXT("ProjectionScaleBias"));
		SourceTexelOffsets01Parameter.Bind(Initializer.ParameterMap,TEXT("SourceTexelOffsets01"));
		SourceTexelOffsets23Parameter.Bind(Initializer.ParameterMap,TEXT("SourceTexelOffsets23"));
	}
	FDownsampleSceneDepthPixelShader() {}

	virtual UBOOL Serialize(FArchive& Ar)
	{
		UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
		Ar << ProjectionScaleBiasParameter;
		Ar << SourceTexelOffsets01Parameter;
		Ar << SourceTexelOffsets23Parameter;
		Ar << SceneTextureParameters;
		return bShaderHasOutdatedParameters;
	}

	FShaderParameter ProjectionScaleBiasParameter;
	FShaderParameter SourceTexelOffsets01Parameter;
	FShaderParameter SourceTexelOffsets23Parameter;
	FSceneTextureShaderParameters SceneTextureParameters;;
};

IMPLEMENT_SHADER_TYPE(,FDownsampleSceneDepthPixelShader,TEXT("DownsampleDepthPixelShader"),TEXT("Main"),SF_Pixel,VER_HALFRES_MOTIONBLURDOF4,0);

/** Updates the downsized depth buffer with the current full resolution depth buffer. */
void FSceneRenderer::UpdateDownsampledDepthSurface()
{
#if CONSOLE
	RHISetRenderTarget(NULL, GSceneRenderTargets.GetSmallDepthSurface());
	/** Updates the quarter-sized depth buffer with the current contents of the scene depth texture. */
	RHIRestoreColorDepth(NULL, GSceneRenderTargets.GetSceneDepthTexture());
#else
	if(!GSupportsDepthTextures)
	{
		// On pc, scene depth is stored in scene color alpha, so we have to resolve it first to get the most recent results
		GSceneRenderTargets.ResolveSceneColor();
	}
			
	// Bind the downsampled translucency buffer as a color buffer
	// D3D requires a color buffer of the same size to be bound, even though we are not writing to color
	RHISetRenderTarget(GSceneRenderTargets.GetTranslucencyBufferSurface(), GSceneRenderTargets.GetSmallDepthSurface());
	RHISetColorWriteEnable(FALSE);
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("DownsampleDepth"));

	for (INT ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FViewInfo& View = Views(ViewIndex);
		// Set shaders and texture
		TShaderMapRef<FScreenVertexShader> ScreenVertexShader(GetGlobalShaderMap());
		TShaderMapRef<FDownsampleSceneDepthPixelShader> PixelShader(GetGlobalShaderMap());

		extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

		FGlobalBoundShaderState DownsampleDepthBoundShaderState;
		SetGlobalBoundShaderState(DownsampleDepthBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *ScreenVertexShader, *PixelShader, sizeof(FFilterVertex));

		// Used to remap view space Z (which is stored in scene color alpha) into post projection z and w so we can write z/w into the downsized depth buffer
		const FVector2D ProjectionScaleBias(View.ProjectionMatrix.M[2][2], View.ProjectionMatrix.M[3][2]);
		SetPixelShaderValue(PixelShader->GetPixelShader(), PixelShader->ProjectionScaleBiasParameter, ProjectionScaleBias);

		const UINT DownsampledBufferSizeX = GSceneRenderTargets.GetBufferSizeX() / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor();
		const UINT DownsampledBufferSizeY = GSceneRenderTargets.GetBufferSizeY() / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor();

		// Offsets of the four full resolution pixels corresponding with a low resolution pixel
		const FVector4 Offsets01(0.0f, 0.0f, 1.0f / DownsampledBufferSizeX, 0.0f);
		SetPixelShaderValue(PixelShader->GetPixelShader(), PixelShader->SourceTexelOffsets01Parameter, Offsets01);
		const FVector4 Offsets23(0.0f, 1.0f / DownsampledBufferSizeY, 1.0f / DownsampledBufferSizeX, 1.0f / DownsampledBufferSizeY);
		SetPixelShaderValue(PixelShader->GetPixelShader(), PixelShader->SourceTexelOffsets23Parameter, Offsets23);
		PixelShader->SceneTextureParameters.Set(&View,*PixelShader);

		RHISetBlendState(TStaticBlendState<>::GetRHI());
		RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetDepthState(TStaticDepthState<TRUE,CF_Always>::GetRHI());

		const UINT DownsampledX = appTrunc(View.RenderTargetX / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor());
		const UINT DownsampledY = appTrunc(View.RenderTargetY / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor());
		const UINT DownsampledSizeX = appTrunc(View.RenderTargetSizeX / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor());
		const UINT DownsampledSizeY = appTrunc(View.RenderTargetSizeY / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor());

		RHISetViewport(DownsampledX, DownsampledY, 0.0f, DownsampledX + DownsampledSizeX, DownsampledY + DownsampledSizeY, 1.0f);
		RHISetViewParameters(View);

		DrawDenormalizedQuad(
			0, 0,
			DownsampledSizeX, DownsampledSizeY,
			View.RenderTargetX, View.RenderTargetY,
			View.RenderTargetSizeX, View.RenderTargetSizeY,
			DownsampledSizeX, DownsampledSizeY,
			GSceneRenderTargets.GetBufferSizeX(), GSceneRenderTargets.GetBufferSizeY()
			);
	}
	RHISetColorWriteEnable(TRUE);
#endif
}

/**
* Clears the scene color depth (stored in alpha channel) to max depth
* This is needed for depth bias blend materials to show up correctly
*/
void FSceneRenderer::ClearSceneColorDepth()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("Clear Depth"));

	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views(ViewIndex);

		ViewSetViewport(ViewIndex,FALSE);

		const FLOAT MaxDistance = 1000000.0f;
		const FLOAT MaxDepth = View.EncodeFloatW(MaxDistance);
		const FLinearColor ClearDepthColor(0,0,0,MaxDepth);

		FBatchedElements BatchedElements;
		INT V00 = BatchedElements.AddVertex(FVector4(-1,-1,0,1),FVector2D(0,0),ClearDepthColor,FHitProxyId());
		INT V10 = BatchedElements.AddVertex(FVector4(1,-1,0,1),FVector2D(1,0),ClearDepthColor,FHitProxyId());
		INT V01 = BatchedElements.AddVertex(FVector4(-1,1,0,1),FVector2D(0,1),ClearDepthColor,FHitProxyId());
		INT V11 = BatchedElements.AddVertex(FVector4(1,1,0,1),FVector2D(1,1),ClearDepthColor,FHitProxyId());
		
		// No alpha blending, no depth tests or writes, no backface culling.
		RHISetBlendState(TStaticBlendState<>::GetRHI());
		RHISetDepthState(TStaticDepthState<FALSE,CF_Always>::GetRHI());
		RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetColorWriteMask(CW_ALPHA);

		// Draw a quad using the generated vertices.
		BatchedElements.AddTriangle(V00,V10,V11,GWhiteTexture,BLEND_Opaque);
		BatchedElements.AddTriangle(V00,V11,V01,GWhiteTexture,BLEND_Opaque);
		BatchedElements.Draw(
			FMatrix::Identity,
			ViewFamily.RenderTarget->GetSizeX(),
			ViewFamily.RenderTarget->GetSizeY(),
			FALSE
			);

		RHISetColorWriteMask(CW_RGBA);
	}
}


/** 
* Renders the scene to capture target textures 
*/
void FSceneRenderer::RenderSceneCaptures()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("Scene Captures"));
	SCOPE_CYCLE_COUNTER(STAT_SceneCaptureRenderingTime);

	// Disable texture fading.
	FLOAT PrevMipLevelFadingState = GEnableMipLevelFading;
	GEnableMipLevelFading = -1.0f;

	// disable tiling for rendering captures
	TGuardValue<UBOOL> GuardUseTilingCode(GUseTilingCode, FALSE);

	for( TSparseArray<FCaptureSceneInfo*>::TConstIterator CaptureIt(Scene->SceneCapturesRenderThread); CaptureIt; ++CaptureIt )
	{
		FCaptureSceneInfo* CaptureInfo = *CaptureIt;
        CaptureInfo->CaptureScene(this);
	}

	// Restore texture fading to its previous state.
	GEnableMipLevelFading = PrevMipLevelFadingState;
}

/** Updates the game-thread actor and primitive visibility states. */
void FSceneRenderer::SaveVisibilityState()
{
	// Update LastRenderTime for the primitives which were visible.
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views(ViewIndex);

		// Replace the view's actor visibility history with the new visibility set.
		if(View.ActorVisibilityHistory)
		{
			check(View.ActorVisibilitySet);
			View.ActorVisibilityHistory->SetStates(View.ActorVisibilitySet);
		}
		else
		{
			check(!View.ActorVisibilitySet);
		}
	}
}

/** 
* Processes a primitive that is determined to be visible.
*
* @param ViewIndex - The index of the view this primitive is visible in
* @param ViewVisibilityMap - A map indicating in which views this primitive is visible
* @param CompactPrimitiveSceneInfo - The Compact primitive scene info of the primitive
* @param DistanceSquared - The distance between the view and the primitive
* @param bIsDefinitelyUnoccluded - TRUE if the primitive is definitely unoccluded
* @return TRUE if the primitive's relevance indicated it needs a PreRenderView for the given view
*/
UBOOL FSceneRenderer::ProcessVisible(
    INT ViewIndex, 
    TBitArray<>& ViewVisibilityMap,
    const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo,
    FLOAT DistanceSquared,
    UBOOL bIsDefinitelyUnoccluded
    )
{
	SCOPE_CONDITIONAL_CYCLE_COUNTER(STAT_ProcessVisibleTime, !bIsSceneCapture);
	FViewInfo& View = Views(ViewIndex);
	FSceneViewState* ViewState = (FSceneViewState*)View.State;

	const INT PrimitiveId = CompactPrimitiveSceneInfo.PrimitiveSceneInfo->Id;

	if (CompactPrimitiveSceneInfo.PrimitiveSceneInfo->StaticMeshes.Num() > 0)
	{
		CONSOLE_PREFETCH(&CompactPrimitiveSceneInfo.PrimitiveSceneInfo->StaticMeshes(0));
		CONSOLE_PREFETCH((char*)(&CompactPrimitiveSceneInfo.PrimitiveSceneInfo->StaticMeshes(0)) + CACHE_LINE_SIZE);
	}

	// Compute the primitive's view relevance.
	FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap(PrimitiveId);
	ViewRelevance = CompactPrimitiveSceneInfo.Proxy->GetViewRelevance(&View);

	// Mark this entry in View.PrimitiveViewRelevanceMap as having been cached for this frame
	ViewRelevance.bInitializedThisFrame = TRUE;

	const UBOOL bShouldUpdateLODFading = GAllowScreenDoorFade && View.State != NULL && View.ViewOrigin.W > 0.0f;
	if(ViewRelevance.bStaticRelevance)
	{
		const FLOAT LODFactorDistanceSquared = DistanceSquared * Square(View.LODDistanceFactor);
	    // Mark the primitive's static meshes as visible.
	    const INT StaticMeshesNum = CompactPrimitiveSceneInfo.PrimitiveSceneInfo->StaticMeshes.Num();

		// Go through the meshes and find the LOD to render
		SBYTE LODToRender = INDEX_NONE;
		for(INT MeshIndex = 0;MeshIndex < StaticMeshesNum;MeshIndex++)
		{
			const FStaticMesh& StaticMesh = CompactPrimitiveSceneInfo.PrimitiveSceneInfo->StaticMeshes(MeshIndex);
			if(	(LODFactorDistanceSquared >= StaticMesh.MinDrawDistanceSquared && LODFactorDistanceSquared < StaticMesh.MaxDrawDistanceSquared) )
			{
				if (bShouldUpdateLODFading && StaticMesh.LODIndex != INDEX_NONE)
				{
					// Update the LOD that should be used this frame and get the LOD that should be used for rendering
					// If a LOD transition fade got kicked off, we need to continue rendering the old LOD for this frame
					LODToRender = UpdatePrimitiveLODUsed(View, StaticMesh.LODIndex, CompactPrimitiveSceneInfo);
				}
				else
				{
					LODToRender = StaticMesh.LODIndex;
				}
				break;
			}
		}
		
		const UBOOL bDrawShadowDepth = Square(CompactPrimitiveSceneInfo.Bounds.SphereRadius) > GMinScreenRadiusForShadowDepthSquared * LODFactorDistanceSquared;
		const FLOAT ScreenThresholdSq = GetScreenThresholdSq();
		const UBOOL bDrawDepthOnly = Square(CompactPrimitiveSceneInfo.Bounds.SphereRadius) > ScreenThresholdSq * LODFactorDistanceSquared;
		extern FLOAT MinScreenRadiusForVelocityPassSquared;
		const UBOOL bDrawVelocity = !CompactPrimitiveSceneInfo.PrimitiveSceneInfo->bStaticShadowing && 
			Square(CompactPrimitiveSceneInfo.Bounds.SphereRadius) > MinScreenRadiusForVelocityPassSquared * LODFactorDistanceSquared;

	    for(INT MeshIndex = 0;MeshIndex < StaticMeshesNum;MeshIndex++)
	    {
		    const FStaticMesh& StaticMesh = CompactPrimitiveSceneInfo.PrimitiveSceneInfo->StaticMeshes(MeshIndex);
		    if(	LODToRender != INDEX_NONE && StaticMesh.LODIndex == LODToRender
				|| LODToRender == INDEX_NONE && LODFactorDistanceSquared >= StaticMesh.MinDrawDistanceSquared && LODFactorDistanceSquared < StaticMesh.MaxDrawDistanceSquared )
		    {
			    // Mark static mesh as visible for rendering
			    View.StaticMeshVisibilityMap(StaticMesh.Id) = TRUE;
				if (bDrawShadowDepth && StaticMesh.CastShadow)
				{
					View.StaticMeshShadowDepthMap(StaticMesh.Id) = TRUE;
				}
			    View.NumVisibleStaticMeshElements++;

			    // If the static mesh is an occluder, check whether it covers enough of the screen to be used as an occluder.
			    if(	StaticMesh.bUseAsOccluder && bDrawDepthOnly )
			    {
				    View.StaticMeshOccluderMap(StaticMesh.Id) = TRUE;
			    }

			    // If the static mesh is an occluder, check whether it covers enough of the screen to be used as an occluder.
			    if(	bDrawVelocity )
			    {
				    View.StaticMeshVelocityMap(StaticMesh.Id) = TRUE;
			    }
		    }
	    }
    }
	else if (bShouldUpdateLODFading && !ViewRelevance.bStaticButFadingRelevance)
	{
		// Update the LOD used for primitives with dynamic relevance
		UpdatePrimitiveLODUsed(View, FMeshElement::QuantizeLODIndex(CompactPrimitiveSceneInfo.Proxy->GetLOD(&View)), CompactPrimitiveSceneInfo);
	}

	if(ViewRelevance.bDynamicRelevance)
	{
		// Keep track of visible dynamic primitives.
		View.VisibleDynamicPrimitives.AddItem(CompactPrimitiveSceneInfo.PrimitiveSceneInfo);

		View.NumVisibleDynamicPrimitives++;
	}

	if( ViewRelevance.bTranslucentRelevance )
	{
		for (UINT CheckDPGIndex = 0; CheckDPGIndex < SDPG_MAX_SceneRender; CheckDPGIndex++)
		{
			if (ViewRelevance.GetDPG(CheckDPGIndex) == TRUE)
			{
				// Add to set of dynamic translucent primitives
				View.TranslucentPrimSet[CheckDPGIndex].AddScenePrimitive(
					CompactPrimitiveSceneInfo.PrimitiveSceneInfo,
					View,
					ViewRelevance.bUsesSceneColor,
					ViewRelevance.bDynamicLitTranslucencyPrepass,
					ViewRelevance.bDynamicLitTranslucencyPostRenderDepthPass,
					ViewRelevance.bSeparateTranslucencyRelevance);

				if( ViewRelevance.bDistortionRelevance )
				{
					// Add to set of dynamic distortion primitives
					View.DistortionPrimSet[CheckDPGIndex].AddScenePrimitive(CompactPrimitiveSceneInfo.PrimitiveSceneInfo,View);
				}

				if( ViewRelevance.bTranslucencyDoFRelevance && CheckDPGIndex == SDPG_World )
				{
					View.bRenderedToDoFBlurBuffer = TRUE;
				}
			}
		}
		if (ViewRelevance.bInheritDominantShadowsRelevance)
		{
			bHasInheritDominantShadowMaterials = TRUE;
		}
	}

	if( ViewRelevance.bSoftMaskedRelevance )
	{
		for (UINT CheckDPGIndex = 0; CheckDPGIndex < SDPG_MAX_SceneRender; CheckDPGIndex++)
		{
			if (ViewRelevance.GetDPG(CheckDPGIndex) == TRUE)
			{
				// Add to set of dynamic translucent primitives
				View.TranslucentPrimSet[CheckDPGIndex].AddScenePrimitiveSoftMasked(CompactPrimitiveSceneInfo.PrimitiveSceneInfo,View);
			}
		}
	}

	if( ViewRelevance.bDecalStaticRelevance )
	{
		// determine static mesh visibility generated by decal interactions
		for( INT DecalIdx = 0; DecalIdx < CompactPrimitiveSceneInfo.Proxy->Decals.Num(); DecalIdx++ )
		{
			FDecalInteraction* Decal = CompactPrimitiveSceneInfo.Proxy->Decals(DecalIdx);
			if( Decal &&
				Decal->DecalStaticMesh &&
				DistanceSquared >= Decal->DecalStaticMesh->MinDrawDistanceSquared && 
				DistanceSquared < Decal->DecalStaticMesh->MaxDrawDistanceSquared )
			{
				// Distance cull using decal's CullDistance (perspective views only)
				FLOAT SquaredDistanceToDecal = 0.0f;
				if( View.ViewOrigin.W > 0.0f )
				{
					// Compute the distance between the view and the decal
					SquaredDistanceToDecal = ( Decal->DecalState.Bounds.GetCenter() - View.ViewOrigin ).SizeSquared();
				}

				// Distance cull using decal's CullDistance
				const FLOAT SquaredCullDistance = Decal->DecalState.SquaredCullDistance;
				if( SquaredCullDistance == 0.0f || SquaredDistanceToDecal < SquaredCullDistance )
				{
					// Frustum check
					const FBox& DecalBounds = Decal->DecalState.Bounds;
					if( View.ViewFrustum.IntersectBox( DecalBounds.GetCenter(), DecalBounds.GetExtent() ) )
					{
						// Mark the decal static mesh as visible for rendering
						View.DecalStaticMeshVisibilityMap(Decal->DecalStaticMesh->Id) = TRUE;
					}
				}
			}
		}
	}

	if( ViewRelevance.IsRelevant() )
    {
		if( ViewRelevance.bDecalDynamicRelevance )
		{
			// Find set of dynamic decals with different material relevancies
			UBOOL bHasDistortion = FALSE;
			UBOOL bHasOpaque = FALSE;
			UBOOL bHasTranslucent = FALSE;
			for( INT DecalIdx = 0; DecalIdx < CompactPrimitiveSceneInfo.Proxy->Decals.Num(); DecalIdx++ )
			{
				FDecalInteraction* Decal = CompactPrimitiveSceneInfo.Proxy->Decals(DecalIdx);
				if( Decal)
				{
					bHasDistortion |= Decal->DecalState.MaterialViewRelevance.bDistortion;
					bHasOpaque |= Decal->DecalState.MaterialViewRelevance.bOpaque;
					bHasOpaque |= Decal->DecalState.MaterialViewRelevance.bMasked;
					bHasTranslucent |= Decal->DecalState.MaterialViewRelevance.bTranslucency;
				}
			}
			// add relevant dynamic decals to their DPG lists
			if( bHasDistortion | bHasOpaque | bHasTranslucent )
			{
				for (UINT CheckDPGIndex = 0; CheckDPGIndex < SDPG_MAX_SceneRender; CheckDPGIndex++)
				{
					if (ViewRelevance.GetDPG(CheckDPGIndex) == TRUE)
					{
						if (bHasDistortion)
						{
							// Add to set of dynamic distortion primitives
							View.DistortionPrimSet[CheckDPGIndex].AddScenePrimitive( CompactPrimitiveSceneInfo.PrimitiveSceneInfo,View );								
						}
						if (bHasOpaque)
						{
							// Add to set of dynamic opaque decal primitives
							View.VisibleOpaqueDynamicDecalPrimitives[CheckDPGIndex].AddItem( CompactPrimitiveSceneInfo.PrimitiveSceneInfo );
						}
						if (bHasTranslucent)
						{
							// Add to set of dynamic translucent decal primitives
							View.VisibleTranslucentDynamicDecalPrimitives[CheckDPGIndex].AddItem( CompactPrimitiveSceneInfo.PrimitiveSceneInfo );
						}
					}
				}
			}
		}

		FPrimitiveSceneInfo* PrimitiveSceneInfo = CompactPrimitiveSceneInfo.PrimitiveSceneInfo;

		// This primitive is in the view frustum, view relevant, and unoccluded; it's visible.
		View.PrimitiveVisibilityMap(PrimitiveId) = TRUE;
        ViewVisibilityMap(ViewIndex) = TRUE;

		// If the primitive's static meshes need to be updated before they can be drawn, update them now.
		PrimitiveSceneInfo->ConditionalUpdateStaticMeshes();

		// Check to see if the primitive has a decal which is using a lit material
		const UBOOL bHasLitDecals = CompactPrimitiveSceneInfo.Proxy->HasLitDecals(&View) && ViewRelevance.bDecalDynamicRelevance;

		// Iterate over the lights affecting the primitive.
		for(FLightPrimitiveInteraction* Interaction = PrimitiveSceneInfo->LightList;
			Interaction;
			Interaction = Interaction->GetNextLight()
			)
		{
			CONSOLE_PREFETCH(Interaction->GetNextLight());

			// The light doesn't need to be rendered if it only affects light-maps or if it is a skylight.
			const FLightSceneInfo* LightSceneInfo = Interaction->GetLight();
			const UBOOL bForceLightDynamic = ViewRelevance.bForceDirectionalLightsDynamic && (LightSceneInfo->LightType == LightType_Directional || LightSceneInfo->LightType == LightType_DominantDirectional);
			const UBOOL bIsNotSkyLight = !Interaction->IsLightMapped() && LightSceneInfo->LightType != LightType_Sky;
			const UBOOL bDirectionalLightAttached = PrimitiveSceneInfo->DynamicLightSceneInfo && PrimitiveSceneInfo->DynamicLightSceneInfo->LightType == LightType_Directional;

			const UBOOL bIsBasePassLitLightEnvironmentLight = 
				LightSceneInfo->LightEnvironment 
				// Check if the current light is a directional light and it will be rendered in the base pass
				&& (PrimitiveSceneInfo->DynamicLightSceneInfo && PrimitiveSceneInfo->DynamicLightSceneInfo == LightSceneInfo && LightSceneInfo->LightType == LightType_Directional 
				// Or check if the current light is a SH light (the directional light must also exist), 
				|| LightSceneInfo->LightType == LightType_SphericalHarmonic && bDirectionalLightAttached
				// And it is allowed in the base pass 
				&& (PrimitiveSceneInfo->bRenderSHLightInBasePass 
				// Or if it is not allowed but the primitive is in the foreground DPG and foreground self-shadowing is disabled.
				// In this case there will not be a modulated shadow pass between the base pass and the SH light so it should be merged.
				|| PrimitiveSceneInfo->SHLightSceneInfo && ViewRelevance.GetDPG(SDPG_Foreground) && !GSystemSettings.bEnableForegroundSelfShadowing));
			// Check whether the light is a dominant light that will be applied in the base pass
			const UBOOL bIsBasePassLitShadowedLight = GOnePassDominantLight
				&& IsDominantLightType(LightSceneInfo->LightType);
			// Determine whether the light is an SH light that will be applied the base pass
			const UBOOL bIsBasePassSHLight = GOnePassDominantLight 
				&& PrimitiveSceneInfo->DynamicLightSceneInfo 
				&& IsDominantLightType(PrimitiveSceneInfo->DynamicLightSceneInfo->LightType)
				&& PrimitiveSceneInfo->SHLightSceneInfo 
				&& PrimitiveSceneInfo->SHLightSceneInfo == LightSceneInfo
				&& LightSceneInfo->LightType == LightType_SphericalHarmonic;
			const UBOOL bIgnoredDominantLight = IsDominantLightType(LightSceneInfo->LightType) 
				&& PrimitiveSceneInfo->BrightestDominantLightSceneInfo 
				&& PrimitiveSceneInfo->BrightestDominantLightSceneInfo != LightSceneInfo;

			const UBOOL bRenderLight = bForceLightDynamic || bIsNotSkyLight;
			const UBOOL bMergedIntoBasePass = bIsBasePassLitLightEnvironmentLight || bIsBasePassLitShadowedLight || bIsBasePassSHLight || bIgnoredDominantLight;
			
			// Use multi-pass lighting to render dynamic, non-skylight lights that are not going to be merged into the base pass.
			Interaction->SetNeedsLightRenderingPass(bRenderLight && !bMergedIntoBasePass);

			if ( bRenderLight )
			{
				FVisibleLightViewInfo& VisibleLightViewInfo = View.VisibleLightInfos(Interaction->GetLightId());
				for(UINT DPGIndex = 0;DPGIndex < SDPG_MAX_SceneRender;DPGIndex++)
				{
					if(ViewRelevance.GetDPG(DPGIndex))
					{
						// indicate that the light is affecting some static or dynamic lit primitives
						VisibleLightViewInfo.DPGInfo[DPGIndex].bHasVisibleLitPrimitives = TRUE;

						if (!bMergedIntoBasePass)
						{
							if( ViewRelevance.bDynamicRelevance )
							{
								// Add dynamic primitives to the light's list of visible dynamic affected primitives.
								VisibleLightViewInfo.DPGInfo[DPGIndex].VisibleDynamicLitPrimitives.AddItem(PrimitiveSceneInfo);
							}
							if ( bHasLitDecals )
							{
								// Add to the light's list of The primitives which are visible, affected by this light and receiving lit decals.
								VisibleLightViewInfo.DPGInfo[DPGIndex].VisibleLitDecalPrimitives.AddItem(PrimitiveSceneInfo);
							}
						}
					}
				}
			}
		}

		// If the primitive's last render time is older than last frame, consider it newly visible and update its visibility change time
		if (PrimitiveSceneInfo->LastRenderTime < ViewFamily.CurrentWorldTime - ViewFamily.DeltaWorldTime - DELTA)
		{
			PrimitiveSceneInfo->LastVisibilityChangeTime = ViewFamily.CurrentWorldTime;
		}
		PrimitiveSceneInfo->LastRenderTime = ViewFamily.CurrentWorldTime;

		// If the primitive is definitely unoccluded (rather than only estimated to be unoccluded), then update the
		// signal the game thread that it's visible.
		if(bIsDefinitelyUnoccluded)
		{
			// Update the PrimitiveComponent's LastRenderTime.
			PrimitiveSceneInfo->Component->LastRenderTime = ViewFamily.CurrentWorldTime;

			if(PrimitiveSceneInfo->Owner)
			{
				if(View.ActorVisibilitySet)
				{
					// Add the actor to the visible actor set.
					View.ActorVisibilitySet->AddActor(PrimitiveSceneInfo->Owner);
				}

				// Update the actor's LastRenderTime.
				PrimitiveSceneInfo->Owner->LastRenderTime = ViewFamily.CurrentWorldTime;
			}

			// Store the primitive for parent occlusion rendering.
			if ( ViewState && ViewState->IsViewParent() )
			{
				ViewState->ParentPrimitives.Add(PrimitiveSceneInfo->Component);
			}
			// update last render time for decals rendered on this receiver primitive
			if( PrimitiveSceneInfo->Proxy )
			{
				for( INT DecalIdx=0; DecalIdx < PrimitiveSceneInfo->Proxy->Decals.Num(); DecalIdx++ )
				{
					FDecalInteraction* DecalInteraction = PrimitiveSceneInfo->Proxy->Decals(DecalIdx);
					if( DecalInteraction && 
						DecalInteraction->Decal )
					{
						DecalInteraction->Decal->LastRenderTime = ViewFamily.CurrentWorldTime;
					}
				}
			}
		}
		#if !FINAL_RELEASE
			// if we are freezing the scene, then remember the primitive was rendered
			if (ViewState && ViewState->bIsFreezing)
			{
				ViewState->FrozenPrimitives.Add(PrimitiveSceneInfo->Component);
			}
		#endif
	}

    return ViewRelevance.bNeedsPreRenderView;
}

/*-----------------------------------------------------------------------------
Computing Minimal Resolve Rectangles
-----------------------------------------------------------------------------*/

/** 
* template function to compute the viewspace bounds of a ViewBox along a given axis.
*
* @param PrimVerts - an array of vertices describing a convex hull in view space
* @param PrimBounds - [out] the view space bounds of the hull 
* @return TRUE is a valid set of bounds were found, FALSE if the convex hull is completely offscreen
*/
template <INT AXIS>
UBOOL CalculateAxisBounds(const TArray<FVector4>& PrimVerts, FBox& PrimBounds)
{
	UBOOL VisibleResultsFound = FALSE;
	const FLOAT VMin = -1.0f;
	const FLOAT VMax = +1.0f;
	const INT	MinOut = 2;
	const INT	MaxOut = 1;


	PrimBounds.IsValid = TRUE;
	PrimBounds.Min[AXIS] = VMax;
	PrimBounds.Max[AXIS] = VMin;

	INT Ocumulate = 0;
	INT Acumulate = ~0;
	INT AnythingVisible = 0;

	// Allocate an array of flags based on the primitive vertex count
	const INT NumPrimVerts = PrimVerts.Num();
	TArray<INT> OutFlags(NumPrimVerts);

	for (INT i=0; i<NumPrimVerts; ++i)
	{
		const FVector4& Vertex = PrimVerts(i);
		INT& OutFlag = OutFlags(i);
		
		OutFlag = 0;

		FLOAT MinOffset = Vertex[AXIS] - (VMin * Vertex.W);
		FLOAT MaxOffset = Vertex[AXIS] - (VMax * Vertex.W);

		// set min/max out flags for this Axis
		if (MinOffset < 0)
		{
			// point is offscreen to the min side
			OutFlag |= MinOut;
		}
		if (MaxOffset > 0)
		{
			// point is offscreen to the max side
			OutFlag |= MaxOut;
		}

		// keep running OR and AND of flags
		Ocumulate |= OutFlag;
		Acumulate &= OutFlag;

		// update our bounds prediction if fully visible
		if (OutFlag == 0)
		{
			AnythingVisible = 1;

			FLOAT PrimBoundsMin = Vertex[AXIS] - (PrimBounds.Min[AXIS] * Vertex.W);
			FLOAT PrimBoundsMax = Vertex[AXIS] - (PrimBounds.Max[AXIS] * Vertex.W);

			if (PrimBoundsMin < 0)
			{
				PrimBounds.Min[AXIS] = Vertex[AXIS] / Vertex.W;
			}
			if (PrimBoundsMax > 0)
			{
				PrimBounds.Max[AXIS] = Vertex[AXIS] / Vertex.W;
			}
		}
	}

	if (Ocumulate == 0)
	{
		// Everything was fully onscreen, bounds are good as-is
		VisibleResultsFound = TRUE;
	}
	else if (Acumulate != 0)
	{
		// Everything was fully offscreen
		VisibleResultsFound = FALSE;
	}
	else if (AnythingVisible == 0)
	{
		// Everything was offscreen, but still may span across the screen.
		// We have to use the full range to be safe
		PrimBounds.Min[AXIS] = VMin;
		PrimBounds.Max[AXIS] = VMax;
		VisibleResultsFound = TRUE;
	}
	else
	{
		// we have a mix of onscreen and offscreen points. We'll need a second pass to determine the true bounds
		VisibleResultsFound = TRUE;

		for (INT i=0; i<NumPrimVerts; ++i)
		{
			const FVector4& Vertex = PrimVerts(i);
			INT& OutFlag = OutFlags(i);

			if ((OutFlag & MinOut) &&
				(Vertex[AXIS] - (PrimBounds.Min[AXIS] * Vertex.W) < 0))
			{
				PrimBounds.Min[AXIS] = VMin;
			}

			if ((OutFlag & MaxOut) &&
				(Vertex[AXIS] - (PrimBounds.Max[AXIS] * Vertex.W) > 0))
			{
				PrimBounds.Max[AXIS] = VMax;
			}
		}
	}

	return VisibleResultsFound;
}

/** 
* Helper function to compute the viewspace bounds of a ViewBox in two dimentions 
* @param PrimVerts - an array of vertices describing a convex hull in view space
* @param PrimBounds - [out] the view space bounds of the hull 
* @return TRUE is a valid set of bounds were found, FALSE if the convex hull is completely offscreen
*/
static UBOOL CalculateViewBounds(const TArray<FVector4>& PrimVerts, FBox& PrimBounds)
{
	// we are onscreen if both axis report an onscreen result
	return CalculateAxisBounds<0>(PrimVerts, PrimBounds) && CalculateAxisBounds<1>(PrimVerts, PrimBounds);
}

/** 
* Helper interface to fetch distortion primitive data from a view 
*/
class DistortionPrimitiveFetchInterface
{
public:
	static INT FetchCount(const FViewInfo& View, INT DPGIndex) 
	{ 
		return View.DistortionPrimSet[DPGIndex].NumPrims(); 
	}
	static const FPrimitiveSceneInfo* FetchPrimitive(const FViewInfo& View, INT DPGIndex, INT PrimIndex) 
	{ 
		return View.DistortionPrimSet[DPGIndex].GetPrim(PrimIndex); 
	}
};

/** 
* Helper interface to fetch translucent primitive data from a view 
*/
class TranslucentPrimitiveFetchInterface
{
public:
	static INT FetchCount(const FViewInfo& View, INT DPGIndex) 
	{ 
		return View.TranslucentPrimSet[DPGIndex].NumSceneColorPrims(); 
	}
	static const FPrimitiveSceneInfo* FetchPrimitive(const FViewInfo& View, INT DPGIndex, INT PrimIndex) 
	{ 
		return View.TranslucentPrimSet[DPGIndex].GetSceneColorPrim(PrimIndex); 
	}
};

/** 
* template function to determine the screen pixel extents of a set of primitives, using one of the fetch interfaces above 
*
* @param Views - set of views to find screen extents for as the rect can span multiple views
* @param DPGIndex - current depth group pass being rendered
* @param PixelRect - [out] screen space min,max bounds for primitives
* @return TRUE if any of the primitives had visible bounds on screen
*/
template <typename FETCH_INTERFACE>
UBOOL ComputePixelBoundsOfPrimitives(const TArray<FViewInfo>& Views, INT DPGIndex, FIntRect& PixelRect)
{
	FBox ScreenBounds(0);
	FIntRect RenderTargetBounds(0,0,0,0);
	UBOOL VisibleBoundsFound= FALSE;

	PixelRect.Min.X= 0;
	PixelRect.Min.Y= 0;
	PixelRect.Max.X= 0;
	PixelRect.Max.Y= 0;
	
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views(ViewIndex);
		const INT NumPrims= FETCH_INTERFACE::FetchCount(View, DPGIndex);
		
		// update our global render target bounds for all views
		if (ViewIndex == 0)
		{
			RenderTargetBounds.Min.X = View.RenderTargetX;
			RenderTargetBounds.Min.Y = View.RenderTargetY;
			RenderTargetBounds.Max.X = View.RenderTargetX + View.RenderTargetSizeX;
			RenderTargetBounds.Max.Y = View.RenderTargetY + View.RenderTargetSizeY;
		}
		else
		{
			RenderTargetBounds.Min.X = Min(RenderTargetBounds.Min.X, View.RenderTargetX);
			RenderTargetBounds.Min.Y = Min(RenderTargetBounds.Min.Y, View.RenderTargetY);
			RenderTargetBounds.Max.X = Max(RenderTargetBounds.Max.X, View.RenderTargetX + View.RenderTargetSizeX);
			RenderTargetBounds.Max.Y = Max(RenderTargetBounds.Max.Y, View.RenderTargetY + View.RenderTargetSizeY);
		}

		FBox ViewBounds(0);

		// build an array to hold all the bounding-box vertices in this view
		FVector WorldVerts[8];
		TArray<FVector4> ViewSpaceVerts(8);

		for (INT PrimIndex=0; PrimIndex<NumPrims; ++PrimIndex)
		{
			// get the primitive
			const FPrimitiveSceneInfo* Prim= FETCH_INTERFACE::FetchPrimitive(View, DPGIndex, PrimIndex);
			const FBox PrimBox= Prim->Bounds.GetBox();

			// extract the eight vertices of the bounding box
			WorldVerts[0] = FVector(PrimBox.Min);
			WorldVerts[1] = FVector(PrimBox.Min.X, PrimBox.Min.Y, PrimBox.Max.Z);
			WorldVerts[2] = FVector(PrimBox.Min.X, PrimBox.Max.Y, PrimBox.Min.Z);
			WorldVerts[3] = FVector(PrimBox.Max.X, PrimBox.Min.Y, PrimBox.Min.Z);
			WorldVerts[4] = FVector(PrimBox.Max.X, PrimBox.Max.Y, PrimBox.Min.Z);
			WorldVerts[5] = FVector(PrimBox.Max.X, PrimBox.Min.Y, PrimBox.Max.Z);
			WorldVerts[6] = FVector(PrimBox.Min.X, PrimBox.Max.Y, PrimBox.Max.Z);
			WorldVerts[7] = FVector(PrimBox.Max);


			// transform each to homogenous view space to setup our ViewVert elements
			ViewSpaceVerts(0) = View.ViewProjectionMatrix.TransformFVector(WorldVerts[0]);
			ViewSpaceVerts(1) = View.ViewProjectionMatrix.TransformFVector(WorldVerts[1]);
			ViewSpaceVerts(2) = View.ViewProjectionMatrix.TransformFVector(WorldVerts[2]);
			ViewSpaceVerts(3) = View.ViewProjectionMatrix.TransformFVector(WorldVerts[3]);
			ViewSpaceVerts(4) = View.ViewProjectionMatrix.TransformFVector(WorldVerts[4]);
			ViewSpaceVerts(5) = View.ViewProjectionMatrix.TransformFVector(WorldVerts[5]);
			ViewSpaceVerts(6) = View.ViewProjectionMatrix.TransformFVector(WorldVerts[6]);
			ViewSpaceVerts(7) = View.ViewProjectionMatrix.TransformFVector(WorldVerts[7]);

			// determine the screen space bounds of this box
			FBox PrimBounds(0);
			if (CalculateViewBounds(ViewSpaceVerts, PrimBounds))
			{
				ViewBounds += PrimBounds;
			}
		}

		if (ViewBounds.IsValid)
		{
			// convert the view bounds to screen space
			FIntRect ViewPixels;

			// mirror the view bounds on the Y axis
			FLOAT OldMinY= ViewBounds.Min.Y;
			ViewBounds.Min.Y= -ViewBounds.Max.Y;
			ViewBounds.Max.Y= -OldMinY;

			// convert each axis from (-1,+1) to (0,1) range
			const FVector HalfScale(0.5f, 0.5f, 0.0f);
			ViewBounds.Min = (ViewBounds.Min * HalfScale) + HalfScale;
			ViewBounds.Max = (ViewBounds.Max * HalfScale) + HalfScale;
			ViewBounds.Min.X= Clamp(ViewBounds.Min.X, 0.0f, 1.0f);
			ViewBounds.Min.Y= Clamp(ViewBounds.Min.Y, 0.0f, 1.0f);
			ViewBounds.Max.X= Clamp(ViewBounds.Max.X, 0.0f, 1.0f);
			ViewBounds.Max.Y= Clamp(ViewBounds.Max.Y, 0.0f, 1.0f);

			// scale and offset to the pixel dimensions of this view
			ViewBounds.Min.X *= (FLOAT)View.RenderTargetSizeX;
			ViewBounds.Min.Y *= (FLOAT)View.RenderTargetSizeY;
			ViewBounds.Max.X *= (FLOAT)View.RenderTargetSizeX;
			ViewBounds.Max.Y *= (FLOAT)View.RenderTargetSizeY;

			ViewBounds.Min.X += (FLOAT)View.RenderTargetX;
			ViewBounds.Min.Y += (FLOAT)View.RenderTargetY;
			ViewBounds.Max.X += (FLOAT)View.RenderTargetX;
			ViewBounds.Max.Y += (FLOAT)View.RenderTargetY;

			// add to the render target bounds
			ScreenBounds += ViewBounds;
		}
	}

	if (ScreenBounds.IsValid &&
		RenderTargetBounds.Area() > 0)
	{
		// scale the final screen bounds to pixel dimensions
		PixelRect.Min.X = appTrunc(ScreenBounds.Min.X) - 1;
		PixelRect.Min.Y = appTrunc(ScreenBounds.Min.Y) - 1;
		PixelRect.Max.X = appTrunc(ScreenBounds.Max.X) + 1;
		PixelRect.Max.Y = appTrunc(ScreenBounds.Max.Y) + 1;

		// snap to 32-pixel alignment
		PixelRect.Min.X = (PixelRect.Min.X   ) & ~31; //round down
		PixelRect.Max.X = (PixelRect.Max.X+31) & ~31; //round up	
		PixelRect.Min.Y = (PixelRect.Min.Y   ) & ~31;	
		PixelRect.Max.Y = (PixelRect.Max.Y+31) & ~31;

		// clamp to the total render target bounds we found
		PixelRect.Min.X = Clamp(PixelRect.Min.X, RenderTargetBounds.Min.X, RenderTargetBounds.Max.X);
		PixelRect.Max.X = Clamp(PixelRect.Max.X, RenderTargetBounds.Min.X, RenderTargetBounds.Max.X);
		PixelRect.Min.Y = Clamp(PixelRect.Min.Y, RenderTargetBounds.Min.Y, RenderTargetBounds.Max.Y);
		PixelRect.Max.Y = Clamp(PixelRect.Max.Y, RenderTargetBounds.Min.Y, RenderTargetBounds.Max.Y);

		// make sure it's legit
		INT XSize= PixelRect.Max.X - PixelRect.Min.X;
		INT YSize= PixelRect.Max.Y - PixelRect.Min.Y;
		if (XSize > 0 && YSize > 0)
		{
			// valid onscreen bounds were found
			VisibleBoundsFound = TRUE;
		}
	}

	return VisibleBoundsFound;
}

/** 
* Helper used to compute the minimal screen bounds of all translucent primitives which require scene color 
*
* @param DPGIndex - current depth group pass being rendered
* @param PixelRect - [out] screen space min,max bounds for primitives
* @return TRUE if any of the primitives had visible bounds on screen
*/
UBOOL FSceneRenderer::ComputeTranslucencyResolveRectangle(INT DPGIndex, FIntRect& PixelRect)
{
	return ComputePixelBoundsOfPrimitives<TranslucentPrimitiveFetchInterface>(Views, DPGIndex, PixelRect);
}

/** Helper used to compute the Squared Screen Sized threshold used to determine if a primitive should render to depth */
FLOAT FSceneRenderer::GetScreenThresholdSq() const
{
	FLOAT ScreenThresholdSq = (Scene->NumWholeSceneShadowLights > 0) || (Scene->NumDirectionalLightFunctions > 0) ? GMinScreenRadiusForDepthPrepassWithShadowsSquared : GMinScreenRadiusForDepthPrepassSquared;
	return ScreenThresholdSq;
}
/** 
* Helper used to compute the minimual screen bounds of all distortion primitives which require scene color 
* 
* @param DPGIndex - current depth group pass being rendered
* @param PixelRect - [out] screen space min,max bounds for primitives
* @return TRUE if any of the primitives had visible bounds on screen
*/
UBOOL FSceneRenderer::ComputeDistortionResolveRectangle(INT DPGIndex, FIntRect& PixelRect)
{
	return ComputePixelBoundsOfPrimitives<DistortionPrimitiveFetchInterface>(Views, DPGIndex, PixelRect);
}

/*-----------------------------------------------------------------------------
BeginRenderingViewFamily
-----------------------------------------------------------------------------*/

/**
 * Helper function performing actual work in render thread.
 *
 * @param SceneRenderer	Scene renderer to use for rendering.
 */
static void RenderViewFamily_RenderThread( FSceneRenderer* SceneRenderer )
{
    FMemMark MemStackMark(GRenderingThreadMemStack);

    {
		SCOPE_CYCLE_COUNTER(STAT_TotalSceneRenderingTime);

		if(SceneRenderer->ViewFamily.ShowFlags & SHOW_HitProxies)
		{
			// Render the scene's hit proxies.
			SceneRenderer->RenderHitProxies();
		}
		else
		{
			if(SceneRenderer->ViewFamily.ShowFlags & SHOW_SceneCaptureUpdates)
			{
				// Render the scene for each capture
				SceneRenderer->RenderSceneCaptures();
			}

			// Render the scene.
			SceneRenderer->Render();
		}

        // Delete the scene renderer.
		delete SceneRenderer;
	}

#if STATS
#if CONSOLE
	/** Update STATS with the total GPU time taken to render the last frame. */
	SET_CYCLE_COUNTER(STAT_TotalGPUFrameTime, RHIGetGPUFrameCycles(), 1);
#endif
	const FCycleStat* InitViewsStat = GStatManager.GetCycleStat(STAT_InitViewsTime);
	if (InitViewsStat)
	{
		SET_CYCLE_COUNTER(STAT_InitViewsTime2, InitViewsStat->Cycles, InitViewsStat->NumCallsPerFrame);
	}
	const FCycleStat* OcclusionResultStat = GStatManager.GetCycleStat(STAT_OcclusionResultTime);
	if (OcclusionResultStat)
	{
		SET_CYCLE_COUNTER(STAT_OcclusionResultTime2, OcclusionResultStat->Cycles, OcclusionResultStat->NumCallsPerFrame);
	}
	const FCycleStat* ProjectedShadowsStat = GStatManager.GetCycleStat(STAT_ProjectedShadowDrawTime);
	if (ProjectedShadowsStat)
	{
		SET_CYCLE_COUNTER(STAT_ShadowRendering, ProjectedShadowsStat->Cycles, ProjectedShadowsStat->NumCallsPerFrame);
	}

	const FCycleStat* DepthPassStat = GStatManager.GetCycleStat(STAT_DepthDrawTime);
	const FCycleStat* BasePassStat = GStatManager.GetCycleStat(STAT_BasePassDrawTime);
	const FCycleStat* LightingStat = GStatManager.GetCycleStat(STAT_LightingDrawTime);
	const FCycleStat* UnlitDecalsStat = GStatManager.GetCycleStat(STAT_UnlitDecalDrawTime);
	const FCycleStat* BeginOcclusionStat = GStatManager.GetCycleStat(STAT_BeginOcclusionTestsTime);
	const FCycleStat* TranslucencyStat = GStatManager.GetCycleStat(STAT_TranslucencyDrawTime);
	const FCycleStat* VelocityStat = GStatManager.GetCycleStat(STAT_VelocityDrawTime);
	const FCycleStat* PostProcessStat = GStatManager.GetCycleStat(STAT_PostProcessDrawTime);
	const FCycleStat* SceneCaptureStat = GStatManager.GetCycleStat(STAT_SceneCaptureRenderingTime);
	const FCycleStat* RenderViewFamilyStat = GStatManager.GetCycleStat(STAT_TotalSceneRenderingTime);
	
	if (InitViewsStat && DepthPassStat && BasePassStat && ProjectedShadowsStat && LightingStat && UnlitDecalsStat && BeginOcclusionStat && TranslucencyStat && VelocityStat && SceneCaptureStat && PostProcessStat && RenderViewFamilyStat)
	{
		const DWORD AccumulatedCycles = 
			InitViewsStat->Cycles +
			DepthPassStat->Cycles +
			BasePassStat->Cycles +
			ProjectedShadowsStat->Cycles +
			LightingStat->Cycles +
			UnlitDecalsStat->Cycles + 
			BeginOcclusionStat->Cycles +
			TranslucencyStat->Cycles +
			VelocityStat->Cycles +
			PostProcessStat->Cycles + 
			SceneCaptureStat->Cycles;

		DWORD UnaccountedCycles = 0;
		if (RenderViewFamilyStat->Cycles > AccumulatedCycles)
		{
			UnaccountedCycles = RenderViewFamilyStat->Cycles - AccumulatedCycles;
		}
		SET_CYCLE_COUNTER(STAT_UnaccountedSceneRenderingTime, UnaccountedCycles, 1);
	}

	const FCycleStat* UpdateStat0 = GStatManager.GetCycleStat(STAT_ParticleUpdateRTTime);
	const FCycleStat* UpdateStat1 = GStatManager.GetCycleStat(STAT_InfluenceWeightsUpdateRTTime);
	const FCycleStat* UpdateStat2 = GStatManager.GetCycleStat(STAT_GPUSkinUpdateRTTime);
	const FCycleStat* UpdateStat3 = GStatManager.GetCycleStat(STAT_RemoveSceneLightTime);
	const FCycleStat* UpdateStat4 = GStatManager.GetCycleStat(STAT_UpdateSceneLightTime);
	const FCycleStat* UpdateStat5 = GStatManager.GetCycleStat(STAT_AddSceneLightTime);
	const FCycleStat* UpdateStat6 = GStatManager.GetCycleStat(STAT_RemoveScenePrimitiveTime);
	const FCycleStat* UpdateStat7 = GStatManager.GetCycleStat(STAT_AddScenePrimitiveRenderThreadTime);
	const FCycleStat* UpdateStat8 = GStatManager.GetCycleStat(STAT_UpdatePrimitiveTransformRenderThreadTime);

	if (UpdateStat0 && UpdateStat1 && UpdateStat2 && UpdateStat3 && UpdateStat4 && UpdateStat5 && UpdateStat6 && UpdateStat7 && UpdateStat8)
	{
		const DWORD TotalUpdateCycles = 
			UpdateStat0->Cycles +
			UpdateStat1->Cycles +
			UpdateStat2->Cycles +
			UpdateStat3->Cycles +
			UpdateStat4->Cycles +
			UpdateStat5->Cycles +
			UpdateStat6->Cycles +
			UpdateStat7->Cycles +
			UpdateStat8->Cycles;
		SET_CYCLE_COUNTER(STAT_TotalRTUpdateTime, TotalUpdateCycles, 1);
	}

#endif
}

void BeginRenderingViewFamily(FCanvas* Canvas,const FSceneViewFamily* ViewFamily)
{
	// Enforce the editor only show flags restrictions.
	check(GIsEditor || !(ViewFamily->ShowFlags & SHOW_EditorOnly_Mask));

	// Flush the canvas first.
	Canvas->Flush();

	if( ViewFamily->Scene )
	{
		// Set the world's "needs full lighting rebuild" flag if the scene has any uncached static lighting interactions.
		FScene* const Scene = ViewFamily->Scene->GetRenderScene();
		UWorld* const World = Scene->GetWorld();
		if(World)
		{
			if (Scene->NumUncachedStaticLightingInteractions && !World->GetWorldInfo()->bForceNoPrecomputedLighting)
			{
				World->GetWorldInfo()->SetMapNeedsLightingFullyRebuilt(TRUE);
			}
			World->GetWorldInfo()->bMapHasMultipleDominantLightsAffectingOnePrimitive = Scene->NumMultipleDominantLightInteractions > 0;
		}

#if GEMINI_TODO
		// We need to pass the scene's hit proxies through to the hit proxy consumer!
		// Otherwise its possible the hit proxy consumer will cache a hit proxy ID it doesn't have a reference for.
		// Note that the effects of not doing this correctly are minor: clicking on a primitive that moved without invalidating the viewport's
		// cached hit proxies won't work.  Is this worth the pain?
#endif

		// Increment FrameNumber before render the scene. Wrapping around is no problem.
		// This is the only spot we change GFrameNumber, other places can only read.
		++GFrameNumber;

		// Construct the scene renderer.  This copies the view family attributes into its own structures.
		FSceneRenderer* SceneRenderer = ::new FSceneRenderer(ViewFamily, Canvas->GetHitProxyConsumer(), Canvas->GetFullTransform());

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FDrawSceneCommand,
			FSceneRenderer*,SceneRenderer,SceneRenderer,
		{
			RenderViewFamily_RenderThread(SceneRenderer);
		});
	}
	else
	{
		// Construct the scene renderer.  This copies the view family attributes into its own structures.
		FSceneRenderer* SceneRenderer = ::new FSceneRenderer(ViewFamily, Canvas->GetHitProxyConsumer(), Canvas->GetFullTransform());

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FDrawSceneCommandPP,
			FSceneRenderer*,SceneRenderer,SceneRenderer,
		{
			SceneRenderer->RenderPostProcessOnly();
			delete SceneRenderer;
		});
	}
}

/** 
* Helper function to determine post process resolve optimization.
* @return TRUE if the pre post process resolve is deferred
*/
UBOOL FSceneRenderer::DeferPrePostProcessResolve()
{
	// Determines whether to resolve scene color once right before post processing, or at the end of each DPG.
	// Not tested on PS3, but shouldn't matter anyway since PS3 does not actually do resolves (see RHICopyToResolveTarget())
#if PS3
	UBOOL bDeferPrePostProcessResolve = FALSE;
#else
	UBOOL bDeferPrePostProcessResolve = TRUE;
#endif

	// Disable the optimization if any post process effects run before SDPG_PostProcess,
	// Since they might need to read from scene color at the end of a DPG.
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		FViewInfo& View = Views(ViewIndex);
		for( INT EffectIdx=0; EffectIdx < View.PostProcessSceneProxies.Num(); EffectIdx++ )
		{
			const FPostProcessSceneProxy* PPEffectProxy = View.PostProcessSceneProxies(EffectIdx);
			if( PPEffectProxy
				&& PPEffectProxy->GetDepthPriorityGroup() != SDPG_PostProcess
				// Post process effects that affect lighting only are expected to not require a scene color resolve (ie by using alpha blending)
				&& !PPEffectProxy->GetAffectsLightingOnly())
			{
				bDeferPrePostProcessResolve = FALSE;
				break;
			}
		}
	}

	return bDeferPrePostProcessResolve; 
}

//@todo debug -
UBOOL FActorVisibilitySet::DebugVerifyHash(const AActor* VisibleActor)
{
	// verify that the elements in the set hash are valid
	if( !VisibleActors.VerifyHashElementsKey(VisibleActor) )
	{
		// if failed then dumps info about the hash and 
		warnf(TEXT("FActorVisibilitySet::DebugVerifyHash failed on actor = %s"),
			*VisibleActor->GetPathName());
		
		VisibleActors.DumpHashElements(*GWarn);
		GWarn->Flush();

		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

/**
 * Renders radial blur using multiple samples of the scene color buffer
 *
 * @param DPGIndex - current depth priority group index
 * @param bSceneColorIsDirty - TRUE if the current scene color needs a resolve
 * @return TRUE if anything was rendered
 */
UBOOL FSceneRenderer::RenderRadialBlur(UINT DPGIndex, UBOOL bSceneColorIsDirty)
{
	UBOOL bDirty = FALSE;
	UBOOL bRender = FALSE;

	if (Scene != NULL)
	{
		for( TMap<const URadialBlurComponent*, FRadialBlurSceneProxy*>::TConstIterator BlurIt(Scene->RadialBlurInfos); BlurIt; ++BlurIt)
		{
			const FRadialBlurSceneProxy* RadialBlur = BlurIt.Value();
			for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
			{
				// set viewport and view parameters to match view size
				FViewInfo& View = Views(ViewIndex);	
				// skip radial blur proxies that should be rendered using velocities and the existing motion blur pass
				if (!RadialBlur->bRenderAsVelocity &&
					RadialBlur->IsRenderable(&View,DPGIndex,0))
				{
					bRender = TRUE;
				}
			}
		}
	}
	if( bRender )
	{
		SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("RadialBlur"));

		// render each radial blur component proxy in the scene
		for( TMap<const URadialBlurComponent*, FRadialBlurSceneProxy*>::TIterator BlurIt(Scene->RadialBlurInfos); BlurIt; ++BlurIt)
		{
			// resolve the scene color buffer if necessary so it can be sampled
			if (bSceneColorIsDirty)
			{
				GSceneRenderTargets.FinishRenderingSceneColor( TRUE, FResolveRect(0, 0, FamilySizeX, FamilySizeY) );
				bSceneColorIsDirty = FALSE;
			}
			// then continue rendering to it
			GSceneRenderTargets.BeginRenderingSceneColor();

			FRadialBlurSceneProxy* RadialBlur = BlurIt.Value();
			if (!RadialBlur->bRenderAsVelocity)
			{
				for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
				{
					// set viewport and view parameters to match view size
					FViewInfo& View = Views(ViewIndex);			
					RHISetViewport(View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
					RHISetViewParameters(View);
					const UBOOL bRendered = RadialBlur->Draw(&View,DPGIndex,0);
					bDirty |= bRendered;
					bSceneColorIsDirty |= bRendered;
				}				
			}
		}
	}

	return bDirty;
}


/** Helper function to capture scene renderers and properly protect the post process proxies that are held by the game thread and should not be deleted */
FSceneRenderer* CreateSceneCaptureRenderer(FSceneView* InView, FSceneViewFamily* InViewFamily, TArray<class FPostProcessSceneProxy*>& InPostProcessSceneProxies, FHitProxyConsumer* HitProxyConsumer,const FMatrix& InCanvasTransform,UBOOL bInIsSceneCapture)
{
	//append the post process proxies onto each view so they are copied in the SceneRenderer
	if (InView)
	{
		InView->PostProcessSceneProxies.Append(InPostProcessSceneProxies);
	}

	//create the scene renderer
	FSceneRenderer* CaptureSceneRenderer = ::new FSceneRenderer(
		InViewFamily,
		HitProxyConsumer,
		InCanvasTransform,
		bInIsSceneCapture
		);

	//Remove the post process proxies from the source data so it is not deleted
	if (InView)
	{
		InView->PostProcessSceneProxies.Empty();
	}

	return CaptureSceneRenderer;
}

/** Helper function to delete the capture scene renderer and properly protect the post process proxy */
void DeleteSceneCaptureRenderer(FSceneRenderer* InSceneRenderer)
{
	//Remove the post process proxies from the source data so it is not deleted
	for (int ViewIndex = 0; ViewIndex < InSceneRenderer->Views.Num(); ++ViewIndex)
	{
		FViewInfo& TempViewInfo = InSceneRenderer->Views(ViewIndex);
		//NOTE - the proxies are stored in 
		TempViewInfo.PostProcessSceneProxies.Empty();
		TempViewInfo.FSceneView::PostProcessSceneProxies.Empty();

	}

	check (InSceneRenderer);
	delete InSceneRenderer;
}

UBOOL FViewInfo::UseFullMotionBlur() const
{
	extern INT GMotionBlurFullMotionBlur;

	return GMotionBlurFullMotionBlur < 0 ? MotionBlurParams.bFullMotionBlur : (GMotionBlurFullMotionBlur > 0);
}

UBOOL FViewInfo::RequiresMotionBlurButHasNoUberPostProcess(const FViewInfo &View) const
{
	UBOOL bRequiresUberpostprocess = FALSE;
	UBOOL bIncludesUberpostprocess = FALSE;

	if(PostProcessChain)
	{
		for( INT EffectIdx=0; EffectIdx < PostProcessChain->Effects.Num(); EffectIdx++ )
		{
			UPostProcessEffect* Effect = PostProcessChain->Effects(EffectIdx);

			// Seems the array can have 0 pointers so we check for it
			if(Effect)
			{
				bRequiresUberpostprocess = bRequiresUberpostprocess || Effect->RequiresUberpostprocess();
				bIncludesUberpostprocess = bIncludesUberpostprocess || Effect->IncludesUberpostprocess();
			}
		}
	}

	return bRequiresUberpostprocess && !bIncludesUberpostprocess;
}

/*-----------------------------------------------------------------------------
	Stat declarations.
-----------------------------------------------------------------------------*/

// Cycle stats are rendered in reverse order from what they are declared in.
// They are organized so that stats at the top of the screen are earlier in the frame, 
// And stats that are indented are lower in the call hierarchy.

// The purpose of the SceneRendering stat group is to show where rendering thread time is going from a high level.
// It should only contain stats that are likely to track a lot of time in a typical scene, not edge case stats that are rarely non-zero.
DECLARE_STATS_GROUP(TEXT("SceneRendering"),STATGROUP_SceneRendering);

// Amount of time measured by 'RenderViewFamily' that is not accounted for in its children stats
// Use a more detailed profiler (like an instruction trace or sampling capture on Xbox 360) to track down where this time is going if needed
// Update RenderViewFamily_RenderThread if more stats are added under STATGROUP_SceneRendering so that STAT_UnaccountedSceneRenderingTime stays up to date
DECLARE_CYCLE_STAT(TEXT("   Unaccounted"),STAT_UnaccountedSceneRenderingTime,STATGROUP_SceneRendering);
// Scene capture specific stats, since scene capture time is excluded from all the other stats (like InitViews)
DECLARE_CYCLE_STAT(TEXT("      SC Base pass"),STAT_SceneCaptureBasePassTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("      SC InitViews"),STAT_SceneCaptureInitViewsTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("   Scene capture rendering"),STAT_SceneCaptureRenderingTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("   Post Process rendering"),STAT_PostProcessDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("   Velocity drawing"),STAT_VelocityDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("   Translucency drawing"),STAT_TranslucencyDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("   BeginOcclusionTests"),STAT_BeginOcclusionTestsTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("   Unlit Decal drawing"),STAT_UnlitDecalDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("   Lighting drawing"),STAT_LightingDrawTime,STATGROUP_SceneRendering);
// Use 'stat shadowrendering' to get more detail
DECLARE_CYCLE_STAT(TEXT("   Proj Shadow drawing"),STAT_ProjectedShadowDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("      SoftMasked drawing"),STAT_SoftMaskedDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("      Dynamic Primitive drawing"),STAT_DynamicPrimitiveDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("      StaticDrawList drawing"),STAT_StaticDrawListDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("   Base pass drawing"),STAT_BasePassDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("   Depth drawing"),STAT_DepthDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("      Dynamic shadow setup"),STAT_DynamicShadowSetupTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("      Occlusion Result"),STAT_OcclusionResultTime,STATGROUP_SceneRendering);
// Use 'stat initviews' to get more detail
DECLARE_CYCLE_STAT(TEXT("   InitViews"),STAT_InitViewsTime,STATGROUP_SceneRendering);
// Measures the time spent in RenderViewFamily_RenderThread
// Note that this is not the total rendering thread time, any other rendering commands will not be counted here
DECLARE_CYCLE_STAT(TEXT("RenderViewFamily"),STAT_TotalSceneRenderingTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Total GPU rendering"),STAT_TotalGPUFrameTime,STATGROUP_SceneRendering);

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Lights"),STAT_SceneLights,STATGROUP_SceneRendering);

// The InitViews stats group contains information on how long visibility culling took and how effective it was
DECLARE_STATS_GROUP(TEXT("InitViews"),STATGROUP_InitViews);
DECLARE_CYCLE_STAT(TEXT("   GatherShadowPrimitives"),STAT_GatherShadowPrimitivesTime,STATGROUP_InitViews);
DECLARE_CYCLE_STAT(TEXT("   Init dynamic shadows"),STAT_InitDynamicShadowsTime,STATGROUP_InitViews);
DECLARE_CYCLE_STAT(TEXT("      ProcessVisible"),STAT_ProcessVisibleTime,STATGROUP_InitViews);
DECLARE_CYCLE_STAT(TEXT("      Decompress Occlusion"),STAT_DecompressPrecomputedOcclusion,STATGROUP_InitViews);
DECLARE_CYCLE_STAT(TEXT("      Occlusion Result"),STAT_OcclusionResultTime2,STATGROUP_InitViews);
DECLARE_CYCLE_STAT(TEXT("   PerformViewFrustumCulling"),STAT_PerformViewFrustumCullingTime,STATGROUP_InitViews);
DECLARE_CYCLE_STAT(TEXT("InitViews"),STAT_InitViewsTime2,STATGROUP_InitViews);

DECLARE_DWORD_COUNTER_STAT(TEXT("Frustum Culled primitives"),STAT_CulledPrimitives,STATGROUP_InitViews);
DECLARE_DWORD_COUNTER_STAT(TEXT("LOD Dropped primitives"),STAT_LODDroppedPrimitives,STATGROUP_InitViews);
DECLARE_DWORD_COUNTER_STAT(TEXT("MinDrawDistance dropped primitives"),STAT_MinDrawDroppedPrimitives,STATGROUP_InitViews);
DECLARE_DWORD_COUNTER_STAT(TEXT("MaxDrawDistance dropped primitives"),STAT_MaxDrawDroppedPrimitives,STATGROUP_InitViews);
DECLARE_DWORD_COUNTER_STAT(TEXT("Statically occluded primitives"),STAT_StaticallyOccludedPrimitives,STATGROUP_InitViews);
DECLARE_DWORD_COUNTER_STAT(TEXT("Occluded primitives"),STAT_OccludedPrimitives,STATGROUP_InitViews);
DECLARE_DWORD_COUNTER_STAT(TEXT("Occlusion queries"),STAT_OcclusionQueries,STATGROUP_InitViews);
DECLARE_DWORD_COUNTER_STAT(TEXT("Visible static mesh elements"),STAT_VisibleStaticMeshElements,STATGROUP_InitViews);
DECLARE_DWORD_COUNTER_STAT(TEXT("Visible dynamic primitives"),STAT_VisibleDynamicPrimitives,STATGROUP_InitViews);

// The ShadowRendering stats group shows what kind of shadows are taking a lot of rendering thread time to render
// Shadow setup is tracked in the InitViews group
DECLARE_STATS_GROUP(TEXT("ShadowRendering"),STATGROUP_ShadowRendering);
DECLARE_CYCLE_STAT(TEXT("   PerObject Shadow Projections"),STAT_RenderPerObjectShadowProjectionsTime,STATGROUP_ShadowRendering);
DECLARE_CYCLE_STAT(TEXT("   PerObject Shadow Depths"),STAT_RenderPerObjectShadowDepthsTime,STATGROUP_ShadowRendering);
DECLARE_CYCLE_STAT(TEXT("   WholeScene Shadow Projections"),STAT_RenderWholeSceneShadowProjectionsTime,STATGROUP_ShadowRendering);
DECLARE_CYCLE_STAT(TEXT("   WholeScene Shadow Depths"),STAT_RenderWholeSceneShadowDepthsTime,STATGROUP_ShadowRendering);
DECLARE_CYCLE_STAT(TEXT("   Modulated Shadows"),STAT_RenderModulatedShadowsTime,STATGROUP_ShadowRendering);
DECLARE_CYCLE_STAT(TEXT("   Normal Shadows"),STAT_RenderNormalShadowsTime,STATGROUP_ShadowRendering);
DECLARE_CYCLE_STAT(TEXT("Render Dynamic Shadows"),STAT_ShadowRendering,STATGROUP_ShadowRendering);

DECLARE_DWORD_COUNTER_STAT(TEXT("Per Object shadows"),STAT_PerObjectShadows,STATGROUP_ShadowRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Cached PreShadows"),STAT_CachedPreShadows,STATGROUP_ShadowRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Transclucent PreShadows"),STAT_TranslucentPreShadows,STATGROUP_ShadowRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("PreShadows"),STAT_PreShadows,STATGROUP_ShadowRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Whole Scene shadows"),STAT_WholeSceneShadows,STATGROUP_ShadowRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Shadowing dominant lights on screen"),STAT_ShadowCastingDominantLights,STATGROUP_ShadowRendering);
