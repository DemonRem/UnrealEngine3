/*=============================================================================
	SceneRendering.cpp: Scene rendering.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/*-----------------------------------------------------------------------------
	Globals
-----------------------------------------------------------------------------*/

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

/*-----------------------------------------------------------------------------
	FViewInfo
-----------------------------------------------------------------------------*/

/** 
 * Initialization constructor. Passes all parameters to FSceneView constructor
 */
FViewInfo::FViewInfo(
	const FSceneViewFamily* InFamily,
	FSceneViewStateInterface* InState,
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
	const TArray<FPrimitiveSceneInfo*>& InHiddenPrimitives,
	FLOAT InLODDistanceFactor
	)
	:	FSceneView(
			InFamily,
			InState,
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
	,	bRequiresVelocities( FALSE )
	,	bIgnoreOcclusionQueries( FALSE )
	,	NumVisibleStaticMeshElements(0)
	,	NumVisibleDynamicPrimitives(0)
	,	IndividualOcclusionQueries((FSceneViewState*)InState,1)
	,	GroupedOcclusionQueries((FSceneViewState*)InState,FOcclusionQueryBatcher::OccludedPrimitiveQueryBatchSize)
{
	PrevViewProjMatrix.SetIdentity();

	// Clear fog constants.
	for(INT LayerIndex = 0; LayerIndex < ARRAY_COUNT(FogDistanceScale); LayerIndex++)
	{
		FogMinHeight[LayerIndex] = FogMaxHeight[LayerIndex] = FogDistanceScale[LayerIndex] = 0.0f;
		FogStartDistance[LayerIndex] = 0.f;
		FogInScattering[LayerIndex] = FLinearColor::Black;
		FogExtinctionDistance[LayerIndex] = FLT_MAX;
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
	,	bRequiresVelocities( FALSE )
	,	NumVisibleStaticMeshElements(0)
	,	NumVisibleDynamicPrimitives(0)
	,	IndividualOcclusionQueries((FSceneViewState*)InView->State,1)
	,	GroupedOcclusionQueries((FSceneViewState*)InView->State,FOcclusionQueryBatcher::OccludedPrimitiveQueryBatchSize)
{
	PrevViewProjMatrix.SetIdentity();

	// Clear fog constants.
	for(INT LayerIndex = 0; LayerIndex < ARRAY_COUNT(FogDistanceScale); LayerIndex++)
	{
		FogMinHeight[LayerIndex] = FogMaxHeight[LayerIndex] = FogDistanceScale[LayerIndex] = 0.0f;
		FogStartDistance[LayerIndex] = 0.f;
		FogInScattering[LayerIndex] = FLinearColor::Black;
		FogExtinctionDistance[LayerIndex] = FLT_MAX;
	}

	if( PostProcessChain )
	{
		// create render proxies for any post process effects in this view
		for( INT EffectIdx=0; EffectIdx < PostProcessChain->Effects.Num(); EffectIdx++ )
		{
			UPostProcessEffect* Effect = PostProcessChain->Effects(EffectIdx);
			// only add a render proxy if the effect is enabled
			if( Effect->IsShown(InView) )
			{
				FPostProcessSceneProxy* PostProcessSceneProxy = Effect->CreateSceneProxy(
					PostProcessSettings && Effect->bUseWorldSettings ? PostProcessSettings : NULL
					);
				if( PostProcessSceneProxy )
				{
					PostProcessSceneProxies.AddRawItem(PostProcessSceneProxy);
					bRequiresVelocities = bRequiresVelocities || PostProcessSceneProxy->RequiresVelocities( MotionBlurParameters );
				}
			}
		}

		// Mark the final post-processing effect so that we can render it directly to the view render target.
		UINT DPGIndex = SDPG_PostProcess;
		INT FinalIdx = -1;
		for( INT ProxyIdx=0; ProxyIdx < PostProcessSceneProxies.Num(); ProxyIdx++ )
		{
			if( PostProcessSceneProxies(ProxyIdx).GetDepthPriorityGroup() == DPGIndex )
			{
				FinalIdx = ProxyIdx;
				PostProcessSceneProxies(FinalIdx).TerminatesPostProcessChain( FALSE );
			}
		}
		if (FinalIdx != -1)
		{
			PostProcessSceneProxies(FinalIdx).TerminatesPostProcessChain( TRUE );
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

FSceneRenderer::FSceneRenderer(const FSceneViewFamily* InViewFamily,FHitProxyConsumer* HitProxyConsumer,const FMatrix& InCanvasTransform)
:	Scene(InViewFamily->Scene ? (FScene*)InViewFamily->Scene->GetRenderScene() : NULL)
,	ViewFamily(*InViewFamily)
,	CanvasTransform(InCanvasTransform)
{
	// Collect dynamic shadow stats if the view family has the appropriate container object.
	if( InViewFamily->DynamicShadowStats )
	{
		bShouldGatherDynamicShadowStats = TRUE;
	}
	else
	{
		bShouldGatherDynamicShadowStats = FALSE;
	}

	// Copy the individual views.
	Views.Empty(InViewFamily->Views.Num());
	for(INT ViewIndex = 0;ViewIndex < InViewFamily->Views.Num();ViewIndex++)
	{
		// Construct a FViewInfo with the FSceneView properties.
		FViewInfo* ViewInfo = new(Views) FViewInfo(InViewFamily->Views(ViewIndex));
		ViewFamily.Views(ViewIndex) = ViewInfo;
		ViewInfo->Family = &ViewFamily;

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

	// Allocate the render target space to 
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

#if !FINAL_RELEASE
	// copy off the requests
	// (I apologize for the const_cast, but didn't seem worth refactoring just for the freezerendering command)
	bHasRequestedToggleFreeze = const_cast<FRenderTarget*>(InViewFamily->RenderTarget)->HasToggleFreezeCommand();
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
#if STATS
	// Stringify stats.
	if( bShouldGatherDynamicShadowStats )
	{
		check(ViewFamily.DynamicShadowStats);

		// Move from TMap to TArray and sort shadow stats.
		TArray<FSortedShadowStats> SortedShadowStats;
		for( TMap<FLightPrimitiveInteraction*,FCombinedShadowStats>::TIterator It(InteractionToDynamicShadowStatsMap); It; ++It )
		{
			FSortedShadowStats SortedStat;
			SortedStat.Interaction	= It.Key();
			SortedStat.ShadowStat	= It.Value();
			SortedShadowStats.AddItem( SortedStat );
		}

		// Only sort if there is something to sort.
		if( SortedShadowStats.Num() )
		{
			Sort<USE_COMPARE_CONSTREF(FSortedShadowStats,SceneRendering)>( SortedShadowStats.GetTypedData(), SortedShadowStats.Num() );
		}

		const ULightComponent* PreviousLightComponent = NULL;

		// Iterate over sorted list and stringify entries.
		for( INT StatIndex=0; StatIndex<SortedShadowStats.Num(); StatIndex++ )
		{
			const FSortedShadowStats&	SortedStat		= SortedShadowStats(StatIndex);
			FLightPrimitiveInteraction* Interaction		= SortedStat.Interaction;
			const FCombinedShadowStats&	ShadowStat		= SortedStat.ShadowStat;
			const ULightComponent*		LightComponent	= Interaction->GetLight()->LightComponent;
			
			// Only emit light row if the light has changed.
			if( PreviousLightComponent != LightComponent )
			{
				PreviousLightComponent = LightComponent;
				FDynamicShadowStatsRow StatsRow;
				
				// Figure out light name and add it.
				FString	LightName;
				if( LightComponent->GetOwner() )
				{
					LightName = LightComponent->GetOwner()->GetName();
				}
				else
				{
					LightName = FString(TEXT("Ownerless ")) + LightComponent->GetClass()->GetName();
				}
				new(StatsRow.Columns) FString(*LightName);
				
				// Add radius information for pointlight derived classes.
				FString Radius;
				if( LightComponent->IsA(UPointLightComponent::StaticClass()) )
				{
					Radius = FString::Printf(TEXT("Radius: %i"),appTrunc(((UPointLightComponent*)LightComponent)->Radius));
				}
				new(StatsRow.Columns) FString(*Radius);

				// Empty column.
				new(StatsRow.Columns) FString(TEXT(""));

				// Empty column.
				new(StatsRow.Columns) FString(TEXT(""));

				// Class name.
				new(StatsRow.Columns) FString(LightComponent->GetClass()->GetName());

				// Fully qualified name.
				new(StatsRow.Columns) FString(*LightComponent->GetPathName().Replace(PLAYWORLD_PACKAGE_PREFIX,TEXT("")));

				// Add row.
				INT Index = ViewFamily.DynamicShadowStats->AddZeroed();
				(*ViewFamily.DynamicShadowStats)(Index) = StatsRow;
			}

			// Subjects
			for( INT SubjectIndex=0; SubjectIndex<ShadowStat.SubjectPrimitives.Num(); SubjectIndex++ )
			{
				const FPrimitiveSceneInfo*	PrimitiveSceneInfo	= ShadowStat.SubjectPrimitives(SubjectIndex);
				UPrimitiveComponent*		PrimitiveComponent	= PrimitiveSceneInfo->Component;
				FDynamicShadowStatsRow		StatsRow;

				// Empty column.
				new(StatsRow.Columns) FString(TEXT(""));

				// Figure out actor name and add it.
				FString	PrimitiveName;
				if( PrimitiveComponent->GetOwner() )
				{
					PrimitiveName = PrimitiveComponent->GetOwner()->GetName();
				}
				else if( PrimitiveComponent->IsA(UModelComponent::StaticClass()) )
				{
					PrimitiveName = FString(TEXT("BSP"));
				}
				else
				{
					PrimitiveName = FString(TEXT("Ownerless"));
				}
				new(StatsRow.Columns) FString(*PrimitiveName);

				// Shadow type.
				FString ShadowType;
				if( ShadowStat.ShadowResolution != INDEX_NONE )
				{
					ShadowType = FString::Printf(TEXT("Projected (Res: %i)"), ShadowStat.ShadowResolution);
				}
				else
				{
					ShadowType = TEXT("Volume");
				}
				new(StatsRow.Columns) FString(*ShadowType);

				// Shadow pass number.
				FString ShadowPassNumber = TEXT("");
				if( ShadowStat.ShadowResolution != INDEX_NONE )
				{
					if( ShadowStat.ShadowPassNumber != INDEX_NONE )
					{
						ShadowPassNumber = FString::Printf(TEXT("%i"),ShadowStat.ShadowPassNumber);
					}
					else
					{
						ShadowPassNumber = TEXT("N/A");
					}
				}
				new(StatsRow.Columns) FString(*ShadowPassNumber);

				// Class name.
				new(StatsRow.Columns) FString(PrimitiveComponent->GetClass()->GetName());

				// Fully qualified name.
				new(StatsRow.Columns) FString(*PrimitiveComponent->GetPathName().Replace(PLAYWORLD_PACKAGE_PREFIX,TEXT("")));

				// Add row.
				INT Index = ViewFamily.DynamicShadowStats->AddZeroed();
				(*ViewFamily.DynamicShadowStats)(Index) = StatsRow;
			}

			// PreShadow
			for( INT PreShadowIndex=0; PreShadowIndex<ShadowStat.PreShadowPrimitives.Num(); PreShadowIndex++ )
			{
				const FPrimitiveSceneInfo*	PrimitiveSceneInfo	= ShadowStat.PreShadowPrimitives(PreShadowIndex);
				UPrimitiveComponent*		PrimitiveComponent	= PrimitiveSceneInfo->Component;
				FDynamicShadowStatsRow		StatsRow;

				// Empty column.
				new(StatsRow.Columns) FString(TEXT(""));

				// Empty column.
				new(StatsRow.Columns) FString(TEXT(""));

				// Figure out actor name and add it.
				FString	PrimitiveName;
				if( PrimitiveComponent->GetOwner() )
				{
					PrimitiveName = PrimitiveComponent->GetOwner()->GetName();
				}
				else if( PrimitiveComponent->IsA(UModelComponent::StaticClass()) )
				{
					PrimitiveName = FString(TEXT("BSP"));
				}
				else
				{
					PrimitiveName = FString(TEXT("Ownerless"));
				}
				new(StatsRow.Columns) FString(*PrimitiveName);

				// Empty column.
				new(StatsRow.Columns) FString(TEXT(""));

				// Class name.
				new(StatsRow.Columns) FString(PrimitiveComponent->GetClass()->GetName());

				// Fully qualified name.
				new(StatsRow.Columns) FString(*PrimitiveComponent->GetPathName().Replace(PLAYWORLD_PACKAGE_PREFIX,TEXT("")));

				// Add row.
				INT Index = ViewFamily.DynamicShadowStats->AddZeroed();
				(*ViewFamily.DynamicShadowStats)(Index) = StatsRow;
			}
		}
	}
#endif
}


/**
 * Initialize scene's views.
 * Check visibility, sort translucent items, etc.
 */
void FSceneRenderer::InitViews()
{
	SCOPE_CYCLE_COUNTER(STAT_InitViewsTime);

	// Setup motion blur parameters.
	PreviousFrameTime = ViewFamily.CurrentRealTime;
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		FViewInfo& View = Views(ViewIndex);
		FSceneViewState* ViewState = (FSceneViewState*) View.State;
		static UBOOL bEnableTimeScale = TRUE;

		// We can't use LatentOcclusionQueries when doing TiledScreenshot because in that case
		// 1-frame lag = 1-tile lag = clipped geometry, so we turn off occlusion queries
		// Occlusion culling is also disabled for hit proxy rendering.
		extern UBOOL GIsTiledScreenshot;
		const UBOOL bIsHitTesting = (ViewFamily.ShowFlags & SHOW_HitProxies) != 0;
		View.bIgnoreOcclusionQueries = GIsTiledScreenshot || GIgnoreAllOcclusionQueries || bIsHitTesting;

		if ( ViewState )
		{
			if(View.bRequiresVelocities)
			{
				FLOAT DeltaTime = View.Family->CurrentRealTime - ViewState->LastRenderTime;
				if ( DeltaTime < -0.0001f || ViewState->LastRenderTime < 0.0001f )
				{
					// Time was reset or initialization.
					ViewState->PrevProjMatrix				= View.ProjectionMatrix;
					ViewState->PendingPrevProjMatrix		= View.ProjectionMatrix;
					ViewState->PrevViewMatrix				= View.ViewMatrix;
					ViewState->PrevViewOrigin				= View.ViewOrigin;
					ViewState->PendingPrevViewMatrix		= View.ViewMatrix;
					ViewState->MotionBlurTimeScale			= 1.0f;
				}
				else if ( DeltaTime > 0.0001f )
				{
					// New frame detected.
					ViewState->PrevViewMatrix				= ViewState->PendingPrevViewMatrix;
					ViewState->PrevViewOrigin				= ViewState->PendingPrevViewOrigin;
					ViewState->PrevProjMatrix				= ViewState->PendingPrevProjMatrix;
					ViewState->PendingPrevProjMatrix		= View.ProjectionMatrix;
					ViewState->PendingPrevViewMatrix		= View.ViewMatrix;
					ViewState->PendingPrevViewOrigin		= View.ViewOrigin;
					ViewState->MotionBlurTimeScale			= bEnableTimeScale ? (1.0f / (DeltaTime * 30.0f)) : 1.0f;
				}

				/** Disable motion blur if the camera has changed too much this frame. */
				FLOAT RotationThreshold = appCos( View.MotionBlurParameters.RotationThreshold*PI/180.0f );
				FLOAT AngleAxis0 = View.ViewMatrix.GetAxis(0) | ViewState->PrevViewMatrix.GetAxis(0);
				FLOAT AngleAxis1 = View.ViewMatrix.GetAxis(1) | ViewState->PrevViewMatrix.GetAxis(1);
				FLOAT AngleAxis2 = View.ViewMatrix.GetAxis(2) | ViewState->PrevViewMatrix.GetAxis(2);
				FVector Distance = FVector(View.ViewOrigin) - ViewState->PrevViewOrigin;
				if ( AngleAxis0 < RotationThreshold ||
					AngleAxis1 < RotationThreshold ||
					AngleAxis2 < RotationThreshold ||
					Distance.Size() > View.MotionBlurParameters.TranslationThreshold )
				{
					ViewState->PrevProjMatrix				= View.ProjectionMatrix;
					ViewState->PendingPrevProjMatrix		= View.ProjectionMatrix;
					ViewState->PrevViewMatrix				= View.ViewMatrix;
					ViewState->PendingPrevViewMatrix		= View.ViewMatrix;
					ViewState->PrevViewOrigin				= View.ViewOrigin;
					ViewState->PendingPrevViewOrigin		= View.ViewOrigin;
					ViewState->MotionBlurTimeScale			= 1.0f;

					// PT: If the motion blur shader is the last shader in the post-processing chain then it is the one that is
					//     adjusting for the viewport offset.  So it is always required and we can't just disable the work the
					//     shader does.  The correct fix would be to disable the effect when we don't need it and to properly mark
					//     the uber-postprocessing effect as the last effect in the chain.

					//View.bRequiresVelocities				= FALSE;

					// Issue: This will only get set if View.bRequiresVelocities was true before (i.e. when the motion blur effect is enabled).  That
					//        means the fix for items popping in on camera cuts will only work as long as the motion-blur effect is enabled.

					View.bIgnoreOcclusionQueries = TRUE;
				}

				View.PrevViewProjMatrix = ViewState->PrevViewMatrix * ViewState->PrevProjMatrix;
			}
			
			PreviousFrameTime = ViewState->LastRenderTime;
			ViewState->LastRenderTime = View.Family->CurrentRealTime;
		}

		/** Each view starts out rendering to the HDR scene color */
		View.bUseLDRSceneColor = FALSE;
	}

	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		FViewInfo& View = Views(ViewIndex);
		FSceneViewState* const ViewState = (FSceneViewState*)View.State;

		if(ViewState)
		{
			// Reset the view's occlusion query pool.
			ViewState->OcclusionQueryPool.Reset();
		}

		// Initialize the view elements' dynamic resources.
		View.InitDynamicResources();

		// Allocate the view's visibility maps.
		View.PrimitiveVisibilityMap = FBitArray(TRUE,Scene->Primitives.GetMaxIndex());
		View.StaticMeshVisibilityMap = FBitArray(FALSE,Scene->StaticMeshes.GetMaxIndex());
		View.StaticMeshOccluderMap = FBitArray(FALSE,Scene->StaticMeshes.GetMaxIndex());
		View.StaticMeshVelocityMap = FBitArray(FALSE,Scene->StaticMeshes.GetMaxIndex());

		View.VisibleLightInfos.Empty(Scene->Lights.GetMaxIndex());
		for(INT LightIndex = 0;LightIndex < Scene->Lights.GetMaxIndex();LightIndex++)
		{
			new(View.VisibleLightInfos) FVisibleLightInfo();
		}

		View.PrimitiveViewRelevanceMap.Empty(Scene->Primitives.GetMaxIndex());
		View.PrimitiveViewRelevanceMap.AddZeroed(Scene->Primitives.GetMaxIndex());

		// Mark the primitives in the view's HiddenPrimitives array as not visible.
		for(INT PrimitiveIndex = 0;PrimitiveIndex < View.HiddenPrimitives.Num();PrimitiveIndex++)
		{
			View.PrimitiveVisibilityMap(View.HiddenPrimitives(PrimitiveIndex)->Id) = FALSE;
		}
	}

	// Cull the scene's primitives against the view frustum.
	INT NumOccludedPrimitives = 0;
	INT NumCulledPrimitives = 0;
	for(TSparseArray<FPrimitiveSceneInfoCompact>::TConstIterator PrimitiveIt(Scene->Primitives);PrimitiveIt;++PrimitiveIt)
	{
		const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo = *PrimitiveIt;

		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{		
			FViewInfo& View = Views(ViewIndex);
			FSceneViewState* ViewState = (FSceneViewState*)View.State;

#if !FINAL_RELEASE
			if( ViewState )
			{
#if !CONSOLE
				// For visibility child views, check if the primitive was visible in the parent view.
				const FSceneViewState* const ViewParent = (FSceneViewState*)ViewState->GetViewParent();
				if(ViewParent && !ViewParent->ParentPrimitives.Contains(CompactPrimitiveSceneInfo.Component) )
				{
					continue;
				}
#endif

				// For views with frozen visibility, check if the primitive is in the frozen visibility set.
				if(ViewState->bIsFrozen && !ViewState->FrozenPrimitives.Contains(CompactPrimitiveSceneInfo.Component) )
				{
					continue;
				}
			}
#endif

			// Distance to camera in perspective viewports.
			FLOAT DistanceSquared = 0.0f;

			// Cull primitives based on distance to camera in perspective viewports and calculate distance, also used later for static
			// mesh elements.
			if( View.ViewOrigin.W > 0.0f )
			{
				// Compute the distance between the view and the primitive.
				DistanceSquared = (CompactPrimitiveSceneInfo.Bounds.Origin - View.ViewOrigin).SizeSquared();
				// Cull the primitive if the view is farther than its cull distance.
				if( DistanceSquared * Square(View.LODDistanceFactor) > Square(CompactPrimitiveSceneInfo.CullDistance) )
				{
					NumCulledPrimitives++;
					continue;
				}
			}

			if(!View.PrimitiveVisibilityMap.AccessCorrespondingBit(PrimitiveIt))
			{
				// Skip primitives which were marked as not visible because they were in the view's HiddenPrimitives array.
				continue;
			}

			// Check the primitive's bounding box against the view frustum.
			UBOOL bPrimitiveIsVisible = FALSE;
			if(View.ViewFrustum.IntersectSphere(CompactPrimitiveSceneInfo.Bounds.Origin,CompactPrimitiveSceneInfo.Bounds.SphereRadius))
			{
				// Prefetch the full primitive scene info.
				PREFETCH(CompactPrimitiveSceneInfo.PrimitiveSceneInfo);
				PREFETCH(CompactPrimitiveSceneInfo.Proxy);

				// Check whether the primitive is occluded this frame.
				const UBOOL bIsOccluded =
					!(ViewFamily.ShowFlags & SHOW_Wireframe) &&
					ViewState &&
					ViewState->UpdatePrimitiveOcclusion(CompactPrimitiveSceneInfo,View,ViewFamily.CurrentRealTime);
				if(!bIsOccluded)
				{
					// Compute the primitive's view relevance.
					FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap(PrimitiveIt.GetIndex());
					ViewRelevance = CompactPrimitiveSceneInfo.Proxy->GetViewRelevance(&View);

					if(ViewRelevance.bStaticRelevance)
					{
						// Mark the primitive's static meshes as visible.
						for(INT MeshIndex = 0;MeshIndex < CompactPrimitiveSceneInfo.PrimitiveSceneInfo->StaticMeshes.Num();MeshIndex++)
						{
							const FStaticMesh& StaticMesh = CompactPrimitiveSceneInfo.PrimitiveSceneInfo->StaticMeshes(MeshIndex);
							if(DistanceSquared >= StaticMesh.MinDrawDistanceSquared && DistanceSquared < StaticMesh.MaxDrawDistanceSquared)
							{
								// Mark static mesh as visible for rendering
								View.StaticMeshVisibilityMap(StaticMesh.Id) = TRUE;
								View.NumVisibleStaticMeshElements++;
							}
						}
					}

					if(ViewRelevance.bDynamicRelevance)
					{
						// Keep track of visible dynamic primitives.
						View.VisibleDynamicPrimitives.AddItem(CompactPrimitiveSceneInfo.PrimitiveSceneInfo);
						// process view for this primitive proxy
						CompactPrimitiveSceneInfo.Proxy->PreRenderView(&View, GetGlobalSceneRenderState()->FrameNumber);

						View.NumVisibleDynamicPrimitives++;
					}

					for (UINT CheckDPGIndex = 0; CheckDPGIndex < SDPG_MAX_SceneRender; CheckDPGIndex++)
					{
						if (ViewRelevance.GetDPG(CheckDPGIndex) == TRUE)
						{
							if ( ViewRelevance.bDecalRelevance )
							{
								// Add to the set of decal primitives.
										View.DecalPrimSet[CheckDPGIndex].AddScenePrimitive(CompactPrimitiveSceneInfo.PrimitiveSceneInfo);
							}

							if( ViewRelevance.bTranslucentRelevance	)
							{
								// Add to set of dynamic translucent primitives
										View.TranslucentPrimSet[CheckDPGIndex].AddScenePrimitive(CompactPrimitiveSceneInfo.PrimitiveSceneInfo,View,ViewRelevance.bUsesSceneColor);

								if( ViewRelevance.bDistortionRelevance )
								{
									// Add to set of dynamic distortion primitives
									View.DistortionPrimSet[CheckDPGIndex].AddScenePrimitive(CompactPrimitiveSceneInfo.PrimitiveSceneInfo,View);
								}
							}
						}
					}

					if( ViewRelevance.IsRelevant() )
					{
						// This primitive is in the view frustum, view relevant, and unoccluded; it's visible.
						bPrimitiveIsVisible = TRUE;

						// If the primitive's static meshes need to be updated before they can be drawn, update them now.
						CompactPrimitiveSceneInfo.PrimitiveSceneInfo->ConditionalUpdateStaticMeshes();

						// Add to the scene's list of primitive which are visible and that have lit decals.
						const UBOOL bHasLitDecals = CompactPrimitiveSceneInfo.Proxy->HasLitDecals(&View);
						if ( bHasLitDecals )
						{
							View.VisibleLitDecalPrimitives.AddItem( CompactPrimitiveSceneInfo.PrimitiveSceneInfo );
						}

						// Iterate over the lights affecting the primitive.
						for(const FLightPrimitiveInteraction* Interaction = CompactPrimitiveSceneInfo.PrimitiveSceneInfo->LightList;
							Interaction;
							Interaction = Interaction->GetNextLight()
							)
						{
							// The light doesn't need to be rendered if it only affects light-maps or if it is a skylight.
							const UBOOL bRenderLight =
								(ViewRelevance.bForceDirectionalLightsDynamic && Interaction->GetLight()->LightType == LightType_Directional) || 
								(!Interaction->IsLightMapped() && Interaction->GetLight()->LightType != LightType_Sky);

							if ( bRenderLight )
							{
								FVisibleLightInfo& VisibleLightInfo = View.VisibleLightInfos(Interaction->GetLightId());
								for(UINT DPGIndex = 0;DPGIndex < SDPG_MAX_SceneRender;DPGIndex++)
								{
									if(ViewRelevance.GetDPG(DPGIndex))
									{
										// indicate that the light is affecting some static or dynamic lit primitives
										VisibleLightInfo.DPGInfo[DPGIndex].bHasVisibleLitPrimitives = TRUE;

										if( ViewRelevance.bDynamicRelevance )
										{
											// Add dynamic primitives to the light's list of visible dynamic affected primitives.
											VisibleLightInfo.DPGInfo[DPGIndex].VisibleDynamicLitPrimitives.AddItem(CompactPrimitiveSceneInfo.PrimitiveSceneInfo);
										}
										if ( bHasLitDecals )
										{
											// Add to the light's list of The primitives which are visible, affected by this light and receiving lit decals.
											VisibleLightInfo.DPGInfo[DPGIndex].VisibleLitDecalPrimitives.AddItem(CompactPrimitiveSceneInfo.PrimitiveSceneInfo);
											CompactPrimitiveSceneInfo.Proxy->InitLitDecalFlags(DPGIndex);
										}
									}
								}
							}
						}
					}
				}
				else
				{
					NumOccludedPrimitives++;
				}
			}

			// Update the primitive's visibility state.
			View.PrimitiveVisibilityMap.AccessCorrespondingBit(PrimitiveIt) = bPrimitiveIsVisible;
		}
	}
	INC_DWORD_STAT_BY(STAT_OccludedPrimitives,NumOccludedPrimitives);
	INC_DWORD_STAT_BY(STAT_CulledPrimitives,NumCulledPrimitives);

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
	for(TSparseArray<FLightSceneInfo*>::TConstIterator LightIt(Scene->Lights);LightIt;++LightIt)
	{
		FLightSceneInfo* LightSceneInfo = *LightIt;

		// view frustum cull lights in each view
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{		
			FViewInfo& View = Views(ViewIndex);

			// only check if the light has any visible lit primitives 
			// (or uses modulated shadows since they are always rendered)
			if (LightSceneInfo->LightShadowMode == LightShadow_Modulate || 
				View.VisibleLightInfos(LightSceneInfo->Id).HasVisibleLitPrimitives())
			{
				// dir lights are always visible, and point/spot only if in the frustum
				if (LightSceneInfo->LightType == LightType_Point || LightSceneInfo->LightType == LightType_Spot)
				{
					if (View.ViewFrustum.IntersectSphere(LightSceneInfo->GetOrigin(), LightSceneInfo->GetRadius()))
					{
						View.VisibleLightInfos(LightSceneInfo->Id).bInViewFrustum = TRUE;
					}
				}
				else
				{
					View.VisibleLightInfos(LightSceneInfo->Id).bInViewFrustum = TRUE;
				}
			}
		}
	}

	if( (ViewFamily.ShowFlags & SHOW_DynamicShadows) && GSystemSettings.bAllowDynamicShadows )
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
TCHAR* GetSceneDPGName( ESceneDepthPriorityGroup DPG )
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
* Renders the view family. 
*/
void FSceneRenderer::Render()
{
	// Allocate the maximum scene render target space for the current view family.
	GSceneRenderTargets.Allocate( ViewFamily.RenderTarget->GetSizeX(), ViewFamily.RenderTarget->GetSizeY() );

	UBOOL SavedGInvertCullMode = GInvertCullMode;
	if( Views.Num() > 0 && 
		Views(0).ViewMatrix.Determinant() < 0.0f )
	{
		GInvertCullMode = TRUE;
	}	

	// Find the visible primitives.
	InitViews();

	const UBOOL bIsWireframe = (ViewFamily.ShowFlags & SHOW_Wireframe) != 0;

	// Whether to clear the scene color buffer before rendering color for the first DPG. When using tiling, this 
	// gets cleared later automatically.
	UBOOL bRequiresClear = !GUseTilingCode && (ViewFamily.bClearScene || bIsWireframe);

	for(UINT DPGIndex = 0;DPGIndex < SDPG_MAX_SceneRender;DPGIndex++)
	{
		// WARNING: if any Xbox360 rendering uses SDPG_UnrealEdBackground, it will not show up with tiling enabled
		// unless you go out of your way to Resolve the scene color and Restore it at the beginning of the tiling pass.
		// SDPG_World and SDPG_Foreground work just fine, however.
		UBOOL bWorldDpg = (DPGIndex == SDPG_World);

		// Skip Editor-only DPGs for game rendering.
		if( GIsGame && (DPGIndex == SDPG_UnrealEdBackground || DPGIndex == SDPG_UnrealEdForeground) )
		{
			continue;
		}

		SCOPED_DRAW_EVENT(EventDPG)(DEC_SCENE_ITEMS,TEXT("DPG %s"),GetSceneDPGName((ESceneDepthPriorityGroup)DPGIndex));

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

		if( GUseTilingCode && bWorldDpg )
		{
			RHIMSAAInitPrepass();
		}

		// Draw the scene pre-pass
		UBOOL bDirtyPrePass = RenderPrePass(DPGIndex,bIsOcclusionTesting);

		// Render the velocities of movable objects for the motion blur effect.
		if( !GUseTilingCode && bWorldDpg )
		{
			RenderVelocities(DPGIndex);
		}

		if( GUseTilingCode && bWorldDpg )
		{
			RHIMSAABeginRendering();
		}
		else
		{
			// Begin drawing the scene color.
			GSceneRenderTargets.BeginRenderingSceneColor();
		}

		UBOOL bBasePassDirtiedColor = FALSE;

		// Clear scene color buffer if necessary.
		if( bRequiresClear )
		{
			SCOPED_DRAW_EVENT( EventClear )( DEC_SCENE_ITEMS, TEXT( "ClearView" ) );

			// Clear the entire viewport to make sure no post process filters in any data from an invalid region
			RHISetViewport( GlobalContext, 0, 0, 0.0f, ViewFamily.RenderTarget->GetSizeX(), ViewFamily.RenderTarget->GetSizeY(), 1.0f );
			RHIClear( GlobalContext, TRUE, FLinearColor::Black, FALSE, 0, FALSE, 0 );

			// Clear the viewports to their background color
			if( GIsEditor )
			{
				// Clear each viewport to its own background color
				for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
				{
					SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("ClearView%d"),ViewIndex);

					FViewInfo& View = Views(ViewIndex);

					// Set the device viewport for the view.
					RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);

					// Clear the scene color surface when rendering the first DPG.
					RHIClear(GlobalContext,TRUE,View.BackgroundColor,FALSE,0,FALSE,0);
				}

				// Clear the depths to max depth so that depth bias blend materials always show up
				ClearSceneColorDepth();
			}
			// Only clear once.
			bRequiresClear = FALSE;
		}

		// Draw the base pass pass for all visible primitives.
		if ( ViewFamily.ShowFlags & SHOW_TextureDensity )
		{
			bBasePassDirtiedColor |= RenderTextureDensities(DPGIndex);
		}
		else
		{
			// First base pass pass is for the Begin/End tiling block
			bBasePassDirtiedColor |= RenderBasePass(DPGIndex,TRUE);
		}

		if( GUseTilingCode && bWorldDpg )
		{
			RHISetBlendState(GlobalContext, TStaticBlendState<>::GetRHI());
			RHIMSAAEndRendering(GSceneRenderTargets.GetSceneDepthTexture(), GSceneRenderTargets.GetSceneColorTexture());
		}
		else
		{
			// Finish rendering the base pass scene color for this DPG.
			// Always force a resolve for SDPG_World even if nothing was rendered as we may need to restore the scene color during lighting passes
			GSceneRenderTargets.FinishRenderingSceneColor(bBasePassDirtiedColor||bWorldDpg);
		}

		UBOOL bSceneColorDirty = FALSE;

		if( bWorldDpg )
		{
			if( GUseTilingCode )
			{
				// Need to set the render targets before we try resolving, so call in twice
				GSceneRenderTargets.BeginRenderingSceneColor(FALSE);
				RHIMSAARestoreDepth(GSceneRenderTargets.GetSceneDepthTexture());
				// render velocities for motion blur
				RenderVelocities(DPGIndex);
				// restore the scene color from texture
				GSceneRenderTargets.BeginRenderingSceneColor(TRUE);

				// render a second pass for base pass items which should not use MSAA predicated tiling
				// this will include anything which uses dynamic data that needs to allocate space on the command buffer
				// since that could easily cause the static buffer used for tiling to run out of space
				UBOOL bBasePassDirtiedColorUntiled = RenderBasePass(DPGIndex,FALSE);
				bBasePassDirtiedColor |= bBasePassDirtiedColorUntiled;
				bSceneColorDirty |= bBasePassDirtiedColorUntiled;

				if( bBasePassDirtiedColorUntiled )
				{
					// @note sz - this resolve not needed since light rendering will save/restore the scene color buffer 
					// if it needs to use the light attenuation buffer
					
					// resolve scene color if anything was rendered from the 2nd base pass
					//GSceneRenderTargets.FinishRenderingSceneColor(TRUE);
					
					// also resolve depth buffer
					GSceneRenderTargets.ResolveSceneDepthTexture();
				}				
			}
			else
			{
				// Scene depth values resolved to scene depth texture
				// only resolve depth values from world dpg
				GSceneRenderTargets.ResolveSceneDepthTexture();
			}
		}
		
		if(ViewFamily.ShowFlags & SHOW_Lighting)
		{
			// Render the scene lighting.
			bSceneColorDirty |= RenderLights(DPGIndex);

			if( !(ViewFamily.ShowFlags & SHOW_ShaderComplexity) 
			&& ViewFamily.ShowFlags & SHOW_DynamicShadows
			&& GSystemSettings.bAllowDynamicShadows )
			{
				// Render the modulated shadows.
				bSceneColorDirty |= RenderModulatedShadows(DPGIndex);
			}
		}

		// If any opaque elements were drawn, render pre-fog decals for opaque receivers.
		if( (bBasePassDirtiedColor || bSceneColorDirty) && 
			(ViewFamily.ShowFlags & SHOW_Decals) )
		{
			GSceneRenderTargets.BeginRenderingSceneColor(FALSE);
			const UBOOL bDecalsRendered = RenderDecals( DPGIndex, FALSE, TRUE );
			GSceneRenderTargets.FinishRenderingSceneColor(FALSE);

			bSceneColorDirty |= bDecalsRendered;
			if (bDecalsRendered && !CanBlendWithFPRenderTarget(GRHIShaderPlatform))
			{
				//update scene color texture with the last decal rendered
				GSceneRenderTargets.ResolveSceneColor();
				bSceneColorDirty = FALSE;
			}
		}

		if(ViewFamily.ShowFlags & SHOW_Fog)
		{
			// Render the scene fog.
			bSceneColorDirty |= RenderFog(DPGIndex);
		}

		if( ViewFamily.ShowFlags & SHOW_UnlitTranslucency )
		{
			// Distortion pass
			bSceneColorDirty |= RenderDistortion(DPGIndex);
		}

		if(bSceneColorDirty)
		{
			// Save the color buffer if any uncommitted changes have occurred
			GSceneRenderTargets.ResolveSceneColor();
			bSceneColorDirty = FALSE;
		}

		if( ViewFamily.ShowFlags & SHOW_UnlitTranslucency )
		{
			SCOPE_CYCLE_COUNTER(STAT_TranslucencyDrawTime);

			// Translucent pass.
			const UBOOL bTranslucencyDirtiedColor = RenderTranslucency( DPGIndex );

			// If any translucent elements were drawn, render post-fog decals for translucent receivers.
			if( bTranslucencyDirtiedColor )
			{
				if (ViewFamily.ShowFlags & SHOW_Decals)
				{
					RenderDecals( DPGIndex, TRUE, FALSE );	
				}

				if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
				{
					// Finish rendering scene color after rendering translucency for this DPG.
					GSceneRenderTargets.FinishRenderingSceneColor( TRUE );
				}
				else
				{
					// Finish rendering the LDR translucency
					GSceneRenderTargets.FinishRenderingSceneColorLDR(FALSE); 

					//blend the LDR translucency onto scene color
					CombineLDRTranslucency(GlobalContext);
				}
			}
		}

		// post process effects pass for scene DPGs
		RenderPostProcessEffects(DPGIndex);
	}

	// post process effects pass for post process DPGs
	RenderPostProcessEffects(SDPG_PostProcess);

	// Finish rendering for each view.
	{
		SCOPED_DRAW_EVENT(EventFinish)(DEC_SCENE_ITEMS,TEXT("FinishRendering"));
		
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{	
			SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

			FinishRenderViewTarget(&Views(ViewIndex));
		}
	}

#if !FINAL_RELEASE
	// display a message saying we're frozen
	{
		SCOPED_DRAW_EVENT(EventFrozenText)(DEC_SCENE_ITEMS,TEXT("FrozenText"));

		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{	
			SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

			FSceneViewState* ViewState = (FSceneViewState*)Views(ViewIndex).State;
			if (ViewState && (ViewState->HasViewParent()
				|| ViewState->bIsFrozen
				))
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
		INC_DWORD_STAT_BY(STAT_VisibleStaticMeshElements,View.NumVisibleStaticMeshElements);
		INC_DWORD_STAT_BY(STAT_VisibleDynamicPrimitives,View.NumVisibleDynamicPrimitives);

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

	// clear the motion blur information for the scene
	if (ViewFamily.Scene)
	{
		ViewFamily.Scene->ClearMotionBlurInfo();
	}

#if !FINAL_RELEASE
	// clear the commands
	bHasRequestedToggleFreeze = FALSE;
#endif

	GInvertCullMode = SavedGInvertCullMode;
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
			SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);
			FinishRenderViewTarget(&Views(ViewIndex));
		}
	}
}

/** Renders the scene's prepass and occlusion queries */
UBOOL FSceneRenderer::RenderPrePass(UINT DPGIndex,UBOOL bIsOcclusionTesting)
{
	SCOPED_DRAW_EVENT(EventPrePass)(DEC_SCENE_ITEMS,TEXT("PrePass"));

	UBOOL bWorldDpg = (DPGIndex == SDPG_World);
	UBOOL bDirty=0;

	if( !GUseTilingCode || !bWorldDpg )
	{
		GSceneRenderTargets.BeginRenderingPrePass();
	}
 
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

		FViewInfo& View = Views(ViewIndex);
		const FSceneViewState* ViewState = (FSceneViewState*)View.State;

		// Set the device viewport for the view.
		RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
		RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

		if( GUseTilingCode && bWorldDpg )
		{
			RHIMSAAFixViewport();
		}

		//@GEMINI_TODO: the Editor currently relies on the prepass clearing the depth.
		if( GIsEditor || bIsOcclusionTesting || (DPGIndex == SDPG_World) || (DPGIndex == SDPG_Foreground) )
		{
			// Clear the depth buffer as required
			RHIClear(GlobalContext,FALSE,FLinearColor::Black,TRUE,1.0f,TRUE,0);
		}

		// Opaque blending, depth tests and writes.
		RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());
		RHISetDepthState(GlobalContext,TStaticDepthState<TRUE,CF_LessEqual>::GetRHI());

		// Draw a depth pass to avoid overdraw in the other passes.
		if(bIsOcclusionTesting)
		{
			// Write the depths of primitives which were unoccluded the preview frame.
			{
				SCOPE_CYCLE_COUNTER(STAT_DepthDrawTime);

				// Build a map from static mesh ID to whether it should be used as an occluder.
				// This TConstSubsetIterator only iterates over the static-meshes which have their bit set in the static-mesh visibility map.
				for(TSparseArray<FStaticMesh*>::TConstSubsetIterator StaticMeshIt(Scene->StaticMeshes,View.StaticMeshVisibilityMap);
					StaticMeshIt;
					++StaticMeshIt
					)
				{
					const FStaticMesh* StaticMesh = *StaticMeshIt;
					if(StaticMesh->PrimitiveSceneInfo->bUseAsOccluder)
					{
						View.StaticMeshOccluderMap.AccessCorrespondingBit(StaticMeshIt) = TRUE;
					}
				}
				
				// Draw the depth pass for the view.
				bDirty |= Scene->DPGs[DPGIndex].PositionOnlyDepthDrawList.DrawVisible(GlobalContext,&View,View.StaticMeshOccluderMap);
				bDirty |= Scene->DPGs[DPGIndex].DepthDrawList.DrawVisible(GlobalContext,&View,View.StaticMeshOccluderMap);

				// Draw the dynamic occluder primitives using a depth drawing policy.
				TDynamicPrimitiveDrawer<FDepthDrawingPolicyFactory> Drawer(GlobalContext,&View,DPGIndex,FDepthDrawingPolicyFactory::ContextType(),TRUE);
				for(INT PrimitiveIndex = 0;PrimitiveIndex < View.VisibleDynamicPrimitives.Num();PrimitiveIndex++)
				{
					const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives(PrimitiveIndex);
					const FPrimitiveViewRelevance& PrimitiveViewRelevance = View.PrimitiveViewRelevanceMap(PrimitiveSceneInfo->Id);
					const UPrimitiveComponent* Component = PrimitiveSceneInfo->Component;

					const FMatrix* CheckMatrix = NULL;
					const FMotionBlurInfo* MBInfo;
					if (PrimitiveSceneInfo->Scene->GetPrimitiveMotionBlurInfo(PrimitiveSceneInfo, MBInfo) == TRUE)
					{
						CheckMatrix = &(MBInfo->PreviousLocalToWorld);
					}

					// Don't draw dynamic primitives with depth-only if we're going to render their velocities anyway.
					const UBOOL bHasVelocity = View.bRequiresVelocities
						&& !PrimitiveSceneInfo->bStaticShadowing
						&& (Abs(PrimitiveSceneInfo->Component->MotionBlurScale - 1.0f) > 0.0001f 
						|| (CheckMatrix && (!Component->LocalToWorld.Equals(*CheckMatrix, 0.0001f)))
						);

					if(	!bHasVelocity &&
						PrimitiveViewRelevance.GetDPG(DPGIndex) && 
						PrimitiveSceneInfo->bUseAsOccluder &&
						// only draw opaque primitives if wireframe is disabled
						(PrimitiveViewRelevance.bOpaqueRelevance || ViewFamily.ShowFlags & SHOW_Wireframe)
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
				bDirty |= Drawer.IsDirty();
			}

			// Perform occlusion queries for the world DPG.
			{
				SCOPED_DRAW_EVENT(EventBeginOcclude)(DEC_SCENE_ITEMS,TEXT("BeginOcclusionTests View%d"),ViewIndex);
				BeginOcclusionTests(View);
			}
		}
	}
	if( !GUseTilingCode || !bWorldDpg )
	{
		GSceneRenderTargets.FinishRenderingPrePass();
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

	UBOOL bWorldDpg = (DPGIndex == SDPG_World);
	UBOOL bDirty=0;

	// only allow rendering of mesh elements with dynamic data if not rendering in a Begin/End tiling block (only applies to world dpg and if tiling is enabled)
	UBOOL bRenderDynamicData = !bIsTiledRendering || !bWorldDpg || !GUseTilingCode;
	// only allow rendering of mesh elements with static data if within the Begin/End tiling block (only applies to world dpg and if tiling is enabled)
	UBOOL bRenderStaticData = bIsTiledRendering || !bWorldDpg || !GUseTilingCode;

	// Opaque blending, depth tests and writes.
	RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());
	RHISetDepthState(GlobalContext,TStaticDepthState<TRUE,CF_LessEqual>::GetRHI());

	// Draw the scene's emissive and light-map color.
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);
		SCOPE_CYCLE_COUNTER(STAT_BasePassDrawTime);

		FViewInfo& View = Views(ViewIndex);

		// Set the device viewport for the view.
		RHISetViewport(GlobalContext,View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
		RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

		if( bIsTiledRendering && GUseTilingCode && bWorldDpg )
		{
			RHIMSAAFixViewport();
		}

		if( bRenderStaticData )
		{
			//if we are rendering directional lightmaps, make sure simple lightmap draw lists are empty
			//otherwise if we are rendering simple lightmaps, make sure directional lightmap draw lists are empty
			checkSlow((GSystemSettings.bAllowDirectionalLightMaps 
				&& Scene->DPGs[DPGIndex].BasePassSimpleVertexLightMapDrawList.NumMeshes() == 0
				&& Scene->DPGs[DPGIndex].BasePassSimpleLightMapTextureDrawList.NumMeshes() == 0)
				|| (!GSystemSettings.bAllowDirectionalLightMaps 
				&& Scene->DPGs[DPGIndex].BasePassDirectionalVertexLightMapDrawList.NumMeshes() == 0
				&& Scene->DPGs[DPGIndex].BasePassDirectionalLightMapTextureDrawList.NumMeshes() == 0));

			// Draw the scene's base pass draw lists.
			bDirty |= Scene->DPGs[DPGIndex].BasePassNoLightMapDrawList.DrawVisible(GlobalContext,&View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassDirectionalVertexLightMapDrawList.DrawVisible(GlobalContext,&View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSimpleVertexLightMapDrawList.DrawVisible(GlobalContext,&View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassDirectionalLightMapTextureDrawList.DrawVisible(GlobalContext,&View,View.StaticMeshVisibilityMap);
			bDirty |= Scene->DPGs[DPGIndex].BasePassSimpleLightMapTextureDrawList.DrawVisible(GlobalContext,&View,View.StaticMeshVisibilityMap);
		}

		// Draw the dynamic non-occluded primitives using a base pass drawing policy.
		TDynamicPrimitiveDrawer<FBasePassOpaqueDrawingPolicyFactory> Drawer(GlobalContext,&View,DPGIndex,FBasePassOpaqueDrawingPolicyFactory::ContextType(),TRUE);
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

		// only render decals after both the static and dynamic pass have rendered. Assumes that dynamic pass comes after static pass
		if( bRenderDynamicData )
		{
			// Draw decals for non-occluded primitives using a base pass drawing policy.
			for(INT PrimitiveIndex = 0;PrimitiveIndex < View.VisibleLitDecalPrimitives.Num();PrimitiveIndex++)
			{
				const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleLitDecalPrimitives(PrimitiveIndex);
				const FPrimitiveViewRelevance& PrimitiveViewRelevance = View.PrimitiveViewRelevanceMap(PrimitiveSceneInfo->Id);

				const UBOOL bVisible = View.PrimitiveVisibilityMap(PrimitiveSceneInfo->Id);
				const UBOOL bRelevantDPG = PrimitiveViewRelevance.GetDPG(DPGIndex) != 0;

				// Only draw decals if the primitive is visible and relevant in the current DPG.
				if(bVisible && bRelevantDPG)
				{
					Drawer.SetPrimitive(PrimitiveSceneInfo);
					PrimitiveSceneInfo->Proxy->DrawLitDecalElements(
						GlobalContext,
						&Drawer,
						&View,
						DPGIndex,
						FALSE
						);
				}
			}
		}		

		bDirty |= Drawer.IsDirty(); 

		if( bRenderDynamicData )
		{
			// Draw the base pass for the view's batched mesh elements.
			bDirty |= DrawViewElements<FBasePassOpaqueDrawingPolicyFactory>(GlobalContext,&View,FBasePassOpaqueDrawingPolicyFactory::ContextType(),DPGIndex,TRUE);

			// Draw the view's batched simple elements(lines, sprites, etc).
			bDirty |= View.BatchedViewElements[DPGIndex].Draw(GlobalContext,View.ViewProjectionMatrix,appTrunc(View.SizeX),appTrunc(View.SizeY),FALSE);
		}

#if 0 // XBOX
		if( DPGIndex == SDPG_World &&
			bIsTiledRendering )
		{
			// Hack to render foreground shadows onto the world when using modulated shadows.
			// We're rendering a depth-only pass for dynamic foreground prims. Then, during the shadow projection
			// pass, we have both world and resolved foreground depth values so that shadows will be projected on the
			// foreground prims as well as the world.

			SCOPED_DRAW_EVENT(EventForegroundHack)(DEC_SCENE_ITEMS,TEXT("Foreground Depths"));

			RHISetColorWriteEnable(GlobalContext,FALSE);

			bDirty |= Scene->DPGs[SDPG_Foreground].BasePassNoLightMapDrawList.DrawVisible(GlobalContext,&View,View.StaticMeshVisibilityMap);

			// using FBasePassDrawingPolicy here instead of FDepthOnlyDrawingPolicy since masked materials are not handled by the depth only policy
			TDynamicPrimitiveDrawer<FBasePassOpaqueDrawingPolicyFactory> ForegroundDrawer(GlobalContext,&View,SDPG_Foreground,FBasePassOpaqueDrawingPolicyFactory::ContextType(),TRUE);
			for(INT PrimitiveIndex = 0;PrimitiveIndex < View.VisibleDynamicPrimitives.Num();PrimitiveIndex++)
			{
				const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives(PrimitiveIndex);
				const FPrimitiveViewRelevance& PrimitiveViewRelevance = View.PrimitiveViewRelevanceMap(PrimitiveSceneInfo->Id);
				const UBOOL bVisible = View.PrimitiveVisibilityMap(PrimitiveSceneInfo->Id);

				if(	bVisible && 
					PrimitiveViewRelevance.GetDPG(SDPG_Foreground) )
				{
					ForegroundDrawer.SetPrimitive(PrimitiveSceneInfo);
					PrimitiveSceneInfo->Proxy->DrawDynamicElements(
						&ForegroundDrawer,
						&View,
						SDPG_Foreground
						);
				}
			}

			RHISetColorWriteEnable(GlobalContext,TRUE);

			bDirty |= ForegroundDrawer.IsDirty();
		}
#endif

	}

	return bDirty;
}

/** 
* Renders the post process effects for a view. 
* @param DPGIndex - current depth priority group (DPG)
*/
void FSceneRenderer::RenderPostProcessEffects(UINT DPGIndex)
{
	SCOPED_DRAW_EVENT(EventPP)(DEC_SCENE_ITEMS,TEXT("PostProcessEffects"));

	UBOOL bSetAllocations = FALSE;

	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

		FViewInfo& View = Views(ViewIndex);
		RHISetViewParameters( GlobalContext, &View, View.ViewProjectionMatrix, View.ViewOrigin );

		// render any custom post process effects
		for( INT EffectIdx=0; EffectIdx < View.PostProcessSceneProxies.Num(); EffectIdx++ )
		{
			FPostProcessSceneProxy* PPEffectProxy = &View.PostProcessSceneProxies(EffectIdx);
			if( PPEffectProxy && 
				PPEffectProxy->GetDepthPriorityGroup() == DPGIndex )
			{
				if (!bSetAllocations)
				{
					// allocate more GPRs for pixel shaders
					RHISetShaderRegisterAllocation(32, 96);
					bSetAllocations = TRUE;
				}

				// render the effect
				PPEffectProxy->Render(GlobalContext, DPGIndex,View,CanvasTransform);
			}
		}
	}

	if (bSetAllocations)
	{
		// restore default GPR allocation
		RHISetShaderRegisterAllocation(64, 64);
	}
}

/**
* Clears the scene color depth (stored in alpha channel) to max depth
* This is needed for depth bias blend materials to show up correctly
*/
void FSceneRenderer::ClearSceneColorDepth()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("Clear Depth"));

	const FLinearColor ClearDepthColor(0,0,0,65530);
	
	FBatchedElements BatchedElements;
	INT V00 = BatchedElements.AddVertex(FVector4(-1,-1,0,1),FVector2D(0,0),ClearDepthColor,FHitProxyId());
	INT V10 = BatchedElements.AddVertex(FVector4(1,-1,0,1),FVector2D(1,0),ClearDepthColor,FHitProxyId());
	INT V01 = BatchedElements.AddVertex(FVector4(-1,1,0,1),FVector2D(0,1),ClearDepthColor,FHitProxyId());
	INT V11 = BatchedElements.AddVertex(FVector4(1,1,0,1),FVector2D(1,1),ClearDepthColor,FHitProxyId());

	// No alpha blending, no depth tests or writes, no backface culling.
	RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());
	RHISetDepthState(GlobalContext,TStaticDepthState<FALSE,CF_Always>::GetRHI());
	RHISetRasterizerState(GlobalContext,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	RHISetColorWriteMask(GlobalContext,CW_ALPHA);

	// Draw a quad using the generated vertices.
	BatchedElements.AddTriangle(V00,V10,V11,GWhiteTexture,BLEND_Opaque);
	BatchedElements.AddTriangle(V00,V11,V01,GWhiteTexture,BLEND_Opaque);
	BatchedElements.Draw(
		GlobalContext,
		FMatrix::Identity,
		ViewFamily.RenderTarget->GetSizeX(),
		ViewFamily.RenderTarget->GetSizeY(),
		FALSE
		);

	RHISetColorWriteMask(GlobalContext,CW_RGBA);
}

/** 
* Renders the scene to capture target textures 
*/
void FSceneRenderer::RenderSceneCaptures()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("Scene Captures"));

	// disable tiling for rendering captures
	UBOOL SavedUseTilingCode = GUseTilingCode;
	GUseTilingCode = FALSE;

	for( TSparseArray<FCaptureSceneInfo*>::TConstIterator CaptureIt(Scene->SceneCaptures); CaptureIt; ++CaptureIt )
	{
		FCaptureSceneInfo* CaptureInfo = *CaptureIt;
        CaptureInfo->CaptureScene(this);
	}

	// restore tiling setting
	GUseTilingCode = SavedUseTilingCode;
}

