//------------------------------------------------------------------------------
// Manages digital audio.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include "FxMath.h"
#include "FxDigitalAudio.h"
#include "FxAudioFile.h"


#ifdef __UNREAL__
	// In Unreal, the audio is not stored in the session files so there is no
	// need to include the compression code.
	#include "Engine.h"
#else
	// FxDigitalAudio objects are compressed and uncompressed during 
	// serialization to save memory.
	#include "zlib.h"
#endif

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxDigitalAudioVersion 0

FX_IMPLEMENT_CLASS(FxDigitalAudio,kCurrentFxDigitalAudioVersion,FxNamedObject);

// Class constants.
const FxUInt32 FxDigitalAudio::kMinimumSampleRate      =  8000;
const FxUInt32 FxDigitalAudio::kMaximumSampleRate      = 48000;
const FxUInt32 FxDigitalAudio::kMinimumBitsPerSample   = 8;
const FxUInt32 FxDigitalAudio::kMaximumBitsPerSample   = 16;
const FxUInt32 FxDigitalAudio::kRequiredSampleRateLow  =  8000;
const FxUInt32 FxDigitalAudio::kRequiredSampleRateHigh = 16000;
// For normalizing 8-bit samples.
const FxDReal FxDigitalAudio::k8BitSampleShift = -SCHAR_MIN;
// Constants from Kaizer's equation for a Kaiser window.
// (See Hammimg, "Digital Filters.")
const FxDReal FxDigitalAudio::kStopbandAttenuationLimitDueToBessel = 8.71;
const FxDReal FxDigitalAudio::kStopbandAttenuationLimitDueNPoints = 7.95;
const FxDReal FxDigitalAudio::kKaiserCoeffForBessel = 0.1102;
const FxDReal FxDigitalAudio::kKaiserCoeffForNPoints = 28.72;
// Constants used in the resampling algorithm.
// These constants are a pretty good approximation to the filtering done 
// in CoolEdit.
// The ratio of the transition band width to the cut-off frequency.
const FxDReal FxDigitalAudio::kTransitionBandWidthRatio = 1.0 / 16.0;
// In dB.
const FxDReal FxDigitalAudio::kStopbandAttenuation = 80.0;

FxDigitalAudio::FxDigitalAudio()
	: _isValid(FxFalse)
	, _bitsPerSample(0)
	, _sampleRate(0)
	, _channelCount(0)
	, _sampleCount(0)
	, _pSampleBuffer(NULL)
{
}

// Memory-based constructor.
FxDigitalAudio::FxDigitalAudio( FxUInt32 bitsPerSample, FxUInt32 sampleRate, 
							    FxUInt32 channelCount, FxUInt32 sampleCount,
				                const FxByte* pSampleBuffer )
								: _isValid(FxFalse)
								, _bitsPerSample(bitsPerSample)
								, _sampleRate(sampleRate)
								, _channelCount(channelCount)
								, _sampleCount(sampleCount)
								, _pSampleBuffer(NULL)
{
	// Make sure the values are valid.
	if( (bitsPerSample == kMinimumBitsPerSample  || 
		 bitsPerSample == kMaximumBitsPerSample) &&
		 sampleRate >= kMinimumSampleRate        &&
		 sampleRate <= kMaximumSampleRate        &&
		 pSampleBuffer != NULL                   && 
		 sampleCount > 0 )
	{
		// Create the sample buffer.
		_createSampleBuffer(pSampleBuffer);
		_isValid = FxTrue;
	}
}

// File-based constructor.
FxDigitalAudio::FxDigitalAudio( const FxString& filename )
	: _isValid(FxFalse)
	, _bitsPerSample(0)
	, _sampleRate(0)
	, _channelCount(0)
	, _sampleCount(0)
	, _pSampleBuffer(NULL)
{
	FxAudioFile audioFile(filename);
	if( audioFile.Load() && audioFile.IsValid() )
	{
		_bitsPerSample = audioFile.GetBitsPerSample();
		_sampleRate    = audioFile.GetSampleRate();
		_channelCount  = audioFile.GetChannelCount();
		_sampleCount   = audioFile.GetSampleCount();
		// Create the sample buffer.
		_createSampleBuffer( audioFile.GetSampleBuffer() );
		_isValid = FxTrue;
	}
}

