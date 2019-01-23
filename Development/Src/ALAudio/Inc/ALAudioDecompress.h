/*=============================================================================
	ALAudioDecompress.h: Unreal OpenAL audio interface object.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_ALAUDIODECOMPRESS
#define _INC_ALAUDIODECOMPRESS

// 108ms of 44.1KHz data
// 216ms of 22KHz data
#define MONO_PCM_BUFFER_SAMPLES		10240
#define MONO_PCM_BUFFER_SIZE		( MONO_PCM_BUFFER_SAMPLES * sizeof( SWORD ) )

class FCompressedAudioInfo
{
public: 
	FCompressedAudioInfo( void )
	{
	}

	virtual ~FCompressedAudioInfo( void )
	{
	}

	/** 
	 * Set up the header information for the TTS decompressor
	 */
	virtual UBOOL	ReadCompressedInfo( USoundNodeWave* Wave )
	{
		return( FALSE );
	}

	/** 
	 * Extract the PCM data from the compressed format
	 * 
	 * @param	PCMData		where to place the decompressed sound
	 * @param	bLooping	whether to loop the sound by seeking to the start, or pad the buffer with zeroes
	 * @param	BufferSize	number of bytes of PCM data to create
	 *
	 * @return	UBOOL		TRUE if the end of the data was reached (for both single shot and looping sounds)
	 */
	virtual UBOOL	ReadCompressedData( TArray<BYTE>* PCMData, INT NumChannels, UBOOL bLooping, DWORD BufferSize = 0 );
};

/** 
 * Helper class to parse ogg vorbis data
 */
class FVorbisAudioInfo : public FCompressedAudioInfo
{
public:
	FVorbisAudioInfo( void ) 
	{ 
		SrcBufferData = NULL;
		SrcBufferDataSize = 0; 
	}

	virtual ~FVorbisAudioInfo( void ) 
	{ 
		ov_clear( &vf ); 
	}

	/** Emulate read from memory functionality */
	size_t			Read( void *ptr, DWORD size );
	int				Seek( DWORD offset, int whence );
	int				Close( void );
	long			Tell( void );

	/** 
	 * Reads the header information of an ogg vorbis file
	 * 
	 * @param	OggData		raw binary ogg file
	 */
	virtual UBOOL	ReadCompressedInfo( USoundNodeWave* Wave );

	/** 
	 * Decompresses ogg data to raw PCM data. 
	 * 
	 * @param	PCMData		where to place the decompressed sound
	 * @param	bLooping	whether to loop the sound by seeking to the start, or pad the buffer with zeroes
	 * @param	BufferSize	number of bytes of PCM data to create
	 *
	 * @return	UBOOL		TRUE if the end of the data was reached (for both single shot and looping sounds)
	 */
	virtual UBOOL	ReadCompressedData( TArray<BYTE>* PCMData, INT NumChannels, UBOOL bLooping, DWORD BufferSize = 0 );

	/** 
	 * Decompress an entire ogg data file to a TArray
	 */
	void			ExpandFile( USoundNodeWave* Wave, TArray<BYTE>* PCMData );

	/** Ogg vorbis decompression state */
	OggVorbis_File	vf;

private:
	const BYTE*		SrcBufferData;
	DWORD			SrcBufferDataSize;
	DWORD			BufferOffset;

public:

/** 
 * Constants used in the vorbis conversion
 */
static const __m128	ConstMax;
static const __m128	ConstMin; 
};

#endif
