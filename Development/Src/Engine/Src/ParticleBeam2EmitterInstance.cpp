/*=============================================================================
	FParticleBeam2EmitterInstance.cpp: 
	Particle beam emitter instance implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"

IMPLEMENT_PARTICLEEMITTERINSTANCE_TYPE(FParticleBeam2EmitterInstance);

/*-----------------------------------------------------------------------------
	ParticleBeam2EmitterInstance
-----------------------------------------------------------------------------*/
/**
 *	Structure for beam emitter instances
 */
/** Constructor	*/
FParticleBeam2EmitterInstance::FParticleBeam2EmitterInstance() :
	FParticleEmitterInstance()
	, BeamTypeData(NULL)
	, BeamModule_Source(NULL)
	, BeamModule_Target(NULL)
	, BeamModule_Noise(NULL)
	, BeamModule_SourceModifier(NULL)
	, BeamModule_SourceModifier_Offset(-1)
	, BeamModule_TargetModifier(NULL)
	, BeamModule_TargetModifier_Offset(-1)
	, FirstEmission(TRUE)
	, LastEmittedParticleIndex(-1)
	, TickCount(0)
	, ForceSpawnCount(0)
	, BeamMethod(0)
	, BeamCount(0)
	, SourceActor(NULL)
	, SourceEmitter(NULL)
	, TargetActor(NULL)
	, TargetEmitter(NULL)
	, VertexCount(0)
	, TriangleCount(0)
{
	TextureTiles.Empty();
	UserSetSourceArray.Empty();
	UserSetSourceTangentArray.Empty();
	UserSetSourceStrengthArray.Empty();
	DistanceArray.Empty();
	TargetPointArray.Empty();
	TargetTangentArray.Empty();
	UserSetTargetStrengthArray.Empty();
	TargetPointSourceNames.Empty();
	UserSetTargetArray.Empty();
	UserSetTargetTangentArray.Empty();
	BeamTrianglesPerSheet.Empty();
}

/** Destructor	*/
FParticleBeam2EmitterInstance::~FParticleBeam2EmitterInstance()
{
	TextureTiles.Empty();
	UserSetSourceArray.Empty();
	UserSetSourceTangentArray.Empty();
	UserSetSourceStrengthArray.Empty();
	DistanceArray.Empty();
	TargetPointArray.Empty();
	TargetTangentArray.Empty();
	UserSetTargetStrengthArray.Empty();
	TargetPointSourceNames.Empty();
	UserSetTargetArray.Empty();
	UserSetTargetTangentArray.Empty();
	BeamTrianglesPerSheet.Empty();
}

// Accessors
/**
 *	Set the beam type
 *
 *	@param	NewMethod	
 */
void FParticleBeam2EmitterInstance::SetBeamType(INT NewMethod)
{
}

/**
 *	Set the tessellation factor
 *
 *	@param	NewFactor
 */
void FParticleBeam2EmitterInstance::SetTessellationFactor(FLOAT NewFactor)
{
}

/**
 *	Set the end point position
 *
 *	@param	NewEndPoint
 */
void FParticleBeam2EmitterInstance::SetEndPoint(FVector NewEndPoint)
{
	if (UserSetTargetArray.Num() < 1)
	{
		UserSetTargetArray.Add(1);

	}
	UserSetTargetArray(0) = NewEndPoint;
}

/**
 *	Set the distance
 *
 *	@param	Distance
 */
void FParticleBeam2EmitterInstance::SetDistance(FLOAT Distance)
{
}

/**
 *	Set the source point
 *
 *	@param	NewSourcePoint
 *	@param	SourceIndex			The index of the source being set
 */
void FParticleBeam2EmitterInstance::SetSourcePoint(FVector NewSourcePoint,INT SourceIndex)
{
	if (SourceIndex < 0)
		return;

	if (UserSetSourceArray.Num() < (SourceIndex + 1))
	{
		UserSetSourceArray.Add((SourceIndex + 1) - UserSetSourceArray.Num());
	}
	UserSetSourceArray(SourceIndex) = NewSourcePoint;
}

/**
 *	Set the source tangent
 *
 *	@param	NewTangentPoint		The tangent value to set it to
 *	@param	SourceIndex			The index of the source being set
 */
void FParticleBeam2EmitterInstance::SetSourceTangent(FVector NewTangentPoint,INT SourceIndex)
{
	if (SourceIndex < 0)
		return;

	if (UserSetSourceTangentArray.Num() < (SourceIndex + 1))		
	{
		UserSetSourceTangentArray.Add((SourceIndex + 1) - UserSetSourceTangentArray.Num());
	}
	UserSetSourceTangentArray(SourceIndex) = NewTangentPoint;
}

/**
 *	Set the source strength
 *
 *	@param	NewSourceStrength	The source strenght to set it to
 *	@param	SourceIndex			The index of the source being set
 */
void FParticleBeam2EmitterInstance::SetSourceStrength(FLOAT NewSourceStrength,INT SourceIndex)
{
	if (SourceIndex < 0)
		return;

	if (UserSetSourceStrengthArray.Num() < (SourceIndex + 1))
	{
		UserSetSourceStrengthArray.Add((SourceIndex + 1) - UserSetSourceStrengthArray.Num());
	}
	UserSetSourceStrengthArray(SourceIndex) = NewSourceStrength;
}

/**
 *	Set the target point
 *
 *	@param	NewTargetPoint		The target point to set it to
 *	@param	TargetIndex			The index of the target being set
 */
