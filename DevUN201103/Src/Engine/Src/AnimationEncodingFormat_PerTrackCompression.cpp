/*=============================================================================
	AnimationEncodingFormat_PerTrackCompression.cpp: Per-track decompressor
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "EngineAudioDeviceClasses.h"
#include "AnimationCompression.h"
#include "AnimationEncodingFormat_PerTrackCompression.h"
#include "AnimationUtils.h"


/**
 * Handles Byte-swapping a single track of animation data from a MemoryReader or to a MemoryWriter
 *
 * @param	Seq					The Animation Sequence being operated on.
 * @param	MemoryStream		The MemoryReader or MemoryWriter object to read from/write to.
 * @param	Offset				The starting offset into the compressed byte stream for this track (can be INDEX_NONE to indicate an identity track)
 */
void AEFPerTrackCompressionCodec::ByteSwapOneTrack(UAnimSequence& Seq, FMemoryArchive& MemoryStream, INT Offset)
{
	// Translation data.
	if (Offset != INDEX_NONE)
	{
		checkSlow( (Offset % 4) == 0 && "CompressedByteStream not aligned to four bytes" );

		BYTE* TrackData = Seq.CompressedByteStream.GetTypedData() + Offset;

		// Read the header
		AC_UnalignedSwap(MemoryStream, TrackData, sizeof(INT));

		const INT Header = *(reinterpret_cast<INT*>(TrackData - sizeof(INT)));


		INT KeyFormat;
		INT NumKeys;
		INT FormatFlags;
		FAnimationCompression_PerTrackUtils::DecomposeHeader(Header, /*OUT*/ KeyFormat, /*OUT*/ NumKeys, /*OUT*/ FormatFlags);

		INT FixedComponentSize = 0;
		INT FixedComponentCount = 0;
		INT KeyComponentSize = 0;
		INT KeyComponentCount = 0;
		FAnimationCompression_PerTrackUtils::GetAllSizesFromFormat(KeyFormat, FormatFlags, /*OUT*/ KeyComponentCount, /*OUT*/ KeyComponentSize, /*OUT*/ FixedComponentCount, /*OUT*/ FixedComponentSize);

		// Handle per-track metadata
		for (INT i = 0; i < FixedComponentCount; ++i)
		{
			AC_UnalignedSwap(MemoryStream, TrackData, FixedComponentSize);
		}

		// Handle keys
		for (INT KeyIndex = 0; KeyIndex < NumKeys; ++KeyIndex)
		{
			for (INT i = 0; i < KeyComponentCount; ++i)
			{
				AC_UnalignedSwap(MemoryStream, TrackData, KeyComponentSize);
			}
		}

		// Handle the key frame table if present
		if ((FormatFlags & 0x8) != 0)
		{
			// Make sure the key->frame table is 4 byte aligned
			PreservePadding(TrackData, MemoryStream);

			const INT FrameTableEntrySize = (Seq.NumFrames <= 0xFF) ? sizeof(BYTE) : sizeof(WORD);
			for (INT i = 0; i < NumKeys; ++i)
			{
				AC_UnalignedSwap(MemoryStream, TrackData, FrameTableEntrySize);
			}
		}

		// Make sure the next track is 4 byte aligned
		PreservePadding(TrackData, MemoryStream);
	}
}

/**
 * Preserves 4 byte alignment within a stream
 *
 * @param	TrackData [inout]	The current data offset (will be returned four byte aligned from the start of the compressed byte stream)
 * @param	MemoryStream		The MemoryReader or MemoryWriter object to read from/write to.
 */
void AEFPerTrackCompressionCodec::PreservePadding(BYTE*& TrackData, FMemoryArchive& MemoryStream)
{
	// Preserve padding
	const PTRINT ByteStreamLoc = (PTRINT) TrackData;
	const INT PadCount = static_cast<INT>( Align(ByteStreamLoc, 4) - ByteStreamLoc );
	if (MemoryStream.IsSaving())
	{
		const BYTE PadSentinel = 85; // (1<<1)+(1<<3)+(1<<5)+(1<<7)

		for (INT PadByteIndex = 0; PadByteIndex < PadCount; ++PadByteIndex)
		{
			MemoryStream.Serialize( (void*)&PadSentinel, sizeof(BYTE) );
		}
		TrackData += PadCount;
	}
	else
	{
		MemoryStream.Serialize(TrackData, PadCount);
		TrackData += PadCount;
	}
}

/**
 * Handles Byte-swapping incoming animation data from a MemoryReader
 *
 * @param	Seq					An Animation Sequence to contain the read data.
 * @param	MemoryReader		The MemoryReader object to read from.
 * @param	SourceArVersion		The version of the archive that the data is coming from.
 */
