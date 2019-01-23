/*=============================================================================
	ParticleNxFluidEmitterInstance.cpp: NxFluid emitter instance implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"
#include "LevelUtils.h"
#include "UnNovodexSupport.h"

IMPLEMENT_PARTICLEEMITTERINSTANCE_TYPE(FParticleMeshNxFluidEmitterInstance);
IMPLEMENT_PARTICLEEMITTERINSTANCE_TYPE(FParticleSpriteNxFluidEmitterInstance);

/*-----------------------------------------------------------------------------
	FPhysXEmitterInstance
-----------------------------------------------------------------------------*/

FPhysXEmitterInstance::FPhysXEmitterInstance(struct FPhysXFluidTypeData& Fluid, FParticleEmitterInstance& EmitterInstance) :
PhysXFluid(Fluid),
ParticleEmitterInstance(EmitterInstance)
{
	FluidEmitter        = 0;
	FluidEmitterActive  = FALSE;
	FluidNeedsToDisable = FALSE;
	FluidNeedsInit      = FALSE;
}

FPhysXEmitterInstance::~FPhysXEmitterInstance()
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	if(FluidEmitter)
	{
		PhysXFluid.ReleaseEmitter(*FluidEmitter);
		FluidEmitter = 0;
	}
	if(GWorld && GWorld->RBPhysScene)
	{
		GWorld->RBPhysScene->PhysicalEmitters.RemoveItem(this);
	}
#endif
}

void FPhysXEmitterInstance::Tick(FParticleEmitterInstance & EmitterInstance, FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)

	// queue up the emitter to be physical. will be initialized after the next fetchResults.
	if(!FluidEmitter && GWorld && GWorld->RBPhysScene && GIsGame)
	{
		FRBPhysScene *Scene = GWorld->RBPhysScene;
		if(!Scene->PhysicalEmitters.ContainsItem(this))
		{
			Scene->PhysicalEmitters.Add(1);
			INT N = Scene->PhysicalEmitters.Num();
			Scene->PhysicalEmitters(N - 1) = this;
		}
		FluidEmitterActive = TRUE;
	}
	else if(!FluidEmitter && (!GWorld || !GWorld->RBPhysScene) && !GIsGame)
	{
		FluidEmitter = PhysXFluid.CreateEmitter(*this);
		if(FluidEmitter)
		{
			FluidEmitterActive = TRUE;
			FluidNeedsInit     = TRUE;
			// make sure the actor doesn't die before the particles.
			// this seems like a hack so would be nice to figure out the
			// "correct" way to do this...
			AActor *Owner = 0;
			if(EmitterInstance.Component)
			{
				Owner = EmitterInstance.Component->GetOwner();
			}
			if(Owner && Owner->LifeSpan != 0.0f)
			{
				Owner->LifeSpan = ::Max(Owner->LifeSpan, EmitterInstance.EmitterDuration + PhysXFluid.FluidEmitterParticleLifetime);
			}
			// if the emitter doesn't actually emit particles and we have a "base" object
			// then fill the objects volume with particles.
			AActor *Base = Owner ? Owner->Base : 0;
			UBOOL bUseFillVolume = (PhysXFluid.FluidEmitterType == FET_FILL_OWNER_VOLUME) ? TRUE : FALSE;
			if(	Base && bUseFillVolume)
			{
				PhysXFluid.FillVolume(*Base);
			}
		}
	}
	// update the emitter...
	if(FluidEmitter)
	{
		if(EmitterInstance.EmitterDuration > 0.0f && EmitterInstance.SecondsSinceCreation > EmitterInstance.EmitterDuration)
		{
			FluidNeedsToDisable = TRUE;
		}
		else
		{
			UBOOL bCurrEnabled = FluidEmitter->getFlag(NX_FEF_ENABLED) ? TRUE : FALSE;
			if(bSuppressSpawning && bCurrEnabled)
			{
				FluidNeedsToDisable = TRUE;
				FluidNeedsInit      = FALSE;
			}
			else if(!bSuppressSpawning && !bCurrEnabled)
			{
				FluidNeedsToDisable = FALSE;
				FluidNeedsInit      = TRUE;
			}
		}
	}
