/*=============================================================================
	UnAudio.cpp: Unreal base audio.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h" 
#include "EngineAudioDeviceClasses.h"
#include "UnAudioEffect.h"

/** 
 * Default settings for a null reverb effect
 */
FAudioReverbEffect::FAudioReverbEffect( void )
{
	Time = 0.0;
	Volume = 0.0f;

	Density = 1.0f;					
	Diffusion = 1.0f;				
	Gain = 0.32f;					
	GainHF = 0.89f;					
	DecayTime = 1.49f;				
	DecayHFRatio = 0.83f;			
	ReflectionsGain = 0.05f;		
	ReflectionsDelay = 0.007f;		
	LateGain = 1.26f;				
	LateDelay = 0.011f;				
	AirAbsorptionGainHF = 0.994f;	
	RoomRolloffFactor = 0.0f;		
	DecayHFLimit = TRUE;			
}

/** 
 * Construct generic reverb settings based in the I3DL2 standards
 */
FAudioReverbEffect::FAudioReverbEffect( FLOAT InRoom, 
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
								 		FLOAT InAirAbsorption )
{
	Time = 0.0;
	Volume = 0.0f;

	Density = InDensity;
	Diffusion = InDiffusion;
	Gain = InRoom;
	GainHF = InRoomHF;
	DecayTime = InDecayTime;
	DecayHFRatio = InDecayHFRatio;
	ReflectionsGain = InReflections;
	ReflectionsDelay = InReflectionsDelay;
	LateGain = InReverb;
	LateDelay = InReverbDelay;
	RoomRolloffFactor = InRoomRolloffFactor;
	AirAbsorptionGainHF = InAirAbsorption;
	DecayHFLimit = TRUE;
}

/** 
 * Get interpolated reverb parameters
 */
void FAudioReverbEffect::Interpolate( FLOAT InterpValue, FAudioReverbEffect& Start, FAudioReverbEffect& End )
{
	FLOAT InvInterpValue = 1.0f - InterpValue;

	Volume = ( Start.Volume * InvInterpValue ) + ( End.Volume * InterpValue );
	Density = ( Start.Density * InvInterpValue ) + ( End.Density * InterpValue );				
	Diffusion = ( Start.Diffusion * InvInterpValue ) + ( End.Diffusion * InterpValue );				
	Gain = ( Start.Gain * InvInterpValue ) + ( End.Gain * InterpValue );					
	GainHF = ( Start.GainHF * InvInterpValue ) + ( End.GainHF * InterpValue );					
	DecayTime = ( Start.DecayTime * InvInterpValue ) + ( End.DecayTime * InterpValue );				
	DecayHFRatio = ( Start.DecayHFRatio * InvInterpValue ) + ( End.DecayHFRatio * InterpValue );			
	ReflectionsGain = ( Start.ReflectionsGain * InvInterpValue ) + ( End.ReflectionsGain * InterpValue );		
	ReflectionsDelay = ( Start.ReflectionsDelay * InvInterpValue ) + ( End.ReflectionsDelay * InterpValue );		
	LateGain = ( Start.LateGain * InvInterpValue ) + ( End.LateGain * InterpValue );				
	LateDelay = ( Start.LateDelay * InvInterpValue ) + ( End.LateDelay * InterpValue );				
	AirAbsorptionGainHF = ( Start.AirAbsorptionGainHF * InvInterpValue ) + ( End.AirAbsorptionGainHF * InterpValue );	
	RoomRolloffFactor = ( Start.RoomRolloffFactor * InvInterpValue ) + ( End.RoomRolloffFactor * InterpValue );		
	/** Turn on halfway through the interpolation */
	DecayHFLimit = ( InterpValue > 0.5f );
}

