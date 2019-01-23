/*=============================================================================
	SceneRendering.cpp: Scene rendering.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

/** The amount of time a primitive is considered to be probably visible after it was last actually visible. */
static const FLOAT GPrimitiveProbablyVisibleTime = 8.0f;

/** The percent of previously unoccluded primitives which are requeried every frame. */
static const FLOAT GPercentUnoccludedRequeries = 0.125f;

/**
* A vertex shader for rendering a texture on a simple element.
*/
class FOcclusionQueryVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FOcclusionQueryVertexShader,Global);
public:

	static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

	FOcclusionQueryVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
	}
	FOcclusionQueryVertexShader() {}

	void SetParameters(FCommandContextRHI* Context,const FMatrix& ViewProjectionMatrix)
	{
	}

	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
	}
};

// Shader implementations.
IMPLEMENT_SHADER_TYPE(,FOcclusionQueryVertexShader,TEXT("OcclusionQueryVertexShader"),TEXT("Main"),SF_Vertex,0,0);

/**
* The occlusion query vertex declaration resource type.
*/
class FOcclusionQueryVertexDeclaration : public FRenderResource
{
public:

	FVertexDeclarationRHIRef VertexDeclarationRHI;

	/** Destructor. */
	virtual ~FOcclusionQueryVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		Elements.AddItem(FVertexElement(0,0,VET_Float3,VEU_Position,0));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.Release();
	}
};

/** The occlusion query vertex declaration. */
TGlobalResource<FOcclusionQueryVertexDeclaration> GOcclusionQueryVertexDeclaration;

void FOcclusionQueryPool::Release()
{
	OcclusionQueries.Empty();
}

FOcclusionQueryRHIParamRef FOcclusionQueryPool::Allocate()
{
	// Check if there are any free occlusion queries in the pool.
	if(NextFreeQueryIndex >= OcclusionQueries.Num())
	{
		check(NextFreeQueryIndex == OcclusionQueries.Num());

		// There aren't any free occlusion queries; create a new occlusion query.
		OcclusionQueries.AddItem(RHICreateOcclusionQuery());
	}

	// Return the next free occlusion query in the pool.
	return OcclusionQueries(NextFreeQueryIndex++);
}

void FOcclusionQueryPool::Reset()
{
	NextFreeQueryIndex = 0;
}

FOcclusionQueryPool::~FOcclusionQueryPool()
{
	Release();
}

/**
* Expands a primitive bounds slightly to prevent the primitive from occluding it.
* @param Bounds - The base bounds of the primitive.
* @return The expanded bounds.
*/
static FBoxSphereBounds GetOcclusionBounds(const FBoxSphereBounds& Bounds)
{
	// Scale the box proportional to it's size. This is to prevent false occlusion reports.
	static const FLOAT BoundsOffset = 1.0f;
	static const FLOAT BoundsScale = 1.1f;
	static const FLOAT BoundsScaledOffset = BoundsOffset * BoundsScale;
	static const FVector BoundsScaledOffsetVector = FVector(1,1,1) * BoundsScaledOffset;
	return FBoxSphereBounds(
		Bounds.Origin,
		Bounds.BoxExtent * BoundsScale + BoundsScaledOffsetVector,
		Bounds.SphereRadius * BoundsScale + BoundsScaledOffset
		);
}

FBoundShaderStateRHIRef FSceneRenderer::OcclusionTestBoundShaderState;

UBOOL FSceneViewState::WasPrimitivePreviouslyOccluded(const UPrimitiveComponent* Primitive,FLOAT MinTime) const
{
	const FPrimitiveOcclusionHistory* PrimitiveOcclusionHistory = PrimitiveOcclusionHistorySet.Find(Primitive);

	// If the primitive hasn't been in the view frustum recently, or it hasn't been unoccluded since before MinTime, return TRUE.
	return
		!PrimitiveOcclusionHistory ||
		PrimitiveOcclusionHistory->LastVisibleTime < MinTime;
}

