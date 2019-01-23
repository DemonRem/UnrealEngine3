/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleLifetime extends ParticleModuleLifetimeBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Lifetime) rawdistributionfloat	Lifetime;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);

	/**
	 *	Called when the module is created, this function allows for setting values that make
	 *	sense for the type of emitter they are being used in.
	 *
	 *	@param	Owner			The UParticleEmitter that the module is being added to.
	 */
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner);

	virtual FLOAT	GetMaxLifetime();
}

defaultproperties
{
	bSpawnModule=true

	Begin Object Class=DistributionFloatUniform Name=DistributionLifetime
	End Object
	Lifetime=(Distribution=DistributionLifetime)
}
