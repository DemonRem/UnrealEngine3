/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SoundNodeWave extends SoundNode
	PerObjectConfig
	native( SoundNode )
	hidecategories( Object )
	editinlinenew;

enum ETTSSpeaker
{
	TTSSPEAKER_Paul,
	TTSSPEAKER_Harry,
	TTSSPEAKER_Frank,
	TTSSPEAKER_Dennis,
	TTSSPEAKER_Kit,
	TTSSPEAKER_Betty,
	TTSSPEAKER_Ursula,
	TTSSPEAKER_Rita,
	TTSSPEAKER_Wendy,
};

/** Platform agnostic compression quality. 1..100 with 1 being best compression and 100 being best quality */
var(Compression)		int								CompressionQuality<Tooltip=1 smallest size, 100 is best quality>;
/** If set, forces wave data to be decompressed during playback instead of upfront on platforms that have a choice. */
var(Compression)		bool							bForceRealtimeDecompression;

/** Whether to free the resource data after it has been uploaded to the hardware */
var		transient const bool							bDynamicResource;
/** Whether to free this resource after it has played - designed for TTS of log */
var		transient const bool							bOneTimeUse;

/** Set to true to speak SpokenText using TTS */
var(TTS)	bool										UseTTS;
/** Speaker to use for TTS */
var(TTS)	ETTSSpeaker									TTSSpeaker;
/** A localized version of the text that is actually spoken in the audio. */
var(TTS)	localized string							SpokenText<ToolTip=The phonetic version of the dialog>;

/** Playback volume of sound 0 to 1 */
var(Info)	editconst const	float						Volume;
/** Playback pitch for sound 0.5 to 2.0 */
var(Info)	editconst const	float						Pitch;
/** Duration of sound in seconds. */
var(Info)	editconst const	float						Duration;
/** Number of channels of multichannel data; 1 or 2 for regular mono and stereo files */
var(Info)	editconst const	int							NumChannels;
/** Cached sample rate for displaying in the tools */
var(Info)	editconst const int							SampleRate;

/** Cached sample data size for tracking stats */
var			   const int								SampleDataSize;
/** Offsets into the bulk data for the source wav data */
var			   const	array<int>						ChannelOffsets;
/** Sizes of the bulk data for the source wav data */
var			   const	array<int>						ChannelSizes;
/** Uncompressed wav data 16 bit in mono or stereo - stereo not allowed for multichannel data */
var		native const	UntypedBulkData_Mirror			RawData{FByteBulkData};
/** Pointer to 16 bit PCM data - used to preview sounds */
var		native const	pointer							RawPCMData{SWORD};

/** Cached ogg vorbis data. */
var		native const	UntypedBulkData_Mirror			CompressedPCData{FByteBulkData};
/** Cached cooked Xbox 360 data to speed up iteration times. */
var		native const	UntypedBulkData_Mirror			CompressedXbox360Data{FByteBulkData};
/** Cached cooked PS3 data to speed up iteration times. */
var		native const	UntypedBulkData_Mirror			CompressedPS3Data{FByteBulkData};

/** Resource index to cross reference with buffers */
var		transient const int								ResourceID;
/** Size of resource copied from the bulk data */
var		transient const int								ResourceSize;
/** Memory containing the data copied from the compressed bulk data */
var		native const pointer							ResourceData{const BYTE};

/**
 * A line of subtitle text and the time at which it should be displayed.
 */
struct native SubtitleCue
{
	/** The text too appear in the subtitle. */
	var() localized string	Text;

	/** The time at which the subtitle is to be displayed, in seconds relative to the beginning of the line. */
	var() localized float	Time;
};

/**
 * Subtitle cues.  If empty, use SpokenText as the subtitle.  Will often be empty,
 * as the contents of the subtitle is commonly identical to what is spoken.
 */
var(Subtitles) localized array<SubtitleCue>				Subtitles;

/** Provides contextual information for the sound to the translator. */
var(Subtitles) localized string							Comment<ToolTip=Contextual information for the sound to the translator>;

var(Subtitles)			 bool							bAlwaysLocalise<ToolTip=Localise this sound even if there are no subtitles>;

/** TRUE if this sound is considered mature. */
var(Subtitles) localized bool							bMature<ToolTip=For marking any adult language>;

/** TRUE if the subtitles have been split manually. */
var(Subtitles) localized bool							bManualWordWrap<ToolTip=Disable automatic generation of line breaks>;

defaultproperties
{
	Volume=0.75
	Pitch=1.0
	CompressionQuality=40
}

cpptext
{
	/** UObject interface. */
	virtual void Serialize( FArchive& Ar );

	/** 
	 * Frees up all the resources allocated in this class
	 */
	void FreeResources( void );

	/**
	 * Frees the sound resource data.
	 */
	virtual void FinishDestroy( void );

	/**
	 * Outside the Editor, uploads resource to audio device and performs general PostLoad work.
	 *
	 * This function is being called after all objects referenced by this object have been serialized.
	 */
	virtual void PostLoad( void );

	/** 
	 * Invalidate compressed data
	 */
	virtual void PostEditChange( UProperty* PropertyThatChanged );

	/** 
	 * Copy the compressed audio data from the bulk data
	 */
	void InitAudioResource( FByteBulkData& CompressedData );

	/** 
	 * Remove the compressed audio data associated with the passed in wave
	 */
	void RemoveAudioResource( void );

	/** 
	 * USoundNode interface.
	 */
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
	
	/** 
	 * 
	 */
	virtual INT GetMaxChildNodes( void ) 
	{ 
		return( 0 );  
	}
	
	/** 
	 * Gets the time in seconds of the associated sound data
	 */	
	virtual FLOAT GetDuration( void );

	/** 
	 * Get the name of the class used to help out when handling events in UnrealEd.
	 * @return	String name of the helper class.
	 */
	virtual const FString GetEdHelperClassName( void ) const
	{
		return FString( TEXT("UnrealEd.SoundNodeWaveHelper") );
	}

	/**
	 * Returns whether this wave file is a localized resource.
	 *
	 * @return TRUE if it is a localized resource, FALSE otherwise.
	 */
	virtual UBOOL IsLocalizedResource( void );

	/**
	 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
	 *
	 * @return size of resource as to be displayed to artists/ LDs in the Editor.
	 */
	virtual INT GetResourceSize( void );

	/** 
	 * Returns the name of the exporter factory used to export this object
	 * Used when multiple factories have the same extension
	 */
	virtual FName GetExporterName( void );

	/** 
	 * Returns a one line description of an object for viewing in the thumbnail view of the generic browser
	 */
	virtual FString GetDesc( void );

	/** 
	 * Returns detailed info to populate listview columns
	 */
	virtual FString GetDetailedDescription( INT InIndex );

	/**
	 * Used by various commandlets to purge Editor only data from the object.
	 * 
	 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
	 */
	virtual void StripData(UE3::EPlatformType TargetPlatform);
	
	/** 
	 * Makes sure ogg vorbis data is available for this sound node by converting on demand
	 */
	UBOOL ValidateData( void );	
}