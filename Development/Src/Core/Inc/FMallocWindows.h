/*=============================================================================
	FMallocWindows.h: Windows optimized memory allocator.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __FMALLOCWINDOWS_H__
#define __FMALLOCWINDOWS_H__

// Not supported in 64-bit
#ifndef _WIN64

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#define MEM_TIME(st)

//
// Optimized Windows virtual memory allocator.
//
class FMallocWindows : public FMalloc
{
private:
	// Counts.
	enum { POOL_COUNT = 42 };

	/** Maximum allocation for the pooled allocator */
	enum { MAX_POOLED_ALLOCATION_SIZE   = 32768+1 };

	/** Size of indirect pools */
	enum { INDIRECT_TABLE_BIT_SIZE = 5 };
	enum { INDIRECT_TABLE_SIZE = ( 1 << INDIRECT_TABLE_BIT_SIZE ) };
	enum { INDIRECT_TABLE_SHIFT = ( 32 - INDIRECT_TABLE_BIT_SIZE ) };

	/** Shift to get the 64k reference from the indirect tables */
	enum { POOL_BIT_SHIFT = 16 };
	/** Used to mask off the bits that have been used to lookup the indirect table */
	enum { POOL_MASK = ( ( 1 << ( INDIRECT_TABLE_SHIFT - POOL_BIT_SHIFT ) ) - 1 ) };

	// Forward declares.
	struct FFreeMem;
	struct FPoolTable;

	// Memory pool info. 32 bytes.
	struct FPoolInfo
	{
		/** Bytes allocated for pool. */
		DWORD			Bytes;		
		/** Bytes allocated from OS. */
		DWORD			OsBytes;
		/** Number of allocated elements in this pool, when counts down to zero can free the entire pool. */
		DWORD			Taken;      
		/** Memory base. */
		BYTE*			Mem;		
		/** Index of pool. */
		FPoolTable*		Table;		
		/** Pointer to first free memory in this pool. */
		FFreeMem*		FirstMem;   
		FPoolInfo*		Next;
		FPoolInfo**		PrevLink;

		void Link( FPoolInfo*& Before )
		{
			if( Before )
			{
				Before->PrevLink = &Next;
			}
			Next     = Before;
			PrevLink = &Before;
			Before   = this;
		}

		void Unlink()
		{
			if( Next )
			{
				Next->PrevLink = PrevLink;
			}
			*PrevLink = Next;
		}
	};

	/** Information about a piece of free memory. 8 bytes. */
	struct FFreeMem
	{
		/** Next or MemLastPool[], always in order by pool. */
		FFreeMem*	Next;				
		/** Number of consecutive free blocks here, at least 1. */
		DWORD		NumFreeBlocks;

		FPoolInfo* GetPool()
		{
			return (FPoolInfo*)((INT)this & 0xffff0000);
		}
	};

	/** Pool table. */
	struct FPoolTable
	{
		FPoolInfo*			FirstPool;
		FPoolInfo*			ExhaustedPool;
		DWORD				BlockSize;

#if STATS
		/** Number of currently active pools */
		DWORD				NumActivePools;

		/** Largest number of pools simultaneously active */
		DWORD				MaxActivePools;

		/** Number of requests currently active */
		DWORD				ActiveRequests;

		/** High watermark of requests simultaneously active */
		DWORD				MaxActiveRequests;

		/** Minimum request size (in bytes) */
		DWORD				MinRequest;

		/** Maximum request size (in bytes) */
		DWORD				MaxRequest;

		/** Total number of requests ever */
		QWORD				TotalRequests;

		/** Total waste from all allocs in this table */
		QWORD				TotalWaste;
#endif
	};

	// Variables.
	FPoolTable  PoolTable[POOL_COUNT];
	FPoolTable	OsTable;
	FPoolInfo*	PoolIndirect[INDIRECT_TABLE_SIZE];
	FPoolTable* MemSizeToPoolTable[MAX_POOLED_ALLOCATION_SIZE];

	DWORD		PageSize;

#if STATS
	DWORD		OsCurrent;
	DWORD		OsPeak;
	DWORD		WasteCurrent;
	DWORD		WastePeak;
	DWORD		UsedCurrent;
	DWORD		UsedPeak;
	DWORD		CurrentAllocs;
	DWORD		TotalAllocs;
	DOUBLE		MemTime;
