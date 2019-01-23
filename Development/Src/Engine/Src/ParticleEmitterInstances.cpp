/*=============================================================================
	ParticleEmitterInstances.cpp: Particle emitter instance implementations.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"
#include "LevelUtils.h"

IMPLEMENT_PARTICLEEMITTERINSTANCE_TYPE(FParticleSpriteEmitterInstance);
IMPLEMENT_PARTICLEEMITTERINSTANCE_TYPE(FParticleSpriteSubUVEmitterInstance);
IMPLEMENT_PARTICLEEMITTERINSTANCE_TYPE(FParticleMeshEmitterInstance);

// Scale the particle bounds by appSqrt(2.0f) / 2.0f
#define PARTICLESYSTEM_BOUNDSSCALAR		0.707107f

/*-----------------------------------------------------------------------------
	FParticleEmitterInstance
-----------------------------------------------------------------------------*/
/**
 *	ParticleEmitterInstance
 *	The base structure for all emitter instance classes
 */
FParticleEmitterInstanceType FParticleEmitterInstance::StaticType(TEXT("FParticleEmitterInstance"),NULL);

// Only update the PeakActiveParticles if the frame rate is 20 or better
const FLOAT FParticleEmitterInstance::PeakActiveParticleUpdateDelta = 0.05f;

/** Constructor	*/
FParticleEmitterInstance::FParticleEmitterInstance() :
	  SpriteTemplate(NULL)
    , Component(NULL)
    , CurrentLODLevelIndex(0)
    , CurrentLODLevel(NULL)
    , TypeDataOffset(0)
    , SubUVDataOffset(0)
	, OrbitModuleOffset(0)
    , KillOnDeactivate(0)
    , bKillOnCompleted(0)
	, bRequiresSorting(FALSE)
    , ParticleData(NULL)
    , ParticleIndices(NULL)
    , InstanceData(NULL)
    , InstancePayloadSize(0)
    , PayloadOffset(0)
    , ParticleSize(0)
    , ParticleStride(0)
    , ActiveParticles(0)
    , MaxActiveParticles(0)
    , SpawnFraction(0.0f)
    , SecondsSinceCreation(0.0f)
    , EmitterTime(0.0f)
    , LoopCount(0)
	, IsRenderDataDirty(0)
    , Module_AxisLock(NULL)
    , EmitterDuration(0.0f)
	, TrianglesToRender(0)
	, MaxVertexIndex(0)
	, CurrentMaterial(NULL)
{
}

/** Destructor	*/
FParticleEmitterInstance::~FParticleEmitterInstance()
{
  	appFree(ParticleData);
    appFree(ParticleIndices);
    appFree(InstanceData);
}

/**
 *	Set the KillOnDeactivate flag to the given value
 *
 *	@param	bKill	Value to set KillOnDeactivate to.
 */
void FParticleEmitterInstance::SetKillOnDeactivate(UBOOL bKill)
{
	KillOnDeactivate = bKill;
}

/**
 *	Set the KillOnCompleted flag to the given value
 *
 *	@param	bKill	Value to set KillOnCompleted to.
 */
void FParticleEmitterInstance::SetKillOnCompleted(UBOOL bKill)
{
	bKillOnCompleted = bKill;
}

/**
 *	Initialize the parameters for the structure
 *
 *	@param	InTemplate		The ParticleEmitter to base the instance on
 *	@param	InComponent		The owning ParticleComponent
 *	@param	bClearResources	If TRUE, clear all resource data
 */
void FParticleEmitterInstance::InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent, UBOOL bClearResources)
{
	SpriteTemplate = CastChecked<UParticleSpriteEmitter>(InTemplate);
    Component = InComponent;
	SetupEmitterDuration();
}

/**
 *	Initialize the instance
 */
void FParticleEmitterInstance::Init()
{
	// This assert makes sure that packing is as expected.
	// Added FBaseColor...
	// Linear color change
	// Added Flags field
	check(sizeof(FBaseParticle) == 128);

	// Calculate particle struct size, size and average lifetime.
	ParticleSize = sizeof(FBaseParticle);
	INT	ReqBytes;
	INT ReqInstanceBytes = 0;
	INT TempInstanceBytes;

	UParticleLODLevel* HighLODLevel = SpriteTemplate->GetLODLevel(0);
	check(HighLODLevel);
	UParticleModule* TypeDataModule = HighLODLevel->TypeDataModule;
	if (TypeDataModule)
	{
		ReqBytes = TypeDataModule->RequiredBytes(this);
		if (ReqBytes)
		{
			TypeDataOffset	 = ParticleSize;
			ParticleSize	+= ReqBytes;
		}

		TempInstanceBytes = TypeDataModule->RequiredBytesPerInstance(this);
		if (TempInstanceBytes)
		{
			ReqInstanceBytes += TempInstanceBytes;
		}
	}

	// Set the current material
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	check(LODLevel);
	check(LODLevel->RequiredModule);
	CurrentMaterial = LODLevel->RequiredModule->Material;

	// NOTE: This code assumes that the same module order occurs in all LOD levels
	for (INT i = 0; i < LODLevel->Modules.Num(); i++)
	{
		UParticleModule* ParticleModule = LODLevel->Modules(i);
		check(ParticleModule);
		if (ParticleModule->IsA(UParticleModuleTypeDataBase::StaticClass()) == FALSE)
		{
			ReqBytes	= ParticleModule->RequiredBytes(this);
			if (ReqBytes)
			{
				ModuleOffsetMap.Set(ParticleModule, ParticleSize);
				ParticleSize	+= ReqBytes;
			}

			TempInstanceBytes = ParticleModule->RequiredBytesPerInstance(this);
			if (TempInstanceBytes)
			{
			    ModuleInstanceOffsetMap.Set(ParticleModule, ReqInstanceBytes);
				ReqInstanceBytes += TempInstanceBytes;
			}
		}

		if (ParticleModule->IsA(UParticleModuleOrientationAxisLock::StaticClass()))
		{
			Module_AxisLock	= Cast<UParticleModuleOrientationAxisLock>(ParticleModule);
		}
	}

	if ((InstanceData == NULL) || (ReqInstanceBytes > InstancePayloadSize))
	{
		InstanceData = (BYTE*)(appRealloc(InstanceData, ReqInstanceBytes));
		InstancePayloadSize = ReqInstanceBytes;
	}

	appMemzero(InstanceData, InstancePayloadSize);

	// Offset into emitter specific payload (e.g. TrailComponent requires extra bytes).
	PayloadOffset = ParticleSize;
	
	// Update size with emitter specific size requirements.
	ParticleSize += RequiredBytes();

	// Make sure everything is at least 16 byte aligned so we can use SSE for FVector.
	ParticleSize = Align(ParticleSize, 16);

	// E.g. trail emitters store trailing particles directly after leading one.
	ParticleStride			= CalculateParticleStride(ParticleSize);

	// Set initial values.
	SpawnFraction			= 0;
	SecondsSinceCreation	= 0;
	
	Location				= Component->LocalToWorld.GetOrigin();
	OldLocation				= Location;
	
	TrianglesToRender		= 0;
	MaxVertexIndex			= 0;

	if (ParticleData == NULL)
	{
		MaxActiveParticles	= 0;
		ActiveParticles		= 0;
	}

	ParticleBoundingBox.Init();
	check(LODLevel->RequiredModule);
	if (LODLevel->RequiredModule->RandomImageChanges == 0)
	{
		LODLevel->RequiredModule->RandomImageTime	= 1.0f;
	}
	else
	{
		LODLevel->RequiredModule->RandomImageTime	= 0.99f / (LODLevel->RequiredModule->RandomImageChanges + 1);
	}

	// Resize to sensible default.
	if (GIsGame == TRUE)
	{
		if ((LODLevel->PeakActiveParticles > 0) || (SpriteTemplate->InitialAllocationCount > 0))
		{
			// In-game... we assume the editor has set this properly, but still clamp at 100 to avoid wasting
			// memory.
			if (SpriteTemplate->InitialAllocationCount > 0)
			{
				Resize(Min( SpriteTemplate->InitialAllocationCount, 100 ));
			}
			else
			{
				Resize(Min( LODLevel->PeakActiveParticles, 100 ));
			}
		}
		else
		{
			// This is to force the editor to 'select' a value
			Resize(10);
		}
	}

	LoopCount = 0;

	// Propagate killon flags
	SetKillOnDeactivate(LODLevel->RequiredModule->bKillOnDeactivate);
	SetKillOnCompleted(LODLevel->RequiredModule->bKillOnCompleted);

	// Propagate sorting flag.
	bRequiresSorting = LODLevel->RequiredModule->bRequiresSorting;
	
	// Reset the burst lists
	if (BurstFired.Num() < SpriteTemplate->LODLevels.Num())
	{
		BurstFired.AddZeroed(SpriteTemplate->LODLevels.Num() - BurstFired.Num());
	}
	for (INT LODIndex = 0; LODIndex < SpriteTemplate->LODLevels.Num(); LODIndex++)
	{
		LODLevel = SpriteTemplate->LODLevels(LODIndex);
		check(LODLevel);
		if (BurstFired(LODIndex).BurstFired.Num() < LODLevel->RequiredModule->BurstList.Num())
		{
			BurstFired(LODIndex).BurstFired.AddZeroed(LODLevel->RequiredModule->BurstList.Num() - BurstFired(LODIndex).BurstFired.Num());
		}
	}
	ResetBurstList();

	// Tag it as dirty w.r.t. the renderer
	IsRenderDataDirty	= 1;
}

/**
 *	Resize the particle data array
 *
 *	@param	NewMaxActiveParticles	The new size to use
 */
void FParticleEmitterInstance::Resize(INT NewMaxActiveParticles, UBOOL bSetMaxActiveCount)
{
	if (NewMaxActiveParticles > MaxActiveParticles)
	{
		// Alloc (or realloc) the data array
		// Allocations > 16 byte are always 16 byte aligned so ParticleData can be used with SSE.
		// NOTE: We don't have to zero the memory here... It gets zeroed when grabbed later.
		ParticleData = (BYTE*) appRealloc(ParticleData, ParticleStride * NewMaxActiveParticles);
		check(ParticleData);

		// Allocate memory for indices.
		if (ParticleIndices == NULL)
		{
			// Make sure that we clear all when it is the first alloc
			MaxActiveParticles = 0;
		}
		ParticleIndices	= (WORD*) appRealloc(ParticleIndices, sizeof(WORD) * NewMaxActiveParticles);

		// Fill in default 1:1 mapping.
		for (INT i=MaxActiveParticles; i<NewMaxActiveParticles; i++)
		{
			ParticleIndices[i] = i;
		}

		// Set the max count
		MaxActiveParticles = NewMaxActiveParticles;
	}

	// Set the PeakActiveParticles
	if (bSetMaxActiveCount)
	{
		UParticleLODLevel* LODLevel	= SpriteTemplate->GetLODLevel(0);
		check(LODLevel);
		if (MaxActiveParticles > LODLevel->PeakActiveParticles)
		{
			LODLevel->PeakActiveParticles = MaxActiveParticles;
		}
	}
}

/**
 *	Tick the instance.
 *
 *	@param	DeltaTime			The time slice to use
 *	@param	bSuppressSpawning	If TRUE, do not spawn during Tick
 */
