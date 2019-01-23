/*=============================================================================
	AnimationCompressionAlgorithm.cpp: Skeletal mesh animation compression.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "AnimationCompression.h"
#include "AnimationUtils.h"
#include "FloatPacker.h"

IMPLEMENT_CLASS(UAnimationCompressionAlgorithm);

// Writes the specified data to Seq->CompresedByteStream with four-byte alignment.
#define AC_UnalignedWriteToStream( Src, Len )										\
	{																				\
		const INT Ofs = Seq->CompressedByteStream.Add( Len );						\
		appMemcpy( Seq->CompressedByteStream.GetTypedData()+Ofs, (Src), (Len) );	\
	}

static const BYTE PadSentinel = 85; //(1<<1)+(1<<3)+(1<<5)+(1<<7)

static void BitwiseCompress(UAnimSequence* Seq, AnimationCompressionFormat TargetTranslationFormat, AnimationCompressionFormat TargetRotationFormat)
{
	// Ensure supported compression formats.
	UBOOL bInvalidCompressionFormat = FALSE;
	if( !(TargetTranslationFormat == ACF_None) )
	{
		appMsgf( AMT_OK, TEXT("Unknown or unsupported translation compression format (%i)"), (INT)TargetTranslationFormat );
		bInvalidCompressionFormat = TRUE;
	}
	if ( !(TargetRotationFormat >= ACF_None && TargetRotationFormat < ACF_MAX) )
	{
		appMsgf( AMT_OK, TEXT("Unknown or unsupported rotation compression format (%i)"), (INT)TargetRotationFormat );
		bInvalidCompressionFormat = TRUE;
	}
	if ( bInvalidCompressionFormat )
	{
		Seq->TranslationCompressionFormat	= ACF_None;
		Seq->RotationCompressionFormat		= ACF_None;
		Seq->CompressedTrackOffsets.Empty();
		Seq->CompressedByteStream.Empty();
	}
	else
	{
		Seq->RotationCompressionFormat		= TargetRotationFormat;
		Seq->TranslationCompressionFormat	= TargetTranslationFormat;

		check( Seq->TranslationData.Num() == Seq->RotationData.Num() );
		const INT NumTracks = Seq->RotationData.Num();

		if ( NumTracks == 0 )
		{
			debugf( NAME_Warning, TEXT("When compressing %s: no key-reduced data"), *Seq->SequenceName.ToString() );
		}

		Seq->CompressedTrackOffsets.Empty( NumTracks*4 );
		Seq->CompressedTrackOffsets.Add( NumTracks*4 );
		Seq->CompressedByteStream.Empty();

		for ( INT TrackIndex = 0 ; TrackIndex < NumTracks ; ++TrackIndex )
		{
			// Translation data.
			const FTranslationTrack& SrcTrans	= Seq->TranslationData(TrackIndex);

			const INT OffsetTrans				= Seq->CompressedByteStream.Num();
			const INT NumKeysTrans				= SrcTrans.PosKeys.Num();

			// Warn on empty data.
			if ( NumKeysTrans == 0 )
			{
				debugf( NAME_Warning, TEXT("When compressing %s track %i: no translation keys"), *Seq->SequenceName.ToString(), TrackIndex );
			}

			checkMsg( (OffsetTrans % 4) == 0, "CompressedByteStream not aligned to four bytes" );
			Seq->CompressedTrackOffsets(TrackIndex*4) = OffsetTrans;
			Seq->CompressedTrackOffsets(TrackIndex*4+1) = NumKeysTrans;

			for ( INT KeyIndex = 0 ; KeyIndex < NumKeysTrans ; ++KeyIndex )
			{
				// A translation track of n keys is packed as n uncompressed float[3].
				AC_UnalignedWriteToStream( &(SrcTrans.PosKeys(KeyIndex)), sizeof(FVector) );
			}

			// Compress rotation data.
			const FRotationTrack& SrcRot	= Seq->RotationData(TrackIndex);
			const INT OffsetRot				= Seq->CompressedByteStream.Num();
			const INT NumKeysRot			= SrcRot.RotKeys.Num();

			checkMsg( (OffsetRot % 4) == 0, "CompressedByteStream not aligned to four bytes" );
			Seq->CompressedTrackOffsets(TrackIndex*4+2) = OffsetRot;
			Seq->CompressedTrackOffsets(TrackIndex*4+3) = NumKeysRot;

			if ( NumKeysRot > 1 )
			{
				// For a rotation track of n>1 keys, the first 24 bytes are reserved for compression info
				// (eg Fixed32 stores float Mins[3]; float Ranges[3]), followed by n elements of the compressed type.
				// Compute Mins and Ranges for rotation X,Y,Zs.
				FLOAT MinX = 1.f;
				FLOAT MinY = 1.f;
				FLOAT MinZ = 1.f;
				FLOAT MaxX = -1.f;
				FLOAT MaxY = -1.f;
				FLOAT MaxZ = -1.f;
				for ( INT KeyIndex = 0 ; KeyIndex < SrcRot.RotKeys.Num() ; ++KeyIndex )
				{
					FQuat Quat( SrcRot.RotKeys(KeyIndex) );
					if ( Quat.W < 0.f )
					{
						Quat.X = -Quat.X;
						Quat.Y = -Quat.Y;
						Quat.Z = -Quat.Z;
						Quat.W = -Quat.W;
					}
					Quat.Normalize();

					MinX = ::Min( MinX, Quat.X );
					MaxX = ::Max( MaxX, Quat.X );
					MinY = ::Min( MinY, Quat.Y );
					MaxY = ::Max( MaxY, Quat.Y );
					MinZ = ::Min( MinZ, Quat.Z );
					MaxZ = ::Max( MaxZ, Quat.Z );
				}
				const FLOAT Mins[3]	= { MinX,		MinY,		MinZ };
				FLOAT Ranges[3]		= { MaxX-MinX,	MaxY-MinY,	MaxZ-MinZ };
				if ( Ranges[0] == 0.f ) { Ranges[0] = 1.f; }
				if ( Ranges[1] == 0.f ) { Ranges[1] = 1.f; }
				if ( Ranges[2] == 0.f ) { Ranges[2] = 1.f; }

				AC_UnalignedWriteToStream( Mins, sizeof(FLOAT)*3 );
				AC_UnalignedWriteToStream( Ranges, sizeof(FLOAT)*3 );

				// n elements of the compressed type.
				for ( INT KeyIndex = 0 ; KeyIndex < SrcRot.RotKeys.Num() ; ++KeyIndex )
				{
					const FQuat& Quat = SrcRot.RotKeys(KeyIndex);
					if ( TargetRotationFormat == ACF_None )
					{
						AC_UnalignedWriteToStream( &SrcRot.RotKeys(KeyIndex), sizeof(FQuat) );
					}
					else if ( TargetRotationFormat == ACF_Float96NoW )
					{
						const FQuatFloat96NoW QuatFloat96NoW( Quat );
						AC_UnalignedWriteToStream( &QuatFloat96NoW, sizeof(FQuatFloat96NoW) );
					}
					else if ( TargetRotationFormat == ACF_Fixed32NoW )
					{
						const FQuatFixed32NoW QuatFixed32NoW( Quat );
						AC_UnalignedWriteToStream( &QuatFixed32NoW, sizeof(FQuatFixed32NoW) );
					}
					else if ( TargetRotationFormat == ACF_Fixed48NoW )
					{
						const FQuatFixed48NoW QuatFixed48NoW( Quat );
						AC_UnalignedWriteToStream( &QuatFixed48NoW, sizeof(FQuatFixed48NoW) );
					}
					else if ( TargetRotationFormat == ACF_IntervalFixed32NoW )
					{
						const FQuatIntervalFixed32NoW QuatIntervalFixed32NoW( Quat, Mins, Ranges );
						AC_UnalignedWriteToStream( &QuatIntervalFixed32NoW, sizeof(FQuatIntervalFixed32NoW) );
					}
					else if ( TargetRotationFormat == ACF_Float32NoW )
					{
						const FQuatFloat32NoW QuatFloat32NoW( Quat );
						AC_UnalignedWriteToStream( &QuatFloat32NoW, sizeof(FQuatFloat32NoW) );
					}
				}
			}
			else if ( NumKeysRot == 1 )
			{
				// For a rotation track of n=1 keys, the single key is packed as an FQuatFloat96NoW.
				const FQuat& Quat = SrcRot.RotKeys(0);
				const FQuatFloat96NoW QuatFloat96NoW( Quat );
				AC_UnalignedWriteToStream( &QuatFloat96NoW, sizeof(FQuatFloat96NoW) );
			}
			else
			{
				debugf( NAME_Warning, TEXT("When compressing %s track %i: no rotation keys"), *Seq->SequenceName.ToString(), TrackIndex );
			}

			// Align to four bytes.
			//const INT Pad = Seq->CompressedByteStream.Num() % 4;
			const INT Pad = Align( Seq->CompressedByteStream.Num(), 4 ) - Seq->CompressedByteStream.Num();
			if ( Pad )
			{
				const INT PadOfs = Seq->CompressedByteStream.Add( Pad );
				BYTE* Data = Seq->CompressedByteStream.GetTypedData() + PadOfs;
				for ( INT i = 0 ; i < Pad ; ++i )
				{
					Data[i] = PadSentinel;
				}
			}
		}

		// Trim unused memory.
		Seq->CompressedByteStream.Shrink();
	}

	// Finally, whack key-reduced data.
	Seq->TranslationData.Empty();
	Seq->RotationData.Empty();
}

/**
 * Tracks
 */
