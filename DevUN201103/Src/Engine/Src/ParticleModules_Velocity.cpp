/*=============================================================================
	ParticleModules_Velocity.cpp: 
	Velocity-related particle module implementations.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleVelocityBase);

/*-----------------------------------------------------------------------------
	UParticleModuleVelocity implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleVelocity);

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SpawnEx(Owner, Offset, SpawnTime, NULL);
}

/**
 *	Extended version of spawn, allows for using a random stream for distribution value retrieval
 *
 *	@param	Owner				The particle emitter instance that is spawning
 *	@param	Offset				The offset to the modules payload data
 *	@param	SpawnTime			The time of the spawn
 *	@param	InRandomStream		The random stream to use for retrieving random values
 */
void UParticleModuleVelocity::SpawnEx(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, class FRandomStream* InRandomStream)
{
	SPAWN_INIT;
	{
		FVector FromOrigin;
		FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);

		FVector OwnerScale(1.0f);
		if ((bApplyOwnerScale == TRUE) && Owner && Owner->Component)
		{
			OwnerScale = Owner->Component->Scale * Owner->Component->Scale3D;
			AActor* Actor = Owner->Component->GetOwner();
			if (Actor && !Owner->Component->AbsoluteScale)
			{
				OwnerScale *= Actor->DrawScale * Actor->DrawScale3D;
			}
		}

		UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
		check(LODLevel);
		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			FromOrigin = Particle.Location.SafeNormal();
			if (bInWorldSpace == TRUE)
			{
				Vel = Owner->Component->LocalToWorld.InverseTransformNormal(Vel);
			}
		}
		else
		{
			FromOrigin = (Particle.Location - Owner->Location).SafeNormal();
			if (bInWorldSpace == FALSE)
			{
				Vel = Owner->Component->LocalToWorld.TransformNormal(Vel);
			}
		}
		Vel *= OwnerScale;
		Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream) * OwnerScale;
		Particle.Velocity		+= Vel;
		Particle.BaseVelocity	+= Vel;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocityInheritParent implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleVelocity_Seeded);

/**
 *	Called on a particle when it is spawned.
 *
 *	@param	Owner			The emitter instance that spawned the particle
 *	@param	Offset			The payload data offset for this module
 *	@param	SpawnTime		The spawn time of the particle
 */
void UParticleModuleVelocity_Seeded::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	FParticleRandomSeedInstancePayload* Payload = (FParticleRandomSeedInstancePayload*)(Owner->GetModuleInstanceData(this));
	SpawnEx(Owner, Offset, SpawnTime, (Payload != NULL) ? &(Payload->RandomStream) : NULL);
}

/**
 *	Returns the number of bytes the module requires in the emitters 'per-instance' data block.
 *	
 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
 *
 *	@return UINT		The number of bytes the module needs per emitter instance.
 */
UINT UParticleModuleVelocity_Seeded::RequiredBytesPerInstance(FParticleEmitterInstance* Owner)
{
	return RandomSeedInfo.GetInstancePayloadSize();
}

/**
 *	Allows the module to prep its 'per-instance' data block.
 *	
 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
 *	@param	InstData	Pointer to the data block for this module.
 */
UINT UParticleModuleVelocity_Seeded::PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData)
{
	return PrepRandomSeedInstancePayload(Owner, (FParticleRandomSeedInstancePayload*)InstData, RandomSeedInfo);
}

/** 
 *	Called when an emitter instance is looping...
 *
 *	@param	Owner	The emitter instance that owns this module
 */
void UParticleModuleVelocity_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner)
{
	if (RandomSeedInfo.bResetSeedOnEmitterLooping == TRUE)
	{
		FParticleRandomSeedInstancePayload* Payload = (FParticleRandomSeedInstancePayload*)(Owner->GetModuleInstanceData(this));
		PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocityInheritParent implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleVelocityInheritParent);

void UParticleModuleVelocityInheritParent::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;

	FVector Vel = FVector(0.0f);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Vel = Owner->Component->LocalToWorld.InverseTransformNormal(Owner->Component->PartSysVelocity);
	}
	else
	{
		Vel = Owner->Component->PartSysVelocity;
	}

	FVector vScale = Scale.GetValue(Owner->EmitterTime, Owner->Component);

	Vel *= vScale;

	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocityOverLifetime implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleVelocityOverLifetime);

void UParticleModuleVelocityOverLifetime::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	if (Absolute)
	{
		SPAWN_INIT;
		FVector OwnerScale(1.0f);
		if ((bApplyOwnerScale == TRUE) && Owner && Owner->Component)
		{
			OwnerScale = Owner->Component->Scale * Owner->Component->Scale3D;
			AActor* Actor = Owner->Component->GetOwner();
			if (Actor && !Owner->Component->AbsoluteScale)
			{
				OwnerScale *= Actor->DrawScale * Actor->DrawScale3D;
			}
		}
		FVector Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
		Particle.Velocity		= Vel;
		Particle.BaseVelocity	= Vel;
	}
}

void UParticleModuleVelocityOverLifetime::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	FVector OwnerScale(1.0f);
	if ((bApplyOwnerScale == TRUE) && Owner && Owner->Component)
	{
		OwnerScale = Owner->Component->Scale * Owner->Component->Scale3D;
		AActor* Actor = Owner->Component->GetOwner();
		if (Actor && !Owner->Component->AbsoluteScale)
		{
			OwnerScale *= Actor->DrawScale * Actor->DrawScale3D;
		}
	}
	if (Absolute)
	{
		if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
		{
			if (bInWorldSpace == FALSE)
			{
				FVector Vel;
				BEGIN_UPDATE_LOOP;
				{
					Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
					Particle.Velocity = Owner->Component->LocalToWorld.TransformNormal(Vel) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
			else
			{
				BEGIN_UPDATE_LOOP;
				{
					Particle.Velocity = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
		}
		else
		{
			if (bInWorldSpace == FALSE)
			{
				BEGIN_UPDATE_LOOP;
				{
					Particle.Velocity = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
			else
			{
				FVector Vel;
				FMatrix InvMat = Owner->Component->LocalToWorld.Inverse();
				BEGIN_UPDATE_LOOP;
				{
					Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
					Particle.Velocity = InvMat.TransformNormal(Vel) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
		}
	}
	else
	{
		if (LODLevel->RequiredModule->bUseLocalSpace == FALSE)
		{
			FVector Vel;
			if (bInWorldSpace == FALSE)
			{
				BEGIN_UPDATE_LOOP;
				{
					Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
					Particle.Velocity *= Owner->Component->LocalToWorld.TransformNormal(Vel) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
			else
			{
				BEGIN_UPDATE_LOOP;
				{
					Particle.Velocity *= VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
		}
		else
		{
			if (bInWorldSpace == FALSE)
			{
				BEGIN_UPDATE_LOOP;
				{
					Particle.Velocity *= VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
			else
			{
				FVector Vel;
				FMatrix InvMat = Owner->Component->LocalToWorld.Inverse();
				BEGIN_UPDATE_LOOP;
				{
					Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
					Particle.Velocity *= InvMat.TransformNormal(Vel) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
		}
	}
}
