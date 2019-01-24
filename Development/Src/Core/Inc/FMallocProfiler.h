/*=============================================================================
	FMallocProfiler.h: Memory profiling support.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#ifndef FMALLOC_PROFILER_H
#define FMALLOC_PROFILER_H

#include "UMemoryDefines.h"

#if USE_MALLOC_PROFILER

/*=============================================================================
	FBufferedFileWriter
=============================================================================*/

enum EProfilingPayloadSubType
{
	SUBTYPE_EndOfStreamMarker	= 0,
	SUBTYPE_EndOfFileMarker		= 1,
	SUBTYPE_SnapshotMarker		= 2,
	SUBTYPE_FrameTimeMarker		= 3,
	SUBTYPE_TextMarker			= 4,

	SUBTYPE_TotalUsed			= 5,
	SUBTYPE_TotalAllocated		= 6,

	SUBTYPE_CPUUsed				= 7,
	SUBTYPE_CPUSlack			= 8,
	SUBTYPE_CPUWaste			= 9,

	SUBTYPE_GPUUsed				= 10,
	SUBTYPE_GPUSlack			= 11,
	SUBTYPE_GPUWaste			= 12,

	SUBTYPE_OSOverhead			= 13,
	SUBTYPE_ImageSize			= 14,
};

/**
 * Special buffered file writer, used to serialize data before GFileManager is initialized.
 */
class FMallocProfilerBufferedFileWriter : public FArchive
{
public:
	/** Internal file writer used to serialize to HDD. */
	FArchive*		FileWriter;
	/** Buffered data being serialized before GFileManager has been set up.	*/
	TArray<BYTE>	BufferedData;
	/** Timestamped filename with path.	*/
	FFilename		FullFilepath;
	/** Timestamped file path for the memory captures, just add extension. */
	FString			BaseFilePath;
	/** File number. Index 0 is the base .mprof file. */
	INT				FileNumber;

	/**
	 * Constructor. Called before GMalloc is initialized!!!
	 */
	FMallocProfilerBufferedFileWriter();

	/**
	 * Destructor, cleaning up FileWriter.
	 */
	virtual ~FMallocProfilerBufferedFileWriter();

	/**
	 * Whether we are currently buffering to memory or directly writing to file.
	 *
	 * @return	TRUE if buffering to memory, FALSE otherwise
	 */
	UBOOL IsBufferingToMemory();

	/**
	 * Splits the file and creates a new one with different extension.
	 */
	void Split();

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
	FMalloc*								UsedMalloc;
	/** Whether operations should be tracked. FALSE e.g. in tracking internal functions.			*/
	UBOOL									bShouldTrackOperations;
	/** Whether or not EndProfiling()  has been Ended.  Once it has been ended it has ended most operations are no longer valid **/
	UBOOL									bEndProfilingHasBeenCalled;
	/** Time malloc profiler was created. Used to convert arbitrary DOUBLE time to relative FLOAT.	*/
	DOUBLE									StartTime;
	/** File writer used to serialize data. Safe to call before GFileManager has been initialized.	*/
	FMallocProfilerBufferedFileWriter		BufferedFileWriter;

	/** Critical section to sequence tracking.														*/
	FCriticalSection						CriticalSection;

	/** Mapping from program counter to index in program counter array.								*/
	TMap<QWORD,INT>							ProgramCounterToIndexMap;
	/** Array of unique call stack address infos.													*/
	TArray<struct FCallStackAddressInfo>	CallStackAddressInfoArray;

	/** Mapping from callstack CRC to offset in call stack info buffer.								*/
	TMap<DWORD,INT>							CRCToCallStackIndexMap;
	/** Buffer of unique call stack infos.															*/
	FCompressedGrowableBuffer				CallStackInfoBuffer;

	/** Mapping from name to index in name array.													*/
	TMap<FString,INT>						NameToNameTableIndexMap;
	/** Array of unique names.																		*/
	TArray<FString>							NameArray;
	/** Simple count of memory operations															*/
	QWORD									MemoryOperationCount;

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
	 * Tracks slack, waste and anything else relevant in the allocator (once every 1024 memory operations)
	 */
	void TrackSpecialMemory();

	/**
	 * Begins profiling operation and opens file.
	 */
	void BeginProfiling();

	/**
	 * Ends profiling operation and closes file.
	 */
	void EndProfiling();

	/**
	 * Embeds token into stream to snapshot memory at this point.
	 */
	void SnapshotMemory(const FString& MarkerName);

	/**
	 * Embeds float token into stream (e.g. delta time).
	 */
	void EmbedFloatMarker(EProfilingPayloadSubType SubType, FLOAT DeltaTime);

	/**
	 * Embeds token into stream to snapshot memory at this point.
	 */
	void EmbedDwordMarker(EProfilingPayloadSubType SubType, DWORD Info);

