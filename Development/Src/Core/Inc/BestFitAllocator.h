/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


#ifndef BEST_FIT_ALLOCATOR_H
#define BEST_FIT_ALLOCATOR_H


#define LOG_EVERY_ALLOCATION			0
#define DUMP_ALLOC_FREQUENCY			0 // 100

/*-----------------------------------------------------------------------------
	Custom fixed size pool best fit texture memory allocator
-----------------------------------------------------------------------------*/

/**
 * Simple best fit allocator, splitting and coalescing whenever/ wherever possible.
 *
 * - uses TDynamicMap to find memory chunk given a pointer (potentially colliding with malloc/ free from main thread)
 * - uses separate linked list for free allocations, assuming that relatively few free chunks due to coalescing
 */
struct FBestFitAllocator
{
	/**
	 * Contains information of a single allocation or free block.
	 */
	struct FMemoryChunk
	{
		/**
		 * Private constructor.
		 *
		 * @param	InBase					Pointer to base of chunk
		 * @param	InSize					Size of chunk
		 * @param	ChunkToInsertAfter		Chunk to insert this after.
		 * @param	FirstFreeChunk			Reference to first free chunk pointer.
		 */
		FMemoryChunk( BYTE* InBase, INT InSize, FMemoryChunk*& ChunkToInsertAfter, FMemoryChunk*& InFirstChunk, FMemoryChunk*& InFirstFreeChunk )
		:	Base(InBase)
		,	Size(InSize)
		,	bIsAvailable(FALSE)
		,	FirstChunk(InFirstChunk)
		,	FirstFreeChunk(InFirstFreeChunk)
		{
			Link( ChunkToInsertAfter );
			// This is going to change bIsAvailable.
			LinkFree();
		}
		
		/**
		 * Unlinks/ removes the chunk from the linked lists it belongs to.
		 */
		~FMemoryChunk()
		{
			// Remove from linked lists.
			Unlink();
			UnlinkFree();
		}

		/**
		 * Inserts this chunk after the passed in one.
		 *
		 * @param	ChunkToInsertAfter	Chunk to insert after
		 */
		void Link( FMemoryChunk*& ChunkToInsertAfter )
		{
			if( ChunkToInsertAfter )
			{
				NextChunk		= ChunkToInsertAfter->NextChunk;
				PreviousChunk	= ChunkToInsertAfter;
				ChunkToInsertAfter->NextChunk = this;
				if( NextChunk )
				{
					NextChunk->PreviousChunk = this;
				}				
			}
			else
			{
				PreviousChunk		= NULL;
				NextChunk			= NULL;
				ChunkToInsertAfter	= this;
			}
		}

		/**
		 * Inserts this chunk at the head of the free chunk list.
		 */
		void LinkFree()
		{
			check(!bIsAvailable);
			bIsAvailable = TRUE;

			if( FirstFreeChunk )
			{
				NextFreeChunk		= FirstFreeChunk;
				PreviousFreeChunk	= NULL;
				FirstFreeChunk->PreviousFreeChunk = this;
				FirstFreeChunk		= this;
			}
			else
			{
				PreviousFreeChunk	= NULL;
				NextFreeChunk		= NULL;
				FirstFreeChunk		= this;
			}
		}

		/**
		 * Removes itself for linked list.
		 */
		void Unlink()
		{
			if( PreviousChunk )
			{
				PreviousChunk->NextChunk = NextChunk;
			}
			else
			{
				FirstChunk = NextChunk;
			}
			
			if( NextChunk )
			{
				NextChunk->PreviousChunk = PreviousChunk;
			}

			PreviousChunk	= NULL;
			NextChunk		= NULL;
		}

		/**
		 * Removes itself for linked "free" list.
		 */
		void UnlinkFree()
		{
			check(bIsAvailable);
			bIsAvailable = FALSE;

			if( PreviousFreeChunk )
			{
				PreviousFreeChunk->NextFreeChunk = NextFreeChunk;
			}
			else
			{
				FirstFreeChunk = NextFreeChunk;
			}

			if( NextFreeChunk )
			{
				NextFreeChunk->PreviousFreeChunk = PreviousFreeChunk;
			}

			PreviousFreeChunk	= NULL;
			NextFreeChunk		= NULL;
		}