void FParticleEmitterInstance::Tick(FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	SCOPE_CYCLE_COUNTER(STAT_SpriteTickTime);

	// Grab the current LOD level
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
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

	// If this the FirstTime we are being ticked?
	UBOOL bFirstTime = (SecondsSinceCreation > 0.0f) ? FALSE : TRUE;
	SecondsSinceCreation += DeltaTime;

	// Update time within emitter loop.
	EmitterTime = SecondsSinceCreation;
	if (EmitterDuration > KINDA_SMALL_NUMBER)
	{
		EmitterTime = appFmod(SecondsSinceCreation, EmitterDuration);
	}

	// Get the emitter delay time
	FLOAT EmitterDelay = LODLevel->RequiredModule->EmitterDelay;

	// Determine if the emitter has looped
	if ((SecondsSinceCreation - (EmitterDuration * LoopCount)) >= EmitterDuration)
	{
		LoopCount++;
		ResetBurstList();

		if (LODLevel->RequiredModule->bDurationRecalcEachLoop == TRUE)
		{
			SetupEmitterDuration();
		}
	}

	// Don't delay unless required
	if ((LODLevel->RequiredModule->bDelayFirstLoopOnly == TRUE) && (LoopCount > 0))
	{
		EmitterDelay = 0;
	}

	// 'Reset' the emitter time so that the modules function correctly
	EmitterTime -= EmitterDelay;

	// Kill off any dead particles
	KillParticles();

	// If not suppressing spawning...
	if (!bSuppressSpawning && (EmitterTime >= 0.0f))
	{
		SCOPE_CYCLE_COUNTER(STAT_SpriteSpawnTime);
		// If emitter is not done - spawn at current rate.
		// If EmitterLoops is 0, then we loop forever, so always spawn.
		if ((LODLevel->RequiredModule->EmitterLoops == 0) || 
			(LoopCount < LODLevel->RequiredModule->EmitterLoops) ||
			(SecondsSinceCreation < (EmitterDuration * LODLevel->RequiredModule->EmitterLoops)) ||
			bFirstTime)
		{
            bFirstTime = FALSE;
			SpawnFraction = Spawn(DeltaTime);
		}
	}

	// Reset particle parameters.
	ResetParticleParameters(DeltaTime, STAT_SpriteParticlesUpdated);

	// Update the particles
	SCOPE_CYCLE_COUNTER(STAT_SpriteUpdateTime);

	CurrentMaterial = LODLevel->RequiredModule->Material;

	UParticleModuleTypeDataBase* pkBase = 0;
	if (LODLevel->TypeDataModule)
	{
		pkBase = Cast<UParticleModuleTypeDataBase>(LODLevel->TypeDataModule);
		//@todo. Need to track TypeData offset into payload!
		pkBase->PreUpdate(this, TypeDataOffset, DeltaTime);
	}

	// Update existing particles (might respawn dying ones).
	UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
	for (INT ModuleIndex = 0; ModuleIndex < LODLevel->UpdateModules.Num(); ModuleIndex++)
	{
		UParticleModule* HighModule	= LODLevel->UpdateModules(ModuleIndex);
		if (HighModule && HighModule->bEnabled)
		{
			UParticleModule* OffsetModule = HighestLODLevel->UpdateModules(ModuleIndex);
			UINT* Offset = ModuleOffsetMap.Find(OffsetModule);
			HighModule->Update(this, Offset ? *Offset : 0, DeltaTime);
		}
	}

	// Handle the TypeData module
	if (pkBase)
	{
		//@todo. Need to track TypeData offset into payload!
		pkBase->Update(this, TypeDataOffset, DeltaTime);
		pkBase->PostUpdate(this, TypeDataOffset, DeltaTime);
	}

	// Update the orbit data...
	UpdateOrbitData(DeltaTime);

	// Calculate bounding box and simulate velocity.
	UpdateBoundingBox(DeltaTime);

	// Invalidate the contents of the vertex/index buffer.
	IsRenderDataDirty = 1;

	// 'Reset' the emitter time so that the delay functions correctly
	EmitterTime += EmitterDelay;
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
void FParticleEmitterInstance::TickEditor(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, FLOAT Multiplier, FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	// Verify all the data...
	check(HighLODLevel);
	check(LowLODLevel);
	check(HighLODLevel->UpdateModules.Num() == LowLODLevel->UpdateModules.Num());
	check(HighLODLevel->SpawnModules.Num() == LowLODLevel->SpawnModules.Num());
	
	// We don't allow different TypeDataModules
	if (HighLODLevel->TypeDataModule)
	{
		check(LowLODLevel->TypeDataModule);
		check(HighLODLevel->TypeDataModule->GetClass() == LowLODLevel->TypeDataModule->GetClass());
	}

	// Stats
	SCOPE_CYCLE_COUNTER(STAT_SpriteTickTime);

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

	// FirstTime this instance has been ticked?
	UBOOL bFirstTime = (SecondsSinceCreation > 0.0f) ? FALSE : TRUE;
	SecondsSinceCreation += DeltaTime;

	// Update time within emitter loop.
	EmitterTime = SecondsSinceCreation;
	FLOAT	Duration	= EmitterDurations(HighLODLevel->Level);
	if (Duration > KINDA_SMALL_NUMBER)
	{
		EmitterTime = appFmod(SecondsSinceCreation, Duration);
	}

	// Take delay into account
	FLOAT EmitterDelay = HighLODLevel->RequiredModule->EmitterDelay;

	// Handle looping
	if ((SecondsSinceCreation - (Duration * LoopCount)) >= Duration)
	{
		LoopCount++;
		ResetBurstList();

		if (HighLODLevel->RequiredModule->bDurationRecalcEachLoop == TRUE)
		{
			SetupEmitterDuration();
		}
	}

	// Don't delay if it is not requested
	if ((LoopCount > 0) && (HighLODLevel->RequiredModule->bDelayFirstLoopOnly == TRUE))
	{
		EmitterDelay = 0;
	}

	// 'Reset' the emitter time so that the modules function correctly
	EmitterTime -= EmitterDelay;

	// Kill any dead particles off
	KillParticles();

	// If not suppressing spawning...
	if (!bSuppressSpawning && (EmitterTime >= 0.0f))
	{
		SCOPE_CYCLE_COUNTER(STAT_SpriteSpawnTime);
		// If emitter is not done - spawn at current rate.
		// If EmitterLoops is 0, then we loop forever, so always spawn.
		if ((HighLODLevel->RequiredModule->EmitterLoops == 0) || 
			(LoopCount < HighLODLevel->RequiredModule->EmitterLoops) ||
			(SecondsSinceCreation < (Duration * HighLODLevel->RequiredModule->EmitterLoops)) ||
			bFirstTime)
		{
            bFirstTime = FALSE;
			SpawnFraction = SpawnEditor(HighLODLevel, LowLODLevel, Multiplier, DeltaTime);
		}
	}

	// Update particle information.
	ResetParticleParameters(DeltaTime, STAT_SpriteParticlesUpdated);

	SCOPE_CYCLE_COUNTER(STAT_SpriteUpdateTime);

	// Grab the type data module if present
	UParticleModuleTypeDataBase* pkBase = 0;
	if (HighLODLevel->TypeDataModule)
	{
		pkBase = Cast<UParticleModuleTypeDataBase>(HighLODLevel->TypeDataModule);
		
		//@todo. Need to track TypeData offset into payload!
		pkBase->PreUpdate(this, TypeDataOffset, DeltaTime);
	}

	// Update existing particles (might respawn dying ones).
	for (INT ModuleIndex = 0; ModuleIndex < HighLODLevel->UpdateModules.Num(); ModuleIndex++)
	{
		if (!HighLODLevel->UpdateModules(ModuleIndex))
		{
			continue;
		}

		UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
		UParticleModule* OffsetModule	= 	HighestLODLevel->UpdateModules(ModuleIndex);
		UINT* Offset = ModuleOffsetMap.Find(OffsetModule);

		UParticleModule* HighModule	= HighLODLevel->UpdateModules(ModuleIndex);
		UParticleModule* LowModule	= LowLODLevel->UpdateModules(ModuleIndex);

		if (HighModule->bEnabled)
		{
			HighModule->UpdateEditor(this, Offset ? *Offset : 0, DeltaTime, LowModule, Multiplier);
		}
		else
		if (LowModule->bEnabled)
		{
			LowModule->UpdateEditor(this, Offset ? *Offset : 0, DeltaTime, HighModule, (1.0f - Multiplier));
		}
	}

	// Update the TypeData module
	if (pkBase)
	{
		//@todo. Need to track TypeData offset into payload!
		pkBase->Update(this, TypeDataOffset, DeltaTime);
		pkBase->PostUpdate(this, TypeDataOffset, DeltaTime);
	}

	// Update the orbit data...
	UpdateOrbitData(DeltaTime);

	// Calculate bounding box and simulate velocity.
	UpdateBoundingBox(DeltaTime);

	// Invalidate the contents of the vertex/index buffer.
	IsRenderDataDirty = 1;

	// 'Reset' the emitter time so that the delay functions correctly
	EmitterTime += EmitterDelay;
}

/**
 *	Rewind the instance.
 */
void FParticleEmitterInstance::Rewind()
{
	SecondsSinceCreation = 0;
	EmitterTime = 0;
	LoopCount = 0;
	ResetBurstList();
}

/**
 *	Retrieve the bounding box for the instance
 *
 *	@return	FBox	The bounding box
 */
FBox FParticleEmitterInstance::GetBoundingBox()
{ 
	return ParticleBoundingBox;
}

/**
 *	Update the bounding box for the emitter
 *
 *	@param	DeltaTime		The time slice to use
 */
void FParticleEmitterInstance::UpdateBoundingBox(FLOAT DeltaTime)
{
	if (Component)
	{
		// Take component scale into account
		FVector Scale = FVector(1.0f, 1.0f, 1.0f);
		Scale *= Component->Scale * Component->Scale3D;
		AActor* Actor = Component->GetOwner();
		if (Actor && !Component->AbsoluteScale)
		{
			Scale *= Actor->DrawScale * Actor->DrawScale3D;
		}

		UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
		check(LODLevel);

		FLOAT	MaxSizeScale	= 1.0f;
		FVector	NewLocation;
		FLOAT	NewRotation;
		ParticleBoundingBox.Init();
		UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
		check(HighestLODLevel);
		// For each particle, offset the box appropriately 
		for (INT i=0; i<ActiveParticles; i++)
		{
			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
			
			// Do linear integrator and update bounding box
			// Do angular integrator, and wrap result to within +/- 2 PI
			Particle.OldLocation	= Particle.Location;
			if ((Particle.Flags & STATE_Particle_Freeze) == 0)
			{
				if ((Particle.Flags & STATE_Particle_FreezeTranslation) == 0)
				{
					NewLocation	= Particle.Location + (DeltaTime * Particle.Velocity);
				}
				else
				{
					NewLocation	= Particle.Location;
				}
				if ((Particle.Flags & STATE_Particle_FreezeRotation) == 0)
				{
					NewRotation = (DeltaTime * Particle.RotationRate) + Particle.Rotation;
				}
				else
				{
					NewRotation	= Particle.Rotation;
				}
			}
			else
			{
				NewLocation	= Particle.Location;
				NewRotation	= Particle.Rotation;
			}

			FVector Size = Particle.Size * Scale;
			MaxSizeScale			= Max(MaxSizeScale, Size.GetAbsMax()); //@todo particles: this does a whole lot of compares that can be avoided using SSE/ Altivec.

			Particle.Rotation	 = appFmod(NewRotation, 2.f*(FLOAT)PI);
			Particle.Location	 = NewLocation;

			if (Component->bWarmingUp == FALSE)
			{	
				if (LODLevel->OrbitModules.Num() > 0)
				{
					UParticleModuleOrbit* OrbitModule = HighestLODLevel->OrbitModules(LODLevel->OrbitModules.Num() - 1);
					if (OrbitModule)
					{
						UINT* OrbitOffsetIndex = ModuleOffsetMap.Find(OrbitModule);
						if (OrbitOffsetIndex)
						{
							INT CurrentOffset = *(OrbitOffsetIndex);
							const BYTE* ParticleBase = (const BYTE*)&Particle;
							PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);
							const FLOAT Max = OrbitPayload.Offset.GetAbsMax();
							FVector OrbitOffset(Max, Max, Max);
							ParticleBoundingBox += (NewLocation + OrbitOffset);
							ParticleBoundingBox += (NewLocation - OrbitOffset);
						}
					}
				}
				else
				{
					ParticleBoundingBox += NewLocation;
				}
			}
		}

		if (Component->bWarmingUp == FALSE)
		{
			ParticleBoundingBox = ParticleBoundingBox.ExpandBy(MaxSizeScale * PARTICLESYSTEM_BOUNDSSCALAR);
			// Transform bounding box into world space if the emitter uses a local space coordinate system.
			if (LODLevel->RequiredModule->bUseLocalSpace) 
			{
				ParticleBoundingBox = ParticleBoundingBox.TransformBy(Component->LocalToWorld);
			}
		}
	}
}

