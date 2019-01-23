/*=============================================================================
	UnName.h: Unreal global name types.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*----------------------------------------------------------------------------
	Definitions.
----------------------------------------------------------------------------*/

/** Maximum size of name. */
enum {NAME_SIZE	= 128};

/** Name index. */
typedef INT NAME_INDEX;

/** Externally, the instance number to represent no instance number is NAME_NO_NUMBER, 
    but internally, we add 1 to indices, so we use this #define internally for 
	zero'd memory initialization will still make NAME_None as expected */
#define NAME_NO_NUMBER_INTERNAL	0

/** Conversion routines between external representations and internal */
#define NAME_INTERNAL_TO_EXTERNAL(x) (x - 1)
#define NAME_EXTERNAL_TO_INTERNAL(x) (x + 1)

/** Special value for an FName with no number */
#define NAME_NO_NUMBER NAME_INTERNAL_TO_EXTERNAL(NAME_NO_NUMBER_INTERNAL)



/** Enumeration for finding name. */
enum EFindName
{
	/** Find a name; return 0 if it doesn't exist. */
	FNAME_Find,

	/** Find a name or add it if it doesn't exist. */
	FNAME_Add,

	/** Finds a name and replaces it. Adds it if missing */
	FNAME_Replace,
};

typedef QWORD EObjectFlags;	// @warning: mirrored in UnObjBas.h

/*----------------------------------------------------------------------------
	FNameEntry.
----------------------------------------------------------------------------*/

/**
 * A global name, as stored in the global name table.
 */
struct FNameEntry
{
	/** Index of name in hash. */
	NAME_INDEX		Index;

	/** RF_TagImp, RF_TagExp */
	EObjectFlags	Flags;

	/** Pointer to the next entry in this hash bin's linked list. */
	FNameEntry*		HashNext;

	/** Name, variable-sized - note that AllocateNameEntry only allocates memory as needed. */
	TCHAR			Name[NAME_SIZE];

	/**
	 * Used to safely check whether any of the passed in flags are set. This is required
	 * as EObjectFlags currently is a 64 bit data type and UBOOL is a 32 bit data type so
	 * simply using GetFlags() & RF_MyFlagBiggerThanMaxInt won't work correctly when 
	 * assigned directly to an UBOOL.
	 *
	 * @param	FlagsToCheck	Object flags to check for.
	 * @return					TRUE if any of the passed in flags are set, FALSE otherwise  (including no flags passed in).
	 */
	UBOOL HasAnyFlags( EObjectFlags FlagsToCheck ) const
	{
		return (Flags & FlagsToCheck) != 0;
	}
	/**
	 * Sets the passed in flags on the name entry.
	 *
	 * @param	FlagsToSet		Flags to set.
	 */
	void SetFlags( EObjectFlags Set )
	{
		// make sure we only set the expected tags on here
		Flags |= Set;
	}
	/**
	 * Clears the passed in flags on the name entry.
	 *
	 * @param	FlagsToClear	Flags to clear.
	 */
	void ClearFlags( EObjectFlags Clear )
	{
		Flags &= ~Clear;
	}

	// Functions.
	friend FArchive& operator<<( FArchive& Ar, FNameEntry& E );
	friend FNameEntry* AllocateNameEntry( const TCHAR* Name, DWORD Index, EObjectFlags Flags, FNameEntry* HashNext );
};

/*----------------------------------------------------------------------------
	FName.
----------------------------------------------------------------------------*/

#define checkName checkSlow

enum ELinkerNameTableConstructor    {ENAME_LinkerConstructor};

/**
 * Public name, available to the world.  Names are stored as a combination of
 * an index into a table of unique strings and an instance number.
 * Names are case-insensitive.
 */
class FName 
{
public:

	/** @name Accessors */
	//@{
	NAME_INDEX GetIndex() const
	{
		checkName(Index < Names.Num());
		checkName(Names(Index));
		return Index;
	}
	INT GetNumber() const;
	const TCHAR* GetName() const
	{
		return Names(Index)->Name;
	}
	//@}

