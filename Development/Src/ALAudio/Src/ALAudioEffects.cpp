/*=============================================================================
	ALAudioEffects.cpp: Unreal OpenAL Audio interface object.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.

=============================================================================*/

/*------------------------------------------------------------------------------------
	Audio includes.
------------------------------------------------------------------------------------*/

#include "ALAudioPrivate.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack( push, 8 )
#endif

#include <efx.h>

#if SUPPORTS_PRAGMA_PACK
#pragma pack( pop )
#endif

/*------------------------------------------------------------------------------------
	FALAudioEffectsManager.
------------------------------------------------------------------------------------*/

FALAudioEffectsManager::FALAudioEffectsManager( UAudioDevice* InDevice )
	: FAudioEffectsManager( InDevice )
{
	UBOOL ErrorsFound = FALSE;

	MaxEffectSlots = 0;
	MaxSends = 0;
	ReverbEffect = AL_EFFECT_NULL;
	LowPassFilter = AL_FILTER_NULL;

	UALAudioDevice* AudioDevice = ( UALAudioDevice* )InDevice;

	SUPPORTS_EFX = FALSE;
	AudioDevice->FindProcs( TRUE );		
	if( SUPPORTS_EFX )
	{
		for( MaxEffectSlots = 0; MaxEffectSlots < MAX_EFFECT_SLOTS; MaxEffectSlots++ )
		{
			alGenAuxiliaryEffectSlots( 1, EffectSlots + MaxEffectSlots );
			if( AudioDevice->alError( TEXT( "Init (creating aux effect slots)" ), 0 ) )
			{
				break;
			}
		}

		alcGetIntegerv( AudioDevice->GetDevice(), ALC_MAX_AUXILIARY_SENDS, sizeof( MaxSends ), &MaxSends );

		debugf( NAME_Init, TEXT( "Found EFX extension with %d effect slots and %d potential sends" ), MaxEffectSlots, MaxSends );

		// Check for supported EFX features that are useful
		alGenEffects( 1, &ReverbEffect );
		if( alIsEffect( ReverbEffect ) )
		{
			alEffecti( ReverbEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB );
		}

		debugf( NAME_Init, TEXT( "...'reverb' %ssupported" ), AudioDevice->alError( TEXT( "" ), 0 ) ? TEXT( "un" ) : TEXT( "" ) );

		if( MaxEffectSlots > 1 )
		{
			alGenEffects( 1, &EQEffect );
			if( alIsEffect( EQEffect ) )
			{
				alEffecti( EQEffect, AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER );
			}

			debugf( NAME_Init, TEXT( "...'equalizer' %ssupported" ), AudioDevice->alError( TEXT( "" ), 0 ) ? TEXT( "un" ) : TEXT( "" ) );
		}

		// Generate a low pass filter effect
		alGenFilters( 1, &LowPassFilter );
		if( alIsFilter( LowPassFilter ) )
		{
			alFilteri( LowPassFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS );

			alFilterf( LowPassFilter, AL_LOWPASS_GAIN, 1.0f );
			alFilterf( LowPassFilter, AL_LOWPASS_GAINHF, 0.0f );
		}

		debugf( NAME_Init, TEXT( "...'low pass filter' %ssupported" ), AudioDevice->alError( TEXT( "" ), 0 ) ? TEXT( "un" ) : TEXT( "" ) );

		alAuxiliaryEffectSloti( EffectSlots[0], AL_EFFECTSLOT_EFFECT, ReverbEffect );
		alAuxiliaryEffectSloti( EffectSlots[0], AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, AL_TRUE );

		if( MaxEffectSlots > 1 )
		{
			alAuxiliaryEffectSloti( EffectSlots[1], AL_EFFECTSLOT_EFFECT, EQEffect );
			alAuxiliaryEffectSloti( EffectSlots[1], AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, AL_TRUE );
		}

		ErrorsFound |= AudioDevice->alError( TEXT( "Generating effect slots" ) );
	}

	if( ErrorsFound || MaxEffectSlots < 1 || MaxSends < 1 )
	{
		MaxEffectSlots = 0;
		MaxSends = 0;
		debugf( NAME_Init, TEXT( "...failed to initialise EFX (reverb and low pass filter)" ) );
	}
}

