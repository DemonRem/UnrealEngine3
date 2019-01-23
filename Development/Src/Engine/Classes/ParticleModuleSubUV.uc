/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleSubUV extends ParticleModuleSubUVBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(SubUV) rawdistributionfloat	SubImageIndex;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual void	UpdateSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual void	UpdateMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	SpawnEditorSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnEditorTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	SpawnEditorMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnEditorTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditorSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditorMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	
			UBOOL	DetermineSpriteImageIndex(FParticleEmitterInstance* Owner, FBaseParticle* Particle, 
						EParticleSubUVInterpMethod eMethod, FSubUVSpritePayload*& SubUVPayload, INT& ImageIndex, FLOAT& Interp, FLOAT DeltaTime);
			UBOOL	DetermineMeshImageIndex(FParticleEmitterInstance* Owner, FBaseParticle* Particle, 
						EParticleSubUVInterpMethod eMethod, FSubUVMeshPayload*& SubUVPayload, INT& ImageIndex, FLOAT& Interp, FLOAT DeltaTime);

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

	Begin Object Class=DistributionFloatConstant Name=DistributionSubImage
	End Object
	SubImageIndex=(Distribution=DistributionSubImage)
}
