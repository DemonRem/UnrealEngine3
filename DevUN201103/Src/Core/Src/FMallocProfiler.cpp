/*=============================================================================
FMallocProfiler.cpp: Memory profiling support.
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

#include "FMallocProfiler.h"
#include "ProfilingHelpers.h"

#if USE_MALLOC_PROFILER

/**
 * Maximum depth of stack backtrace.
 * Reducing this will sometimes truncate the amount of callstack info you get but will also reduce
 * the number of over all unique call stacks as some of the script call stacks are REALLY REALLY
 * deep and end up eating a lot of memory which will OOM you on consoles. A good value for consoles 
 * when doing long runs is 50.
 */
#define	MEMORY_PROFILER_MAX_BACKTRACE_DEPTH			75
/** Number of backtrace entries to skip											*/
#define MEMORY_PROFILER_SKIP_NUM_BACKTRACE_ENTRIES	5

/** Magic value, determining that file is a memory profiler file.				*/
#define MEMORY_PROFILER_MAGIC						0xDA15F7D8
/** Version of memory profiler.													*/
#define MEMORY_PROFILER_VERSION						2

/** Whether we are performing symbol lookup at runtime or not.					*/
#if !CONSOLE
#define SERIALIZE_SYMBOL_INFO 1
#else
#define SERIALIZE_SYMBOL_INFO 0
#endif

enum EProfilingPayloadType
{
	TYPE_Malloc  = 0,
	TYPE_Free	 = 1,
	TYPE_Realloc = 2,
	TYPE_Other   = 3,
	// Don't add more than 4 values - we only have 2 bits to store this.
};

/*=============================================================================
	Helpers
=============================================================================*/

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
	/** Whether symbol information is being serialized. */
	DWORD	bShouldSerializeSymbolInfo;
	/** Name of executable, used for finding symbols.	*/
	FString ExecutableName;

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
	/** The file offset for module information.			*/
	DWORD	ModulesOffset;
	/** The number of module entries.					*/
	DWORD	ModuleEntries;
	/** Number of data files total.						*/
	DWORD	NumDataFiles;

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
			<< Header.bShouldSerializeSymbolInfo
			<< Header.NameTableOffset
			<< Header.NameTableEntries
			<< Header.CallStackAddressTableOffset
			<< Header.CallStackAddressTableEntries
			<< Header.CallStackTableOffset
			<< Header.CallStackTableEntries
			<< Header.ModulesOffset
			<< Header.ModuleEntries
			<< Header.NumDataFiles;
		check( Ar.IsSaving() );
		SerializeStringAsANSICharArray( Header.ExecutableName, Ar, 255 );
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
#if SERIALIZE_SYMBOL_INFO
	/** Index into name table for filename.			*/
	INT		FilenameNameTableIndex;
	/** Index into name table for function name.	*/
	INT		FunctionNameTableIndex;
	/** Line number in file.						*/
	INT		LineNumber;
#endif

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
#if SERIALIZE_SYMBOL_INFO
			<< AddressInfo.FilenameNameTableIndex
			<< AddressInfo.FunctionNameTableIndex
			<< AddressInfo.LineNumber
#endif
			;
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
		// Serialize valid callstack indices.
		INT i=0;
		for( ; i<ARRAY_COUNT(CallStackInfo.AddressIndices) && CallStackInfo.AddressIndices[i]!=-1; i++ )
		{
			Ar << CallStackInfo.AddressIndices[i];
		}
		// Terminate list of address indices with -1 if we have a normal callstack.
		INT Stopper = -1;
		// And terminate with -2 if the callstack was truncated.
		if( i== ARRAY_COUNT(CallStackInfo.AddressIndices) )
		{
			Stopper = -2;
		}

		Ar << Stopper;
		return Ar;
	}
};

/*=============================================================================
	Allocation infos.
=============================================================================*/

/**
 * Relevant information for a single malloc operation.
 *
 * 16 bytes
 */
struct FProfilerAllocInfo
{
	/** Pointer of allocation, lower two bits are used for payload type.	*/
	QWORD Pointer;
	/** Index of callstack.													*/
	INT CallStackIndex;
	/** Size of allocation.													*/
	DWORD Size;

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
			<< AllocInfo.Size;
		return Ar;
	}
};

