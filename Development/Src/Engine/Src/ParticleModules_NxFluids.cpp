/*=============================================================================
	ParticleModules_NxFluids.cpp: NxFluid emitter module implementations.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EnginePhysicsClasses.h"
#include "UnNovodexSupport.h"

/**
 *	FPhysXFluidTypeData
 */
FPhysXFluidTypeData::FPhysXFluidTypeData()
{
	NovodexFluid          = 0;
	RBPhysScene           = 0;
	PrimaryEmitter        = 0;
	FluidNumParticles     = 0;
	FluidNumCreated       = 0;
	FluidNumDeleted       = 0;
	FluidIDCreationBuffer = 0;
	FluidIDDeletionBuffer = 0;
	FluidParticleBuffer   = 0;
	FluidParticleBufferEx = 0;
	FluidParticleContacts = 0;

	ParticleRanks = 0;
	
	FluidParticlePacketData         = NULL;
	FluidNumParticleForceUpdate     = 0;
	FluidParticleForceUpdate        = NULL;
	bFluidApplyParticleForceUpdates = FALSE;
};

FPhysXFluidTypeData::~FPhysXFluidTypeData()
{
};

void FPhysXFluidTypeData::PostEditChange()
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	if(NovodexFluid)
	{
		NxScene         &Scene       = NovodexFluid->getScene();
		NxU32            NumEmitters = NovodexFluid->getNbEmitters();
		NxFluidEmitter **Emitters    = NovodexFluid->getEmitters();
		NxFluid         *OldFluid    = NovodexFluid;
		FRBPhysScene    *OldUScene   = RBPhysScene;
		NovodexFluid = 0;
		PrimaryEmitter = 0;
		RBPhysScene = 0;
		for(NxU32 i=0; i<NumEmitters; i++)
		{
			FPhysXEmitterInstance *Instance = (FPhysXEmitterInstance*)Emitters[i]->userData;
			Instance->OnLostFluidEmitter();
		}
		Scene.releaseFluid(*OldFluid);
		if(OldUScene)
		{
			DestroyRBPhysScene(OldUScene);
		}
	}
#endif
}

void FPhysXFluidTypeData::FinishDestroy()
{
	check(NovodexFluid == 0);
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	// destroy the fluid if there are no emitters...
	if(NovodexFluid && !NovodexFluid->getNbEmitters())
	{
		NxScene &scene = NovodexFluid->getScene();
		check(scene.isWritable());
		scene.releaseFluid(*NovodexFluid);
		NovodexFluid = 0;
		PrimaryEmitter = 0;
	}
	if(!NovodexFluid)
	{
		if(FluidIDCreationBuffer)
		{
			delete [] FluidIDCreationBuffer;
			FluidIDCreationBuffer = 0;
			FluidNumCreated       = 0;
		}
		if(FluidIDDeletionBuffer)
		{
			delete [] FluidIDDeletionBuffer;
			FluidIDDeletionBuffer = 0;
			FluidNumDeleted       = 0;
		}
		if(FluidParticleBuffer)
		{
			delete [] FluidParticleBuffer;
			FluidParticleBuffer = 0;
			FluidNumParticles = 0;
		}
		if(FluidParticleBufferEx)
		{
			delete [] FluidParticleBufferEx;
			FluidParticleBufferEx = 0;
		}
		if(FluidParticleContacts)
		{
			delete [] FluidParticleContacts;
			FluidParticleContacts = 0;
		}

		if(ParticleRanks)
		{
			delete [] ParticleRanks;
			ParticleRanks = 0;
		}

		if(FluidAddParticlePos)
		{
			delete [] FluidAddParticlePos;
			FluidAddParticlePos = 0;
		}
		if(FluidAddParticleVel)
		{
			delete [] FluidAddParticleVel;
			FluidAddParticleVel = 0;
		}
		if(FluidAddParticleLife)
		{
			delete [] FluidAddParticleLife;
			FluidAddParticleLife = 0;
		}

		if(FluidParticlePacketData)
		{
			delete[] FluidParticlePacketData;
			FluidParticlePacketData = 0;
		}

		if(FluidParticleForceUpdate)
		{
			appFree(FluidParticleForceUpdate);
			FluidParticleForceUpdate = NULL;
			FluidNumParticleForceUpdate = 0;
		}
	}
#endif //WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	if(RBPhysScene)
	{
		DestroyRBPhysScene(RBPhysScene);
		RBPhysScene = 0;
	}
}

void FPhysXFluidTypeData::UpdatePrimaryEmitter(void)
{
	PrimaryEmitter = 0;
	if(NovodexFluid)
	{
	#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
		NxU32            NumEmitters = NovodexFluid->getNbEmitters();
		NxFluidEmitter **Emitters    = NovodexFluid->getEmitters();
		for(NxU32 i=0; i<NumEmitters; i++)
		{
			NxFluidEmitter *CurrentEmitter = Emitters[i];
			FPhysXEmitterInstance &instance = *(FPhysXEmitterInstance*)CurrentEmitter->userData;
			if(!PrimaryEmitter && instance.IsActive())
			{
				PrimaryEmitter = CurrentEmitter;
			}
			else
			{
				instance.IndexAndRankBuffersInSync = FALSE;
				if( ParticleRanks )
				{
					instance.ParticleEmitterInstance.ActiveParticles = 0;
				}
			}
		}
	#endif
	}
}

