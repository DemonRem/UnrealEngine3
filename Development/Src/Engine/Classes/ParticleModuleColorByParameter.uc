/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleColorByParameter extends ParticleModuleColorBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);
	
var(Color) name		ColorParam;
var(Color) color	DefaultColor;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	AutoPopulateInstanceProperties(UParticleSystemComponent* PSysComp);
}

defaultproperties
{
	bSpawnModule=true
	bUpdateModule=false
	
	DefaultColor=(R=255,G=255,B=255,A=255)
}