/**
 * Relevant information for a single free operation.
 *
 * 8 bytes
 */
struct FProfilerFreeInfo
{
	/** Free'd pointer, lower two bits are used for payload type.			*/
	QWORD Pointer;
	
	/**
	 * Serialization operator.
	 *
	 * @param	Ar			Archive to serialize to
	 * @param	FreeInfo	Free info to serialize
	 * @return	Passed in archive
	 */
	friend FArchive& operator << ( FArchive& Ar, FProfilerFreeInfo FreeInfo )
	{
		Ar	<< FreeInfo.Pointer;
		return Ar;
	}
};

/**
 * Relevant information for a single realloc operation.
 *
 * 24 bytes
 */
struct FProfilerReallocInfo
{
	/** Old pointer, lower two bits are used for payload type.				*/
	QWORD OldPointer;
	/** New pointer, might be identical to old.								*/
	QWORD NewPointer;
	/** Index of callstack.													*/
	INT CallStackIndex;
	/** Size of allocation.													*/
	DWORD Size;

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
			<< ReallocInfo.Size;
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
	QWORD	DummyPointer;
	/** Subtype.															*/
	INT		SubType;
	/** Subtype specific payload.											*/
	DWORD	Payload;

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
// Default to FALSE as we might need to do a few allocations here.
,	bShouldTrackOperations( FALSE )
,   bEndProfilingHasBeenCalled( FALSE )
,	CallStackInfoBuffer( 512 * 1024, COMPRESS_ZLIB )
,	MemoryOperationCount( 0 )
{
#if LOAD_SYMBOLS_FOR_STACK_WALKING
	// Initialize symbols.
	appInitStackWalking();
#endif
	StartTime = appSeconds();

	BeginProfiling();
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
		bShouldTrackOperations = FALSE;

		// Gather information about operation.
		FProfilerAllocInfo AllocInfo;
		AllocInfo.Pointer			= (QWORD) Ptr | TYPE_Malloc;
		AllocInfo.CallStackIndex	= GetCallStackIndex();
		AllocInfo.Size				= Size;

		// Serialize to HDD.
		BufferedFileWriter << AllocInfo;

		// Re-enable allocation tracking again.
		PossiblySplitMprof();
		bShouldTrackOperations = TRUE;
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
	if( bShouldTrackOperations && Ptr )
	{
		// Disable allocation tracking as we're using malloc & co internally.
		bShouldTrackOperations = FALSE;

		// Gather information about operation.
		FProfilerFreeInfo FreeInfo;
		FreeInfo.Pointer = (QWORD) Ptr | TYPE_Free;

		// Serialize to HDD.
		BufferedFileWriter << FreeInfo;

		// Re-enable allocation tracking again.
		PossiblySplitMprof();
		bShouldTrackOperations = TRUE;
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
	if( bShouldTrackOperations && (OldPtr || NewPtr) )
	{
		// Disable allocation tracking as we're using malloc & co internally.
		bShouldTrackOperations = FALSE;

		// Gather information about operation.
		FProfilerReallocInfo ReallocInfo;
		ReallocInfo.OldPointer		= (QWORD) OldPtr | TYPE_Realloc;
		ReallocInfo.NewPointer		= (QWORD) NewPtr;
		ReallocInfo.CallStackIndex	= GetCallStackIndex();
		ReallocInfo.Size			= NewSize;

		// Serialize to HDD.
		BufferedFileWriter << ReallocInfo;

		// Re-enable allocation tracking again.
		PossiblySplitMprof();
		bShouldTrackOperations = TRUE;
	}
}

/**
 * Tracks slack and waste in the allocator every 1024 memory operations
 */
void FMallocProfiler::TrackSpecialMemory()
{
	if( ( MemoryOperationCount++ & 0x3ff ) == 0 )
	{
		FMemoryAllocationStats MemStats;
		UsedMalloc->GetAllocationInfo( MemStats );

		EmbedDwordMarker( SUBTYPE_TotalUsed, MemStats.TotalUsed );
		EmbedDwordMarker( SUBTYPE_TotalAllocated, MemStats.TotalAllocated );
		EmbedDwordMarker( SUBTYPE_CPUUsed, MemStats.CPUUsed );
		EmbedDwordMarker( SUBTYPE_CPUSlack, MemStats.CPUSlack );
		EmbedDwordMarker( SUBTYPE_CPUWaste, MemStats.CPUWaste );
		EmbedDwordMarker( SUBTYPE_GPUUsed, MemStats.GPUUsed );
		EmbedDwordMarker( SUBTYPE_GPUSlack, MemStats.GPUSlack );
		EmbedDwordMarker( SUBTYPE_GPUWaste, MemStats.GPUWaste );
		EmbedDwordMarker( SUBTYPE_OSOverhead, MemStats.OSOverhead );
		EmbedDwordMarker( SUBTYPE_ImageSize, MemStats.ImageSize );
	}
}

/**
 * Begins profiling operation and opens file.
 */
void FMallocProfiler::BeginProfiling()
{
	FScopeLock Lock( &CriticalSection );
	check(!bShouldTrackOperations);

	// Serialize dummy header, overwritten in EndProfiling.
	FProfilerHeader DummyHeader;
	appMemzero( &DummyHeader, sizeof(DummyHeader) );
	BufferedFileWriter << DummyHeader;

	// Start tracking now that we're initialized.
	bShouldTrackOperations = TRUE;
	bEndProfilingHasBeenCalled = FALSE;
}

/**
 * Ends profiling operation and closes file.
 */
void FMallocProfiler::EndProfiling()
{
	FScopeLock Lock( &CriticalSection );

	bEndProfilingHasBeenCalled = TRUE;

	// Stop all further tracking operations. This cannot be re-enabled as we close the file writer at the end.
	bShouldTrackOperations = FALSE;

	// Write end of stream marker.
	FProfilerOtherInfo EndOfStream;
	EndOfStream.DummyPointer	= TYPE_Other;
	EndOfStream.SubType			= SUBTYPE_EndOfStreamMarker;
	EndOfStream.Payload			= 0;
	BufferedFileWriter << EndOfStream;

	debugf(TEXT("FMallocProfiler: dumping file [%s]"),*BufferedFileWriter.FullFilepath);
#if SERIALIZE_SYMBOL_INFO
	// Look up symbols on platforms supporting it at runtime.
	for( INT AddressIndex=0; AddressIndex<CallStackAddressInfoArray.Num(); AddressIndex++ )
	{
		FCallStackAddressInfo&	AddressInfo = CallStackAddressInfoArray(AddressIndex);
		// Look up symbols.
		const FProgramCounterSymbolInfo SymbolInfo	= appProgramCounterToSymbolInfo( AddressInfo.ProgramCounter );

		// Convert to strings, and clean up in the process.
		const FString ModulName		= FFilename(SymbolInfo.ModuleName).GetCleanFilename();
		const FString FileName		= FString(SymbolInfo.Filename);
		const FString FunctionName	= FString(SymbolInfo.FunctionName);

		// Propagate to our own struct, also populating name table.
		AddressInfo.FilenameNameTableIndex	= GetNameTableIndex( FileName );
		AddressInfo.FunctionNameTableIndex	= GetNameTableIndex( FunctionName );
		AddressInfo.LineNumber				= SymbolInfo.LineNumber;
	}
#endif // SERIALIZE_SYMBO_INFO

	// Archive used to write out symbol information. This will always be written to the first file, which
	// in the case of multiple files won't be pointed to by BufferedFileWriter.
	FArchive* SymbolFileWriter = NULL;
	// Use the BufferedFileWriter if we're still on the first file.
	if( BufferedFileWriter.FileNumber == 0 )
	{
		SymbolFileWriter = &BufferedFileWriter;
	}
	else
	{
		// Close the last file and transfer it to the PC
		BufferedFileWriter.Close();
		warnf(TEXT("FMallocProfiler: done writing file [%s]"), *(BufferedFileWriter.FullFilepath) );

		FString PartFilename = BufferedFileWriter.BaseFilePath + FString::Printf(TEXT(".m%i"), BufferedFileWriter.FileNumber);
		SendDataToPCViaUnrealConsole( TEXT("UE_PROFILER!MEMORY:"), *PartFilename );

		// Create a file writer appending to the first file.
		BufferedFileWriter.FullFilepath = BufferedFileWriter.BaseFilePath + TEXT(".mprof");
		SymbolFileWriter = GFileManager->CreateFileWriter( *BufferedFileWriter.FullFilepath, FILEWRITE_Append );

		// Seek to the end of the first file.
		SymbolFileWriter->Seek( SymbolFileWriter->Tell() );
	}

	// Real header, written at start of the file but written out right before we close the file.
	FProfilerHeader Header;
	Header.Magic				= MEMORY_PROFILER_MAGIC;
	Header.Version				= MEMORY_PROFILER_VERSION;
	Header.Platform				= appGetPlatformType();
	Header.bShouldSerializeSymbolInfo = SERIALIZE_SYMBOL_INFO ? 1 : 0;
#if XBOX
	Header.ExecutableName		= appExecutableName();
#endif
	Header.NumDataFiles			= BufferedFileWriter.FileNumber + 1;

	// Write out name table and update header with offset and count.
	Header.NameTableOffset	= SymbolFileWriter->Tell();
	Header.NameTableEntries	= NameArray.Num();
	for( INT NameIndex=0; NameIndex<NameArray.Num(); NameIndex++ )
	{
		SerializeStringAsANSICharArray( NameArray(NameIndex), *SymbolFileWriter );
	}

	// Write out callstack address infos and update header with offset and count.
	Header.CallStackAddressTableOffset	= SymbolFileWriter->Tell();
	Header.CallStackAddressTableEntries	= CallStackAddressInfoArray.Num();
	for( INT CallStackAddressIndex=0; CallStackAddressIndex<CallStackAddressInfoArray.Num(); CallStackAddressIndex++ )
	{
		(*SymbolFileWriter) << CallStackAddressInfoArray(CallStackAddressIndex);
	}

	// Write out callstack infos and update header with offset and count.
	Header.CallStackTableOffset			= SymbolFileWriter->Tell();
	Header.CallStackTableEntries		= CallStackInfoBuffer.Num();

	CallStackInfoBuffer.Lock();
	for( INT CallStackIndex=0; CallStackIndex<CallStackInfoBuffer.Num(); CallStackIndex++ )
	{
		FCallStackInfo* CallStackInfo = (FCallStackInfo*) CallStackInfoBuffer.Access( CallStackIndex * sizeof(FCallStackInfo) );
		(*SymbolFileWriter) << (*CallStackInfo);
	}
	CallStackInfoBuffer.Unlock();

	Header.ModulesOffset				= SymbolFileWriter->Tell();
	Header.ModuleEntries				= appGetProcessModuleCount();

	TArray<FModuleInfo> ProcModules(Header.ModuleEntries);

	Header.ModuleEntries = appGetProcessModuleSignatures(&ProcModules(0), ProcModules.Num());

	for(DWORD ModuleIndex = 0; ModuleIndex < Header.ModuleEntries; ++ModuleIndex)
	{
		FModuleInfo &CurModule = ProcModules(ModuleIndex);

		(*SymbolFileWriter) << CurModule.BaseOfImage
							<< CurModule.ImageSize
							<< CurModule.TimeDateStamp
							<< CurModule.PdbSig
							<< CurModule.PdbAge
							<< CurModule.PdbSig70.Data1
							<< CurModule.PdbSig70.Data2
							<< CurModule.PdbSig70.Data3
							<< *((DWORD*)&CurModule.PdbSig70.Data4[0])
							<< *((DWORD*)&CurModule.PdbSig70.Data4[4]);

		SerializeStringAsANSICharArray(CurModule.ModuleName, (*SymbolFileWriter));
		SerializeStringAsANSICharArray(CurModule.ImageName, (*SymbolFileWriter));
		SerializeStringAsANSICharArray(CurModule.LoadedImageName, (*SymbolFileWriter));
	}

	// Seek to the beginning of the file and write out proper header.
	SymbolFileWriter->Seek( 0 );
	(*SymbolFileWriter) << Header;

	// Close file writers.
	SymbolFileWriter->Close();

	warnf(TEXT("FMallocProfiler: done writing file [%s]"), *(BufferedFileWriter.FullFilepath) );

	// Send the final part
	SendDataToPCViaUnrealConsole( TEXT("UE_PROFILER!MEMORY:"), *(BufferedFileWriter.FullFilepath) );
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
	QWORD FullBackTrace[MEMORY_PROFILER_MAX_BACKTRACE_DEPTH + MEMORY_PROFILER_SKIP_NUM_BACKTRACE_ENTRIES];
	appCaptureStackBackTrace( FullBackTrace, MEMORY_PROFILER_MAX_BACKTRACE_DEPTH + MEMORY_PROFILER_SKIP_NUM_BACKTRACE_ENTRIES );
	// Skip first 5 entries as they are inside the allocator.
	QWORD* BackTrace = &FullBackTrace[MEMORY_PROFILER_SKIP_NUM_BACKTRACE_ENTRIES];
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
		// Set mapping for future use.
		Index = CallStackInfoBuffer.Num();
		CRCToCallStackIndexMap.Set( CRC, Index );

		// Set up callstack info with captured call stack, translating program counters
		// to indices in program counter array (unique entries).
		FCallStackInfo CallStackInfo;
		CallStackInfo.CRC = CRC;
		for( INT i=0; i<MEMORY_PROFILER_MAX_BACKTRACE_DEPTH; i++ )
		{
			CallStackInfo.AddressIndices[i] = GetProgramCounterIndex( BackTrace[i] );
		}

		// Append to compressed buffer.
		CallStackInfoBuffer.Append( &CallStackInfo, sizeof(FCallStackInfo) );
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

/**
 * Exec handler. Parses command and returns TRUE if handled.
 *
 * @param	Cmd		Command to parse
 * @param	Ar		Output device to use for logging
 * @return	TRUE if handled, FALSE otherwise
 */
UBOOL FMallocProfiler::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	// End profiling.
	if (ParseCommand(&Cmd, TEXT("MPROF")))
	{
		if (ParseCommand(&Cmd, TEXT("START")))
		{
			if (bEndProfilingHasBeenCalled)
			{
				warnf(TEXT("FMallocProfiler: Memory recording has already been stopped and cannot be restarted."));
			}
			else
			{
				warnf(TEXT("FMallocProfiler: Memory recording is automatically started when the game is run and is still running."));
			}
		}
		else if (ParseCommand(&Cmd, TEXT("STOP")))
		{
			if (bEndProfilingHasBeenCalled)
			{
				warnf(TEXT("FMallocProfiler: Memory recording has already been stopped."));
			}
			else
			{
				warnf(TEXT("FMallocProfiler: Stopping recording."));
				EndProfiling();
			}
		}
		else if (ParseCommand(&Cmd, TEXT("MARK")) || ParseCommand(&Cmd, TEXT("SNAPSHOT")))
		{
			if (bEndProfilingHasBeenCalled == TRUE)
			{
				warnf(TEXT("FMallocProfiler: Memory recording has already been stopped.  Markers have no meaning at this point."));
			}
			else
			{
				FString SnapshotName;
				ParseToken(Cmd, SnapshotName, TRUE);

				Ar.Logf(TEXT("FMallocProfiler: Recording a snapshot marker %s"), *SnapshotName);
				SnapshotMemory(SnapshotName);
			}
		}
		else
		{
			if (bEndProfilingHasBeenCalled)
			{
				warnf(TEXT("FMallocProfiler: Status: Memory recording has been stopped."));
			}
			else
			{
				warnf(TEXT("FMallocProfiler: Status: Memory recording is ongoing."));
				warnf(TEXT("  Use MPROF MARK [FriendlyName] to insert a marker."));
				warnf(TEXT("  Use MPROF STOP to stop recording and write the recording to disk."));
			}
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("DUMPALLOCSTOFILE")) )
	{
		if( bEndProfilingHasBeenCalled == TRUE )
		{
			warnf(TEXT("FMallocProfiler: EndProfiling() has been called further actions will not be recorded please restart memory tracking process"));
			return TRUE;
		}

		warnf(TEXT("FMallocProfiler: DUMPALLOCSTOFILE"));
		EndProfiling();
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("SNAPSHOTMEMORY")) )
	{
		if( bEndProfilingHasBeenCalled == TRUE )
		{
			warnf(TEXT("FMallocProfiler: EndProfiling() has been called further actions will not be recorded please restart memory tracking process"));
			return TRUE;
		}

		FString SnapshotName;
		ParseToken(Cmd, SnapshotName, TRUE);

		Ar.Logf(TEXT("FMallocProfiler: SNAPSHOTMEMORY %s"), *SnapshotName);
		SnapshotMemory(SnapshotName);
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("SNAPSHOTMEMORYFRAME")) )
	{
		if (!bEndProfilingHasBeenCalled)
		{
			EmbedFloatMarker(SUBTYPE_FrameTimeMarker, (FLOAT)GDeltaTime);
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd,TEXT("SNAPSHOTMEMORYTEXT")) )
	{
		if (!bEndProfilingHasBeenCalled)
		{
			EmbedTextMarker( Cmd );
		}
		return TRUE;
	}