/**
 *	Retrieved the per-particle bytes that this emitter type requires.
 *
 *	@return	UINT	The number of required bytes for particles in the instance
 */
UINT FParticleEmitterInstance::RequiredBytes()
{
	UINT uiBytes = 0;

	// This code assumes that the module stacks are identical across LOD levevls...
	UParticleLODLevel* LODLevel = SpriteTemplate->GetLODLevel(0);
	check(LODLevel);

	// Check for SubUV utilization, and update the required bytes accordingly
	EParticleSubUVInterpMethod	InterpolationMethod	= (EParticleSubUVInterpMethod)LODLevel->RequiredModule->InterpolationMethod;
	if (InterpolationMethod != PSUVIM_None)
	{
		SubUVDataOffset = PayloadOffset;
		if (LODLevel->TypeDataModule &&
			LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataMesh::StaticClass()))
		{
			if ((InterpolationMethod == PSUVIM_Random) || (InterpolationMethod == PSUVIM_Random_Blend))
			{
				uiBytes	= sizeof(FSubUVMeshRandomPayload);
			}
			else
			{
				uiBytes	= sizeof(FSubUVMeshPayload);
			}
		}
		else
		{
			if ((InterpolationMethod == PSUVIM_Random) || (InterpolationMethod == PSUVIM_Random_Blend))
			{
				uiBytes	= sizeof(FSubUVSpriteRandomPayload);
			}
			else
			{
				uiBytes	= sizeof(FSubUVSpritePayload);
			}
		}
	}

	return uiBytes;
}

/**
 *	Get the pointer to the instance data allocated for a given module.
 *
 *	@param	Module		The module to retrieve the data block for.
 *	@return	BYTE*		The pointer to the data
 */
BYTE* FParticleEmitterInstance::GetModuleInstanceData(UParticleModule* Module)
{
	// If there is instance data present, look up the modules offset
	if (InstanceData)
	{
		UINT* Offset = ModuleInstanceOffsetMap.Find(Module);
		if (Offset)
		{
			if (*Offset < (UINT)InstancePayloadSize)
			{
				return &(InstanceData[*Offset]);
			}
		}
	}
	return NULL;
}

/**
 *	Calculate the stride of a single particle for this instance
 *
 *	@param	ParticleSize	The size of the particle
 *
 *	@return	UINT			The stride of the particle
 */
UINT FParticleEmitterInstance::CalculateParticleStride(UINT ParticleSize)
{
	return ParticleSize;
}

/**
 *	Reset the burst list information for the instance
 */
void FParticleEmitterInstance::ResetBurstList()
{
	for (INT BurstIndex = 0; BurstIndex < BurstFired.Num(); BurstIndex++)
	{
		FLODBurstFired* CurrBurstFired = &(BurstFired(BurstIndex));
		for (INT FiredIndex = 0; FiredIndex < CurrBurstFired->BurstFired.Num(); FiredIndex++)
		{
			CurrBurstFired->BurstFired(FiredIndex) = FALSE;
		}
	}
}

/**
 *	Get the current burst rate offset (delta time is artifically increased to generate bursts)
 *
 *	@param	DeltaTime	The time slice (In/Out)
 *	@param	Burst		The number of particles to burst (Output)
 *
 *	@return	FLOAT		The time slice increase to use
 */
FLOAT FParticleEmitterInstance::GetCurrentBurstRateOffset(FLOAT& DeltaTime, INT& Burst)
{
	FLOAT SpawnRateInc = 0.0f;

	// Grab the current LOD level
	UParticleLODLevel* LODLevel	= SpriteTemplate->GetCurrentLODLevel(this);
	check(LODLevel);
	if (LODLevel->RequiredModule->BurstList.Num() > 0)
    {
		// For each burst in the list
        for (INT i = 0; i < LODLevel->RequiredModule->BurstList.Num(); i++)
        {
            FParticleBurst* BurstEntry = &(LODLevel->RequiredModule->BurstList(i));
			// If it hasn't been fired
			if (LODLevel->Level < BurstFired.Num())
			{
				if (BurstFired(LODLevel->Level).BurstFired(i) == FALSE)
				{
					// If it is time to fire it
					if (EmitterTime >= BurstEntry->Time)
					{
						// Make sure there is a valid time slice
						if (DeltaTime < 0.00001f)
						{
							DeltaTime = 0.00001f;
						}
						// Calculate the increase time slice
						INT Count = BurstEntry->Count;
						if (BurstEntry->CountLow)
						{
							Count = appTrunc((FLOAT)(BurstEntry->CountLow) + (appSRand() * (FLOAT)(BurstEntry->Count - BurstEntry->CountLow)));
						}
						SpawnRateInc += Count / DeltaTime;
						Burst += Count;
						BurstFired(LODLevel->Level).BurstFired(i)	= TRUE;
					}
				}
			}
        }
   }

	return SpawnRateInc;
}

/**
 *	Get the current burst rate offset (delta time is artifically increased to generate bursts)
 *	Editor version (for interpolation)
 *
 *	@param	HighLODLevel	The higher LOD level
 *	@param	LowLODLevel		The lower LOD level
 *	@param	Multiplier		The interpolation value to use
 *	@param	DeltaTime		The time slice (In/Out)
 *	@param	Burst			The number of particles to burst (Output)
 *
 *	@return	FLOAT			The time slice increase to use
 */
FLOAT FParticleEmitterInstance::GetCurrentBurstRateOffsetEditor(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, 
	FLOAT Multiplier, FLOAT& DeltaTime, INT& Burst)
{
	FLOAT SpawnRateInc = 0.0f;

	FLOAT HighSpawnRateInc	= 0.0f;
	FLOAT LowSpawnRateInc	= 0.0f;
	INT	HighBurstCount		= 0;
	INT	LowBurstCount		= 0;

	// Determine the High LOD burst information
	if (HighLODLevel->RequiredModule->BurstList.Num() > 0)
    {
        for (INT i = 0; i < HighLODLevel->RequiredModule->BurstList.Num(); i++)
        {
            FParticleBurst* Burst = &(HighLODLevel->RequiredModule->BurstList(i));
			if (HighLODLevel->Level < BurstFired.Num())
			{
				if (BurstFired(HighLODLevel->Level).BurstFired(i) == FALSE)
				{
					if (EmitterTime >= Burst->Time)
					{
						if (DeltaTime < 0.00001f)
						{
							DeltaTime = 0.00001f;
						}
						INT Count = Burst->Count;
						if (Burst->CountLow)
						{
							Count = appTrunc((FLOAT)(Burst->CountLow) + (appSRand() * (FLOAT)(Burst->Count - Burst->CountLow)));
						}
						HighSpawnRateInc += Count / DeltaTime;
						HighBurstCount += Count;
						BurstFired(HighLODLevel->Level).BurstFired(i) = TRUE;
					}
				}
			}
        }
   }

	// Determine the Low LOD burst information
	if (LowLODLevel->RequiredModule->BurstList.Num() > 0)
    {
        for (INT i = 0; i < LowLODLevel->RequiredModule->BurstList.Num(); i++)
        {
            FParticleBurst* Burst = &(LowLODLevel->RequiredModule->BurstList(i));
			if (LowLODLevel->Level < BurstFired.Num())
			{
				if (BurstFired(LowLODLevel->Level).BurstFired(i) == FALSE)
				{
					if (EmitterTime >= Burst->Time)
					{
						if (DeltaTime < 0.00001f)
						{
							DeltaTime = 0.00001f;
						}
						INT Count = Burst->Count;
						if (Burst->CountLow)
						{
							Count = appTrunc((FLOAT)(Burst->CountLow) + (appSRand() * (FLOAT)(Burst->Count - Burst->CountLow)));
						}
						LowSpawnRateInc += Count / DeltaTime;
						LowBurstCount += Count;
						BurstFired(LowLODLevel->Level).BurstFired(i) = TRUE;
					}
				}
			}
        }
   }

	// Interpolate between the two
	SpawnRateInc	+= (HighSpawnRateInc * Multiplier) + (LowSpawnRateInc	* (1.0f - Multiplier));
	Burst			+= appTrunc((HighBurstCount	 * Multiplier) + (LowBurstCount		* (1.0f - Multiplier)));

	return SpawnRateInc;
}

/**
 *	Reset the particle parameters
 */
void FParticleEmitterInstance::ResetParticleParameters(FLOAT DeltaTime, DWORD StatId)
{
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
	check(HighestLODLevel);

	INT OrbitCount = LODLevel->OrbitModules.Num();
	for (INT ParticleIndex = 0; ParticleIndex < ActiveParticles; ParticleIndex++)
	{
		DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
		Particle.Velocity		= Particle.BaseVelocity;
		Particle.Size			= Particle.BaseSize;
		Particle.RotationRate	= Particle.BaseRotationRate;
		Particle.Color			= Particle.BaseColor;
		Particle.RelativeTime	+= Particle.OneOverMaxLifetime * DeltaTime;

		if (OrbitCount > 0)
		{
			for (INT OrbitIndex = 0; OrbitIndex < OrbitCount; OrbitIndex++)
			{
				UParticleModuleOrbit* OrbitModule = HighestLODLevel->OrbitModules(OrbitIndex);
				if (OrbitModule)
				{
					UINT* OrbitOffset = ModuleOffsetMap.Find(OrbitModule);
					if (OrbitOffset)
					{
						INT CurrentOffset = *(OrbitOffset);
						const BYTE* ParticleBase = (const BYTE*)&Particle;
						PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);
						if (OrbitIndex < (OrbitCount - 1))
						{
							OrbitPayload.PreviousOffset = OrbitPayload.Offset;
						}
						OrbitPayload.Offset = OrbitPayload.BaseOffset;
						OrbitPayload.RotationRate = OrbitPayload.BaseRotationRate;
					}
				}
			}
		}

		INC_DWORD_STAT(StatId);
	}
}

/**
 *	Calculate the orbit offset data.
 */
void FParticleEmitterInstance::CalculateOrbitOffset(FOrbitChainModuleInstancePayload& Payload, 
	FVector& AccumOffset, FVector& AccumRotation, FVector& AccumRotationRate, 
	FLOAT DeltaTime, FVector& Result, FMatrix& RotationMat, TArray<FVector>& Offsets, UBOOL bStoreResult)
{
	AccumRotation += AccumRotationRate * DeltaTime;
	Payload.Rotation = AccumRotation;
	if (AccumRotation.IsNearlyZero() == FALSE)
	{
		FVector RotRot = RotationMat.TransformNormal(AccumRotation);
		FVector ScaledRotation = RotRot * 360.0f;
		FRotator Rotator = FRotator::MakeFromEuler(ScaledRotation);
		FMatrix RotMat = FRotationMatrix(Rotator);

		RotationMat *= RotMat;

		Result = RotationMat.TransformFVector(AccumOffset);
	}
	else
	{
		Result = AccumOffset;
	}

	if (bStoreResult == TRUE)
	{
		new(Offsets) FVector(Result);
	}
	AccumOffset = FVector(0.0f);
	AccumRotation = FVector(0.0f);
	AccumRotationRate = FVector(0.0f);
}

