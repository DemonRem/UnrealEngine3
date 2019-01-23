/*=============================================================================
	BitArray.cpp: Bit array implementation
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

FBitArray::FBitReference::operator UBOOL() const
{
	 return (Data & Mask) != 0;
}

void FBitArray::FBitReference::operator=(const UBOOL NewValue)
{
	if(NewValue)
	{
		Data |= Mask;
	}
	else
	{
		Data &= ~Mask;
	}
}

FBitArray::FBitArray( const UBOOL Value, const UINT InNumBits ):
	Data(NULL),
	NumBits(InNumBits),
	MaxBits(InNumBits)
{
	Realloc();
	if(NumBits)
	{
		appMemset(Data,Value ? 0xff : 0,(NumBits + NumBitsPerDWORD - 1) / NumBitsPerDWORD * sizeof(DWORD));
	}
}

FBitArray::FBitArray(const FBitArray& Copy):
	Data(NULL)
{
	*this = Copy;
}

FBitArray::~FBitArray()
{
	Empty();
}

FBitArray& FBitArray::operator =(const FBitArray& Copy)
{
	// check for self assignment since we don't use swamp() mechanic
	if( this == &Copy )
	{
		return *this;
	}

	Empty();
	NumBits = MaxBits = Copy.NumBits;
	if(NumBits)
	{
		const INT NumDWORDs = (MaxBits + NumBitsPerDWORD - 1) / NumBitsPerDWORD;
		Realloc();
		appMemcpy(Data,Copy.Data,NumDWORDs * sizeof(DWORD));
	}
	return *this;
}

INT FBitArray::AddItem(const UBOOL Value)
{
	const INT Index = NumBits;
	const UBOOL bReallocate = (NumBits + 1) > MaxBits;

	NumBits++;

	if(bReallocate)
	{
		// Allocate memory for the new bits.
		const UINT MaxDWORDs = (NumBits + 3 * NumBits / 8 + 31 + NumBitsPerDWORD - 1) / NumBitsPerDWORD;
		MaxBits = MaxDWORDs * NumBitsPerDWORD;
		Realloc();
	}

	(*this)(Index) = Value;

	return Index;
}

void FBitArray::Empty()
{
	if(Data != ReservedData)
	{
		appFree(Data);
	}
	Data = NULL;
	NumBits = MaxBits = 0;
}

void FBitArray::Realloc()
{
	if(Data || MaxBits)
	{
		const INT NumDWORDs = (NumBits + NumBitsPerDWORD - 1) / NumBitsPerDWORD;
		const INT MaxDWORDs = (MaxBits + NumBitsPerDWORD - 1) / NumBitsPerDWORD;

		if(MaxDWORDs <= NumReservedDWORDs)
		{
			// If the old allocation wasn't in the reserved data area, move it into the reserved data area.
			if(Data != ReservedData && Data != NULL)
			{
				appMemcpy(ReservedData,Data,NumDWORDs * sizeof(DWORD));
				appFree(Data);
			}

			// The new allocation will be in the reserved data area.
			Data = ReservedData;
		}
		else if(Data == ReservedData)
		{
			// The new allocation will be on the heap.
			Data = appMalloc(MaxDWORDs * sizeof(DWORD));

			// Move the data out of the reserved data area into the new allocation.
			appMemcpy(Data,ReservedData,NumDWORDs * sizeof(DWORD));
		}
		else
		{
			// Reallocate the data for the new size.
			Data = appRealloc(Data,MaxDWORDs * sizeof(DWORD));
		}
	}
}
