/*=============================================================================
	FMallocDebugProxy.h: Proxy allocator for logging backtrace of 
							  allocations.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/** Whether to use the call stack capturing malloc proxy */
#define KEEP_ALLOCATION_BACKTRACE	0
/** Whether to start with capture enabled */
#define START_BACKTRACE_ON_LAUNCH	0

// make sure this code can't be used unless explicitly enabled above
#if KEEP_ALLOCATION_BACKTRACE


/** The number of internal allocation calls to strip off the top of every back-trace. */
#define BASE_BACKTRACE_TRACK_DEPTH 6
/** The number of allocations to track, not counting the internal allocation functions */
#define MAX_BACKTRACE_TRACK_DEPTH 10

/**
 * Debug memory proxy that captures stack backtraces while passing allocations through
 * to the FMalloc passed in via the constructor.
 */
class FMallocDebugProxy : public FMalloc
{

private:
	/** Whether we are current tracking memory allocations */
	UBOOL				bIsTracking;
	/** Whether we are in the process of sending a memory operation string to the host */
	UBOOL				bSendingCommand;

	/** Malloc we're based on, aka using under the hood */
	FMalloc*			UsedMalloc;

	int					MemoryAllocations;
	int					MemoryFrees;

	FCriticalSection*	BufferSection;

private:

	/**
	 * If this is much larger (I think about 1444 bytes), the string is occasionally truncated somewhere between DmSendNotificationString and UnrealWatson!
	 */
	ANSICHAR PendingCommandString[1024];
	UINT PendingCommandLength;

	void FlushPendingCommands()
	{
		if(PendingCommandLength)
		{
			// Append an end message token.
			PendingCommandString[PendingCommandLength++] = '$';

			// Ensure that the command is null terminated.  This may be necessary if the formatted print runs out of space.
			PendingCommandString[PendingCommandLength] = '\0';

			// send the command over the network to anyone listening
			if (GDebugChannel)
			{
				GDebugChannel->SendText("MEM", PendingCommandString);
			}

			PendingCommandLength = 0;
		}
	}

	void AddBufferedCommand(const ANSICHAR* Format,...)
	{
		// we can't do anything without a critical section to protect us!
		if (BufferSection == NULL)
		{
			if (GSynchronizeFactory == NULL)
			{
				return;
			}
			else
			{
				BufferSection = GSynchronizeFactory->CreateCriticalSection();

			}
		}

		FScopeLock ScopeLock(BufferSection);

		INT CommandLength = INDEX_NONE;
		while(TRUE)
		{
			const INT RemainingBufferSpace = 512/*ARRAY_COUNT(PendingCommandString)*/ - PendingCommandLength - 2;
			va_list ArgList;
			va_start(ArgList,Format);
			if(RemainingBufferSpace > 0)
			{
				CommandLength = appGetVarArgsAnsi(&PendingCommandString[PendingCommandLength],RemainingBufferSpace+2,RemainingBufferSpace,Format,ArgList);
			}

			// CommandLength shouldn't be greater than Remaining, but just in case...
			if (RemainingBufferSpace <= 0 || CommandLength == INDEX_NONE || CommandLength >= RemainingBufferSpace)
			{
				FlushPendingCommands();
			}
			else
			{
				break;
			}
		};

		PendingCommandLength += CommandLength;
	}

	void SendMallocCommand( DWORD Ptr, DWORD Size )
	{
		bSendingCommand = TRUE;

		// If there are no memory tags, capture the call stack.
		QWORD BackTrace[MAX_BACKTRACE_TRACK_DEPTH + BASE_BACKTRACE_TRACK_DEPTH];
		appMemzero(BackTrace,sizeof(BackTrace));
		DWORD NumStackCalls = appCaptureStackBackTrace(BackTrace, MAX_BACKTRACE_TRACK_DEPTH);

		// Capture the time of the allocation.
		const DWORD Time = appTrunc( appSeconds() * 1000.0 );

		// Choose a format string based on the number of calls to output.
		const ANSICHAR* FormatString;
		switch(NumStackCalls)
		{
		case 0: FormatString = "&Allocate%08x%08x%08x%08x-00000000"; break;
		case 1: FormatString = "&Allocate%08x%08x%08x%08x-%08x"; break;
		case 2: FormatString = "&Allocate%08x%08x%08x%08x-%08x%08x"; break;
		case 3: FormatString = "&Allocate%08x%08x%08x%08x-%08x%08x%08x"; break;
		case 4: FormatString = "&Allocate%08x%08x%08x%08x-%08x%08x%08x%08x"; break;
		case 5: FormatString = "&Allocate%08x%08x%08x%08x-%08x%08x%08x%08x%08x"; break;
		case 6: FormatString = "&Allocate%08x%08x%08x%08x-%08x%08x%08x%08x%08x%08x"; break;
		case 7: FormatString = "&Allocate%08x%08x%08x%08x-%08x%08x%08x%08x%08x%08x%08x"; break;
		case 8: FormatString = "&Allocate%08x%08x%08x%08x-%08x%08x%08x%08x%08x%08x%08x%08x"; break;
		case 9: FormatString = "&Allocate%08x%08x%08x%08x-%08x%08x%08x%08x%08x%08x%08x%08x%08x"; break;
		default: FormatString = "&Allocate%08x%08x%08x%08x-%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x"; break;
		};

		// Buffer a malloc command for the debug host.
		AddBufferedCommand(
			FormatString,
			Ptr,
			Time,
			Size,
			appGetCurrentThreadId(),
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 0],
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 1],
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 2],
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 3],
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 4],
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 5],
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 6],
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 7],
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 8],
			(DWORD) BackTrace[BASE_BACKTRACE_TRACK_DEPTH + 9]
			);

		MemoryAllocations++;

		bSendingCommand = FALSE;
	}

	void SendFreeCommand( DWORD Ptr )
	{
		bSendingCommand = TRUE;

		// Buffer a free command for the debug host.
		AddBufferedCommand("&Free%08x",Ptr);

		MemoryFrees++;

		bSendingCommand = FALSE;
	}

	/**
	 * Captures stack backtrace for this allocation and also keeps track of pointer & size.
	 *
	 * @param	Ptr		pointer to keep track off	
	 * @param	Size	size of this allocation
	 */
	void InternalMalloc( void* Ptr, DWORD Size )
	{
		// We don't want to waste gobs of memory if we're not keeping track of anything.
		if( !bIsTracking || bSendingCommand || !Ptr )
		{
			return;
		}

		// Ignore any memory ops inside this function
		SendMallocCommand( ( DWORD )Ptr, Size );
	}

	/**
	 * Removes pointer from internal allocation pool.
	 *
	 * @param	Ptr		pointer to free	
	 */
	void InternalFree( void* Ptr )
	{
		// delete NULL is legal ANSI C++
		if( Ptr == NULL )
		{
			return;
		}

		if( !bIsTracking || bSendingCommand )
		{
			return;
		}

		SendFreeCommand( ( DWORD )Ptr );
	}

	void InternalRealloc(void* Ptr,void* NewPtr,DWORD NewSize)
	{
		if(bIsTracking && !bSendingCommand)
		{
			bSendingCommand = TRUE;
			AddBufferedCommand("&Realloc%08x%08x%08x%08x%08x",(DWORD)Ptr,(DWORD)NewPtr,NewSize,appTrunc( appSeconds() * 1000.0 ), appGetCurrentThreadId());
			bSendingCommand = FALSE;
		}
	}

