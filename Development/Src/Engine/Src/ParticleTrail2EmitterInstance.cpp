/*=============================================================================
	ParticleTrail2EmitterInstance.cpp: 
	Particle trail2 emitter instance implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"

IMPLEMENT_PARTICLEEMITTERINSTANCE_TYPE(FParticleTrail2EmitterInstance);

/*-----------------------------------------------------------------------------
	ParticleTrail2EmitterInstance.
-----------------------------------------------------------------------------*/
/**
 *	Structure for trail emitter instances
 */

/** Constructor	*/
FParticleTrail2EmitterInstance::FParticleTrail2EmitterInstance() :
	FParticleEmitterInstance()
	, TrailTypeData(NULL)
	, TrailModule_Source(NULL)
	, TrailModule_Source_Offset(0)
	, TrailModule_Spawn(NULL)
	, TrailModule_Spawn_Offset(0)
	, TrailModule_Taper(NULL)
	, TrailModule_Taper_Offset(0)
	, FirstEmission(0)
	, LastEmittedParticleIndex(-1)
	, LastSelectedParticleIndex(-1)
	, TickCount(0)
	, ForceSpawnCount(0)
	, VertexCount(0)
	, TriangleCount(0)
	, Tessellation(0)
	, TrailCount(0)
	, MaxTrailCount(0)
	, SourceActor(NULL)
	, SourceEmitter(NULL)
	, ActuallySpawned(0)
{
	TextureTiles.Empty();
	TrailSpawnTimes.Empty();
	SourcePosition.Empty();
	LastSourcePosition.Empty();
	CurrentSourcePosition.Empty();
	LastSpawnPosition.Empty();
	LastSpawnTangent.Empty();
	SourceDistanceTravelled.Empty();
	SourceOffsets.Empty();
}

/** Destructor	*/
FParticleTrail2EmitterInstance::~FParticleTrail2EmitterInstance()
{
	TextureTiles.Empty();
	TrailSpawnTimes.Empty();
	SourcePosition.Empty();
	LastSourcePosition.Empty();
	CurrentSourcePosition.Empty();
	LastSpawnPosition.Empty();
	LastSpawnTangent.Empty();
	SourceDistanceTravelled.Empty();
	SourceOffsets.Empty();
}

/**
 *	Initialize the parameters for the structure
 *
 *	@param	InTemplate		The ParticleEmitter to base the instance on
 *	@param	InComponent		The owning ParticleComponent
 *	@param	bClearResources	If TRUE, clear all resource data
 */
void FParticleTrail2EmitterInstance::InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent, UBOOL bClearResources)
{
	FParticleEmitterInstance::InitParameters(InTemplate, InComponent, bClearResources);

	// We don't support LOD on trails
	UParticleLODLevel* LODLevel	= InTemplate->GetLODLevel(0);
	check(LODLevel);
	TrailTypeData	= CastChecked<UParticleModuleTypeDataTrail2>(LODLevel->TypeDataModule);
	check(TrailTypeData);

	TrailModule_Source			= NULL;
	TrailModule_Source_Offset	= 0;
	TrailModule_Spawn			= NULL;
	TrailModule_Spawn_Offset	= 0;
	TrailModule_Taper			= NULL;
	TrailModule_Taper_Offset	= 0;

	// Always have at least one trail
	if (TrailTypeData->MaxTrailCount == 0)
	{
		TrailTypeData->MaxTrailCount	= 1;
	}

	//@todo. Remove this statement once multiple trails per emitter is implemented. 
	TrailTypeData->MaxTrailCount	= 1;

	// Always have at least one particle per trail
	if (TrailTypeData->MaxParticleInTrailCount == 0)
	{
		// Doesn't make sense to have 0 for this...
		warnf(TEXT("TrailEmitter %s --> MaxParticleInTrailCount == 0!"), *(InTemplate->GetPathName()));
		TrailTypeData->MaxParticleInTrailCount	= 1;
	}

	MaxTrailCount				= TrailTypeData->MaxTrailCount;
	TrailSpawnTimes.AddZeroed(MaxTrailCount);
	SourceDistanceTravelled.AddZeroed(MaxTrailCount);
	SourcePosition.AddZeroed(MaxTrailCount);
	LastSourcePosition.AddZeroed(MaxTrailCount);
	CurrentSourcePosition.AddZeroed(MaxTrailCount);
	LastSpawnPosition.AddZeroed(MaxTrailCount);
	LastSpawnTangent.AddZeroed(MaxTrailCount);
	SourceDistanceTravelled.AddZeroed(MaxTrailCount);
	FirstEmission				= TRUE;
	LastEmittedParticleIndex	= -1;
	LastSelectedParticleIndex	= -1;
	TickCount					= 0;
	ForceSpawnCount				= 0;

	VertexCount					= 0;
	TriangleCount				= 0;

	TextureTiles.Empty();
	TextureTiles.AddItem(TrailTypeData->TextureTile);

	// Resolve any actors...
	ResolveSource();
}

/**
 *	Initialize the instance
 */
void FParticleTrail2EmitterInstance::Init()
{
	FParticleEmitterInstance::Init();
	// Setup the modules prior to initializing...
	SetupTrailModules();
}

/**
 *	Tick the instance.
 *
 *	@param	DeltaTime			The time slice to use
 *	@param	bSuppressSpawning	If TRUE, do not spawn during Tick
 */
