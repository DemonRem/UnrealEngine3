//------------------------------------------------------------------------------
// Manages digital audio.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxDigitalAudio_H__
#define FxDigitalAudio_H__

#include "FxPlatform.h"
#include "FxNamedObject.h"
#include "FxArchive.h"
#include "FxString.h"
#include "FxMemFnCallback.h"

namespace OC3Ent
{

namespace Face
{

// In memory digital audio.  Digital audio objects are named and can be
// serialized to FxArchives.
class FxDigitalAudio : public FxNamedObject
{
	// Declare the class.
	FX_DECLARE_CLASS(FxDigitalAudio, FxNamedObject);
public:
	// Constructor.
	FxDigitalAudio();
	// Memory-based constructor.
	// bitsPerSample must be 8 or 16.
	FxDigitalAudio( FxUInt32 bitsPerSample, FxUInt32 sampleRate, 
					FxUInt32 channelCount,	FxUInt32 sampleCount,
					const FxByte* pSampleBuffer );
	// File-based constructor.
	FxDigitalAudio( const FxString& filename );
	// Copy constructor.
	FxDigitalAudio( const FxDigitalAudio& other );
	// Assignment operator.
	FxDigitalAudio& operator=( const FxDigitalAudio& other );
	// Destructor.
	virtual ~FxDigitalAudio();

#ifdef __UNREAL__
	/**
	 * Creates a FxDigitalAudio object from the passed in USoundNodeWave object.
	 *
	 * @param SoundNodeWave	Wave file to create FxDigitalAudio object from
	 * @return FxDigitalAudio object created from passed in SoundNodeWave or NULL if an error occured
	 */
	static FxDigitalAudio* CreateFromSoundNodeWave( class USoundNodeWave* SoundNodeWave );
#endif

	// Returns FxTrue if the digital audio is valid, FxFalse otherwise.
	FxBool IsValid( void ) const;
	// Returns the bits per sample.
	FxUInt32 GetBitsPerSample( void ) const;
	// Returns the sample rate.
	FxUInt32 GetSampleRate( void ) const;
	// Returns the channel count.
	FxUInt32 GetChannelCount( void ) const;
	// Returns the sample count.
	FxUInt32 GetSampleCount( void ) const;
	// Returns the sample at index.
	FxDReal GetSample( FxSize index ) const;
	// Returns the duration of the digital audio in seconds.
	FxDReal GetDuration( void ) const;
	// Returns a pointer to the sample buffer.
	const FxDReal* GetSampleBuffer( void ) const;
	// Sets the sample buffer.
	void SetSampleBuffer( FxDReal* buffer, FxUInt32 sampleCount );

	// Returns FxTrue if any samples appear to be clipped, FxFalse otherwise.
	FxBool IsClipped( void ) const;
	// Returns the number of possibly clipped samples.
	FxUInt32 GetNumClippedSamples( void ) const;

	// Returns FxTrue if the digital audio should be resampled, FxFalse
	// otherwise.
	FxBool NeedsResample( void ) const;
	// Resamples the audio to either 8 kHz or 16 kHz.  If the original audio
	// sample rate is above 16 kHz, after resampling the audio will be at
	// 16 kHz.  If the original audio sample rate is above 8 kHz but below
	// 16 kHz, after resampling the audio will be at 8 kHz.  In these cases
	// FxTrue is returned.  If the original audio sample rate is below 8 kHz,
	// no resampling takes place and FxFalse is returned.  FxFalse is also
	// returned if the digital audio is not valid.  This routine optionally 
	// takes a pointer to a callback function to receive progress
	// notifications to allow progress bars in client applications.  The 
	// callback function should return nothing and take one FxReal argument
	// which will be the current progress (from zero to one) at the time of
	// the callback.  
	FxBool Resample( FxFunctor* pProgressCallback = NULL );

	// Export the sample buffer.
	// exportBitsPerSample must be 8 or 16.
	// This routine will (possibly) produce a clipped export sample buffer.
	// It is the caller's responsibility to delete the export sample buffer.
	FxByte* ExportSampleBuffer( FxUInt32 exportBitsPerSample ) const;

	// Serialization.
	virtual void Serialize( FxArchive& arc );

	// Class constants.
	static const FxUInt32 kMinimumSampleRate;
	static const FxUInt32 kMaximumSampleRate;
	static const FxUInt32 kMinimumBitsPerSample;
	static const FxUInt32 kMaximumBitsPerSample;
	static const FxUInt32 kRequiredSampleRateLow;
	static const FxUInt32 kRequiredSampleRateHigh;
	// For normalizing 8-bit samples.
	static const FxDReal k8BitSampleShift;
	// Constants from Kaizer's equation for a Kaiser window.
	// (See Hammimg, "Digital Filters.")
	static const FxDReal kStopbandAttenuationLimitDueToBessel;
	static const FxDReal kStopbandAttenuationLimitDueNPoints;
	static const FxDReal kKaiserCoeffForBessel;
	static const FxDReal kKaiserCoeffForNPoints;
	// Constants used in the resampling algorithm.
	// The ratio of the transition band width to the cut-off frequency.
	static const FxDReal kTransitionBandWidthRatio;
	// In dB.
	static const FxDReal kStopbandAttenuation;

protected:
	FxBool	 _isValid;
	FxUInt32 _bitsPerSample;
	FxUInt32 _sampleRate;
	FxUInt32 _channelCount;
	FxUInt32 _sampleCount;
	FxDReal* _pSampleBuffer;

	void _createSampleBuffer( const FxByte* pSampleBuffer );
	
	// Modified Bessel function of the first kind (from Numerical Recipies)
	FxDReal _bessi0( FxDReal x );
};

} // namespace Face

} // namespace OC3Ent

#endif
