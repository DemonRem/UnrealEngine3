/*=============================================================================
	UnAudio.cpp: Unreal base audio.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Minimum HF gain when the low pass filter is applied
#define MIN_HF_GAIN	0.01f

enum EDebugState
{
	DEBUGSTATE_None,
	DEBUGSTATE_IsolateDryAudio,
	DEBUGSTATE_IsolateReverb,
	DEBUGSTATE_IsolateEQ,
	DEBUGSTATE_IsolateRadio,
	DEBUGSTATE_IsolateCenter
};

class FAudioEffect
{
public:
	FAudioEffect( void ) {}
	virtual ~FAudioEffect( void ) {}

	/** Time when this reverb was initiated or completed faded in */
	DOUBLE		Time;
};

class FAudioReverbEffect : public FAudioEffect
{
public:
	/** Sets the default values for a reverb effect */
	FAudioReverbEffect( void );
	/** Sets the platform agnostic parameters */
	FAudioReverbEffect( FLOAT InRoom, 
						FLOAT InRoomHF, 
						FLOAT InRoomRolloffFactor, 
						FLOAT InDecayTime, 
						FLOAT InDecayHFRatio, 
						FLOAT InReflections, 
						FLOAT InReflectionsDelay, 
						FLOAT InReverb, 
						FLOAT InReverbDelay, 
						FLOAT InDiffusion, 
						FLOAT InDensity, 
						FLOAT InAirAbsorption );

	/** Interpolates between Start and Ed reverb effect settings */
	void Interpolate( FLOAT InterpValue, FAudioReverbEffect& Start, FAudioReverbEffect& End );

	/** Overall volume of effect */
	FLOAT		Volume;					// 0.0 to 1.0

	/** Platform agnostic parameters that define a reverb effect. Min < Default < Max */
	FLOAT		Density;				// 0.0 < 1.0 < 1.0
	FLOAT		Diffusion;				// 0.0 < 1.0 < 1.0
	FLOAT		Gain;					// 0.0 < 0.32 < 1.0 
	FLOAT		GainHF;					// 0.0 < 0.89 < 1.0
	FLOAT		DecayTime;				// 0.1 < 1.49 < 20.0	Seconds
	FLOAT		DecayHFRatio;			// 0.1 < 0.83 < 2.0
	FLOAT		ReflectionsGain;		// 0.0 < 0.05 < 3.16
	FLOAT		ReflectionsDelay;		// 0.0 < 0.007 < 0.3	Seconds
	FLOAT		LateGain;				// 0.0 < 1.26 < 10.0
	FLOAT		LateDelay;				// 0.0 < 0.011 < 0.1	Seconds
	FLOAT		AirAbsorptionGainHF;	// 0.892 < 0.994 < 1.0
	FLOAT		RoomRolloffFactor;		// 0.0 < 0.0 < 10.0
	UBOOL		DecayHFLimit;			// FALSE TRUE - TRUE
};

class FAudioEQEffect : public FAudioEffect
{
public:
	/** Sets the default values for a EQ effect */
	FAudioEQEffect( void );
	/** Sets the platform agnostic parameters */
	FAudioEQEffect( FLOAT InHFFrequency, FLOAT InHFGain, FLOAT InMFCutoffFrequency, FLOAT InMFBandwidthFrequency, FLOAT InMFGain, FLOAT InLFFrequency, FLOAT InLFGain ); 

	/** Interpolates between Start and Ed reverb effect settings */
	void Interpolate( FLOAT InterpValue, FAudioEQEffect& Start, FAudioEQEffect& End );

	/** High frequency filter cutoff frequency (Hz) */
	FLOAT		HFFrequency;
	/** High frequency gain */
	FLOAT		HFGain;

	/** Middle frequency filter cutoff frequency (Hz) */
	FLOAT		MFCutoffFrequency;
	/** Middle frequency filter bandwidth frequency (Hz) */
	FLOAT		MFBandwidthFrequency;
	/** Middle frequency filter gain */
	FLOAT		MFGain;

	/** Low frequency filter cutoff frequency (Hz) */
	FLOAT		LFFrequency;			
	/** Low frequency filter gain */
	FLOAT		LFGain;
};

/** 
 * Manager class to handle the interface to various audio effects
 */
class FAudioEffectsManager
{
public:
	FAudioEffectsManager( UAudioDevice* Device );

	virtual ~FAudioEffectsManager( void ) 
	{
	}

	/** 
	 * Feed in new settings to the audio effect system
	 */
	void Update( void );

	/** 
	 * Engine hook to handle setting and fading in of reverb effects
	 */
	void SetReverbSettings( const FReverbSettings& ReverbSettings );

	/** 
	 * Engine hook to handle setting and fading in of EQ effects and group ducking
	 */
	void SetModeSettings( const FModeSettings& ModeSettings );

	/** 
	 * Calls the platform specific code to set the parameters that define reverb
	 */
	virtual void SetReverbEffectParameters( const FAudioReverbEffect& ReverbEffectParameters )
	{
	}

	/** 
	 * Calls the platform specific code to set the parameters that define EQ
	 */
	virtual void SetEQEffectParameters( const FAudioEQEffect& EQEffectParameters ) 
	{
	}

	/** 
	 * Platform dependent call to init effect data on a sound source
	 */
	virtual void* InitEffect( FSoundSource* Source )
	{
		return( NULL ); 
	}

	/** 
	 * Platform dependent call to update the sound output with new parameters
	 */
	virtual void* UpdateEffect( class FSoundSource* Source ) 
	{ 
		return( NULL ); 
	}

	/**
	 * Converts and volume (0.0f to 1.0f) to a deciBel value
	 */
	LONG VolumeToDeciBels( FLOAT Volume );

	/**
	 * Converts and volume (0.0f to 1.0f) to a MilliBel value (a Hundredth of a deciBel)
	 */
	LONG VolumeToMilliBels( FLOAT Volume );

	/** 
	 * Set volume levels for debugging purposes
	 */
	void SetMixDebugState( EDebugState DebugState );

	/** 
	 * Returns volumes of 0 or 1 depending on the mix settings
	 */
	void GetVolumes( FLOAT& DryVolume, FLOAT& ReverbVolume, FLOAT& EQVolume, FLOAT& RadioVolume, FLOAT& CenterVolume );

protected:
	/** 
	 * Helper function to interpolate between different effects
	 */
	void Interpolate( FAudioReverbEffect& Current, FAudioReverbEffect& Start, FAudioReverbEffect& End );
	void Interpolate( FAudioEQEffect& Current, FAudioEQEffect& Start, FAudioEQEffect& End );

	UAudioDevice *			Device;

	EDebugState				DebugState;

	ReverbPreset			CurrentReverbType;

	FAudioReverbEffect		SourceReverbEffect;
	FAudioReverbEffect		CurrentReverbEffect;
	FAudioReverbEffect		DestinationReverbEffect;

	ESoundMode				CurrentMode;

	FAudioEQEffect			SourceEQEffect;
	FAudioEQEffect			CurrentEQEffect;
	FAudioEQEffect			DestinationEQEffect;

	/** The parameters that define the reverb presets */
	static	FAudioReverbEffect ReverbPresets[REVERB_MAX];

	/** The parameters that define the EQ presets */
	static	FAudioEQEffect EQPresets[SOUNDMODE_MAX];
};

// end 