NxFluidEmitter*	FPhysXFluidTypeData::CreateEmitter(FPhysXEmitterInstance &Instance)
{
	class NxFluidEmitter *emitter = 0;
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	// detect if we need to switch to game mode from editor mode or editor mode from game mode...
	if(NovodexFluid && ((GIsGame && RBPhysScene) || (!GIsGame && !RBPhysScene)))
	{
		NxScene         &Scene       = NovodexFluid->getScene();
		NxU32            NumEmitters = NovodexFluid->getNbEmitters();
		NxFluidEmitter **Emitters    = NovodexFluid->getEmitters();
		NxFluid         *OldFluid    = NovodexFluid;
		FRBPhysScene    *OldUScene   = RBPhysScene;
		NovodexFluid = 0;
		PrimaryEmitter = 0;
		RBPhysScene = 0;
		for(NxU32 i=0; i<NumEmitters; i++)
		{
			FPhysXEmitterInstance *Inst = (FPhysXEmitterInstance*)Emitters[i]->userData;
			Inst->OnLostFluidEmitter();
		}
		Scene.releaseFluid(*OldFluid);
		if(OldUScene)
		{
			DestroyRBPhysScene(OldUScene);
		}
	}
	// create the fluid if needed...
	if(!NovodexFluid)
	{
		NxScene *scene = 0;
		// if we are in cascade create our own scene pair...
		if(scene == 0 && GIsEditor == TRUE && GIsGame == FALSE && RBPhysScene == 0)
		{
			// set gravity from WorldInfo
			AWorldInfo* Info = (AWorldInfo*)(AWorldInfo::StaticClass()->GetDefaultObject());
			check(Info);
			FVector Gravity(0, 0, Info->DefaultGravityZ * Info->RBPhysicsGravityScaling);
			RBPhysScene = CreateRBPhysScene( Gravity );
			if( RBPhysScene )
			{
				NxPlaneShapeDesc PlaneShape;
				PlaneShape.normal.set( 0, 0, 1 );
				PlaneShape.d = -5;
				NxActorDesc Actor;
				FRBCollisionChannelContainer CollidesWith;
				CollidesWith.SetChannel(RBCC_Default, TRUE);
				PlaneShape.groupsMask = CreateGroupsMask(RBCC_Default, &CollidesWith);
				Actor.shapes.pushBack( &PlaneShape );
				NxScene *EffectsScene = RBPhysScene->GetNovodexPrimaryScene();
				if(EffectsScene)
				{
					EffectsScene->createActor( Actor );
				}
			}
		}
		// select our cascade scene...
		if(scene == 0 && RBPhysScene)
		{
			if(scene == 0)
			{
				scene = RBPhysScene->GetNovodexPrimaryScene();
			}
		}
		// get the world's default scene pair...
		if(scene == 0 && GWorld && GWorld->RBPhysScene)
		{
			scene = GWorld->RBPhysScene->GetNovodexPrimaryScene();
		}
		check(scene && scene->isWritable());
		// assuming we found a scene somewhere, create the fluid...
		if(scene && FluidMaxParticles > 0)
		{
			FluidIDCreationBuffer = new DWORD[FluidMaxParticles];
			check(FluidIDCreationBuffer);
			appMemset(FluidIDCreationBuffer, 0, sizeof(DWORD)*FluidMaxParticles);
			FluidNumCreated = 0;
			
			FluidIDDeletionBuffer = new DWORD[FluidMaxParticles];
			check(FluidIDDeletionBuffer);
			appMemset(FluidIDDeletionBuffer, 0, sizeof(DWORD)*FluidMaxParticles);
			FluidNumDeleted = 0;
			
			FluidParticleBuffer   = new NxFluidParticle[FluidMaxParticles];
			check(FluidParticleBuffer);
			appMemset(FluidParticleBuffer, 0, sizeof(NxFluidParticle)*FluidMaxParticles);
			FluidNumParticles = 0;

			if( bNeedsExtendedParticleData )
			{
				FluidParticleBufferEx = new NxFluidParticleEx[FluidMaxParticles];
				check(FluidParticleBufferEx);
				appMemset(FluidParticleBufferEx, 0, sizeof(NxFluidParticleEx)*FluidMaxParticles);
			}
			
			FluidParticlePacketData = new NxFluidPacket[GNovodexSDK->getParameter(NX_CONSTANT_FLUID_MAX_PACKETS)];
			check(FluidParticlePacketData);
			appMemset(FluidParticlePacketData, 0, sizeof(NxFluidPacket));

			if( bNeedsParticleContactData )
			{
				FluidParticleContacts = new NxVec3[FluidMaxParticles];
				check(FluidParticleContacts);
				memset(FluidParticleContacts, 0, sizeof(NxVec3)*FluidMaxParticles);
			}
			
			if( bNeedsParticleRanks )
			{
				ParticleRanks = new WORD[FluidMaxParticles];
				check(ParticleRanks);
				for( INT i = 0; i < FluidMaxParticles; ++i ) { ParticleRanks[i] = i; }
			}
			
			NxFluidDesc fluidDesc;
			fluidDesc.setToDefault();
			
			const NxReal Epsilon = 0.01f;
			
			// setup fluid parameters.
			fluidDesc.maxParticles                 = FluidMaxParticles;
			fluidDesc.restParticlesPerMeter        = (NxReal)::Max(FluidRestParticlesPerMeter, Epsilon);
			fluidDesc.restDensity                  = (NxReal)::Max(FluidRestDensity, Epsilon);
			fluidDesc.kernelRadiusMultiplier       = (NxReal)::Max(FluidKernelRadiusMultiplier, 1.0f);
			fluidDesc.packetSizeMultiplier         = 4<<((NxU32)FluidPacketSizeMultiplier - (NxU32)FPSM_4);
			fluidDesc.motionLimitMultiplier        = (NxReal)::Clamp(FluidMotionLimitMultiplier, 4.0f, fluidDesc.packetSizeMultiplier*fluidDesc.kernelRadiusMultiplier);
			fluidDesc.collisionDistanceMultiplier  = (NxReal)::Clamp(FluidCollisionDistanceMultiplier, 0.0f, fluidDesc.packetSizeMultiplier*fluidDesc.kernelRadiusMultiplier);
			fluidDesc.stiffness                    = (NxReal)::Max(FluidStiffness, Epsilon);
			fluidDesc.viscosity                    = (NxReal)::Max(FluidViscosity, Epsilon);
			fluidDesc.damping                      = (NxReal)::Max(FluidDamping, 0.0f);
			fluidDesc.fadeInTime                   = (NxReal)::Max(FluidFadeInTime, 0.0f);
			fluidDesc.externalAcceleration         = U2NPosition(FluidExternalAcceleration);
			fluidDesc.staticCollisionRestitution   = (NxReal)::Clamp(FluidStaticCollisionRestitution, 0.0f, 1.0f);
			fluidDesc.staticCollisionAdhesion      = (NxReal)::Clamp(FluidStaticCollisionAdhesion, 0.0f, 1.0f);
			fluidDesc.staticCollisionAttraction    = (NxReal)::Max(FluidStaticCollisionAttraction, 0.0f);
			fluidDesc.dynamicCollisionRestitution  = (NxReal)::Clamp(FluidDynamicCollisionRestitution, 0.0f, 1.0f);
			fluidDesc.dynamicCollisionAdhesion     = (NxReal)::Clamp(FluidDynamicCollisionAdhesion, 0.0f, 1.0f);
			fluidDesc.dynamicCollisionAttraction   = (NxReal)::Max(FluidDynamicCollisionAttraction, 0.0f);
			fluidDesc.collisionResponseCoefficient = (NxReal)::Max(FluidCollisionResponseCoefficient, 0.0f);
			fluidDesc.flags                        = NX_FF_VISUALIZATION | NX_FF_ENABLED;
			
			// Check to see that packetSizeMultiplier is power of 2.
			check((fluidDesc.packetSizeMultiplier & (fluidDesc.packetSizeMultiplier - 1)) == 0);
			
			// put fluid into async compartment.
			FRBPhysScene *RBScene = RBPhysScene ? RBPhysScene : (GWorld?GWorld->RBPhysScene:0);
			if(RBScene)
			{
				fluidDesc.compartment = RBScene->GetNovodexFluidCompartment();
			}
			
			// enable hardware support for fluid if hardware is available...
			if(IsPhysXHardwarePresent())
			{
				fluidDesc.flags |= NX_FF_HARDWARE;
			}
			
			// set fluid solver.
			switch(FluidSimulationMethod)
			{
				case FSM_SPH:
					fluidDesc.simulationMethod = NX_F_SPH;
					break;
				case FSM_NO_PARTICLE_INTERACTION:
					fluidDesc.simulationMethod = NX_F_NO_PARTICLE_INTERACTION;
					break;
				case FSM_MIXED_MODE:
					fluidDesc.simulationMethod = NX_F_MIXED_MODE;
					break;
			}
			
			// enable/disable collision against static shapes.
			if(bFluidStaticCollision)
			{
				fluidDesc.collisionMethod |= NX_F_STATIC;
			}
			else
			{
				fluidDesc.collisionMethod &= ~NX_F_STATIC;
			}
			
			// enable/disable collision against dynamic shapes.
			if(bFluidDynamicCollision)
			{
				fluidDesc.collisionMethod |= NX_F_DYNAMIC;
			}
			else
			{
				fluidDesc.collisionMethod &= ~NX_F_DYNAMIC;
			}
			
			// enable/disable two-way interaction between rigid-bodies and fluid.
			if(bFluidTwoWayCollision)
			{
				fluidDesc.flags |= NX_FF_COLLISION_TWOWAY;
			}
			else
			{
				fluidDesc.flags &= ~NX_FF_COLLISION_TWOWAY;
			}
			
			// enable/disable gravity.
			if(bDisableGravity)
			{
				fluidDesc.flags |= NX_FF_DISABLE_GRAVITY;
			}
			else
			{
				fluidDesc.flags &= ~NX_FF_DISABLE_GRAVITY;
			}

			// setup group mask for the fluid.
			FRBCollisionChannelContainer CollidesWith;
			CollidesWith.SetChannel(RBCC_Default, TRUE);
			CollidesWith.SetChannel(RBCC_GameplayPhysics, TRUE);
			CollidesWith.SetChannel(RBCC_FluidDrain, TRUE);
			fluidDesc.groupsMask = CreateGroupsMask(RBCC_EffectPhysics, &CollidesWith);
			
			// set the collision group.
			fluidDesc.collisionGroup = UNX_GROUP_DEFAULT;
			
			fluidDesc.particlesWriteData.numParticlesPtr             = (NxU32*)&FluidNumParticles;
			
			fluidDesc.particlesWriteData.bufferPos                   = &FluidParticleBuffer[0].mPos[0];
			fluidDesc.particlesWriteData.bufferVel                   = &FluidParticleBuffer[0].mVel[0];
			fluidDesc.particlesWriteData.bufferLife                  = &FluidParticleBuffer[0].mLife;
			fluidDesc.particlesWriteData.bufferDensity               = &FluidParticleBuffer[0].mDensity;
			fluidDesc.particlesWriteData.bufferId                    = &FluidParticleBuffer[0].mID;
			if(FluidParticleContacts)
			{
				fluidDesc.particlesWriteData.bufferCollisionNormal   = &FluidParticleContacts[0].x;
			}
			
			fluidDesc.particlesWriteData.bufferPosByteStride         = sizeof(NxFluidParticle);
			fluidDesc.particlesWriteData.bufferVelByteStride         = sizeof(NxFluidParticle);
			fluidDesc.particlesWriteData.bufferLifeByteStride        = sizeof(NxFluidParticle);
			fluidDesc.particlesWriteData.bufferDensityByteStride     = sizeof(NxFluidParticle);
			fluidDesc.particlesWriteData.bufferIdByteStride          = sizeof(NxFluidParticle);
			if(FluidParticleContacts)
			{
				fluidDesc.particlesWriteData.bufferCollisionNormalByteStride = sizeof(FluidParticleContacts[0]);
			}
			
			fluidDesc.particleCreationIdWriteData.numIdsPtr          = (NxU32*)&FluidNumCreated;
			fluidDesc.particleCreationIdWriteData.bufferId           = (NxU32*)FluidIDCreationBuffer;
			fluidDesc.particleCreationIdWriteData.bufferIdByteStride = sizeof(FluidIDCreationBuffer[0]);
			
			fluidDesc.particleDeletionIdWriteData.numIdsPtr          = (NxU32*)&FluidNumDeleted;
			fluidDesc.particleDeletionIdWriteData.bufferId           = (NxU32*)FluidIDDeletionBuffer;
			fluidDesc.particleDeletionIdWriteData.bufferIdByteStride = sizeof(FluidIDDeletionBuffer[0]);

			/* Setup particle packet data buffer */

			fluidDesc.fluidPacketData.bufferFluidPackets = FluidParticlePacketData;
			fluidDesc.fluidPacketData.numFluidPacketsPtr = (NxU32 *) &FluidNumParticlePackets;

			check(fluidDesc.isValid());
			NovodexFluid = scene->createFluid(fluidDesc);
			check(NovodexFluid);
		}
	}
	// create the emitter.
	if(NovodexFluid)
	{
		NxFluidEmitterDesc emitterDesc;
		emitterDesc.userData = &Instance;
		
		switch(FluidEmitterShape)
		{
			case FES_RECTANGLE:
				emitterDesc.shape = NX_FE_RECTANGULAR;
				break;
			case FES_ELLIPSE:
				emitterDesc.shape = NX_FE_ELLIPSE;
				break;
		}
		
		emitterDesc.maxParticles           = (NxU32)::Max(FluidEmitterMaxParticles, 0);
		emitterDesc.dimensionX             = U2PScale*(NxReal)::Max(FluidEmitterDimensionX, 0.0f);
		emitterDesc.dimensionY             = U2PScale*(NxReal)::Max(FluidEmitterDimensionY, 0.0f);
		emitterDesc.randomPos              = U2NPosition(FluidEmitterRandomPos);
		emitterDesc.randomAngle            = (NxReal)::Max(FluidEmitterRandomAngle, 0.0f);
		emitterDesc.fluidVelocityMagnitude = U2PScale*(NxReal)::Max(FluidEmitterFluidVelocityMagnitude, 0.0f);
		emitterDesc.rate                   = (NxReal)::Max(FluidEmitterRate, 0.0f);
		emitterDesc.particleLifetime       = (NxReal)::Max(FluidEmitterParticleLifetime, 0.1f);
		emitterDesc.repulsionCoefficient   = (NxReal)::Max(FluidEmitterRepulsionCoefficient, 0.0f);
		emitterDesc.flags                  = 0;
		
		switch(FluidEmitterType)
		{
			case FET_CONSTANT_FLOW:
				emitterDesc.type = NX_FE_CONSTANT_FLOW_RATE;
				break;
			case FET_CONSTANT_PRESSURE:
				emitterDesc.type = NX_FE_CONSTANT_PRESSURE;
				break;
			case FET_FILL_OWNER_VOLUME:
				emitterDesc.type = NX_FE_CONSTANT_FLOW_RATE;
				if(GIsGame)
				{
					// we don't want to emit anything in game.
					emitterDesc.rate = 0.0f;
				}
				break;
		}
		
		emitter = NovodexFluid->createEmitter(emitterDesc);
		check(emitter);
	}
	// destroy the fluid if there are no emitters...
	if(NovodexFluid && !NovodexFluid->getNbEmitters())
	{
		NxScene &scene = NovodexFluid->getScene();
		scene.releaseFluid(*NovodexFluid);
		NovodexFluid = 0;
	}
	if(!NovodexFluid && RBPhysScene)
	{
		DestroyRBPhysScene(RBPhysScene);
		RBPhysScene = 0;
	}
	PrimaryEmitter = 0;
#endif
	return emitter;
}

