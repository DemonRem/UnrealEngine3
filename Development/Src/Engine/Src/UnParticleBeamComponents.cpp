/*-----------------------------------------------------------------------------
	UnParticleBeamComponents.cpp: Particle beam related code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
-----------------------------------------------------------------------------*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"
#include "LevelUtils.h"

//----------------------------------------------------------------------------
extern FSceneView*			GParticleView;

/** Beam particle stat objects */
DECLARE_STATS_GROUP(TEXT("BeamParticles"),STATGROUP_BeamParticles);

DECLARE_DWORD_COUNTER_STAT(TEXT("Beam Particles"),STAT_BeamParticles,STATGROUP_BeamParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Beam Ptcl Render Calls"),STAT_BeamParticlesRenderCalls,STATGROUP_BeamParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Beam Ptcl Render Calls Completed"),STAT_BeamParticlesRenderCallsCompleted,STATGROUP_BeamParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Beam Ptcls Spawned"),STAT_BeamParticlesSpawned,STATGROUP_BeamParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Beam Ptcl Update Calls"),STAT_BeamParticlesUpdateCalls,STATGROUP_BeamParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Beam Ptcls Updated"),STAT_BeamParticlesUpdated,STATGROUP_BeamParticles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Beam Ptcls Killed"),STAT_BeamParticlesKilled,STATGROUP_BeamParticles);

DECLARE_CYCLE_STAT(TEXT("Beam Spawn Time"),STAT_BeamSpawnTime,STATGROUP_BeamParticles);
DECLARE_CYCLE_STAT(TEXT("Beam VB Pack Time"),STAT_BeamVBPackingTime,STATGROUP_BeamParticles);
DECLARE_CYCLE_STAT(TEXT("Beam IB Pack Time"),STAT_BeamIBPackingTime,STATGROUP_BeamParticles);
DECLARE_CYCLE_STAT(TEXT("Beam Render Time"),STAT_BeamRenderingTime,STATGROUP_BeamParticles);
DECLARE_CYCLE_STAT(TEXT("Beam Res Update Time"),STAT_BeamResourceUpdateTime,STATGROUP_BeamParticles);
DECLARE_CYCLE_STAT(TEXT("Beam Tick Time"),STAT_BeamTickTime,STATGROUP_BeamParticles);
DECLARE_CYCLE_STAT(TEXT("Beam Update Time"),STAT_BeamUpdateTime,STATGROUP_BeamParticles);
DECLARE_CYCLE_STAT(TEXT("Beam PSys Comp Tick Time"),STAT_BeamPSysCompTickTime,STATGROUP_BeamParticles);

//=============================================================================
// ParticleBeam2EmitterInstance
//=============================================================================
IMPLEMENT_CLASS(UParticleBeam2EmitterInstance);