FxDigitalAudio::FxDigitalAudio( const FxDigitalAudio& other )
	: Super(other)
	, _isValid(other._isValid)
	, _bitsPerSample(other._bitsPerSample)
	, _sampleRate(other._sampleRate)
	, _channelCount(other._channelCount)
	, _sampleCount(other._sampleCount)
{
	if( _sampleCount > 0 )
	{
		_pSampleBuffer = new FxDReal[_sampleCount];
		FxMemcpy(_pSampleBuffer,other._pSampleBuffer,_sampleCount*sizeof(FxDReal));
	}
	else
	{
		_pSampleBuffer = NULL;
	}
}

FxDigitalAudio& FxDigitalAudio::operator=( const FxDigitalAudio& other )
{
	if( this == &other ) return *this;
	Super::operator=( other );
	_isValid       = other._isValid;
	_bitsPerSample = other._bitsPerSample;
	_sampleRate    = other._sampleRate;
	_channelCount  = other._channelCount;
	_sampleCount   = other._sampleCount;
	if( _sampleCount > 0 )
	{
		_pSampleBuffer = new FxDReal[_sampleCount];
		FxMemcpy(_pSampleBuffer,other._pSampleBuffer,_sampleCount*sizeof(FxDReal));
	}
	else
	{
		_pSampleBuffer = NULL;
	}
	return *this;
}

FxDigitalAudio::~FxDigitalAudio()
{
	if( _pSampleBuffer )
	{
		delete [] _pSampleBuffer;
		_pSampleBuffer = NULL;
	}
}

#ifdef __UNREAL__
/**
 * Creates a FxDigitalAudio object from the passed in USoundNodeWave object.
 *
 * @param SoundNodeWave	Wave file to create FxDigitalAudio object from
 * @return FxDigitalAudio object created from passed in SoundNodeWave or NULL if an error occurred
 */
FxDigitalAudio* FxDigitalAudio::CreateFromSoundNodeWave( USoundNodeWave* SoundNodeWave )
{
	FxDigitalAudio* DigitalAudio = NULL;
	if( SoundNodeWave )
	{
		FWaveModInfo WaveInfo;
		if( WaveInfo.ReadWaveInfo((BYTE*)SoundNodeWave->RawData.Lock(LOCK_READ_ONLY),SoundNodeWave->RawData.GetBulkDataSize()) )
		{
			DigitalAudio = new FxDigitalAudio(
			static_cast<FxUInt32>(*WaveInfo.pBitsPerSample), 
			static_cast<FxUInt32>(*WaveInfo.pSamplesPerSec),
			static_cast<FxUInt32>(*WaveInfo.pChannels), 
			static_cast<FxUInt32>(((WaveInfo.SampleDataSize * FX_CHAR_BIT) / *WaveInfo.pBitsPerSample)) / static_cast<FxUInt32>(*WaveInfo.pChannels), 
			static_cast<const FxByte*>(WaveInfo.SampleDataStart));
		}
		SoundNodeWave->RawData.Unlock();
	}
	return DigitalAudio;
}
#endif

FxBool FxDigitalAudio::IsValid( void ) const
{
	return _isValid;
}

FxUInt32 FxDigitalAudio::GetBitsPerSample( void ) const
{
	return _bitsPerSample;
}

FxUInt32 FxDigitalAudio::GetSampleRate( void ) const
{
	return _sampleRate;
}

FxUInt32 FxDigitalAudio::GetChannelCount( void ) const
{
	return _channelCount;
}

FxUInt32 FxDigitalAudio::GetSampleCount( void ) const
{
	return _sampleCount;
}

FxDReal FxDigitalAudio::GetSample( FxSize index ) const
{
	if( _pSampleBuffer )
	{
		return _pSampleBuffer[index];
	}
	return 0.0;
}

FxDReal FxDigitalAudio::GetDuration( void ) const
{
	return static_cast<FxDReal>(_sampleCount) / 
		   static_cast<FxDReal>(_sampleRate);
}

const FxDReal* FxDigitalAudio::GetSampleBuffer( void ) const
{
	return _pSampleBuffer;
}

