/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleMeshRotation extends ParticleModuleRotationBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

/** Initial rotation distribution, in ROTATIONS (1 = 360 degrees).						*/
var(Rotation)					rawdistributionvector	StartRotation;

/** Whether to apply the parents rotation as well										*/
var(Rotation)					bool					bInheritParent;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=true

	Begin Object Class=DistributionVectorUniform Name=DistributionStartRotation
		Min=(X=0.0,Y=0.0,Z=0.0)
		Max=(X=360.0,Y=360.0,Z=360.0)
	End Object
	StartRotation=(Distribution=DistributionStartRotation)
	
	bInheritParent=false
}
