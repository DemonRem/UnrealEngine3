/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleOrbit extends ParticleModuleOrbitBase
	native(Particle)
	editinlinenew
	dontcollapsecategories
	hidecategories(Object,Orbit);

/**
 *	Chaining options
 *	Orbit modules will chain together in the order they appear in the module stack.
 *	The combination of a module with the one prior to it is defined by using one
 *	of the following enumerations:
 */
enum EOrbitChainMode
{
	/** Add the module values to the previous results						*/
	EOChainMode_Add,
	/**	Multiply the module values by the previous results					*/
	EOChainMode_Scale,
	/**	'Break' the chain and apply the values from the	previous results	*/
	EOChainMode_Link
};

var(Chaining)	EOrbitChainMode		ChainMode;
 
/**
 *	OrbitOptions structure
 *	Container struct for holding options on the data updating for the module.
 */
struct native OrbitOptions
{
	/**
	 *	Whether to process the data during spawning.
	 */
	var()	bool	bProcessDuringSpawn;
	/**
	 *	Whether to process the data during updating.
	 */
	var()	bool	bProcessDuringUpdate;
	/**
	 *	Whether to use emitter time during data retrieval.
	 */
	var()	bool	bUseEmitterTime;
	
	structdefaultproperties
	{
		bProcessDuringSpawn=true
	}
};

/**
 *	Offset
 *	The amount to offset the sprite from the particle positon.
 */
var(Offset)			rawdistributionvector		OffsetAmount;
var(Offset)			orbitoptions				OffsetOptions;

/**
 *	Rotation
 *	The amount to rotate the offset about the particle positon.
 *	In 'Turns'
 *		0.0 = no rotation
 *		0.5	= 180 degree rotation
 *		1.0 = 360 degree rotation
 */
var(Rotation)		rawdistributionvector		RotationAmount;
var(Rotation)		orbitoptions				RotationOptions;

/**
 *	RotationRate
 *	The rate at which to rotate the offset about the particle positon.
 *	In 'Turns'
 *		0.0 = no rotation
 *		0.5	= 180 degree rotation
 *		1.0 = 360 degree rotation
 */
var(RotationRate)	rawdistributionvector		RotationRateAmount;
var(RotationRate)	orbitoptions				RotationRateOptions;

cpptext
{
	/**
	 *	Called on a particle that is freshly spawned by the emitter.
	 *	
	 *	@param	Owner		The FParticleEmitterInstance that spawned the particle.
	 *	@param	Offset		The modules offset into the data payload of the particle.
	 *	@param	SpawnTime	The time of the spawn.
	 */
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	/**
	 *	Called on a particle that is being updated by its emitter.
	 *	
	 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
	 *	@param	Offset		The modules offset into the data payload of the particle.
	 *	@param	DeltaTime	The time since the last update.
	 */
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);

	/**
	 *	Returns the number of bytes that the module requires in the particle payload block.
	 *
	 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
	 *
	 *	@return	UINT		The number of bytes the module needs per particle.
	 */
	virtual UINT	RequiredBytes(FParticleEmitterInstance* Owner = NULL);

	/**
	 *	Returns the number of bytes the module requires in the emitters 'per-instance' data block.
	 *	
	 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
	 *
	 *	@return UINT		The number fo bytes the module needs per emitter instance.
	 */
	virtual UINT	RequiredBytesPerInstance(FParticleEmitterInstance* Owner = NULL);

	// For Cascade
	/**
	 *	In the editor, called on a particle that is freshly spawned by the emitter.
	 *	Is responsible for performing the intepolation between LOD levels for realtime 
	 *	previewing.
	 *	
	 *	@param	Owner			The FParticleEmitterInstance that spawned the particle.
	 *	@param	Offset			The modules offset into the data payload of the particle.
	 *	@param	SpawnTime		The time of the spawn.
	 *	@param	LowerLODModule	The next lower LOD module - used during interpolation.
	 *	@param	Multiplier		The interpolation alpha value to use.
	 */
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	/**
	 *	In the editor, called on a particle that is being updated by its emitter.
	 *	Is responsible for performing the intepolation between LOD levels for realtime 
	 *	previewing.
	 *	
	 *	@param	Owner			The FParticleEmitterInstance that 'owns' the particle.
	 *	@param	Offset			The modules offset into the data payload of the particle.
	 *	@param	DeltaTime		The time since the last update.
	 *	@param	LowerLODModule	The next lower LOD module - used during interpolation.
	 *	@param	Multiplier		The interpolation alpha value to use.
	 */
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=true
	bUpdateModule=true

	ChainMode=EOChainMode_Link
	
	Begin Object Class=DistributionVectorUniform Name=DistributionOffsetAmount
		Min=(X=0,Y=0,Z=0)
		Max=(X=0,Y=50,Z=0)
	End Object
	OffsetAmount=(Distribution=DistributionOffsetAmount)

	Begin Object Class=DistributionVectorUniform Name=DistributionRotationAmount
		Min=(X=0,Y=0,Z=0)
		Max=(X=1,Y=1,Z=1)
	End Object
	RotationAmount=(Distribution=DistributionRotationAmount)

	Begin Object Class=DistributionVectorUniform Name=DistributionRotationRateAmount
		Min=(X=0,Y=0,Z=0)
		Max=(X=1,Y=1,Z=1)
	End Object
	RotationRateAmount=(Distribution=DistributionRotationRateAmount)
}