void FxDigitalAudio::SetSampleBuffer( FxDReal* buffer, FxUInt32 sampleCount )
{
	if( _pSampleBuffer )
	{
		delete [] _pSampleBuffer;
	}
	_pSampleBuffer = new FxDReal[sampleCount];
	FxMemcpy(_pSampleBuffer, buffer, sampleCount);
}

FxBool FxDigitalAudio::IsClipped( void ) const
{
	return (0 != GetNumClippedSamples()) ? FxTrue : FxFalse;
}

FxUInt32 FxDigitalAudio::GetNumClippedSamples( void ) const
{
	// This is a decent approximation to the number of actual clipped samples
	// in the sample buffer.
	const FxDReal k8BitRescale  = 1.0 / static_cast<FxDReal>(1 << 8);
	const FxDReal k16BitRescale = 1.0 / static_cast<FxDReal>(1 << 16);
	const FxDReal kRescale = (16 == _bitsPerSample) ? k16BitRescale : k8BitRescale;
	FxUInt32 numClipped = 0;
	FxDReal sampleValue = 0.0;
	for( FxUInt32 i = 0; i < _sampleCount; ++i )
	{
		// Transform sample to be in the range [0,1].
		sampleValue = _pSampleBuffer[i] * kRescale + 0.5;
		
		// This is a very naive clipping test.
		if( FxDRealEquality(sampleValue, 1.0) || 
			FxDRealEquality(sampleValue, 0.0) )
		{
			++numClipped;
		}
	}
	return numClipped;
}

FxBool FxDigitalAudio::NeedsResample( void ) const
{
	if( _sampleRate != kRequiredSampleRateLow &&
		_sampleRate != kRequiredSampleRateHigh )
	{
		return FxTrue;
	}
	return FxFalse;
}

