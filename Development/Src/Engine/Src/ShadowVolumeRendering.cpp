/*=============================================================================
	ShadowVolumeRendering.cpp: Implementation for rendering shadow volumes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"

//
FShadowVolumeCache::~FShadowVolumeCache()
{
	// Remove the cache from the resource list.
	check(IsInRenderingThread());
	Release();
}

//
FShadowVolumeCache::FCachedShadowVolume* FShadowVolumeCache::AddShadowVolume(const class FLightSceneInfo* LightSceneInfo,FShadowIndexBuffer& IndexBuffer)
{
	// If the cache isn't already in the resource list, add it.
	check(IsInRenderingThread());
	Init();

	FCachedShadowVolume ShadowVolume;
	ShadowVolume.NumTriangles = IndexBuffer.Indices.Num() / 3;

	DWORD Size = IndexBuffer.Indices.Num() * sizeof(WORD);
	if( Size )
	{
		ShadowVolume.IndexBufferRHI = RHICreateIndexBuffer( sizeof(WORD), Size, NULL, FALSE /*TRUE*/ );

		// Initialize the buffer.
		void* Buffer = RHILockIndexBuffer(ShadowVolume.IndexBufferRHI,0,Size);
		appMemcpy(Buffer,&IndexBuffer.Indices(0),Size);
		RHIUnlockIndexBuffer(ShadowVolume.IndexBufferRHI);
	}

	return &CachedShadowVolumes.Set(LightSceneInfo,ShadowVolume);
}

//
void FShadowVolumeCache::RemoveShadowVolume(const class FLightSceneInfo* LightSceneInfo)
{
	CachedShadowVolumes.Remove(LightSceneInfo);
}

//
const FShadowVolumeCache::FCachedShadowVolume* FShadowVolumeCache::GetShadowVolume(const class FLightSceneInfo* LightSceneInfo) const
{
	return CachedShadowVolumes.Find(LightSceneInfo);
}

//
void FShadowVolumeCache::ReleaseRHI()
{
	// Release the cached shadow volume index buffers.
	CachedShadowVolumes.Empty();
}

/**
 * A vertex shader for rendering a shadow volume.
 */
class FShadowVolumeVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShadowVolumeVertexShader,Global);
public:

	/**
	 * Returns whether the vertex shader should be cached or not.
	 *
	 * @return	Always returns TRUE
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform)
	{
		return TRUE;
	}

	/** Empty default constructor */
	FShadowVolumeVertexShader( )	{ }

	/**
	 * Constructor that initializes the shader parameters.
	 *
	 * @param Initializer	Used for finding the shader parameters
	 */
	FShadowVolumeVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		LightPosition.Bind(Initializer.ParameterMap,TEXT("LightPosition"));
		BaseExtrusion.Bind(Initializer.ParameterMap,TEXT("BaseExtrusion"));
		LocalToWorld.Bind(Initializer.ParameterMap,TEXT("LocalToWorld"),TRUE);
	}

	/**
	 * Activates the vertex shaders for rendering and sets the shader parameters that are common
	 * to all shadow volumes.
	 *
	 * @param Context			RHI context (will always be NULL)
	 * @param View				Current view
	 * @param LightSceneInfo	Represents the current light
	 */
	void SetParameters( FCommandContextRHI* Context, const FSceneView* View, const FLightSceneInfo* LightSceneInfo )
	{
		// Set the homogenous position of the light.
		SetVertexShaderValue(Context,GetVertexShader(),LightPosition,LightSceneInfo->GetPosition());
	}

	/**
	 * Sets the shader parameters that are different between shadow volumes.
	 *
	 * @param Context			RHI Context (will always be NULL)
	 * @param InLocalToWorld	Transformation matrix from local space to world space
	 * @param InBaseExtrusion	Amount to extrude front-facing triangles to handle z-fighting in some cases (usually 0.0f)
	 */
	void SetInstanceParameters(FCommandContextRHI* Context,const FMatrix& InLocalToWorld,FLOAT InBaseExtrusion)
	{
		// Set a small extrusion for front-facing polygons.
		SetVertexShaderValue(Context,GetVertexShader(),BaseExtrusion,InBaseExtrusion);

		// Set the local to world matrix.
		SetVertexShaderValue(Context,GetVertexShader(),LocalToWorld,InLocalToWorld);
	}

	/**
	 * Serialize the vertex shader.
	 *
	 * @param Ar	Archive to serialize to/from
	 */
	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << LightPosition;
		Ar << BaseExtrusion;
		Ar << LocalToWorld;
	}

	virtual EShaderRecompileGroup GetRecompileGroup() const
	{
		return SRG_GLOBAL_MISC_SHADOW;
	}


