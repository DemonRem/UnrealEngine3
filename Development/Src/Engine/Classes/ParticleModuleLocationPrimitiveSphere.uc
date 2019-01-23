//=============================================================================
// ParticleModuleLocationPrimitiveSphere
// Location primitive spawning within a Sphere.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class ParticleModuleLocationPrimitiveSphere extends ParticleModuleLocationPrimitiveBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Location) rawdistributionfloat	StartRadius;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI);
}

defaultproperties
{
	Begin Object Class=DistributionFloatConstant Name=DistributionStartRadius
		Constant=50.0
	End Object
	StartRadius=(Distribution=DistributionStartRadius)

	bSupported3DDrawMode=true
}
