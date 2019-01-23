/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleColorOverLife extends ParticleModuleColorBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Color)					rawdistributionvector	ColorOverLife;
var(Color)					rawdistributionfloat	AlphaOverLife;
var(Color)					bool					bClampAlpha;

cpptext
{
	virtual void	PostLoad();
	virtual	void	PostEditChange(UProperty* PropertyThatChanged);
	virtual	void	AddModuleCurvesToEditor(UInterpCurveEdSetup* EdSetup);
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);

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
	bUpdateModule=true
	bCurvesAsColor=true
	bClampAlpha=true

	Begin Object Class=DistributionVectorConstantCurve Name=DistributionColorOverLife
	End Object
	ColorOverLife=(Distribution=DistributionColorOverLife)

	Begin Object Class=DistributionFloatConstant Name=DistributionAlphaOverLife
		Constant=1.0f;
	End Object
	AlphaOverLife=(Distribution=DistributionAlphaOverLife)
}