private:
	/** Represents the shader registers for the homogenous world space position of light */
	FShaderParameter	LightPosition;

	/** Represents the shader registers for the amount to extrude front-facing triangles */
	FShaderParameter	BaseExtrusion;

	/** Represents the shader registers for the matrix that transforms from local space to world space */
	FShaderParameter	LocalToWorld;
};

IMPLEMENT_SHADER_TYPE(,FShadowVolumeVertexShader,TEXT("ShadowVolumeVertexShader"),TEXT("Main"),SF_Vertex,0,0);


/**
 * Helper class that is used by FSceneRenderer::RenderShadowVolumes and FPrimitiveSceneProxy.
 */
class FMyShadowVolumeDrawInterface : public FShadowVolumeDrawInterface
{
public:
	/**
	 * Constructor
	 *
	 * @param InLightSceneInfo	Represents the current light
	 */
	FMyShadowVolumeDrawInterface( FCommandContextRHI* InContext, const FLightSceneInfo* InLightSceneInfo,UINT InDepthPriorityGroup ):
		ShadowVolumeVertexShader(GetGlobalShaderMap()),
		View(NULL),
		LightSceneInfo(InLightSceneInfo),
		DepthPriorityGroup(InDepthPriorityGroup),
		bDirty(FALSE),
		Context(InContext)
	{
	}

	/** Destructor */
	~FMyShadowVolumeDrawInterface()
	{
		// Restore states if necessary.
		if ( bDirty )
		{
			RHISetDepthState(Context,TStaticDepthState<>::GetRHI());
			RHISetColorWriteEnable(Context,TRUE);
			RHISetStencilState(Context,TStaticStencilState<>::GetRHI());
			RHISetRasterizerState(Context,TStaticRasterizerState<>::GetRHI());
		}
		GSceneRenderTargets.FinishRenderingShadowVolumes();
	}

	FCommandContextRHI* GetContext() { return Context; }

	/**
	 * Returns whether the render target has been modified or not; that is, if any
	 * shadow volumes got rendered.
	 *
	 * @return	TRUE if any shadow volume was rendered
	 */
	UBOOL IsDirty( )
	{
		return bDirty;
	}

	/**
	 * FPrimitiveSceneProxy will call back to the virtual this->DrawShadowVolume().
	 *
	 * @param Interaction	Represents the current view
	 */
	void SetView( const FViewInfo *InView )
	{
		View = InView;

		GSceneRenderTargets.BeginRenderingShadowVolumes();

		// Set the device viewport for the view.
		RHISetViewport(Context,View->RenderTargetX,View->RenderTargetY,0.0f,View->RenderTargetX + View->RenderTargetSizeX,View->RenderTargetY + View->RenderTargetSizeY,1.0f);
		RHISetViewParameters( Context, View, View->ViewProjectionMatrix, View->ViewOrigin );

		// Set the light's scissor rectangle.
		LightSceneInfo->SetScissorRect(Context, View);
	}