class FCompressionMemorySummary
{
public:
	FCompressionMemorySummary(UBOOL bInEnabled)
		:	bEnabled( bInEnabled )
		,	TotalRaw( 0 )
		,	TotalBeforeCompressed( 0 )
		,	TotalAfterCompressed( 0 )
	{
		if ( bEnabled )
		{
			GWarn->BeginSlowTask( TEXT("Compressing animations"), TRUE );
		}
	}

	void GatherPreCompressionStats(UAnimSequence* Seq, INT ProgressNumerator, INT ProgressDenominator)
	{
		if ( bEnabled )
		{
			GWarn->StatusUpdatef( ProgressNumerator,
									ProgressDenominator,
									*FString::Printf( TEXT("Compressing %s (%i/%i)"), *Seq->SequenceName.ToString(), ProgressNumerator, ProgressDenominator ) );

			TotalRaw += Seq->GetApproxRawSize();
			TotalBeforeCompressed += Seq->GetApproxCompressedSize();
		}
	}

	void GatherPostCompressionStats(UAnimSequence* Seq)
	{
		if ( bEnabled )
		{
			TotalAfterCompressed += Seq->GetApproxCompressedSize();
		}
	}

	~FCompressionMemorySummary()
	{
		if ( bEnabled )
		{
			GWarn->EndSlowTask();

			const FLOAT TotalRawKB					= static_cast<FLOAT>(TotalRaw)/1024.f;
			const FLOAT TotalBeforeCompressedKB		= static_cast<FLOAT>(TotalBeforeCompressed)/1024.f;
			const FLOAT TotalAfterCompressedKB		= static_cast<FLOAT>(TotalAfterCompressed)/1024.f;
			const FLOAT TotalBeforeSavingKB			= TotalRawKB - TotalBeforeCompressedKB;
			const FLOAT TotalAfterSavingKB			= TotalRawKB - TotalAfterCompressedKB;
			const FLOAT OldCompressionRatio			= (TotalBeforeCompressedKB > 0.f) ? (TotalRawKB / TotalBeforeCompressedKB) : 0.f;
			const FLOAT NewCompressionRatio			= (TotalAfterCompressedKB > 0.f) ? (TotalRawKB / TotalAfterCompressedKB) : 0.f;

			appMsgf( AMT_OK,
				TEXT("Raw: %7.2fKB - Compressed: %7.2fKB\nSaving: %7.2fKB (%.2f)\n")
				TEXT("Raw: %7.2fKB - Compressed: %7.2fKB\nSaving: %7.2fKB (%.2f)\n"),
				TotalRawKB, TotalBeforeCompressedKB, TotalBeforeSavingKB, OldCompressionRatio,
				TotalRawKB, TotalAfterCompressedKB, TotalAfterSavingKB, NewCompressionRatio );
		}
	}

private:
	UBOOL bEnabled;
	INT TotalRaw;
	INT TotalBeforeCompressed;
	INT TotalAfterCompressed;
};

