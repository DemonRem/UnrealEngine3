/*=============================================================================
	SceneRenderTargets.cpp: Scene render target implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"

#define NO_STEREO_D3D9 1
#define NO_STEREO_D3D11 1
#include "ue3stereo.h"
#undef NO_STEREO_D3D9
#undef NO_STEREO_D3D11

/*-----------------------------------------------------------------------------
FSceneRenderTargets
-----------------------------------------------------------------------------*/

/** Resolution for the texture pool visualizer texture. */
enum
{
	TexturePoolVisualizerSizeX = 280,
	TexturePoolVisualizerSizeY = 140,
};

// 0=off, >0=texture id, changed by "VisualizeTexture" console command, useful for debugging
extern INT GVisualizeTexture;


/** The global render targets used for scene rendering. */
TGlobalResource<FSceneRenderTargets> GSceneRenderTargets;

/**
 * Returns a NULL target if we're not running in stereo. If running in stereo
 * return the specified altBuffer. altBuffer may not be the correct size, and
 * so could trigger warnings or errors in the d3d runtime. However, by providing
 * a buffer that will be unused, we're cueiing the stereo driver that this call
 * should be stereoized.
 */
FSurfaceRHIRef StereoizedDrawNullTarget(const FSurfaceRHIRef& altBuffer) 
{
	return nv::stereo::IsStereoEnabled() ? altBuffer : FSurfaceRHIRef();
}

#if PS3 && !USE_NULL_RHI
/**
 * Simple container for a frame buffer instance, used in DumpMemoryUsage
 */
struct FBufferInstance
{
	FString Name;
	INT Offset;
	INT Region;
	INT Size;
	INT Tiled;

	static UBOOL CreateFromSurface(/*out*/ FBufferInstance& Result, const TCHAR* Name, const FSurfaceRHIRef& Surface)
	{
		if (IsValidRef(Surface) && (Surface->AllocatedSize > 0))
		{
			Result.Name = FString::Printf(TEXT("'         %-36ls'"), Name);
			Result.Offset = Surface->MemoryOffset;
			Result.Region = Surface->MemoryRegion;
			Result.Size = Surface->AllocatedSize;
			Result.Tiled = Surface->bUsesTiledRegister;

			return TRUE;
		}

		return FALSE;
	}

	static void CreateFromResource(/*out*/ FBufferInstance& Result, const TCHAR* Name, const FPS3RHIGPUResource* Mem)
	{
		Result.Name = FString::Printf(TEXT("'         %-36ls'"), Name);
		Result.Offset = Mem->MemoryOffset;
		Result.Region = Mem->MemoryRegion;
		Result.Size = Mem->AllocatedSize;
		Result.Tiled = Mem->bUsesTiledRegister;
	}

	static UBOOL CreateFromResolveSurface(/*out*/ FBufferInstance& Result, const TCHAR* Name, const FSurfaceRHIRef& Surface)
	{
		if (IsValidRef(Surface) && (Surface->ResolveTargetMemoryOffset != Surface->MemoryOffset) && (Surface->AllocatedSize > 0))
		{
			Result.Name = FString::Printf(TEXT("'Resolved %-36ls'"), Name);
			Result.Offset = Surface->ResolveTargetMemoryOffset;
			Result.Region = Surface->MemoryRegion;
			Result.Size = Surface->AllocatedSize;
			Result.Tiled = Surface->bUsesTiledRegister;
			return TRUE;
		}

		return FALSE;
	}
};
#endif

IMPLEMENT_COMPARE_CONSTREF( INT, SceneRenderTargets, { return A - B; } )

/** 
 * Dumps information about render target memory usage
 * Must be called on the rendering thread or while the rendering thread is blocked
 * Currently implemented for PS3 or 360
 * @return the amount of memory all of the RenderTargets are utilizing
 */
INT FSceneRenderTargets::DumpMemoryUsage(FOutputDevice& Ar) const
{
	if (!IsInitialized())
	{
		return 0;
	}

#if (PS3 || XBOX) && !FINAL_RELEASE && !USE_NULL_RHI
	Ar.Logf(TEXT("Listing scene render targets."));
#if XBOX
	INT TotalPhysicalSize = 0;
	TMap<INT, TArray<INT> > UniqueMainMemoryTextures;
	TMap<INT, TArray<INT> > UniqueEDRAMSurfaces;
	for (INT RTIdx = 0; RTIdx < MAX_SCENE_RENDERTARGETS; RTIdx++)
	{
		// Ignore unused textures
		if (IsValidRef(RenderTargets[RTIdx].Texture))
		{
			INT OverlappingTextureIndex = -1;
			// Search for an already reported texture sharing the same memory
			for (INT RTIdx2 = 0; RTIdx2 < RTIdx; RTIdx2++)
			{
				if (RTIdx != RTIdx2 
					// Check if texture resources are equal
					&& (RenderTargets[RTIdx].Texture == RenderTargets[RTIdx2].Texture 
					// Check if texture resources are different but they share memory through the RHICreateSharedTexture method
					|| IsValidRef(RenderTargets[RTIdx].Texture) && IsValidRef(RenderTargets[RTIdx2].Texture)
					&& RenderTargets[RTIdx].Texture->GetSharedTexture() && RenderTargets[RTIdx2].Texture->GetSharedTexture()
					&& RenderTargets[RTIdx].Texture->GetSharedTexture()->GetSharedMemory()->BaseAddress == RenderTargets[RTIdx2].Texture->GetSharedTexture()->GetSharedMemory()->BaseAddress))
				{
					OverlappingTextureIndex = RTIdx2;
					break;
				}
			}

			if (OverlappingTextureIndex >= 0)
			{
				TArray<INT>* ExistingMainMemoryTextureInfo = UniqueMainMemoryTextures.Find(OverlappingTextureIndex);
				check(ExistingMainMemoryTextureInfo);
				ExistingMainMemoryTextureInfo->AddUniqueItem(RTIdx);
			}
			else
			{
				TArray<INT> NewTextures;
				NewTextures.AddItem(RTIdx);
				UniqueMainMemoryTextures.Set(RTIdx, NewTextures);
			}

			INT EDRAMUsage = 0;
			INT EDRAMOffset = 0;
			if (IsValidRef(RenderTargets[RTIdx].Surface))
			{
				EDRAMUsage = RenderTargets[RTIdx].Surface.XeSurfaceInfo.GetSize();
				EDRAMOffset = RenderTargets[RTIdx].Surface.XeSurfaceInfo.GetOffset();
			}

			TArray<INT>* ExistingEDRAMSurfaces = UniqueEDRAMSurfaces.Find(EDRAMOffset);
			if (!ExistingEDRAMSurfaces)
			{
				TArray<INT> NewSurfaces;
				NewSurfaces.AddItem(RTIdx);
				ExistingEDRAMSurfaces = &UniqueEDRAMSurfaces.Set(EDRAMOffset, NewSurfaces);
			}
			check(ExistingEDRAMSurfaces);
			ExistingEDRAMSurfaces->AddUniqueItem(RTIdx);

			if (OverlappingTextureIndex >= 0)
			{
				Ar.Logf(TEXT("	RenderTarget %2i %s sharing memory with %s, %d EDRAM memory tiles, %d EDRAM offset"), 
					RTIdx, 
					*GetRenderTargetName((ESceneRenderTargetTypes)RTIdx), 
					*GetRenderTargetName((ESceneRenderTargetTypes)OverlappingTextureIndex),
					EDRAMUsage,
					EDRAMOffset);
			}
			else
			{
				const INT PhysicalResourceSize = RenderTargets[RTIdx].Texture->PhysicalSize;
				TotalPhysicalSize += PhysicalResourceSize;

				// for RenderTargets that exist print out how many EDRAM tiles they utilize
				Ar.Logf(TEXT("	RenderTarget %2i %s using %.2fMb Physical memory, %d EDRAM memory tiles, %d EDRAM offset"), 
					RTIdx, 
					*GetRenderTargetName((ESceneRenderTargetTypes)RTIdx), 
					PhysicalResourceSize / (1024.0f * 1024.0f),
					EDRAMUsage,
					EDRAMOffset
					);
			}
		}
	}

	Ar.Logf(TEXT("Total rendertarget memory: %.2fMb Physical"),  
		TotalPhysicalSize / (1024.0f * 1024.0f));

	Ar.Logf(TEXT(""));
	Ar.Logf(TEXT("Sorted by unique main memory texture allocations:"));
	UniqueMainMemoryTextures.KeySort<COMPARE_CONSTREF_CLASS(INT, SceneRenderTargets)>();
	for (TMap<INT, TArray<INT> >::TIterator It(UniqueMainMemoryTextures); It; ++It)
	{
		INT MaxResourceSize = 0;
		FString RenderTargetNames;
		for (INT i = 0; i < It.Value().Num(); i++)
		{
			const INT RenderTargetIndex = It.Value()(i);
			MaxResourceSize = Max(MaxResourceSize, RenderTargets[RenderTargetIndex].Texture->PhysicalSize);
			const UBOOL bIsLastEntry = i == It.Value().Num() - 1;
			RenderTargetNames += GetRenderTargetName((ESceneRenderTargetTypes)RenderTargetIndex) + (bIsLastEntry ? TEXT("") : TEXT(", "));
		}
		
		Ar.Logf(TEXT("	%5.2fMb Physical memory shared by %s"), MaxResourceSize / (1024.0f * 1024.0f), *RenderTargetNames);
	}

	Ar.Logf(TEXT(""));
	Ar.Logf(TEXT("Sorted by EDRAM offset:"));
	UniqueEDRAMSurfaces.KeySort<COMPARE_CONSTREF_CLASS(INT, SceneRenderTargets)>();
	for (TMap<INT, TArray<INT> >::TIterator It(UniqueEDRAMSurfaces); It; ++It)
	{
		INT GroupEDRAMOffset = 0;
		FString SurfaceNames;
		const TArray<INT>& SurfaceIndices = It.Value();
		for (INT i = 0; i < SurfaceIndices.Num(); i++)
		{
			const INT RenderTargetIndex = SurfaceIndices(i);
			INT EDRAMUsage = 0;
			if (IsValidRef(RenderTargets[RenderTargetIndex].Surface))
			{
				EDRAMUsage = RenderTargets[RenderTargetIndex].Surface.XeSurfaceInfo.GetSize();
				GroupEDRAMOffset = RenderTargets[RenderTargetIndex].Surface.XeSurfaceInfo.GetOffset();
			}
			const UBOOL bIsLastEntry = i == It.Value().Num() - 1;
			SurfaceNames += FString::Printf(TEXT("%s using %d tiles%s"), 
				*GetRenderTargetName((ESceneRenderTargetTypes)RenderTargetIndex), 
				EDRAMUsage,
				bIsLastEntry ? TEXT("") : TEXT(", "));
		}

		Ar.Logf(TEXT("	Offset %4d: %s"), GroupEDRAMOffset , *SurfaceNames);
	}
	Ar.Logf(TEXT(""));

	return TotalPhysicalSize;
#elif PS3
	// Gather all of the offsets and sizes
	TArray<FBufferInstance> Buffers;
	FBufferInstance Buffer;
	
	// Back buffers are tracked separately
	for (INT FrameIdx = 0; FrameIdx < 2; ++FrameIdx)
	{
		ensure(FBufferInstance::CreateFromSurface(Buffer, *FString::Printf(TEXT("Backbuffer%d"), FrameIdx), GPS3Gcm->GetColorBuffer(FrameIdx)));
		Buffers.AddItem(Buffer);
	}

	// Add all the scene render targets
	for( INT RTIdx=0; RTIdx < MAX_SCENE_RENDERTARGETS; RTIdx++ )
	{
		// Ignore unused textures
		if (IsValidRef(RenderTargets[RTIdx].Texture))
		{
			FString Name = GetRenderTargetName((ESceneRenderTargetTypes)RTIdx);
			if (FBufferInstance::CreateFromSurface(Buffer, *Name, RenderTargets[RTIdx].Surface))
			{
				Buffers.AddItem(Buffer);
			}
			else
			{
				if (RenderTargets[RTIdx].Texture->AllocatedSize > 0)
				{
					Ar.Logf(TEXT("Warning: RenderTargets[%i] named %s has a valid texture but no surface (this probably means it was allocated but never used)"), RTIdx, *Name);
					FBufferInstance::CreateFromResource(Buffer, *Name, RenderTargets[RTIdx].Texture);
					Buffers.AddItem(Buffer);
				}
			}

			if (FBufferInstance::CreateFromResolveSurface(Buffer, *Name, RenderTargets[RTIdx].Surface))
			{
				Buffers.AddItem(Buffer);
			}
		}
	}

	// Now loop over them and print out non-duplicates
	Ar.Logf(TEXT("Memory usage for SceneRenderTarget targets and the frame buffers:"));
	INT TotalSize = 0;
	for (INT i = 0; i < Buffers.Num(); ++i)
	{
		FBufferInstance& CurrentBuffer = Buffers(i);

		INT DuplicateId = -1;
		for (INT j = 0; j < i; ++j)
		{
			if (Buffers(j).Offset == CurrentBuffer.Offset)
			{
				DuplicateId = j;
				break;
			}
		}

		if (DuplicateId >= 0)
		{
			Ar.Logf(TEXT("	%5.2fMb - %s is sharing memory with %s"),
				0.0f,
				*CurrentBuffer.Name, 
				*Buffers(DuplicateId).Name);
		}
		else
		{
			Ar.Logf(TEXT("	%5.2fMb - %s (offset: 0x%7X, region: %d%s)"), 
				CurrentBuffer.Size / (1024.0f * 1024.0f),
			    *CurrentBuffer.Name,
				CurrentBuffer.Offset,
				CurrentBuffer.Region,
				CurrentBuffer.Tiled ? TEXT(", Tiled") : TEXT(""));
			TotalSize += CurrentBuffer.Size;
		}
	}
 
	// Only SceneRenderTargets and back buffers are accounted for above; find out the sum total of memory used
	// for render targets and print out the 'unaccounted' for memory.
	extern void UpdateMemStats();
	UpdateMemStats();
	INT TotalRenderTargetMemory = GStatManager.GetStatValueDWORD(STAT_LocalRenderTargetSize);

	// Display the totals
	Ar.Logf(TEXT("Total SceneRenderTarget memory: %.2fMb"), TotalSize / (1024.0f * 1024.0f));
	Ar.Logf(TEXT("Total GCM RenderTarget memory:  %.2fMb"), TotalRenderTargetMemory / (1024.0f * 1024.0f));
	Ar.Logf(TEXT("Non-SceneRenderTarget memory:   %.2fMb"), (TotalRenderTargetMemory - TotalSize) / (1024.0f * 1024.0f));

	return TotalSize;
#endif
#endif // #if (PS3 || XBOX) && !FINAL_RELEASE && !USE_NULL_RHI

#if !FINAL_RELEASE && (USE_NULL_RHI || (!PS3 && !XBOX))
	// Still output something (knowing no information was gathered is still useful)
	Ar.Logf(TEXT("Listing scene render targets."));
	Ar.Logf(TEXT("Note: DumpMemoryUsage command not supported for this RHI"));
#endif

	return 0;
}

void FSceneRenderTargets::Allocate(UINT MinSizeX,UINT MinSizeY)
{
	check(IsInRenderingThread());

#if CONSOLE
	// force to always use the global screen sizes to avoid reallocating the scene buffers
	MinSizeX = GScreenWidth;
	MinSizeY = GScreenHeight;
#endif

	if(BufferSizeX < MinSizeX || BufferSizeY < MinSizeY)
	{
		// Reinitialize the render targets for the given size.
		SetBufferSize( Max(BufferSizeX,MinSizeX), Max(BufferSizeY,MinSizeY) );

		UpdateRHI();
	}
}

/**
 * Update the rendertarget based on the specified usage flags.
 * @param RenderTargetUsage		Bit-wise combination of FSceneRenderTargetUsage flags.
 */
void FSceneRenderTargets::UpdateRenderTargetUsage( FSurfaceRHIParamRef SurfaceRHI, DWORD RenderTargetUsage )
{
#if USE_PS3_RHI
	// Are we going to fully overwrite the entire rendertarget with a full-screen quad?
	if ( RenderTargetUsage & RTUsage_FullOverwrite )
	{
		// Use dual-buffering, to allow distortion effects.
		if ( RenderTargetUsage & RTUsage_DontSwapBuffer )
		{
			// Keep current buffer as they are.
			SurfaceRHI->DualBufferWithResolveTarget();
		}
		else
		{
			// Ping-pong surface and resolve-target.
			SurfaceRHI->SwapWithResolveTarget();
		}
	}
	else
	{
		// Make the resolvetarget point to the same memory as the surface.
		SurfaceRHI->UnifyWithResolveTarget();
	}
#endif
#if NGP
	extern void SetResolveDepthBuffer( UBOOL bResolveDepth );
	SetResolveDepthBuffer( (RenderTargetUsage & RTUsage_ResolveDepth) ? TRUE : FALSE );
#endif
}

/**
 * Sets the current backbuffer for this frame. This needs to be called in the beginning of every frame
 * when backbuffers are swapped.
 * @param InBackBuffer	- Backbuffer to use for this frame.
 **/