	/**
	 * Called by FSceneRenderer::RenderShadowVolumes() for each primitive affected by the light.
	 *
	 * @param Interaction	Represents the light to draw shadows from
	 */
	void DrawShadowVolume( FLightPrimitiveInteraction *Interaction )
	{
		FPrimitiveSceneInfo *PrimitiveSceneInfo = Interaction->GetPrimitiveSceneInfo();

		// Compute the primitive's view relevance.  Note that the view won't necessarily have it cached,
		// since the primitive might not be visible.
		FPrimitiveViewRelevance ViewRelevance = PrimitiveSceneInfo->Proxy->GetViewRelevance(View);

		// Only draw shadow volumes for primitives which are relevant in this view and DPG.
		if(ViewRelevance.IsRelevant() && ViewRelevance.GetDPG(DepthPriorityGroup))
		{		
			// Whether the shadow volume has been rendered or not.
			UBOOL bShadowVolumeWasRendered = FALSE;

			// Only render shadow volumes for lights above a certain threshold.
			const FLightSceneInfo* TheLightSceneInfo = Interaction->GetLight();
			if( TheLightSceneInfo->GetRadius() == 0.f 
			||  TheLightSceneInfo->GetRadius() > GEngine->ShadowVolumeLightRadiusThreshold )
			{
				FVector4 ScreenPosition = View->WorldToScreen(PrimitiveSceneInfo->Bounds.Origin);

				// Determine screen space percentage of bounding sphere.
				FLOAT ScreenPercentage = Max(
					1.0f / 2.0f * View->ProjectionMatrix.M[0][0],
					1.0f / 2.0f * View->ProjectionMatrix.M[1][1]
				) 
				*	PrimitiveSceneInfo->Bounds.SphereRadius
				/	Max(Abs(ScreenPosition.W),1.0f);

				// Only draw shadow volumes for primitives with bounds covering more than a specific percentage of the screen.
				if( ScreenPercentage > GEngine->ShadowVolumePrimitiveScreenSpacePercentageThreshold )
				{
					PrimitiveSceneInfo->Proxy->DrawShadowVolumes( this, View, TheLightSceneInfo, DepthPriorityGroup );
					bShadowVolumeWasRendered = TRUE;
				}
			}

			// Remove cached data if it hasn't been used to render this view. This is only done in the Editor to avoid
			// running out of memory. The code doesn't handle splitscreen or multiple perspective windows.
			if( GIsEditor && !bShadowVolumeWasRendered )
			{
				PrimitiveSceneInfo->Proxy->RemoveCachedShadowVolumeData( TheLightSceneInfo );
			}
		}
	}

	/**
	  * Called by FPrimitiveSceneProxy to render shadow volumes to the stencil buffer.
	  *
	  * @param IndexBuffer Shadow volume index buffer, indexing into the original vertex buffer
	  * @param VertexFactory Vertex factory that represents the shadow volume vertex format
	  * @param LocalToWorld World matrix for the primitive
	  * @param FirstIndex First index to use in the index buffer
	  * @param NumPrimitives Number of triangles in the triangle list
	  * @param MinVertexIndex Lowest index used in the index buffer
	  * @param MaxVertexIndex Highest index used in the index buffer
	  */
	virtual void DrawShadowVolume(
		FIndexBufferRHIParamRef IndexBuffer,
		const FLocalShadowVertexFactory& VertexFactory,
		const FMatrix& LocalToWorld,
		UINT FirstIndex,
		UINT NumPrimitives,
		UINT MinVertexIndex,
		UINT MaxVertexIndex
		)
	{
		// Set states if necessary.
		if ( !bDirty )
		{
			RHISetDepthState(Context,TStaticDepthState<FALSE,CF_Less>::GetRHI());
			RHISetRasterizerState(Context,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
			ShadowVolumeVertexShader->SetParameters(Context, View, LightSceneInfo);
			bDirty = TRUE;
		}

		// Check the determinant of the LocalToWorld transform to determine the winding of the shadow volume.
		const FLOAT LocalToWorldDeterminant = LocalToWorld.Determinant();
		if(IsNegativeFloat(LocalToWorldDeterminant))
		{
			RHISetStencilState(Context,TStaticStencilState<TRUE,CF_Always,SO_Keep,SO_Increment,SO_Keep,TRUE,CF_Always,SO_Keep,SO_Decrement,SO_Keep>::GetRHI());
		}
		else
		{
			RHISetStencilState(Context,TStaticStencilState<TRUE,CF_Always,SO_Keep,SO_Decrement,SO_Keep,TRUE,CF_Always,SO_Keep,SO_Increment,SO_Keep>::GetRHI());
		}

		ShadowVolumeVertexShader->SetInstanceParameters(Context, LocalToWorld, VertexFactory.GetBaseExtrusion());
		
		VertexFactory.Set(Context);

		if (!IsValidRef(ShadowVolumeBoundShaderState))
		{
			DWORD Strides[MaxVertexElementCount];
			VertexFactory.GetStreamStrides(Strides);

			ShadowVolumeBoundShaderState = RHICreateBoundShaderState(VertexFactory.GetDeclaration(), Strides, ShadowVolumeVertexShader->GetVertexShader(), FPixelShaderRHIRef());
		}
		
		RHISetBoundShaderState(Context, ShadowVolumeBoundShaderState);

		RHIDrawIndexedPrimitive(Context, IndexBuffer, PT_TriangleList, 0, MinVertexIndex, MaxVertexIndex-MinVertexIndex, FirstIndex, NumPrimitives);
	}

private:
	/** Vertex shader that extrudes the shadow volume. */
	TShaderMapRef<FShadowVolumeVertexShader> ShadowVolumeVertexShader;
	/** bound shader state for volume rendering */
	static FBoundShaderStateRHIRef ShadowVolumeBoundShaderState;

	/** Current view. */
	const FViewInfo *View;

	/** Current light. */
	const FLightSceneInfo *LightSceneInfo;

	/** Current DPG. */
	BITFIELD DepthPriorityGroup : UCONST_SDPG_NumBits;

	/** Flag that tells whether any shadow volumes got rendered to the render target. */
	UBOOL bDirty;

	/** Context to use for rendering */
	FCommandContextRHI* Context;
};

FBoundShaderStateRHIRef FMyShadowVolumeDrawInterface::ShadowVolumeBoundShaderState;

/**
* Used by RenderLights to figure out if shadow volumes need to be rendered to the stencil buffer.
*
* @param LightSceneInfo Represents the current light
* @return TRUE if anything needs to be rendered
*/
UBOOL FSceneRenderer::CheckForShadowVolumes( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex )
{
	UBOOL bResult=FALSE;
	if( UEngine::ShadowVolumesAllowed() )
	{
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			const FViewInfo& View = Views(ViewIndex);
			const FVisibleLightInfo& VisibleLightInfo = View.VisibleLightInfos(LightSceneInfo->Id);
			const FVisibleLightInfo::FDPGInfo& VisibleLightDPGInfo = VisibleLightInfo.DPGInfo[DPGIndex];
			// shadow volume is rendered if any lit primitives affected by this light are visible
			// or if it is using modulated shadows and is visible in the view frustum
			if( VisibleLightDPGInfo.bHasVisibleLitPrimitives ||
				(LightSceneInfo->LightShadowMode == LightShadow_Modulate && VisibleLightInfo.bInViewFrustum) )
			{
				if (LightSceneInfo->NumShadowVolumeInteractions > 0)
				{
					bResult = TRUE;
					break;
				}
			}
		}
	}
	return bResult;
}