void FSceneViewState::TrimOcclusionHistory(FLOAT MinHistoryTime,FLOAT MinQueryTime)
{
	for(THashSet<FPrimitiveOcclusionHistory,FPrimitiveOcclusionHistory::KeyFuncs,1>::TIterator PrimitiveIt(PrimitiveOcclusionHistorySet);PrimitiveIt;PrimitiveIt.Next())
	{
		// If the primitive has an old pending occlusion query, release it.
		if(PrimitiveIt->LastConsideredTime < MinQueryTime)
		{
			PrimitiveIt->PendingOcclusionQuery.Release();
		}

		// If the primitive hasn't been considered for visibility recently, remove its history from the set.
		if(PrimitiveIt->LastConsideredTime < MinHistoryTime)
		{
			PrimitiveIt.RemoveCurrent();
		}
	}
}

UBOOL FSceneViewState::UpdatePrimitiveOcclusion(const FPrimitiveSceneInfoCompact& CompactPrimitiveSceneInfo,FViewInfo& View,FLOAT CurrentRealTime)
{
	// Find the primitive's occlusion history.
	FPrimitiveOcclusionHistory* PrimitiveOcclusionHistory = PrimitiveOcclusionHistorySet.Find(CompactPrimitiveSceneInfo.Component);
	UBOOL bIsOccluded;
	UBOOL bOcclusionStateIsDefinite = FALSE;
	if(!PrimitiveOcclusionHistory)
	{
		// If the primitive doesn't have an occlusion history yet, create it.
		PrimitiveOcclusionHistory = PrimitiveOcclusionHistorySet.Add(FPrimitiveOcclusionHistory(CompactPrimitiveSceneInfo.Component));

		// If the primitive hasn't been visible recently enough to have a history, treat it as unoccluded this frame so it will be rendered as an occluder and its true occlusion state can be determined.
		bIsOccluded = FALSE;

		// Flag the primitive's occlusion state as indefinite, which will force it to be queried this frame.
		bOcclusionStateIsDefinite = FALSE;
	}
	else
	{
		if(View.bIgnoreOcclusionQueries)
		{
			// If the view is ignoring occlusion queries, the primitive is definitely unoccluded.
			bIsOccluded = FALSE;
			bOcclusionStateIsDefinite = TRUE;
		}
		else
		{
			// Read the occlusion query results.
			DWORD NumPixels = 0;
			if(	IsValidRef(PrimitiveOcclusionHistory->PendingOcclusionQuery) &&
				RHIGetOcclusionQueryResult(PrimitiveOcclusionHistory->PendingOcclusionQuery,NumPixels,TRUE))
			{
				// The primitive is occluded if non of its bounding box's pixels were visible in the previous frame's occlusion query.
				bIsOccluded = (NumPixels == 0);

				PrimitiveOcclusionHistory->LastPixelsPercentage = NumPixels / (View.SizeX * View.SizeY);

				// Flag the primitive's occlusion state as definite, since we have query results for it.
				bOcclusionStateIsDefinite = TRUE;
			}
			else
			{
				// If there's no occlusion query for the primitive, set it's visibility state to whether it has been unoccluded recently.
				bIsOccluded = (PrimitiveOcclusionHistory->LastVisibleTime + GPrimitiveProbablyVisibleTime < CurrentRealTime);
				if (bIsOccluded)
				{
					PrimitiveOcclusionHistory->LastPixelsPercentage = 0.0f;
				}
				else
				{
					PrimitiveOcclusionHistory->LastPixelsPercentage = 1.0f;
				}
			}
		}

		// Clear the primitive's pending occlusion query.
		PrimitiveOcclusionHistory->PendingOcclusionQuery.Release();
	}

	// Set the primitive's considered time to keep its occlusion history from being trimmed.
	PrimitiveOcclusionHistory->LastConsideredTime = CurrentRealTime;

	// Enqueue the next frame's occlusion query for the primitive.
	if (!View.bIgnoreOcclusionQueries
#if !FINAL_RELEASE
		&& !HasViewParent() && !bIsFrozen
#endif
		)
	{
		const FBoxSphereBounds OcclusionBounds = GetOcclusionBounds(CompactPrimitiveSceneInfo.Bounds);

		// Don't query primitives whose bounding boxes possibly intersect the viewer's near clipping plane.
		const FLOAT BoundsPushOut = FBoxPushOut(View.NearClippingPlane,OcclusionBounds.BoxExtent);
		const FLOAT ClipDistance = View.NearClippingPlane.PlaneDot(OcclusionBounds.Origin);
		if((View.bHasNearClippingPlane && ClipDistance < -BoundsPushOut) ||
			(!View.bHasNearClippingPlane && OcclusionBounds.SphereRadius < HALF_WORLD_MAX))
		{
			// Decide whether to use an individual or grouped occlusion query.
			UBOOL bIndividualOcclusionQuery = FALSE;
			UBOOL bGroupedOcclusionQuery = FALSE;
			if(CompactPrimitiveSceneInfo.bAllowApproximateOcclusion)
			{
				if(bIsOccluded)
				{
					// Primitives that were occluded the previous frame use grouped queries.
					bGroupedOcclusionQuery = TRUE;
				}
				else
				{
					// If the primitive's is definitely unoccluded, only requery it occasionally.
					static FRandomStream UnoccludedQueryRandomStream(0);
					bIndividualOcclusionQuery = !bOcclusionStateIsDefinite || (UnoccludedQueryRandomStream.GetFraction() < GPercentUnoccludedRequeries);
				}
			}
			else
			{
				// Primitives that need precise occlusion results use individual queries.
				bIndividualOcclusionQuery = TRUE;
			}

			// Create the primitive's occlusion query in the appropriate batch.
			if(bIndividualOcclusionQuery)
			{
				PrimitiveOcclusionHistory->PendingOcclusionQuery = View.IndividualOcclusionQueries.BatchPrimitive(OcclusionBounds);	
			}
			else if(bGroupedOcclusionQuery)
			{
				PrimitiveOcclusionHistory->PendingOcclusionQuery = View.GroupedOcclusionQueries.BatchPrimitive(OcclusionBounds);	
			}
		}
		else
		{
			// If the primitive's bounding box intersects the near clipping plane, treat it as definitely unoccluded.
			bIsOccluded = FALSE;
			bOcclusionStateIsDefinite = TRUE;
		}
	}

	if((bOcclusionStateIsDefinite 
#if !FINAL_RELEASE
		|| bIsFrozen
#endif
		) && !bIsOccluded)
	{
		// Update the primitive's visibility time in the occlusion history.
		// This is only done when we have definite occlusion results for the primitive.
		PrimitiveOcclusionHistory->LastVisibleTime = CurrentRealTime;
	}

	return bIsOccluded;
}

