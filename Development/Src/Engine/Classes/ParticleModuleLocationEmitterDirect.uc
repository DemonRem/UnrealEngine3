//=============================================================================
// ParticleModuleLocationEmitterDirect
//
// A location module that uses particles from another emitters particles as
// position for its particles.
//
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class ParticleModuleLocationEmitterDirect extends ParticleModuleLocationBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

//=============================================================================
// Variables
//=============================================================================
// LocationEmitter

// The source emitter for spawn locations
var(Location)						export		noclear	name					EmitterName;

//=============================================================================
// C++ functions
//=============================================================================
cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

//=============================================================================
// Default properties
//=============================================================================
defaultproperties
{
	bSpawnModule=true
	bUpdateModule=true

	EmitterName=None
}