void FParticleEmitterInstance::UpdateOrbitData(FLOAT DeltaTime)
{
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	check(LODLevel);
	UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
	check(HighestLODLevel);

	INT ModuleCount = LODLevel->OrbitModules.Num();
	if (ModuleCount > 0)
	{
		for(INT i=ActiveParticles-1; i>=0; i--)
		{
			const INT	CurrentIndex	= ParticleIndices[i];
			const BYTE* ParticleBase	= ParticleData + CurrentIndex * ParticleStride;
			FBaseParticle& Particle		= *((FBaseParticle*) ParticleBase);
			if ((Particle.Flags & STATE_Particle_Freeze) == 0)
			{
				TArray<FVector> Offsets;
				FVector AccumulatedOffset = FVector(0.0f);
				FVector AccumulatedRotation = FVector(0.0f);
				FVector AccumulatedRotationRate = FVector(0.0f);

				FOrbitChainModuleInstancePayload* LocalOrbitPayload = NULL;
				FOrbitChainModuleInstancePayload* PrevOrbitPayload = NULL;
				BYTE PrevOrbitChainMode = 0;
				FMatrix AccumRotMatrix;
				AccumRotMatrix.SetIdentity();

				INT CurrentAccumCount = 0;

				for (INT OrbitIndex = 0; OrbitIndex < ModuleCount; OrbitIndex++)
				{
					UParticleModuleOrbit* OrbitModule = LODLevel->OrbitModules(OrbitIndex);
					check(OrbitModule);
					UParticleModuleOrbit* HighestOrbitModule = HighestLODLevel->OrbitModules(OrbitIndex);
					check(HighestOrbitModule);

					UINT* ModuleOffset = ModuleOffsetMap.Find(HighestOrbitModule);
					check(ModuleOffset);

					INT CurrentOffset = *ModuleOffset;
					PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);

					// The last orbit module holds the last final offset position
					UBOOL bCalculateOffset = FALSE;
					if (OrbitIndex == (ModuleCount - 1))
					{
						LocalOrbitPayload = &OrbitPayload;
						bCalculateOffset = TRUE;
					}

					// Determine the offset, rotation, rotationrate for the current particle
					if (OrbitModule->ChainMode == EOChainMode_Add)
					{
						if (OrbitModule->bEnabled == TRUE)
						{
							AccumulatedOffset += OrbitPayload.Offset;
							AccumulatedRotation += OrbitPayload.Rotation;
							AccumulatedRotationRate += OrbitPayload.RotationRate;
						}
					}
					else
					if (OrbitModule->ChainMode == EOChainMode_Scale)
					{
						if (OrbitModule->bEnabled == TRUE)
						{
							AccumulatedOffset *= OrbitPayload.Offset;
							AccumulatedRotation *= OrbitPayload.Rotation;
							AccumulatedRotationRate *= OrbitPayload.RotationRate;
						}
					}
					else
					if (OrbitModule->ChainMode == EOChainMode_Link)
					{
						if ((OrbitIndex > 0) && (PrevOrbitChainMode == EOChainMode_Link))
						{
							// Calculate the offset with the current accumulation
							FVector ResultOffset;
							CalculateOrbitOffset(*PrevOrbitPayload, 
								AccumulatedOffset, AccumulatedRotation, AccumulatedRotationRate, 
								DeltaTime, ResultOffset, AccumRotMatrix, Offsets);
							if (OrbitModule->bEnabled == FALSE)
							{
								AccumulatedOffset = FVector(0.0f);
								AccumulatedRotation = FVector(0.0f);
								AccumulatedRotationRate = FVector(0.0f);
							}
						}

						if (OrbitModule->bEnabled == TRUE)
						{
							AccumulatedOffset = OrbitPayload.Offset;
							AccumulatedRotation = OrbitPayload.Rotation;
							AccumulatedRotationRate = OrbitPayload.RotationRate;
						}
					}

					if (bCalculateOffset == TRUE)
					{
						// Push the current offset into the array
						FVector ResultOffset;
						CalculateOrbitOffset(OrbitPayload, 
							AccumulatedOffset, AccumulatedRotation, AccumulatedRotationRate, 
							DeltaTime, ResultOffset, AccumRotMatrix, Offsets);
					}

					INT Dummy = 0;
					if (OrbitModule->bEnabled)
					{
						PrevOrbitPayload = &OrbitPayload;
						PrevOrbitChainMode = OrbitModule->ChainMode;
					}
				}

				LocalOrbitPayload->Offset = FVector(0.0f);
				for (INT AccumIndex = 0; AccumIndex < Offsets.Num(); AccumIndex++)
				{
					LocalOrbitPayload->Offset += Offsets(AccumIndex);
				}
			}
		}
	}
}

/**
 *	Spawn particles for this emitter instance
 *
 *	@param	DeltaTime		The time slice to spawn over
 *
 *	@return	FLOAT			The leftover fraction of spawning
 */
FLOAT FParticleEmitterInstance::Spawn(FLOAT DeltaTime)
{
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	check(LODLevel);

	// For beams, we probably want to ignore the SpawnRate distribution,
	// and focus strictly on the BurstList...
	FLOAT SpawnRate = 0.0f;
	INT SpawnCount = 0;
	FLOAT SpawnRateDivisor = 0.0f;
	FLOAT OldLeftover = SpawnFraction;

	UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);

	UBOOL bProcessSpawnRate = TRUE;
	for (INT SpawnModIndex = 0; SpawnModIndex < LODLevel->SpawningModules.Num(); SpawnModIndex++)
	{
		UParticleModuleSpawnBase* SpawnModule = LODLevel->SpawningModules(SpawnModIndex);
		if (SpawnModule)
		{
			UParticleModule* OffsetModule = HighestLODLevel->SpawningModules(SpawnModIndex);

			INT Number = 0;
			FLOAT Rate = 0.0f;

			UINT* Offset = ModuleOffsetMap.Find(OffsetModule);
			if (SpawnModule->GetSpawnAmount(this, Offset ? *Offset : 0, OldLeftover, DeltaTime, Number, Rate) == FALSE)
			{
				bProcessSpawnRate = FALSE;
			}

			SpawnCount += Number;
			SpawnRate += Rate;
		}
	}

	// Figure out spawn rate for this tick.
	if (bProcessSpawnRate)
	{
		SpawnRate += LODLevel->RequiredModule->SpawnRate.GetValue(EmitterTime, Component);
	}

	// Take Bursts into account as well...
	INT		Burst		= 0;
	FLOAT	BurstTime	= GetCurrentBurstRateOffset(DeltaTime, Burst);
	SpawnRate += BurstTime;

	// Spawn new particles...
	if (SpawnRate > 0.f)
	{
//		SpawnFraction = Spawn(SpawnFraction, SpawnRate, DeltaTime, Burst, BurstTime);
//		FLOAT FParticleEmitterInstance::Spawn(FLOAT OldLeftover, FLOAT Rate, FLOAT DeltaTime, INT Burst, FLOAT BurstTime)
		// Ensure continous spawning... lots of fiddling.
		FLOAT	NewLeftover = OldLeftover + DeltaTime * SpawnRate;
		INT		Number		= appFloor(NewLeftover);
		FLOAT	Increment	= 1.f / SpawnRate;
		FLOAT	StartTime	= DeltaTime + OldLeftover * Increment - Increment;
		NewLeftover			= NewLeftover - Number;

		// If we have calculated less than the burst count, force the burst count
		if (Number < Burst)
		{
			Number = Burst;
		}

		// Take the burst time fakery into account
		if (BurstTime > 0.0f)
		{
			NewLeftover -= BurstTime / Burst;
			NewLeftover	= Clamp<FLOAT>(NewLeftover, 0, NewLeftover);
		}

		// Handle growing arrays.
		INT NewCount = ActiveParticles + Number;
		if (NewCount >= MaxActiveParticles)
		{
			if (DeltaTime < PeakActiveParticleUpdateDelta)
			{
				Resize(NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1);
			}
			else
			{
				Resize((NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1), FALSE);
			}
		}

		// Spawn particles.
		for (INT i=0; i<Number; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[ActiveParticles]);

			FLOAT SpawnTime = StartTime - i * Increment;

			PreSpawn(Particle);

			if (LODLevel->TypeDataModule)
			{
				UParticleModuleTypeDataBase* pkBase = Cast<UParticleModuleTypeDataBase>(LODLevel->TypeDataModule);
				pkBase->Spawn(this, TypeDataOffset, SpawnTime);
			}

			for (INT ModuleIndex = 0; ModuleIndex < LODLevel->SpawnModules.Num(); ModuleIndex++)
			{
				UParticleModule* SpawnModule	= LODLevel->SpawnModules(ModuleIndex);

				UParticleLODLevel* HighestLODLevel2 = SpriteTemplate->LODLevels(0);
				UParticleModule* OffsetModule	= 	HighestLODLevel2->SpawnModules(ModuleIndex);
				UINT* Offset = ModuleOffsetMap.Find(OffsetModule);

				if (SpawnModule->bEnabled)
				{
					SpawnModule->Spawn(this, Offset ? *Offset : 0, SpawnTime);
				}
			}
			PostSpawn(Particle, 1.f - FLOAT(i+1) / FLOAT(Number), SpawnTime);

			ActiveParticles++;

			INC_DWORD_STAT(STAT_SpriteParticlesSpawned);
		}
		return NewLeftover;
	}
	return SpawnFraction;
}

/**
 *	Spawn particles for this emitter instance (editor version)
 *
 *	@param	HighLODLevel	The high LOD level to use
 *	@param	LowLODLevel		The low LOD level to use
 *	@param	Multiplier		The interpolation multiplier
 *	@param	DeltaTime		The time slice to spawn over
 *
 *	@return	FLOAT			The leftover fraction of spawning
 */
