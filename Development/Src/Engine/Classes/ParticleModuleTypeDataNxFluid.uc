/**
 * Copyright © 2004-2005 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleTypeDataNxFluid extends ParticleModuleTypeDataBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

cpptext
{
	virtual FParticleEmitterInstance *CreateInstance(UParticleEmitter *InEmitterParent, UParticleSystemComponent *InComponent);
	
	virtual void                      PostEditChange(UProperty* PropertyThatChanged);
	virtual void                      FinishDestroy();
	
	void                              TickEmitter(class NxFluidEmitter &emitter, FLOAT DeltaTime);
}

var() editinline	PhysXFluidTypeData		PhysXFluid;