void FPhysXFluidTypeData::ReleaseEmitter(class NxFluidEmitter &emitter)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	check(NovodexFluid);
	check(NovodexFluid==&emitter.getFluid());
	check(NovodexFluid->getScene().isWritable());
	// release the emitter...
	if(NovodexFluid==&emitter.getFluid())
	{
		NovodexFluid->releaseEmitter(emitter);
	}
	// release the fluid if there are no emitters...
	if(NovodexFluid && !NovodexFluid->getNbEmitters())
	{
		NxScene &scene = NovodexFluid->getScene();
		scene.releaseFluid(*NovodexFluid);
		NovodexFluid = 0;
	}
	if(!NovodexFluid && RBPhysScene)
	{
		DestroyRBPhysScene(RBPhysScene);
		RBPhysScene = 0;
	}
	PrimaryEmitter = 0;
#endif
}

void FPhysXFluidTypeData::TickEmitter(class NxFluidEmitter &emitter, FLOAT DeltaTime)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	check(NovodexFluid);
	check(NovodexFluid==&emitter.getFluid());
	if(!PrimaryEmitter)
	{
		UpdatePrimaryEmitter();
	}
	// only tick once per step, we do this by only ticking only for a single emitter.
	if(PrimaryEmitter == &emitter)
	{
		// tick the cascade physics scene if needed...
		if(RBPhysScene)
		{
			// Get step settings 
			FLOAT MaxSubstep   = NxTIMESTEP;
			FLOAT MaxDeltaTime = NxMAXDELTATIME;
			FLOAT PhysDT       = ::Min(DeltaTime, MaxDeltaTime);
			NxScene &NovodexScene = NovodexFluid->getScene();
			NovodexScene.setTiming(MaxSubstep, 5);
			NovodexScene.simulate(PhysDT);
			NovodexScene.fetchResults(NX_ALL_FINISHED, true);
		}

		// initialize new particles.
		if(FluidNumCreated)
		{
			// set reasonable defaults...
			if(FluidParticleContacts)
			{
				for(INT i=0; i<FluidNumCreated; i++)
				{
					FluidParticleContacts[FluidIDCreationBuffer[i]].zero();
				}
			}
			if( FluidParticleBufferEx )
			{
				for(INT i=0; i<FluidNumCreated; i++)
				{
					NxFluidParticleEx &NParticleEx = FluidParticleBufferEx[FluidIDCreationBuffer[i]];
					NParticleEx.mAngVel.zero();
					NParticleEx.mRot.zero();
					NParticleEx.mSize.set(1.0f);
				}
			}
		}

		// fetch the emitter instance.
		FPhysXEmitterInstance &PhysXEmitterInstance = *(FPhysXEmitterInstance*)emitter.userData;
		PhysXEmitterInstance.ParticleEmitterInstance.InitPhysicsParticleData(FluidNumCreated);
		FluidNumCreated = 0;
	}