/** 
 * Reverb preset table - may be exposed in the future
 * 
 * Room - Reverb Gain - 0.0 < x < 1.0 - overall reverb gain - master volume control
 * RoomHF - Reverb Gain High Frequency - 0.0 < x < 1.0 - attenuates the high frequeny reflected sound
 * Rolloff - Room Rolloff - 0.0 < x < 10.0 - multiplies the attenuation due to distance
 * Decay - Decay Time - 0.1 < x < 20.0 - larger is more reverb
 * DecayHF - Decay High Frequency Ratio - 0.1 < x < 2.0 - how much the quicker or slower the high frequencies decay relative to the lower frequencies.
 * Ref - Reflections Gain - 0.0 < x < 3.16 - controls the amount of initial reflections
 * RefDel - Reflections Delay - 0.0 < x < 0.3 - the time between the listener receiving the direct path sound and the first reflection
 * Rev - Late Reverb Gain - 0.0 < x < 10.0 - gain of the late reverb
 * RevDel - Late Reverb Delay - 0.0 < x 0.1 - time difference between late reverb and first reflections
 * Diff - Diffusion - 0.0 < x < 1.0 - Echo density in the reverberation decay - lower is more grainy
 * Dens - Density - 0.0 < x < 1.0 - Coloration of the late reverb - lower value is more
 * AA - Air Absorption - 0.892 < x < 1.0 - lower value means more absorption
 */
FAudioReverbEffect FAudioEffectsManager::ReverbPresets[REVERB_MAX] =
{
	//					Room	 RoomHF	  Rolloff Decay DecayHF Ref     RefDel  Rev      RevDel  Diff   Dens   AA
	FAudioReverbEffect( 0.0000f, 0.0000f, 0.00f, 1.00f, 0.50f, 0.0000f, 0.020f, 0.0000f, 0.040f, 1.00f, 1.00f, 1.000f ),	// DEFAULT

	FAudioReverbEffect( 0.6666f, 0.2511f, 0.00f, 1.49f, 0.54f, 0.6531f, 0.007f, 0.3055f, 0.011f, 1.00f, 0.60f, 0.950f ),	// BATHROOM
	FAudioReverbEffect( 0.3333f, 0.7079f, 0.00f, 2.31f, 0.64f, 0.4411f, 0.012f, 0.9089f, 0.017f, 1.00f, 1.00f, 0.994f ),	// STONEROOM
	FAudioReverbEffect( 0.6666f, 0.5781f, 0.00f, 4.32f, 0.59f, 0.4032f, 0.020f, 0.7170f, 0.030f, 1.00f, 1.00f, 0.960f ),	// AUDITORIUM
	FAudioReverbEffect( 0.3000f, 0.5623f, 0.00f, 3.92f, 0.70f, 0.2427f, 0.020f, 0.9977f, 0.029f, 1.00f, 1.00f, 0.994f ),	// CONCERTHALL
	FAudioReverbEffect( 0.5333f, 1.0000f, 0.00f, 2.91f, 1.30f, 0.5000f, 0.015f, 0.7063f, 0.022f, 1.00f, 1.00f, 0.960f ),	// CAVE
	FAudioReverbEffect( 0.4000f, 0.7079f, 0.00f, 1.49f, 0.59f, 0.2458f, 0.007f, 0.6019f, 0.011f, 1.00f, 1.00f, 0.994f ),	// HALLWAY
	FAudioReverbEffect( 0.5000f, 0.7612f, 0.00f, 2.70f, 0.79f, 0.2472f, 0.013f, 0.6346f, 0.020f, 1.00f, 1.00f, 0.990f ),	// STONECORRIDOR
	FAudioReverbEffect( 0.6333f, 0.7328f, 0.00f, 1.49f, 0.86f, 0.2500f, 0.007f, 0.9954f, 0.011f, 1.00f, 1.00f, 0.970f ),	// ALLEY
	FAudioReverbEffect( 1.0000f, 0.0224f, 0.00f, 1.49f, 0.54f, 0.0525f, 0.162f, 0.4937f, 0.088f, 0.79f, 1.00f, 0.940f ),	// FOREST
	FAudioReverbEffect( 1.0000f, 0.3981f, 0.00f, 1.49f, 0.67f, 0.0730f, 0.007f, 0.0779f, 0.011f, 0.50f, 1.00f, 0.970f ),	// CITY
	FAudioReverbEffect( 1.0000f, 0.0562f, 0.00f, 1.49f, 0.21f, 0.0407f, 0.300f, 0.0984f, 0.100f, 0.27f, 1.00f, 0.960f ),	// MOUNTAINS
	FAudioReverbEffect( 1.0000f, 0.3162f, 0.00f, 1.49f, 0.83f, 0.0000f, 0.061f, 0.5623f, 0.025f, 1.00f, 1.00f, 0.960f ),	// QUARRY
	FAudioReverbEffect( 1.0000f, 0.1000f, 0.00f, 1.49f, 0.50f, 0.0585f, 0.179f, 0.0553f, 0.100f, 0.21f, 1.00f, 1.000f ),	// PLAIN
	FAudioReverbEffect( 0.8000f, 1.0000f, 0.00f, 1.65f, 1.50f, 0.2082f, 0.008f, 0.2652f, 0.012f, 1.00f, 1.00f, 0.970f ),	// PARKINGLOT
	FAudioReverbEffect( 0.6666f, 0.3162f, 0.00f, 2.81f, 0.14f, 1.6387f, 0.014f, 0.4742f, 0.021f, 0.80f, 0.60f, 0.940f ),	// SEWERPIPE
	FAudioReverbEffect( 1.0000f, 0.0100f, 0.00f, 1.49f, 0.10f, 0.5963f, 0.007f, 0.1412f, 0.011f, 1.00f, 1.00f, 1.000f ),	// UNDERWATER
	FAudioReverbEffect( 0.6666f, 0.5011f, 0.00f, 1.10f, 0.83f, 0.6310f, 0.005f, 0.5623f, 0.010f, 1.00f, 1.00f, 0.994f ),	// SMALLROOM
	FAudioReverbEffect( 0.6666f, 0.5011f, 0.00f, 1.30f, 0.83f, 0.3162f, 0.010f, 0.7943f, 0.020f, 1.00f, 1.00f, 0.994f ),	// MEDIUMROOM
	FAudioReverbEffect( 1.0000f, 0.5011f, 0.00f, 1.50f, 0.83f, 0.1585f, 0.020f, 0.3162f, 0.040f, 1.00f, 1.00f, 0.994f ),	// LARGEROOM
	FAudioReverbEffect( 0.8666f, 0.5011f, 0.00f, 1.80f, 0.70f, 0.2239f, 0.015f, 0.3981f, 0.030f, 1.00f, 1.00f, 0.994f ),	// MEDIUMHALL
	FAudioReverbEffect( 1.0000f, 0.5011f, 0.00f, 1.80f, 0.70f, 0.1000f, 0.030f, 0.1885f, 0.060f, 1.00f, 1.00f, 0.994f ),	// LARGEHALL
	FAudioReverbEffect( 0.6666f, 0.7943f, 0.00f, 1.30f, 0.90f, 1.0000f, 0.002f, 1.0000f, 0.010f, 1.00f, 0.75f, 1.000f )		// PLATE
};																													 