void FParticleBeam2EmitterInstance::SetTargetPoint(FVector NewTargetPoint,INT TargetIndex)
{
	if (TargetIndex < 0)
		return;

	if (UserSetTargetArray.Num() < (TargetIndex + 1))
	{
		UserSetTargetArray.Add((TargetIndex + 1) - UserSetTargetArray.Num());
	}
	UserSetTargetArray(TargetIndex) = NewTargetPoint;
}

/**
 *	Set the target tangent
 *
 *	@param	NewTangentPoint		The tangent to set it to
 *	@param	TargetIndex			The index of the target being set
 */
void FParticleBeam2EmitterInstance::SetTargetTangent(FVector NewTangentPoint,INT TargetIndex)
{
	if (TargetIndex < 0)
		return;

	if (UserSetTargetTangentArray.Num() < (TargetIndex + 1))
	{
		UserSetTargetTangentArray.Add((TargetIndex + 1) - UserSetTargetTangentArray.Num());
	}
	UserSetTargetTangentArray(TargetIndex) = NewTangentPoint;
}

/**
 *	Set the target strength
 *
 *	@param	NewTargetStrength	The strength to set it ot
 *	@param	TargetIndex			The index of the target being set
 */
void FParticleBeam2EmitterInstance::SetTargetStrength(FLOAT NewTargetStrength,INT TargetIndex)
{
	if (TargetIndex < 0)
		return;

	if (UserSetTargetStrengthArray.Num() < (TargetIndex + 1))
	{
		UserSetTargetStrengthArray.Add((TargetIndex + 1) - UserSetTargetStrengthArray.Num());
	}
	UserSetTargetStrengthArray(TargetIndex) = NewTargetStrength;
}

/**
 *	Initialize the parameters for the structure
 *
 *	@param	InTemplate		The ParticleEmitter to base the instance on
 *	@param	InComponent		The owning ParticleComponent
 *	@param	bClearResources	If TRUE, clear all resource data
 */
void FParticleBeam2EmitterInstance::InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent, UBOOL bClearResources)
{
	FParticleEmitterInstance::InitParameters(InTemplate, InComponent, bClearResources);

	UParticleLODLevel* LODLevel	= InTemplate->GetLODLevel(0);
	check(LODLevel);
	BeamTypeData = CastChecked<UParticleModuleTypeDataBeam2>(LODLevel->TypeDataModule);
	check(BeamTypeData);

	//@todo. Determine if we need to support local space.
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		LODLevel->RequiredModule->bUseLocalSpace	= FALSE;
	}

	BeamModule_Source			= NULL;
	BeamModule_Target			= NULL;
	BeamModule_Noise			= NULL;
	BeamModule_SourceModifier	= NULL;
	BeamModule_TargetModifier	= NULL;

	// Always have at least one beam
	if (BeamTypeData->MaxBeamCount == 0)
	{
		BeamTypeData->MaxBeamCount	= 1;
	}

	BeamCount					= BeamTypeData->MaxBeamCount;
	FirstEmission				= TRUE;
	LastEmittedParticleIndex	= -1;
	TickCount					= 0;
	ForceSpawnCount				= 0;

	BeamMethod					= BeamTypeData->BeamMethod;

	TextureTiles.Empty();
	TextureTiles.AddItem(BeamTypeData->TextureTile);

	UserSetSourceArray.Empty();
	UserSetSourceTangentArray.Empty();
	UserSetSourceStrengthArray.Empty();
	DistanceArray.Empty();
	TargetPointArray.Empty();
	TargetPointSourceNames.Empty();
	UserSetTargetArray.Empty();
	UserSetTargetTangentArray.Empty();
	UserSetTargetStrengthArray.Empty();

	// Resolve any actors...
	ResolveSource();
	ResolveTarget();
}

/**
 *	Initialize the instance
 */
void FParticleBeam2EmitterInstance::Init()
{
	// Setup the modules prior to initializing...
	SetupBeamModules();
	FParticleEmitterInstance::Init();
	SetupBeamModifierModules();
}

/**
 *	Tick the instance.
 *
 *	@param	DeltaTime			The time slice to use
 *	@param	bSuppressSpawning	If TRUE, do not spawn during Tick
 */
