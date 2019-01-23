/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleRotation extends ParticleModuleRotationBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

/** Initial rotation distribution, in degrees. */
var(Rotation) rawdistributionfloat	StartRotation;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=true

	Begin Object Class=DistributionFloatUniform Name=DistributionStartRotation
		Min=0.0
		Max=1.0
	End Object
	StartRotation=(Distribution=DistributionStartRotation)
}