#endif //WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
}

void FPhysXEmitterInstance::UpdateBoundingBox(FParticleEmitterInstance & EmitterInstance, FLOAT DeltaTime)
{
	if(FluidEmitter)
	{
	#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
		NxBounds3 nxbounds;
		NxFluid &fluid = FluidEmitter->getFluid();
		fluid.getWorldBounds(nxbounds);
		EmitterInstance.ParticleBoundingBox.Init();
		EmitterInstance.ParticleBoundingBox.Min = N2UPosition(nxbounds.min);
		EmitterInstance.ParticleBoundingBox.Max = N2UPosition(nxbounds.max);
		EmitterInstance.ParticleBoundingBox = EmitterInstance.ParticleBoundingBox.ExpandBy(1.0f);
		EmitterInstance.ParticleBoundingBox.IsValid = 1;
	#endif
	}
}

void FPhysXEmitterInstance::RemovedFromScene(void)
{
	if(FluidEmitter)
	{
		FluidEmitterActive  = FALSE;
		FluidNeedsInit      = FALSE;
		FluidNeedsToDisable = FALSE;
		
		// if we are in cascade, remove immediately...
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
		if(GIsEditor && !GIsGame)
		{
			if(GWorld && GWorld->RBPhysScene)
			{
				GWorld->RBPhysScene->PhysicalEmitters.RemoveItem(this);
			}
			PhysXFluid.ReleaseEmitter(*FluidEmitter);
			FluidEmitter = 0;
		}
#endif
	}
}

#ifndef NX_DISABLE_FLUIDS
static QSORT_RETURN
ComparePacketSizes( const NxFluidPacket* A, const NxFluidPacket* B )
{ 
	return A->numParticles>B->numParticles ? 1 : A->numParticles<B->numParticles? -1 : 0; 
}
#endif // NX_DISABLE_FLUIDS

static void
IncrementPacketCulling( FPhysXFluidTypeData & PhysXFluid )
{
#ifndef NX_DISABLE_FLUIDS
	if( !PhysXFluid.NovodexFluid || PhysXFluid.FluidPacketMaxCount <= 0 )
	{	// No fluid or culling turned off
		return;
	}

	NxFluidPacketData FluidPacketData = PhysXFluid.NovodexFluid->getFluidPacketData();

	INT NumParticlePackets = FluidPacketData.numFluidPacketsPtr ? *FluidPacketData.numFluidPacketsPtr : 0;

	if( NumParticlePackets <= PhysXFluid.FluidPacketMaxCount || !FluidPacketData.bufferFluidPackets )
	{	// No need to cull or packet data invalid
		return;
	}

	appQsort( FluidPacketData.bufferFluidPackets, NumParticlePackets, sizeof(NxFluidPacket), (QSORT_COMPARE)ComparePacketSizes );

	// Cull Min. size packets
	NxParticleUpdateData UpdateData;
	UpdateData.bufferFlagByteStride = sizeof( NxU32 );
	UpdateData.bufferFlag = (NxU32*)appAlloca( sizeof( NxU32 )* PhysXFluid.FluidNumParticles );
	appMemzero( UpdateData.bufferFlag, PhysXFluid.FluidNumParticles*sizeof( NxU32 ) );

	for(INT i = 0; i < NumParticlePackets-PhysXFluid.FluidPacketMaxCount; ++i )
	{
		NxFluidPacket& FluidPacket = FluidPacketData.bufferFluidPackets[i];
		NxU32 * Flag = UpdateData.bufferFlag + FluidPacket.firstParticleIndex;
		for( UINT j = 0; j < FluidPacket.numParticles; ++j )
		{
			*Flag++ = (NxU32)NX_FP_DELETE;
		}
	}

	PhysXFluid.NovodexFluid->updateParticles( UpdateData );
#endif // NX_DISABLE_FLUIDS
}