	/**
	 * Converts an FName to a readable format
	 *
	 * @return String representation of the name
	 */
	FString ToString() const;

	/**
	 * Converts an FName to a readable format, in place
	 * 
	 * @param Out String to fill ot with the string representation of the name
	 */
	void ToString(FString& Out) const;

	/** @name Name flags */
	//@{
	/**
	 * Used to safely check whether the passed in flag is set. This is required as 
	 * EObjectFlags currently is a 64 bit data type and UBOOL is a 32 bit data type so
	 * simply using GetFlags() & RF_MyFlagBiggerThanMaxInt won't work correctly when 
	 * assigned directly to an UBOOL.
	 *
	 * @param	FlagToCheck		Object flag to check for.
	 * @return					TRUE if the passed in flag is set, FALSE otherwise (including no flag passed in).
	 */
	UBOOL HasAnyFlags( EObjectFlags FlagToCheck ) const
	{
		return Names(Index)->HasAnyFlags( FlagToCheck );
	}
	EObjectFlags GetFlags() const
	{
		checkName(Index < Names.Num());
		checkName(Names(Index));
		return Names(Index)->Flags;
	}
	void SetFlags( EObjectFlags Set ) const
	{
		checkName(Index < Names.Num());
		checkName(Names(Index));
		Names(Index)->SetFlags( Set );
	}
	void ClearFlags( EObjectFlags Clear ) const
	{
		checkName(Index < Names.Num());
		checkName(Names(Index));
		Names(Index)->ClearFlags( Clear );
	}
	//@}

	UBOOL operator==( const FName& Other ) const
	{
		return Index == Other.Index && Number == Other.Number;
	}
	UBOOL operator!=( const FName& Other ) const
	{
		return Index != Other.Index || Number != Other.Number;
	}
	UBOOL IsValid() const
	{
		return Index>=0 && Index<Names.Num() && Names(Index)!=NULL;
	}

	/**
	 * Create an FName with a hardcoded string index.
	 *
	 * @param N The harcdcoded value the string portion of the name will have. The number portion will be NAME_NO_NUMBER
	 */
	FName( enum EName N )
	: Index( N )
	, Number( NAME_NO_NUMBER_INTERNAL )
	{}

	/**
	 * Create an FName with a hardcoded string index and (instance).
	 *
	 * @param N The harcdcoded value the string portion of the name will have
	 * @param InNumber The hardcoded value for the number portion of the name
	 */
	FName( enum EName N, INT InNumber )
	: Index( N )
	, Number( InNumber )
	{}

	/**
	 * Create an uninitialized FName
	 */
	FName()
	{}

	/**
	 * Create an FName. If FindType is FNAME_Find, and the string part of the name 
	 * doesn't already exist, then the name will be NAME_None
	 *
	 * @param Name			Value for the string portion of the name
	 * @param FindType		Action to take (see EFindName)
	 * @param unused
	 */
	FName( const TCHAR* Name, EFindName FindType=FNAME_Add, UBOOL bUnused=TRUE );

	/**
	 * Create an FName. If FindType is FNAME_Find, and the string part of the name 
	 * doesn't already exist, then the name will be NAME_None
	 *
	 * @param Name Value for the string portion of the name
	 * @param Number Value for the number portion of the name
	 * @param FindType Action to take (see EFindName)
	 */
	FName( const TCHAR* Name, INT InNumber, EFindName FindType=FNAME_Add );

	/**
	 * Constructor used by ULinkerLoad when loading its name table; Creates an FName with an instance
	 * number of 0 that does not attempt to split the FName into string and number portions.
	 */
	FName( ELinkerNameTableConstructor, const TCHAR* Name );

	// Copy operator.
	FName& operator=( const FName& Other )
	{
		Index = Other.Index;
		Number = Other.Number;
		return *this;
	}

	/** @name Name subsystem */
	//@{
	static void StaticInit();
	static void StaticExit();
	static void DisplayHash( class FOutputDevice& Ar );
	static void Hardcode( FNameEntry* AutoName );
	//@}