FxBool FxDigitalAudio::Resample( FxFunctor* pProgressCallback )
{
	// First determine if resampling is possible and to what rate.
	if( IsValid() )
	{
		// 600.0 seconds is 10 minutes of audio.  The simple resampling method
		// below breaks down and causes overflow conditions depending on the
		// duration of the audio passed in (there is a different threshold for
		// each sample rate).  10 minutes is the approximate limit (the real 
		// limit is closer to 10.9 minutes) for CD quality audio (44.1 kHz).
		// Rather than complicate the code with calculations per sample rate,
		// I've chosen to use CD quality audio as the one and only case because
		// it is the worst case scenario.  If we detect this condition, we 
		// return FxFalse without resampling and the user will see an error
		// message instructing him to resample the audio in an external audio
		// processing application such as SoundForge or CoolEdit.
		if( GetDuration() > 600.0 )
		{
			return FxFalse;
		}
		if( _sampleRate > kRequiredSampleRateLow )
		{
			FxUInt32 newSampleRate;
			if( _sampleRate > kRequiredSampleRateHigh )
			{
				newSampleRate = kRequiredSampleRateHigh;
			}
			else
			{
				newSampleRate = kRequiredSampleRateLow;
			}

			// Find the lowest common denominator of the original and new
			// sample rates.
			FxUInt32 commonDenominator = 1;
			for( FxUInt32 i = 2; 
				 i <= FxMin(_sampleRate, newSampleRate); 
				 ++i )
			{
				if( (_sampleRate % i == 0) && (newSampleRate % i == 0) )
				{
					commonDenominator = i;
				}
			}
			
			// New sample rate divided by the original sampling rate.
			FxUInt32 interpolationRatio = newSampleRate / commonDenominator;
			// Ratio of the new sample rate to the decimated sampling rate.
			FxUInt32 decimationRatio = _sampleRate / commonDenominator;

			// Determine how many points should be in the filter.
			// Decimated Nyquist in terms of interpolated Nyquist.
			FxDReal decimatedNyquist = 0.5 / decimationRatio;

			// Width of the transition band of the filter in terms of the 
			// sampling frequency of the data the filter operates on.
			FxDReal deltaF = decimatedNyquist * kTransitionBandWidthRatio;

			// Magic equation for Kaiser windowed filter. 
			// (See Hamming, "Digital Filters")
			FxUInt32 numFilterPoints = 
				static_cast<FxUInt32>(FxCeil( 
				(kStopbandAttenuation - kStopbandAttenuationLimitDueNPoints) /
				(kKaiserCoeffForNPoints * deltaF) ));

			// We want at least one point on either side.
			numFilterPoints = FxMax(numFilterPoints, (FxUInt32)1);
			// Allocate the filter (plus one for the central point at zero).
			FxDReal* pFilter = new FxDReal[numFilterPoints + 1];

			// Create the filter.
			const FxDReal pi = FxAtan(1.0) * 4.0;
			
			// The basic interpolation filter is described as:
			// interpolationRatio * sin(i * pi / interplationRatio) / (i * pi).
			// Here we also make it a low-pass filter by changing the cut-off
			// frequency:
			// interpolationRatio * sin(i * pi / decimationRatio) / (i * pi).
			// Finally it is windowed with a Kaiser window to reduce the Gibbs
			// phenomenon:
			// interpolationRatio * sin(i * pi / decimationRatio) / (i * pi) * kaiser.
			// Magic equation. (See Hamming, "Digital Filters")
			FxDReal bet = kKaiserCoeffForBessel * (kStopbandAttenuation - kStopbandAttenuationLimitDueToBessel);
			FxDReal bottom = _bessi0(bet);
			// The filter's central point.  The filter is symmetric around the
			// central point, but I chose to only allocate the "right" side
			// of the filter.
			pFilter[0] = static_cast<FxDReal>(interpolationRatio) / 
				         static_cast<FxDReal>(decimationRatio);
			for( FxUInt32 i = 1; i <= numFilterPoints; ++i )
			{
				FxDReal temp = i / static_cast<FxDReal>(numFilterPoints);
				FxDReal arg = bet * FxSqrt(1.0 - temp*temp);
				FxDReal top = _bessi0(arg);
				// The weights of the Kaiser window. 
				// (See Hamming, "Digital Filters")
				FxDReal kaiser = top / bottom;
				// Filter weights.
				pFilter[i] = 
					(interpolationRatio * FxSin(i * pi / decimationRatio)) /
					(i * pi) * kaiser;
			}
			
			// The new sample count.
			FxUInt32 newSampleCount = (_sampleCount * interpolationRatio) / 
				                      decimationRatio + 1;
			FxDReal* pNewSampleBuffer = new FxDReal[newSampleCount];

			// Increase progress callback for each new sample computed.
			FxReal percentComplete = 0.0f;
			FxReal percentStepPerSample = 1.0f / 
				                          static_cast<FxReal>(newSampleCount);
			FxUInt32 updateFrequency = newSampleCount / 100;
			FxBool complete = FxFalse;
			
			// Do the decimation / resampling.
			FxUInt32 maxOldIndex = _sampleCount - 1;
			FxUInt32 maxNewIndex = newSampleCount - 1;
			FxDReal sum;
			FxUInt32 newSample;
			FxUInt32 nearestOldSample;
			FxUInt32 filterPoint;
			FxUInt32 oldIndex;
			FxUInt32 oldIndexPlusOne;
			FxUInt32 actualOldIndex;
			FxUInt32 actualOldIndexPlusOne;
			
			// For each new sample.
			for( FxUInt32 i = 0; i <= maxNewIndex; ++i )
			{
				sum = 0.0;
				newSample = i * decimationRatio;
				nearestOldSample = newSample / interpolationRatio;
				filterPoint = newSample % interpolationRatio;
				
				// Do both sides of the filter at the same time.
				oldIndex        = nearestOldSample;
				oldIndexPlusOne = nearestOldSample + 1;
				FxUInt32 offset = interpolationRatio - filterPoint - filterPoint;
				for( FxUInt32 j = filterPoint;
					 j < numFilterPoints;
					 j += interpolationRatio )
				{
					actualOldIndex = FxMax(FxMin(oldIndex, maxOldIndex), (FxUInt32)0);
					actualOldIndexPlusOne = FxMax(FxMin(oldIndexPlusOne, maxOldIndex), (FxUInt32)0);
					// Left side of the filter.
					sum += pFilter[j] * _pSampleBuffer[actualOldIndex];
					// Right side of the filter.
					FxUInt32 rightSideFilterIndex = j + offset;
					if( rightSideFilterIndex < numFilterPoints )
					{
						sum += pFilter[rightSideFilterIndex] * 
							   _pSampleBuffer[actualOldIndexPlusOne];
					}
					oldIndex--;
					oldIndexPlusOne++;
				}

				pNewSampleBuffer[i] = sum;

				percentComplete += percentStepPerSample;
				if( pProgressCallback && i%updateFrequency == 0 )
				{
					if( percentComplete >= 1.0f && !complete )
					{
						(*pProgressCallback)(percentComplete);
						complete = FxTrue;
					}
				}
			}

			// Copy the new data.
			_sampleCount = newSampleCount;
			_sampleRate  = newSampleRate;
			delete [] _pSampleBuffer;
			_pSampleBuffer = pNewSampleBuffer;
			// Clean up.
			delete [] pFilter;
			pFilter = NULL;
			if( pProgressCallback )
			{
				if( percentComplete < 1.0f && !complete )
				{
					(*pProgressCallback)(1.0f);
					complete = FxTrue;
				}
				delete pProgressCallback;
				pProgressCallback = NULL;
			}

			return FxTrue;
		}
	}
	return FxFalse;
}