/** Updates the game-thread actor and primitive visibility states. */
void FSceneRenderer::SaveVisibilityState()
{
	class FActorVisibilitySet : public FActorVisibilityHistoryInterface
	{
	public:

		/**
		 * Adds an actor to the visibility set.  Ensures that duplicates are not added.
		 * @param VisibleActor - The actor which is visible.
		 */
		void AddActor(const AActor* VisibleActor)
		{
			if(!VisibleActors.Find(VisibleActor))
			{
				VisibleActors.Set(VisibleActor,TRUE);
			}
		}

		// FActorVisibilityHistoryInterface
		virtual UBOOL GetActorVisibility(const AActor* TestActor) const
		{
			return VisibleActors.Find(TestActor) != NULL;
		}

	private:

		// The set of visible actors.
		TMap<const AActor*,UBOOL> VisibleActors;
	};

	// Update LastRenderTime for the primitives which were visible.
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views(ViewIndex);

		// Allocate an actor visibility set for the actor.
		FActorVisibilitySet* ActorVisibilitySet = new FActorVisibilitySet;

		// check if we are freezing the frame
		FSceneViewState* ViewState = (FSceneViewState*)View.State;
		const UBOOL bIsParent = ViewState && ViewState->IsViewParent();
		if ( bIsParent )
		{
			ViewState->ParentPrimitives.Empty();
		}
#if !FINAL_RELEASE
		const UBOOL bIsFreezing = ViewState && ViewState->bIsFreezing;
#endif

		// Iterate over the visible primitives.
		for(TSparseArray<FPrimitiveSceneInfoCompact>::TConstSubsetIterator PrimitiveIt(Scene->Primitives,View.PrimitiveVisibilityMap);
			PrimitiveIt;
			++PrimitiveIt
			)
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveIt->PrimitiveSceneInfo;

			// When using latent occlusion culling, this check will indicate the primitive is occluded unless it has been unoccluded the past two frames.
			// This ignores the first frame a primitive becomes visible without occlusion information and waits until the occlusion query results are available
			// the next frame before signaling to the game systems that the primitive is visible.
			const UBOOL bWasOccluded = ViewState && ViewState->WasPrimitivePreviouslyOccluded(PrimitiveIt->Component,ViewFamily.CurrentRealTime);
			if(!bWasOccluded)
			{
				PrimitiveIt->Component->LastRenderTime = ViewFamily.CurrentWorldTime;
				PrimitiveIt->PrimitiveSceneInfo->LastRenderTime = ViewFamily.CurrentWorldTime;
				if(PrimitiveSceneInfo->Owner)
				{
					ActorVisibilitySet->AddActor(PrimitiveSceneInfo->Owner);
					PrimitiveSceneInfo->Owner->LastRenderTime = ViewFamily.CurrentWorldTime;
				}
				if(PrimitiveSceneInfo->LightEnvironmentSceneInfo)
				{
					PrimitiveSceneInfo->LightEnvironmentSceneInfo->Component->LastRenderTime = ViewFamily.CurrentWorldTime;
				}
				// Store the primitive for parent occlusion rendering.
				if ( bIsParent )
				{
					ViewState->ParentPrimitives.Add(PrimitiveIt->Component);
				}
			}
#if !FINAL_RELEASE
			// if we are freezing the scene, then remember the primitive was rendered
			if (bIsFreezing)
			{
				ViewState->FrozenPrimitives.Add(PrimitiveIt->Component);
			}
#endif
		}
		
		// Update the view's actor visibility history with the new visibility set.
		if(View.ActorVisibilityHistory)
		{
			View.ActorVisibilityHistory->SetStates(ActorVisibilitySet);
		}
		else
		{
			delete ActorVisibilitySet;
		}
	}
}

