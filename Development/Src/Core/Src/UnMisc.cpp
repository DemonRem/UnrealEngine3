/*=============================================================================
	UnMisc.cpp: Various core platform-independent functions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Core includes.
#include "CorePrivate.h"

/** For FConfigFile in appInit							*/
#include "FConfigCacheIni.h"
#include "Localization.h"

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

/**
 * PERF_ISSUE_FINDER
 *
 * Once a level is loaded you should not have LazyLoaded Array data being loaded. 
 *
 * Turn his on to have the engine log out when an LazyLoaded array's data is being loaded
 * Right now this will log even when you should be loading LazyArrays.  So you will get a bunch of
 * false positive log messages on Level load.  Once that has occured any logs will be true perf issues.
 * 
 */
//#define LOG_LAZY_ARRAY_LOADS 1

/*-----------------------------------------------------------------------------
	UObjectSerializer.
-----------------------------------------------------------------------------*/

/**
 * Callback used to allow object register its direct object references that are not already covered by
 * the token stream.
 *
 * @param ObjectArray	array to add referenced objects to via AddReferencedObject
 */
void UObjectSerializer::AddReferencedObjects( TArray<UObject*>& ObjectArray )
{
	AddReferencedObjectsViaSerialization( ObjectArray );
}

/**
 * Adds an object to the serialize list
 *
 * @param Object The object to add to the list
 */
void UObjectSerializer::AddObject(FSerializableObject* Object)
{
	check(Object);
	// Make sure there are no duplicates. Should be impossible...
	SerializableObjects.AddUniqueItem(Object);
}

/**
 * Removes a window from the list so it won't receive serialization events
 *
 * @param Object The object to remove from the list
 */
void UObjectSerializer::RemoveObject(FSerializableObject* Object)
{
	check(Object);
	SerializableObjects.RemoveItem(Object);
}

/**
 * Forwards this call to all registered objects so they can serialize
 * any UObjects they depend upon
 *
 * @param Ar The archive to serialize with
 */
void UObjectSerializer::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	// Let each registered object handle its serialization
	for (INT i = 0; i < SerializableObjects.Num(); i++)
	{
		FSerializableObject* Object = SerializableObjects(i);
		check(Object);
		Object->Serialize(Ar);
	}
}

/**
 * Destroy function that gets called before the object is freed. This might
 * be as late as from the destructor.
 */
void UObjectSerializer::FinishDestroy()
{
	if ( !HasAnyFlags(RF_ClassDefaultObject) )
	{
		// Make sure FSerializableObjects that are around after exit purge don't
		// reference this object.
		check( FSerializableObject::GObjectSerializer == this );
		FSerializableObject::GObjectSerializer = NULL;
	}

	Super::FinishDestroy();
}

IMPLEMENT_CLASS(UObjectSerializer);

/** Static used for serializing non- UObject objects */
UObjectSerializer* FSerializableObject::GObjectSerializer = NULL;


/*-----------------------------------------------------------------------------
	FOutputDevice implementation.
-----------------------------------------------------------------------------*/

void FOutputDevice::Log( EName Event, const TCHAR* Str )
{
	if( !FName::SafeSuppressed(Event) )
	{
		Serialize( Str, Event );
	}
}
void FOutputDevice::Log( const TCHAR* Str )
{
	if( !FName::SafeSuppressed(NAME_Log) )
	{	
		Serialize( Str, NAME_Log );
	}
}
void FOutputDevice::Log( const FString& S )
{
	if( !FName::SafeSuppressed(NAME_Log) )
	{	
		Serialize( *S, NAME_Log );
	}
}
void FOutputDevice::Log( enum EName Type, const FString& S )
{
	if( !FName::SafeSuppressed(Type) )
	{
		Serialize( *S, Type );
	}
}
VARARG_BODY( void, FOutputDevice::Logf, const TCHAR*, VARARG_EXTRA(enum EName Event) )
{
	// We need to use malloc here directly as GMalloc might not be safe.
	if( !FName::SafeSuppressed(Event) )
	{
		INT		BufferSize	= 1024;
		TCHAR*	Buffer		= NULL;
		INT		Result		= -1;

		while(Result == -1)
		{
			Buffer = (TCHAR*) appSystemRealloc( Buffer, BufferSize * sizeof(TCHAR) );
			GET_VARARGS_RESULT( Buffer, BufferSize, BufferSize-1, Fmt, Fmt, Result );
			BufferSize *= 2;
		};
		Buffer[Result] = 0;
		Serialize( Buffer, Event );

		appSystemFree( Buffer );
	}
}
VARARG_BODY( void, FOutputDevice::Logf, const TCHAR*, VARARG_NONE )
{
	// We need to use malloc here directly as GMalloc might not be safe.	
	if( !FName::SafeSuppressed(NAME_Log) )
	{
		INT		BufferSize	= 1024;
		TCHAR*	Buffer		= NULL;
		INT		Result		= -1;

		while(Result == -1)
		{
			Buffer = (TCHAR*) appSystemRealloc( Buffer, BufferSize * sizeof(TCHAR) );
			GET_VARARGS_RESULT( Buffer, BufferSize, BufferSize-1, Fmt, Fmt, Result );
			BufferSize *= 2;
		};
		Buffer[Result] = 0;
		Serialize( Buffer, NAME_Log );

		appSystemFree( Buffer );
	}
}


/*-----------------------------------------------------------------------------
	FArray implementation.
-----------------------------------------------------------------------------*/

#define TRACK_ARRAY_SLACK 0
#define START_WITH_SLACK_TRACKING_ENABLED 0

#if TRACK_ARRAY_SLACK

/**
 * Slack tracker. Used to identify callstacks of excessive re-allocations due to lack of array presizing.
 */
struct FSlackTracker
{
private:
	/** Maximum number of backtrace depth. */
	static const INT MAX_BACKTRACE_DEPTH = 50;

	/** Helper structure to capture callstack addresses and slack count. */
	struct FCallStack
	{
		/** Slack count, aka the number of calls to CalculateSlack */
		QWORD SlackCount;
		/** Program counter addresses for callstack. */
		DWORD64	Addresses[MAX_BACKTRACE_DEPTH];
	};

	/** Compare function, sorting callstack by slack count in descending order. */
	IMPLEMENT_COMPARE_CONSTREF( FCallStack, UnMisc, { return B.SlackCount - A.SlackCount; } );

	/** Array of unique callstacks. */
	TArray<FCallStack> CallStacks;
	/** Mapping from callstack CRC to index in callstack array. */
	TMap<DWORD,INT> CRCToCallStackIndexMap;
	/** Whether we are currently capturing or not, used to avoid re-entrancy. */
	UBOOL bAvoidCapturing;
	/** Whether slack tracking is enabled. */
	UBOOL bIsEnabled;
	/** Frame counter at the time tracking was enabled. */
	QWORD StartFrameCounter;
	/** Frame counter at the time tracking was disabled. */
	QWORD StopFrameCounter;

public:
	/** Constructor, initializing all member variables */
	FSlackTracker()
	:	bAvoidCapturing(FALSE)
	,	bIsEnabled(START_WITH_SLACK_TRACKING_ENABLED)
	,	StartFrameCounter(0)
	,	StopFrameCounter(0)
	{}

	/**
	 * Captures the current stack and updates slack tracking information.
	 */
	void CaptureSlackTrace()
	{
		// Avoid re-rentrancy as the code uses TArray/TMap.
		if( !bAvoidCapturing && bIsEnabled )
		{
			// Scoped TRUE/ FALSE.
			bAvoidCapturing = TRUE;

			// Capture callstack and create CRC.
			#define NUM_ENTRIES_TO_SKIP 4
			QWORD FullBackTrace[MAX_BACKTRACE_DEPTH + NUM_ENTRIES_TO_SKIP];
			appCaptureStackBackTrace( FullBackTrace, MAX_BACKTRACE_DEPTH + NUM_ENTRIES_TO_SKIP );
			// Skip first NUM_ENTRIES_TO_SKIP entries as they are inside this code
			QWORD* BackTrace = &FullBackTrace[NUM_ENTRIES_TO_SKIP];
			DWORD CRC = appMemCrc( BackTrace, MAX_BACKTRACE_DEPTH * sizeof(QWORD), 0 );

			// Use index if found
			INT* IndexPtr = CRCToCallStackIndexMap.Find( CRC );
			if( IndexPtr )
			{
				// Increase slack count for existing callstack.
				CallStacks(*IndexPtr).SlackCount++;
			}
			// Encountered new call stack, add to array and set index mapping.
			else
			{
				// Add to array and set mapping for future use.
				INT Index = CallStacks.Add(); 
				CRCToCallStackIndexMap.Set( CRC, Index );

				// Fill in callstack and count.
				FCallStack& CallStack = CallStacks(Index);
				appMemcpy( CallStack.Addresses, BackTrace, sizeof(QWORD) * MAX_BACKTRACE_DEPTH );
				CallStack.SlackCount = 1;
			}

			// We're done capturing.
			bAvoidCapturing = FALSE;
		}
	}

	/**
	 * Dumps capture slack trace summary to the passed in log.
	 */
	void DumpSlackTraces( INT SlackThreshold, FOutputDevice& Ar )
	{
		// Avoid distorting results while we log them.
		check( !bAvoidCapturing );
		bAvoidCapturing = TRUE;

		// Make a copy as sorting causes index mismatch with TMap otherwise.
		TArray<FCallStack> SortedCallStacks = CallStacks;
		// Sort callstacks in descending order by slack count.
		Sort<USE_COMPARE_CONSTREF(FCallStack,UnMisc)>( SortedCallStacks.GetTypedData(), SortedCallStacks.Num() );

		// Iterate over each callstack to get total slack count.
		QWORD TotalSlackCount = 0;
		for( INT CallStackIndex=0; CallStackIndex<SortedCallStacks.Num(); CallStackIndex++ )
		{
			const FCallStack& CallStack = SortedCallStacks(CallStackIndex);
			TotalSlackCount += CallStack.SlackCount;
		}

		// Calculate the number of frames we captured.
		INT FramesCaptured = 0;
		if( bIsEnabled )
		{
			FramesCaptured = GFrameCounter - StartFrameCounter;
		}
		else
		{
			FramesCaptured = StopFrameCounter - StartFrameCounter;
		}

		// Log quick summary as we don't log each individual so totals in CSV won't represent real totals.
		Ar.Logf(TEXT("Captured %i unique callstacks totalling %i reallocations over %i frames, averaging %5.2f realloc/frame"), SortedCallStacks.Num(), (int)TotalSlackCount, FramesCaptured, (FLOAT) TotalSlackCount / FramesCaptured);

		// Iterate over each callstack and write out info in human readable form in CSV format
		for( INT CallStackIndex=0; CallStackIndex<SortedCallStacks.Num(); CallStackIndex++ )
		{
			const FCallStack& CallStack = SortedCallStacks(CallStackIndex);

			// Avoid log spam by only logging above threshold.
			if( CallStack.SlackCount > SlackThreshold )
			{
				// First row is slack count.
				FString CallStackString = appItoa(CallStack.SlackCount);
				// Iterate over all addresses in the callstack to look up symbol name.
				for( INT AddressIndex=0; AddressIndex<ARRAY_COUNT(CallStack.Addresses) && CallStack.Addresses[AddressIndex]; AddressIndex++ )
				{
					ANSICHAR AddressInformation[512];
					AddressInformation[0] = 0;
					appProgramCounterToHumanReadableString( CallStack.Addresses[AddressIndex], AddressInformation, ARRAY_COUNT(AddressInformation)-1, VF_DISPLAY_FILENAME );
					CallStackString = CallStackString + TEXT(",") + FString(AddressInformation);
				}

				// Finally log with ',' prefix so "Log:" can easily be discarded as row in Excel.
				Ar.Logf(TEXT(",%s"),*CallStackString);
			}
		}

		// Done logging.
		bAvoidCapturing = FALSE;
	}

	/** Resets slack tracking. */
	void ResetTracking()
	{
		check(!bAvoidCapturing)
		CRCToCallStackIndexMap.Empty();
		CallStacks.Empty();
	}

	/** Toggles tracking. */
	void ToggleTracking()
	{
		bIsEnabled = !bIsEnabled;
		// Enabled
		if( bIsEnabled )
		{
			debugf(TEXT("FArray slack tracking is now enabled."));
			StartFrameCounter = GFrameCounter;
		}
		// Disabled.
		else
		{
			StopFrameCounter = GFrameCounter;
			debugf(TEXT("FArray slack tracking is now disabled."));
		}
	}
};

/** Slack tracker pointer. This isn't a static variable as the destructor of other static variables might invoke it after it has been destroyed. */
static FSlackTracker* GSlackTracker = NULL;

#endif // TRACK_ARRAY_SLACK

int FArray::CalculateSlack( INT ElementSize ) const
{
#if TRACK_ARRAY_SLACK 
	if( !GSlackTracker )
	{
		GSlackTracker = new FSlackTracker();
	}
	GSlackTracker->CaptureSlackTrace();
#endif // TRACK_ARRAY_SLACK
	return ArrayNum + 3*ArrayNum/8 + 16;
}

void FArray::Remove( INT Index, INT Count, INT ElementSize, DWORD Alignment )
{
	checkSlow(Count >= 0);
	checkSlow(Index >= 0); 
	checkSlow(Index <= ArrayNum);
	checkSlow(Index + Count <= ArrayNum);

	appMemmove
	(
		(BYTE*)Data + (Index      ) * ElementSize,
		(BYTE*)Data + (Index+Count) * ElementSize,
		(ArrayNum - Index - Count ) * ElementSize
	);
	ArrayNum -= Count;
	
	if(	((3*ArrayNum < 2*ArrayMax) || ((ArrayMax-ArrayNum)*ElementSize >= 16384)) &&
		((ArrayMax-ArrayNum > 64) || (ArrayNum == 0)) )
	{
		ArrayMax = ArrayNum;
		Realloc(ElementSize, Alignment);
	}
	checkSlow(ArrayNum >= 0);
	checkSlow(ArrayMax >= ArrayNum);
}


/*-----------------------------------------------------------------------------
	FString implementation.
-----------------------------------------------------------------------------*/

FString FString::Chr( TCHAR Ch )
{
	TCHAR Temp[2]={Ch,0};
	return FString(Temp);
}

FString FString::LeftPad( INT ChCount ) const
{
	INT Pad = ChCount - Len();
	if( Pad > 0 )
	{
		TCHAR* Ch = (TCHAR*)appAlloca((Pad+1)*sizeof(TCHAR));
		INT i;
		for( i=0; i<Pad; i++ )
			Ch[i] = ' ';
		Ch[i] = 0;
		return FString(Ch) + *this;
	}
	else return *this;
}
FString FString::RightPad( INT ChCount ) const
{
	INT Pad = ChCount - Len();
	if( Pad > 0 )
	{
		TCHAR* Ch = (TCHAR*)appAlloca((Pad+1)*sizeof(TCHAR));
		INT i;
		for( i=0; i<Pad; i++ )
			Ch[i] = ' ';
		Ch[i] = 0;
		return *this + FString(Ch);
	}
	else return *this;
}

UBOOL FString::IsNumeric() const
{
	if ( Len() == 0 )
		return 0;

	TCHAR C = (*this)(0);
	
	if( C == '-' || C =='.' || appIsDigit( C ) )
	{
		UBOOL HasDot = (C == '.');

		for( INT i=1; i<Len(); i++ )
		{
			C = (*this)(i);

			if( C == '.' )
			{
				if( HasDot )
				{
					return 0;
				}
				else
				{
					HasDot = 1;
				}
			}
			else if( !appIsDigit(C) )
			{
				return 0;
			}
		}

		return 1;
	}
	else
	{
		return 0;
	}
}

/**
 * Breaks up a delimited string into elements of a string array.
 *
 * @param	InArray		The array to fill with the string pieces
 * @param	pchDelim	The string to delimit on
 * @param	InCullEmpty	If 1, empty strings are not added to the array
 *
 * @return	The number of elements in InArray
 */
INT FString::ParseIntoArray( TArray<FString>* InArray, const TCHAR* pchDelim, UBOOL InCullEmpty ) const
{
	check(InArray);
	InArray->Empty();

	FString S = *this;
	INT i = S.InStr( pchDelim );
	INT DelimLength = appStrlen(pchDelim);

	while( i >= 0 )
	{
		new(*InArray)FString( S.Left(i) );
		S = S.Mid( i + DelimLength, S.Len() );
		i = S.InStr( pchDelim );
	}

	new(*InArray)FString( S );

	if( InCullEmpty )
	{
		FString::CullArray( InArray );
	}

	return InArray->Num();
}

/**
 * Takes a string, and skips over all instances of white space and returns the new string
 *
 * @param	WhiteSpace		An array of white space strings
 * @param	NumWhiteSpaces	The length of the WhiteSpace array
 * @param	S				The input and output string
 */
static void SkipOver(const TCHAR** WhiteSpace, INT NumWhiteSpaces, FString& S)
{
	UBOOL bStop = false;

	// keep going until we hit non-white space
	while (!bStop)
	{
		// we stop it we don't find any white space
		bStop = true;
		// loop over all possible white spaces to search for
		for (INT iWS = 0; iWS < NumWhiteSpaces; iWS++)
		{
			// get the length (tiny optimization)
			INT WSLen = appStrlen(WhiteSpace[iWS]);

			// if we start with this bit of whitespace, chop it off, and keep looking for more whitespace
			if (appStrnicmp(*S, WhiteSpace[iWS], WSLen) == 0)
			{
				// chop it off
				S = S.Mid(WSLen);
				// keep looking!
				bStop = false;
				break;
			}
		}
	}
}

/**
 * Splits the input string on the first bit of white space, and returns the initial token
 * (to the left of the white space), and the rest (white space and to the right)
 *
 * @param	WhiteSpace		An array of white space strings
 * @param	NumWhiteSpaces	The length of the WhiteSpace array
 * @param	Token			The first token before any white space
 * @param	S				The input and outputted remainder string
 *
 * @return	Was there a valid token before the end of the string?
 */
static UBOOL SplitOn( const TCHAR** WhiteSpace, INT NumWhiteSpaces, FString& Token, FString& S, TCHAR& InCh )
{
	// this is the index of the first instance of whitespace
	INT SmallestToken = MAXINT;
	InCh = TEXT(' ');

	// Keeps track if we are inside quotations or not (if we are, we don't stop at whitespace until we see the ending quote)
	UBOOL bInQuotes = false;

	// loop through all possible white spaces
	for (INT iWS = 0; iWS < NumWhiteSpaces; iWS++)
	{
		// look for the first instance of it
		INT NextWS = S.InStr(WhiteSpace[iWS]);

		// if shouldn't be at the start of the string, because SkipOver should have been called
		check(NextWS != 0);

		// if we found this white space, and it is before any other white spaces, remember it
		if (NextWS > 0 && NextWS < SmallestToken)
		{
			SmallestToken = NextWS;
			InCh = *WhiteSpace[iWS];
		}
	}

	// if we found some white space, SmallestToken is pointing to the the first one
	if (SmallestToken != MAXINT)
	{
		// get the token before the white space
		Token = S.Left(SmallestToken);
		// update out string with the remainder
		S = S.Mid(SmallestToken);
		// we found a token
		return true;
	}

	// we failed to find a token
	return false;
}

INT FString::ParseIntoArrayWS( TArray<FString>* InArray, const TCHAR* pchExtraDelim ) const
{
	// default array of White Spaces, the last entry can be replaced with the optional pchExtraDelim string
	// (if you want to split on white space and another character)
	static const TCHAR* WhiteSpace[] = 
	{
		TEXT(" "),
		TEXT("\t"),
		TEXT("\r"),
		TEXT("\n"),
		TEXT(""),
	};

	// start with just the standard whitespaces
	INT NumWhiteSpaces = ARRAY_COUNT(WhiteSpace) - 1;
	// if we got one passed in, use that in addition
	if (pchExtraDelim && *pchExtraDelim)
	{
		WhiteSpace[NumWhiteSpaces++] = pchExtraDelim;
	}

	check(InArray);
	InArray->Empty();

	// this is our temporary workhorse string
	FString S = *this;

	UBOOL bStop = false;
	// keep going until we run out of tokens
	while (!bStop)
	{
		// skip over any white space at the beginning of the string
		SkipOver(WhiteSpace, NumWhiteSpaces, S);
		
		// find the first token in the string, and if we get one, add it to the output array of tokens
		FString Token;
		TCHAR ch;
		if (SplitOn(WhiteSpace, NumWhiteSpaces, Token, S, ch))
		{
			if( Token[0] == TEXT('"') )
			{
				INT SaveSz = Token.Len();

				FString Wk = FString::Printf( TEXT("%s%c"), *Token, ch );
				for( INT x = 1 ; x < S.Len() ; ++x )
				{
					if( S(x) == TEXT('"') )
					{
						Wk += TEXT("\"");
						break;
					}
					else
					{
						Wk = Wk + S.Mid(x,1);
					}
				}

				Token = Wk;

				INT DiffSz = Token.Len() - SaveSz;
				S = S.Mid( DiffSz );
			}

			// stick it on the end
			new(*InArray)FString(Token);
		}
		else
		{
			// if the remaining string is not empty, then we need to add the last token
			if (S.Len())
			{
				new(*InArray)FString(S);
			}

			// and, we're done this crazy ride
			bStop = true;
		}
	}

	// simply return the number of elements in the output array
	return InArray->Num();
}