/**
* Used by RenderLights to figure out if shadow volumes need to be rendered to the attenuation buffer.
*
* @param LightSceneInfo Represents the current light
* @return TRUE if anything needs to be rendered
*/
UBOOL FSceneRenderer::CheckForShadowVolumeAttenuation( const FLightSceneInfo* LightSceneInfo )
{
	return (LightSceneInfo->LightShadowMode == LightShadow_Modulate);
}

/**
 * Used by FSceneRenderer::RenderLights to render shadow volumes to the attenuation buffer.
 *
 * @param LightSceneInfo Represents the current light
 * @return TRUE if anything got rendered
 */
UBOOL FSceneRenderer::RenderShadowVolumes( const FLightSceneInfo* LightSceneInfo, UINT DPGIndex )
{
	SCOPED_DRAW_EVENT(EventShadowVolume)(DEC_SCENE_ITEMS,TEXT("Shadow volume"));
	SCOPE_CYCLE_COUNTER(STAT_ShadowVolumeDrawTime);

	UBOOL bResult=FALSE;
	if( UEngine::ShadowVolumesAllowed() )
	{
		// Render to the stencil buffer for all views.
		FMyShadowVolumeDrawInterface SVDI( GlobalContext, LightSceneInfo, DPGIndex );
		for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

			const FViewInfo& View = Views(ViewIndex);
			const FVisibleLightInfo& VisibleLightInfo = View.VisibleLightInfos(LightSceneInfo->Id);
			const FVisibleLightInfo::FDPGInfo& VisibleLightDPGInfo = VisibleLightInfo.DPGInfo[DPGIndex];
			// shadow volume is rendered if any lit primitives affected by this light are visible
			// or if it is using modulated shadows and is visible in the view frustum
			if( VisibleLightDPGInfo.bHasVisibleLitPrimitives ||
				(LightSceneInfo->LightShadowMode == LightShadow_Modulate && VisibleLightInfo.bInViewFrustum) )
			{
				SVDI.SetView( &View );

				// used to validate the accuracy of NumShadowVolumeInteractions
				UINT ValidateCount = 0;
				// Iterate over all primitives that interacts with this light.
				FLightPrimitiveInteraction *Interaction = LightSceneInfo->DynamicPrimitiveList;
				while ( Interaction )
				{
					if ( Interaction->GetDynamicShadowType() == DST_Volume )
					{
#if STATS
						if( bShouldGatherDynamicShadowStats )
						{
							FCombinedShadowStats ShadowStat;
							ShadowStat.SubjectPrimitives.AddItem( Interaction->GetPrimitiveSceneInfo() );
							InteractionToDynamicShadowStatsMap.Set( Interaction, ShadowStat );
						}
#endif
						++ ValidateCount;
						SVDI.DrawShadowVolume( Interaction );
					}
					Interaction = Interaction->GetNextPrimitive();
				}			
				check(ValidateCount == LightSceneInfo->NumShadowVolumeInteractions);
			}
		}
		bResult = SVDI.IsDirty();
	}
	return bResult;	
}