/**
 * Reduce the number of keyframes and bitwise compress the specified sequence.
 *
 * @param	AnimSet		The animset to compress.
 * @param	SkelMesh	The skeletal mesh against which to compress the animation.  Not needed by all compression schemes.
 * @param	bOutput		If FALSE don't generate output or compute memory savings.
 * @return				FALSE if a skeleton was needed by the algorithm but not provided.
 */
UBOOL UAnimationCompressionAlgorithm::Reduce(UAnimSequence* AnimSeq, USkeletalMesh* SkelMesh, UBOOL bOutput)
{
	UBOOL bResult = FALSE;

	const UBOOL bSekeltonExistsIfNeeded = ( SkelMesh || !bNeedsSkeleton );
	if ( bSekeltonExistsIfNeeded )
	{
		// Build skeleton metadata to use during the key reduction.
		TArray<FBoneData> BoneData;
		if ( SkelMesh )
		{
			FAnimationUtils::BuildSekeltonMetaData( SkelMesh->RefSkeleton, BoneData );
		}

		UAnimationCompressionAlgorithm* TrivialKeyRemover =
			ConstructObject<UAnimationCompressionAlgorithm>( UAnimationCompressionAlgorithm_RemoveTrivialKeys::StaticClass() );

		FCompressionMemorySummary CompressionMemorySummary( bOutput );
		CompressionMemorySummary.GatherPreCompressionStats( AnimSeq, 0, 1 );

		// Remove trivial keys.
		TrivialKeyRemover->DoReduction( AnimSeq, SkelMesh, BoneData );

		// General key reduction.
		DoReduction( AnimSeq, SkelMesh, BoneData );

		// Bitwise compression.
		BitwiseCompress( AnimSeq,
						static_cast<AnimationCompressionFormat>(TranslationCompressionFormat),
						static_cast<AnimationCompressionFormat>(RotationCompressionFormat) );

		AnimSeq->CompressionScheme = static_cast<UAnimationCompressionAlgorithm*>( StaticDuplicateObject( this, this, AnimSeq, TEXT("None"), ~RF_RootSet ) );
		AnimSeq->MarkPackageDirty();

		CompressionMemorySummary.GatherPostCompressionStats( AnimSeq );

		bResult = TRUE;
	}

	return bResult;
}

