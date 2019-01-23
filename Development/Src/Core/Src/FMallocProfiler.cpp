/*=============================================================================
	FMallocProfiler.cpp: Memory profiling support.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

#include "FMallocProfiler.h"

/** Maximum depth of stack backtrace										*/
#define	MEMORY_PROFILER_MAX_BACKTRACE_DEPTH		50

/** Magic value, determining that file is a memory profiler file.			*/
#define MEMORY_PROFILER_MAGIC					0xDA15F7D8
/** Version of memory profiler.												*/
#define MEMORY_PROFILER_VERSION					2


enum EProfilingPayloadType
{
	TYPE_Malloc  = 0,
	TYPE_Free	 = 1,
	TYPE_Realloc = 2,
	TYPE_Other   = 3,
	// Don't add more than 4 values - we only have 2 bits to store this.
};

enum EProfilingPayloadSubType
{
	SUBTYPE_EndOfStreamMarker	= 0,
};


/*=============================================================================
	Profiler header.
=============================================================================*/

struct FProfilerHeader
{
	/** Magic to ensure we're opening the right file.	*/
	DWORD	Magic;
	/** Version number to detect version mismatches.	*/
	DWORD	Version;
	/** Platform that was captured.						*/
	DWORD	Platform;
	/** Back trace depth.								*/
	DWORD	BackTraceDepth;

	/** Offset in file for name table.					*/
	DWORD	NameTableOffset;
	/** Number of name table entries.					*/
	DWORD	NameTableEntries;

	/** Offset in file for callstack address table.		*/
	DWORD	CallStackAddressTableOffset;
	/** Number of callstack address entries.			*/
	DWORD	CallStackAddressTableEntries;

	/** Offset in file for callstack table.				*/
	DWORD	CallStackTableOffset;
	/** Number of callstack entries.					*/
	DWORD	CallStackTableEntries;

	/**
	 * Serialization operator.
	 *
	 * @param	Ar			Archive to serialize to
	 * @param	Header		Header to serialize
	 * @return	Passed in archive
	 */
	friend FArchive& operator << ( FArchive& Ar, FProfilerHeader Header )
	{
		Ar	<< Header.Magic
			<< Header.Version
			<< Header.Platform
			<< Header.BackTraceDepth
			<< Header.NameTableOffset
			<< Header.NameTableEntries
			<< Header.CallStackAddressTableOffset
			<< Header.CallStackAddressTableEntries
			<< Header.CallStackTableOffset
			<< Header.CallStackTableEntries;
		return Ar;
	}
};

/*=============================================================================
	CallStack address information.
=============================================================================*/

/**
 * Helper structure encapsulating a single address/ point in a callstack
 */
struct FCallStackAddressInfo
{
	/** Program counter address of entry.			*/
	QWORD	ProgramCounter;
	/** Index into name table for module.			*/
	INT		ModuleNameTableIndex;
	/** Index into name table for filename.			*/
	INT		FilenameNameTableIndex;
	/** Index into name table for function name.	*/
	INT		FunctionNameTableIndex;
	/** Line number in file.						*/
	INT		LineNumber;
	/** Symbol displacement of address.				*/
	INT		SymbolDisplacement;

	/**
	 * Serialization operator.
	 *
	 * @param	Ar			Archive to serialize to
	 * @param	AddressInfo	AddressInfo to serialize
	 * @return	Passed in archive
	 */
	friend FArchive& operator << ( FArchive& Ar, FCallStackAddressInfo AddressInfo )
	{
		Ar	<< AddressInfo.ProgramCounter
			<< AddressInfo.ModuleNameTableIndex
			<< AddressInfo.FilenameNameTableIndex
			<< AddressInfo.FunctionNameTableIndex
			<< AddressInfo.LineNumber
			<< AddressInfo.SymbolDisplacement;
		return Ar;
	}
};

/**
 * Helper structure encapsulating a callstack.
 */
struct FCallStackInfo
{
	/** CRC of program counters for this callstack.				*/
	DWORD	CRC;
	/** Array of indices into callstack address info array.		*/
	INT		AddressIndices[MEMORY_PROFILER_MAX_BACKTRACE_DEPTH];

	/**
	 * Serialization operator.
	 *
	 * @param	Ar			Archive to serialize to
	 * @param	AllocInfo	Callstack info to serialize
	 * @return	Passed in archive
	 */
	friend FArchive& operator << ( FArchive& Ar, FCallStackInfo CallStackInfo )
	{
		Ar << CallStackInfo.CRC;
		for( INT i=0; i<ARRAY_COUNT(CallStackInfo.AddressIndices); i++ )
		{
			Ar << CallStackInfo.AddressIndices[i];
		}
		return Ar;
	}
};

