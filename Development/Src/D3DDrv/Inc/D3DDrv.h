/*=============================================================================
	D3DDrv.h: Public D3D RHI definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_D3DDRV
#define _INC_D3DDRV

#if USE_D3D_RHI || USE_NULL_RHI

// D3D headers.
#pragma pack(push,8)
#define D3D_OVERLOADS 1
#include "../../../External/DirectX9/Include/d3d9.h"
#include "../../../External/DirectX9/Include/d3dx9.h"
#undef DrawText
#pragma pack(pop)


//
// RHI globals.
//

/** TRUE to if the RHI supports command lists, FALSE otherwise. */
#define RHI_SUPPORTS_COMMAND_LISTS 0

// D3D RHI public headers.
#include "D3DUtil.h"

#endif

#if USE_D3D_RHI

#include "D3DState.h"
#include "D3DResources.h"
#include "D3DRenderTarget.h"
#include "D3DViewport.h"


/*-----------------------------------------------------------------------------
	Empty PC RHI functions.
-----------------------------------------------------------------------------*/

inline void RHISetViewPixelParameters( FCommandContextRHI* Context, const class FSceneView* View, FPixelShaderRHIParamRef PixelShader, const class FShaderParameter* SceneDepthCalcParameter, const class FShaderParameter* ScreenPositionScaleBiasParameter )
{
}

#endif

#endif
