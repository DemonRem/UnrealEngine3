/*=============================================================================
	MemoryBase.h: Base memory management definitions.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __MEMORYBASE_H__
#define __MEMORYBASE_H__

/** The global memory allocator. */
extern class FMalloc* GMalloc;

// Memory allocator.

enum ECacheBehaviour
{
	CACHE_Normal		= 0,
	CACHE_WriteCombine	= 1,
	CACHE_None			= 2,
	CACHE_Virtual		= 3,
	CACHE_MAX			// needs to be last entry
};

struct FMemoryAllocationStats
{
	// Reported by the OS
	SIZE_T	TotalUsed;			// The total amount of memory used by the game
	SIZE_T	TotalAllocated;		// The total amount of memory allocated from the OS

	// Virtual memory for Xbox and PC/Host memory for PS3 (tracked in the allocators)
	SIZE_T	CPUUsed;			// The allocated in use by the application virtual memory
	SIZE_T	CPUSlack;			// The allocated from the OS, but not used by the application
	SIZE_T	CPUWaste;			// Alignment waste from a pooled allocator plus book keeping overhead
	SIZE_T	CPUAvailable;		// The sum of everything (except image size)

	// Physical memory for Xbox and PC/Local memory for PS3 (tracked in the allocators)
	SIZE_T	GPUUsed;
	SIZE_T	GPUSlack;
	SIZE_T	GPUWaste;
	SIZE_T	GPUAvailable;		// The sum of everything (except image size)

	SIZE_T	HostUsed;
	SIZE_T	HostSlack;
	SIZE_T	HostWaste;
	SIZE_T	HostAvailable;		// The sum of everything (except image size)

	// Reported by the OS
	SIZE_T	OSReportedUsed;		// Used memory as reported by the operating system
	SIZE_T	OSReportedFree;		// Free memory as reported by the operating system
	SIZE_T	OSOverhead;			// The overhead of the operating system
	SIZE_T	ImageSize;			// Size of loaded executable and stack

	FMemoryAllocationStats( void )
	{
		TotalUsed = 0;
		TotalAllocated = 0;

		CPUUsed = 0;
		CPUSlack = 0;
		CPUWaste = 0;
		CPUAvailable = 0;

		GPUUsed = 0;
		GPUSlack = 0;
		GPUWaste = 0;
		GPUAvailable = 0;

		HostUsed = 0;
		HostSlack = 0;
		HostWaste = 0;
		HostAvailable = 0;

		OSReportedUsed = 0;	
		OSReportedFree = 0;	
		OSOverhead = 0;		
		ImageSize = 0;		
	}
};

//
// C style memory allocation stubs that fall back to C runtime
//
#ifndef appSystemMalloc
#define appSystemMalloc		malloc
#endif
#ifndef appSystemFree
#define appSystemFree		free
#endif
#ifndef appSystemRealloc
#define appSystemRealloc	realloc
#endif

/**
 * Inherit from FUseSystemMallocForNew if you want your objects to be placed in memory
 * alloced by the system malloc routines, bypassing GMalloc. This is e.g. used by FMalloc
 * itself.
 */
class FUseSystemMallocForNew
{
public:
	/**
	 * Overloaded new operator using the system allocator.
	 *
	 * @param	Size	Amount of memory to allocate (in bytes)
	 * @return			A pointer to a block of memory with size Size or NULL
	 */
	void* operator new( size_t Size )
	{
		return appSystemMalloc( Size );
	}

	/**
	 * Overloaded delete operator using the system allocator
	 *
	 * @param	Ptr		Pointer to delete
	 */
	void operator delete( void* Ptr )
	{
		appSystemFree( Ptr );
	}

	/**
	 * Overloaded array new operator using the system allocator.
	 *
	 * @param	Size	Amount of memory to allocate (in bytes)
	 * @return			A pointer to a block of memory with size Size or NULL
	 */
	void* operator new[]( size_t Size )
	{
		return appSystemMalloc( Size );
	}

	/**
	 * Overloaded array delete operator using the system allocator
	 *
	 * @param	Ptr		Pointer to delete
	 */
	void operator delete[]( void* Ptr )
	{
		appSystemFree( Ptr );
	}
};

