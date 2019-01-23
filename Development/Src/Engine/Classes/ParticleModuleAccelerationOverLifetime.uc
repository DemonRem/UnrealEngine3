/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleAccelerationOverLifetime extends ParticleModuleAccelerationBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Acceleration) rawdistributionvector	AccelOverLife;

cpptext
{
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=false
	bUpdateModule=true

	Begin Object Class=DistributionVectorConstantCurve Name=DistributionAccelOverLife
	End Object
	AccelOverLife=(Distribution=DistributionAccelOverLife)
}