void FParticleBeam2EmitterInstance::Tick(FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	SCOPE_CYCLE_COUNTER(STAT_BeamTickTime);
	if (Component)
	{
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

		// Increment the time alive
		SecondsSinceCreation += DeltaTime;

		// Update time within emitter loop.
		EmitterTime = SecondsSinceCreation;

		// We don't support beam LOD, so always use level 0
		UParticleLODLevel* LODLevel	= SpriteTemplate->GetLODLevel(0);
		check(LODLevel);	// TTP #33141

		if (EmitterDuration > KINDA_SMALL_NUMBER)
		{
			EmitterTime = appFmod(SecondsSinceCreation, EmitterDuration);
		}

		// Take the delay into account
		FLOAT EmitterDelay = LODLevel->RequiredModule->EmitterDelay;

		// Handle looping
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

		// Only delay if requested
		if ((LODLevel->RequiredModule->bDelayFirstLoopOnly == TRUE) && (LoopCount > 0))
		{
			EmitterDelay = 0;
		}

		// 'Reset' the emitter time so that the modules function correctly
		EmitterTime -= EmitterDelay;

		// Kill before the spawn... Otherwise, we can get 'flashing'
		KillParticles();

		// If not suppressing spawning...
		if (!bSuppressSpawning && (EmitterTime >= 0.0f))
		{
			if ((LODLevel->RequiredModule->EmitterLoops == 0) || 
				(LoopCount < LODLevel->RequiredModule->EmitterLoops) ||
				(SecondsSinceCreation < (EmitterDuration * LODLevel->RequiredModule->EmitterLoops)))
			{
				// For beams, we probably want to ignore the SpawnRate distribution,
				// and focus strictly on the BurstList...
				FLOAT SpawnRate = 0.0f;
				// Figure out spawn rate for this tick.
				SpawnRate = LODLevel->RequiredModule->SpawnRate.GetValue(EmitterTime, Component);
				// Take Bursts into account as well...
				INT		Burst		= 0;
				FLOAT	BurstTime	= GetCurrentBurstRateOffset(DeltaTime, Burst);
				SpawnRate += BurstTime;

				// Spawn new particles...

				//@todo. Fix the issue of 'blanking' beams when the count drops...
				// This is a temporary hack!
				if ((ActiveParticles < BeamCount) && (SpawnRate <= 0.0f))
				{
					// Force the spawn of a single beam...
					SpawnRate = 1.0f / DeltaTime;
				}

				// Force beams if the emitter is marked "AlwaysOn"
				if ((ActiveParticles < BeamCount) && BeamTypeData->bAlwaysOn)
				{
					Burst		= BeamCount;
					if (DeltaTime > KINDA_SMALL_NUMBER)
					{
						BurstTime	 = Burst / DeltaTime;
						SpawnRate	+= BurstTime;
					}
				}

				if (SpawnRate > 0.f)
				{
					SpawnFraction = Spawn(SpawnFraction, SpawnRate, DeltaTime, Burst, BurstTime);
				}
			}
		}

		// Reset particle data
		ResetParticleParameters(DeltaTime, STAT_BeamParticlesUpdated);

		// Type data module
		UParticleModuleTypeDataBase* pkBase = 0;
		if (LODLevel->TypeDataModule)
		{
			pkBase = Cast<UParticleModuleTypeDataBase>(LODLevel->TypeDataModule);
			//@todo. Need to track TypeData offset into payload!
			pkBase->PreUpdate(this, TypeDataOffset, DeltaTime);
		}

		// Update existing particles (might respawn dying ones).
		for (INT i = 0; i < LODLevel->UpdateModules.Num(); i++)
		{
			UParticleModule* ParticleModule	= LODLevel->UpdateModules(i);
			if (!ParticleModule || !ParticleModule->bEnabled)
			{
				continue;
			}
			UINT* Offset = ModuleOffsetMap.Find(ParticleModule);
			ParticleModule->Update(this, Offset ? *Offset : 0, DeltaTime);
		}

		//@todo. This should ALWAYS be true for Beams...
		if (pkBase)
		{
			// The order of the update here is VERY important
			UINT* Offset;
			if (BeamModule_Source && BeamModule_Source->bEnabled)
			{
				Offset = ModuleOffsetMap.Find(BeamModule_Source);
				BeamModule_Source->Update(this, Offset ? *Offset : 0, DeltaTime);
			}
			if (BeamModule_SourceModifier && BeamModule_SourceModifier->bEnabled)
			{
				INT TempOffset = BeamModule_SourceModifier_Offset;
				BeamModule_SourceModifier->Update(this, TempOffset, DeltaTime);
			}
			if (BeamModule_Target && BeamModule_Target->bEnabled)
			{
				Offset = ModuleOffsetMap.Find(BeamModule_Target);
				BeamModule_Target->Update(this, Offset ? *Offset : 0, DeltaTime);
			}
			if (BeamModule_TargetModifier && BeamModule_TargetModifier->bEnabled)
			{
				INT TempOffset = BeamModule_TargetModifier_Offset;
				BeamModule_TargetModifier->Update(this, TempOffset, DeltaTime);
			}
			if (BeamModule_Noise && BeamModule_Noise->bEnabled)
			{
				Offset = ModuleOffsetMap.Find(BeamModule_Noise);
				BeamModule_Noise->Update(this, Offset ? *Offset : 0, DeltaTime);
			}

			// The TypeData module update will always happen last...
			// This is required, as it will operate on the results from the other
			// beam-specific modules.
			pkBase->Update(this, TypeDataOffset, DeltaTime);
			pkBase->PostUpdate(this, TypeDataOffset, DeltaTime);
		}

		// Calculate bounding box and simulate velocity.
		UpdateBoundingBox(DeltaTime);

		if (!bSuppressSpawning)
		{
			// Ensure that we flip the 'FirstEmission' flag
			FirstEmission = FALSE;
		}

		// Invalidate the contents of the vertex/index buffer.
		IsRenderDataDirty = 1;

		// Bump the tick count
		TickCount++;

		// 'Reset' the emitter time so that the delay functions correctly
		EmitterTime += EmitterDelay;
	}
	INC_DWORD_STAT(STAT_BeamParticlesUpdateCalls);
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
void FParticleBeam2EmitterInstance::TickEditor(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, FLOAT Multiplier, FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	Tick(DeltaTime, bSuppressSpawning);
}

/**
 *	Update the bounding box for the emitter
 *
 *	@param	DeltaTime		The time slice to use
 */
void FParticleBeam2EmitterInstance::UpdateBoundingBox(FLOAT DeltaTime)
{
	if (Component)
	{
		FLOAT MaxSizeScale	= 1.0f;
		ParticleBoundingBox.Init();

		//@todo. Currently, we don't support UseLocalSpace for beams
		//if (Template->UseLocalSpace == false) 
		{
			ParticleBoundingBox += Component->LocalToWorld.GetOrigin();
		}

		FVector	NoiseMin(0.0f);
		FVector NoiseMax(0.0f);
		// Noise points have to be taken into account...
		if (BeamModule_Noise)
		{
			BeamModule_Noise->GetNoiseRange(NoiseMin, NoiseMax);
		}

		// Take scale into account as well
		FVector Scale = FVector(1.0f, 1.0f, 1.0f);
		Scale *= Component->Scale * Component->Scale3D;
		AActor* Actor = Component->GetOwner();
		if (Actor && !Component->AbsoluteScale)
		{
			Scale *= Actor->DrawScale * Actor->DrawScale3D;
		}

		// Take each particle into account
		for (INT i=0; i<ActiveParticles; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[i]);

			INT						CurrentOffset		= TypeDataOffset;
			FBeam2TypeDataPayload*	BeamData			= NULL;
			FVector*				InterpolatedPoints	= NULL;
			FLOAT*					NoiseRate			= NULL;
			FLOAT*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			FLOAT*					TaperValues			= NULL;
			FLOAT*					NoiseDistanceScale	= NULL;
			FBeamParticleModifierPayloadData* SourceModifier = NULL;
			FBeamParticleModifierPayloadData* TargetModifier = NULL;

			BeamTypeData->GetDataPointers(this, (const BYTE*)Particle, CurrentOffset, 
				BeamData, InterpolatedPoints, NoiseRate, NoiseDelta, TargetNoisePoints, 
				NextNoisePoints, TaperValues, NoiseDistanceScale,
				SourceModifier, TargetModifier);

			// Do linear integrator and update bounding box
			Particle->OldLocation = Particle->Location;
			Particle->Location	+= DeltaTime * Particle->Velocity;
			ParticleBoundingBox += Particle->Location;
			ParticleBoundingBox += Particle->Location + NoiseMin;
			ParticleBoundingBox += Particle->Location + NoiseMax;
			ParticleBoundingBox += BeamData->SourcePoint;
			ParticleBoundingBox += BeamData->SourcePoint + NoiseMin;
			ParticleBoundingBox += BeamData->SourcePoint + NoiseMax;
			ParticleBoundingBox += BeamData->TargetPoint;
			ParticleBoundingBox += BeamData->TargetPoint + NoiseMin;
			ParticleBoundingBox += BeamData->TargetPoint + NoiseMax;

			// Do angular integrator, and wrap result to within +/- 2 PI
			Particle->Rotation	+= DeltaTime * Particle->RotationRate;
			Particle->Rotation	 = appFmod(Particle->Rotation, 2.f*(FLOAT)PI);
			FVector Size = Particle->Size * Scale;
			MaxSizeScale		 = Max(MaxSizeScale, Size.GetAbsMax()); //@todo particles: this does a whole lot of compares that can be avoided using SSE/ Altivec.
		}
		ParticleBoundingBox = ParticleBoundingBox.ExpandBy(MaxSizeScale);

		//@todo. Transform bounding box into world space if the emitter uses a local space coordinate system.
		/***
		if (Template->UseLocalSpace) 
		{
		ParticleBoundingBox = ParticleBoundingBox.TransformBy(Component->LocalToWorld);
		}
		***/
	}
}