FString FString::Replace(const TCHAR* From, const TCHAR* To) const
{
	if (Len() == 0)
	{
		return *this;
	}

	FString Result;
	
	FString TempString = *this;

	// get a pointer into the character data
	TCHAR* Travel = (TCHAR*)GetData();

	// precalc the length of the From string
	INT FromLength = appStrlen(From);

	while (TRUE)
	{
		// look for From in the remaining string
		TCHAR* FromLocation = appStrstr(Travel, From);
		if (FromLocation)
		{
			// replace the first letter of the From with 0 so we can do a strcpy (FString +=)
			TCHAR C = *FromLocation;
			*FromLocation = 0;
			
			// copy everything up to the From
			Result += Travel;

			// copy over the To
			Result += To;

			// retore the letter, just so we don't have 0's in the string
			*FromLocation = *From;

			Travel = FromLocation + FromLength;
		}
		else
		{
			break;
		}
	}

	// copy anything left over
	Result += Travel;

	return Result;
}


/**
 * Returns a copy of this string with all quote marks escaped (unless the quote is already escaped)
 */
FString FString::ReplaceQuotesWithEscapedQuotes() const
{
	if ( InStr(TEXT("\"")) != INDEX_NONE )
	{
		FString Result;

		const TCHAR* pChar = **this;

		bool bEscaped = false;
		while ( *pChar != 0 )
		{
			if ( bEscaped )
			{
				bEscaped = false;
			}
			else if ( *pChar == TCHAR('\\') )
			{
				bEscaped = true;
			}
			else if ( *pChar == TCHAR('"') )
			{
				Result += TCHAR('\\');
			}

			Result += *pChar++;
		}
		
		return Result;
	}

	return *this;
}


VARARG_BODY( FString, FString::Printf, const TCHAR*, VARARG_NONE )
{
	INT		BufferSize	= 1024;
	TCHAR*	Buffer		= NULL;
	INT		Result		= -1;

	while(Result == -1)
	{
		Buffer = (TCHAR*) appRealloc( Buffer, BufferSize * sizeof(TCHAR) );
		GET_VARARGS_RESULT( Buffer, BufferSize, BufferSize-1, Fmt, Fmt, Result );
		BufferSize *= 2;
	};
	Buffer[Result] = 0;

	FString ResultString(Buffer);
	appFree( Buffer );

	return ResultString;
}

FArchive& operator<<( FArchive& Ar, FString& A )
{
	INT SaveNum;
	if( Ar.IsSaving() )
	{
		// > 0 for ANSICHAR, < 0 for UNICHAR serialization
		SaveNum	= appIsPureAnsi(*A) ? A.Num() : -A.Num();
	}
	Ar << SaveNum;
	if( Ar.IsLoading() )
	{
		A.ArrayMax = A.ArrayNum = Abs(SaveNum);
		A.Realloc( sizeof(TCHAR), DEFAULT_ALIGNMENT );

		if( SaveNum>=0 )
		{
			// String was serialized as a series of ANSICHARs.
#if	UNICODE
			ANSICHAR* AnsiString = (ANSICHAR*) appMalloc( A.Num() * sizeof(ANSICHAR) );
			Ar.Serialize( AnsiString, A.Num() * sizeof(ANSICHAR) );
			for( INT i=0; i<A.Num(); i++ )
			{
				A(i) = FromAnsi(AnsiString[i]);
			}
			appFree(AnsiString);
#else
			Ar.Serialize( A.GetData(), A.Num() * sizeof(ANSICHAR) );
#endif
		}
		else
		{
			// String was serialized as a series of UNICHARs.
#if !UNICODE
			UNICHAR* UniString = (UNICHAR*) appMalloc( A.Num() * sizeof(UNICHAR) );
			Ar.Serialize( UniString, sizeof(UNICHAR) * A.Num() );
			for( INT i=0; i<A.Num(); i++ )
			{
				A(i) = FromUnicode( UniString[i] );
			}
			appFree(UniString);
#else
			Ar.Serialize( A.GetData(), sizeof(UNICHAR) * A.Num() );
#if !__INTEL_BYTE_ORDER__
			for( INT i=0; i<A.Num(); i++ )
			{
				// Serializing as one large blob requires cleaning up for different endianness... @todo xenon cooking: move into cooking phase?
				// Engine verifies that sizeof(ANSICHAR) == 1 and sizeof(UNICHAR) == 2 on startup.
				A(i) = INTEL_ORDER16((WORD)A(i));
			}
#endif
#endif
		}

		// Throw away empty string.
		if( A.Num()==1 )
		{
			A.Empty();
		}
	}
	else
	{
		A.CountBytes( Ar );
#if !UNICODE
		Ar.Serialize( A.GetData(), A.Num() );
#else
		if( SaveNum>=0 )
		{
			// String is being serialized as a series of ANSICHARs.
			if(A.Num() > 0)
			{
				ANSICHAR* AnsiString = (ANSICHAR*) appAlloca( A.Num() );
				for( INT i=0; i<A.Num(); i++ )
				{
					AnsiString[i] = ToAnsi(A(i));
				}
				Ar.Serialize( AnsiString, sizeof(ANSICHAR) * A.Num() );
			}
		}
		else
		{
			// String is serialized as a series of UNICHARs.	
			Ar.Serialize( A.GetData(), sizeof(UNICHAR) * A.Num() );
		}
#endif
	}
	return Ar;
}

/*-----------------------------------------------------------------------------
	FFilename implementation
-----------------------------------------------------------------------------*/

// Returns the text following the last period.
FString FFilename::GetExtension() const
{
	INT Pos = InStr(TEXT("."), 1);
	if (Pos != -1)
	{
		return Mid(Pos+1, Len() - Pos - 1);
	}

	return TEXT("");
}

// Returns the filename (with extension), minus any path information.
FString FFilename::GetCleanFilename() const
{
	INT Pos = InStr(PATH_SEPARATOR, TRUE);

	// in case we are using slashes on a platform that uses backslashes
	Pos = Max(Pos, InStr(TEXT("/"), TRUE));

	// in case we are using backslashes on a platform that doesn't use backslashes
	Pos = Max(Pos, InStr(TEXT("\\"), TRUE));

	if ( Pos != INDEX_NONE )
	{
		return Mid(Pos + 1);
	}

	return *this;
}

// Returns the same thing as GetCleanFilename, but without the extension
FString FFilename::GetBaseFilename( UBOOL bRemovePath ) const
{
	FString Wk = bRemovePath ? GetCleanFilename() : FString(*this);

	// remove the extension
	INT Pos = Wk.InStr(TEXT("."), TRUE);
	if ( Pos != INDEX_NONE )
	{
		return Wk.Left(Pos);
	}

	return Wk;
}

// Returns the path in front of the filename
FString FFilename::GetPath() const
{
	INT Pos = InStr(PATH_SEPARATOR, TRUE);

	// in case we are using slashes on a platform that uses backslashes
	Pos = Max(Pos, InStr(TEXT("/"), TRUE));

	// in case we are using backslashes on a platform that doesn't use backslashes
	Pos = Max(Pos, InStr(TEXT("\\"), TRUE));
	if ( Pos != INDEX_NONE )
	{
		return Left(Pos);
	}

	return TEXT("");
}

/**
 * Returns the localized package name by appending the language suffix before the extension.
 *
 * @param	Language	Language to use.
 * @return	Localized package name
 */
FString FFilename::GetLocalizedFilename( const TCHAR* Language ) const
{
	// Use default language if none specified.
	if( !Language )
	{
		Language = UObject::GetLanguage();
	}

	// Prepend path and path separator.
	FFilename LocalizedFilename = GetPath();
	if( LocalizedFilename.Len() )
	{
		LocalizedFilename += PATH_SEPARATOR;
	}

	// Append _LANG to filename.
	LocalizedFilename += GetBaseFilename() + TEXT("_") + Language;
	
	// Append extension if used.
	if( GetExtension().Len() )
	{
		LocalizedFilename += FString(TEXT(".")) + GetExtension();
	}
	
	return LocalizedFilename;
}

/*-----------------------------------------------------------------------------
	String functions.
-----------------------------------------------------------------------------*/

//
// Returns whether the string is pure ANSI.
//
UBOOL appIsPureAnsi( const TCHAR* Str )
{
#if UNICODE
	for( ; *Str; Str++ )
	{
		if( *Str>0xff )
		{
			return 0;
		}
	}
#endif
	return 1;
}


//
// Formats the text for appOutputDebugString.
//
void VARARGS appOutputDebugStringf( const TCHAR *Format, ... )
{
	TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Format, Format );
	appOutputDebugString( TempStr );
}


/** Sends a formatted message to a remote tool. */
void VARARGS appSendNotificationStringf( const ANSICHAR *Format, ... )
{
	ANSICHAR TempStr[4096];
	GET_VARARGS_ANSI( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Format, Format );
	appSendNotificationString( TempStr );
}


//
// Failed assertion handler.
//warning: May be called at library startup time.
//
void VARARGS appFailAssertFunc( const ANSICHAR* Expr, const ANSICHAR* File, INT Line, const TCHAR* Format/*=TEXT("")*/, ... )
{
	TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Format, Format );
#if !CONSOLE
	const SIZE_T StackTraceSize = 65535;
	ANSICHAR* StackTrace = (ANSICHAR*) appSystemMalloc( StackTraceSize );
	StackTrace[0] = 0;
	// Walk the stack and dump it to the allocated memory.
	appStackWalkAndDump( StackTrace, StackTraceSize, 3 );
	GError->Logf( TEXT("Assertion failed: %s [File:%s] [Line: %i]\n%s\nStack: %s"), ANSI_TO_TCHAR(Expr), ANSI_TO_TCHAR(File), Line, TempStr, ANSI_TO_TCHAR(StackTrace) );
	appSystemFree( StackTrace );
#else
	GError->Logf( TEXT("Assertion failed: %s [File:%s] [Line: %i]\n%s\nStack: Not avail yet"), ANSI_TO_TCHAR(Expr), ANSI_TO_TCHAR(File), Line, TempStr );
#endif
}


//
// Failed assertion handler.  This version only calls appOutputDebugString.
//
void VARARGS appFailAssertFuncDebug( const ANSICHAR* Expr, const ANSICHAR* File, INT Line, const TCHAR* Format/*=TEXT("")*/, ... )
{
	TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Format, Format );
	appOutputDebugStringf( TEXT("%s(%i): Assertion failed: %s\n%s\n"), ANSI_TO_TCHAR(File), Line, ANSI_TO_TCHAR(Expr), TempStr );

#if PS3
	PS3Callstack();
#endif
}
void VARARGS appFailAssertFuncDebug( const ANSICHAR* Expr, const ANSICHAR* File, INT Line, enum EName Type, const TCHAR* Format/*=TEXT("")*/, ... )
{
	TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Format, Format );
	appOutputDebugStringf( TEXT("%s(%i): Assertion failed: %s\n%s\n"), ANSI_TO_TCHAR(File), Line, ANSI_TO_TCHAR(Expr), TempStr );

#if PS3
	PS3Callstack();
#endif
}

//
// Gets the extension of a file, such as "PCX".  Returns NULL if none.
// string if there's no extension.
//
const TCHAR* appFExt( const TCHAR* fname )
{
	if( appStrstr(fname,TEXT(":")) )
	{
		fname = appStrstr(fname,TEXT(":"))+1;
	}

	while( appStrstr(fname,TEXT("/")) )
	{
		fname = appStrstr(fname,TEXT("/"))+1;
	}

	while( appStrstr(fname,TEXT(".")) )
	{
		fname = appStrstr(fname,TEXT("."))+1;
	}

	return fname;
}

//
// Convert an integer to a string.
//
FString appItoa( INT InNum )
{
	SQWORD	Num					= InNum; // This avoids having to deal with negating -MAXINT-1
	FString NumberString;
	TCHAR*	NumberChar[10]		= { TEXT("0"), TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9") };
	UBOOL	bIsNumberNegative	= FALSE;

	// Correctly handle negative numbers and convert to positive integer.
	if( Num < 0 )
	{
		bIsNumberNegative = TRUE;
		Num = -Num;
	}

	// Convert to string assuming base ten and a positive integer.
	do 
	{
		NumberString += NumberChar[Num % 10];
		Num /= 10;
	} while( Num );

	// Append sign as we're going to reverse string afterwards.
	if( bIsNumberNegative )
	{
		NumberString += TEXT("-");
	}

	return NumberString.Reverse();
}

//
// Find string in string, case insensitive, requires non-alphanumeric lead-in.
//
const TCHAR* appStrfind( const TCHAR* Str, const TCHAR* Find )
{
	if( Find == NULL || Str == NULL )
	{
		return NULL;
	}
	UBOOL Alnum  = 0;
	TCHAR f      = (*Find<'a' || *Find>'z') ? (*Find) : (*Find+'A'-'a');
	INT   Length = appStrlen(Find++)-1;
	TCHAR c      = *Str++;
	while( c )
	{
		if( c>='a' && c<='z' )
		{
			c += 'A'-'a';
		}
		if( !Alnum && c==f && !appStrnicmp(Str,Find,Length) )
		{
			return Str-1;
		}
		Alnum = (c>='A' && c<='Z') || (c>='0' && c<='9');
		c = *Str++;
	}
	return NULL;
}

/** 
 * Finds string in string, case insensitive 
 * @param Str The string to look through
 * @param Find The string to find inside Str
 * @return Position in Str if Find was found, otherwise, NULL
 */
const TCHAR* appStristr(const TCHAR* Str, const TCHAR* Find)
{
	// both strings must be valid
	if( Find == NULL || Str == NULL )
	{
		return NULL;
	}
	// get upper-case first letter of the find string (to reduce the number of full strnicmps)
	TCHAR FindInitial = appToUpper(*Find);
	// get length of find string, and increment past first letter
	INT   Length = appStrlen(Find++) - 1;
	// get the first letter of the search string, and increment past it
	TCHAR StrChar = *Str++;
	// while we aren't at end of string...
	while (StrChar)
	{
		// make sure it's upper-case
		StrChar = appToUpper(StrChar);
		// if it matches the first letter of the find string, do a case-insensitive string compare for the length of the find string
		if (StrChar == FindInitial && !appStrnicmp(Str, Find, Length))
		{
			// if we found the string, then return a pointer to the beginning of it in the search string
			return Str-1;
		}
		// go to next letter
		StrChar = *Str++;
	}

	// if nothing was found, return NULL
	return NULL;
}

/**
 * Returns a static string that is full of a variable number of space (or other)
 * characters that can be used to space things out, or calculate string widths
 * Since it is static, only one return value from a call is valid at a time.
 *
 * @param NumCharacters Number of characters to but into the string, max of 255
 * @param Char Optional character to put into the string (defaults to space)
 * 
 * @return The string of NumCharacters characters.
 */
const TCHAR* appSpc( INT NumCharacters, BYTE Char )
{
	// static string storage
	static TCHAR StaticString[256];
	// previous number of chars, used to avoid duplicate work if it didn't change
	static INT OldNum=-1;
	// previous character filling the string, used to avoid duplicate work if it didn't change
	static BYTE OldChar=255;
	
	check(NumCharacters < 256);
	// if the character changed, we need to start this string over from scratch
	if (OldChar != Char)
	{
		OldNum = -1;
		OldChar = Char;
	}

	// if the number changed, fill in the array
	if( NumCharacters != OldNum )
	{
		// fill out the array with the characer
		for( OldNum=0; OldNum<NumCharacters; OldNum++ )
		{
			StaticString[OldNum] = Char;
		}
		// null terminate it
		StaticString[NumCharacters] = 0;
	}

	// return the one string
	return StaticString;
}

// 
// Trim spaces from an ascii string by zeroing them.
//
void appTrimSpaces( ANSICHAR* String )
{		
	// Find 0 terminator.
	INT t=0;
	while( (String[t]!=0 ) && (t< 1024) ) t++;
	if (t>0) t--;
	// Zero trailing spaces.
	while( (String[t]==32) && (t>0) )
	{
		String[t]=0;
		t--;
	}
}

/** 
* Duplicates a string using appMalloc.  THE RETURNED STRING MUST BE FREED BY CALLER! 
*
* @param String - null terminated array of tchars to copy
* @return newly allocated and copied tchar array 
*/
TCHAR* appStrdup(const TCHAR* String)
{
	INT StrLen = appStrlen(String);
	TCHAR* NewString = (TCHAR*)appMalloc( StrLen * sizeof(TCHAR));
	appStrcpy(NewString, StrLen, String); 
	return NewString;
}

/*-----------------------------------------------------------------------------
	Memory functions.
-----------------------------------------------------------------------------*/

//
// Memory functions.
//
void appMemswap( void* Ptr1, void* Ptr2, DWORD Size )
{
	void* Temp = appAlloca(Size);
	appMemcpy( Temp, Ptr1, Size );
	appMemcpy( Ptr1, Ptr2, Size );
	appMemcpy( Ptr2, Temp, Size );
}

/*-----------------------------------------------------------------------------
	CRC functions.
-----------------------------------------------------------------------------*/

//
// CRC32 computer based on CRC32_POLY.
//
DWORD appMemCrc( const void* InData, INT Length, DWORD CRC )
{
	BYTE* Data = (BYTE*)InData;
	CRC = ~CRC;
	for( INT i=0; i<Length; i++ )
		CRC = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ Data[i]];
	return ~CRC;
}

//
// String CRC.
//
DWORD appStrCrc( const TCHAR* Data )
{
	INT Length = appStrlen( Data );
	DWORD CRC = 0xFFFFFFFF;
	for( INT i=0; i<Length; i++ )
	{
		TCHAR C   = Data[i];
		INT   CL  = (C&255);
		CRC       = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ CL];;
#if UNICODE
		INT   CH  = (C>>8)&255;
		CRC       = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ CH];;
#endif
	}
	return ~CRC;
}

//
// String CRC, case insensitive.
//
DWORD appStrCrcCaps( const TCHAR* Data )
{
	INT Length = appStrlen( Data );
	DWORD CRC = 0xFFFFFFFF;
	for( INT i=0; i<Length; i++ )
	{
		TCHAR C   = appToUpper(Data[i]);
		INT   CL  = (C&255);
		CRC       = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ CL];
#if UNICODE
		INT   CH  = (C>>8)&255;
		CRC       = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ CH];
#endif
	}
	return ~CRC;
}

//
// Returns smallest N such that (1<<N)>=Arg.
// Note: appCeilLogTwo(0)=0 because (1<<0)=1 >= 0.
//
#if !XBOX
static BYTE GLogs[257];

BYTE appCeilLogTwo( DWORD Arg )
{
	if( --Arg == MAXDWORD )
	{
		return 0;
	}
	BYTE Shift = Arg<=0x10000 ? (Arg<=0x100?0:8) : (Arg<=0x1000000?16:24);
	return Shift + GLogs[Arg>>Shift];
}
#endif

/*-----------------------------------------------------------------------------
	MD5 functions, adapted from MD5 RFC by Brandon Reinhart
-----------------------------------------------------------------------------*/

//
// Constants for MD5 Transform.
//

enum {S11=7};
enum {S12=12};
enum {S13=17};
enum {S14=22};
enum {S21=5};
enum {S22=9};
enum {S23=14};
enum {S24=20};
enum {S31=4};
enum {S32=11};
enum {S33=16};
enum {S34=23};
enum {S41=6};
enum {S42=10};
enum {S43=15};
enum {S44=21};

static BYTE PADDING[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0
};

//
// Basic MD5 transformations.
//
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

//
// Rotates X left N bits.
//
#define ROTLEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

//
// Rounds 1, 2, 3, and 4 MD5 transformations.
// Rotation is separate from addition to prevent recomputation.
//
#define FF(a, b, c, d, x, s, ac) { \
	(a) += F ((b), (c), (d)) + (x) + (DWORD)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

#define GG(a, b, c, d, x, s, ac) { \
	(a) += G ((b), (c), (d)) + (x) + (DWORD)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

#define HH(a, b, c, d, x, s, ac) { \
	(a) += H ((b), (c), (d)) + (x) + (DWORD)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

#define II(a, b, c, d, x, s, ac) { \
	(a) += I ((b), (c), (d)) + (x) + (DWORD)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

