/**
 * Copyright © 2004-2005 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleTypeDataMeshNxFluid extends ParticleModuleTypeDataMesh
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

cpptext
{
	virtual FParticleEmitterInstance *CreateInstance(UParticleEmitter *InEmitterParent, UParticleSystemComponent *InComponent);
	
	virtual void                      SetToSensibleDefaults();	
	virtual void                      PostEditChange(UProperty* PropertyThatChanged);
	virtual void                      FinishDestroy();
	
	void                              TickEmitter(class NxFluidEmitter &emitter, FLOAT DeltaTime);
	
	void                              TickRotationSpherical(FLOAT DeltaTime);
	void                              TickRotationBox(FLOAT DeltaTime);
}

enum FluidMeshRotationMethod
{
	FMRM_DISABLED,
	FMRM_SPHERICAL,
	FMRM_BOX,
	FMRM_LONG_BOX,
	FMRM_FLAT_BOX,
};

var() editinline	PhysXFluidTypeData		PhysXFluid;

var(Fluid) const FluidMeshRotationMethod			FluidRotationMethod;	// Made const since some methods require contact buffer allocation
var(Fluid) float									FluidRotationCoefficient;

defaultproperties
{
	FluidRotationMethod                = FMRM_SPHERICAL;
	FluidRotationCoefficient           = 5.0f;
}