FLOAT FParticleEmitterInstance::SpawnEditor(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, 
	FLOAT Multiplier, FLOAT DeltaTime)
{
	// For beams, we probably want to ignore the SpawnRate distribution,
	// and focus strictly on the BurstList...
	FLOAT SpawnRate = 0.0f;
	INT SpawnCount = 0;
	FLOAT SpawnRateDivisor = 0.0f;
	FLOAT OldLeftover = SpawnFraction;

	UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);

	UBOOL bProcessSpawnRate = TRUE;
	for (INT SpawnModIndex = 0; SpawnModIndex < HighLODLevel->SpawningModules.Num(); SpawnModIndex++)
	{
		INT HighNumber = 0;
		FLOAT HighRate = 0.0f;
		INT LowNumber = 0;
		FLOAT LowRate = 0.0f;

		UParticleModule* OffsetModule = HighestLODLevel->SpawningModules(SpawnModIndex);
		UINT* Offset = ModuleOffsetMap.Find(OffsetModule);

		UParticleModuleSpawnBase* HighSpawnModule = HighLODLevel->SpawningModules(SpawnModIndex);
		if (HighSpawnModule)
		{
			if (HighSpawnModule->GetSpawnAmount(this, Offset ? *Offset : 0, OldLeftover, DeltaTime, HighNumber, HighRate) == FALSE)
			{
				bProcessSpawnRate = FALSE;
			}
		}

		UParticleModuleSpawnBase* LowSpawnModule = LowLODLevel->SpawningModules(SpawnModIndex);
		if (LowSpawnModule)
		{
			if (LowSpawnModule->GetSpawnAmount(this, Offset ? *Offset : 0, OldLeftover, DeltaTime, LowNumber, LowRate) == FALSE)
			{
				bProcessSpawnRate = FALSE;
			}
		}

		SpawnCount += appTrunc((HighNumber * Multiplier) + (LowNumber * (1.0f - Multiplier)));
		SpawnRate += (HighRate * Multiplier) + (LowRate * (1.0f - Multiplier));
	}

	// Figure out spawn rate for this tick.
	if (bProcessSpawnRate)
	{
		FLOAT HighSpawnRate = HighLODLevel->RequiredModule->SpawnRate.GetValue(EmitterTime, Component);
		FLOAT LowSpawnRate = LowLODLevel->RequiredModule->SpawnRate.GetValue(EmitterTime, Component);
		SpawnRate += (HighSpawnRate * Multiplier) + (LowSpawnRate * (1.0f - Multiplier));
	}

	// Take Bursts into account as well...
	INT		Burst		= 0;
	FLOAT	BurstTime	= GetCurrentBurstRateOffsetEditor(HighLODLevel, LowLODLevel, Multiplier, DeltaTime, Burst);
	SpawnRate += BurstTime;

	// Spawn new particles...
	if (SpawnRate > 0.f)
	{
		//		SpawnFraction = Spawn(SpawnFraction, SpawnRate, DeltaTime, Burst, BurstTime);
		//		FLOAT FParticleEmitterInstance::Spawn(FLOAT OldLeftover, FLOAT Rate, FLOAT DeltaTime, INT Burst, FLOAT BurstTime)
		// Ensure continous spawning... lots of fiddling.
		FLOAT	NewLeftover = OldLeftover + DeltaTime * SpawnRate;
		INT		Number		= appFloor(NewLeftover);
		FLOAT	Increment	= 1.f / SpawnRate;
		FLOAT	StartTime	= DeltaTime + OldLeftover * Increment - Increment;
		NewLeftover			= NewLeftover - Number;

		// If we have calculated less than the burst count, force the burst count
		if (Number < Burst)
		{
			Number = Burst;
		}

		// Take the burst time fakery into account
		if (BurstTime > 0.0f)
		{
			NewLeftover -= BurstTime / Burst;
			NewLeftover	= Clamp<FLOAT>(NewLeftover, 0, NewLeftover);
		}

		// Handle growing arrays.
		INT NewCount = ActiveParticles + Number;
		if (NewCount >= MaxActiveParticles)
		{
			if (DeltaTime < PeakActiveParticleUpdateDelta)
			{
				Resize(NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1);
			}
			else
			{
				Resize((NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1), FALSE);
			}
		}

		// Spawn particles.
		for (INT i=0; i<Number; i++)
		{
			DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[ActiveParticles]);

			FLOAT SpawnTime = StartTime - i * Increment;

			PreSpawn(Particle);

			if (HighLODLevel->TypeDataModule)
			{
				UParticleModuleTypeDataBase* HighBase = Cast<UParticleModuleTypeDataBase>(HighLODLevel->TypeDataModule);
				UParticleModuleTypeDataBase* LowBase = Cast<UParticleModuleTypeDataBase>(LowLODLevel->TypeDataModule);
				HighBase->SpawnEditor(this, TypeDataOffset, SpawnTime, LowBase, Multiplier);
			}

			for (INT ModuleIndex = 0; ModuleIndex < HighLODLevel->SpawnModules.Num(); ModuleIndex++)
			{
				UParticleModule* SpawnModule	= HighLODLevel->SpawnModules(ModuleIndex);
				UParticleModule* LowSpawnModule	= LowLODLevel->SpawnModules(ModuleIndex);

				UParticleLODLevel* HighestLODLevel2 = SpriteTemplate->LODLevels(0);
				UParticleModule* OffsetModule	= 	HighestLODLevel2->SpawnModules(ModuleIndex);
				UINT* Offset = ModuleOffsetMap.Find(OffsetModule);

				if (SpawnModule->bEnabled)
				{
					SpawnModule->SpawnEditor(this, Offset ? *Offset : 0, SpawnTime, LowSpawnModule, Multiplier);
				}
			}
			PostSpawn(Particle, 1.f - FLOAT(i+1) / FLOAT(Number), SpawnTime);

			ActiveParticles++;

			INC_DWORD_STAT(STAT_SpriteParticlesSpawned);
		}
		return NewLeftover;
	}
	return SpawnFraction;
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
FLOAT FParticleEmitterInstance::Spawn(FLOAT OldLeftover, FLOAT Rate, FLOAT DeltaTime, INT Burst, FLOAT BurstTime)
{
	// Ensure continous spawning... lots of fiddling.
	FLOAT	NewLeftover = OldLeftover + DeltaTime * Rate;
	INT		Number		= appFloor(NewLeftover);
	FLOAT	Increment	= 1.f / Rate;
	FLOAT	StartTime	= DeltaTime + OldLeftover * Increment - Increment;
	NewLeftover			= NewLeftover - Number;

	// If we have calculated less than the burst count, force the burst count
	if (Number < Burst)
	{
		Number = Burst;
	}

	// Take the burst time fakery into account
	if (BurstTime > 0.0f)
	{
		NewLeftover -= BurstTime / Burst;
		NewLeftover	= Clamp<FLOAT>(NewLeftover, 0, NewLeftover);
	}

	// Handle growing arrays.
	INT NewCount = ActiveParticles + Number;
	if (NewCount >= MaxActiveParticles)
	{
		if (DeltaTime < PeakActiveParticleUpdateDelta)
		{
			Resize(NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1);
		}
		else
		{
			Resize((NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1), FALSE);
		}
	}

	UParticleLODLevel* LODLevel	= SpriteTemplate->GetCurrentLODLevel(this);
	check(LODLevel);
	// Spawn particles.
	for (INT i=0; i<Number; i++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[ActiveParticles]);

		FLOAT SpawnTime = StartTime - i * Increment;
	
		PreSpawn(Particle);

		if (LODLevel->TypeDataModule)
		{
			UParticleModuleTypeDataBase* pkBase = Cast<UParticleModuleTypeDataBase>(LODLevel->TypeDataModule);
			pkBase->Spawn(this, TypeDataOffset, SpawnTime);
		}

		for (INT ModuleIndex = 0; ModuleIndex < LODLevel->SpawnModules.Num(); ModuleIndex++)
		{
			UParticleModule* SpawnModule	= LODLevel->SpawnModules(ModuleIndex);

			UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
			UParticleModule* OffsetModule	= 	HighestLODLevel->SpawnModules(ModuleIndex);
			UINT* Offset = ModuleOffsetMap.Find(OffsetModule);
			
			if (SpawnModule->bEnabled)
			{
				SpawnModule->Spawn(this, Offset ? *Offset : 0, SpawnTime);
			}
		}
		PostSpawn(Particle, 1.f - FLOAT(i+1) / FLOAT(Number), SpawnTime);

		ActiveParticles++;
	}
	INC_DWORD_STAT_BY(STAT_SpriteParticlesSpawned,Number);

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
FLOAT FParticleEmitterInstance::SpawnEditor(
	UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, FLOAT Multiplier, 
	FLOAT OldLeftover, FLOAT Rate, FLOAT DeltaTime, INT Burst, FLOAT BurstTime)
{
	// Ensure continous spawning... lots of fiddling.
	FLOAT	NewLeftover = OldLeftover + DeltaTime * Rate;
	INT		Number		= appFloor(NewLeftover);
	FLOAT	Increment	= 1.f / Rate;
	FLOAT	StartTime	= DeltaTime + OldLeftover * Increment - Increment;
	NewLeftover			= NewLeftover - Number;

	if (Number < Burst)
	{
		Number = Burst;
	}

	if (BurstTime > 0.0f)
	{
		NewLeftover -= BurstTime / Burst;
		NewLeftover	= Clamp<FLOAT>(NewLeftover, 0, NewLeftover);
	}

	// Handle growing arrays.
	INT NewCount = ActiveParticles + Number;
	if (NewCount >= MaxActiveParticles)
	{
		if (DeltaTime < PeakActiveParticleUpdateDelta)
		{
			Resize(NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1);
		}
		else
		{
			Resize((NewCount + appTrunc(appSqrt((FLOAT)NewCount)) + 1), FALSE);
		}
	}

	// Spawn particles.
	for (INT i=0; i<Number; i++)
	{
		DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[ActiveParticles]);
		
		FLOAT SpawnTime = StartTime - i * Increment;
	
		PreSpawn(Particle);

		if (HighLODLevel->TypeDataModule)
		{
			check(LowLODLevel->TypeDataModule);

			UParticleModuleTypeDataBase* Base = Cast<UParticleModuleTypeDataBase>(HighLODLevel->TypeDataModule);
			UParticleModuleTypeDataBase* LowBase = Cast<UParticleModuleTypeDataBase>(LowLODLevel->TypeDataModule);
			
			Base->SpawnEditor(this, TypeDataOffset, SpawnTime, LowBase, Multiplier);
		}

		for (INT ModuleIndex = 0; ModuleIndex < HighLODLevel->SpawnModules.Num(); ModuleIndex++)
		{
			UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
			UParticleModule* OffsetModule	= 	HighestLODLevel->SpawnModules(ModuleIndex);
			UINT* Offset = ModuleOffsetMap.Find(OffsetModule);

			UParticleModule* HighModule	= HighLODLevel->SpawnModules(ModuleIndex);
			UParticleModule* LowModule	= LowLODLevel->SpawnModules(ModuleIndex);

			if (HighModule->bEnabled)
			{
				HighModule->SpawnEditor(this, Offset ? *Offset : 0, SpawnTime, LowModule, Multiplier);
			}
			else
			if (LowModule->bEnabled)
			{
				LowModule->SpawnEditor(this, Offset ? *Offset : 0, SpawnTime, HighModule, (1.0f - Multiplier));
			}
		}
		PostSpawn(Particle, 1.f - FLOAT(i+1) / FLOAT(Number), SpawnTime);

		ActiveParticles++;

		INC_DWORD_STAT(STAT_SpriteParticlesSpawned);
	}
	return NewLeftover;
}

/**
 *	Handle any pre-spawning actions required for particles
 *
 *	@param	Particle	The particle being spawned.
 */
void FParticleEmitterInstance::PreSpawn(FBaseParticle* Particle)
{
	// By default, just clear out the particle
	appMemzero(Particle, ParticleSize);
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		// If not using local space, initialize the particle location
		Particle->Location = Location;
	}
}

/**
 *	Has the instance completed it's run?
 *
 *	@return	UBOOL	TRUE if the instance is completed, FALSE if not
 */
UBOOL FParticleEmitterInstance::HasCompleted()
{
	// Validity check
	if (SpriteTemplate == NULL)
	{
		return TRUE;
	}

	// If it hasn't finished looping or if it loops forever, not completed.
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	check(LODLevel);
	if ((LODLevel->RequiredModule->EmitterLoops == 0) || 
		(SecondsSinceCreation < (EmitterDuration * LODLevel->RequiredModule->EmitterLoops)))
	{
		return FALSE;
	}

	// If there are active particles, not completed
	if (ActiveParticles > 0)
	{
		return FALSE;
	}

	return TRUE;
}

/**
 *	Handle any post-spawning actions required by the instance
 *
 *	@param	Particle					The particle that was spawned
 *	@param	InterpolationPercentage		The percentage of the time slice it was spawned at
 *	@param	SpawnTIme					The time it was spawned at
 */
void FParticleEmitterInstance::PostSpawn(FBaseParticle* Particle, FLOAT InterpolationPercentage, FLOAT SpawnTime)
{
	// Interpolate position if using world space.
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
	{
		if (FDistSquared(OldLocation, Location) > 1.f)
		{
			Particle->Location += InterpolationPercentage * (OldLocation - Location);	
		}
	}

	// Offset caused by any velocity
	Particle->OldLocation = Particle->Location;
	Particle->Location   += SpawnTime * Particle->Velocity;
}

/**
 *	Kill off any dead particles. (Remove them from the active array)
 */
void FParticleEmitterInstance::KillParticles()
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
			// Move it to the 'back' of the list
			ParticleIndices[i]	= ParticleIndices[ActiveParticles-1];
			ParticleIndices[ActiveParticles-1]	= CurrentIndex;
			ActiveParticles--;

			INC_DWORD_STAT(STAT_SpriteParticlesKilled);
		}
	}
}

/**
 *	Kill the particle at the given instance
 *
 *	@param	Index		The index of the particle to kill.
 */
void FParticleEmitterInstance::KillParticle(INT Index)
{
	if (Index < ActiveParticles)
	{
		INT KillIndex = ParticleIndices[Index];
		// Move it to the 'back' of the list
		for (INT i=Index; i < ActiveParticles - 1; i++)
		{
			ParticleIndices[i] = ParticleIndices[i+1];
		}
		ParticleIndices[ActiveParticles-1] = KillIndex;
		ActiveParticles--;

		INC_DWORD_STAT(STAT_SpriteParticlesKilled);
	}
}

/**
 *	This is used to force "kill" particles irrespective of their duration.
 *	Basically, this takes all particles and moves them to the 'end' of the 
 *	particle list so we can insta kill off trailed particles in the level.
 *
 *	NOTE: we should probably add a boolean to the particle trail type such
 *	that this is Data Driven.  For E3 we are using this.
 */
