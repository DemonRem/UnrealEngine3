/*=============================================================================
	FMallocDebugProxyWindows.h: Proxy allocator for logging backtrace of 
	                            allocations.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * Whether to load symbols at startup. This might result in better results for walking the stack but will
 * also severely slow down the stack walking itself.
 */
#define LOAD_SYMBOLS_FOR_STACK_WALKING 0

/**
 * Debug memory proxy that captures stack backtraces while passing allocations through
 * to the FMalloc passed in via the constructor.
 */
class FMallocDebugProxyWindows : public FMalloc
{
private:
	//@todo: "static const int" doesn't compile in release mode for some obscure reason.
	/** Maximum depth of stack backtrace										*/
	#define	MAX_BACKTRACE_DEPTH	20

	/** Malloc we're based on, aka using under the hood							*/
	FMalloc*			UsedMalloc;

	/**	Whether we are currently dumping or not									*/
	UBOOL				bCurrentlyDumping;

	/**
	 * Debug information kept for each allocation... Consumes A LOT of memory!
	 */
	class FAllocInfo
	{
	public:
		/** Base address of memory allocation									*/
		void*				BaseAddress;
		/** Size of memory allocation											*/
		DWORD				Size;
		/** Stack backtrace, MAX_BACKTRACE_DEPTH levels deep					*/
		DWORD64				BackTrace[MAX_BACKTRACE_DEPTH];
		/** CRC of BackTrace, used for output and grouping allocations			*/
		DWORD				CRC;
		/** Thread Id used for tracking which thread an allocation came from	*/
		DWORD				ThreadId;

		/** Next allocation in pool												*/
		FAllocInfo*			Next;
		/** Previous allocation in pool											*/
		FAllocInfo*			Previous;

		/**
		 * Links current allocation info to pool. Note that we can only be linked to one pool at a time.
		 *
		 * @param Pool FAllocInfo "this" is going to be linked to.
		 */
		void LinkPool( FAllocInfo*& Pool )
		{
			if( Pool )
			{
				Next		= Pool->Next;
				Previous	= Pool;
				if( Pool->Next )
				{
                    Pool->Next->Previous = this;
				}
				Pool->Next = this;
			}
			else
			{
				Pool		= this;
				Next		= NULL;
				Previous	= NULL;
			}
		}

		/**
		 * Unlinks current allocation info from pool. Note that we can only be linked to one pool at a time.
		 *
		 * @param Pool FAllocInfo "this" is going to be unlinked from.
		 */
		void UnlinkPool( FAllocInfo*& Pool )
		{
			if( Previous )
			{
				Previous->Next = Next;
			}

			if( Next )
			{
				Next->Previous = Previous;
			}

			if( !Previous && !Next )
			{
				Pool = NULL;
			}

			if( !Previous && Next )
			{
				Pool = Next;
			}
		}
	};

	/** Array of allocation infos - one pool per each pointer & 0xFFFF			*/
	FAllocInfo*		AllocationPool[0xFFFF];

	/** Whether we are currently walking the stack */
	UBOOL			bCurrentlyWalkingStack;
	/** Whether we are currently tracking allocations */
	UBOOL			bIsTracking;

	/** @name Internal stack-walking methods */

	/**
	 * Captures stack backtrace for this allocation and also keeps track of pointer & size.
	 *
	 * @param	Ptr		pointer to keep track off	
	 * @param	Size	size of this allocation
	 */
	void LogBacktrace( void* Ptr, DWORD Size )
	{
		// Don't bother with allocations that occur during stack walking, or if we are disabled
		if( bCurrentlyWalkingStack || bCurrentlyDumping )
		{
			return;
		}

		// Create a new FAllocInfo object.
		FAllocInfo* Allocation	= (FAllocInfo*) appSystemMalloc( sizeof(FAllocInfo) );
		check( Allocation );
		appMemzero(Allocation, sizeof(FAllocInfo));
		Allocation->BaseAddress	= Ptr;
		Allocation->Size		= Size;
		Allocation->ThreadId	= GetCurrentThreadId();

		// Walk the stack.
		bCurrentlyWalkingStack	= 1;
		appCaptureStackBackTrace( Allocation->BackTrace, MAX_BACKTRACE_DEPTH );
		bCurrentlyWalkingStack	= 0;

		// Use the lower 16 bits of the pointer as an index to a pool and link it to it.
		Allocation->LinkPool( AllocationPool[(DWORD)Ptr & 0xFFFF] );
	}