/*=============================================================================
	Allocation infos.
=============================================================================*/

/**
 * Relevant information for a single malloc operation.
 *
 * 20 bytes (assuming 32 bit pointers)
 */
struct FProfilerAllocInfo
{
	/** Pointer of allocation, lower two bits are used for payload type.	*/
	PTRINT Pointer;
	/** Index of callstack.													*/
	INT CallStackIndex;
	/** Size of allocation.													*/
	DWORD Size;
	/** 0 based time of allocation.											*/
	FLOAT CurrentTime;
	/** Thread Id of calling code.											*/
	DWORD ThreadId;

	/**
	 * Serialization operator.
	 *
	 * @param	Ar			Archive to serialize to
	 * @param	AllocInfo	Allocation info to serialize
	 * @return	Passed in archive
	 */
	friend FArchive& operator << ( FArchive& Ar, FProfilerAllocInfo AllocInfo )
	{
		Ar	<< AllocInfo.Pointer
			<< AllocInfo.CallStackIndex
			<< AllocInfo.Size
			<< AllocInfo.CurrentTime
			<< AllocInfo.ThreadId;
		return Ar;
	}
};

/**
 * Relevant information for a single free operation.
 *
 * 16 bytes (assuming 32 bit pointers)
 */
struct FProfilerFreeInfo
{
	/** Free'd pointer, lower two bits are used for payload type.			*/
	PTRINT Pointer;
	/** Index of callstack.													*/
	INT CallStackIndex;
	/** 0 based time of allocation.											*/
	FLOAT CurrentTime;
	/** Thread Id of calling code.											*/
	DWORD ThreadId;

	/**
	 * Serialization operator.
	 *
	 * @param	Ar			Archive to serialize to
	 * @param	FreeInfo	Free info to serialize
	 * @return	Passed in archive
	 */
	friend FArchive& operator << ( FArchive& Ar, FProfilerFreeInfo FreeInfo )
	{
		Ar	<< FreeInfo.Pointer
			<< FreeInfo.CallStackIndex
			<< FreeInfo.CurrentTime
			<< FreeInfo.ThreadId;
		return Ar;
	}
};

/**
 * Relevant information for a single realloc operation.
 *
 * 24 bytes (assuming 32 bit pointers)
 */
struct FProfilerReallocInfo
{
	/** Old pointer, lower two bits are used for payload type.				*/
	PTRINT OldPointer;
	/** New pointer, might be indentical to old.							*/
	PTRINT NewPointer;
	/** Index of callstack.													*/
	INT CallStackIndex;
	/** Size of allocation.													*/
	DWORD Size;
	/** 0 based time of allocation.											*/
	FLOAT CurrentTime;
	/** Thread Id of calling code.											*/
	DWORD ThreadId;

	/**
	 * Serialization operator.
	 *
	 * @param	Ar			Archive to serialize to
	 * @param	ReallocInfo	Realloc info to serialize
	 * @return	Passed in archive
	 */
	friend FArchive& operator << ( FArchive& Ar, FProfilerReallocInfo ReallocInfo )
	{
		Ar	<< ReallocInfo.OldPointer
			<< ReallocInfo.NewPointer
			<< ReallocInfo.CallStackIndex
			<< ReallocInfo.Size
			<< ReallocInfo.CurrentTime
			<< ReallocInfo.ThreadId;
		return Ar;
	}
};

/**
 * Helper structure for misc data like e.g. end of stream marker.
 *
 * 12 bytes (assuming 32 bit pointers)
 */
struct FProfilerOtherInfo
{
	/** Dummy pointer as all tokens start with a pointer (TYPE_Other)		*/
	PTRINT	DummyPointer;
	/** Subtype.															*/
	INT		SubType;
	/** Subtype specific payload.											*/
	INT		Payload;

	/**
	 * Serialization operator.
	 *
	 * @param	Ar			Archive to serialize to
	 * @param	OtherInfo	Info to serialize
	 * @return	Passed in archive
	 */
	friend FArchive& operator << ( FArchive& Ar, FProfilerOtherInfo OtherInfo )
	{
		Ar	<< OtherInfo.DummyPointer
			<< OtherInfo.SubType
			<< OtherInfo.Payload;
		return Ar;
	}
};

/*=============================================================================
	FMallocProfiler implementation.
=============================================================================*/


/**
 * Constructor, initializing all member variables and potentially loading symbols.
 *
 * @param	InMalloc	The allocator wrapped by FMallocProfiler that will actually do the allocs/deallocs.
 */
