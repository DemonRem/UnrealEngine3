//------------------------------------------------------------------------------
// An audio file.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include <fstream>
using std::ifstream;
using std::ofstream;
using std::ios;

#ifdef WIN32
	#include <windows.h>
	#include <mmreg.h>
	#include <msacm.h>

	#if _MSC_VER > 1000
		// Silently link msacm32.lib and winmm.lib for the acm*() and 
		// mmio*() calls.
		#pragma comment(lib, "msacm32")
		#pragma comment(lib, "winmm")
	#endif
#endif

#include "FxAudioFile.h"
#include "FxUtil.h"

#ifdef __UNREAL__
	#include "Engine.h"
#endif

namespace OC3Ent
{

namespace Face
{

// Constants.
static const FxUInt16 kWaveFormatPCM = 1;

// File types.
static const FxUInt32 kWaveRiffType     = FX_MAKE_FOURCC('W', 'A', 'V', 'E');
static const FxUInt32 kAiffFormType     = FX_MAKE_FOURCC('A', 'I', 'F', 'F');

// Chunk IDs.
static const FxUInt32 kRiffChunkID      = FX_MAKE_FOURCC('R', 'I', 'F', 'F');
static const FxUInt32 kDataChunkID      = FX_MAKE_FOURCC('d', 'a', 't', 'a');
static const FxUInt32 kFmt_ChunkID      = FX_MAKE_FOURCC('f', 'm', 't', ' ');
static const FxUInt32 kFormChunkID      = FX_MAKE_FOURCC('F', 'O', 'R', 'M');
static const FxUInt32 kCommonChunkID    = FX_MAKE_FOURCC('C', 'O', 'M', 'M');
static const FxUInt32 kSoundDataChunkID = FX_MAKE_FOURCC('S', 'S', 'N', 'D');
static const FxUInt32 kListChunkID      = FX_MAKE_FOURCC('L', 'I', 'S', 'T');
static const FxUInt32 kInfoChunkID      = FX_MAKE_FOURCC('I', 'N', 'F', 'O');
static const FxUInt32 kCopyrightChunkID = FX_MAKE_FOURCC('I', 'C', 'O', 'P');

// Pack the chunk structures on byte boundaries.
#pragma pack(push, 1)

// Common structures.
struct ChunkHeader
{
	FxUInt32 chunkID;
	FxUInt32 chunkSize;
};

// WAVE structures.
struct RiffChunk
{
	FxUInt32 chunkID;
	FxUInt32 chunkSize;
	FxUInt32 riffType;
};

struct WaveFormatChunk
{
	FxUInt16 formatTag;      // Format category.
	FxUInt16 channelCount;   // Number of channels.
	FxUInt32 sampleRate;     // Sampling rate.
	FxUInt32 avgBytesPerSec; // For buffer estimation.
	FxUInt16 blockAlign;     // Data block size.
	FxUInt16 bitsPerSample;
};

// AIFF structures.
struct AiffFormChunk
{
	FxUInt32 chunkID;
	FxUInt32 chunkSize;
	FxUInt32 formType;
};

struct AiffCommonChunk
{
	FxUInt16 channelCount;
	FxUInt32 sampleCount;
	FxUInt16 bitsPerSample;
	FxUInt8  sampleRate[10];
};

struct AiffSoundDataChunk
{
	FxUInt32 offset;
	FxUInt32 blockSize;
};

// Restore packing to whatever it was.
#pragma pack(pop)

FxArray<FxAudioFormatHandler*>* FxAudioFile::_formatHandlers = NULL;

FxAudioFile::FxAudioFile()
	: _filename("") 
    , _isValid(FxFalse)
	, _bitsPerSample(0)
	, _sampleRate(0)
	, _channelCount(0) 
	, _sampleCount(0)
	, _pSampleBuffer(NULL)
{
}

FxAudioFile::FxAudioFile( const FxString& filename )
	: _filename(filename)
	, _isValid(FxFalse)
	, _bitsPerSample(0)
	, _sampleRate(0)
	, _channelCount(0) 
	, _sampleCount(0)
	, _pSampleBuffer(NULL)
{
}

FxAudioFile::FxAudioFile( FxDigitalAudio* pDigitalAudio )
{
	if( pDigitalAudio )
	{
		_filename      = "";
		_isValid       = FxTrue;
		_bitsPerSample = static_cast<FxInt32>(pDigitalAudio->GetBitsPerSample());
		_sampleRate    = static_cast<FxInt32>(pDigitalAudio->GetSampleRate());
		_channelCount  = static_cast<FxInt32>(pDigitalAudio->GetChannelCount());
		_sampleCount   = static_cast<FxInt32>(pDigitalAudio->GetSampleCount());
		_pSampleBuffer = pDigitalAudio->ExportSampleBuffer(_bitsPerSample);
	}
}

FxAudioFile::~FxAudioFile()
{
	if( _pSampleBuffer )
	{
		delete [] _pSampleBuffer;
		_pSampleBuffer = NULL;
	}
}

void FxAudioFile::Startup( void )
{
	FxAssert(_formatHandlers == NULL);
	_formatHandlers = new FxArray<FxAudioFormatHandler*>();
}

void FxAudioFile::Shutdown( void )
{
	FxAssert(_formatHandlers != NULL);
	if( _formatHandlers )
	{
		while( _formatHandlers->Length() )
		{
			delete (*_formatHandlers)[0];
			_formatHandlers->Remove(0);
		}
		delete _formatHandlers;
		_formatHandlers = NULL;
	}
}

void FxAudioFile::RegisterAudioFormatHandler( FxAudioFormatHandler* pHandler )
{
	FxAssert(_formatHandlers != NULL);
	if( _formatHandlers )
	{
		_formatHandlers->PushBack(pHandler);
	}
}

FxArray<FxString> FxAudioFile::GetSupportedExtensions( void )
{
	FxArray<FxString> exts;
	FxAssert(_formatHandlers != NULL);
	if( _formatHandlers )
	{
		FxSize numHandlers = _formatHandlers->Length();
		exts.Reserve(numHandlers);
		for( FxSize i = 0; i < numHandlers; ++i )
		{
			FxAssert((*_formatHandlers)[i] != NULL);
			exts.PushBack((*_formatHandlers)[i]->GetFileExt());
		}
	}
	return exts;
}

FxArray<FxString> FxAudioFile::GetDescriptions( void )
{
	FxArray<FxString> descs;
	FxAssert(_formatHandlers != NULL);
	if( _formatHandlers )
	{
		FxSize numHandlers = _formatHandlers->Length();
		descs.Reserve(numHandlers);
		for( FxSize i = 0; i < numHandlers; ++i )
		{
			FxAssert((*_formatHandlers)[i] != NULL);
			descs.PushBack((*_formatHandlers)[i]->GetDesc());
		}
	}
	return descs;
}

FxBool FxAudioFile::Load( void )
{
	FxAudioFormatHandler* pHandler = _findAudioFormatHandler(_filename);
	if( pHandler )
	{
		if( pHandler->Load(*this, _filename) )
		{
			_isValid = FxTrue;
			return FxTrue;
		}
	}
	return FxFalse;
}

FxBool FxAudioFile::Save( const FxString& filename )
{
	FxAudioFormatHandler* pHandler = _findAudioFormatHandler(filename);
	if( pHandler )
	{
		if( pHandler->Save(*this, filename) )
		{
			_isValid = FxTrue;
			return FxTrue;
		}
	}
	return FxFalse;
}

const FxString& FxAudioFile::GetFilename( void ) const
{
	return _filename;
}

FxBool FxAudioFile::IsValid( void ) const
{
	return _isValid;
}

FxInt32 FxAudioFile::GetBitsPerSample( void ) const
{
	return _bitsPerSample;
}

FxInt32 FxAudioFile::GetSampleRate( void ) const
{
	return _sampleRate;
}

FxInt32 FxAudioFile::GetChannelCount( void ) const
{
	return _channelCount;
}

FxInt32 FxAudioFile::GetSampleCount( void ) const
{
	return _sampleCount;
}

FxByte* FxAudioFile::GetSampleBuffer( void ) const
{
	return _pSampleBuffer;
}

void FxAudioFile::SetBitsPerSample( FxInt32 bitsPerSample )
{
	_bitsPerSample = bitsPerSample;
}

void FxAudioFile::SetSampleRate( FxInt32 sampleRate )
{
	_sampleRate = sampleRate;
}

void FxAudioFile::SetChannelCount( FxInt32 channelCount )
{
	_channelCount = channelCount;
}

void FxAudioFile::SetSampleCount( FxInt32 sampleCount )
{
	_sampleCount = sampleCount;
}

void FxAudioFile::SetSampleBuffer( FxByte* pSampleBuffer )
{
	_pSampleBuffer = pSampleBuffer;
}

FxAudioFormatHandler* FxAudioFile::_findAudioFormatHandler( const FxString& filename )
{
	FxAssert(_formatHandlers != NULL);
	if( _formatHandlers )
	{
		FxSize numHandlers = _formatHandlers->Length();
		for( FxSize i = 0; i < numHandlers; ++i )
		{
			FxAssert((*_formatHandlers)[i] != NULL);
			if( (*_formatHandlers)[i]->IsSupportedExt(filename) )
			{
				return (*_formatHandlers)[i];
			}
		}
	}
	return NULL;
}

FxBool FxWaveFormatHandler::Load( FxAudioFile& audioFile, const FxString& filename )
{
	FxAssert(filename.Length() > 0);
	FxAssert(audioFile.GetSampleBuffer() == NULL);
	if( SupportsLoad() && IsSupportedExt(filename) )
	{
#ifdef WIN32
		FxChar* pRawSourceWaveFormat   = NULL;
		FxChar* pSourceSampleData      = NULL;
		FxChar* pDestinationSampleData = NULL;

		// How come Microsoft can't be const-correct?!
#ifdef __UNREAL__
		HMMIO hWaveFile = mmioOpen(ANSI_TO_TCHAR(filename.GetData()), 0, 
			MMIO_READ | MMIO_ALLOCBUF);
#else
		FxWString filenameW(filename.GetData());
		HMMIO hWaveFile = mmioOpen(const_cast<FxWChar*>(filenameW.GetData()), 0, 
			MMIO_READ | MMIO_ALLOCBUF);
#endif

		if( !hWaveFile )
		{
			return FxFalse;
		}

		// Read the header.
		MMCKINFO header = {0};
		header.fccType = mmioFOURCC('W', 'A', 'V', 'E');
		MMRESULT mmResult = mmioDescend(hWaveFile, &header, 0,	MMIO_FINDRIFF);
		if( mmResult )
		{
			mmioClose(hWaveFile, 0);
			return FxFalse;
		}

		// Start reading chunks.
		MMCKINFO chunk = {0};
		FxSize sourceSampleDataBytes = 0;
		mmResult = mmioDescend(hWaveFile, &chunk, &header, 0);
		while( mmResult == MMSYSERR_NOERROR )
		{
			// Figure out what to do with the current chunk.
			FxSize byteCount = 0;
			FxSize bytesRead = 0;
			FxChar* pData = NULL;
			switch( chunk.ckid )
			{
			case mmioFOURCC('f', 'm', 't', ' '):
				byteCount = chunk.cksize;
				if( byteCount < sizeof(WAVEFORMATEX) )
				{
					byteCount = sizeof(WAVEFORMATEX);
				}
				pData = new FxChar[byteCount];
				bytesRead = mmioRead(hWaveFile, pData, chunk.cksize);
				pRawSourceWaveFormat = pData;
				pData = NULL;
				break;
			case mmioFOURCC('d', 'a', 't', 'a'):
				byteCount = chunk.cksize;
				pData = new FxChar[byteCount];
				bytesRead = mmioRead(hWaveFile, pData, 
					static_cast<LONG>(byteCount));
				pSourceSampleData = pData;
				pData = NULL;
				sourceSampleDataBytes = byteCount;
				break;
			}

			// Pop out of the current chunk and start the next one.
			mmResult = mmioAscend(hWaveFile, &chunk, 0);
			mmResult = mmioDescend(hWaveFile, &chunk, &header, 0);
		}

		// Close the file.
		mmResult = mmioClose(hWaveFile, 0);
		if( mmResult )
		{
			delete [] pRawSourceWaveFormat;
			delete [] pSourceSampleData;
			return FxFalse;
		}
		hWaveFile = 0;
		// Check that we got everything we need.
		if( pRawSourceWaveFormat == NULL )
		{
			delete [] pRawSourceWaveFormat;
			delete [] pSourceSampleData;
			return FxFalse;
		}
		else if( sourceSampleDataBytes == NULL )
		{
			delete [] pRawSourceWaveFormat;
			delete [] pSourceSampleData;
			return FxFalse;
		}

		// First, convert the sample data to PCM if necessary.
		WAVEFORMATEX *pSourceWaveFormat = reinterpret_cast<WAVEFORMATEX*>(pRawSourceWaveFormat);
		DWORD destinationSampleDataBytes = 0;
		if( pSourceWaveFormat->wFormatTag != WAVE_FORMAT_PCM )
		{
			// Set up the format descriptor.
			WAVEFORMATEX destinationWaveFormat = {0};
			destinationWaveFormat.wFormatTag      = WAVE_FORMAT_PCM;
			destinationWaveFormat.nChannels       = pSourceWaveFormat->nChannels;
			destinationWaveFormat.nSamplesPerSec  = pSourceWaveFormat->nSamplesPerSec;
			destinationWaveFormat.nAvgBytesPerSec =	(pSourceWaveFormat->nChannels * pSourceWaveFormat->nSamplesPerSec * 16) / FX_CHAR_BIT;
			destinationWaveFormat.nBlockAlign     = (pSourceWaveFormat->nChannels * 16) / FX_CHAR_BIT;
			destinationWaveFormat.wBitsPerSample  = 16;

			if( FxWaveFormatHandler::_acmConvert(pSourceWaveFormat, pSourceSampleData, static_cast<DWORD>(sourceSampleDataBytes), &destinationWaveFormat, 
				                                 pDestinationSampleData, destinationSampleDataBytes) == FxFalse )
			{
				delete [] pRawSourceWaveFormat;
				delete [] pSourceSampleData;
				delete [] pDestinationSampleData;
				return FxFalse;
			}
			delete [] pSourceSampleData;
			pSourceSampleData = pDestinationSampleData;
			pDestinationSampleData = 0;
			sourceSampleDataBytes = destinationSampleDataBytes;
			destinationSampleDataBytes = 0;
			FxMemcpy(pSourceWaveFormat, &destinationWaveFormat, sizeof(WAVEFORMATEX));
		}

		// Next, if the result isn't mono, knock it down to mono.
		if( pSourceWaveFormat->nChannels > 1 )
		{
			// Set up the format descriptor.
			WAVEFORMATEX destinationWaveFormat = {0};
			destinationWaveFormat.wFormatTag      = WAVE_FORMAT_PCM;
			destinationWaveFormat.nChannels       = 1;
			destinationWaveFormat.nSamplesPerSec  = pSourceWaveFormat->nSamplesPerSec;
			destinationWaveFormat.nAvgBytesPerSec = (1 * pSourceWaveFormat->nSamplesPerSec * pSourceWaveFormat->wBitsPerSample) / FX_CHAR_BIT;
			destinationWaveFormat.nBlockAlign     = (1 * pSourceWaveFormat->wBitsPerSample) / FX_CHAR_BIT;
			destinationWaveFormat.wBitsPerSample  = pSourceWaveFormat->wBitsPerSample;

			if( FxWaveFormatHandler::_acmConvert(pSourceWaveFormat, pSourceSampleData, static_cast<DWORD>(sourceSampleDataBytes), &destinationWaveFormat, 
				                                 pDestinationSampleData, destinationSampleDataBytes) == FxFalse )
			{
				delete [] pRawSourceWaveFormat;
				delete [] pSourceSampleData;
				delete [] pDestinationSampleData;
				return FxFalse;
			}
			delete [] pSourceSampleData;
			pSourceSampleData = pDestinationSampleData;
			pDestinationSampleData = 0;
			sourceSampleDataBytes = destinationSampleDataBytes;
			destinationSampleDataBytes = 0;
			FxMemcpy(pSourceWaveFormat, &destinationWaveFormat, sizeof(WAVEFORMATEX));
		}

		// Success!
		audioFile.SetBitsPerSample(pSourceWaveFormat->wBitsPerSample);
		audioFile.SetSampleRate(pSourceWaveFormat->nSamplesPerSec);
		audioFile.SetChannelCount(pSourceWaveFormat->nChannels);
		audioFile.SetSampleCount(static_cast<FxUInt32>((sourceSampleDataBytes * FX_CHAR_BIT) / pSourceWaveFormat->wBitsPerSample));
		audioFile.SetSampleBuffer(reinterpret_cast<FxByte*>(pSourceSampleData));
		pSourceSampleData = 0;
		
		// Clean up.
		delete [] pRawSourceWaveFormat;
		pRawSourceWaveFormat = 0;
		delete [] pSourceSampleData;
		pSourceSampleData = 0;
		delete [] pDestinationSampleData;
		pDestinationSampleData = 0;

		return FxTrue;
#else // !WIN32
	FxChar* pChunk = NULL;
	
	FxBool foundHeader = FxFalse;
	FxBool foundData   = FxFalse;

	// Open the WAVE file.
	ifstream input(filename.GetData(), ios::in | ios::binary);
	if( input.fail() )
	{
		return FxFalse;
	}

	// Read the RIFF chunk.
	RiffChunk riffChunk = {0};
	input.read(reinterpret_cast<FxChar*>(&riffChunk), sizeof(riffChunk));
	if( input.fail() )
	{
		return FxFalse;
	}

	// Verify it's a RIFF file with a WAVE chunk.
	if( (riffChunk.chunkID != kRiffChunkID) || (riffChunk.riffType != kWaveRiffType) )
	{
		return FxFalse;
	}

	// Loop until end-of-file.
	while( input.peek() != EOF )
	{
		// Read the chunk header.
		ChunkHeader chunkHeader = {0};
		input.read(reinterpret_cast<FxChar*>(&chunkHeader), sizeof(chunkHeader));

		// Handle the case where the file is actually a few bytes larger
		// than the data says it should be.
		if( input.peek() == EOF )
		{
			continue;
		}

		// If we're not at the end of the file, return an error.
		if( input.fail() )
		{
			return FxFalse;
		}

		// Rearrange the bits into something intelligible.
		FX_SWAP_LITTLE_ENDIAN_BYTES(chunkHeader.chunkSize);

		// Decide what to do with the chunk.
		if( chunkHeader.chunkID == kFmt_ChunkID )
		{
			// Check the chunk size.
			if( chunkHeader.chunkSize < sizeof(WaveFormatChunk) )
			{
				return FxFalse;
			}

			// Allocate a buffer for the chunk.
			pChunk = new FxChar[chunkHeader.chunkSize];

			// Read the format chunk.
			input.read(pChunk, chunkHeader.chunkSize);
			if( input.fail() )
			{
				return FxFalse;
			}

			// Set up a pointer to the format chunk.
			WaveFormatChunk* pWaveFormatChunk =	reinterpret_cast<WaveFormatChunk*>(pChunk);

			// Rearrange the bits into something intelligible.
			FX_SWAP_LITTLE_ENDIAN_BYTES(pWaveFormatChunk->avgBytesPerSec);
			FX_SWAP_LITTLE_ENDIAN_BYTES(pWaveFormatChunk->bitsPerSample);
			FX_SWAP_LITTLE_ENDIAN_BYTES(pWaveFormatChunk->blockAlign);
			FX_SWAP_LITTLE_ENDIAN_BYTES(pWaveFormatChunk->channelCount);
			FX_SWAP_LITTLE_ENDIAN_BYTES(pWaveFormatChunk->formatTag);
			FX_SWAP_LITTLE_ENDIAN_BYTES(pWaveFormatChunk->sampleRate);

			// Check that the data is in PCM format.
			if( pWaveFormatChunk->formatTag != kWaveFormatPCM )
			{
				return FxFalse;
			}

			// Update the values in the caller with the info in the format chunk.
			audioFile.SetBitsPerSample(static_cast<FxInt32>(pWaveFormatChunk->bitsPerSample));
			audioFile.SetSampleRate(static_cast<FxInt32>(pWaveFormatChunk->sampleRate));
			audioFile.SetChannelCount(static_cast<FxInt32>(pWaveFormatChunk->channelCount));

			// Free the chunk.
			delete [] pChunk;
			pChunk = NULL;

			// Found the header.
			foundHeader = FxTrue;
		}
		else if( chunkHeader.chunkID == kDataChunkID )
		{
			// The following code assumes (rightly) that the header has already been found.
			if( foundHeader == FxFalse )
			{
				return FxFalse;
			}

			// Calculate the number of samples.
			if( audioFile.GetBitsPerSample() == 8 )
			{
				audioFile.SetSampleCount(static_cast<FxInt32>(chunkHeader.chunkSize));
			}
			else if( audioFile.GetBitsPerSample() == 16 )
			{
				audioFile.SetSampleCount(static_cast<FxInt32>(chunkHeader.chunkSize / 2));
			}
			else
			{
				return FxFalse;
			}

			// Correct for the number of channels.
			audioFile.SetSampleCount(audioFile.GetSampleCount() / audioFile.GetChannelCount());

			// Allocate a buffer for the chunk.
			FxByte* sampleBuffer = new FxByte[chunkHeader.chunkSize];

			// Read the samples.
			input.read(reinterpret_cast<FxChar*>(sampleBuffer), chunkHeader.chunkSize);
			if( input.fail() )
			{
				return FxFalse;
			}

#if defined(FX_BIG_ENDIAN)
			// For 16-bit samples, we have to swap the bytes.
			if( audioFile.GetBitsPerSample() == 16 )
			{
				FxInt16* pSample = reinterpret_cast<FxInt16*>(sampleBuffer);
				FxSize maxIndex = static_cast<FxSize>(audioFile.GetSampleCount() * audioFile.GetChannelCount());
				for( FxSize i = 0; i < maxIndex; ++i )
				{
					// Rearrange the bits into something intelligible.
					FX_SWAP_LITTLE_ENDIAN_BYTES(pSample[i]);
				}
			}
#endif
			audioFile.SetSampleBuffer(sampleBuffer);
			foundData = FxTrue;
		}
		else
		{
			// Skip over the chunk.
			input.ignore(chunkHeader.chunkSize);
		}
	}

	// Success!
	if( foundHeader && foundData )
	{
		return FxTrue;
	}
#endif
	}
	return FxFalse;
}

FxBool FxWaveFormatHandler::Save( FxAudioFile& audioFile, const FxString& filename )
{
	FxAssert(filename.Length() > 0);
	FxAssert(audioFile.GetChannelCount() == 1);
	FxAssert(audioFile.GetSampleCount() > 0);
	FxAssert(audioFile.GetSampleBuffer() != NULL);
	if( SupportsSave() && IsSupportedExt(filename) )
	{
#ifdef WIN32
		// How come Microsoft can't be const-correct?!
#ifdef __UNREAL__
		HMMIO hWaveFile = mmioOpen(ANSI_TO_TCHAR(filename.GetData()), 0, 
			MMIO_CREATE|MMIO_WRITE|MMIO_EXCLUSIVE | MMIO_ALLOCBUF);
#else
		FxWString filenameW(filename.GetData());
		HMMIO hWaveFile = mmioOpen(const_cast<FxWChar*>(filenameW.GetData()), 0, 
			MMIO_CREATE|MMIO_WRITE|MMIO_EXCLUSIVE | MMIO_ALLOCBUF);
#endif

		if( !hWaveFile )
		{
			return FxFalse;
		}

		// Write the header.
		MMCKINFO header = {0};
		header.fccType = mmioFOURCC('W', 'A', 'V', 'E');
		MMRESULT mmResult = mmioCreateChunk(hWaveFile, &header, MMIO_CREATERIFF);

		// Set up the WAVEFORMATEX structure.
		WAVEFORMATEX destinationWaveFormat = {0};
		destinationWaveFormat.wFormatTag      = WAVE_FORMAT_PCM;
		destinationWaveFormat.nChannels       = audioFile.GetChannelCount();
		destinationWaveFormat.nSamplesPerSec  = audioFile.GetSampleRate();
		destinationWaveFormat.nAvgBytesPerSec = 
			(1 * audioFile.GetSampleRate() * audioFile.GetBitsPerSample()) / FX_CHAR_BIT;
		destinationWaveFormat.nBlockAlign     = (1 * audioFile.GetBitsPerSample()) / FX_CHAR_BIT;
		destinationWaveFormat.wBitsPerSample  = audioFile.GetBitsPerSample();

		// Start writing chunks.
		MMCKINFO chunk = {0};
		chunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
		chunk.cksize = sizeof(WAVEFORMATEX) + destinationWaveFormat.cbSize;
		mmResult = mmioCreateChunk(hWaveFile, &chunk, 0);
		mmResult = mmioWrite(hWaveFile, reinterpret_cast<FxChar*>(&destinationWaveFormat), chunk.cksize);
		mmResult = mmioAscend(hWaveFile, &chunk, 0);

		chunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
		mmResult   = mmioCreateChunk(hWaveFile, &chunk, 0);
		mmResult   = mmioWrite(hWaveFile, reinterpret_cast<FxChar*>(audioFile.GetSampleBuffer()), (audioFile.GetSampleCount() * audioFile.GetBitsPerSample()) / FX_CHAR_BIT);

		// Close the file.
		mmResult = mmioAscend(hWaveFile, &chunk, 0);
		mmResult = mmioAscend(hWaveFile, &header, 0);
		mmResult = mmioClose(hWaveFile, 0);

		return (MMSYSERR_NOERROR == mmResult) ? FxTrue : FxFalse;
#else // !WIN32
		// Calculate some byte counts.
		FxUInt32 sampleByteCount    = (audioFile.GetSampleCount() * audioFile.GetBitsPerSample()) / FX_CHAR_BIT;
		FxUInt32 riffChunkByteCount = sizeof(kWaveRiffType) + sizeof(kFmt_ChunkID) + sizeof(FxUInt32) + sizeof(WaveFormatChunk) +
			                          sizeof(kDataChunkID) + sizeof(FxUInt32) + sampleByteCount;

		// Open the file.
		ofstream output(filename.GetData(), ios::out | ios::binary);
		if( output.fail() )
		{
			return FxFalse;
		}

		// Set up the RIFF chunk.
		RiffChunk riffChunk = { kRiffChunkID, riffChunkByteCount, kWaveRiffType	};

		// Rearrange the bits into something intelligible.
		FX_SWAP_LITTLE_ENDIAN_BYTES(riffChunk.chunkSize);

		// Write the RIFF chunk.
		output.write(reinterpret_cast<const FxChar*>(&riffChunk), sizeof(riffChunk));
		if( output.fail() )
		{
			return FxFalse;
		}

		// Set up the format chunk header.
		ChunkHeader fmtChunkHeader = { kFmt_ChunkID, sizeof(WaveFormatChunk) };

		// Rearrange the bits into something intelligible.
		FX_SWAP_LITTLE_ENDIAN_BYTES(fmtChunkHeader.chunkSize);

		// Write the format chunk header.
		output.write(reinterpret_cast<const FxChar*>(&fmtChunkHeader), sizeof(fmtChunkHeader));
		if( output.fail() )
		{
			return FxFalse;
		}

		// Set up the format data.
		WaveFormatChunk waveFormatChunk = { kWaveFormatPCM,	audioFile.GetChannelCount(), audioFile.GetSampleRate(),
				                            (audioFile.GetSampleRate() * audioFile.GetBitsPerSample()) / FX_CHAR_BIT,
				                            audioFile.GetBitsPerSample() / FX_CHAR_BIT, audioFile.GetBitsPerSample() };

		// Rearrange the bits into something intelligible.
		FX_SWAP_LITTLE_ENDIAN_BYTES(waveFormatChunk.avgBytesPerSec);
		FX_SWAP_LITTLE_ENDIAN_BYTES(waveFormatChunk.bitsPerSample);
		FX_SWAP_LITTLE_ENDIAN_BYTES(waveFormatChunk.blockAlign);
		FX_SWAP_LITTLE_ENDIAN_BYTES(waveFormatChunk.channelCount);
		FX_SWAP_LITTLE_ENDIAN_BYTES(waveFormatChunk.sampleRate);

		// Write the format data.
		output.write(reinterpret_cast<const FxChar*>(&waveFormatChunk), sizeof(waveFormatChunk));
		if( output.fail() )
		{
			return FxFalse;
		}

		// Set up the data chunk header.
		ChunkHeader dataChunkHeader = { kDataChunkID, sampleByteCount };

		// Rearrange the bits into something intelligible.
		FX_SWAP_LITTLE_ENDIAN_BYTES(dataChunkHeader.chunkSize);

		// Write the data chunk header.
		output.write(reinterpret_cast<const FxChar*>(&dataChunkHeader), sizeof(dataChunkHeader));
		if( output.fail() )
		{
			return FxFalse;
		}

#if defined(FX_BIG_ENDIAN)
		// For 16-bit samples, we have to swap the bytes...which means we
		// have to make a copy of the buffer.
		if( audioFile.GetBitsPerSample() == 16 )
		{
			// Make a copy of the sample buffer.
			FxSize sampleCount = static_cast<FxSize>(audioFile.GetSampleCount());
			FxInt16* swapBuffer = new FxInt16[sampleCount];
			FxMemcpy(swapBuffer, audioFile.GetSampleBuffer(), sampleByteCount);

			// Byte-swap the new sample buffer.
			for( FxSize i = 0; i < sampleCount; ++i )
			{
				// Rearrange the bits into something intelligible.
				FX_SWAP_LITTLE_ENDIAN_BYTES(swapBuffer[i]);
			}

			// Write the byte-swapped sample buffer.
			output.write(reinterpret_cast<const FxChar*>(swapBuffer), sampleByteCount);

			// Free the byte-swapped sample buffer.
			delete [] swapBuffer;
			swapBuffer = NULL;

			// Check for errors.
			if( output.fail() )
			{
				return FxFalse;
			}
		}
		else
#endif
		{
			// Write the samples.
			output.write(reinterpret_cast<const FxChar*>(audioFile.GetSampleBuffer()), sampleByteCount);
			if( output.fail() )
			{
				return FxFalse;
			}
		}

		// Success!
		return FxTrue;
#endif
	}
	return FxFalse;
}

#ifdef WIN32
FxBool FxWaveFormatHandler::
_acmConvert( WAVEFORMATEX* pSrcWaveFormat, FxChar* pSrcSampleData, DWORD srcSampleDataBytes,
		     WAVEFORMATEX* pDstWaveFormat, FxChar*& pDstSampleData, DWORD& dstSampleDataBytes )
{
	// Open the conversion stream.
	HACMSTREAM hACMStream = 0;
	MMRESULT mmResult = acmStreamOpen(&hACMStream, 0, pSrcWaveFormat, pDstWaveFormat, 0, 0, 0, ACM_STREAMOPENF_NONREALTIME);
	if( !mmResult )
	{
		// Get the size of the output sample data buffer.
		mmResult = acmStreamSize(hACMStream, srcSampleDataBytes, &dstSampleDataBytes, ACM_STREAMSIZEF_SOURCE);
		if( !mmResult )
		{
			// Allocate a buffer for output.
			pDstSampleData = new FxChar[dstSampleDataBytes];

			// Set up the ACM stream header.
			ACMSTREAMHEADER acmStreamHeader = {sizeof(ACMSTREAMHEADER)};
			acmStreamHeader.pbSrc =	reinterpret_cast<LPBYTE>(pSrcSampleData);
			acmStreamHeader.cbSrcLength = srcSampleDataBytes;
			acmStreamHeader.pbDst = reinterpret_cast<LPBYTE>(pDstSampleData);
			acmStreamHeader.cbDstLength = dstSampleDataBytes;

			// Prepare the stream header.
			mmResult = acmStreamPrepareHeader(hACMStream, &acmStreamHeader, 0);
			if( !mmResult )
			{
				// Perform the conversion.
				mmResult = acmStreamConvert(hACMStream, &acmStreamHeader, 0);
				if( !mmResult )
				{
					// Unprepare the stream header.
					mmResult = acmStreamUnprepareHeader(hACMStream,	&acmStreamHeader, 0);
					if( !mmResult )
					{
						// Close the conversion stream.
						mmResult = acmStreamClose(hACMStream, 0);
						if( !mmResult )
						{
							return FxTrue;
						}
					}
				}
			}
		}
	}
	return FxFalse;
}
#endif

FxBool FxAiffFormatHandler::Load( FxAudioFile& audioFile, const FxString& filename )
{
	FxAssert(filename.Length() > 0);
	FxAssert(audioFile.GetSampleBuffer() == NULL);
	if( SupportsLoad() && IsSupportedExt(filename) )
	{
		FxBool foundHeader = FxFalse;
		FxBool foundData   = FxFalse;

		// Open the AIFF file.
		ifstream input(filename.GetData(), ios::in | ios::binary);
		
		// Read the Form chunk.
		AiffFormChunk aiffFormChunk = {0};
		input.read(reinterpret_cast<FxChar*>(&aiffFormChunk), sizeof(aiffFormChunk));
		if( input.fail() )
		{
			return FxFalse;
		}
		if( aiffFormChunk.chunkID != kFormChunkID )
		{
			return FxFalse;
		}

		// Rearrange the bits into something intelligible.
		FX_SWAP_BIG_ENDIAN_BYTES(aiffFormChunk.chunkSize);

		// Verify that the file is an AIFF file by checking the formType.
		if( aiffFormChunk.formType != kAiffFormType )
		{
			return FxFalse;
		}

		// Loop through the rest of the chunks.
		while( input.peek() != EOF )
		{
			// Read the chunk header.
			ChunkHeader chunkHeader = {0};
			input.read(reinterpret_cast<FxChar*>(&chunkHeader), sizeof(chunkHeader));
			if( input.fail() )
			{
				return FxFalse;
			}

			// Rearrange the bits into something intelligible.
			FX_SWAP_BIG_ENDIAN_BYTES(chunkHeader.chunkSize);

			// Decide what to do with the chunk.
			if( chunkHeader.chunkID == kCommonChunkID )
			{
				// Read the common chunk.
				AiffCommonChunk aiffCommonChunk = {0};
				input.read(reinterpret_cast<FxChar*>(&aiffCommonChunk), sizeof(aiffCommonChunk));
				if( input.fail() )
				{
					return FxFalse;
				}

				// Rearrange the bits into something intelligible.
				FX_SWAP_BIG_ENDIAN_BYTES(aiffCommonChunk.channelCount);
				FX_SWAP_BIG_ENDIAN_BYTES(aiffCommonChunk.sampleCount);
				FX_SWAP_BIG_ENDIAN_BYTES(aiffCommonChunk.bitsPerSample);
				// Don't swap the bytes for the sample rate.  The Apple code 
				// expects things to be in big endian format, not Intel format.
				//FX_SWAP_BIG_ENDIAN_BYTES(aiffCommonChunk.sampleRate);

				// Convert the extended floating point sample rate to a
				// double.
				FxDReal sampleRate = 0.0;
				FxAiffFormatHandler::_extendedToDouble(aiffCommonChunk.sampleRate, sampleRate);

				// Update the variables in the caller.
				audioFile.SetBitsPerSample(static_cast<FxInt32>(aiffCommonChunk.bitsPerSample));
				audioFile.SetSampleRate(static_cast<FxInt32>(sampleRate));
				audioFile.SetChannelCount(static_cast<FxInt32>(aiffCommonChunk.channelCount));
				audioFile.SetSampleCount(static_cast<FxInt32>(aiffCommonChunk.sampleCount));

				// Found the header.
				foundHeader = FxTrue;
			}
			else if( chunkHeader.chunkID == kSoundDataChunkID )
			{
				// The following code assumes (rightly) that the
				// header has already been found.
				if( foundHeader == FxFalse )
				{
					return FxFalse;
				}

				// Read the Sound Data chunk.
				AiffSoundDataChunk aiffSoundDataChunk = {0};
				input.read(reinterpret_cast<FxChar*>(&aiffSoundDataChunk), sizeof(aiffSoundDataChunk));
				if( input.fail() )
				{
					return FxFalse;
				}

				// Rearrange the bits into something intelligible.
				FX_SWAP_BIG_ENDIAN_BYTES(aiffSoundDataChunk.offset);
				FX_SWAP_BIG_ENDIAN_BYTES(aiffSoundDataChunk.blockSize);

				// Allocate a buffer for the samples.
				FxSize sampleBufferBytes = (audioFile.GetSampleCount() * audioFile.GetChannelCount() * audioFile.GetBitsPerSample()) / FX_CHAR_BIT;
				FxByte* sampleBuffer = new FxByte[sampleBufferBytes];

				// Read the samples.
				input.read(reinterpret_cast<FxChar*>(sampleBuffer), sampleBufferBytes);
				if( input.fail() )
				{
					delete [] sampleBuffer;
					return FxFalse;
				}

#if !defined(FX_BIG_ENDIAN)
				// For 16-bit samples, we have to swap the bytes.
				if( audioFile.GetBitsPerSample() == 16 )
				{
					FxInt16* pSample = reinterpret_cast<FxInt16*>(sampleBuffer);
					FxSize maxIndex = audioFile.GetSampleCount() * audioFile.GetChannelCount();
					for( FxSize i = 0; i < maxIndex; ++i )
					{
						// Rearrange the bits into something intelligible.
						FX_SWAP_BIG_ENDIAN_BYTES(pSample[i]);
					}
				}
				else
#endif
					// 8-bit AIFF samples are signed, so we have to normalize
					// them to unsigned values.
					if( audioFile.GetBitsPerSample() == 8 )
					{
						FxInt8* pSample = reinterpret_cast<FxInt8*>(sampleBuffer);
						FxSize maxIndex = audioFile.GetSampleCount() * audioFile.GetChannelCount();
						for( FxSize i = 0; i < maxIndex; ++i )
						{
							pSample[i] += 0x80;
						}
					}

					audioFile.SetSampleBuffer(sampleBuffer);

					// Found the data.
					foundData = FxTrue;
			}
			else
			{
				// Skip over the chunk.
				input.ignore(chunkHeader.chunkSize);
			}
		}

		// Success!
		if( foundHeader && foundData )
		{
			return FxTrue;
		}
	}
	return FxFalse;
}

FxBool FxAiffFormatHandler::Save( FxAudioFile& audioFile, const FxString& filename )
{
	FxAssert(filename.Length() > 0);
	FxAssert(audioFile.GetChannelCount() == 1);
	FxAssert(audioFile.GetSampleCount() > 0);
	FxAssert(audioFile.GetSampleBuffer() != NULL);
	if( SupportsSave() && IsSupportedExt(filename) )
	{
		// Calculate some byte counts.
		FxUInt32 sampleByteCount    = (audioFile.GetSampleCount() * audioFile.GetBitsPerSample()) / FX_CHAR_BIT;
		FxUInt32 aiffChunkByteCount = sizeof(kAiffFormType) + sizeof(ChunkHeader) + sizeof(AiffCommonChunk) + 
			                          sizeof(ChunkHeader) + sizeof(AiffSoundDataChunk) + sampleByteCount;

		// Open the file.
		ofstream output(filename.GetData(), ios::out | ios::binary);
		if( output.fail() )
		{
			return FxFalse;
		}

		// Set up the Form chunk.
		AiffFormChunk aiffFormChunk = { kFormChunkID, aiffChunkByteCount, kAiffFormType	};

		// Rearrange the bits into something intelligible.
		FX_SWAP_BIG_ENDIAN_BYTES(aiffFormChunk.chunkSize);

		// Write the Form chunk.
		output.write(reinterpret_cast<const FxChar*>(&aiffFormChunk), sizeof(aiffFormChunk));
		if( output.fail() )
		{
			return FxFalse;
		}

		// Set up the Common chunk header.
		ChunkHeader commonChunkHeader = { kCommonChunkID, sizeof(AiffCommonChunk) };

		// Rearrange the bits into something intelligible.
		FX_SWAP_BIG_ENDIAN_BYTES(commonChunkHeader.chunkSize);

		// Write the Common chunk header.
		output.write(reinterpret_cast<const FxChar*>(&commonChunkHeader), sizeof(commonChunkHeader));
		if( output.fail() )
		{
			return FxFalse;
		}

		// Set up the Common chunk.
		AiffCommonChunk aiffCommonChunk = { audioFile.GetChannelCount(), audioFile.GetSampleCount(), audioFile.GetBitsPerSample(), {0} };

		// Convert the sample rate to an extended floating point value.
		FxDReal sampleRate = static_cast<FxDReal>(audioFile.GetSampleRate());
		FxAiffFormatHandler::_doubleToExtended(sampleRate, aiffCommonChunk.sampleRate);

		// Rearrange the bits into something intelligible.
		FX_SWAP_BIG_ENDIAN_BYTES(aiffCommonChunk.channelCount);
		FX_SWAP_BIG_ENDIAN_BYTES(aiffCommonChunk.sampleCount);
		FX_SWAP_BIG_ENDIAN_BYTES(aiffCommonChunk.bitsPerSample);
		// Don't swap the bytes for the sample rate.  The Apple code provides it
		// in big endian format already.
		//FX_SWAP_BIG_ENDIAN_BYTES(aiffCommonChunk.sampleRate);

		// Write the Common chunk.
		output.write(reinterpret_cast<const FxChar*>(&aiffCommonChunk), sizeof(aiffCommonChunk));
		if( output.fail() )
		{
			return FxFalse;
		}

		// Set up the Sound Data chunk header.
		ChunkHeader soundDataChunkHeader = { kSoundDataChunkID,	sizeof(AiffSoundDataChunk) + sampleByteCount };

		// Rearrange the bits into something intelligible.
		FX_SWAP_BIG_ENDIAN_BYTES(soundDataChunkHeader.chunkSize);

		// Write the Sound Data chunk header.
		output.write(reinterpret_cast<const FxChar*>(&soundDataChunkHeader), sizeof(soundDataChunkHeader));
		if( output.fail() )
		{
			return FxFalse;
		}

		// Set up the Sound Data chunk.
		AiffSoundDataChunk aiffSoundDataChunk = { 0, 0 };

		// Rearrange the bits into something intelligible.
		FX_SWAP_BIG_ENDIAN_BYTES(aiffSoundDataChunk.blockSize);
		FX_SWAP_BIG_ENDIAN_BYTES(aiffSoundDataChunk.offset);

		// Write the Sound Data chunk.
		output.write(reinterpret_cast<const FxChar*>(&aiffSoundDataChunk), sizeof(aiffSoundDataChunk));
		if( output.fail() )
		{
			return FxFalse;
		}

#if !defined(FX_BIG_ENDIAN)
		// For 16-bit samples, we have to swap the bytes...which means we
		// have to make a copy of the buffer.
		if( audioFile.GetBitsPerSample() == 16 )
		{
			// Make a copy of the sample buffer.
			FxSize sampleCount = audioFile.GetSampleCount();
			FxInt16* swapBuffer = new FxInt16[sampleCount];
			FxMemcpy(swapBuffer, audioFile.GetSampleBuffer(), sampleByteCount);

			// Byte-swap the new sample buffer.
            for( FxSize i = 0; i < sampleCount; ++i )
			{
				// Rearrange the bits into something intelligible.
				FX_SWAP_BIG_ENDIAN_BYTES(swapBuffer[i]);
			}

			// Write the byte-swapped sample buffer.
			output.write(reinterpret_cast<const FxChar*>(swapBuffer), sampleByteCount);

			// Free the byte-swapped sample buffer.
			delete [] swapBuffer;
			swapBuffer = 0;

			// Check for errors.
			if( output.fail() )
			{
				return FxFalse;
			}
		}
		else
#endif
			// 8-bit WAVE samples are unsigned, so we have to normalize them to
			// signed values.
			if( audioFile.GetBitsPerSample() == 8 )
			{
				// Make a copy of the sample buffer.
				FxSize sampleCount = audioFile.GetSampleCount();
				FxInt8* swapBuffer = new FxInt8[sampleCount];
				FxMemcpy(swapBuffer, audioFile.GetSampleBuffer(), sampleByteCount);

				// Normalize the new sample buffer.
				for( FxSize i = 0; i < sampleCount; ++i )
				{
					swapBuffer[i] -= 0x80;
				}

				// Write the new sample buffer.
				output.write(reinterpret_cast<const FxChar*>(swapBuffer), sampleByteCount);

				// Free the new sample buffer.
				delete [] swapBuffer;
				swapBuffer = 0;

				// Check for errors.
				if( output.fail() )
				{
					return FxFalse;
				}
			}

			// Success!
			return FxTrue;	
	}
	return FxFalse;
}

// The following code was pulled from SOX, and apparently, it
// originally came from Apple, so I'm leaving all this in. I only need it so I
// can convert a 10-byte (extended) double to a regular double and vice versa,
// and then only when working with AIFF files. It's been cleaned up a bit, but
// it's essentially the original code.

// =============================================================================
// Copyright © 1988-1991 Apple Computer, Inc.
// All rights reserved.
//
// Machine-independent I/O routines for IEEE floating-point numbers.
//
// NaN's and infinities are converted to HUGE_VAL or HUGE, which
// happens to be infinity on IEEE machines.  Unfortunately, it is
// impossible to preserve NaN's in a machine-independent way.
// Infinities are, however, preserved on IEEE machines.
//
// These routines have been tested on the following machines:
//    Apple Macintosh, MPW 3.1 C compiler
//    Apple Macintosh, THINK C compiler
//    Silicon Graphics IRIS, MIPS compiler
//    Cray X/MP and Y/MP
//    Digital Equipment VAX
//
// Implemented by Malcolm Slaney and Ken Turkowski.
//
// Malcolm Slaney contributions during 1988-1990 include big- and little-
// endian file I/O, conversion to and from Motorola's extended 80-bit
// floating-point format, and conversions to and from IEEE single-
// precision floating-point format.
//
// In 1991, Ken Turkowski implemented the conversions to and from
// IEEE double-precision format, added more precision to the extended
// conversions, and accommodated conversions involving +/- infinity,
// NaN's, and denormalized numbers.

#define FX_REAL_TO_UNSIGNED_LONG(f) \
	((FxUInt32)(((FxInt32)(f - 2147483648.0)) + 2147483647L) + 1)
#define FX_UNSIGNED_LONG_TO_REAL(u) \
	(((FxDReal)((FxInt32)(u - 2147483647L - 1))) + 2147483648.0)

//@todo Note that I did not provide wrappers for HUGE_VAL, frexp, and ldexp in
//      the code below so we may have to revisit this code on other platforms.

void FxAiffFormatHandler::_doubleToExtended( FxDReal& value, FxUInt8* pExtended )
{
	FxAssert(pExtended != NULL);

	if( pExtended != NULL )
	{
		FxInt32  sign         = 0;
		FxInt32  exponent     = 0;
		FxUInt32 highMantissa = 0;
		FxUInt32 lowMantissa  = 0;

		if( value < 0.0 )
		{
			sign = 0x8000;
			value *= -1.0;
		}

		if( value == 0.0 )
		{
			exponent     = 0;
			highMantissa = 0;
			lowMantissa  = 0;
		}
		else
		{
			FxDReal fMant = frexp(value, &exponent);
			if( (exponent > 16384) || !(fMant < 1.0) )
			{
				// Infinity or NaN.
				exponent     = sign | 0x7FFF;
				highMantissa = 0;
				lowMantissa  = 0;
				// Infinity.
			}
			else
			{
				// Finite.
				exponent += 16382;
				if( exponent < 0 )
				{
					// Denormalized.
					fMant = ldexp(fMant, exponent);
					exponent = 0;
				}
				exponent |= sign;
				fMant = ldexp(fMant, 32);
				FxDReal fsMant = FxFloor(fMant);
				highMantissa = FX_REAL_TO_UNSIGNED_LONG(fsMant);
				fMant = ldexp(fMant - fsMant, 32);
				fsMant = FxFloor(fMant);
				lowMantissa = FX_REAL_TO_UNSIGNED_LONG(fsMant);
			}
		}

		pExtended[0] = exponent >> 8;
		pExtended[1] = exponent;
		pExtended[2] = highMantissa >> 24;
		pExtended[3] = highMantissa >> 16;
		pExtended[4] = highMantissa >> 8;
		pExtended[5] = highMantissa;
		pExtended[6] = lowMantissa >> 24;
		pExtended[7] = lowMantissa >> 16;
		pExtended[8] = lowMantissa >> 8;
		pExtended[9] = lowMantissa;
	}
}

void FxAiffFormatHandler::_extendedToDouble( FxUInt8 const* pExtended, FxDReal& value )
{
	FxAssert(pExtended != NULL);
	value = 0.0;
	if( pExtended != NULL )
	{
		FxInt32 exponent = ((pExtended[0] & 0x7F) << 8) | (pExtended[1] & 0xFF);
		FxUInt32 highMantissa =
			  ((FxUInt32)(pExtended[2] & 0xFF) << 24)
			| ((FxUInt32)(pExtended[3] & 0xFF) << 16)
			| ((FxUInt32)(pExtended[4] & 0xFF) << 8)
			| ((FxUInt32)(pExtended[5] & 0xFF));
		FxUInt32 lowMantissa =
			  ((FxUInt32)(pExtended[6] & 0xFF) << 24)
			| ((FxUInt32)(pExtended[7] & 0xFF) << 16)
			| ((FxUInt32)(pExtended[8] & 0xFF) << 8)
			| ((FxUInt32)(pExtended[9] & 0xFF));

		if( (exponent != 0) || (highMantissa != 0) || (lowMantissa != 0) )
		{
			if( exponent == 0x7FFF )
			{
				// Infinity or NaN.
				value = HUGE_VAL;
			}
			else
			{
				// Remove the bias.
				exponent -= 16383;
				exponent -= 31;
				value = ldexp(FX_UNSIGNED_LONG_TO_REAL(highMantissa), exponent);
				exponent -= 32;
				value += ldexp(FX_UNSIGNED_LONG_TO_REAL(lowMantissa), exponent);
			}
		}

		// Check the sign bit.
		if( pExtended[0] & 0x80 )
		{
			value *= -1.0;
		}
	}
}

} // namespace Face

} // namespace OC3Ent
