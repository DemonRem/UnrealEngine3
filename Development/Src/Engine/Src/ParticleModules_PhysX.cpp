/*=============================================================================
	ParticleModules_PhysX.cpp: PhysX Emitter Source.
	Copyright 2007-2008 AGEIA Technologies.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EnginePhysicsClasses.h"

#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
	#include "UnNovodexSupport.h"
	#include "PhysXParticleSystem.h"
	#include "PhysXParticleSetMesh.h"
#endif		// WITH_NOVODEX && !NX_DISABLE_FLUIDS

IMPLEMENT_CLASS(UParticleModuleTypeDataMeshPhysX);

#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
void UParticleModuleTypeDataMeshPhysX::TryCreateRenderInstance(UParticleEmitter *InEmitterParent, FParticleMeshPhysXEmitterInstance *InSpawnEmitterInstance)
{
	if(!RenderInstance)
	{
		RenderInstance = new FPhysXMeshInstance(*this);
		check(RenderInstance);
	}
	RenderInstance->SpawnInstances.AddUniqueItem(InSpawnEmitterInstance);
}

void UParticleModuleTypeDataMeshPhysX::TryRemoveRenderInstance(FParticleMeshPhysXEmitterInstance *InSpawnEmitterInstance)
{
	if(RenderInstance)
	{
		RenderInstance->SpawnInstances.RemoveItem(InSpawnEmitterInstance);
		
		if(RenderInstance->SpawnInstances.Num() == 0)
		{
			delete RenderInstance;
			RenderInstance = NULL;
		}
	}
}
#endif	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS

FParticleEmitterInstance *UParticleModuleTypeDataMeshPhysX::CreateInstance(UParticleEmitter *InEmitterParent, UParticleSystemComponent *InComponent)
{
#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
	SetToSensibleDefaults(InEmitterParent);

	FParticleEmitterInstance* Instance = new FParticleMeshPhysXEmitterInstance(*this);
	check(Instance);
	Instance->InitParameters(InEmitterParent, InComponent);

	TryCreateRenderInstance(InEmitterParent, (FParticleMeshPhysXEmitterInstance*)Instance);

	return Instance;
#else	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
	return NULL;
#endif	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
}

void UParticleModuleTypeDataMeshPhysX::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	Super::SetToSensibleDefaults(Owner);
	if(PhysXParSys == NULL) // If fluid not set, set the default one
		PhysXParSys = (UPhysXParticleSystem*)UObject::StaticLoadObject(UPhysXParticleSystem::StaticClass(), NULL, TEXT("EngineResources.DefaultPhysXParSys"), NULL, LOAD_None, NULL);
}

void UParticleModuleTypeDataMeshPhysX::PreEditChange(UProperty* PropertyAboutToChange)
{
#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
	if(RenderInstance)
	{
		if(RenderInstance->PSet)
			RenderInstance->PSet->RemoveAllParticles();

		while(RenderInstance && RenderInstance->SpawnInstances.Num())
			TryRemoveRenderInstance(RenderInstance->SpawnInstances(RenderInstance->SpawnInstances.Num()-1));
	}

	if(PhysXParSys)
		PhysXParSys->RemovedFromScene();
#endif	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS

	Super::PreEditChange(PropertyAboutToChange);
}


void UParticleModuleTypeDataMeshPhysX::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
	if(PhysXParSys)
		PhysXParSys->RemovedFromScene();
#endif	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
}

void UParticleModuleTypeDataMeshPhysX::FinishDestroy()
{
#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
	if(RenderInstance)
	{
		check(RenderInstance->SpawnInstances.Num() == 0);
		delete RenderInstance;
		RenderInstance = NULL;
	}
#endif	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
	Super::FinishDestroy();
}

/////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(UParticleModuleTypeDataPhysX);

FParticleEmitterInstance *UParticleModuleTypeDataPhysX::CreateInstance(UParticleEmitter *InEmitterParent, UParticleSystemComponent *InComponent)
{
#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
	SetToSensibleDefaults(InEmitterParent);

	FParticleEmitterInstance* Instance = new FParticleSpritePhysXEmitterInstance(*this);
	check(Instance);
	Instance->InitParameters(InEmitterParent, InComponent);

	return Instance;
#else	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
	return NULL;
#endif	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
}

void UParticleModuleTypeDataPhysX::SetToSensibleDefaults(UParticleEmitter* Owner)
{
	Super::SetToSensibleDefaults(Owner);
	if(PhysXParSys == NULL) // If fluid not set, set the default one
		PhysXParSys = (UPhysXParticleSystem*)UObject::StaticLoadObject(UPhysXParticleSystem::StaticClass(), NULL, TEXT("EngineResources.DefaultPhysXParSys"), NULL, LOAD_None, NULL);
}

void UParticleModuleTypeDataPhysX::PreEditChange(UProperty* PropertyAboutToChange)
{
	if(PhysXParSys)
		PhysXParSys->RemovedFromScene();

	Super::PreEditChange(PropertyAboutToChange);
}

void UParticleModuleTypeDataPhysX::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PhysXParSys)
		PhysXParSys->RemovedFromScene();
}

void UParticleModuleTypeDataPhysX::FinishDestroy()
{
	Super::FinishDestroy();
}
