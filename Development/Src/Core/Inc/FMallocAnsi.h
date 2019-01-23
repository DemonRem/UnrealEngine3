/*=============================================================================
	FMallocAnsi.h: ANSI memory allocator.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifdef _MSC_VER
#define USE_ALIGNED_MALLOC 1
#else
//@todo gcc: this should be implemented more elegantly on other platforms
#define USE_ALIGNED_MALLOC 0
#endif

//
// ANSI C memory allocator.
//
class FMallocAnsi : public FMalloc
{
	// Alignment.
	enum {ALLOCATION_ALIGNMENT=16};

public:
	// FMalloc interface.
	virtual void* Malloc( DWORD Size, DWORD Alignment )
	{
		check(Alignment == DEFAULT_ALIGNMENT && "Alignment currently unsupported in Ansi Malloc");
#if USE_ALIGNED_MALLOC
		void* Ptr = _aligned_malloc( Size, ALLOCATION_ALIGNMENT );
		check(Ptr);
		return Ptr;
#else
		void* Ptr = malloc( Size + ALLOCATION_ALIGNMENT + sizeof(void*) + sizeof(DWORD) );
		check(Ptr);
		void* AlignedPtr = Align( (BYTE*)Ptr + sizeof(void*) + sizeof(DWORD), ALLOCATION_ALIGNMENT );
		*((void**)( (BYTE*)AlignedPtr - sizeof(void*)					))	= Ptr;
		*((DWORD*)( (BYTE*)AlignedPtr - sizeof(void*) - sizeof(DWORD)	))	= Size;
		return AlignedPtr;
#endif
	}

	virtual void* Realloc( void* Ptr, DWORD NewSize, DWORD Alignment )
	{
		check(Alignment == DEFAULT_ALIGNMENT && "Alignment currently unsupported in Ansi Malloc");
		void* Result;
#if USE_ALIGNED_MALLOC
		if( Ptr && NewSize )
		{
			Result = _aligned_realloc( Ptr, NewSize, ALLOCATION_ALIGNMENT );
		}
		else if( Ptr == NULL )
		{
			Result = _aligned_malloc( NewSize, ALLOCATION_ALIGNMENT );
		}
		else
		{
			_aligned_free( Ptr );
			Result = NULL;
		}
#else
		if( Ptr && NewSize )
		{
			// Can't use realloc as it might screw with alignment.
			Result = Malloc( NewSize );
			appMemcpy( Result, Ptr, Min(NewSize, *((DWORD*)( (BYTE*)Ptr - sizeof(void*) - sizeof(DWORD))) ) );
			Free( Ptr );
		}
		else if( Ptr == NULL )
		{
			Result = Malloc( NewSize );
		}
		else
		{
			free( *((void**)((BYTE*)Ptr-sizeof(void*))) );
			Result = NULL;
		}
#endif
		return Result;
	}

	virtual void Free( void* Ptr )
	{
#if USE_ALIGNED_MALLOC
		_aligned_free( Ptr );
#else
		if( Ptr )
			free( *((void**)((BYTE*)Ptr-sizeof(void*))) );
#endif
	}

	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar ) 
	{ 
#if (defined _MSC_VER)
		if( ParseCommand(&Cmd,TEXT("HEAPCHECK")) )
		{
			INT Result = _heapchk();
			check(Result!=_HEAPBADBEGIN);
			check(Result!=_HEAPBADNODE);
			check(Result!=_HEAPBADPTR);
			check(Result!=_HEAPEMPTY);
			check(Result==_HEAPOK);
			return TRUE;
		}
#endif
		return FALSE; 
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