//
// MD5 initialization.  Begins an MD5 operation, writing a new context.
//
void appMD5Init( FMD5Context* context )
{
	context->count[0] = context->count[1] = 0;
	// Load magic initialization constants.
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

//
// MD5 block update operation.  Continues an MD5 message-digest operation,
// processing another message block, and updating the context.
//
void appMD5Update( FMD5Context* context, BYTE* input, INT inputLen )
{
	INT i, index, partLen;

	// Compute number of bytes mod 64.
	index = (INT)((context->count[0] >> 3) & 0x3F);

	// Update number of bits.
	if ((context->count[0] += ((DWORD)inputLen << 3)) < ((DWORD)inputLen << 3))
		context->count[1]++;
	context->count[1] += ((DWORD)inputLen >> 29);

	partLen = 64 - index;

	// Transform as many times as possible.
	if (inputLen >= partLen) {
		appMemcpy( &context->buffer[index], input, partLen );
		appMD5Transform( context->state, context->buffer );
		for (i = partLen; i + 63 < inputLen; i += 64)
			appMD5Transform( context->state, &input[i] );
		index = 0;
	} else
		i = 0;

	// Buffer remaining input.
	appMemcpy( &context->buffer[index], &input[i], inputLen-i );
}

//
// MD5 finalization. Ends an MD5 message-digest operation, writing the
// the message digest and zeroizing the context.
// Digest is 16 BYTEs.
//
void appMD5Final ( BYTE* digest, FMD5Context* context )
{
	BYTE bits[8];
	INT index, padLen;

	// Save number of bits.
	appMD5Encode( bits, context->count, 8 );

	// Pad out to 56 mod 64.
	index = (INT)((context->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	appMD5Update( context, PADDING, padLen );

	// Append length (before padding).
	appMD5Update( context, bits, 8 );

	// Store state in digest
	appMD5Encode( digest, context->state, 16 );

	// Zeroize sensitive information.
	appMemset( context, 0, sizeof(*context) );
}

//
// MD5 basic transformation. Transforms state based on block.
//
void appMD5Transform( DWORD* state, BYTE* block )
{
	DWORD a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	appMD5Decode( x, block, 64 );

	// Round 1
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
	FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

	// Round 2
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
	GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
	GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
	GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

	// Round 3
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
	HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

	// Round 4
	II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
	II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
	II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
	II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
	II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
	II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
	II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	appMemset( x, 0, sizeof(x) );
}

//
// Encodes input (DWORD) into output (BYTE).
// Assumes len is a multiple of 4.
//
void appMD5Encode( BYTE* output, DWORD* input, INT len )
{
	INT i, j;

	for (i = 0, j = 0; j < len; i++, j += 4) {
		output[j] = (BYTE)(input[i] & 0xff);
		output[j+1] = (BYTE)((input[i] >> 8) & 0xff);
		output[j+2] = (BYTE)((input[i] >> 16) & 0xff);
		output[j+3] = (BYTE)((input[i] >> 24) & 0xff);
	}
}

//
// Decodes input (BYTE) into output (DWORD).
// Assumes len is a multiple of 4.
//
void appMD5Decode( DWORD* output, BYTE* input, INT len )
{
	INT i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((DWORD)input[j]) | (((DWORD)input[j+1]) << 8) |
		(((DWORD)input[j+2]) << 16) | (((DWORD)input[j+3]) << 24);
}

/*-----------------------------------------------------------------------------
	SHA-1
-----------------------------------------------------------------------------*/

// Rotate x bits to the left
#ifndef ROL32
#ifdef _MSC_VER
#define ROL32(_val32, _nBits) _rotl(_val32, _nBits)
#else
#define ROL32(_val32, _nBits) (((_val32)<<(_nBits))|((_val32)>>(32-(_nBits))))
#endif
#endif

#ifdef __INTEL_BYTE_ORDER__
	#define SHABLK0(i) (m_block->l[i] = (ROL32(m_block->l[i],24) & 0xFF00FF00) | (ROL32(m_block->l[i],8) & 0x00FF00FF))
#else
	#define SHABLK0(i) (m_block->l[i])
#endif

#define SHABLK(i) (m_block->l[i&15] = ROL32(m_block->l[(i+13)&15] ^ m_block->l[(i+8)&15] \
	^ m_block->l[(i+2)&15] ^ m_block->l[i&15],1))

// SHA-1 rounds
#define _R0(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK0(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define _R1(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define _R2(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0x6ED9EBA1+ROL32(v,5); w=ROL32(w,30); }
#define _R3(v,w,x,y,z,i) { z+=(((w|x)&y)|(w&x))+SHABLK(i)+0x8F1BBCDC+ROL32(v,5); w=ROL32(w,30); }
#define _R4(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0xCA62C1D6+ROL32(v,5); w=ROL32(w,30); }

FSHA1::FSHA1()
{
	m_block = (SHA1_WORKSPACE_BLOCK *)m_workspace;

	Reset();
}

FSHA1::~FSHA1()
{
	Reset();
}

void FSHA1::Reset()
{
	// SHA1 initialization constants
	m_state[0] = 0x67452301;
	m_state[1] = 0xEFCDAB89;
	m_state[2] = 0x98BADCFE;
	m_state[3] = 0x10325476;
	m_state[4] = 0xC3D2E1F0;

	m_count[0] = 0;
	m_count[1] = 0;
}

void FSHA1::Transform(DWORD *state, BYTE *buffer)
{
	// Copy state[] to working vars
	DWORD a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];

	appMemcpy(m_block, buffer, 64);

	// 4 rounds of 20 operations each. Loop unrolled.
	_R0(a,b,c,d,e, 0); _R0(e,a,b,c,d, 1); _R0(d,e,a,b,c, 2); _R0(c,d,e,a,b, 3);
	_R0(b,c,d,e,a, 4); _R0(a,b,c,d,e, 5); _R0(e,a,b,c,d, 6); _R0(d,e,a,b,c, 7);
	_R0(c,d,e,a,b, 8); _R0(b,c,d,e,a, 9); _R0(a,b,c,d,e,10); _R0(e,a,b,c,d,11);
	_R0(d,e,a,b,c,12); _R0(c,d,e,a,b,13); _R0(b,c,d,e,a,14); _R0(a,b,c,d,e,15);
	_R1(e,a,b,c,d,16); _R1(d,e,a,b,c,17); _R1(c,d,e,a,b,18); _R1(b,c,d,e,a,19);
	_R2(a,b,c,d,e,20); _R2(e,a,b,c,d,21); _R2(d,e,a,b,c,22); _R2(c,d,e,a,b,23);
	_R2(b,c,d,e,a,24); _R2(a,b,c,d,e,25); _R2(e,a,b,c,d,26); _R2(d,e,a,b,c,27);
	_R2(c,d,e,a,b,28); _R2(b,c,d,e,a,29); _R2(a,b,c,d,e,30); _R2(e,a,b,c,d,31);
	_R2(d,e,a,b,c,32); _R2(c,d,e,a,b,33); _R2(b,c,d,e,a,34); _R2(a,b,c,d,e,35);
	_R2(e,a,b,c,d,36); _R2(d,e,a,b,c,37); _R2(c,d,e,a,b,38); _R2(b,c,d,e,a,39);
	_R3(a,b,c,d,e,40); _R3(e,a,b,c,d,41); _R3(d,e,a,b,c,42); _R3(c,d,e,a,b,43);
	_R3(b,c,d,e,a,44); _R3(a,b,c,d,e,45); _R3(e,a,b,c,d,46); _R3(d,e,a,b,c,47);
	_R3(c,d,e,a,b,48); _R3(b,c,d,e,a,49); _R3(a,b,c,d,e,50); _R3(e,a,b,c,d,51);
	_R3(d,e,a,b,c,52); _R3(c,d,e,a,b,53); _R3(b,c,d,e,a,54); _R3(a,b,c,d,e,55);
	_R3(e,a,b,c,d,56); _R3(d,e,a,b,c,57); _R3(c,d,e,a,b,58); _R3(b,c,d,e,a,59);
	_R4(a,b,c,d,e,60); _R4(e,a,b,c,d,61); _R4(d,e,a,b,c,62); _R4(c,d,e,a,b,63);
	_R4(b,c,d,e,a,64); _R4(a,b,c,d,e,65); _R4(e,a,b,c,d,66); _R4(d,e,a,b,c,67);
	_R4(c,d,e,a,b,68); _R4(b,c,d,e,a,69); _R4(a,b,c,d,e,70); _R4(e,a,b,c,d,71);
	_R4(d,e,a,b,c,72); _R4(c,d,e,a,b,73); _R4(b,c,d,e,a,74); _R4(a,b,c,d,e,75);
	_R4(e,a,b,c,d,76); _R4(d,e,a,b,c,77); _R4(c,d,e,a,b,78); _R4(b,c,d,e,a,79);

	// Add the working vars back into state
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
}

// Use this function to hash in binary data and strings
void FSHA1::Update(BYTE *data, DWORD len)
{
	DWORD i, j;

	j = (m_count[0] >> 3) & 63;

	if((m_count[0] += len << 3) < (len << 3)) m_count[1]++;

	m_count[1] += (len >> 29);

	if((j + len) > 63)
	{
		i = 64 - j;
		appMemcpy(&m_buffer[j], data, i);
		Transform(m_state, m_buffer);

		for( ; i + 63 < len; i += 64) Transform(m_state, &data[i]);

		j = 0;
	}
	else i = 0;

	appMemcpy(&m_buffer[j], &data[i], len - i);
}

void FSHA1::Final()
{
	DWORD i;
	BYTE finalcount[8];

	for(i = 0; i < 8; i++)
	{
		finalcount[i] = (BYTE)((m_count[((i >= 4) ? 0 : 1)] >> ((3 - (i & 3)) * 8) ) & 255); // Endian independent
	}

	Update((BYTE*)"\200", 1);

	while ((m_count[0] & 504) != 448)
	{
		Update((BYTE*)"\0", 1);
	}

	Update(finalcount, 8); // Cause a SHA1Transform()

	for(i = 0; i < 20; i++)
	{
		m_digest[i] = (BYTE)((m_state[i >> 2] >> ((3 - (i & 3)) * 8) ) & 255);
	}
}

// Get the raw message digest
void FSHA1::GetHash(BYTE *puDest)
{
	appMemcpy(puDest, m_digest, 20);
}

/**
* Calculate the hash on a single block and return it
*
* @param Data Input data to hash
* @param DataSize Size of the Data block
* @param OutHash Resulting hash value (20 byte buffer)
*/
void FSHA1::HashBuffer(void* Data, DWORD DataSize, BYTE* OutHash)
{
	// do an atomic hash operation
	FSHA1 Sha;
	Sha.Update((BYTE*)Data, DataSize);
	Sha.Final();
	Sha.GetHash(OutHash);
}


/*-----------------------------------------------------------------------------
	Exceptions.
-----------------------------------------------------------------------------*/

//
// Throw a string exception with a message.
//
VARARG_BODY( void VARARGS, appThrowf, const TCHAR*, VARARG_NONE )
{
	static TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Fmt, Fmt );
#if EXCEPTIONS_DISABLED // @todo hack
	debugf(TEXT("THROW: %s"), TempStr);
	#if PS3
		PS3Callstack();
	#endif
	appDebugBreak();
#else
	throw( TempStr );
#endif
}

/*-----------------------------------------------------------------------------
	Parameter parsing.
-----------------------------------------------------------------------------*/

//
// Get a string from a text string.
//
UBOOL Parse
(
	const TCHAR* Stream, 
	const TCHAR* Match,
	TCHAR*		 Value,
	INT			 MaxLen
)
{
	const TCHAR* Found = appStrfind(Stream,Match);
	const TCHAR* Start;

	if( Found )
	{
		Start = Found + appStrlen(Match);
		if( *Start == '\x22' )
		{
			// Quoted string with spaces.
			appStrncpy( Value, Start+1, MaxLen );
			Value[MaxLen-1]=0;
			TCHAR* Temp = appStrstr( Value, TEXT("\x22") );
			if( Temp != NULL )
				*Temp=0;
		}
		else
		{
			// Non-quoted string without spaces.
			appStrncpy( Value, Start, MaxLen );
			Value[MaxLen-1]=0;
			TCHAR* Temp;
			Temp = appStrstr( Value, TEXT(" ")  ); if( Temp ) *Temp=0;
			Temp = appStrstr( Value, TEXT("\r") ); if( Temp ) *Temp=0;
			Temp = appStrstr( Value, TEXT("\n") ); if( Temp ) *Temp=0;
			Temp = appStrstr( Value, TEXT("\t") ); if( Temp ) *Temp=0;
			Temp = appStrstr( Value, TEXT(",")  ); if( Temp ) *Temp=0;
		}
		return 1;
	}
	else return 0;
}

//
// Checks if a command-line parameter exists in the stream.
//
UBOOL ParseParam( const TCHAR* Stream, const TCHAR* Param )
{
	const TCHAR* Start = Stream;
	if( *Stream )
	{
		while( (Start=appStrfind(Start+1,Param)) != NULL )
		{
			if( Start>Stream && (Start[-1]=='-' || Start[-1]=='/') )
			{
				const TCHAR* End = Start + appStrlen(Param);
				if ( End == NULL || *End == 0 || appIsWhitespace(*End) )
                    return 1;
			}
		}
	}
	return 0;
}

// 
// Parse a string.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FString& Value )
{
	TCHAR Temp[4096]=TEXT("");
	if( ::Parse( Stream, Match, Temp, ARRAY_COUNT(Temp) ) )
	{
		Value = Temp;
		return 1;
	}
	else return 0;
}

//
// Parse a quadword.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, QWORD& Value )
{
	return Parse( Stream, Match, *(SQWORD*)&Value );
}

//
// Parse a signed quadword.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SQWORD& Value )
{
	TCHAR Temp[4096]=TEXT(""), *Ptr=Temp;
	if( ::Parse( Stream, Match, Temp, ARRAY_COUNT(Temp) ) )
	{
		Value = 0;
		UBOOL Negative = (*Ptr=='-');
		Ptr += Negative;
		while( *Ptr>='0' && *Ptr<='9' )
			Value = Value*10 + *Ptr++ - '0';
		if( Negative )
			Value = -Value;
		return 1;
	}
	else return 0;
}

//
// Get an object from a text stream.
//
UBOOL ParseObject( const TCHAR* Stream, const TCHAR* Match, UClass* Class, UObject*& DestRes, UObject* InParent )
{
	TCHAR TempStr[1024];
	if( !Parse( Stream, Match, TempStr, ARRAY_COUNT(TempStr) ) )
	{
		return 0;
	}
	else if( appStricmp(TempStr,TEXT("NONE"))==0 )
	{
		DestRes = NULL;
		return 1;
	}
	else
	{
		// Look this object up.
		UObject* Res;
		Res = UObject::StaticFindObject( Class, InParent, TempStr );
		if( !Res )
			return 0;
		DestRes = Res;
		return 1;
	}
}

//
// Get a name.
//
UBOOL Parse
(
	const TCHAR* Stream, 
	const TCHAR* Match, 
	FName& Name
)
{
	TCHAR TempStr[NAME_SIZE];

	if( !Parse(Stream,Match,TempStr,NAME_SIZE) )
		return 0;

	Name = FName(TempStr);

	return 1;
}

//
// Get a DWORD.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, DWORD& Value )
{
	const TCHAR* Temp = appStrfind(Stream,Match);
	TCHAR* End;
	if( Temp==NULL )
		return 0;
	Value = appStrtoi( Temp + appStrlen(Match), &End, 10 );

	return 1;
}

//
// Get a byte.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, BYTE& Value )
{
	const TCHAR* Temp = appStrfind(Stream,Match);
	if( Temp==NULL )
		return 0;
	Temp += appStrlen( Match );
	Value = (BYTE)appAtoi( Temp );
	return Value!=0 || appIsDigit(Temp[0]);
}

//
// Get a signed byte.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SBYTE& Value )
{
	const TCHAR* Temp = appStrfind(Stream,Match);
	if( Temp==NULL )
		return 0;
	Temp += appStrlen( Match );
	Value = appAtoi( Temp );
	return Value!=0 || appIsDigit(Temp[0]);
}

//
// Get a word.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, WORD& Value )
{
	const TCHAR* Temp = appStrfind( Stream, Match );
	if( Temp==NULL )
		return 0;
	Temp += appStrlen( Match );
	Value = (WORD)appAtoi( Temp );
	return Value!=0 || appIsDigit(Temp[0]);
}

//
// Get a signed word.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SWORD& Value )
{
	const TCHAR* Temp = appStrfind( Stream, Match );
	if( Temp==NULL )
		return 0;
	Temp += appStrlen( Match );
	Value = (SWORD)appAtoi( Temp );
	return Value!=0 || appIsDigit(Temp[0]);
}

//
// Get a floating-point number.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FLOAT& Value )
{
	const TCHAR* Temp = appStrfind( Stream, Match );
	if( Temp==NULL )
		return 0;
	Value = appAtof( Temp+appStrlen(Match) );
	return 1;
}

//
// Get a signed double word.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, INT& Value )
{
	const TCHAR* Temp = appStrfind( Stream, Match );
	if( Temp==NULL )
		return 0;
	Value = appAtoi( Temp + appStrlen(Match) );
	return 1;
}

//
// Get a boolean value.
//
UBOOL ParseUBOOL( const TCHAR* Stream, const TCHAR* Match, UBOOL& OnOff )
{
	TCHAR TempStr[16];
	if( Parse( Stream, Match, TempStr, 16 ) )
	{
		OnOff
		=	!appStricmp(TempStr,TEXT("On"))
		||	!appStricmp(TempStr,TEXT("True"))
		||	!appStricmp(TempStr,GTrue)
		||	!appStricmp(TempStr,TEXT("1"));
		return 1;
	}
	else return 0;
}

//
// Get a globally unique identifier.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FGuid& Guid )
{
	TCHAR Temp[256];
	if( !Parse( Stream, Match, Temp, ARRAY_COUNT(Temp) ) )
		return 0;

	Guid.A = Guid.B = Guid.C = Guid.D = 0;
	if( appStrlen(Temp)==32 )
	{
		TCHAR* End;
		Guid.D = appStrtoi( Temp+24, &End, 16 ); Temp[24]=0;
		Guid.C = appStrtoi( Temp+16, &End, 16 ); Temp[16]=0;
		Guid.B = appStrtoi( Temp+8,  &End, 16 ); Temp[8 ]=0;
		Guid.A = appStrtoi( Temp+0,  &End, 16 ); Temp[0 ]=0;
	}
	return 1;
}

//
// Sees if Stream starts with the named command.  If it does,
// skips through the command and blanks past it.  Returns 1 of match,
// 0 if not.
//
UBOOL ParseCommand
(
	const TCHAR** Stream, 
	const TCHAR*  Match
)
{
	while( (**Stream==' ')||(**Stream==9) )
		(*Stream)++;

	if( appStrnicmp(*Stream,Match,appStrlen(Match))==0 )
	{
		*Stream += appStrlen(Match);
		if( !appIsAlnum(**Stream) )
		{
			while ((**Stream==' ')||(**Stream==9)) (*Stream)++;
			return 1; // Success.
		}
		else
		{
			*Stream -= appStrlen(Match);
			return 0; // Only found partial match.
		}
	}
	else return 0; // No match.
}

//
// Get next command.  Skips past comments and cr's.
//
void ParseNext( const TCHAR** Stream )
{
	// Skip over spaces, tabs, cr's, and linefeeds.
	SkipJunk:
	while( **Stream==' ' || **Stream==9 || **Stream==13 || **Stream==10 )
		++*Stream;

	if( **Stream==';' )
	{
		// Skip past comments.
		while( **Stream!=0 && **Stream!=10 && **Stream!=13 )
			++*Stream;
		goto SkipJunk;
	}

	// Upon exit, *Stream either points to valid Stream or a nul.
}