FMallocProfiler::FMallocProfiler(FMalloc* InMalloc)
:	UsedMalloc( InMalloc )
,	bShouldTrackOperations( TRUE )
,	BufferedFileWriter( TEXT("MemoryProfiler") )
{
#if LOAD_SYMBOLS_FOR_STACK_WALKING
	// Initialize symbols.
	appInitStackWalking();
#endif
	StartTime = appSeconds();

	// Serialize dummy header, overwritten in EndProfiling.
	FProfilerHeader DummyHeader;
	appMemzero( &DummyHeader, sizeof(DummyHeader) );
	BufferedFileWriter << DummyHeader;
}

/**
 * Tracks malloc operation.
 *
 * @param	Ptr		Allocated pointer 
 * @param	Size	Size of allocated pointer
 */
void FMallocProfiler::TrackMalloc( void* Ptr, DWORD Size )
{
	// Avoid tracking operations caused by tracking!
	if( bShouldTrackOperations )
	{
		// Disable allocation tracking as we're using malloc & co internally.
		bShouldTrackOperations		= FALSE;
		
		// Gather information about operation.
		FProfilerAllocInfo AllocInfo;
		AllocInfo.Pointer			= (PTRINT) Ptr | TYPE_Malloc;
		AllocInfo.CallStackIndex	= GetCallStackIndex();
		AllocInfo.Size				= Size;
		AllocInfo.CurrentTime		= (FLOAT) (appSeconds() - StartTime);
		AllocInfo.ThreadId			= appGetCurrentThreadId();

		// Serialize to HDD.
		BufferedFileWriter << AllocInfo;

		// Re-enable allocation tracking again.
		bShouldTrackOperations		= TRUE;
	}
}
	
/**
 * Tracks free operation
 *
 * @param	Ptr		Freed pointer
 */
void FMallocProfiler::TrackFree( void* Ptr )
{
	// Avoid tracking operations caused by tracking!
	if( bShouldTrackOperations )
	{
		// Disable allocation tracking as we're using malloc & co internally.
		bShouldTrackOperations		= FALSE;

		// Gather information about operation.
		FProfilerFreeInfo FreeInfo;
		FreeInfo.Pointer			= (PTRINT) Ptr | TYPE_Free;
		FreeInfo.CallStackIndex		= GetCallStackIndex();
		FreeInfo.CurrentTime		= (FLOAT) (appSeconds() - StartTime);
		FreeInfo.ThreadId			= appGetCurrentThreadId();

		// Serialize to HDD.
		BufferedFileWriter << FreeInfo;

		// Re-enable allocation tracking again.
		bShouldTrackOperations		= TRUE;
	}
}
	
/**
 * Tracks realloc operation
 *
 * @param	OldPtr	Previous pointer
 * @param	NewPtr	New pointer
 * @param	NewSize	New size of allocation
 */
void FMallocProfiler::TrackRealloc( void* OldPtr, void* NewPtr, DWORD NewSize )
{
	// Avoid tracking operations caused by tracking!
	if( bShouldTrackOperations )
	{
		// Disable allocation tracking as we're using malloc & co internally.
		bShouldTrackOperations		= FALSE;
		
		// Gather information about operation.
		FProfilerReallocInfo ReallocInfo;
		ReallocInfo.OldPointer		= (PTRINT) OldPtr | TYPE_Realloc;
		ReallocInfo.NewPointer		= (PTRINT) NewPtr;
		ReallocInfo.CallStackIndex	= GetCallStackIndex();
		ReallocInfo.Size			= NewSize;
		ReallocInfo.CurrentTime		= (FLOAT) (appSeconds() - StartTime);
		ReallocInfo.ThreadId		= appGetCurrentThreadId();

		// Serialize to HDD.
		BufferedFileWriter << ReallocInfo;

		// Re-enable allocation tracking again.
		bShouldTrackOperations		= TRUE;
	}
}

/**
 * Ends profiling operation and closes file.
 */