/**
 *	Retrieved the per-particle bytes that this emitter type requires.
 *
 *	@return	UINT	The number of required bytes for particles in the instance
 */
UINT FParticleBeam2EmitterInstance::RequiredBytes()
{
	UINT uiBytes = FParticleEmitterInstance::RequiredBytes();

	// Flag bits indicating particle 
	uiBytes += sizeof(INT);

	return uiBytes;
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
FLOAT FParticleBeam2EmitterInstance::Spawn(FLOAT OldLeftover, FLOAT Rate, FLOAT DeltaTime, INT Burst, FLOAT BurstTime)
{
	FLOAT	NewLeftover = OldLeftover + DeltaTime * Rate;

	SCOPE_CYCLE_COUNTER(STAT_BeamSpawnTime);

	// Ensure continous spawning... lots of fiddling.
	INT		Number		= appFloor(NewLeftover);
	FLOAT	Increment	= 1.f / Rate;
	FLOAT	StartTime	= DeltaTime + OldLeftover * Increment - Increment;
	NewLeftover			= NewLeftover - Number;

	// Always match the burst at a minimum
	if (Number < Burst)
	{
		Number = Burst;
	}

	// Account for burst time simulation
	if (BurstTime > KINDA_SMALL_NUMBER)
	{
		NewLeftover -= BurstTime / Burst;
		NewLeftover	= Clamp<FLOAT>(NewLeftover, 0, NewLeftover);
	}

	// Force a beam
	UBOOL bNoLivingParticles = false;
	if (ActiveParticles == 0)
	{
		bNoLivingParticles = true;
		if (Number == 0)
			Number = 1;
	}

	// Don't allow more than BeamCount beams...
	if (Number + ActiveParticles > BeamCount)
	{
		Number	= BeamCount - ActiveParticles;
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

	UParticleLODLevel* LODLevel	= SpriteTemplate->GetLODLevel(0);
	check(LODLevel);

	// Spawn particles.
	for (INT i=0; i<Number; i++)
	{
		INT iParticleIndex = ParticleIndices[ActiveParticles];

		DECLARE_PARTICLE_PTR(pkParticle, ParticleData + ParticleStride * iParticleIndex);

		FLOAT SpawnTime = StartTime - i * Increment;

		PreSpawn(pkParticle);
		for (INT n = 0; n < LODLevel->SpawnModules.Num(); n++)
		{
			UParticleModule* SpawnModule	= LODLevel->SpawnModules(n);
			UINT* Offset = ModuleOffsetMap.Find(SpawnModule);
			if (SpawnModule->bEnabled)
			{
				SpawnModule->Spawn(this, Offset ? *Offset : 0, SpawnTime);
			}
		}

		// The order of the Spawn here is VERY important as the modules may(will) depend on it occuring as such.
		UINT* Offset;
		if (BeamModule_Source && BeamModule_Source->bEnabled)
		{
			Offset = ModuleOffsetMap.Find(BeamModule_Source);
			BeamModule_Source->Spawn(this, Offset ? *Offset : 0, DeltaTime);
		}
		if (BeamModule_SourceModifier && BeamModule_SourceModifier->bEnabled)
		{
			INT Offset = BeamModule_SourceModifier_Offset;
			BeamModule_SourceModifier->Spawn(this, Offset, DeltaTime);
		}
		if (BeamModule_Target && BeamModule_Target->bEnabled)
		{
			Offset = ModuleOffsetMap.Find(BeamModule_Target);
			BeamModule_Target->Spawn(this, Offset ? *Offset : 0, DeltaTime);
		}
		if (BeamModule_TargetModifier && BeamModule_TargetModifier->bEnabled)
		{
			INT Offset = BeamModule_TargetModifier_Offset;
			BeamModule_TargetModifier->Spawn(this, Offset, DeltaTime);
		}
		if (BeamModule_Noise && BeamModule_Noise->bEnabled)
		{
			Offset = ModuleOffsetMap.Find(BeamModule_Noise);
			BeamModule_Noise->Spawn(this, Offset ? *Offset : 0, DeltaTime);
		}
		if (LODLevel->TypeDataModule)
		{
			//@todo. Need to track TypeData offset into payload!
			LODLevel->TypeDataModule->Spawn(this, TypeDataOffset, SpawnTime);
		}

		PostSpawn(pkParticle, 1.f - FLOAT(i+1) / FLOAT(Number), SpawnTime);

		ActiveParticles++;

		INC_DWORD_STAT(STAT_BeamParticles);
		INC_DWORD_STAT(STAT_BeamParticlesSpawned);

		LastEmittedParticleIndex = iParticleIndex;
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
FLOAT FParticleBeam2EmitterInstance::SpawnEditor(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, FLOAT Multiplier, 
												 FLOAT OldLeftover, FLOAT Rate, FLOAT DeltaTime, INT Burst, FLOAT BurstTime)
{
	return Spawn(OldLeftover, Rate, DeltaTime, Burst, BurstTime);
}

/**
 *	Handle any pre-spawning actions required for particles
 *
 *	@param	Particle	The particle being spawned.
 */
void FParticleBeam2EmitterInstance::PreSpawn(FBaseParticle* Particle)
{
	FParticleEmitterInstance::PreSpawn(Particle);
	if (BeamTypeData)
	{
		BeamTypeData->PreSpawn(this, Particle);
	}
}

/**
 *	Kill off any dead particles. (Remove them from the active array)
 */
void FParticleBeam2EmitterInstance::KillParticles()
{
	// Loop over the active particles... If their RelativeTime is > 1.0f (indicating they are dead),
	// move them to the 'end' of the active particle list.
	for (INT i=ActiveParticles-1; i>=0; i--)
	{
		const INT	CurrentIndex	= ParticleIndices[i];
		const BYTE* ParticleBase	= ParticleData + CurrentIndex * ParticleStride;
		FBaseParticle& Particle		= *((FBaseParticle*) ParticleBase);
		if (Particle.RelativeTime > 1.0f)
		{
			ParticleIndices[i]	= ParticleIndices[ActiveParticles-1];
			ParticleIndices[ActiveParticles-1]	= CurrentIndex;
			ActiveParticles--;

			DEC_DWORD_STAT(STAT_BeamParticles);
			INC_DWORD_STAT(STAT_BeamParticlesKilled);
		}
	}
}

/**
 *	Setup the beam module pointers
 */
void FParticleBeam2EmitterInstance::SetupBeamModules()
{
	// Beams are a special case... 
	// We don't want standard Spawn/Update calls occuring on Beam-type modules.
	UParticleLODLevel* LODLevel	= SpriteTemplate->GetLODLevel(0);
	check(LODLevel);

	// Go over all the modules in the LOD level
	for (INT ii = 0; ii < LODLevel->Modules.Num(); ii++)
	{
		UParticleModule* CheckModule = LODLevel->Modules(ii);
		if (CheckModule->GetModuleType() == EPMT_Beam)
		{
			UBOOL bRemove = FALSE;

			if (CheckModule->IsA(UParticleModuleBeamSource::StaticClass()))
			{
				//if (BeamModule_Source)
				//{
				//	debugf(TEXT("Warning: Multiple beam source modules!"));
				//}
				BeamModule_Source	= Cast<UParticleModuleBeamSource>(CheckModule);
				bRemove = TRUE;
			}
			else
			if (CheckModule->IsA(UParticleModuleBeamTarget::StaticClass()))
			{
				//if (BeamModule_Target)
				//{
				//	debugf(TEXT("Warning: Multiple beam Target modules!"));
				//}
				BeamModule_Target	= Cast<UParticleModuleBeamTarget>(CheckModule);
				bRemove = TRUE;
			}
			else
			if (CheckModule->IsA(UParticleModuleBeamNoise::StaticClass()))
			{
				//if (BeamModule_Noise)
				//{
				//	debugf(TEXT("Warning: Multiple beam Noise modules!"));
				//}
				BeamModule_Noise	= Cast<UParticleModuleBeamNoise>(CheckModule);
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
*	Setup the beam modifer module pointers
*/
void FParticleBeam2EmitterInstance::SetupBeamModifierModules()
{
	// Beams are a special case... 
	// We don't want standard Spawn/Update calls occuring on Beam-type modules.
	UParticleLODLevel* LODLevel	= SpriteTemplate->GetLODLevel(0);
	check(LODLevel);

	// Go over all the modules in the LOD level
	for (INT ii = 0; ii < LODLevel->Modules.Num(); ii++)
	{
		UParticleModule* CheckModule = LODLevel->Modules(ii);
		if (CheckModule->GetModuleType() == EPMT_Beam)
		{
			UBOOL bRemove = FALSE;

			if (CheckModule->IsA(UParticleModuleBeamModifier::StaticClass()))
			{
				UParticleModuleBeamModifier* ModifyModule = Cast<UParticleModuleBeamModifier>(CheckModule);
				if (ModifyModule->PositionOptions.bModify || ModifyModule->TangentOptions.bModify || ModifyModule->StrengthOptions.bModify)
				{
					if (ModifyModule->ModifierType == PEB2MT_Source)
					{
						BeamModule_SourceModifier = ModifyModule;
						bRemove = TRUE;
						UINT* Offset = ModuleOffsetMap.Find(CheckModule);
						if (Offset)
						{
							BeamModule_SourceModifier_Offset = (INT)(*Offset);
						}
					}
					else
					if (ModifyModule->ModifierType == PEB2MT_Target)
					{
						BeamModule_TargetModifier = ModifyModule;
						bRemove = TRUE;
						UINT* Offset = ModuleOffsetMap.Find(CheckModule);
						if (Offset)
						{
							BeamModule_TargetModifier_Offset = (INT)(*Offset);
						}
					}
				}
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
 *	Resolve the source for the beam
 */
void FParticleBeam2EmitterInstance::ResolveSource()
{
	if (BeamModule_Source)
	{
		if (BeamModule_Source->SourceName != NAME_None)
		{
			switch (BeamModule_Source->SourceMethod)
			{
			case PEB2STM_Actor:
				if (SourceActor == NULL)
				{
					FParticleSysParam Param;
					for (INT i = 0; i < Component->InstanceParameters.Num(); i++)
					{
						Param = Component->InstanceParameters(i);
						if (Param.Name == BeamModule_Source->SourceName)
						{
							SourceActor = Param.Actor;
							break;
						}
					}
				}
				break;
			case PEB2STM_Emitter:
			case PEB2STM_Particle:
				if (SourceEmitter == NULL)
				{
					for (INT ii = 0; ii < Component->EmitterInstances.Num(); ii++)
					{
						FParticleEmitterInstance* pkEmitInst = Component->EmitterInstances(ii);
						if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == BeamModule_Source->SourceName))
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
 *	Resolve the target for the beam
 */
void FParticleBeam2EmitterInstance::ResolveTarget()
{
	if (BeamModule_Target)
	{
		if (BeamModule_Target->TargetName != NAME_None)
		{
			switch (BeamModule_Target->TargetMethod)
			{
			case PEB2STM_Actor:
				if (TargetActor == NULL)
				{
					FParticleSysParam Param;
					for (INT i = 0; i < Component->InstanceParameters.Num(); i++)
					{
						Param = Component->InstanceParameters(i);
						if (Param.Name == BeamModule_Target->TargetName)
						{
							TargetActor = Param.Actor;
							break;
						}
					}
				}
				break;
			case PEB2STM_Emitter:
				if (TargetEmitter == NULL)
				{
					for (INT ii = 0; ii < Component->EmitterInstances.Num(); ii++)
					{
						FParticleEmitterInstance* pkEmitInst = Component->EmitterInstances(ii);
						if (pkEmitInst && (pkEmitInst->SpriteTemplate->EmitterName == BeamModule_Target->TargetName))
						{
							TargetEmitter = pkEmitInst;
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
 *	Determine the vertex and triangle counts for the emitter
 */
void FParticleBeam2EmitterInstance::DetermineVertexAndTriangleCount()
{
	// Need to determine # tris per beam...
	INT VerticesToRender = 0;
	INT TrianglesToRender = 0;

	check(BeamTypeData);
	INT Sheets = BeamTypeData->Sheets ? BeamTypeData->Sheets : 1;

	BeamTrianglesPerSheet.Empty(ActiveParticles);
	BeamTrianglesPerSheet.AddZeroed(ActiveParticles);
	for (INT i = 0; i < ActiveParticles; i++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[i]);

		INT						CurrentOffset		= TypeDataOffset;
		FBeam2TypeDataPayload*	BeamData			= NULL;
		FVector*				InterpolatedPoints	= NULL;
		FLOAT*					NoiseRate			= NULL;
		FLOAT*					NoiseDelta			= NULL;
		FVector*				TargetNoisePoints	= NULL;
		FVector*				NextNoisePoints		= NULL;
		FLOAT*					TaperValues			= NULL;
		FLOAT*					NoiseDistanceScale	= NULL;
		FBeamParticleModifierPayloadData* SourceModifier = NULL;
		FBeamParticleModifierPayloadData* TargetModifier = NULL;

		BeamTypeData->GetDataPointers(this, (const BYTE*)Particle, CurrentOffset, 
			BeamData, InterpolatedPoints, NoiseRate, NoiseDelta, TargetNoisePoints, 
			NextNoisePoints, TaperValues, NoiseDistanceScale,
			SourceModifier, TargetModifier);

		BeamTrianglesPerSheet(i) = BeamData->TriangleCount;

		// Take sheets into account
		INT LocalTriangles = 0;
		if (BeamData->TriangleCount > 0)
		{
			// Stored triangle count is per sheet...
			LocalTriangles	+= BeamData->TriangleCount * Sheets;
			VerticesToRender += (BeamData->TriangleCount + 2) * Sheets;
			// 4 Degenerates Per Sheet (except for last one)
			LocalTriangles	+= (Sheets - 1) * 4;
			TrianglesToRender += LocalTriangles;
			// Multiple beams?
			if (i < (ActiveParticles - 1))
			{
				// 4 Degenerates Per Beam (except for last one)
				TrianglesToRender	+= 4;
			}
		}
	}

	VertexCount = VerticesToRender;
	TriangleCount = TrianglesToRender;
}

/**
 *	Retrieves the dynamic data for the emitter
 *	
 *	@param	bSelected					Whether the emitter is selected in the editor
 *
 *	@return	FDynamicEmitterDataBase*	The dynamic data, or NULL if it shouldn't be rendered
 */
FDynamicEmitterDataBase* FParticleBeam2EmitterInstance::GetDynamicData(UBOOL bSelected)
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

	// Allocate it for now, but we will want to change this to do some form
	// of caching
	if (ActiveParticles > 0)
	{
		// Get the material instance. If there is none, or the material isn't flagged for use with particle systems, use the DefaultMaterial.
		UMaterialInterface* MaterialInst = LODLevel->RequiredModule->Material;
		if (MaterialInst == NULL || !MaterialInst->UseWithBeamTrails())
		{
			MaterialInst = GEngine->DefaultMaterial;
		}

		// Allocate the data
		FDynamicBeam2EmitterData* NewEmitterData = ::new FDynamicBeam2EmitterData(ActiveParticles, ParticleStride, bRequiresSorting,MaterialInst->GetRenderProxy(bSelected));
		check(NewEmitterData);
		check(NewEmitterData->ParticleData);

		// Fill it in...
		INT						ParticleCount	= ActiveParticles;
		UBOOL					bSorted			= FALSE;
		const INT				ScreenAlignment	= SpriteTemplate->ScreenAlignment;

		NewEmitterData->bUseLocalSpace = FALSE;
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

		DetermineVertexAndTriangleCount();

		NewEmitterData->TrianglesPerSheet.Empty(BeamTrianglesPerSheet.Num());
		NewEmitterData->TrianglesPerSheet.AddZeroed(BeamTrianglesPerSheet.Num());
		for (INT BeamIndex = 0; BeamIndex < BeamTrianglesPerSheet.Num(); BeamIndex++)
		{
			NewEmitterData->TrianglesPerSheet(BeamIndex) = BeamTrianglesPerSheet(BeamIndex);
		}
		NewEmitterData->ActiveParticleCount = ActiveParticles;
		NewEmitterData->VertexCount = VertexCount;
		NewEmitterData->PrimitiveCount = TriangleCount;

		NewEmitterData->ScreenAlignment = SpriteTemplate->ScreenAlignment;
		NewEmitterData->EmitterRenderMode = LODLevel->RequiredModule->EmitterRenderMode;

		NewEmitterData->bLockAxis = FALSE;
		NewEmitterData->bSelected = bSelected;

		BeamTypeData->GetDataPointerOffsets(this, NULL, TypeDataOffset, 
			NewEmitterData->BeamDataOffset, NewEmitterData->InterpolatedPointsOffset,
			NewEmitterData->NoiseRateOffset, NewEmitterData->NoiseDeltaTimeOffset,
			NewEmitterData->TargetNoisePointsOffset, NewEmitterData->NextNoisePointsOffset, 
			NewEmitterData->TaperCount, NewEmitterData->TaperValuesOffset,
			NewEmitterData->NoiseDistanceScaleOffset);

		if (BeamModule_Source)
		{
			NewEmitterData->bUseSource = TRUE;
		}
		else
		{
			NewEmitterData->bUseSource = FALSE;
		}

		if (BeamModule_Target)
		{
			NewEmitterData->bUseTarget = TRUE;
		}
		else
		{
			NewEmitterData->bUseTarget = FALSE;
		}

		if (BeamModule_Noise)
		{
			NewEmitterData->bLowFreqNoise_Enabled = BeamModule_Noise->bLowFreq_Enabled;
			NewEmitterData->bHighFreqNoise_Enabled = FALSE;
			NewEmitterData->bSmoothNoise_Enabled = BeamModule_Noise->bSmooth;

		}
		else
		{
			NewEmitterData->bLowFreqNoise_Enabled = FALSE;
			NewEmitterData->bHighFreqNoise_Enabled = FALSE;
			NewEmitterData->bSmoothNoise_Enabled = FALSE;
		}
		NewEmitterData->TessFactor = (BeamTypeData->TessellationFactor > 0) ? BeamTypeData->TessellationFactor : 1;
		NewEmitterData->Sheets = (BeamTypeData->Sheets > 0) ? BeamTypeData->Sheets : 1;
		NewEmitterData->TextureTile = BeamTypeData->TextureTile;
		NewEmitterData->TextureTileDistance = BeamTypeData->TextureTileDistance;
		NewEmitterData->TaperMethod = BeamTypeData->TaperMethod;
		NewEmitterData->InterpolationPoints = BeamTypeData->InterpolationPoints;

		NewEmitterData->NoiseTessellation	= 0;
		NewEmitterData->Frequency			= 1;
		NewEmitterData->NoiseRangeScale		= 1.0f;
		NewEmitterData->NoiseTangentStrength= 1.0f;


		if ((BeamModule_Noise == NULL) || (BeamModule_Noise->bLowFreq_Enabled == FALSE))
		{
			NewEmitterData->TessFactor	= BeamTypeData->InterpolationPoints ? BeamTypeData->InterpolationPoints : 1;
		}
		else
		{
			if (BeamTypeData->InterpolationPoints > 0)
			{
				//@todo. HANDLE THIS CASE
//				debugf(TEXT("Interpolated noisy beams not supported yet - forcing to 0 InterpolationPoints!"));
//				BeamTypeData->InterpolationPoints = 0;
			}
			NewEmitterData->Frequency			= (BeamModule_Noise->Frequency > 0) ? BeamModule_Noise->Frequency : 1;
			NewEmitterData->NoiseTessellation	= (BeamModule_Noise->NoiseTessellation > 0) ? BeamModule_Noise->NoiseTessellation : 1;
			NewEmitterData->NoiseTangentStrength= BeamModule_Noise->NoiseTangentStrength.GetValue(EmitterTime);
			if (BeamModule_Noise->bNRScaleEmitterTime)
			{
				NewEmitterData->NoiseRangeScale = BeamModule_Noise->NoiseRangeScale.GetValue(EmitterTime, Component);
			}
			else
			{
				//@todo.SAS. Need to address this!!!!
				//					check(0 && TEXT("NoiseRangeScale - No way to get per-particle setting at this time."));
				//					NewEmitterData->NoiseRangeScale	= BeamModule_Noise->NoiseRangeScale.GetValue(Particle->RelativeTime, Component);
				NewEmitterData->NoiseRangeScale = BeamModule_Noise->NoiseRangeScale.GetValue(EmitterTime, Component);
			}
			NewEmitterData->NoiseSpeed = BeamModule_Noise->NoiseSpeed.GetValue(EmitterTime);
			NewEmitterData->NoiseLockTime = BeamModule_Noise->NoiseLockTime;
			NewEmitterData->NoiseLockRadius = BeamModule_Noise->NoiseLockRadius;
			NewEmitterData->bTargetNoise = BeamModule_Noise->bTargetNoise;
			NewEmitterData->NoiseTension = BeamModule_Noise->NoiseTension;
		}

		INT MaxSegments	= ((NewEmitterData->TessFactor * NewEmitterData->Frequency) + 1 + 1);		// Tessellation * Frequency + FinalSegment + FirstEdge;

		// Determine the index count
		NewEmitterData->IndexCount	= 0;
		for (INT Beam = 0; Beam < ActiveParticles; Beam++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[Beam]);

			INT						CurrentOffset		= TypeDataOffset;
			FBeam2TypeDataPayload*	BeamData			= NULL;
			FVector*				InterpolatedPoints	= NULL;
			FLOAT*					NoiseRate			= NULL;
			FLOAT*					NoiseDelta			= NULL;
			FVector*				TargetNoisePoints	= NULL;
			FVector*				NextNoisePoints		= NULL;
			FLOAT*					TaperValues			= NULL;
			FLOAT*					NoiseDistanceScale	= NULL;
			FBeamParticleModifierPayloadData* SourceModifier = NULL;
			FBeamParticleModifierPayloadData* TargetModifier = NULL;

			BeamTypeData->GetDataPointers(this, (const BYTE*)Particle, CurrentOffset, BeamData, 
				InterpolatedPoints, NoiseRate, NoiseDelta, TargetNoisePoints, NextNoisePoints, 
				TaperValues, NoiseDistanceScale, SourceModifier, TargetModifier);

			if (BeamData->TriangleCount > 0)
			{
				if (NewEmitterData->IndexCount == 0)
				{
					NewEmitterData->IndexCount = 2;
				}
				NewEmitterData->IndexCount	+= BeamData->TriangleCount * NewEmitterData->Sheets;	// 1 index per triangle in the strip PER SHEET
				NewEmitterData->IndexCount	+= ((NewEmitterData->Sheets - 1) * 4);					// 4 extra indices per stitch (degenerates)
				if (Beam > 0)
				{
					NewEmitterData->IndexCount	+= 4;	// 4 extra indices per beam (degenerates)
				}
			}
		}

		if (NewEmitterData->IndexCount > 15000)
		{
			NewEmitterData->IndexStride	= sizeof(DWORD);
		}
		else
		{
			NewEmitterData->IndexStride	= sizeof(WORD);
		}

		//@todo. SORTING IS A DIFFERENT ISSUE NOW! 
		//		 GParticleView isn't going to be valid anymore?
		BYTE* PData = NewEmitterData->ParticleData;
		for (INT i = 0; i < ParticleCount; i++)
		{
			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
			appMemcpy(PData, &Particle, ParticleStride);
			PData += ParticleStride;
		}

		// Set the debug rendering flags...
		NewEmitterData->bRenderGeometry = BeamTypeData->RenderGeometry;
		NewEmitterData->bRenderDirectLine = BeamTypeData->RenderDirectLine;
		NewEmitterData->bRenderLines = BeamTypeData->RenderLines;
		NewEmitterData->bRenderTessellation = BeamTypeData->RenderTessellation;

		return NewEmitterData;
	}
	return NULL;
}
