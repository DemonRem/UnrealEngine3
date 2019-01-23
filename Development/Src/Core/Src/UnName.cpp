/*=============================================================================
	UnName.cpp: Unreal global name code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	FName helpers.
-----------------------------------------------------------------------------*/

/**
* Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Class->Name.Index)". 
*
* @param	Index	Name index to look up string for
* @return			Associated name
*/
const TCHAR* DebugFName(INT Index)
{
	// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
	static TCHAR TempName[256];
	appStrcpy(TempName, *FName::SafeString((EName)Index));
	return TempName;
}

/**
* Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Class->Name.Index, Class->Name.Number)". 
*
* @param	Index	Name index to look up string for
* @param	Number	Internal instance number of the FName to print (which is 1 more than the printed number)
* @return			Associated name
*/
const TCHAR* DebugFName(INT Index, INT Number)
{
	// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
	static TCHAR TempName[256];
	appStrcpy(TempName, *FName::SafeString((EName)Index, Number));
	return TempName;
}

/**
 * Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Class->Name)". 
 *
 * @param	Name	Name to look up string for
 * @return			Associated name
 */
const TCHAR* DebugFName(FName& Name)
{
	// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
	static TCHAR TempName[256];
	appStrcpy(TempName, *FName::SafeString((EName)Name.Index, Name.Number));
	return TempName;
}

/**
 * Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Class)". 
 *
 * @param	Object	Object to look up the name for 
 * @return			Associated name
 */
const TCHAR* DebugFName(UObject* Object)
{
	// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
	static TCHAR TempName[256];
	appStrcpy(TempName,Object ? *FName::SafeString((EName)Object->Name.Index, Object->Name.Number) : TEXT("NULL"));
	return TempName;
}

/**
 * Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Object)". 
 *
 * @param	Object	Object to look up the name for 
 * @return			Fully qualified path name
 */
const TCHAR* DebugPathName(UObject* Object)
{
	if( Object )
	{
		// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
		static TCHAR PathName[1024];
		PathName[0] = 0;

		// Keep track of how many outers we have as we need to print them in inverse order.
		UObject*	TempObject = Object;
		INT			OuterCount = 0;
		while( TempObject )
		{
			TempObject = TempObject->GetOuter();
			OuterCount++;
		}

		// Iterate over each outer + self in reverse oder and append name.
		for( INT OuterIndex=OuterCount-1; OuterIndex>=0; OuterIndex-- )
		{
			// Move to outer name.
			TempObject = Object;
			for( INT i=0; i<OuterIndex; i++ )
			{
				TempObject = TempObject->GetOuter();
			}

			// Dot separate entries.
			if( OuterIndex != OuterCount -1 )
			{
				appStrcat( PathName, TEXT(".") );
			}
			// And app end the name.
			appStrcat( PathName, DebugFName( TempObject ) );
		}

		return PathName;
	}
	else
	{
		return TEXT("None");
	}
}

/**
 * Helper function that can be used inside the debuggers watch window. E.g. "DebugFName(Object)". 
 *
 * @param	Object	Object to look up the name for 
 * @return			Fully qualified path name prepended by class name
 */
const TCHAR* DebugFullName(UObject* Object)
{
	if( Object )
	{
		// Hardcoded static array. This function is only used inside the debugger so it should be fine to return it.
    	static TCHAR FullName[1024];
		FullName[0]=0;

		// Class Full.Path.Name
		appStrcat( FullName, DebugFName(Object->GetClass()) );
		appStrcat( FullName, TEXT(" "));
		appStrcat( FullName, DebugPathName(Object) );

		return FullName;
	}
	else
	{
		return TEXT("None");
	}
}


/*-----------------------------------------------------------------------------
	FName statics.
-----------------------------------------------------------------------------*/

// Static variables.
FNameEntry*					FName::NameHash[4096];
TArrayNoInit<FNameEntry*>	FName::Names;
INT							FName::NameEntryMemorySize;


/*-----------------------------------------------------------------------------
	FName implementation.
-----------------------------------------------------------------------------*/

//
// Hardcode a name.
//
void FName::Hardcode(FNameEntry* AutoName)
{
	// Add name to name hash.
	INT iHash          = appStrihash(AutoName->Name) & (ARRAY_COUNT(NameHash)-1);
	AutoName->HashNext = NameHash[iHash];
	NameHash[iHash]    = AutoName;

	// Expand the table if needed.
	for( INT i=Names.Num(); i<=AutoName->Index; i++ )
	{
		Names.AddItem( NULL );
	}

	// Add name to table.
	if( Names(AutoName->Index) )
	{
		appErrorf( TEXT("Hardcoded name '%s' at index %i was duplicated. Existing entry is '%'."), AutoName->Name, AutoName->Index, Names(AutoName->Index)->Name );
	}
	Names(AutoName->Index) = AutoName;
}

