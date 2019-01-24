/*=============================================================================
	RHI.cpp: Render Hardware Interface implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

//
// RHI globals.
//

UBOOL GIsRHIInitialized = FALSE;
INT GMaxTextureMipCount = MAX_TEXTURE_MIP_COUNT;
INT GMinTextureResidentMipCount = 7;
UBOOL GSupportsDepthTextures = FALSE;
UBOOL GSupportsHardwarePCF = FALSE;
UBOOL GSupportsVertexTextureFetch = FALSE;
UBOOL GSupportsFetch4 = FALSE;
UBOOL GSupportsFPFiltering = TRUE;
UBOOL GSupportsRenderTargetFormat_PF_G8 = TRUE;
UBOOL GSupportsQuads = FALSE;
UBOOL GUsesInvertedZ = FALSE;
FLOAT GPixelCenterOffset = 0.5f;
INT GMaxPerObjectShadowDepthBufferSizeX = 2048;
INT GMaxPerObjectShadowDepthBufferSizeY = 2048;
INT GMaxWholeSceneDominantShadowDepthBufferSize = 2048;
INT GCurrentColorExpBias = 0;
UBOOL GUseTilingCode = FALSE;
UBOOL GUseMSAASplitScreen = FALSE;
#if USE_NULL_RHI
	UBOOL GUsingNullRHI = TRUE;
#else
	UBOOL GUsingNullRHI = FALSE;
#endif
UBOOL GUsingES2RHI = FALSE;
UBOOL GUsingMobileRHI = FALSE;

/** Whether to use the post-process code path on mobile. */
UBOOL GMobileUsePostProcess = FALSE;

INT GDrawUPVertexCheckCount = MAXINT;
INT GDrawUPIndexCheckCount = MAXINT;
UBOOL GSupportsVertexInstancing = FALSE;
UBOOL GSupportsEmulatedVertexInstancing = FALSE;
UBOOL GVertexElementsCanShareStreamOffset = TRUE;
UBOOL GModShadowsWithAlphaEmissiveBit = FALSE;
INT GOptimalMSAALevel = 4;
UBOOL GProfilingGPU = FALSE;

/** Whether we are profiling GPU hitches. */
UBOOL GProfilingGPUHitches = FALSE;

/** Bit flags from ETextureFormatSupport, specifying what texture formats a platform supports. */
DWORD GTextureFormatSupport = TEXSUPPORT_DXT;

/* A global depth bias to use when user clip planes are enabled, to avoid z-fighting. */
FLOAT GDepthBiasOffset = 0.0f;

INT GNumActiveGPUsForRendering = 1;

FVertexElementTypeSupportInfo GVertexElementTypeSupport;
