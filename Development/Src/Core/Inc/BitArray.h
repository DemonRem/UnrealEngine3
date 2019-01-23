/*=============================================================================
	BitArray.h: Bit array definition.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * A dynamically sized bit array.
 */
class FBitArray
{
private:
	enum { NumBitsPerDWORD = 32 };
public:

	/** Used to access a bit in the array as a UBOOL. */
	class FBitReference
	{
	public:

		FBitReference(DWORD& InData,DWORD InMask):
			Data(InData),
			Mask(InMask)
		{}

		operator UBOOL() const;
		void operator=(const UBOOL NewValue);

	private:
		DWORD& Data;
		DWORD Mask;
	};

	/** The base type of the bit array iterators. Can be used to access the element with the same index in another array. */
	class FBaseIterator
	{
		friend class FBitArray;
	protected:

		/** Initialization constructor. */
		FBaseIterator(DWORD InCurrentBitMask,INT InDWORDIndex):
			CurrentBitMask(InCurrentBitMask),
			DWORDIndex(InDWORDIndex)
		{}

		DWORD CurrentBitMask;
		INT DWORDIndex;
	};

	/**
	 * Minimal initialization constructor.
	 * @param Value - The value to initial the bits to.
	 * @param InNumBits - The initial number of bits in the array.
	 */
	FBitArray( const UBOOL Value = FALSE, const UINT InNumBits = 0 );

	/**
	 * Copy constructor.
	 */
	FBitArray(const FBitArray& Copy);

	/**
	 * Assignment operator.
	 */
	FBitArray& operator=(const FBitArray& Copy);

	/**
	 * Destructor.
	 */
	~FBitArray();

	/**
	 * Adds a bit to the array with the given value.
	 * @return The index of the added bit.
	 */
	INT AddItem(const UBOOL Value);

	/**
	 * Removes all bits from the array.
	 */
	void Empty();

	// Accessors.
	FORCEINLINE INT Num() const { return NumBits; }
	FORCEINLINE FBitReference operator()(INT Index)
	{
		checkSlow(Index >= 0);
		checkSlow(Index < NumBits);
		return FBitArray::FBitReference(
			((DWORD*)Data)[Index / NumBitsPerDWORD],
			1 << (Index & (NumBitsPerDWORD - 1))
			);
	}
	FORCEINLINE const FBitReference operator()(INT Index) const
	{
		checkSlow(Index >= 0);
		checkSlow(Index < NumBits);
		return FBitArray::FBitReference(
			((DWORD*)Data)[Index / NumBitsPerDWORD],
			1 << (Index & (NumBitsPerDWORD - 1))
			);
	}
	FORCEINLINE FBitReference AccessCorrespondingBit(const FBaseIterator& Iterator)
	{
		checkSlow(Iterator.CurrentBitMask);
		checkSlow(Iterator.DWORDIndex >= 0);
		checkSlow(((UINT)Iterator.DWORDIndex + 1) * NumBitsPerDWORD - 1 - appCountLeadingZeros(Iterator.CurrentBitMask) < (UINT)NumBits);
		return FBitArray::FBitReference(
			((DWORD*)Data)[Iterator.DWORDIndex],
			Iterator.CurrentBitMask
			);
	}
	FORCEINLINE const FBitReference AccessCorrespondingBit(const FBaseIterator& Iterator) const
	{
		checkSlow(Iterator.CurrentBitMask);
		checkSlow(Iterator.DWORDIndex >= 0);
		checkSlow(((UINT)Iterator.DWORDIndex + 1) * NumBitsPerDWORD - 1 - appCountLeadingZeros(Iterator.CurrentBitMask) < (UINT)NumBits);
		return FBitArray::FBitReference(
			((DWORD*)Data)[Iterator.DWORDIndex],
			Iterator.CurrentBitMask
			);
	}

	/** BitArray iterator. */
	class FConstIterator : public FBaseIterator
	{
	public:
		FConstIterator( const FBitArray& InArray ): FBaseIterator(1,0), Array(InArray), DataPointer((DWORD*)InArray.Data) {}
		FORCEINLINE void operator++()
		{
			++DWORDIndex;

			CurrentBitMask <<= 1;
			if(!CurrentBitMask)
			{
				// Advance to the next DWORD.
				DataPointer++;
				CurrentBitMask = 1;
			}
		}
		FORCEINLINE operator UBOOL() const { return DWORDIndex < Array.Num(); }
		FORCEINLINE UBOOL GetValue() const { return (*DataPointer) & CurrentBitMask; }
	private:
		const FBitArray& Array;
		DWORD* DataPointer;
	};

	/** An iterator which only iterates over set bits. */
	class FConstSetBitIterator : public FBaseIterator
	{
	public:

		/** Constructor. */
		FConstSetBitIterator(const FBitArray& InArray):
			FBaseIterator(0,-1),
			DataPointer((DWORD*)InArray.Data),
			BaseBitIndex(-NumBitsPerDWORD),
			NumDWORDs((InArray.Num() + NumBitsPerDWORD - 1) / NumBitsPerDWORD),
			RemainingBitMask(0),
			CurrentBitIndex(0)
		{
			if(NumDWORDs)
			{
				// Reset the unallocated bits of the top DWORD to zero.
				const DWORD CleanMask = 0xFFFFFFFF >> (NumDWORDs * NumBitsPerDWORD - InArray.Num());
				DataPointer[NumDWORDs - 1] &= CleanMask;
			}

			++(*this);
		}

