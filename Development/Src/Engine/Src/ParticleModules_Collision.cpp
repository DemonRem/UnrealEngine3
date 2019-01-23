/*=============================================================================
	ParticleModules_Collision.cpp: 
	Collision-related particle module implementations.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EnginePhysicsClasses.h"
#include "UnNovodexSupport.h"

/*-----------------------------------------------------------------------------
	FParticlesStatGroup
-----------------------------------------------------------------------------*/
DECLARE_CYCLE_STAT(TEXT("Particle Collision Time"),STAT_ParticleCollisionTime,STATGROUP_Particles);

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleCollisionBase);

/*-----------------------------------------------------------------------------
	UParticleModuleCollision implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleCollision);

UINT UParticleModuleCollision::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FParticleCollisionPayload);
}

void UParticleModuleCollision::Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime)
{
	SCOPE_CYCLE_COUNTER(STAT_ParticleCollisionTime);
	SPAWN_INIT;
	{
		PARTICLE_ELEMENT(FParticleCollisionPayload, CollisionPayload);
		CollisionPayload.UsedDampingFactor = DampingFactor.GetValue(Owner->EmitterTime, Owner->Component);
		CollisionPayload.UsedDampingFactorRotation = DampingFactorRotation.GetValue(Owner->EmitterTime, Owner->Component);
		CollisionPayload.UsedCollisions = appTrunc(MaxCollisions.GetValue(Owner->EmitterTime, Owner->Component));
		CollisionPayload.Delay = DelayAmount.GetValue(Owner->EmitterTime, Owner->Component);
		if (CollisionPayload.Delay > SpawnTime)
		{
			Particle.Flags |= STATE_Particle_DelayCollisions;
		}
	}
}

void UParticleModuleCollision::Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_ParticleCollisionTime);
	AActor* Actor = Owner->Component->GetOwner();
	if (!Actor && GIsGame)
	{
		return;
	}

	UBOOL bMeshEmitter = Owner->Type()->IsA(FParticleMeshEmitterInstance::StaticType);
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);

	FParticleMeshEmitterInstance* pkMeshInst = bMeshEmitter ? CastEmitterInstance<FParticleMeshEmitterInstance>(Owner) : NULL;

	FVector ParentScale = FVector(1.0f, 1.0f, 1.0f);
	if (Owner->Component)
	{
		ParentScale *= Owner->Component->Scale * Owner->Component->Scale3D;
		if (Actor && !Owner->Component->AbsoluteScale)
		{
			ParentScale *= Actor->DrawScale * Actor->DrawScale3D;
		}
	}

	BEGIN_UPDATE_LOOP;
	{
		if ((Particle.Flags & STATE_Particle_IgnoreCollisions) != 0)
		{
			CONTINUE_UPDATE_LOOP;
		}

		PARTICLE_ELEMENT(FParticleCollisionPayload, CollisionPayload);
		if ((Particle.Flags & STATE_Particle_DelayCollisions) != 0)
		{
			if (CollisionPayload.Delay > Particle.RelativeTime)
			{
				CONTINUE_UPDATE_LOOP;
			}
			Particle.Flags &= ~STATE_Particle_DelayCollisions;
		}

		FVector			Location;
		FVector			OldLocation;

		// Location won't be calculated till after tick so we need to calculate an intermediate one here.
		Location	= Particle.Location + Particle.Velocity * DeltaTime;
		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			// Transform the location and old location into world space
			Location		= Owner->Component->LocalToWorld.TransformFVector(Location);
			OldLocation		= Owner->Component->LocalToWorld.TransformFVector(Particle.OldLocation);
		}
		else
		{
			OldLocation	= Particle.OldLocation;
		}
		FVector	Direction = (Location - OldLocation).SafeNormal();

        // Determine the size
		FVector Size = Particle.Size.Size() * ParentScale;

		FVector	Extent(0.0f);

		if (pkMeshInst)
		{
			FMatrix kMat(FMatrix::Identity);
			FTranslationMatrix kTransMat(Location);
			FScaleMatrix kScaleMat(FVector(Particle.Size.X, Particle.Size.Y, Particle.Size.Z));
			FRotator kRotator;

            if (pkMeshInst->MeshRotationActive)
            {
				FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + pkMeshInst->MeshRotationOffset);
				kRotator = FRotator::MakeFromEuler(PayloadData->Rotation);
			}
			else
			{
				FLOAT fRot = Particle.Rotation * 180.0f / PI;
				FVector kRotVec = FVector(fRot, fRot, fRot);
				kRotator = FRotator::MakeFromEuler(kRotVec);
			}
			
			FRotationMatrix kRotMat(kRotator);

			kMat = kRotMat * kScaleMat * kTransMat;
			if (LODLevel->RequiredModule->bUseLocalSpace)
			{
				kMat = kMat * pkMeshInst->Component->LocalToWorld;
			}

		}

		FCheckResult	Hit;

		Hit.Normal.X = 0.0f;
		Hit.Normal.Y = 0.0f;
		Hit.Normal.Z = 0.0f;

		if (!Owner->Component->SingleLineCheck(Hit, Actor, Location + Direction * Size / DirScalar, OldLocation, TRACE_ProjTargets | TRACE_AllBlocking, Extent))
		{
			UBOOL bDecrementMaxCount = TRUE;
			if (bPawnsDoNotDecrementCount == TRUE)
			{
				if (Hit.Actor)
				{
					if (Hit.Actor->IsA(APawn::StaticClass()))
					{
						//debugf(TEXT("Particle from %s Collided with a PAWN!"), *(Owner->Component->Template->GetPathName()));
						bDecrementMaxCount = FALSE;
					}
				}
			}

			if (bDecrementMaxCount && (bOnlyVerticalNormalsDecrementCount == TRUE))
			{
				if ((Hit.Normal.IsNearlyZero() == FALSE) && (Abs(Hit.Normal.Z) + VerticalFudgeFactor) < 1.0f)
				{
					//debugf(TEXT("Particle from %s had a non-vertical hit!"), *(Owner->Component->Template->GetPathName()));
					bDecrementMaxCount = FALSE;
				}
			}

			if (CollisionPayload.UsedCollisions > 0)
			{
				if (LODLevel->RequiredModule->bUseLocalSpace)
				{
					// Transform the particle velocity to world space
					FVector OldVelocity		= Owner->Component->LocalToWorld.TransformNormal(Particle.Velocity);
					FVector	BaseVelocity	= Owner->Component->LocalToWorld.TransformNormal(Particle.BaseVelocity);
					BaseVelocity			= BaseVelocity.MirrorByVector(Hit.Normal) * CollisionPayload.UsedDampingFactor;

					Particle.BaseVelocity		= Owner->Component->LocalToWorld.Inverse().TransformNormal(BaseVelocity);
					Particle.BaseRotationRate	= Particle.BaseRotationRate * CollisionPayload.UsedDampingFactorRotation.X;
					if (pkMeshInst && pkMeshInst->MeshRotationActive)
					{
						FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + pkMeshInst->MeshRotationOffset);
						PayloadData->RotationRateBase *= CollisionPayload.UsedDampingFactorRotation;
					}

					// Reset the current velocity and manually adjust location to bounce off based on normal and time of collision.
					FVector NewVelocity	= Direction.MirrorByVector(Hit.Normal) * (Location - OldLocation).Size() * CollisionPayload.UsedDampingFactor;
					Particle.Velocity		= FVector(0.f);

					// New location
					FVector	NewLocation		= Location + NewVelocity * (1.f - Hit.Time);
                    Particle.Location		= Owner->Component->LocalToWorld.Inverse().TransformFVector(NewLocation);

					if (bApplyPhysics && Hit.Component)
					{
						FVector vImpulse;
						vImpulse = -(NewVelocity - OldVelocity) * ParticleMass.GetValue(Particle.RelativeTime, Owner->Component);
						Hit.Component->AddImpulse(vImpulse, Hit.Location, Hit.BoneName);
					}
				}
				else
				{
					FVector vOldVelocity = Particle.Velocity;

					// Reflect base velocity and apply damping factor.
					Particle.BaseVelocity		= Particle.BaseVelocity.MirrorByVector(Hit.Normal) * CollisionPayload.UsedDampingFactor;
					Particle.BaseRotationRate	= Particle.BaseRotationRate * CollisionPayload.UsedDampingFactorRotation.X;
					if (pkMeshInst && pkMeshInst->MeshRotationActive)
					{
						FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + pkMeshInst->MeshRotationOffset);
						PayloadData->RotationRateBase *= CollisionPayload.UsedDampingFactorRotation;
					}

					// Reset the current velocity and manually adjust location to bounce off based on normal and time of collision.
					FVector vNewVelocity	= Direction.MirrorByVector(Hit.Normal) * (Location - OldLocation).Size() * CollisionPayload.UsedDampingFactor;
					Particle.Velocity		= FVector(0.f);
					Particle.Location	   += vNewVelocity * (1.f - Hit.Time);

					if (bApplyPhysics && Hit.Component)
					{
						FVector vImpulse;
						vImpulse = -(vNewVelocity - vOldVelocity) * ParticleMass.GetValue(Particle.RelativeTime, Owner->Component);
						Hit.Component->AddImpulse(vImpulse, Hit.Location, Hit.BoneName);
					}
				}

				if (bDecrementMaxCount)
				{
					CollisionPayload.UsedCollisions--;
				}
			}
			else
			{
				switch (CollisionCompletionOption)
				{
				case EPCC_Kill:
					{
						KILL_CURRENT_PARTICLE;
					}
					break;
				case EPCC_Freeze:
					{
						Particle.Flags |= STATE_Particle_Freeze;
					}
					break;
				case EPCC_HaltCollisions:
					{
						Particle.Flags |= STATE_Particle_IgnoreCollisions;
					}
					break;
				case EPCC_FreezeTranslation:
					{
						Particle.Flags |= STATE_Particle_FreezeTranslation;
					}
					break;
				case EPCC_FreezeRotation:
					{
						Particle.Flags |= STATE_Particle_FreezeRotation;
					}
					break;
				case EPCC_FreezeMovement:
					{
						Particle.Flags |= STATE_Particle_FreezeRotation;
						Particle.Flags |= STATE_Particle_FreezeTranslation;
					}
					break;
				}
			}
		}
	}
	END_UPDATE_LOOP;
}

void UParticleModuleCollision::SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	SCOPE_CYCLE_COUNTER(STAT_ParticleCollisionTime);
	UParticleLODLevel*			LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	UParticleModuleCollision*	LowModule	= Cast<UParticleModuleCollision>(LowerLODModule);

	SPAWN_INIT;
	{
		PARTICLE_ELEMENT(FParticleCollisionPayload, CollisionPayload);

		FVector	UsedDampingFactorA			= DampingFactor.GetValue(Owner->EmitterTime, Owner->Component);
		FVector	UsedDampingFactorB			= LowModule->DampingFactor.GetValue(Owner->EmitterTime, Owner->Component);

		FVector	UsedDampingFactorRotationA	= DampingFactorRotation.GetValue(Owner->EmitterTime, Owner->Component);
		FVector	UsedDampingFactorRotationB	= LowModule->DampingFactorRotation.GetValue(Owner->EmitterTime, Owner->Component);

		FLOAT	MaxCollisionsA				= MaxCollisions.GetValue(Owner->EmitterTime, Owner->Component);
		FLOAT	MaxCollisionsB				= LowModule->MaxCollisions.GetValue(Owner->EmitterTime, Owner->Component);

		FLOAT	DelayA						= DelayAmount.GetValue(Owner->EmitterTime, Owner->Component);
		FLOAT	DelayB						= LowModule->DelayAmount.GetValue(Owner->EmitterTime, Owner->Component);

		CollisionPayload.UsedDampingFactor			= (UsedDampingFactorA * Multiplier) + (UsedDampingFactorB * (1.0f - Multiplier));
		CollisionPayload.UsedDampingFactorRotation	= (UsedDampingFactorRotationA * Multiplier) + (UsedDampingFactorRotationB * (1.0f - Multiplier));
		CollisionPayload.UsedCollisions				= appTrunc((MaxCollisionsA * Multiplier) + (MaxCollisionsB * (1.0f - Multiplier)));
		CollisionPayload.Delay						= (DelayA * Multiplier) + (DelayB * (1.0f - Multiplier));
		if (CollisionPayload.Delay > SpawnTime)
		{
			Particle.Flags |= STATE_Particle_DelayCollisions;
		}
	}
}

void UParticleModuleCollision::UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier)
{
	Update(Owner, Offset, DeltaTime);
}

/**
 *	Called when the module is created, this function allows for setting values that make
 *	sense for the type of emitter they are being used in.
 *
 *	@param	Owner			The UParticleEmitter that the module is being added to.
 */
void UParticleModuleCollision::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	UDistributionFloatUniform* MaxCollDist = Cast<UDistributionFloatUniform>(MaxCollisions.Distribution);
	if (MaxCollDist)
	{
		MaxCollDist->Min = 1.0f;
		MaxCollDist->Max = 1.0f;
		MaxCollDist->bIsDirty = TRUE;
	}
}

UBOOL UParticleModuleCollision::GenerateLODModuleValues(UParticleModule* SourceModule, FLOAT Percentage, UParticleLODLevel* LODLevel)
{
	// disable collision on emitters at the lowest LOD level
	if (LODLevel->LevelSetting == 100)
	{
		bEnabled = FALSE;
	}
	return TRUE;
}
