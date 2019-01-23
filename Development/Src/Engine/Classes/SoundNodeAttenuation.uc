/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SoundNodeAttenuation extends SoundNode
	native(Sound)
	collapsecategories
	hidecategories(Object)
	editinlinenew;


enum SoundDistanceModel
{
	ATTENUATION_Linear,
	ATTENUATION_Logarithmic,
	ATTENUATION_Inverse,
	ATTENUATION_LogReverse,
	ATTENUATION_NaturalSound
};

var()					SoundDistanceModel		DistanceModel;

/**
 * This is the point at which to start attenuating.  Prior to this point the sound will be at full volume.
 **/
var()					rawdistributionfloat	MinRadius;
var()					rawdistributionfloat	MaxRadius;
var()					float					dBAttenuationAtMax;
/**
 * This is the point at which to start applying a low pass filter.  Prior to this point the sound will be dry.
 **/
var()					rawdistributionfloat	LPFMinRadius;
var()					rawdistributionfloat	LPFMaxRadius;

var()					bool					bSpatialize;
var()					bool					bAttenuate;
var()					bool					bAttenuateWithLowPassFilter;

cpptext
{
	// USoundNode interface.

	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
	virtual FLOAT MaxAudibleDistance(FLOAT CurrentMaxDistance) { return ::Max<FLOAT>(CurrentMaxDistance,MaxRadius.GetValue(0.9f)); }
}

defaultproperties
{
	Begin Object Class=DistributionFloatUniform Name=DistributionMinRadius
		Min=400
		Max=400
	End Object
	MinRadius=(Distribution=DistributionMinRadius)

	Begin Object Class=DistributionFloatUniform Name=DistributionMaxRadius
		Min=5000
		Max=5000
	End Object
	MaxRadius=(Distribution=DistributionMaxRadius)
	dBAttenuationAtMax=-60;

	Begin Object Class=DistributionFloatUniform Name=DistributionLPFMinRadius
		Min=1500
		Max=1500
	End Object
	LPFMinRadius=(Distribution=DistributionLPFMinRadius)

	Begin Object Class=DistributionFloatUniform Name=DistributionLPFMaxRadius
		Min=5000
		Max=5000
	End Object
	LPFMaxRadius=(Distribution=DistributionLPFMaxRadius)

	bSpatialize=true
	bAttenuate=true
	bAttenuateWithLowPassFilter=true
}
