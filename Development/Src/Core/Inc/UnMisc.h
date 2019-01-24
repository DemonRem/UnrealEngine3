/*=============================================================================
	UnMisc.h: Misc helper classes.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * Growable compressed buffer. Usage is to append frequently but only request and therefore decompress
 * very infrequently. The prime usage case is the memory profiler keeping track of full call stacks.
 */
struct FCompressedGrowableBuffer
{
	/**
	 * Constructor
	 *
	 * @param	MaxPendingBufferSize	Max chunk size to compress in uncompressed bytes
	 * @param	CompressionFlags		Compression flags to compress memory with
	 */
	FCompressedGrowableBuffer( INT MaxPendingBufferSize, ECompressionFlags CompressionFlags );

	/**
	 * Locks the buffer for reading. Needs to be called before calls to Access and needs
	 * to be matched up with Unlock call.
	 */
	void Lock();
	/**
	 * Unlocks the buffer and frees temporary resources used for accessing.
	 */
	void Unlock();

	/**
	 * Appends passed in data to the buffer. The data needs to be less than the max
	 * pending buffer size. The code will assert on this assumption.	 
	 *
	 * @param	Data	Data to append
	 * @param	Size	Size of data in bytes.
	 * @return	Offset of data, used for retrieval later on
	 */
	INT Append( void* Data, INT Size );

	/**
	 * Accesses the data at passed in offset and returns it. The memory is read-only and
	 * memory will be freed in call to unlock. The lifetime of the data is till the next
	 * call to Unlock, Append or Access
	 *
	 * @param	Offset	Offset to return corresponding data for
	 */
	void* Access( INT Offset );

	/**
	 * @return	Number of entries appended.
	 */
	INT Num()
	{
		return NumEntries;
	}

private:
	/** Helper structure for book keeping. */
	struct FBufferBookKeeping
	{
		/** Offset into compressed data.				*/
		INT CompressedOffset;
		/** Size of compressed data in this chunk.		*/
		INT CompressedSize;
		/** Offset into uncompressed data.				*/
		INT UncompressedOffset;
		/** Size of uncompressed data in this chunk.	*/
		INT UncompressedSize;
	};

	/** Maximum chunk size to compress in uncompressed bytes.				*/
	INT					MaxPendingBufferSize;
	/** Compression flags used to compress the data.						*/
	ECompressionFlags	CompressionFlags;
	/** Current offset in uncompressed data.								*/
	INT					CurrentOffset;
	/** Number of entries in buffer.										*/
	INT					NumEntries;
	/** Compressed data.													*/
	TArray<BYTE>		CompressedBuffer;
	/** Data pending compression once size limit is reached.				*/
	TArray<BYTE>		PendingCompressionBuffer;
	/** Temporary decompression buffer used between Lock/ Unlock.			*/
	TArray<BYTE>		DecompressedBuffer;
	/** Index into book keeping info associated with decompressed buffer.	*/
	INT					DecompressedBufferBookKeepingInfoIndex;
	/** Book keeping information for decompression/ access.					*/
	TArray<FBufferBookKeeping>	BookKeepingInfo;
};


/**
 * Serializes a string as ANSI char array.
 *
 * @param	String			String to serialize
 * @param	Ar				Archive to serialize with
 * @param	MinCharacters	Minimum number of characters to serialize.
 */
extern void SerializeStringAsANSICharArray( const FString& String, FArchive& Ar, INT MinCharacters=0 );

/**
 * Parses a string into tokens, separating switches (beginning with - or /) from
 * other parameters
 *
 * @param	CmdLine		the string to parse
 * @param	Tokens		[out] filled with all parameters found in the string
 * @param	Switches	[out] filled with all switches found in the string
 */
void appParseCommandLine(const TCHAR* CmdLine, TArray<FString>& Tokens, TArray<FString>& Switches);

/**
 * Function to encrypt and decrypt an array of bytes using obscurity
 * 
 * @param InAndOutData data to encrypt or decrypt, and also the result
 * @param Offset byte-offset to start encrypting/decrypting
 */
void SecurityByObscurityEncryptAndDecrypt(TArray<BYTE>& InAndOutData, INT Offset=0);





