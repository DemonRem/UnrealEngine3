/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class AudioComponent extends ActorComponent
	native
	noexport
	collapsecategories
	hidecategories(Object)
	hidecategories(ActorComponent)
	editinlinenew;

var()					SoundCue			SoundCue;
var		native const	SoundNode			CueFirstNode; // This is just a pointer to the root node in SoundCue.

/**
 *	Struct used for storing one per-instance named paramter for this AudioComponent.
 *	Certain nodes in the SoundCue may reference parameters by name so they can be adjusted per-instance.
 */
struct native AudioComponentParam
{
	var()	name		ParamName;
	var()	float		FloatParam;
	var() SoundNodeWave WaveParam;
};

/** Array of per-instance parameters for this AudioComponent. */
var()	editinline array<AudioComponentParam>		InstanceParameters;

/** Spatialise to the owner's coordinates */
var						bool				bUseOwnerLocation;
/** Auto start this component on creation */
var						bool				bAutoPlay;
/** Auto destroy this component on completion */
var						bool				bAutoDestroy;
/** Stop sound when owner is destroyed */
var						bool				bStopWhenOwnerDestroyed;
/** Whether the wave instances should remain active if they're dropped by the prioritization code. Useful for e.g. vehicle sounds that shouldn't cut out. */
var						bool				bShouldRemainActiveIfDropped;
/** whether we were occluded the last time we checked */
var						bool				bWasOccluded;
/** If true, subtitles in the sound data will be ignored. */
var		transient		bool				bSuppressSubtitles;
/** Set to true when the component has resources that need cleanup */
var		transient		bool				bWasPlaying;

/** Whether audio effects are applied */
var		native const	bool				bApplyEffects;
/** Whether to artificially prioritise the component to play */
var		native			bool				bAlwaysPlay;
/** Is this audio component allowed to be spatialized? */
var						bool				bAllowSpatialization;
/** Whether or not this sound plays when the game is paused in the UI */
var						bool				bIsUISound;
/** Whether or not this audio component is a music clip */
var		transient		bool				bIsMusic;
/** Whether or not the audio component should be excluded from reverb EQ processing */
var		transient		bool				bNoReverb;
/** Whether the current component has finished playing */
var		transient		bool				bFinished;

var	duplicatetransient native const	array<pointer>		WaveInstances{struct FWaveInstance};
var	duplicatetransient native const	array<byte>			SoundNodeData;

/**
 * We explicitly disregard SoundNodeOffsetMap/WaveMap/ResetWaveMap for GC as all references are already
 * handled elsewhere and we can't NULL references anyways.
 */
var	duplicatetransient native const	Map{USoundNode*,UINT} SoundNodeOffsetMap;
var	duplicatetransient native const	multimap_mirror		SoundNodeResetWaveMap{TMultiMap<USoundNode*,FWaveInstance*>};

var duplicatetransient native const	pointer				Listener{struct FListener};

var	duplicatetransient native const	float				PlaybackTime;
var duplicatetransient native const	PortalVolume		PortalVolume;
var	duplicatetransient native		vector				Location;
var	duplicatetransient native const	vector				ComponentLocation;

/** Used by the subtitle manager to prioritize subtitles wave instances spawned by this component. */
var		native			float				SubtitlePriority;

var						float				FadeInStartTime;
var						float				FadeInStopTime;
/** This is the volume level we are fading to **/
var						float				FadeInTargetVolume;

var						float				FadeOutStartTime;
var						float				FadeOutStopTime;
/** This is the volume level we are fading to **/
var						float				FadeOutTargetVolume;

var						float				AdjustVolumeStartTime;
var						float				AdjustVolumeStopTime;
/** This is the volume level we are adjusting to **/
var						float				AdjustVolumeTargetVolume;
var						float				CurrAdjustVolumeTargetVolume;

// Temporary variables for node traversal.
var		native const	SoundNode			CurrentNotifyBufferFinishedHook;
var		native const	vector				CurrentLocation;
var		native const	float				CurrentVolume;
var		native const	float				CurrentPitch;
var		native const	float				CurrentHighFrequencyGain;
var		native const	int					CurrentUseSpatialization;
var		native const	int					CurrentUseSeamlessLooping;

