/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleColor extends ParticleModuleColorBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

/** Initial color for a particle as a function of Emitter time. Range is 0-255 for X/Y/Z, corresponding to R/G/B. */
var(Color) rawdistributionvector	StartColor;
var(Color) rawdistributionfloat		StartAlpha;
var(Color) bool						bClampAlpha;

cpptext
{
	virtual void	PostLoad();
	virtual	void	PostEditChange(UProperty* PropertyThatChanged);
	virtual	void	AddModuleCurvesToEditor(UInterpCurveEdSetup* EdSetup);
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);

	/**
	 *	Called when the module is created, this function allows for setting values that make
	 *	sense for the type of emitter they are being used in.
	 *
	 *	@param	Owner			The UParticleEmitter that the module is being added to.
	 */
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner);
}

defaultproperties
{
	bSpawnModule=true
	bUpdateModule=false
	bCurvesAsColor=true
	bClampAlpha=true

	Begin Object Class=DistributionVectorConstant Name=DistributionStartColor
	End Object
	StartColor=(Distribution=DistributionStartColor)

	Begin Object Class=DistributionFloatConstant Name=DistributionStartAlpha
		Constant=1.0f;
	End Object
	StartAlpha=(Distribution=DistributionStartAlpha)
}
