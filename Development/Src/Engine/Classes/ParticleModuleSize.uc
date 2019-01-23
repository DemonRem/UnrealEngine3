/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleSize extends ParticleModuleSizeBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Size) rawdistributionvector	StartSize;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=true
	bUpdateModule=false

	Begin Object Class=DistributionVectorUniform Name=DistributionStartSize
		Min=(X=1,Y=1,Z=1)
		Max=(X=1,Y=1,Z=1)
	End Object
	StartSize=(Distribution=DistributionStartSize)
}