	void InternalMalloc( void* Ptr, DWORD Size )
	{
		if( bIsTracking )
		{
			LogBacktrace( Ptr, Size );
		}
	}

	/**
	 * Removes pointer from internal allocation pool.
	 *
	 * @param	Ptr		pointer to free	
	 */
	void InternalFree( void* Ptr )
	{
		// Don't bother with allocations that occur during stack walking, or if we are disabled
		if ( bCurrentlyWalkingStack || !bIsTracking || bCurrentlyDumping )
		{
			return;
		}

		// Delete NULL is legal ANSI C++.
		if( Ptr == NULL )
		{
			return;
		}

		// Try to find allocation in pool indexed by the lower 16 bits of the passed in pointer.
		FAllocInfo* Allocation = AllocationPool[(DWORD)Ptr & 0xFFFF];
		while( Allocation && ( Allocation->BaseAddress != Ptr ) )
		{
			Allocation = Allocation->Next;
		}

		// Gracefully handle double free'ing and not handled allocations e.g. caused by using Start/StopTracking.
		if( !Allocation )
		{
			return;
		}

		// Unlink from pool and free allocation info.
		check( Allocation->BaseAddress == Ptr );
		Allocation->UnlinkPool( AllocationPool[(DWORD)Ptr & 0xFFFF] );
		appSystemFree( Allocation );
	}

public:
	/**
	 * @param	InMalloc	The allocator wrapped by FMallocDebugProxyWindows that will actually do the allocs/deallocs.
	 */
	FMallocDebugProxyWindows(FMalloc* InMalloc)
		:	UsedMalloc( InMalloc ),
			bCurrentlyDumping( FALSE ),
			bCurrentlyWalkingStack( FALSE ),
			bIsTracking(START_BACKTRACE_ON_LAUNCH)
	{}

	void* Malloc( DWORD Size, DWORD Alignment )
	{
		void* Ptr = UsedMalloc->Malloc( Size, Alignment );
		InternalMalloc( Ptr, Size );
		return Ptr;
	}
	void* Realloc( void* Ptr, DWORD NewSize, DWORD Alignment )
	{
		void* NewPtr = UsedMalloc->Realloc( Ptr, NewSize, Alignment );
		InternalFree( Ptr );

		if( NewPtr )
		{
			InternalMalloc( NewPtr, NewSize );
		}
		return NewPtr;
	}
	void Free( void* Ptr )
	{
		UsedMalloc->Free( Ptr );
		InternalFree( Ptr );
	}
	void* PhysicalAlloc( DWORD Size, ECacheBehaviour InCacheBehaviour )
	{
		void* Ptr = UsedMalloc->PhysicalAlloc( Size, InCacheBehaviour );
		InternalMalloc( Ptr, Size );
		return Ptr;
	}
	void PhysicalFree( void* Ptr )
	{
		UsedMalloc->PhysicalFree( Ptr );
		InternalFree( Ptr );
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
	/** Track all mallocs from the current thread */
	void BeginTrackingThread( void ) 
	{
		UsedMalloc->BeginTrackingThread();
	}
	/** Stop tracking all mallocs from a given thread */
	void EndTrackingThread( void ) 
	{
		UsedMalloc->EndTrackingThread();
	}
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		if( ParseCommand(&Cmd,TEXT("STARTTRACKING")) )
		{
			StartTracking();
			return TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("STOPTRACKING")) )
		{
			StopTracking();
			return TRUE;
		}
		return UsedMalloc->Exec(Cmd, Ar);
	}

