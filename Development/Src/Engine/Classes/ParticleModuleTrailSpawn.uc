/**
 *	ParticleModuleTrailSpawn
 *	The trail spawn module.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleTrailSpawn extends ParticleModuleTrailBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

//*************************************************************************************************
// Trail Spawning Variables
//*************************************************************************************************
enum ETrail2SpawnMethod
{
	/** Use the emitter spawn settings									*/
	PET2SM_Emitter,
	/** Spawn based on the velocity of the source						*/
	PET2SM_Velocity,
	/** Spawn base on the distanced covered by the source				*/
	PET2SM_Distance
};

/** Method by which to determine when to spawn												*/
var			deprecated						ETrail2SpawnMethod			SpawnMethod;

/** Velocity is only used if BeamMethod is Velocity											*/
var			deprecated	export noclear		distributionfloat			SpawnVelocity;

/** 
 *	VelocityScale is only used if BeamMethod is Velocity
 *	The Spawn count is determined by multiplying the determined velocity of the source by
 *	this velocity scale.
 */
var			deprecated						float						VelocityScale;

/** Distance is only used if SpawnMethod is Distance										*/
var			deprecated	export noclear		distributionvector			SpawnDistance;

/** 
 *	SpawnDistanceMap
 *	This parameter will map a given distance range [MinInput..MaxInput]
 *	to the given spawn values [MinOutput..MaxOutput]
 *	Anything below the MinOutput will result in no particles being spawned
 *	NOTE: The distance travelled is accumulated. If it takes 10 frames to travel the min.
 *	distance, then MinOutput particles will be spawned every 10 frames...
 *	IMPORTANT! This type must be a floatparticleparam type, but nothing is forcing it now!
 */
var(Spawn)	export noclear		distributionfloatparticleparameter	SpawnDistanceMap;

/** 
 *	MinSpawnVelocity
 *	The minimum velocity the source must be travelling at in order to spawn particles.
 */
var(Spawn)									float								MinSpawnVelocity;

//*************************************************************************************************
// C++ Text
//*************************************************************************************************
cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual UINT	RequiredBytes(FParticleEmitterInstance* Owner = NULL);
	virtual void	SetToSensibleDefaults();
	virtual void	PostEditChange(UProperty* PropertyThatChanged);

	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	// Trail
//	virtual void	GetDataPointers(FParticleEmitterInstance* Owner, const BYTE* ParticleBase, 
//			INT& CurrentOffset, FBeam2TypeDataPayload*& BeamData, FVector*& InterpolatedPoints, 
//			FLOAT*& NoiseRate, FVector*& TargetNoisePoints, FVector*& CurrentNoisePoints, 
//			FLOAT*& TaperValues);

			UINT	GetSpawnCount(FParticleTrail2EmitterInstance* TrailInst, FLOAT DeltaTime);
}

//*************************************************************************************************
// Default properties
//*************************************************************************************************
defaultproperties
{
	SpawnMethod=PET2SM_Emitter

	VelocityScale=0.10
	
	Begin Object Class=DistributionFloatConstant Name=DistributionSpawnVelocity
		Constant=25.0
	End Object
	SpawnVelocity=DistributionSpawnVelocity

	Begin Object Class=DistributionVectorConstant Name=DistributionSpawnDistance
		Constant=(X=10,Y=10,Z=10)
	End Object
	SpawnDistance=DistributionSpawnDistance
	
	Begin Object Class=DistributionFloatParticleParameter Name=DistributionSpawnDistanceMap
		ParameterName="None"
		MinInput=10.0
		MaxInput=100.0
		MinOutput=1.0
		MaxOutput=5.0
		Constant=1.0
	End Object
	SpawnDistanceMap=DistributionSpawnDistanceMap
	
	MinSpawnVelocity=0.0
}
