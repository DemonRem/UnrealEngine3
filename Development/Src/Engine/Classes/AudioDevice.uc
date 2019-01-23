/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class AudioDevice extends Subsystem
	config( engine )
	native( AudioDevice )
	transient;

struct native Listener
{
	var const PortalVolume PortalVolume;
	var vector Location;
	var vector Up;
	var vector Right;
	var vector Front;
};

/**
 * Enum describing the sound modes available for use in game.
 */
enum ESoundMode
{
	/** Normal - No EQ applied */
	SOUNDMODE_NORMAL,
	/** Death - Death EQ applied */
	SOUNDMODE_DEATH,
	/** Cover - EQ applied to indicate player is in cover */
	SOUNDMODE_COVER,
	/** Roadie Run - Accentuates high-pitched bullet whips, etc. */
	SOUNDMODE_ROADIE_RUN,
	/** TacCom - Tactical command EQ lowers game volumes */
	SOUNDMODE_TACCOM,
	/** Applied to the radio effect */
	SOUNDMODE_RADIO,
};

/** 
 * Structure defining a sound mode (used for EQ and volume ducking)
 */
struct native ModeSettings
{
	var	ESoundMode Mode;
	var	float FadeTime;
	
	structdefaultproperties
	{
		Mode=SOUNDMODE_NORMAL
		FadeTime=0.1
	}
};

/**
 * Structure containing configurable properties of a sound group.
 */
struct native SoundGroupProperties
{
	/** Volume multiplier. */
	var() float Volume;
	/** Voice center channel volume - Not a multiplier (no propagation)	*/
	var() float VoiceCenterChannelVolume;
	/** Radio volume multiplier - Not a multiplier (no propagation) */
	var() float VoiceRadioVolume;
	
	/** Sound mode voice - whether to apply audio effects */
	var() bool bApplyEffects;
	/** Whether to artificially prioritise the component to play */
	var() bool bAlwaysPlay;
	/** Whether or not this sound plays when the game is paused in the UI */
	var() bool bIsUISound;
	/** Whether or not this is music (propagates only if parent is TRUE) */
	var() bool bIsMusic;
	/** Whether or not this sound group is excluded from reverb EQ */
	var() bool bNoReverb;

	structdefaultproperties
	{
		Volume=1
		VoiceCenterChannelVolume=0
		VoiceRadioVolume=0
		bApplyEffects=FALSE
		bAlwaysPlay=FALSE
		bIsUISound=FALSE
		bIsMusic=FALSE
		bNoReverb=FALSE
	}
	
	structcpptext
	{
		/** Interpolate the data in sound groups */
		void Interpolate( FLOAT InterpValue, FSoundGroupProperties& Start, FSoundGroupProperties& End );
	}
};

/**
 * Structure containing information about a sound group.
 */
struct native SoundGroup
{
	/** Configurable properties like volume and priority. */
	var() SoundGroupProperties	Properties;
	/** Name of this sound group. */
	var() name					GroupName;
	/** Array of names of child sound groups. Empty for leaf groups. */
	var() array<name>			ChildGroupNames;
};

/**
 * Elements of data for sound group volume control
 */
struct native SoundGroupAdjuster
{
	var	name	GroupName;
	var	float	VolumeAdjuster;
	
	structdefaultproperties
	{
		GroupName="Master"
		VolumeAdjuster=1
	}
};

/**
 * Group of adjusters
 */
struct native SoundGroupEffect
{
	var array<SoundGroupAdjuster>	GroupEffect;
};

var		config const	int							MaxChannels;
var		config const	bool						UseEffectsProcessing;

var		transient const	array<AudioComponent>		AudioComponents;
var		native const	array<pointer>				Sources{FSoundSource};
var		native const	array<pointer>				FreeSources{FSoundSource};
var		native const	DynamicMap_Mirror			WaveInstanceSourceMap{TDynamicMap<FWaveInstance*, FSoundSource*>};

var		native const	bool						bGameWasTicking;