/*-----------------------------------------------------------------------------
FModShadowVolumeVertexShader
-----------------------------------------------------------------------------*/

/**
* Constructor - binds all shader params
* @param Initializer - init data from shader compiler
*/
FModShadowVolumeVertexShader::FModShadowVolumeVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
:	FGlobalShader(Initializer)
{		
}

/**
* @param Platform - current platform being compiled
* @return TRUE if this global shader should be compiled
*/
UBOOL FModShadowVolumeVertexShader::ShouldCache(EShaderPlatform Platform)
{
	return TRUE;
}

/**
* Sets the current vertex shader
* @param Context - command buffer context
* @param View - current view
* @param ShadowInfo - projected shadow info for a single light
*/
void FModShadowVolumeVertexShader::SetParameters(
									   FCommandContextRHI* Context,
									   const FSceneView* View,
									   const FLightSceneInfo* LightSceneInfo
									   )
{
}

/**
* Serialize the parameters for this shader
* @param Ar - archive to serialize to
*/
void FModShadowVolumeVertexShader::Serialize(FArchive& Ar)
{
	FShader::Serialize(Ar);
}

IMPLEMENT_SHADER_TYPE(,FModShadowVolumeVertexShader,TEXT("ModShadowVolumeVertexShader"),TEXT("Main"),SF_Vertex,0,0);

/*-----------------------------------------------------------------------------
FModShadowVolumePixelShader
-----------------------------------------------------------------------------*/

/**
* Constructor - binds all shader params
* @param Initializer - init data from shader compiler
*/
FModShadowVolumePixelShader::FModShadowVolumePixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
:	FGlobalShader(Initializer)
{
	SceneTextureParams.Bind(Initializer.ParameterMap);
	ShadowModulateColorParam.Bind(Initializer.ParameterMap,TEXT("ShadowModulateColor"));
	ScreenToWorldParam.Bind(Initializer.ParameterMap,TEXT("ScreenToWorld"),TRUE);
}

/**
* @param Platform - current platform being compiled
* @return TRUE if this global shader should be compiled
*/
UBOOL FModShadowVolumePixelShader::ShouldCache(EShaderPlatform Platform)
{
	return TRUE;
}

/**
* Sets the current pixel shader
* @param Context - command buffer context
* @param View - current view
* @param ShadowInfo - projected shadow info for a single light
*/
void FModShadowVolumePixelShader::SetParameters(
									  FCommandContextRHI* Context,
									  const FSceneView* View,
									  const FLightSceneInfo* LightSceneInfo
									  )
{
	SceneTextureParams.Set(Context,View,this);

	// color to modulate shadowed areas on screen
	SetPixelShaderValue( Context, GetPixelShader(), ShadowModulateColorParam, LightSceneInfo->ModShadowColor );
	// screen space to world space transform
	FMatrix ScreenToWorld = FMatrix(
		FPlane(1,0,0,0),
		FPlane(0,1,0,0),
		FPlane(0,0,(1.0f - Z_PRECISION),1),
		FPlane(0,0,-View->NearClippingDistance * (1.0f - Z_PRECISION),0)
		) * 
		View->InvViewProjectionMatrix;
	SetPixelShaderValue( Context, GetPixelShader(), ScreenToWorldParam, ScreenToWorld );

}

