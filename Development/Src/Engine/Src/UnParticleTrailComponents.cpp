/*-----------------------------------------------------------------------------
	UnParticleTrailComponents.cpp: Particle trail related code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
-----------------------------------------------------------------------------*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"
#include "LevelUtils.h"

//#define _TRAIL2_TESSELLATE_TO_SOURCE_
//#define _TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_
//#define _TRAILS_DEBUG_UP_VECTORS_

//----------------------------------------------------------------------------
extern FSceneView*			GParticleView;

/** trail stats */
DECLARE_STATS_GROUP(TEXT("TrailParticles"),STATGROUP_TrailParticles);

DECLARE_DWORD_COUNTER_STAT(TEXT("Trail Particles"),STAT_TrailParticles,STATGROUP_TrailParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Trail Ptcl Render Calls"),STAT_TrailParticlesRenderCalls,STATGROUP_TrailParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Trail Ptcl Render Calls Completed"),STAT_TrailParticlesRenderCallsCompleted,STATGROUP_TrailParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Trail Ptcls Spawned"),STAT_TrailParticlesSpawned,STATGROUP_TrailParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Trail Ptcl Update Calls"),STAT_TrailParticlesUpdateCalls,STATGROUP_TrailParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Trail Ptcls Updated"),STAT_TrailParticlesUpdated,STATGROUP_TrailParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Trail Ptcls Killed"),STAT_TrailParticlesKilled,STATGROUP_TrailParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Trail Ptcl Tris"),STAT_TrailParticlesTrianglesRendered,STATGROUP_TrailParticles);

DECLARE_CYCLE_STAT(TEXT("Trail Spawn Time"),STAT_TrailSpawnTime,STATGROUP_TrailParticles);
DECLARE_CYCLE_STAT(TEXT("Trail VB Pack Time"),STAT_TrailVBPackingTime,STATGROUP_TrailParticles);
DECLARE_CYCLE_STAT(TEXT("Trail IB Pack Time"),STAT_TrailIBPackingTime,STATGROUP_TrailParticles);
DECLARE_CYCLE_STAT(TEXT("Trail Render Time"),STAT_TrailRenderingTime,STATGROUP_TrailParticles);
DECLARE_CYCLE_STAT(TEXT("Trail Res Update Time"),STAT_TrailResourceUpdateTime,STATGROUP_TrailParticles);
DECLARE_CYCLE_STAT(TEXT("Trail Tick Time"),STAT_TrailTickTime,STATGROUP_TrailParticles);
DECLARE_CYCLE_STAT(TEXT("Trail Update Time"),STAT_TrailUpdateTime,STATGROUP_TrailParticles);
DECLARE_CYCLE_STAT(TEXT("Trail PSys Comp Tick Time"),STAT_TrailPSysCompTickTime,STATGROUP_TrailParticles);

IMPLEMENT_CLASS(UParticleTrail2EmitterInstance);