	return UsedMalloc->Exec(Cmd, Ar);
}

/**
 * Embeds token into stream to snapshot memory at this point.
 */
void FMallocProfiler::SnapshotMemory(const FString& MarkerName)
{
	FScopeLock Lock( &CriticalSection );

	if( bShouldTrackOperations )
	{
		// Write marker snapshot to stream.
		FProfilerOtherInfo SnapshotMarker;
		SnapshotMarker.DummyPointer	= TYPE_Other;
		SnapshotMarker.SubType		= SUBTYPE_SnapshotMarker;
		SnapshotMarker.Payload		= GetNameTableIndex(MarkerName);
		BufferedFileWriter << SnapshotMarker;
	}
}

/**
 * Embeds token into stream to snapshot memory at this point.
 */
void FMallocProfiler::EmbedFloatMarker(EProfilingPayloadSubType SubType, FLOAT DeltaTime)
{
	FScopeLock Lock( &CriticalSection );

	if( bShouldTrackOperations )
	{
		union { FLOAT f; DWORD ui; } TimePacker;
		TimePacker.f = DeltaTime;

		// Write marker snapshot to stream.
		FProfilerOtherInfo SnapshotMarker;
		SnapshotMarker.DummyPointer	= TYPE_Other;
		SnapshotMarker.SubType = SubType;
		SnapshotMarker.Payload = TimePacker.ui;
		BufferedFileWriter << SnapshotMarker;
	}
}