#endif
}

void FPhysXFluidTypeData::AddForceField(class NxFluidEmitter &emitter, FForceApplicator* Applicator, const FBox& FieldBoundingBox)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	if(NovodexFluid)
	{
		check(NovodexFluid==&emitter.getFluid());

		if(!PrimaryEmitter)
		{
			UpdatePrimaryEmitter();
		}

		if( PrimaryEmitter == &emitter )
		{
			NxU32 MaxParticles = NovodexFluid->getMaxParticles();
			NxParticleData ParticleWriteData = NovodexFluid->getParticlesWriteData();

			NxU32 NumActiveParticles = *ParticleWriteData.numParticlesPtr;


			//Make sure we have space to store force updates
			if((FluidParticleForceUpdate == NULL) || (FluidNumParticleForceUpdate != MaxParticles))
			{
				if(FluidParticleForceUpdate != NULL)
				{
					appFree(FluidParticleForceUpdate);
					FluidParticleForceUpdate = NULL;
				}

				FluidParticleForceUpdate = (FVector *)appMalloc(sizeof(FVector) * MaxParticles);
				appMemset(FluidParticleForceUpdate, 0, sizeof(FVector) * MaxParticles);
				FluidNumParticleForceUpdate = MaxParticles;

			}

			UBOOL NonZero = FALSE;

			for(INT i=0; i<FluidNumParticlePackets; i++)
			{
				NxFluidPacket& FluidPacket = FluidParticlePacketData[i];

				FBox PacketBox = FBox(N2UPosition(FluidPacket.aabb.min), N2UPosition(FluidPacket.aabb.max));

				if(!PacketBox.Intersect(FieldBoundingBox))
					continue;

				INT FirstParticleIndex = FluidPacket.firstParticleIndex;

				BYTE *PositionPtr = ((BYTE *)ParticleWriteData.bufferPos) + FirstParticleIndex * ParticleWriteData.bufferPosByteStride;
				BYTE *VelocityPtr = ((BYTE *)ParticleWriteData.bufferVel) + FirstParticleIndex * ParticleWriteData.bufferVelByteStride;
				FVector *ForcePtr = FluidParticleForceUpdate + FirstParticleIndex;
				
				UBOOL CallNonZero = Applicator->ComputeForce(
					(FVector *) PositionPtr, ParticleWriteData.bufferPosByteStride, P2UScale,
					(FVector *) VelocityPtr, ParticleWriteData.bufferVelByteStride, 1.0f,
					ForcePtr, sizeof(FVector), FluidForceScale,
					0, 0, 0,
					FluidPacket.numParticles, PacketBox );
				
				if(CallNonZero)
				{
					NonZero = TRUE;
				}
			}
			
			if(NonZero)
			{
				bFluidApplyParticleForceUpdates = TRUE;
			}
		}
	}
