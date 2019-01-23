//=============================================================================
// ParticleModuleLocationPrimitiveBase
// Base class for setting particle spawn locations based on primitives.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class ParticleModuleLocationPrimitiveBase extends ParticleModuleLocationBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

var(Location) bool					Positive_X;
var(Location) bool					Positive_Y;
var(Location) bool					Positive_Z;
var(Location) bool					Negative_X;
var(Location) bool					Negative_Y;
var(Location) bool					Negative_Z;
var(Location) bool					SurfaceOnly;
var(Location) bool					Velocity;
var(Location) rawdistributionfloat	VelocityScale;
var(Location) rawdistributionvector	StartLocation;

cpptext
{
	virtual void	DetermineUnitDirection(FParticleEmitterInstance* Owner, FVector& vUnitDir);
}

defaultproperties
{
	bSpawnModule=true

	Positive_X=true
	Positive_Y=true
	Positive_Z=true
	Negative_X=true
	Negative_Y=true
	Negative_Z=true

	SurfaceOnly=false
	Velocity=false

	Begin Object Class=DistributionFloatConstant Name=DistributionVelocityScale
		Constant=1
	End Object
	VelocityScale=(Distribution=DistributionVelocityScale)

	Begin Object Class=DistributionVectorConstant Name=DistributionStartLocation
		Constant=(X=0,Y=0,Z=0)
	End Object
	StartLocation=(Distribution=DistributionStartLocation)
}