/** 
 * Default settings for a NULL EQ effect
 */
FAudioEQEffect::FAudioEQEffect( void )
{
	Time = 0.0;

	HFFrequency = 12000.0f;
	HFGain = 1.0f;
	MFCutoffFrequency = 8000.0f;
	MFBandwidthFrequency = 1000.0f;
	MFGain = 1.0f;
	LFFrequency = 2000.0f;
	LFGain = 1.0f;
}

FAudioEQEffect::FAudioEQEffect( FLOAT InHFFrequency, FLOAT InHFGain, FLOAT InMFCutoffFrequency, FLOAT InMFBandwidthFrequency, FLOAT InMFGain, FLOAT InLFFrequency, FLOAT InLFGain )
{
	Time = 0.0;

	HFFrequency = InHFFrequency;
	HFGain = InHFGain;
	MFCutoffFrequency = InMFCutoffFrequency;
	MFBandwidthFrequency = InMFBandwidthFrequency;
	MFGain = InMFGain;
	LFFrequency = InLFFrequency;
	LFGain = InLFGain;
}

void FAudioEQEffect::Interpolate( FLOAT InterpValue, FAudioEQEffect& Start, FAudioEQEffect& End )
{
	FLOAT InvInterpValue = 1.0f - InterpValue;
	
	HFFrequency = ( Start.HFFrequency * InvInterpValue ) + ( End.HFFrequency * InterpValue );
	HFGain = ( Start.HFGain * InvInterpValue ) + ( End.HFGain * InterpValue );
	MFCutoffFrequency = ( Start.MFCutoffFrequency * InvInterpValue ) + ( End.MFCutoffFrequency * InterpValue );
	MFBandwidthFrequency = ( Start.MFBandwidthFrequency * InvInterpValue ) + ( End.MFBandwidthFrequency * InterpValue );
	MFGain = ( Start.MFGain * InvInterpValue ) + ( End.MFGain * InterpValue );
	LFFrequency = ( Start.LFFrequency * InvInterpValue ) + ( End.LFFrequency * InterpValue );
	LFGain = ( Start.LFGain * InvInterpValue ) + ( End.LFGain * InterpValue );
}