/**
 * Embeds token into stream to snapshot memory at this point.
 */
void FMallocProfiler::EmbedDwordMarker(EProfilingPayloadSubType SubType, DWORD Info)
{
	FScopeLock Lock( &CriticalSection );

	if( bShouldTrackOperations && Info != 0 )
	{
		// Write marker snapshot to stream.
		FProfilerOtherInfo SnapshotMarker;
		SnapshotMarker.DummyPointer	= TYPE_Other;
		SnapshotMarker.SubType = SubType;
		SnapshotMarker.Payload = Info;
		BufferedFileWriter << SnapshotMarker;
	}
}

/**
 * Embeds token into stream referencing passed in text.
 * 
 * @param	Text	Text to embed reference to into token stream
 */
void FMallocProfiler::EmbedTextMarker(const FString& Text)
{
	FScopeLock Lock( &CriticalSection );

	if( bShouldTrackOperations )
	{
		// Write marker snapshot to stream.
		FProfilerOtherInfo TextMarker;
		TextMarker.DummyPointer	= TYPE_Other;
		TextMarker.SubType = SUBTYPE_TextMarker;
		TextMarker.Payload = GetNameTableIndex(Text);
		BufferedFileWriter << TextMarker;
	}
}

/** 
 * Checks whether file is too big and will create new files with different extension but same base name.
 */
