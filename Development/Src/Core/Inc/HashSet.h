/*=============================================================================
	HashSet.h: Hash set definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * A default implementation of the KeyFuncs used by THashSet which uses the element as a key.
 */
template<class ElementType>
struct DefaultKeyFuncs
{
	typedef ElementType KeyType;
	typedef typename TTypeInfo<KeyType>::ConstInitType KeyInitType;
	typedef typename TTypeInfo<ElementType>::ConstInitType ElementInitType;

	/**
	 * @return The key used to index the given element.
	 */
	static KeyInitType GetSetKey(ElementInitType Element)
	{
		return Element;
	}

	/**
	 * @return True if the keys match.
	 */
	static UBOOL Matches(KeyInitType A,KeyInitType B)
	{
		return A == B;
	}

	/** Calculates a hash index for a key. */
	static DWORD GetTypeHash(KeyInitType Key)
	{
		return ::GetTypeHash(Key);
	}
};

/**
 * A set of elements, hashed using keys derived from the elements.  Add and Remove are constant time.
 * The KeyFuncs template parameter provides static functions which define how the keys are derived and compared.
 */
template<typename ElementType,typename KeyFuncs = DefaultKeyFuncs<ElementType>, INT AverageElementsPerBucket = 2 >
class THashSet
{
	typedef typename KeyFuncs::KeyType KeyType;
	typedef typename TTypeInfo<ElementType>::ConstInitType ElementInitType;
	typedef typename TTypeInfo<KeyType>::ConstInitType KeyInitType;
public:

	/**
	 * Minimal initialization constructor.
	 */
	THashSet():
		FirstElementLink(NULL),
		NumElements(0),
		Hash(NULL),
		HashSize(8)
	{}

	/**
	 * Copy constructor.
	 */
	THashSet(const THashSet& Copy):
		FirstElementLink(NULL),
		NumElements(0),
		Hash(NULL),
		HashSize(8)
	{
		*this = Copy;
	}

	/**
	 * Assignment operator.
	 */
	THashSet& operator=(const THashSet& Copy)
	{
		Empty();
		for(FElementLink* ElementLink = Copy.ElementLink;ElementLink;ElementLink = ElementLink->Next)
		{
			Add(ElementLink->Element);
		}
	}

	/**
	 * Destructor.
	 */
	~THashSet()
	{
		DeleteElements();
		delete Hash;
		Hash = NULL;
		HashSize = 0;
	}

	/**
	 * Removes all elements from the set.
	 */
	void Empty()
	{
		DeleteElements();
		NumElements = 0;
		HashSize = 8;
		Rehash();
	}

	/**
	 * Returns the number of elements.
	 */
	INT Num() const
	{
		return NumElements;
	}

	/**
	 * Adds an element to the set.
	 * @return A pointer to the element stored in the set.  The pointer remains valid as long as the element isn't removed from the set.
	 */
	ElementType* Add(ElementInitType InElement)
	{
		AllocHash();

		// Create a new element link for the element.
		FElementLink* ElementLink = new FElementLink(InElement);
		ElementLink->Link(FirstElementLink);
		NumElements++;

		// Add the new element link to the hash.
		INT iHash = (KeyFuncs::GetTypeHash(KeyFuncs::GetSetKey(InElement)) & (HashSize-1));
		ElementLink->HashLink(Hash[iHash]);

		// Check if the hash needs to be resized.
		ConditionalRehash();

		return &ElementLink->Element;
	}

	/**
	 * Removes an element from the set.
	 * @param Element - A pointer to the element in the set, as returned by Add or Find.
	 */
	void Remove(ElementType* Element)
	{
		// Derive the element link pointer from the element pointer.
		FElementLink* ElementLink = (FElementLink*)(FElementWrapper*)Element;

		// Remove the element link from the element list.
		*ElementLink->PrevLink = ElementLink->Next;
		if(ElementLink->Next)
		{
			ElementLink->Next->PrevLink = ElementLink->PrevLink;
		}

		// Remove the element link from the hash.
		*ElementLink->HashPrevLink = ElementLink->HashNext;
		if(ElementLink->HashNext)
		{
			ElementLink->HashNext->HashPrevLink = ElementLink->HashPrevLink;
		}

		// Delete the element link.
		delete ElementLink;
		NumElements--;

		// Check if the hash needs to be resized.
		ConditionalRehash();
	}

	/**
	 * Finds an element with the given key in the set.
	 * @param Key - The key to search for.
	 * @return A pointer to an element with the given key.  If no element in the set has the given key, this will return NULL.
	 */
	ElementType* Find(KeyInitType Key)
	{
		if(Hash)
		{
			for(FElementLink* ElementLink = Hash[(KeyFuncs::GetTypeHash(Key) & (HashSize-1))];ElementLink;ElementLink = ElementLink->HashNext)
			{
				if(KeyFuncs::Matches(KeyFuncs::GetSetKey(ElementLink->Element),Key))
				{
					return &ElementLink->Element;
				}
			}
		}
		return NULL;
	}

