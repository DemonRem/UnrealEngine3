/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleRotationOverLifetime extends ParticleModuleRotationBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

/**
 *	The rotation to apply.
 */
var(Rotation) rawdistributionfloat	RotationOverLife;

/**
 *	If TRUE,  the particle rotation is multiplied by the value retrieved from RotationOverLife.
 *	If FALSE, the particle rotation is incremented by the value retrieved from RotationOverLife.
 */
var(Rotation)					bool				Scale;

cpptext
{
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=false
	bUpdateModule=true

	Begin Object Class=DistributionFloatConstantCurve Name=DistributionRotOverLife
	End Object
	RotationOverLife=(Distribution=DistributionRotOverLife)
	
	// Setting to true to support existing modules...
	Scale=true
}