/** 
* Global state shared by all FSceneRender instances 
* @return global state
*/
FGlobalSceneRenderState* FSceneRenderer::GetGlobalSceneRenderState()
{
	static FGlobalSceneRenderState GlobalSceneRenderState;
	return &GlobalSceneRenderState;
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
	{
		SCOPE_CYCLE_COUNTER(STAT_TotalSceneRenderingTime);

		SceneRenderer->GlobalContext = RHIGetGlobalContext();

		// keep track of global frame number
		SceneRenderer->GetGlobalSceneRenderState()->FrameNumber++;

		// Commit the scene's pending light attachments.
		SceneRenderer->Scene->CommitPendingLightAttachments();

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

	INC_DWORD_STAT_BY(STAT_DrawEvents,FDrawEvent::Counter);
	STAT(FDrawEvent::Counter=0);

#if STATS && CONSOLE
	/** Update STATS with the total GPU time taken to render the last frame. */
	SET_CYCLE_COUNTER(STAT_TotalGPUFrameTime, RHIGetGPUFrameCycles(), 1);
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
		FScene* const Scene = (FScene*)ViewFamily->Scene->GetRenderScene();
		UWorld* const World = Scene->GetWorld();
		if(World)
		{
			World->GetWorldInfo()->SetMapNeedsLightingFullyRebuilt(Scene->NumUncachedStaticLightingInteractions > 0);
		}

#if GEMINI_TODO
		// We need to pass the scene's hit proxies through to the hit proxy consumer!
		// Otherwise its possible the hit proxy consumer will cache a hit proxy ID it doesn't have a reference for.
		// Note that the effects of not doing this correctly are minor: clicking on a primitive that moved without invalidating the viewport's
		// cached hit proxies won't work.  Is this worth the pain?
#endif

		// Construct the scene renderer.  This copies the view family attributes into its own structures.
		FSceneRenderer* SceneRenderer = ::new FSceneRenderer(ViewFamily,Canvas->GetHitProxyConsumer(),Canvas->GetFullTransform());

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
		FSceneRenderer* SceneRenderer = ::new FSceneRenderer(ViewFamily,Canvas->GetHitProxyConsumer(),Canvas->GetFullTransform());

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FDrawSceneCommandPP,
			FSceneRenderer*,SceneRenderer,SceneRenderer,
		{
			SceneRenderer->GlobalContext = RHIGetGlobalContext();
			SceneRenderer->RenderPostProcessOnly();
			delete SceneRenderer;
		});
	}

	// We need to flush rendering commands if stats gathering is enabled to ensure that the stats are valid/ captured
	// before this function returns.
	if( ViewFamily->DynamicShadowStats )
	{
		FlushRenderingCommands();
	}
}

