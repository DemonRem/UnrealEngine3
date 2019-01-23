/*=============================================================================
	ParticleModules_Size.cpp: 
	Size-related particle module implementations.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UParticleModuleSizeBase);

/*-----------------------------------------------------------------------------
	UParticleModuleSize implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSize);

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
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
void UParticleModuleSize::SpawnEx(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, class FRandomStream* InRandomStream)
{
	SPAWN_INIT;
	FVector Size		 = StartSize.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
	Particle.Size		+= Size;
	Particle.BaseSize	+= Size;
}

/*-----------------------------------------------------------------------------
	UParticleModuleSize_Seeded implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSize_Seeded);

/**
 *	Called on a particle when it is spawned.
 *
 *	@param	Owner			The emitter instance that spawned the particle
 *	@param	Offset			The payload data offset for this module
 *	@param	SpawnTime		The spawn time of the particle
 */
void UParticleModuleSize_Seeded::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
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
UINT UParticleModuleSize_Seeded::RequiredBytesPerInstance(FParticleEmitterInstance* Owner)
{
	return RandomSeedInfo.GetInstancePayloadSize();
}

/**
 *	Allows the module to prep its 'per-instance' data block.
 *	
 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
 *	@param	InstData	Pointer to the data block for this module.
 */
UINT UParticleModuleSize_Seeded::PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData)
{
	return PrepRandomSeedInstancePayload(Owner, (FParticleRandomSeedInstancePayload*)InstData, RandomSeedInfo);
}

/** 
 *	Called when an emitter instance is looping...
 *
 *	@param	Owner	The emitter instance that owns this module
 */
void UParticleModuleSize_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner)
{
	if (RandomSeedInfo.bResetSeedOnEmitterLooping == TRUE)
	{
		FParticleRandomSeedInstancePayload* Payload = (FParticleRandomSeedInstancePayload*)(Owner->GetModuleInstanceData(this));
		PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleSizeMultiplyVelocity implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSizeMultiplyVelocity);

void UParticleModuleSizeMultiplyVelocity::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FVector SizeScale = VelocityMultiplier.GetValue(Particle.RelativeTime, Owner->Component) * Particle.Velocity.Size();
	if(MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if(MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if(MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}
}

void UParticleModuleSizeMultiplyVelocity::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
	{
		FVector SizeScale = VelocityMultiplier.GetValue(Particle.RelativeTime, Owner->Component) * Particle.Velocity.Size();
		if(MultiplyX)
		{
			Particle.Size.X *= SizeScale.X;
		}
		if(MultiplyY)
		{
			Particle.Size.Y *= SizeScale.Y;
		}
		if(MultiplyZ)
		{
			Particle.Size.Z *= SizeScale.Z;
		}
	}
	END_UPDATE_LOOP;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleSizeMultiplyVelocity::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionVectorConstant* VelocityMultiplierDist = Cast<UDistributionVectorConstant>(VelocityMultiplier.Distribution);
	if (VelocityMultiplierDist)
	{
		VelocityMultiplierDist->Constant = FVector(1.0f,1.0f,1.0f);
		VelocityMultiplierDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleSizeMultiplyLife implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSizeMultiplyLife);

void UParticleModuleSizeMultiplyLife::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
	if(MultiplyX)
	{
		Particle.Size.X *= SizeScale.X;
	}
	if(MultiplyY)
	{
		Particle.Size.Y *= SizeScale.Y;
	}
	if(MultiplyZ)
	{
		Particle.Size.Z *= SizeScale.Z;
	}
}

void UParticleModuleSizeMultiplyLife::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	const FRawDistribution* FastDistribution = LifeMultiplier.GetFastRawDistribution();
	if( FastDistribution && MultiplyX && MultiplyY && MultiplyZ)
	{
#if 1
		FVector SizeScale;
		// fast path
		BEGIN_UPDATE_LOOP;
			FastDistribution->GetValue3None(Particle.RelativeTime, &SizeScale.X);
			Particle.Size *= SizeScale;
		END_UPDATE_LOOP;
#else
		//@NOTE.SAS. 08/8/9
		// This reduces LHS in my test case from ~999 to < 100, but doesn't show any significant 
		// performance gain from it to warrant making the change at this point.
		FVector SizeScale(1.0f);
		FBaseParticle* PrevParticle = (FBaseParticle*)((BYTE*)&(Owner->ParticleData) + Owner->ParticleStride * Owner->ParticleIndices[0]);
		// fast path
		BEGIN_UPDATE_LOOP;
			PrevParticle->Size.X *= SizeScale.X;
			PrevParticle->Size.Y *= SizeScale.Y;
			PrevParticle->Size.Z *= SizeScale.Z;
			FastDistribution->GetValue3None(Particle.RelativeTime, &SizeScale.X);
			PrevParticle = &Particle;
		END_UPDATE_LOOP;
#endif
	}
	else
	{
		BEGIN_UPDATE_LOOP;
			FVector SizeScale = LifeMultiplier.GetValue(Particle.RelativeTime, Owner->Component);
			if(MultiplyX)
			{
				Particle.Size.X *= SizeScale.X;
			}
			if(MultiplyY)
			{
				Particle.Size.Y *= SizeScale.Y;
			}
			if(MultiplyZ)
			{
				Particle.Size.Z *= SizeScale.Z;
			}
		END_UPDATE_LOOP;
	}
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleSizeMultiplyLife::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	LifeMultiplier.Distribution = Cast<UDistributionVectorConstantCurve>(StaticConstructObject(UDistributionVectorConstantCurve::StaticClass(), this));
	UDistributionVectorConstantCurve* LifeMultiplierDist = Cast<UDistributionVectorConstantCurve>(LifeMultiplier.Distribution);
	if (LifeMultiplierDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (INT Key = 0; Key < 2; Key++)
		{
			INT	KeyIndex = LifeMultiplierDist->CreateNewKey(Key * 1.0f);
			for (INT SubIndex = 0; SubIndex < 3; SubIndex++)
			{
				LifeMultiplierDist->SetKeyOut(SubIndex, KeyIndex, 1.0f);
			}
		}
		LifeMultiplierDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleSizeScale implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSizeScale);

void UParticleModuleSizeScale::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
		FVector ScaleFactor = SizeScale.GetValue(Particle.RelativeTime, Owner->Component);
		Particle.Size = Particle.BaseSize * ScaleFactor;
	END_UPDATE_LOOP;
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleSizeScale::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionVectorConstant* SizeScaleDist = Cast<UDistributionVectorConstant>(SizeScale.Distribution);
	if (SizeScaleDist)
	{
		SizeScaleDist->Constant = FVector(1.0f,1.0f,1.0f);
		SizeScaleDist->bIsDirty = TRUE;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleSizeScaleByTime implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleSizeScaleByTime);

/**
 *	Called during the spawning of a particle.
 *	
 *	@param	Owner		The emitter instance that owns the particle.
 *	@param	Offset		The offset into the particle payload for this module.
 *	@param	SpawnTime	The spawn time of the particle.
 */
void UParticleModuleSizeScaleByTime::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SPAWN_INIT;
	{
		PARTICLE_ELEMENT(FScaleSizeByLifePayload, Payload);
		Payload.AbsoluteTime = SpawnTime;
	}
}