public:
	// FMalloc interface.
	FMallocDebugProxy( FMalloc* InMalloc )
		: bIsTracking( START_BACKTRACE_ON_LAUNCH )
		, bSendingCommand(FALSE)
		, UsedMalloc( InMalloc )
		, MemoryAllocations(0)
		, MemoryFrees(0)
		, BufferSection(NULL)
		, PendingCommandLength(0)
	{
	}

	virtual void* Malloc( DWORD Size, DWORD Alignment )
	{
		void* Ptr = UsedMalloc->Malloc( Size, Alignment );
		InternalMalloc( Ptr, Size );
		return Ptr;
	}
	
	virtual void* Realloc( void* Ptr, DWORD NewSize, DWORD Alignment )
	{
		void* NewPtr = UsedMalloc->Realloc( Ptr, NewSize, Alignment );
		if(Ptr)
		{
			InternalRealloc( Ptr, NewPtr, NewSize );
		}
		else
		{
			InternalMalloc(NewPtr,NewSize);
		}
		return NewPtr;
	}
	
	virtual void Free( void* Ptr )
	{
		UsedMalloc->Free( Ptr );
		InternalFree( Ptr );
	}
	
	virtual void* PhysicalAlloc( DWORD Size, ECacheBehaviour InCacheBehaviour )
	{
		void* Ptr = UsedMalloc->PhysicalAlloc( Size, InCacheBehaviour );
		InternalMalloc( Ptr, Size );
		return Ptr;
	}
	
	virtual void PhysicalFree( void* Ptr )
	{
		UsedMalloc->PhysicalFree( Ptr );
		InternalFree( Ptr );
	}

	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		if( ParseCommand(&Cmd,TEXT("STARTTRACKING") ) )
		{
			MemoryAllocations = 0;
			MemoryFrees = 0;

			bIsTracking = TRUE;

			// Reset the memory tracking state.
			AddBufferedCommand("&Reset");

			return TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("STOPTRACKING")) )
		{
			if (BufferSection)
			{
				FScopeLock ScopeLock(BufferSection);
				FlushPendingCommands();
			}
			bIsTracking = FALSE;
			return TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("SETTRACKINGBASELINE")) )
		{
			AddBufferedCommand("&SetBaseline");
			if (BufferSection)
			{
				FScopeLock ScopeLock(BufferSection);
				FlushPendingCommands();
			}
			return TRUE;
		}
		else if( ParseCommand(&Cmd,TEXT("DUMPALLOCS")) )
		{
			const FString	Token			= ParseToken( Cmd, 0 );
			const UBOOL		bSummaryOnly	= Token == TEXT("SUMMARYONLY");
			DumpAllocs( bSummaryOnly, Ar );
			return TRUE;
		}

		return( UsedMalloc->Exec( Cmd, Ar ) );
	}

	virtual void DumpAllocs( UBOOL bSummaryOnly, FOutputDevice& Ar )
	{
		if( bSummaryOnly )
		{
			AddBufferedCommand("&DumpAll%08x%08x", MemoryAllocations, MemoryFrees );
		}
		else
		{
			AddBufferedCommand("&DumpGCD%08x%08x", MemoryAllocations, MemoryFrees );
		}
		if (BufferSection)
		{
			FScopeLock ScopeLock(BufferSection);
			FlushPendingCommands();
		}
	}

	/**
	 * Passes request for gathering memory allocations for both virtual and physical allocations
	 * on to used memory manager.
	 *
	 * @param Virtual	[out] size of virtual allocations
	 * @param Physical	[out] size of physical allocations	
	 */
	virtual void GetAllocationInfo( SIZE_T& Virtual, SIZE_T& Physical )
	{
		UsedMalloc->GetAllocationInfo( Virtual, Physical );
	}
};

#endif
