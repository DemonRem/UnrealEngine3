/*=============================================================================
	FMallocProxySimpleTrack.h: Simple named-section tracking for allocations.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "TrackAllocSections.h"

class FMallocProxySimpleTrack : public FMalloc
{
public:
	struct FSimpleAlloc
	{
		INT Size;
		TCHAR Section[256];
		INT Num;
	};
	IMPLEMENT_COMPARE_CONSTREF(FSimpleAlloc,FMallocProxySimpleTrack,{ return appStrnicmp(A.Section,B.Section,256) <= 0 ? 1 : -1; })

	/** Allocator we are actually passing requests to. */
	FMalloc*							UsedMalloc;
	/** Dynamic map from allocation pointer to size.			*/
	TDynamicMap<PTRINT,FSimpleAlloc>	AllocToSizeMap;
	/** Total size of current allocations in bytes.				*/
	SIZE_T								TotalAllocSize;
	/** Number of allocations.									*/
	SIZE_T								NumAllocs;
	/** Used to avoid re-entrancy (ie when doing map allocations) */
	UBOOL								bIsTracking;

	FMallocProxySimpleTrack(FMalloc* InMalloc)
		:	UsedMalloc( InMalloc )
	{
		TotalAllocSize = 0;
		NumAllocs = 0;
		bIsTracking = FALSE;
	}

	/** Create the synch object if we are tracking allocations */
	virtual ~FMallocProxySimpleTrack(void)
	{
	}

	void* Malloc( DWORD Size, DWORD Alignment )
	{
		void* Pointer = UsedMalloc->Malloc(Size, Alignment);
		AddAllocation( Pointer, Size, GAllocSectionState.GetCurrentSectionName() );
		return Pointer;
	}
	void* Realloc( void* Ptr, DWORD NewSize, DWORD Alignment )
	{
		RemoveAllocation( Ptr );
		void* Pointer = UsedMalloc->Realloc(Ptr, NewSize, Alignment);
		AddAllocation( Pointer, NewSize, GAllocSectionState.GetCurrentSectionName() );
		return Pointer;
	}
	void Free( void* Ptr )
	{
		RemoveAllocation( Ptr );
		UsedMalloc->Free(Ptr);
	}
	void* PhysicalAlloc( DWORD Size, ECacheBehaviour InCacheBehaviour )
	{
		void* Pointer = UsedMalloc->PhysicalAlloc(Size, InCacheBehaviour);
		AddAllocation( Pointer, Size, GAllocSectionState.GetCurrentSectionName() );
		return Pointer;
	}
	void PhysicalFree( void* Ptr )
	{
		RemoveAllocation( Ptr );
		UsedMalloc->PhysicalFree(Ptr);
	}

	void GetAllocationInfo( SIZE_T& Virtual, SIZE_T& Physical )
	{
		UsedMalloc->GetAllocationInfo( Virtual, Physical );
	}

	/**
	 * Add allocation to keep track of.
	 *
	 * @param	Pointer		Allocation
	 * @param	Size		Allocation size in bytes
	 */
	void AddAllocation( void* Pointer, SIZE_T Size, const TCHAR* Section )
	{
		if(!GExitPurge && !bIsTracking)
		{
			bIsTracking = TRUE;

			NumAllocs++;
			TotalAllocSize += Size;

			FSimpleAlloc Alloc;
			appMemzero(&Alloc, sizeof(FSimpleAlloc));
			appStrncpy(Alloc.Section, Section, 256);
			Alloc.Size = Size;
			Alloc.Num = 1;

			AllocToSizeMap.Set( (PTRINT) Pointer, Alloc );

			bIsTracking = FALSE;
		}
	}
	
	/**
	 * Remove allocation from list to track.
	 *
	 * @param	Pointer		Allocation
	 */
	void RemoveAllocation( void* Pointer )
	{
		if(!GExitPurge && !bIsTracking)
		{
			if(Pointer)
			{
				bIsTracking = TRUE;

				NumAllocs--;
				FSimpleAlloc* AllocPtr = AllocToSizeMap.Find( (PTRINT) Pointer );
				check(AllocPtr);
				TotalAllocSize -= AllocPtr->Size;
				AllocToSizeMap.Remove( (PTRINT) Pointer );

				bIsTracking = FALSE;
			}
		}
	}

	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		if( ParseCommand(&Cmd,TEXT("DUMPALLOCS")) )
		{
			// Group allocations by file/line
			TArray<FSimpleAlloc> GroupedAllocs;

			// Iterate over all allocations
			for ( TDynamicMap<PTRINT,FSimpleAlloc>::TIterator AllocIt(AllocToSizeMap); AllocIt; ++AllocIt )
			{
				FSimpleAlloc Alloc = AllocIt.Value();

				// See if we have this location already.
				INT EntryIndex = INDEX_NONE;
				for(INT i=0; i<GroupedAllocs.Num(); i++)
				{
					if((appStrcmp(GroupedAllocs(i).Section, Alloc.Section) == 0))
					{
						EntryIndex = i;
						break;
					}
				}

				// If we found it - update size
				if(EntryIndex != INDEX_NONE)
				{
					FSimpleAlloc& FoundAlloc = GroupedAllocs(EntryIndex);
					FoundAlloc.Size += Alloc.Size;
					FoundAlloc.Num++;
				}
				// If we didn't add to array.
				else
				{
					GroupedAllocs.AddItem(Alloc);
				}
			}

			// sort if desired
			UBOOL bAlphaSort = ParseParam( Cmd, TEXT("ALPHASORT") );
			if( bAlphaSort )
			{
				Sort<USE_COMPARE_CONSTREF(FSimpleAlloc,FMallocProxySimpleTrack)>(&GroupedAllocs(0),GroupedAllocs.Num());
			}

			// Now print out amount allocated for each allocating location.
			Ar.Logf( TEXT("===== Dumping stat allocations [Section],[Size],[NumAllocs]") );			
			for(INT AllocIndex = 0; AllocIndex < GroupedAllocs.Num(); AllocIndex++)
			{
				FSimpleAlloc& Alloc = GroupedAllocs(AllocIndex);
				Ar.Logf( TEXT("%s,%d,%d"), Alloc.Section, Alloc.Size, Alloc.Num );
			}

			return TRUE;
		}

		return UsedMalloc->Exec(Cmd, Ar);
	}
};