/** The global memory allocator's interface. */
class FMalloc  : public FUseSystemMallocForNew, public FExec
{
public:
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
		return Size; // implementations where this is not possible need not implement it.
	}

	/** 
	 * Malloc
	 */
	virtual void* Malloc( DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT ) = 0;

	/** 
	 * Realloc
	 */
	virtual void* Realloc( void* Original, DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT ) = 0;

	/** 
	 * Free
	 */
	virtual void Free( void* Original ) = 0;

	/** 
	 * Physical alloc
	 */
	virtual void* PhysicalAlloc( DWORD Count, ECacheBehaviour CacheBehaviour = CACHE_WriteCombine ) 
	{ 
		return NULL;
	}

	/** 
	 * Physical free
	 */
	virtual void PhysicalFree( void* Original )
	{
	}
		
	/** 
	 * Handles any commands passed in on the command line
	 */
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar ) 
	{ 
		return FALSE; 
	}
	
	/** 
	 * Called every game thread tick 
	 */
	virtual void Tick( FLOAT DeltaTime ) 
	{ 
	}

	/**
	 * Returns if the allocator is guaranteed to be thread-safe and therefore
	 * doesn't need a unnecessary thread-safety wrapper around it.
	 */
	virtual UBOOL IsInternallyThreadSafe() const 
	{ 
		return FALSE; 
	}

	/**
	 * Gathers all current memory stats
	 *
	 * @param FMemoryAllocationStats	[out] structure containing information about the size of allocations
	 */
	virtual void GetAllocationInfo( FMemoryAllocationStats& MemStats ) 
	{
	}

	/**
	 * Dumps details about all allocations to an output device
	 *
	 * @param Ar	[in] Output device
	 */
	virtual void DumpAllocations( class FOutputDevice& Ar ) 
	{
		Ar.Logf( TEXT( "DumpAllocations not implemented" ) );
	}

	/**
	 * Validates the allocator's heap
	 */
	virtual UBOOL ValidateHeap()
	{
		return( TRUE );
	}

	/**
	 * Keeps trying to allocate memory until we fail
	 *
	 * @param Ar Device to send output to
	 */
	virtual void CheckMemoryFragmentationLevel( class FOutputDevice& Ar ) 
	{ 
		Ar.Log( TEXT("CheckMemoryFragmentationLevel not implemented") ); 
	}

	/**
	 * If possible give memory back to the os from unused segments
	 *
	 * @param ReservePad - amount of space to reserve when trimming
	 * @param bShowStats - log stats about how much memory was actually trimmed. Disable this for perf
	 * @return TRUE if succeeded
	 */
	virtual UBOOL TrimMemory(SIZE_T /*ReservePad*/,UBOOL bShowStats=FALSE) 
	{ 
		return FALSE; 
	}

	/** Total number of calls Malloc, if implemented by derived class. */
	static QWORD TotalMallocCalls;
	/** Total number of calls Malloc, if implemented by derived class. */
	static QWORD TotalFreeCalls;
	/** Total number of calls Malloc, if implemented by derived class. */
	static QWORD TotalReallocCalls;
	/** Total number of calls to PhysicalAlloc, if implemented by derived class. */
	static QWORD TotalPhysicalAllocCalls;
	/** Total number of calls to PhysicalFree, if implemented by derived class. */
	static QWORD TotalPhysicalFreeCalls;
};

/*-----------------------------------------------------------------------------
	Memory functions.
-----------------------------------------------------------------------------*/

/** @name Memory functions */
//@{
/** Copies count bytes of characters from Src to Dest. If some regions of the source
 * area and the destination overlap, memmove ensures that the original source bytes
 * in the overlapping region are copied before being overwritten.  NOTE: make sure
 * that the destination buffer is the same size or larger than the source buffer!
 */
void* appMemmove( void* Dest, const void* Src, INT Count );
INT appMemcmp( const void* Buf1, const void* Buf2, INT Count );
UBOOL appMemIsZero( const void* V, int Count );
DWORD appMemCrc( const void* Data, INT Length, DWORD CRC=0 );
void appMemswap( void* Ptr1, void* Ptr2, DWORD Size );

/**
 * Sets the first Count chars of Dest to the character C.
 */
#define appMemset( Dest, C, Count )			memset( Dest, C, Count )

#ifndef DEFINED_appMemcpy
	#define appMemcpy( Dest, Src, Count )	memcpy( Dest, Src, Count )
	/** On some platforms memcpy optimized for big blocks is available */
	#define appBigBlockMemcpy( Dest, Src, Count )	memcpy( Dest, Src, Count )
	/** On some platforms memcpy optimized for big blocks that avoid L2 cache pollution are available */
	#define appStreamingMemcpy( Dest, Src, Count )	memcpy( Dest, Src, Count )
#endif

#ifndef DEFINED_appMemzero
	#define appMemzero( Dest, Count )		memset( Dest, 0, Count )
#endif

//
// C style memory allocation stubs.
//
/** 
 * appMallocQuantizeSize returns the actual size of allocation request likely to be returned
 * so for the template containers that use slack, they can more wisely pick
 * appropriate sizes to grow and shrink to.
 *
 * @param Size			The size of a hypothetical allocation request
 * @param Alignment		The alignment of a hypothetical allocation request
 * @return				Returns the usable size that the allocation request would return. In other words you can ask for this greater amount without using any more actual memory.
 */
extern DWORD appMallocQuantizeSize( DWORD Size, DWORD Alignment=DEFAULT_ALIGNMENT );
extern void* appMalloc( DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT );
extern void* appRealloc( void* Original, DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT );
extern void appFree( void* Original );
extern void* appPhysicalAlloc( DWORD Count, ECacheBehaviour CacheBehaviour = CACHE_WriteCombine );
extern void appPhysicalFree( void* Original );

#endif