void FSceneRenderTargets::SetBackBuffer( FSurfaceRHIParamRef InBackBuffer )
{
	BackBuffer = InBackBuffer;

#if NGP
	if (InBackBuffer)
	{
		if ( GMobileUsePostProcess )
		{
			RenderTargets[LightAttenuation0].Surface = BackBuffer;
			RenderTargets[LightAttenuation0].Texture = RHIGetResolveTarget( BackBuffer );
		}
		else
		{
			RenderTargets[SceneColor].Surface = BackBuffer;
			RenderTargets[SceneColor].Texture = RHIGetResolveTarget( BackBuffer );
		}
	}
#endif

#if PS3
	if (InBackBuffer)
	{
		// Create a texture to store the resolved light attenuation values, and a render-targetable surface to hold the unresolved light attenuation values.
		RenderTargets[LightAttenuation0].Surface = BackBuffer;
		RenderTargets[LightAttenuation0].Texture = RHIGetResolveTarget( BackBuffer );

		// Reuse the light attenuation texture/surface for hit proxies.
		RenderTargets[HitProxy] = RenderTargets[LightAttenuation0];
	}
#endif
}

/**
 * Sets the backbuffer as the current rendertarget.
 * @param RenderTargetUsage	- Bit-wise combination of FSceneRenderTargetUsage flags.
 */
void FSceneRenderTargets::BeginRenderingBackBuffer( DWORD RenderTargetUsage /*= RTUsage_Default*/ )
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingBackBuffer"));

	UpdateRenderTargetUsage( BackBuffer, RenderTargetUsage );
	RHISetRenderTarget( BackBuffer, FSurfaceRHIRef());
}

void FSceneRenderTargets::BeginRenderingFilter(FSceneRenderTargetIndex FilterColorIndex)
{
	// Set the filter color surface as the render target
	RHISetRenderTarget(GetFilterColorSurface(FilterColorIndex), FSurfaceRHIRef());
	RHISetViewport(0,0,0.0f,GSceneRenderTargets.GetFilterBufferSizeX(),GSceneRenderTargets.GetFilterBufferSizeY(),1.0f);
}

void FSceneRenderTargets::FinishRenderingFilter(FSceneRenderTargetIndex FilterColorIndex)
{
	// Resolve the filter color surface 
	RHICopyToResolveTarget(GetFilterColorSurface(FilterColorIndex), FALSE, FResolveParams());
}


void FSceneRenderTargets::BeginRenderingLUTBlend()
{
	RHISetRenderTarget(GetLUTBlendSurface(), FSurfaceRHIRef());

	UINT Scale = 1;
#ifdef XBOX
	// * 2 : SourceX needs to be 32 pixel aligned as this is a X360 Resolve() requirement
	Scale = 2;
#endif

	// texture array is unwrapped to 2d
	RHISetViewport(0, 0, 0.0f, 16 * 16 * Scale, 16, 1.0f);
}

void FSceneRenderTargets::FinishRenderingLUTBlend()
{
	// Resolve the filter color surface 
#if XBOX && !USE_NULL_RHI
	for(UINT Slice = 0; Slice < 16; ++Slice)
	{
		// x360 resolve source x needs to be multiple of 32
		FResolveParams Param(RenderTargets[LUTBlend].TextureArray, Slice, FResolveRect(Slice * 32, 0, Slice * 32 + 16, 16));

		RHICopyToResolveTarget(GetLUTBlendSurface(), FALSE, Param);
	}
#else // XBOX && !USE_NULL_RHI
	RHICopyToResolveTarget(GetLUTBlendSurface(), FALSE, FResolveParams());
#endif // XBOX && !USE_NULL_RHI
}

/** Clears the GBuffer render targets to default values. */
void FSceneRenderTargets::ClearGBufferTargets()
{
	#if !CONSOLE
		if (GRHIShaderPlatform == SP_PCD3D_SM5)
		{
			SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("ClearGBufferTargets"));

			//@todo - faster to clear at the same time with MRT?
			RHISetRenderTarget(GSceneRenderTargets.GetWorldNormalGBufferSurface(),FSurfaceRHIRef());
			RHIClear(TRUE,FLinearColor(0,0,1,0),FALSE,0,FALSE,0);

			RHISetRenderTarget(GSceneRenderTargets.GetWorldReflectionNormalGBufferSurface(),FSurfaceRHIRef());
			RHIClear(TRUE,FLinearColor(0,0,1,0),FALSE,0,FALSE,0);

			RHISetRenderTarget(GSceneRenderTargets.GetSpecularGBufferSurface(),FSurfaceRHIRef());
			RHIClear(TRUE,FLinearColor(0,0,0,0),FALSE,0,FALSE,0);

			RHISetRenderTarget(GSceneRenderTargets.GetDiffuseGBufferSurface(),FSurfaceRHIRef());
			RHIClear(TRUE,FLinearColor(0,0,0,0),FALSE,0,FALSE,0);

			if (GSystemSettings.RenderThreadSettings.bAllowSubsurfaceScattering)
			{
				RHISetRenderTarget(GSceneRenderTargets.GetSubsurfaceInscatteringSurface(),FSurfaceRHIRef());
				RHIClear(TRUE,FLinearColor(0,0,0,0),FALSE,0,FALSE,0);

				RHISetRenderTarget(GSceneRenderTargets.GetSubsurfaceScatteringAttenuationSurface(),FSurfaceRHIRef());
				RHIClear(TRUE,FLinearColor(0,0,0,0),FALSE,0,FALSE,0);
			}

			if(IsSeparateTranslucencyActive())
			{
				RHISetRenderTarget(RenderTargets[SeparateTranslucency].Surface, GetSceneDepthSurface());
				RHIClear(TRUE,FLinearColor(0,0,0,1),FALSE,0,FALSE,0);
#if ENABLE_SEPARATE_TRANSLUCENCY_DEPTH
				RHISetRenderTarget(RenderTargets[SeparateTranslucencyDepth].Surface, 0);
				RHIClear(TRUE,FLinearColor(0,0,0,0),FALSE,0,FALSE,0);
#endif		
			}
		}
	#endif
}

/**
 * Sets the scene color target and restores its contents if necessary
 * @param RenderTargetUsage	- Bit-wise combination of FSceneRenderTargetUsage flags.
 * @param bGBufferPass - Whether the pass about to be rendered is the GBuffer population pass
 * @param bLightingPass - Whether the pass about to be rendered is an additive lighting pass
 */
void FSceneRenderTargets::BeginRenderingSceneColor( DWORD RenderTargetUsage /*= RTUsage_Default*/, UBOOL bGBufferPass, UBOOL bLightingPass )
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingSceneColor"));

	if(RenderTargetUsage & RTUsage_RestoreSurface)
	{
		// Initialize the scene color surface to its previously resolved contents.
		RHICopyFromResolveTarget(GetSceneColorSurface());
	}

	UpdateRenderTargetUsage( GetSceneColorSurface(), RenderTargetUsage );

	// Set the scene color surface as the render target, and the scene depth surface as the depth-stencil target.
	RHISetRenderTarget( GetSceneColorSurface(), GetSceneDepthSurface());

	#if !CONSOLE
		if (GRHIShaderPlatform == SP_PCD3D_SM5)
		{
			checkSlow(!(bGBufferPass && bLightingPass));
			if (bGBufferPass)
			{
				RHISetMRTRenderTarget(GSceneRenderTargets.GetWorldNormalGBufferSurface(), 1);
				RHISetMRTColorWriteEnable(TRUE, 1);

				RHISetMRTRenderTarget(GSceneRenderTargets.GetWorldReflectionNormalGBufferSurface(), 2);
				RHISetMRTColorWriteEnable(TRUE, 2);

				RHISetMRTRenderTarget(GSceneRenderTargets.GetSpecularGBufferSurface(), 3);
				RHISetMRTColorWriteEnable(TRUE, 3);

				RHISetMRTRenderTarget(GSceneRenderTargets.GetDiffuseGBufferSurface(), 4);
				RHISetMRTColorWriteEnable(TRUE, 4);

				if (GSystemSettings.RenderThreadSettings.bAllowSubsurfaceScattering)
				{
					RHISetMRTRenderTarget(GetSubsurfaceInscatteringSurface(), 5);
					RHISetMRTColorWriteEnable(TRUE, 5);

					RHISetMRTRenderTarget(GetSubsurfaceScatteringAttenuationSurface(), 6);
					RHISetMRTColorWriteEnable(TRUE, 6);
				}
			}
			else if (bLightingPass)
			{
				RHISetMRTRenderTarget(GetSubsurfaceInscatteringSurface(), 1);
				RHISetMRTColorWriteEnable(TRUE, 1);
			}
		}
	#endif
} 

/**
* Called when finished rendering to the scene color surface
* @param bKeepChanges - if TRUE then the SceneColorSurface is resolved to the SceneColorTexture
*/
void FSceneRenderTargets::FinishRenderingSceneColor(UBOOL bKeepChanges, const FResolveRect& ResolveRect)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingSceneColor"));

	if(bKeepChanges)
	{
		// Resolve the scene color surface to the scene color texture.
		RHICopyToResolveTarget(GetSceneColorSurface(), TRUE, FResolveParams(ResolveRect));
		bSceneColorTextureIsRaw= FALSE;
	}

	#if !CONSOLE
		if (GRHIShaderPlatform == SP_PCD3D_SM5)
		{
			RHISetMRTRenderTarget(FSurfaceRHIRef(), 1);
			RHISetMRTColorWriteEnable(FALSE, 1);

			RHISetMRTRenderTarget(FSurfaceRHIRef(), 2);
			RHISetMRTColorWriteEnable(FALSE, 2);

			RHISetMRTRenderTarget(FSurfaceRHIRef(), 3);
			RHISetMRTColorWriteEnable(FALSE, 3);

			RHISetMRTRenderTarget(FSurfaceRHIRef(), 4);
			RHISetMRTColorWriteEnable(FALSE, 4);

			if (GSystemSettings.RenderThreadSettings.bAllowSubsurfaceScattering)
			{
				// Unset the subsurface scattering MRTs.
				RHISetMRTRenderTarget(FSurfaceRHIRef(), 5);
				RHISetMRTColorWriteEnable(FALSE, 5);

				RHISetMRTRenderTarget(FSurfaceRHIRef(), 6);
				RHISetMRTColorWriteEnable(FALSE, 6);
			}
		}
	#endif
}

/**
 * Sets the LDR version of the scene color target.
 * @param RenderTargetUsage	- Bit-wise combination of FSceneRenderTargetUsage flags.
 */
void FSceneRenderTargets::BeginRenderingSceneColorLDR( DWORD RenderTargetUsage /*= 0*/ )
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingSceneColorLDR"));

	UpdateRenderTargetUsage( GetSceneColorLDRSurface(), RenderTargetUsage );

	// Set the light attenuation surface as the render target, and the scene depth buffer as the depth-stencil surface.
	RHISetRenderTarget(GetSceneColorLDRSurface(),GetSceneDepthSurface());
}

/**
* Called when finished rendering to the LDR version of the scene color surface.
* @param bKeepChanges - if TRUE then the SceneColorSurface is resolved to the LDR SceneColorTexture
* @param ResolveParams - optional resolve params
*/
void FSceneRenderTargets::FinishRenderingSceneColorLDR(UBOOL bKeepChanges,const FResolveRect& ResolveRect)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingSceneColorLDR"));

	if(bKeepChanges)
	{
		// Resolve the scene color surface to the scene color texture.
		RHICopyToResolveTarget(GetSceneColorLDRSurface(), TRUE, FResolveParams(ResolveRect));
	}
}


/**
* Saves a previously rendered scene color target
*/
void FSceneRenderTargets::ResolveSceneColor(const FResolveRect& ResolveRect)
{
    SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("ResolveSceneColor"));
	RHICopyToResolveTarget(GetSceneColorSurface(), TRUE, FResolveParams(ResolveRect));
	bSceneColorTextureIsRaw= FALSE;
}

void FSceneRenderTargets::ResolveSubsurfaceScatteringSurfaces(const FResolveRect& ResolveRect)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("ResolveSubsurfaceScattering"));
	#if !CONSOLE
		check(GSystemSettings.RenderThreadSettings.bAllowSubsurfaceScattering);
		RHICopyToResolveTarget(GetSubsurfaceInscatteringSurface(), TRUE, FResolveParams(ResolveRect));
		RHICopyToResolveTarget(GetSubsurfaceScatteringAttenuationSurface(), TRUE, FResolveParams(ResolveRect));
	#endif
}

/** Resolves the GBuffer targets so that their resolved textures can be sampled. */
void FSceneRenderTargets::ResolveGBufferSurfaces(const FResolveRect& ResolveRect)
{
#if !CONSOLE
	if (GRHIShaderPlatform == SP_PCD3D_SM5)
	{
		SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("ResolveGBufferSurfaces"));
		RHICopyToResolveTarget(GetWorldNormalGBufferSurface(), TRUE, FResolveParams(ResolveRect));
		RHICopyToResolveTarget(GetWorldReflectionNormalGBufferSurface(), TRUE, FResolveParams(ResolveRect));
		RHICopyToResolveTarget(GetSpecularGBufferSurface(), TRUE, FResolveParams(ResolveRect));
		RHICopyToResolveTarget(GetDiffuseGBufferSurface(), TRUE, FResolveParams(ResolveRect));
	}
#endif
}

/**
* Sets the raw version of the scene color target.
*/
void FSceneRenderTargets::BeginRenderingSceneColorRaw()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingSceneColorRaw"));

	// Use the raw version of the scene color as the render target, and use the standard scene depth buffer as the depth-stencil surface.
	RHISetRenderTarget( GetSceneColorRawSurface(), GetSceneDepthSurface());
}

/**
 * Saves a previously rendered scene color surface in the raw bit format.
 */
void FSceneRenderTargets::SaveSceneColorRaw(UBOOL bConvertToFixedPoint, const FResolveRect& ResolveRect)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("SaveSceneColorRaw"));

#if XBOX
	if (bConvertToFixedPoint)
	{
		RHICopyToResolveTarget(RenderTargets[SceneColorFixedPoint].Surface, TRUE, FResolveParams(ResolveRect));
	}
	else
#endif
	{
		RHICopyToResolveTarget(GetSceneColorRawSurface(), TRUE, FResolveParams(ResolveRect));
	}
	bSceneColorTextureIsRaw= TRUE;
}

/**
 * Restores a previously saved raw-scene color surface.
 */
void FSceneRenderTargets::RestoreSceneColorRaw()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("RestoreSceneColorRaw"));

	// Initialize the scene color surface to its previously resolved contents.
	RHICopyFromResolveTargetFast(GetSceneColorRawSurface());

	// Set the scene color surface as the render target, and the scene depth surface as the depth-stencil target.
	RHISetRenderTarget( GetSceneColorSurface(), GetSceneDepthSurface());
}

/**
 * Restores a rectangle from a previously saved raw-scene color surface.
 */
void FSceneRenderTargets::RestoreSceneColorRectRaw(FLOAT X1,FLOAT Y1,FLOAT X2,FLOAT Y2)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("RestoreSceneColorRectRaw"));

	// Initialize the scene color surface to its previously resolved contents.
	RHICopyFromResolveTargetRectFast(GetSceneColorRawSurface(), X1, Y1, X2, Y2);

	// Set the scene color surface as the render target, and the scene depth surface as the depth-stencil target.
	RHISetRenderTarget( GetSceneColorSurface(), GetSceneDepthSurface());
}

void FSceneRenderTargets::BeginRenderingPrePass()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingPrePass"));

	// Set the scene depth surface and a DUMMY buffer as color buffer
	// (as long as it's the same dimension as the depth buffer),
	RHISetRenderTarget( GetLightAttenuationSurface(), GetSceneDepthSurface());

	// Disable color writes since we only want z depths
	RHISetColorWriteEnable(FALSE);
}

void FSceneRenderTargets::FinishRenderingPrePass()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingPrePass"));

	// Re-enable color writes
	RHISetColorWriteEnable(TRUE);
}

void FSceneRenderTargets::BeginRenderingPostTranslucencyDepth()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingPostTranslucencyDepth"));

	GSceneRenderTargets.BeginRenderingSceneColor();

	if(GSupportsDepthTextures)
	{
		// Disable all color writes since we only want z depths
		RHISetColorWriteEnable(FALSE);
	}
	else
	{
		// Depths are stored in scene color alpha so mask out just RGB
		RHISetColorWriteMask(CW_ALPHA);
	}
}

void FSceneRenderTargets::FinishRenderingPostTranslucencyDepth(UBOOL bKeep,const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingPostTranslucencyDepth"));

	if(GSupportsDepthTextures)
	{
		// Re-enable color writes
		RHISetColorWriteEnable(TRUE);

		if(bKeep)
		{
			// We need to resolve SceneDepth so PP will use the updated depth.
			RHICopyToResolveTarget(GetSceneDepthSurface(), TRUE, ResolveParams);
		}
	}
	else
	{
		// Re-enable all channels.
		RHISetColorWriteMask(CW_RGBA);

		// No need to resolve SceneColor here as it will be resolved later
		// due to the translucency dirtying scene color.
	}
}