// Multipliers used before propagation to WaveInstance
var		native const	float				CurrentVolumeMultiplier;
var		native const	float				CurrentPitchMultiplier;

var		native const	float				CurrentVoiceCenterChannelVolume;
var		native const	float				CurrentVoiceRadioVolume;

// Serialized multipliers used to e.g. override volume for ambient sound actors.
var()					float				VolumeMultiplier;
var()					float				PitchMultiplier;

/** while playing, this component will check for occlusion from its closest listener every this many seconds
 * and call OcclusionChanged() if the status changes
 */
var float OcclusionCheckInterval;
/** last time we checked for occlusion */
var transient float LastOcclusionCheckTime;

var const DrawSoundRadiusComponent PreviewSoundRadius;

native final function Play();
native final function Stop();

/**
 * This is called in place of "play".  So you will say AudioComponent->FadeIn().
 * This is useful for fading in music or some constant playing sound.
 *
 * If FadeTime is 0.0, this is the same as calling Play() but just modifying the volume by
 * FadeVolumeLevel. (e.g. you will play instantly but the FadeVolumeLevel will affect the AudioComponent)
 *
 * If FadeTime is > 0.0, this will call Play(), and then increase the volume level of this
 * AudioCompoenent to the passed in FadeVolumeLevel over FadeInTime seconds.
 *
 * The VolumeLevel is MODIFYING the AudioComponent's "base" volume.  (e.g.  if you have an
 * AudioComponent that is volume 1000 and you pass in .5 as your VolumeLevel then you will fade to 500 )
 *
 * @param FadeInDuration how long it should take to reach the FadeVolumeLevel
 * @param FadeVolumeLevel the percentage of the AudioComponents's calculated volume in which to fade to
 **/
native final function FadeIn( FLOAT FadeInDuration, FLOAT FadeVolumeLevel );

/**
 * This is called in place of "stop".  So you will say AudioComponent->FadeOut().
 * This is useful for fading out music or some constant playing sound.
 *
 * If FadeTime is 0.0, this is the same as calling Stop().
 *
 * If FadeTime is > 0.0, this will decrease the volume level of this
 * AudioCompoenent to the passed in FadeVolumeLevel over FadeInTime seconds.
 *
 * The VolumeLevel is MODIFYING the AudioComponent's "base" volume.  (e.g.  if you have an
 * AudioComponent that is volume 1000 and you pass in .5 as your VolumeLevel then you will fade to 500 )
 *
 * @param FadeOutDuration how long it should take to reach the FadeVolumeLevel
 * @param FadeVolumeLevel the percentage of the AudioComponents's calculated volume in which to fade to
 **/
native final function FadeOut( FLOAT FadeOutDuration, FLOAT FadeVolumeLevel );


/**
 * This will allow one to adjust the volume of an AudioComponent on the fly
 **/
native final function AdjustVolume( FLOAT AdjustVolumeDuration, FLOAT AdjustVolumeLevel );


native final function SetFloatParameter(name InName, float InFloat);

native final function SetWaveParameter(name InName, SoundNodeWave InWave);

/** stops the audio (if playing), detaches the component, and resets the component's properties to the values of its template */
native final function ResetToDefaults();

/** called when we finish playing audio, either because it played to completion or because a Stop() call turned it off early */
delegate OnAudioFinished(AudioComponent AC);

/** called when OcclusionCheckInterval > 0.0 and the occlusion status changes */
event OcclusionChanged(bool bNowOccluded)
{
	VolumeMultiplier *= bNowOccluded ? 0.5 : 2.0;
}

defaultproperties
{
	bUseOwnerLocation=true
	bAutoDestroy=false
	bAutoPlay=false
	bAllowSpatialization=true

	VolumeMultiplier=1.0
	PitchMultiplier=1.0

	FadeInStopTime=-1.0f
	FadeOutStopTime=-1.0f
	AdjustVolumeStopTime=-1.0f

	FadeInTargetVolume=1.0f
	FadeOutTargetVolume=1.0f
	AdjustVolumeTargetVolume=1.0f
	CurrAdjustVolumeTargetVolume=1.0f
}