FxByte* FxDigitalAudio::ExportSampleBuffer( FxUInt32 exportBitsPerSample ) const
{
	if( exportBitsPerSample == kMinimumBitsPerSample ||
		exportBitsPerSample == kMaximumBitsPerSample )
	{
		FxByte* pExportSampleBuffer = 
			new FxByte[(_sampleCount * exportBitsPerSample) / FX_CHAR_BIT];
		if( pExportSampleBuffer )
		{
			// Set up the correct pointer types based on exportBitsPerSample.
			FxUInt8* pByteSampleBuffer = 
				reinterpret_cast<FxUInt8*>(pExportSampleBuffer);
			FxInt16* pWordSampleBuffer =
				reinterpret_cast<FxInt16*>(pExportSampleBuffer);

			// Convert all the samples.
			for( FxUInt32 i = 0; i < _sampleCount; ++i )
			{
				FxDReal value = _pSampleBuffer[i];

				// (Possibly clip) and export the sample.
				if( exportBitsPerSample == kMinimumBitsPerSample )
				{
					// Need shift for 8 bits.
					value += k8BitSampleShift;
					if( value < 0 ) value = 0;
					if( value > UCHAR_MAX ) value = UCHAR_MAX;
					pByteSampleBuffer[i] = 
						static_cast<FxUInt8>(FxNearestInteger(value));
				}
				else if( exportBitsPerSample == kMaximumBitsPerSample )
				{
					if( value < SHRT_MIN ) value = SHRT_MIN;
					if( value > SHRT_MAX ) value = SHRT_MAX;
					pWordSampleBuffer[i] = 
						static_cast<FxInt16>(FxNearestInteger(value));
				}
			}
		}
		return pExportSampleBuffer;
	}
	return NULL;
}