void FPhysXEmitterInstance::SyncPhysicsData(FParticleEmitterInstance & EmitterInstance, FPhysXFluidTypeData * LODLevelPhysXFluid)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	// initialize on the first tick if needed...
	if(!FluidEmitter && FluidEmitterActive)
	{
		FluidEmitter = PhysXFluid.CreateEmitter(*this);
		if(FluidEmitter)
		{
			FluidNeedsInit      = TRUE;
			FluidNeedsToDisable = FALSE;
			// make sure the actor doesn't die before the particles.
			// this seems like a hack so would be nice to figure out the
			// "correct" way to do this...
			AActor *Owner = 0;
			if(EmitterInstance.Component)
			{
				Owner = EmitterInstance.Component->GetOwner();
			}
			if(Owner && Owner->LifeSpan != 0.0f)
			{
				Owner->LifeSpan = ::Max(Owner->LifeSpan, EmitterInstance.EmitterDuration + PhysXFluid.FluidEmitterParticleLifetime);
			}
			// if the emitter doesn't actually emit particles and we have a "base" object
			// then fill the objects volume with particles.
			AActor *Base = Owner ? Owner->Base : 0;
			UBOOL bUseFillVolume = (PhysXFluid.FluidEmitterType == FET_FILL_OWNER_VOLUME) ? TRUE : FALSE;
			if(	Base && bUseFillVolume)
			{
				PhysXFluid.FillVolume(*Base);
			}
		}
	}
	// finish initialization...
	if(FluidEmitter)
	{
	#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
		if(FluidNeedsToDisable)
		{
			FluidEmitter->setFlag(NX_FEF_ENABLED, false);
			FluidNeedsToDisable = FALSE;
		}
		if(FluidEmitterActive && FluidNeedsInit)
		{
			FluidEmitter->setFlag(NX_FEF_ENABLED, true);
			FluidNeedsInit      = FALSE;
			FluidNeedsToDisable = FALSE;
		}
		else if(!FluidEmitterActive)
		{
			if(GWorld && GWorld->RBPhysScene)
			{
				GWorld->RBPhysScene->PhysicalEmitters.RemoveItem(this);
			}
			PhysXFluid.ReleaseEmitter(*FluidEmitter);
			FluidEmitter = 0;
		}
	#endif
	}
	if(FluidEmitter)
	{

		if(!FluidEmitter->getFrameShape())
		{
			const FMatrix & UM = EmitterInstance.Component->LocalToWorld;
			// mat[] permutes the x, y and z axes of UM
			NxReal mat[] =
			{
				UM.M[1][0],UM.M[1][1],UM.M[1][2],0,
				UM.M[2][0],UM.M[2][1],UM.M[2][2],0,
				UM.M[0][0],UM.M[0][1],UM.M[0][2],0,
				UM.M[3][0]*U2PScale,UM.M[3][1]*U2PScale,UM.M[3][2]*U2PScale,1
			};
			NxMat34 pose;
			pose.setColumnMajor44(mat);
			FluidEmitter->setGlobalPose(pose);
		}

		if( PhysXFluid.PrimaryEmitter == FluidEmitter )
		{
			IncrementPacketCulling( PhysXFluid );
		}
	}
	else if(GWorld && GWorld->RBPhysScene)
	{
		GWorld->RBPhysScene->PhysicalEmitters.RemoveItem(this);
	}
	// Cascade LOD support.
	if(FluidEmitter)
	{
		check(LODLevelPhysXFluid);
		if(LODLevelPhysXFluid)
		{
			// Random Position.
			NxVec3 CurrRandPos = FluidEmitter->getRandomPos();
			NxVec3 RandPos     = U2NPosition(LODLevelPhysXFluid->FluidEmitterRandomPos);
			if(CurrRandPos != RandPos)
			{
				FluidEmitter->setRandomPos(RandPos);
			}
			// Random Angle.
			NxReal CurrRandAng = FluidEmitter->getRandomAngle();
			NxReal RandAng     = (NxReal)::Max(LODLevelPhysXFluid->FluidEmitterRandomAngle, 0.0f);
			if(CurrRandAng != RandAng)
			{
				FluidEmitter->setRandomAngle(RandAng);
			}
			// Fluid Velocity Magnitude.
			NxReal CurrVelMag = FluidEmitter->getFluidVelocityMagnitude();
			NxReal VelMag     = U2PScale*(NxReal)::Max(LODLevelPhysXFluid->FluidEmitterFluidVelocityMagnitude, 0.0f);
			if(CurrVelMag != VelMag)
			{
				FluidEmitter->setFluidVelocityMagnitude(VelMag);
			}
			// Rate.
			UBOOL bIsFlow = PhysXFluid.FluidEmitterType == FET_CONSTANT_FLOW ? TRUE : FALSE;
			if(bIsFlow)
			{
				NxReal CurrRate = FluidEmitter->getRate();
				NxReal Rate     = (NxReal)::Max(LODLevelPhysXFluid->FluidEmitterRate, 0.0f);
				if(CurrRate != Rate)
				{
					FluidEmitter->setRate(Rate);
				}
			}
			// Particle Lifetime.
			NxReal CurrLifetime = FluidEmitter->getParticleLifetime();
			NxReal Lifetime     = (NxReal)::Max(LODLevelPhysXFluid->FluidEmitterParticleLifetime, 0.1f);
			if(CurrLifetime != Lifetime)
			{
				FluidEmitter->setParticleLifetime(Lifetime);
			}
			// Repulsion Coefficient.
			NxReal CurrRepCoef = FluidEmitter->getRepulsionCoefficient();
			NxReal RepCoef     = (NxReal)::Max(LODLevelPhysXFluid->FluidEmitterRepulsionCoefficient, 0.0f);
			if(CurrRepCoef != RepCoef)
			{
				FluidEmitter->setRepulsionCoefficient(RepCoef);
			}
		}
	}