UBOOL FSceneViewState::IsShadowOccluded(const UPrimitiveComponent* Primitive,const ULightComponent* Light) const
{
	// Find the shadow's occlusion query from the previous frame.
	const FSceneViewState::FProjectedShadowKey Key(Primitive,Light);
	const FOcclusionQueryRHIRef* Query = ShadowOcclusionQueryMap.Find(Key);

	// Read the occlusion query results.
	DWORD NumPixels = 0;
	if(Query && RHIGetOcclusionQueryResult(*Query,NumPixels,TRUE))
	{
		// If the shadow's occlusion query didn't have any pixels visible the previous frame, it's occluded.
		return NumPixels == 0;
	}
	else
	{
		// If the shadow wasn't queried the previous frame, it isn't occluded.
		return FALSE;
	}
}

/**
 *	Retrieves the percentage of the views render target the primitive touched last time it was rendered.
 *
 *	@param	Primitive				The primitive of interest.
 *	@param	OutCoveragePercentage	REFERENCE: The screen coverate percentage. (OUTPUT)
 *	@return	UBOOL					TRUE if the primitive was found and the results are valid, FALSE is not
 */
UBOOL FSceneViewState::GetPrimitiveCoveragePercentage(const UPrimitiveComponent* Primitive, FLOAT& OutCoveragePercentage)
{
	FPrimitiveOcclusionHistory* PrimitiveOcclusionHistory = PrimitiveOcclusionHistorySet.Find(Primitive);
	if (PrimitiveOcclusionHistory)
	{
		OutCoveragePercentage = PrimitiveOcclusionHistory->LastPixelsPercentage;
		return TRUE;
	}

	return FALSE;
}

