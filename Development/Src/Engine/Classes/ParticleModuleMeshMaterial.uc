/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleMeshMaterial extends ParticleModuleMaterialBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

//=============================================================================
//	Properties
//=============================================================================
/**
 *	The array of materials to apply to the mesh particles.
 */
var(MeshMaterials)	array<MaterialInterface>		MeshMaterials;

//=============================================================================
//	C++
//=============================================================================
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
	 *	Returns the number of bytes the module requires in the emitters 'per-instance' data block.
	 *	
	 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
	 *
	 *	@return UINT		The number fo bytes the module needs per emitter instance.
	 */
	virtual UINT	RequiredBytesPerInstance(FParticleEmitterInstance* Owner = NULL);

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
}

//=============================================================================
//	Default properties
//=============================================================================
defaultproperties
{
	bSpawnModule=true
	bUpdateModule=true
}
