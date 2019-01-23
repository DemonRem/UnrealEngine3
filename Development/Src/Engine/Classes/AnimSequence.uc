/**
 * One animation sequence of keyframes. Contains a number of tracks of data.
 * The Outer of AnimSequence is expected to be its AnimSet.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class AnimSequence extends Object
	native(Anim)
	hidecategories(Object);

/*
 * Triggers an animation notify.  Each AnimNotifyEvent contains an AnimNotify object
 * which has its Notify method called and passed to the animation.
 */
struct native AnimNotifyEvent
{
	var()	float						Time;
	var()	editinline AnimNotify		Notify;
	var()	name						Comment;
};

/**
 * Raw keyframe data for one track.  Each array will contain either NumKey elements or 1 element.
 * One element is used as a simple compression scheme where if all keys are the same, they'll be
 * reduced to 1 key that is constant over the entire sequence.
 */
struct native RawAnimSequenceTrack
{
	/** Position keys. */
	var array<vector>	PosKeys;

	/** Rotation keys. */
	var array<quat>		RotKeys;

	/** Key times, in seconds. */
	var array<float>	KeyTimes;
};

/** Name of the animation sequence. Used in AnimNodeSequence. */
var		name									SequenceName;

/** Animation notifies, sorted by time (earliest notification first). */
var()	editinline array<AnimNotifyEvent>		Notifies;

/** Length (in seconds) of this AnimSequence if played back with a speed of 1.0. */
var		float									SequenceLength;

/** Number of raw frames in this sequence (not used by engine - just for informational purposes). */
var		int										NumFrames;

/** Number for tweaking playback rate of this animation globally. */
var()	float									RateScale;

/** 
 * if TRUE, disable interpolation between last and first frame when looping.
 */
var()	bool									bNoLoopingInterpolation;

/**
 * Raw uncompressed keyframe data.
 */
var		const  array<RawAnimSequenceTrack>		RawAnimData;

/**
 * Keyframe position data for one track.  Pos(i) occurs at Time(i).  Pos.Num() always equals Time.Num().
 */
struct native TranslationTrack
{
	var array<vector>	PosKeys;
	var array<float>	Times;
};

/**
 * Keyframe rotation data for one track.  Rot(i) occurs at Time(i).  Rot.Num() always equals Time.Num().
 */
struct native RotationTrack
{
	var array<quat>		RotKeys;
	var array<float>	Times;
};

/**
 * Translation data post keyframe reduction.  TranslationData.Num() is zero if keyframe reduction
 * has not yet been applied.
 */
var transient const array<TranslationTrack>		TranslationData;

/**
 * Rotation data post keyframe reduction.  RotationData.Num() is zero if keyframe reduction
 * has not yet been applied.
 */
var transient const array<RotationTrack>		RotationData;

/**
 * The compression scheme that was most recently used to compress this animation.
 * May be NULL.
 */
var() editinline editconst editoronly AnimationCompressionAlgorithm	CompressionScheme;

/**
 * Indicates animation data compression format.
 */
enum AnimationCompressionFormat
{
	ACF_None,
	ACF_Float96NoW,
	ACF_Fixed48NoW,
	ACF_IntervalFixed32NoW,
	ACF_Fixed32NoW,
	ACF_Float32NoW,
};

/** The compression format that was used to compress translation tracks. */
var const AnimationCompressionFormat		TranslationCompressionFormat;

/** The compression format that was used to compress rotation tracks. */
var const AnimationCompressionFormat		RotationCompressionFormat;

struct native CompressedTrack
{
	var array<byte>		ByteStream;
	var array<float>	Times;
	var float			Mins[3];
	var float			Ranges[3]; 
};

/**
 * An array of 4*NumTrack ints, arranged as follows:
 *   [0] Trans0.Offset
 *   [1] Trans0.NumKeys
 *   [2] Rot0.Offset
 *   [3] Rot0.NumKeys
 *   [4] Trans1.Offset
 *   . . .
 */
var			array<int>		CompressedTrackOffsets;

/**
 * ByteStream for compressed animation data.
 * All keys are currently stored at evenly-spaced intervals (ie no explicit key times).
 *
 * For a translation track of n keys, data is packed as n uncompressed float[3]:
 *
 * For a rotation track of n>1 keys, the first 24 bytes are reserved for compression info
 * (eg Fixed32 stores float Mins[3]; float Ranges[3]), followed by n elements of the compressed type.
 * For a rotation track of n=1 keys, the single key is packed as an FQuatFloat96NoW.
 */
var native	array<byte>		CompressedByteStream;

