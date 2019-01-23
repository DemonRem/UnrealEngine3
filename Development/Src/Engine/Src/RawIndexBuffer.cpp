/*=============================================================================
	RawIndexBuffer.cpp: Raw index buffer implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

#if !CONSOLE && !PLATFORM_UNIX
#include "NVTriStrip.h"
#endif

/*-----------------------------------------------------------------------------
FRawIndexBuffer
-----------------------------------------------------------------------------*/

INT FRawIndexBuffer::Stripify()
{
	return StripifyIndexBuffer(Indices);
}

void FRawIndexBuffer::CacheOptimize()
{
	CacheOptimizeIndexBuffer(Indices);
}

void FRawIndexBuffer::InitRHI()
{
	DWORD Size = Indices.Num() * sizeof(WORD);
	if( Size > 0 )
	{
		// Create the index buffer.
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(WORD),Size,NULL,FALSE);

		// Initialize the buffer.
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI,0,Size);
		appMemcpy(Buffer,&Indices(0),Size);
		RHIUnlockIndexBuffer(IndexBufferRHI);		
	}
}

FArchive& operator<<(FArchive& Ar,FRawIndexBuffer& I)
{
	I.Indices.BulkSerialize( Ar );
	if(Ar.Ver() < VER_RENDERING_REFACTOR)
	{
		DWORD Size = 0;
		Ar << Size;
	}
	return Ar;
}

/*-----------------------------------------------------------------------------
FRawIndexBuffer32
-----------------------------------------------------------------------------*/

void FRawIndexBuffer32::InitRHI()
{
	DWORD Size = Indices.Num() * sizeof(DWORD);
	if( Size > 0 )
	{
		// Create the index buffer.
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(DWORD),Size,NULL,FALSE);

		// Initialize the buffer.
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI,0,Size);
		appMemcpy(Buffer,&Indices(0),Size);
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
}

FArchive& operator<<(FArchive& Ar,FRawIndexBuffer32& I)
{
	I.Indices.BulkSerialize( Ar );
	if(Ar.Ver() < VER_RENDERING_REFACTOR)
	{
		DWORD Size = 0;
		Ar << Size;
	}
	return Ar;
}

/**
 * Converts a triangle list into a triangle strip.
 */
INT StripifyIndexBuffer(TArray<WORD>& Indices)
{
#if !CONSOLE && !PLATFORM_UNIX

	PrimitiveGroup*	PrimitiveGroups = NULL;
	WORD			NumPrimitiveGroups = 0;

	SetListsOnly(false);
	GenerateStrips(&Indices(0),Indices.Num(),&PrimitiveGroups,&NumPrimitiveGroups);

	Indices.Empty();
	Indices.Add(PrimitiveGroups->numIndices);
	appMemcpy(&Indices(0),PrimitiveGroups->indices,Indices.Num() * sizeof(WORD));

	delete [] PrimitiveGroups;

	return Indices.Num() - 2;
#else
	return 0;
#endif
}

/**
 * Orders a triangle list for better vertex cache coherency.
 */
void CacheOptimizeIndexBuffer(TArray<WORD>& Indices)
{
#if !CONSOLE && !PLATFORM_UNIX

	PrimitiveGroup*	PrimitiveGroups = NULL;
	WORD			NumPrimitiveGroups = 0;

	SetListsOnly(true);
	GenerateStrips(&Indices(0),Indices.Num(),&PrimitiveGroups,&NumPrimitiveGroups);

	Indices.Empty();
	Indices.Add(PrimitiveGroups->numIndices);
	appMemcpy(&Indices(0),PrimitiveGroups->indices,Indices.Num() * sizeof(WORD));

	delete [] PrimitiveGroups;
#else
	check(0);
#endif
}