void FParticleTrail2EmitterInstance::Tick(FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	SCOPE_CYCLE_COUNTER(STAT_TrailTickTime);
	if (Component)
	{
		// Only support the high LOD
		UParticleLODLevel* LODLevel	= SpriteTemplate->GetLODLevel(0);
		check(LODLevel);

		// Make sure we don't try and do any interpolation on the first frame we are attached (OldLocation is not valid in this circumstance)
		if(Component->bJustAttached)
		{
			Location	= Component->LocalToWorld.GetOrigin();
			OldLocation	= Location;
		}
		else
		{
			// Keep track of location for world- space interpolation and other effects.
			OldLocation	= Location;
			Location	= Component->LocalToWorld.GetOrigin();
		}

		SecondsSinceCreation += DeltaTime;

		// Update time within emitter loop.
		EmitterTime = SecondsSinceCreation;

		if (EmitterDuration > KINDA_SMALL_NUMBER)
		{
			EmitterTime = appFmod(SecondsSinceCreation, EmitterDuration);
		}

		// Take delay into account
		FLOAT EmitterDelay = LODLevel->RequiredModule->EmitterDelay;

		// If looping, handle it
		if ((SecondsSinceCreation - (EmitterDuration * LoopCount)) >= EmitterDuration)
		{
			LoopCount++;
			ResetBurstList();

			if ((LoopCount == 1) && (LODLevel->RequiredModule->bDelayFirstLoopOnly == TRUE) && 
				((LODLevel->RequiredModule->EmitterLoops == 0) || (LODLevel->RequiredModule->EmitterLoops > 1)))
			{
				// Need to correct the emitter durations...
				for (INT LODIndex = 0; LODIndex < SpriteTemplate->LODLevels.Num(); LODIndex++)
				{
					UParticleLODLevel* TempLOD = SpriteTemplate->LODLevels(LODIndex);
					EmitterDurations(TempLOD->Level) -= TempLOD->RequiredModule->EmitterDelay;
				}
				EmitterDuration		= EmitterDurations(CurrentLODLevelIndex);
			}
		}

		// Don't delay unless required
		if ((LODLevel->RequiredModule->bDelayFirstLoopOnly == TRUE) && (LoopCount > 0))
		{
			EmitterDelay = 0;
		}

		// 'Reset' the emitter time so that the modules function correctly
		EmitterTime -= EmitterDelay;

		// Update the source data (position, etc.)
		UpdateSourceData(DeltaTime);

		// Kill before the spawn... Otherwise, we can get 'flashing'
		KillParticles();

		// We need to update the source travelled distance
		for (INT i=0; i<ActiveParticles; i++)
		{
			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);

			INT						CurrentOffset		= TypeDataOffset;
			FTrail2TypeDataPayload*	TrailData			= NULL;
			FLOAT*					TaperValues			= NULL;

			TrailTypeData->GetDataPointers(this, (const BYTE*)&Particle, CurrentOffset, TrailData, TaperValues);

			if (TRAIL_EMITTER_IS_START(TrailData->Flags))
			{
				UBOOL	bGotSource	= FALSE;

				FVector LastPosition = SourcePosition(TrailData->TrailIndex);
				FVector Position;

				if (TrailModule_Source)
				{
					Position = CurrentSourcePosition(TrailData->TrailIndex);
					bGotSource	= TRUE;
				}

				if (!bGotSource)
				{
					// Assume it should be taken from the emitter...
					Position	= Component->LocalToWorld.GetOrigin();
				}

				FVector Travelled	= Position - LastPosition;
				FLOAT	Distance	= Travelled.Size();

				SourceDistanceTravelled(TrailData->TrailIndex) += Distance;
				if (Distance > KINDA_SMALL_NUMBER)
				{
					SourcePosition(TrailData->TrailIndex) = Position;
				}
			}
			else
			{
				// Nothing...
			}
		}

		// If not suppressing spawning...
		if (!bSuppressSpawning)
		{
			if ((LODLevel->RequiredModule->EmitterLoops == 0) || 
				(LoopCount < LODLevel->RequiredModule->EmitterLoops) ||
				(SecondsSinceCreation < (EmitterDuration * LODLevel->RequiredModule->EmitterLoops)))
			{
				// For Trails, we probably want to ignore the SpawnRate distribution,
				// and focus strictly on the BurstList...
				FLOAT SpawnRate = 0.0f;
				// Figure out spawn rate for this tick.
				SpawnRate = LODLevel->RequiredModule->SpawnRate.GetValue(EmitterTime, Component);

				// Take Bursts into account as well...
				INT		Burst		= 0;
				FLOAT	BurstTime	= GetCurrentBurstRateOffset(DeltaTime, Burst);
				SpawnRate += BurstTime;

				// Spawn new particles...

				//@todo. Fix the issue of 'blanking' Trails when the count drops...
				// This is a temporary hack!
				if ((ActiveParticles < MaxTrailCount) && (SpawnRate <= KINDA_SMALL_NUMBER))
				{
					// Force the spawn of a single Trail...
					SpawnRate = 1.0f / DeltaTime;
				}

				if (TrailModule_Spawn && TrailModule_Spawn->bEnabled)
				{
					INT	SpawnModCount = TrailModule_Spawn->GetSpawnCount(this, DeltaTime);
					INT	MaxParticlesAllowed	= MaxTrailCount * TrailTypeData->MaxParticleInTrailCount;
					if ((SpawnModCount + ActiveParticles) > MaxParticlesAllowed)
					{
						SpawnModCount	= MaxParticlesAllowed - ActiveParticles - 1;
						if (SpawnModCount < 0)
						{
							SpawnModCount = 0;
						}
					}

					if (ActiveParticles >= (TrailTypeData->MaxParticleInTrailCount * MaxTrailCount))
					{
						SpawnModCount = 0;
					}

					if (SpawnModCount)
					{
						//debugf(TEXT("SpawnModCount = %d"), SpawnModCount);
						// Set the burst for this, if there are any...
						SpawnFraction	= 0.0f;
						Burst			= SpawnModCount;
						SpawnRate		= Burst / DeltaTime;
					}
				}
				else
				{
					if ((ActiveParticles > 0) && (SourceDistanceTravelled(0) == 0.0f))
					{
						SpawnRate = 0.0f;
						//debugf(TEXT("Killing SpawnRate (no distance travelled)"));
					}
				}

				if (SpawnRate > 0.f)
				{
					SpawnFraction = Spawn(SpawnFraction, SpawnRate, DeltaTime, Burst, BurstTime);
				}
			}
		}

		// Reset velocity and size.
		ResetParticleParameters(DeltaTime, STAT_TrailParticlesUpdated);

		UParticleModuleTypeDataBase* pkBase = 0;
		if (LODLevel->TypeDataModule)
		{
			pkBase = Cast<UParticleModuleTypeDataBase>(LODLevel->TypeDataModule);
			//@todo. Need to track TypeData offset into payload!
			pkBase->PreUpdate(this, TypeDataOffset, DeltaTime);
		}

		// Update existing particles (might respawn dying ones).
		for (INT i=0; i<LODLevel->UpdateModules.Num(); i++)
		{
			UParticleModule* ParticleModule	= LODLevel->UpdateModules(i);
			if (!ParticleModule || !ParticleModule->bEnabled)
			{
				continue;
			}

			UINT* Offset = ModuleOffsetMap.Find(ParticleModule);
			ParticleModule->Update(this, Offset ? *Offset : 0, DeltaTime);
		}

		//@todo. This should ALWAYS be true for Trails...
		if (pkBase)
		{
			// The order of the update here is VERY important
			if (TrailModule_Source && TrailModule_Source->bEnabled)
			{
				TrailModule_Source->Update(this, TrailModule_Source_Offset, DeltaTime);
			}
			if (TrailModule_Spawn && TrailModule_Spawn->bEnabled)
			{
				TrailModule_Spawn->Update(this, TrailModule_Spawn_Offset, DeltaTime);
			}
			if (TrailModule_Taper && TrailModule_Taper->bEnabled)
			{
				TrailModule_Taper->Update(this, TrailModule_Taper_Offset, DeltaTime);
			}

			//@todo. Need to track TypeData offset into payload!
			pkBase->Update(this, TypeDataOffset, DeltaTime);
			pkBase->PostUpdate(this, TypeDataOffset, DeltaTime);
		}

		// Calculate bounding box and simulate velocity.
		UpdateBoundingBox(DeltaTime);

		//DetermineVertexAndTriangleCount();

		if (!bSuppressSpawning)
		{
			// Ensure that we flip the 'FirstEmission' flag
			FirstEmission = false;
		}

		// Invalidate the contents of the vertex/index buffer.
		IsRenderDataDirty = 1;

		// Bump the tick count
		TickCount++;

		// 'Reset' the emitter time so that the delay functions correctly
		EmitterTime += EmitterDelay;
	}
	INC_DWORD_STAT(STAT_TrailParticlesUpdateCalls);
}