#endif // WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
}

void FPhysXEmitterInstance::OnLostFluidEmitter(void)
{
	FluidEmitter = PhysXFluid.CreateEmitter(*this);
	if(FluidEmitter)
	{
		FluidEmitterActive  = TRUE;
		FluidNeedsInit      = TRUE;
		FluidNeedsToDisable = FALSE;
	}
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	if(!FluidEmitter && GWorld && GWorld->RBPhysScene)
	{
		GWorld->RBPhysScene->PhysicalEmitters.RemoveItem(this);
	}
#endif
}

void FPhysXEmitterInstance::AddForceField(FForceApplicator* Applicator, const FBox& FieldBoundingBox)
{
	if(FluidEmitter)
	{
		PhysXFluid.AddForceField(*FluidEmitter, Applicator, FieldBoundingBox);
	}
}

void FPhysXEmitterInstance::ApplyParticleForces()
{
	if(FluidEmitter)
	{
		PhysXFluid.ApplyParticleForces(*FluidEmitter);
	}
}

/*-----------------------------------------------------------------------------
	FParticleMeshNxFluidEmitterInstance
-----------------------------------------------------------------------------*/

FParticleMeshNxFluidEmitterInstance::FParticleMeshNxFluidEmitterInstance(class UParticleModuleTypeDataMeshNxFluid &TypeData) :
FParticleMeshEmitterInstance(),
FluidTypeData(TypeData),
PhysXEmitterInstance(TypeData.PhysXFluid,*this)
{
}

void FParticleMeshNxFluidEmitterInstance::Init(void)
{
	FParticleMeshEmitterInstance::Init();
	ActiveParticles = 0;
}

void FParticleMeshNxFluidEmitterInstance::Tick(FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	PhysXEmitterInstance.Tick(*this, DeltaTime, bSuppressSpawning);

	if(PhysXEmitterInstance.FluidEmitter )
	{
		FluidTypeData.TickEmitter(*PhysXEmitterInstance.FluidEmitter, DeltaTime);
	}

	// do standard tick...
	INT OldActiveParticles = ActiveParticles;
	// pass in 0 delta time, so the lifetime calculation doesn't get confused
	FParticleEmitterInstance::Tick(0.0f, bSuppressSpawning);
	// so now we need to increment this ourselves:
	SecondsSinceCreation += DeltaTime;
	// after the standard tick function, make sure no more particles got emitted...
	check(ActiveParticles >= OldActiveParticles);
	ActiveParticles = OldActiveParticles;
	// update the bounding box...
	UpdateBoundingBox(DeltaTime);
}