cpptext
{
	// UObject interface

	virtual void Serialize(FArchive& Ar);
	virtual void PreSave();
	virtual void PostLoad();
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	/**
	 * Used by various commandlets to purge Editor only data from the object.
	 * 
	 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
	 */
	virtual void StripData(UE3::EPlatformType TargetPlatform);

	// AnimSequence interface

	/**
	 * Reconstructs a bone atom from key-reduced tracks.
	 */
	static void ReconstructBoneAtom(FBoneAtom& OutAtom,
									const FTranslationTrack& TranslationTrack,
									const FRotationTrack& RotationTrack,
									FLOAT SequenceLength,
									FLOAT Time,
									UBOOL bLooping);

	/**
	 * Reconstructs a bone atom from compressed tracks.
	 */
	static void ReconstructBoneAtom(FBoneAtom& OutAtom,
									const FCompressedTrack& TranslationTrack,
									const FCompressedTrack& RotationTrack,
									AnimationCompressionFormat TranslationCompressionFormat,
									AnimationCompressionFormat RotationCompressionFormat,
									FLOAT SequenceLength,
									FLOAT Time,
									UBOOL bLooping);

	/**
	 * Reconstructs a bone atom from compressed tracks.
	 */
	static void ReconstructBoneAtom(FBoneAtom& OutAtom,
									const BYTE* TransStream,
									INT NumTransKeys,
									const BYTE* RotStream,
									INT NumRotKeys,
									AnimationCompressionFormat TranslationCompressionFormat,
									AnimationCompressionFormat RotationCompressionFormat,
									FLOAT SequenceLength,
									FLOAT Time,
									UBOOL bLooping);

	/**
	 * Decompresses a translation key from the specified compressed translation track.
	 */
	static void ReconstructTranslation(class FVector& Out, const BYTE* Stream, INT KeyIndex, AnimationCompressionFormat TranslationCompressionFormat);

	/**
	 * Decompresses a rotation key from the specified compressed rotation track.
	 */
	static void ReconstructRotation(class FQuat& Out, const BYTE* Stream, INT KeyIndex, AnimationCompressionFormat RotationCompressionFormat, const FLOAT *Mins, const FLOAT *Ranges);

	/**
	 * Decompresses a translation key from the specified compressed translation track.
	 */
	static void ReconstructTranslation(class FVector& Out, const BYTE* Stream, INT KeyIndex);

	/**
	 * Decompresses a rotation key from the specified compressed rotation track.
	 */
	static void ReconstructRotation(class FQuat& Out, const BYTE* Stream, INT KeyIndex, UBOOL bTrackHasCompressionInfo, AnimationCompressionFormat RotationCompressionFormat);

	/**
	 * Populates the key reduced arrays from raw animation data.
	 */
	static void SeparateRawDataToTracks(const TArray<FRawAnimSequenceTrack>& RawAnimData,
										FLOAT SequenceLength,
										TArray<FTranslationTrack>& OutTranslationData,
										TArray<FRotationTrack>& OutRotationData);

	/**
	 * Compressed translation data will be byte swapped in chunks of this size.
	 */
	static INT GetCompressedTranslationStride(AnimationCompressionFormat TranslationCompressionFormat);

	/**
	 * Compressed rotation data will be byte swapped in chunks of this size.
	 */
	static INT GetCompressedRotationStride(AnimationCompressionFormat RotationCompressionFormat);

	/**
	 * Compressed translation data will be byte swapped in chunks of this size.
	 */
	INT GetCompressedTranslationStride() const;

	/**
	 * Compressed rotation data will be byte swapped in chunks of this size.
	 */
	INT GetCompressedRotationStride() const;

	/**
	 * Interpolate keyframes in this sequence to find the bone transform (relative to parent).
	 * 
	 * @param	OutAtom			[out] Output bone transform.
	 * @param	TrackIndex		Index of track to interpolate.
	 * @param	Time			Time on track to interpolate to.
	 * @param	bLooping		TRUE if the animation is looping.
	 * @param	bUseRawData		If TRUE, use raw animation data instead of compressed data.
	 */
	void GetBoneAtom(FBoneAtom& OutAtom, INT TrackIndex, FLOAT Time, UBOOL bLooping, UBOOL bUseRawData) const;

	/** Sort the Notifies array by time, earliest first. */
	void SortNotifies();

	/**
	 * @return		A reference to the AnimSet this sequence belongs to.
	 */
	UAnimSet* GetAnimSet() const;

	/**
	 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
	 *
	 * @return size of resource as to be displayed to artists/ LDs in the Editor.
	 */
	virtual INT GetResourceSize();

	/**
	 * @return		The approximate size of raw animation data.
	 */
	INT GetApproxRawSize() const;

	/**
	 * @return		The approximate size of key-reduced animation data.
	 */
	INT GetApproxReducedSize() const;

	/**
	 * @return		The approximate size of compressed animation data.
	 */
	INT GetApproxCompressedSize() const;
	
	/**
	 * Crops the raw anim data either from Start to CurrentTime or CurrentTime to End depending on 
	 * value of bFromStart
	 *
	 * @param	CurrentTime		marker for cropping (either beginning or end)
	 * @param	bFromStart		whether marker is begin or end marker
	 */
	void CropRawAnimData( FLOAT CurrentTime, UBOOL bFromStart );
}

defaultproperties
{
	RateScale=1.0
}