/**
 *	Tick the instance in the editor.
 *	This function will interpolate between the current LODLevels to allow for
 *	the designer to visualize how the selected LOD setting would look.
 *
 *	@param	HighLODLevel		The higher LOD level selected
 *	@param	LowLODLevel			The lower LOD level selected
 *	@param	Multiplier			The interpolation value to use between the two
 *	@param	DeltaTime			The time slice to use
 *	@param	bSuppressSpawning	If TRUE, do not spawn during Tick
 */
void FParticleTrail2EmitterInstance::TickEditor(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, FLOAT Multiplier, FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	Tick(DeltaTime, bSuppressSpawning);
}

/**
 *	Update the bounding box for the emitter
 *
 *	@param	DeltaTime		The time slice to use
 */
void FParticleTrail2EmitterInstance::UpdateBoundingBox(FLOAT DeltaTime)
{
	if (Component)
	{
		FLOAT MaxSizeScale	= 1.0f;
		ParticleBoundingBox.Init();

		// Handle local space usage
		UParticleLODLevel* LODLevel = SpriteTemplate->GetLODLevel(0);
		check(LODLevel);
		if (LODLevel->RequiredModule->bUseLocalSpace == false) 
		{
			ParticleBoundingBox += Component->LocalToWorld.GetOrigin();
		}

		// Take scale into account
		FVector Scale = FVector(1.0f, 1.0f, 1.0f);
		Scale *= Component->Scale * Component->Scale3D;
		AActor* Actor = Component->GetOwner();
		if (Actor && !Component->AbsoluteScale)
		{
			Scale *= Actor->DrawScale * Actor->DrawScale3D;
		}

		// As well as each particle
		for (INT i=0; i<ActiveParticles; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[i]);

			// Do linear integrator and update bounding box
			Particle->OldLocation = Particle->Location;
			Particle->Location	+= DeltaTime * Particle->Velocity;
			ParticleBoundingBox += Particle->Location;
			FVector Size = Particle->Size * Scale;
			ParticleBoundingBox += Particle->Location + Size;
			ParticleBoundingBox += Particle->Location - Size;

			// Do angular integrator, and wrap result to within +/- 2 PI
			Particle->Rotation	+= DeltaTime * Particle->RotationRate;
			Particle->Rotation	 = appFmod(Particle->Rotation, 2.f*(FLOAT)PI);
			MaxSizeScale		 = Max(MaxSizeScale, Size.GetAbsMax()); //@todo particles: this does a whole lot of compares that can be avoided using SSE/ Altivec.
		}
		ParticleBoundingBox = ParticleBoundingBox.ExpandBy(MaxSizeScale);

		// Transform bounding box into world space if the emitter uses a local space coordinate system.
		if (LODLevel->RequiredModule->bUseLocalSpace) 
		{
			ParticleBoundingBox = ParticleBoundingBox.TransformBy(Component->LocalToWorld);
		}
	}
}

/**
 *	Spawn particles for this instance
 *
 *	@param	OldLeftover		The leftover time from the last spawn
 *	@param	Rate			The rate at which particles should be spawned
 *	@param	DeltaTime		The time slice to spawn over
 *	@param	Burst			The number of burst particle
 *	@param	BurstTime		The burst time addition (faked time slice)
 *
 *	@return	FLOAT			The leftover fraction of spawning
 */