FOcclusionQueryBatcher::FOcclusionQueryBatcher(class FSceneViewState* ViewState,UINT InMaxBatchedPrimitives)
:	CurrentBatchOcclusionQuery(FOcclusionQueryRHIRef())
,	MaxBatchedPrimitives(InMaxBatchedPrimitives)
,	NumBatchedPrimitives(0)
,	OcclusionQueryPool(ViewState ? &ViewState->OcclusionQueryPool : NULL)
{}

FOcclusionQueryBatcher::~FOcclusionQueryBatcher()
{
	check(!Primitives.Num());
}

void FOcclusionQueryBatcher::Flush(FCommandContextRHI* Context)
{
	if(BatchOcclusionQueries.Num())
	{
		WORD* BakedIndices = new WORD[MaxBatchedPrimitives * 12 * 3];
		for(UINT PrimitiveIndex = 0;PrimitiveIndex < MaxBatchedPrimitives;PrimitiveIndex++)
		{
			for(INT Index = 0;Index < 12*3;Index++)
			{
				BakedIndices[PrimitiveIndex * 12 * 3 + Index] = PrimitiveIndex * 8 + GCubeIndices[Index];
			}
		}

		// Draw the batches.
		for(INT BatchIndex = 0;BatchIndex < BatchOcclusionQueries.Num();BatchIndex++)
		{
			FOcclusionQueryRHIParamRef BatchOcclusionQuery = BatchOcclusionQueries(BatchIndex);
			const INT NumPrimitivesInBatch = Clamp<INT>( Primitives.Num() - BatchIndex * MaxBatchedPrimitives, 0, MaxBatchedPrimitives );
				
			RHIBeginOcclusionQuery(Context,BatchOcclusionQuery);

			FLOAT* RESTRICT Vertices;
			WORD* RESTRICT Indices;
			RHIBeginDrawIndexedPrimitiveUP(Context,PT_TriangleList,NumPrimitivesInBatch * 12,NumPrimitivesInBatch * 8,sizeof(FVector),*(void**)&Vertices,0,NumPrimitivesInBatch * 12 * 3,sizeof(WORD),*(void**)&Indices);

			for(INT PrimitiveIndex = 0;PrimitiveIndex < NumPrimitivesInBatch;PrimitiveIndex++)
			{
				const FPrimitive& Primitive = Primitives(BatchIndex * MaxBatchedPrimitives + PrimitiveIndex);
				const UINT BaseVertexIndex = PrimitiveIndex * 8;
				const FVector PrimitiveBoxMin = Primitive.Origin - Primitive.Extent;
				const FVector PrimitiveBoxMax = Primitive.Origin + Primitive.Extent;

				Vertices[ 0] = PrimitiveBoxMin.X; Vertices[ 1] = PrimitiveBoxMin.Y; Vertices[ 2] = PrimitiveBoxMin.Z;
				Vertices[ 3] = PrimitiveBoxMin.X; Vertices[ 4] = PrimitiveBoxMin.Y; Vertices[ 5] = PrimitiveBoxMax.Z;
				Vertices[ 6] = PrimitiveBoxMin.X; Vertices[ 7] = PrimitiveBoxMax.Y; Vertices[ 8] = PrimitiveBoxMin.Z;
				Vertices[ 9] = PrimitiveBoxMin.X; Vertices[10] = PrimitiveBoxMax.Y; Vertices[11] = PrimitiveBoxMax.Z;
				Vertices[12] = PrimitiveBoxMax.X; Vertices[13] = PrimitiveBoxMin.Y; Vertices[14] = PrimitiveBoxMin.Z;
				Vertices[15] = PrimitiveBoxMax.X; Vertices[16] = PrimitiveBoxMin.Y; Vertices[17] = PrimitiveBoxMax.Z;
				Vertices[18] = PrimitiveBoxMax.X; Vertices[19] = PrimitiveBoxMax.Y; Vertices[20] = PrimitiveBoxMin.Z;
				Vertices[21] = PrimitiveBoxMax.X; Vertices[22] = PrimitiveBoxMax.Y; Vertices[23] = PrimitiveBoxMax.Z;

				Vertices += 24;
			}

			appMemcpy(Indices,BakedIndices,sizeof(WORD) * NumPrimitivesInBatch * 12 * 3);

			RHIEndDrawIndexedPrimitiveUP(Context);
			RHIEndOcclusionQuery(Context,BatchOcclusionQuery);
		}

		delete[] BakedIndices;

		INC_DWORD_STAT_BY(STAT_OcclusionQueries,BatchOcclusionQueries.Num());

		// Reset the batch state.
		BatchOcclusionQueries.Empty();
		Primitives.Empty();
		CurrentBatchOcclusionQuery = FOcclusionQueryRHIRef();
	}
}