/**
 * Create an FName. If FindType is FNAME_Find, and the string part of the name 
 * doesn't already exist, then the name will be NAME_None
 *
 * @param Name Value for the string portion of the name
 * @param FindType Action to take (see EFindName)
 */
FName::FName( const TCHAR* Name, EFindName FindType, UBOOL )
{
	Init(Name, NAME_NO_NUMBER_INTERNAL, FindType);
}

/**
 * Create an FName. If FindType is FNAME_Find, and the string part of the name 
 * doesn't already exist, then the name will be NAME_None
 *
 * @param Name Value for the string portion of the name
 * @param Number Value for the number portion of the name
 * @param FindType Action to take (see EFindName)
 */
FName::FName( const TCHAR* Name, INT Number, EFindName FindType )
{
	Init(Name, Number, FindType);
}


/**
 * Constructor used by ULinkerLoad when loading its name table; Creates an FName with an instance
 * number of 0 that does not attempt to split the FName into string and number portions.
 */
FName::FName( ELinkerNameTableConstructor, const TCHAR* Name )
{
	Init(Name, NAME_NO_NUMBER_INTERNAL, FNAME_Add, FALSE);
}


/**
 * Shared initialization code (between two constructors)
 * 
 * @param InName		String name of the name/number pair
 * @param InNumber		Number part of the name/number pair
 * @param FindType		Operation to perform on names
 * @param bSplitName	If TRUE, the constructor will attempt to split a number off of the string portion (turning Rocket_17 to Rocket and number 17)
 */
void FName::Init(const TCHAR* InName, INT InNumber, EFindName FindType, UBOOL bSplitName/*=TRUE*/ )
{
	// initialiuze the name subsystem if necessary
	if (!GetIsInitialized())
	{
		StaticInit();
	}

	TCHAR TempBuffer[NAME_SIZE];
	INT TempNumber;
	// if we were passed in a number, we can't split again, other wise, a_1_2_3_4 would change everytime
	// it was loaded in
	if (InNumber == NAME_NO_NUMBER_INTERNAL && bSplitName == TRUE )
	{
		if (SplitNameWithCheck(InName, TempBuffer, ARRAY_COUNT(TempBuffer), TempNumber))
		{
			InName = TempBuffer;
			InNumber = NAME_EXTERNAL_TO_INTERNAL(TempNumber);
		}
	}

	check(InName);

	// If empty or invalid name was specified, return NAME_None.
	if( !InName[0] )
	{
		Index = NAME_None;
		Number = NAME_NO_NUMBER_INTERNAL;
		return;
	}

	// set the number
	Number = InNumber;

	// Try to find the name in the hash.
	INT iHash = appStrihash(InName) & (ARRAY_COUNT(NameHash)-1);
	for( FNameEntry* Hash=NameHash[iHash]; Hash; Hash=Hash->HashNext )
	{
		if( appStricmp( InName, Hash->Name )==0 )
		{
			// Found it in the hash.
			Index = Hash->Index;

			// Check to see if the caller wants to replace the contents of the
			// FName with the specified value. This is useful for compiling
			// script classes where the file name is lower case but the class
			// was intended to be uppercase.
			if (FindType == FNAME_Replace)
			{
				// This should be impossible due to the compare above
				// This *must* be true, or we'll overwrite memory when the
				// copy happens if it is longer
				checkSlow(appStrlen(InName) == appStrlen(Hash->Name));
				// Can't rely on the template override for static arrays since the safe crt version of strcpy will fill in
				// the remainder of the array of NAME_SIZE with 0xfd.  So, we have to pass in the length of the dynamically allocated array instead.
				appStrcpy(Hash->Name,appStrlen(Hash->Name)+1,InName);
			}

			return;
		}
	}

	// Didn't find name.
	if( FindType==FNAME_Find )
	{
		// Not found.
		Index = NAME_None;
		Number = NAME_NO_NUMBER_INTERNAL;
		return;
	}

	// Add new entry.
	Index = Names.Add();

	// Allocate and set the name.
	Names(Index) = NameHash[iHash] = AllocateNameEntry( InName, Index, 0, NameHash[iHash] );
}


/**
 * Converts an FName to a readable format
 *
 * @return String representation of the name
 */
