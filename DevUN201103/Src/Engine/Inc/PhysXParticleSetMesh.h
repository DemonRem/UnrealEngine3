/*=============================================================================
	PhysXParticleSetMesh.h: PhysX Emitter Source.
	Copyright 2007-2008 AGEIA Technologies.
=============================================================================*/

#ifndef __PHYSXPARTICLESETMESH_H__
#define __PHYSXPARTICLESETMESH_H__

#if WITH_NOVODEX && !NX_DISABLE_FLUIDS

#include "PhysXParticleSet.h"

/**
Extends PhysXRenderParticle with information to render the mesh.
*/
struct PhysXRenderParticleMesh: public PhysXRenderParticle
{	
	FQuat Rot;
	FVector AngVel;
	FVector Size;
	FLinearColor Color;
};

/**
Represents one FPhysXParticleSet connected to one FParticleMeshPhysXEmitterRenderInstance.
The object of this class tracks the particles belonging to all instances of a 
UParticleModuleTypeDataMeshPhysX. It updates the vertex buffer for rendering. 
*/
class FPhysXParticleSetMesh: public FPhysXParticleSet
{
public:

	FPhysXParticleSetMesh(class FPhysXMeshInstance& InEmitterInstance);
	virtual ~FPhysXParticleSetMesh();

	virtual void AsyncUpdate(FLOAT DeltaTime, UBOOL bProcessSimulationStep);
	virtual void RemoveAllParticles();

	UParticleModuleTypeDataMeshPhysX&	PhysXTypeData;
	class FPhysXMeshInstance&			MeshInstance;		

	FORCEINLINE void AddParticle(const PhysXRenderParticleMesh& RenderParticle, const UParticleSystemComponent* const OwnerComponent)
	{
		const PhysXRenderParticle* Particle = (const PhysXRenderParticle*)&RenderParticle;
		FPhysXParticleSet::AddParticle(Particle, OwnerComponent);
	}

private:

	void FillInVertexBuffer(FLOAT DeltaTime, BYTE FluidRotationMethod, FLOAT FluidRotationCoefficient);
	void ApplyParticleModulesToSingleParticle(
		const PhysXRenderParticleMesh& NParticle,
		FParticleMeshPhysXEmitterInstance* SpawnInstance, 
		const NxVec3& Velocity,
		FLinearColor& Color,
		NxVec3& Size);
};

/**
	FPhysXMeshInstance provides facilities for the fast PhysX instanced rendering path.
*/
class FPhysXMeshInstance
{
public:

	FPhysXMeshInstance(class UParticleModuleTypeDataMeshPhysX &TypeData);
	~FPhysXMeshInstance();

	FDynamicEmitterDataBase* GetDynamicData(UBOOL bSelected);

	FDynamicMeshEmitterData::FParticleInstancedMeshInstance *PrepareInstanceWriteBuffer(INT CurrentMaxParticles);
#if WITH_NOVODEX
	FParticleMeshPhysXEmitterInstance* GetSpawnInstance();
#endif	//#if WITH_NOVODEX

	UParticleModuleTypeDataMeshPhysX& PhysXTypeData;
	class FPhysXParticleSetMesh* PSet;
#if WITH_NOVODEX
	TArray<FParticleMeshPhysXEmitterInstance*> SpawnInstances;
#endif	//#if WITH_NOVODEX

	INT ActiveParticles;

private:

	void AllocateDynamicEmitter(INT CurrentMaxParticles);

	FDynamicMeshEmitterData* DynamicEmitter;
};

#endif	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS

#endif
