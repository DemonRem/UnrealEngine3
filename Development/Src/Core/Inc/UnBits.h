/*=============================================================================
	UnBits.h: Unreal bitstream manipulation classes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	FBitWriter.
-----------------------------------------------------------------------------*/

//
// Writes bitstreams.
//
struct FBitWriter : public FArchive
{
	friend struct FBitWriterMark;
public:
	/**
	 * Default constructor. Zeros everything.
	 */
	FBitWriter(void);
	/**
	 * Constructor using known size the buffer needs to be
	 */
	FBitWriter( INT InMaxBits );
	void SerializeBits( void* Src, INT LengthBits );
	void SerializeInt( DWORD& Value, DWORD Max );
	void WriteInt( DWORD Result, DWORD Max );
	void WriteBit( BYTE In );
	void Serialize( void* Src, INT LengthBytes );

	/**
	 * Returns a pointer to our internal buffer
	 */
	FORCEINLINE BYTE* GetData(void)
	{
		return &Buffer(0);
	}

	/**
	 * Returns the number of bytes written
	 */
	FORCEINLINE INT GetNumBytes(void)
	{
		return (Num+7)>>3;
	}

	/**
	 * Returns the number of bits written
	 */
	FORCEINLINE INT GetNumBits(void)
	{
		return Num;
	}

	/**
	 * Returns the number of bits the buffer supports
	 */
	FORCEINLINE INT GetMaxBits(void)
	{
		return Max;
	}

	/**
	 * Marks this bit writer as overflowed
	 */
	FORCEINLINE void SetOverflowed(void)
	{
		ArIsError = 1;
	}

	/**
	 * This assignment operator relies on the fast copy of the buffer from
	 * tarray. If this isn't present this code will be slow.
	 *
	 * @param In the bitwriter to copy
	 */
	FORCEINLINE FBitWriter& operator=(const FBitWriter& In)
	{
		// Simple copies
		Num = In.Num;
		Max = In.Max;
		// Use optimized tarray copy
		Buffer = In.Buffer;
		return *this;
	}

	/**
	 * Resets the bit writer back to its initial state
	 */
	void Reset(void);

private:
	TArray<BYTE> Buffer;
	INT   Num;
	INT   Max;
};

//
// For pushing and popping FBitWriter positions.
//
struct FBitWriterMark
{
public:
	FBitWriterMark()
	:	Num         ( 0 )
	{}
	FBitWriterMark( FBitWriter& Writer )
	:	Overflowed	( Writer.ArIsError )
	,	Num			( Writer.Num )
	{}
	INT GetNumBits()
	{
		return Num;
	}
	void Pop( FBitWriter& Writer );
private:
	UBOOL			Overflowed;
	INT				Num;
};

/*-----------------------------------------------------------------------------
	FBitReader.
-----------------------------------------------------------------------------*/

//
// Reads bitstreams.
//
struct FBitReader : public FArchive
{
public:
	FBitReader( BYTE* Src=NULL, INT CountBits=0 );
	void SetData( FBitReader& Src, INT CountBits );
	void SerializeBits( void* Dest, INT LengthBits );
	void SerializeInt( DWORD& Value, DWORD Max );
	DWORD ReadInt( DWORD Max );
	BYTE ReadBit();
	void Serialize( void* Dest, INT LengthBytes );
	BYTE* GetData();
	UBOOL AtEnd();
	void SetOverflowed();
	INT GetNumBytes();
	INT GetNumBits();
	INT GetPosBits();
private:
	TArray<BYTE> Buffer;
	INT   Num;
	INT   Pos;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

