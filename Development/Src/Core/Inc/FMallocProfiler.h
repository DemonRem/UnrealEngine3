/*=============================================================================
	FMallocProfiler.h: Memory profiling support.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * Whether to load symbols at startup. This might result in better results for walking the stack but will
 * also severely slow down the stack walking itself.
 */
#define LOAD_SYMBOLS_FOR_STACK_WALKING 0

/*=============================================================================
	FBufferedFileWriter
=============================================================================*/

/**
 * Special buffered file writer, used to serialize data before GFileManager is initialized.
 */
class FBufferedFileWriter : public FArchive
{
public:
	/** Internal file writer used to serialize to HDD.							*/
	FArchive*		FileWriter;
	/** Buffered data being serialized before GFileManager has been set up.		*/
	TArray<BYTE>	BufferedData;
	/** Base filename to use when writing to HDD.								*/
	const TCHAR*	BaseFilename;

	/**
	 * Constructor. Called before GMalloc is initalized!!!
	 */
	FBufferedFileWriter( const TCHAR* InBaseFilename );

	/**
	 * Destructor, cleaning up FileWriter.
	 */
	virtual ~FBufferedFileWriter();

	// FArchive interface.
	virtual void Serialize( void* V, INT Length );
	virtual void Seek( INT InPos );
	virtual UBOOL Close();
	virtual INT Tell();
};

/*=============================================================================
	FMallocProfiler
=============================================================================*/

/**
 * Memory profiling malloc, routing allocations to real malloc and writing information on all 
 * operations to a file for analysis by a standalone tool.
 */
class FMallocProfiler : public FMalloc
{
private:
	/** Malloc we're based on, aka using under the hood												*/
	FMalloc*			UsedMalloc;
	/** Whether operations should be tracked. FALSE e.g. in tracking internal functions.			*/
	UBOOL				bShouldTrackOperations;
	/** Time malloc profiler was created. Used to convert arbitrary DOUBLE time to relative FLOAT.	*/
	DOUBLE				StartTime;
	/** File writer used to serialize data. Safe to call before GFileManager has been initialized.	*/
	FBufferedFileWriter BufferedFileWriter;

	/** Mapping from program counter to index in program counter array.								*/
	TMap<QWORD,INT> ProgramCounterToIndexMap;
	/** Array of unique call stack address infos.													*/
	TArray<struct FCallStackAddressInfo> CallStackAddressInfoArray;

	/** Mapping from callstack CRC to index in call stack info array.								*/
	TMap<DWORD,INT> CRCToCallStackIndexMap;
	/** Array of unique call stack infos.															*/
	TArray<struct FCallStackInfo> CallStackInfoArray;

	/** Mapping from name to index in name array.													*/
	TMap<FString,INT> NameToNameTableIndexMap;
	/** Array of unique names.																		*/
	TArray<FString> NameArray;

	/** 
	 * Returns index of callstack, captured by this function into array of callstack infos. If not found, adds it.
	 *
	 * @return index into callstack array
	 */
	INT GetCallStackIndex();

	/**
	 * Returns index of passed in program counter into program counter array. If not found, adds it.
	 *
	 * @param	ProgramCounter	Program counter to find index for
	 * @return	Index of passed in program counter if it's not NULL, INDEX_NONE otherwise
	 */
	INT GetProgramCounterIndex( QWORD ProgramCounter );

	/**
	 * Returns index of passed in name into name array. If not found, adds it.
	 *
	 * @param	Name	Name to find index for
	 * @return	Index of passed in name
	 */
	INT GetNameTableIndex( const FString& Name );

	/**
	 * Tracks malloc operation.
	 *
	 * @param	Ptr		Allocated pointer 
	 * @param	Size	Size of allocated pointer
	 */
	void TrackMalloc( void* Ptr, DWORD Size );
	
	/**
	 * Tracks free operation
	 *
	 * @param	Ptr		Freed pointer
	 */
	void TrackFree( void* Ptr );
	
	/**
	 * Tracks realloc operation
	 *
	 * @param	OldPtr	Previous pointer
	 * @param	NewPtr	New pointer
	 * @param	NewSize	New size of allocation
	 */
	void TrackRealloc( void* OldPtr, void* NewPtr, DWORD NewSize );

	/**
	 * Ends profiling operation and closes file.
	 */
	void EndProfiling();

public:
	/**
	 * Constructor, initializing all member variables and potentially loading symbols.
	 *
	 * @param	InMalloc	The allocator wrapped by FMallocProfiler that will actually do the allocs/deallocs.
	 */
	FMallocProfiler(FMalloc* InMalloc);

	void* Malloc( DWORD Size, DWORD Alignment )
	{
		void* Ptr = UsedMalloc->Malloc( Size, Alignment );
		TrackMalloc( Ptr, Size );
		return Ptr;
	}
	void* Realloc( void* OldPtr, DWORD NewSize, DWORD Alignment )
	{
		void* NewPtr = UsedMalloc->Realloc( OldPtr, NewSize, Alignment );
		TrackRealloc( OldPtr, NewPtr, NewSize );
		return NewPtr;
	}
	void Free( void* Ptr )
	{
		UsedMalloc->Free( Ptr );
		TrackFree( Ptr );
	}
	void* PhysicalAlloc( DWORD Size, ECacheBehaviour InCacheBehaviour )
	{
		void* Ptr = UsedMalloc->PhysicalAlloc( Size, InCacheBehaviour );
		TrackMalloc( Ptr, Size );
		return Ptr;
	}
	void PhysicalFree( void* Ptr )
	{
		UsedMalloc->PhysicalFree( Ptr );
		TrackFree( Ptr );
	}

	/**
	 * Passes request for gathering memory allocations for both virtual and physical allocations
	 * on to used memory manager.
	 *
	 * @param Virtual	[out] size of virtual allocations
	 * @param Physical	[out] size of physical allocations	
	 */
	void GetAllocationInfo( SIZE_T& Virtual, SIZE_T& Physical )
	{
		UsedMalloc->GetAllocationInfo( Virtual, Physical );
	}

	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		if( ParseCommand(&Cmd,TEXT("DUMPALLOCS")) )
		{
			EndProfiling();
			return TRUE;
		}
		return UsedMalloc->Exec(Cmd, Ar);
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
