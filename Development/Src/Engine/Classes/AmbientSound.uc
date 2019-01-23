//=============================================================================
// Ambient sound, sits there and emits its sound.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class AmbientSound extends Keypoint
	native(Sound);

/** Should the audio component automatically play on load? */
var() bool bAutoPlay;

/** Audio component to play */
var(Audio) editconst const AudioComponent AudioComponent;

/** Is the audio component currently playing? */
var private bool bIsPlaying;

cpptext
{
public:
	// AActor interface.
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();

protected:
	/**
	 * Starts audio playback if wanted.
	 */
	virtual void UpdateComponentsInternal(UBOOL bCollisionUpdate = FALSE);
public:
}

defaultproperties
{
	Begin Object NAME=Sprite
		Sprite=Texture2D'EngineResources.S_Ambient'
	End Object

	Begin Object Class=AudioComponent Name=AudioComponent0
		bAutoPlay=false
		bStopWhenOwnerDestroyed=true
		bShouldRemainActiveIfDropped=true
	End Object
	AudioComponent=AudioComponent0
	Components.Add(AudioComponent0)

	bAutoPlay=TRUE
	
	RemoteRole=ROLE_None
}