	/** @name Name subsystem accessors */
	//@{
	static FString SafeString( EName Index, INT InstanceNumber=NAME_NO_NUMBER_INTERNAL )
	{
		return GetIsInitialized()
			? (Names.IsValidIndex(Index) && Names(Index))
				? FName(Index, InstanceNumber).ToString()
				: FString(TEXT("Invalid"))
			: FString(TEXT("Uninitialized"));
	}
	static UBOOL SafeSuppressed( EName Index )
	{
#ifdef RF_Suppress
#error
#else
#define RF_Suppress				DECLARE_UINT64(0x0000100000000000)		// @warning: Mirrored in UnObjBas.h. Suppressed log name.
#endif
		return GetIsInitialized() && (Names(Index)->Flags & RF_Suppress);
#undef RF_Suppress
	}
	static INT GetMaxNames()
	{
		return Names.Num();
	}
	/**
	 * @return Size of all name entries.
	 */
	static INT GetNameEntryMemorySize()
	{
		return NameEntryMemorySize;
	}
	static FNameEntry* GetEntry( int i )
	{
		return Names(i);
	}
	static UBOOL GetInitialized()
	{
		return GetIsInitialized();
	}
	//@}

	/**
	 * Helper function to split an old-style name (Class_Number, ie Rocket_17) into
	 * the component parts usable by new-style FNames
	 *
	 * @param OldName		Old-style name
	 * @param NewName		Ouput string portion of the name/number pair
	 * @param NewNumber		Number portion of the name/number pair
	 */
	static void SplitOldName(const TCHAR* OldName, FString& NewName, INT& NewNumber);

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
	static UBOOL SplitNameWithCheck(const TCHAR* OldName, TCHAR* NewName, INT NewNameLen, INT& NewNumber);

private:
	/** Index into the Names array (used to find String portion of the string/number pair) */
	NAME_INDEX Index;

	/** Number portion of the string/number pair (stored internally as 1 more than actual, so zero'd memory will be the default, no-instance case) */
	INT			Number;

	// Static subsystem variables.
	/** Table of all names. This is a NoInit because we may need to use it before the constructor is called due to random static variable initialization order */
	static TArrayNoInit<FNameEntry*>	Names;					// Table of all names.
	static FNameEntry*					NameHash[4096];			// Hashed names.
	static INT							NameEntryMemorySize;	// Size of all name entries.

	/**
	 * Return the static initialized flag. Must be in a function like this so we don't have problems with 
	 * different initialization order of static variables across the codebase. Use this function to get or set the variable.
	 */
	static UBOOL& GetIsInitialized()
	{
		static UBOOL bIsInitialized = 0;
		return bIsInitialized;
	}

	friend const TCHAR* DebugFName(INT);
	friend const TCHAR* DebugFName(INT, INT);
	friend const TCHAR* DebugFName(FName&);
	friend const TCHAR* DebugFName(UObject*);
	friend FNameEntry* AllocateNameEntry( const TCHAR* Name, DWORD Index, EObjectFlags Flags, FNameEntry* HashNext );

	/**
	 * Shared initialization code (between two constructors)
	 * 
	 * @param InName String name of the name/number pair
	 * @param InNumber Number part of the name/number pair
	 * @param FindType Operation to perform on names
	 * @param bSplitName If TRUE, this function will attempt to split a number off of the string portion (turning Rocket_17 to Rocket and number 17)
	 */
	void Init(const TCHAR* InName, INT InNumber, EFindName FindType, UBOOL bSplitName=TRUE);

public:
	/**
	 * This function is only for use by the debugger FName add-in; it should never be called from code
	 */
	static TArray<FNameEntry*>&	GetPureNamesTable(void)
	{
		return FName::Names;
	}
};
inline DWORD GetTypeHash( const FName N )
{
	return N.GetIndex();
}

/** Type information for FName objects.  No construction or destruction necessary. */
template <> class TTypeInfo<FName>		: public TTypeInfoAtomicBase<FName> {};