void FSceneRenderTargets::BeginRenderingDoFBlurBuffer()
{
#if XBOX
	RHISetMRTRenderTarget(GetDoFBlurBufferSurface(),1);
	RHISetMRTColorWriteMask(CW_RED,1);
#endif
}

void FSceneRenderTargets::FinishRenderingDoFBlurBuffer()
{
#if XBOX
	RHISetMRTRenderTarget(NULL,1);
	RHISetMRTColorWriteMask(CW_RGBA,1);
#endif
}

void FSceneRenderTargets::BeginRenderingShadowDepth(UBOOL bIsWholeSceneDominantShadow)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingShadowDepth"));

	if(IsHardwarePCFSupported() || IsFetch4Supported())
	{
		// set the shadow z surface as the depth buffer
		// have to bind a color target that is the same size as the depth texture on platforms that support Hardware PCF and Fetch4
		RHISetRenderTarget(GetShadowDepthColorSurface(bIsWholeSceneDominantShadow), GetShadowDepthZSurface(bIsWholeSceneDominantShadow));   
		// disable color writes since we only want z depths
		RHISetColorWriteEnable(FALSE);
	}
	else if( GSupportsDepthTextures)
	{
		// set the shadow z surface as the depth buffer
		RHISetRenderTarget(FSurfaceRHIRef(), GetShadowDepthZSurface(bIsWholeSceneDominantShadow));   
		// disable color writes since we only want z depths
		RHISetColorWriteEnable(FALSE);
	}
	else
	{
		// Set the shadow color surface as the render target, and the shadow z surface as the depth buffer
		RHISetRenderTarget(GetShadowDepthColorSurface(bIsWholeSceneDominantShadow), GetShadowDepthZSurface(bIsWholeSceneDominantShadow));
	}
}

/** Binds the appropriate shadow depth render target for rendering to the preshadow cache, and sets up color write enable state. */
void FSceneRenderTargets::BeginRenderingPreshadowCacheDepth()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginPreshadowCacheDepth"));

	if(IsHardwarePCFSupported() || IsFetch4Supported())
	{
		// set the shadow z surface as the depth buffer
		// have to bind a color target that is the same size as the depth texture on platforms that support Hardware PCF and Fetch4
		RHISetRenderTarget(GetPreshadowCacheDepthColorSurface(), GetPreshadowCacheDepthZSurface());   
		// disable color writes since we only want z depths
		RHISetColorWriteEnable(FALSE);
	}
	else if( GSupportsDepthTextures)
	{
		// set the shadow z surface as the depth buffer
		RHISetRenderTarget(FSurfaceRHIRef(), GetPreshadowCacheDepthZSurface());   
		// disable color writes since we only want z depths
		RHISetColorWriteEnable(FALSE);
	}
	else
	{
		// Set the shadow color surface as the render target, and the shadow z surface as the depth buffer
		RHISetRenderTarget(GetPreshadowCacheDepthColorSurface(), GetPreshadowCacheDepthZSurface());
	}
}

/** Binds the appropriate shadow depth cube map for rendering. */
void FSceneRenderTargets::BeginRenderingCubeShadowDepth(INT ShadowResolution)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingCubeShadowDepth"));
	RHISetRenderTarget(FSurfaceRHIRef(), GetCubeShadowDepthZSurface(ShadowResolution));   
	// disable color writes since we only want z depths
	RHISetColorWriteEnable(FALSE);
}

/**
 * Called when finished rendering to the subject shadow depths so the surface can be copied to texture
 * @param ResolveParams - optional resolve params
 */
void FSceneRenderTargets::FinishRenderingShadowDepth(UBOOL bIsWholeSceneDominantShadow, const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingShadowDepth"));

	if( GSupportsDepthTextures || IsHardwarePCFSupported() || IsFetch4Supported())
	{
		// Resolve the shadow depth z surface.
		RHICopyToResolveTarget(GetShadowDepthZSurface(bIsWholeSceneDominantShadow), FALSE, ResolveParams);
		// restore color writes
		RHISetColorWriteEnable(TRUE);
	}
	else
	{
		// Resolve the shadow depth color surface.
		RHICopyToResolveTarget(GetShadowDepthColorSurface(bIsWholeSceneDominantShadow), FALSE, ResolveParams);
	}
}

/**
 * Resolve the preshadow cache depth surface
 * @param ResolveParams - optional resolve params
 */
void FSceneRenderTargets::ResolvePreshadowCacheDepth(const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("ResolvePreshadowCacheDepth"));

	if( GSupportsDepthTextures || IsHardwarePCFSupported() || IsFetch4Supported())
	{
		// Resolve the shadow depth z surface.
		RHICopyToResolveTarget(GetPreshadowCacheDepthZSurface(), FALSE, ResolveParams);
	}
	else
	{
		// Resolve the shadow depth color surface.
		RHICopyToResolveTarget(GetPreshadowCacheDepthColorSurface(), FALSE, ResolveParams);
	}
}

/** Resolves the approprate shadow depth cube map and restores default state. */
void FSceneRenderTargets::FinishRenderingCubeShadowDepth(INT ShadowResolution, const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingCubeShadowDepth"));
	RHICopyToResolveTarget(GetCubeShadowDepthZSurface(ShadowResolution), FALSE, ResolveParams);
	RHISetColorWriteEnable(TRUE);
}

void FSceneRenderTargets::BeginRenderingLightAttenuation(UBOOL bUseTexture0)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingLightAttenuation%u"),bUseTexture0 ? 0 : 1);

	// Set the light attenuation surface as the render target, and the scene depth buffer as the depth-stencil surface.
	RHISetRenderTarget(GetLightAttenuationSurface(bUseTexture0),GetSceneDepthSurface());
}

void FSceneRenderTargets::FinishRenderingLightAttenuation(UBOOL bUseTexture0)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingLightAttenuation%u"),bUseTexture0 ? 0 : 1);

	// Resolve the light attenuation surface.
	RHICopyToResolveTarget(GetLightAttenuationSurface(bUseTexture0), FALSE, FResolveParams());
}

void FSceneRenderTargets::BeginRenderingTranslucency(const FViewInfo& View, UBOOL bDownSampled, UBOOL bStateChanged)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("Begin %s Translucency"), bDownSampled ? TEXT("Downsampled") : TEXT("FullRes"));
	if (bDownSampled)
	{
		// Render to the downsampled translucency buffer, using the small depth buffer
		RHISetRenderTarget(GetTranslucencyBufferSurface(), GetSmallDepthSurface());
		GCurrentColorExpBias = 3;
		RHISetRenderTargetBias( appPow(2.0f,GCurrentColorExpBias ));

		const UINT DownsampledX = appTrunc(View.RenderTargetX / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor());
		const UINT DownsampledY = appTrunc(View.RenderTargetY / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor());
		const UINT DownsampledSizeX = appTrunc(View.RenderTargetSizeX / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor());
		const UINT DownsampledSizeY = appTrunc(View.RenderTargetSizeY / GSceneRenderTargets.GetSmallColorDepthDownsampleFactor());

		// Setup the viewport for rendering to the downsampled translucency buffer
		RHISetViewport(DownsampledX, DownsampledY, 0.0f, DownsampledX + DownsampledSizeX, DownsampledY + DownsampledSizeY, 1.0f);
		//@todo - currently PSR_ScreenPositionScaleBias is being set and is dependent on the full resolution, does this need to be fixed?
		RHISetViewParameters(View);

		if (bStateChanged)
		{
			// We want to support BLEND_Additive and BLEND_Translucent with the same offscreen buffer,
			// So RGB will additively accumulate the color to add to scene color, which is SourceColor * SourceAlpha,
			// And Alpha will multiplicatively accumulate the factor to multiply by scene color, which is (1 - SourceAlpha).
			// Clear the translucency buffer to values which will not modify scene color.
			RHIClear(TRUE, FLinearColor(0, 0, 0, 1), FALSE, 0, FALSE, 0);
		}
	}
	else
	{
		// Use the scene color buffer.
		GSceneRenderTargets.BeginRenderingSceneColor();
	
		// viewport to match view size
		RHISetViewport(View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
		RHISetViewParameters(View);
	}
	// Enable depth test, disable depth writes.
	RHISetDepthState(TStaticDepthState<FALSE,CF_LessEqual>::GetRHI());
}

void FSceneRenderTargets::BeginRenderingSeparateTranslucency(const FViewInfo& View)
{
	if(IsSeparateTranslucencyActive())
	{
		SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("Begin SeparateTranslucency"));

		// Use a separate render target for translucency
		RHISetRenderTarget(RenderTargets[SeparateTranslucency].Surface, GetSceneDepthSurface());
#if ENABLE_SEPARATE_TRANSLUCENCY_DEPTH
		RHISetMRTRenderTarget(RenderTargets[SeparateTranslucencyDepth].Surface, 1);
		RHISetMRTColorWriteEnable(TRUE, 1);
#endif
		// viewport to match view size
		RHISetViewport(View.RenderTargetX,View.RenderTargetY,0.0f,View.RenderTargetX + View.RenderTargetSizeX,View.RenderTargetY + View.RenderTargetSizeY,1.0f);
		RHISetViewParameters(View);
	}
}

void FSceneRenderTargets::FinishRenderingSeparateTranslucency()
{
}

void FSceneRenderTargets::ResolveFullResTransluceny()
{
	if(IsSeparateTranslucencyActive())
	{
		RHICopyToResolveTarget(RenderTargets[SeparateTranslucency].Surface, TRUE, FResolveParams());
#if ENABLE_SEPARATE_TRANSLUCENCY_DEPTH
		RHICopyToResolveTarget(RenderTargets[SeparateTranslucencyDepth].Surface, TRUE, FResolveParams());
		RHISetMRTColorWriteEnable(FALSE, 1);
#endif
	}
}

void FSceneRenderTargets::FinishRenderingTranslucency(const FResolveParams& ResolveParams, UBOOL bDownSampled)
{
	if (bDownSampled)
	{
		SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("Finish Downsampled Translucency"));

		GCurrentColorExpBias = 0;
		RHISetRenderTargetBias( appPow(2.0f,GCurrentColorExpBias ));
		RHICopyToResolveTarget(GetTranslucencyBufferSurface(), FALSE, ResolveParams);
	}
}

void FSceneRenderTargets::BeginRenderingAOInput(UBOOL bUseDownsizedDepthBuffer)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingAOInput"));
	if (bUseDownsizedDepthBuffer)
	{
		RHISetRenderTarget(GetAOInputSurface(),GetSmallDepthSurface());
	}
	else
	{
		RHISetRenderTarget(GetAOInputSurface(),StereoizedDrawNullTarget(GetSceneDepthSurface()));
	}
}

void FSceneRenderTargets::FinishRenderingAOInput(const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingAOInput"));
	RHICopyToResolveTarget(GetAOInputSurface(), FALSE, ResolveParams);
}

void FSceneRenderTargets::BeginRenderingAOOutput(UBOOL bUseDownsizedDepthBuffer)
{
	if (bUseDownsizedDepthBuffer)
	{
		RHISetRenderTarget(GetAOOutputSurface(),GetSmallDepthSurface());
	}
	else
	{
		RHISetRenderTarget(GetAOOutputSurface(),StereoizedDrawNullTarget(GetSceneDepthSurface()));
	}
}

void FSceneRenderTargets::FinishRenderingAOOutput(const FResolveParams& ResolveParams)
{
	RHICopyToResolveTarget(GetAOOutputSurface(), FALSE, ResolveParams);
}

void FSceneRenderTargets::BeginRenderingAOHistory(UBOOL bUseDownsizedDepthBuffer)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingAOHistory"));
	if (bUseDownsizedDepthBuffer)
	{
		RHISetRenderTarget(GetAOHistorySurface(),GetSmallDepthSurface());
	}
	else
	{
		RHISetRenderTarget(GetAOHistorySurface(),StereoizedDrawNullTarget(GetSceneDepthSurface()));
	}
}

void FSceneRenderTargets::FinishRenderingAOHistory(const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingAOHistory"));
	RHICopyToResolveTarget(GetAOHistorySurface(), FALSE, ResolveParams);
}

void FSceneRenderTargets::BeginRenderingDistortionAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingDistortionAccumulation"));

	// use RGBA8 light target for accumulating distortion offsets	
	// R = positive X offset
	// G = positive Y offset
	// B = negative X offset
	// A = negative Y offset

	RHISetRenderTarget(GetLightAttenuationSurface(),GetSceneDepthSurface());
}

void FSceneRenderTargets::FinishRenderingDistortionAccumulation(const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingDistortionAccumulation"));

	RHICopyToResolveTarget(GetLightAttenuationSurface(), FALSE, ResolveParams);
}

void FSceneRenderTargets::BeginRenderingDistortionDepth()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingDistortionDepth"));
}

void FSceneRenderTargets::FinishRenderingDistortionDepth()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingDistortionDepth"));
}

/** Starts rendering to the velocity buffer. */
void FSceneRenderTargets::BeginRenderingVelocities()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingVelocities"));

#if XBOX
	// Set the motion blur velocity buffer as the render target, and the small depth surface as the depth-stencil target.
	RHISetRenderTarget( GetVelocitySurface(), GetSmallDepthSurface() );
#else
	RHISetRenderTarget( GetVelocitySurface(), GetSceneDepthSurface() );
#endif

	if(GSystemSettings.RenderThreadSettings.MotionBlurSkinningRT)
	{
		PrevPerBoneMotionBlur.LockData();
	}
}

/** Stops rendering to the velocity buffer. */
void FSceneRenderTargets::FinishRenderingVelocities()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingVelocities"));

	// Resolve the velocity buffer to a texture, so it can be read later.
	RHICopyToResolveTarget(GetVelocitySurface(), FALSE, FResolveParams());

	// after RHICopyToResolveTarget so the vertex texture is set to NULL
	if(GSystemSettings.RenderThreadSettings.MotionBlurSkinningRT)
	{
		PrevPerBoneMotionBlur.UnlockData();
	}
}

void FSceneRenderTargets::BeginRenderingFogFrontfacesIntegralAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingFogFrontfacesIntegralAccumulation"));
	RHISetRenderTarget(GetFogFrontfacesIntegralAccumulationSurface(),FSurfaceRHIRef());
}

void FSceneRenderTargets::FinishRenderingFogFrontfacesIntegralAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingFogFrontfacesIntegralAccumulation"));

	RHICopyToResolveTarget(GetFogFrontfacesIntegralAccumulationSurface(), FALSE, FResolveParams());
}

void FSceneRenderTargets::BeginRenderingFogBackfacesIntegralAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingFogBackfacesIntegralAccumulation"));
	RHISetRenderTarget(GetFogBackfacesIntegralAccumulationSurface(),FSurfaceRHIRef());
}

void FSceneRenderTargets::FinishRenderingFogBackfacesIntegralAccumulation()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingFogBackfacesIntegralAccumulation"));

	RHICopyToResolveTarget(GetFogBackfacesIntegralAccumulationSurface(), FALSE, FResolveParams());
}

void FSceneRenderTargets::ResolveSceneDepthTexture()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("ResolveSceneDepthTexture"));

	if(GSupportsDepthTextures)
	{
		// Resolve the scene depth surface.
		RHICopyToResolveTarget(GetSceneDepthSurface(), TRUE, FResolveParams());
	}
}

void FSceneRenderTargets::BeginRenderingHitProxies()
{
	RHISetRenderTarget(GetHitProxySurface(),GetSceneDepthSurface());
}

void FSceneRenderTargets::FinishRenderingHitProxies()
{
	RHICopyToResolveTarget(GetHitProxySurface(), FALSE, FResolveParams());
}

void FSceneRenderTargets::BeginRenderingFogBuffer()
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("BeginRenderingFogBuffer"));

	RHISetMRTRenderTarget(GetFogBufferSurface(),1);
}

void FSceneRenderTargets::FinishRenderingFogBuffer(const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("FinishRenderingFogBuffer"));

	RHISetMRTRenderTarget(NULL,1);
	RHICopyToResolveTarget(GetFogBufferSurface(), FALSE, ResolveParams);
}

void FSceneRenderTargets::SetBufferSize( const UINT InBufferSizeX, const UINT InBufferSizeY )
{
	// ensure sizes are dividable by DividableBy to get post processing effects with lower resolution working well
	const UINT DividableBy = 8;

	const UINT Mask = ~(DividableBy - 1);
	BufferSizeX = (InBufferSizeX + DividableBy - 1) & Mask;
	BufferSizeY = (InBufferSizeY + DividableBy - 1) & Mask;

	FilterDownsampleFactor = 4;
	FilterBufferSizeX = BufferSizeX / FilterDownsampleFactor + 2;
	FilterBufferSizeY = BufferSizeY / FilterDownsampleFactor + 2;

	FogAccumulationDownsampleFactor = 2;
	FogAccumulationBufferSizeX = Max< UINT >( 1, BufferSizeX / FogAccumulationDownsampleFactor );
	FogAccumulationBufferSizeY = Max< UINT >( 1, BufferSizeY / FogAccumulationDownsampleFactor );

	SetAODownsampleFactor(AODownsampleFactor);
}