//
// Grab the next space-delimited string from the input stream.
// If quoted, gets entire quoted string.
//
UBOOL ParseToken( const TCHAR*& Str, TCHAR* Result, INT MaxLen, UBOOL UseEscape )
{
	INT Len=0;

	// Skip spaces and tabs.
	while( *Str==' ' || *Str==9 )
		Str++;
	if( *Str == TEXT('"') )
	{
		// Get quoted string.
		Str++;
		while( *Str && *Str!=TEXT('"') && (Len+1)<MaxLen )
		{
			TCHAR c = *Str++;
			if( c=='\\' && UseEscape )
			{
				// Get escape.
				c = *Str++;
				if( !c )
					break;
			}
			if( (Len+1)<MaxLen )
				Result[Len++] = c;
		}
		if( *Str==TEXT('"') )
			Str++;
	}
	else
	{
		// Get unquoted string.
		for( ; *Str && *Str!=' ' && *Str!=9; Str++ )
			if( (Len+1)<MaxLen )
				Result[Len++] = *Str;
	}
	Result[Len]=0;
	return Len!=0;
}
UBOOL ParseToken( const TCHAR*& Str, FString& Arg, UBOOL UseEscape )
{
	Arg.Empty();

	// Skip spaces and tabs.
	while( appIsWhitespace(*Str) )
	{
		Str++;
	}

	if ( *Str == TEXT('"') )
	{
		// Get quoted string.
		Str++;
		while( *Str && *Str != TCHAR('"') )
		{
			TCHAR c = *Str++;
			if( c==TEXT('\\') && UseEscape )
			{
				// Get escape.
				c = *Str++;
				if( !c )
					break;
			}

			Arg += c;
		}

		if ( *Str == TEXT('"') )
		{
			Str++;
		}
	}
	else
	{
		// Get unquoted string.
		for( ; *Str && !appIsWhitespace(*Str); Str++ )
		{
			Arg += *Str;
		}
	}

	return Arg.Len() > 0;
}
FString ParseToken( const TCHAR*& Str, UBOOL UseEscape )
{
	TCHAR Buffer[1024];
	if( ParseToken( Str, Buffer, ARRAY_COUNT(Buffer), UseEscape ) )
		return Buffer;
	else
		return TEXT("");
}

//
// Get a line of Stream (everything up to, but not including, CR/LF.
// Returns 0 if ok, nonzero if at end of stream and returned 0-length string.
//
UBOOL ParseLine
(
	const TCHAR**	Stream,
	TCHAR*			Result,
	INT				MaxLen,
	UBOOL			Exact
)
{
	UBOOL GotStream=0;
	UBOOL IsQuoted=0;
	UBOOL Ignore=0;

	*Result=0;
	while( **Stream!=0 && **Stream!=10 && **Stream!=13 && --MaxLen>0 )
	{
		// Start of comments.
		if( !IsQuoted && !Exact && (*Stream)[0]=='/' && (*Stream)[1]=='/' )
			Ignore = 1;
		
		// Command chaining.
		if( !IsQuoted && !Exact && **Stream=='|' )
			break;

		// Check quoting.
		IsQuoted = IsQuoted ^ (**Stream==34);
		GotStream=1;

		// Got stuff.
		if( !Ignore )
			*(Result++) = *((*Stream)++);
		else
			(*Stream)++;
	}
	if( Exact )
	{
		// Eat up exactly one CR/LF.
		if( **Stream == 13 )
			(*Stream)++;
		if( **Stream == 10 )
			(*Stream)++;
	}
	else
	{
		// Eat up all CR/LF's.
		while( **Stream==10 || **Stream==13 || **Stream=='|' )
			(*Stream)++;
	}
	*Result=0;
	return **Stream!=0 || GotStream;
}
UBOOL ParseLine
(
	const TCHAR**	Stream,
	FString&		Result,
	UBOOL			Exact
)
{
	UBOOL GotStream=0;
	UBOOL IsQuoted=0;
	UBOOL Ignore=0;

	Result = TEXT("");

	while( **Stream!=0 && **Stream!=10 && **Stream!=13 )
	{
		// Start of comments.
		if( !IsQuoted && !Exact && (*Stream)[0]=='/' && (*Stream)[1]=='/' )
			Ignore = 1;

		// Command chaining.
		if( !IsQuoted && !Exact && **Stream=='|' )
			break;

		// Check quoting.
		IsQuoted = IsQuoted ^ (**Stream==34);
		GotStream=1;

		// Got stuff.
		if( !Ignore )
		{
			Result.AppendChar( *((*Stream)++) );
		}
		else
		{
			(*Stream)++;
		}
	}
	if( Exact )
	{
		// Eat up exactly one CR/LF.
		if( **Stream == 13 )
			(*Stream)++;
		if( **Stream == 10 )
			(*Stream)++;
	}
	else
	{
		// Eat up all CR/LF's.
		while( **Stream==10 || **Stream==13 || **Stream=='|' )
			(*Stream)++;
	}

	return **Stream!=0 || GotStream;
}

/*----------------------------------------------------------------------------
	Localization.
----------------------------------------------------------------------------*/

const TCHAR* GKnownLanguageExtensions[] = 
{
	TEXT("int"),
	TEXT("jpn"),
	TEXT("deu"),
	TEXT("fra"),
	TEXT("esm"),
	TEXT("esn"),
	TEXT("ita"),
	TEXT("kor"),
	TEXT("chi"),
	// @todo: Put more languages in here!
	NULL
};

FString LocalizeLabel( const TCHAR* Section, const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt, UBOOL Optional )
{
	return Localize(Section,Key,Package,LangExt,Optional)+TEXT(":");
}

FString Localize( const TCHAR* Section, const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt, UBOOL bOptional )
{
	// Errors during early loading can cause Localize to be called before GConfig is initialized.
	if( !GIsStarted || !GConfig || !GSys )
	{
		return Key;
	}

	// The default behaviour for Localize is to use the configured language which is indicated by passing in NULL.
	if( LangExt == NULL )
	{
		LangExt = UObject::GetLanguage();
	}

	FString Result;
	UBOOL	bFoundMatch = FALSE;

	// We allow various .inis to contribute multiple paths for localization files.
	for( INT PathIndex=0; PathIndex<GSys->LocalizationPaths.Num(); PathIndex++ )
	{
		// Try specified language first
		FFilename FilenameLang	= FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("%s") PATH_SEPARATOR TEXT("%s.%s"), *GSys->LocalizationPaths(PathIndex), LangExt	  , Package, LangExt	 );
		if ( GConfig->GetString( Section, Key, Result, *FilenameLang ) )
		{
			// found it in the localized language file
			bFoundMatch = TRUE;
			break;
		}
	}

	if ( !bFoundMatch && appStricmp(LangExt, TEXT("INT")) )
	{
		// if we haven't found it yet, fall back to default (int) and see if it exists there.
		for( INT PathIndex=0; PathIndex<GSys->LocalizationPaths.Num(); PathIndex++ )
		{
			FFilename FilenameInt	= FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("%s") PATH_SEPARATOR TEXT("%s.%s"), *GSys->LocalizationPaths(PathIndex), TEXT("INT"), Package, TEXT("INT") );
			if ( GConfig->GetString( Section, Key, Result, *FilenameInt ) )
			{
				bFoundMatch = TRUE;
				break;
			}
		}

		if ( bFoundMatch )
		{
			static UBOOL bShowMissingLoc = ParseParam(appCmdLine(), TEXT("SHOWMISSINGLOC"));
			if ( bShowMissingLoc )
			{
				// the value was not found in the loc file for the current language, but was found in the english files
				// if we want to see the location for missing localized text, return the error string instead of the english text
				bFoundMatch = FALSE;
				bOptional = FALSE;
			}
		}
		else
		{
			// if it wasn't found in the english files either, just ignore it
			bOptional = TRUE;
		}
	}

	// Complain about missing localization for non optional queries.
	if( !bFoundMatch && !bOptional )
	{
		debugfSuppressed( NAME_LocalizationWarning, TEXT("No localization: %s.%s.%s (%s)"), Package, Section, Key, LangExt );
		Result = FString::Printf( TEXT("<?%s?%s.%s.%s?>"), LangExt, Package, Section, Key );
	}

	// if we have any \n's in the text file, replace them with real line feeds
	// (when read in from a .int file, the \n's are converted to \\n)
	if (Result.InStr(TEXT("\\n")) != -1)
	{
		Result= Result.Replace(TEXT("\\n"), TEXT("\n"));
	}

	// repeat for \t's
	if (Result.InStr(TEXT("\\t")) != -1)
	{
		Result= Result.Replace(TEXT("\\t"), TEXT("\t"));
	}

	// Use "###" as a comment token.
	Result.Split( TEXT("###"), &Result, NULL );
	
	return Result;
}
FString LocalizeError( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return Localize( TEXT("Errors"), Key, Package, LangExt );
}
FString LocalizeProgress( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return Localize( TEXT("Progress"), Key, Package, LangExt );
}
FString LocalizeQuery( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return Localize( TEXT("Query"), Key, Package, LangExt );
}
FString LocalizeGeneral( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return Localize( TEXT("General"), Key, Package, LangExt );
}
FString LocalizeUnrealEd( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return Localize( TEXT("UnrealEd"), Key, Package, LangExt );
}

#if UNICODE
FString LocalizeLabel( const ANSICHAR* Section, const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt, UBOOL Optional )
{
	return LocalizeLabel( ANSI_TO_TCHAR(Section), ANSI_TO_TCHAR(Key), Package, LangExt, Optional );
}
FString Localize( const ANSICHAR* Section, const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt, UBOOL Optional )
{
	return Localize( ANSI_TO_TCHAR(Section), ANSI_TO_TCHAR(Key), Package, LangExt, Optional );
}
FString LocalizeError( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return LocalizeError( ANSI_TO_TCHAR(Key), Package, LangExt );
}
FString LocalizeProgress( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return LocalizeProgress( ANSI_TO_TCHAR(Key), Package, LangExt );
}
FString LocalizeQuery( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return LocalizeQuery( ANSI_TO_TCHAR(Key), Package, LangExt );
}
FString LocalizeGeneral( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return LocalizeGeneral( ANSI_TO_TCHAR(Key), Package, LangExt );
}
FString LocalizeUnrealEd( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt )
{
	return LocalizeUnrealEd( ANSI_TO_TCHAR(Key), Package, LangExt );
}
#endif

/*-----------------------------------------------------------------------------
	High level file functions.
-----------------------------------------------------------------------------*/

//
// Load a binary file to a dynamic array.
//
UBOOL appLoadFileToArray( TArray<BYTE>& Result, const TCHAR* Filename, FFileManager* FileManager )
{
	FArchive* Reader = FileManager->CreateFileReader( Filename );
	if( !Reader )
		return 0;
	Result.Empty();
	Result.Add( Reader->TotalSize() );
	Reader->Serialize( &Result(0), Result.Num() );
	UBOOL Success = Reader->Close();
	delete Reader;
	return Success;
}

//
// Converts an arbitrary text buffer to an FString.
// Supports all combination of ANSI/Unicode files and platforms.
//
void appBufferToString( FString& Result, const BYTE* Buffer, INT Size )
{
	TArray<TCHAR>& ResultArray = Result.GetCharArray();
	ResultArray.Empty();

	if( Size >= 2 && !( Size & 1 ) && Buffer[0] == 0xff && Buffer[1] == 0xfe )
	{
		// Unicode Intel byte order. Less 1 for the FFFE header, additional 1 for null terminator.
		ResultArray.Add( Size / 2 );
		for( INT i = 0; i < ( Size / 2 ) - 1; i++ )
		{
			ResultArray( i ) = FromUnicode( ( WORD )( ANSICHARU )Buffer[i * 2 + 2] + ( WORD )( ANSICHARU )Buffer[i * 2 + 3] * 256 );
		}
	}
	else if( Size >= 2 && !( Size & 1 ) && Buffer[0] == 0xfe && Buffer[1] == 0xff )
	{
		// Unicode non-Intel byte order. Less 1 for the FFFE header, additional 1 for null terminator.
		ResultArray.Add( Size / 2 );
		for( INT i = 0; i < ( Size / 2 ) - 1; i++ )
		{
			ResultArray( i ) = FromUnicode( ( WORD )( ANSICHARU )Buffer[i * 2 + 3] + ( WORD )( ANSICHARU )Buffer[i * 2 + 2] * 256 );
		}
	}
	else
	{
		// ANSI. Additional 1 for null terminator.
		ResultArray.Add( Size + 1 );
		for( INT i = 0; i < Size; i++ )
		{
			ResultArray( i ) = FromAnsi( Buffer[i] );
		}
	}

	// Ensure null terminator is present
	ResultArray.Last() = 0;
}

//
// Load a text file to an FString.
// Supports all combination of ANSI/Unicode files and platforms.
//
UBOOL appLoadFileToString( FString& Result, const TCHAR* Filename, FFileManager* FileManager )
{
	FArchive* Reader = FileManager->CreateFileReader( Filename );
	if( !Reader )
	{
		return 0;
	}

	INT Size = Reader->TotalSize();
	TArray<ANSICHAR> Ch( Size );
	Reader->Serialize( &Ch( 0 ), Size );
	UBOOL Success = Reader->Close();
	delete Reader;

	appBufferToString( Result, ( const BYTE* )&Ch( 0 ), Ch.Num() );
	return Success;
}

//
// Save a binary array to a file.
//
UBOOL appSaveArrayToFile( const TArray<BYTE>& Array, const TCHAR* Filename, FFileManager* FileManager )
{
	FArchive* Ar = FileManager->CreateFileWriter( Filename );
	if( !Ar )
		return 0;
	Ar->Serialize( const_cast<BYTE*>(&Array(0)), Array.Num() );
	delete Ar;
	return 1;
}

//
// Write the FString to a file.
// Supports all combination of ANSI/Unicode files and platforms.
//
UBOOL appSaveStringToFile( const FString& String, const TCHAR* Filename, FFileManager* FileManager )
{
	if( !String.Len() )
		return 0;
	FArchive* Ar = FileManager->CreateFileWriter( Filename );
	if( !Ar )
		return 0;
	UBOOL SaveAsUnicode=0, Success=1;
#if UNICODE
	for( INT i=0; i<String.Len(); i++ )
	{
		if( (*String)[i] != (TCHAR)(ANSICHARU)ToAnsi((*String)[i]) )
		{
			UNICHAR BOM = UNICODE_BOM;
			Ar->Serialize( &BOM, sizeof(BOM) );
			SaveAsUnicode = 1;
			break;
		}
	}
#endif
	if( SaveAsUnicode || sizeof(TCHAR)==1 )
	{
		Ar->Serialize( const_cast<TCHAR*>(*String), String.Len()*sizeof(TCHAR) );
	}
	else
	{
		TArray<ANSICHAR> AnsiBuffer(String.Len());
		for( INT i=0; i<String.Len(); i++ )
			AnsiBuffer(i) = ToAnsi((*String)[i]);
		Ar->Serialize( const_cast<ANSICHAR*>(&AnsiBuffer(0)), String.Len() );
	}
	delete Ar;
	if( !Success )
		GFileManager->Delete( Filename );
	return Success;
}

static INT BitmapIndex = -1;
UBOOL appCreateBitmap( const TCHAR* Pattern, INT Width, INT Height, FColor* Data, FFileManager* FileManager )
{
	TCHAR File[MAX_SPRINTF]=TEXT("");
	// if the Pattern already has a .bmp extension, then use that the file to write to
	if (FFilename(Pattern).GetExtension() == TEXT("bmp"))
	{
		appStrcpy(File, Pattern);
	}
	else
	{
		for( INT TestBitmapIndex=BitmapIndex+1; TestBitmapIndex<65536; TestBitmapIndex++ )
		{
			appSprintf( File, TEXT("%s%05i.bmp"), Pattern, TestBitmapIndex );
			if( FileManager->FileSize(File) < 0 )
			{
				BitmapIndex = TestBitmapIndex;
				break;
			}
		}
		if (BitmapIndex == 65536)
		{
			return FALSE;
		}
	}

	FArchive* Ar = FileManager->CreateFileWriter( File );
	if( Ar )
	{
		// Types.
		#if SUPPORTS_PRAGMA_PACK
			#pragma pack (push,1)
		#endif
		struct BITMAPFILEHEADER
		{
			WORD   bfType GCC_PACK(1);
			DWORD   bfSize GCC_PACK(1);
			WORD   bfReserved1 GCC_PACK(1); 
			WORD   bfReserved2 GCC_PACK(1);
			DWORD   bfOffBits GCC_PACK(1);
		} FH; 
		struct BITMAPINFOHEADER
		{
			DWORD  biSize GCC_PACK(1); 
			INT    biWidth GCC_PACK(1);
			INT    biHeight GCC_PACK(1);
			WORD  biPlanes GCC_PACK(1);
			WORD  biBitCount GCC_PACK(1);
			DWORD  biCompression GCC_PACK(1);
			DWORD  biSizeImage GCC_PACK(1);
			INT    biXPelsPerMeter GCC_PACK(1); 
			INT    biYPelsPerMeter GCC_PACK(1);
			DWORD  biClrUsed GCC_PACK(1);
			DWORD  biClrImportant GCC_PACK(1); 
		} IH;
		#if SUPPORTS_PRAGMA_PACK
			#pragma pack (pop)
		#endif

		UINT	BytesPerLine = Align(Width * 3,4);

		// File header.
		FH.bfType       		= INTEL_ORDER16((WORD) ('B' + 256*'M'));
		FH.bfSize       		= INTEL_ORDER32((DWORD) (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + BytesPerLine * Height));
		FH.bfReserved1  		= INTEL_ORDER16((WORD) 0);
		FH.bfReserved2  		= INTEL_ORDER16((WORD) 0);
		FH.bfOffBits    		= INTEL_ORDER32((DWORD) (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)));
		Ar->Serialize( &FH, sizeof(FH) );

		// Info header.
		IH.biSize               = INTEL_ORDER32((DWORD) sizeof(BITMAPINFOHEADER));
		IH.biWidth              = INTEL_ORDER32((DWORD) Width);
		IH.biHeight             = INTEL_ORDER32((DWORD) Height);
		IH.biPlanes             = INTEL_ORDER16((WORD) 1);
		IH.biBitCount           = INTEL_ORDER16((WORD) 24);
		IH.biCompression        = INTEL_ORDER32((DWORD) 0); //BI_RGB
		IH.biSizeImage          = INTEL_ORDER32((DWORD) BytesPerLine * Height);
		IH.biXPelsPerMeter      = INTEL_ORDER32((DWORD) 0);
		IH.biYPelsPerMeter      = INTEL_ORDER32((DWORD) 0);
		IH.biClrUsed            = INTEL_ORDER32((DWORD) 0);
		IH.biClrImportant       = INTEL_ORDER32((DWORD) 0);
		Ar->Serialize( &IH, sizeof(IH) );

		// Colors.
		for( INT i=Height-1; i>=0; i-- )
		{
			for( INT j=0; j<Width; j++ )
			{
				Ar->Serialize( &Data[i*Width+j].B, 1 );
				Ar->Serialize( &Data[i*Width+j].G, 1 );
				Ar->Serialize( &Data[i*Width+j].R, 1 );
			}

			// Pad each row's length to be a multiple of 4 bytes.

			for(UINT PadIndex = Width * 3;PadIndex < BytesPerLine;PadIndex++)
			{
				BYTE	B = 0;
				Ar->Serialize(&B,1);
			}
		}

		// Success.
		delete Ar;
	}
	else 
		return 0;

	// Success.
	return 1;
}

/**
 * Finds a usable splash pathname for the given filename
 * 
 * @param SplashFilename Name of the desired splash name ("Splash.bmp")
 * @param OutPath String containing the path to the file, if this function returns TRUE
 *
 * @return TRUE if a splash screen was found
 */
UBOOL appGetSplashPath(const TCHAR* SplashFilename, FString& OutPath)
{
	// we need a file manager!
	if (GFileManager == NULL)
	{
		return FALSE;
	}

	// first look in game's splash directory
	OutPath = appGameDir() + "Splash\\" + SplashFilename;
	
	// if this was found, then we're done
	if (GFileManager->FileSize(*OutPath) != -1)
	{
		return TRUE;
	}

	// next look in Engine\\Splash
	OutPath = FString(TEXT("..\\Engine\\Splash\\")) + SplashFilename;

	// if this was found, then we're done
	if (GFileManager->FileSize(*OutPath) != -1)
	{
		return TRUE;
	}

	// if not found yet, then return failure
	return FALSE;
}


/*-----------------------------------------------------------------------------
	Files.
-----------------------------------------------------------------------------*/

/**
 * This will recurse over a directory structure looking for files.
 * 
 * @param Result The output array that is filled out with a file paths
 * @param RootDirectory The root of the directory structure to recurse through
 * @param bFindPackages Should this function add package files to the Results
 * @param bFindNonPackages Should this function add non-package files to the Results
 */
