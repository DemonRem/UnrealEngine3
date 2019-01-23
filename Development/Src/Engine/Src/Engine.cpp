/*=============================================================================
	Engine.cpp: Unreal engine package.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ChartCreation.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Global subsystems in the engine.
FMemStack			GEngineMem;

// don't include this on consoles or in gcc
#if !CONSOLE && defined(_MSC_VER)

// don't include this on consoles or in gcc
#endif // !CONSOLE && defined(_MSC_VER)

/**
 * Stats objects for Engine
 */
DECLARE_STATS_GROUP(TEXT("Engine"),STATGROUP_Engine);
DECLARE_CYCLE_STAT(TEXT("FrameTime"),STAT_FrameTime,STATGROUP_Engine);

/** Terrain stats */
DECLARE_CYCLE_STAT(TEXT("Terrain Foliage Time"),STAT_TerrainFoliageTime,STATGROUP_Engine);
DECLARE_CYCLE_STAT(TEXT("Foliage Render Time"),STAT_FoliageRenderTime,STATGROUP_Engine);
DECLARE_CYCLE_STAT(TEXT("Terrain Smooth Time"),STAT_TerrainSmoothTime,STATGROUP_Engine);
DECLARE_CYCLE_STAT(TEXT("Terrain Render Time"),STAT_TerrainRenderTime,STATGROUP_Engine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Terrain Triangles"),STAT_TerrainTriangles,STATGROUP_Engine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Foliage Instances"),STAT_TerrainFoliageInstances,STATGROUP_Engine);

/** Decal stats */
DECLARE_DWORD_COUNTER_STAT(TEXT("Decal Triangles"),STAT_DecalTriangles,STATGROUP_Engine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Decal Draw Calls"),STAT_DecalDrawCalls,STATGROUP_Engine);
DECLARE_CYCLE_STAT(TEXT("Decal Render Time"),STAT_DecalRenderTime,STATGROUP_Engine);

/** Input stat */
DECLARE_CYCLE_STAT(TEXT("Input Time"),STAT_InputTime,STATGROUP_Engine);

/** HUD stat */
DECLARE_CYCLE_STAT(TEXT("HUD Time"),STAT_HudTime,STATGROUP_Engine);

/** Shadow volume stats */
DECLARE_DWORD_COUNTER_STAT(TEXT("Shadow Volume Tris"),STAT_ShadowVolumeTriangles,STATGROUP_Engine);
DECLARE_CYCLE_STAT(TEXT("Shadow Extrusion Time"),STAT_ShadowExtrusionTime,STATGROUP_Engine);
DECLARE_CYCLE_STAT(TEXT("Shadow Volume Render Time"),STAT_ShadowVolumeRenderTime,STATGROUP_Engine);

/** Static mesh tris rendered */
DECLARE_DWORD_COUNTER_STAT(TEXT("Static Mesh Tris"),STAT_StaticMeshTriangles,STATGROUP_Engine);

/** Skeletal stats */
DECLARE_CYCLE_STAT(TEXT("Skel Skin Time"),STAT_SkinningTime,STATGROUP_Engine);
DECLARE_CYCLE_STAT(TEXT("Update Cloth Verts Time"),STAT_UpdateClothVertsTime,STATGROUP_Engine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Skel Mesh Tris"),STAT_SkelMeshTriangles,STATGROUP_Engine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Skel Verts CPU Skin"),STAT_CPUSkinVertices,STATGROUP_Engine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Skel Verts GPU Skin"),STAT_GPUSkinVertices,STATGROUP_Engine);

/** Frame chart stats */
DECLARE_STATS_GROUP(TEXT("FPSChart"),STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("00 - 05 FPS"), STAT_FPSChart_0_5,		STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("05 - 10 FPS"), STAT_FPSChart_5_10,		STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("10 - 15 FPS"), STAT_FPSChart_10_15,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("15 - 20 FPS"), STAT_FPSChart_15_20,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("20 - 25 FPS"), STAT_FPSChart_20_25,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("25 - 30 FPS"), STAT_FPSChart_25_30,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("30 - 35 FPS"), STAT_FPSChart_30_35,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("35 - 40 FPS"), STAT_FPSChart_35_40,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("40 - 45 FPS"), STAT_FPSChart_40_45,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("45 - 50 FPS"), STAT_FPSChart_45_50,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("50 - 55 FPS"), STAT_FPSChart_50_55,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("55 - 60 FPS"), STAT_FPSChart_55_60,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("60 - .. FPS"), STAT_FPSChart_60_INF,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("30+ FPS"), STAT_FPSChart_30Plus,	STATGROUP_FPSChart);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Unaccounted time"), STAT_FPSChart_UnaccountedTime,	STATGROUP_FPSChart);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Frame count"),	STAT_FPSChart_FrameCount, STATGROUP_FPSChart);

