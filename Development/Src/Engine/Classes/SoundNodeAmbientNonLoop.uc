/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SoundNodeAmbientNonLoop extends SoundNodeAmbient
	native(Sound)
	collapsecategories
	hidecategories(Object)
	dependson(SoundNodeAttenuation)
	editinlinenew;


var()								rawdistributionfloat	DelayTime;

struct native AmbientSoundSlot
{
	var()	SoundNodeWave	Wave;
	var()	float			PitchScale;
	var()	float			VolumeScale;
	var()	float			Weight;

	structdefaultproperties
	{
		PitchScale=1.0
		VolumeScale=1.0
		Weight=1.0
	}
};

var()								array<AmbientSoundSlot>	SoundSlots;

cpptext
{
	// USoundNode interface.
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
	virtual void GetNodes( class UAudioComponent* AudioComponent, TArray<USoundNode*>& SoundNodes );
	virtual void GetAllNodes( TArray<USoundNode*>& SoundNodes ); // Like above but returns ALL (not just active) nodes.

	/**
	 * Notifies the sound node that a wave instance in its subtree has finished.
	 *
	 * @param WaveInstance	WaveInstance that was finished
	 */
	virtual void NotifyWaveInstanceFinished( struct FWaveInstance* WaveInstance );

	/** Pick which slot to play next. */
	INT PickNextSlot();
}


defaultproperties
{
	Begin Object Class=DistributionFloatUniform Name=DistributionDelayTime
		Min=1
		Max=1
	End Object
	DelayTime=(Distribution=DistributionDelayTime)
}