FLOAT FParticleTrail2EmitterInstance::Spawn(FLOAT OldLeftover, FLOAT Rate, FLOAT DeltaTime, INT Burst, FLOAT BurstTime)
{
	// If not a trail, get out
	if (!TrailTypeData)
	{
		return OldLeftover;
	}

	FLOAT	NewLeftover;

	SCOPE_CYCLE_COUNTER(STAT_TrailSpawnTime);

	UParticleLODLevel* LODLevel	= SpriteTemplate->GetLODLevel(0);
	check(LODLevel);

	// Ensure continous spawning... lots of fiddling.
	NewLeftover = OldLeftover + DeltaTime * Rate;

	INT		Number		= appFloor(NewLeftover);
	FLOAT	Increment	= 1.f / Rate;
	FLOAT	StartTime	= DeltaTime + OldLeftover * Increment - Increment;
	NewLeftover			= NewLeftover - Number;

	// Always at least match the burst
	if (Number < Burst)
	{
		Number = Burst;
	}

	// Offset burst time
	if (BurstTime > KINDA_SMALL_NUMBER)
	{
		NewLeftover -= BurstTime / Burst;
		NewLeftover	= Clamp<FLOAT>(NewLeftover, 0, NewLeftover);
	}

	// Determine if no particles are alive
	UBOOL bNoLivingParticles = false;
	if (ActiveParticles == 0)
	{
		bNoLivingParticles = true;
		if (Number == 0)
			Number = 1;
	}

	// Spawn for each trail
	if ((Number > 0) && (Number < TrailCount))
	{
		Number	= TrailCount;
	}

	// Don't allow more than TrailCount trails...
	INT	MaxParticlesAllowed	= MaxTrailCount * TrailTypeData->MaxParticleInTrailCount;
	if ((Number + ActiveParticles) > MaxParticlesAllowed)
	{
		Number	= MaxParticlesAllowed - ActiveParticles - 1;
		if (Number < 0)
		{
			Number = 0;
		}
	}

	// Handle growing arrays.
	INT NewCount = ActiveParticles + Number;
	if (NewCount >= MaxActiveParticles)
	{
		if (DeltaTime < 0.25f)
		{
			Resize(NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1);
		}
		else
		{
			Resize((NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1), FALSE);
		}
	}

	// Spawn particles.
	for (INT i = 0; (i < Number) && (((INT)ActiveParticles + 1) < MaxParticlesAllowed); i++)
	{
		INT		ParticleIndex	= ParticleIndices[ActiveParticles];

		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndex);

		INT		TempOffset		= TypeDataOffset;

		INT						CurrentOffset		= TypeDataOffset;
		FTrail2TypeDataPayload*	TrailData			= NULL;
		FLOAT*					TaperValues			= NULL;

		TrailTypeData->GetDataPointers(this, (const BYTE*)Particle, CurrentOffset, 
			TrailData, TaperValues);

		FLOAT SpawnTime = StartTime - i * Increment;

		PreSpawn(Particle);
		for (INT n=0; n<LODLevel->SpawnModules.Num(); n++)
		{
			UParticleModule* SpawnModule = LODLevel->SpawnModules(n);
			if (!SpawnModule || !SpawnModule->bEnabled)
			{
				continue;
			}

			UINT* Offset = ModuleOffsetMap.Find(SpawnModule);
			SpawnModule->Spawn(this, Offset ? *Offset : 0, SpawnTime);
		}

		if ((1.0f / Particle->OneOverMaxLifetime) < 0.001f)
		{
			Particle->OneOverMaxLifetime = 1.f / 0.001f;
		}

		// The order of the Spawn here is VERY important as the modules may(will) depend on it occuring as such.
		if (TrailModule_Source && TrailModule_Source->bEnabled)
		{
			TrailModule_Source->Spawn(this, TrailModule_Source_Offset, DeltaTime);
		}
		if (TrailModule_Spawn && TrailModule_Spawn->bEnabled)
		{
			TrailModule_Spawn->Spawn(this, TrailModule_Spawn_Offset, DeltaTime);
		}
		if (TrailModule_Taper && TrailModule_Taper->bEnabled)
		{
			TrailModule_Taper->Spawn(this, TrailModule_Taper_Offset, DeltaTime);
		}
		if (LODLevel->TypeDataModule)
		{
			//@todo. Need to track TypeData offset into payload!
			LODLevel->TypeDataModule->Spawn(this, TypeDataOffset, SpawnTime);
		}

		PostSpawn(Particle, 1.f - FLOAT(i+1) / FLOAT(Number), SpawnTime);

		SourceDistanceTravelled(TrailData->TrailIndex) = 0.0f;
		LastSourcePosition(TrailData->TrailIndex)	= SourcePosition(TrailData->TrailIndex);
		SourcePosition(TrailData->TrailIndex) = Particle->Location;

		//debugf(TEXT("TrailEmitter: Spawn with tangent %s"), *(TrailData->Tangent.ToString()));
		FVector	SrcPos			= SourcePosition(TrailData->TrailIndex);
		FVector	LastSrcPos		= LastSourcePosition(TrailData->TrailIndex);
		FVector	CheckTangent	= SrcPos - LastSrcPos;
		CheckTangent.Normalize();
		//debugf(TEXT("TrailEmitter: CheckTangent       %s (%s - %s"), *CheckTangent.ToString(), *SrcPos.ToString(), *LastSrcPos.ToString());

		// Clear the next and previous - just to be safe
		TrailData->Flags = TRAIL_EMITTER_SET_NEXT(TrailData->Flags, TRAIL_EMITTER_NULL_NEXT);
		TrailData->Flags = TRAIL_EMITTER_SET_PREV(TrailData->Flags, TRAIL_EMITTER_NULL_PREV);

		// Set the tangents
		FVector	Dir	= Component->LocalToWorld.GetOrigin() - OldLocation;
		Dir.Normalize();
		TrailData->Tangent	=  Dir;

		UBOOL bAddedParticle = FALSE;
		// Determine which trail to attach to
		if (bNoLivingParticles)
		{
			// These are the first particles!
			// Tag it as the 'only'
			TrailData->Flags = TRAIL_EMITTER_SET_ONLY(TrailData->Flags);
			bNoLivingParticles	= FALSE;
			bAddedParticle		= TRUE;
		}
		else
		{
			INT iNextIndex = TRAIL_EMITTER_NULL_NEXT;
			INT iPrevIndex = TRAIL_EMITTER_NULL_PREV;

			// We need to check for existing particles, and 'link up' with them
			for (INT CheckIndex = 0; CheckIndex < ActiveParticles; CheckIndex++)
			{
				// Only care about 'head' particles...
				INT CheckParticleIndex = ParticleIndices[CheckIndex];

				// Don't check the particle of interest...
				// although this should never happen...
				if (ParticleIndex == CheckParticleIndex)
				{
					continue;
				}

				// Grab the particle and its associated trail data
				DECLARE_PARTICLE_PTR(CheckParticle, ParticleData + ParticleStride * CheckParticleIndex);

				CurrentOffset		= TypeDataOffset;

				FTrail2TypeDataPayload*	CheckTrailData		= NULL;
				FLOAT*					CheckTaperValues	= NULL;

				TrailTypeData->GetDataPointers(this, (const BYTE*)CheckParticle, CurrentOffset, 
					CheckTrailData, CheckTaperValues);

				//@todo. Determine how to handle multiple trails...
				if (TRAIL_EMITTER_IS_ONLY(CheckTrailData->Flags))
				{
					CheckTrailData->Flags	= TRAIL_EMITTER_SET_END(CheckTrailData->Flags);
					CheckTrailData->Flags	= TRAIL_EMITTER_SET_NEXT(CheckTrailData->Flags, TRAIL_EMITTER_NULL_NEXT);
					CheckTrailData->Flags	= TRAIL_EMITTER_SET_PREV(CheckTrailData->Flags, ParticleIndex);

					// Now, 'join' them
					TrailData->Flags		= TRAIL_EMITTER_SET_PREV(TrailData->Flags, TRAIL_EMITTER_NULL_NEXT);
					TrailData->Flags		= TRAIL_EMITTER_SET_NEXT(TrailData->Flags, CheckParticleIndex);
					TrailData->Flags		= TRAIL_EMITTER_SET_START(TrailData->Flags);

					bAddedParticle = TRUE;
					break;
				}
				else
					// ISSUE: How do we determine which 'trail' to join up with????
					if (TRAIL_EMITTER_IS_START(CheckTrailData->Flags))
					{
						check(TRAIL_EMITTER_GET_NEXT(CheckTrailData->Flags) != TRAIL_EMITTER_NULL_NEXT);

						CheckTrailData->Flags	= TRAIL_EMITTER_SET_MIDDLE(CheckTrailData->Flags);
						CheckTrailData->Flags	= TRAIL_EMITTER_SET_PREV(CheckTrailData->Flags, ParticleIndex);
						// Now, 'join' them
						TrailData->Flags		= TRAIL_EMITTER_SET_PREV(TrailData->Flags, TRAIL_EMITTER_NULL_NEXT);
						TrailData->Flags		= TRAIL_EMITTER_SET_NEXT(TrailData->Flags, CheckParticleIndex);
						TrailData->Flags		= TRAIL_EMITTER_SET_START(TrailData->Flags);

						//SourceDistanceTravelled(TrailData->TrailIndex) += SourceDistanceTravelled(CheckTrailData->TrailIndex);

						bAddedParticle = TRUE;
						break;
					}
			}
		}

		if (bAddedParticle)
		{
			TrailData->Tangent	= FVector(0.0f);
			ActiveParticles++;

			check((INT)ActiveParticles < TrailTypeData->MaxParticleInTrailCount);

			INC_DWORD_STAT(STAT_TrailParticles);
			INC_DWORD_STAT(STAT_TrailParticlesSpawned);

			LastEmittedParticleIndex = ParticleIndex;
		}
		else
		{
			check(TEXT("Failed to add particle to trail!!!!"));
		}
	}

	if (ForceSpawnCount > 0)
	{
		ForceSpawnCount = 0;
	}

	return NewLeftover;
}

/**
 *	Spawn particles for this instance
 *	Editor version for interpolation
 *
 *	@param	HighLODLevel	The higher LOD level
 *	@param	LowLODLevel		The lower LOD level
 *	@param	Multiplier		The interpolation value to use
 *	@param	OldLeftover		The leftover time from the last spawn
 *	@param	Rate			The rate at which particles should be spawned
 *	@param	DeltaTime		The time slice to spawn over
 *	@param	Burst			The number of burst particle
 *	@param	BurstTime		The burst time addition (faked time slice)
 *
 *	@return	FLOAT			The leftover fraction of spawning
 */
FLOAT FParticleTrail2EmitterInstance::SpawnEditor(
	UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, FLOAT Multiplier, 
	FLOAT OldLeftover, FLOAT Rate, FLOAT DeltaTime, INT Burst, FLOAT BurstTime)
{
	return Spawn(OldLeftover, Rate, DeltaTime, Burst, BurstTime);
}

