/**
 *	ParticleModuleTrailSource
 *
 *	This module implements a single source for a Trail emitter.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class ParticleModuleTrailSource extends ParticleModuleTrailBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object);

enum ETrail2SourceMethod
{
	/** Default	- use the emitter position. 
	 *	This is the fallback for when other modes can't be resolved.
	 */
	PET2SRCM_Default,
	/** Particle	- use the particles from a given emitter in the system.		
	 *	The name of the emitter should be set in SourceName.
	 */
	PET2SRCM_Particle,
	/** Actor		- use the actor as the source.
	 *	The name of the actor should be set in SourceName.
	 */
	PET2SRCM_Actor
};

/** The method flag																			*/
var(Source)									ETrail2SourceMethod				SourceMethod;

/** The strength of the tangent from the source point for each Trail						*/
var(Source)									name							SourceName;

/** The strength of the tangent from the source point for each Trail						*/
var(Source)									rawdistributionfloat			SourceStrength;

/** Whether to lock the source to the life of the particle									*/
var(Source)									bool							bLockSourceStength;

/**
 *	SourceOffsetCount
 *	The number of source offsets that can be expected to be found on the instance.
 *	These must be named
 *		TrailSourceOffset#
 */
var(Source)									INT								SourceOffsetCount;

/** Default offsets from the source(s). 
 *	If there are < MaxBeamCount slots, the grabbing of values will simply wrap.
 */
var(Source)		editfixedsize				array<vector>					SourceOffsetDefaults;

/**	Particle selection method, when using the SourceMethod of Particle						*/
var(Source)									EParticleSourceSelectionMethod	SelectionMethod;

/**	Interhit particle rotation - only valid for SourceMethod of PET2SRCM_Particle			*/
var(Source)									bool							bInheritRotation;

cpptext
{
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);
	virtual UINT	RequiredBytes(FParticleEmitterInstance* Owner = NULL);
	virtual void	SetToSensibleDefaults();
	virtual void	PostEditChange(UProperty* PropertyThatChanged);

	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	virtual void	AutoPopulateInstanceProperties(UParticleSystemComponent* PSysComp);

			void	GetDataPointers(FParticleTrail2EmitterInstance* TrailInst, 
						const BYTE* ParticleBase, INT& CurrentOffset, 
						FTrailParticleSourcePayloadData*& ParticleSource);
			void	GetDataPointerOffsets(FParticleTrail2EmitterInstance* TrailInst, 
						const BYTE* ParticleBase, INT& CurrentOffset, 
						INT& ParticleSourceOffset);
						
			UBOOL	ResolveSourceData(FParticleTrail2EmitterInstance* TrailInst, 
						const BYTE* ParticleBase, FTrail2TypeDataPayload* TrailData, 
						INT& CurrentOffset, INT	ParticleIndex, UBOOL bSpawning);

			UBOOL	ResolveSourcePoint(FParticleTrail2EmitterInstance* TrailInst, 
				FBaseParticle& Particle, FTrail2TypeDataPayload& TrailData, 
				FVector& Position, FVector& Tangent);

			FVector	ResolveSourceOffset(FParticleTrail2EmitterInstance* TrailInst, 
				FBaseParticle& Particle, FTrail2TypeDataPayload& TrailData);
}

defaultproperties
{
	SourceMethod=PET2SRCM_Default
	SourceName="None"

	Begin Object Class=DistributionFloatConstant Name=DistributionSourceStrength
		Constant=100.0
	End Object
	SourceStrength=(Distribution=DistributionSourceStrength)
	
	SelectionMethod=EPSSM_Sequential
	
	bInheritRotation=false
}