void FParticleEmitterInstance::KillParticlesForced()
{
	// Loop over the active particles... If their RelativeTime is > 1.0f (indicating they are dead),
	// move them to the 'end' of the active particle list.
	for (INT i=ActiveParticles-1; i>=0; i--)
	{
		const INT	CurrentIndex	= ParticleIndices[i];
		ParticleIndices[i]	= ParticleIndices[ActiveParticles-1];
		ParticleIndices[ActiveParticles-1]	= CurrentIndex;
		ActiveParticles--;

		INC_DWORD_STAT(STAT_SpriteParticlesKilled);
	}
}

/**
 *	Called when the instance if removed from the scene
 *	Perform any actions required, such as removing components, etc.
 */
void FParticleEmitterInstance::RemovedFromScene()
{
}

/**
 *	Retrieve the particle at the given index
 *
 *	@param	Index			The index of the particle of interest
 *
 *	@return	FBaseParticle*	The pointer to the particle. NULL if not present/active
 */
FBaseParticle* FParticleEmitterInstance::GetParticle(INT Index)
{
	// See if the index is valid. If not, return NULL
	if ((Index >= ActiveParticles) || (Index < 0))
	{
		return NULL;
	}

	// Grab and return the particle
	DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[Index]);
	return Particle;
}

/**
 *	Calculates the emitter duration for the instance.
 */
void FParticleEmitterInstance::SetupEmitterDuration()
{
	// Validity check
	if (SpriteTemplate == NULL)
	{
		return;
	}

	// Set up the array for each LOD level
	INT EDCount = EmitterDurations.Num();
	if ((EDCount == 0) || (EDCount != SpriteTemplate->LODLevels.Num()))
	{
		EmitterDurations.Empty();
		EmitterDurations.Insert(0, SpriteTemplate->LODLevels.Num());
	}

	// Calculate the duration for each LOD level
	for (INT LODIndex = 0; LODIndex < SpriteTemplate->LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* TempLOD = SpriteTemplate->LODLevels(LODIndex);
		UParticleModuleRequired* RequiredModule = TempLOD->RequiredModule;
		if (RequiredModule->bEmitterDurationUseRange)
		{
			FLOAT	Rand		= appFrand();
			FLOAT	Duration	= RequiredModule->EmitterDurationLow + 
				((RequiredModule->EmitterDuration - RequiredModule->EmitterDurationLow) * Rand);
			EmitterDurations(TempLOD->Level) = Duration + RequiredModule->EmitterDelay;
		}
		else
		{
			EmitterDurations(TempLOD->Level) = RequiredModule->EmitterDuration + RequiredModule->EmitterDelay;
		}

		if ((LoopCount == 1) && (RequiredModule->bDelayFirstLoopOnly == TRUE) && 
			((RequiredModule->EmitterLoops == 0) || (RequiredModule->EmitterLoops > 1)))
		{
			EmitterDurations(TempLOD->Level) -= RequiredModule->EmitterDelay;
		}
	}

	// Set the current duration
	EmitterDuration	= EmitterDurations(CurrentLODLevelIndex);
}

/*-----------------------------------------------------------------------------
	ParticleSpriteEmitterInstance
-----------------------------------------------------------------------------*/
/**
 *	ParticleSpriteEmitterInstance
 *	The structure for a standard sprite emitter instance.
 */
/** Constructor	*/
FParticleSpriteEmitterInstance::FParticleSpriteEmitterInstance() :
	FParticleEmitterInstance()
{
}

/** Destructor	*/
FParticleSpriteEmitterInstance::~FParticleSpriteEmitterInstance()
{
}

/**
 *	Retrieves the dynamic data for the emitter
 *	
 *	@param	bSelected					Whether the emitter is selected in the editor
 *
 *	@return	FDynamicEmitterDataBase*	The dynamic data, or NULL if it shouldn't be rendered
 */
FDynamicEmitterDataBase* FParticleSpriteEmitterInstance::GetDynamicData(UBOOL bSelected)
{
	// Make sure there is a template present
	if (!SpriteTemplate)
	{
		return NULL;
	}

	// If the template is disabled, don't return data.
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	if ((LODLevel == NULL) || (LODLevel->RequiredModule->bEnabled == FALSE))
	{
		return NULL;
	}

	// Allocate it for now, but we will want to change this to do some form
	// of caching
	if (ActiveParticles > 0)
	{
		// Make sure we will not be allocating enough memory
		// Also clears TTP #33373
		check(MaxActiveParticles >= ActiveParticles);

		// Get the material instance. If there is none, or the material isn't flagged for use with particle systems, use the DefaultMaterial.
		UMaterialInterface* MaterialInst = CurrentMaterial;
		if ((MaterialInst == NULL) || (MaterialInst->UseWithParticleSprites() == FALSE))
		{
			MaterialInst = GEngine->DefaultMaterial;
		}

		// Allocate the dynamic data
		FDynamicSpriteEmitterData* NewEmitterData = ::new FDynamicSpriteEmitterData(MaxActiveParticles, ParticleStride, bRequiresSorting,
			MaterialInst->GetRenderProxy(bSelected),
			(LODLevel->RequiredModule->bUseMaxDrawCount == TRUE) ? LODLevel->RequiredModule->MaxDrawCount : -1
			);
		check(NewEmitterData);
		check(NewEmitterData->ParticleData);
		check(NewEmitterData->ParticleIndices);

		// Fill it in...
		INT		ParticleCount	= ActiveParticles;
		UBOOL	bSorted			= FALSE;
		INT		ScreenAlignment	= LODLevel->RequiredModule->ScreenAlignment;

		// Take scale into account
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
		NewEmitterData->ParticleCount = ActiveParticles;
		NewEmitterData->bSelected = bSelected;
		NewEmitterData->VertexCount = ActiveParticles * 4;
		NewEmitterData->IndexCount = ActiveParticles * 6;
		NewEmitterData->IndexStride = sizeof(WORD);
		NewEmitterData->ScreenAlignment	= LODLevel->RequiredModule->ScreenAlignment;
		NewEmitterData->bUseLocalSpace = LODLevel->RequiredModule->bUseLocalSpace;
		NewEmitterData->EmitterRenderMode = LODLevel->RequiredModule->EmitterRenderMode;
		NewEmitterData->bLockAxis = FALSE;
		NewEmitterData->bSelected = bSelected;
		if (Module_AxisLock && (Module_AxisLock->bEnabled == TRUE))
		{
			NewEmitterData->LockAxisFlag = Module_AxisLock->LockAxisFlags;
			if (Module_AxisLock->LockAxisFlags != EPAL_NONE)
			{
				NewEmitterData->bLockAxis = TRUE;
			}
		}

		// If there are orbit modules, add the orbit module data
		if (LODLevel->OrbitModules.Num() > 0)
		{
			UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
			UParticleModuleOrbit* LastOrbit = HighestLODLevel->OrbitModules(LODLevel->OrbitModules.Num() - 1);
			check(LastOrbit);

			UINT* LastOrbitOffset = ModuleOffsetMap.Find(LastOrbit);
			NewEmitterData->OrbitModuleOffset = *LastOrbitOffset;
		}

		check(NewEmitterData->ParticleData);
		appMemcpy(NewEmitterData->ParticleData, ParticleData, MaxActiveParticles * ParticleStride);
		check(NewEmitterData->ParticleIndices);
		appMemcpy(NewEmitterData->ParticleIndices, ParticleIndices, MaxActiveParticles * sizeof(WORD));

		return NewEmitterData;
	}

	return NULL;
}

/*-----------------------------------------------------------------------------
	ParticleSpriteSubUVEmitterInstance
-----------------------------------------------------------------------------*/
/**
 *	ParticleSpriteSubUVEmitterInstance
 *	Structure for SubUV sprite instances
 */
/** Constructor	*/
FParticleSpriteSubUVEmitterInstance::FParticleSpriteSubUVEmitterInstance() :
	FParticleEmitterInstance()
{
}

/** Destructor	*/
FParticleSpriteSubUVEmitterInstance::~FParticleSpriteSubUVEmitterInstance()
{
}

/**
 *	Kill off any dead particles. (Remove them from the active array)
 */
void FParticleSpriteSubUVEmitterInstance::KillParticles()
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
            FLOAT* pkFloats = (FLOAT*)((BYTE*)&Particle + PayloadOffset);
			pkFloats[0] = 0.0f;
			pkFloats[1] = 0.0f;
			pkFloats[2] = 0.0f;
			pkFloats[3] = 0.0f;
			pkFloats[4] = 0.0f;

			ParticleIndices[i]	= ParticleIndices[ActiveParticles-1];
			ParticleIndices[ActiveParticles-1]	= CurrentIndex;
			ActiveParticles--;
		}
	}
}

/**
 *	Retrieves the dynamic data for the emitter
 *	
 *	@param	bSelected					Whether the emitter is selected in the editor
 *
 *	@return	FDynamicEmitterDataBase*	The dynamic data, or NULL if it shouldn't be rendered
 */
FDynamicEmitterDataBase* FParticleSpriteSubUVEmitterInstance::GetDynamicData(UBOOL bSelected)
{
	// Make sure the template is valid
	if (!SpriteTemplate)
	{
		return NULL;
	}

	// If the template is disabled, don't return data.
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
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
		if ((MaterialInst == NULL) || (MaterialInst->UseWithParticleSubUV() == FALSE))
		{
			MaterialInst = GEngine->DefaultMaterial;
		}

		// Allocate the dynamic data for the instance
		FDynamicSubUVEmitterData* NewEmitterData = ::new FDynamicSubUVEmitterData(MaxActiveParticles, ParticleStride, bRequiresSorting,
			MaterialInst->GetRenderProxy(bSelected),
			(LODLevel->RequiredModule->bUseMaxDrawCount == TRUE) ? LODLevel->RequiredModule->MaxDrawCount : -1
			);
		check(NewEmitterData);
		check(NewEmitterData->ParticleData);
		check(NewEmitterData->ParticleIndices);

		// Fill it in...
		NewEmitterData->ActiveParticleCount = ActiveParticles;
		NewEmitterData->bSelected = bSelected;
		NewEmitterData->ScreenAlignment	= LODLevel->RequiredModule->ScreenAlignment;
		NewEmitterData->bUseLocalSpace = LODLevel->RequiredModule->bUseLocalSpace;
		NewEmitterData->SubUVDataOffset = SubUVDataOffset;
		NewEmitterData->SubImages_Horizontal = LODLevel->RequiredModule->SubImages_Horizontal;
		NewEmitterData->SubImages_Vertical = LODLevel->RequiredModule->SubImages_Vertical;
		NewEmitterData->bDirectUV = LODLevel->RequiredModule->bDirectUV;
		NewEmitterData->EmitterRenderMode = LODLevel->RequiredModule->EmitterRenderMode;
		NewEmitterData->bSelected = bSelected;

		INT		ParticleCount	= ActiveParticles;
		UBOOL	bSorted			= FALSE;
		INT		ScreenAlignment	= LODLevel->RequiredModule->ScreenAlignment;

		// Take scale into account
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

		NewEmitterData->VertexCount = ActiveParticles * 4;
		NewEmitterData->IndexCount = ActiveParticles * 6;
		NewEmitterData->IndexStride = sizeof(WORD);

		NewEmitterData->bLockAxis = FALSE;
		if (Module_AxisLock && (Module_AxisLock->bEnabled == TRUE))
		{
			NewEmitterData->LockAxisFlag = Module_AxisLock->LockAxisFlags;
			if (Module_AxisLock->LockAxisFlags != EPAL_NONE)
			{
				NewEmitterData->bLockAxis	= TRUE;
			}
		}

		// If there are orbit modules, add the orbit module data
		if (LODLevel->OrbitModules.Num() > 0)
		{
			UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
			UParticleModuleOrbit* LastOrbit = HighestLODLevel->OrbitModules(LODLevel->OrbitModules.Num() - 1);
			check(LastOrbit);

			UINT* LastOrbitOffset = ModuleOffsetMap.Find(LastOrbit);
			NewEmitterData->OrbitModuleOffset = *LastOrbitOffset;
		}

		appMemcpy(NewEmitterData->ParticleData, ParticleData, MaxActiveParticles * ParticleStride);
		appMemcpy(NewEmitterData->ParticleIndices, ParticleIndices, MaxActiveParticles * sizeof(WORD));

		return NewEmitterData;
	}

	return NULL;
}