/**
 * Reduce the number of keyframes and bitwise compress all sequences in the specified animation set.
 *
 * @param	AnimSet		The animset to compress.
 * @param	SkelMesh	The skeletal mesh against which to compress the animation.  Not needed by all compression schemes.
 * @param	bOutput		If FALSE don't generate output or compute memory savings.
 * @return				FALSE if a skeleton was needed by the algorithm but not provided.
 */
UBOOL UAnimationCompressionAlgorithm::Reduce(UAnimSet* AnimSet, USkeletalMesh* SkelMesh, UBOOL bOutput)
{
	UBOOL bResult = FALSE;

	const UBOOL bSekeltonExistsIfNeeded = ( SkelMesh || !bNeedsSkeleton );
	if ( bSekeltonExistsIfNeeded )
	{
		// Build skeleton metadata to use during the key reduction.
		TArray<FBoneData> BoneData;
		if ( SkelMesh )
		{
			FAnimationUtils::BuildSekeltonMetaData( SkelMesh->RefSkeleton, BoneData );
		}

		UAnimationCompressionAlgorithm* TrivialKeyRemover =
			ConstructObject<UAnimationCompressionAlgorithm>( UAnimationCompressionAlgorithm_RemoveTrivialKeys::StaticClass() );

		FCompressionMemorySummary CompressionMemorySummary( bOutput );

		for( INT SeqIndex = 0 ; SeqIndex < AnimSet->Sequences.Num() ; ++SeqIndex )
		{
			UAnimSequence* AnimSeq = AnimSet->Sequences(SeqIndex);
			CompressionMemorySummary.GatherPreCompressionStats( AnimSeq, SeqIndex, AnimSet->Sequences.Num() );

			// Remove trivial keys.
			TrivialKeyRemover->DoReduction( AnimSeq, SkelMesh, BoneData );

			// General key reduction.
			DoReduction( AnimSeq, SkelMesh, BoneData );

			// Bitwise compression.
			BitwiseCompress( AnimSeq,
							static_cast<AnimationCompressionFormat>(TranslationCompressionFormat),
							static_cast<AnimationCompressionFormat>(RotationCompressionFormat) );

			AnimSeq->CompressionScheme = static_cast<UAnimationCompressionAlgorithm*>( StaticDuplicateObject( this, this, AnimSeq, TEXT("None") ) );

			CompressionMemorySummary.GatherPostCompressionStats( AnimSeq );
		}

		AnimSet->MarkPackageDirty();
		bResult = TRUE;
	}

	return bResult;
}