FString FName::ToString() const
{
	checkName(Index < Names.Num());
	checkName(Names(Index));
	if (Number != NAME_NO_NUMBER_INTERNAL)
	{
		return FString(Names(Index)->Name) + TEXT("_") + appItoa(NAME_INTERNAL_TO_EXTERNAL(Number));
	}
	else
	{
		return FString(Names(Index)->Name);
	}
}

/**
 * Converts an FName to a readable format, in place
 * 
 * @param Out String to fill ot with the string representation of the name
 */
void FName::ToString(FString& Out) const
{
	// a version of ToString that saves at least one string copy

	checkName(Index < Names.Num());
	checkName(Names(Index));
	if (Number != NAME_NO_NUMBER_INTERNAL)
	{
		Out = FString(Names(Index)->Name) + TEXT("_") + appItoa(NAME_INTERNAL_TO_EXTERNAL(Number));
	}
	else
	{
		Out = FString(Names(Index)->Name);
	}
}

INT FName::GetNumber() const
{
	return Number;
}


/*-----------------------------------------------------------------------------
	CRC functions. (MOVED FROM UNMISC.CPP FOR MASSIVE WORKAROUND @todo put back!
-----------------------------------------------------------------------------*/
// CRC 32 polynomial.
#define CRC32_POLY 0x04c11db7

/** 
* Helper class for initializing the global GCRCTable
*/
class FCRCTableInit
{
public:
	/**
	* Constructor
	*/
	FCRCTableInit()
	{
		// Init CRC table.
		for( DWORD iCRC=0; iCRC<256; iCRC++ )
		{
			for( DWORD c=iCRC<<24, j=8; j!=0; j-- )
			{
				GCRCTable[iCRC] = c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
			}
		}
	}	
};
DWORD GCRCTable[256];


/*-----------------------------------------------------------------------------
	FName subsystem.
-----------------------------------------------------------------------------*/

//
// Initialize the name subsystem.
//
void FName::StaticInit()
{
	/** Global instance used to initialize the GCRCTable. It used to be initialized in appInit() */
	//@todo: Massive workaround for static init order without needing to use a function call for every use of GCRCTable
	// This ASSUMES that fname::StaticINit is going to be called BEFORE ANY use of GCRCTable
	static FCRCTableInit GCRCTableInit;

	check(GetIsInitialized() == FALSE);
	check((ARRAY_COUNT(NameHash)&(ARRAY_COUNT(NameHash)-1)) == 0);
	GetIsInitialized() = 1;

	// initialize the TArrayNoInit() on first use, NOT when the constructor is called (as it could happen
	// AFTER this function)
	appMemzero(&FName::Names, sizeof(FName::Names));

	// Init the name hash.
	for (INT HashIndex = 0; HashIndex < ARRAY_COUNT(FName::NameHash); HashIndex++)
	{
		NameHash[HashIndex] = NULL;
	}

	// Register all hardcoded names.
	#define REGISTER_NAME(num,namestr) Hardcode(AllocateNameEntry(TEXT(#namestr),num,0,NULL));
	#include "UnNames.h"

#if DO_CHECK
	// Verify no duplicate names.
	for (INT HashIndex = 0; HashIndex < ARRAY_COUNT(NameHash); HashIndex++)
	{
		for (FNameEntry* Hash = NameHash[HashIndex]; Hash; Hash = Hash->HashNext)
		{
			for (FNameEntry* Other = Hash->HashNext; Other; Other = Other->HashNext)
			{
				if (appStricmp(Hash->Name, Other->Name) == 0)
				{
					// we can't print out here because there may be no log yet if thi shappens before main starts
					appDebugBreak();
				}
			}
		}
	}
#endif
}

//
// Shut down the name subsystem.
//
void FName::StaticExit()
{
	check(GetIsInitialized());

	debugf( NAME_Exit, TEXT("Name subsystem shutting down") );

	// Kill all names.
	for( INT i=0; i<Names.Num(); i++ )
	{
		if( Names(i) )
		{
			delete Names(i);
		}
	}

	// Empty tables.
	Names.Empty();
	GetIsInitialized() = 0;
}

//
// Display the contents of the global name hash.
//
void FName::DisplayHash( FOutputDevice& Ar )
{
	INT UsedBins=0, NameCount=0, MemUsed = 0;
	for( INT i=0; i<ARRAY_COUNT(NameHash); i++ )
	{
		if( NameHash[i] != NULL ) UsedBins++;
		for( FNameEntry *Hash = NameHash[i]; Hash; Hash=Hash->HashNext )
		{
			NameCount++;
			// Count how much memory this entry is using
			MemUsed += sizeof(FNameEntry) - ((NAME_SIZE - appStrlen(Hash->Name) + 1) * sizeof(TCHAR));
		}
	}
	Ar.Logf( TEXT("Hash: %i names, %i/%i hash bins, Mem in bytes %i"), NameCount, UsedBins, ARRAY_COUNT(NameHash), MemUsed);
}