FAudioEQEffect FAudioEffectsManager::EQPresets[] =
{
	FAudioEQEffect( 12000.0f, 1.0f, 8000.0f, 1000.0f, 1.0f, 2000.0f, 1.0f ),			// SOUNDMODE_NORMAL

	FAudioEQEffect( 6000.0f, 0.1f, 4000.0f, 4000.0f, 0.1f, 2400.0f, 2.0f ),				// SOUNDMODE_DEATH
	FAudioEQEffect( 11500.0f, 2.0f, 2080.0f, 1700.0f, 0.38f, 1300.0f, 2.0f ),			// SOUNDMODE_COVER
	FAudioEQEffect( 9400.0f, 0.25f, 2400.0f, 1800.0f, 0.73f, 1400.0f, 2.0f ),			// SOUNDMODE_ROADIE_RUN
	FAudioEQEffect( 10000.0f, 0.30f, 5700.0f, 2800.0f, 0.30f, 3000.0f, 0.30f ),			// SOUNDMODE_TACCOM
	FAudioEQEffect( 10000.0f, 0.1f, 2000.0f, 633.0f, 2.0f, 500.0f, 0.1f ),				// SOUNDMODE_RADIO
};

/** 
 * Set volume levels for debugging purposes
 */
void FAudioEffectsManager::SetMixDebugState( EDebugState InDebugState )
{
	DebugState = InDebugState;
}

/** 
 * Returns volumes of 0 or 1 depending on the mix settings
 */
void FAudioEffectsManager::GetVolumes( FLOAT& DryVolume, FLOAT& ReverbVolume, FLOAT& EQVolume, FLOAT& RadioVolume, FLOAT& CenterVolume )
{
	if( DebugState == DEBUGSTATE_None )
	{
		DryVolume = 1.0f;
		ReverbVolume = 1.0f;
		EQVolume = 1.0f;
		RadioVolume = 1.0f;
		CenterVolume = 1.0f;
	}
	else
	{
		DryVolume = 0.0f;
		ReverbVolume = 0.0f;
		EQVolume = 0.0f;
		RadioVolume = 0.0f;
		CenterVolume = 0.0f;

		switch( DebugState )
		{
		case DEBUGSTATE_IsolateDryAudio:
			DryVolume = 1.0f;
			break;

		case DEBUGSTATE_IsolateReverb:
			ReverbVolume = 1.0f;
			break;

		case DEBUGSTATE_IsolateEQ:
			EQVolume = 1.0f;
			break;

		case DEBUGSTATE_IsolateRadio:
			RadioVolume = 1.0f;
			break;

		case DEBUGSTATE_IsolateCenter:
			CenterVolume = 1.0f;
			break;	
		}
	}
}

/**
 * Converts and volume (0.0f to 1.0f) to a deciBel value
 */
LONG FAudioEffectsManager::VolumeToDeciBels( FLOAT Volume )
{
	if( Volume > 0.0f )
	{
		return( ( LONG )( 20.0f * log10f( Volume ) ) );
	}

	return( -100 );
}


/**
 * Converts and volume (0.0f to 1.0f) to a MilliBel value (a Hundredth of a deciBel)
 */
LONG FAudioEffectsManager::VolumeToMilliBels( FLOAT Volume )
{
	if( Volume > 0.0f )
	{
		return( ( LONG )( 2000.0f * log10f( Volume ) ) );
	}

	return( -10000 );
}

/** 
 * Gets the parameters for reverb based on settings and time
 */
void FAudioEffectsManager::Interpolate( FAudioReverbEffect& Current, FAudioReverbEffect& Start, FAudioReverbEffect& End )
{
	FLOAT InterpValue = 1.0f;
	if( End.Time - Start.Time > 0.0 )
	{
		InterpValue = ( FLOAT )( ( appSeconds() - Start.Time ) / ( End.Time - Start.Time ) );
	}

	if( InterpValue > 1.0f )
	{
		Current = End;
		return;
	}

	if( InterpValue < 0.0f )
	{
		Current = Start;
		return;
	}

	Current.Interpolate( InterpValue, Start, End );
}

/** 
 * Gets the parameters for EQ based on settings and time
 */
