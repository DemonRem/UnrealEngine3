/* *****************************************************************
 * FonixTtsDtSimple.h
 ******************************************************************* */

//Sample rate selection
#define WAVE_FORMAT_1M16  0x0001	// 11Khz Output (71 samles per buffer)
#define WAVE_FORMAT_08M16 0x0002	// 8Khz Output (51 samples per buffer)

// for Symbian only
#define OUTPUT_BUFFER	  0x0000
#define OUTPUT_STREAMING  0x0004
#define OUTPUT_QUEUE	  0x0008	

typedef struct FTTSDTUDATA_TAG
{
	volatile int gDECtalkInit;
	volatile int gDECtalkFlags;
	volatile int gDECtalkUserCallback;
	volatile int gDECtalkPktSize[3];
	volatile int gDECtalkOverflow;
	volatile int gDECtalkBufPos;
	volatile int gDECtalkBufSize;
	volatile short gDECtalkLastIndex[2];
	volatile short gDECtalkDmyBuffer[71];
	volatile short * gDECtalkBufPtr;
	volatile void * gDECtalkUserDict;
	volatile void * gUserData;
	volatile int gDECtalkLanguageID;
	volatile int gDECtalkVoiceID;
	volatile int gDECtalkStatus;
} FTTSDTUDATA_T;

// Voice definitions
typedef enum
{
	Paul,
	Harry,
	Frank,
	Dennis,
	Kit,
	Betty,
	Ursula,
	Rita,
	Wendy
} FnxDECtalkVoiceId;	// these voices are order the same as the order in the TTS Voice control command

// Language definitions
typedef enum
{
	None = -1,
	US_English,
	French,
	German,
	Castilian_Spanish,
	Reserved,
	UK_English,
	Latin_American_Spanish,
	Italian
} FnxDECtalkLanguageId;

#if defined OS_SYMBIAN || defined OS_PALM
#define DT_EXPORT(a,b)	__declspec(dllexport) a b
#else
#define DT_EXPORT(a,b)	a b
#endif

#if defined(__cplusplus)
extern "C" {
#endif
/*******************************************************************************
int FnxTTSDtSimpleOpen(short *(*callback)(short *,long),void * user_dict )
intilizes the TTS
Accepts a 'callback' function and a user dictionary as input
  The callback function is called by the engine whenever there is output data
  (either audio data or index marks) for the user to handle.
  If a NULL is passed in for this callback function, the API uses an internal
  callback function.
Returns the completion/error status flag
*******************************************************************************/
#if !defined OS_SYMBIAN && !defined OS_BREW
DT_EXPORT(int, FnxTTSDtSimpleOpen(short *(*callback)(short *,long),void *user_dict ));
#else
DT_EXPORT(int, FnxTTSDtSimpleOpen(short *(*callback)(short *,long,void*),void *user_dict, void *user_data));
#endif

/*******************************************************************************
void FnxTTSDtSimpleClose()
closes the TTS core
*******************************************************************************/
DT_EXPORT(void, FnxTTSDtSimpleClose(void));

/*******************************************************************************
void FnxTTSDtSimpleStart(char *String,short *buffer,int buflength,int flags )
creates raw audio output and stores it in the supplied buffer for the input string
Accepts an input string, buffer, length, and format flag as input
Returns the number of samples or a negative value for error status
If the synthesized wave is larger than the buffer, only that portion
which will fit in the buffer is returned and the buffer length required is
returned as a negative number of samples.

*******************************************************************************/
DT_EXPORT(int, FnxTTSDtSimpleStart(char *String, short *buffer,int buflength,int flags ));

/*******************************************************************************
void FnxTTSDtSimpleChangeVoice(FnxDECtalkVoiceId NewVoice, int flags)
creates audio output and plays for the input string
*******************************************************************************/
DT_EXPORT(void, FnxTTSDtSimpleChangeVoice( FnxDECtalkVoiceId NewVoice ,int flags ));

/*******************************************************************************
int FnxTTSDtSimpleHaltSpeech();
Stops current TTS string
Returns the completion/error status flag
*******************************************************************************/
#ifndef OS_SYMBIAN
DT_EXPORT(int, FnxTTSDtSimpleHaltSpeech());
#else
/*******************************************************************************
int FnxTTSDtSimpleHaltSpeech(void* threadData);
Stops current TTS string
Pass in NULL if calling from the same thread as FnxTTSDtSimpleStart
or call FnxTTSDtSimpleGetThreadData to get the parameter to pass
in for calling from a different thread.
Returns the completion/error status flag
*******************************************************************************/
DT_EXPORT(int, FnxTTSDtSimpleHaltSpeech(const void* threadData));
#endif

/*******************************************************************************
void FnxTTSResetSpeech()
Resets the TTS Engine to the Default Parameters
Returns the completion/error status flag
*******************************************************************************/
DT_EXPORT(int, FnxTTSDtSimpleResetSpeech());

/*******************************************************************************
int FnxTTSDtSimpleSetLanguage( int LanguageID, void *user_dict )
initializes the TTS
Accepts a user dictionary as input
Returns the completion/error status flag - if fails language is not changed
*******************************************************************************/
DT_EXPORT(int, FnxTTSDtSimpleSetLanguage( int LanguageID, void *user_dict ));


#ifdef OS_SYMBIAN
/*******************************************************************************
void FnxTTSDtCloseSTDLIB()
Closes the STDLIB for the dll.
Must be called after FnxTTSDtSimpleClose to avoid memory leaks
*******************************************************************************/
DT_EXPORT(void, FnxTtsDtCloseSTDLIB( ));

/*******************************************************************************
int FnxTTSDtSimpleSetDictionary(char *pPath)
Sets the path to the dictionary.
Ex. "c:\\system\\apps\\MyApp\\dtalk_us.dic"
Returns a negative if the parameter is NULL or if memory 
cannot be allocated.
*******************************************************************************/
DT_EXPORT(int, FnxTTSDtSimpleSetDictionary(char *pPath));

/*******************************************************************************
int FnxTTSDtSimpleGetSpeechData(short* buffer, int buflength)
Fills a buffer with speech data.  This will fill the buffer up to the
buflength if it is divisible by 51 for an 8k sample rate or 71 for 11k sample rate;
Returns the number of samples copied into the buffer and 0 when there is no
more data to copy.
*******************************************************************************/
DT_EXPORT(int, FnxTTSDtSimpleGetSpeechData(short* buffer, int buflength));


/*******************************************************************************
void* FnxTTSDtSimpleGetThreadData()
Returns a void* that can be used to pass in to FnxTTSDtSimpleHaltSpeech if
calling it from another thread.
*******************************************************************************/
DT_EXPORT(const void*, FnxTTSDtSimpleGetThreadData());
#endif

#if defined(__cplusplus)
}
#endif