/*-----------------------------------------------------------------------------
	ParticleMeshEmitterInstance
-----------------------------------------------------------------------------*/
/**
 *	Structure for mesh emitter instances
 */

/** Constructor	*/
FParticleMeshEmitterInstance::FParticleMeshEmitterInstance() :
	  FParticleEmitterInstance()
	, MeshTypeData(NULL)
	, MeshComponentIndex(-1)
	, MeshRotationActive(FALSE)
	, MeshRotationOffset(0)
{
}

/** Destructor	*/
FParticleMeshEmitterInstance::~FParticleMeshEmitterInstance()
{
	if (Component && (MeshComponentIndex >= 0) && (MeshComponentIndex < Component->SMComponents.Num()))
	{
		Component->SMComponents(MeshComponentIndex) = NULL;
		MeshComponentIndex = -1;
	}
}

/**
 *	Initialize the parameters for the structure
 *
 *	@param	InTemplate		The ParticleEmitter to base the instance on
 *	@param	InComponent		The owning ParticleComponent
 *	@param	bClearResources	If TRUE, clear all resource data
 */
void FParticleMeshEmitterInstance::InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent, UBOOL bClearResources)
{
	FParticleEmitterInstance::InitParameters(InTemplate, InComponent, bClearResources);

	// Get the type data module
	UParticleLODLevel* LODLevel	= InTemplate->GetLODLevel(0);
	check(LODLevel);
	MeshTypeData = CastChecked<UParticleModuleTypeDataMesh>(LODLevel->TypeDataModule);
	check(MeshTypeData);

    // Grab the MeshRotationRate module offset, if there is one...
    MeshRotationActive = FALSE;
	if (LODLevel->RequiredModule->ScreenAlignment == PSA_Velocity)
	{
		MeshRotationActive = TRUE;
	}
	else
	{
	    for (INT i = 0; i < LODLevel->Modules.Num(); i++)
	    {
	        if (LODLevel->Modules(i)->IsA(UParticleModuleMeshRotationRate::StaticClass())	||
	            LODLevel->Modules(i)->IsA(UParticleModuleMeshRotation::StaticClass())		||
				LODLevel->Modules(i)->IsA(UParticleModuleUberRainImpacts::StaticClass())	||
				LODLevel->Modules(i)->IsA(UParticleModuleUberRainSplashA::StaticClass())
				)
	        {
	            MeshRotationActive = TRUE;
	            break;
	        }
		}
    }
}

/**
 *	Initialize the instance
 */
void FParticleMeshEmitterInstance::Init()
{
	FParticleEmitterInstance::Init();

	// If there is a mesh present (there should be!)
	if (MeshTypeData->Mesh)
	{
		UStaticMeshComponent* MeshComponent = NULL;

		// If the index is set, try to retrieve it from the component
		if (MeshComponentIndex != -1)
		{
			if (MeshComponentIndex < Component->SMComponents.Num())
			{
				MeshComponent = Component->SMComponents(MeshComponentIndex);
			}
			// If it wasn't retrieved, force it to get recreated
			if (MeshComponent == NULL)
			{
				MeshComponentIndex = -1;
			}
		}

		if (MeshComponentIndex == -1)
		{
			// create the component if necessary
			MeshComponent = ConstructObject<UStaticMeshComponent>(UStaticMeshComponent::StaticClass(),Component);
			MeshComponent->bAcceptsDecals		= FALSE;
			MeshComponent->CollideActors		= FALSE;
			MeshComponent->BlockActors			= FALSE;
			MeshComponent->BlockZeroExtent		= FALSE;
			MeshComponent->BlockNonZeroExtent	= FALSE;
			MeshComponent->BlockRigidBody		= FALSE;
			// allocate space for material instance constants
			MeshComponent->Materials.InsertZeroed(0, MeshTypeData->Mesh->LODModels(0).Elements.Num());
			MeshComponent->StaticMesh		= MeshTypeData->Mesh;
			MeshComponent->CastShadow		= MeshTypeData->CastShadows;
			MeshComponent->bAcceptsLights	= Component->bAcceptsLights;

			MeshComponentIndex = Component->SMComponents.AddItem(MeshComponent);
		}
		check(MeshComponent);

		// Constructing MaterialInstanceConstant for each mesh instance is done so that
		// particle 'vertex color' can be set on each individual mesh.
		// They are tagged as transient so they don't get saved in the package.
		for (INT MatIndex = 0; MatIndex < MeshTypeData->Mesh->LODModels(0).Elements.Num(); MatIndex++)
		{
			FStaticMeshElement* MeshElement = &(MeshTypeData->Mesh->LODModels(0).Elements(MatIndex));
			UMaterialInterface* Parent = MeshElement->Material;
			if (Parent)
			{
				UMaterial* CheckMat = Parent->GetMaterial();
				if (CheckMat)
				{
					// during post load, the item will be tagged as Lit if the material applied to the sprite temple is (LightingModel != MLM_Unlit). This means the Attach/Detach light calls will occur on it.
					// In the case of mesh emitters, the actual material utilized is the one applied to the mesh itself. This check just warns that the SpriteTemplate material does not match the mesh material. This can lead to improper lighting - ie the mesh may not need lighting according to it's material, but the sprite material thinks it does need it, and vice-versa.
					if (Component->bAcceptsLights != (CheckMat->LightingModel != MLM_Unlit))
					{
						UBOOL bLitMaterials = FALSE;
						// Make sure no other emitter requires listing
						for (INT CheckEmitterIndex = 0; CheckEmitterIndex < Component->Template->Emitters.Num(); CheckEmitterIndex++)
						{
							UParticleEmitter* CheckEmitter = Component->Template->Emitters(CheckEmitterIndex);
							if (CheckEmitter != SpriteTemplate)
							{
								UParticleSpriteEmitter* SpriteEmitter = Cast<UParticleSpriteEmitter>(CheckEmitter);
								if (SpriteEmitter)
								{
									UParticleLODLevel* LODLevel	= SpriteEmitter->GetLODLevel(0);
									if (LODLevel && LODLevel->RequiredModule && LODLevel->bEnabled)
									{
										if (LODLevel->RequiredModule->Material && LODLevel->RequiredModule->Material->GetMaterial()->LightingModel != MLM_Unlit)
										{
											bLitMaterials	= TRUE;
											break;
										}
									}
								}
							}
						}

						if (bLitMaterials != Component->bAcceptsLights)
						{
							INT ReportIndex = -1;
							for (INT EmitterIndex = 0; EmitterIndex < Component->Template->Emitters.Num(); EmitterIndex++)
							{
								UParticleEmitter* Emitter = Component->Template->Emitters(EmitterIndex);
								if (Emitter)
								{
									if (Emitter == SpriteTemplate)
									{
										ReportIndex = EmitterIndex;
										break;
									}
								}
							}
#if defined(_MESH_PARTICLE_EMITTER_LIT_MISMATCH_)
							warnf(TEXT("MeshEmitter - lighting mode mismatch!\n\t\tTo correct this problem, set the Emitter->Material to the same thing as the material applied to the static mesh being used in the mesh emitter.\n\tMaterial: %s\n\tEmitter:  %s\n\tPSys:     %s (#%d)\n\t\t\t(NOTE: It may be anoter emitter in the particle system...)"), 
							*(CheckMat->GetFullName()), *GetFullName(),
							Component ? (Component->Template? *Component->Template->GetName() : TEXT("No Template")) : TEXT("No Component!"), ReportIndex);
#endif	//#if defined(_MESH_PARTICLE_EMITTER_LIT_MISMATCH_)
						}
					}
				}

				UMaterialInstanceConstant* MatInst = NULL;
				if (MeshComponent->Materials.Num() > MatIndex)
				{
					MatInst = Cast<UMaterialInstanceConstant>(MeshComponent->Materials(MatIndex));
				}
				if (MatInst == NULL)
				{
					// create the instance constant if necessary
					MatInst = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), MeshComponent);
					if (MeshComponent->Materials.Num() > MatIndex)
					{
						MeshComponent->Materials(MatIndex) = MatInst;
					}
					else
					{
						INT CheckIndex = MeshComponent->Materials.AddItem(MatInst);
						check(CheckIndex == MatIndex);
					}
				}
				MatInst->SetParent(Parent);
				MatInst->SetFlags(RF_Transient);
			}
		}
	}
}

/**
 *	Resize the particle data array
 *
 *	@param	NewMaxActiveParticles	The new size to use
 */
void FParticleMeshEmitterInstance::Resize(INT NewMaxActiveParticles, UBOOL bSetMaxActiveCount)
{
	INT OldMaxActiveParticles = MaxActiveParticles;
	FParticleEmitterInstance::Resize(NewMaxActiveParticles, bSetMaxActiveCount);

	if (MeshRotationActive)
    {
        for (INT i = OldMaxActiveParticles; i < NewMaxActiveParticles; i++)
	    {
		    DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
            FMeshRotationPayloadData* PayloadData	= (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshRotationOffset);
			PayloadData->RotationRateBase			= FVector(0.0f);
	    }
    }
}

/**
 *	Tick the instance.
 *
 *	@param	DeltaTime			The time slice to use
 *	@param	bSuppressSpawning	If TRUE, do not spawn during Tick
 */
void FParticleMeshEmitterInstance::Tick(FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	// See if we are handling mesh rotation
    if (MeshRotationActive)
    {
		// Update the rotation for each particle
        for (INT i = 0; i < ActiveParticles; i++)
	    {
		    DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
            FMeshRotationPayloadData* PayloadData	= (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshRotationOffset);
			PayloadData->RotationRate				= PayloadData->RotationRateBase;
			if (LODLevel->RequiredModule->ScreenAlignment == PSA_Velocity)
			{
				// Determine the rotation to the velocity vector and apply it to the mesh
				FVector	NewDirection	= Particle.Velocity;
				NewDirection.Normalize();
				FVector	OldDirection(1.0f, 0.0f, 0.0f);

				FQuat Rotation	= FQuatFindBetween(OldDirection, NewDirection);
				FVector Euler	= Rotation.Euler();
				PayloadData->Rotation.X	= Euler.X;
				PayloadData->Rotation.Y	= Euler.Y;
				PayloadData->Rotation.Z	= Euler.Z;
			}
	    }
    }

	// Call the standard tick
	FParticleEmitterInstance::Tick(DeltaTime, bSuppressSpawning);

	// Apply rotation if it is active
    if (MeshRotationActive)
    {
        for (INT i = 0; i < ActiveParticles; i++)
	    {
		    DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
			if ((Particle.Flags & STATE_Particle_FreezeRotation) == 0)
			{
	            FMeshRotationPayloadData* PayloadData	 = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshRotationOffset);
				PayloadData->Rotation					+= DeltaTime * PayloadData->RotationRate;
			}
        }
    }

	// Do we need to tick the mesh instances or will the engine do it?
	if ((ActiveParticles == 0) & bSuppressSpawning)
	{
		RemovedFromScene();
	}
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
void FParticleMeshEmitterInstance::TickEditor(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, FLOAT Multiplier, FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
    if (MeshRotationActive)
    {
        for (INT i = 0; i < ActiveParticles; i++)
	    {
		    DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
            FMeshRotationPayloadData* PayloadData	= (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshRotationOffset);
			PayloadData->RotationRate				= PayloadData->RotationRateBase;
			if (LODLevel->RequiredModule->ScreenAlignment == PSA_Velocity)
			{
				// Determine the rotation to the velocity vector and apply it to the mesh
				FVector	NewDirection	= Particle.Velocity;
				NewDirection.Normalize();
				FVector	OldDirection(1.0f, 0.0f, 0.0f);

				FQuat Rotation	= FQuatFindBetween(OldDirection, NewDirection);
				FVector Euler	= Rotation.Euler();
				PayloadData->Rotation.X	= Euler.X;
				PayloadData->Rotation.Y	= Euler.Y;
				PayloadData->Rotation.Z	= Euler.Z;
			}
	    }
    }

	// 
	FParticleEmitterInstance::TickEditor(HighLODLevel, LowLODLevel, Multiplier, DeltaTime, bSuppressSpawning);

    if (MeshRotationActive)
    {
        for (INT i = 0; i < ActiveParticles; i++)
	    {
		    DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
			if ((Particle.Flags & STATE_Particle_FreezeRotation) == 0)
			{
	            FMeshRotationPayloadData* PayloadData	 = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshRotationOffset);
				PayloadData->Rotation					+= DeltaTime * PayloadData->RotationRate;
			}
        }
    }

	// Do we need to tick the mesh instances or will the engine do it?
	if ((ActiveParticles == 0) & bSuppressSpawning)
	{
		RemovedFromScene();
	}
}