void appFindFilesInDirectory(TArray<FString>& Results, const TCHAR* RootDirectory, UBOOL bFindPackages, UBOOL bFindNonPackages)
{
	// cache a temp FString of the root directory
	FString Root(RootDirectory);

	// make a wild card to look for directories in the current directory
	FString Wildcard = FString(RootDirectory) * TEXT("*.*");
	TArray<FString> SubDirs; 
	// find the directories, not files
	GFileManager->FindFiles(SubDirs, *Wildcard, FALSE, TRUE);
	// recurse through the directories looking for more files/directories
	for (INT SubDirIndex = 0; SubDirIndex < SubDirs.Num(); SubDirIndex++)
	{
		appFindFilesInDirectory(Results,  *(Root * SubDirs(SubDirIndex)), bFindPackages, bFindNonPackages);
	}

	// look for any files in this directory
	TArray<FString> Files;
	// look for files, not directories
	GFileManager->FindFiles(Files, *Wildcard, TRUE, FALSE);
	// go over the file list
	for (INT FileIndex = 0; FileIndex < Files.Num(); FileIndex++)
	{
		// create a filename out of the found file
		FFilename Filename(Files(FileIndex));

		// this file is a package if its extension is registered as a package extension
		// (if we are doing this before GSys is setup, then we can't tell if it's a valid 
		// extension, which comes from GSys (ini files), so assume it is NOT a package)
		UBOOL bIsPackage = GSys && GSys->Extensions.FindItemIndex(*Filename.GetExtension()) != INDEX_NONE;

		// add this file if its a package and we want packages, or vice versa
		if ((bFindPackages && bIsPackage) || (bFindNonPackages && !bIsPackage))
		{
			Results.AddItem(Root * Files(FileIndex));
		}
	}
}

/**
 * This struct holds information about a cached package
 */
struct FDLCInfo
{
	/** Full path to the package */
	FString Path;
	/** Which user this content is associated with, or NO_USER_SPECIFIED if no user */
	INT UserIndex;

	/** Constructor */
	FDLCInfo(FString InPath, INT InUserIndex)
		: Path(InPath)
		, UserIndex(InUserIndex)
	{
	}
};

struct FMapPackageFileCache : public FPackageFileCache
{
private:
	TMap<FString, FString> FileLookup;		// find a package file by lowercase filename
	TMap<FString, FDLCInfo> DownloadedFileLookup;		// find a downloaded package file by lowercase filename

	/** 
	 * Cache all packages in this path and any directories under it
	 * 
	 * @param InPath The path to look in for packages
	 */
	void CachePath( const TCHAR* InPath);
public:
	virtual void CachePaths();
	virtual UBOOL CachePackage( const TCHAR* InPathName, UBOOL InOverrideDupe=0, UBOOL WarnIfExists=1 );
	virtual UBOOL FindPackageFile( const TCHAR* InName, const FGuid* Guid, FString& OutFileName, const TCHAR* Language=NULL, UBOOL bShouldLookForSFPackages=TRUE );
	virtual TArray<FString> GetPackageFileList();

	/**
	 * Add a downloaded content package to the list of known packages.
	 * Can be assigned to a particular ID for removing with ClearDownloadadPackages.
	 *
	 * @param InPlatformPathName The platform-specific full path name to a package (will be passed directly to CreateFileReader)
	 * @param UserIndex Optional user to associate with the package so that it can be flushed later
	 *
	 * @return TRUE if successful
	 */
	virtual UBOOL CacheDownloadedPackage(const TCHAR* InPlatformPathName, INT UserIndex);

	/**
	 * Remove any downloaded packages associated with the given user from the cache.
	 * If the UserIndex is not supplied, only packages cached without a UserIndex
	 * will be removed (it will not remove all packages).
	 *
	 * @param UserIndex Optional UserIndex for which packages to remove
	 */
	virtual void ClearDownloadedPackages(INT UserIndex);
};


/**
 * Replaces all slashes and backslashes in the string with the correct path separator character for the current platform.
 *
 * @param	InFilename	a string representing a filename.
 */
void FPackageFileCache::NormalizePathSeparators( FString& InFilename )
{
	for( TCHAR* p=(TCHAR*)*InFilename; *p; p++ )
	{
		if( *p == '\\' || *p == '/' )
		{
			*p = PATH_SEPARATOR[0];
		}
	}
}


/**
 * Strips all path and extension information from a relative or fully qualified file name.
 *
 * @param	InPathName	a relative or fully qualified file name
 *
 * @return	the passed in string, stripped of path and extensions
 */
FString FPackageFileCache::PackageFromPath( const TCHAR* InPathName )
{
	FString PackageName = InPathName;
	INT i = PackageName.InStr( PATH_SEPARATOR, 1 );
	if( i != -1 )
		PackageName = PackageName.Mid(i+1);

	i = PackageName.InStr( TEXT("/"), 1 );
	if( i != -1 )
		PackageName = PackageName.Mid(i+1);

	i = PackageName.InStr( TEXT("\\"), 1 );
	if( i != -1 )
		PackageName = PackageName.Mid(i+1);

	i = PackageName.InStr( TEXT(".") );
	if( i != -1 )
		PackageName = PackageName.Left(i);
	PackageName = PackageName.ToLower();

	return PackageName;
}

/**
 * Parses a fully qualified or relative filename into its components (filename, path, extension).
 *
 * @param	InPathName	the filename to parse
 * @param	Path		[out] receives the value of the path portion of the input string
 * @param	Filename	[out] receives the value of the filename portion of the input string
 * @param	Extension	[out] receives the value of the extension portion of the input string
 */
void FPackageFileCache::SplitPath( const TCHAR* InPathName, FString& Path, FString& Filename, FString& Extension )
{
	Filename = InPathName;

	// first, make sure all path separators are the correct type for this platform
	NormalizePathSeparators(Filename);

	INT i = Filename.InStr( PATH_SEPARATOR, TRUE );
	if( i != INDEX_NONE )
	{
		Path = Filename.Left(i);
		Filename = Filename.Mid(i+1);
	}
	else
	{
		Path = TEXT("");
	}

	i = Filename.InStr( TEXT("."), TRUE );
	if( i != -1 )
	{
		Extension = Filename.Mid(i+1);
		Filename = Filename.Left(i);
	}
	else
	{
		Extension = TEXT("");
	}
}

/** 
 * Cache all packages in this path and any directories under it
 * 
 * @param InPath The path to look in for packages
 */
void FMapPackageFileCache::CachePath( const TCHAR* InPath)
{
	// find all packages in, and under the path
	TArray<FString> Packages;
	appFindFilesInDirectory(Packages, InPath, TRUE, FALSE);

	for (INT PackageIndex = 0; PackageIndex < Packages.Num(); PackageIndex++)
	{
		CachePackage(*Packages(PackageIndex));
	}
}

/**
 * Takes a fully pathed string and eliminates relative pathing (eg: annihilates ".." with the adjacent directory).
 * Assumes all slashes have been converted to PATH_SEPARATOR[0].
 * For example, takes the string:
 *	BaseDirectory/SomeDirectory/../SomeOtherDirectory/Filename.ext
 * and converts it to:
 *	BaseDirectory/SomeOtherDirectory/Filename.ext
 *
 * @param	InString	a pathname potentially containing relative pathing
 */
static FString CollapseRelativeDirectories(const FString& InString)
{
	// For each occurrance of "..PATH_SEPARATOR" eat a directory to the left.

	FString ReturnString = InString;
	FString LeftString, RightString;

	FPackageFileCache::NormalizePathSeparators(ReturnString);

	const FString SearchString = FString::Printf(TEXT("..") PATH_SEPARATOR);
	while( ReturnString.Split( SearchString, &LeftString, &RightString ) )
	{
		//debugf(TEXT("== %s == %s =="), *LeftString, *RightString );

		// Strip the first directory off LeftString.
		INT Index = LeftString.Len()-1;

		// Eat the slash on the end of left if there is one.
		if ( Index >= 0 && LeftString[Index] == PATH_SEPARATOR[0] )
		{
			--Index;
		}

		// Eat leftwards until a slash is hit.
		while ( Index >= 0 && LeftString[Index] != PATH_SEPARATOR[0] )
		{
			LeftString[Index--] = 0;
		}

		ReturnString = FString( *LeftString ) + FString( *RightString );
		//debugf(TEXT("after split: %s"), *ReturnString);
	}

	return ReturnString;
}

// Converts a Takes a potentially relative path and fills it out.
FString appConvertRelativePathToFull(const FString& InString)
{
	FString FullyPathed;
	if ( InString.StartsWith( TEXT("../") ) || InString.StartsWith( TEXT("..\\") ) )
	{
		FullyPathed = FString( appBaseDir() );
	}
	FullyPathed *= InString;

	return CollapseRelativeDirectories( FullyPathed );
}

/**
 * Adds the package name specified to the runtime lookup map.  The stripped package name (minus extension or path info) will be mapped
 * to the fully qualified or relative filename specified.
 *
 * @param	InPathName		a fully qualified or relative [to Binaries] path name for an Unreal package file.
 * @param	InOverrideDupe	specify TRUE to replace existing mapping with the new path info
 * @param	WarnIfExists	specify TRUE to write a warning to the log if there is an existing entry for this package name
 *
 * @return	TRUE if the specified path name was successfully added to the lookup table; FALSE if an entry already existed for this package
 */
UBOOL FMapPackageFileCache::CachePackage( const TCHAR* InPathName, UBOOL InOverrideDupe, UBOOL WarnIfExists )
{
	// strip all path and extension info from the file
	FString PackageName = PackageFromPath( InPathName );

	// replace / and \ with PATH_SEPARATOR
	FFilename FixedPathName = InPathName;
	NormalizePathSeparators(FixedPathName);

	FString* ExistingEntry = FileLookup.Find(*PackageName);
	if( !InOverrideDupe && ExistingEntry )
	{
		// Expand relative paths to make sure that we don't get invalid ambiguous name warnings
		// eg flesh out c:\dir1\dir2\..\somePath2\file.war and ..\somePath2\file.war to see if they truly are the same.

		const FFilename FullExistingEntry = appConvertRelativePathToFull( *ExistingEntry );
		//debugf(TEXT("%s --> %s"), **ExistingEntry, *FullExistingEntry);

		const FFilename FullFixedPathName = appConvertRelativePathToFull( FixedPathName );
		//debugf(TEXT("%s --> %s"), *FixedPathName, *FullFixedPathName );

		// If the expanded existing entry is the same as the old, ignore the Cache request.
		if( FullFixedPathName.GetBaseFilename(FALSE) == FullExistingEntry.GetBaseFilename(FALSE) )
		{
			return TRUE;
		}


		if( WarnIfExists == TRUE )
		{
			SET_WARN_COLOR(COLOR_RED);
			warnf( NAME_Error, TEXT("Ambiguous package name: Using \'%s\', not \'%s\'"), *FullExistingEntry, *FullFixedPathName);
			CLEAR_WARN_COLOR();

			if( GIsUnattended == FALSE )
			{
				appMsgf(AMT_OK,TEXT("Ambiguous package name: Using \'%s\', not \'%s\'"), *FullExistingEntry, *FullFixedPathName);
			}
		}

		return FALSE;
	}
	else
	{
		//warnf(TEXT("Setting %s -> %s"), *PackageName, *FixedPathName );
		FileLookup.Set( *PackageName, *FixedPathName );
		return TRUE;
	}
}

/** 
 * Cache all packages found in the engine's configured paths directories, recursively.
 */
void FMapPackageFileCache::CachePaths()
{
	check(GSys);
	FileLookup.Empty();

	// decide which paths to use by commandline parameter
	FString PathSet(TEXT("Normal"));
	Parse(appCmdLine(), TEXT("PATHS="), PathSet);

	TArray<FString>& Paths = (PathSet == TEXT("Cutdown")) ? GSys->CutdownPaths : GSys->Paths;

	// get the list of script package directories
	appGetScriptPackageDirectories(Paths);

	// loop through the specified paths and cache all packages found therein
	for (INT PathIndex = 0; PathIndex < Paths.Num(); PathIndex++)
	{
		CachePath(*Paths(PathIndex));
	}
}

/**
 * Finds the fully qualified or relative pathname for the package specified.
 *
 * @param	InName			a string representing the filename of an Unreal package; may include path and/or extension.
 * @param	Guid			if specified, searches the directory containing files downloaded from other servers (cache directory) for the specified package.
 * @param	OutFilename		receives the full [or relative] path that was originally registered for the package specified.
 * @param	Language		Language version to retrieve if overridden for that particular language
 * @param	bShouldLookForSFPackages If TRUE, first look for Package_SF before looking for Package. Defaults to TRUE.
 *
 * @return	TRUE if the package was successfully found.
 */
UBOOL FMapPackageFileCache::FindPackageFile( const TCHAR* InName, const FGuid* Guid, FString& OutFileName, const TCHAR* Language, UBOOL bShouldLookForSFPackages )
{
	// Use current language if none specified.
	if( Language == NULL )
	{
		Language = UObject::GetLanguage();
	}

	// Don't return it if it's a library.
	if( appStrlen(InName)>appStrlen(DLLEXT) && appStricmp( InName + appStrlen(InName)-appStrlen(DLLEXT), DLLEXT )==0 )
	{
		return FALSE;
	}

	// Get the base name (engine, not ..\MyGame\Script\Engine.u)
	FFilename BasePackageName = PackageFromPath(InName);

	// track if the file was found
	UBOOL bFoundFile = FALSE;
	FString PackageFileName;

#if XBOX
	// on xbox, we don't support package downloading, so no need to waste any extra cycles/disk io dealing with it
	Guid = NULL;
#endif

#if !CONSOLE
	// First, check to see if the file exists.  This doesn't work for case sensitive filenames but it'll work for commandlets with hardcoded paths or
	// when calling this with the result of FindPackageFile:
	//		[pseudo code here]
	//		PackagePath = FindPackageFile(PackageName);
	//		LoadPackage(PackagePath);
	//			GetPackageLinker(PackagePath);
	//				PackagePath2 = FindPackageFile(PackagePath);
	if (GFileManager->FileSize(InName)>=0)
	{
		// Make sure we have it in the cache.
		CachePackage( InName, 1 );

		// remember the path as the given path
		PackageFileName = InName;
		bFoundFile = TRUE;
		//warnf(TEXT("FindPackage: %s found directly"), InName);
	}
#endif

	// if we haven't found it yet, search by package name
	if (!bFoundFile)
	{
		// if we don't allow looking for _SF packages, skip the first iteration of the loop
		for (INT SeekFreePass = bShouldLookForSFPackages ? 0 : 1; SeekFreePass < 2 && !bFoundFile; SeekFreePass++)
		{
			for (INT LocPass = 0; LocPass < 3 && !bFoundFile; LocPass++)
			{
				// first seek free pass, look for package_SF,
				// second pass, just use base
				FFilename PackageName = (SeekFreePass == 0) ? BasePackageName + STANDALONE_SEEKFREE_SUFFIX : BasePackageName;

				// First pass we look at Package_LANG.ext
				if( LocPass == 0 )
				{
					PackageName = PackageName.GetLocalizedFilename( Language );
				}
				// Second pass we look at Package_INT.ext for partial localization.
				else if( LocPass == 1 )
				{
					PackageName = PackageName.GetLocalizedFilename( TEXT("INT") );
				}
				// Third pass just Package.ext
				
				// first, look in downloaded packages (so DLC can override normal packages)
				FDLCInfo* ExistingDLCEntry = DownloadedFileLookup.Find(*PackageName);
				// if we found some downloaded content, return it
				if (ExistingDLCEntry)
				{
					PackageFileName = ExistingDLCEntry->Path;
					bFoundFile = TRUE;
				}

				FString* ExistingEntry = FileLookup.Find(*PackageName);
				if( ExistingEntry )
				{
					PackageFileName = *ExistingEntry;
					bFoundFile = TRUE;
				}
			}
		}
	}

	// if we successfully found the package by name, then we have to check the Guid to make sure it matches
	if (bFoundFile && Guid)
	{
		// @todo: If we could get to list of linkers here, it would be faster to check
		// then to open the file and read it
		FArchive* PackageReader = GFileManager->CreateFileReader(*PackageFileName);
		// this had better open
		check(PackageReader != NULL);

		// read in the package summary
		FPackageFileSummary Summary;
		*PackageReader << Summary;

		// compare Guids
		if (Summary.Guid != *Guid)
		{
			bFoundFile = FALSE;
		}

		// close package file
		delete PackageReader;
	}

	// if matching by filename and guid succeeded, then we are good to go, otherwise, look in the cache for a guid match (if a Guid was specified)
	if (bFoundFile)
	{
		OutFileName = PackageFileName;
	}
	else if (Guid)
	{
		// figure out the name it would have been saved as (CachePath\Guid.CacheExt)
		FString GuidPackageName = GSys->CachePath * Guid->String() + GSys->CacheExt;
		// does it exist?
		if (GFileManager->FileSize(*GuidPackageName) != -1)
		{
			// if no name was specified, we've done all the checking we can
			if (InName != NULL)
			{
				// temporarily allow the GConfigCache to perform file operations if they were off
				UBOOL bWereFileOpsDisabled = GConfig->AreFileOperationsDisabled();
				GConfig->EnableFileOperations();

				FString IniName = GSys->CachePath * TEXT("Cache.ini");
				FString PackageName;
				// yes, now make sure the cache.ini entry matches names
				FConfigCacheIni CacheIni;
				// compare names if it's found in the ini
				if (CacheIni.GetString(TEXT("Cache"), *Guid->String(), PackageName, *IniName))
				{
					// if the guid matched and the name, then mark that we found the file
					// @todo: if it didn't match, we probably should delete it, as something is fishy
					if (BasePackageName == PackageName)
					{
						bFoundFile = TRUE;
						// set the package path to the cache path
						OutFileName = GuidPackageName;

						// if we found the cache package, and will therefore use it, then touch it 
						// so that it won't expire after N days
						GFileManager->TouchFile(*OutFileName);
					}
				}

				// re-disable file ops if they were before
				if (bWereFileOpsDisabled)
				{
					GConfig->DisableFileOperations();
				}
			}
		}
	}

	return bFoundFile;
}

/**
 * Returns the list of fully qualified or relative pathnames for all registered packages.
 */
TArray<FString> FMapPackageFileCache::GetPackageFileList()
{
	TArray<FString>	Result;
	for(TMap<FString, FDLCInfo>::TIterator It(DownloadedFileLookup);It;++It)
	{
		::new(Result) FString(It.Value().Path);
	}
	for(TMap<FString,FString>::TIterator It(FileLookup);It;++It)
	{
		::new(Result) FString(It.Value());
	}
	return Result;
}

/**
 * Add a downloaded content package to the list of known packages.
 * Can be assigned to a particular ID for removing with ClearDownloadadPackages.
 *
 * @param InPlatformPathName The platform-specific full path name to a package (will be passed directly to CreateFileReader)
 * @param UserIndex Optional user to associate with the package so that it can be flushed later
 *
 * @return TRUE if successful
 */
UBOOL FMapPackageFileCache::CacheDownloadedPackage(const TCHAR* InPlatformPathName, INT UserIndex)
{
	// make a new DLC info structure
	FDLCInfo NewInfo(FString(InPlatformPathName), UserIndex);

	// get the package name from the full path
	FString PackageName = PackageFromPath(InPlatformPathName);

	// add this to our list of downloaded package
	DownloadedFileLookup.Set(*PackageName, NewInfo);

	return TRUE;
}

/**
 * Remove any downloaded packages associated with the given user from the cache.
 * If the UserIndex is not supplied, only packages cached without a UserIndex
 * will be removed (it will not remove all packages).
 *
 * @param UserIndex Optional UserIndex for which packages to remove
 */
void FMapPackageFileCache::ClearDownloadedPackages(INT UserIndex)
{
	// go hrough the downloaded content package
	for(TMap<FString, FDLCInfo>::TIterator It(DownloadedFileLookup);It;++It)
	{
		// if it the user we want to remove, then remove it from the map
		if (It.Value().UserIndex == UserIndex)
		{
			It.RemoveCurrent();
		}
	}
}

FPackageFileCache* GPackageFileCache;

/** 
* Creates a temporary filename. 
*
* @param Path - file pathname
* @param Result1024 - destination buffer to store results of unique path (@warning must be >= MAX_SPRINTF size)
*/
void appCreateTempFilename( const TCHAR* Path, TCHAR* Result, SIZE_T ResultSize )
{
	check( ResultSize >= MAX_SPRINTF );
	static INT i=0;
	do
		appSprintf( Result, TEXT("%s\\%04X.tmp"), Path, i++ );
	while( GFileManager->FileSize(Result)>0 );
}

/*-----------------------------------------------------------------------------
	Game/ mode specific directories.
-----------------------------------------------------------------------------*/

/**
 * Returns the base directory of the current game by looking at the global
 * GGameName variable. This is usually a subdirectory of the installation
 * root directory and can be overridden on the command line to allow self
 * contained mod support.
 *
 * @return base directory
 */
FString appGameDir()
{
	return FString::Printf( TEXT("..") PATH_SEPARATOR TEXT("%sGame") PATH_SEPARATOR, GGameName );
}

/**
 * Returns the directory the engine uses to look for the leaf ini files. This
 * can't be an .ini variable for obvious reasons.
 *
 * @return config directory
 */
FString appGameConfigDir()
{
	return appGameDir() + TEXT("Config") PATH_SEPARATOR;
}