#endif
}

void FPhysXFluidTypeData::ApplyParticleForces(class NxFluidEmitter &emitter)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)

	if((NovodexFluid == NULL) || !bFluidApplyParticleForceUpdates)
	{
		return;
	}

	INT MaxParticles = NovodexFluid->getMaxParticles();

	if((FluidParticleForceUpdate == NULL) || (MaxParticles != FluidNumParticleForceUpdate))
	{
		return;
	}

	NxParticleUpdateData ParticleUpdateData;

	ParticleUpdateData.forceMode = NX_ACCELERATION;
	ParticleUpdateData.bufferForce = (NxF32 *)FluidParticleForceUpdate;
	ParticleUpdateData.bufferFlag = NULL;

	ParticleUpdateData.bufferForceByteStride = sizeof(FVector);
	ParticleUpdateData.bufferFlagByteStride = 0;

	NovodexFluid->updateParticles(ParticleUpdateData);

	bFluidApplyParticleForceUpdates = FALSE;
	appMemset(FluidParticleForceUpdate, 0, sizeof(FVector) * FluidNumParticleForceUpdate);
#endif //WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
}

#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
static UBOOL FindAxisVolume(const NxShape &NovodexShape, NxVec3 &LineMin, NxVec3 &LineMax)
{
	UBOOL Intersects = FALSE;
	NxVec3 Normal = LineMax - LineMin;
	Normal.normalize();
	NxRaycastHit MinResult;
	NxRaycastHit MaxResult;
	bool MinHit = NovodexShape.raycast(NxRay(LineMin, Normal), 1000.0f, NX_RAYCAST_IMPACT, MinResult, true);
	bool MaxHit = false;
	if(MinHit)
	{
		MaxHit = NovodexShape.raycast(NxRay(LineMax, -Normal), 1000.0f, NX_RAYCAST_IMPACT, MaxResult, true);
	}
	//check(MinHit == MaxHit); // this case happens because of some numerical error?
	if(MinHit && MaxHit)
	{
		LineMin = MinResult.worldImpact;
		LineMax = MaxResult.worldImpact;
		Intersects = TRUE;
	}
	return Intersects;
}
#endif