void FParticleMeshNxFluidEmitterInstance::TickEditor(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, FLOAT Multiplier, FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	Tick(DeltaTime, bSuppressSpawning);
	SyncPhysicsData();
}

void FParticleMeshNxFluidEmitterInstance::UpdateBoundingBox(FLOAT DeltaTime)
{
	if(Component)
	{
		PhysXEmitterInstance.UpdateBoundingBox( *this, DeltaTime );
	}
}

void FParticleMeshNxFluidEmitterInstance::RemovedFromScene(void)
{
	 PhysXEmitterInstance.RemovedFromScene();
}

void FParticleMeshNxFluidEmitterInstance::SyncPhysicsData(void)
{
	FPhysXFluidTypeData * LODLevelPhysXFluid = 0;
	UParticleLODLevel *LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	UParticleModuleTypeDataMeshNxFluid *TypeData = LODLevel ? Cast<UParticleModuleTypeDataMeshNxFluid>(LODLevel->TypeDataModule) : 0;
	if(TypeData)
	{
		LODLevelPhysXFluid = &TypeData->PhysXFluid;
	}

	PhysXEmitterInstance.SyncPhysicsData( *this, LODLevelPhysXFluid );
}

void FParticleMeshNxFluidEmitterInstance::InitPhysicsParticleData(DWORD NumParticles)
{
	if( NumParticles <= 0 )
	{
		return;
	}

#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	if(MaxActiveParticles < (INT)NumParticles)
	{
		Resize(PhysXEmitterInstance.PhysXFluid.FluidMaxParticles);
		for(INT i=0; i<MaxActiveParticles; i++)
		{
			ParticleIndices[i] = i;
		}
	}
	ActiveParticles = 0;
	
	UParticleLODLevel *LODLevel = 0;
	if(SpriteTemplate)
	{
		LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	}
	
	const FLOAT SpawnTime = 0;
	
	// initialize from spawn modules...
	if(LODLevel && NumParticles > 0)
	{
		DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * 0);
		FMeshRotationPayloadData *PayloadData = 0;
		if(MeshRotationActive)
		{
			PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshRotationOffset);
		}
		
		for(DWORD i=0; i<NumParticles; i++)
		{
			memset(&Particle, 0, sizeof(FBaseParticle));
			if(PayloadData) { memset(PayloadData, 0, sizeof(FMeshRotationPayloadData)); }
			
			INT NumSpawnModules = LODLevel->SpawnModules.Num();
			for(INT ModIndex=0; ModIndex<NumSpawnModules; ModIndex++)
			{
				UParticleModule *Module = LODLevel->SpawnModules(ModIndex);
				check(Module);
				if(Module && Module->bEnabled)
				{
					Module->Spawn(this, 0, SpawnTime);
				}
			}

			DWORD FluidParticleID = PhysXEmitterInstance.PhysXFluid.FluidIDCreationBuffer[i];
			NxFluidParticleEx &NParticleEx = PhysXEmitterInstance.PhysXFluid.FluidParticleBufferEx[FluidParticleID];
			NParticleEx.mSize = U2NVectorCopy(Particle.BaseSize);
			if(PayloadData)
			{
				const FRotator &Orientation = FRotator::MakeFromEuler(PayloadData->Rotation);
				Orientation.Quaternion();
				NParticleEx.mRot    = U2NQuaternion(Orientation.Quaternion());
				NParticleEx.mAngVel = U2NVectorCopy(PayloadData->RotationRateBase * (((FLOAT)PI) / 180.0f));
			}
		}
	}
#endif
}