/**
 * Returns the directory the engine uses to output logs. This currently can't 
 * be an .ini setting as the game starts logging before it can read from .ini
 * files.
 *
 * @return log directory
 */
FString appGameLogDir()
{
	return appGameDir() + TEXT("Logs") PATH_SEPARATOR;
}

/** 
 * Returns the base directory of the "core" engine that can be shared across
 * several games or across games & mods. Shaders and base localization files
 * e.g. reside in the engine directory.
 *
 * @return engine directory
 */
FString appEngineDir()
{
	return TEXT("..") PATH_SEPARATOR TEXT("Engine") PATH_SEPARATOR;
}

/**
 * Returns the directory the root configuration files are located.
 *
 * @return root config directory
 */
FString appEngineConfigDir()
{
	return appEngineDir() + TEXT("Config") + PATH_SEPARATOR;
}

/**
 * Returns the directory the engine should save compiled script packages to.
 */
FString appScriptOutputDir()
{
	check(GConfig);

	FString ScriptOutputDir;

#if FINAL_RELEASE
	verify(GConfig->GetString( TEXT("Editor.EditorEngine"), TEXT("FRScriptOutputPath"), ScriptOutputDir, GEngineIni ));
#else
	if ( ParseParam(appCmdLine(), TEXT("FINAL_RELEASE")) )
	{
		verify(GConfig->GetString( TEXT("Editor.EditorEngine"), TEXT("FRScriptOutputPath"), ScriptOutputDir, GEngineIni ));
	}
	else
	{
		verify(GConfig->GetString( TEXT("Editor.EditorEngine"), TEXT("EditPackagesOutPath"), ScriptOutputDir, GEngineIni ));
	}
#endif

	return ScriptOutputDir;
}

/**
 * Returns the pathnames for the directories which contain script packages.
 *
 * @param	ScriptPackagePaths	receives the list of directory paths to use for loading script packages 
 */
void appGetScriptPackageDirectories( TArray<FString>& ScriptPackagePaths )
{
	check(GSys);

#if FINAL_RELEASE
	ScriptPackagePaths += GSys->FRScriptPaths;
#else
	if ( ParseParam(appCmdLine(), TEXT("FINAL_RELEASE")) )
	{
		ScriptPackagePaths += GSys->FRScriptPaths;
	}
	else
	{
		ScriptPackagePaths += GSys->ScriptPaths;
	}
#endif
}

/*-----------------------------------------------------------------------------
	Init and Exit.
-----------------------------------------------------------------------------*/

//
// General initialization.
//
TCHAR GCmdLine[4096]=TEXT("");

const TCHAR* appCmdLine()
{
	return GCmdLine;
}

/**
 * This will completely load an .ini file into the passed in FConfigFile.  This means that it will 
 * recurse up the BasedOn hierarchy loading each of those .ini.  The passed in FConfigFile will then
 * have the data after combining all of those .ini 
 *
 * @param FilenameToLoad - this is the path to the file to 
 * @param ConfigFile - This is the FConfigFile which will have the contents of the .ini loaded into and Combined()
 * @param bUpdateIniTimeStamps - whether to update the timestamps array.  Only for Default___.ini should this be set to TRUE.  The generated .inis already have the timestamps.
 *
 **/
void LoadAnIniFile( const TCHAR* FilenameToLoad, FConfigFile& ConfigFile, UBOOL bUpdateIniTimeStamps )
{
	// This shouldn't be getting called if seekfree is enabled on console.
	check(!GUseSeekFreeLoading || !CONSOLE);

	// if the file does not exist then return
	if( GFileManager->FileSize( FilenameToLoad ) <= 0 )
	{
		//warnf( TEXT( "LoadAnIniFile was unable to find FilenameToLoad: %s "), FilenameToLoad );
		return;
	}

	TArray<DOUBLE> TimestampsOfInis;

	// Keep a list of ini's, starting with Source and ending with the root configuration file.
	TArray<FString> IniList;
	INT IniIndex = IniList.AddItem( FString(FilenameToLoad) );

	FConfigFile TmpConfigFile;
	UBOOL bFoundBasedOnText = FALSE; 

	// Recurse inis till we found a root ini (aka one without BasedOn being set).
	do
	{
		// Spit out friendly error if there was a problem locating .inis (e.g. bad command line parameter or missing folder, ...).
		if( GFileManager->FileSize( *IniList(IniIndex) ) < 0 )
		{
			GConfig = NULL;
			appErrorf( NAME_FriendlyError, TEXT("Couldn't locate '%s' which is required to run '%s'"), *IniList(IniIndex), GGameName );
		}

		// read in the based on .ini file
		TmpConfigFile.Read( *IniList(IniIndex) );
		//debugf( TEXT( "Just read in: %s" ), *IniList(IniIndex) );

		IniIndex = IniList.AddZeroed(); // so the get can replace it
		bFoundBasedOnText = TmpConfigFile.GetString( TEXT("Configuration"), TEXT("BasedOn"), IniList(IniIndex) );

	} while( bFoundBasedOnText == TRUE );

	// Discard empty entry.
	IniIndex--;
	//debugf( TEXT( "Discard empty entry" ) );

	// Read root ini.
	//debugf( TEXT( "Combining configFile: %s" ), *IniList(IniIndex) );
	ConfigFile.Read( *IniList(IniIndex) );
	DOUBLE AFiletimestamp = GFileManager->GetFileTimestamp(*IniList(IniIndex));
	TimestampsOfInis.AddItem( AFiletimestamp );

	// Traverse ini list back to front, merging along the way.
	for( IniIndex--; IniIndex >= 0; IniIndex-- )
	{
		//debugf( TEXT( "Combining configFile: %s" ), *IniList(IniIndex) );
		ConfigFile.Combine( *IniList(IniIndex) );
		AFiletimestamp = GFileManager->GetFileTimestamp( *IniList(IniIndex) );
		TimestampsOfInis.AddItem( AFiletimestamp );
	}

	if( bUpdateIniTimeStamps == TRUE )
	{
		// for loop of the number of files
		for( INT Idx = 0; Idx < TimestampsOfInis.Num(); ++Idx )
		{
			DOUBLE ATimestamp = TimestampsOfInis(Idx);

			TCHAR TimestampIdx[MAX_SPRINTF]=TEXT("");
			appSprintf( TimestampIdx, TEXT("%d"), Idx );
			ConfigFile.SetDouble( TEXT("IniVersion"), TimestampIdx, ATimestamp );
		}

	}
}

/**
 * This will load up two .ini files and then determine if the Generated one is outdated.
 * Outdatedness is determined by the following mechanic:
 *
 * When a generated .ini is written out it will store the timestamps of the files it was generated
 * from.  This way whenever the Default__.inis are modified the Generated .ini will view itself as
 * outdated and regenerate it self.
 *
 * Outdatedness also can be affected by commandline params which allow one to delete all .ini, have
 * automated build system etc.
 *
 * Additionally, this function will save the previous generation of the .ini to the Config dir with
 * a datestamp.
 *
 * Finally, this function will load the Generated .ini into the global ConfigManager.
 *
 * @param DefaultIniFile		The Default .ini file (e.g. DefaultEngine.ini )
 * @param GeneratedIniFile		The Generated .ini file (e.g. FooGameEngine.ini )
 * @param YesNoToAll			[out] Receives the user's selection if an .ini was out of date.
 *
 **/
void appCheckIniForOutdatedness( const TCHAR* DefaultIniFile, const TCHAR* GeneratedIniFile, UINT& YesNoToAll )
{
	// Not needed for seekfree loading on console
	if( !GUseSeekFreeLoading || !CONSOLE )
	{
		// need to check to see if the file already exists in the GConfigManager's cache
		// if it does exist then go ahead and return as we don't need to load it up again
		if( GConfig->FindConfigFile( GeneratedIniFile ) != NULL )
		{
			//debugf( TEXT( "Request to load a config file that was already loaded: %s" ), GeneratedIniFile );
			return;
		}

		// recursively load up the config file
		FConfigFile DefaultConfigFile;
		LoadAnIniFile( DefaultIniFile, DefaultConfigFile, TRUE );

		FConfigFile GeneratedConfigFile; 
		LoadAnIniFile( GeneratedIniFile, GeneratedConfigFile, FALSE );

		// now that we have both of the files loaded we need to check the various dates
		// here we look to see if both the number of timestamp entries matches AND the contents
		// of those entries match

		UBOOL bFilesWereDifferent = FALSE;
		UBOOL bFoundIniEntry = TRUE;
		INT Count = 0;
		while( bFoundIniEntry == TRUE )
		{
			TCHAR TimestampIdx[MAX_SPRINTF]=TEXT("");
			appSprintf( TimestampIdx, TEXT("%d"), Count );

			DOUBLE DefaultIniTimestamp = 0;
			DOUBLE GeneratedIniTimestamp = 0;

			bFoundIniEntry = DefaultConfigFile.GetDouble( TEXT("IniVersion"), TimestampIdx, DefaultIniTimestamp );
			GeneratedConfigFile.GetDouble( TEXT("IniVersion"), TimestampIdx, GeneratedIniTimestamp );

			if( DefaultIniTimestamp != GeneratedIniTimestamp )
			{
				int breakme = 0;
				bFilesWereDifferent = TRUE;
				break;
			}
			

			++Count;
		}

		// Regenerate the ini file?
		UBOOL bShouldRegenerate = FALSE;
		if( ParseParam(appCmdLine(),TEXT("REGENERATEINIS")) == TRUE )
		{
			bShouldRegenerate = TRUE;
		}
		else if( bFilesWereDifferent == TRUE )
		{
			if( ( GIsUnattended == TRUE )
				|| ( GIsSilent == TRUE )
				|| ( ParseParam(appCmdLine(),TEXT("UPDATEINISAUTO")) == TRUE )
				)
			{
				bShouldRegenerate = TRUE;
			}
			else
			{
				// Check to see if the GeneratedIniFile exists.  If it does then we might need to prompt to 
				// overwrite it.  Otherwise, since it does not exist we will just create it
				const UBOOL bGeneratedFileExists = (GFileManager->FileSize(GeneratedIniFile) > 0 );
				if( bGeneratedFileExists == FALSE )
				{
					bShouldRegenerate = TRUE;
				}
				else
				{
					// Flag indicating whether the user has requested 'Yes/No To All'.
					static INT GIniYesNoToAll = -1;
					// Make sure GIniYesNoToAll's 'uninitialized' value is kosher.
					checkAtCompileTime( ART_YesAll != -1 );
					checkAtCompileTime( ART_NoAll != -1 );

					// The file exists but is different.
					// Prompt the user if they haven't already responded with a 'Yes/No To All' answer.
					if( GIniYesNoToAll != ART_YesAll && GIniYesNoToAll != ART_NoAll )
					{
						YesNoToAll = appMsgf( AMT_YesNoYesAllNoAll, TEXT("Your ini (%s) file is outdated. Do you want to automatically update it saving the previous version? Not doing so might cause crashes!"), GeneratedIniFile );
						// Record whether the user responded with a 'Yes/No To All' answer.
						if ( YesNoToAll == ART_YesAll || YesNoToAll == ART_NoAll )
						{
							GIniYesNoToAll = YesNoToAll;
						}
					}
					else
					{
						// The user has already responded with a 'Yes/No To All' answer, so note it 
						// in the output arg so that calling code can operate on its value.
						YesNoToAll = GIniYesNoToAll;
					}
					// Regenerate the file if approved by the user.
					bShouldRegenerate = (YesNoToAll == ART_Yes) || (YesNoToAll == ART_YesAll);
				}
			}
		}

		// Regenerate the file.
		if( bShouldRegenerate )
		{
			// overwrite the GeneratedFile with the Default data PLUS the default data's timestamps which it has
			DefaultConfigFile.Dirty = TRUE;
			DefaultConfigFile.Write( GeneratedIniFile );
		}

		// after this has run we are guaranteed to have the correct generated .ini files 
		// so we need to load them up into the GlobalConfigManager
		GConfig->LoadFile( GeneratedIniFile, &DefaultConfigFile );
	}
}

/**
 * This will create the .ini filenames for the Default and the Game based off the passed in values.
 * (e.g. DefaultEditor.ini, MyLeetGameEditor.ini  and the respective relative paths to those files )
 *
 * Once the names have been created we will call a function to check to see if the generated .ini
 * is outdated.
 *
 * @param GeneratedIniName				The Global TCHAR[1024] that unreal uses ( e.g. GEngineIni )
 * @param CommandLineDefaultIniToken	The token to look for on the command line to parse the passed in Default<Type>Ini
 * @param CommandLineIniToken			The token to look for on the command line to parse the passed in <Type>Ini
 * @param IniFileName					The IniFile's name  (e.g. Engine.ini Editor.ini )
 * @param DefaultIniPrefix				What the prefix for the Default .inis should be  ( e.g. Default )
 * @param IniPrefix						What the prefix for the Game's .inis should be  ( e.g. <gameName> )
 * @param IniSectionToLookFor			The section to look for in the .ini
 * @param IniKeyToLookFor				The key to look for in the specified section of the .ini
 * @param YesNoToAll					[out] Receives the user's selection if an .ini was out of date.
 */
void appCreateIniNamesAndThenCheckForOutdatedness( TCHAR* GeneratedIniName, const TCHAR* CommandLineDefaultIniName, const TCHAR* CommandLineIniName, const TCHAR* IniFileName, const TCHAR* DefaultIniPrefix, const TCHAR* IniPrefix, UINT& YesNoToAll )
{
	// This shouldn't be getting called for seekfree on console.
	check(!GUseSeekFreeLoading || !CONSOLE);

	TCHAR DefaultIniName[MAX_SPRINTF]=TEXT("");

	// if the command line doesn't have the default INI NAME on it it calculate it
	if( Parse( appCmdLine(), CommandLineDefaultIniName, DefaultIniName, ARRAY_COUNT(DefaultIniName) ) != TRUE )
	{
		appSprintf( DefaultIniName, TEXT("%s%s%s"), *appGameConfigDir(), DefaultIniPrefix, IniFileName );
	}

	// if the command line doesn't have the INI NAME on it it calculate it
	if( Parse( appCmdLine(), CommandLineIniName, GeneratedIniName, ARRAY_COUNT(DefaultIniName) ) != TRUE )
	{
		appSprintf( GeneratedIniName, TEXT("%s%s%s%s"), *appGameConfigDir(), IniPrefix, GGameName, IniFileName );
	}
	
	// so now we have the names we are going to be utilizing for both the default ini and the game ini file
	// we need to check to see if the ini should be regenerated
	appCheckIniForOutdatedness( DefaultIniName, GeneratedIniName, YesNoToAll );
}

/**
 * Generates a default INI name using the current platform and the IniFileName suffix passed in.
 *
 * @param IniFileName			INI File suffix to append to the end of the generated name (Engine.ini, Editor.ini, etc...)
 * @param OutGeneratedIniName	Pointer to store the generated filename in, should be atleast 1024 bytes.
 */
void appCreateDefaultIniName(const TCHAR* IniFileName, TCHAR* OutGeneratedIniName)
{
	appSprintf( OutGeneratedIniName, TEXT("%s%s%s%s"), *appGameConfigDir(), DEFAULT_INI_PREFIX, INI_PREFIX, IniFileName );
}

void appDeleteOldLogs()
{
	INT PurgeLogsDays = 0;
	GConfig->GetInt(TEXT("LogFiles"), TEXT("PurgeLogsDays"), PurgeLogsDays, GEngineIni);
	if (PurgeLogsDays > 0)
	{
		// get a list of files in the log dir
		TArray<FString> Files;
		GFileManager->FindFiles(Files, *FString::Printf(TEXT("%s*.*"), *appGameLogDir()), TRUE, FALSE);

		// delete all those with the backup text in their name and that are older than the specified number of days
		DOUBLE MaxFileAgeSeconds = 60.0 * 60.0 * 24.0 * DOUBLE(PurgeLogsDays);
		for (INT i = 0; i < Files.Num(); i++)
		{
			FString FullFileName = appGameLogDir() + Files(i);
			if (FullFileName.InStr(BACKUP_LOG_FILENAME_POSTFIX) != INDEX_NONE && GFileManager->GetFileAgeSeconds(*FullFileName) > MaxFileAgeSeconds)
			{
				debugf(TEXT("Deleting old log file %s"), *Files(i));
				GFileManager->Delete(*FullFileName);
			}
		}
	}
}