void FPhysXFluidTypeData::FillVolume(class AActor &BaseActor)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	if(NovodexFluid)
	{
		if(!FluidAddParticlePos)  FluidAddParticlePos  = new NxVec3[FluidMaxParticles];
		if(!FluidAddParticleVel)  FluidAddParticleVel  = new NxVec3[FluidMaxParticles];
		if(!FluidAddParticleLife) FluidAddParticleLife = new FLOAT[FluidMaxParticles];
		
		NxVec3 *Positions  = FluidAddParticlePos;
		NxVec3 *Velocities = FluidAddParticleVel;
		NxReal *Lifes      = FluidAddParticleLife;
		
		NxFluid &Fluid        = *NovodexFluid;
		NxU32    MaxParticles = (NxU32)(FluidMaxParticles-FluidNumParticles);
		NxU32    NumParticles = 0;
		FLOAT    ParticleSize = (FLOAT)(1.0f / FluidRestParticlesPerMeter);
		
		NxParticleData ParticleData;
		ParticleData.bufferPos            = &Positions[0].x;
		ParticleData.bufferPosByteStride  = sizeof(Positions[0]);
		ParticleData.bufferVel            = &Velocities[0].x;
		ParticleData.bufferVelByteStride  = sizeof(Velocities[0]);
		ParticleData.bufferLife           = Lifes;
		ParticleData.bufferLifeByteStride = sizeof(Lifes[0]);
		ParticleData.numParticlesPtr      = &NumParticles;
		
		INT NumComponents = BaseActor.Components.Num();
		
		const NxReal BoundsEpsilon = 0.01f;
		
		for(INT i=0; i<NumComponents && NumParticles<MaxParticles; i++)
		{
			UPrimitiveComponent *PrimComp = Cast<UPrimitiveComponent>(BaseActor.Components(i));
			NxActor *NovodexActor = PrimComp ? PrimComp->GetNxActor() : 0;
			INT NumNovodexShapes = 0;
			const NxShape *const*NovodexShapes = 0;
			if(NovodexActor)
			{
				NumNovodexShapes = (INT)NovodexActor->getNbShapes();
				NovodexShapes    = NovodexActor->getShapes();
			}
			for(INT s=0; s<NumNovodexShapes && NumParticles<MaxParticles; s++)
			{
				const NxShape &NovodexShape = *NovodexShapes[s];
				NxBounds3 ShapeBounds;
				NovodexShape.getWorldBounds(ShapeBounds);
				NxVec3 Extents;
				ShapeBounds.getExtents(Extents);
				NxVec3 Areas(Extents.y*Extents.z, Extents.x*Extents.z, Extents.x*Extents.y);
				if(Areas.x < Areas.y && Areas.x < Areas.z) // X-Axis
				{
					// if the ray is 'on' the surface, it counts as a miss. this covers up for that.
					ShapeBounds.min.y += BoundsEpsilon;
					ShapeBounds.max.y -= BoundsEpsilon;
					ShapeBounds.min.z += BoundsEpsilon;
					ShapeBounds.max.z -= BoundsEpsilon;
					if(ShapeBounds.min.y > ShapeBounds.max.y)
					{
						ShapeBounds.min.y = ShapeBounds.max.y = (ShapeBounds.min.y + ShapeBounds.max.y) * 0.5f;
					}
					if(ShapeBounds.min.z > ShapeBounds.max.z)
					{
						ShapeBounds.min.z = ShapeBounds.max.z = (ShapeBounds.min.z + ShapeBounds.max.z) * 0.5f;
					}
					// make sure our rays don't start inside the surface...
					ShapeBounds.min.x -= BoundsEpsilon;
					ShapeBounds.max.x += BoundsEpsilon;
					for(NxReal Y=ShapeBounds.min.y; Y<=ShapeBounds.max.y && NumParticles<MaxParticles; Y+=ParticleSize)
					for(NxReal Z=ShapeBounds.min.z; Z<=ShapeBounds.max.z && NumParticles<MaxParticles; Z+=ParticleSize)
					{
						NxVec3 LineMin(ShapeBounds.min.x, Y, Z);
						NxVec3 LineMax(ShapeBounds.max.x, Y, Z);
						if(FindAxisVolume(NovodexShape, LineMin, LineMax))
						{
							for(FLOAT X=LineMin.x; X<=LineMax.x && NumParticles<MaxParticles; X+=ParticleSize)
							{
								Positions[NumParticles++] = NxVec3(X,Y,Z);
							}
						}
					}
				}
				else if(Areas.y < Areas.z) // Y-Axis
				{
					// if the ray is 'on' the surface, it counts as a miss. this covers up for that.
					ShapeBounds.min.x += BoundsEpsilon;
					ShapeBounds.max.x -= BoundsEpsilon;
					ShapeBounds.min.z += BoundsEpsilon;
					ShapeBounds.max.z -= BoundsEpsilon;
					if(ShapeBounds.min.x > ShapeBounds.max.x)
					{
						ShapeBounds.min.x = ShapeBounds.max.x = (ShapeBounds.min.x + ShapeBounds.max.x) * 0.5f;
					}
					if(ShapeBounds.min.z > ShapeBounds.max.z)
					{
						ShapeBounds.min.z = ShapeBounds.max.z = (ShapeBounds.min.z + ShapeBounds.max.z) * 0.5f;
					}
					// make sure our rays don't start inside the surface...
					ShapeBounds.min.y -= BoundsEpsilon;
					ShapeBounds.max.y += BoundsEpsilon;
					for(NxReal X=ShapeBounds.min.x; X<=ShapeBounds.max.x && NumParticles<MaxParticles; X+=ParticleSize)
					for(NxReal Z=ShapeBounds.min.z; Z<=ShapeBounds.max.z && NumParticles<MaxParticles; Z+=ParticleSize)
					{
						NxVec3 LineMin(X, ShapeBounds.min.y, Z);
						NxVec3 LineMax(X, ShapeBounds.max.y, Z);
						if(FindAxisVolume(NovodexShape, LineMin, LineMax))
						{
							for(FLOAT Y=LineMin.y; Y<=LineMax.y && NumParticles<MaxParticles; Y+=ParticleSize)
							{
								Positions[NumParticles++] = NxVec3(X,Y,Z);
							}
						}
					}
				}
				else // Z-Axis
				{
					// if the ray is 'on' the surface, it counts as a miss. this covers up for that.
					ShapeBounds.min.x += BoundsEpsilon;
					ShapeBounds.max.x -= BoundsEpsilon;
					ShapeBounds.min.y += BoundsEpsilon;
					ShapeBounds.max.y -= BoundsEpsilon;
					if(ShapeBounds.min.x > ShapeBounds.max.x)
					{
						ShapeBounds.min.x = ShapeBounds.max.x = (ShapeBounds.min.x + ShapeBounds.max.x) * 0.5f;
					}
					if(ShapeBounds.min.y > ShapeBounds.max.y)
					{
						ShapeBounds.min.y = ShapeBounds.max.y = (ShapeBounds.min.y + ShapeBounds.max.y) * 0.5f;
					}
					// make sure our rays don't start inside the surface...
					ShapeBounds.min.z -= BoundsEpsilon;
					ShapeBounds.max.z += BoundsEpsilon;
					for(NxReal X=ShapeBounds.min.x; X<=ShapeBounds.max.x && NumParticles<MaxParticles; X+=ParticleSize)
					for(NxReal Y=ShapeBounds.min.y; Y<=ShapeBounds.max.y && NumParticles<MaxParticles; Y+=ParticleSize)
					{
						NxVec3 LineMin(X, Y, ShapeBounds.min.z);
						NxVec3 LineMax(X, Y, ShapeBounds.max.z);
						if(FindAxisVolume(NovodexShape, LineMin, LineMax))
						{
							for(FLOAT Z=LineMin.z; Z<=LineMax.z && NumParticles<MaxParticles; Z+=ParticleSize)
							{
								Positions[NumParticles++] = NxVec3(X,Y,Z);
							}
						}
					}
				}
			}
		}

		// Finish up the rest of the initial data...
		for(NxU32 i=0; i<NumParticles; i++)
		{
			Velocities[i] = NxVec3(NxMath::rand(-1.0f, 1.0f), NxMath::rand(-1.0f, 1.0f), NxMath::rand(-1.0f, 1.0f));
			Velocities[i].setMagnitude(U2PScale*FluidEmitterFluidVelocityMagnitude);
			Lifes[i]      = (NxReal)FluidEmitterParticleLifetime;
		}

		if(NumParticles > 0)
		{
			// Before we add the particles, make sure we initialize any existing particles
			// in the creation buffer...
			if(FluidNumCreated)
			{
				if(!PrimaryEmitter)
				{
					UpdatePrimaryEmitter();
				}
				check(PrimaryEmitter);
				if(PrimaryEmitter)
				{
					// set reasonable defaults...
					if(FluidParticleContacts)
					{
						for(INT i=0; i<FluidNumCreated; i++)
						{
							FluidParticleContacts[FluidIDCreationBuffer[i]].zero();
						}
					}
					if( FluidParticleBufferEx )
					{
						for(INT i=0; i<FluidNumCreated; i++)
						{
							NxFluidParticleEx &NParticleEx = FluidParticleBufferEx[FluidIDCreationBuffer[i]];
							NParticleEx.mAngVel.zero();
							NParticleEx.mRot.zero();
							NParticleEx.mSize.set(1.0f);
						}
					}
					FPhysXEmitterInstance &PhysXEmitterInstance = *(FPhysXEmitterInstance*)PrimaryEmitter->userData;
					PhysXEmitterInstance.ParticleEmitterInstance.InitPhysicsParticleData(FluidNumCreated);
					FluidNumCreated = 0;
				}
			}
			Fluid.addParticles(ParticleData);
		}
	}