/**
 * Helper function to split an old-style name (Class_Number, ie Rocket_17) into
 * the component parts usable by new-style FNames
 *
 * @param OldName		Old-style name
 * @param NewName		Ouput string portion of the name/number pair
 * @param NewNumber		Number portion of the name/number pair
 */
void FName::SplitOldName(const TCHAR* OldName, FString& NewName, INT& NewNumber)
{
	TCHAR Temp[NAME_SIZE];
	// try to split the name
	if (SplitNameWithCheck(OldName, Temp, NAME_SIZE, NewNumber))
	{
		// if the split succeeded, copy the temp buffer to the string
		NewName = Temp;
	}
	else
	{
		// otherwise, just copy the old name to new name, with no number
		NewName = OldName;
		NewNumber = NAME_NO_NUMBER;
	}
}

/**
 * Helper function to split an old-style name (Class_Number, ie Rocket_17) into
 * the component parts usable by new-style FNames. Only use results if this function
 * returns TRUE.
 *
 * @param OldName		Old-style name
 * @param NewName		Ouput string portion of the name/number pair
 * @param NewNameLen	Size of NewName buffer (in TCHAR units)
 * @param NewNumber		Number portion of the name/number pair
 *
 * @return TRUE if the name was split, only then will NewName/NewNumber have valid values
 */
UBOOL FName::SplitNameWithCheck(const TCHAR* OldName, TCHAR* NewName, INT NewNameLen, INT& NewNumber)
{
	UBOOL bSucceeded = FALSE;

	// get string length
	const TCHAR* LastChar = OldName + (appStrlen(OldName) - 1);
	
	// if the last char is a number, then we will try to split
	const TCHAR* Ch = LastChar;
	if (*Ch >= '0' && *Ch <= '9')
	{
		// go backwards, looking an underscore or the start of the string
		// (we don't look at first char because '_9' won't split well)
		while (*Ch >= '0' && *Ch <= '9' && Ch > OldName)
		{
			Ch--;
		}

		// if the first non-number was an underscore (as opposed to a letter),
		// we can split
		if (*Ch == '_')
		{
			// check for the case where there are multiple digits after the _ and the first one
			// is a 0 ("Rocket_04"). Can't split this case. (So, we check if the first char
			// is not 0 or the length of the number is 1 (since ROcket_0 is valid)
			if (Ch[1] != '0' || LastChar - Ch == 1)
			{
				// attempt to convert what's following it to a number
				NewNumber = appAtoi(Ch + 1);

				// copy the name portion into the buffer
				appStrncpy(NewName, OldName, Min(Ch - OldName + 1, NewNameLen));

				// mark successful
				bSucceeded = TRUE;
			}
		}
	}

	// return success code
	return bSucceeded;
}


/*-----------------------------------------------------------------------------
	FNameEntry implementation.
-----------------------------------------------------------------------------*/

FArchive& operator<<( FArchive& Ar, FNameEntry& E )
{
	if( Ar.IsLoading() )
	{
		FString Str;
		Ar << Str;
		// Can't rely on the template override for static arrays since the safe crt version of strcpy will fill in
		// the remainder of the array of NAME_SIZE with 0xfd.  So, we have to pass in the length of the dynamically allocated array instead.
		appStrcpy( E.Name, Str.Len()+1, *Str.Left(NAME_SIZE-1) );
	}
	else
	{
		FString Str(E.Name);
		Ar << Str;
	}
	Ar << E.Flags;
	return Ar;
}

FNameEntry* AllocateNameEntry( const TCHAR* Name, DWORD Index, EObjectFlags Flags, FNameEntry* HashNext )
{
	const SIZE_T NameLen = appStrlen(Name);
	INT NameEntrySize	  = sizeof(FNameEntry) - (NAME_SIZE - NameLen - 1)*sizeof(TCHAR);
	FNameEntry* NameEntry = (FNameEntry*)appMalloc( NameEntrySize );
	STAT( FName::NameEntryMemorySize += NameEntrySize );
	NameEntry->Index      = Index;
	NameEntry->Flags      = Flags;
	NameEntry->HashNext   = HashNext;
	// Can't rely on the template override for static arrays since the safe crt version of strcpy will fill in
	// the remainder of the array of NAME_SIZE with 0xfd.  So, we have to pass in the length of the dynamically allocated array instead.
	appStrcpy( NameEntry->Name, NameLen + 1, Name );
	return NameEntry;
}