void FAudioEffectsManager::Interpolate( FAudioEQEffect& Current, FAudioEQEffect& Start, FAudioEQEffect& End )
{
	FLOAT InterpValue = 1.0f;
	if( End.Time - Start.Time > 0.0 )
	{
		InterpValue = ( FLOAT )( ( appSeconds() - Start.Time ) / ( End.Time - Start.Time ) );
	}

	if( InterpValue > 1.0f )
	{
		Current = End;
		return;
	}

	if( InterpValue < 0.0f )
	{
		Current = Start;
		return;
	}

	Current.Interpolate( InterpValue, Start, End );
}

/** 
 * Clear out any reverb and EQ settings
 */
FAudioEffectsManager::FAudioEffectsManager( UAudioDevice* InDevice )
{
	Device = InDevice;

	// Default mix settings
	DebugState = DEBUGSTATE_None;
	
	// Clear out the default reverb settings
	FReverbSettings ReverbSettings;
	ReverbSettings.ReverbType = REVERB_Default;
	ReverbSettings.Volume = 0.0f;
	ReverbSettings.FadeTime = 0.1f;
	CurrentReverbType = REVERB_SmallRoom;
	SetReverbSettings( ReverbSettings );

	FModeSettings ModeSettings;
	ModeSettings.Mode = 0;
	ModeSettings.FadeTime = 0.1f;
	CurrentMode = SOUNDMODE_DEATH;
	SetModeSettings( ModeSettings );
}

/**
 * Called every tick from UGameViewportClient::Draw
 * 
 * Sets new reverb mode if necessary. Otherwise interpolates to the current settings and calls SetEffect to handle
 * the platform specific aspect.
 */
void FAudioEffectsManager::SetReverbSettings( const FReverbSettings& ReverbSettings )
{
	/** Update the settings if the reverb type has changed */
	if( ReverbSettings.ReverbType != CurrentReverbType )
	{
		debugfSuppressed( NAME_DevAudio, TEXT( "UAudioDevice::SetReverbSettings(): Old - %i  New - %i:%f(%f)" ),
			( INT )CurrentReverbType, ReverbSettings.ReverbType, ReverbSettings.Volume, ReverbSettings.FadeTime );

		SourceReverbEffect = CurrentReverbEffect;
		SourceReverbEffect.Time = appSeconds();

		DestinationReverbEffect = ReverbPresets[ReverbSettings.ReverbType];
		DestinationReverbEffect.Time = SourceReverbEffect.Time + ReverbSettings.FadeTime;
		DestinationReverbEffect.Volume = ReverbSettings.Volume;
		if( ReverbSettings.ReverbType == REVERB_Default )
		{
			DestinationReverbEffect.Volume = 0.0f;
		}

		CurrentReverbType = ( ReverbPreset )ReverbSettings.ReverbType;
	}
}

/**
 * Sets new EQ mode if necessary. Otherwise interpolates to the current settings and calls SetEffect to handle
 * the platform specific aspect.
 */
void FAudioEffectsManager::SetModeSettings( const FModeSettings& ModeSettings )
{
	if( ModeSettings.Mode != CurrentMode )
	{
		debugfSuppressed( NAME_DevAudio, TEXT( "UAudioDevice::SetEQSettings(): Old - %i  New - %i:%f" ),
			( INT )CurrentMode, ModeSettings.Mode, ModeSettings.FadeTime );

		SourceEQEffect = CurrentEQEffect;
		SourceEQEffect.Time = appSeconds();

		DestinationEQEffect = EQPresets[ModeSettings.Mode];
		DestinationEQEffect.Time = SourceEQEffect.Time + ModeSettings.FadeTime;

		CurrentMode = ( ESoundMode )ModeSettings.Mode;
	}
}

/** 
 * Feed in new settings to the audio effect system
 */
void FAudioEffectsManager::Update( void )
{
	/** Interpolate the settings depending on time */
	Interpolate( CurrentReverbEffect, SourceReverbEffect, DestinationReverbEffect );

	/** Call the platform specific code to set the effect */
	SetReverbEffectParameters( CurrentReverbEffect );

	/** Interpolate the settings depending on time */
	Interpolate( CurrentEQEffect, SourceEQEffect, DestinationEQEffect );

	/** Call the platform specific code to set the effect */
	SetEQEffectParameters( CurrentEQEffect );
}

// end 