		/** Base of chunk.								*/
		BYTE*					Base;
		/** Size of chunk.								*/
		INT						Size;
		/** Whether chunk is available or not.			*/
		UBOOL					bIsAvailable;

		/** First chunk.								*/
		FMemoryChunk*&			FirstChunk;
		/** First free chunk.							*/
		FMemoryChunk*&			FirstFreeChunk;
		/** Pointer to previous chunk.					*/
		FMemoryChunk*			PreviousChunk;
		/** Pointer to next chunk.						*/
		FMemoryChunk*			NextChunk;

		/** Pointer to previous free chunk.				*/
		FMemoryChunk*			PreviousFreeChunk;
		/** Pointer to next free chunk.					*/
		FMemoryChunk*			NextFreeChunk;
	};

	/** Constructor, zero initializing all member variables */
	FBestFitAllocator()
	:	MemorySize(0)
	,	MemoryBase(NULL)
	,	AllocationAlignment(0)
	,	FirstChunk(NULL)
	,	FirstFreeChunk(NULL)
	,	TimeSpentInAllocator(0.0)
	,	AllocatedMemorySize(0)
	,	AvailableMemorySize(0)
	{}

	/**
	 * Initialize this allocator with 
	 */
	void Initialize( BYTE* InMemoryBase, INT InMemorySize, INT InAllocationAlignment )
	{
		// Update size, pointer and alignment.
		MemoryBase			= InMemoryBase;
		MemorySize			= InMemorySize;
		AllocationAlignment	= InAllocationAlignment;
		check( Align( MemoryBase, AllocationAlignment ) == MemoryBase );

		// Update stats in a thread safe way.
		appInterlockedExchange( &AvailableMemorySize, MemorySize );
		// Allocate initial chunk.
		FirstChunk			= new FMemoryChunk( MemoryBase, MemorySize, FirstChunk, FirstChunk, FirstFreeChunk );
	}

	/**
	 * Returns whether allocator has been initialized.
	 */
	UBOOL IsInitialized()
	{
		return MemoryBase != NULL;
	}

	/**
	 * Allocate physical memory.
	 *
	 * @param	AllocationSize	Size of allocation
	 * @return	Pointer to allocated memory
	 */
	void* Allocate( INT AllocationSize )
	{
		SCOPE_SECONDS_COUNTER(TimeSpentInAllocator);
		check( FirstChunk );

		// Make sure everything is appropriately aligned.
		AllocationSize = Align( AllocationSize, AllocationAlignment );

		// Perform a "best fit" search, returning first perfect fit if there is one.
		FMemoryChunk* CurrentChunk	= FirstFreeChunk;
		FMemoryChunk* BestChunk		= NULL;
		while( CurrentChunk )
		{
			// Check whether chunk is available and large enough to hold allocation.
			check( CurrentChunk->bIsAvailable );
			if( CurrentChunk->Size >= AllocationSize )
			{
				// Compare with current best chunk if one exists. 
				if( BestChunk )
				{
					// Tighter fits are preferred.
					if( CurrentChunk->Size < BestChunk->Size )
					{
						BestChunk = CurrentChunk;
					}
				}
				// No existing best chunk so use this one.
				else
				{
					BestChunk = CurrentChunk;
				}

				// We have a perfect fit, no need to iterate further.
				if( BestChunk->Size == AllocationSize )
				{
					break;
				}
			}
			CurrentChunk = CurrentChunk->NextFreeChunk;
		}

		// Dump allocation info and return NULL if we weren't able to satisfy allocation request.
		if( !BestChunk )
		{
#if !FINAL_RELEASE
			DumpAllocs();
			GLog->FlushThreadedLogs();
			debugf(TEXT("Ran out of memory for allocation in best-fit allocator of size %i KByte"), AllocationSize / 1024);
#endif
			return NULL;
		}

		// Mark as being in use.
		BestChunk->UnlinkFree();

		// Split chunk to avoid waste.
		if( BestChunk->Size > AllocationSize )
		{
			Split( BestChunk, AllocationSize );
		}

		// Ensure that everything's in range.
		check( (BestChunk->Base + BestChunk->Size) <= (MemoryBase + MemorySize) );
		check( BestChunk->Base >= MemoryBase );

		// Update usage stats in a thread safe way.
		appInterlockedAdd( &AllocatedMemorySize, +BestChunk->Size );
		appInterlockedAdd( &AvailableMemorySize, -BestChunk->Size );

		// Keep track of mapping and return pointer.
		PointerToChunkMap.Set( (PTRINT) BestChunk->Base, BestChunk );
		return BestChunk->Base;
	}