void FParticleMeshNxFluidEmitterInstance::SetParticleData(DWORD NumParticles)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	if(MaxActiveParticles < (INT)NumParticles)
	{
		Resize(PhysXEmitterInstance.PhysXFluid.FluidMaxParticles);
		for(INT i=0; i<MaxActiveParticles; i++)
		{
			ParticleIndices[i] = i;
		}
	}
	ActiveParticles = (INT)NumParticles;

	FLOAT MaxLife    = PhysXEmitterInstance.PhysXFluid.FluidEmitterParticleLifetime;
	FLOAT InvMaxLife = MaxLife > 0.0f ? 1.0f / MaxLife : 1.0f;
	
	if(MeshRotationActive && SpriteTemplate->ScreenAlignment == PSA_Velocity)
	{
		for(INT i=0; i<ActiveParticles; i++)
		{
			const NxFluidParticle   &NParticle   = PhysXEmitterInstance.PhysXFluid.FluidParticleBuffer[i];
			const NxFluidParticleEx &NParticleEx = PhysXEmitterInstance.PhysXFluid.FluidParticleBufferEx[NParticle.mID];
			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
			FMeshRotationPayloadData *PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshRotationOffset);
			Particle.RelativeTime = 1.0f - (NParticle.mLife * InvMaxLife);
			Particle.Location     = N2UPosition(NParticle.mPos);
			Particle.Velocity     = N2UPosition(NParticle.mVel);
			Particle.BaseSize     = N2UVectorCopy(NParticleEx.mSize);
			Particle.Size         = N2UVectorCopy(NParticleEx.mSize);
			// build orientation
			FVector	NewDirection = Particle.Velocity;
			NewDirection.Normalize();
			FVector	OldDirection(1.0f, 0.0f, 0.0f);
			FQuat Rotation = FQuatFindBetween(OldDirection, NewDirection);
			PayloadData->Rotation = Rotation.Euler();
		}
	}
	else if(MeshRotationActive)
	{
		for(INT i=0; i<ActiveParticles; i++)
		{
			const NxFluidParticle   &NParticle   = PhysXEmitterInstance.PhysXFluid.FluidParticleBuffer[i];
			const NxFluidParticleEx &NParticleEx = PhysXEmitterInstance.PhysXFluid.FluidParticleBufferEx[NParticle.mID];
			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
			FMeshRotationPayloadData *PayloadData = (FMeshRotationPayloadData*)((BYTE*)&Particle + MeshRotationOffset);
			Particle.RelativeTime = 1.0f - (NParticle.mLife * InvMaxLife);
			Particle.Location     = N2UPosition(NParticle.mPos);
			Particle.Velocity     = N2UPosition(NParticle.mVel);
			Particle.BaseSize     = N2UVectorCopy(NParticleEx.mSize);
			Particle.Size         = N2UVectorCopy(NParticleEx.mSize);
			PayloadData->Rotation = N2UQuaternion(NParticleEx.mRot).Euler();
		}
	}
	else
	{
		for(INT i=0; i<ActiveParticles; i++)
		{
			const NxFluidParticle   &NParticle   = PhysXEmitterInstance.PhysXFluid.FluidParticleBuffer[i];
			const NxFluidParticleEx &NParticleEx = PhysXEmitterInstance.PhysXFluid.FluidParticleBufferEx[NParticle.mID];
			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[i]);
			Particle.RelativeTime = 1.0f - (NParticle.mLife * InvMaxLife);
			Particle.Location     = N2UPosition(NParticle.mPos);
			Particle.Velocity     = N2UPosition(NParticle.mVel);
			Particle.BaseSize     = N2UVectorCopy(NParticleEx.mSize);
			Particle.Size         = N2UVectorCopy(NParticleEx.mSize);
		}
	}
	
	// Hack to make sure we always have enough time to render our particles...
	// TODO: find out why LifeSpan seems to expire faster than the particles...
	if(ActiveParticles > 0)
	{
		AActor *Owner = Component ? Component->GetOwner() : 0;
		if(Owner && Owner->LifeSpan != 0 && Owner->LifeSpan < 1)
		{
			Owner->LifeSpan = 1;
		}
	}
#endif
}

/*-----------------------------------------------------------------------------
	ParticleSpriteNxFluidEmitterInstance.
-----------------------------------------------------------------------------*/

FParticleSpriteNxFluidEmitterInstance::FParticleSpriteNxFluidEmitterInstance(class UParticleModuleTypeDataNxFluid &TypeData) :
FParticleSpriteSubUVEmitterInstance(),
FluidTypeData(TypeData),
PhysXEmitterInstance(TypeData.PhysXFluid,*this)
{
}