/**
 *	Handle any pre-spawning actions required for particles
 *
 *	@param	Particle	The particle being spawned.
 */
void FParticleTrail2EmitterInstance::PreSpawn(FBaseParticle* Particle)
{
	FParticleEmitterInstance::PreSpawn(Particle);
	if (TrailTypeData)
	{
		TrailTypeData->PreSpawn(this, Particle);
	}
}

/**
 *	Kill off any dead particles. (Remove them from the active array)
 */
void FParticleTrail2EmitterInstance::KillParticles()
{
	if (ActiveParticles)
	{
		// Loop over the active particles... If their RelativeTime is > 1.0f (indicating they are dead),
		// move them to the 'end' of the active particle list.
		for (INT i = ActiveParticles - 1; i >= 0; i--)
		{
			const INT	CurrentIndex	= ParticleIndices[i];

			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * CurrentIndex);
			INT						CurrentOffset		= TypeDataOffset;
			FTrail2TypeDataPayload*	TrailData			= NULL;
			FLOAT*					TaperValues			= NULL;

			TrailTypeData->GetDataPointers(this, (const BYTE*)Particle, CurrentOffset, 
				TrailData, TaperValues);

			if (Particle->RelativeTime > 1.0f)
			{
#if defined(_TRAILS_DEBUG_KILL_PARTICLES_)
				debugf(TEXT("Killing Particle %4d - Next = %4d, Prev = %4d, Type = %8s"), 
					CurrentIndex, 
					TRAIL_EMITTER_GET_NEXT(TrailData->Flags),
					TRAIL_EMITTER_GET_PREV(TrailData->Flags),
					TRAIL_EMITTER_IS_ONLY(TrailData->Flags) ? TEXT("ONLY") :
					TRAIL_EMITTER_IS_START(TrailData->Flags) ? TEXT("START") :
					TRAIL_EMITTER_IS_END(TrailData->Flags) ? TEXT("END") :
					TRAIL_EMITTER_IS_MIDDLE(TrailData->Flags) ? TEXT("MIDDLE") :
					TEXT("????")
					);
#endif	//#if defined(_TRAILS_DEBUG_KILL_PARTICLES_)

				if (TRAIL_EMITTER_IS_START(TrailData->Flags) ||
					TRAIL_EMITTER_IS_ONLY(TrailData->Flags))
				{
					// Set the 'next' one in the list to the start
					INT Next = TRAIL_EMITTER_GET_NEXT(TrailData->Flags);
					if (Next != TRAIL_EMITTER_NULL_NEXT)
					{
						DECLARE_PARTICLE_PTR(NextParticle, ParticleData + ParticleStride * Next);

						CurrentOffset		= TypeDataOffset;

						FTrail2TypeDataPayload*	NextTrailData	= NULL;
						FLOAT*					NextTaperValues	= NULL;

						TrailTypeData->GetDataPointers(this, (const BYTE*)NextParticle, CurrentOffset, 
							NextTrailData, NextTaperValues);

						if (TRAIL_EMITTER_IS_END(NextTrailData->Flags))
						{
							NextTrailData->Flags = TRAIL_EMITTER_SET_ONLY(NextTrailData->Flags);
							check(TRAIL_EMITTER_GET_NEXT(NextTrailData->Flags) == TRAIL_EMITTER_NULL_NEXT);
						}
						else
						{
							NextTrailData->Flags = TRAIL_EMITTER_SET_START(NextTrailData->Flags);
						}
						NextTrailData->Flags = TRAIL_EMITTER_SET_PREV(NextTrailData->Flags, TRAIL_EMITTER_NULL_PREV);
					}
				}
				else
					if (TRAIL_EMITTER_IS_END(TrailData->Flags))
					{
						// See if there is a 'prev'
						INT Prev = TRAIL_EMITTER_GET_PREV(TrailData->Flags);
						if (Prev != TRAIL_EMITTER_NULL_PREV)
						{
							DECLARE_PARTICLE_PTR(PrevParticle, ParticleData + ParticleStride * Prev);
							CurrentOffset		= TypeDataOffset;

							FTrail2TypeDataPayload*	PrevTrailData	= NULL;
							FLOAT*					PrevTaperValues	= NULL;

							TrailTypeData->GetDataPointers(this, (const BYTE*)PrevParticle, CurrentOffset, 
								PrevTrailData, PrevTaperValues);
							if (TRAIL_EMITTER_IS_START(PrevTrailData->Flags))
							{
								PrevTrailData->Flags = TRAIL_EMITTER_SET_ONLY(PrevTrailData->Flags);
							}
							else
							{
								PrevTrailData->Flags = TRAIL_EMITTER_SET_END(PrevTrailData->Flags);
							}
							PrevTrailData->Flags = TRAIL_EMITTER_SET_NEXT(PrevTrailData->Flags, TRAIL_EMITTER_NULL_NEXT);
						}
					}
					else
						if (TRAIL_EMITTER_IS_MIDDLE(TrailData->Flags))
						{
							// Break the trail? Or kill off from here to the end

							INT	Next	= TRAIL_EMITTER_GET_NEXT(TrailData->Flags);
							INT	Prev	= TRAIL_EMITTER_GET_PREV(TrailData->Flags);

#define _TRAIL_KILL_BROKEN_SEGMENT_
#if defined(_TRAIL_KILL_BROKEN_SEGMENT_)
							if (Prev != TRAIL_EMITTER_NULL_PREV)
							{
								DECLARE_PARTICLE_PTR(PrevParticle, ParticleData + ParticleStride * Prev);
								CurrentOffset		= TypeDataOffset;

								FTrail2TypeDataPayload*	PrevTrailData	= NULL;
								FLOAT*					PrevTaperValues	= NULL;

								TrailTypeData->GetDataPointers(this, (const BYTE*)PrevParticle, CurrentOffset, 
									PrevTrailData, PrevTaperValues);
								if (TRAIL_EMITTER_IS_START(PrevTrailData->Flags))
								{
									PrevTrailData->Flags = TRAIL_EMITTER_SET_ONLY(PrevTrailData->Flags);
								}
								else
								{
									PrevTrailData->Flags = TRAIL_EMITTER_SET_END(PrevTrailData->Flags);
								}
								PrevTrailData->Flags = TRAIL_EMITTER_SET_NEXT(PrevTrailData->Flags, TRAIL_EMITTER_NULL_NEXT);
							}

							while (Next != TRAIL_EMITTER_NULL_NEXT)
							{
								DECLARE_PARTICLE_PTR(NextParticle, ParticleData + ParticleStride * Next);

								CurrentOffset		= TypeDataOffset;

								FTrail2TypeDataPayload*	NextTrailData	= NULL;
								FLOAT*					NextTaperValues	= NULL;

								TrailTypeData->GetDataPointers(this, (const BYTE*)NextParticle, CurrentOffset, 
									NextTrailData, NextTaperValues);

								Next	= TRAIL_EMITTER_GET_NEXT(NextTrailData->Flags);
								TRAIL_EMITTER_SET_FORCEKILL(NextTrailData->Flags);
							}
#else	//#if defined(_TRAIL_KILL_BROKEN_SEGMENT_)
							//@todo. Fill in code to make the broken segment a new trail??
							if (Next != TRAIL_EMITTER_NULL_NEXT)
							{
							}
#endif	//#if defined(_TRAIL_KILL_BROKEN_SEGMENT_)
						}
						else
							if (TRAIL_EMITTER_IS_FORCEKILL(TrailData->Flags))
							{
							}
							else
							{
								check(!TEXT("What the hell are you doing in here?"));
							}

							// Clear it out...
							TrailData->Flags	= TRAIL_EMITTER_SET_NEXT(TrailData->Flags, TRAIL_EMITTER_NULL_NEXT);
							TrailData->Flags	= TRAIL_EMITTER_SET_PREV(TrailData->Flags, TRAIL_EMITTER_NULL_PREV);

							ParticleIndices[i]	= ParticleIndices[ActiveParticles-1];
							ParticleIndices[ActiveParticles-1]	= CurrentIndex;
							ActiveParticles--;

							DEC_DWORD_STAT(STAT_TrailParticles);
							INC_DWORD_STAT(STAT_TrailParticlesKilled);
			}
		}
	}
}