void FSceneRenderTargets::InitDynamicRHI()
{
	if(BufferSizeX > 0 && BufferSizeY > 0)
	{
		// Allocate render targets needed for rendering to a downsampled translucency buffer
		// Size of translucency buffer must match the small depth buffer size or else translucent objects will not render correctly
		TranslucencyBufferSizeX = Max<UINT>(BufferSizeX / SmallColorDepthDownsampleFactor, 1);
		TranslucencyBufferSizeY = Max<UINT>(BufferSizeY / SmallColorDepthDownsampleFactor, 1);

#if WITH_MOBILE_RHI
		if( GUsingMobileRHI )
		{
			// initialize render targets to NULL
			appMemzero(RenderTargets, sizeof(RenderTargets));

			// From ES2RHIImplementation.cpp
			extern UBOOL GMobileUseOffscreenRendering;

#if NGP
			if ( GMobileUsePostProcess )
			{
				// Main scene rendertarget (offscreen).
				RenderTargets[SceneColor].Texture = RHICreateTexture2D(BufferSizeX, BufferSizeY, PF_A8R8G8B8, 1, TexCreate_ResolveTargetable | TexCreate_PointFilterNGP, NULL);
				RenderTargets[SceneColor].Surface = RHICreateTargetableSurface(BufferSizeX, BufferSizeY, PF_A8R8G8B8, RenderTargets[SceneColor].Texture, TargetSurfCreate_Dedicated | TargetSurfCreate_Multisample, TEXT("DefaultColor"));

				// Create a buffer for the MSAA scene depth.
				RenderTargets[SceneDepthZ].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_DepthStencil,1,TexCreate_ResolveTargetable|TexCreate_DepthStencil|TexCreate_Multisample,NULL);
				RenderTargets[SceneDepthZ].Surface = RHICreateTargetableSurface(BufferSizeX,BufferSizeY,PF_DepthStencil,RenderTargets[SceneDepthZ].Texture,TargetSurfCreate_Multisample,TEXT("DefaultDepth"));

				extern INT MobileGetMSAAFactor();
				if ( MobileGetMSAAFactor() == 1 )
				{
					// Reuse the SceneDepth buffer.
					RenderTargets[ResolvedDepthBuffer].Texture = RenderTargets[SceneDepthZ].Texture;
					RenderTargets[ResolvedDepthBuffer].Surface = RenderTargets[SceneDepthZ].Surface;
				}
				else
				{
					// Create a buffer for the resolved MSAA scene depth.
					RenderTargets[ResolvedDepthBuffer].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_R32F,1,TexCreate_ResolveTargetable,NULL);
					RenderTargets[ResolvedDepthBuffer].Surface = RHICreateTargetableSurface(BufferSizeX,BufferSizeY,PF_R32F,RenderTargets[ResolvedDepthBuffer].Texture,TargetSurfCreate_Dedicated,TEXT("ResolvedDepth"));
				}

				// Half-rez used by the for downsampled SceneColor
				RenderTargets[TranslucencyBuffer].Texture = RHICreateTexture2D(TranslucencyBufferSizeX,TranslucencyBufferSizeY,PF_A8R8G8B8,1,TexCreate_ResolveTargetable,NULL);
				RenderTargets[TranslucencyBuffer].Surface = RHICreateTargetableSurface(TranslucencyBufferSizeX,TranslucencyBufferSizeY,PF_A8R8G8B8,RenderTargets[TranslucencyBuffer].Texture,TargetSurfCreate_Dedicated,TEXT("TranslucencyBuffer"));

				// Smaller rendertargets, used by post-process (offscreen).
				RenderTargets[FilterColor].Texture = RHICreateTexture2D(FilterBufferSizeX, FilterBufferSizeY, PF_A8R8G8B8, 1, TexCreate_ResolveTargetable, NULL);
				RenderTargets[FilterColor].Surface = RHICreateTargetableSurface(FilterBufferSizeX, FilterBufferSizeY, PF_A8R8G8B8, RenderTargets[FilterColor].Texture, TargetSurfCreate_Dedicated, TEXT("FilterColor"));
				RenderTargets[FilterColor2].Texture = RHICreateTexture2D(FilterBufferSizeX, FilterBufferSizeY, PF_A8R8G8B8, 1, TexCreate_ResolveTargetable, NULL);
				RenderTargets[FilterColor2].Surface = RHICreateTargetableSurface(FilterBufferSizeX, FilterBufferSizeY, PF_A8R8G8B8, RenderTargets[FilterColor2].Texture, TargetSurfCreate_Dedicated, TEXT("FilterColor2"));

				{
					const EPixelFormat LUTFormat = PF_A8R8G8B8;
					UINT Extend = 16;
					UINT Width = Extend * Extend;
					UINT Height = Extend;
					RenderTargets[LUTBlend].Texture = RHICreateTexture2D(Width, Height, LUTFormat, 1, TexCreate_ResolveTargetable, NULL);
					RenderTargets[LUTBlend].Surface = RHICreateTargetableSurface(Width, Height, LUTFormat, RenderTargets[LUTBlend].Texture, TargetSurfCreate_Dedicated, TEXT("LUTBlend"));
				}

				// Post-process rendertarget, and final display buffer (frame buffer).
				RenderTargets[LightAttenuation0].Surface = RHIGetViewportBackBuffer(NULL);
			}
			else
			{
				RenderTargets[SceneColor].Surface = RHIGetViewportBackBuffer(NULL);
				RenderTargets[SceneColor].Texture = RHIGetResolveTarget( RenderTargets[SceneColor].Surface );
			}
#else
			// allocate only the render targets we'll need on mobile devices
			if (GMobileUseOffscreenRendering)
			{
				RenderTargets[SceneColor].Texture = RHICreateTexture2D(BufferSizeX, BufferSizeY, PF_A8R8G8B8, 1, TexCreate_ResolveTargetable, NULL);
				RenderTargets[SceneColor].Surface = RHICreateTargetableSurface(
					BufferSizeX, BufferSizeY, PF_A8R8G8B8, RenderTargets[SceneColor].Texture, TargetSurfCreate_Dedicated, TEXT("DefaultColor"));

				RenderTargets[SceneDepthZ].Texture = NULL;//RHICreateTexture2D(BufferSizeX, BufferSizeY, PF_DepthStencil, 1 ,TexCreate_ResolveTargetable|TexCreate_DepthStencil, NULL);
				RenderTargets[SceneDepthZ].Surface = RHICreateTargetableSurface(
					BufferSizeX, BufferSizeY, PF_DepthStencil, RenderTargets[SceneDepthZ].Texture, 0, TEXT("DefaultDepth"));
			}
			else
			{
				RenderTargets[SceneColor].Surface = RHIGetViewportBackBuffer(NULL);
			}
#endif

			return;
		}
#endif		// WITH_MOBILE_RHI

		const DWORD MultiSampleFlag = TargetSurfCreate_Multisample;

#if PS3
		UBOOL bCreateSceneDepthTexture = TRUE;
#else
		UBOOL bCreateSceneDepthTexture = GSupportsDepthTextures || GRHIShaderPlatform == SP_PCD3D_SM5;
#endif

		if(bCreateSceneDepthTexture)
		{
			DWORD CreateFlags = MultiSampleFlag;
			
#if !CONSOLE
			if (GRHIShaderPlatform == SP_PCD3D_SM5)
			{
				// Note that the surface needs to be dedicated, so that we can keep the resolved depth from the world DPG around after we clear the depth to render subsequent DPGs.
				CreateFlags |= TargetSurfCreate_Dedicated;
			}
#endif

			// Create a texture to store the resolved scene depth, and a render-targetable surface to hold the unresolved scene depth.
			RenderTargets[SceneDepthZ].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_DepthStencil,1,TexCreate_ResolveTargetable|TexCreate_DepthStencil,NULL);
			RenderTargets[SceneDepthZ].Surface = RHICreateTargetableSurface(
				BufferSizeX,BufferSizeY,PF_DepthStencil,RenderTargets[SceneDepthZ].Texture,CreateFlags,TEXT("DefaultDepth"));
		}
		else
		{
			// Create a surface to store the unresolved scene depth.
			RenderTargets[SceneDepthZ].Surface = RHICreateTargetableSurface(
				BufferSizeX,BufferSizeY,PF_DepthStencil,FTexture2DRHIRef(),TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("DefaultDepth"));
		}

		if (SceneColorBufferFormat == PF_A32B32G32R32F && !GPixelFormats[PF_A32B32G32R32F].Supported)
		{
			SceneColorBufferFormat = PF_FloatRGB;
		}

		if (SceneColorBufferFormat == PF_FloatRGB)
		{
			// Potentially allocate an alpha channel in the scene color texture to store the resolved scene depth.
			SceneColorBufferFormat = GSupportsDepthTextures ? PF_FloatRGB : PF_FloatRGBA;
		}

		//@todo - implement for PS3
#if !PS3 && !USE_NULL_RHI
		// XeHiZOffset needs to be updated if SmallColorDepthDownsampleFactor changes
		check(SmallColorDepthDownsampleFactor == 2);
		// Create a quarter-sized version of the scene depth.
		RenderTargets[SmallDepthZ].Surface = RHICreateTargetableSurface(Max<UINT>(BufferSizeX / SmallColorDepthDownsampleFactor,1),Max<UINT>(BufferSizeY / SmallColorDepthDownsampleFactor,1),PF_DepthStencil,FTexture2DRHIRef(),TargetSurfCreate_Dedicated,TEXT("SmallDepth"));
#if XBOX
		// Not done for PC as Geforce 6/7 cards give random results when occlusion testing with a downsampled depth buffer (but other SM3 cards work fine)
		bUseDownsizedOcclusionQueries = TRUE;
		// PC also creates the small depth buffer, but has not been tested with all the users that check bDownsizedDepthSupported
		bDownsizedDepthSupported = TRUE;
#endif
#endif

		const FIntPoint WholeSceneDominantShadowBufferResolution = GetShadowDepthTextureResolution(TRUE);
#if XBOX && !USE_NULL_RHI
		// Create a texture to store the resolved scene colors, and a dedicated render-targetable surface to hold the unresolved scene colors.
		extern SIZE_T XeCalculateTextureBytes(DWORD SizeX,DWORD SizeY,DWORD SizeZ,BYTE Format);
		const SIZE_T ExpandedSceneColorSize = XeCalculateTextureBytes(BufferSizeX,BufferSizeY,1,SceneColorBufferFormat);
		const SIZE_T RawSceneColorSize = XeCalculateTextureBytes(BufferSizeX,BufferSizeY,1,PF_A2B10G10R10);
		const SIZE_T WholeSceneDominantShadowBufferSize = XeCalculateTextureBytes(WholeSceneDominantShadowBufferResolution.X,WholeSceneDominantShadowBufferResolution.Y,1,PF_ShadowDepth);
		const SIZE_T SharedSceneColorSize = Max(Max(ExpandedSceneColorSize,RawSceneColorSize), WholeSceneDominantShadowBufferSize);
		FSharedMemoryResourceRHIRef SceneColorMemoryBuffer = RHICreateSharedMemory(SharedSceneColorSize);

		RenderTargets[SceneColor].Texture = RHICreateSharedTexture2D(BufferSizeX,BufferSizeY,SceneColorBufferFormat,1,SceneColorMemoryBuffer,TexCreate_ResolveTargetable);
		RenderTargets[SceneColor].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,SceneColorBufferFormat,RenderTargets[SceneColor].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("DefaultColor"));

		// Create a version of the scene color textures that represent the raw bits (i.e. that can do the resolves without any format conversion)
		RenderTargets[SceneColorRaw].Texture = RHICreateSharedTexture2D(BufferSizeX,BufferSizeY,PF_A2B10G10R10,1,SceneColorMemoryBuffer,TexCreate_ResolveTargetable);
		RenderTargets[SceneColorRaw].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,PF_A2B10G10R10,RenderTargets[SceneColorRaw].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("DefaultColorRaw"));

		// Create a version of the scene color textures that is converted to fixed point.
		// This version has banding in the darks so it is only used when the banding will not be noticeable.
		RenderTargets[SceneColorFixedPoint].Texture = RenderTargets[SceneColorRaw].Texture;
		RenderTargets[SceneColorFixedPoint].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,PF_FloatRGB,RenderTargets[SceneColorRaw].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("DefaultColorFixedPoint"));
#else
		// Create a texture to store the resolved scene colors, and a dedicated render-targetable surface to hold the unresolved scene colors.
		RenderTargets[SceneColor].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,SceneColorBufferFormat,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[SceneColor].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,SceneColorBufferFormat,RenderTargets[SceneColor].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("DefaultColor"));
		RenderTargets[SceneColorRaw].Texture = RenderTargets[SceneColor].Texture;
		RenderTargets[SceneColorRaw].Surface = RenderTargets[SceneColor].Surface;

		RenderTargets[SceneColorFixedPoint].Texture = NULL;
		RenderTargets[SceneColorFixedPoint].Surface = NULL;
#endif

		// make sure our flag noting the scene color texture is use is setup
		bSceneColorTextureIsRaw= FALSE;

		const EPixelFormat LightAttenuationBufferFormat = PF_A8R8G8B8;
#if XBOX && !USE_NULL_RHI
		const SIZE_T LightAttenuationSize = XeCalculateTextureBytes(BufferSizeX,BufferSizeY,1,LightAttenuationBufferFormat);
		// Create a shared memory buffer for light attenuation so we can share it with other render target textures
		// Since light attenuation uses have a very short lifetime
		LightAttenuationMemoryBuffer = RHICreateSharedMemory(LightAttenuationSize);

		RenderTargets[LightAttenuation0].Texture = RHICreateSharedTexture2D(BufferSizeX,BufferSizeY,LightAttenuationBufferFormat,1,LightAttenuationMemoryBuffer,TexCreate_ResolveTargetable);
		RenderTargets[LightAttenuation0].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,LightAttenuationBufferFormat,RenderTargets[LightAttenuation0].Texture,MultiSampleFlag,TEXT("LightAttenuation0"));

		// LightAttenuation1 is only used with one pass dominant lights, it provides the ability to have more shadow casting lights handled correctly in the base pass
		if (GOnePassDominantLight)
		{
			RenderTargets[LightAttenuation1].Texture = RHICreateSharedTexture2D(BufferSizeX,BufferSizeY,LightAttenuationBufferFormat,1,SceneColorMemoryBuffer,TexCreate_ResolveTargetable);
			RenderTargets[LightAttenuation1].Surface = RHICreateTargetableSurface(
				BufferSizeX,BufferSizeY,LightAttenuationBufferFormat,RenderTargets[LightAttenuation1].Texture,MultiSampleFlag,TEXT("LightAttenuation1"));
		}
#else

#if !PS3
		// Create a texture to store the resolved light attenuation values, and a render-targetable surface to hold the unresolved light attenuation values.
		RenderTargets[LightAttenuation0].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,LightAttenuationBufferFormat,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[LightAttenuation0].Surface = RHICreateTargetableSurface(
			BufferSizeX,BufferSizeY,LightAttenuationBufferFormat,RenderTargets[LightAttenuation0].Texture,MultiSampleFlag,TEXT("LightAttenuation0"));
#endif

		if (GOnePassDominantLight)
		{
			// Create a texture to store the resolved light attenuation values, and a render-targetable surface to hold the unresolved light attenuation values.
			RenderTargets[LightAttenuation1].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,LightAttenuationBufferFormat,1,TexCreate_ResolveTargetable,NULL);
			RenderTargets[LightAttenuation1].Surface = RHICreateTargetableSurface(
				BufferSizeX,BufferSizeY,LightAttenuationBufferFormat,RenderTargets[LightAttenuation1].Texture,MultiSampleFlag,TEXT("LightAttenuation1"));
		}
#endif

		{
			const EPixelFormat LUTFormat = PF_A8R8G8B8;
			UINT Extend = 16;
			UINT Width = Extend * Extend;
			UINT Height = Extend;
#if XBOX && !USE_NULL_RHI
			RenderTargets[LUTBlend].TextureArray = RHICreateTexture2DArray(Extend, Extend, Extend, LUTFormat, 1, TexCreate_ResolveTargetable, NULL);

			// * 2 : SourceX needs to be 32 pixel aligned as this is a X360 Resolve() requirement
			RenderTargets[LUTBlend].Surface = RHICreateTargetableSurface(Width * 2, Height, LUTFormat, 0, MultiSampleFlag, TEXT("LUTBlend"));
#else
			// Create a texture to store the resolved light attenuation values, and a render-targetable surface to hold the unresolved light attenuation values.
			RenderTargets[LUTBlend].Texture = RHICreateTexture2D(Width, Height, LUTFormat, 1, TexCreate_ResolveTargetable, NULL);
			RenderTargets[LUTBlend].Surface = RHICreateTargetableSurface(Width, Height, LUTFormat, RenderTargets[LUTBlend].Texture, MultiSampleFlag, TEXT("LUTBlend"));
#endif
		}

		PrevPerBoneMotionBlur.SetTexelSizeAndInitResource(4096);

#if !PS3
		// Reuse the light attenuation texture/surface for hit proxies.
		RenderTargets[HitProxy] = RenderTargets[LightAttenuation0];
#endif

		// Create the filter targetable texture and surface.
