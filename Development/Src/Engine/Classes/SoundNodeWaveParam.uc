/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/** sound node that takes a runtime parameter for the wave to play */
class SoundNodeWaveParam extends SoundNode
	native
	collapsecategories
	hidecategories(Object)
	editinlinenew;

/** the name of the wave parameter to use to look up the SoundNodeWave we should play */
var() name WaveParameterName;

cpptext
{
	virtual INT GetMaxChildNodes()
	{
		return 1;
	}
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
	virtual void GetNodes( class UAudioComponent* AudioComponent, TArray<USoundNode*>& SoundNodes );
}
