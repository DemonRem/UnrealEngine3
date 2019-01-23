/*=============================================================================
	ParticleModules_Orbit.cpp: Orbit particle modules implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"
#include "EngineParticleClasses.h"

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleOrbitBase);

/*-----------------------------------------------------------------------------
	UParticleModuleOrbit implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleOrbit);

/**
 *	Called on a particle that is freshly spawned by the emitter.
 *	
 *	@param	Owner		The FParticleEmitterInstance that spawned the particle.
 *	@param	Offset		The modules offset into the data payload of the particle.
 *	@param	SpawnTime	The time of the spawn.
 */
void UParticleModuleOrbit::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	{
		PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);

		if (OffsetOptions.bProcessDuringSpawn == TRUE)
		{
			// Process the offset
			FVector LocalOffset;
			if (OffsetOptions.bUseEmitterTime == FALSE)
			{
				LocalOffset = OffsetAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalOffset = OffsetAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			OrbitPayload.BaseOffset += LocalOffset;
			OrbitPayload.Offset += LocalOffset;
		}

		if (RotationOptions.bProcessDuringSpawn == TRUE)
		{
			// Process the rotation
			FVector LocalRotation;
			if (RotationOptions.bUseEmitterTime == FALSE)
			{
				LocalRotation = RotationAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalRotation = RotationAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			OrbitPayload.Rotation += LocalRotation;
		}

		if (RotationRateOptions.bProcessDuringSpawn == TRUE)
		{
			// Process the rotation rate
			FVector LocalRotationRate;
			if (RotationRateOptions.bUseEmitterTime == FALSE)
			{
				LocalRotationRate = RotationRateAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalRotationRate = RotationRateAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			OrbitPayload.BaseRotationRate += LocalRotationRate;
			OrbitPayload.RotationRate += LocalRotationRate;
		}
	}
}

/**
 *	Called on a particle that is being updated by its emitter.
 *	
 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
 *	@param	Offset		The modules offset into the data payload of the particle.
 *	@param	DeltaTime	The time since the last update.
 */
void UParticleModuleOrbit::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
	{
		PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);

		if (OffsetOptions.bProcessDuringUpdate == TRUE)
		{
			// Process the offset
			FVector LocalOffset;
			if (OffsetOptions.bUseEmitterTime == FALSE)
			{
				LocalOffset = OffsetAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalOffset = OffsetAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}

			//@todo. Do we need to update the base offset here???
//			OrbitPayload.BaseOffset += LocalOffset;
			OrbitPayload.Offset += LocalOffset;
		}

		if (RotationOptions.bProcessDuringUpdate == TRUE)
		{
			// Process the rotation
			FVector LocalRotation;
			if (RotationOptions.bUseEmitterTime == FALSE)
			{
				LocalRotation = RotationAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalRotation = RotationAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			OrbitPayload.Rotation += LocalRotation;
		}


		if (RotationRateOptions.bProcessDuringUpdate == TRUE)
		{
			// Process the rotation rate
			FVector LocalRotationRate;
			if (RotationRateOptions.bUseEmitterTime == FALSE)
			{
				LocalRotationRate = RotationRateAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalRotationRate = RotationRateAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			//@todo. Do we need to update the base rotationrate here???
//			OrbitPayload.BaseRotationRate += LocalRotationRate;
			OrbitPayload.RotationRate += LocalRotationRate;
		}
	}
	END_UPDATE_LOOP;
}

/**
 *	Returns the number of bytes that the module requires in the particle payload block.
 *
 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
 *
 *	@return	UINT		The number of bytes the module needs per particle.
 */
UINT UParticleModuleOrbit::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FOrbitChainModuleInstancePayload);
}

/**
 *	Returns the number of bytes the module requires in the emitters 'per-instance' data block.
 *	
 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
 *
 *	@return UINT		The number fo bytes the module needs per emitter instance.
 */
UINT UParticleModuleOrbit::RequiredBytesPerInstance(FParticleEmitterInstance* Owner)
{
	return 0;
}

// For Cascade
/**
 *	In the editor, called on a particle that is freshly spawned by the emitter.
 *	Is responsible for performing the intepolation between LOD levels for realtime 
 *	previewing.
 *	
 *	@param	Owner			The FParticleEmitterInstance that spawned the particle.
 *	@param	Offset			The modules offset into the data payload of the particle.
 *	@param	SpawnTime		The time of the spawn.
 *	@param	LowerLODModule	The next lower LOD module - used during interpolation.
 *	@param	Multiplier		The interpolation alpha value to use.
 */