#if XBOX && !USE_NULL_RHI
		// Share memory with the light attenuation buffer since uses of the filter texture don't overlap with uses of the light attenuation buffer
		// when SeperatableBlur is on FilterColor:DOF, FilterColor2:Bloom
		RenderTargets[FilterColor].Texture = RHICreateSharedTexture2D(FilterBufferSizeX,FilterBufferSizeY,PF_A16B16G16R16,1,LightAttenuationMemoryBuffer,TexCreate_ResolveTargetable);
		RenderTargets[FilterColor].Surface = RHICreateTargetableSurface(
			FilterBufferSizeX,FilterBufferSizeY,PF_A16B16G16R16,RenderTargets[FilterColor].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("FilterColor"));

		RenderTargets[FilterColor2].Texture = RHICreateTexture2D(FilterBufferSizeX,FilterBufferSizeY,PF_A16B16G16R16,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[FilterColor2].Surface = RHICreateTargetableSurface(
			FilterBufferSizeX,FilterBufferSizeY,PF_A16B16G16R16,RenderTargets[FilterColor2].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("FilterColor2"));

		RenderTargets[FilterColor3].Texture = RHICreateTexture2D(FilterBufferSizeX,FilterBufferSizeY,PF_A16B16G16R16,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[FilterColor3].Surface = RHICreateTargetableSurface(
			FilterBufferSizeX,FilterBufferSizeY,PF_A16B16G16R16,RenderTargets[FilterColor3].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("FilterColor3"));
#else
		// Using 16 bit fixed point for PC as it is filterable and gives the necessary precision
		// Using fp16 for PS3 as it is filterable and gives the necessary precision
		const EPixelFormat FilterBufferFormat = PF_A16B16G16R16;

		RenderTargets[FilterColor].Texture = RHICreateTexture2D(FilterBufferSizeX,FilterBufferSizeY,FilterBufferFormat,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[FilterColor].Surface = RHICreateTargetableSurface(
			FilterBufferSizeX,FilterBufferSizeY,FilterBufferFormat,RenderTargets[FilterColor].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("FilterColor"));

		RenderTargets[FilterColor2].Texture = RHICreateTexture2D(FilterBufferSizeX,FilterBufferSizeY,FilterBufferFormat,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[FilterColor2].Surface = RHICreateTargetableSurface(
			FilterBufferSizeX,FilterBufferSizeY,FilterBufferFormat,RenderTargets[FilterColor2].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("FilterColor2"));

		RenderTargets[FilterColor3].Texture = RHICreateTexture2D(FilterBufferSizeX,FilterBufferSizeY,FilterBufferFormat,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[FilterColor3].Surface = RHICreateTargetableSurface(
			FilterBufferSizeX,FilterBufferSizeY,FilterBufferFormat,RenderTargets[FilterColor3].Texture,TargetSurfCreate_Dedicated|MultiSampleFlag,TEXT("FilterColor3"));
#endif

		const EPixelFormat TranslucencyBufferTextureFormat = PF_FloatRGBA;
#if XBOX
		const EPixelFormat TranslucencyBufferSurfaceFormat = PF_A16B16G16R16;
#else
		const EPixelFormat TranslucencyBufferSurfaceFormat = TranslucencyBufferTextureFormat;
#endif

		// Create a dedicated render target for rendering to the downsampled translucency buffer,
		// is also used to store half resolution scene for postprocessing (faster bloom)
		// TODO: Can we overlap with another buffer (e.g. light attenuation buffer)?
		{
#if XBOX && !USE_NULL_RHI
			extern SIZE_T XeCalculateTextureBytes(DWORD SizeX,DWORD SizeY,DWORD SizeZ,BYTE Format);
			SIZE_T BufferSize	= XeCalculateTextureBytes(TranslucencyBufferSizeX, TranslucencyBufferSizeY, 1, TranslucencyBufferTextureFormat);
			// Overlay the velocity buffer and the quarter screen resolve buffer (used by the low quality postprocess filter)
			FSharedMemoryResourceRHIRef TranslucencyMemoryBuffer = RHICreateSharedMemory(BufferSize);

			RenderTargets[TranslucencyBuffer].Texture = RHICreateSharedTexture2D(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferTextureFormat,1,TranslucencyMemoryBuffer,TexCreate_ResolveTargetable);
			RenderTargets[HalfResPostProcess].Texture = RHICreateSharedTexture2D(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferTextureFormat,1,TranslucencyMemoryBuffer,TexCreate_ResolveTargetable);

			RenderTargets[TranslucencyBuffer].Surface = RHICreateTargetableSurface(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferSurfaceFormat,RenderTargets[TranslucencyBuffer].Texture,TargetSurfCreate_Dedicated,TEXT("TranslucencyBuffer"));
			RenderTargets[HalfResPostProcess].Surface = RHICreateTargetableSurface(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferSurfaceFormat,RenderTargets[HalfResPostProcess].Texture,TargetSurfCreate_Dedicated,TEXT("HalfResPostProcess"));
#elif PS3
			RenderTargets[TranslucencyBuffer].Texture = RHICreateTexture2D(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferTextureFormat,1,TexCreate_ResolveTargetable,NULL);
			RenderTargets[TranslucencyBuffer].Surface = RHICreateTargetableSurface(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferSurfaceFormat,RenderTargets[TranslucencyBuffer].Texture,TargetSurfCreate_Dedicated,TEXT("TranslucencyBuffer"));

			// The second translucency buffer is shared with the first, as the two only used in a serial chain
			// this requires some adjustment in another place: search for "DoDownSample"
			// it could be made working for other platforms as well
			RenderTargets[HalfResPostProcess] = RenderTargets[TranslucencyBuffer];
#else
			RenderTargets[TranslucencyBuffer].Texture = RHICreateTexture2D(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferTextureFormat,1,TexCreate_ResolveTargetable,NULL);
			RenderTargets[HalfResPostProcess].Texture = RHICreateTexture2D(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferTextureFormat,1,TexCreate_ResolveTargetable,NULL);

			RenderTargets[TranslucencyBuffer].Surface = RHICreateTargetableSurface(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferSurfaceFormat,RenderTargets[TranslucencyBuffer].Texture,TargetSurfCreate_Dedicated,TEXT("TranslucencyBuffer"));
			RenderTargets[HalfResPostProcess].Surface = RHICreateTargetableSurface(TranslucencyBufferSizeX,TranslucencyBufferSizeY,TranslucencyBufferSurfaceFormat,RenderTargets[HalfResPostProcess].Texture,TargetSurfCreate_Dedicated,TEXT("HalfResPostProcess"));
#endif
		}

		AllocateAOBuffers();

		// Set up velocity buffer / quarter size scene color shared texture
		DWORD VelocityBufferFormat	= PF_A8R8G8B8;
#if XBOX && !USE_NULL_RHI
		VelocityBufferSizeX	= BufferSizeX / 2;
		VelocityBufferSizeY	= BufferSizeY / 2;
		const DWORD DownsampledBufferFormat = PF_A2B10G10R10;
		extern SIZE_T XeCalculateTextureBytes(DWORD SizeX,DWORD SizeY,DWORD SizeZ,BYTE Format);
		SIZE_T VelocityBufferSize	= GSystemSettings.RenderThreadSettings.bAllowMotionBlur ? XeCalculateTextureBytes(VelocityBufferSizeX,VelocityBufferSizeY,1,VelocityBufferFormat) : 0;
		SIZE_T QuarterBufferSize	= XeCalculateTextureBytes(BufferSizeX / SmallColorDepthDownsampleFactor,BufferSizeY / SmallColorDepthDownsampleFactor,1,DownsampledBufferFormat);
		SIZE_T SharedVelocitySize	= Max(VelocityBufferSize,QuarterBufferSize);

		// Overlay the velocity buffer and the quarter screen resolve buffer (used by the low quality postprocess filter)
		FSharedMemoryResourceRHIRef VelocityMemoryBuffer = RHICreateSharedMemory(SharedVelocitySize);

		// Create a shared texture to store a 2x downsampled render target
		extern IDirect3DSurface9* GD3DBackBufferResolveSource;
		RenderTargets[QuarterSizeSceneColor].Texture = RHICreateSharedTexture2D(BufferSizeX / SmallColorDepthDownsampleFactor,BufferSizeY / SmallColorDepthDownsampleFactor,DownsampledBufferFormat,1,VelocityMemoryBuffer,TexCreate_ResolveTargetable);
		RenderTargets[QuarterSizeSceneColor].Surface = GD3DBackBufferResolveSource;

		// Create a small render target overlapping with the velocity buffer
		// This will be used when rendering height fog during ambient occlusion downsampling
		// 8 bits per channel for storing the fog factors of 4 neighboring pixels
		DWORD FogBufferFormat = PF_A8R8G8B8;
		RenderTargets[FogBuffer].Texture = RHICreateSharedTexture2D(BufferSizeX/2,BufferSizeY/2,FogBufferFormat,1,VelocityMemoryBuffer,TexCreate_ResolveTargetable);
		RenderTargets[FogBuffer].Surface = RHICreateTargetableSurface(BufferSizeX/2,BufferSizeY/2,FogBufferFormat,RenderTargets[FogBuffer].Texture,MultiSampleFlag,TEXT("FogBuffer"));

		// Create a buffer for the translucency DoF blur value. 
		// This will be created overlapping the depth buffer so we can use the stencil bits.
		const DWORD DoFBlurBufferFormat = PF_A8R8G8B8;
		const DWORD DoFBlurResolveFormat = PF_G8;
		const SIZE_T DoFBlurResolveBufferSize = XeCalculateTextureBytes(BufferSizeX,BufferSizeY,1,DoFBlurResolveFormat);
		const DWORD TranslucencyDominantLightAttenuationFormat = PF_G8;
		const SIZE_T TranslucencyDominantLightAttenuationBufferSize = XeCalculateTextureBytes(BufferSizeX,BufferSizeY,1,TranslucencyDominantLightAttenuationFormat);

		FSharedMemoryResourceRHIRef DoFBlurResolveBuffer = RHICreateSharedMemory(Max(TranslucencyDominantLightAttenuationBufferSize, DoFBlurResolveBufferSize));
		RenderTargets[DoFBlurBuffer].Texture = RHICreateSharedTexture2D(BufferSizeX,BufferSizeY,DoFBlurResolveFormat,1,DoFBlurResolveBuffer,TexCreate_ResolveTargetable);
		RenderTargets[DoFBlurBuffer].Surface = RHICreateTargetableSurface(BufferSizeX,BufferSizeY,DoFBlurBufferFormat,RenderTargets[DoFBlurBuffer].Texture,TargetSurfCreate_Dedicated,TEXT("DoFBlurBuffer"));

#if !DISABLE_TRANSLUCENCY_DOMINANT_LIGHT_ATTENUATION
		// Create a texture to store dominant light screenspace shadow factors after the light pass is completed,
		// So that translucency (which is rendered later) can re-use the shadow factors projected onto opaque pixels if desired.
		RenderTargets[TranslucencyDominantLightAttenuation].Texture = RHICreateSharedTexture2D(BufferSizeX,BufferSizeY,TranslucencyDominantLightAttenuationFormat,1,DoFBlurResolveBuffer,TexCreate_ResolveTargetable);
		RenderTargets[TranslucencyDominantLightAttenuation].Surface = NULL;
#endif

#else
		VelocityBufferSizeX	= BufferSizeX;
		VelocityBufferSizeY	= BufferSizeY;

#if !DISABLE_TRANSLUCENCY_DOMINANT_LIGHT_ATTENUATION
		// Only the R channel is needed, but can't use PF_G8 since that is not supported on all PC video cards
		const DWORD TranslucencyDominantLightAttenuationFormat = PF_A8R8G8B8;
		RenderTargets[TranslucencyDominantLightAttenuation].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,TranslucencyDominantLightAttenuationFormat,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[TranslucencyDominantLightAttenuation].Surface = NULL;
#endif

#endif

		// Is motion blur allowed?
		if ( GSystemSettings.RenderThreadSettings.bAllowMotionBlur )
		{
			// Create a texture to store the resolved velocity 2d-vectors, and a render-targetable surface to hold them.
#if XBOX && !USE_NULL_RHI
			RenderTargets[VelocityBuffer].Texture = RHICreateSharedTexture2D(VelocityBufferSizeX,VelocityBufferSizeY,VelocityBufferFormat,1,VelocityMemoryBuffer,TexCreate_ResolveTargetable);
#else
			RenderTargets[VelocityBuffer].Texture = RHICreateTexture2D(VelocityBufferSizeX,VelocityBufferSizeY,VelocityBufferFormat,1,TexCreate_ResolveTargetable,NULL);
#endif
			RenderTargets[VelocityBuffer].Surface = RHICreateTargetableSurface(
				VelocityBufferSizeX,VelocityBufferSizeY,VelocityBufferFormat,RenderTargets[VelocityBuffer].Texture,MultiSampleFlag,TEXT("VelocityBuffer"));
		}

		// Are dynamic shadows allowed?
		if ( GSystemSettings.RenderThreadSettings.bAllowDynamicShadows )
		{
			const FIntPoint ObjectShadowBufferResolution = GetShadowDepthTextureResolution(FALSE);
			const FIntPoint TranslucencyShadowBufferSize = GetTranslucencyShadowDepthTextureResolution();
			const FIntPoint PreshadowCacheBufferSize = GetPreshadowCacheTextureResolution();
			#if !PS3
				if ( !GSupportsDepthTextures && !IsValidRef(RenderTargets[ShadowDepthColor].Surface) )
				{
					//create the shadow depth color surface
					//platforms with GSupportsDepthTextures don't need a depth color target
					//platforms with GSupportsHardwarePCF still need a color target due to API restrictions (except PS3)
					RenderTargets[ShadowDepthColor].Texture = RHICreateTexture2D(ObjectShadowBufferResolution.X,ObjectShadowBufferResolution.Y,PF_R32F,1,TexCreate_ResolveTargetable,NULL);
					RenderTargets[ShadowDepthColor].Surface = RHICreateTargetableSurface(
						ObjectShadowBufferResolution.X,ObjectShadowBufferResolution.Y,PF_R32F,RenderTargets[ShadowDepthColor].Texture,0,TEXT("ShadowDepthRT"));
				}
				if ( !GSupportsDepthTextures && !IsValidRef(RenderTargets[DominantShadowDepthColor].Surface) )
				{
					//create the shadow depth color surface
					//platforms with GSupportsDepthTextures don't need a depth color target
					//platforms with GSupportsHardwarePCF still need a color target due to API restrictions (except PS3)
					RenderTargets[DominantShadowDepthColor].Texture = RHICreateTexture2D(WholeSceneDominantShadowBufferResolution.X,WholeSceneDominantShadowBufferResolution.Y,PF_R32F,1,TexCreate_ResolveTargetable,NULL);
					RenderTargets[DominantShadowDepthColor].Surface = RHICreateTargetableSurface(
						WholeSceneDominantShadowBufferResolution.X,WholeSceneDominantShadowBufferResolution.Y,PF_R32F,RenderTargets[DominantShadowDepthColor].Texture,0,TEXT("ShadowDepthRT"));
				}
				if ( !GSupportsDepthTextures && !IsValidRef(RenderTargets[TranslucencyShadowDepthColor].Surface) )
				{
					//create the shadow depth color surface
					//platforms with GSupportsDepthTextures don't need a depth color target
					//platforms with GSupportsHardwarePCF still need a color target due to API restrictions (except PS3)
					RenderTargets[TranslucencyShadowDepthColor].Texture = RHICreateTexture2D(TranslucencyShadowBufferSize.X,TranslucencyShadowBufferSize.Y,PF_R32F,1,TexCreate_ResolveTargetable,NULL);
					RenderTargets[TranslucencyShadowDepthColor].Surface = RHICreateTargetableSurface(
						TranslucencyShadowBufferSize.X,TranslucencyShadowBufferSize.Y,PF_R32F,RenderTargets[TranslucencyShadowDepthColor].Texture,0,TEXT("TranslucencyShadowDepthColor"));
				}

				if ( !GSupportsDepthTextures && !IsValidRef(RenderTargets[PreshadowCacheDepthColor].Surface) )
				{
					//create the shadow depth color surface
					//platforms with GSupportsDepthTextures don't need a depth color target
					//platforms with GSupportsHardwarePCF still need a color target due to API restrictions (except PS3)
					RenderTargets[PreshadowCacheDepthColor].Texture = RHICreateTexture2D(PreshadowCacheBufferSize.X,PreshadowCacheBufferSize.Y,PF_R32F,1,TexCreate_ResolveTargetable,NULL);
					RenderTargets[PreshadowCacheDepthColor].Surface = RHICreateTargetableSurface(
						PreshadowCacheBufferSize.X,PreshadowCacheBufferSize.Y,PF_R32F,RenderTargets[PreshadowCacheDepthColor].Texture,0,TEXT("PreshadowCacheDepthColor"));
				}
			#endif

			if (GRHIShaderPlatform == SP_PCD3D_SM5)
			{
				// Create several shadow depth cube maps with different resolutions, to handle different sized shadows on the screen
				for (INT SurfaceIndex = 0; SurfaceIndex < NumCubeShadowDepthSurfaces; SurfaceIndex++)
				{
					const INT SurfaceResolution = GetCubeShadowDepthZResolution(SurfaceIndex);

					// Create a cube texture
					RenderTargets[CubeShadowDepthZ0 + SurfaceIndex].TextureCube = RHICreateTextureCube(
						SurfaceResolution,PF_ShadowDepth,1,TexCreate_DepthStencil,NULL);

					// Create a cube surface, where the cube faces can be indexed in a geometry shader
					RenderTargets[CubeShadowDepthZ0 + SurfaceIndex].Surface = RHICreateTargetableCubeSurface(
						SurfaceResolution,
						PF_ShadowDepth,
						RenderTargets[CubeShadowDepthZ0 + SurfaceIndex].TextureCube,
						CubeFace_MAX,
						0,
						TEXT("CubeShadowDepthZ")
						);
				}
			}

			//create the shadow depth texture and/or surface
			if (IsHardwarePCFSupported())
			{
				// Create a depth texture, used to sample PCF values
				RenderTargets[ShadowDepthZ].Texture = RHICreateTexture2D(
					ObjectShadowBufferResolution.X,ObjectShadowBufferResolution.Y,PF_FilteredShadowDepth,1,TexCreate_DepthStencil,NULL);
    
				// Don't create a dedicated surface
				RenderTargets[ShadowDepthZ].Surface = RHICreateTargetableSurface(
					ObjectShadowBufferResolution.X,
					ObjectShadowBufferResolution.Y,
					PF_FilteredShadowDepth,
					RenderTargets[ShadowDepthZ].Texture,
					0,
					TEXT("ShadowDepthZ")
					);

				RenderTargets[DominantShadowDepthZ].Texture = RHICreateTexture2D(
					WholeSceneDominantShadowBufferResolution.X,WholeSceneDominantShadowBufferResolution.Y,PF_FilteredShadowDepth,1,TexCreate_DepthStencil,NULL);

				RenderTargets[DominantShadowDepthZ].Surface = RHICreateTargetableSurface(
					WholeSceneDominantShadowBufferResolution.X,
					WholeSceneDominantShadowBufferResolution.Y,
					PF_FilteredShadowDepth,
					RenderTargets[DominantShadowDepthZ].Texture,
					0,
					TEXT("DominantShadowDepthZ")
					);

				RenderTargets[TranslucencyShadowDepthZ].Texture = RHICreateTexture2D(
					TranslucencyShadowBufferSize.X,TranslucencyShadowBufferSize.Y,PF_FilteredShadowDepth,1,TexCreate_DepthStencil,NULL);

				RenderTargets[TranslucencyShadowDepthZ].Surface = RHICreateTargetableSurface(
					TranslucencyShadowBufferSize.X,
					TranslucencyShadowBufferSize.Y,
					PF_FilteredShadowDepth,
					RenderTargets[TranslucencyShadowDepthZ].Texture,
					0,
					TEXT("TranslucencyShadowDepthZ")
					);

				RenderTargets[PreshadowCacheDepthZ].Texture = RHICreateTexture2D(
					PreshadowCacheBufferSize.X,PreshadowCacheBufferSize.Y,PF_FilteredShadowDepth,1,TexCreate_DepthStencil,NULL);

				RenderTargets[PreshadowCacheDepthZ].Surface = RHICreateTargetableSurface(
					PreshadowCacheBufferSize.X,
					PreshadowCacheBufferSize.Y,
					PF_FilteredShadowDepth,
					RenderTargets[PreshadowCacheDepthZ].Texture,
					0,
					TEXT("PreshadowCache")
					);
			}
			else if (IsFetch4Supported())
			{
				// Create a D24 depth stencil texture for use with Fetch4 shadows
				RenderTargets[ShadowDepthZ].Texture = RHICreateTexture2D(
					ObjectShadowBufferResolution.X,ObjectShadowBufferResolution.Y,PF_D24,1,TexCreate_DepthStencil,NULL);
    
				// Don't create a dedicated surface
				RenderTargets[ShadowDepthZ].Surface = RHICreateTargetableSurface(
					ObjectShadowBufferResolution.X,
					ObjectShadowBufferResolution.Y,
					PF_D24,
					RenderTargets[ShadowDepthZ].Texture,
					0,
					TEXT("ShadowDepthZ")
					);

				RenderTargets[DominantShadowDepthZ].Texture = RHICreateTexture2D(
					WholeSceneDominantShadowBufferResolution.X,WholeSceneDominantShadowBufferResolution.Y,PF_D24,1,TexCreate_DepthStencil,NULL);

				RenderTargets[DominantShadowDepthZ].Surface = RHICreateTargetableSurface(
					WholeSceneDominantShadowBufferResolution.X,
					WholeSceneDominantShadowBufferResolution.Y,
					PF_D24,
					RenderTargets[DominantShadowDepthZ].Texture,
					0,
					TEXT("DominantShadowDepthZ")
					);

				RenderTargets[TranslucencyShadowDepthZ].Texture = RHICreateTexture2D(
					TranslucencyShadowBufferSize.X,TranslucencyShadowBufferSize.Y,PF_D24,1,TexCreate_DepthStencil,NULL);

				RenderTargets[TranslucencyShadowDepthZ].Surface = RHICreateTargetableSurface(
					TranslucencyShadowBufferSize.X,
					TranslucencyShadowBufferSize.Y,
					PF_D24,
					RenderTargets[TranslucencyShadowDepthZ].Texture,
					0,
					TEXT("TranslucencyShadowDepthZ")
					);

				RenderTargets[PreshadowCacheDepthZ].Texture = RHICreateTexture2D(
					PreshadowCacheBufferSize.X,PreshadowCacheBufferSize.Y,PF_D24,1,TexCreate_DepthStencil,NULL);

				RenderTargets[PreshadowCacheDepthZ].Surface = RHICreateTargetableSurface(
					PreshadowCacheBufferSize.X,
					PreshadowCacheBufferSize.Y,
					PF_D24,
					RenderTargets[PreshadowCacheDepthZ].Texture,
					0,
					TEXT("PreshadowCacheDepthZ")
					);
			}
			else
			{
				if( GSupportsDepthTextures )
				{
					// Create a texture to store the resolved shadow depth
#if XBOX && !USE_NULL_RHI
					// Overlap the shadow depth texture with the light attenuation buffer, since their lifetimes don't overlap
					RenderTargets[ShadowDepthZ].Texture = RHICreateSharedTexture2D(
						ObjectShadowBufferResolution.X,ObjectShadowBufferResolution.Y,PF_ShadowDepth,1,LightAttenuationMemoryBuffer,TexCreate_ResolveTargetable);
					// Share memory with scene color, since their lifetimes don't overlap
					RenderTargets[DominantShadowDepthZ].Texture = RHICreateSharedTexture2D(
						WholeSceneDominantShadowBufferResolution.X,WholeSceneDominantShadowBufferResolution.Y,PF_ShadowDepth,1,SceneColorMemoryBuffer,TexCreate_ResolveTargetable);
#else
					RenderTargets[ShadowDepthZ].Texture = RHICreateTexture2D(
						ObjectShadowBufferResolution.X,ObjectShadowBufferResolution.Y,PF_ShadowDepth,1,TexCreate_ResolveTargetable,NULL);

					RenderTargets[DominantShadowDepthZ].Texture = RHICreateTexture2D(
						WholeSceneDominantShadowBufferResolution.X,WholeSceneDominantShadowBufferResolution.Y,PF_ShadowDepth,1,TexCreate_ResolveTargetable,NULL);
#endif
					RenderTargets[TranslucencyShadowDepthZ].Texture = RHICreateTexture2D(
						TranslucencyShadowBufferSize.X,TranslucencyShadowBufferSize.Y,PF_ShadowDepth,1,TexCreate_ResolveTargetable,NULL);

					RenderTargets[PreshadowCacheDepthZ].Texture = RHICreateTexture2D(
						PreshadowCacheBufferSize.X,PreshadowCacheBufferSize.Y,PF_ShadowDepth,1,TexCreate_ResolveTargetable,NULL);
				}
    
				// Create a dedicated depth-stencil target surface for shadow depth rendering.
				RenderTargets[ShadowDepthZ].Surface = RHICreateTargetableSurface(
					ObjectShadowBufferResolution.X,
					ObjectShadowBufferResolution.Y,
					PF_ShadowDepth,
					RenderTargets[ShadowDepthZ].Texture,
					0,
					TEXT("ShadowDepthZ")
					);

				RenderTargets[DominantShadowDepthZ].Surface = RHICreateTargetableSurface(
					WholeSceneDominantShadowBufferResolution.X,
					WholeSceneDominantShadowBufferResolution.Y,
					PF_ShadowDepth,
					RenderTargets[DominantShadowDepthZ].Texture,
					0,
					TEXT("DominantShadowDepthZ")
					);

				RenderTargets[TranslucencyShadowDepthZ].Surface = RHICreateTargetableSurface(
					TranslucencyShadowBufferSize.X,
					TranslucencyShadowBufferSize.Y,
					PF_ShadowDepth,
					RenderTargets[TranslucencyShadowDepthZ].Texture,
					0,
					TEXT("TranslucencyShadowDepthZ")
					);

				RenderTargets[PreshadowCacheDepthZ].Surface = RHICreateTargetableSurface(
					PreshadowCacheBufferSize.X,
					PreshadowCacheBufferSize.Y,
					PF_ShadowDepth,
					RenderTargets[PreshadowCacheDepthZ].Texture,
					0,
					TEXT("PreshadowCache")
					);
			}
		}
		
		// Are fog volumes allowed?
		if ( GSystemSettings.RenderThreadSettings.bAllowFogVolumes )
		{
#if XBOX && !USE_NULL_RHI
			//have to use a low precision format that supports blending and filtering since the only fp format Xenon can blend to (7e3) doesn't have enough precision
			// Overlap with the light attenuation buffer since both the light attenuation texture and the fog integral textures have short lifetimes that don't overlap
			RenderTargets[FogFrontfacesIntegralAccumulation].Texture = RHICreateSharedTexture2D(FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,PF_A8R8G8B8,1,LightAttenuationMemoryBuffer,TexCreate_ResolveTargetable);
		    RenderTargets[FogFrontfacesIntegralAccumulation].Surface = RHICreateTargetableSurface(
			    FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,PF_A8R8G8B8,RenderTargets[FogFrontfacesIntegralAccumulation].Texture,0,TEXT("FogFrontfacesIntegralAccumulationRT"));

			// Overlap with the velocity buffer since both the velocity texture and the fog integral textures have short lifetimes that don't overlap
			RenderTargets[FogBackfacesIntegralAccumulation].Texture = RHICreateSharedTexture2D(FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,PF_A8R8G8B8,1,VelocityMemoryBuffer,TexCreate_ResolveTargetable);
		    RenderTargets[FogBackfacesIntegralAccumulation].Surface = RHICreateTargetableSurface(
			    FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,PF_A8R8G8B8,RenderTargets[FogBackfacesIntegralAccumulation].Texture,0,TEXT("FogBackfacesIntegralAccumulationRT"));
#elif PS3
			checkf((FogAccumulationBufferSizeX == TranslucencyBufferSizeX) && (FogAccumulationBufferSizeY == TranslucencyBufferSizeY));

			// Overlap with the low-resolution scene color / translucency buffer; as the fog integral has a short lifetime that doesn't overlap either of those
			RenderTargets[FogBackfacesIntegralAccumulation] = RenderTargets[TranslucencyBuffer];
#else
			//FogFrontfacesIntegralAccumulation texture and surface not used
			//allocate the highest precision render target with blending and filtering
			RenderTargets[FogBackfacesIntegralAccumulation].Texture = RHICreateTexture2D(FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,SceneColorBufferFormat,1,TexCreate_ResolveTargetable,NULL);
			RenderTargets[FogBackfacesIntegralAccumulation].Surface = RHICreateTargetableSurface(
				FogAccumulationBufferSizeX,FogAccumulationBufferSizeY,SceneColorBufferFormat,RenderTargets[FogBackfacesIntegralAccumulation].Texture,0,TEXT("FogBackfacesIntegralAccumulationRT"));
#endif
		}

        RenderTargets[StereoFix].Texture = RHICreateStereoFixTexture();

		// Create the D3D11-specific render targets if running on D3D11.
		#if !CONSOLE
			if (GRHIShaderPlatform == SP_PCD3D_SM5)
			{
				// Create a dummy white MSAA surface.
				RenderTargets[WhiteDummy].Texture = RHICreateTexture2D(1,1,PF_A8R8G8B8,1,TexCreate_ResolveTargetable,NULL);
				RenderTargets[WhiteDummy].Surface = RHICreateTargetableSurface(1,1,PF_A8R8G8B8,RenderTargets[WhiteDummy].Texture,MultiSampleFlag,TEXT("WhiteDummy"));
				RHISetRenderTarget(RenderTargets[WhiteDummy].Surface,FSurfaceRHIRef());
				RHIClear(TRUE,FLinearColor(1,1,1,1),FALSE,0,FALSE,0);

				// Create the world-space normal g-buffer.
				const EPixelFormat NormalGBufferFormat   = GSystemSettings.bHighPrecisionGBuffers ? PF_FloatRGBA : PF_A2B10G10R10;
				RenderTargets[WorldNormalGBuffer].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,NormalGBufferFormat,1,TexCreate_ResolveTargetable,NULL);
				RenderTargets[WorldNormalGBuffer].Surface = RHICreateTargetableSurface(
					BufferSizeX,BufferSizeY,NormalGBufferFormat,
					RenderTargets[WorldNormalGBuffer].Texture,
					MultiSampleFlag,
					TEXT("WorldNormalGBuffer")
					);

				// Create the world-space reflection normal g-buffer.
				RenderTargets[WorldReflectionNormalGBuffer].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_A2B10G10R10,1,TexCreate_ResolveTargetable,NULL);
				RenderTargets[WorldReflectionNormalGBuffer].Surface = RHICreateTargetableSurface(
					BufferSizeX,BufferSizeY,PF_A2B10G10R10,
					RenderTargets[WorldReflectionNormalGBuffer].Texture,
					MultiSampleFlag,
					TEXT("WorldReflectionNormalGBuffer")
					);

				// Create the specular color and power g-buffer.
				const EPixelFormat SpecularGBufferFormat = GSystemSettings.bHighPrecisionGBuffers ? PF_FloatRGBA : PF_A8R8G8B8;
				RenderTargets[SpecularGBuffer].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,SpecularGBufferFormat,1,TexCreate_ResolveTargetable,NULL);
				RenderTargets[SpecularGBuffer].Surface = RHICreateTargetableSurface(
					BufferSizeX,BufferSizeY,SpecularGBufferFormat,
					RenderTargets[SpecularGBuffer].Texture,
					MultiSampleFlag,
					TEXT("SpecularGBuffer")
					);

				// Create the diffuse color g-buffer.
				const EPixelFormat DiffuseGBufferFormat  = GSystemSettings.bHighPrecisionGBuffers ? PF_FloatRGBA : PF_A8R8G8B8;
				RenderTargets[DiffuseGBuffer].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,DiffuseGBufferFormat,1,TexCreate_ResolveTargetable,NULL);
				RenderTargets[DiffuseGBuffer].Surface = RHICreateTargetableSurface(
					BufferSizeX,BufferSizeY,DiffuseGBufferFormat,
					RenderTargets[DiffuseGBuffer].Texture,
					MultiSampleFlag,
					TEXT("DiffuseGBuffer")
					);

				// Allocate a half res depth buffer to be used by image reflections
				RenderTargets[ReflectionSmallDepthZ].Surface = RHICreateTargetableSurface(Max<UINT>(BufferSizeX / SmallColorDepthDownsampleFactor,1),Max<UINT>(BufferSizeY / SmallColorDepthDownsampleFactor,1),PF_DepthStencil,FTexture2DRHIRef(),0,TEXT("ReflectionSmallDepth"));

				// Create the subsurface scattering render targets if they are enabled.
				if(GSystemSettings.RenderThreadSettings.bAllowSubsurfaceScattering)
				{
					RenderTargets[SubsurfaceInscattering].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_FloatR11G11B10,1,TexCreate_ResolveTargetable,NULL);
					RenderTargets[SubsurfaceInscattering].Surface = RHICreateTargetableSurface(
						BufferSizeX,BufferSizeY,PF_FloatR11G11B10,
						RenderTargets[SubsurfaceInscattering].Texture,
						MultiSampleFlag,
						TEXT("SubsurfaceInscattering")
						);

					RenderTargets[SubsurfaceScatteringAttenuation].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_A8R8G8B8,1,TexCreate_ResolveTargetable,NULL);
					RenderTargets[SubsurfaceScatteringAttenuation].Surface = RHICreateTargetableSurface(
						BufferSizeX,BufferSizeY,PF_A8R8G8B8,
						RenderTargets[SubsurfaceScatteringAttenuation].Texture,
						MultiSampleFlag,
						TEXT("SubsurfaceScatteringAttenuation")
						);
				}

				// Create the BokehDOF render target.
				RenderTargets[BokehDOF].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_FloatRGBA,1,TexCreate_ResolveTargetable,NULL);
				RenderTargets[BokehDOF].Surface = RHICreateTargetableSurface(
					BufferSizeX,BufferSizeY,PF_FloatRGBA,
					RenderTargets[BokehDOF].Texture,
					0,
					TEXT("BokehDOF")
					);

				if(IsSeparateTranslucencyActive())
				{
					// Create the SeparateTranslucency render target.
					RenderTargets[SeparateTranslucency].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_FloatRGBA,1,TexCreate_ResolveTargetable,NULL);
					RenderTargets[SeparateTranslucency].Surface = RHICreateTargetableSurface(
						BufferSizeX,BufferSizeY,PF_FloatRGBA,
						RenderTargets[SeparateTranslucency].Texture,
						MultiSampleFlag,
						TEXT("SeparateTranslucency")
						);
