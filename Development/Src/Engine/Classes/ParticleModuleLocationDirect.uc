/**
 *	ParticleModuleLocationDirect
 *
 *	Sets the location of particles directly.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class ParticleModuleLocationDirect extends ParticleModuleLocationBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

/** */
var(Location) rawdistributionvector	Location;
var(Location) rawdistributionvector	LocationOffset;
var(Location) rawdistributionvector	ScaleFactor;
var(Location) rawdistributionvector	Direction;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual UINT	RequiredBytes(FParticleEmitterInstance* Owner = NULL);
	virtual void	SetToSensibleDefaults();
	virtual void	PostEditChange(UProperty* PropertyThatChanged);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=true
	bUpdateModule=true

	Begin Object Class=DistributionVectorUniform Name=DistributionLocation
	End Object
	Location=(Distribution=DistributionLocation)

	Begin Object Class=DistributionVectorConstant Name=DistributionLocationOffset
		Constant=(X=0,Y=0,Z=0)
	End Object
	LocationOffset=(Distribution=DistributionLocationOffset)

	Begin Object Class=DistributionVectorConstant Name=DistributionScaleFactor
		Constant=(X=1,Y=1,Z=1)
	End Object
	ScaleFactor=(Distribution=DistributionScaleFactor)

	Begin Object Class=DistributionVectorUniform Name=DistributionDirection
	End Object
	Direction=(Distribution=DistributionDirection)
}