void FParticleSpriteNxFluidEmitterInstance::Init(void)
{
	FParticleSpriteSubUVEmitterInstance::Init();
	ActiveParticles = 0;
	PhysXEmitterInstance.IndexAndRankBuffersInSync = FALSE;
}

void FParticleSpriteNxFluidEmitterInstance::Tick(FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	PhysXEmitterInstance.Tick(*this, DeltaTime, bSuppressSpawning);

	if(PhysXEmitterInstance.FluidEmitter )
	{
		FluidTypeData.TickEmitter(*PhysXEmitterInstance.FluidEmitter, DeltaTime);
	}

	// do standard tick...
	INT OldActiveParticles = ActiveParticles;
	// pass in 0 delta time, so the lifetime calculation doesn't get confused
	FParticleEmitterInstance::Tick(0.0f, bSuppressSpawning);
	// so now we need to increment this ourselves:
	SecondsSinceCreation += DeltaTime;
	// after the standard tick function, make sure no more particles got emitted...
	check(ActiveParticles >= OldActiveParticles);
	ActiveParticles = OldActiveParticles;
	// update the bounding box...
	UpdateBoundingBox(DeltaTime);
}

void FParticleSpriteNxFluidEmitterInstance::TickEditor(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, FLOAT Multiplier, FLOAT DeltaTime, UBOOL bSuppressSpawning)
{
	Tick(DeltaTime, bSuppressSpawning);
	SyncPhysicsData();
}

void FParticleSpriteNxFluidEmitterInstance::UpdateBoundingBox(FLOAT DeltaTime)
{
	if(Component)
	{
		if( DeltaTime > 0.0f )
		{
			FParticleSpriteSubUVEmitterInstance::UpdateBoundingBox( DeltaTime );
		}
		else
		{
			PhysXEmitterInstance.UpdateBoundingBox( *this, DeltaTime );
		}
	}
}

void FParticleSpriteNxFluidEmitterInstance::RemovedFromScene(void)
{
	 PhysXEmitterInstance.RemovedFromScene();
}

void FParticleSpriteNxFluidEmitterInstance::SyncPhysicsData(void)
{
	FPhysXFluidTypeData * LODLevelPhysXFluid = 0;
	UParticleLODLevel *LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
	UParticleModuleTypeDataNxFluid *TypeData = LODLevel ? Cast<UParticleModuleTypeDataNxFluid>(LODLevel->TypeDataModule) : 0;
	if(TypeData)
	{
		LODLevelPhysXFluid = &TypeData->PhysXFluid;
	}

	PhysXEmitterInstance.SyncPhysicsData( *this, LODLevelPhysXFluid );
}

void FParticleSpriteNxFluidEmitterInstance::InitPhysicsParticleData(DWORD NumParticles)
{
}

