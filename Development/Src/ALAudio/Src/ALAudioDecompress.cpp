/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "ALAudioPrivate.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack( push, 8 )
#endif

#include <vorbis/vorbisfile.h>
#include "ALAudioDecompress.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack( pop )
#endif

/*------------------------------------------------------------------------------------
	FCompressedAudioInfo.
------------------------------------------------------------------------------------*/

/** 
 * Create zeroed PCM data - stub implementation
 * 
 * @param	PCMData		where to place the silent sound
 * @param	bLooping	whether to loop the wav by seeking to the start, or pad the buffer with zeroes
 * @param	BufferSize	number of bytes of PCM data to create. A value of 0 means decompress the entire sound.
 *
 * @return	UBOOL		FALSE meaning the sound has not ended
 */
UBOOL FCompressedAudioInfo::ReadCompressedData( TArray<BYTE>* PCMData, INT NumChannels, UBOOL bLooping, DWORD BufferSize )
{
	PCMData->Empty( BufferSize * NumChannels );
	PCMData->Add( BufferSize * NumChannels );

	return( FALSE );
}

/*------------------------------------------------------------------------------------
	FVorbisAudioInfo.
------------------------------------------------------------------------------------*/

/** Emulate read from memory functionality */
size_t FVorbisAudioInfo::Read( void *Ptr, DWORD Size )
{
	size_t BytesToRead = Min( Size, SrcBufferDataSize - BufferOffset );
	appMemcpy( Ptr, SrcBufferData + BufferOffset, BytesToRead );
	BufferOffset += BytesToRead;
	return( BytesToRead );
}

static size_t OggRead( void *ptr, size_t size, size_t nmemb, void *datasource )
{
	FVorbisAudioInfo* OggInfo = ( FVorbisAudioInfo* )datasource;
	return( OggInfo->Read( ptr, size * nmemb ) );
}

int FVorbisAudioInfo::Seek( DWORD offset, int whence )
{
	switch( whence )
	{
	case SEEK_SET:
		BufferOffset = offset;
		break;

	case SEEK_CUR:
		BufferOffset += offset;
		break;

	case SEEK_END:
		BufferOffset = SrcBufferDataSize - offset;
		break;
	}

	return( BufferOffset );
}

static int OggSeek( void *datasource, ogg_int64_t offset, int whence )
{
	FVorbisAudioInfo* OggInfo = ( FVorbisAudioInfo* )datasource;
	return( OggInfo->Seek( offset, whence ) );
}

int FVorbisAudioInfo::Close( void )
{
	return( 0 );
}

static int OggClose( void *datasource )
{
	FVorbisAudioInfo* OggInfo = ( FVorbisAudioInfo* )datasource;
	return( OggInfo->Close() );
}

long FVorbisAudioInfo::Tell( void )
{
	return( BufferOffset );
}

static long OggTell( void *datasource )
{
	FVorbisAudioInfo *OggInfo = ( FVorbisAudioInfo* )datasource;
	return( OggInfo->Tell() );
}

/** 
 * Reads the header information of an ogg vorbis file
 * 
 * @param	Resource		Info about vorbis data
 */
UBOOL FVorbisAudioInfo::ReadCompressedInfo( USoundNodeWave* Wave )
{
	SCOPE_CYCLE_COUNTER( STAT_VorbisPrepareDecompressionTime );

	ov_callbacks		Callbacks;

	SrcBufferData = Wave->ResourceData;
	SrcBufferDataSize = Wave->ResourceSize;
	BufferOffset = 0;

	Callbacks.read_func = OggRead;
	Callbacks.seek_func = OggSeek;
	Callbacks.close_func = OggClose;
	Callbacks.tell_func = OggTell;

	// Set up the read from memory variables
	if( ov_open_callbacks( this, &vf, NULL, 0, Callbacks, sizeof( OggVorbis_File ) ) < 0 ) 
	{
		Wave->RemoveAudioResource();
		return( FALSE );
	}

	vorbis_info* vi = ov_info( &vf, -1 );
	Wave->SampleRate = vi->rate;
	Wave->NumChannels = vi->channels;
	Wave->SampleDataSize = ov_pcm_total( &vf, -1 ) * Wave->NumChannels * sizeof( SHORT );
	Wave->Duration = ( FLOAT )ov_time_total( &vf, -1 );

	return( TRUE );
}

/** 
 * Decompress an entire ogg vorbis data file to a TArray
 */
void FVorbisAudioInfo::ExpandFile( USoundNodeWave* Wave, TArray<BYTE>* PCMData )
{
	char*		Destination;
	DWORD		BytesRead, TotalBytesRead, BytesToRead;

	// A zero buffer size means decompress the entire ogg vorbis stream to PCM.
	TotalBytesRead = 0;
	BytesToRead = Wave->SampleDataSize;

	PCMData->Empty( BytesToRead );
	PCMData->Add( BytesToRead );

	Destination = ( char* )PCMData->GetTypedData();
	while( TotalBytesRead < BytesToRead )
	{
		BytesRead = ov_read( &vf, Destination, BytesToRead - TotalBytesRead, 1, NULL );

		TotalBytesRead += BytesRead;
		Destination += BytesRead;
	}
}

/** 
 * Decompresses ogg vorbis data to raw PCM data. 
 * 
 * @param	PCMData		where to place the decompressed sound
 * @param	bLooping	whether to loop the wav by seeking to the start, or pad the buffer with zeroes
 * @param	BufferSize	number of bytes of PCM data to create. A value of 0 means decompress the entire sound.
 *
 * @return	UBOOL		TRUE if the end of the data was reached (for both single shot and looping sounds)
 */
UBOOL FVorbisAudioInfo::ReadCompressedData( TArray<BYTE>* PCMData, INT NumChannels, UBOOL bLooping, DWORD BufferSize )
{
	UBOOL		bLooped;
	char*		Destination;
	DWORD		BytesRead, TotalBytesRead, BytesToRead;

	SCOPE_CYCLE_COUNTER( STAT_VorbisDecompressTime );

	bLooped = FALSE;

	PCMData->Empty( BufferSize * NumChannels );
	PCMData->Add( BufferSize * NumChannels );

	// Work out number of samples to read
	BytesToRead = BufferSize * NumChannels;
	TotalBytesRead = 0;
	Destination = ( char* )PCMData->GetTypedData();

	while( TotalBytesRead < BytesToRead )
	{
		BytesRead = ov_read( &vf, Destination, BytesToRead - TotalBytesRead, 1, NULL );

		if( !BytesRead )
		{
			// We've reached the end
			bLooped = TRUE;
			if( bLooping )
			{
				ov_pcm_seek_page( &vf, 0 );
			}
			else
			{
				BYTE* Destination = PCMData->GetTypedData() + TotalBytesRead;
				INT Count = ( BytesToRead - TotalBytesRead );
				appMemzero( Destination, Count );

				BytesRead += BytesToRead - TotalBytesRead;
			}
		}

		TotalBytesRead += BytesRead;
		Destination += BytesRead;
	}

	return( bLooped );
}

// end