void AEFPerTrackCompressionCodec::ByteSwapIn(
	UAnimSequence& Seq, 
	FMemoryReader& MemoryReader,
	INT SourceArVersion)
{
	INT OriginalNumBytes = MemoryReader.TotalSize();
	Seq.CompressedByteStream.Empty(OriginalNumBytes);
	Seq.CompressedByteStream.Add(OriginalNumBytes);

	const INT NumTracks = Seq.CompressedTrackOffsets.Num() / 2;
	for ( INT TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex )
	{
		const INT OffsetTrans = Seq.CompressedTrackOffsets(TrackIndex*2+0);
		ByteSwapOneTrack(Seq, MemoryReader, OffsetTrans);

		const INT OffsetRot = Seq.CompressedTrackOffsets(TrackIndex*2+1);
		ByteSwapOneTrack(Seq, MemoryReader, OffsetRot);
	}
}


/**
 * Handles Byte-swapping outgoing animation data to an array of BYTEs
 *
 * @param	Seq					An Animation Sequence to write.
 * @param	SerializedData		The output buffer.
 * @param	ForceByteSwapping	TRUE is byte swapping is not optional.
 */
void AEFPerTrackCompressionCodec::ByteSwapOut(
	UAnimSequence& Seq,
	TArray<BYTE>& SerializedData, 
	UBOOL ForceByteSwapping)
{
	FMemoryWriter MemoryWriter(SerializedData, TRUE);
	MemoryWriter.SetByteSwapping(ForceByteSwapping);

	const INT NumTracks = Seq.CompressedTrackOffsets.Num() / 2;
	for ( INT TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex )
	{
		const INT OffsetTrans = Seq.CompressedTrackOffsets(TrackIndex*2+0);
		ByteSwapOneTrack(Seq, MemoryWriter, OffsetTrans);

		const INT OffsetRot = Seq.CompressedTrackOffsets(TrackIndex*2+1);
		ByteSwapOneTrack(Seq, MemoryWriter, OffsetRot);
	}
}



/**
 * Extracts a single BoneAtom from an Animation Sequence.
 *
 * @param	OutAtom			The BoneAtom to fill with the extracted result.
 * @param	Seq				An Animation Sequence to extract the BoneAtom from.
 * @param	TrackIndex		The index of the track desired in the Animation Sequence.
 * @param	Time			The time (in seconds) to calculate the BoneAtom for.
 * @param	bLooping		TRUE if the animation should be played in a cyclic manner.
 */
void AEFPerTrackCompressionCodec::GetBoneAtom(
	FBoneAtom& OutAtom,
	const UAnimSequence& Seq,
	INT TrackIndex,
	FLOAT Time,
	UBOOL bLooping)
{
	// Initialize to identity to set the scale and in case of a missing rotation or translation codec
	OutAtom.SetIdentity();

	// Use the CompressedTrackOffsets stream to find the data addresses
	const INT* RESTRICT TrackData= Seq.CompressedTrackOffsets.GetTypedData() + (TrackIndex * 2);
	const INT TransKeysOffset = TrackData[0];
	const INT RotKeysOffset = TrackData[1];
	const FLOAT RelativePos = Time / (FLOAT)Seq.SequenceLength;

	GetBoneAtomTranslation(OutAtom, Seq, TransKeysOffset, Time, RelativePos, bLooping);
	GetBoneAtomRotation(OutAtom, Seq, RotKeysOffset, Time, RelativePos, bLooping);
}
	





