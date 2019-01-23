/**
 * Copyright 2004-2007 Epic Games, Inc. All Rights Reserved.
 */
class SpeechRecognition extends Object
	native
	collapsecategories
	hidecategories(Object);

struct native RecognisableWord
{
	var()		int				Id;
	var()		string			Word;
};

struct native RecogVocabulary
{
	/** Arrays of words that can be recognised - note that words need an ID unique among the contents of all three arrays */
	var()		array<RecognisableWord>	WhoDictionary;
	var()		array<RecognisableWord>	WhatDictionary;
	var()		array<RecognisableWord>	WhereDictionary;
	
	/** Name of vocab file */
	var			string					VocabName;
	
	/** Cached processed vocabulary data */
	var			array<byte>				VocabData;
	
	/** Working copy of vocab data */
	var			array<byte>				WorkingVocabData;
	
	structcpptext
	{
		/**
		 * Creates the work data required for speech recognition
		 */
		UBOOL CreateSpeechRecognitionData( class USpeechRecognition* Owner, FString Folder, INT Index );

		/** 
		 * Loads the created vocabulary after it has been modified by BuildVoice
		 */
		UBOOL LoadSpeechRecognitionData( void );

		/** 
		 * Clear out all the created vocab data
		 */
		void Clear( void );

		/** 
		 * Returns name of created vocab file
		 */
		FString GetVocabName( void );
	
		/** 
		 * Returns address of converted vocab data
		 */
		void* GetVocabData( void );
		
		/** 
		 * Return the number of items in this vocabulary
		 */
		INT GetNumItems( void );
	
		/** 
		 * Return the number of bytes allocated by this resource
		 */
		INT GetResourceSize( void );
	
		/**
		 * Write dictionary to a text file
		 */
		void OutputDictionary( TArrayNoInit<struct FRecognisableWord>& Dictionary, FString& Line );
		UBOOL SaveDictionary( FString& TextFile );
		
		/** 
		 * Looks up the word in the dictionary
		 */		
		FString GetStringFromWordId( DWORD WordId );
		
		/** 
		 * Initialise the recogniser
		 */
		UBOOL InitSpeechRecognition( class USpeechRecognition* Owner );
	}
};

struct native RecogUserData
{
	/** Bitfield of active vocabularies */
	var			int					ActiveVocabularies;
	/** Workspace for recognition data */
	var			array<byte>			UserData;
};

/** Language to recognise data in */
var()		string					Language<ToolTip=Use 3 letter code eg. INT, FRA, etc.>;
/** Threshhold below which the recognised word will be ignored */
var()		float					ConfidenceThreshhold<ToolTip=Values between 1 and 100.>;

/** Array of vocabularies that can be swapped in and out */
var()		array<RecogVocabulary>	Vocabularies;

/** Cached neural net data */
var			array<byte>				VoiceData;
/** Working copy of neural net data */
var			array<byte>				WorkingVoiceData;
/** Cached user data */
var			array<byte>				UserData;

/** Cached user data - max users */
var			array<RecogUserData>	InstanceData[4];

/** Whether this object has been altered */
var duplicatetransient transient	bool	bDirty;
/** Whether the object was successfully initialised or not */
var duplicatetransient transient	bool	bInitialised;

/** Cached pointers to Fonix data */
var	duplicatetransient native const pointer	FnxVoiceData;

cpptext
{
	/**
	 * Standard Serialize function
	 */
	virtual void Serialize( FArchive& Ar );

	/**
	 * Initialise the recogniser
	 */
	UBOOL InitSpeechRecognition( INT MaxLocalTalkers );

	/**
	 * Creates the work data required for speech recognition
	 */
	UBOOL CreateSpeechRecognitionData( void );

	/**
	 * Process input samples
	 */
	DWORD RecogniseSpeech( DWORD UserIndex, SWORD* Samples, INT NumSamples );

	/** 
	 * Select vocabularies
	 */
	UBOOL SelectVocabularies( DWORD LocalTalker, DWORD VocabBitField );

	/**
	 * Returns an array of recognised words based on the grammer rules $who $what [$where]
	 */
	UBOOL GetResult( DWORD UserIndex, TArray<struct FSpeechRecognizedWord>& Words );

	/**
	 * Looks up the word in the dictionary
	 */
	FString GetStringFromWordId( DWORD WordID );

	/**
	 * Returns a one line description of an object for viewing in the thumbnail view of the generic browser
	 */
	virtual FString GetDesc( void );

	/**
	 * Returns detailed info to populate listview columns
	 */
	virtual FString GetDetailedDescription( INT InIndex );

	/**
	 * @return		The size of the asset.
	 */
	virtual INT GetResourceSize( void );

	/**
	 * Callback after any property has changed
	 */
	virtual void PostEditChange( UProperty* PropertyThatChanged );

	/**
	 * Called before the package is saved
	 */
	virtual void PreSave( void );

	/**
	 * Manages any error codes that may have occurred
	 */
	FString HandleError( INT Error );

	/**
	 * Convert the language code to Fonix folder
	 */
	FString GetLanguageFolder( void );
}

defaultproperties
{
	Language="INT"
	ConfidenceThreshhold=50
}
