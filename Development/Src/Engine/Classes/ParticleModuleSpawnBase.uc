/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleSpawnBase extends ParticleModule
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object)
	abstract;

var(Spawn)	bool				bProcessSpawnRate;

cpptext
{
	/**
	 *	Retrieve the spawn amount this module is contributing.
	 *	Note that if multiple Spawn-specific modules are present, if any one
	 *	of them ignores the SpawnRate processing it will be ignored.
	 *
	 *	@param	Owner		The particle emitter instance that is spawning.
	 *	@param	Offset		The offset into the particle payload for the module.
	 *	@param	OldLeftover	The bit of timeslice left over from the previous frame.
	 *	@param	DeltaTime	The time that has expired since the last frame.
	 *	@param	Number		The number of particles to spawn. (OUTPUT)
	 *	@param	Rate		The spawn rate of the module. (OUTPUT)
	 *
	 *	@return	UBOOL		FALSE if the SpawnRate should be ignored.
	 *						TRUE if the SpawnRate should still be processed.
	 */
	virtual UBOOL GetSpawnAmount(FParticleEmitterInstance* Owner, INT Offset, FLOAT OldLeftover, 
		FLOAT DeltaTime, INT& Number, FLOAT& Rate)
	{
		return bProcessSpawnRate;
	}

	/**
	 *	Retrieve the spawn amount this module is contributing.
	 *	Note that if multiple Spawn-specific modules are present, if any one
	 *	of them ignores the SpawnRate processing it will be ignored.
	 *	Editor-only version - this will interpolate between the module and the provided LowerLODModule
	 *	using the value of multiplier.
	 *
	 *	@param	Owner			The particle emitter instance that is spawning.
	 *	@param	Offset			The offset into the particle payload for the module.
	 *	@param	LowerLODModule	The lower LOD module to interpolate with.
	 *	@param	Multiplier		The interpolation 'alpha' value.
	 *	@param	OldLeftover		The bit of timeslice left over from the previous frame.
	 *	@param	DeltaTime		The time that has expired since the last frame.
	 *	@param	Number			The number of particles to spawn. (OUTPUT)
	 *	@param	Rate			The spawn rate of the module. (OUTPUT)
	 *
	 *	@return	UBOOL			FALSE if the SpawnRate should be ignored.
	 *							TRUE if the SpawnRate should still be processed.
	 */
	virtual UBOOL GetSpawnAmount(FParticleEmitterInstance* Owner, INT Offset, UParticleModule* LowerLODModule, FLOAT Multiplier,
		FLOAT OldLeftover, FLOAT DeltaTime, INT& Number, FLOAT& Rate)
	{
		return bProcessSpawnRate;
	}
}

defaultproperties
{
	bProcessSpawnRate=true
}
