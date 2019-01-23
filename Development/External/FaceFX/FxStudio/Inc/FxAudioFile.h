//------------------------------------------------------------------------------
// An audio file.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxAudioFile_H__
#define FxAudioFile_H__

#include "FxPlatform.h"
#include "FxString.h"
#include "FxArray.h"
#include "FxDigitalAudio.h"

namespace OC3Ent
{

namespace Face
{

class FxAudioFormatHandler;

class FxAudioFile
{
public:
	FxAudioFile();
	// Construct and load an audio file from disc.
    FxAudioFile( const FxString& filename );
	// Construct an audio file from an FxDigitalAudio object for saving to 
	// disc (for debugging only).
	FxAudioFile( FxDigitalAudio* pDigitalAudio );
	virtual ~FxAudioFile();

	static void Startup( void );
	static void Shutdown( void );
	static void RegisterAudioFormatHandler( FxAudioFormatHandler* pHandler );
	static FxArray<FxString> GetSupportedExtensions( void );
	static FxArray<FxString> GetDescriptions( void );

	FxBool Load( void );
	FxBool Save( const FxString& filename );

	const FxString& GetFilename( void ) const;
	FxBool IsValid( void ) const;
	FxInt32 GetBitsPerSample( void ) const;
	FxInt32 GetSampleRate( void ) const;
	FxInt32 GetChannelCount( void ) const;
	FxInt32 GetSampleCount( void ) const;
	FxByte* GetSampleBuffer( void ) const;

	void SetBitsPerSample( FxInt32 bitsPerSample );
	void SetSampleRate( FxInt32 sampleRate );
	void SetChannelCount( FxInt32 channelCount );
	void SetSampleCount( FxInt32 sampleCount );
	void SetSampleBuffer( FxByte* pSampleBuffer );

protected:
	FxString _filename;
	FxBool	 _isValid;
	FxInt32  _bitsPerSample;
	FxInt32  _sampleRate; 
	FxInt32  _channelCount; 
	FxInt32  _sampleCount;
    FxByte*  _pSampleBuffer;

	static FxArray<FxAudioFormatHandler*>* _formatHandlers;

	FxAudioFormatHandler* _findAudioFormatHandler( const FxString& filename );
};

class FxAudioFormatHandler
{
public:
	// Constructor.
	FxAudioFormatHandler( const FxString& fileExt, const FxString& desc, 
		                  FxBool supportsLoad, FxBool supportsSave )
						  : _fileExt(fileExt)
						  , _desc(desc)
						  , _supportsLoad(supportsLoad)
						  , _supportsSave(supportsSave) {}
	// Destructor.
	virtual ~FxAudioFormatHandler() {}

	// Returns the file extension that the audio format handler supports.
	const FxString& GetFileExt( void ) const { return _fileExt; }
	// Returns the description of the audio format.
	const FxString& GetDesc( void ) const { return _desc; }
	// Returns FxTrue if the audio format handler supports loading.
	FxBool SupportsLoad( void ) const { return _supportsLoad; }
	// Returns FxTrue if the audio format handler supports saving.
	FxBool SupportsSave( void ) const { return _supportsSave; }

	// Loads the file named filename to audioFile.
	virtual FxBool Load( FxAudioFile& audioFile, const FxString& filename ) = 0;
	// Saves audioFile to the file named filename.
	virtual FxBool Save( FxAudioFile& audioFile, const FxString& filename ) = 0;

	// Convenience function for determining if the filename has the supported
	// file extension.
	FxBool IsSupportedExt( const FxString& filename )
	{
		//@todo This is probably a not very portable assumption.
		if( !stricmp(filename.AfterLast('.').GetData(), _fileExt.GetData()) )
		{
			return FxTrue;
		}
		return FxFalse;
	}

protected:
	FxString _fileExt;
	FxString _desc;
	FxBool   _supportsLoad;
	FxBool   _supportsSave;
};

class FxWaveFormatHandler : public FxAudioFormatHandler
{
public:
	FxWaveFormatHandler() : FxAudioFormatHandler("wav", "Wave Files", FxTrue, FxTrue) {}
	virtual ~FxWaveFormatHandler() {}

	virtual FxBool Load( FxAudioFile& audioFile, const FxString& filename );
	virtual FxBool Save( FxAudioFile& audioFile, const FxString& filename );

protected:
#ifdef WIN32
	static FxBool _acmConvert( WAVEFORMATEX* pSrcWaveFormat, FxChar* pSrcSampleData, DWORD srcSampleDataBytes,
		                       WAVEFORMATEX* pDstWaveFormat, FxChar*& pDstSampleData, DWORD& dstSampleDataBytes );
#endif
};

class FxAiffFormatHandler : public FxAudioFormatHandler
{
public:
	FxAiffFormatHandler() : FxAudioFormatHandler("aif", "Aiff Files", FxTrue, FxTrue) {}
	virtual ~FxAiffFormatHandler() {}

	virtual FxBool Load( FxAudioFile& audioFile, const FxString& filename );
	virtual FxBool Save( FxAudioFile& audioFile, const FxString& filename );

protected:
	static void _doubleToExtended( FxDReal& value, FxUInt8* pExtended );
	static void _extendedToDouble( FxUInt8 const* pExtended, FxDReal& value );
};

} // namespace Face

} // namespace OC3Ent

#endif