		/** Advancement operator. */
		FORCEINLINE void operator++()
		{
			// Remove the current bit from RemainingBitMask.
			RemainingBitMask &= ~CurrentBitMask;

			// Advance to the next non-zero DWORD.
			while(!RemainingBitMask)
			{
				DWORDIndex++;
				BaseBitIndex += NumBitsPerDWORD;
				if(DWORDIndex <= NumDWORDs - 1)
				{
					RemainingBitMask = DataPointer[DWORDIndex];
				}
				else
				{
					// We've advanced past the end of the array, zero the BitMask and return.
					RemainingBitMask = 0;
					return;
				}
			};

			// We can assume that RemainingBitMask!=0 here.
			checkSlow(RemainingBitMask);

			// This operation has the effect of unsetting the lowest set bit of BitMask
			const DWORD NewRemainingBitMask = RemainingBitMask & (RemainingBitMask - 1);

			// This operation XORs the above mask with the original mask, which has the effect
			// of returning only the bits which differ; specifically, the lowest bit
			CurrentBitMask = NewRemainingBitMask ^ RemainingBitMask;

			// If the Nth bit was the lowest set bit of BitMask, then this gives us N
			CurrentBitIndex = BaseBitIndex + NumBitsPerDWORD - 1 - appCountLeadingZeros(CurrentBitMask);
		}
		/** Completion test operator. */
		FORCEINLINE operator UBOOL() const
		{
			// If BitMask=0, we have reached the end of the bit array.
			return RemainingBitMask;
		}
		/** Index accessor. */
		FORCEINLINE INT GetIndex() const
		{
			return CurrentBitIndex;
		}

	private:

		DWORD* DataPointer;
		INT BaseBitIndex;
		INT NumDWORDs;

		DWORD RemainingBitMask;
		INT CurrentBitIndex;
	};

	/** An iterator which only iterates over the bits which are set in both of two bit-arrays. */
	class FConstDualSetBitIterator : public FBaseIterator
	{
	public:

		/** Constructor. */
		FConstDualSetBitIterator(const FBitArray& InArrayA,const FBitArray& InArrayB):
			FBaseIterator(0,-1),
			DataPointerA((DWORD*)InArrayA.Data),
			DataPointerB((DWORD*)InArrayB.Data),
			BaseBitIndex(-NumBitsPerDWORD),
			NumDWORDs((InArrayA.Num() + NumBitsPerDWORD - 1) / NumBitsPerDWORD),
			RemainingBitMask(0),
			CurrentBitIndex(0)
		{
			check(InArrayA.Num() == InArrayB.Num());

			if(NumDWORDs)
			{
				// Reset the unallocated bits of the top DWORD to zero.
				const DWORD CleanMask = 0xFFFFFFFF >> (NumDWORDs * NumBitsPerDWORD - InArrayA.Num());
				DataPointerA[NumDWORDs - 1] &= CleanMask;
				DataPointerB[NumDWORDs - 1] &= CleanMask;
			}
			
			++(*this);
		}

		/** Advancement operator. */
		FORCEINLINE void operator++()
		{
			// Remove the previously iterated bit from RemainingBitMask.
			RemainingBitMask &= ~CurrentBitMask;

			// Advance to the next non-zero DWORD.
			while(!RemainingBitMask)
			{
				DWORDIndex++;
				BaseBitIndex += NumBitsPerDWORD;
				if(DWORDIndex <= NumDWORDs - 1)
				{
					RemainingBitMask = DataPointerA[DWORDIndex] & DataPointerB[DWORDIndex];
				}
				else
				{
					// We've advanced past the end of the array, zero the BitMask and return.
					RemainingBitMask = 0;
					return;
				}
			};

			// We can assume that RemainingBitMask!=0 here.
			checkSlow(RemainingBitMask);

			// This operation has the effect of unsetting the lowest set bit of BitMask
			const DWORD NewRemainingBitMask = RemainingBitMask & (RemainingBitMask - 1);

			// This operation XORs the above mask with the original mask, which has the effect
			// of returning only the bits which differ; specifically, the lowest bit
			CurrentBitMask = NewRemainingBitMask ^ RemainingBitMask;

			// If the Nth bit was the lowest set bit of BitMask, then this gives us N
			CurrentBitIndex = BaseBitIndex + NumBitsPerDWORD - 1 - appCountLeadingZeros(CurrentBitMask);
		}
		/** Completion test operator. */
		FORCEINLINE operator UBOOL() const
		{
			// If BitMask=0, we have reached the end of the bit array.
			return RemainingBitMask;
		}
		/** Index accessor. */
		FORCEINLINE INT GetIndex() const
		{
			return CurrentBitIndex;
		}

	private:

		DWORD* DataPointerA;
		DWORD* DataPointerB;
		INT BaseBitIndex;
		INT NumDWORDs;

		DWORD RemainingBitMask;
		INT CurrentBitIndex;
	};

private:
	void* Data;
	INT NumBits;
	INT MaxBits;

	enum { NumReservedDWORDs = 4 };
	DWORD ReservedData[NumReservedDWORDs];

	void Realloc();
};