void FMallocProfiler::PossiblySplitMprof()
{
	// Nothing to do if we haven't created a file write yet. This happens at startup as GFileManager is initialized after
	// quite a few allocations.
	if( !BufferedFileWriter.IsBufferingToMemory() )
	{
		const INT CurrentSize = BufferedFileWriter.Tell();

		// Create a new file if current one exceeds 1 GByte.
		#define SIZE_OF_DATA_FILE 1 * 1024 * 1024 * 1024

		if (CurrentSize > SIZE_OF_DATA_FILE) 
		{
			warnf( TEXT("FMallocProfiler: Splitting recording into a new file.") );
			BufferedFileWriter.Split();
		}
	}
}




/*=============================================================================
	FMallocProfilerBufferedFileWriter implementation
=============================================================================*/

/**
 * Constructor. Called before GMalloc is initialized!!!
 */
FMallocProfilerBufferedFileWriter::FMallocProfilerBufferedFileWriter()
:	FileWriter( NULL )
,	FileNumber( 0 )
{
	ArIsSaving		= TRUE;
	ArIsPersistent	= TRUE;
	BaseFilePath = TEXT("");
}

/**
 * Destructor, cleaning up FileWriter.
 */
FMallocProfilerBufferedFileWriter::~FMallocProfilerBufferedFileWriter()
{
	if( FileWriter )
	{
		delete FileWriter;
		FileWriter = NULL;
	}
}