#endif

	// Implementation. 
	void OutOfMemory()
	{
		appErrorf( *LocalizeError("OutOfMemory", TEXT("Core")) );
	}

	/** 
	 * Create a 64k page of FPoolInfo structures for tracking allocations
	 */
	FPoolInfo* CreateIndirect()
	{
		FPoolInfo* Indirect = (FPoolInfo*)VirtualAlloc( NULL, 2048 * sizeof(FPoolInfo), MEM_COMMIT, PAGE_READWRITE );
		if( !Indirect )
		{
			OutOfMemory();
		}
		STAT(WasteCurrent += 2048 * sizeof(FPoolInfo));
		return Indirect;
	}

public:
	// FMalloc interface.
	FMallocWindows()
	:	PageSize		( 0 )
#if STATS
	,	OsCurrent		( 0 )
	,	OsPeak			( 0 )
	,	WasteCurrent	( 0 )
	,	WastePeak		( 0 )
	,	UsedCurrent		( 0 )
	,	UsedPeak		( 0 )
	,	CurrentAllocs	( 0 )
	,	TotalAllocs		( 0 )
	,	MemTime			( 0.0 )
#endif
	{
		// Get OS page size.
		SYSTEM_INFO SI;
		GetSystemInfo( &SI );
		PageSize = SI.dwPageSize;
		check(!(PageSize & (PageSize - 1)));

#if STATS
		appMemset( &OsTable, 0, sizeof( FPoolTable ) );
		appMemset( PoolTable, 0, sizeof( FPoolTable ) * POOL_COUNT );
#endif
		// Init tables.
		OsTable.FirstPool = NULL;
		OsTable.ExhaustedPool = NULL;
		OsTable.BlockSize = 0;

		PoolTable[0].FirstPool = NULL;
		PoolTable[0].ExhaustedPool = NULL;
		PoolTable[0].BlockSize = 8;

		for( DWORD i = 1; i < 5; i++ )
		{
			PoolTable[i].FirstPool = NULL;
			PoolTable[i].ExhaustedPool = NULL;
			PoolTable[i].BlockSize = (8 << ((i + 1) >> 2)) + (2 << i);
#if STATS
			PoolTable[i].MinRequest = PoolTable[i].BlockSize;
#endif
		}

		for( DWORD i=5; i<POOL_COUNT; i++ )
		{
			PoolTable[i].FirstPool = NULL;
			PoolTable[i].ExhaustedPool = NULL;
			PoolTable[i].BlockSize = (4 + ((i + 7) & 3)) << (1 + ((i + 7) >> 2));
#if STATS
			PoolTable[i].MinRequest = PoolTable[i].BlockSize;
#endif
		}

		for( DWORD i=0; i<MAX_POOLED_ALLOCATION_SIZE; i++ )
		{
			DWORD Index;
			for( Index = 0; PoolTable[Index].BlockSize < i; Index++ );
			checkSlow(Index < POOL_COUNT);
			MemSizeToPoolTable[i] = &PoolTable[Index];
		}

		for( DWORD i = 0; i < ARRAY_COUNT(PoolIndirect); i++ )
		{
			PoolIndirect[i] = NULL;
		}

		check(MAX_POOLED_ALLOCATION_SIZE - 1 == PoolTable[POOL_COUNT - 1].BlockSize);
	}
	
	/** 
	 * Malloc
	 */
	virtual void* Malloc( DWORD Size, DWORD Alignment )
	{
		check(Alignment == DEFAULT_ALIGNMENT && "Alignment currently unsupported in Windows");
		MEM_TIME(MemTime -= appSeconds());
		STAT(CurrentAllocs++);
		STAT(TotalAllocs++);
		FFreeMem* Free;
		if( Size < MAX_POOLED_ALLOCATION_SIZE )
		{
			// Allocate from pool.
			FPoolTable* Table = MemSizeToPoolTable[Size];
			checkSlow(Size <= Table->BlockSize);

#if STATS
			// keep track of memory lost to padding
			Table->TotalWaste += Table->BlockSize - Size;
			Table->TotalRequests++;
			Table->ActiveRequests++;
			Table->MaxActiveRequests = Max(Table->MaxActiveRequests, Table->ActiveRequests);
			Table->MaxRequest = Size > Table->MaxRequest ? Size : Table->MaxRequest;
			Table->MinRequest = Size < Table->MinRequest ? Size : Table->MinRequest;
#endif
			FPoolInfo* Pool = Table->FirstPool;
			if( !Pool )
			{
				// Must create a new pool.
				DWORD Blocks = 65536 / Table->BlockSize;
				DWORD Bytes = Blocks * Table->BlockSize;
				checkSlow(Blocks >= 1);
				checkSlow(Blocks * Table->BlockSize <= Bytes);

				// Allocate memory.
				Free = (FFreeMem*)VirtualAlloc( NULL, Bytes, MEM_COMMIT, PAGE_READWRITE );
				if( !Free )
				{
					OutOfMemory();
				}

				// Create pool in the indirect table.
				FPoolInfo*& Indirect = PoolIndirect[((DWORD)Free >> INDIRECT_TABLE_SHIFT)];
				if( !Indirect )
				{
					Indirect = CreateIndirect();
				}
				Pool = &Indirect[((DWORD)Free >> POOL_BIT_SHIFT) & POOL_MASK];

				// Init pool.
				Pool->Link( Table->FirstPool );
				Pool->Mem        = (BYTE*)Free;
				Pool->Bytes	     = Bytes;
				Pool->OsBytes	 = Align(Bytes, PageSize);
				STAT(OsPeak = Max(OsPeak, OsCurrent += Pool->OsBytes));
				STAT(WastePeak = Max(WastePeak, WasteCurrent += Pool->OsBytes - Pool->Bytes));
				Pool->Table		 = Table;
				Pool->Taken		 = 0;
				Pool->FirstMem   = Free;

#if STATS
				Table->NumActivePools++;
				Table->MaxActivePools = Max(Table->MaxActivePools, Table->NumActivePools);
#endif
				// Create first free item.
				Free->NumFreeBlocks = Blocks;
				Free->Next       = NULL;
			}

			// Pick first available block and unlink it.
			Pool->Taken++;
			checkSlow(Pool->FirstMem);
			checkSlow(Pool->FirstMem->NumFreeBlocks > 0);
			Free = (FFreeMem*)((BYTE*)Pool->FirstMem + --Pool->FirstMem->NumFreeBlocks * Table->BlockSize);
			if( !Pool->FirstMem->NumFreeBlocks )
			{
				Pool->FirstMem = Pool->FirstMem->Next;
				if( !Pool->FirstMem )
				{
					// Move to exhausted list.
					Pool->Unlink();
					Pool->Link( Table->ExhaustedPool );
				}
			}
			STAT(UsedPeak = Max(UsedPeak, UsedCurrent += Table->BlockSize));
		}
		else
		{
			// Use OS for large allocations.
			INT AlignedSize = Align(Size,PageSize);
			Free = (FFreeMem*)VirtualAlloc( NULL, AlignedSize, MEM_COMMIT, PAGE_READWRITE );
			if( !Free )
			{
				OutOfMemory();
			}
			checkSlow(!((SIZE_T)Free & 65535));

			// Create indirect.
			FPoolInfo*& Indirect = PoolIndirect[((DWORD)Free >> INDIRECT_TABLE_SHIFT)];
			if( !Indirect )
			{
				Indirect = CreateIndirect();
			}

			// Init pool.
			FPoolInfo* Pool = &Indirect[((DWORD)Free >> POOL_BIT_SHIFT) & POOL_MASK];
			Pool->Mem		= (BYTE*)Free;
			Pool->Bytes		= Size;
			Pool->OsBytes	= AlignedSize;
			Pool->Table		= &OsTable;
			STAT(OsPeak = Max(OsPeak, OsCurrent += AlignedSize));
			STAT(UsedPeak = Max(UsedPeak, UsedCurrent += Size));
			STAT(WastePeak = Max(WastePeak, WasteCurrent += AlignedSize - Size));
		}

		MEM_TIME(MemTime += appSeconds());
		return Free;
	}
	
	/** 
	 * Realloc
	 */
	virtual void* Realloc( void* Ptr, DWORD NewSize, DWORD Alignment )
	{
		check(Alignment == DEFAULT_ALIGNMENT && "Alignment currently unsupported in Windows");
		MEM_TIME(MemTime -= appSeconds());
		void* NewPtr = Ptr;
		if( Ptr && NewSize )
		{
			FPoolInfo* Pool = &PoolIndirect[(DWORD)Ptr >> INDIRECT_TABLE_SHIFT][((DWORD)Ptr >> POOL_BIT_SHIFT) & POOL_MASK];
			if( Pool->Table != &OsTable )
			{
				// Allocated from pool, so grow or shrink if necessary.
				if( NewSize>Pool->Table->BlockSize || MemSizeToPoolTable[NewSize] != Pool->Table )
				{
					NewPtr = Malloc( NewSize, Alignment );
					appMemcpy( NewPtr, Ptr, Min( NewSize, Pool->Table->BlockSize ) );
					Free( Ptr );
				}
			}
			else
			{
				// Allocated from OS.
				checkSlow(!((INT)Ptr & 65535));
				if( NewSize > Pool->OsBytes || NewSize * 3 < Pool->OsBytes * 2 )
				{
					// Grow or shrink.
					NewPtr = Malloc( NewSize, Alignment );
					appMemcpy( NewPtr, Ptr, Min(NewSize, Pool->Bytes) );
					Free( Ptr );
				}
				else
				{
					// Keep as-is, reallocation isn't worth the overhead.
					STAT(UsedCurrent += NewSize - Pool->Bytes);
					STAT(UsedPeak = Max(UsedPeak, UsedCurrent));
					STAT(WasteCurrent += Pool->Bytes - NewSize);
					Pool->Bytes = NewSize;
				}
			}
		}
		else if( Ptr == NULL )
		{
			NewPtr = Malloc( NewSize, Alignment );
		}
		else
		{
			Free( Ptr );
			NewPtr = NULL;
		}

		MEM_TIME(MemTime += appSeconds());
		return NewPtr;
	}
	
	/** 
	 * Free
	 */
	virtual void Free( void* Ptr )
	{
		if( !Ptr )
		{
			return;
		}

		MEM_TIME(MemTime -= appSeconds());
		STAT(CurrentAllocs--);

		// Windows version.
		FPoolInfo* Pool = &PoolIndirect[(DWORD)Ptr >> INDIRECT_TABLE_SHIFT][((DWORD)Ptr >> POOL_BIT_SHIFT) & POOL_MASK];
		checkSlow(Pool->Bytes != 0);
		if( Pool->Table != &OsTable )
		{
#if STATS
			Pool->Table->ActiveRequests--;
#endif
			// If this pool was exhausted, move to available list.
			if( !Pool->FirstMem )
			{
				Pool->Unlink();
				Pool->Link( Pool->Table->FirstPool );
			}

			// Free a pooled allocation.
			FFreeMem* Free		= (FFreeMem*)Ptr;
			Free->NumFreeBlocks	= 1;
			Free->Next			= Pool->FirstMem;
			Pool->FirstMem		= Free;
			STAT(UsedCurrent -= Pool->Table->BlockSize);

			// Free this pool.
			checkSlow(Pool->Taken >= 1);
			if( --Pool->Taken == 0 )
			{
#if STATS
				Pool->Table->NumActivePools--;
#endif
				// Free the OS memory.
				Pool->Unlink();
				verify( VirtualFree( Pool->Mem, 0, MEM_RELEASE )!=0 );
				STAT(OsCurrent -= Pool->OsBytes);
				STAT(WasteCurrent -= Pool->OsBytes - Pool->Bytes);
			}
		}
		else
		{
			// Free an OS allocation.
			checkSlow(!((INT)Ptr & 65535));
			STAT(UsedCurrent -= Pool->Bytes);
			STAT(OsCurrent -= Pool->OsBytes);
			STAT(WasteCurrent -= Pool->OsBytes - Pool->Bytes);
			verify( VirtualFree( Ptr, 0, MEM_RELEASE ) != 0 );
		}

		MEM_TIME(MemTime += appSeconds());
	}

	/**
	 * Gathers memory allocations for both virtual and physical allocations.
	 *
	 * @param FMemoryAllocationStats	[out] structure containing information about the size of allocations
	 */
	virtual void GetAllocationInfo( FMemoryAllocationStats& MemStats )
	{
		// Just get memory information for the process and report the working set instead
		PROCESS_MEMORY_COUNTERS Counters;
		GetProcessMemoryInfo(GetCurrentProcess(), &Counters, sizeof(Counters));

		MemStats.TotalUsed = Counters.WorkingSetSize;
		MemStats.TotalAllocated = Counters.WorkingSetSize;

#if STATS
		MemStats.CPUUsed = UsedCurrent;
		MemStats.CPUWaste = WasteCurrent;
		MemStats.CPUSlack = OsCurrent - WasteCurrent - UsedCurrent;

		DOUBLE Waste = 0.0;
		for( INT PoolIndex = 0; PoolIndex < POOL_COUNT; PoolIndex++ )
		{
			Waste += ( ( DOUBLE )PoolTable[PoolIndex].TotalWaste / ( DOUBLE )PoolTable[PoolIndex].TotalRequests ) * ( DOUBLE )PoolTable[PoolIndex].ActiveRequests;
		}
		MemStats.CPUWaste += ( DWORD )Waste;
#endif
	}

	/**
	 * Validates the allocator's heap
	 */
	virtual UBOOL ValidateHeap()
	{
		for( INT i = 0; i < POOL_COUNT; i++ )
		{
			FPoolTable* Table = &PoolTable[i];
			for( FPoolInfo** PoolPtr = &Table->FirstPool; *PoolPtr; PoolPtr = &(*PoolPtr)->Next )
			{
				FPoolInfo* Pool = *PoolPtr;
				check(Pool->PrevLink == PoolPtr);
				check(Pool->FirstMem);
				for( FFreeMem* Free = Pool->FirstMem; Free; Free = Free->Next )
				{
					check(Free->NumFreeBlocks > 0);
				}
			}
			for( FPoolInfo** PoolPtr = &Table->ExhaustedPool; *PoolPtr; PoolPtr = &(*PoolPtr)->Next )
			{
				FPoolInfo* Pool = *PoolPtr;
				check(Pool->PrevLink == PoolPtr);
				check(!Pool->FirstMem);
			}
		}

		return( TRUE );
	}

	/**
	 * Dumps details about all allocations to an output device
	 *
	 * @param Ar	[in] Output device
	 */
	virtual void DumpAllocations( class FOutputDevice& Ar ) 
	{
		ValidateHeap();

		STAT(Ar.Logf( TEXT("Memory Allocation Status") ));
		STAT(Ar.Logf( TEXT("Current Memory %.2f MB used, plus %.2f MB waste"), UsedCurrent / ( 1024.0f * 1024.0f ), ( OsCurrent - UsedCurrent ) / ( 1024.0f * 1024.0f ) ));
		STAT(Ar.Logf( TEXT("Peak Memory %.2f MB used, plus %.2f MB waste"), UsedPeak / ( 1024.0f * 1024.0f ), ( OsPeak - UsedPeak ) / ( 1024.0f * 1024.0f ) ));
		STAT(Ar.Logf( TEXT("Allocs      % 6i Current / % 6i Total"), CurrentAllocs, TotalAllocs ));
		MEM_TIME(Ar.Logf( TEXT( "Seconds     % 5.3f" ), MemTime ));
		MEM_TIME(Ar.Logf( TEXT( "MSec/Allc   % 5.5f" ), 1000.0 * MemTime / MemAllocs ));
		STAT(Ar.Logf( TEXT("") ));

		PROCESS_MEMORY_COUNTERS Counters;
		GetProcessMemoryInfo(GetCurrentProcess(), &Counters, sizeof(Counters));

		Ar.Logf(TEXT("GlobalMemoryStatus"));
		Ar.Logf(TEXT("WorkingSetSize          %6.2f MB"), Counters.WorkingSetSize / ( 1024.0f * 1024.0f ));
		STAT(Ar.Logf( TEXT("") ));

#if STATS
		Ar.Logf( TEXT("Block Size Num Pools Max Pools Cur Allocs Total Allocs Min Req Max Req Mem Used Mem Align Efficiency") );
		Ar.Logf( TEXT("---------- --------- --------- ---------- ------------ ------- ------- -------- --------- ----------") );

		DWORD TotalMemory = 0;
		DWORD TotalWaste = 0;
		DWORD TotalActiveRequests = 0;
		DWORD TotalTotalRequests = 0;
		DWORD TotalPools = 0;

		for( INT i = 0; i < POOL_COUNT; i++ )
		{
			FPoolTable* Table = PoolTable + i;
			
			DWORD MemUsed = ( Table->BlockSize * Table->ActiveRequests ) / 1024;
			DWORD MemWaste = ( DWORD )( ( ( DOUBLE )Table->TotalWaste / ( DOUBLE )Table->TotalRequests ) * ( DOUBLE )Table->ActiveRequests ) / 1024;

			Ar.Logf( TEXT("% 10i % 9i % 9i % 10i % 12i % 7i % 7i % 7iK % 8iK % 9.2f%%"),
				Table->BlockSize,
				Table->NumActivePools,
				Table->MaxActivePools,
				Table->ActiveRequests,
				( DWORD )Table->TotalRequests,
				Table->MinRequest,
				Table->MaxRequest,
				MemUsed - MemWaste,
				MemWaste,
				MemUsed ? 100.0f * ( MemUsed - MemWaste) / MemUsed : 100.0f );

			TotalMemory += MemUsed - MemWaste;
			TotalWaste += MemWaste;
			TotalActiveRequests += Table->ActiveRequests;
			TotalTotalRequests += Table->TotalRequests;
			TotalPools += Table->NumActivePools;
		}

		Ar.Logf( TEXT( "" ) );
		Ar.Logf( TEXT( "%iK allocated (with %iK waste). Efficiency %.2f%%" ), TotalMemory, TotalWaste, TotalMemory ? 100.0f * ( TotalMemory - TotalWaste) / TotalMemory : 100.0f );
		Ar.Logf( TEXT( "Allocations %i Current / %i Total (in %i pools)"), TotalActiveRequests, TotalTotalRequests, TotalPools );
#endif
	}
};

#endif	// #ifndef _WIN64


#endif