/**
* Main initialization function for app
*/
void appInit( const TCHAR* InCmdLine, FOutputDevice* InLog, FOutputDeviceConsole* InLogConsole, FOutputDeviceError* InError, FFeedbackContext* InWarn, FFileManager* InFileManager, FCallbackEventObserver* InCallbackEventDevice, FCallbackQueryDevice* InCallbackQueryDevice, FConfigCache*(*ConfigFactory)() )
{
	GCallbackEvent = InCallbackEventDevice;
	check(GCallbackEvent);
	GCallbackQuery = InCallbackQueryDevice;
	check(GCallbackQuery);

#if TRACK_SERIALIZATION_PERFORMANCE || LOOKING_FOR_PERF_ISSUES
	if ( GObjectSerializationPerfTracker == NULL )
	{
		GObjectSerializationPerfTracker = new FStructEventMap;
	}

	if ( GClassSerializationPerfTracker == NULL )
	{
		GClassSerializationPerfTracker = new FStructEventMap;
	}
#endif

#if !XBOX
	// Init log table.
	{for( INT i=0,e=-1,c=0; i<=256; i++ )
	{
		GLogs[i] = e+1;
		if( !i || ++c>=(1<<e) )
		{
			c=0, e++;
		}
	}}
#endif

	// Command line.
#if _MSC_VER
	if( *InCmdLine=='\"' )
	{
		InCmdLine++;
		while( *InCmdLine && *InCmdLine!='\"' )
		{
			InCmdLine++;
		}
		if( *InCmdLine )
		{
			InCmdLine++;
		}
	}
	while( *InCmdLine && *InCmdLine!=' ' )
	{
		InCmdLine++;
	}
	if( *InCmdLine )
	{
		InCmdLine++;
	}
#endif
	appStrcpy( GCmdLine, InCmdLine );

	// Error history.
	appStrcpy( GErrorHist, TEXT("General protection fault!\r\n\r\nHistory: ") );

	// Platform specific pre-init.
	appPlatformPreInit();

	// Initialize file manager before making it default.
	InFileManager->Init(TRUE);
	GFileManager = InFileManager;
	// Switch into executable's directory.
	GFileManager->SetDefaultDirectory();

	// Output devices.
	GLogConsole	= InLogConsole;
	GError		= InError;
	GLog->AddOutputDevice( InLog );
	GLog->AddOutputDevice( InLogConsole );
	// Only create debug output device if a debugger is attached or we're running on a console.
	if( CONSOLE || appIsDebuggerPresent() )
	{
		GLog->AddOutputDevice( new FOutputDeviceDebug() );
	}

	// Feedback context.
	GWarn        = InWarn;
	if( ParseParam(appCmdLine(),TEXT("WARNINGSASERRORS")) == TRUE )
	{
		GWarn->TreatWarningsAsErrors = TRUE;
	}

	if( ParseParam(appCmdLine(),TEXT("UNATTENDED")) == TRUE )
	{
		GIsUnattended = TRUE;
	}

	if( ParseParam(appCmdLine(),TEXT("SILENT")) == TRUE )
	{
		GIsSilent = TRUE;
	}

#if ENABLE_SCRIPT_TRACING
	if ( ParseParam(appCmdLine(), TEXT("UTRACE")) )
	{
		GIsUTracing = TRUE;
		debugf(NAME_Init, TEXT("Script tracing features enabled."));
	}
#endif

	// Show log if wanted.
	if( GLogConsole && ParseParam(appCmdLine(),TEXT("LOG")) )
	{
		GLogConsole->Show( TRUE );
	}

	// Query whether this is an epic internal build or not.
	if( GFileManager->FileSize( TEXT("..") PATH_SEPARATOR TEXT("Binaries") PATH_SEPARATOR TEXT("EpicInternal.txt") ) >= 0 )
	{
		GIsEpicInternal = TRUE;
	}

	//// Command line.
	debugf( NAME_Init, TEXT("Version: %i"), GEngineVersion );
	debugf( NAME_Init, TEXT("Epic Internal: %i"), GIsEpicInternal );
	debugf( NAME_Init, TEXT("Compiled: %s %s"), ANSI_TO_TCHAR(__DATE__), ANSI_TO_TCHAR(__TIME__) );
	debugf( NAME_Init, TEXT("Command line: %s"), appCmdLine() );
	debugf( NAME_Init, TEXT("Base directory: %s"), appBaseDir() );
	debugf( NAME_Init, TEXT("Character set: %s"), sizeof(TCHAR)==1 ? TEXT("ANSI") : TEXT("Unicode") );

	//// Init config.
	GConfig = ConfigFactory();

#if CONSOLE
	// We only supported munged inis on console.
	if( GUseSeekFreeLoading )
	{
		// hardcode the global ini names (not allowing for command line overrides)
		appSprintf(GEngineIni, TEXT("%s%s%sEngine.ini"), *appGameConfigDir(), INI_PREFIX, GGameName);
		appSprintf(GGameIni, TEXT("%s%s%sGame.ini"), *appGameConfigDir(), INI_PREFIX, GGameName);
		appSprintf(GInputIni, TEXT("%s%s%sInput.ini"), *appGameConfigDir(), INI_PREFIX, GGameName);
		appSprintf(GUIIni, TEXT("%s%s%sUI.ini"), *appGameConfigDir(), INI_PREFIX, GGameName);

		// preload all needed ini files so that we don't seek around for them later
		GConfig->LoadCoalescedFile(*(appGameConfigDir() + COOKED_CONFIG_DIR));
	}
	else
#endif
	{
		UINT YesNoToAll = ART_No;
#if !CONSOLE
		appCreateIniNamesAndThenCheckForOutdatedness( GEditorIni, TEXT("DEFEDITORINI="), TEXT("EDITORINI="), TEXT("Editor.ini"), DEFAULT_INI_PREFIX, INI_PREFIX, YesNoToAll );
		appCreateIniNamesAndThenCheckForOutdatedness( GEditorUserSettingsIni, TEXT("DEFEDITORUSERSETTINGSINI="), TEXT("EDITORUSERSETTINGSINI="), TEXT("EditorUserSettings.ini"), DEFAULT_INI_PREFIX, INI_PREFIX, YesNoToAll );
#endif
		appCreateIniNamesAndThenCheckForOutdatedness( GEngineIni, TEXT("DEFENGINEINI="), TEXT("ENGINEINI="), TEXT("Engine.ini"), DEFAULT_INI_PREFIX, INI_PREFIX, YesNoToAll );
		appCreateIniNamesAndThenCheckForOutdatedness( GGameIni, TEXT("DEFGAMEINI="), TEXT("GAMEINI="), TEXT("Game.ini"), DEFAULT_INI_PREFIX, INI_PREFIX, YesNoToAll );
		appCreateIniNamesAndThenCheckForOutdatedness( GInputIni, TEXT("DEFINPUTINI="), TEXT("INPUTINI="), TEXT("Input.ini"), DEFAULT_INI_PREFIX, INI_PREFIX, YesNoToAll );
		appCreateIniNamesAndThenCheckForOutdatedness( GUIIni, TEXT("DEFUIINI="), TEXT("UIINI="), TEXT("UI.ini"), DEFAULT_INI_PREFIX, INI_PREFIX, YesNoToAll );
	}

	// okie so after the above has run we now have the REQUIRED set of engine .inis  (all of the other .inis)
	// that are gotten from .uc files' config() are not requires and are dynamically loaded when the .u files are loaded

	// if a logging build, clear out old log files
#if !NO_LOGGING && !FINAL_RELEASE
	appDeleteOldLogs();
#endif

	// Platform specific init.
	appPlatformInit();

	// Init sockets layer
	appSocketInit();

#if STATS
	// Initialize stats before objects because there are object stats
	GStatManager.Init();
#endif

	// Object initialization.
	UObject::StaticInit();

	// Memory initalization.
	GMem.Init( 65536 );

	// System initialization.
	USystem* DefaultSystemObject = USystem::StaticClass()->GetDefaultObject<USystem>();
	FArchive DummyAr;
	USystem::StaticClass()->Link(DummyAr,FALSE);
	DefaultSystemObject->LoadConfig(NULL,NULL,UE3::LCPF_ReadParentSections);

	if((GUseSeekFreeLoading && !CONSOLE) || (ParseParam(appCmdLine(), TEXT("cookededitor"))))
	{
		DefaultSystemObject->Paths		= DefaultSystemObject->SeekFreePCPaths;
		// clear out the script paths, as we won't be using them
		DefaultSystemObject->ScriptPaths.Empty();
		DefaultSystemObject->FRScriptPaths.Empty();
//		DefaultSystemObject->Extensions = DefaultSystemObject->SeekFreePCExtensions;
	}

	GSys = new USystem;
	GSys->AddToRoot();
	for( INT i=0; i<GSys->Suppress.Num(); i++ )
	{
		GSys->Suppress(i).SetFlags( RF_Suppress );
	}

	// Language.
	TCHAR CookerLanguage[8];
	if( Parse( appCmdLine(), TEXT("LANGUAGEFORCOOKING="), CookerLanguage, ARRAY_COUNT(CookerLanguage) ) == TRUE )
	{
		UObject::SetLanguage( CookerLanguage );
	}
	else
	{
		UObject::SetLanguage(*appGetLanguageExt());
	}

	if( GUseSeekFreeLoading && CONSOLE )
	{
		// preload all needed localization files so that we don't seek around for them later
		GConfig->LoadCoalescedFile(NULL);

		// make sure no more .ini or .int files are read/written after this point, since everything has been read in
		GConfig->DisableFileOperations();
	}

	// Cache file paths
	GPackageFileCache = new FMapPackageFileCache;
	GPackageFileCache->CachePaths();

	// do any post appInit processing
	appPlatformPostInit();
}

//
// Pre-shutdown.
// Called from within guarded exit code, only during non-error exits.
//
void appPreExit()
{
#if TRACK_SERIALIZATION_PERFORMANCE || LOOKING_FOR_PERF_ISSUES
	if ( GObjectSerializationPerfTracker != NULL )
	{
		delete GObjectSerializationPerfTracker;
		GObjectSerializationPerfTracker = NULL;
	}

	if ( GClassSerializationPerfTracker != NULL )
	{
		delete GClassSerializationPerfTracker;
		GClassSerializationPerfTracker = NULL;
	}
#endif

#if XBOX
#if ALLOW_NON_APPROVED_FOR_SHIPPING_LIB
	debugf( NAME_Exit, TEXT( "Rebooting console ...") );
	DmReboot( DMBOOT_COLD );
#endif
#endif

	debugf( NAME_Exit, TEXT("Preparing to exit.") );
	UObject::StaticExit();
	// Clean up the thread pool
	if (GThreadPool != NULL)
	{
		GThreadPool->Destroy();
	}
}

//
// Shutdown.
// Called outside guarded exit code, during all exits (including error exits).
//
void appExit()
{
	debugf( NAME_Exit, TEXT("Exiting.") );
	if( GConfig )
	{
		GConfig->Exit();
		delete GConfig;
		GConfig = NULL;
	}
	FName::StaticExit();
	GLog->TearDown();
	GLog = NULL;
}

/*-----------------------------------------------------------------------------
	USystem.
-----------------------------------------------------------------------------*/

IMPLEMENT_COMPARE_CONSTREF( FString, UnMisc, { return appStricmp(*A,*B); } );

#pragma warning (push)
#pragma warning (disable : 4717)
static void Recurse(UBOOL B)
{
	if(B)
		Recurse(B);
}
#pragma warning (pop)

USystem::USystem()
:	SavePath			( E_NoInit )
,	CachePath			( E_NoInit )
,	CacheExt			( E_NoInit )
,	ScreenShotPath		( E_NoInit )
,	Paths				( E_NoInit )
,	ScriptPaths			( E_NoInit )
,	FRScriptPaths		( E_NoInit )
,	CutdownPaths		( E_NoInit )
,	Suppress			( E_NoInit )
,	Extensions			( E_NoInit )
,	LocalizationPaths	( E_NoInit )
{}

void USystem::StaticConstructor()
{
	new(GetClass(),TEXT("StaleCacheDays"),			RF_Public)UIntProperty   (CPP_PROPERTY(StaleCacheDays		), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("MaxStaleCacheSize"),		RF_Public)UIntProperty   (CPP_PROPERTY(MaxStaleCacheSize	), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("MaxOverallCacheSize"),		RF_Public)UIntProperty   (CPP_PROPERTY(MaxOverallCacheSize	), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("AsyncIOBandwidthLimit"),	RF_Public)UFloatProperty (CPP_PROPERTY(AsyncIOBandwidthLimit), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("SavePath"),				RF_Public)UStrProperty   (CPP_PROPERTY(SavePath				), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("CachePath"),				RF_Public)UStrProperty   (CPP_PROPERTY(CachePath			), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("CacheExt"),				RF_Public)UStrProperty   (CPP_PROPERTY(CacheExt				), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("ScreenShotPath"),			RF_Public)UStrProperty   (CPP_PROPERTY(ScreenShotPath		), TEXT("Options"), CPF_Config );

	UArrayProperty* A = new(GetClass(),TEXT("Paths"),RF_Public)UArrayProperty( CPP_PROPERTY(Paths), TEXT("Options"), CPF_Config );
	A->Inner = new(A,TEXT("StrProperty0"),RF_Public)UStrProperty;

	UArrayProperty* ScriptPathArray = new(GetClass(),TEXT("ScriptPaths"),RF_Public)UArrayProperty( CPP_PROPERTY(ScriptPaths), TEXT("Options"), CPF_Config );
	ScriptPathArray->Inner = new(ScriptPathArray,TEXT("StrProperty0"),RF_Public)UStrProperty;

	UArrayProperty* FinalReleaseScriptPathArray = new(GetClass(),TEXT("FRScriptPaths"),RF_Public)UArrayProperty( CPP_PROPERTY(FRScriptPaths), TEXT("Options"), CPF_Config );
	FinalReleaseScriptPathArray->Inner = new(FinalReleaseScriptPathArray,TEXT("StrProperty0"),RF_Public)UStrProperty;

	UArrayProperty* D = new(GetClass(),TEXT("Suppress"),RF_Public)UArrayProperty( CPP_PROPERTY(Suppress), TEXT("Options"), CPF_Config );
	D->Inner = new(D,TEXT("NameProperty0"),RF_Public)UNameProperty;

	UArrayProperty* E = new(GetClass(),TEXT("Extensions"),RF_Public)UArrayProperty( CPP_PROPERTY(Extensions), TEXT("Options"), CPF_Config );
	E->Inner = new(E,TEXT("StrProperty0"),RF_Public)UStrProperty;

	UArrayProperty* F = new(GetClass(),TEXT("LocalizationPaths"),RF_Public)UArrayProperty( CPP_PROPERTY(LocalizationPaths), TEXT("Options"), CPF_Config );
	F->Inner = new(F,TEXT("StrProperty0"),RF_Public)UStrProperty;

	UArrayProperty* G = new(GetClass(),TEXT("CutdownPaths"),RF_Public)UArrayProperty( CPP_PROPERTY(CutdownPaths), TEXT("Options"), CPF_Config );
	G->Inner = new(G,TEXT("StrProperty0"),RF_Public)UStrProperty;

	UArrayProperty* H = new(GetClass(),TEXT("SeekFreePCExtensions"),RF_Public)UArrayProperty( CPP_PROPERTY(SeekFreePCExtensions), TEXT("Options"), CPF_Config );
	H->Inner = new(H,TEXT("StrProperty0"),RF_Public)UStrProperty;

	UArrayProperty* I = new(GetClass(),TEXT("SeekFreePCPaths"),RF_Public)UArrayProperty( CPP_PROPERTY(SeekFreePCPaths), TEXT("Options"), CPF_Config );
	I->Inner = new(I,TEXT("StrProperty0"),RF_Public)UStrProperty;
}

///////////////////////////////////////////////////////////////////////////////

/** DEBUG used for exe "DEBUG BUFFEROVERFLOW" */
static void BufferOverflowFunction(SIZE_T BufferSize, const ANSICHAR* Buffer) 
{
	ANSICHAR LocalBuffer[32];
	for( UINT i = 0; i < BufferSize; i++ ) 
	{
		LocalBuffer[i] = Buffer[i];
	}
	debugf(TEXT("BufferOverflowFunction BufferSize=%d LocalBuffer=%s"),BufferSize, ANSI_TO_TCHAR(LocalBuffer));
}

UBOOL USystem::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	if( ParseCommand(&Cmd,TEXT("CONFIGHASH")) )
	{
		GConfig->Dump( Ar );
		return TRUE;
	}
	else if ( ParseCommand(&Cmd,TEXT("CONFIGMEM")) )
	{
		GConfig->ShowMemoryUsage(Ar);
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("EXIT")) || ParseCommand(&Cmd,TEXT("QUIT")))
	{
		// Ignore these commands when running the editor
		if( !GIsEditor )
		{
			Ar.Log( TEXT("Closing by request") );
			appRequestExit( 0 );
		}
		return TRUE;
	}
	else if( ParseCommand( &Cmd, TEXT("DEBUG") ) )
	{
		if( ParseCommand(&Cmd,TEXT("CRASH")) )
		{
			appErrorf( TEXT("%s"), TEXT("Crashing at your request") );
			return TRUE;
		}
		else if( ParseCommand( &Cmd, TEXT("GPF") ) )
		{
			Ar.Log( TEXT("Crashing with voluntary GPF") );
			*(int *)NULL = 123;
			return TRUE;
		}
		else if( ParseCommand( &Cmd, TEXT("ASSERT") ) )
		{
			check(0);
			return TRUE;
		}
		else if( ParseCommand( &Cmd, TEXT("RESETLOADERS") ) )
		{
			UObject::ResetLoaders( NULL );
			return TRUE;
		}
		else if( ParseCommand( &Cmd, TEXT("BUFFEROVERRUN") ) )
		{
 			// stack overflow test - this case should be caught by /GS (Buffer Overflow Check) compile option
 			ANSICHAR SrcBuffer[] = "12345678901234567890123456789012345678901234567890";
			BufferOverflowFunction(ARRAY_COUNT(SrcBuffer),SrcBuffer);
			return TRUE;
		}
		else if( ParseCommand(&Cmd, TEXT("CRTINVALID")) )
		{
			FString::Printf(NULL);
			return TRUE;
		}
#if 0
		else if( ParseCommand( &Cmd, TEXT("RECURSE") ) )
		{
			Ar.Logf( TEXT("Recursing") );
			Recurse(1);
			return TRUE;
		}
		else if( ParseCommand( &Cmd, TEXT("EATMEM") ) )
		{
			Ar.Log( TEXT("Eating up all available memory") );
			while( 1 )
			{
				void* Eat = appMalloc(65536);
				memset( Eat, 0, 65536 );
			}
			return TRUE;
		}
#endif
		else return FALSE;
	}
	else if( ParseCommand(&Cmd,TEXT("DIR")) )		// DIR [path\pattern]
	{
		TArray<FString> Files;
		TArray<FString> Directories;

		GFileManager->FindFiles( Files, Cmd, 1, 0 );
		GFileManager->FindFiles( Directories, Cmd, 0, 1 );

		// Directories
		Sort<USE_COMPARE_CONSTREF(FString,UnMisc)>( &Directories(0), Directories.Num() );
		for( INT x = 0 ; x < Directories.Num() ; x++ )
		{
			Ar.Logf( TEXT("[%s]"), *Directories(x) );
		}

		// Files
		Sort<USE_COMPARE_CONSTREF(FString,UnMisc)>( &Files(0), Files.Num() );
		for( INT x = 0 ; x < Files.Num() ; x++ )
		{
			Ar.Logf( TEXT("[%s]"), *Files(x) );
		}

		return TRUE;
	}
	// Display information about the name table only rather than using HASH
	else if( ParseCommand( &Cmd, TEXT("NAMEHASH") ) )
	{
		FName::DisplayHash(Ar);
		return TRUE;
	}
#if TRACK_ARRAY_SLACK
	else if( ParseCommand( &Cmd, TEXT("DUMPSLACKTRACES") ) )
	{
		if( GSlackTracker )
		{
			GSlackTracker->DumpSlackTraces( 100, Ar );
		}
		return TRUE;
	}
	else if( ParseCommand( &Cmd, TEXT("RESETSLACKTRACKING") ) )
	{
		if( GSlackTracker )
		{
			GSlackTracker->ResetTracking();
		}
		return TRUE;
	}
	else if( ParseCommand( &Cmd, TEXT("TOGGLESLACKTRACKING") ) )
	{
		if( GSlackTracker )
		{
			GSlackTracker->ToggleTracking();
		}
		return TRUE;
	}
#endif // TRACK_ARRAY_SLACK
	// View the last N number of names added to the name table. Useful for
	// tracking down name table bloat
	else if( ParseCommand( &Cmd, TEXT("VIEWNAMES") ) )
	{
		INT NumNames = 0;
		if (Parse(Cmd,TEXT("NUM="),NumNames))
		{
			for (INT NameIndex = FName::GetMaxNames() - NumNames;
				NameIndex < FName::GetMaxNames(); NameIndex++)
			{
				Ar.Logf(TEXT("%d->%s"),NameIndex,*FName((EName)NameIndex).ToString());
			}
		}
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
}


/**
 * Performs periodic cleaning of the cache
 */
void USystem::PerformPeriodicCacheCleanup()
{
	// find any .tmp files in the cache directory
	TArray<FString> TempFiles; 
	GFileManager->FindFiles(TempFiles, *(CachePath * TEXT("*.tmp")), TRUE, FALSE);

	// and delete them
	for (INT FileIndex = 0; FileIndex < TempFiles.Num(); FileIndex++)
	{
		GFileManager->Delete(*(CachePath * TempFiles(FileIndex)));
	}

	// now clean old cache files until we get down to MaxStaleCacheSize (converting days to seconds)
	CleanCache(MaxStaleCacheSize * 1024 * 1024, StaleCacheDays * 60 * 60 * 24);
}

/**
 * Cleans out the cache as necessary to free the space needed for an incoming
 * incoming downloaded package 
 *
 * @param SpaceNeeded Amount of space needed to download a package
 */
void USystem::CleanCacheForNeededSpace(INT SpaceNeeded)
{
	// clean the cache until we get down the overall max size - SpaceNeeded
	CleanCache(Max(0, MaxOverallCacheSize * 1024 * 1024 - SpaceNeeded), 0);
}


// helper struct to contain cache information
struct FCacheInfo
{
	static INT Compare(const FCacheInfo& A, const FCacheInfo& B)
	{
		// we want oldest to newest, so return < 0 if A is older (bigger age)
		return (INT)(B.Age - A.Age);
	}

	/** File name, age and size */
	FString Name;
	DOUBLE Age;
	INT Size;
};

/**
 * Internal helper function for cleaning the cache
 *
 * @param MaxSize Max size the total of the cache can be (for files older than ExpirationSeconds)
 * @param ExpirationSeconds Only delete files older than this down to MaxSize
 */
void USystem::CleanCache(INT MaxSize, DOUBLE ExpirationSeconds)
{
	// first, find all of the cache files
	TArray<FString> CacheFiles;
	GFileManager->FindFiles(CacheFiles, *(GSys->CachePath * TEXT("*") + GSys->CacheExt), TRUE, FALSE);

	// build a full view of the cache
	TArray<FCacheInfo> Cache;
	INT TotalCacheSize = 0;
	for (INT FileIndex = 0; FileIndex < CacheFiles.Num(); FileIndex++)
	{
		// get how old the file is
		FString Filename = GSys->CachePath * CacheFiles(FileIndex);
		DOUBLE Age = GFileManager->GetFileAgeSeconds(*Filename);

		// we only care about files older than ExpirationSeconds
		if (Age > ExpirationSeconds)
		{
			// fill out a cacheinfo object
			FCacheInfo* CacheInfo = new(Cache) FCacheInfo;
			CacheInfo->Name = Filename;
			CacheInfo->Age = Age;
			CacheInfo->Size = GFileManager->FileSize(*CacheInfo->Name);

			// remember total size of cache
			TotalCacheSize += CacheInfo->Size;
		}
	}

	// sort by age, so older ones are first
	Sort<FCacheInfo, FCacheInfo>((FCacheInfo*)Cache.GetData(), Cache.Num());

	// now delete files until we get <= MaxSize
	INT DeleteIndex = 0;
	while (TotalCacheSize > MaxSize)
	{
		// delete the next file to be deleted
		FCacheInfo& CacheInfo = Cache(DeleteIndex++);
		GFileManager->Delete(*CacheInfo.Name);

		// update the total cache size
		TotalCacheSize -= CacheInfo.Size;

		debugf( TEXT("Purged file from cache: %s (Age: %.0f > %.0f) [Cache is now %d / %d]"), *CacheInfo.Name, CacheInfo.Age, ExpirationSeconds, TotalCacheSize, MaxSize);
	}
}


IMPLEMENT_CLASS(USystem);

/*-----------------------------------------------------------------------------
	Game Name.
-----------------------------------------------------------------------------*/

const TCHAR* appGetGameName()
{
	return GGameName;
}