	/** Checks for an element in the set, and if it was found, removes it. */
	void FindAndRemove(KeyInitType Key)
	{
		ElementType* Element = Find(Key);
		if(Element)
		{
			Remove(Element);
		}
	}

	/**
	 * Checks if the element contains an element with the given key.
	 * @param Key - The key to check for.
	 * @return TRUE if the set contains an element with the given key.
	 */
	UBOOL Contains(KeyInitType Key) const
	{
		return Find(Key) != NULL;
	}

	/**
	 * Finds an element with the given key in the set.
	 * @param Key - The key to search for.
	 * @return A pointer to an element with the given key.  If no element in the set has the given key, this will return NULL.
	 */
	const ElementType* Find(KeyInitType Key) const
	{
		if(Hash)
		{
			for(FElementLink* ElementLink = Hash[(KeyFuncs::GetTypeHash(Key) & (HashSize-1))];ElementLink;ElementLink = ElementLink->HashNext)
			{
				if(KeyFuncs::Matches(KeyFuncs::GetSetKey(ElementLink->Element),Key))
				{
					return &ElementLink->Element;
				}
			}
		}
		return NULL;
	}

private:

	/**
	 * A wrapped for ElementType which allows it to be used as a base class, even if it's a pointer type.
	 */
	struct FElementWrapper
	{
		ElementType Element;
		FElementWrapper(ElementInitType InElement):
			Element(InElement)
		{}
	};

	class FElementLink : public FElementWrapper
	{
	public:
		FElementLink** HashPrevLink;
		FElementLink* HashNext;

		FElementLink** PrevLink;
		FElementLink* Next;

		FElementLink(ElementInitType InElement):
			FElementWrapper(InElement)
		{}

		void HashLink(FElementLink*& HashHead)
		{
			HashPrevLink = &HashHead;
			HashNext = HashHead;
			if(HashHead)
			{
				HashHead->HashPrevLink = &HashNext;
			}
			HashHead = this;
		}

		void Link(FElementLink*& ListHead)
		{
			PrevLink = &ListHead;
			Next = ListHead;
			if(ListHead)
			{
				ListHead->PrevLink = &Next;
			}
			ListHead = this;
		}
	};

	FElementLink* FirstElementLink;
	INT NumElements;
	FElementLink** Hash;
	INT HashSize;

	void ConditionalRehash()
	{
		INT DesiredHashSize = 1 << appCeilLogTwo(NumElements / AverageElementsPerBucket + 8);

		if(HashSize != DesiredHashSize)
		{
			HashSize = DesiredHashSize;
			Rehash();
		}
	}

	void Rehash()
	{
		checkSlow(!(HashSize&(HashSize-1)));
		FElementLink** NewHash = new FElementLink*[HashSize];
		for( INT i=0; i<HashSize; i++ )
		{
			NewHash[i] = NULL;
		}
		for(FElementLink* ElementLink = FirstElementLink;ElementLink;ElementLink = ElementLink->Next)
		{
			INT iHash = (KeyFuncs::GetTypeHash(KeyFuncs::GetSetKey(ElementLink->Element)) & (HashSize-1));
			ElementLink->HashLink(NewHash[iHash]);
		}
		if( Hash )
			delete[] Hash;
		Hash = NewHash;
	}

	FORCEINLINE void AllocHash()
	{
		if(!Hash)
		{
			Rehash();
		}
	}

	void DeleteElements()
	{
		FElementLink* PrevElementLink = FirstElementLink;
		FElementLink* ElementLink;
		while(PrevElementLink)
		{
			ElementLink = PrevElementLink->Next;
			delete PrevElementLink;
			PrevElementLink = ElementLink;
		}
		FirstElementLink = NULL;
	}

public:

	/**
	 * Used to iterate over the elements of a THashSet.
	 */
	class TConstIterator
	{
	public:
		TConstIterator(const THashSet& InSet)
		:	CurrentLink(InSet.FirstElementLink)
		{}

		/** Prefetches memory for the next link. */
		void PrefetchNext() const
		{
			if( CurrentLink )
			{
				PREFETCH( CurrentLink->Next );
			}
		}

		/**
		 * Advances the iterator to the next element.
		 */
		void Next()
		{
			checkSlow(CurrentLink);
			CurrentLink = CurrentLink->Next;
		}

		/**
		 * Checks for the end of the list.
		 */
		operator UBOOL() const { return CurrentLink != NULL; }

		// Accessors.
		ElementType* operator->() const
		{
			checkSlow(CurrentLink);
			return &CurrentLink->Element;
		}
		ElementType& operator*() const
		{
			checkSlow(CurrentLink);
			return CurrentLink->Element;
		}

	private:
		FElementLink* CurrentLink;
	};