/**
 * Whether we are currently buffering to memory or directly writing to file.
 *
 * @return	TRUE if buffering to memory, FALSE otherwise
 */
UBOOL FMallocProfilerBufferedFileWriter::IsBufferingToMemory()
{
	return FileWriter == NULL;
}

/**
 * Splits the file and creates a new one with different extension.
 */
void FMallocProfilerBufferedFileWriter::Split()
{
	// Increase file number.
	FileNumber++;

	// Delete existing file writer. A new one will be automatically generated
	// the first time further data is being serialized.
	if( FileWriter )
	{
		// Serialize the end-of-file marker
		FProfilerOtherInfo EndOfFileToken;
		EndOfFileToken.DummyPointer	= TYPE_Other;
		EndOfFileToken.SubType = SUBTYPE_EndOfFileMarker;
		EndOfFileToken.Payload = 0;
		Serialize(&EndOfFileToken, sizeof(EndOfFileToken));

		FullFilepath = "";
		delete FileWriter;
		FileWriter = NULL;

		// Copy the file over to the PC if it's not the first file
		// (the first one gets copied over when the recording is done)
		if (FileNumber > 1)
		{
			FString PartFilename = BaseFilePath + FString::Printf(TEXT(".m%i"), FileNumber - 1);
			SendDataToPCViaUnrealConsole( TEXT("UE_PROFILER!MEMORY:"), *PartFilename );
		}
	}
}