FOcclusionQueryRHIParamRef FOcclusionQueryBatcher::BatchPrimitive(const FBoxSphereBounds& Bounds)
{
	// Check if the current batch is full.
	if(!IsValidRef(CurrentBatchOcclusionQuery) || NumBatchedPrimitives >= MaxBatchedPrimitives)
	{
		check(OcclusionQueryPool);
		CurrentBatchOcclusionQuery = OcclusionQueryPool->Allocate();
		BatchOcclusionQueries.AddItem(CurrentBatchOcclusionQuery);
		NumBatchedPrimitives = 0;
	}

	// Add the primitive to the current batch.
	FPrimitive* const Primitive = new(Primitives) FPrimitive;
	Primitive->Origin = Bounds.Origin;
	Primitive->Extent = Bounds.BoxExtent;
	NumBatchedPrimitives++;

	return CurrentBatchOcclusionQuery;
}

void FSceneRenderer::BeginOcclusionTests(FViewInfo& View)
{
	FSceneViewState* ViewState = (FSceneViewState*)View.State;

	if(ViewState && !View.bIgnoreOcclusionQueries)
	{
		SCOPE_CYCLE_COUNTER(STAT_OcclusionQueryTime);

		// Lookup the vertex shader.
		TShaderMapRef<FOcclusionQueryVertexShader> VertexShader(GetGlobalShaderMap());

		// Issue this frame's occlusion queries (occlusion queries from last frame may still be in flight)
		TMap<FSceneViewState::FProjectedShadowKey, FOcclusionQueryRHIRef>& ShadowOcclusionQueryMap = ViewState->ShadowOcclusionQueryMap;

		// Clear primitives which haven't been visible recently out of the occlusion history, and reset old pending occlusion queries.
		ViewState->TrimOcclusionHistory(ViewFamily.CurrentRealTime - GPrimitiveProbablyVisibleTime,ViewFamily.CurrentRealTime);

		// Depth tests, no depth writes, no color writes, opaque, solid rasterization wo/ backface culling.
		RHISetDepthState(GlobalContext,TStaticDepthState<FALSE,CF_LessEqual>::GetRHI());
		RHISetColorWriteEnable(GlobalContext,FALSE);
		// We only need to render the front-faces of the culling geometry (this halves the amount of pixels we touch)
		RHISetRasterizerState(GlobalContext,
			GInvertCullMode ? TStaticRasterizerState<FM_Solid,CM_CCW>::GetRHI() : TStaticRasterizerState<FM_Solid,CM_CW>::GetRHI()); 
		RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());

		// Cache the bound shader state
		if (!IsValidRef(OcclusionTestBoundShaderState))
		{
			DWORD Strides[MaxVertexElementCount];
			appMemzero(Strides, sizeof(Strides));
			Strides[0] = sizeof(FVector); // Stride comes from code in FOcclusionQueryBatcher 

			OcclusionTestBoundShaderState = RHICreateBoundShaderState(GOcclusionQueryVertexDeclaration.VertexDeclarationRHI, Strides, VertexShader->GetVertexShader(), FPixelShaderRHIRef());
		}

		// Set the occlusion query shaders.
		VertexShader->SetParameters(GlobalContext,View.ViewProjectionMatrix);

		RHISetBoundShaderState(GlobalContext, OcclusionTestBoundShaderState);

		ShadowOcclusionQueryMap.Reset();

		for(INT ShadowIndex = 0;ShadowIndex < ProjectedShadows.Num();ShadowIndex++)
		{
			const FProjectedShadowInfo& ProjectedShadowInfo = ProjectedShadows(ShadowIndex);

			// Don't query preshadows, since they are culled if their subject is occluded.
			if(!ProjectedShadowInfo.bPreShadow)
			{
				// If the shadow frustum is farther from the view origin than the near clipping plane,
				// it can't intersect the near clipping plane.
				UBOOL bIntersectsNearClippingPlane = ProjectedShadowInfo.ReceiverFrustum.IntersectSphere(View.ViewOrigin,View.NearClippingDistance * appSqrt(3.0f));
				if( !bIntersectsNearClippingPlane )
				{
					// Allocate an occlusion query for the primitive from the occlusion query pool.
					FOcclusionQueryRHIParamRef ShadowOcclusionQuery = ViewState->OcclusionQueryPool.Allocate();

					// Draw the primitive's bounding box, using the occlusion query.
					RHIBeginOcclusionQuery(GlobalContext,ShadowOcclusionQuery);

					void* VerticesPtr;
					void* IndicesPtr;
					// preallocate memory to fill out with vertices and indices
					RHIBeginDrawIndexedPrimitiveUP(GlobalContext, PT_TriangleList, 12, 8, sizeof(FVector), VerticesPtr, 0, 36, sizeof(WORD), IndicesPtr);
					FVector* Vertices = (FVector*)VerticesPtr;
					WORD* Indices = (WORD*)IndicesPtr;

					// Generate vertices for the shadow's frustum.
					for(UINT Z = 0;Z < 2;Z++)
					{
						for(UINT Y = 0;Y < 2;Y++)
						{
							for(UINT X = 0;X < 2;X++)
							{
								const FVector4 UnprojectedVertex = ProjectedShadowInfo.InvReceiverMatrix.TransformFVector4(
									FVector4(
										(X ? -1.0f : 1.0f),
										(Y ? -1.0f : 1.0f),
										(Z ?  1.0f : 0.0f),
										1.0f
										)
									);
								const FVector ProjectedVertex = UnprojectedVertex / UnprojectedVertex.W;
								Vertices[GetCubeVertexIndex(X,Y,Z)] = ProjectedVertex;
							}
						}
					}

					// we just copy the indices right in
					appMemcpy(Indices, GCubeIndices, sizeof(GCubeIndices));

					FSceneViewState::FProjectedShadowKey Key(ProjectedShadowInfo.SubjectSceneInfo->Component,ProjectedShadowInfo.LightSceneInfo->LightComponent);
					checkSlow(ShadowOcclusionQueryMap.Find(Key) == NULL);
					ShadowOcclusionQueryMap.Set(Key, ShadowOcclusionQuery);

					RHIEndDrawIndexedPrimitiveUP(GlobalContext);
					RHIEndOcclusionQuery(GlobalContext,ShadowOcclusionQuery);
				}
			}
		}

		// Don't do primitive occlusion if we have a view parent or are frozen.
#if !FINAL_RELEASE
		if ( !ViewState->HasViewParent() && !ViewState->bIsFrozen )
#endif
		{
			View.IndividualOcclusionQueries.Flush(GlobalContext);
			View.GroupedOcclusionQueries.Flush(GlobalContext);
		}
		
		// Reenable color writes.
		RHISetColorWriteEnable(GlobalContext,TRUE);

		// Kick the commands.
		RHIKickCommandBuffer(GlobalContext);
	}
}