	/** Used to iterate over the elements of a THashSet. */
	class TIterator
	{
	public:
		TIterator(THashSet& InSet):
			Set(InSet),
			CurrentLink(InSet.FirstElementLink),
			NextLink(NULL)
		{
			if(CurrentLink)
			{
				NextLink = CurrentLink->Next;
			}
		}

		/** Prefetches memory for the next link. */
		void PrefetchNext() const
		{
			PREFETCH( NextLink );
		}

		/** Advances the iterator to the next element. */
		void Next()
		{
			CurrentLink = NextLink;
			if(CurrentLink)
			{
				NextLink = CurrentLink->Next;
			}
		}

		/** Removes the current element and advances to the next element. */
		void RemoveCurrent()
		{
			Set.Remove(&CurrentLink->Element);
			CurrentLink = NULL;
		}

		/**
		 * Checks for the end of the list.
		 */
		operator UBOOL() const { return CurrentLink != NULL; }

		// Accessors.
		ElementType* operator->() const
		{
			checkSlow(CurrentLink);
			return &CurrentLink->Element;
		}
		ElementType& operator*() const
		{
			checkSlow(CurrentLink);
			return CurrentLink->Element;
		}

	private:
		THashSet& Set;
		FElementLink* CurrentLink;
		FElementLink* NextLink;
	};

};


/**
 * An efficient pointer set container for when elements are added before being queried and no remove functionality is needed.
 */
template<typename ElementPointerType>
class TAddOnlyPointerHashSet
{
public:

	/**
	 * Minimal initialization constructor.
	 */
	TAddOnlyPointerHashSet():
		ElementArray(NULL),
		NumElements(0),
		MaxNumElements(0),
		IsHashValid(TRUE)
	{
		HashSize = 1;
		Hash = &DummyLink;
		DummyLink = NULL;
	}

	/**
	 * Destructor.
	 */
	~TAddOnlyPointerHashSet()
	{
		delete[] ElementArray;

		if (Hash != &DummyLink)
		{
			delete[] Hash;
		}
	}

	/**
	 * Removes all elements from the set without de-allocating memory
	 */
	void LightweightEmpty()
	{
		NumElements = 0;
		IsHashValid = FALSE;
		appMemzero(Hash, HashSize * sizeof(FElementLink*));
	}

	/**
	 * Returns the number of elements.
	 */
	INT Num() const
	{
		return NumElements;
	}

	/**
	 * Adds an element to the set.
	 */
	void Add(ElementPointerType *ElementPointer)
	{
		check(!IsHashValid);
		// Create a new element link for the element.
		if (NumElements == MaxNumElements)
		{
			MaxNumElements = Max<INT>((MaxNumElements + MaxNumElements / 2), 128);
			FElementLink* NewElementArray = new FElementLink[MaxNumElements];

			appMemcpy(NewElementArray, ElementArray, NumElements * sizeof(FElementLink));

			delete[] ElementArray;
			ElementArray = NewElementArray;
		}

		ElementArray[NumElements].ElementPointer = ElementPointer;
		++ NumElements;
	}

	/**
	 * Queries for the presence of a given key in the set.
	 * @param Key - The key to search for.
	 * @return TRUE If the key was found, FALSE if not
	 */
	UBOOL Find(ElementPointerType* Key) const
	{
		check(IsHashValid);
		INT iHash = PointerHash(Key) & (HashSize - 1);
		for(FElementLink* ElementLink = Hash[iHash];ElementLink;ElementLink = ElementLink->HashNext)
		{
			if (Key == ElementLink->ElementPointer)
				return TRUE;
		}
		return FALSE;
	}

	/*
	 * Computes the hash for the set once all elements have been added.
	 * The hash is not valid until this function has been called
	 */
	void ComputeHash()
	{
		check(!IsHashValid);
		INT DesiredHashSize = 1 << appCeilLogTwo(NumElements / 2 + 8);
		if (DesiredHashSize > HashSize)
		{
			if (Hash != &DummyLink)
			{
				delete[] Hash;
			}
			
			HashSize = DesiredHashSize;
			Hash = new FElementLink*[HashSize];
		}
		appMemzero(Hash, HashSize * sizeof(FElementLink*));

		for (INT iElement = 0; iElement < NumElements; ++ iElement)
		{
			INT iHash = PointerHash(ElementArray[iElement].ElementPointer) & (HashSize - 1);
			ElementArray[iElement].HashNext = Hash[iHash];
			Hash[iHash] = &ElementArray[iElement];
		}
		IsHashValid = TRUE;
	}

private:
	class FElementLink
	{
	public:
		ElementPointerType* ElementPointer;
		FElementLink* HashNext;

		void HashLink(FElementLink*& HashHead)
		{
			HashNext = HashHead;
			HashHead = this;
		}
	};

	FElementLink* ElementArray;
	INT NumElements;
	INT MaxNumElements;

	FElementLink** Hash;
	INT HashSize;
	UBOOL IsHashValid;
	FElementLink* DummyLink;
};