void UParticleModuleOrbit::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleModuleOrbit* LowModule = Cast<UParticleModuleOrbit>(LowerLODModule);
	SPAWN_INIT;
	{
		PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);

		if (OffsetOptions.bProcessDuringSpawn == TRUE)
		{
			// Process the offset
			FVector LocalOffset;
			FVector LocalOffsetA;
			FVector LocalOffsetB;
			if (OffsetOptions.bUseEmitterTime == FALSE)
			{
				LocalOffsetA = OffsetAmount.GetValue(Particle.RelativeTime, Owner->Component);
				LocalOffsetB = LowModule->OffsetAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalOffsetA = OffsetAmount.GetValue(Owner->EmitterTime, Owner->Component);
				LocalOffsetB = LowModule->OffsetAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			LocalOffset = (LocalOffsetA * Multiplier) + (LocalOffsetB * (1.0f - Multiplier));

			OrbitPayload.BaseOffset += LocalOffset;
			OrbitPayload.Offset += LocalOffset;
		}

		if (RotationOptions.bProcessDuringSpawn == TRUE)
		{
			// Process the rotation
			FVector LocalRotation;
			FVector LocalRotationA;
			FVector LocalRotationB;
			if (RotationOptions.bUseEmitterTime == FALSE)
			{
				LocalRotationA = RotationAmount.GetValue(Particle.RelativeTime, Owner->Component);
				LocalRotationB = LowModule->RotationAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalRotationA = RotationAmount.GetValue(Owner->EmitterTime, Owner->Component);
				LocalRotationB = LowModule->RotationAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			LocalRotation = (LocalRotationA * Multiplier) + (LocalRotationB * (1.0f - Multiplier));
			OrbitPayload.Rotation += LocalRotation;
		}

		if (RotationRateOptions.bProcessDuringSpawn == TRUE)
		{
			// Process the rotation rate
			FVector LocalRotationRate;
			FVector LocalRotationRateA;
			FVector LocalRotationRateB;
			if (RotationRateOptions.bUseEmitterTime == FALSE)
			{
				LocalRotationRateA = RotationRateAmount.GetValue(Particle.RelativeTime, Owner->Component);
				LocalRotationRateB = LowModule->RotationRateAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalRotationRateA = RotationRateAmount.GetValue(Owner->EmitterTime, Owner->Component);
				LocalRotationRateB = LowModule->RotationRateAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			LocalRotationRate = (LocalRotationRateA * Multiplier) + (LocalRotationRateB * (1.0f - Multiplier));

			OrbitPayload.BaseRotationRate += LocalRotationRate;
			OrbitPayload.RotationRate += LocalRotationRate;
		}
	}
}

/**
 *	In the editor, called on a particle that is being updated by its emitter.
 *	Is responsible for performing the intepolation between LOD levels for realtime 
 *	previewing.
 *	
 *	@param	Owner			The FParticleEmitterInstance that 'owns' the particle.
 *	@param	Offset			The modules offset into the data payload of the particle.
 *	@param	DeltaTime		The time since the last update.
 *	@param	LowerLODModule	The next lower LOD module - used during interpolation.
 *	@param	Multiplier		The interpolation alpha value to use.
 */
void UParticleModuleOrbit::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	UParticleModuleOrbit* LowModule = Cast<UParticleModuleOrbit>(LowerLODModule);
	BEGIN_UPDATE_LOOP;
	{
		PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);

		if (OffsetOptions.bProcessDuringUpdate == TRUE)
		{
			// Process the offset
			FVector LocalOffset;
			FVector LocalOffsetA;
			FVector LocalOffsetB;
			if (OffsetOptions.bUseEmitterTime == FALSE)
			{
				LocalOffsetA = OffsetAmount.GetValue(Particle.RelativeTime, Owner->Component);
				LocalOffsetB = LowModule->OffsetAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalOffsetA = OffsetAmount.GetValue(Owner->EmitterTime, Owner->Component);
				LocalOffsetB = LowModule->OffsetAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			LocalOffset = (LocalOffsetA * Multiplier) + (LocalOffsetB * (1.0f - Multiplier));

			//@todo. Do we need to update the base offset here???
//			OrbitPayload.BaseOffset += LocalOffset;
			OrbitPayload.Offset += LocalOffset;
		}

		if (RotationOptions.bProcessDuringUpdate == TRUE)
		{
			// Process the rotation
			FVector LocalRotation;
			FVector LocalRotationA;
			FVector LocalRotationB;
			if (RotationOptions.bUseEmitterTime == FALSE)
			{
				LocalRotationA = RotationAmount.GetValue(Particle.RelativeTime, Owner->Component);
				LocalRotationB = LowModule->RotationAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalRotationA = RotationAmount.GetValue(Owner->EmitterTime, Owner->Component);
				LocalRotationB = LowModule->RotationAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			LocalRotation = (LocalRotationA * Multiplier) + (LocalRotationB * (1.0f - Multiplier));
			OrbitPayload.Rotation += LocalRotation;
		}

		if (RotationRateOptions.bProcessDuringUpdate == TRUE)
		{
			// Process the rotation rate
			// Process the rotation rate
			FVector LocalRotationRate;
			FVector LocalRotationRateA;
			FVector LocalRotationRateB;
			if (RotationRateOptions.bUseEmitterTime == FALSE)
			{
				LocalRotationRateA = RotationRateAmount.GetValue(Particle.RelativeTime, Owner->Component);
				LocalRotationRateB = LowModule->RotationRateAmount.GetValue(Particle.RelativeTime, Owner->Component);
			}
			else
			{
				LocalRotationRateA = RotationRateAmount.GetValue(Owner->EmitterTime, Owner->Component);
				LocalRotationRateB = LowModule->RotationRateAmount.GetValue(Owner->EmitterTime, Owner->Component);
			}
			LocalRotationRate = (LocalRotationRateA * Multiplier) + (LocalRotationRateB * (1.0f - Multiplier));

			//@todo. Do we need to update the base rotationrate here???
//			OrbitPayload.BaseRotationRate += LocalRotationRate;
			OrbitPayload.RotationRate += LocalRotationRate;
		}
	}
	END_UPDATE_LOOP;
}