void FMallocProfiler::EndProfiling()
{
	// Stop all further tracking operations. This cannot be re-enabled as we close the file writer at the end.
	bShouldTrackOperations = FALSE;

#if !CONSOLE
	// Fill in callstack address information by performing symbol lookup.
	for( INT AddressIndex=0; AddressIndex<CallStackAddressInfoArray.Num(); AddressIndex++ )
	{
		FCallStackAddressInfo&			AddressInfo = CallStackAddressInfoArray(AddressIndex);
		// Look up symbols.
		const FProgramCounterSymbolInfo SymbolInfo	= appProgramCounterToSymbolInfo( AddressInfo.ProgramCounter );

		// Convert to strings, and clean up in the process.
		FString ModulName		= FFilename(SymbolInfo.ModuleName).GetCleanFilename();
		FString FileName		= FString(SymbolInfo.Filename);
		FString FunctionName	= FString(SymbolInfo.FunctionName);

		// Propagate to our own struct, also populating name table.
		AddressInfo.ModuleNameTableIndex	= GetNameTableIndex( ModulName );	
		AddressInfo.FilenameNameTableIndex	= GetNameTableIndex( FileName );
		AddressInfo.FunctionNameTableIndex	= GetNameTableIndex( FunctionName );
		AddressInfo.LineNumber				= SymbolInfo.LineNumber;
		AddressInfo.SymbolDisplacement		= SymbolInfo.SymbolDisplacement;	
	}
#endif

	// Write end of stream marker.
	FProfilerOtherInfo EndOfStream;
	EndOfStream.DummyPointer	= TYPE_Other;
	EndOfStream.SubType			= SUBTYPE_EndOfStreamMarker;
	EndOfStream.Payload			= 0;
	BufferedFileWriter << EndOfStream;

	// Real header, written at end.
	FProfilerHeader Header;
	Header.Magic				= MEMORY_PROFILER_MAGIC;
	Header.Version				= MEMORY_PROFILER_VERSION;
	Header.Platform				= appGetPlatformType();
	Header.BackTraceDepth		= MEMORY_PROFILER_MAX_BACKTRACE_DEPTH;

	// Write out name table and update header with offset and count.
	Header.NameTableOffset				= BufferedFileWriter.Tell();
	Header.NameTableEntries				= NameArray.Num();
	for( INT NameIndex=0; NameIndex<NameArray.Num(); NameIndex++ )
	{
		const FString&	Name	= NameArray(NameIndex);
		INT				Length	= Name.Len();
		BufferedFileWriter << Length;
		for( INT CharIndex=0; CharIndex<Name.Len(); CharIndex++ )
		{
			ANSICHAR AnsiChar = ToAnsi( Name[CharIndex] );
			BufferedFileWriter << AnsiChar;
		}
	}

	// Write out callstack address infos and update header with offset and count.
	Header.CallStackAddressTableOffset	= BufferedFileWriter.Tell();
	Header.CallStackAddressTableEntries	= CallStackAddressInfoArray.Num();
	for( INT CallStackAddressIndex=0; CallStackAddressIndex<CallStackAddressInfoArray.Num(); CallStackAddressIndex++ )
	{
		BufferedFileWriter << CallStackAddressInfoArray(CallStackAddressIndex);
	}

	// Write out callstack infos and update header with offset and count.
	Header.CallStackTableOffset			= BufferedFileWriter.Tell();
	Header.CallStackTableEntries		= CallStackInfoArray.Num();
	for( INT CallStackIndex=0; CallStackIndex<CallStackInfoArray.Num(); CallStackIndex++ )
	{
		BufferedFileWriter << CallStackInfoArray(CallStackIndex);
	}

	// Seek to the beginning of the file and write out proper header.
	BufferedFileWriter.Seek( 0 );
	BufferedFileWriter << Header;

	// At last, close file writer.
	BufferedFileWriter.Close();
}

/**
 * Returns index of passed in program counter into program counter array. If not found, adds it.
 *
 * @param	ProgramCounter	Program counter to find index for
 * @return	Index of passed in program counter if it's not NULL, INDEX_NONE otherwise
 */
INT FMallocProfiler::GetProgramCounterIndex( QWORD ProgramCounter )
{
	INT	Index = INDEX_NONE;

	// Look up index in unique array of program counter infos, if we have a valid program counter.
	if( ProgramCounter )
	{
		// Use index if found.
		INT* IndexPtr = ProgramCounterToIndexMap.Find( ProgramCounter );
		if( IndexPtr )
		{
			Index = *IndexPtr;
		}
		// Encountered new program counter, add to array and set index mapping.
		else
		{
			// Add to aray and set mapping for future use.
			Index = CallStackAddressInfoArray.AddZeroed();
			ProgramCounterToIndexMap.Set( ProgramCounter, Index );

			// Only initialize program counter, rest will be filled in at the end, once symbols are loaded.
			FCallStackAddressInfo& CallStackAddressInfo = CallStackAddressInfoArray(Index);
			CallStackAddressInfo.ProgramCounter = ProgramCounter;
		}
		check(Index!=INDEX_NONE);
	}

	return Index;
}