	/**
	 * Embeds token into stream referencing passed in text.
	 * 
	 * @param	Text	Text to embed reference to into token stream
	 */
	void EmbedTextMarker(const FString& Text);

	/** 
	 * Checks whether file is too big and will create new files with different extension but same base name.
	 */
	void PossiblySplitMprof();

public:
	/**
	 * Constructor, initializing all member variables and potentially loading symbols.
	 *
	 * @param	InMalloc	The allocator wrapped by FMallocProfiler that will actually do the allocs/deallocs.
	 */
	FMallocProfiler(FMalloc* InMalloc);

	/** 
	 * QuantizeSize returns the actual size of allocation request likely to be returned
	 * so for the template containers that use slack, they can more wisely pick
	 * appropriate sizes to grow and shrink to.
	 *
	 * CAUTION: QuantizeSize is a special case and is NOT guarded by a thread lock, so must be intrinsically thread safe!
	 *
	 * @param Size			The size of a hypothetical allocation request
	 * @param Alignment		The alignment of a hypothetical allocation request
	 * @return				Returns the usable size that the allocation request would return. In other words you can ask for this greater amount without using any more actual memory.
	 */
	virtual DWORD QuantizeSize( DWORD Size, DWORD Alignment )
	{
		return UsedMalloc->QuantizeSize(Size,Alignment); 
	}

	/** 
	 * Malloc
	 */
	virtual void* Malloc( DWORD Size, DWORD Alignment )
	{
		FScopeLock Lock( &CriticalSection );
		void* Ptr = UsedMalloc->Malloc( Size, Alignment );
		TrackMalloc( Ptr, Size );
		TrackSpecialMemory();
		return Ptr;
	}

	/** 
	 * Realloc
	 */
	virtual void* Realloc( void* OldPtr, DWORD NewSize, DWORD Alignment )
	{
		FScopeLock Lock( &CriticalSection );
		void* NewPtr = UsedMalloc->Realloc( OldPtr, NewSize, Alignment );
		TrackRealloc( OldPtr, NewPtr, NewSize );
		TrackSpecialMemory();
		return NewPtr;
	}

	/** 
	 * Free
	 */
	virtual void Free( void* Ptr )
	{
		FScopeLock Lock( &CriticalSection );
		UsedMalloc->Free( Ptr );
		TrackFree( Ptr );
		TrackSpecialMemory();
	}

	/** 
	 * Physical alloc
	 */
	virtual void* PhysicalAlloc( DWORD Size, ECacheBehaviour InCacheBehaviour )
	{
		FScopeLock Lock( &CriticalSection );
		void* Ptr = UsedMalloc->PhysicalAlloc( Size, InCacheBehaviour );
		TrackMalloc( Ptr, Size );
		TrackSpecialMemory();
		return Ptr;
	}

	/** 
	 * Physical free
	 */
	virtual void PhysicalFree( void* Ptr )
	{
		FScopeLock Lock( &CriticalSection );
		UsedMalloc->PhysicalFree( Ptr );
		TrackFree( Ptr );
		TrackSpecialMemory();
	}

	/**
	 * Returns if the allocator is guaranteed to be thread-safe and therefore
	 * doesn't need a unnecessary thread-safety wrapper around it.
	 */
	virtual UBOOL IsInternallyThreadSafe() const 
	{ 
		return TRUE; 
	}

	/**
	 * Passes request for gathering memory allocations for both virtual and physical allocations
	 * on to used memory manager.
	 *
	 * @param FMemoryAllocationStats	[out] structure containing information about the size of allocations
	 */
	virtual void GetAllocationInfo( FMemoryAllocationStats& MemStats )
	{
		FScopeLock Lock( &CriticalSection );
		UsedMalloc->GetAllocationInfo( MemStats );
	}

	/**
	 * Dumps details about all allocations to an output device
	 *
	 * @param Ar	[in] Output device
	 */
	virtual void DumpAllocations( class FOutputDevice& Ar ) 
	{
		FScopeLock Lock( &CriticalSection );
		UsedMalloc->DumpAllocations( Ar );
	}

	/**
	 * Validates the allocator's heap
	 */
	virtual UBOOL ValidateHeap()
	{
		FScopeLock Lock( &CriticalSection );
		return( UsedMalloc->ValidateHeap() );
	}

	/**
	 * Exec handler. Parses command and returns TRUE if handled.
	 *
	 * @param	Cmd		Command to parse
	 * @param	Ar		Output device to use for logging
	 * @return	TRUE if handled, FALSE otherwise
	 */
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar );

	/** Called every game thread tick */
	virtual void Tick( FLOAT DeltaTime ) 
	{ 
		FScopeLock Lock( &CriticalSection );
		UsedMalloc->Tick(DeltaTime);
	}
};

#endif //USE_MALLOC_PROFILER

#endif	//#ifndef FMALLOC_PROFILER_H