	/**
	 * Frees allocation associated with passed in pointer.
	 *
	 * @param	Pointer		Pointer to free.
	 */
	void Free( void* Pointer )
	{
		SCOPE_SECONDS_COUNTER(TimeSpentInAllocator);

		// Look up pointer in TMap.
		FMemoryChunk* MatchingChunk = PointerToChunkMap.FindRef( (PTRINT) Pointer );
		check( MatchingChunk );
		// Remove the entry
		PointerToChunkMap.Remove((PTRINT) Pointer);

		// Update usage stats in a thread safe way.
		appInterlockedAdd( &AllocatedMemorySize, -MatchingChunk->Size );
		appInterlockedAdd( &AvailableMemorySize, +MatchingChunk->Size );

		// Free the chunk.
		FreeChunk(MatchingChunk);
	}

	/**
	 * Dump allocation information.
	 */
	void DumpAllocs( FOutputDevice& Ar=*GLog )
	{		
		// Memory usage stats.
		INT				UsedSize		= 0;
		INT				FreeSize		= 0;
		INT				NumUsedChunks	= 0;
		INT				NumFreeChunks	= 0;
		
		// Fragmentation and allocation size visualiztion.
		INT				NumBlocks		= MemorySize / AllocationAlignment;
		INT				Dimension		= 1 + NumBlocks / appTrunc(appSqrt(NumBlocks));			
		TArray<FColor>	AllocationVisualization;
		AllocationVisualization.AddZeroed( Dimension * Dimension );
		INT				VisIndex		= 0;

		// Traverse linked list and gather allocation information.
		FMemoryChunk*	CurrentChunk	= FirstChunk;	
		while( CurrentChunk )
		{
			FColor VisColor;
			// Free chunk.
			if( CurrentChunk->bIsAvailable )
			{
				NumFreeChunks++;
				FreeSize += CurrentChunk->Size;
				VisColor = FColor(0,255,0);
			}
			// Allocated chunk.
			else
			{
				NumUsedChunks++;
				UsedSize += CurrentChunk->Size;
				
				// Slightly alternate coloration to also visualize allocation sizes.
				if( NumUsedChunks % 2 == 0 )
				{
					VisColor = FColor(255,0,0);
				}
				else
				{
					VisColor = FColor(192,0,0);
				}
			}

			for( INT i=0; i<(CurrentChunk->Size/AllocationAlignment); i++ )
			{
				AllocationVisualization(VisIndex++) = VisColor;
			}

			CurrentChunk = CurrentChunk->NextChunk;
		}

		check(UsedSize == AllocatedMemorySize);
		check(FreeSize == AvailableMemorySize);

		// Write out bitmap for visualization of fragmentation and allocation patterns.
		appCreateBitmap( TEXT("TextureMemory"), Dimension, Dimension, AllocationVisualization.GetTypedData() );
		Ar.Logf( TEXT("BestFitAllocator: Allocated %i KByte in %i chunks, leaving %i KByte in %i chunks."), UsedSize / 1024, NumUsedChunks, FreeSize / 1024, NumFreeChunks );
		Ar.Logf( TEXT("BestFitAllocator: %5.2f ms in allocator"), TimeSpentInAllocator * 1000 );
	}

	/**
	 * Retrieves allocation stats.
	 *
	 * @param	OutAllocatedMemorySize	[out]	Size of allocated memory
	 * @param	OutAvailableMemorySize	[out]	Size of available memory
	 */
	void GetMemoryStats( INT& OutAllocatedMemorySize, INT& OutAvailableMemorySize )
	{
		OutAllocatedMemorySize = AllocatedMemorySize;
		OutAvailableMemorySize = AvailableMemorySize;
	}

protected:

	/**
	 * Split allocation into two, first chunk being used and second being available.
	 *
	 * @param	BaseChunk	Chunk to split
	 * @param	FirstSize	New size of first chunk
	 */
	void Split( FMemoryChunk* BaseChunk, INT FirstSize )
	{
		check( BaseChunk );
		check( FirstSize < BaseChunk->Size );
		check( FirstSize > 0 );
		check( !BaseChunk->NextChunk || !BaseChunk->NextChunk->bIsAvailable );
		check( !BaseChunk->PreviousChunk || !BaseChunk->PreviousChunk->bIsAvailable );

		// Calculate size of second chunk...
		INT SecondSize = BaseChunk->Size - FirstSize;
		// ... and create it.
		new FMemoryChunk( BaseChunk->Base + FirstSize, SecondSize, BaseChunk, FirstChunk, FirstFreeChunk );

		// Resize base chunk.
		BaseChunk->Size = FirstSize;
	}

	/**
	 * Frees the passed in chunk and coalesces if possible.
	 *
	 * @param	Chunk	 Chunk to mark as available. 
	 */
	void FreeChunk( FMemoryChunk* Chunk )
	{
		check(Chunk);
		// Mark chunk as available.
		Chunk->LinkFree();
		// Kick of merge pass.
		Coalesce( Chunk );
		// Not save to access Chunk at this point as Coalesce might have deleted it!
	}

	/**
	 * Tries to coalesce/ merge chunks now that we have a new freed one.
	 *
	 * @param	FreedChunk	Chunk that just became available.
	 */
	void Coalesce( FMemoryChunk* FreedChunk )
	{
		check( FreedChunk );

		FMemoryChunk* PreviousChunk = FreedChunk->PreviousChunk;
		FMemoryChunk* NextChunk		= FreedChunk->NextChunk;

		// If previous chunk is available, try to merge with it and following ones.
		if( PreviousChunk && PreviousChunk->bIsAvailable )
		{
			FMemoryChunk* CurrentChunk = FreedChunk;
			// This is required to handle the case of AFA (where A=available and F=freed) to ensure that
			// there is only a single A afterwards.
			while( CurrentChunk && CurrentChunk->bIsAvailable )
			{
				PreviousChunk->Size += CurrentChunk->Size;
				// Chunk is no longer needed, delete and move on to next.
				CurrentChunk = DeleteAndReturnNext( CurrentChunk );
			}
		}
		// Try to merge with next chunk.
		else if( NextChunk && NextChunk->bIsAvailable )
		{
			NextChunk->Base -= FreedChunk->Size;
			NextChunk->Size += FreedChunk->Size;
			// Chunk is no longer needed, delete it.
			DeleteAndReturnNext( FreedChunk );
		}
	}

	/**
	 * Deletes the passed in chunk after unlinking and returns the next one.
	 *
	 * @param	Chunk	Chunk to delete
	 * @return	the next chunk pointed to by deleted one
	 */
	FMemoryChunk* DeleteAndReturnNext( FMemoryChunk* Chunk )
	{
		// Keep track of chunk to return.
		FMemoryChunk* ChunkToReturn = Chunk->NextChunk;
		// Deletion will unlink.
		delete Chunk;
		return ChunkToReturn;
	}

	/** Size of memory pool.										*/
	INT									MemorySize;
	/** Base of memory pool.										*/
	BYTE*								MemoryBase;
	/** Allocation alignment requirements.							*/
	INT									AllocationAlignment;
	/** Head of linked list of chunks. Sorted by memory address.	*/
	FMemoryChunk*						FirstChunk;
	/** Head of linked list of free chunks.	Unsorted.				*/
	FMemoryChunk*						FirstFreeChunk;
	/** Cummulative time spent in allocator.						*/
	DOUBLE								TimeSpentInAllocator;
	/** Allocated memory in bytes.									*/
	volatile INT						AllocatedMemorySize;
	/** Available memory in bytes.									*/
	volatile INT						AvailableMemorySize;
	/** Mapping from pointer to chunk for fast removal.				*/
	TDynamicMap<PTRINT,FMemoryChunk*>	PointerToChunkMap;
};