#if ENABLE_SEPARATE_TRANSLUCENCY_DEPTH
					// Create the SeparateTranslucencyDepth render target.
					RenderTargets[SeparateTranslucencyDepth].Texture = RHICreateTexture2D(BufferSizeX,BufferSizeY,PF_R32F,1,TexCreate_ResolveTargetable,NULL);
					RenderTargets[SeparateTranslucencyDepth].Surface = RHICreateTargetableSurface(
						BufferSizeX,BufferSizeY,PF_R32F,
						RenderTargets[SeparateTranslucencyDepth].Texture,
						MultiSampleFlag,
						TEXT("SeparateTranslucencyDepth")
						);
#endif
				}
			}
		#endif
	}
}

void FSceneRenderTargets::ReleaseDynamicRHI()
{
	// make sure no scene render targets and textures are in use before releasing them
	RHISetRenderTarget(FSurfaceRHIRef(),FSurfaceRHIRef());

	PrevPerBoneMotionBlur.ReleaseResources();

	for( INT RTIdx=0; RTIdx < MAX_SCENE_RENDERTARGETS; RTIdx++ )
	{
		RenderTargets[RTIdx].Texture.SafeRelease();
		RenderTargets[RTIdx].TextureCube.SafeRelease();
		RenderTargets[RTIdx].Surface.SafeRelease();
	}

	BackBuffer.SafeRelease();
}

