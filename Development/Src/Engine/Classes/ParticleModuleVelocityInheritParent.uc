/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleVelocityInheritParent extends ParticleModuleVelocityBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Velocity) rawdistributionvector	Scale;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=true

	Begin Object Class=DistributionVectorConstant Name=DistributionScale
		Constant=(X=1.0,Y=1.0,Z=1.0)
	End Object
	Scale=(Distribution=DistributionScale)
}