FALAudioEffectsManager::~FALAudioEffectsManager( void )
{
	if( MaxEffectSlots > 0 )
	{
		for( INT i = 0; i < MaxEffectSlots; i++ )
		{
			alAuxiliaryEffectSloti( EffectSlots[i], AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL );
			alAuxiliaryEffectSloti( EffectSlots[i], AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, AL_FALSE );
		}

		if( alIsEffect( ReverbEffect ) )
		{
			alDeleteEffects( 1, &ReverbEffect );
		}

		if( MaxEffectSlots > 1 )
		{
			if( alIsEffect( EQEffect ) )
			{
				alDeleteEffects( 1, &EQEffect );
			}
		}

		if( alIsFilter( LowPassFilter ) )
		{
			alDeleteFilters( 1, &LowPassFilter );
		}

		alDeleteAuxiliaryEffectSlots( MaxEffectSlots, EffectSlots );
	}
}

void FALAudioEffectsManager::SetReverbEffectParameters( const FAudioReverbEffect& ReverbEffectParameters )
{
	if( MaxEffectSlots > 0 )
	{
		alEffectf( ReverbEffect, AL_REVERB_DENSITY, ReverbEffectParameters.Density );
		alEffectf( ReverbEffect, AL_REVERB_DIFFUSION, ReverbEffectParameters.Diffusion );
		alEffectf( ReverbEffect, AL_REVERB_GAIN, ReverbEffectParameters.Gain );
		alEffectf( ReverbEffect, AL_REVERB_GAINHF, ReverbEffectParameters.GainHF );
		alEffectf( ReverbEffect, AL_REVERB_DECAY_TIME, ReverbEffectParameters.DecayTime );
		alEffectf( ReverbEffect, AL_REVERB_DECAY_HFRATIO, ReverbEffectParameters.DecayHFRatio );
		alEffectf( ReverbEffect, AL_REVERB_REFLECTIONS_GAIN, ReverbEffectParameters.ReflectionsGain );
		alEffectf( ReverbEffect, AL_REVERB_REFLECTIONS_DELAY, ReverbEffectParameters.ReflectionsDelay );
		alEffectf( ReverbEffect, AL_REVERB_LATE_REVERB_GAIN, ReverbEffectParameters.LateGain );
		alEffectf( ReverbEffect, AL_REVERB_LATE_REVERB_DELAY, ReverbEffectParameters.LateDelay );
		alEffectf( ReverbEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, ReverbEffectParameters.AirAbsorptionGainHF );
		alEffectf( ReverbEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, ReverbEffectParameters.RoomRolloffFactor );
		alEffecti( ReverbEffect, AL_REVERB_DECAY_HFLIMIT, ReverbEffectParameters.DecayHFLimit );

		alAuxiliaryEffectSlotf( EffectSlots[0], AL_EFFECTSLOT_GAIN, ReverbEffectParameters.Volume );
		alAuxiliaryEffectSloti( EffectSlots[0], AL_EFFECTSLOT_EFFECT, ReverbEffect );
	}

}