/**
* Serialize the parameters for this shader
* @param Ar - archive to serialize to
*/
void FModShadowVolumePixelShader::Serialize(FArchive& Ar)
{
	FShader::Serialize(Ar);
	Ar << SceneTextureParams;
	Ar << ShadowModulateColorParam;
	Ar << ScreenToWorldParam;
}

/**
* Attenuate the shadowed area of a shadow volume. For use with modulated shadows
* @param LightSceneInfo - Represents the current light
* @return TRUE if anything got rendered
*/
UBOOL FSceneRenderer::AttenuateShadowVolumes( const FLightSceneInfo* LightSceneInfo )
{
	SCOPE_CYCLE_COUNTER(STAT_LightingDrawTime);
	SCOPED_DRAW_EVENT(EventRenderModShadowVolume)(DEC_SCENE_ITEMS,TEXT("ModShadowVolume"));

	UBOOL bDirty = FALSE;
	for(INT ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		SCOPED_DRAW_EVENT(EventView)(DEC_SCENE_ITEMS,TEXT("View%d"),ViewIndex);

		const FViewInfo* View = &Views(ViewIndex);

		// Set the device viewport for the view.
		RHISetViewport(GlobalContext,View->RenderTargetX,View->RenderTargetY,0.0f,View->RenderTargetX + View->RenderTargetSizeX,View->RenderTargetY + View->RenderTargetSizeY,1.0f);
		RHISetViewParameters( GlobalContext, View, FMatrix::Identity, View->ViewOrigin );

		// Set the light's scissor rectangle.
		LightSceneInfo->SetScissorRect(GlobalContext,View);
		// set stencil to only render for the shadow volume fragments
		RHISetStencilState(GlobalContext,TStaticStencilState<TRUE,CF_NotEqual>::GetRHI());
		// set fill/blend state
		RHISetRasterizerState(GlobalContext,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());

		if (CanBlendWithFPRenderTarget(GRHIShaderPlatform))
		{
			// modulated blending, preserve alpha
			RHISetBlendState(GlobalContext,TStaticBlendState<BO_Add,BF_DestColor,BF_Zero,BO_Add,BF_Zero,BF_One>::GetRHI());	
		}
		else
		{
			//opaque, blending will be done in the shader
			RHISetBlendState(GlobalContext,TStaticBlendState<>::GetRHI());	
		}

		// turn off depth reads/writes
		RHISetDepthState(GlobalContext,TStaticDepthState<FALSE,CF_Always>::GetRHI());

		// Set the shader state.
		TShaderMapRef<FModShadowVolumeVertexShader> VertexShader(GetGlobalShaderMap());
		VertexShader->SetParameters(GlobalContext,View,LightSceneInfo);

		FModShadowVolumePixelShader* PixelShader = LightSceneInfo->GetModShadowVolumeShader();
		check(PixelShader);
		PixelShader->SetParameters(GlobalContext,View,LightSceneInfo);

		RHISetBoundShaderState(GlobalContext,LightSceneInfo->GetModShadowVolumeBoundShaderState());

		// Draw a quad for the view.
		const UINT BufferSizeX = GSceneRenderTargets.GetBufferSizeX();
		const UINT BufferSizeY = GSceneRenderTargets.GetBufferSizeY();
		DrawDenormalizedQuad(
			GlobalContext,
			0,0,
			View->RenderTargetSizeX,View->RenderTargetSizeY,
			View->RenderTargetX,View->RenderTargetY,
			View->RenderTargetSizeX,View->RenderTargetSizeY,
			View->RenderTargetSizeX,View->RenderTargetSizeY,
			BufferSizeX,BufferSizeY
			);

		// Reset the scissor rectangle and stencil state.
		RHISetScissorRect(GlobalContext,FALSE,0,0,0,0);
		RHISetDepthBoundsTest(GlobalContext, FALSE, 0.0f, 1.0f);
		RHISetStencilState(GlobalContext,TStaticStencilState<>::GetRHI());

		bDirty = TRUE;
	}

	return bDirty;
}