/**
 *	Setup the modules for the trail emitter
 */
void FParticleTrail2EmitterInstance::SetupTrailModules()
{
	// Trails are a special case... 
	// We don't want standard Spawn/Update calls occuring on Trail-type modules.
	UParticleLODLevel* LODLevel = SpriteTemplate->GetLODLevel(0);
	check(LODLevel);
	for (INT ii = 0; ii < LODLevel->Modules.Num(); ii++)
	{
		UParticleModule* CheckModule = LODLevel->Modules(ii);
		if (CheckModule->GetModuleType() == EPMT_Trail)
		{
			UBOOL bRemove = FALSE;

			UINT* Offset;
			if (CheckModule->IsA(UParticleModuleTrailSource::StaticClass()))
			{
				if (TrailModule_Source)
				{
					debugf(TEXT("Warning: Multiple Trail Source modules!"));
				}
				TrailModule_Source	= Cast<UParticleModuleTrailSource>(CheckModule);
				Offset = ModuleOffsetMap.Find(TrailModule_Source);
				if (Offset)
				{
					TrailModule_Source_Offset	= *Offset;
				}
				bRemove	= TRUE;
			}
			else
				if (CheckModule->IsA(UParticleModuleTrailSpawn::StaticClass()))
				{
					if (TrailModule_Spawn)
					{
						debugf(TEXT("Warning: Multiple Trail spawn modules!"));
					}
					TrailModule_Spawn	= Cast<UParticleModuleTrailSpawn>(CheckModule);
					Offset = ModuleOffsetMap.Find(TrailModule_Spawn);
					if (Offset)
					{
						TrailModule_Spawn_Offset	= *Offset;
					}
					bRemove = TRUE;
				}
				else
					if (CheckModule->IsA(UParticleModuleTrailTaper::StaticClass()))
					{
						if (TrailModule_Taper)
						{
							debugf(TEXT("Warning: Multiple Trail taper modules!"));
						}
						TrailModule_Taper	= Cast<UParticleModuleTrailTaper>(CheckModule);
						Offset = ModuleOffsetMap.Find(TrailModule_Taper);
						if (Offset)
						{
							TrailModule_Taper_Offset	= *Offset;
						}
						bRemove = TRUE;
					}

					//@todo. Remove from the Update/Spawn lists???
					if (bRemove)
					{
						for (INT jj = 0; jj < LODLevel->UpdateModules.Num(); jj++)
						{
							if (LODLevel->UpdateModules(jj) == CheckModule)
							{
								LODLevel->UpdateModules.Remove(jj);
								break;
							}
						}

						for (INT kk = 0; kk < LODLevel->SpawnModules.Num(); kk++)
						{
							if (LODLevel->SpawnModules(kk) == CheckModule)
							{
								LODLevel->SpawnModules.Remove(kk);
								break;
							}
						}
					}
		}
	}
}

/**
 *	Resolve the source of the trail
 */
void FParticleTrail2EmitterInstance::ResolveSource()
{
	if (TrailModule_Source)
	{
		if (TrailModule_Source->SourceName != NAME_None)
		{
			switch (TrailModule_Source->SourceMethod)
			{
			case PET2SRCM_Actor:
				if (SourceActor == NULL)
				{
					FParticleSysParam Param;
					for (INT i = 0; i < Component->InstanceParameters.Num(); i++)
					{
						Param = Component->InstanceParameters(i);
						if (Param.Name == TrailModule_Source->SourceName)
						{
							SourceActor = Param.Actor;
							break;
						}
					}

					if (TrailModule_Source->SourceOffsetCount > 0)
					{
						for (INT i = 0; i < Component->InstanceParameters.Num(); i++)
						{
							Param = Component->InstanceParameters(i);
							FString ParamName = Param.Name.ToString();
							TCHAR* TrailSourceOffset	= appStrstr(*ParamName, TEXT("TrailSourceOffset"));
							if (TrailSourceOffset)
							{
								// Parse off the digit
								INT	Index	= appAtoi(TrailSourceOffset);
								if (Index >= 0)
								{
									if (Param.ParamType	== PSPT_Vector)
									{
										SourceOffsets.Insert(Index);
										SourceOffsets(Index)	= Param.Vector;
									}
									else
										if (Param.ParamType == PSPT_Scalar)
										{
											SourceOffsets.InsertZeroed(Index);
											SourceOffsets(Index)	= FVector(Param.Scalar, 0.0f, 0.0f);
										}
								}
							}
						}
					}
				}
				break;
			case PET2SRCM_Particle:
				if (SourceEmitter == NULL)
				{
					for (INT ii = 0; ii < Component->EmitterInstances.Num(); ii++)
					{
						FParticleEmitterInstance* pkEmitInst = Component->EmitterInstances(ii);
						if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == TrailModule_Source->SourceName))
						{
							SourceEmitter = pkEmitInst;
							break;
						}
					}
				}
				break;
			}
		}
	}
}

/**
 *	Update the source data for the trail
 *
 *	@param	DeltaTime		The time slice to use for the update
 */
void FParticleTrail2EmitterInstance::UpdateSourceData(FLOAT DeltaTime)
{
	FVector	Position = Component->LocalToWorld.GetOrigin();
	FVector	Dir	= Component->LocalToWorld.GetAxis(0);
	if (TrailModule_Source == NULL)
	{
		Dir.Normalize();
	}

	for (INT i = 0; i < ActiveParticles; i++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[i]);

		INT						CurrentOffset		= TypeDataOffset;
		FTrail2TypeDataPayload*	TrailData			= NULL;
		FLOAT*					TaperValues			= NULL;

		TrailTypeData->GetDataPointers(this, (const BYTE*)Particle, CurrentOffset, TrailData, TaperValues);
		if (TRAIL_EMITTER_IS_START(TrailData->Flags))
		{
			FVector	Tangent;
			if (TrailModule_Source)
			{
				TrailModule_Source->ResolveSourcePoint(this, *Particle, *TrailData, Position, Tangent);
			}
			else
			{
				Tangent		=  Dir;
			}

			//FVector	Delta = Position - CurrentSourcePosition(TrailData->TrailIndex);
#if 0
			FVector	Delta	= CurrentSourcePosition(TrailData->TrailIndex) - LastSourcePosition(TrailData->TrailIndex);
			debugf(TEXT("\tTrail %d (0x%08x) --> %s - Distance = %s (%f) | %s vs %s"), 
				TrailData->TrailIndex, (DWORD)this,
				*Position.ToString(),
				*Delta.ToString(), Delta.Size(),
				*CurrentSourcePosition(TrailData->TrailIndex).ToString(),
				*LastSourcePosition(TrailData->TrailIndex).ToString()
				);
#endif
			CurrentSourcePosition(TrailData->TrailIndex)	= Position;
		}
	}
}

