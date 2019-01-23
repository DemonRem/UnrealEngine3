/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleLocation extends ParticleModuleLocationBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Location) rawdistributionvector	StartLocation;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI);
}

defaultproperties
{
	bSpawnModule=true

	Begin Object Class=DistributionVectorUniform Name=DistributionStartLocation
	End Object
	StartLocation=(Distribution=DistributionStartLocation)

	bSupported3DDrawMode=true
}