/**
 *	Update the bounding box for the emitter
 *
 *	@param	DeltaTime		The time slice to use
 */
void FParticleMeshEmitterInstance::UpdateBoundingBox(FLOAT DeltaTime)
{
	//@todo. Implement proper bound determination for mesh emitters.
	// Currently, just 'forcing' the mesh size to be taken into account.
	if (Component)
	{
		// Take scale into account
		FVector Scale = FVector(1.0f, 1.0f, 1.0f);
		Scale *= Component->Scale * Component->Scale3D;
		AActor* Actor = Component->GetOwner();
		if (Actor && !Component->AbsoluteScale)
		{
			Scale *= Actor->DrawScale * Actor->DrawScale3D;
		}

		// Get the static mesh bounds
		FBoxSphereBounds MeshBound;
		if (Component->bWarmingUp == FALSE)
		{	
			if (MeshTypeData->Mesh)
			{
				MeshBound = MeshTypeData->Mesh->Bounds;
			}
			else
			{
				warnf(TEXT("MeshEmitter with no mesh set?? - %s"), Component->Template ? *(Component->Template->GetPathName()) : TEXT("??????"));
				MeshBound = FBoxSphereBounds();
			}
		}
		else
		{
			// This isn't used anywhere if the bWarmingUp flag is false, but GCC doesn't like it not touched.
			appMemzero(&MeshBound, sizeof(FBoxSphereBounds));
		}

		FLOAT MaxSizeScale	= 1.0f;
		FVector	NewLocation;
		FLOAT	NewRotation;
		ParticleBoundingBox.Init();
		for (INT i=0; i<ActiveParticles; i++)
		{
			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
			
			// Do linear integrator and update bounding box
			Particle.OldLocation = Particle.Location;
			if ((Particle.Flags & STATE_Particle_Freeze) == 0)
			{
				if ((Particle.Flags & STATE_Particle_FreezeTranslation) == 0)
				{
					NewLocation	= Particle.Location + DeltaTime * Particle.Velocity;
				}
				else
				{
					NewLocation = Particle.Location;
				}
				if ((Particle.Flags & STATE_Particle_FreezeRotation) == 0)
				{
					NewRotation	= Particle.Rotation + DeltaTime * Particle.RotationRate;
				}
				else
				{
					NewRotation = Particle.Rotation;
				}
			}
			else
			{
				// Don't move it...
				NewLocation = Particle.Location;
				NewRotation = Particle.Rotation;
			}

			// Do angular integrator, and wrap result to within +/- 2 PI
			FVector Size = Particle.Size * Scale;
			MaxSizeScale = Max(MaxSizeScale, Size.GetAbsMax()); //@todo particles: this does a whole lot of compares that can be avoided using SSE/ Altivec.

			Particle.Rotation = appFmod(NewRotation, 2.f*(FLOAT)PI);
			Particle.Location = NewLocation;
			if (Component->bWarmingUp == FALSE)
			{	
				ParticleBoundingBox += (Particle.Location + MeshBound.SphereRadius * Size);
				ParticleBoundingBox += (Particle.Location - MeshBound.SphereRadius * Size);
			}
		}

		if (Component->bWarmingUp == FALSE)
		{	
			ParticleBoundingBox = ParticleBoundingBox.ExpandBy(MaxSizeScale * PARTICLESYSTEM_BOUNDSSCALAR);
			// Transform bounding box into world space if the emitter uses a local space coordinate system.
			UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
			check(LODLevel);
			if (LODLevel->RequiredModule->bUseLocalSpace) 
			{
				ParticleBoundingBox = ParticleBoundingBox.TransformBy(Component->LocalToWorld);
			}
		}
	}
}

/**
 *	Retrieved the per-particle bytes that this emitter type requires.
 *
 *	@return	UINT	The number of required bytes for particles in the instance
 */
UINT FParticleMeshEmitterInstance::RequiredBytes()
{
	UINT uiBytes = FParticleEmitterInstance::RequiredBytes();
	MeshRotationOffset	= PayloadOffset + uiBytes;
	uiBytes += sizeof(FMeshRotationPayloadData);
	return uiBytes;
}

/**
 *	Handle any post-spawning actions required by the instance
 *
 *	@param	Particle					The particle that was spawned
 *	@param	InterpolationPercentage		The percentage of the time slice it was spawned at
 *	@param	SpawnTIme					The time it was spawned at
 */
void FParticleMeshEmitterInstance::PostSpawn(FBaseParticle* Particle, FLOAT InterpolationPercentage, FLOAT SpawnTime)
{
	FParticleEmitterInstance::PostSpawn(Particle, InterpolationPercentage, SpawnTime);
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	if (LODLevel->RequiredModule->ScreenAlignment == PSA_Velocity)
	{
		// Determine the rotation to the velocity vector and apply it to the mesh
		FVector	NewDirection	= Particle->Velocity;
		NewDirection.Normalize();
		FVector	OldDirection(1.0f, 0.0f, 0.0f);

		FQuat Rotation	= FQuatFindBetween(OldDirection, NewDirection);
		FVector Euler	= Rotation.Euler();

		FMeshRotationPayloadData* PayloadData	= (FMeshRotationPayloadData*)((BYTE*)Particle + MeshRotationOffset);
		PayloadData->Rotation.X	+= Euler.X;
		PayloadData->Rotation.Y	+= Euler.Y;
		PayloadData->Rotation.Z	+= Euler.Z;
		//
	}
}

/**
 *	Retrieves the dynamic data for the emitter
 *	
 *	@param	bSelected					Whether the emitter is selected in the editor
 *
 *	@return	FDynamicEmitterDataBase*	The dynamic data, or NULL if it shouldn't be rendered
 */
FDynamicEmitterDataBase* FParticleMeshEmitterInstance::GetDynamicData(UBOOL bSelected)
{
	// Make sure the template is valid
	if (!SpriteTemplate)
	{
		return NULL;
	}

	// If the template is disabled, don't return data.
	UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	if ((LODLevel == NULL) ||
		(LODLevel->RequiredModule->bEnabled == FALSE))
	{
		return NULL;
	}
	CurrentMaterial = LODLevel->RequiredModule->Material;

	if ((MeshComponentIndex == -1) || (MeshComponentIndex >= Component->SMComponents.Num()))
	{
		// Not initialized?
		return NULL;
	}

	UStaticMeshComponent* MeshComponent = Component->SMComponents(MeshComponentIndex);
	if (MeshComponent == NULL)
	{
		// The mesh component has been GC'd?
		return NULL;
	}

	// Allocate it for now, but we will want to change this to do some form
	// of caching
	if (ActiveParticles > 0)
	{
		// Get the material isntance. If none is present, use the DefaultMaterial
		// Allocate the dynamic data
		FMaterialRenderProxy* MatResource = NULL;
		if (LODLevel->TypeDataModule)
		{
			UParticleModuleTypeDataMesh* MeshTD = CastChecked<UParticleModuleTypeDataMesh>(LODLevel->TypeDataModule);
			if (MeshTD->bOverrideMaterial == TRUE)
			{
				if (CurrentMaterial)
				{
					MatResource = CurrentMaterial->GetRenderProxy(bSelected);
				}
			}
		}
		FDynamicMeshEmitterData* NewEmitterData = ::new FDynamicMeshEmitterData(
			this, 
			MaxActiveParticles, ParticleStride, bRequiresSorting,
			MatResource, 
			LODLevel->RequiredModule->InterpolationMethod, 
			SubUVDataOffset, MeshTypeData->Mesh, MeshComponent,
			(LODLevel->RequiredModule->bUseMaxDrawCount == TRUE) ? LODLevel->RequiredModule->MaxDrawCount : -1
			);
		check(NewEmitterData);
		check(NewEmitterData->ParticleData);
		check(NewEmitterData->ParticleIndices);

		// Take scale into account
		NewEmitterData->Scale = FVector(1.0f, 1.0f, 1.0f);
		if (Component)
		{
			check(SpriteTemplate);
			UParticleLODLevel* LODLevel2 = SpriteTemplate->GetCurrentLODLevel(this);
			check(LODLevel2);
			check(LODLevel2->RequiredModule);
			// Take scale into account
			if (LODLevel2->RequiredModule->bUseLocalSpace == FALSE)
			{
				NewEmitterData->Scale *= Component->Scale * Component->Scale3D;
				AActor* Actor = Component->GetOwner();
				if (Actor && !Component->AbsoluteScale)
				{
					NewEmitterData->Scale *= Actor->DrawScale * Actor->DrawScale3D;
				}
			}
		}

		// Fill it in...
		INT		ParticleCount	= ActiveParticles;
		UBOOL	bSorted			= FALSE;

		NewEmitterData->ActiveParticleCount = ActiveParticles;
		NewEmitterData->bSelected = bSelected;
		NewEmitterData->ScreenAlignment = LODLevel->RequiredModule->ScreenAlignment;
		NewEmitterData->bUseLocalSpace = LODLevel->RequiredModule->bUseLocalSpace;
		NewEmitterData->MeshAlignment = MeshTypeData->MeshAlignment;
		NewEmitterData->bMeshRotationActive = MeshRotationActive;
		NewEmitterData->MeshRotationOffset = MeshRotationOffset;
		NewEmitterData->bScaleUV = LODLevel->RequiredModule->bScaleUV;
		NewEmitterData->SubImages_Horizontal = LODLevel->RequiredModule->SubImages_Horizontal;
		NewEmitterData->SubImages_Vertical = LODLevel->RequiredModule->SubImages_Vertical;
		NewEmitterData->SubUVOffset = FVector(0.0f);
		NewEmitterData->EmitterRenderMode = LODLevel->RequiredModule->EmitterRenderMode;
		NewEmitterData->bSelected = bSelected;

		if (Module_AxisLock && (Module_AxisLock->bEnabled == TRUE))
		{
			NewEmitterData->LockAxisFlag = Module_AxisLock->LockAxisFlags;
			if (Module_AxisLock->LockAxisFlags != EPAL_NONE)
			{
				NewEmitterData->bLockAxis = TRUE;
			}
		}

		// If there are orbit modules, add the orbit module data
		if (LODLevel->OrbitModules.Num() > 0)
		{
			UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels(0);
			UParticleModuleOrbit* LastOrbit = HighestLODLevel->OrbitModules(LODLevel->OrbitModules.Num() - 1);
			check(LastOrbit);

			UINT* LastOrbitOffset = ModuleOffsetMap.Find(LastOrbit);
			NewEmitterData->OrbitModuleOffset = *LastOrbitOffset;
		}

		appMemcpy(NewEmitterData->ParticleData, ParticleData, MaxActiveParticles * ParticleStride);
		appMemcpy(NewEmitterData->ParticleIndices, ParticleIndices, MaxActiveParticles * sizeof(WORD));

		return NewEmitterData;
	}
	return NULL;
}