#endif // WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
}

/*-----------------------------------------------------------------------------
	UParticleModuleTypeDataMeshNxFluid implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleTypeDataMeshNxFluid);

FParticleEmitterInstance* UParticleModuleTypeDataMeshNxFluid::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
	SetToSensibleDefaults();
	FParticleEmitterInstance* Instance = new FParticleMeshNxFluidEmitterInstance(*this);
	check(Instance);

	Instance->InitParameters(InEmitterParent, InComponent);

	return Instance;
}

void UParticleModuleTypeDataMeshNxFluid::SetToSensibleDefaults()
{
	if (Mesh == NULL)
	{
		Mesh = (UStaticMesh*)UObject::StaticLoadObject(UStaticMesh::StaticClass(),NULL,TEXT("EditorMeshes.TexPropCube"),NULL,LOAD_None,NULL);
	}
	PhysXFluid.bNeedsExtendedParticleData = TRUE;
	PhysXFluid.bNeedsParticleContactData =
		FluidRotationMethod == FMRM_BOX ||
		FluidRotationMethod == FMRM_LONG_BOX ||
		FluidRotationMethod == FMRM_FLAT_BOX;
}

void UParticleModuleTypeDataMeshNxFluid::PostEditChange(UProperty* PropertyThatChanged)
{
	// Recalculate this just in case the change was the FluidRotationMethod
	PhysXFluid.bNeedsParticleContactData =
		FluidRotationMethod == FMRM_BOX ||
		FluidRotationMethod == FMRM_LONG_BOX ||
		FluidRotationMethod == FMRM_FLAT_BOX;

	PhysXFluid.PostEditChange();

	Super::PostEditChange( PropertyThatChanged );
}

void UParticleModuleTypeDataMeshNxFluid::FinishDestroy()
{
	PhysXFluid.FinishDestroy();

	Super::FinishDestroy();
}

void UParticleModuleTypeDataMeshNxFluid::TickEmitter(class NxFluidEmitter &emitter, FLOAT DeltaTime)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	PhysXFluid.TickEmitter(emitter, DeltaTime);

	// fetch the emitter instance.
	FPhysXEmitterInstance &PhysXEmitterInstance = *(FPhysXEmitterInstance*)emitter.userData;
	FParticleMeshNxFluidEmitterInstance * EmitterInstance =
		CastEmitterInstance<FParticleMeshNxFluidEmitterInstance>(&PhysXEmitterInstance.ParticleEmitterInstance);
	check(EmitterInstance);

	// only tick once per step, we do this by only ticking only for a single emitter.
	if(PhysXFluid.PrimaryEmitter == &emitter)
	{
		// tick rotation...
		switch(FluidRotationMethod)
		{
			case FMRM_SPHERICAL:
				TickRotationSpherical(DeltaTime);
				break;
			case FMRM_BOX:
			case FMRM_LONG_BOX:
			case FMRM_FLAT_BOX:
				TickRotationBox(DeltaTime);
				break;
		}

		// update the emitter particle buffer.
		EmitterInstance->SetParticleData(PhysXFluid.FluidNumParticles);
	}
	else
	{
		EmitterInstance->SetParticleData(0);
	}
#endif
}

void UParticleModuleTypeDataMeshNxFluid::TickRotationSpherical(FLOAT DeltaTime)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	const NxReal Epsilon = 0.001f;
	
	// retrieve global up axis.
	NxVec3 UpVector(0.0f, 0.0f, 1.0f);
	if(PhysXFluid.NovodexFluid)
	{
		PhysXFluid.NovodexFluid->getScene().getGravity(UpVector);
		UpVector = -UpVector;
		UpVector.normalize();
	}
	
	for(INT i=0; i<PhysXFluid.FluidNumParticles; i++)
	{
		NxVec3             vel  = PhysXFluid.FluidParticleBuffer[i].mVel;
		NxU32              id   = PhysXFluid.FluidParticleBuffer[i].mID;
		NxFluidParticleEx &node = PhysXFluid.FluidParticleBufferEx[id];
		vel.z = 0; //project onto xy plane.
		NxReal velmag = vel.magnitude();
		if(velmag > Epsilon)
		{
			NxVec3 avel;
			avel.cross(vel, UpVector);
			NxReal avelm = avel.normalize();
			if(avelm > Epsilon)
			{
				//set magnitude to magnitude of linear motion (later maybe clamp)
				avel *= -velmag;

				NxReal w = velmag;
				NxReal v = (NxReal)DeltaTime*w*FluidRotationCoefficient;
				NxReal q = NxMath::cos(v);
				NxReal s = NxMath::sin(v)/w;

				NxQuat &rot = node.mRot;
				rot.multiply(NxQuat(avel*s,q), rot);
				rot.normalize();
			}
		}
	}
#endif
}

void UParticleModuleTypeDataMeshNxFluid::TickRotationBox(FLOAT DeltaTime)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	const NxReal Epsilon = 0.01f;
	
	// retrieve global up axis.
	NxVec3 GlobalUpVector(0.0f, 0.0f, 1.0f);
	if(PhysXFluid.NovodexFluid)
	{
		PhysXFluid.NovodexFluid->getScene().getGravity(GlobalUpVector);
		GlobalUpVector = -GlobalUpVector;
		GlobalUpVector.normalize();
	}
	
	// check for required buffers...
	check(PhysXFluid.FluidParticleContacts);
	
	// use the rotation coefficient as the particle size variable.
	const NxReal ParticleSize = FluidRotationCoefficient > Epsilon ? FluidRotationCoefficient : 1.0f;
	
	// some constant values.
	const NxReal MaxRotPerStep = 2.5f;
	
	for(INT i=0; i<PhysXFluid.FluidNumParticles; i++)
	{
		NxVec3  Vel       = PhysXFluid.FluidParticleBuffer[i].mVel;
		NxU32   ID        = PhysXFluid.FluidParticleBuffer[i].mID;
		NxQuat &Rot       = PhysXFluid.FluidParticleBufferEx[ID].mRot;
		NxVec3 &AngVel    = PhysXFluid.FluidParticleBufferEx[ID].mAngVel;
		NxVec3  Contact   = PhysXFluid.FluidParticleContacts[ID];

		NxReal VelMag     = Vel.magnitude();
		NxReal Limit      = VelMag / ParticleSize;
		
		NxVec3 UpVector   = GlobalUpVector;
		UBOOL  bColliding = FALSE;
		
		// check bounce...
		if(Contact.magnitudeSquared() > Epsilon)
		{
			Contact.normalize();
			UpVector = Contact; // So we can rest on a ramp.
			NxVec3 t = Vel - Contact * Contact.dot(Vel);
			AngVel = -t.cross(Contact) / ParticleSize;
			bColliding = TRUE;
		}
        
		// compute rotation matrix.
		NxMat33 rot33;
		rot33.fromQuat(Rot);
		
		// rest pose correction.
		NxVec3 Up(0.0f);
		NxReal Best = 0;
		switch(FluidRotationMethod)
		{
			case FMRM_FLAT_BOX:
			{
				Up = rot33.getColumn(2);
				if(Up.z < 0)
				{
					Up = -Up;
				}
				break;
			}
			default:
			{
				for(int j=(FluidRotationMethod==FMRM_LONG_BOX ? 1 : 0); j<3; j++)
				{
					NxVec3 tmp = rot33.getColumn(j);
					NxReal d   = tmp.dot(UpVector);
					if(d > Best)
					{
						Up   = tmp;
						Best = d;
					}
					if(-d > Best)
					{
						Up   = -tmp;
						Best = -d;
					}
				}
				break;
			}
		}
        
        NxReal Mag = 0.0f;
        
        NxVec3 PoseCorrection(0.0f);
        if(bColliding)
        {
			PoseCorrection = Up.cross(UpVector);
			Mag            = PoseCorrection.magnitude();
			NxReal MaxMag  = 0.5f / (1.0f+Limit);
			if(Mag > MaxMag)
			{
				PoseCorrection *= MaxMag / Mag;
			}
		}
		
		// limit angular velocity.
		Mag = AngVel.magnitude();
		if(Mag > Limit)
		{
			AngVel *= Limit / Mag;
		}
		
		// integrate rotation.
		NxVec3 DeltaRot = AngVel * (NxReal)DeltaTime;
		
		// apply combined rotation.
		NxVec3 Axis  = DeltaRot + PoseCorrection;
		NxReal Angle = Axis.normalize();
		if(Angle > Epsilon)
		{
			if(Angle > MaxRotPerStep)
			{
				Angle = MaxRotPerStep;
			}
			NxQuat TempRot;
			TempRot.fromAngleAxisFast(Angle, Axis);
			Rot = TempRot * Rot;
		}
	}
#endif //iWITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
}

/*-----------------------------------------------------------------------------
	UParticleModuleTypeDataNxFluid implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UParticleModuleTypeDataNxFluid);

FParticleEmitterInstance* UParticleModuleTypeDataNxFluid::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
	SetToSensibleDefaults();

	PhysXFluid.bNeedsParticleRanks = TRUE;

	FParticleEmitterInstance* Instance = new FParticleSpriteNxFluidEmitterInstance(*this);
	check(Instance);

	Instance->InitParameters(InEmitterParent, InComponent);

	return Instance;
}

void UParticleModuleTypeDataNxFluid::PostEditChange(UProperty* PropertyThatChanged)
{
	PhysXFluid.PostEditChange();

	Super::PostEditChange( PropertyThatChanged );
}

void UParticleModuleTypeDataNxFluid::FinishDestroy()
{
	PhysXFluid.FinishDestroy();

	Super::FinishDestroy();
}

void UParticleModuleTypeDataNxFluid::TickEmitter(class NxFluidEmitter &emitter, FLOAT DeltaTime)
{
#if WITH_NOVODEX && !defined(NX_DISABLE_FLUIDS)
	PhysXFluid.TickEmitter(emitter, DeltaTime);

	// fetch the emitter instance.
	FPhysXEmitterInstance &PhysXEmitterInstance = *(FPhysXEmitterInstance*)emitter.userData;
	FParticleSpriteNxFluidEmitterInstance * EmitterInstance =
		CastEmitterInstance<FParticleSpriteNxFluidEmitterInstance>(&PhysXEmitterInstance.ParticleEmitterInstance);
	check(EmitterInstance);

	// only tick once per step, we do this by only ticking only for a single emitter.
	if(PhysXFluid.PrimaryEmitter == &emitter)
	{
		// update the emitter particle buffer.
		EmitterInstance->SetParticleData(PhysXFluid.FluidNumParticles);
	}
	else
	{
		EmitterInstance->ActiveParticles = 0;
	}
#endif
}
