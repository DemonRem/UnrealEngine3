/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleVelocity extends ParticleModuleVelocityBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Velocity) rawdistributionvector	StartVelocity;
var(Velocity) rawdistributionfloat	StartVelocityRadial;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=true

	Begin Object Class=DistributionVectorUniform Name=DistributionStartVelocity
	End Object
	StartVelocity=(Distribution=DistributionStartVelocity)

	Begin Object Class=DistributionFloatUniform Name=DistributionStartVelocityRadial
	End Object
	StartVelocityRadial=(Distribution=DistributionStartVelocityRadial)
}