var		native const	array<Listener>				Listeners;
var		native const	QWORD						CurrentTick;

/** Map from name to the sound group index - used to index the following 4 arrays */
var		native const	Map_Mirror					NameToSoundGroupIndexMap{TMap<FName, INT>};

/** The sound group constants that we are interpolating from */
var		native const	array<SoundGroup>			SourceSoundGroups;
/** The current state of sound group constants */
var		native const	array<SoundGroup>			CurrentSoundGroups;
/** The sound group constants that we are interpolating to */
var		native const	array<SoundGroup>			DestinationSoundGroups;

/** Array of sound groups read from ini file */
var()	config			array<SoundGroup>			SoundGroups;

/** Array of presets that modify sound groups */
var()	config			array<SoundGroupEffect>		SoundGroupEffects;

/** Interface to audio effects processing */
var		native const	pointer						Effects{class FAudioEffectsManager};

var		native const	ESoundMode					CurrentMode;
var		native const	double						SoundModeStartTime;
var		native const	double						SoundModeEndTime;

/** Interface to text to speech processor */
var		native const	pointer						TextToSpeech{class FTextToSpeech};

var		native const	bool						bTestLowPassFilter;
var		native const	bool						bTestEQFilter;

cpptext
{
	friend class FSoundSource;

	/** Constructor */
	UAudioDevice( void ) {}

	/** 
	 * Basic initialisation of the platform agnostic layer of the audio system
	 */
	virtual UBOOL Init( void );

	/** 
	 * Stop all the audio components and sources
	 */
	virtual void Flush( void );

	/** 
	 * Complete the destruction of this class
	 */
	virtual void FinishDestroy( void );

	/** 
	 * Handle pausing/unpausing of sources when entering or leaving pause mode
	 */
	void HandlePause( UBOOL bGameTicking );

	/** 
	 * Iterate over the active AudioComponents for wave instances that could be playing
	 */
	INT GetSortedActiveWaveInstances( TArray<FWaveInstance*>& WaveInstances, UBOOL bGameTicking );

	/** 
	 * Stop sources that are no longer audible
	 */
	void StopSources( TArray<FWaveInstance*>& WaveInstances, INT FirstActiveIndex );

	/** 
	 * Start and/or update any sources that have a high enough priority to play
	 */
	void StartSources( TArray<FWaveInstance*>& WaveInstances, INT FirstActiveIndex, UBOOL bGameTicking );

	/** 
	 * The audio system's main "Tick" function
	 */
	virtual void Update( UBOOL bGameTicking );

	/** 
	 * Sets the listener's location and orientation for the viewport
	 */
	void SetListener( INT ViewportIndex, const FVector& Location, const FVector& Up, const FVector& Right, const FVector& Front );

	/**
	 * Stops all game sounds (and possibly UI) sounds
	 *
	 * @param bShouldStopUISounds If TRUE, this function will stop UI sounds as well
	 */
	virtual void StopAllSounds( UBOOL bShouldStopUISounds = FALSE );

	/**
	 * Pushes the specified reverb settings onto the reverb settings stack.
	 *
	 * @param	ReverbSettings		The reverb settings to use.
	 */
	void SetReverbSettings( const FReverbSettings& ReverbSettings );

	/**
	 * Frees the bulk resource data assocated with this SoundNodeWave.
	 *
	 * @param	SoundNodeWave	wave object to free associated bulk data
	 */
	virtual void FreeResource( USoundNodeWave* SoundNodeWave ) 
	{
	}

	/**
	 * Precaches the passed in sound node wave object.
	 *
	 * @param	SoundNodeWave	Resource to be precached.
	 */
	virtual void Precache( USoundNodeWave* SoundNodeWave ) 
	{
	}

	/** 
	 * Lists all the loaded sounds and their memory footprint
	 */
	virtual void ListSounds( FOutputDevice& Ar ) 
	{
	}

	/** 
	 * Lists a summary of loaded sound collated by group
	 */
	void ListSoundGroups( FOutputDevice& Ar );

	/**
	 * Set up the sound group hierarchy
	 */
	void InitSoundGroups( void );

	/**
	 * Returns the sound group properties associates with a sound group taking into account
	 * the group tree.
	 *
	 * @param	SoundGroupName	name of sound group to retrieve properties from
	 * @return	sound group properties if a sound group with name SoundGroupName exists, NULL otherwise
	 */
	FSoundGroupProperties* GetSoundGroupProperties( FName SoundGroupName );

	/**
	 * Returns an array of existing sound group names.
	 *
	 * @return array of sound group names
	 */
	TArray<FName> GetSoundGroupNames( void );

	/**
	 * Parses the sound groups and propagates multiplicative properties down the tree.
	 */
	void ParseSoundGroups( void );

	/** 
	 * Construct the CurrentSoundGroupProperties map
	 *
	 * This contains the original sound group properties propagated properly, and all adjustments due to the sound mode
	 */
	void GetCurrentSoundGroupState( void );

	/** 
	 * Gets the parameters for EQ based on settings and time
	 */
	void Interpolate( FLOAT InterpValue, FSoundGroupProperties& Current, FSoundGroupProperties& Start, FSoundGroupProperties& End );

	/** 
	 * Set the mode for altering sound group properties
	 */
	void SetSoundGroupMode( FModeSettings& ModeSettings );

	/** 
	 * Adds a component to the audio device
	 */
	void AddComponent( UAudioComponent* AudioComponent );

	/** 
	 * Removes an attached audio component
	 */
	void RemoveComponent( UAudioComponent* AudioComponent );

	/** 
	 * Creates an audio component to handle playing a sound cue
	 */
	static UAudioComponent* CreateComponent( USoundCue* SoundCue, FSceneInterface* Scene, AActor* Actor = NULL, UBOOL Play = TRUE, UBOOL bStopWhenOwnerDestroyed = FALSE, FVector* Location = NULL );

	/** 
	 * Exec
	 */
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar = *GLog );

	/** 
	 * PostEditChange
	 */
	virtual void PostEditChange( UProperty* PropertyThatChanged );

	/** 
	 * Serialize
	 */
	virtual void Serialize( FArchive& Ar );

	/** 
	 * Platform dependent call to init effect data on a sound source
	 */
	virtual void* InitEffect( FSoundSource* Source );

	/** 
	 * Platform dependent call to update the sound output with new parameters
	 */
	virtual void* UpdateEffect( FSoundSource* Source );

	/**
	 * Updates sound group volumes
	 */
	void SetGroupVolume( FName Group, FLOAT Volume );

	/**
	 * Sets a new sound mode and applies it to all appropriate sound groups - Must be defined per platform
	 */
	void SetSoundMode( ESoundMode NewSoundMode );

	/** 
	 * Return the pointer to the sound effects handler
	 */
	class FAudioEffectsManager* GetEffects( void ) 
	{ 
		return( Effects ); 
	}
	
	/** 
	 * Checks to see if a coordinate is within a distance of any listener
	 */
	UBOOL LocationIsAudible( FVector Location, FLOAT MaxDistance );

	/** 
	 * Checks to see if the low pass filter is being tested
	 */
	UBOOL IsTestingLowPassFilter( void ) const 
	{ 
		return( bTestLowPassFilter ); 
	}

	/** 
	 * Checks to see if the EQ filter is being tested
	 */
	UBOOL IsTestingEQFilter( void ) const 
	{ 
		return( bTestEQFilter ); 
	}

protected:
	/** Internal */
	void SortWaveInstances( INT MaxChannels );

	/**
	 * Internal helper function used by ParseSoundGroups to traverse the tree.
	 *
	 * @param CurrentGroup			Subtree to deal with
	 * @param ParentProperties		Propagated properties of parent node
	 */
	void RecurseIntoSoundGroups( FSoundGroup* CurrentGroup, FSoundGroupProperties* ParentProperties );
}