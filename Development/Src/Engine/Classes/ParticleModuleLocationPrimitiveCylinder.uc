//=============================================================================
// ParticleModuleLocationPrimitiveCylinder
// Location primitive spawning within a cylinder.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class ParticleModuleLocationPrimitiveCylinder extends ParticleModuleLocationPrimitiveBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Location) bool					RadialVelocity;
var(Location) rawdistributionfloat	StartRadius;
var(Location) rawdistributionfloat	StartHeight;

enum CylinderHeightAxis
{
	PMLPC_HEIGHTAXIS_X,
	PMLPC_HEIGHTAXIS_Y,
	PMLPC_HEIGHTAXIS_Z
};

var(Location)									CylinderHeightAxis	HeightAxis;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI);
}

defaultproperties
{
	RadialVelocity=true

	Begin Object Class=DistributionFloatConstant Name=DistributionStartRadius
		Constant=50.0
	End Object
	StartRadius=(Distribution=DistributionStartRadius)

	Begin Object Class=DistributionFloatConstant Name=DistributionStartHeight
		Constant=50.0
	End Object
	StartHeight=(Distribution=DistributionStartHeight)

	bSupported3DDrawMode=true

	HeightAxis=PMLPC_HEIGHTAXIS_Z
}
