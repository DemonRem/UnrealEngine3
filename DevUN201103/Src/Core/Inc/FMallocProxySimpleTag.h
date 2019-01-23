/*=============================================================================
	FMallocProxySimpleTag.h: Simple tag based allocation tracker.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __FMALLOCPROXYSIMPLETAG_H__
#define __FMALLOCPROXYSIMPLETAG_H__

class FMallocProxySimpleTag : public FMalloc
{
	/**
	 * Helper structure containing size and tag of allocation.
	 */
	struct FAllocInfo
	{
		/** Size of allocation.	*/
		INT Size;
		/** Tag at time of first allocation. Maintained for realloc. */ 
		INT OriginalTag;
		/** Tag at time of allocation. */
		INT CurrentTag;
		/** Allocation count, used for grouping during output. */
		INT Count;
	};

	/** Allocator we are actually passing requests to. */
	FMalloc*							UsedMalloc;
	/** Map from allocation pointer to size. */
	TMap<PTRINT,FAllocInfo>				AllocToInfoMap;
	/** Total size of current allocations in bytes. */
	SIZE_T								TotalAllocSize;
	/** Number of allocations. */
	SIZE_T								TotalAllocCount;
	/** Used to avoid re-entrancy (i.e. when doing map allocations)	*/
	UBOOL								bIsTracking;

	/**
	 * Add allocation to keep track of.
	 *
	 * @param	Pointer		Allocation
	 * @param	Size		Allocation size in bytes
	 * @param	Tag			Tag to use for original tag
	 */
	void AddAllocation( void* Pointer, SIZE_T Size, INT OriginalTag );
	
	/**
	 * Remove allocation from list to track.
	 *
	 * @param	Pointer		Allocation
	 * @return	Original tag of allocation
	 */
	INT RemoveAllocation( void* Pointer );

public:
	/** Current active tag.	*/
	static INT							CurrentTag;

	/**
	 * Constructor, intializing member variables and underlying allocator to use.
	 *
	 * @param	InMalloc	Allocator to use for allocations.
	 */
	FMallocProxySimpleTag(FMalloc* InMalloc);

	/** 
	 * Malloc
	 */
	virtual void* Malloc( DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT );

	/** 
	 * Realloc
	 */
	virtual void* Realloc( void* Original, DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT );

	/** 
	 * Free
	 */
	virtual void Free( void* Original );

	/** 
	 * Physical alloc
	 */
	virtual void* PhysicalAlloc( DWORD Count, ECacheBehaviour CacheBehaviour = CACHE_WriteCombine );

	/** 
	 * Physical free
	 */
	virtual void PhysicalFree( void* Original );

	/**
	 * Passes request for gathering memory allocations for both virtual and physical allocations
	 * on to used memory manager.
	 *
	 * @param FMemoryAllocationStats	[out] structure containing information about the size of allocations
	 */
	virtual void GetAllocationInfo( FMemoryAllocationStats& MemStats )
	{
		UsedMalloc->GetAllocationInfo( MemStats );
	}

	/**
	 * Dumps details about all allocations to an output device
	 *
	 * @param Ar	[in] Output device
	 */
	virtual void DumpAllocations( class FOutputDevice& Ar );

	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar );
};

#endif
