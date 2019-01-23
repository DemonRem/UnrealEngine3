/**
 *	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class ParticleModuleRequired extends ParticleModule
	native(Particle)
	editinlinenew
	dontcollapsecategories
	hidecategories(Object,Cascade);

//=============================================================================
//	General
//=============================================================================
/** The material to utilize for the emitter at this LOD level.						*/
var(Emitter)						MaterialInterface			Material;
/** The screen alignment to utilize for the emitter at this LOD level.				*/
var(Emitter)						EParticleScreenAlignment	ScreenAlignment;

/** If TRUE, update the emitter in local space										*/
var(Emitter)						bool						bUseLocalSpace;
/** If TRUE, kill the emitter when the particle system is deactivated				*/
var(Emitter)						bool						bKillOnDeactivate;
/** If TRUE, kill the emitter when it completes										*/
var(Emitter)						bool						bKillOnCompleted;
/** Whether this emitter requires sorting as specified by artist.					*/
var(Emitter)						bool						bRequiresSorting;

/** How long, in seconds, the emitter will run before looping.
 *	If set to 0, the emitter will never loop.
 */
var(Emitter)						float						EmitterDuration;
var(Emitter)						float						EmitterDurationLow;
var(Emitter)						bool						bEmitterDurationUseRange;
var(Emitter)						bool						bDurationRecalcEachLoop;

/** The number of times to loop the emitter.
 *	0 indicates loop continuously
 */
var(Emitter)						int							EmitterLoops;

/**
 *	The maximum number of particles to DRAW for this emitter.
 *	If set to 0, it will use whatever number are present.
 */
var(Emitter)						bool						bUseMaxDrawCount;
var(Emitter)						int							MaxDrawCount;

//=============================================================================
//	Spawn-related
//=============================================================================
/** The rate at which to spawn particles									*/
var(Spawn)							rawdistributionfloat		SpawnRate;

//=============================================================================
//	Burst-related
//=============================================================================
/** The method to utilize when burst-emitting particles						*/
var(Burst)							EParticleBurstMethod		ParticleBurstMethod;

/** The array of burst entries.												*/
var(Burst)		export noclear		array<ParticleBurst>		BurstList;

//=============================================================================
//	Delay-related
//=============================================================================
/**
 *	EmitterDelay
 *	This variable indicates the time (in seconds) that this emitter should be
 *	delayed in the particle system.
 */
var(Delay)							float						EmitterDelay;
var(Delay)							bool						bDelayFirstLoopOnly;

//=============================================================================
//	SubUV-related
//=============================================================================
/** The interpolation method to used for the SubUV image selection					*/
var(SubUV)							EParticleSubUVInterpMethod	InterpolationMethod;

/** The number of sub-images horizontally in the texture							*/
var(SubUV)							int							SubImages_Horizontal;

/** The number of sub-images vertically in the texture								*/
var(SubUV)							int							SubImages_Vertical;

/** Whether to scale the UV or not - ie, the model wasn't set with sub uvs			*/
var(SubUV)							bool						bScaleUV;

/**
 *	The amount of time (particle-relative, 0.0 to 1.0) to 'lock' on a random sub image
 *	    0.0 = change every frame
 *      1.0 = select a random image at spawn and hold for the life of the particle
 */
var									float						RandomImageTime;

/** The number of times to change a random image over the life of the particle.		*/
var(SubUV)							int							RandomImageChanges;

/** SUB-UV RELATIVE INTERNAL MEMBERS												*/
var									bool						bDirectUV;

//=============================================================================
//	Cascade-related
//=============================================================================
var(Cascade)						EEmitterRenderMode			EmitterRenderMode;
var(Cascade)						color						EmitterEditorColor;

//=============================================================================
//	C++
//=============================================================================
cpptext
{
	virtual void	PostEditChange(UProperty* PropertyThatChanged);
	virtual void	PostLoad();

	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);

	virtual UBOOL	GenerateLODModuleValues(UParticleModule* SourceModule, FLOAT Percentage, UParticleLODLevel* LODLevel);
}

//=============================================================================
//	Default properties
//=============================================================================
defaultproperties
{
	bSpawnModule=true
	bUpdateModule=true

	EmitterDuration=1.0
	EmitterDurationLow=0.0
	bEmitterDurationUseRange=false
	EmitterLoops=0

	Begin Object Class=DistributionFloatConstant Name=RequiredDistributionSpawnRate
	End Object
	SpawnRate=(Distribution=RequiredDistributionSpawnRate)

	SubImages_Horizontal=1
	SubImages_Vertical=1

	bUseMaxDrawCount=true
	MaxDrawCount=500
}