/** Returns the size of the shadow depth buffer, taking into account platform limitations and game specific resolution limits. */
FIntPoint FSceneRenderTargets::GetShadowDepthTextureResolution(UBOOL bWholeSceneDominantShadow) const
{
	FIntPoint ShadowBufferResolution;
	if (bWholeSceneDominantShadow)
	{
		ShadowBufferResolution = FIntPoint(
			Clamp(GSystemSettings.RenderThreadSettings.MaxWholeSceneDominantShadowResolution,1,GMaxWholeSceneDominantShadowDepthBufferSize),
			Clamp(GSystemSettings.RenderThreadSettings.MaxWholeSceneDominantShadowResolution,1,GMaxWholeSceneDominantShadowDepthBufferSize));
	}
	else
	{
		ShadowBufferResolution = FIntPoint(
			Clamp(GSystemSettings.RenderThreadSettings.MaxShadowResolution,1,GMaxPerObjectShadowDepthBufferSizeX),
			Clamp(GSystemSettings.RenderThreadSettings.MaxShadowResolution,1,GMaxPerObjectShadowDepthBufferSizeY));
	}
	// Shadow buffer should be larger or equal in x than in y, since per-object shadows are always oriented with their longest dimension along the x axis
	checkSlow(ShadowBufferResolution.X >= ShadowBufferResolution.Y);
	return ShadowBufferResolution;
}

/** Returns the dimensions of the translucency shadow depth texture and surface. */
FIntPoint FSceneRenderTargets::GetTranslucencyShadowDepthTextureResolution() const
{
	const FIntPoint ShadowBufferResolution = GetShadowDepthTextureResolution(FALSE);
	return FIntPoint(appTrunc(GSystemSettings.PreShadowResolutionFactor * ShadowBufferResolution.X), appTrunc(GSystemSettings.PreShadowResolutionFactor * ShadowBufferResolution.Y));
}

/** Returns the dimensions of the preshadow cache texture and surface. */
FIntPoint FSceneRenderTargets::GetPreshadowCacheTextureResolution() const
{
	const FIntPoint ShadowBufferResolution = GetShadowDepthTextureResolution(FALSE);
	return FIntPoint(appTrunc(GSystemSettings.PreShadowResolutionFactor * ShadowBufferResolution.X), appTrunc(GSystemSettings.PreShadowResolutionFactor * ShadowBufferResolution.Y));
}

/** Returns an index in the range [0, NumCubeShadowDepthSurfaces) given an input resolution. */
INT FSceneRenderTargets::GetCubeShadowDepthZIndex(INT ShadowResolution) const
{
	FIntPoint ObjectShadowBufferResolution = GetShadowDepthTextureResolution(FALSE);

	// Use a lower resolution because cubemaps use a lot of memory
	ObjectShadowBufferResolution.X /= 2;
	ObjectShadowBufferResolution.Y /= 2;
	const INT SurfaceSizes[NumCubeShadowDepthSurfaces] =
	{
		ObjectShadowBufferResolution.X,
		ObjectShadowBufferResolution.X / 2,
		ObjectShadowBufferResolution.X / 4,
		ObjectShadowBufferResolution.X / 8,
		GSystemSettings.MinShadowResolution
	};

	for (INT SearchIndex = 0; SearchIndex < NumCubeShadowDepthSurfaces; SearchIndex++)
	{
		if (ShadowResolution >= SurfaceSizes[SearchIndex])
		{
			return SearchIndex;
		}
	}

	check(0);
	return 0;
}

/** Returns the appropriate resolution for a given cube shadow index. */
INT FSceneRenderTargets::GetCubeShadowDepthZResolution(INT ShadowIndex) const
{
	checkSlow(ShadowIndex >= 0 && ShadowIndex < NumCubeShadowDepthSurfaces);

	FIntPoint ObjectShadowBufferResolution = GetShadowDepthTextureResolution(FALSE);

	// Use a lower resolution because cubemaps use a lot of memory
	ObjectShadowBufferResolution.X /= 2;
	ObjectShadowBufferResolution.Y /= 2;
	const INT SurfaceSizes[NumCubeShadowDepthSurfaces] =
	{
		ObjectShadowBufferResolution.X,
		ObjectShadowBufferResolution.X / 2,
		ObjectShadowBufferResolution.X / 4,
		ObjectShadowBufferResolution.X / 8,
		GSystemSettings.MinShadowResolution
	};
	return SurfaceSizes[ShadowIndex];
}

UBOOL FSceneRenderTargets::IsHardwarePCFSupported() const 
{
#if PS3
	return GSupportsHardwarePCF;
#else
	return GSystemSettings.RenderThreadSettings.bAllowHardwareShadowFiltering && GSupportsHardwarePCF;
#endif
}

const FTexture2DRHIRef& FSceneRenderTargets::GetRenderTargetTexture(ESceneRenderTargetTypes EnumIndex) const
{ 
	INT Index = (INT)EnumIndex;
	
	if(Index >= 0 && Index < MAX_SCENE_RENDERTARGETS)
	{
		return RenderTargets[Index].Texture;
	}
	else
	{
		static FTexture2DRHIRef Null;

		return Null;
	}
}

const FSurfaceRHIRef& FSceneRenderTargets::GetRenderTargetSurface(ESceneRenderTargetTypes EnumIndex) const
{ 
	INT Index = (INT)EnumIndex;

	if(Index >= 0 && Index < MAX_SCENE_RENDERTARGETS)
	{
		return RenderTargets[Index].Surface;
	}
	else
	{
		static FSurfaceRHIRef Null;

		return Null;
	}
}

FString FSceneRenderTargets::GetRenderTargetInfo(ESceneRenderTargetTypes EnumIndex, FIntPoint &OutExtent) const
{
	INT Index = (INT)EnumIndex;

	OutExtent = FIntPoint(0, 0);

	if(Index < MAX_SCENE_RENDERTARGETS)
	{
		// can be updated incrementally - only used for debugging
		switch(Index)
		{
			case SceneColor:
			case SceneColorRaw:
			case SceneColorFixedPoint:
			case SceneDepthZ:
			case DoFBlurBuffer:
			case TranslucencyDominantLightAttenuation:
			case LightAttenuation0:
			case LightAttenuation1:
			case SubsurfaceInscattering:
			case SubsurfaceScatteringAttenuation:
			case WorldNormalGBuffer:
			case WorldReflectionNormalGBuffer:
			case SpecularGBuffer:
			case DiffuseGBuffer:
			case BokehDOF:
			case SeparateTranslucency:
			case SeparateTranslucencyDepth:
				OutExtent = FIntPoint(BufferSizeX, BufferSizeY);
				break;

			case TranslucencyShadowDepthZ:
			case TranslucencyShadowDepthColor:
			case PreshadowCacheDepthZ:
			case PreshadowCacheDepthColor:
				OutExtent = GetTranslucencyShadowDepthTextureResolution();
				break;

			case TranslucencyBuffer:
			case HalfResPostProcess:
				OutExtent = FIntPoint(TranslucencyBufferSizeX, TranslucencyBufferSizeY);
				break;

			case FilterColor:
			case FilterColor2:
			case FilterColor3:
				OutExtent = FIntPoint(FilterBufferSizeX, FilterBufferSizeY);
				break;

			case VelocityBuffer:
				OutExtent = FIntPoint(VelocityBufferSizeX, VelocityBufferSizeY);
				break;

			case FogFrontfacesIntegralAccumulation:
			case FogBackfacesIntegralAccumulation:
				OutExtent = FIntPoint(FogAccumulationBufferSizeX, FogAccumulationBufferSizeY);
				break;

			case AOInput:
			case AOOutput:
			case AOHistory:
				OutExtent = FIntPoint(AOBufferSizeX, AOBufferSizeY);
				break;

			case WhiteDummy:
				OutExtent = FIntPoint(1, 1);
				break;

			case CubeShadowDepthZ0:
			case CubeShadowDepthZ1:
			case CubeShadowDepthZ2:
			case CubeShadowDepthZ3:
			case CubeShadowDepthZ4:
				{
					INT SurfaceIndex = (INT)Index - (INT)CubeShadowDepthZ0;
					const INT SurfaceResolution = GetCubeShadowDepthZResolution(SurfaceIndex);

					OutExtent = FIntPoint(SurfaceResolution, SurfaceResolution);
				}
				break;
		}

		return GetRenderTargetName((ESceneRenderTargetTypes)Index);
	}

	return TEXT("");
}

void FSceneRenderTargets::SetAODownsampleFactor(INT NewDownsampleFactor)
{
	check(IsInRenderingThread());
	AOBufferSizeX = Max< UINT >( 1, BufferSizeX / NewDownsampleFactor );
	AOBufferSizeY = Max< UINT >( 1, BufferSizeY / NewDownsampleFactor );

	if (NewDownsampleFactor != AODownsampleFactor)
	{
		AODownsampleFactor = NewDownsampleFactor;
		if (IsInitialized())
		{
			RenderTargets[AOInput].Texture.SafeRelease();
			RenderTargets[AOInput].Surface.SafeRelease();
			RenderTargets[AOOutput].Texture.SafeRelease();
			RenderTargets[AOOutput].Surface.SafeRelease();
			RenderTargets[AOHistory].Texture.SafeRelease();
			RenderTargets[AOHistory].Surface.SafeRelease();
		}
		AllocateAOBuffers();
	}
}

void FSceneRenderTargets::AllocateAOBuffers()
{
	// Allocate render targets needed for ambient occlusion calculations if allowed
	//@todo - actually check if there is an ambient occlusion effect that is going to use these before allocating,
	// also only allocate the history buffers if they are going to be used.
	if ( GSystemSettings.RenderThreadSettings.bAllowAmbientOcclusion 
		&& AOBufferSizeX > 0
		&& AOBufferSizeY > 0 )
	{
		const EPixelFormat InputFormat = PF_G16R16F;
		// _FILTER is required to support bilinear filtered lookup on X360 without artifacts
		const EPixelFormat OutputFormat = PF_G16R16F_FILTER;
		// @todo: use fixed point on ATI cards for filtering support
		const EPixelFormat HistoryFormat = PF_G16R16F_FILTER;

		// Create a dedicated render target for ambient occlusion, since it will be filtered in several passes.
#if XBOX && !USE_NULL_RHI
		// Overlap with the light attenuation buffer since their lifetimes don't overlap
		RenderTargets[AOInput].Texture = RHICreateSharedTexture2D(AOBufferSizeX,AOBufferSizeY,InputFormat,1, LightAttenuationMemoryBuffer ,TexCreate_ResolveTargetable);
		RenderTargets[AOOutput].Texture = RHICreateSharedTexture2D(AOBufferSizeX,AOBufferSizeY,OutputFormat,1, LightAttenuationMemoryBuffer ,TexCreate_ResolveTargetable);
		RenderTargets[AOInput].Surface = RHICreateTargetableSurface(
			AOBufferSizeX,AOBufferSizeY,InputFormat,RenderTargets[AOInput].Texture,TargetSurfCreate_Dedicated,TEXT("AmbientOcclusion"));
		RenderTargets[AOOutput].Surface = RHICreateTargetableSurface(
			AOBufferSizeX,AOBufferSizeY,OutputFormat,RenderTargets[AOOutput].Texture,TargetSurfCreate_Dedicated,TEXT("AmbientOcclusion"));
#else
		RenderTargets[AOInput].Texture = RHICreateTexture2D(AOBufferSizeX,AOBufferSizeY,InputFormat,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[AOOutput].Texture = RenderTargets[AOInput].Texture;
		RenderTargets[AOInput].Surface = RHICreateTargetableSurface(
			AOBufferSizeX,AOBufferSizeY,InputFormat,RenderTargets[AOInput].Texture,TargetSurfCreate_Dedicated,TEXT("AmbientOcclusion"));
		RenderTargets[AOOutput].Surface = RenderTargets[AOInput].Surface;
#endif

		// Create a dedicated render target for ambient occlusion history, since we will be reading from the history and writing to it in the same draw call.
		RenderTargets[AOHistory].Texture = RHICreateTexture2D(AOBufferSizeX,AOBufferSizeY,HistoryFormat,1,TexCreate_ResolveTargetable,NULL);
		RenderTargets[AOHistory].Surface = RHICreateTargetableSurface(
			AOBufferSizeX,AOBufferSizeY,HistoryFormat,RenderTargets[AOHistory].Texture,TargetSurfCreate_Dedicated,TEXT("AOHistory"));

		bAOHistoryNeedsCleared = TRUE;
	}
}

/** Returns a string matching the given ESceneRenderTargetTypes */
FString FSceneRenderTargets::GetRenderTargetName(ESceneRenderTargetTypes RTEnum) const
{
	FString RenderTargetName;
#define RTENUMNAME(x) case x: RenderTargetName = TEXT(#x); break;
	switch(RTEnum)
	{
		RTENUMNAME(FilterColor)
		RTENUMNAME(SceneColor)
		RTENUMNAME(SceneColorRaw)
		RTENUMNAME(SceneColorFixedPoint)
		RTENUMNAME(SceneDepthZ)
		RTENUMNAME(SmallDepthZ)
		RTENUMNAME(ReflectionSmallDepthZ)
		RTENUMNAME(ShadowDepthZ)
		RTENUMNAME(DominantShadowDepthZ)
		RTENUMNAME(TranslucencyShadowDepthZ)
		RTENUMNAME(PreshadowCacheDepthZ)
		RTENUMNAME(ShadowDepthColor)
		RTENUMNAME(DominantShadowDepthColor)
		RTENUMNAME(TranslucencyShadowDepthColor)
		RTENUMNAME(PreshadowCacheDepthColor)
		RTENUMNAME(LightAttenuation0)
		RTENUMNAME(LightAttenuation1)
		RTENUMNAME(TranslucencyBuffer)
		RTENUMNAME(HalfResPostProcess)
		RTENUMNAME(TranslucencyDominantLightAttenuation)
		RTENUMNAME(AOInput)
		RTENUMNAME(AOOutput)
		RTENUMNAME(AOHistory)
		RTENUMNAME(VelocityBuffer)
		RTENUMNAME(QuarterSizeSceneColor)
		RTENUMNAME(FogFrontfacesIntegralAccumulation)
		RTENUMNAME(FogBackfacesIntegralAccumulation)
		RTENUMNAME(HitProxy)
		RTENUMNAME(FogBuffer)
		RTENUMNAME(FilterColor2)
		RTENUMNAME(DoFBlurBuffer)
        RTENUMNAME(StereoFix)
		RTENUMNAME(LUTBlend)
		RTENUMNAME(TexturePoolMemory)
		RTENUMNAME(FilterColor3)
		RTENUMNAME(SubsurfaceInscattering)
		RTENUMNAME(SubsurfaceScatteringAttenuation)
		RTENUMNAME(WorldNormalGBuffer)
		RTENUMNAME(WorldReflectionNormalGBuffer)
		RTENUMNAME(SpecularGBuffer)
		RTENUMNAME(DiffuseGBuffer)
		RTENUMNAME(WhiteDummy)
		RTENUMNAME(BokehDOF)
		RTENUMNAME(CubeShadowDepthZ0)
		RTENUMNAME(CubeShadowDepthZ1)
		RTENUMNAME(CubeShadowDepthZ2)
		RTENUMNAME(CubeShadowDepthZ3)
		RTENUMNAME(CubeShadowDepthZ4)
		RTENUMNAME(SeparateTranslucency)
		RTENUMNAME(SeparateTranslucencyDepth)
		default: RenderTargetName = FString::Printf(TEXT("%08X"),(INT)RTEnum);
	}
#undef RTENUMNAME
	return RenderTargetName;
}

const FTextureRHIRef& FSceneRenderTargets::GetLUTBlendTexture() const 
{ 
#if XBOX && !USE_NULL_RHI
	return (FTextureRHIRef&)RenderTargets[LUTBlend].TextureArray; 
#else // XBOX && !USE_NULL_RHI
	return (const FTextureRHIRef&)RenderTargets[LUTBlend].Texture; 
#endif // XBOX && !USE_NULL_RHI
}

/** Updates the TexturePoolMemory texture, if it's currently being visualized on-screen. */
void FSceneRenderTargets::UpdateTexturePoolVisualizer()
{
#if !FINAL_RELEASE && CONSOLE
	if ( (GVisualizeTexture - 1) == TexturePoolMemory )
	{
		if ( IsValidRef(RenderTargets[TexturePoolMemory].Texture) == FALSE )
		{
			RenderTargets[TexturePoolMemory].Texture = RHICreateTexture2D(TexturePoolVisualizerSizeX,TexturePoolVisualizerSizeY,PF_A8R8G8B8,1,TexCreate_Uncooked,NULL);
		}

		UINT Pitch;
		FColor* TextureData = (FColor*) RHILockTexture2D( RenderTargets[TexturePoolMemory].Texture, 0, TRUE, Pitch, FALSE );
		if ( TextureData )
		{
			RHIGetTextureMemoryVisualizeData( TextureData, TexturePoolVisualizerSizeX, TexturePoolVisualizerSizeY, Pitch, 4096 );
		}
		RHIUnlockTexture2D( RenderTargets[TexturePoolMemory].Texture, 0, FALSE );
	}
#endif
}


/*-----------------------------------------------------------------------------
FSceneTextureShaderParameters
-----------------------------------------------------------------------------*/

//
void FSceneTextureShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	// only used if Material has an expression that requires SceneColorTexture
	SceneColorTextureParameter.Bind(ParameterMap,TEXT("SceneColorTexture"),TRUE);
	// only used if Material has an expression that requires SceneDepthTexture
	SceneDepthTextureParameter.Bind(ParameterMap,TEXT("SceneDepthTexture"),TRUE);
	// only used if Material has an expression that requires SceneColorTextureMSAA
	SceneDepthSurfaceParameter.Bind(ParameterMap,TEXT("SceneDepthSurface"),TRUE);
	// only used if Material has an expression that requires SceneDepthTexture
	SceneDepthCalcParameter.Bind(ParameterMap,TEXT("MinZ_MaxZRatio"),TRUE);
	// only used if Material has an expression that requires ScreenPosition biasing
	ScreenPositionScaleBiasParameter.Bind(ParameterMap,TEXT("ScreenPositionScaleBias"),TRUE);
#if !CONSOLE
    // Contains parameters needed to transform from stereo clip space to mono clip space
    NvStereoFixTextureParameter.Bind(ParameterMap,TEXT("NvStereoFixTexture"),TRUE);
#endif
}