/**
 *	Determine the vertex and triangle counts for the emitter
 */
void FParticleTrail2EmitterInstance::DetermineVertexAndTriangleCount()
{
	UINT	NewSize		= 0;
	INT		TessFactor	= TrailTypeData->TessellationFactor ? TrailTypeData->TessellationFactor : 1;
	INT		Sheets		= TrailTypeData->Sheets ? TrailTypeData->Sheets : 1;
	INT		TheTrailCount	= 0;
	INT		IndexCount	= 0;

	VertexCount		= 0;
	TriangleCount	= 0;

	FVector	TessDistCheck;
	FLOAT	TessRatio;
	INT		SegmentTessFactor;
	INT		CheckParticleCount = 0;

	for (INT ii = 0; ii < ActiveParticles; ii++)
	{
		INT		LocalVertexCount	= 0;
		INT		LocalIndexCount		= 0;

		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[ii]);

		INT						CurrentOffset		= TypeDataOffset;
		FTrail2TypeDataPayload*	TrailData			= NULL;
		FLOAT*					TaperValues			= NULL;

		TrailTypeData->GetDataPointers(this, (const BYTE*)Particle, CurrentOffset, 
			TrailData, TaperValues);

		FTrail2TypeDataPayload*	StartTrailData	= NULL;

		if (TRAIL_EMITTER_IS_START(TrailData->Flags))
		{
			StartTrailData		 = TrailData;

			// Count the number of particles in this trail
			INT	ParticleCount	 = 1;
			CheckParticleCount++;

			LocalVertexCount	+= 2;
			VertexCount			+= 2;
			LocalIndexCount		+= 2;

			UBOOL	bDone	= FALSE;

#if defined(_TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_)
			if (TrailTypeData->TessellationFactorDistance <= 0.0f)
#else	//#if defined(_TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_)
			if (1)
#endif	//#if defined(_TRAIL2_TESSELLATE_SCALE_BY_DISTANCE_)
			{
				while (!bDone)
				{
					ParticleCount++;
					CheckParticleCount++;

#if !defined(_TRAIL2_TESSELLATE_TO_SOURCE_)
					if (TRAIL_EMITTER_IS_START(TrailData->Flags))
					{
						LocalVertexCount	+= 2 * Sheets;
						VertexCount			+= 2 * Sheets;
						LocalIndexCount		+= 2 * Sheets;
					}
					else
#endif	//#if !defined(_TRAIL2_TESSELLATE_TO_SOURCE_)
					{
						LocalVertexCount	+= 2 * TessFactor * Sheets;
						VertexCount			+= 2 * TessFactor * Sheets;
						LocalIndexCount		+= 2 * TessFactor * Sheets;
					}

					// The end will have Next set to the NULL flag...
					INT	Next	= TRAIL_EMITTER_GET_NEXT(TrailData->Flags);
					if (Next == TRAIL_EMITTER_NULL_NEXT)
					{
						bDone = TRUE;
					}
					else
					{
						DECLARE_PARTICLE_PTR(NextParticle, ParticleData + ParticleStride * Next);

						CurrentOffset		= TypeDataOffset;
						TrailData			= NULL;
						TaperValues			= NULL;

						TrailTypeData->GetDataPointers(this, (const BYTE*)NextParticle, CurrentOffset, 
							TrailData, TaperValues);
					}
				}

				StartTrailData->TriangleCount	= LocalIndexCount - 2;

				// Handle degenerates - 4 tris per stitch
				LocalIndexCount	+= ((Sheets - 1) * 4);

				IndexCount	+= LocalIndexCount;

				TheTrailCount++;
			}
			else
			{
				while (!bDone)
				{
					SegmentTessFactor	= TessFactor;

					// The end will have Next set to the NULL flag...
					INT	Next	= TRAIL_EMITTER_GET_NEXT(TrailData->Flags);
					if (Next != TRAIL_EMITTER_NULL_NEXT)
					{
						DECLARE_PARTICLE_PTR(NextParticle, ParticleData + ParticleStride * Next);
						if (NextParticle->RelativeTime < 1.0f)
						{
							TessDistCheck		= (Particle->Location - NextParticle->Location);
							TessRatio			= TessDistCheck.Size() / TrailTypeData->TessellationFactorDistance;
							if (TessRatio <= KINDA_SMALL_NUMBER)
							{
								SegmentTessFactor	= 1;
							}
							else
								if (TessRatio < 1.0f)
								{
									SegmentTessFactor	= appTrunc(TessFactor * TessRatio);
								}
						}
						else
						{
							SegmentTessFactor	= 0;
						}
					}

					ParticleCount++;
					CheckParticleCount++;
					LocalVertexCount	+= 2 * SegmentTessFactor * Sheets;
					VertexCount			+= 2 * SegmentTessFactor * Sheets;
					LocalIndexCount		+= 2 * SegmentTessFactor * Sheets;

					// The end will have Next set to the NULL flag...
					if (Next == TRAIL_EMITTER_NULL_NEXT)
					{
						bDone = TRUE;
					}
					else
					{
						DECLARE_PARTICLE_PTR(NParticle, ParticleData + ParticleStride * Next);

						CurrentOffset		= TypeDataOffset;
						TrailData			= NULL;
						TaperValues			= NULL;

						TrailTypeData->GetDataPointers(this, (const BYTE*)NParticle, CurrentOffset, 
							TrailData, TaperValues);
					}
				}

				StartTrailData->TriangleCount	= LocalIndexCount - 2;

				// Handle degenerates - 4 tris per stitch
				LocalIndexCount	+= ((Sheets - 1) * 4);

				IndexCount	+= LocalIndexCount;

				TheTrailCount++;
			}
		}
	}

	if (TheTrailCount > 0)
	{
		IndexCount		+= 4 * (TheTrailCount - 1);	// 4 extra indices per Trail (degenerates)
		TriangleCount	 = IndexCount - 2;
	}
	else
	{
		IndexCount		= 0;
		TriangleCount	= 0;
	}
	//	TriangleCount	-= 1;

	//#define _TRAILS_DEBUG_VERT_TRI_COUNTS_
#if defined(_TRAILS_DEBUG_VERT_TRI_COUNTS_)
	debugf(TEXT("Trail VertexCount = %3d, TriangleCount = %3d"), VertexCount, TriangleCount);
#endif	//#if defined(_TRAILS_DEBUG_VERT_TRI_COUNTS_)
}

/**
 *	Retrieves the dynamic data for the emitter
 *	
 *	@param	bSelected					Whether the emitter is selected in the editor
 *
 *	@return	FDynamicEmitterDataBase*	The dynamic data, or NULL if it shouldn't be rendered
 */