void FParticleSpriteNxFluidEmitterInstance::SetParticleData(DWORD NumParticles)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	FPhysXFluidTypeData & FluidData = PhysXEmitterInstance.PhysXFluid;
	NxFluidParticle * ParticleBuffer = FluidData.FluidParticleBuffer;

	if( MaxActiveParticles < (INT)FluidData.FluidMaxParticles )
	{
		INT OldMaxActiveParticles = MaxActiveParticles;
		Resize( FluidData.FluidMaxParticles );
		check( MaxActiveParticles == FluidData.FluidMaxParticles );
		for( INT i = OldMaxActiveParticles; i < MaxActiveParticles; ++i )
		{
			ParticleIndices[i] = i;
			FluidData.ParticleRanks[i] = i;
		}
	}

	if( !PhysXEmitterInstance.IndexAndRankBuffersInSync )
	{	// Need to re-establish index buffer
		for( INT i = 0; i < FluidData.FluidMaxParticles; ++i )
		{
			ParticleIndices[FluidData.ParticleRanks[i]] = i;
		}
		PhysXEmitterInstance.IndexAndRankBuffersInSync = TRUE;
		debugf(TEXT("Rebuit index buffer from rank buffer in PhysX fluid primary"));
	}

	check( (INT)FluidData.FluidNumParticles <= MaxActiveParticles );

	UParticleLODLevel *LODLevel = 0;
	INT NumSpawnModules = 0;
	if(SpriteTemplate)
	{
		LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
		if( LODLevel )
		{
			NumSpawnModules = LODLevel->SpawnModules.Num();
		}
	}
	
	// Active particle ID map
	UINT NumBytes = (FluidData.FluidMaxParticles+7)>>3;
	NumBytes = (NumBytes+15)&~15;	// Round up to multiple of 16 bytes, to potentially make memset's job easier
	BYTE * ActiveMap = (BYTE*)appAlloca( NumBytes );
	appMemzero( ActiveMap, NumBytes );

	// Run through fluid buffer and update active particles
	const FLOAT InvMaxLife = FluidData.FluidEmitterParticleLifetime > 0.0f ? 1.0f / FluidData.FluidEmitterParticleLifetime : 1.0f;
	for( INT i = 0; i < FluidData.FluidNumParticles; ++i )
	{
		const NxFluidParticle & NParticle = ParticleBuffer[i];
		const WORD ID = NParticle.mID;

		// Generate inverse map
		ActiveMap[ID>>3] |= 1<<(ID&7);

		DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ID);
		const FLOAT RelativeTime = 1.0f - (NParticle.mLife * InvMaxLife);

		const WORD Rank = FluidData.ParticleRanks[ID];
		if( Rank >= ActiveParticles )
		{	// Need to activate this ID
			const WORD NextID = ParticleIndices[ActiveParticles];
			ParticleIndices[Rank] = NextID;
			ParticleIndices[ActiveParticles] = ID;
			FluidData.ParticleRanks[NextID] = Rank;
			FluidData.ParticleRanks[ID] = ActiveParticles;
			Particle.RelativeTime = BIG_NUMBER;	// So that the particle will get initialized
			++ActiveParticles;
		}

		// This ID is now in use, if it wasn't already
		if( RelativeTime < Particle.RelativeTime )
		{	// This particle needs to be initialized
			INT SaveActiveParticles = ActiveParticles;	// Module->Spawn uses ActiveParticles to determine which particle to spawn
			ActiveParticles = FluidData.ParticleRanks[ID];
			appMemset( &Particle, 0, ParticleStride );
			for( INT ModIndex = 0; ModIndex < NumSpawnModules; ++ModIndex )
			{
				UParticleModule *Module = LODLevel->SpawnModules(ModIndex);
				check(Module);
				if(Module && Module->bEnabled)
				{
					Module->Spawn(this, 0, 0);
				}
			}
			ActiveParticles = SaveActiveParticles;
		}

		// Set particle data
		Particle.RelativeTime			= RelativeTime;
		Particle.OldLocation			= Particle.Location;
		Particle.Location				= N2UPosition(NParticle.mPos);
		Particle.Velocity				= N2UPosition(NParticle.mVel);
		Particle.OneOverMaxLifetime		= InvMaxLife;
	}

	// Run through current particle data and kill particles that are no longer active
	for( INT i = 0; i < ActiveParticles; )
	{
		const WORD ID = ParticleIndices[i];
		if( ((ActiveMap[ID>>3]>>(ID&7))&1) == 0 )
		{	// No longer active, kill this particle
			--ActiveParticles;
			const WORD LastID = ParticleIndices[ActiveParticles];
			ParticleIndices[i] = LastID;
			ParticleIndices[ActiveParticles] = ID;
			FluidData.ParticleRanks[LastID] = i;
			FluidData.ParticleRanks[ID] = ActiveParticles;
		}
		else
		{
			++i;
		}
	}

	check( ActiveParticles == FluidData.FluidNumParticles );
	ActiveParticles = FluidData.FluidNumParticles;

	// Hack to make sure we always have enough time to render our particles...
	// TODO: find out why LifeSpan seems to expire faster than the particles...
	if(ActiveParticles > 0)
	{
		AActor *Owner = Component ? Component->GetOwner() : 0;
		if(Owner && Owner->LifeSpan != 0 && Owner->LifeSpan < 1)
		{
			Owner->LifeSpan = 1;
		}
	}
#endif
}

