/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This SoundNode uses UEngine::bAllowMatureLanguage to determine whether child nodes
 * that have USoundNodeWave::bMature=TRUE should be played.
 */
class SoundNodeMature extends SoundNode
	native(Sound)
	collapsecategories
	hidecategories(Object)
	editinlinenew;

/** Whether or not the output is mature or not **/
var deprecated const editfixedsize array<bool> IsMature;

cpptext
{
	// USoundNode interface.
	virtual void GetNodes( class UAudioComponent* AudioComponent, TArray<USoundNode*>& SoundNodes );
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );

	virtual INT GetMaxChildNodes();
}

defaultproperties
{
}
