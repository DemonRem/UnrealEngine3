/*=============================================================================
	FMallocTBB.h: Intel-TBB 64-bit scalable memory allocator.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __FMALLOCTBB_H__
#define __FMALLOCTBB_H__

// Only use for WIN64
#ifdef _WIN64

#ifdef _DEBUG
	// Undefing _DEBUG to always like the release allocator lib.
	#define UNDEFD_DEBUG
	#undef _DEBUG
#endif
#include <tbb/scalable_allocator.h>
#ifdef UNDEFD_DEBUG
	#define _DEBUG
#endif

#define MEM_TIME(st)

//
// TBB 64-bit scalable memory allocator.
//
class FMallocTBB : public FMalloc
{
private:
	DOUBLE		MemTime;

	// Implementation. 
	void OutOfMemory()
	{
		appErrorf( *LocalizeError("OutOfMemory",TEXT("Core")) );
	}

public:
	// FMalloc interface.
	FMallocTBB() :
		MemTime		( 0.0 )
	{
	}
	
	/** 
	 * Malloc
	 */
	virtual void* Malloc( DWORD Size, DWORD Alignment )
	{
		check(Alignment == DEFAULT_ALIGNMENT && "Alignment currently unsupported in Windows");
		MEM_TIME(MemTime -= appSeconds());
		void* Free = scalable_malloc(Size);
		if( !Free )
		{
			OutOfMemory();
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
		void* NewPtr = scalable_realloc(Ptr, NewSize);
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
		scalable_free(Ptr);
		MEM_TIME(MemTime += appSeconds());
	}

	/**
	 * Dumps details about all allocations to an output device
	 *
	 * @param Ar	[in] Output device
	 */
	virtual void DumpAllocations( FOutputDevice& Ar )
	{
		STAT(Ar.Logf( TEXT("Memory Allocation Status") ));
		MEM_TIME(Ar.Logf( "Seconds     % 5.3f", MemTime ));
	}

	/**
	 * Returns stats about current memory usage.
	 *
	 * @param FMemoryAllocationStats	[out] structure containing information about the size of allocations
	 */
	virtual void GetAllocationInfo( FMemoryAllocationStats& MemStats );
};


#endif	// #ifdef _WIN64

#endif	//#ifndef __FMALLOCTBB_H__
