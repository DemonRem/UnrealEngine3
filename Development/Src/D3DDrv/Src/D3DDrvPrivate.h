/*=============================================================================
	D3DDrvPrivate.h: Private D3D RHI definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Definitions.
#define DEBUG_SHADERS 0

// Dependencies
#include "Engine.h"
#include "D3DDrv.h" // Note that Engine.h already includes D3DDrv.h, but not in all cases.

#if USE_D3D_RHI

//
// Globals.
//

/** The global D3D device. */
extern TD3DRef<IDirect3DDevice9> GDirect3DDevice;

/** The global D3D device's back buffer. */
extern FSurfaceRHIRef GD3DBackBuffer;

/** A list of all viewport RHIs that have been created. */
extern TArray<FD3DViewport*> GD3DViewports;

/** The viewport which is currently being drawn. */
extern FD3DViewport* GD3DDrawingViewport;

/** True if the application has lost D3D device access. */
extern UBOOL GD3DDeviceLost;

/**
*  Initializes GRHIShaderPlatform based on the system's Caps
*/
extern void InitializeD3DShaderPlatform();

/**
 * Reinitializes the D3D device upon a viewport change.
 */
extern void UpdateD3DDeviceFromViewports();

/**
 * Returns a supported screen resolution that most closely matches the input.
 * @param Width - Input: Desired resolution width in pixels. Output: A width that the platform supports.
 * @param Height - Input: Desired resolution height in pixels. Output: A height that the platform supports.
 */
extern void GetSupportedResolution( UINT &Width, UINT &Height );

/**
 * The D3D RHI stats.
 */
enum ED3DRHIStats
{
	STAT_PresentTime = STAT_RHIFirstStat,
	STAT_DrawPrimitiveCalls,
	STAT_RHITriangles,
	STAT_RHILines,
};

#endif