/**
 *	Called during the spawning of particles in the emitter instance.
 *	
 *	@param	Owner		The emitter instance that owns the particle.
 *	@param	Offset		The offset into the particle payload for this module.
 *	@param	DeltaTime	The time slice for this update.
 */
void UParticleModuleSizeScaleByTime::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	BEGIN_UPDATE_LOOP;
	{
		PARTICLE_ELEMENT(FScaleSizeByLifePayload, Payload);
		Payload.AbsoluteTime += DeltaTime;
		FVector ScaleFactor = SizeScaleByTime.GetValue(Payload.AbsoluteTime, Owner->Component);
		Particle.Size.X = Particle.Size.X * (bEnableX ? ScaleFactor.X : 1.0f);
		Particle.Size.Y = Particle.Size.Y * (bEnableY ? ScaleFactor.Y : 1.0f);
		Particle.Size.Z = Particle.Size.Z * (bEnableZ ? ScaleFactor.Z : 1.0f);
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
UINT UParticleModuleSizeScaleByTime::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FScaleSizeByLifePayload);
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleSizeScaleByTime::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionVectorConstantCurve* SizeScaleByTimeDist = Cast<UDistributionVectorConstantCurve>(SizeScaleByTime.Distribution);
	if (SizeScaleByTimeDist)
	{
		// Add two points, one at time 0.0f and one at 1.0f
		for (INT Key = 0; Key < 2; Key++)
		{
			INT	KeyIndex = SizeScaleByTimeDist->CreateNewKey(Key * 1.0f);
			for (INT SubIndex = 0; SubIndex < 3; SubIndex++)
			{
				SizeScaleByTimeDist->SetKeyOut(SubIndex, KeyIndex, 1.0f);
			}
		}
		SizeScaleByTimeDist->bIsDirty = TRUE;
	}
}