// FArchive interface.

void FMallocProfilerBufferedFileWriter::Serialize( void* V, INT Length )
{
#if ALLOW_DEBUG_FILES
	// Copy to buffered memory array if file manager hasn't been set up yet.
	if( GFileManager == NULL || !GFileManager->IsInitialized() )
	{
		const INT Index = BufferedData.Add( Length );
		appMemcpy( &BufferedData(Index), V, Length );
	} 
	// File manager is set up but we haven't created file writer yet, do it now.
	else if( FileWriter == NULL )
	{
		// Get the base path (only used once to prevent the system time from changing for multi-file-captures
		if (BaseFilePath == TEXT(""))
		{
			const FString SysTime = appSystemTimeString();
			BaseFilePath = appProfilingDir() + GGameName + TEXT("-") + SysTime + PATH_SEPARATOR + GGameName;
		}
			
		// Create file writer to serialize data to HDD.
		if( FullFilepath.GetBaseFilename() == TEXT("") )
		{
			// Use .mprof extension for first file.
			if( FileNumber == 0 )
			{
				FullFilepath = BaseFilePath + TEXT(".mprof");
			}
			// Use .mX extension for subsequent files.
			else
			{
				FullFilepath = BaseFilePath + FString::Printf(TEXT(".m%i"),FileNumber);
			}
		}

		GFileManager->MakeDirectory( *BaseFilePath );
		FileWriter = GFileManager->CreateFileWriter( *FullFilepath, FILEWRITE_NoFail );
		checkf( FileWriter );

		// Serialize existing buffered data and empty array.
		FileWriter->Serialize( BufferedData.GetData(), BufferedData.Num() );
		BufferedData.Empty();
	}

	// Serialize data to HDD via FileWriter if it already has been created.
	if( FileWriter )
	{
		FileWriter->Serialize( V, Length );
	}
#endif
}

void FMallocProfilerBufferedFileWriter::Seek( INT InPos )
{
	check( FileWriter );
	FileWriter->Seek( InPos );
}

UBOOL FMallocProfilerBufferedFileWriter::Close()
{
	check( FileWriter );

	UBOOL bResult = FileWriter->Close();

	delete FileWriter;
	FileWriter = NULL;

	return bResult;
}

INT FMallocProfilerBufferedFileWriter::Tell()
{
	check( FileWriter );
	return FileWriter->Tell();
}

#else

// Suppress linker warning "warning LNK4221: no public symbols found; archive member will be inaccessible"
INT FMallocProfilerLinkerHelper;

#endif //USE_MALLOC_PROFILER