/**
 * Wrapper class around a best fit allocator that handles running out of memory without
 * returning NULL, but instead returns a preset pointer and marks itself as "corrupt"
 */
struct FPresizedMemoryPool
{
	/**
	 * Constructor, initializes the BestFitAllocator (will allocate physical memory!)
	 * 
	 * @param PoolSize Size of the memory pool
	 * @param Alignment Default alignment for each allocation
	 */
	FPresizedMemoryPool(INT PoolSize, INT Alignment)
	: bIsCorrupted(FALSE)
	, AllocationFailurePointer(NULL)
	{
		// Allocate a little bit of extra memory so we can distinguish between a real and a failed allocation when we are freeing memory.
		AllocationFailurePointer = (BYTE*)appPhysicalAlloc(PoolSize + Alignment, CACHE_WriteCombine);
		// Initialize allocator.
		Allocator.Initialize( AllocationFailurePointer + Alignment, PoolSize, Alignment );
	}

	/**
	 * Constructor, initializes the BestFitAllocator with already allocated memory
	 * 
	 * @param PoolSize Size of the memory pool
	 * @param Alignment Default alignment for each allocation
	 */
	FPresizedMemoryPool(BYTE* PoolMemory, BYTE* FailedAllocationMemory, INT PoolSize, INT Alignment)
	: bIsCorrupted(FALSE)
	, AllocationFailurePointer(FailedAllocationMemory)
	{
		// Initialize allocator.
		Allocator.Initialize(PoolMemory, PoolSize, Alignment );
	}

	/**
	 * Allocates texture memory.
	 *
	 * @param	Size	Size of allocation
	 * @returns	Pointer to allocated memory
	 */
	void* Allocate(DWORD Size)
	{
#if DUMP_ALLOC_FREQUENCY
		static INT AllocationCounter;
		if( ++AllocationCounter % DUMP_ALLOC_FREQUENCY == 0 )
		{
			Allocator.DumpAllocs();
		}
#endif

		// Initialize allocator if it hasn't already.
		if (!Allocator.IsInitialized())
		{
		}

		// actually do the allocation
		void* Pointer = Allocator.Allocate(Size);

		// We ran out of memory. Instead of crashing rather corrupt the content and display an error message.
		if (Pointer == NULL)
		{
			// Mark texture memory as having been corrupted.
			bIsCorrupted = TRUE;

			// Use special pointer, which is being identified by free.
			Pointer = AllocationFailurePointer;
		}

#if LOG_EVERY_ALLOCATION
		INT AllocSize, AvailSize;
		Allocator.GetMemoryStats( AllocSize, AvailSize );
		debugf(TEXT("Texture Alloc: %p  Size: %6i     Alloc: %8i Avail: %8i"), Pointer, Size, AllocSize, AvailSize );
#endif
		return Pointer;
	}

	/**
	 * Frees texture memory allocated via Allocate
	 *
	 * @param	Pointer		Allocation to free
	 */
	void Free(void* Pointer)
	{
#if LOG_EVERY_ALLOCATION
		INT AllocSize, AvailSize;
		Allocator.GetMemoryStats( AllocSize, AvailSize );
		debugf(TEXT("Texture Free : %p   Before free     Alloc: %8i Avail: %8i"), Pointer, AllocSize, AvailSize );
#endif
		// we never want to free the special pointer
		if (Pointer != AllocationFailurePointer)
		{
			// do the free
			Allocator.Free(Pointer);
		}
	}

	/**
	 * Retrieves allocation stats.
	 *
	 * @param	OutAllocatedMemorySize	[out]	Size of allocated memory
	 * @param	OutAvailableMemorySize	[out]	Size of available memory
	 */
	inline void GetMemoryStats( INT& AllocatedMemorySize, INT& AvailableMemorySize )
	{
		Allocator.GetMemoryStats(AllocatedMemorySize, AvailableMemorySize);
	}

	/** True if we have run out of memory in the pool (and therefore returned AllocationFailurePointer) */
	UBOOL bIsCorrupted;

	/** Single pointer to return when an allocation fails */
	BYTE* AllocationFailurePointer;

	/** The pool itself */
	FBestFitAllocator Allocator;
};

#endif
