/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleCollision extends ParticleModuleCollisionBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Collision)					rawdistributionvector		DampingFactor;
var(Collision)					rawdistributionvector		DampingFactorRotation;
var(Collision)					rawdistributionfloat		MaxCollisions;
var(Collision)					EParticleCollisionComplete	CollisionCompletionOption;
var(Collision)					bool						bApplyPhysics;
var(Collision)					rawdistributionfloat		ParticleMass;

/**
 *	The directional scalar value.
 *	This is a value that is used to scale the bounds to 'assist' in avoiding interpentration
 *	or large gaps.
 */
var(Collision)					float						DirScalar;

/**
 *	If TRUE, then collisions with Pawns will still react, but the
 *	UsedMaxCollisions count will not be decremented.
 *	(ie., They don't 'count' as collisions)
 */
var(Collision)					bool						bPawnsDoNotDecrementCount;
/**
 *	If TRUE, then collisions that do not have a vertical hit normal will
 *	still react, but UsedMaxCollisions count will not be decremented.
 *	(ie., They don't 'count' as collisions)
 *	Useful for having particles come to rest on floors.
 */
var(Collision)					bool						bOnlyVerticalNormalsDecrementCount;
/**
 *	The fudge factor to use to determine vertical.
 *	True vertical will have a Hit.Normal.Z == 1.0
 *	This will allow for Z components in the range of
 *	[1.0-VerticalFudgeFactor..1.0]
 *	to count as vertical collisions.
 */
var(Collision)					float						VerticalFudgeFactor;

/**
 *	How long to delay before checking a particle for collisions.
 *	Value is retrieved using the EmitterTime.
 *	It is stored in the particle payload data.
 *	During update, the particle flag IgnoreCollisions will be set
 *	until the particle relative time has surpassed the DelayAmount.
 */
var(Collision)					rawdistributionfloat		DelayAmount;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual UINT	RequiredBytes(FParticleEmitterInstance* Owner = NULL);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	/**
	 *	Called when the module is created, this function allows for setting values that make
	 *	sense for the type of emitter they are being used in.
	 *
	 *	@param	Owner			The UParticleEmitter that the module is being added to.
	 */
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner);
	
	virtual UBOOL	GenerateLODModuleValues(UParticleModule* SourceModule, FLOAT Percentage, UParticleLODLevel* LODLevel);
}

defaultproperties
{
	bSpawnModule=true
	bUpdateModule=true

	Begin Object Class=DistributionVectorUniform Name=DistributionDampingFactor
	End Object
	DampingFactor=(Distribution=DistributionDampingFactor)

	Begin Object Class=DistributionVectorConstant Name=DistributionDampingFactorRotation
		Constant=(X=1.0,Y=1.0,Z=1.0)
	End Object
	DampingFactorRotation=(Distribution=DistributionDampingFactorRotation)

	Begin Object Class=DistributionFloatUniform Name=DistributionMaxCollisions
	End Object
	MaxCollisions=(Distribution=DistributionMaxCollisions)

	CollisionCompletionOption=EPCC_Kill

	bApplyPhysics=false

	Begin Object Class=DistributionFloatConstant Name=DistributionParticleMass
		Constant=0.1
	End Object
	ParticleMass=(Distribution=DistributionParticleMass)

	DirScalar=3.5
	VerticalFudgeFactor=0.1
	
	Begin Object Class=DistributionFloatConstant Name=DistributionDelayAmount
		Constant=0.0
	End Object
	DelayAmount=(Distribution=DistributionDelayAmount)
}
