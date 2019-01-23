/*=============================================================================
	SparseArray.h: Sparse array definition.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Forward declarations.
template<typename ElementType>
class TSparseArray;

/**
 * The result of a sparse array allocation.
 */
template<typename ElementType>
struct TSparseArrayAllocationInfo
{
	TSparseArray<ElementType>* Array;
	INT Index;
};

/**
 * A sparse array which allows for constant time removal of elements while still allowing indexing.
 */
template<typename ElementType>
class TSparseArray
{
public:

	/**
	 * Allocates space for an element in the array.  The element is not initialized, and you must use the corresponding placement new operator
	 * to construct the element in the allocated memory.
	 */
	TSparseArrayAllocationInfo<ElementType> Add()
	{
		TSparseArrayAllocationInfo<ElementType> Result;
		Result.Array = this;

		if(FreeElements.Num())
		{
			// Allocate one of the free elements for the item.
			Result.Index = FreeElements.Pop();

			// Destruct the previous element at this index.
			Elements(Result.Index).~ElementType();
		}
		else
		{
			// Add a new element for the item.
			Result.Index = Elements.Add();
			AllocationFlags.AddItem(TRUE);
		}

		// Flag the element as allocated.
		AllocationFlags(Result.Index) = TRUE;

		return Result;
	}

	/**
	 * Adds an element to the array.
	 */
	INT AddItem(typename TTypeInfo<ElementType>::ConstInitType Item)
	{
		TSparseArrayAllocationInfo<ElementType> Allocation = Add();
		new(Allocation) ElementType(Item);
		return Allocation.Index;
	}

	/**
	 * Removes an element from the array.
	 */
	void Remove(INT Index)
	{
		check(AllocationFlags(Index));

		// Add the element to the free element pool.
		FreeElements.AddItem(Index);
		AllocationFlags(Index) = FALSE;

		// Don't destruct the element, simply replace it with a default element.  This destructs the element, but prevents a double-destruction
		// from occurring when the sparse array is destructed with free elements in it.
		Elements(Index) = ElementType();
	}

	// Accessors.
	ElementType& operator()(INT Index) { return Elements(Index); }
	const ElementType& operator()(INT Index) const { return Elements(Index); }
	UBOOL IsAllocated(INT Index) const { return AllocationFlags(Index); }
	INT GetMaxIndex() const { return Elements.Num(); }
	INT Num() const { return Elements.Num() - FreeElements.Num(); }

	// Iterator.
	class TIterator
	{
	public:
		TIterator( TSparseArray<ElementType>& InArray ):
			Array(InArray),
			BitArrayIt(InArray.AllocationFlags)
		{}
		FORCEINLINE void operator++()
		{
			// Iterate to the next set allocation flag.
			++BitArrayIt;
		}
		FORCEINLINE INT GetIndex() const { return BitArrayIt.GetIndex(); }
		FORCEINLINE operator UBOOL() const { return (UBOOL)BitArrayIt; }
		FORCEINLINE ElementType& operator*() const { return Array(GetIndex()); }
		FORCEINLINE ElementType* operator->() const { return &Array(GetIndex()); }
		FORCEINLINE operator const FBitArray::FBaseIterator& () const { return BitArrayIt; }
	private:
		TSparseArray<ElementType>& Array;
		FBitArray::FConstSetBitIterator BitArrayIt;
	};

	// Const Iterator.
	class TConstIterator
	{
	public:
		TConstIterator( const TSparseArray<ElementType>& InArray ):
			Array(InArray),
			BitArrayIt(InArray.AllocationFlags)
		{}
		FORCEINLINE void operator++()
		{
			// Iterate to the next set allocation flag.
			++BitArrayIt;
		}
		FORCEINLINE INT GetIndex() const { return BitArrayIt.GetIndex(); }
		FORCEINLINE operator UBOOL() const { return (UBOOL)BitArrayIt; }
		FORCEINLINE const ElementType& operator*() const { return Array(GetIndex()); }
		FORCEINLINE const ElementType* operator->() const { return &Array(GetIndex()); }
		FORCEINLINE operator const FBitArray::FBaseIterator& () const { return BitArrayIt; }
	private:
		const TSparseArray<ElementType>& Array;
		FBitArray::FConstSetBitIterator BitArrayIt;
	};

	/** An iterator which only iterates over the elements of the array which correspond to set bits in a separate bit array. */
	class TConstSubsetIterator
	{
	public:
		TConstSubsetIterator( const TSparseArray<ElementType>& InArray, const FBitArray& InBitArray ):
			Array(InArray),
			BitArrayIt(InArray.AllocationFlags,InBitArray)
		{}
		FORCEINLINE void operator++()
		{
			// Iterate to the next element which is both allocated and has its bit set in the other bit array.
			++BitArrayIt;
		}
		FORCEINLINE INT GetIndex() const { return BitArrayIt.GetIndex(); }
		FORCEINLINE operator UBOOL() const { return (UBOOL)BitArrayIt; }
		FORCEINLINE const ElementType& operator*() const { return Array(GetIndex()); }
		FORCEINLINE const ElementType* operator->() const { return &Array(GetIndex()); }
		FORCEINLINE operator const FBitArray::FBaseIterator& () const { return BitArrayIt; }
	private:
		const TSparseArray<ElementType>& Array;
		FBitArray::FConstDualSetBitIterator BitArrayIt;
	};

private:
	TArray<ElementType> Elements;
	TArray<INT> FreeElements;
	FBitArray AllocationFlags;
};

/**
 * A placement new operator which constructs an element in a sparse array allocation.
 */
template<typename T> void* operator new(size_t Size,const TSparseArrayAllocationInfo<T>& Allocation)
{
	return &(*Allocation.Array)(Allocation.Index);
}