private:
	/** Stats for allocations with same call stack */
	struct FGroupedAllocationInfo
	{
		/** Human readable callstack, separated by comma										*/
		FString		Callstack;
		/** Total size of allocations in this group (aka allocations with the same callstack)	*/
		SIZE_T		TotalSize;
		/** Number of allocations in this group													*/
		DWORD		Count;
		/** ThreadId where this malloc came from												*/
		DWORD		ThreadId;
	};

public:
	void StartTracking()
	{
		bIsTracking = TRUE;
	}

	void StopTracking()
	{
		bIsTracking = FALSE;
	}

	void WriteGroupedCSVFile( TCHAR *Name, TMap<DWORD, FGroupedAllocationInfo>& GroupedAllocations )
	{
		// CSV: Human-readable spreadsheet format.
		FString		CSVFilename = FString::Printf(TEXT("%s%s-%s-%i-%s.csv"), *appGameLogDir(), Name, GGameName, GEngineVersion, *appSystemTimeString() );	
		FArchive*	CSVFile		= GFileManager->CreateFileWriter( *CSVFilename );
		if( CSVFile )
		{	
			// print out headers for each column
			FString AllocationInfoText = FString::Printf( TEXT("ThreadID,TotalSize,Count,AvgAlloc,CallStack%s"), LINE_TERMINATOR );
			CSVFile->Serialize( TCHAR_TO_ANSI(*AllocationInfoText), AllocationInfoText.Len() );

			// Iterate over all allocations outputting a csv of size, count and callstack.
			for( TMap<DWORD, FGroupedAllocationInfo>::TConstIterator It(GroupedAllocations) ; It ; ++It )
			{
				const FGroupedAllocationInfo& GroupedInfo = It.Value();	
				FString AllocationInfoText = FString::Printf( TEXT("0x%08x,%i,%i,%i,%s%s"), GroupedInfo.ThreadId, GroupedInfo.TotalSize, GroupedInfo.Count, GroupedInfo.TotalSize / GroupedInfo.Count, *GroupedInfo.Callstack, LINE_TERMINATOR );
				CSVFile->Serialize( TCHAR_TO_ANSI(*AllocationInfoText), AllocationInfoText.Len() );
			}

			// Close and delete archive.
			CSVFile->Close();
			delete CSVFile;
		}
		else
		{
			debugf(NAME_Warning,TEXT("Could not create CSV file %s for writing."), *CSVFilename);
		}
	}

	void CollateBacktraces( TMap<DWORD, FGroupedAllocationInfo>& GroupedAllocations, UBOOL Summate, SIZE_T& TotalNum, SIZE_T& TotalSize )
	{
		TotalNum = 0;
		TotalSize = 0;

		// Iterate over all pools (16 lower bits of pointer are index into pool).
		for( INT i = 0 ; i < 0xFFFF ; ++i )
		{
			FAllocInfo* AllocInfo = AllocationPool[i];
			while( AllocInfo )
			{
				TotalSize += AllocInfo->Size;

				// Create unique identifier based on call stack.
				AllocInfo->CRC = appMemCrc( AllocInfo->BackTrace, sizeof(AllocInfo->BackTrace), 0 );

				// Group the allocation into the GroupedAllocations map.
				FGroupedAllocationInfo* GroupedInfo = GroupedAllocations.Find( AllocInfo->CRC );
				if( GroupedInfo )
				{
					// Allocation already exists; accumulate stats.
					if( Summate )
					{
						GroupedInfo->TotalSize += AllocInfo->Size;
					}
					GroupedInfo->Count++;
				}
				else
				{
					FString CallStack;

					// Retrieve human readable callstack information.
					INT Depth = 5;
					for( ; Depth<MAX_BACKTRACE_DEPTH; Depth++ )
					{
						const DWORD64 ProgramCounter = AllocInfo->BackTrace[Depth];

						// Null addresses indicate top of stack.
						if( !ProgramCounter )
						{
							if( Depth == 0 )
							{
								CallStack = TEXT("NO INFORMATION");
							}
							else
							{
								CallStack += TEXT("DONE");
							}
							break;
						}
						else
						{
							// Stack evaluation allows 512 chars for the symbol, 1024 for the filename and 1024 for the module name
							ANSICHAR	CallStackEntry[4096];

							// Convert program counter to human readable string.
							CallStackEntry[0] = 0;
							appProgramCounterToHumanReadableString( ProgramCounter, CallStackEntry, VF_DISPLAY_FILENAME );
							CallStack += TEXT("\"");
							CallStack += FString(CallStackEntry);
							CallStack += TEXT("\",");
						}
					}

					if ( Depth == MAX_BACKTRACE_DEPTH )
					{
						CallStack += TEXT("MORE NOT DISPLAYED");
					}

					// Add this allocation to the map.
					FGroupedAllocationInfo NewGroupedInfo;
					NewGroupedInfo.Callstack	= CallStack;
					NewGroupedInfo.TotalSize	= AllocInfo->Size;
					NewGroupedInfo.ThreadId		= AllocInfo->ThreadId;
					NewGroupedInfo.Count		= 1;

					GroupedAllocations.Set( AllocInfo->CRC, NewGroupedInfo );

					++TotalNum;
				}
				AllocInfo = AllocInfo->Next;
			}
		}
	}

	void DumpAllocs( UBOOL bSummaryOnly, FOutputDevice& Ar )
	{
		SIZE_T TotalSize, TotalNum;
	
		bCurrentlyDumping = TRUE;

		GFileManager->SetDefaultDirectory();

		// Group allocations by callstack (or rather CRC of return addresses).
		TMap<DWORD,FGroupedAllocationInfo> GroupedAllocations;

		// Collate the AllocationPool data into something more readable
		CollateBacktraces( GroupedAllocations, TRUE, TotalNum, TotalSize );

		// Write collated info to CSV file
		WriteGroupedCSVFile( TEXT( "MemoryAllocations" ), GroupedAllocations );

		// Dump summary to log.
		Ar.Logf(TEXT(""));
		Ar.Logf(TEXT("Total size of tracked allocations: %.3fM"), TotalSize / 1024.f / 1024.f );
		Ar.Logf(TEXT("Number of allocations: %i grouped, %i total"), GroupedAllocations.Num(), TotalNum );
		Ar.Logf(TEXT(""));

		bCurrentlyDumping = FALSE;
	}

	void DumpBacktraces( void )
	{
		SIZE_T TotalSize, TotalNum;

		bCurrentlyDumping = TRUE;

		GFileManager->SetDefaultDirectory();

		// Group backtraces by CRC of return addresses
		TMap<DWORD,FGroupedAllocationInfo> GroupedAllocations;

		// Collate the AllocationPool data into something more readable
		CollateBacktraces( GroupedAllocations, FALSE, TotalNum, TotalSize );

		// Write collated info to CSV file
		WriteGroupedCSVFile( TEXT( "BacktraceDump" ), GroupedAllocations );

		// Dump summary to log.
		debugf(TEXT(""));
		debugf(TEXT("Number of groups: %i (%i total)"), GroupedAllocations.Num(), TotalNum );
		debugf(TEXT(""));

		bCurrentlyDumping = FALSE;
	}

	void HeapCheck()
	{
		UsedMalloc->HeapCheck();
	}

	void Init()
	{
		// Initialize variables for stack walking.
		appMemzero( AllocationPool, sizeof(AllocationPool) );

#if LOAD_SYMBOLS_FOR_STACK_WALKING
		// Initialize symbols.
		appInitStackWalking();
#endif

		// Route init to underlying malloc implementation.
		UsedMalloc->Init();
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
