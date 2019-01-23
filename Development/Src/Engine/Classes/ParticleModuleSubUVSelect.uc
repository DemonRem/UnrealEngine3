/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleSubUVSelect extends ParticleModuleSubUVBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(SubUV) rawdistributionvector	SubImageSelect;

cpptext
{
	virtual void Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual void UpdateSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual void UpdateMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);

	virtual void UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void UpdateEditorSprite(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void UpdateEditorMesh(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
}

defaultproperties
{
	bSpawnModule=false
	bUpdateModule=true

	Begin Object Class=DistributionVectorConstant Name=DistributionSubImageSelect
	End Object
	SubImageSelect=(Distribution=DistributionSubImageSelect)
}