/** 
 * Returns index of callstack, captured by this function into array of callstack infos. If not found, adds it.
 *
 * @return index into callstack array
 */
INT FMallocProfiler::GetCallStackIndex()
{
	// Index of callstack in callstack info array.
	INT Index = INDEX_NONE;

	// Capture callstack and create CRC.
	QWORD FullBackTrace[MEMORY_PROFILER_MAX_BACKTRACE_DEPTH + 5];
	appCaptureStackBackTrace( FullBackTrace, MEMORY_PROFILER_MAX_BACKTRACE_DEPTH + 5 );
	// Skip first 5 entries as they are inside the allocator.
	QWORD* BackTrace = &FullBackTrace[5];
	DWORD CRC = appMemCrc( BackTrace, MEMORY_PROFILER_MAX_BACKTRACE_DEPTH * sizeof(QWORD), 0 );

	// Use index if found
	INT* IndexPtr = CRCToCallStackIndexMap.Find( CRC );
	if( IndexPtr )
	{
		Index = *IndexPtr;
	}
	// Encountered new call stack, add to array and set index mapping.
	else
	{
		// Add to array and set mapping for future use.
		Index = CallStackInfoArray.Add();
		CRCToCallStackIndexMap.Set( CRC, Index );
	
		// Set up callstack info with captured call stack, translating program counters
		// to indices in program counter array (unique entries).
		FCallStackInfo& CallStackInfo = CallStackInfoArray(Index);
		CallStackInfo.CRC = CRC;
		for( INT i=0; i<MEMORY_PROFILER_MAX_BACKTRACE_DEPTH; i++ )
		{
			CallStackInfo.AddressIndices[i] = GetProgramCounterIndex( BackTrace[i] );
		}
	}

	check(Index!=INDEX_NONE);
	return Index;
}	

/**
 * Returns index of passed in name into name array. If not found, adds it.
 *
 * @param	Name	Name to find index for
 * @return	Index of passed in name
 */
INT FMallocProfiler::GetNameTableIndex( const FString& Name )
{
	// Index of name in name table.
	INT Index = INDEX_NONE;

	// Use index if found.
	INT* IndexPtr = NameToNameTableIndexMap.Find( Name );
	if( IndexPtr )
	{
		Index = *IndexPtr;
	}
	// Encountered new name, add to array and set index mapping.
	else
	{
		Index = NameArray.Num();
		new(NameArray)FString(Name);
		NameToNameTableIndexMap.Set(*Name,Index);
	}

	check(Index!=INDEX_NONE);
	return Index;
}

/*=============================================================================
	FBufferedFileWriter implementation
=============================================================================*/

/**
 * Constructor. Called before GMalloc is initalized!!!
 */
FBufferedFileWriter::FBufferedFileWriter( const TCHAR* InBaseFilename )
:	FileWriter( NULL )
,	BaseFilename( InBaseFilename )
{
	ArIsSaving		= TRUE;
	ArIsPersistent	= TRUE;
}

/**
 * Destructor, cleaning up FileWriter.
 */
FBufferedFileWriter::~FBufferedFileWriter()
{
	if( FileWriter )
	{
		delete FileWriter;
		FileWriter = NULL;
	}
}

// FArchive interface.

void FBufferedFileWriter::Serialize( void* V, INT Length )
{
	// Copy to buffered memory array if file manager hasn't been set up yet.
	if( GFileManager == NULL )
	{
		INT Index = BufferedData.Add( Length );
		appMemcpy( &BufferedData(Index), V, Length );
	} 
	// File manager is set up but we haven't created file writer yet, do it now.
	else if( FileWriter == NULL )
	{
		// Create file writer to serialize data to HDD.
		FString Filename = appGameLogDir() + BaseFilename + TEXT("-") + appSystemTimeString() + TEXT(".mprof");
		FileWriter = GFileManager->CreateFileWriter( *Filename );
		check( FileWriter );

		// Serialize existing buffered data and empty array.
		FileWriter->Serialize( BufferedData.GetData(), BufferedData.Num() );
		BufferedData.Empty();
	}

	// Serialize data to HDD via FileWriter if it already has been created.
	if( FileWriter )
	{
		FileWriter->Serialize( V, Length );
	}
}

void FBufferedFileWriter::Seek( INT InPos )
{
	check( FileWriter );
	FileWriter->Seek( InPos );
}

UBOOL FBufferedFileWriter::Close()
{
	check( FileWriter );
	return FileWriter->Close();
}

INT FBufferedFileWriter::Tell()
{
	check( FileWriter );
	return FileWriter->Tell();
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