/**
 * This will log out that an LazyArray::Load has occurred.  
 *
 * Recommended usage: 
 *
 * PC: The stack trace will SPAM SPAM SPAM out the callstack of what caused the load to occur.
 *
 * Consoles:  If you see lots of log entries then you should probably run the game in a debugger
 *            and set a breakpoint in this function or in UnTemplate.h  LazyArray::Load() so you can track 
 *            down the LazyArray::Load() calls.
 *
 * @todo:  make it so this does not spam when a level is being associated / loaded
 * NOTE:  right now this is VERY VERY spammy and a bit slow as the new stack walk code is slower in SP 2
 **/
void LogLazyArrayPerfIssue()
{
#if defined(LOG_LAZY_ARRAY_LOADS) || LOOKING_FOR_PERF_ISSUES

	debugf( NAME_PerfWarning, TEXT("A LazyArray::Load has been called") );

#if _MSC_VER && !CONSOLE
	const SIZE_T StackTraceSize = 65535;
	ANSICHAR* StackTrace = (ANSICHAR*) appSystemMalloc( StackTraceSize );
	StackTrace[0] = 0;
	// Walk the stack and dump it to the allocated memory.
	appStackWalkAndDump( StackTrace, StackTraceSize, 3 );
	debugf( NAME_Warning, ANSI_TO_TCHAR(StackTrace) );
	appSystemFree( StackTrace );

#endif // #fi stacktrace capability

#endif

}

/*-----------------------------------------------------------------------------
	FIOManager implementation
-----------------------------------------------------------------------------*/

/**
 * Constructor, associating self with GIOManager.
 */
FIOManager::FIOManager()
:	bIsLocked( FALSE )
{
	check( GIOManager==NULL );
	GIOManager = this;
}

/**
 * Destructor, removing association with GIOManager and deleting IO systems.
 */
FIOManager::~FIOManager()
{
	for( INT i=0; i<IOSystems.Num(); i++ )
	{
		delete IOSystems(i);
	}
	IOSystems.Empty();
	check( GIOManager==this );
	GIOManager = NULL;
}

/**
 * Locks the IO manager. This means all outstanding IO requests will be fulfilled
 * and subsequent requests will cause an assert till the manager has been unlocked
 * again. This is mainly to ensure that certain operations like saving a package
 * can rely on there not being any open file handles.
 */
void FIOManager::Lock()
{
	bIsLocked = TRUE;
	// Block till all IO sub systems are done and also flush their handles.
	for( INT i=0; i<IOSystems.Num(); i++ )
	{
		FIOSystem* IO = IOSystems(i);
		IO->BlockTillAllRequestsFinishedAndFlushHandles();
	}
}

/**
 * Removes the lock from the IO manager, allowing subsequent requests.
 */
void FIOManager::Unlock()
{
	bIsLocked = FALSE;
}

/**
 * Returns the IO system matching the passed in tag.
 *
 * @return	FIOSystem matching the passed in tag.
 */
FIOSystem* FIOManager::GetIOSystem( DWORD IOSystemTag )
{
	check(!bIsLocked);
	for( INT i=0; i<IOSystems.Num(); i++ )
	{
		FIOSystem* IO = IOSystems(i);
		// Return this system if tag matches.	
		if( IOSystemTag == IO->GetTag() )
		{
			return IO;
		}
	}
	return NULL;
}

/*-----------------------------------------------------------------------------
	Compression.
-----------------------------------------------------------------------------*/

#include "zlib.h"

DOUBLE GTotalUncompressTime = 0.0;

/**
 * Thread-safe abstract compression routine. Compresses memory from uncompressed buffer and writes it to compressed
 * buffer. Updates CompressedSize with size of compressed data.
 *
 * @param	CompressedBuffer			Buffer compressed data is going to be written to
 * @param	CompressedSize	[in/out]	Size of CompressedBuffer, at exit will be size of compressed data
 * @param	UncompressedBuffer			Buffer containing uncompressed data
 * @param	UncompressedSize			Size of uncompressed data in bytes
 * @return TRUE if compression succeeds, FALSE if it fails because CompressedBuffer was too small or other reasons
 */
UBOOL appCompressMemoryZLIB( void* CompressedBuffer, INT& CompressedSize, void* UncompressedBuffer, INT UncompressedSize )
{
	// Zlib wants to use unsigned long.
	unsigned long ZCompressedSize	= CompressedSize;
	unsigned long ZUncompressedSize	= UncompressedSize;
	// Compress data
	UBOOL bOperationSucceeded = compress( (BYTE*) CompressedBuffer, &ZCompressedSize, (BYTE*) UncompressedBuffer, ZUncompressedSize ) == Z_OK ? TRUE : FALSE;
	// Propagate compressed size from intermediate variable back into out variable.
	CompressedSize = ZCompressedSize;
	return bOperationSucceeded;
}

/**
 * Thread-safe abstract compression routine. Compresses memory from uncompressed buffer and writes it to compressed
 * buffer. Updates CompressedSize with size of compressed data.
 *
 * @param	UncompressedBuffer			Buffer containing uncompressed data
 * @param	UncompressedSize			Size of uncompressed data in bytes
 * @param	CompressedBuffer			Buffer compressed data is going to be written to
 * @param	CompressedSize				Size of CompressedBuffer data in bytes
 * @return TRUE if compression succeeds, FALSE if it fails because CompressedBuffer was too small or other reasons
 */
UBOOL appUncompressMemoryZLIB( void* UncompressedBuffer, INT UncompressedSize, void* CompressedBuffer, INT CompressedSize )
{
	// Zlib wants to use unsigned long.
	unsigned long ZCompressedSize	= CompressedSize;
	unsigned long ZUncompressedSize	= UncompressedSize;
	
	// Uncompress data.
	STAT(GTotalUncompressTime -= appSeconds());
	UBOOL bOperationSucceeded = uncompress( (BYTE*) UncompressedBuffer, &ZUncompressedSize, (BYTE*) CompressedBuffer, ZCompressedSize ) == Z_OK ? TRUE : FALSE;
	STAT(GTotalUncompressTime += appSeconds());

	// Sanity check to make sure we uncompressed as much data as we expected to.
	check( UncompressedSize == ZUncompressedSize );
	return bOperationSucceeded;
}

#if WITH_LZO
#include "lzo/lzoconf.h"
#include "lzo/lzopro/lzo1x.h"
#include "lzo/lzopro/lzo1y.h"
#include "lzo/lzo1f.h"

/**
 * Callback memory allocation function for the LZO*99* compressor to use
 * 
 * @param UserData	Points to the GLZOCallbacks structure
 * @param Items		Number of "items" to allocate
 * @param Size		Size of each "item" to allocate
 * 
 * @return A pointer to a block of memory Items * Size big
 */
static lzo_voidp __LZO_CDECL LZOMalloc(lzo_callback_p UserData, lzo_uint Items, lzo_uint Size)
{
    return appMalloc(Items * Size);
}

/**
 * Callback memory deallocation function for the LZO*99* compressor to use
 * 
 * @param UserData	Points to the GLZOCallbacks structure
 * @param Ptr		Pointer to memory to free
 */
static void __LZO_CDECL LZOFree(lzo_callback_p UserData, lzo_voidp Ptr)
{
    appFree(Ptr);
}

lzo_callback_t GLZOCallbacks = 
{
	LZOMalloc, // allocation routine
	LZOFree, // deallocation routine
	0, // progress callback
	NULL, // user pointer
	0, // user data
	0 // user data
};

// NOTE: The following will be all cleaned up when we decide on the final choice of compressors to use!
// each compression method needs a different amount of work mem
#define LZO_STANDARD_MEM		LZOPRO_LZO1X_1_14_MEM_COMPRESS
#define LZO_SPEED_MEM			LZOPRO_LZO1X_1_08_MEM_COMPRESS
//#define LZO_SIZE_MEM			LZOPRO_LZO1X_1_16_MEM_COMPRESS
#define LZO_SIZE_MEM			0

//#define LZO_STANDARD_MEM		LZOPRO_LZO1Y_1_14_MEM_COMPRESS
//#define LZO_SPEED_MEM			LZOPRO_LZO1Y_1_16_MEM_COMPRESS
//#define LZO_SIZE_MEM			0

//#define LZO_STANDARD_MEM		LZO1F_MEM_COMPRESS
//#define LZO_SPEED_MEM			LZO1F_MEM_COMPRESS
//#define LZO_SIZE_MEM			LZO1F_999_MEM_COMPRESS

// allocate to fit the biggest one
#define LZO_WORK_MEM_SIZE		(Max<INT>(Max<INT>(LZO_STANDARD_MEM, LZO_SPEED_MEM), LZO_SIZE_MEM))


// each compression function must be in the same family (lzo1x, etc) for decompression
#define LZO_STANDARD_COMPRESS	lzopro_lzo1x_1_14_compress
#define LZO_SPEED_COMPRESS		lzopro_lzo1x_1_08_compress
//#define LZO_SIZE_COMPRESS		lzopro_lzo1x_1_16_compress
#define LZO_SIZE_COMPRESS(in, in_len, out, out_len, wrkmem) lzopro_lzo1x_99_compress(in, in_len, out, out_len, &GLZOCallbacks, 10);
#if __WIN32__
#define LZO_DECOMPRESS			lzopro_lzo1x_decompress_safe
#else
#define LZO_DECOMPRESS			lzopro_lzo1x_decompress
#endif

// these were almost up to 1x, and sometimes even faster, but overall not quite as good
//#define LZO_STANDARD_COMPRESS	lzopro_lzo1y_1_14_compress
//#define LZO_SPEED_COMPRESS		lzopro_lzo1y_1_16_compress
////#define LZO_SIZE_COMPRESS		lzopro_lzo1y_1_16_compress
//#define LZO_SIZE_COMPRESS(in, in_len, out, out_len, wrkmem) lzopro_lzo1y_99_compress(in, in_len, out, out_len, &GLZOCallbacks, 10);
//#define LZO_DECOMPRESS			lzopro_lzo1y_decompress

// these were horrible
//#define LZO_STANDARD_COMPRESS	lzopro_lzo1f_1_compress
//#define LZO_SPEED_COMPRESS		lzo1f_1_compress
//#define LZO_SIZE_COMPRESS		lzo1f_999_compress
//#define LZO_DECOMPRESS			lzo1f_decompress

/**
 * Makes sure LZO has been initialized before first use
 */
inline void * GetLZOWorkMem()
{
	static void* LZOWorkMem = NULL;

	// only need to initialize once
	if( !LZOWorkMem )
	{
		// initialized lzo library
		verify( lzo_init() == LZO_E_OK );

		// allocate global work memory for LZO (never freed)
		// @todo: align this to the system's page boundary
		// lzo is faster if this is aligned
		LZOWorkMem = appMalloc( LZO_WORK_MEM_SIZE );
	}

	appMemzero( LZOWorkMem, LZO_WORK_MEM_SIZE );

	// return the allocated work buffer
	return( LZOWorkMem );
}

/**
* Allocate a buffer to copy the data to compress into
*/
#define MAX_UNCOMPRESSED_SIZE	( 128 * 1024 )

BYTE * GetSourceBuffer( INT DataSize )
{
	check( DataSize > 0 );

	static BYTE * SourceBuffer = NULL;
	static INT SourceBufferSize = MAX_UNCOMPRESSED_SIZE + LZO_WORK_MEM_SIZE;

	// only need to initialize once
	if( !SourceBuffer || DataSize + LZO_WORK_MEM_SIZE > SourceBufferSize )
	{
		if( SourceBuffer )
		{
			appFree( SourceBuffer );
		}
		SourceBuffer = ( BYTE * )appMalloc( DataSize + LZO_WORK_MEM_SIZE );
		SourceBufferSize = DataSize + LZO_WORK_MEM_SIZE;

		appMemzero( SourceBuffer, SourceBufferSize );
	}

	check( SourceBuffer );

	// return the allocated work buffer
	return( SourceBuffer );
}

/**
* Allocate a buffer to compress the data into
*/
BYTE * GetCompressBuffer( INT DataSize )
{
	check( DataSize > 0 );

	static BYTE * CompressedBuffer = NULL;
	static INT CompressedBufferSize = MAX_UNCOMPRESSED_SIZE + LZO_WORK_MEM_SIZE;

	// only need to initialize once
	if( !CompressedBuffer || DataSize + LZO_WORK_MEM_SIZE > CompressedBufferSize )
	{
		if( CompressedBuffer )
		{
			appFree( CompressedBuffer );
		}
		CompressedBuffer = ( BYTE * )appMalloc( DataSize + LZO_WORK_MEM_SIZE );
		CompressedBufferSize = DataSize + LZO_WORK_MEM_SIZE;
	}

	check( CompressedBuffer );

	// return the allocated work buffer
	return( CompressedBuffer );
}

/**
 * Thread-safe abstract compression routine. Compresses memory from uncompressed buffer and writes it to compressed
 * buffer. Updates CompressedSize with size of compressed data.
 *
 * @param	Flags						Flags to optionally control memory vs speed
 * @param	CompressedBuffer			Buffer compressed data is going to be written to
 * @param	CompressedSize	[in/out]	Size of CompressedBuffer, at exit will be size of compressed data
 * @param	UncompressedBuffer			Buffer containing uncompressed data
 * @param	UncompressedSize			Size of uncompressed data in bytes
 * @return TRUE if compression succeeds, FALSE if it fails because CompressedBuffer was too small or other reasons
 */
UBOOL appCompressMemoryLZO( ECompressionFlags Flags, void* CompressedBuffer, INT& CompressedSize, void* UncompressedBuffer, INT UncompressedSize )
{
	check( UncompressedSize <= MAX_UNCOMPRESSED_SIZE );

	// we use a temporary buffer here that is bigger than the uncompressed size. this
	// is because lzo expects CompressedSize to be bigger than UncompressedSize, in
	// case the data is incompressible. Since there is no guarantee that CompressedSize
	// is big enough, we can't use it, because lzo will happily overwrite past the end
	// of CompressedBuffer (since it doesn't take as input the size of CompressedBuffer)
	BYTE * CompressScratchBuffer = GetCompressBuffer( UncompressedSize );

	// lzo reads past the end of the source data, so we need to ensure that data is always the same.
	BYTE * SourceData = GetSourceBuffer( UncompressedSize );
	appMemcpy( SourceData, UncompressedBuffer, UncompressedSize );

	// out variable for how big the compressed data actually is
	lzo_uint FinalCompressedSize;
	// attempt to compress the data 
	INT Result = LZO_E_OK;
	if( Flags & COMPRESS_BiasSpeed )
	{
		Result = LZO_SPEED_COMPRESS( SourceData, UncompressedSize, CompressScratchBuffer, &FinalCompressedSize, GetLZOWorkMem() );
	}
	else if( Flags & COMPRESS_BiasMemory )
	{
		Result = LZO_SIZE_COMPRESS( SourceData, UncompressedSize, CompressScratchBuffer, &FinalCompressedSize, GetLZOWorkMem() );
	}
	else
	{
		Result = LZO_STANDARD_COMPRESS( SourceData, UncompressedSize, CompressScratchBuffer, &FinalCompressedSize, GetLZOWorkMem() );
	}

	// Make sure everything is zero the next time we come in
	appMemzero( SourceData, UncompressedSize );

	// this shouldn't ever fail, apparently unless something catastrophic happened
	// but the docs are really not clear, because there are no docs
	check(Result == LZO_E_OK);

	// by default we succeeded (ie fit into available memory)
	UBOOL Return = TRUE;

	// if the compressed size will fit in the CompressedBuffer, copy it in
	if( FinalCompressedSize <= ( DWORD )CompressedSize )
	{
		// copy the data
		appMemcpy( CompressedBuffer, CompressScratchBuffer, FinalCompressedSize );
	}
	else
	{
		// if it doesn't fit, then this function has failed
		Return = FALSE;
	}

	// if this compression succeeded or failed, return how big it compressed it to
	// this way, on a failure, it can be called again with a big enough buffer
	CompressedSize = FinalCompressedSize;

	// return our success/failure
	return( Return );
}

/**
 * Thread-safe abstract compression routine. Compresses memory from uncompressed buffer and writes it to compressed
 * buffer. Updates CompressedSize with size of compressed data.
 *
 * @param	UncompressedBuffer			Buffer containing uncompressed data
 * @param	UncompressedSize			Size of uncompressed data in bytes
 * @param	CompressedBuffer			Buffer compressed data is going to be written to
 * @param	CompressedSize				Size of CompressedBuffer data in bytes
 * @return TRUE if compression succeeds, FALSE if it fails because CompressedBuffer was too small or other reasons
 */
UBOOL appUncompressMemoryLZO( void* UncompressedBuffer, INT UncompressedSize, void* CompressedBuffer, INT CompressedSize )
{
	// lzo wants unsigned
	lzo_uint FinalUncompressedSize;
	STAT(GTotalUncompressTime -= appSeconds());
	INT Result = LZO_DECOMPRESS((BYTE*)CompressedBuffer, CompressedSize, (BYTE*)UncompressedBuffer, &FinalUncompressedSize, GetLZOWorkMem());
	STAT(GTotalUncompressTime += appSeconds());
	
	// assign the output size (even if we fail, we can try again with an appropriate sized buffer)
	UncompressedSize = FinalUncompressedSize;

	// if the call failed, return FALSE
	if (Result != LZO_E_OK)
	{
		return FALSE;
	}

	// success
	return TRUE;
}
#endif	//#if WITH_LZO

/**
 * Thread-safe abstract compression routine. Compresses memory from uncompressed buffer and writes it to compressed
 * buffer. Updates CompressedSize with size of compressed data. Compression controlled by the passed in flags.
 *
 * @param	Flags						Flags to control what method to use and optionally control memory vs speed
 * @param	CompressedBuffer			Buffer compressed data is going to be written to
 * @param	CompressedSize	[in/out]	Size of CompressedBuffer, at exit will be size of compressed data
 * @param	UncompressedBuffer			Buffer containing uncompressed data
 * @param	UncompressedSize			Size of uncompressed data in bytes
 * @return TRUE if compression succeeds, FALSE if it fails because CompressedBuffer was too small or other reasons
 */
UBOOL appCompressMemory( ECompressionFlags Flags, void* CompressedBuffer, INT& CompressedSize, void* UncompressedBuffer, INT UncompressedSize )
{
	// make sure a valid compression scheme was provided
	check(Flags & (COMPRESS_ZLIB | COMPRESS_LZO));

	// Always bias for speed if option is set.
	if( GAlwaysBiasCompressionForSize )
	{
		INT NewFlags = Flags;
		NewFlags &= ~COMPRESS_BiasSpeed;
		NewFlags |= COMPRESS_BiasMemory;
		Flags = (ECompressionFlags) NewFlags;
	}

	if( Flags & COMPRESS_ZLIB )
	{
		return appCompressMemoryZLIB(CompressedBuffer, CompressedSize, UncompressedBuffer, UncompressedSize);
	}
	else
	{
#if WITH_LZO
		return appCompressMemoryLZO(Flags, CompressedBuffer, CompressedSize, UncompressedBuffer, UncompressedSize);
#else	//#if WITH_LZO
		warnf(TEXT("Compressing with LZO support is diabled"));
		return FALSE;
#endif	//#if WITH_LZO
	}
}

/**
 * Thread-safe abstract decompression routine. Uncompresses memory from compressed buffer and writes it to uncompressed
 * buffer. UncompressedSize is expected to be the exact size of the data after decompression.
 *
 * @param	Flags						Flags to control what method to use to decompress
 * @param	UncompressedBuffer			Buffer containing uncompressed data
 * @param	UncompressedSize			Size of uncompressed data in bytes
 * @param	CompressedBuffer			Buffer compressed data is going to be written to
 * @param	CompressedSize				Size of CompressedBuffer data in bytes
 * @return TRUE if compression succeeds, FALSE if it fails because CompressedBuffer was too small or other reasons
 */
UBOOL appUncompressMemory( ECompressionFlags Flags, void* UncompressedBuffer, INT UncompressedSize, void* CompressedBuffer, INT CompressedSize )
{
	// make sure a valid compression scheme was provided
	check(Flags & (COMPRESS_ZLIB | COMPRESS_LZO));

#if WITH_LZO
	return 
		(Flags & COMPRESS_ZLIB) ?
		appUncompressMemoryZLIB(UncompressedBuffer, UncompressedSize, CompressedBuffer, CompressedSize) :
		appUncompressMemoryLZO(UncompressedBuffer, UncompressedSize, CompressedBuffer, CompressedSize);
#else	//#if WITH_LZO
	if ((Flags & COMPRESS_LZO) != 0)
	{
		warnf(TEXT("appUncompressMemory with LZO"));
		return FALSE;
	}
	return appUncompressMemoryZLIB(UncompressedBuffer, UncompressedSize, CompressedBuffer, CompressedSize);
#endif	//#if WITH_LZO
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