FDynamicEmitterDataBase* FParticleTrail2EmitterInstance::GetDynamicData(UBOOL bSelected)
{
	// Validity check
	if (!SpriteTemplate)
	{
		return NULL;
	}

	// If the template is disabled, don't return data.
	UParticleLODLevel* LODLevel = SpriteTemplate->GetLODLevel(0);
	if ((LODLevel == NULL) ||
		(LODLevel->RequiredModule->bEnabled == FALSE))
	{
		return NULL;
	}

	DetermineVertexAndTriangleCount();
	if (TriangleCount <= 0)
	{
		if (ActiveParticles > 0)
		{
			warnf(TEXT("TRAIL: GetDynamicData -- TriangleCount == 0 (APC = %4d) for PSys %s"),
				ActiveParticles, 
				Component ? (Component->Template ? *Component->Template->GetName() : 
				TEXT("No Template")) : TEXT("No Component"));
		}
		return NULL;
	}

	// Allocate it for now, but we will want to change this to do some form
	// of caching
	if (ActiveParticles > 0)
	{
		check(MaxActiveParticles >= ActiveParticles);	// TTP #33385

		// Get the material instance. If there is none, or the material isn't flagged for use with particle systems, use the DefaultMaterial.
		UMaterialInterface* MaterialInst = LODLevel->RequiredModule->Material;
		if (MaterialInst == NULL || !MaterialInst->UseWithBeamTrails())
		{
			MaterialInst = GEngine->DefaultMaterial;
		}

		FDynamicTrail2EmitterData* NewEmitterData = ::new FDynamicTrail2EmitterData(MaxActiveParticles, ParticleStride, bRequiresSorting,
			MaterialInst->GetRenderProxy(bSelected));
		check(NewEmitterData);
		check(NewEmitterData->ParticleData);
		check(NewEmitterData->ParticleIndices);

		// Fill it in...
		INT						ParticleCount	= ActiveParticles;
		UBOOL					bSorted			= FALSE;
		const INT				ScreenAlignment	= SpriteTemplate->ScreenAlignment;

		NewEmitterData->Scale = FVector(1.0f, 1.0f, 1.0f);
		if (Component)
		{
			NewEmitterData->Scale *= Component->Scale * Component->Scale3D;
			AActor* Actor = Component->GetOwner();
			if (Actor && !Component->AbsoluteScale)
			{
				NewEmitterData->Scale *= Actor->DrawScale * Actor->DrawScale3D;
			}
		}

		NewEmitterData->ActiveParticleCount = ActiveParticles;
		NewEmitterData->ScreenAlignment = SpriteTemplate->ScreenAlignment;
		NewEmitterData->bUseLocalSpace = FALSE;
		NewEmitterData->EmitterRenderMode = LODLevel->RequiredModule->EmitterRenderMode;
		NewEmitterData->bLockAxis = FALSE;
		NewEmitterData->TessFactor = TrailTypeData->TessellationFactor ? TrailTypeData->TessellationFactor : 1;
		NewEmitterData->TessStrength = appTrunc(TrailTypeData->TessellationStrength);
		NewEmitterData->TessFactorDistance = TrailTypeData->TessellationFactorDistance;
		NewEmitterData->Sheets = TrailTypeData->Sheets ? TrailTypeData->Sheets : 1;
		NewEmitterData->bSelected = bSelected;

		INT CheckActiveParticles = ActiveParticles;

		NewEmitterData->VertexCount = VertexCount;
		NewEmitterData->IndexCount = TriangleCount + 2;
		NewEmitterData->PrimitiveCount = TriangleCount;
		NewEmitterData->TrailCount = TrailCount;

		//@todo.SAS. Check for requiring DWORD sized indices?
		NewEmitterData->IndexStride = sizeof(WORD);

		TrailTypeData->GetDataPointerOffsets(this, NULL, TypeDataOffset,
			NewEmitterData->TrailDataOffset, NewEmitterData->TaperValuesOffset);
		NewEmitterData->ParticleSourceOffset = -1;
		if (TrailModule_Source)
		{
			TrailModule_Source->GetDataPointerOffsets(this, NULL, 
				TrailModule_Source_Offset, NewEmitterData->ParticleSourceOffset);
		}

		//@todo. SORTING IS A DIFFERENT ISSUE NOW! 
		//		 GParticleView isn't going to be valid anymore?

		// For trails, we need the complete array of particle data and indices...
		check(NewEmitterData->ParticleData);		// TTP #33385
		appMemcpy(NewEmitterData->ParticleData, ParticleData, MaxActiveParticles * ParticleStride);
		check(NewEmitterData->ParticleIndices);		// TTP #33385
		appMemcpy(NewEmitterData->ParticleIndices, ParticleIndices, MaxActiveParticles * sizeof(WORD));

		//@todo.SAS. Optimize this nonsense...
		INT Index;
		NewEmitterData->TrailSpawnTimes.Empty(TrailSpawnTimes.Num());
		for (Index = 0; Index < TrailSpawnTimes.Num(); Index++)
		{
			NewEmitterData->TrailSpawnTimes.Add(appTrunc(TrailSpawnTimes(Index)));
		}
		NewEmitterData->SourcePosition.Empty(SourcePosition.Num());
		for (Index = 0; Index < SourcePosition.Num(); Index++)
		{
			NewEmitterData->SourcePosition.AddItem(SourcePosition(Index));
		}
		NewEmitterData->LastSourcePosition.Empty(LastSourcePosition.Num());
		for (Index = 0; Index < LastSourcePosition.Num(); Index++)
		{
			NewEmitterData->LastSourcePosition.AddItem(LastSourcePosition(Index));
		}
		NewEmitterData->CurrentSourcePosition.Empty(CurrentSourcePosition.Num());
		for (Index = 0; Index < CurrentSourcePosition.Num(); Index++)
		{
			NewEmitterData->CurrentSourcePosition.AddItem(CurrentSourcePosition(Index));
		}
		NewEmitterData->LastSpawnPosition.Empty(LastSpawnPosition.Num());
		for (Index = 0; Index < LastSpawnPosition.Num(); Index++)
		{
			NewEmitterData->LastSpawnPosition.AddItem(LastSpawnPosition(Index));
		}
		NewEmitterData->LastSpawnTangent.Empty(LastSpawnTangent.Num());
		for (Index = 0; Index < LastSpawnTangent.Num(); Index++)
		{
			NewEmitterData->LastSpawnTangent.AddItem(LastSpawnTangent(Index));
		}
		NewEmitterData->SourceDistanceTravelled.Empty(SourceDistanceTravelled.Num());
		for (Index = 0; Index < SourceDistanceTravelled.Num(); Index++)
		{
			NewEmitterData->SourceDistanceTravelled.AddItem(SourceDistanceTravelled(Index));
		}
		NewEmitterData->SourceOffsets.Empty(SourceOffsets.Num());
		for (Index = 0; Index < SourceOffsets.Num(); Index++)
		{
			NewEmitterData->SourceOffsets.AddItem(SourceOffsets(Index));
		}

		return NewEmitterData;
	}

	return NULL;
}