void FxDigitalAudio::Serialize( FxArchive& arc )
{
	Super::Serialize( arc );

	FxUInt16 version = FX_GET_CLASS_VERSION(FxDigitalAudio);
	arc << version;

	arc << _isValid << _bitsPerSample << _sampleRate << _channelCount
		<< _sampleCount;
	
	// Note: uLongf is a zlib type.
	FxSize exportedSampleBufferSize = (_sampleCount * _bitsPerSample) / FX_CHAR_BIT;
	if( arc.IsSaving() )
	{
#ifndef __UNREAL__
		// Compress the exported sample buffer.  zlib requires the initial
		// compressed buffer size to be at least 0.1% larger than the original
		// buffer size plus 12 bytes, so I chose 10% larger to be safe.
		uLongf compressedSampleBufferLength = 
			static_cast<uLongf>(exportedSampleBufferSize + 12 + 
			                    (0.1f * exportedSampleBufferSize));
		FxByte* compressedSampleBuffer = new FxByte[compressedSampleBufferLength];
		FxByte* sourceSampleBuffer = ExportSampleBuffer(_bitsPerSample);
		compress2(compressedSampleBuffer, 
			     &compressedSampleBufferLength, 
				 sourceSampleBuffer, 
				 exportedSampleBufferSize,
				 Z_BEST_COMPRESSION);
		FxSize compressedSize = static_cast<FxSize>(compressedSampleBufferLength);
#else
		FxByte* sourceSampleBuffer = NULL;
		FxSize compressedSize = 0;
		FxByte* compressedSampleBuffer = new FxByte[compressedSize];
#endif // __UNREAL__
		// Write out the compressed size and the compressed buffer.
		arc << compressedSize;
		arc.SerializePODArray(compressedSampleBuffer, compressedSize);

		delete [] sourceSampleBuffer;
		sourceSampleBuffer = NULL;
		delete [] compressedSampleBuffer;
		compressedSampleBuffer = NULL;
	}
	else
	{
		if( _pSampleBuffer )
		{
			delete [] _pSampleBuffer;
			_pSampleBuffer = NULL;
		}
		
		// Read in the compressed size and the compressed buffer.
		FxSize compressedSize = 0;
		arc << compressedSize;
		FxByte* compressedSampleBuffer = new FxByte[compressedSize];
		arc.SerializePODArray(compressedSampleBuffer, compressedSize);

#ifndef __UNREAL__
		// Uncompress.
		uLongf uncompressedSampleBufferSize = static_cast<uLongf>(exportedSampleBufferSize);
		FxByte* uncompressedSampleBuffer = new FxByte[uncompressedSampleBufferSize];
		uncompress(uncompressedSampleBuffer, 
			&uncompressedSampleBufferSize, 
			compressedSampleBuffer, 
			compressedSize);
#else
		FxByte* uncompressedSampleBuffer = new FxByte[compressedSize];
		FxMemcpy(uncompressedSampleBuffer, compressedSampleBuffer, compressedSize);
#endif // __UNREAL__
		// Create the actual sample buffer.
		_createSampleBuffer(uncompressedSampleBuffer);
		delete [] compressedSampleBuffer;
		compressedSampleBuffer = NULL;
		delete [] uncompressedSampleBuffer;
		uncompressedSampleBuffer = NULL;
	}
}

void FxDigitalAudio::_createSampleBuffer( const FxByte* pSampleBuffer )
{
	if( _sampleCount > 0 && pSampleBuffer != NULL )
	{
		_pSampleBuffer = new FxDReal[_sampleCount];

		// Convert and store all the samples.
		for( FxUInt32 i = 0; i < _sampleCount; ++i )
		{
			// If it has multiple channels, this will ensure only the left
			// channel is read.
			FxUInt32 index = i * _channelCount;

			// Get the sample value as a floating point value.
			FxDReal sampleValue = 0.0;
			if( _bitsPerSample == kMinimumBitsPerSample )
			{
				sampleValue =
					reinterpret_cast<const FxUInt8*>(pSampleBuffer)[index];
				sampleValue -= k8BitSampleShift;
			}
			else if( _bitsPerSample == kMaximumBitsPerSample )
			{
				sampleValue =
					reinterpret_cast<const FxInt16*>(pSampleBuffer)[index];
			}

			// Store the sample value.
			_pSampleBuffer[i] = sampleValue;
		}

		// We've knocked the data down to mono, so _channelCount *must* be 1.
		_channelCount = 1;
	}
}

// Modified Bessel function of the first kind (from Numerical Recipes).
FxDReal FxDigitalAudio::_bessi0( FxDReal x )
{
	FxDReal ax, ans;
	FxDReal y;

	if( (ax = FxAbs(x)) < 3.75 )
	{
		y = x / 3.75;
		y *= y;
		ans = 1.0 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492
			  + y * (0.2659732 + y * (0.360768e-1 + y * 0.45813e-2)))));
	}
	else
	{
		y = 3.75 / ax;
		ans = (FxExp(ax) / FxSqrt(ax)) * (0.39894228 + y * (0.1328592e-1
			  + y * (0.225319e-2 + y * (-0.157565e-2 + y * (0.916281e-2
			  + y * (-0.2057706e-1 + y * (0.2635537e-1 + y * (-0.1647633e-1
			  + y * 0.392377e-2))))))));
	}
	return ans;
}

} // namespace Face

} // namespace OC3Ent