void AEFPerTrackCompressionCodec::GetBoneAtomRotation(
	FBoneAtom& OutAtom,
	const UAnimSequence& Seq,
	INT Offset,
	FLOAT Time,
	FLOAT RelativePos,
	UBOOL bLooping)
{
	if (Offset != INDEX_NONE)
	{
		const BYTE* RESTRICT TrackData = Seq.CompressedByteStream.GetTypedData() + Offset + 4;
		const INT Header = *((INT*)(Seq.CompressedByteStream.GetTypedData() + Offset));

		INT KeyFormat;
		INT NumKeys;
		INT FormatFlags;
		INT BytesPerKey;
		INT FixedBytes;
		FAnimationCompression_PerTrackUtils::DecomposeHeader(Header, /*OUT*/ KeyFormat, /*OUT*/ NumKeys, /*OUT*/ FormatFlags, /*OUT*/BytesPerKey, /*OUT*/ FixedBytes);

		// Figure out the key indexes
		INT Index0 = 0;
		INT Index1 = 0;
		FLOAT Alpha = 0.0f;

		if (NumKeys > 1)
		{
			if (FormatFlags & 0x8)
			{
				const BYTE* RESTRICT FrameTable = Align(TrackData + FixedBytes + BytesPerKey * NumKeys, 4);
				Alpha = TimeToIndex(Seq, FrameTable, RelativePos, bLooping, NumKeys, Index0, Index1);
			}
			else
			{
				Alpha = TimeToIndex(Seq, RelativePos, bLooping, NumKeys, Index0, Index1);
			}
		}

		// Unpack the first key
		const BYTE* RESTRICT KeyData0 = TrackData + FixedBytes + (Index0 * BytesPerKey);
		FQuat R0;
		FAnimationCompression_PerTrackUtils::DecompressRotation(KeyFormat, FormatFlags, R0, TrackData, KeyData0);

		// If there is a second key, figure out the lerp between the two of them
		if (Index0 != Index1)
		{
			const BYTE* RESTRICT KeyData1 = TrackData + FixedBytes + (Index1 * BytesPerKey);
			FQuat R1;
			FAnimationCompression_PerTrackUtils::DecompressRotation(KeyFormat, FormatFlags, R1, TrackData, KeyData1);

			// Fast linear quaternion interpolation.
			// To ensure the 'shortest route', we make sure the dot product between the two keys is positive.
			const FLOAT DotResult = (R0 | R1);
			const FLOAT Bias = appFloatSelect(DotResult, 1.0f, -1.0f);

			FQuat BlendedQuat = (R0 * (1.f-Alpha)) + (R1 * (Alpha * Bias));
			OutAtom.SetRotation( BlendedQuat );
			OutAtom.NormalizeRotation();
		}
		else // (Index0 == Index1)
		{
			OutAtom.SetRotation( R0 );
			OutAtom.NormalizeRotation();
		}
	}
	else
	{
		// Identity track
		OutAtom.SetRotation(FQuat::Identity);
	}
}



void AEFPerTrackCompressionCodec::GetBoneAtomTranslation(
	FBoneAtom& OutAtom,
	const UAnimSequence& Seq,
	INT Offset,
	FLOAT Time,
	FLOAT RelativePos,
	UBOOL bLooping)
{
	if (Offset != INDEX_NONE)
	{
		const BYTE* RESTRICT TrackData = Seq.CompressedByteStream.GetTypedData() + Offset + 4;
		const INT Header = *((INT*)(Seq.CompressedByteStream.GetTypedData() + Offset));

		INT KeyFormat;
		INT NumKeys;
		INT FormatFlags;
		INT BytesPerKey;
		INT FixedBytes;
		FAnimationCompression_PerTrackUtils::DecomposeHeader(Header, /*OUT*/ KeyFormat, /*OUT*/ NumKeys, /*OUT*/ FormatFlags, /*OUT*/BytesPerKey, /*OUT*/ FixedBytes);

		// Figure out the key indexes
		INT Index0 = 0;
		INT Index1 = 0;
		FLOAT Alpha = 0.0f;

		if (NumKeys > 1)
		{
			if (FormatFlags & 0x8)
			{
				const BYTE* RESTRICT FrameTable = Align(TrackData + FixedBytes + BytesPerKey * NumKeys, 4);
				Alpha = TimeToIndex(Seq, FrameTable, RelativePos, bLooping, NumKeys, Index0, Index1);
			}
			else
			{
				Alpha = TimeToIndex(Seq, RelativePos, bLooping, NumKeys, Index0, Index1);
			}
		}

		// Unpack the first key
		const BYTE* RESTRICT KeyData0 = TrackData + FixedBytes + (Index0 * BytesPerKey);
		FVector R0;
		FAnimationCompression_PerTrackUtils::DecompressTranslation(KeyFormat, FormatFlags, R0, TrackData, KeyData0);

		// If there is a second key, figure out the lerp between the two of them
		if (Index0 != Index1)
		{
			const BYTE* RESTRICT KeyData1 = TrackData + FixedBytes + (Index1 * BytesPerKey);
			FVector R1;
			FAnimationCompression_PerTrackUtils::DecompressTranslation(KeyFormat, FormatFlags, R1, TrackData, KeyData1);

			OutAtom.SetTranslation(Lerp(R0, R1, Alpha));
		}
		else // (Index0 == Index1)
		{
			OutAtom.SetTranslation(R0);
		}
	}
	else
	{
		// Identity track
		OutAtom.SetTranslation(FVector::ZeroVector);
	}
}