/*-----------------------------------------------------------------------------
	Stat declarations.
-----------------------------------------------------------------------------*/

DECLARE_STATS_GROUP(TEXT("SceneRendering"),STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Occlusion query time"),STAT_OcclusionQueryTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("InitViews time"),STAT_InitViewsTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Dynamic shadow setup time"),STAT_DynamicShadowSetupTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Translucency setup time"),STAT_TranslucencySetupTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Total CPU rendering time"),STAT_TotalSceneRenderingTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Total GPU rendering time"),STAT_TotalGPUFrameTime,STATGROUP_SceneRendering);

DECLARE_CYCLE_STAT(TEXT("Depth drawing time"),STAT_DepthDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Base pass drawing time"),STAT_BasePassDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Shadow volume drawing time"),STAT_ShadowVolumeDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Light function drawing time"),STAT_LightFunctionDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Lighting drawing time"),STAT_LightingDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Proj Shadow drawing time"),STAT_ProjectedShadowDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Mod Shadow drawing time"),STAT_ModulatedShadowDrawTime,STATGROUP_SceneRendering);
DECLARE_CYCLE_STAT(TEXT("Translucency drawing time"),STAT_TranslucencyDrawTime,STATGROUP_SceneRendering);

DECLARE_DWORD_COUNTER_STAT(TEXT("Culled primitives"),STAT_CulledPrimitives,STATGROUP_SceneRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Occluded primitives"),STAT_OccludedPrimitives,STATGROUP_SceneRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Occlusion queries"),STAT_OcclusionQueries,STATGROUP_SceneRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Projected shadows"),STAT_ProjectedShadows,STATGROUP_SceneRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Visible static mesh elements"),STAT_VisibleStaticMeshElements,STATGROUP_SceneRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Visible dynamic primitives"),STAT_VisibleDynamicPrimitives,STATGROUP_SceneRendering);
DECLARE_DWORD_COUNTER_STAT(TEXT("Draw events"),STAT_DrawEvents,STATGROUP_SceneRendering);

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Lights"),STAT_SceneLights,STATGROUP_SceneRendering);