void FSceneTextureShaderParameters::SetSceneColorTextureOnly(FShader* PixelShader) const
{	
	SetTextureParameterDirectly(
		PixelShader->GetPixelShader(),
		SceneColorTextureParameter,
		TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
		GSceneRenderTargets.GetSceneColorTexture()
		);
}

//
void FSceneTextureShaderParameters::Set(const FSceneView* View,FShader* PixelShader, ESamplerFilter ColorFilter, const FTexture2DRHIRef& DesiredSceneColorTexture, ESceneDepthUsage DepthUsage/*=SceneDepthUsage_Normal*/) const
{
	const FPixelShaderRHIRef& RHIPixelShader = PixelShader->GetPixelShader();

	if (SceneColorTextureParameter.IsBound() == TRUE)
	{
		FSamplerStateRHIRef Filter;
		switch ( ColorFilter )
		{
			case SF_Bilinear:
				Filter = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
				break;
			case SF_Trilinear:
				Filter = TStaticSamplerState<SF_Trilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
				break;
			case SF_AnisotropicPoint:
				Filter = TStaticSamplerState<SF_AnisotropicPoint,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
				break;
			case SF_AnisotropicLinear:
				Filter = TStaticSamplerState<SF_AnisotropicLinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
				break;
			case SF_Point:
			default:
				Filter = TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
				break;
		}

		SetTextureParameterDirectly(
			RHIPixelShader,
			SceneColorTextureParameter,
			Filter,
			View->bUseLDRSceneColor ? GSceneRenderTargets.GetSceneColorLDRTexture() : DesiredSceneColorTexture
			);
	}
	if (SceneDepthTextureParameter.IsBound())
	{
#if PS3
		UBOOL bSetSceneDepthTexture = (GSupportsDepthTextures || DepthUsage == SceneDepthUsage_ProjectedShadows) && IsValidRef(GSceneRenderTargets.GetSceneDepthTexture());
#else
		UBOOL bSetSceneDepthTexture = GSupportsDepthTextures
			&& IsValidRef(GSceneRenderTargets.GetSceneDepthTexture());
#endif
		if (bSetSceneDepthTexture)
		{
#if NGP
			const FTexture2DRHIRef& DepthTexture = GSceneRenderTargets.GetResolvedDepthTexture();
#else
			const FTexture2DRHIRef& DepthTexture = GSceneRenderTargets.GetSceneDepthTexture();
#endif
			// Bind the zbuffer as a texture if depth textures are supported
			SetTextureParameterDirectly(
				RHIPixelShader,
				SceneDepthTextureParameter,
				TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				DepthTexture
				);
		}
	}

	if(SceneDepthSurfaceParameter.IsBound() && GRHIShaderPlatform == SP_PCD3D_SM5)
	{
		RHISetSurfaceParameter(
			RHIPixelShader,
			SceneDepthSurfaceParameter.GetBaseIndex(),
			GSceneRenderTargets.GetSceneDepthSurface()
			);
	}

#if !CONSOLE
    if (NvStereoFixTextureParameter.IsBound())
    {
        SetTextureParameter(
            RHIPixelShader,
            NvStereoFixTextureParameter,
            TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			nv::stereo::IsStereoEnabled() ? (const FTextureRHIRef&)GSceneRenderTargets.GetStereoFixTexture() : GWhiteTexture->TextureRHI
            );
    }
#endif

	RHISetViewPixelParameters( View, RHIPixelShader, &SceneDepthCalcParameter, &ScreenPositionScaleBiasParameter );
}

void FSceneTextureShaderParameters::Set(const FSceneView* View,FShader* PixelShader, ESamplerFilter ColorFilter/*=SF_Point*/, ESceneDepthUsage DepthUsage/*=SceneDepthUsage_Normal*/) const
{
	Set(View, PixelShader,ColorFilter, GSceneRenderTargets.GetSceneColorTexture(), DepthUsage);
}

//
FArchive& operator<<(FArchive& Ar,FSceneTextureShaderParameters& Parameters)
{
	Ar << Parameters.SceneColorTextureParameter;
	Ar << Parameters.SceneDepthTextureParameter;
	Ar << Parameters.SceneDepthSurfaceParameter;
	Ar << Parameters.SceneDepthCalcParameter;
	Ar << Parameters.ScreenPositionScaleBiasParameter;
#if CONSOLE
	FShaderResourceParameter dummy;
	Ar << dummy;
#else
    Ar << Parameters.NvStereoFixTextureParameter;
#endif
	return Ar;
}

void FDeferredVertexShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	ScreenToWorldParameter.Bind(ParameterMap,TEXT("ScreenToWorldMatrix"), TRUE);
}

void FDeferredVertexShaderParameters::Set(const FSceneView& View, FShader* VertexShader) const
{
	const FMatrix ScreenToWorld = FMatrix(
		FPlane(1,0,0,0),
		FPlane(0,1,0,0),
		FPlane(0,0,(1.0f - Z_PRECISION),1),
		FPlane(0,0,-View.NearClippingDistance * (1.0f - Z_PRECISION),0)
		) *
		View.InvTranslatedViewProjectionMatrix;

	SetVertexShaderValue(VertexShader->GetVertexShader(), ScreenToWorldParameter, ScreenToWorld);
}

FArchive& operator<<(FArchive& Ar,FDeferredVertexShaderParameters& Parameters)
{
	Ar << Parameters.ScreenToWorldParameter;
	return Ar;
}

void FDeferredPixelShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	SceneTextureParameters.Bind(ParameterMap);
	LightAttenuationSurface.Bind(ParameterMap,TEXT("LightAttenuationSurface"),TRUE);
	WorldNormalGBufferTextureMS.Bind(ParameterMap,TEXT("WorldNormalGBufferTextureMS"), TRUE);
	WorldReflectionNormalGBufferTextureMS.Bind(ParameterMap,TEXT("WorldReflectionNormalGBufferTextureMS"), TRUE);
	SpecularGBufferTextureMS.Bind(ParameterMap,TEXT("SpecularGBufferTextureMS"), TRUE);
	DiffuseGBufferTextureMS.Bind(ParameterMap,TEXT("DiffuseGBufferTextureMS"), TRUE);
	WorldNormalGBufferTexture.Bind(ParameterMap,TEXT("WorldNormalGBufferTexture"), TRUE);
	WorldReflectionNormalGBufferTexture.Bind(ParameterMap,TEXT("WorldReflectionNormalGBufferTexture"), TRUE);
	SpecularGBufferTexture.Bind(ParameterMap,TEXT("SpecularGBufferTexture"), TRUE);
	DiffuseGBufferTexture.Bind(ParameterMap,TEXT("DiffuseGBufferTexture"), TRUE);
	ScreenToWorldParameter.Bind(ParameterMap,TEXT("ScreenToWorldMatrix"), TRUE);
}

void FDeferredPixelShaderParameters::Set(const FSceneView& View, FShader* PixelShader, ESceneDepthUsage SceneDepthUsage) const
{
	SceneTextureParameters.Set(&View, PixelShader, SF_Point, GSceneRenderTargets.GetSceneColorTexture(), SceneDepthUsage);

	if (LightAttenuationSurface.IsBound())
	{
		RHISetSurfaceParameter(
			PixelShader->GetPixelShader(), 
			LightAttenuationSurface.GetBaseIndex(), 
			GSceneRenderTargets.GetEffectiveLightAttenuationSurface(TRUE,TRUE)
			);
	}

	if (WorldNormalGBufferTextureMS.IsBound())
	{
		RHISetSurfaceParameter(
			PixelShader->GetPixelShader(), 
			WorldNormalGBufferTextureMS.GetBaseIndex(), 
			GSceneRenderTargets.GetWorldNormalGBufferSurface());
	}

	SetTextureParameter(PixelShader->GetPixelShader(), WorldNormalGBufferTexture, TStaticSamplerState<>::GetRHI(), GSceneRenderTargets.GetWorldNormalGBufferTexture());

	if (WorldReflectionNormalGBufferTextureMS.IsBound())
	{
		RHISetSurfaceParameter(
			PixelShader->GetPixelShader(), 
			WorldReflectionNormalGBufferTextureMS.GetBaseIndex(), 
			GSceneRenderTargets.GetWorldReflectionNormalGBufferSurface());
	}

	SetTextureParameter(PixelShader->GetPixelShader(), WorldReflectionNormalGBufferTexture, TStaticSamplerState<>::GetRHI(), GSceneRenderTargets.GetWorldReflectionNormalGBufferTexture());

	if (SpecularGBufferTextureMS.IsBound())
	{
		RHISetSurfaceParameter(
			PixelShader->GetPixelShader(), 
			SpecularGBufferTextureMS.GetBaseIndex(), 
			GSceneRenderTargets.GetSpecularGBufferSurface());
	}

	SetTextureParameter(PixelShader->GetPixelShader(), SpecularGBufferTexture, TStaticSamplerState<>::GetRHI(), GSceneRenderTargets.GetSpecularGBufferTexture());

	if (DiffuseGBufferTextureMS.IsBound())
	{
		RHISetSurfaceParameter(
			PixelShader->GetPixelShader(), 
			DiffuseGBufferTextureMS.GetBaseIndex(), 
			GSceneRenderTargets.GetDiffuseGBufferSurface());
	}

	SetTextureParameter(PixelShader->GetPixelShader(), DiffuseGBufferTexture, TStaticSamplerState<>::GetRHI(), GSceneRenderTargets.GetDiffuseGBufferTexture());

	if (ScreenToWorldParameter.IsBound())
	{
		const FMatrix ScreenToWorld = FMatrix(
			FPlane(1,0,0,0),
			FPlane(0,1,0,0),
			FPlane(0,0,(1.0f - Z_PRECISION),1),
			FPlane(0,0,-View.NearClippingDistance * (1.0f - Z_PRECISION),0)
			) *
			View.InvViewProjectionMatrix;

		SetPixelShaderValue(PixelShader->GetPixelShader(), ScreenToWorldParameter, ScreenToWorld);
	}
}

FArchive& operator<<(FArchive& Ar,FDeferredPixelShaderParameters& Parameters)
{
	Ar << Parameters.LightAttenuationSurface;
	Ar << Parameters.SceneTextureParameters;
	Ar << Parameters.WorldNormalGBufferTextureMS;
	Ar << Parameters.WorldReflectionNormalGBufferTextureMS;
	Ar << Parameters.SpecularGBufferTextureMS;
	Ar << Parameters.DiffuseGBufferTextureMS;
	Ar << Parameters.WorldNormalGBufferTexture;
	Ar << Parameters.WorldReflectionNormalGBufferTexture;
	Ar << Parameters.SpecularGBufferTexture;
	Ar << Parameters.DiffuseGBufferTexture;
	Ar << Parameters.ScreenToWorldParameter;

	return Ar;
}

/*-----------------------------------------------------------------------------
FSceneRenderTargetProxy
-----------------------------------------------------------------------------*/

/**
* Constructor
*/
FSceneRenderTargetProxy::FSceneRenderTargetProxy()
:	SizeX(0)
,	SizeY(0)
{	
}

/**
* Set SizeX and SizeY of proxy and re-allocate scene targets as needed
*
* @param InSizeX - scene render target width requested
* @param InSizeY - scene render target height requested
*/
void FSceneRenderTargetProxy::SetSizes(UINT InSizeX,UINT InSizeY)
{
	SizeX = InSizeX;
	SizeY = InSizeY;

	if( IsInRenderingThread() )
	{
		GSceneRenderTargets.Allocate(SizeX,SizeY);
	}
	else
	{
		struct FRenderTargetSizeParams
		{
			UINT SizeX;
			UINT SizeY;
		};
		FRenderTargetSizeParams RenderTargetSizeParams = {SizeX,SizeY};
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			RenderTargetAllocProxyCommand,
			FRenderTargetSizeParams,Parameters,RenderTargetSizeParams,
		{
			GSceneRenderTargets.Allocate(Parameters.SizeX, Parameters.SizeY);
		});
	}
}

/**
* @return RHI surface for setting the render target
*/
const FSurfaceRHIRef& FSceneRenderTargetProxy::GetRenderTargetSurface() const
{
	return GSceneRenderTargets.GetSceneColorSurface();
}

/**
* @return width of the scene render target this proxy will render to
*/
UINT FSceneRenderTargetProxy::GetSizeX() const
{
	return SizeX;
}

/**
* @return height of the scene render target this proxy will render to
*/
UINT FSceneRenderTargetProxy::GetSizeY() const
{
	return SizeY;
}

/**
* @return gamma this render target should be rendered with
*/
FLOAT FSceneRenderTargetProxy::GetDisplayGamma() const
{
	return 1.0f;
}

/**
* @return RHI surface for setting the render target
*/
const FSurfaceRHIRef& FSceneDepthTargetProxy::GetDepthTargetSurface() const
{
	return GSceneRenderTargets.GetSceneDepthSurface();
}


const FTexture2DRHIRef& FSceneRenderTargets::GetFilterColorTexture(FSceneRenderTargetIndex FilterColorIndex) const
{
	switch(FilterColorIndex)
	{
		default:
			check(0);	// falls through (avoids compile warning)
		case SRTI_FilterColor0:
			return RenderTargets[FilterColor].Texture;
		case SRTI_FilterColor1:
			return RenderTargets[FilterColor2].Texture;
		case SRTI_FilterColor2:
			return RenderTargets[FilterColor3].Texture;
	}
}

const FSurfaceRHIRef& FSceneRenderTargets::GetFilterColorSurface(FSceneRenderTargetIndex FilterColorIndex) const
{ 
	switch(FilterColorIndex)
	{
		default:
			check(0);	// falls through (avoids compile warning)
		case SRTI_FilterColor0:
			return RenderTargets[FilterColor].Surface;
		case SRTI_FilterColor1:
			return RenderTargets[FilterColor2].Surface;
		case SRTI_FilterColor2:
			return RenderTargets[FilterColor3].Surface;
	}
}

UBOOL FSceneRenderTargets::IsSeparateTranslucencyActive() const
{
	return GSystemSettings.bAllowSeparateTranslucency && GRHIShaderPlatform == SP_PCD3D_SM5;
}