void FALAudioEffectsManager::SetEQEffectParameters( const FAudioEQEffect& EQEffectParameters )
{
	if( MaxEffectSlots > 1 )
	{
		alEffectf( EQEffect, AL_EQUALIZER_MID1_GAIN, EQEffectParameters.LFGain );
		alEffectf( EQEffect, AL_EQUALIZER_MID1_CENTER, EQEffectParameters.LFFrequency );
		alEffectf( EQEffect, AL_EQUALIZER_MID1_WIDTH, 1.0f );

		alEffectf( EQEffect, AL_EQUALIZER_MID2_GAIN, EQEffectParameters.MFGain );
		alEffectf( EQEffect, AL_EQUALIZER_MID2_CENTER, EQEffectParameters.MFCutoffFrequency );
		alEffectf( EQEffect, AL_EQUALIZER_MID2_WIDTH, EQEffectParameters.MFBandwidthFrequency / EQEffectParameters.MFCutoffFrequency );

		alEffectf( EQEffect, AL_EQUALIZER_HIGH_GAIN, EQEffectParameters.HFGain );
		alEffectf( EQEffect, AL_EQUALIZER_HIGH_CUTOFF, EQEffectParameters.HFFrequency );

		alAuxiliaryEffectSlotf( EffectSlots[1], AL_EFFECTSLOT_GAIN, 1.0f );
		alAuxiliaryEffectSloti( EffectSlots[1], AL_EFFECTSLOT_EFFECT, EQEffect );
	}
}

/** 
 * Sets the output of the source to send to the effect (reverb, low pass filter and EQ)
 */
void* FALAudioEffectsManager::LinkEffect( FSoundSource* Source )
{
	if( MaxEffectSlots > 0 )
	{
		FALSoundSource* ALSource = static_cast< FALSoundSource* >( Source );

		// Set up the filter if requested
		ALuint DryFilter = AL_FILTER_NULL;
		ALuint ReverbFilter = AL_FILTER_NULL;
		ALuint EQFilter = AL_FILTER_NULL;
		if( Source->IsLowPassFilterApplied() )
		{
			DryFilter = LowPassFilter;
			ReverbFilter = LowPassFilter;
		}

		// Set up the reverb effect
		ALuint ReverbEffectSlot = AL_EFFECTSLOT_NULL;
		if( Source->IsReverbApplied() )
		{
			ReverbEffectSlot = EffectSlots[0];
		}

		// Set up the reverb effect
		ALuint EQEffectSlot = AL_EFFECTSLOT_NULL;
		if( ( Source->IsEQApplied() || Device->IsTestingEQFilter() ) && MaxEffectSlots > 1 )
		{
			EQEffectSlot = EffectSlots[1];

			// Kill the direct path so as all output is EQ'd
			alFilterf( LowPassFilter, AL_LOWPASS_GAIN, 0.0f );
			DryFilter = LowPassFilter;
		}

		switch( DebugState )
		{
		case DEBUGSTATE_IsolateDryAudio:
			DryFilter = AL_FILTER_NULL;
			ReverbEffectSlot = AL_EFFECTSLOT_NULL;
			EQEffectSlot = AL_EFFECTSLOT_NULL;
			break;
			
		case DEBUGSTATE_IsolateReverb:
			alFilterf( LowPassFilter, AL_LOWPASS_GAIN, 0.0f );
			DryFilter = LowPassFilter;
			ReverbFilter = AL_FILTER_NULL;
			EQEffectSlot = AL_EFFECTSLOT_NULL;
			break;

		case DEBUGSTATE_IsolateEQ:
			alFilterf( LowPassFilter, AL_LOWPASS_GAIN, 0.0f );
			DryFilter = LowPassFilter;
			ReverbEffectSlot = AL_EFFECTSLOT_NULL;
			EQFilter = AL_FILTER_NULL;
			break;
		}

		// Apply filter to dry signal
		alSourcei( ALSource->GetSourceId(), AL_DIRECT_FILTER, DryFilter );

		// Apply effect and the filter to the wet signal
		alSource3i( ALSource->GetSourceId(), AL_AUXILIARY_SEND_FILTER, ReverbEffectSlot, 0, ReverbFilter );

		if( MaxEffectSlots > 1 )
		{
			alSource3i( ALSource->GetSourceId(), AL_AUXILIARY_SEND_FILTER, EQEffectSlot, 1, EQFilter );
		}
	}

	return( NULL );
}

// end
