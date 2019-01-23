/*=============================================================================
	UnTemplate.h: Unreal templates.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/



/*-----------------------------------------------------------------------------
	Type information.
-----------------------------------------------------------------------------*/

/**
 * Base type information for atomic types which pass by value.
 */
template<typename T>
class TTypeInfoAtomicBase
{
public:
	typedef T ConstInitType;
	enum { NeedsConstructor = 0	};
	enum { NeedsDestructor = 0	};
};

/**
 * Base type information for constructed types which pass by reference.
 */
template<typename T>
class TTypeInfoConstructedBase
{
public:
	typedef const T& ConstInitType;
	enum { NeedsConstructor = 1	};
	enum { NeedsDestructor = 1	};
};

/**
 * The default behaviour is for types to behave as constructed types.
 */
template<typename T>
class TTypeInfo : public TTypeInfoConstructedBase<T>{};

/**
 * C-style pointers require no construction.
 */
template<typename T>
class TTypeInfo<T*>: public TTypeInfoAtomicBase<T*> {};

template <> class TTypeInfo<BYTE>		: public TTypeInfoAtomicBase<BYTE>	{};
template <> class TTypeInfo<SBYTE>		: public TTypeInfoAtomicBase<SBYTE> {};
template <> class TTypeInfo<ANSICHAR>	: public TTypeInfoAtomicBase<ANSICHAR> {};
#if !XBOX // Xbox 360 complains about UNICHAR specialization already having been defined.
template <> class TTypeInfo<UNICHAR>	: public TTypeInfoAtomicBase<UNICHAR> {};
#endif
template <> class TTypeInfo<INT>		: public TTypeInfoAtomicBase<INT> {};
template <> class TTypeInfo<DWORD>		: public TTypeInfoAtomicBase<DWORD> {};
template <> class TTypeInfo<WORD>		: public TTypeInfoAtomicBase<WORD> {};
template <> class TTypeInfo<SWORD>		: public TTypeInfoAtomicBase<SWORD> {};
template <> class TTypeInfo<QWORD>		: public TTypeInfoAtomicBase<QWORD> {};
template <> class TTypeInfo<SQWORD>		: public TTypeInfoAtomicBase<SQWORD> {};
template <> class TTypeInfo<FLOAT>		: public TTypeInfoAtomicBase<FLOAT> {};
template <> class TTypeInfo<DOUBLE>		: public TTypeInfoAtomicBase<DOUBLE> {};

/*-----------------------------------------------------------------------------
	Standard templates.
-----------------------------------------------------------------------------*/

template< class T > inline T Abs( const T A )
{
	return (A>=(T)0) ? A : -A;
}
template< class T > inline T Sgn( const T A )
{
	return (A>0) ? 1 : ((A<0) ? -1 : 0);
}
template< class T > inline T Max( const T A, const T B )
{
	return (A>=B) ? A : B;
}
template< class T > inline T Min( const T A, const T B )
{
	return (A<=B) ? A : B;
}
template< class T > inline T Max3( const T A, const T B, const T C )
{
	return Max ( Max( A, B ), C );
}
template< class T > inline T Min3( const T A, const T B, const T C )
{
	return Min ( Min( A, B ), C );
}
template< class T > inline T Square( const T A )
{
	return A*A;
}
template< class T > inline T Clamp( const T X, const T Min, const T Max )
{
	return X<Min ? Min : X<Max ? X : Max;
}
template< class T > inline T Align( const T Ptr, INT Alignment )
{
	return (T)(((PTRINT)Ptr + Alignment - 1) & ~(Alignment-1));
}
template< class T > inline void Exchange( T& A, T& B )
{
	const T Temp = A;
	A = B;
	B = Temp;
}

template< class T, class U > T Lerp( const T& A, const T& B, const U& Alpha )
{
	return (T)(A + Alpha * (B-A));
}

template<class T> T BiLerp(const T& P00,const T& P10,const T& P01,const T& P11,FLOAT FracX,FLOAT FracY)
{
	return Lerp(
			Lerp(P00,P10,FracX),
			Lerp(P01,P11,FracX),
			FracY
			);
}

// P - end points
// T - tangent directions at end points
// Alpha - distance along spline
template< class T, class U > T CubicInterp( const T& P0, const T& T0, const T& P1, const T& T1, const U& A )
{
	const FLOAT A2 = A  * A;
	const FLOAT A3 = A2 * A;

	return (T)(((2*A3)-(3*A2)+1) * P0) + ((A3-(2*A2)+A) * T0) + ((A3-A2) * T1) + (((-2*A3)+(3*A2)) * P1);
}

template< class T, class U > T CubicInterpDerivative( const T& P0, const T& T0, const T& P1, const T& T1, const U& A )
{
	T a = 6.f*P0 + 3.f*T0 + 3.f*T1 - 6.f*P1;
	T b = -6.f*P0 - 4.f*T0 - 2.f*T1 + 6.f*P1;
	T c = T0;

	const FLOAT A2 = A  * A;

	return (a * A2) + (b * A) + c;
}

template< class T, class U > T CubicInterpSecondDerivative( const T& P0, const T& T0, const T& P1, const T& T1, const U& A )
{
	T a = 12.f*P0 + 6.f*T0 + 6.f*T1 - 12.f*P1;
	T b = -6.f*P0 - 4.f*T0 - 2.f*T1 + 6.f*P1;

	return (a * A) + b;
}

inline DWORD GetTypeHash( const BYTE A )
{
	return A;
}
inline DWORD GetTypeHash( const SBYTE A )
{
	return A;
}
inline DWORD GetTypeHash( const WORD A )
{
	return A;
}
inline DWORD GetTypeHash( const SWORD A )
{
	return A;
}
inline DWORD GetTypeHash( const INT A )
{
	return A;
}
inline DWORD GetTypeHash( const DWORD A )
{
	return A;
}
inline DWORD GetTypeHash( const QWORD A )
{
	return (DWORD)A+((DWORD)(A>>32) * 23);
}
inline DWORD GetTypeHash( const SQWORD A )
{
	return (DWORD)A+((DWORD)(A>>32) * 23);
}
inline DWORD GetTypeHash( const TCHAR* S )
{
	return appStrihash(S);
}

inline DWORD GetTypeHash( const void* A )
{
	return (DWORD)(PTRINT)A;
}

inline DWORD GetTypeHash( void* A )
{
	return (DWORD)(PTRINT)A;
}



// GCC has issues with declaring this GetTypeHash after THashSet (or so it appears to be the problem).
// declare it early to allow for compiling
#ifdef __GNUC__
extern DWORD GetTypeHash(const struct FBoundShaderStateRHIRef &Key);
#endif

#define ExchangeB(A,B) {UBOOL T=A; A=B; B=T;}

/**
 * A hashing function that works well for pointers.
 */
template<class T>
inline DWORD PointerHash(const T* Key,DWORD C = 0)
{
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

	DWORD A;
	DWORD B;
	A = B = 0x9e3779b9;
	A += (*(DWORD*)&Key);
	mix(A,B,C);
	return C;

#undef mix
}

/*----------------------------------------------------------------------------
	FLOAT specialization of templates.
----------------------------------------------------------------------------*/

#if PS3
template<> FORCEINLINE FLOAT Abs( const FLOAT A )
{
	return __fabsf( A );
}
#else
template<> FORCEINLINE FLOAT Abs( const FLOAT A )
{
	return fabsf( A );
}
#endif

#if XBOX
template<> FORCEINLINE FLOAT Max( const FLOAT A, const FLOAT B )
{
	return __fsel( A - B, A, B );
}
template<> FORCEINLINE FLOAT Min( const FLOAT A, const FLOAT B )
{
	return __fsel( A - B, B, A );
}
//@todo optimization: the below causes crashes in release mode when compiled with VS.NET 2003
#elif __HAS_SSE__ && 0
template<> FORCEINLINE FLOAT Max( const FLOAT A, const FLOAT B )
{
	return _mm_max_ss( _mm_set_ss(A), _mm_set_ss(B) ).m128_f32[0];
}
template<> FORCEINLINE FLOAT Min( const FLOAT A, const FLOAT B )
{
	return _mm_max_ss( _mm_set_ss(A), _mm_set_ss(B) ).m128_f32[0];
}
#elif PS3
template<> FORCEINLINE FLOAT Max( const FLOAT A, const FLOAT B )
{
	return __fsels( A - B, A, B );
}
template<> FORCEINLINE FLOAT Min( const FLOAT A, const FLOAT B )
{
	return __fsels( A - B, B, A );
}
#endif

/*----------------------------------------------------------------------------
	DWORD specialization of templates.
----------------------------------------------------------------------------*/

/** Returns the smaller of the two values */
template<> FORCEINLINE DWORD Min(const DWORD A, const DWORD B)
{
    // Negative if B is less than A (i.e. the high bit will be set)
	DWORD Delta  = B - A;
	// Relies on sign bit rotating in
    DWORD Mask   = static_cast<INT>(Delta) >> 31;
    DWORD Result = A + (Delta & Mask);

    return Result;
}

/*----------------------------------------------------------------------------
	Standard macros.
----------------------------------------------------------------------------*/

// Number of elements in an array.
#define ARRAY_COUNT( array ) \
	( sizeof(array) / sizeof((array)[0]) )

// Offset of a struct member.

#ifdef __GNUC__
/**
 * gcc3 thinks &((myclass*)NULL)->member is an invalid use of the offsetof
 * macro. This is a broken heuristic in the compiler and the workaround is
 * to use a non-zero offset.
 */
#define STRUCT_OFFSET( struc, member )	( ( (PTRINT)&((struc*)0x1)->member ) - 0x1 )
#else
#define STRUCT_OFFSET( struc, member )	( (PTRINT)&((struc*)NULL)->member )
#endif

/*-----------------------------------------------------------------------------
	Allocators.
-----------------------------------------------------------------------------*/

template <class T> class TAllocator
{};

/*-----------------------------------------------------------------------------
	Dynamic array template.
-----------------------------------------------------------------------------*/

/**
 * Generic non-const iterator which can operate on types that expose the following:
 * - A type called ElementType representing the contained type.
 * - A method IndexType Num() const that returns the number of items in the container.
 * - A method T& operator(IndexType index) which returns a reference to a contained object by index.
 */
template< class ContainerType, typename IndexType = INT >
class TIndexedContainerIterator
{
public:
	TIndexedContainerIterator(ContainerType& InContainer)
		:	Container( InContainer )
	{
		Reset();
	}

	/** Advances iterator to the next element in the container. */
	void operator++()
	{
		++Index;
	}

	/** @name Element access */
	//@{
	typename ContainerType::ElementType& operator* () const
	{
		return Container( Index );
	}

	typename ContainerType::ElementType& operator-> () const
	{
		return Container( Index );
	}
	//@}

	/** Returns TRUE if the iterator has reached the last element. */
	operator UBOOL() const
	{
		return Container.IsValidIndex(Index);
	}

	/** Returns an index to the current element. */
	IndexType GetIndex() const
	{
		return Index;
	}

	/** Resets the iterator to the first element. */
	void Reset()
	{
		Index = 0;
	}

private:
	ContainerType&	Container;
	IndexType		Index;
};

/**
* Generic const iterator which can operate on types that expose the following:
* - A type called ElementType representing the contained type.
* - A method IndexType Num() const that returns the number of items in the container.
* - A method T& operator(IndexType index) const which returns a reference to a contained object by index.
*/
template< class ContainerType, typename IndexType = INT >
class TIndexedContainerConstIterator
{
public:
	TIndexedContainerConstIterator(const ContainerType& InContainer)
		:	Container( InContainer )
	{
		Reset();
	}

	/** Advances iterator to the next element in the container. */
	void operator++()
	{
		++Index;
	}

	/** @name Element access */
	//@{
	const typename ContainerType::ElementType& operator* () const
	{
		return Container( Index );
	}

	const typename ContainerType::ElementType& operator-> () const
	{
		return Container( Index );
	}
	//@}

	/** Returns TRUE if the iterator has reached the last element. */
	operator UBOOL() const
	{
		return Container.IsValidIndex(Index);
	}

	/** Returns an index to the current element. */
	IndexType GetIndex() const
	{
		return Index;
	}

	/** Resets the iterator to the first element. */
	void Reset()
	{
		Index = 0;
	}

private:
	const ContainerType&	Container;
	IndexType				Index;
};

//
// Base dynamic array.
//
class FArray
{
public:
	void* GetData()
	{
		return Data;
	}
	const void* GetData() const
	{
		return Data;
	}
	UBOOL IsValidIndex( INT i ) const
	{
		return i>=0 && i<ArrayNum;
	}
	FORCEINLINE INT Num() const
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		return ArrayNum;
	}
	void InsertZeroed( INT Index, INT Count, INT ElementSize, DWORD Alignment )
	{
		Insert( Index, Count, ElementSize, Alignment );
		appMemzero( (BYTE*)Data+Index*ElementSize, Count*ElementSize );
	}
	void Insert( INT Index, INT Count, INT ElementSize, DWORD Alignment )
	{
		check(Count>=0);
		check(ArrayNum>=0);
		check(ArrayMax>=ArrayNum);
		check(Index>=0);
		check(Index<=ArrayNum);

		INT OldNum = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = CalculateSlack( ElementSize );
			Realloc( ElementSize, Alignment );
		}
		appMemmove
		(
			(BYTE*)Data + (Index+Count )*ElementSize,
			(BYTE*)Data + (Index       )*ElementSize,
			              (OldNum-Index)*ElementSize
		);
	}
	INT Add( INT Count, INT ElementSize, DWORD Alignment )
	{
		check(Count>=0);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);

		const INT Index = ArrayNum;
		if( (ArrayNum+=Count)>ArrayMax )
		{
			ArrayMax = CalculateSlack( ElementSize );
			Realloc( ElementSize, Alignment );
		}

		return Index;
	}
	INT AddZeroed( INT Count, INT ElementSize, DWORD Alignment )
	{
		INT Index = Add( Count, ElementSize, Alignment );
		appMemzero( (BYTE*)Data+Index*ElementSize, Count*ElementSize );
		return Index;
	}
	void Shrink( INT ElementSize, DWORD Alignment )
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		if( ArrayMax != ArrayNum )
		{
			ArrayMax = ArrayNum;
			Realloc( ElementSize, Alignment );
		}
	}
	void Empty( INT ElementSize, DWORD Alignment, INT Slack=0 )
	{
		checkSlow(Slack>=0);
		ArrayNum = 0;
		// only reallocate if we need to, I don't trust realloc to the same size to work
		if (ArrayMax != Slack)
		{
			ArrayMax = Slack;
			Realloc( ElementSize, Alignment );
		}
	}
	void Swap(INT A, INT B, INT ElementSize)
	{
		appMemswap((BYTE*)Data+(ElementSize*A),(BYTE*)Data+(ElementSize*B),ElementSize);
	}
	FArray()
	:	Data	( NULL )
	,   ArrayNum( 0 )
	,	ArrayMax( 0 )

	{}
	FArray( ENoInit )
	{}
	~FArray()
	{
		if( Data )
		{
			appFree( Data );
		}
		Data = NULL;
		ArrayNum = ArrayMax = 0;
	}
	void CountBytes( FArchive& Ar, INT ElementSize )
	{
		Ar.CountBytes( ArrayNum*ElementSize, ArrayMax*ElementSize );
	}
	/**
	 * Returns the amount of slack in this array in elements.
	 */
	INT GetSlack() const
	{
		return ArrayMax - ArrayNum;
	}
	void Remove( INT Index, INT Count, INT ElementSize, DWORD Alignment );
protected:
	void Realloc( INT ElementSize, DWORD Alignment )
	{
		// Avoid calling appRealloc( NULL, 0 ) as ANSI C mandates returning a valid pointer which is not what we want.
		if( Data || ArrayMax )
		{
			Data = appRealloc( Data, ArrayMax*ElementSize, Alignment );
		}
	}
	INT CalculateSlack( INT ElementSize ) const;

	FArray( INT InNum, INT ElementSize, DWORD Alignment )
	:   Data    ( NULL  )
	,   ArrayNum( InNum )
	,	ArrayMax( InNum )

	{
		Realloc( ElementSize, Alignment );
	}
	void* Data;
	INT	  ArrayNum;
	INT	  ArrayMax;
};

//
// Templated dynamic array, with optional alignment (defaults to the DEFAULT_ALIGNMENT #define)
//
template< class T, DWORD Alignment > class TArray : public FArray
{
protected:
	/**
	 * Copies data from one array into this array. Uses the fast path if the
	 * data in question does not need a constructor.
	 *
	 * @param Source the source array to copy
	 */
	FORCEINLINE void Copy(const TArray<T,Alignment>& Source)
	{
		if (this != &Source)
		{
			// Just empty our array if there is nothing to copy
			if (Source.ArrayNum > 0)
			{
				// Presize the array so there are no extra allocs/memcpys
				Empty(Source.ArrayNum);
				// Determine whether we need per element construction or bulk
				// copy is fine
				if (TTypeInfo<T>::NeedsConstructor)
				{
					// Use the inplace new to copy the element to an array element
					for (INT Index = 0; Index < Source.ArrayNum; Index++)
					{
						::new(*this) T(Source(Index));
					}
				}
				else
				{
					// Use the much faster path for types that allow it
					appMemcpy(Data,&Source(0),sizeof(T) * Source.ArrayNum);
					ArrayNum = Source.ArrayNum;
				}
			}
			else
			{
				Empty();
			}
		}
	}

public:
	typedef T ElementType;
	TArray()
	:	FArray()
	{}
	explicit TArray( INT InNum )
	:	FArray( InNum, sizeof(T), Alignment )
	{}

	/**
	 * Copy constructor. Use the common routine to perform the copy
	 *
	 * @param Other the source array to copy
	 */
	TArray(const TArray& Other) : FArray()
	{
		Copy(Other);
	}

	TArray( ENoInit )
	: FArray( E_NoInit )
	{}
	~TArray()
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		Remove( 0, ArrayNum );
#if (defined _MSC_VER) && (defined _DEBUG)
		//@todo it were nice if we had a cleaner solution for DebugGet
		volatile const T* Dummy = DebugGet(-1);
	}
	/**
	 * Helper function that can be used inside the debuggers watch window to debug TArrays. E.g. "*Class->Defaults.DebugGet(5)". 
	 *
	 * @param	i	Index
	 * @return		pointer to type T at Index i
	 */
	const T* DebugGet( INT i ) const
	{
		if( i >= 0 )
			return &((T*)Data)[i];
		else
			return NULL;
	}
#else
	}
#endif
	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @return pointer to first array entry or NULL if ArrayMax==0
	 */
	T* GetTypedData()
	{
		return (T*)Data;
	}
	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @return pointer to first array entry or NULL if ArrayMax==0
	 */
	const T* GetTypedData() const
	{
		return (T*)Data;
	}
	/** 
	 * Helper function returning the size of the inner type
	 *
	 * @return size in bytes of array type
	 */
	DWORD GetTypeSize() const
	{
		return sizeof(T);
	}

	/** 
	 * Helper function to return the amount of memory allocated by this container 
	 *
	 * @return number of bytes allocated by this container
	 */
	DWORD GetAllocatedSize( void ) const
	{
		return( ArrayMax * sizeof( T ) );
	}

	T& operator()( INT i )
	{
		checkSlow(i>=0);
		checkSlow(i<ArrayNum||(i==0 && ArrayNum==0)); // (i==0 && ArrayNum==0) is workaround for &MyArray(0) abuse
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[i];
	}
	const T& operator()( INT i ) const
	{
		checkSlow(i>=0);
		checkSlow(i<ArrayNum||(i==0 && ArrayNum==0)); // (i==0 && ArrayNum==0) is workaround for &MyArray(0) abuse
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[i];
	}
	T Pop()
	{
		check(ArrayNum>0);
		checkSlow(ArrayMax>=ArrayNum);
		T Result = ((T*)Data)[ArrayNum-1];
		Remove( ArrayNum-1 );
		return Result;
	}
	void Push( const T& Item )
	{
		AddItem(Item);
	}
	T& Top()
	{
		return Last();
	}
	const T& Top() const
	{
		return Last();
	}
	T& Last( INT c=0 )
	{
		check(Data);
		check(c<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[ArrayNum-c-1];
	}
	const T& Last( INT c=0 ) const
	{
		check(Data);
		checkSlow(c<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((T*)Data)[ArrayNum-c-1];
	}
	void Shrink()
	{
		FArray::Shrink( sizeof(T), Alignment );
	}
	UBOOL FindItem( const T& Item, INT& Index ) const
	{
		for( Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return 1;
		return 0;
	}
	INT FindItemIndex( const T& Item ) const
	{
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return INDEX_NONE;
	}
	UBOOL ContainsItem( const T& Item ) const
	{
		return ( FindItemIndex(Item) != INDEX_NONE );
	}
	UBOOL operator==(const TArray<T,Alignment>& OtherArray) const
	{
		if(Num() != OtherArray.Num())
			return 0;
		for(INT Index = 0;Index < Num();Index++)
		{
			if(!((*this)(Index) == OtherArray(Index)))
				return 0;
		}
		return 1;
	}
	UBOOL operator!=(const TArray<T,Alignment>& OtherArray) const
	{
		if(Num() != OtherArray.Num())
			return 1;
		for(INT Index = 0;Index < Num();Index++)
		{
			if(!((*this)(Index) == OtherArray(Index)))
				return 1;
		}
		return 0;
	}
	friend FArchive& operator<<( FArchive& Ar, TArray& A )
	{
		A.CountBytes( Ar );
		if( sizeof(T)==1 )
		{
			// Serialize simple bytes which require no construction or destruction.
			Ar << A.ArrayNum;
			check( A.ArrayNum >= 0 );
			if( Ar.IsLoading() )
			{
				A.ArrayMax = A.ArrayNum;
				A.Realloc( sizeof(T), Alignment );
			}
			Ar.Serialize( A.GetData(), A.Num() );
		}
		else if( Ar.IsLoading() )
		{
			// Load array.
			INT NewNum;
			Ar << NewNum;
			A.Empty( NewNum );
			for( INT i=0; i<NewNum; i++ )
			{
				Ar << *::new(A)T;
			}
		}
		else
		{
			// Save array.
			Ar << A.ArrayNum;
			for( INT i=0; i<A.ArrayNum; i++ )
			{
				Ar << A( i );
			}
		}
		return Ar;
	}
	/**
	 * Bulk serialize array as a single memory blob when loading. Uses regular serialization code for saving
	 * and doesn't serialize at all otherwise (e.g. transient, garbage collection, ...).
	 * 
	 * Requirements:
	 *   - T's << operator needs to serialize ALL member variables in the SAME order they are layed out in memory.
	 *   - T's << operator can NOT perform any fixup operations. This limitation can be lifted by manually copying
	 *     the code after the BulkSerialize call.
	 *   - T can NOT contain any member variables requiring constructor calls or pointers
	 *   - sizeof(T) must be equal to the sum of sizes of it's member variables.
	 *        - e.g. use pragma pack (push,1)/ (pop) to ensure alignment
	 *        - match up BYTE/ WORDs so everything always end up being properly aligned
	 *   - Code can not rely on serialization of T if neither ArIsLoading nor ArIsSaving is TRUE.
	 *   - Can only be called platforms that either have the same endianness as the one the content was saved with
	 *     or had the endian conversion occur in a cooking process like e.g. for consoles.
	 *
	 * Notes:
	 *   - it is safe to call BulkSerialize on TTransArrays
	 *
	 * IMPORTANT:
	 *   - This is Overridden in XeD3dResourceArray.h  Please make certain changes are propogated accordingly
	 *
	 * @param Ar	FArchive to bulk serialize this TArray to/from
	 */
	void BulkSerialize(FArchive& Ar)
	{
		// Serialize element size to detect mismatch across platforms.
		INT SerializedElementSize = 0;
		if( Ar.Ver() >= VER_ADDED_BULKSERIALIZE_SANITY_CHECKING )
		{
			SerializedElementSize = sizeof(T);
			Ar << SerializedElementSize;
		}
#if !__INTEL_BYTE_ORDER__ && !CONSOLE
		// We need to handle endian conversion of content as all non cooked content will always be in 
		// Intel endian format. Consoles have their data converted to the appropriate endianness on
		// cooking which is why we don't have to hit this codepath for the console case.
		Ar << *this;
#else
		if( Ar.IsSaving() || Ar.Ver() < GPackageFileVersion || Ar.LicenseeVer() < GPackageFileLicenseeVersion )
		{
			// Use regular endian clean saving to correctly handle content cooking.
			// @todo: in theory we only need to do this if ArForceByteSwapping though for now it also 
			// @todo: serves as a good sanity check to ensure that bulk serialization matches up with 
			// @todo: regular serialization.
			Ar << *this;
		}
		else 
		{
			CountBytes(Ar);
			if( Ar.IsLoading() )
			{
				// Basic sanity checking to ensure that sizes match.
				checkf(SerializedElementSize==0 || SerializedElementSize==sizeof(T),TEXT("Expected %i, Got: %i"),sizeof(T),SerializedElementSize);
				// Serialize the number of elements, block allocate the right amount of memory and deserialize
				// the data as a giant memory blob in a single call to Serialize. Please see the function header
				// for detailed documentation on limitations and implications.
				INT NewArrayNum;
				Ar << NewArrayNum;
				Empty( NewArrayNum );
				Add( NewArrayNum );
				Ar.Serialize( GetData(), NewArrayNum * sizeof(T) );
			}
		}
#endif
	}
	void CountBytes( FArchive& Ar )
	{
		FArray::CountBytes( Ar, sizeof(T) );
	}

	// Add, Insert, Remove, Empty interface.
	INT Add( INT Count=1 )
	{
		return FArray::Add( Count, sizeof(T), Alignment );
	}
	void Insert( INT Index, INT Count=1 )
	{
		FArray::Insert( Index, Count, sizeof(T), Alignment );
	}
	void InsertZeroed( INT Index, INT Count=1 )
	{
		FArray::InsertZeroed( Index, Count, sizeof(T), Alignment );
	}
	INT InsertItem( const T& Item, INT Index )
	{
		// construct a copy in place at Index (this new operator will insert at 
		// Index, then construct that memory with Item)
		new(*this, Index)T(Item);
		return Index;
	}
	void Remove( INT Index, INT Count=1 )
	{
		check(Index>=0);
		check(Index<=ArrayNum);
		check(Index+Count<=ArrayNum);
		if( TTypeInfo<T>::NeedsDestructor )
			for( INT i=Index; i<Index+Count; i++ )
				(&(*this)(i))->~T();
		FArray::Remove( Index, Count, sizeof(T), Alignment );
	}
	void Empty( INT Slack=0 )
	{
		if( TTypeInfo<T>::NeedsDestructor )
			for( INT i=0; i<ArrayNum; i++ )
				(&(*this)(i))->~T();
		FArray::Empty( sizeof(T), Alignment, Slack );
	}

	/**
	 * Appends the specified array to this array.
	 * Cannot append to self.
	 */
	FORCEINLINE void Append(const TArray<T,Alignment>& Source)
	{
		// Do nothing if the source and target match, or the source is empty.
		if ( this != &Source && Source.Num() > 0 )
		{
			// Allocate memory for the new elements.
			Reserve( ArrayNum + Source.ArrayNum );

			if ( TTypeInfo<T>::NeedsConstructor )
			{
				// Construct each element.
				for ( INT Index = 0 ; Index < Source.ArrayNum ; ++Index )
				{
					// @todo DB: optimize by calling placement new directly, to avoid
					// @todo DB: the superfluous checks in TArray placement new.
					::new(*this) T(Source(Index));
				}
			}
			else
			{
				// Do a bulk copy.
				appMemcpy( (BYTE*)Data + ArrayNum * sizeof(T), Source.Data, sizeof(T) * Source.ArrayNum );
				ArrayNum += Source.ArrayNum;
			}
		}
	}

	/**
	 * Appends the specified array to this array.
	 * Cannot append to self.
	 */
	TArray& operator+=( const TArray& Other )
	{
		Append( Other );
		return *this;
	}

	/**
	 * Copies the source array into this one. Uses the common copy method
	 *
	 * @param Other the source array to copy
	 */
	TArray& operator=( const TArray& Other )
	{
		Copy(Other);
		return *this;
	}

	INT AddItem( const T& Item )
	{
		new(*this) T(Item);
		return Num() - 1;
	}
	INT AddZeroed( INT Count=1 )
	{
		return FArray::AddZeroed( Count, sizeof(T), Alignment );
	}
	INT AddUniqueItem( const T& Item )
	{
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				return Index;
		return AddItem( Item );
	}

	/**
	 * Reserves memory such that the array can contain at least Number elements.
	 */
	void Reserve(INT Number)
	{
		if (Number > ArrayMax)
		{
			ArrayMax = Number;
			FArray::Realloc( sizeof(T), Alignment );
		}
	}

	INT RemoveItem( const T& Item )
	{
		check( ((&Item) < (T*)Data) || ((&Item) >= (T*)Data+ArrayMax) );
		INT OriginalNum=ArrayNum;
		for( INT Index=0; Index<ArrayNum; Index++ )
			if( (*this)(Index)==Item )
				Remove( Index-- );
		return OriginalNum - ArrayNum;
	}
	void SwapItems(INT A, INT B)
	{
		check((A >= 0) && (B >= 0));
		check((ArrayNum > A) && (ArrayNum > B));
		if (A != B)
		{
			FArray::Swap(A,B,sizeof(T));
		}
	}

	/**
	 * Same as empty, but doesn't change memory allocations. It calls the
	 * destructors on held items if needed and then zeros the ArrayNum.
	 */
	void Reset(void)
	{
		if (TTypeInfo<T>::NeedsDestructor)
		{
			for (INT i = 0; i < ArrayNum; i++)
			{
				(&(*this)(i))->~T();
			}
		}
		ArrayNum = 0;
	}

	/**
	 * Searches for the first entry of the specified type, will only work
	 * with TArray<UObject*>.  Optionally return the item's index, and can
	 * specify the start index.
	 */
	template<class SearchType> UBOOL FindItemByClass(SearchType **Item = NULL, INT *ItemIndex = NULL, INT StartIndex = 0)
	{
		UClass *SearchClass = SearchType::StaticClass();
		for (INT Idx = StartIndex; Idx < ArrayNum; Idx++)
		{
			if ((*this)(Idx)->IsA(SearchClass))
			{
				if (Item != NULL)
				{
					*Item = (SearchType*)((*this)(Idx));
				}
				if (ItemIndex != NULL)
				{
					*ItemIndex = Idx;
				}
				return TRUE;
			}
		}
		return FALSE;
	}

	// Iterator.
	class TIterator
	{
	public:
		TIterator( TArray<T,Alignment>& InArray ) : Array(InArray), Index(-1) { ++*this;      }
		void operator++()      { ++Index;                                           }
		void RemoveCurrent()   { Array.Remove(Index--); }
		INT GetIndex()   const { return Index;                                      }
		operator UBOOL() const { return Array.IsValidIndex(Index);                  }
		T& operator*()   const { return Array(Index);                               }
		T* operator->()  const { return &Array(Index);                              }
		T& GetCurrent()  const { return Array( Index );                             }
		T& GetPrev()     const { return Array( Index ? Index-1 : Array.Num()-1 );   }
		T& GetNext()     const { return Array( Index<Array.Num()-1 ? Index+1 : 0 ); }
	private:
		TArray<T,Alignment>& Array;
		INT Index;
	};
};

/**
 * Same as TArray except that byte swapping for its elements can be
 * forced on/off.
 */
template<class T,UBOOL DisableByteSwap> 
class TArrayForceByteSwap : public TArray<T>
{
public:	
	TArrayForceByteSwap( INT InNum )
	:	TArray<T>( InNum )
	{}

	friend FArchive& operator<<( FArchive& Ar, TArrayForceByteSwap& A )
	{
		A.CountBytes( Ar );
		if( sizeof(T)==1 )
		{
			// Serialize simple bytes which require no construction or destruction.
			Ar << A.ArrayNum;
			if( Ar.IsLoading() )
			{
				A.ArrayMax = A.ArrayNum;
				A.Realloc( sizeof(T) );
			}
			// toggle byte swapping of array elements
			DWORD SavedForceSwap = Ar.ForceByteSwapping();
			Ar.SetByteSwapping( SavedForceSwap && !DisableByteSwap );

			// serialize the byte array
			Ar.Serialize( A.GetData(), A.Num() );

			// restore archives byte swap setting
			Ar.SetByteSwapping( SavedForceSwap );
		}
		else if( Ar.IsLoading() )
		{
			// Load array.
			INT NewNum;
			Ar << NewNum;
			A.Empty( NewNum );
			for( INT i=0; i<NewNum; i++ )
			{
				Ar << *::new(A)T;
			}
		}
		else
		{
			// Save array.
			Ar << A.ArrayNum;

			// toggle byte swapping of array elements
			DWORD SavedForceSwap = Ar.ForceByteSwapping();
			Ar.SetByteSwapping( SavedForceSwap && !DisableByteSwap );

			// serialize array elements
			for( INT i=0; i<A.ArrayNum; i++ )
			{
				Ar << A( i );
			}

			// restore archives byte swap setting
			Ar.SetByteSwapping( SavedForceSwap );
		}
		return Ar;
	}
};

template<class T> class TArrayNoInit : public TArray<T>
{
public:
	TArrayNoInit()
	: TArray<T>(E_NoInit)
	{}
	TArrayNoInit& operator=( const TArrayNoInit& Other )
	{
		TArray<T>::operator=(Other);
		return *this;
	}
	TArrayNoInit& operator=( const TArray<T>& Other )
	{
		TArray<T>::operator=(Other);
		return *this;
	}
};

//
// Array operator news.
//
template <class T, DWORD Alignment> void* operator new( size_t Size, TArray<T, Alignment>& Array )
{
	check(Size == sizeof(T));
	const INT Index = Array.FArray::Add(1,sizeof(T), Alignment);
	return &Array(Index);
}
template <class T, DWORD Alignment> void* operator new( size_t Size, TArray<T, Alignment>& Array, INT Index )
{
	check(Size == sizeof(T));
	Array.FArray::Insert(Index,1,sizeof(T), Alignment);
	return &Array(Index);
}

//
// Array exchanger.
//
template <class T> inline void ExchangeArray( TArray<T>& A, TArray<T>& B )
{
	appMemswap( &A, &B, sizeof(FArray) );
}

/*-----------------------------------------------------------------------------
	MRU array.
-----------------------------------------------------------------------------*/

/**
 * Same as TArray except:
 * - Has an upper limit of the number of items it will store.
 * - Any item that is added to the array is moved to the top.
 */
template<class T> class TMRUArray : public TArray<T>
{
public:

	/** The maximum number of items we can store in this array. */
	INT MaxItems;

	TMRUArray<T>()
		:	TArray<T>()
	{
		MaxItems = 0;
	}
	TMRUArray<T>( INT InNum )
		:	TArray<T>( InNum, sizeof(T*) )
	{
		MaxItems = 0;
	}
	TMRUArray<T>( const TMRUArray& Other )
		:	TArray<T>( Other.ArrayNum, sizeof(T*) )
	{
		MaxItems = 0;
	}
	TMRUArray<T>( ENoInit )
		: TArray<T>( E_NoInit )
	{
		MaxItems = 0;
	}
	~TMRUArray()
	{
		checkSlow(this->ArrayNum>=0);
		checkSlow(this->ArrayMax>=this->ArrayNum);
		Remove( 0, this->ArrayNum );
	}

	T& operator()( INT i )
	{
		checkSlow(i>=0);
		checkSlow(i<this->ArrayNum);
		checkSlow(this->ArrayMax>=this->ArrayNum);
		return ((T*)this->Data)[i];
	}
	const T& operator()( INT i ) const
	{
		checkSlow(i>=0);
		checkSlow(i<this->ArrayNum);
		checkSlow(this->ArrayMax>=this->ArrayNum);
		return ((T*)this->Data)[i];
	}

	void Empty( INT Slack=0 )
	{
		TArray<T>::Empty( Slack );
	}
	INT AddItem( const T& Item )
	{
		INT idx = TArray<T>::AddItem( Item );
		this->SwapItems( idx, 0 );
		CullArray();
		return 0;
	}
	INT AddZeroed( INT Count=1 )
	{
		int idx = TArray<T>::AddZeroed( Count, sizeof(T) );
		this->SwapItems( idx, 0 );
		CullArray();
		return 0;
	}
	INT AddUniqueItem( const T& Item )
	{
		// Remove any existing copies of the item.
		RemoveItem(Item);

		this->Insert( 0 );
		(*this)(0) = Item;

		CullArray();

		return 0;
	}

	/**
	 * Makes sure that the array never gets beyond MaxItems in size.
	 */

	void CullArray()
	{
		// 0 = no limit

		if( !MaxItems )
		{
			return;
		}

		while( this->Num() > MaxItems )
		{
			Remove( this->Num()-1, 1 );
		}
	}
};

/*-----------------------------------------------------------------------------
	Indirect array.
	Same as a TArray above, but stores pointers to the elements, to allow
	resizing the array index without relocating the actual elements.

	NOTE: Does not currently allow for alignment, given the way it works, it
	doesn't make much sense
-----------------------------------------------------------------------------*/

template<class T> class TIndirectArray : public FArray
{
public:
	typedef T ElementType;
	TIndirectArray()
	:	FArray()
	{}
	TIndirectArray( INT InNum )
	:	FArray( InNum, sizeof(T*) )
	{}
	TIndirectArray( const TIndirectArray& Other )
	:	FArray( Other.ArrayNum, sizeof(T*), DEFAULT_ALIGNMENT )
	{
		ArrayNum=0;
		for( INT i=0; i<Other.ArrayNum; i++ )
		{
			new(*this)T(Other(i));
		}
	}
	TIndirectArray( ENoInit )
	: FArray( E_NoInit )
	{}
	~TIndirectArray()
	{
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);
		Remove( 0, ArrayNum );
	}
	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @return pointer to first array entry or NULL if ArrayMax==0
	 */
	T** GetTypedData()
	{
		return (T**)Data;
	}
	/**
	 * Helper function for returning a typed pointer to the first array entry.
	 *
	 * @return pointer to first array entry or NULL if ArrayMax==0
	 */
	const T** GetTypedData() const
	{
		return (T**)Data;
	}
	/** 
	 * Helper function returning the size of the inner type
	 *
	 * @return size in bytes of array type
	 */
	DWORD GetTypeSize() const
	{
		return sizeof(T*);
	}
	/**
	 * Copies the source array into this one.
	 *
	 * @param Other the source array to copy
	 */
	TIndirectArray& operator=( const TIndirectArray& Other )
	{
		Empty( Other.Num() );
		for( INT i=0; i<Other.Num(); i++ )
		{
			new(*this)T(Other(i));
		}	
		return *this;
	}
    T& operator()( INT i )
	{
		checkSlow(i>=0);
		checkSlow(i<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return *((T**)Data)[i];
	}
	const T& operator()( INT i ) const
	{
		checkSlow(i>=0);
		checkSlow(i<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return *((T**)Data)[i];
	}
	void Shrink()
	{
		FArray::Shrink( sizeof(T*), DEFAULT_ALIGNMENT );
	}
	UBOOL FindItem( const T*& Item, INT& Index ) const
	{
		for( Index=0; Index<ArrayNum; Index++ )
		{
			if( (*this)(Index)==*Item )
			{
				return 1;
			}
		}
		return 0;
	}
	INT FindItemIndex( const T*& Item ) const
	{
		for( INT Index=0; Index<ArrayNum; Index++ )
		{
			if( (*this)(Index)==*Item )
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
	/**
	 * Special serialize function passing the owning UObject along as required by FUnytpedBulkData
	 * serialization.
	 *
	 * @param	Ar		Archive to serialize with
	 * @param	Owner	UObject this structure is serialized within
	 */
	void Serialize( FArchive& Ar, UObject* Owner )
	{
		CountBytes( Ar );
		if( Ar.IsLoading() )
		{
			// Load array.
			INT NewNum;
			Ar << NewNum;
			Empty( NewNum );
			for( INT i=0; i<NewNum; i++ )
			{
				(*new(*this)T).Serialize( Ar, Owner );
			}
		}
		else
		{
			// Save array.
			Ar << ArrayNum;
			for( INT i=0; i<ArrayNum; i++ )
			{
				(*this)(i).Serialize( Ar, Owner );
			}
		}
	}
	friend FArchive& operator<<( FArchive& Ar, TIndirectArray& A )
	{
		A.CountBytes( Ar );
		if( Ar.IsLoading() )
		{
			// Load array.
			INT NewNum;
			Ar << NewNum;
			A.Empty( NewNum );
			for( INT i=0; i<NewNum; i++ )
			{
				Ar << *new(A)T;
			}
		}
		else
		{
			// Save array.
			Ar << A.ArrayNum;
			for( INT i=0; i<A.ArrayNum; i++ )
			{
				Ar << A( i );
			}
		}
		return Ar;
	}
	void CountBytes( FArchive& Ar )
	{
		FArray::CountBytes( Ar, sizeof(T*) );
	}
	void Remove( INT Index, INT Count=1 )
	{
		check(Index>=0);
		check(Index<=ArrayNum);
		check(Index+Count<=ArrayNum);
		for( INT i=Index; i<Index+Count; i++ )
		{
			delete ((T**)Data)[i];
		}
		FArray::Remove( Index, Count, sizeof(T*), DEFAULT_ALIGNMENT );
	}
	void Empty( INT Slack=0 )
	{
		for( INT i=0; i<ArrayNum; i++ )
		{
			delete ((T**)Data)[i];
		}
		FArray::Empty( sizeof(T*), DEFAULT_ALIGNMENT, Slack );
	}
	INT AddRawItem(T* Item)
	{
		INT	Index = FArray::Add(1,sizeof(T*), DEFAULT_ALIGNMENT);
		((T**)Data)[Index] = Item;
		return Index;
	}
	/** 
	* Helper function to return the amount of memory allocated by this container 
	*
	* @return number of bytes allocated by this container
	*/
	DWORD GetAllocatedSize( void ) const
	{
		return( ArrayMax * (sizeof(T) + sizeof(T*)) );
	}
};

template<class T> void* operator new( size_t Size, TIndirectArray<T>& Array )
{
	check(Size == sizeof(T));
	INT Index = Array.AddRawItem((T*)appMalloc(Size));
	return &Array(Index);
}

/*-----------------------------------------------------------------------------
	Transactional array.
-----------------------------------------------------------------------------*/

// NOTE: Right now, you can't make these aligned (a la TArray's Alignment parameter). If
// you need to do it, you will have to fix up FTransaction::FObjectRecord::SerializeContents()
// to pass the Alignment to the FArray functions
template< class T > class TTransArray : public TArray<T>
{
public:
	// Constructors.
	TTransArray( UObject* InOwner, INT InNum=0 )
	:	TArray<T>( InNum )
	,	Owner( InOwner )
	{
		checkSlow(Owner);
	}
	TTransArray( UObject* InOwner, const TArray<T>& Other )
	:	TArray<T>( Other )
	,	Owner( InOwner )
	{
		checkSlow(Owner);
	}
	TTransArray& operator=( const TTransArray& Other )
	{
		operator=( (const TArray<T>&)Other );
		return *this;
	}

	// Add, Insert, Remove, Empty interface.
	INT Add( INT Count=1 )
	{
		INT Index = TArray<T>::Add( Count );
		if( GUndo )
		{
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		}
		return Index;
	}
	void Insert( INT Index, INT Count=1 )
	{
		FArray::Insert( Index, Count, sizeof(T), DEFAULT_ALIGNMENT );
		if( GUndo )
		{
			GUndo->SaveArray( Owner, this, Index, Count, 1, sizeof(T), SerializeItem, DestructItem );
		}
	}
	void Remove( INT Index, INT Count=1 )
	{
		if( GUndo )
		{
			GUndo->SaveArray( Owner, this, Index, Count, -1, sizeof(T), SerializeItem, DestructItem );
		}
		TArray<T>::Remove( Index, Count );
	}
	void Empty( INT Slack=0 )
	{
		if( GUndo )
		{
			GUndo->SaveArray( Owner, this, 0, this->ArrayNum, -1, sizeof(T), SerializeItem, DestructItem );
		}
		TArray<T>::Empty( Slack );
	}

	// Functions dependent on Add, Remove.
	TTransArray& operator=( const TArray<T>& Other )
	{
		if( this != &Other )
		{
			Empty( Other.Num() );
			for( INT i=0; i<Other.Num(); i++ )
			{
				new( *this )T( Other(i) );
			}
		}
		return *this;
	}
	INT AddItem( const T& Item )
	{
		new(*this) T(Item);
		return this->Num() - 1;
	}
	INT AddZeroed( INT n=1 )
	{
		INT Index = Add(n);
		appMemzero( &(*this)(Index), n*sizeof(T) );
		return Index;
	}
	INT AddUniqueItem( const T& Item )
	{
		for( INT Index=0; Index<this->ArrayNum; Index++ )
		{
			if( (*this)(Index)==Item )
			{
				return Index;
			}
		}
		return AddItem( Item );
	}
	INT RemoveItem( const T& Item )
	{
		check( ((&Item) < (T*)this->Data) || ((&Item) >= (T*)this->Data+this->ArrayMax) );
		INT OriginalNum=this->ArrayNum;
		for( INT Index=0; Index<this->ArrayNum; Index++ )
		{
			if( (*this)(Index)==Item )
			{
				Remove( Index-- );
			}
		}
		return OriginalNum - this->ArrayNum;
	}

	// TTransArray interface.
	UObject* GetOwner()
	{
		return Owner;
	}
	void SetOwner( UObject* NewOwner )
	{
		Owner = NewOwner;
	}
	void ModifyItem( INT Index )
	{
		if( GUndo )
			GUndo->SaveArray( Owner, this, Index, 1, 0, sizeof(T), SerializeItem, DestructItem );
	}
	void ModifyAllItems()
	{
		if( GUndo )
		{
			GUndo->SaveArray( Owner, this, 0, this->Num(), 0, sizeof(T), SerializeItem, DestructItem );
		}
	}
	friend FArchive& operator<<( FArchive& Ar, TTransArray& A )
	{
		if ( Ar.Ver() >= VER_SERIALIZE_TTRANSARRAY_OWNER )
		{
			Ar << A.Owner;
		}
		Ar << (TArray<T>&)A;
		return Ar;
	}
protected:
	static void SerializeItem( FArchive& Ar, void* TPtr )
	{
		Ar << *(T*)TPtr;
	}
	static void DestructItem( void* TPtr )
	{
		((T*)TPtr)->~T();
	}
	UObject* Owner;
private:

	// Disallow the copy constructor.
	TTransArray( const TArray<T>& Other )
	{}
};

//
// Transactional array operator news.
//
template <class T> void* operator new( size_t Size, TTransArray<T>& Array )
{
	check(Size == sizeof(T));
	INT Index = Array.Add();
	return &Array(Index);
}
template <class T> void* operator new( size_t Size, TTransArray<T>& Array, INT Index )
{
	check(Size == sizeof(T));
	Array.Insert(Index);
	return &Array(Index);
}

/*-----------------------------------------------------------------------------
	Dynamic strings.
-----------------------------------------------------------------------------*/

//
// A dynamically sizeable string.
//
class FString : protected TArray<TCHAR>
{
public:
	FString()
	: TArray<TCHAR>()
	{}
	FString( const FString& Other )
	: TArray<TCHAR>( Other.ArrayNum )
	{
		if( ArrayNum )
		{
			appMemcpy( GetData(), Other.GetData(), ArrayNum*sizeof(TCHAR) );
		}
	}
	FString( const TCHAR* In )
	: TArray<TCHAR>( *In ? (appStrlen(In)+1) : 0 )
	{
		if( ArrayNum )
		{
			appMemcpy( GetData(), In, ArrayNum*sizeof(TCHAR) );
		}
	}
	explicit FString( INT InCount, const TCHAR* InSrc )
	:	TArray<TCHAR>( InCount ? InCount+1 : 0 )
	{
		if( ArrayNum )
		{
			appStrncpy( &(*this)(0), InSrc, InCount+1 );
		}
	}
#ifdef UNICODE // separate this out if ANSICHAR != UNICHAR
	FString( const ANSICHAR* In )
	: TArray<TCHAR>( *In ? (strlen(In)+1) : 0 )
    {
		if( ArrayNum )
		{
			appMemcpy( GetData(), ANSI_TO_TCHAR(In), ArrayNum*sizeof(TCHAR) );
		}
    }
#else
	FString( const UNICHAR* In )
	: TArray<TCHAR>( 0 )
    {
		appErrorf(TEXT("Attempting to use a Unicode string on a non-Unicode system!"));
    }
#endif
	FString( ENoInit )
	: TArray<TCHAR>( E_NoInit )
	{}
	FString& operator=( const TCHAR* Other )
	{
		if( GetData() != Other )
		{
			ArrayNum = ArrayMax = *Other ? appStrlen(Other)+1 : 0;
			Realloc( sizeof(TCHAR), DEFAULT_ALIGNMENT ); // @todo: is there a way to get the inherited template param from TArray?
			if( ArrayNum )
			{
				appMemcpy( GetData(), Other, ArrayNum*sizeof(TCHAR) );
			}
		}
		return *this;
	}
	FString& operator=( const FString& Other )
	{
		if( this != &Other )
		{
			ArrayNum = ArrayMax = Other.Num();
			Realloc( sizeof(TCHAR), DEFAULT_ALIGNMENT );
			if( ArrayNum )
			{
				appMemcpy( GetData(), *Other, ArrayNum*sizeof(TCHAR) );
			}
		}
		return *this;
	}
    TCHAR& operator[]( INT i )
	{
		checkSlow(i>=0);
		checkSlow(i<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((TCHAR*)Data)[i];
	}
	const TCHAR& operator[]( INT i ) const
	{
		checkSlow(i>=0);
		checkSlow(i<ArrayNum);
		checkSlow(ArrayMax>=ArrayNum);
		return ((TCHAR*)Data)[i];
	}

	~FString()
	{
		TArray<TCHAR>::Empty();		
	}
	void Empty( INT Slack=0 )
	{
		TArray<TCHAR>::Empty(Slack > 0 ? Slack + 1 : Slack);
	}
	void Shrink()
	{
		TArray<TCHAR>::Shrink();
	}
	const TCHAR* operator*() const
	{
		return Num() ? &(*this)(0) : TEXT("");
	}
	TArray<TCHAR>& GetCharArray()
	{
		//warning: Operations on the TArray<CHAR> can be unsafe, such as adding
		// non-terminating 0's or removing the terminating zero.
		return (TArray<TCHAR>&)*this;
	}
	FString& operator+=( const TCHAR* Str )
	{
		checkSlow(Str);
		checkSlow(ArrayNum>=0);
		checkSlow(ArrayMax>=ArrayNum);

		if( *Str != '\0' )
		{
			if( ArrayNum != 0 )
			{
				INT Index = ArrayNum-1;
				Add( appStrlen(Str) );
				appStrcpy( &(*this)(Index), ArrayNum-Index, Str );
			}
			else if( *Str )
			{
				Add( appStrlen(Str)+1 );
				appStrcpy( &(*this)(0), ArrayNum, Str );
			}
		}
		return *this;
	}
	FString& operator+=(const TCHAR inChar)
	{
		checkSlow(ArrayMax>=ArrayNum);
		checkSlow(ArrayNum>=0);

		if ( inChar != '\0' )
		{
			// position to insert the character.  
			// At the end of the string if we have existing characters, otherwise at the 0 position
			INT InsertIndex = (ArrayNum > 0) ? ArrayNum-1 : 0;	

			// number of characters to add.  If we don't have any existing characters, 
			// we'll need to append the terminating zero as well.
			INT InsertCount = (ArrayNum > 0) ? 1 : 2;				

			Add(InsertCount);
			(*this)(InsertIndex) = inChar;
			(*this)(InsertIndex+1) = '\0';
		}
		return *this;
	}
	FString& AppendChar(const TCHAR inChar)
	{
		*this += inChar;
		return *this;
	}
	FString& operator+=( const FString& Str )
	{
		return operator+=( *Str );
	}
	FString operator+( const TCHAR inChar ) const
	{
		return FString(*this) += inChar;
	}
	FString operator+( const TCHAR* Str ) const
	{
		return FString( *this ) += Str;
	}
	FString operator+( const FString& Str ) const
	{
		return operator+( *Str );
	}
	FString& operator*=( const TCHAR* Str )
	{
		if( ArrayNum>1 && (*this)(ArrayNum-2)!=PATH_SEPARATOR[0] )
			*this += PATH_SEPARATOR;
		return *this += Str;
	}
	FString& operator*=( const FString& Str )
	{
		return operator*=( *Str );
	}
	FString operator*( const TCHAR* Str ) const
	{
		return FString( *this ) *= Str;
	}
	FString operator*( const FString& Str ) const
	{
		return operator*( *Str );
	}
	UBOOL operator<=( const TCHAR* Other ) const
	{
		return !(appStricmp( **this, Other ) > 0);
	}
	UBOOL operator<( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other ) < 0;
	}
	UBOOL operator<( const FString& Other ) const
	{
		return appStricmp( **this, *Other ) < 0;
	}
	UBOOL operator>=( const TCHAR* Other ) const
	{
		return !(appStricmp( **this, Other ) < 0);
	}
	UBOOL operator>( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other ) > 0;
	}
	UBOOL operator>( const FString& Other ) const
	{
		return appStricmp( **this, *Other ) > 0;
	}
	UBOOL operator==( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other )==0;
	}
	UBOOL operator==( const FString& Other ) const
	{
		return appStricmp( **this, *Other )==0;
	}
	UBOOL operator!=( const TCHAR* Other ) const
	{
		return appStricmp( **this, Other )!=0;
	}
	UBOOL operator!=( const FString& Other ) const
	{
		return appStricmp( **this, *Other )!=0;
	}
	UBOOL operator<=(const FString& Str) const
	{
		return !(appStricmp(**this, *Str) > 0);
	}
	UBOOL operator>=(const FString& Str) const
	{
		return !(appStricmp(**this, *Str) < 0);
	}
	INT Len() const
	{
		return Num() ? Num()-1 : 0;
	}
	FString Left( INT Count ) const
	{
		return FString( Clamp(Count,0,Len()), **this );
	}
	FString LeftChop( INT Count ) const
	{
		return FString( Clamp(Len()-Count,0,Len()), **this );
	}
	FString Right( INT Count ) const
	{
		return FString( **this + Len()-Clamp(Count,0,Len()) );
	}
	FString Mid( INT Start, INT Count=MAXINT ) const
	{
		DWORD End = Start+Count;
		Start    = Clamp( (DWORD)Start, (DWORD)0,     (DWORD)Len() );
		End      = Clamp( (DWORD)End,   (DWORD)Start, (DWORD)Len() );
		return FString( End-Start, **this + Start );
	}

	//@{
	/**
	 * Searches the string for a substring, and returns index into this string
	 * of the first found instance. Can search from beginning or end, and ignore case or not.
	 *
	 * @param SubStr The string to search for
	 * @param bSearchFromEnd If TRUE, the search will start at the end of the string and go backwards
	 * @param bIgnoreCase If TRUE, the search will be case insensitive
	 */
	INT InStr( const TCHAR* SubStr, UBOOL bSearchFromEnd=FALSE, UBOOL bIgnoreCase=FALSE ) const
	{
		if( !bSearchFromEnd )
		{
			const TCHAR* Tmp = bIgnoreCase ? appStristr(**this, SubStr) : appStrstr(**this,SubStr);
			return Tmp ? (Tmp-**this) : -1;
		}
		else
		{
			// if ignoring, do a onetime ToUpper on both strings, to avoid ToUppering multiple
			// times in the loop below
			if (bIgnoreCase)
			{
				return ToUpper().InStr(FString(SubStr).ToUpper(), TRUE);
			}
			else
			{
				for( INT i=Len()-1; i>=0; i-- )
				{
					INT j;
					for( j=0; SubStr[j]; j++ )
						if( (*this)(i+j)!=SubStr[j] )
							break;
					if( !SubStr[j] )
						return i;
				}
				return -1;
			}
		}
	}
	INT InStr( const FString& SubStr, UBOOL bSearchFromEnd=FALSE, UBOOL bIgnoreCase=FALSE ) const
	{
		return InStr( *SubStr, bSearchFromEnd, bIgnoreCase );
	}
	//@}

	UBOOL Split( const FString& InS, FString* LeftS, FString* RightS, UBOOL InRight=0 ) const
	{
		INT InPos = InStr(InS,InRight);
		if( InPos<0 )
			return 0;
		if( LeftS )
			*LeftS = Left(InPos);
		if( RightS )
			*RightS = Mid(InPos+InS.Len());
		return 1;
	}
	FString ToUpper() const
	{
		FString New( **this );
		for( INT i=0; i< New.ArrayNum; i++ )
			New(i) = appToUpper(New(i));
		return New;
	}
	FString ToLower() const
	{
		FString New( **this );
		for( INT i=0; i<New.ArrayNum; i++ )
			New(i) = appToLower(New(i));
		return New;
	}
	FString LeftPad( INT ChCount ) const;
	FString RightPad( INT ChCount ) const;
	
	UBOOL IsNumeric() const;
	
	VARARG_DECL( static FString, static FString, return, Printf, VARARG_NONE, const TCHAR*, VARARG_NONE, VARARG_NONE );

	static FString Chr( TCHAR Ch );
	friend FArchive& operator<<( FArchive& Ar, FString& S );
	friend struct FStringNoInit;


	/**
	 * Returns TRUE if this string begins with the specified text.
	 */
	UBOOL StartsWith(const FString& InPrefix ) const
	{
		return !appStrnicmp(**this, *InPrefix, InPrefix.Len());
	}

	/**
	 * Removes whitespace characters from the front of this string.
	 */
	FString Trim()
	{
		INT Pos = 0;
		while(Pos < Len())
		{
			if( appIsWhitespace( (*this)[Pos] ) )
			{
				Pos++;
			}
			else
			{
				break;
			}
		}

		*this = Right( Len()-Pos );

		return *this;
	}

	/**
	 * Removes trailing whitespace characters
	 */
	FString TrimTrailing( void )
	{
		INT Pos = Len() - 1;
		while( Pos >= 0 )
		{
			if( !appIsWhitespace( ( *this )[Pos] ) )
			{
				break;
			}

			Pos--;
		}

		*this = Left( Pos + 1 );

		return( *this );
	}

	/**
	 * Returns a copy of this string with wrapping quotation marks removed.
	 */
	FString TrimQuotes( UBOOL* bQuotesRemoved=NULL ) const
	{
		UBOOL bQuotesWereRemoved=FALSE;
		INT Start = 0, Count = Len();
		if ( Count > 0 )
		{
			if ( (*this)[0] == TCHAR('"') )
			{
				Start++;
				Count--;
				bQuotesWereRemoved=TRUE;
			}

			if ( Len() > 1 && (*this)[Len() - 1] == TCHAR('"') )
			{
				Count--;
				bQuotesWereRemoved=TRUE;
			}
		}

		if ( bQuotesRemoved != NULL )
		{
			*bQuotesRemoved = bQuotesWereRemoved;
		}
		return Mid(Start, Count);
	}

	/**
	 * Breaks up a delimited string into elements of a string array.
	 *
	 * @param	InArray		The array to fill with the string pieces
	 * @param	pchDelim	The string to delimit on
	 * @param	InCullEmpty	If 1, empty strings are not added to the array
	 *
	 * @return	The number of elements in InArray
	 */
	INT ParseIntoArray( TArray<FString>* InArray, const TCHAR* pchDelim, UBOOL InCullEmpty ) const;

	/**
	 * Breaks up a delimited string into elements of a string array, using any whitespace and an 
	 * optional extra delimter, like a ","
	 *
	 * @param	InArray			The array to fill with the string pieces
	 * @param	pchExtraDelim	The string to delimit on
	 *
	 * @return	The number of elements in InArray
	 */
	INT ParseIntoArrayWS( TArray<FString>* InArray, const TCHAR* pchExtraDelim = NULL ) const;

	/**
	 * Takes an array of strings and removes any zero length entries.
	 *
	 * @param	InArray	The array to cull
	 *
	 * @return	The number of elements left in InArray
	 */
	static INT CullArray( TArray<FString>* InArray )
	{
		check(InArray);
		for( INT x=InArray->Num()-1; x>=0; x-- )
		{
			if( (*InArray)(x).Len() == 0 )
			{
				InArray->Remove( x );
			}
		}
		return InArray->Num();
	}
	/**
	 * Returns a copy of this string, with the characters in reverse order
	 */
	FString Reverse() const
	{
		FString New;
		for( int x = Len()-1 ; x > -1 ; x-- )
			New += Mid(x,1);
		return New;
	}

	/**
	 * Reverses the order of characters in this string
	 */
	void ReverseString()
	{
		if ( Len() > 0 )
		{
			TCHAR* StartChar = &(*this)(0);
			TCHAR* EndChar = &(*this)(Len()-1);
			TCHAR TempChar;
			do 
			{
				TempChar = *StartChar;	// store the current value of StartChar
				*StartChar = *EndChar;	// change the value of StartChar to the value of EndChar
				*EndChar = TempChar;	// change the value of EndChar to the character that was previously at StartChar

				StartChar++;
				EndChar--;

			} while( StartChar < EndChar );	// repeat until we've reached the midpoint of the string
		}
	}

	// Replace all occurences of a substring
	FString Replace(const TCHAR* From, const TCHAR* To) const;

	/**
	 * Returns a copy of this string with all quote marks escaped (unless the quote is already escaped)
	 */
	FString ReplaceQuotesWithEscapedQuotes() const;

	// Takes the number passed in and formats the string in comma format ( 12345 becomes "12,345")
	static FString FormatAsNumber( INT InNumber )
	{
		FString Number = appItoa( InNumber ), Result;

		int dec = 0;
		for( int x = Number.Len()-1 ; x > -1 ; --x )
		{
			Result += Number.Mid(x,1);

			dec++;
			if( dec == 3 && x > 0 )
			{
				Result += TEXT(",");
				dec = 0;
			}
		}

		return Result.Reverse();
	}
};
struct FStringNoInit : public FString
{
	FStringNoInit()
	: FString( E_NoInit )
	{}
	FStringNoInit& operator=( const TCHAR* Other )
	{
		if( GetData() != Other )
		{
			ArrayNum = ArrayMax = *Other ? appStrlen(Other)+1 : 0;
			Realloc( sizeof(TCHAR), DEFAULT_ALIGNMENT );
			if( ArrayNum )
			{
				appMemcpy( GetData(), Other, ArrayNum*sizeof(TCHAR) );
			}
		}
		return *this;
	}
	FStringNoInit& operator=( const FString& Other )
	{
		if( this != &Other )
		{
			ArrayNum = ArrayMax = Other.Num();
			Realloc( sizeof(TCHAR), DEFAULT_ALIGNMENT );
			if( ArrayNum )
			{
				appMemcpy( GetData(), *Other, ArrayNum*sizeof(TCHAR) );
			}
		}
		return *this;
	}
};
inline DWORD GetTypeHash( const FString& S )
{
	return appStrihash(*S);
}
template <> class TTypeInfo<FString> : public TTypeInfoConstructedBase<FString>
{
public:
	typedef const TCHAR* ConstInitType;
};

/*-----------------------------------------------------------------------------
	FFilename.
-----------------------------------------------------------------------------*/

/**
 * Utility class for quick inquiries against filenames.
 */
class FFilename : public FString
{
public:
	FFilename()
		: FString()
	{}
	FFilename( const FString& Other )
		: FString( Other )
	{}
	FFilename( const TCHAR* In )
		: FString( In )
	{}
#ifdef UNICODE // separate this out if ANSICHAR != UNICHAR
	FFilename( const ANSICHAR* In )
		: FString( In )
    {}
#endif
	FFilename( ENoInit )
		: FString( E_NoInit )
	{}

	// Returns the text following the last period.

	FString GetExtension() const;

	// Returns the filename (with extension), minus any path information.
	FString GetCleanFilename() const;

	// Returns the same thing as GetCleanFilename, but without the extension
	FString GetBaseFilename( UBOOL bRemovePath=TRUE ) const;

	// Returns the path in front of the filename
	FString GetPath() const;

	/**
	 * Returns the localized package name by appending the language suffix before the extension.
	 *
	 * @param	Language	Language to use.
	 * @return	Localized package name
	 */
	FString GetLocalizedFilename( const TCHAR* Language = NULL ) const;
};

//
// String exchanger.
//
inline void ExchangeString( FString& A, FString& B )
{
	appMemswap( &A, &B, sizeof(FString) );
}

/*----------------------------------------------------------------------------
	Special archivers.
----------------------------------------------------------------------------*/

//
// String output device.
//
class FStringOutputDevice : public FString, public FOutputDevice
{
public:
	FStringOutputDevice( const TCHAR* OutputDeviceName=TEXT("") ):
		FString( OutputDeviceName )
	{}
	void Serialize( const TCHAR* InData, EName Event )
	{
		*this += (TCHAR*)InData;
	}
};


/*----------------------------------------------------------------------------
	Sorting template.
----------------------------------------------------------------------------*/

#define IMPLEMENT_COMPARE_POINTER( Type, Filename, FunctionBody )		\
class Compare##Filename##Type##Pointer									\
	{																	\
	public:																\
		static inline INT Compare( Type* A, Type* B	)					\
			FunctionBody												\
	};


#define IMPLEMENT_COMPARE_CONSTPOINTER( Type, Filename, FunctionBody )	\
class Compare##Filename##Type##ConstPointer								\
	{																	\
	public:																\
	static inline INT Compare( const Type* A, const Type* B	)			\
	FunctionBody												        \
	};


#define IMPLEMENT_COMPARE_CONSTREF( Type, Filename, FunctionBody )		\
class Compare##Filename##Type##ConstRef									\
	{																	\
	public:																\
		static inline INT Compare( const Type& A, const Type& B	)		\
			FunctionBody												\
	};

#define COMPARE_CONSTREF_CLASS( Type, Filename ) Compare##Filename##Type##ConstRef
#define COMPARE_POINTER_CLASS( Type, Filename ) Compare##Filename##Type##Pointer
#define COMPARE_CONSTPOINTER_CLASS( Type, Filename ) Compare##Filename##Type##ConstPointer

#define USE_COMPARE_POINTER( Type, Filename ) Type*,Compare##Filename##Type##Pointer	
#define USE_COMPARE_CONSTPOINTER( Type, Filename ) Type*,Compare##Filename##Type##ConstPointer	
#define USE_COMPARE_CONSTREF( Type, Filename ) Type,Compare##Filename##Type##ConstRef	


/**
 * Helper class for the Sort method.
 */
template<class T> struct TStack
{
	T* Min;
	T* Max;
};

/**
 * Sort elements. The sort is unstable, meaning that the ordering of equal items is not necessarily preserved.
 *
 * @param	First	pointer to the first element to sort
 * @param	Num		the number of items to sort
 */
template<class T, class CompareClass> void Sort( T* First, INT Num )
{
	if( Num < 2 )
	{
		return;
	}
	TStack<T> RecursionStack[32]={{First,First+Num-1}}, Current, Inner;
	for( TStack<T>* StackTop=RecursionStack; StackTop>=RecursionStack; --StackTop )
	{
		Current = *StackTop;
	Loop:
		INT Count = Current.Max - Current.Min + 1;
		if( Count <= 8 )
		{
			// Use simple bubble-sort.
			while( Current.Max > Current.Min )
			{
				T *Max, *Item;
				for( Max=Current.Min, Item=Current.Min+1; Item<=Current.Max; Item++ )
					if( CompareClass::Compare(*Item, *Max) > 0 )
						Max = Item;
				Exchange( *Max, *Current.Max-- );
			}
		}
		else
		{
			// Grab middle element so sort doesn't exhibit worst-cast behavior with presorted lists.
			Exchange( Current.Min[Count/2], Current.Min[0] );

			// Divide list into two halves, one with items <=Current.Min, the other with items >Current.Max.
			Inner.Min = Current.Min;
			Inner.Max = Current.Max+1;
			for( ; ; )
			{
				while( ++Inner.Min<=Current.Max && CompareClass::Compare(*Inner.Min, *Current.Min) <= 0 );
				while( --Inner.Max> Current.Min && CompareClass::Compare(*Inner.Max, *Current.Min) >= 0 );
				if( Inner.Min>Inner.Max )
					break;
				Exchange( *Inner.Min, *Inner.Max );
			}
			Exchange( *Current.Min, *Inner.Max );

			// Save big half and recurse with small half.
			if( Inner.Max-1-Current.Min >= Current.Max-Inner.Min )
			{
				if( Current.Min+1 < Inner.Max )
				{
					StackTop->Min = Current.Min;
					StackTop->Max = Inner.Max - 1;
					StackTop++;
				}
				if( Current.Max>Inner.Min )
				{
					Current.Min = Inner.Min;
					goto Loop;
				}
			}
			else
			{
				if( Current.Max>Inner.Min )
				{
					StackTop->Min = Inner  .Min;
					StackTop->Max = Current.Max;
					StackTop++;
				}
				if( Current.Min+1<Inner.Max )
				{
					Current.Max = Inner.Max - 1;
					goto Loop;
				}
			}
		}
	}
}

/*----------------------------------------------------------------------------
	TMap.
----------------------------------------------------------------------------*/

//
// Maps unique keys to values.
//
/**
 * Base for classes which use hash tables for tracking a list of key/value mappings. 
 */
template< class TK, class TI > class TMapBase
{
protected:
	class TPair
	{
	public:
		INT HashNext;
		TK Key;
		TI Value;
		TPair( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
		: Key( InKey ), Value( InValue )
		{}
		TPair()
		{}
		friend FArchive& operator<<( FArchive& Ar, TPair& F )
		{
			return Ar << F.Key << F.Value;
		}

		/** Comparison operators */
		inline UBOOL operator==( const TPair& Other ) const
		{
			return Key == Other.Key && Value == Other.Value;
		}
		inline UBOOL operator!=( const TPair& Other ) const
		{
			return Key != Other.Key || Value != Other.Value;
		}
	};
	void Rehash()
	{
		checkSlow(!(HashCount&(HashCount-1)));
		delete[] Hash;
		Hash = new INT[HashCount];
		for( INT i=0; i<HashCount; i++ )
		{
			Hash[i] = INDEX_NONE;
		}
		for( INT i=0; i<Pairs.Num(); i++ )
		{
			TPair& Pair    = Pairs(i);
			INT    iHash   = (GetTypeHash(Pair.Key) & (HashCount-1));
			Pair.HashNext  = Hash[iHash];
			Hash[iHash] = i;
		}
	}
	FORCEINLINE void AllocHash()
	{
		if(!Hash)
		{
			Rehash();
		}
	}
	void Relax()
	{
		while( HashCount>Pairs.Num()*2+8 )
		{
			HashCount /= 2;
		}
		Rehash();
	}
	TI& Add( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		TPair& Pair   = *new(Pairs)TPair( InKey, InValue );
		INT    iHash  = (GetTypeHash(Pair.Key) & (HashCount-1));
		Pair.HashNext = Hash[iHash];
		Hash[iHash]   = Pairs.Num()-1;
		if( HashCount*2 + 8 < Pairs.Num() )
		{
			HashCount *= 2;
			Rehash();
		}
		return Pair.Value;
	}
	TArray<TPair> Pairs;
	INT* Hash;
	INT HashCount;
public:
	TMapBase()
	:	Hash( NULL )
	,	HashCount( 8 )
	{}
	TMapBase( const TMapBase& Other )
	:	Pairs( Other.Pairs )
	,	Hash( NULL )
	,	HashCount( Other.HashCount )
	{
		Rehash();
	}
	~TMapBase()
	{
		delete [] Hash;
		Hash = NULL;
		HashCount = 0;
	}
	TMapBase& operator=( const TMapBase& Other )
	{
		Pairs     = Other.Pairs;
		HashCount = Other.HashCount;
		Rehash();
		return *this;
	}
	/** Comparison operator */
	inline UBOOL operator==( const TMapBase& Other ) const
	{
		return Pairs == Other.Pairs;
	}
	inline UBOOL operator!=( const TMapBase& Other ) const
	{
		return Pairs != Other.Pairs;
	}
	void Empty( INT Slack=0 )
	{
		checkSlow(!(HashCount&(HashCount-1)));
		Pairs.Empty( Slack );
		HashCount = 8;
		delete [] Hash;
		Hash = NULL;
	}
    /**
     * Efficiently empties out the map but preserves all allocations and capacities
     */
    void Reset()
    {
        checkSlow(!(HashCount&(HashCount-1)));
        Pairs.Reset();
		if( Hash )
		{
			for( INT i=0; i<HashCount; i++ )
			{
				Hash[i] = INDEX_NONE;
			}
		}
    }
	/**
	 * Shrinks the pairs array to avoid slack.
	 */
	void Shrink()
	{
		Pairs.Shrink();
	}
	/** 
	 * Helper function to return the amount of memory allocated by this container 
	 *
	 * @return number of bytes allocated by this container
	 */
	DWORD GetAllocatedSize( void ) const
	{
		return( Pairs.GetAllocatedSize() + ( HashCount * sizeof( INT ) ) );
	}
	void CountBytes( FArchive& Ar )
	{
		Pairs.CountBytes( Ar );
	}
	TI& Set( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		AllocHash();
		if ( Pairs.Num() > 0 )
		{
			for( INT i=Hash[(GetTypeHash(InKey) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			{
				if( Pairs(i).Key==InKey )
				{
					Pairs(i).Value=InValue;
					return Pairs(i).Value;
				}
			}
		}
		return Add( InKey, InValue );
	}
	INT Remove( typename TTypeInfo<TK>::ConstInitType InKey )
	{
		INT Count=0;
		for( INT i=Pairs.Num()-1; i>=0; i-- )
		{
			if( Pairs(i).Key==InKey )
			{
				Pairs.Remove(i); Count++;
			}
		}
		if( Count )
		{
			Relax();
		}
		return Count;
	}
	// This is slow, but it's good for specific situations
	TK* FindKey( const TI& Value )
	{
		for( INT i = 0 ; i < Pairs.Num() ; ++i )
		{
			if( Pairs(i).Value == Value )
			{
				return &Pairs(i).Key;
			}
		}
		return NULL;
	}
	/**
	 * Returns the key associated with the specified value
	 *
	 * @param	Value	the value to search for
	 *
	 * @return	a pointer to the key associated with the value specified, or NULL if the value isn't contained in this map
	 */
	const TK* FindKey( const TI& Value ) const
	{
		for( INT i = 0 ; i < Pairs.Num() ; ++i )
		{
			if( Pairs(i).Value == Value )
			{
				return &Pairs(i).Key;
			}
		}
		return NULL;
	}
	TI* Find( const TK& Key )
	{
		if( Hash && Pairs.Num() > 0 )
		{
			for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			{
				if( Pairs(i).Key==Key )
				{
					return &Pairs(i).Value;
				}
			}
		}
		return NULL;
	}
	TI FindRef( const TK& Key ) const
	{
		if( Hash && Pairs.Num() > 0 )
		{
			for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			{
				if( Pairs(i).Key==Key )
				{
					return Pairs(i).Value;
				}
			}
		}
		return (TI) NULL;
	}
	const TI* Find( const TK& Key ) const
	{
		if( Hash && Pairs.Num() > 0 )
		{
			for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))]; i!=INDEX_NONE; i=Pairs(i).HashNext )
			{
				if( Pairs(i).Key==Key )
				{
					return &Pairs(i).Value;
				}
			}
		}
		return NULL;
	}
	UBOOL HasKey( const TK& Key ) const
	{
		UBOOL bHasKey = FALSE;
		if( Hash && Pairs.Num() > 0 )
		{
			for( INT i=Hash[(GetTypeHash(Key) & (HashCount-1))];
				i!=INDEX_NONE && bHasKey == FALSE; i=Pairs(i).HashNext )
			{
				bHasKey = Pairs(i).Key==Key;
			}
		}
		return bHasKey;
	}
	/**
	 * Generates an array from the keys in this map.
	 */
	void GenerateKeyArray( TArray<TK>& out_Array ) const
	{
		out_Array.Empty(Pairs.Num());
		if ( Pairs.Num() > 0 )
		{
			out_Array.AddZeroed(Pairs.Num());
			for ( INT PairIndex = 0; PairIndex < Pairs.Num(); PairIndex++ )
			{
				out_Array(PairIndex) = Pairs(PairIndex).Key;
			}
		}
	}
	/**
	 * Generates an array from the values in this map.
	 */
	void GenerateValueArray( TArray<TI>& out_Array ) const
	{
		out_Array.Empty(Pairs.Num());
		if ( Pairs.Num() > 0 )
		{
			out_Array.AddZeroed(Pairs.Num());
			for ( INT PairIndex = 0; PairIndex < Pairs.Num(); PairIndex++ )
			{
				out_Array(PairIndex) = Pairs(PairIndex).Value;
			}
		}
	}

	friend FArchive& operator<<( FArchive& Ar, TMapBase& M )
	{
		Ar << M.Pairs;
		if( Ar.IsLoading() )
	    {
		    while( M.HashCount*2+8 < M.Pairs.Num() )
			{
			    M.HashCount *= 2;
			}

			if( M.Pairs.Num() )
			{
				M.Rehash();
			}
			else
			{
				delete [] M.Hash;
				M.Hash = NULL;
			}
		}
		return Ar;
	}
	void Dump( FOutputDevice& Ar )
	{
		AllocHash();
		Ar.Logf( TEXT("TMapBase: %i items, %i hash slots"), Pairs.Num(), HashCount );
		for( INT i=0; i<HashCount; i++ )
		{
			INT c=0;
			for( INT j=Hash[i]; j!=INDEX_NONE; j=Pairs(j).HashNext )
			{
				c++;
			}
			Ar.Logf( TEXT("   Hash[%i] = %i"), i, c );
		}
	}
	class TIterator
	{
	public:
		TIterator( TMapBase& InMap ) : Map( InMap ), Pairs( InMap.Pairs ), Index( 0 ), Removed(0) {}
		~TIterator()               { if( Removed ) Map.Relax(); }
		void operator++()          { ++Index; }
		void RemoveCurrent()       { Pairs.Remove(Index--); Removed++; }
		operator UBOOL() const     { return Pairs.IsValidIndex(Index); }
		const TK& Key() const      { return Pairs(Index).Key; }
		TI& Value() const          { return Pairs(Index).Value; }
	private:
		TMapBase& Map;
		TArray<TPair>& Pairs;
		INT Index;
		INT Removed;
	};
	class TConstIterator
	{
	public:
		TConstIterator( const TMapBase& InMap ) : Map(InMap), Pairs( InMap.Pairs ), Index( 0 ) {}
		~TConstIterator() {}
		void operator++()          { ++Index; }
		operator UBOOL() const     { return Pairs.IsValidIndex(Index); }
		const TK& Key() const      { return Pairs(Index).Key; }
		const TI& Value() const    { return Pairs(Index).Value; }
	private:
		const TMapBase& Map;
		const TArray<TPair>& Pairs;
		INT Index;
	};
	friend class TIterator;
	friend class TConstIterator;

	// typedefs for the template parameters.
	typedef TK KeyType;
	typedef TI ValueType;

	/**
	 * Helper class for sorting the pairs using each pair's Key; forwards calls to Compare to the class provided by the CompareClass parameter.
	 */
	template<class CompareClass>
	class KeyComparisonClass
	{
	public:
		static INT Compare(const TPair& A, const TPair& B)
		{
			return CompareClass::Compare(A.Key, B.Key);
		}
	};


	/**
	 * Helper class for sorting the pairs array using each pair's Value; forwards calls to Compare to the class provided by the CompareClass parameter.
	 */
	template<class CompareClass>
	class ValueComparisonClass
	{
	public:
		static INT Compare( const TPair& A, const TPair& B )
		{
			return CompareClass::Compare(A.Value, B.Value);
		}
	};

	/**
	 * Sorts the pairs array using each pair's Key as the sort criteria, then rebuilds the map's hash.
	 *
	 * Invoked using "MyMapVar.KeySort<COMPARE_CONSTREF_CLASS(KeyType,Filename)>();"
	 *
	 * @note: not safe to use in TLookupMap.
	 */
	template<class CompareClass>
	void KeySort()
	{
		Sort<TPair, KeyComparisonClass<CompareClass> >( Pairs.GetTypedData(), Pairs.Num() );
		Rehash();
	}


	/**
	 * Sorts the pairs array using each pair's Value as the sort criteria, then rebuilds the map's hash.
	 *
	 * Invoked using "MyMapVar.ValueSort<COMPARE_CONSTREF_CLASS(ValueType,Filename)>();"
	 *
	 * @note: not safe to use in TLookupMap.
	 */
	template<class CompareClass>
	void ValueSort()
	{
		Sort<TPair, ValueComparisonClass<CompareClass> >( Pairs.GetTypedData(), Pairs.Num() );
		Rehash();
	}
};


template< class TK, class TI > class TMap : public TMapBase<TK,TI>
{
public:
	TMap& operator=( const TMap& Other )
	{
		TMapBase<TK,TI>::operator=( Other );
		return *this;
	}

	int Num() const
	{
		return this->Pairs.Num();
	}

	/** removes the pair with the specified key and copies the value that was removed to the ref parameter
	 * @param Key the key to search for
	 * @param RemovedValue if found, the value that was removed (not modified if the key was not found)
	 * @return whether or not the key was found
	 */
	UBOOL RemoveAndCopyValue(const TK& Key, TI& Value)
	{
		if( this->Hash != NULL && this->Pairs.Num() > 0 )
		{
			for( INT i = this->Hash[(GetTypeHash(Key) & (this->HashCount - 1))]; i != INDEX_NONE; i = this->Pairs(i).HashNext )
			{
				if( this->Pairs(i).Key == Key )
				{
					Value = this->Pairs(i).Value;
					this->Pairs.Remove(i);
					this->Relax();
					return TRUE;
				}
			}
		}
		return FALSE;
	}
};

template< class TK, class TI > class TMultiMap : public TMapBase<TK,TI>
{
public:
	TMultiMap& operator=( const TMultiMap& Other )
	{
		TMapBase<TK,TI>::operator=( Other );
		return *this;
	}
	void MultiFind( const TK& Key, TArray<TI>& Values ) const
	{
		if( this->Hash && this->Pairs.Num() > 0 )
		{
			for( INT i=this->Hash[(GetTypeHash(Key) & (this->HashCount-1))]; i!=INDEX_NONE; i=this->Pairs(i).HashNext )
			{
				if( this->Pairs(i).Key==Key )
				{
					new(Values)TI(this->Pairs(i).Value);
				}
			}
		}
	}
	TI& Add( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		this->AllocHash();
		return TMapBase<TK,TI>::Add( InKey, InValue );
	}
	TI& AddUnique( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		this->AllocHash();
		if ( this->Pairs.Num() > 0 )
		{
			for( INT i=this->Hash[(GetTypeHash(InKey) & (this->HashCount-1))]; i!=INDEX_NONE; i=this->Pairs(i).HashNext )
			{
				if( this->Pairs(i).Key==InKey && this->Pairs(i).Value==InValue )
				{
					return this->Pairs(i).Value;
				}
			}
		}
		return Add( InKey, InValue );
	}
	INT RemovePair( typename TTypeInfo<TK>::ConstInitType InKey, typename TTypeInfo<TI>::ConstInitType InValue )
	{
		INT Count=0;
		for( INT i=this->Pairs.Num()-1; i>=0; i-- )
			if( this->Pairs(i).Key==InKey && this->Pairs(i).Value==InValue )
				{this->Pairs.Remove(i); Count++;}
		if( Count )
			this->Relax();
		return Count;
	}

	/**
	 * Removes all pairs containing a specific key.
	 *
	 * Returns	The number of pairs that were removed.
	 */

	INT RemoveKey( typename TTypeInfo<TK>::ConstInitType InKey )
	{
		INT Count = 0;

		for( INT i = this->Pairs.Num()-1 ; i >= 0 ; i-- )
		{
			if( this->Pairs(i).Key == InKey )
			{
				this->Pairs.Remove(i);
				Count++;
			}
		}

		if( Count )
		{
			this->Relax();
		}

		return Count;
	}
	TI* FindPair( const TK& Key, const TI& Value )
	{
		if( this->Hash && this->Pairs.Num() > 0 )
		{
			for( INT i=this->Hash[(GetTypeHash(Key) & (this->HashCount-1))]; i!=INDEX_NONE; i=this->Pairs(i).HashNext )
			{
				if( this->Pairs(i).Key==Key && this->Pairs(i).Value==Value )
				{
					return &this->Pairs(i).Value;
				}
			}
		}
		return NULL;
	}

	/**
	 * Returns the number of pairs contained within this TMultiMap
	 */
	INT Num() const
	{
		return this->Pairs.Num();
	}

	/**
	 * Returns the number of items contained within this TMultiMap associated with the specified key
	 */
	INT Num( const TK& Key ) const
	{
		INT KeyCount = 0;
		if( this->Hash && this->Pairs.Num() > 0 )
		{
			for( INT i=this->Hash[(GetTypeHash(Key) & (this->HashCount-1))]; i!=INDEX_NONE; i=this->Pairs(i).HashNext )
			{
				if( this->Pairs(i).Key==Key )
				{
					KeyCount++;
				}
			}
		}

		return KeyCount;
	}

	/**
	 * Returns the number of unique keys contained within this TMultiMap
	 *
	 * @param	out_Keys	will be filled in with the keys contained by this TMultiMap
	 */
	INT Num( TArray<TK>& out_Keys ) const
	{
		for ( INT PairIndex = 0; PairIndex < this->Pairs.Num(); PairIndex++ )
		{
			if ( !out_Keys.ContainsItem(this->Pairs(PairIndex).Key) )
			{
				INT InsertIndex = out_Keys.AddZeroed();
				out_Keys(InsertIndex) = this->Pairs(PairIndex).Key;
			}
		}

		return out_Keys.Num();
	}
};

/**
 * A mapping from keys to values.  Like TMap, but optimized for dynamically
 * adding and removing values at the cost of a little additional memory
 * overhead.
 */
template<class KeyType,class ValueType> class TDynamicMap
{
protected:
	struct FPair
	{
		KeyType		Key;
		ValueType	Value;
		UBOOL		Free;
		FPair*		HashNext;

		FPair(typename TTypeInfo<KeyType>::ConstInitType InKey,typename TTypeInfo<ValueType>::ConstInitType InValue):
			Key(InKey),
			Value(InValue),
			Free(FALSE),
			HashNext(NULL)
		{}
		FPair():
		HashNext(NULL)
		{}
		friend FArchive& operator<<(FArchive& Ar,FPair& Pair)
		{
			return Ar << Pair.Key << Pair.Value;
		}
	};
	FPair**					Hash;
	INT						HashSize;
	INT						Count;
	TIndirectArray<FPair>	Pairs;
	FPair*					FreePairs;
	UBOOL AllocHash( UBOOL bRehashPairs=FALSE )
	{
		INT	NewHashSize = 1 << appCeilLogTwo(Pairs.Num() / 8 + 8);
		if(NewHashSize != HashSize || !Hash || bRehashPairs)
		{
			Count = 0;
	
			delete [] Hash;
			HashSize = NewHashSize;
			Hash = new FPair*[HashSize];
				
			appMemzero(Hash,sizeof(FPair*) * HashSize);

			for(INT PairIndex = 0;PairIndex < Pairs.Num();PairIndex++)
			{
				FPair& Pair = Pairs(PairIndex);
				if(!Pair.Free)
				{
					INT	HashIndex = (GetTypeHash(Pair.Key) & (HashSize - 1));
					Pair.HashNext = Hash[HashIndex];
					Hash[HashIndex] = &Pair;
					Count++;
				}
			}

			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	ValueType& Add(typename TTypeInfo<KeyType>::ConstInitType InKey,typename TTypeInfo<ValueType>::ConstInitType InValue)
	{
		FPair*	Pair;
		if(FreePairs)
		{
			Pair = FreePairs;
			FreePairs = FreePairs->HashNext;
			Pair->Free = 0;
			Pair->Key = InKey;
			Pair->Value = InValue;
		}
		else
		{
			Pair = new(Pairs)FPair(InKey,InValue);
		}

		if(!AllocHash())
		{
			INT	HashIndex = GetTypeHash(Pair->Key) & (HashSize - 1);
			Pair->HashNext = Hash[HashIndex];
			Hash[HashIndex] = Pair;
			Count++;
		}

		return Pair->Value;
	}
	void RemoveIndex(INT PairIndex)
	{
		if(Hash)
		{
			FPair**	PrevHashNext = &Hash[GetTypeHash(Pairs(PairIndex).Key) & (HashSize - 1)];
			for(FPair* HashPair = *PrevHashNext;HashPair;PrevHashNext = &HashPair->HashNext,HashPair = HashPair->HashNext)
			{
				if(HashPair->Key == Pairs(PairIndex).Key)
				{
					*PrevHashNext = HashPair->HashNext;
					HashPair->HashNext = FreePairs;
					FreePairs = HashPair;
					HashPair->Free = 1;
					HashPair->Value = ValueType();
					Count--;
					break;
				}
			}
		}
	}
public:
	TDynamicMap():
		Hash(NULL),
		HashSize(0),
		Count(0),
		FreePairs(NULL)
	{}
	~TDynamicMap()
	{
		delete [] Hash;
		Hash = NULL;
	}
	INT Num()
	{
		return Count;
	}
	void Shrink()
	{
		FreePairs = NULL;
		for(INT PairIndex = Pairs.Num() - 1;PairIndex >= 0;PairIndex--)
		{
			if(Pairs(PairIndex).Free)
			{
				Pairs.Remove(PairIndex);
			}
		}
	}
	void Empty( INT Slack=0 )
	{
		Pairs.Empty( Slack );
		FreePairs = NULL;
		HashSize = 0;
		delete [] Hash;
		Hash = NULL;
		Count = 0;
	}
	/** 
	* Helper function to return the amount of memory allocated by this container 
	*
	* @return number of bytes allocated by this container
	*/
	DWORD GetAllocatedSize( void ) const
	{
		return( Pairs.GetAllocatedSize() + ( HashSize * sizeof( FPair* ) ) );
	}
	void CountBytes( FArchive& Ar )
	{
		Pairs.CountBytes( Ar );
	}
	ValueType& Set(typename TTypeInfo<KeyType>::ConstInitType InKey,typename TTypeInfo<ValueType>::ConstInitType InValue)
	{
		AllocHash();

		for(FPair* HashPair = Hash[GetTypeHash(InKey) & (HashSize - 1)];HashPair;HashPair = HashPair->HashNext)
		{
			if(HashPair->Key == InKey)
			{
				HashPair->Value = InValue;
				return HashPair->Value;
			}
		}
		return Add(InKey,InValue);
	}
	void Remove(typename TTypeInfo<KeyType>::ConstInitType InKey)
	{
		if(Hash)
		{
			FPair**	PrevHashNext = &Hash[GetTypeHash(InKey) & (HashSize - 1)];
			for(FPair* HashPair = *PrevHashNext;HashPair;PrevHashNext = &HashPair->HashNext,HashPair = HashPair->HashNext)
			{
				if(HashPair->Key == InKey)
				{
					*PrevHashNext = HashPair->HashNext;
					HashPair->HashNext = FreePairs;
					FreePairs = HashPair;
					HashPair->Free = 1;
					HashPair->Value = ValueType();
					Count--;
					break;
				}
			}
		}
	}
	const KeyType* FindKey(const ValueType& Value) const
	{
		for(INT PairIndex = 0;PairIndex < Pairs.Num();PairIndex++)
		{
			if(Pairs(PairIndex).Value == Value)
			{
				return &Pairs(PairIndex).Key;
			}
		}
		return NULL;
	}
	ValueType FindRef(const KeyType& InKey) const
	{
		if(Hash)
		{
			for(FPair* HashPair = Hash[GetTypeHash(InKey) & (HashSize - 1)];HashPair;HashPair = HashPair->HashNext)
			{
				if(HashPair->Key == InKey)
				{
					return HashPair->Value;
				}
			}
		}
		return (ValueType) NULL;
	}
	ValueType* Find(const KeyType& InKey)
	{
		if(Hash)
		{
			for(FPair* HashPair = Hash[GetTypeHash(InKey) & (HashSize - 1)];HashPair;HashPair = HashPair->HashNext)
			{
				if(HashPair->Key == InKey)
				{
					return &HashPair->Value;
				}
			}
		}
		return NULL;
	}
	const ValueType* Find(const KeyType& InKey) const
	{
		if(Hash)
		{
			for(const FPair* HashPair = Hash[GetTypeHash(InKey) & (HashSize - 1)];HashPair;HashPair = HashPair->HashNext)
			{
				if(HashPair->Key == InKey)
				{
					return &HashPair->Value;
				}
			}
		}
		return NULL;
	}
	friend FArchive& operator<<(FArchive& Ar,TDynamicMap& M)
	{
		if(Ar.IsSaving())
		{
			M.Shrink();
		}
		Ar << M.Pairs;
		if( Ar.IsLoading() )
		{
			if( M.Pairs.Num() )
			{
				M.AllocHash();
			}
			else
			{
				delete [] M.Hash;
				M.Hash = NULL;
			}
		}
		return Ar;
	}
	void Dump( FOutputDevice& Ar ) const
	{
		Ar.Logf(TEXT("TDynamicMap: %i items, %i hash slots"),Pairs.Num(),HashSize);
		for(INT HashIndex = 0;HashIndex < HashSize;HashIndex++)
		{
			INT	Depth = 0;
			for(const FPair* HashPair = Hash[HashIndex];HashPair;HashPair = HashPair->HashNext)
			{
				Depth++;
			}
			Ar.Logf(TEXT("\tHash[%i] = %i"),HashIndex,Depth);
		}
	}
	class TIterator
	{
	public:
		TIterator(TDynamicMap& InMap):
			Map(InMap),
			Pairs(InMap.Pairs),
			Index(-1)
		{
			++(*this);
		}
		void operator++()
		{
			do Index++; while(Index < Pairs.Num() && Pairs(Index).Free);
		}
		void RemoveCurrent()
		{
			Map.RemoveIndex(Index--);
		}
		operator UBOOL() const     { return Pairs.IsValidIndex(Index); }
		const KeyType& Key() const { return Pairs(Index).Key; }
		ValueType& Value() const   { return Pairs(Index).Value; }
	private:
		TDynamicMap& Map;
		TIndirectArray<FPair>& Pairs;
		INT Index;
		INT Removed;
	};
	class TConstIterator
	{
	public:
		TConstIterator(const TDynamicMap& InMap):
			Map(InMap),
			Pairs(InMap.Pairs),
			Index(-1)
		{
			++(*this);
		}
		void operator++()
		{
			do Index++; while(Index < Pairs.Num() && Pairs(Index).Free);
		}
		operator UBOOL() const     { return Pairs.IsValidIndex(Index); }
		const KeyType& Key() const       { return Pairs(Index).Key; }
		const ValueType& Value() const   { return Pairs(Index).Value; }
	private:
		const TDynamicMap& Map;
		const TIndirectArray<FPair>& Pairs;
		INT Index;
	};
	friend class TIterator;
	friend class TConstIterator;


	/**
	 * Helper class for sorting the pairs using each pair's Key; forwards calls to Compare to the class provided by the CompareClass parameter.
	 */
	template<class CompareClass>
	class KeyComparisonClass
	{
	public:
		static INT Compare(const FPair* A, const FPair* B)
		{
			if ( A->Free )
			{
				return 1;
			}
			else if ( B->Free )
			{
				return -1;
			}
			return CompareClass::Compare(A->Key, B->Key);
		}
	};


	/**
	 * Helper class for sorting the pairs array using each pair's Value; forwards calls to Compare to the class provided by the CompareClass parameter.
	 */
	template<class CompareClass>
	class ValueComparisonClass
	{
	public:
		static INT Compare( const FPair* A, const FPair* B )
		{
			if ( A->Free )
			{
				return 1;
			}
			else if ( B->Free )
			{
				return -1;
			}
			return CompareClass::Compare(A->Value, B->Value);
		}
	};

	/**
	 * Sorts the pairs array using each pair's Key as the sort criteria, then rebuilds the map's hash.
	 *
	 * Invoked using "MyMapVar.KeySort<COMPARE_CONSTREF_CLASS(KeyType,UniqueSortClassName)>();"
	 */
	template<class CompareClass>
	void KeySort()
	{
		Sort<FPair*, KeyComparisonClass<CompareClass> >( Pairs.GetTypedData(), Pairs.Num() );
		AllocHash(TRUE);
	}


	/**
	 * Sorts the pairs array using each pair's Value as the sort criteria, then rebuilds the map's hash.
	 *
	 * Invoked using "MyMapVar.ValueSort<COMPARE_CONSTREF_CLASS(ValueType,UniqueSortClassName)>();"
	 */
	template<class CompareClass>
	void ValueSort()
	{
		Sort<FPair*, ValueComparisonClass<CompareClass> >( Pairs.GetTypedData(), Pairs.Num() );
		AllocHash(TRUE);
	}
};

/**
 * This specialized TMap class is designed to be used in place of a TArray in cases where fast lookup is required. It presents a minimal
 * TArray-style interface and provides access to the indices of the underlying Pairs array in a protected fashion, so that an element (key) can
 * be easily retrieved by providing an index.
 *
 * The value for an element in this map is the index into the Pairs array for the corresponding key.
 */
template< class T >
class TLookupMap : public TMapBase<T, INT>
{
protected:

	/**
	 * Makes sure that the Value member for each element in the Pairs array is the index of that
	 * element in the Pairs array.
	 *
	 * @param	StartIndex	the index to begin checking.  If not specified, synchronizes the entire array.
	 */
	void SynchronizeIndexValues( INT StartIndex=0 )
	{
		for ( INT PairIndex = StartIndex; PairIndex < this->Pairs.Num(); PairIndex++ )
		{
			this->Pairs(PairIndex).Value = PairIndex;
		}
	}

public:
	/** Copy constructor */
	TLookupMap& operator=( const TLookupMap& Other )
	{
		TMapBase<T,INT>::operator=( Other );
		return *this;
	}

	/**
	 * Returns the number of elements contained in this map.
	 */
	INT Num() const
	{
		return this->Pairs.Num();
	}


	/**
	 * Inserts a new element into the map at a specific index.  If the element already exists,
	 * do not add it again.
	 *
	 * @param	InKey				the element to add.
	 * @param	InsertIndex			the index to insert the new element at.  If not specified, the element
	 *								is appended to the end of the array.
	 * @param	bAllowDuplicates	specify TRUE to allow the element to be inserted even if that element
	 *								already exists at a different location in the array.
	 *
	 * @return	the index [into the Pairs array] where the element was inserted, or the location
	 *			of any previously existing values.
	 */
	void InsertItem( typename TTypeInfo<T>::ConstInitType InKey, INT InsertIndex=INDEX_NONE, UBOOL bAllowDuplicates=FALSE )
	{
		if ( bAllowDuplicates || Find(InKey) == NULL )
		{
			// doesn't exist; add to the map
			this->AllocHash();

			// ensure that we have a valid insertion point
			if ( !this->Pairs.IsValidIndex(InsertIndex) )
			{
				InsertIndex = this->Pairs.Num();
			}

			// insert into the list
			this->Pairs.InsertZeroed(InsertIndex);
			this->Pairs(InsertIndex).Key = InKey;
			this->Pairs(InsertIndex).Value = InsertIndex;

			// all pairs which occur in the array AFTER the one we just inserted will now have
			// the wrong value (which is the index of its key into the array), so update those now
			this->SynchronizeIndexValues(InsertIndex + 1);

			if( this->HashCount*2 + 8 < this->Pairs.Num() )
			{
				this->HashCount *= 2;
			}
			this->Rehash();
		}
	}

	/**
	 * Adds a new element to the map.  If the element already exists, do not add it again.
	 *
	 * @param	InKey				the element to add.
	 * @param	bAllowDuplicates	specify TRUE to allow the element to be inserted even if that element
	 *								already exists at a different location in the array.
	 *
	 * @return	the index [into the Pairs array] where the element was inserted, or the location
	 *			of any previously existing values.
	 */
	INT AddItem( typename TTypeInfo<T>::ConstInitType InKey, UBOOL bAllowDuplicates=FALSE )
	{
		INT Result = INDEX_NONE;
		INT* CurrentIndex=NULL;
		if ( !bAllowDuplicates && (CurrentIndex = Find(InKey)) != NULL )
		{
			// already exists, just return the existing index
			Result = *CurrentIndex;
		}
		else
		{
			this->AllocHash();

			// doesn't exist; add to the map
			Result = this->Pairs.AddZeroed();
			this->Pairs(Result).Key = InKey;
			this->Pairs(Result).Value = Result;

			INT    iHash  = (GetTypeHash(this->Pairs(Result).Key) & (this->HashCount-1));
			this->Pairs(Result).HashNext = this->Hash[iHash];
			this->Hash[iHash]   = this->Pairs.Num()-1;
			if( this->HashCount*2 + 8 < this->Pairs.Num() )
			{
				this->HashCount *= 2;
				this->Rehash();
			}
		}

		return Result;
	}

	/**
	 * Removes an element at a specified index from the map.
	 *
	 * @param	IndexToRemove	the index of the element that should be removed
	 *
	 * @return	TRUE if the element was successfully removed.
	 */
	UBOOL Remove( INT IndexToRemove )
	{
		if ( this->Pairs.IsValidIndex(IndexToRemove) )
		{
			this->Pairs.Remove(IndexToRemove);

			// after removal of the element, all pairs which occur in the array AFTER the
			// the element removed no longer have the correct value for the Pair.Value
			this->SynchronizeIndexValues(IndexToRemove);
			this->Relax();
			return TRUE;
		}

		return FALSE;
	}

	/**
	 * Removes an element from the map.
	 *
	 * @param	InKey	the element that should be removed
	 *
	 * @return	the number of elements that were removed.
	 */
	INT RemoveItem( typename TTypeInfo<T>::ConstInitType InKey )
	{
		INT Count=0, LastIndexRemoved=this->Pairs.Num();

		for ( INT PairIndex = this->Pairs.Num() - 1; PairIndex >= 0; PairIndex-- )
		{
			if ( this->Pairs(PairIndex).Key == InKey )
			{
				this->Pairs.Remove(PairIndex);
				LastIndexRemoved = PairIndex;
				Count++;
			}
		}

		if( Count )
		{
			// if any elements were removed, then all pairs which occur in the array AFTER the
			// the lowest element removed no longer have the correct value for the Pair.Value
			this->SynchronizeIndexValues(LastIndexRemoved);
			this->Relax();
		}
		return Count;
	}

	/* === TArray interface === */
	/**
	 * Retrieve the element (key) at the specified index
	 */
	T& GetItem( INT PairIndex )
	{
		checkSlow(this->Pairs.IsValidIndex(PairIndex));
		return this->Pairs(PairIndex).Key;
	}
	const T& GetItem( INT PairIndex ) const
	{
		checkSlow(this->Pairs.IsValidIndex(PairIndex));
		return this->Pairs(PairIndex).Key;
	}
	T& operator()( INT PairIndex )
	{
		checkSlow(this->Pairs.IsValidIndex(PairIndex));
		return this->Pairs(PairIndex).Key;
	}
	const T& operator()( INT PairIndex ) const
	{
		checkSlow(this->Pairs.IsValidIndex(PairIndex));
		return this->Pairs(PairIndex).Key;
	}
	UBOOL IsValidIndex( INT PairIndex ) const
	{
		return this->Pairs.IsValidIndex(PairIndex);
	}

	/**
	 * Appends the specified array to this lookup map.  The TArray must be of the same type as this lookup map
	 */
	TLookupMap& operator+=( const TArray<T>& Other )
	{
		for ( INT Index = 0; Index < Other.Num(); Index++ )
		{
			AddItem(Other(Index));
		}
		return *this;
	}

};

/**
 * Encapsulates a link in a single linked list with constant access time.
 */
template<class ElementType> class TLinkedList
{
public:

	/**
	 * Used to iterate over the elements of a linked list.
	 */
	class TIterator
	{
	public:
		TIterator(TLinkedList* FirstLink)
		:	CurrentLink(FirstLink)
		{}

		/**
		 * Advances the iterator to the next element.
		 */
		void Next()
		{
			checkSlow(CurrentLink);
			CurrentLink = CurrentLink->NextLink;
		}

		/**
		 * Checks for the end of the list.
		 */
		operator UBOOL() const { return CurrentLink != NULL; }

		// Accessors.
		ElementType& operator->() const
		{
			checkSlow(CurrentLink);
			return CurrentLink->Element;
		}
		ElementType& operator*() const
		{
			checkSlow(CurrentLink);
			return CurrentLink->Element;
		}

	private:
		TLinkedList* CurrentLink;
	};

	// Constructors.
	TLinkedList():
		NextLink(NULL),
		PrevLink(NULL)
	{}
	TLinkedList(const ElementType& InElement):
		Element(InElement),
		NextLink(NULL),
		PrevLink(NULL)
	{}

	/**
	 * Removes this element from the list in constant time.
	 *
	 * This function is safe to call even if the element is not linked.
	 */
	void Unlink()
	{
		if( NextLink )
		{
			NextLink->PrevLink = PrevLink;
		}
		if( PrevLink )
		{
			*PrevLink = NextLink;
		}
		// Make it safe to call Unlink again.
		NextLink = NULL;
		PrevLink = NULL;
	}

	/**
	 * Adds this element to a list, before the given element.
	 *
	 * @param Before	The link to insert this element before.
	 */
	void Link(TLinkedList*& Before)
	{
		if(Before)
		{
			Before->PrevLink = &NextLink;
		}
		NextLink = Before;
		PrevLink = &Before;
		Before = this;
	}

	/**
	 * Returns whether element is currently linked.
	 *
	 * @return TRUE if currently linked, FALSE othwerise
	 */
	UBOOL IsLinked()
	{
		return PrevLink != NULL;
	}

	// Accessors.
	ElementType& operator->()
	{
		return Element;
	}
	const ElementType& operator->() const
	{
		return Element;
	}
	ElementType& operator*()
	{
		return Element;
	}
	const ElementType& operator*() const
	{
		return Element;
	}
	TLinkedList* Next()
	{
		return NextLink;
	}
private:
	ElementType Element;
	TLinkedList* NextLink;
	TLinkedList** PrevLink;
};

/**
 * Double linked list.
 */
template<class ElementType> class TDoubleLinkedList
{
public:

	class TDoubleLinkedListNode
	{
	public:
		friend class TDoubleLinkedList;
		friend class TIterator;

		/** Constructor */
		TDoubleLinkedListNode( const ElementType& InValue )
		: Value(InValue), NextNode(NULL), PrevNode(NULL)
		{
		}

		const ElementType& GetValue() const
		{
			return Value;
		}

		ElementType& GetValue()
		{
			return Value;
		}

#ifdef __GNUG__
		//@fixme - gcc compile error
		// gcc doesn't seem to like the forward friend declaration of the TIterator class, so these are necessary until I find out 
		// the correct gcc syntax for doing this....meanwhile, I'll enforce the intent of this class's access using the ms compiler
		TDoubleLinkedListNode* GetNext() { return NextNode; }
		TDoubleLinkedListNode* GetPrev() { return PrevNode; }
#endif

	protected:
		ElementType				Value;
		TDoubleLinkedListNode*	NextNode;
		TDoubleLinkedListNode*	PrevNode;
	};

	/**
	 * Used to iterate over the elements of a linked list.
	 */
	class TIterator
	{
	public:

		TIterator(TDoubleLinkedListNode* StartingNode)
		: CurrentNode(StartingNode)
		{}

		/**
		 * Checks for the end of the list.
		 */
		operator UBOOL() const { return CurrentNode != NULL; }

		void operator++()
		{
			checkSlow(CurrentNode);
#ifdef __GNUG__
			CurrentNode = CurrentNode->GetNext();
#else
			CurrentNode = CurrentNode->NextNode;
#endif
		}

		void operator--()
		{
			checkSlow(CurrentNode);
#ifdef __GNUG__
			CurrentNode = CurrentNode->GetPrev();
#else
			CurrentNode = CurrentNode->PrevNode;
#endif
		}

		// Accessors.
		ElementType& operator->() const
		{
			checkSlow(CurrentNode);
			return CurrentNode->GetValue();
		}
		ElementType& operator*() const
		{
			checkSlow(CurrentNode);
			return CurrentNode->GetValue();
		}
		TDoubleLinkedListNode* GetNode() const
		{
			checkSlow(CurrentNode);
			return CurrentNode;
		}

	private:

		TDoubleLinkedListNode*	CurrentNode;
	};

	/** Constructors. */
	TDoubleLinkedList() : HeadNode(NULL), TailNode(NULL), ListSize(0)
	{}

	/** Destructor */
	virtual ~TDoubleLinkedList()
	{
		Clear();
	}

	// Adding/Removing methods
	/**
	 * Add the specified value to the beginning of the list, making that value the new head of the list.
	 *
	 * @param	InElement	the value to add to the list.
	 *
	 * @return	whether the node was successfully added into the list
	 */
	UBOOL AddHead( const ElementType& InElement )
	{
		TDoubleLinkedListNode* NewNode = new TDoubleLinkedListNode(InElement);
		if ( NewNode == NULL )
		{
			return FALSE;
		}

		// have an existing head node - change the head node to point to this one
		if ( HeadNode != NULL )
		{
			NewNode->NextNode = HeadNode;
			HeadNode->PrevNode = NewNode;
			HeadNode = NewNode;
		}
		else
		{
			HeadNode = TailNode = NewNode;
		}

		SetListSize(ListSize + 1);
		return TRUE;
	}

	/**
	 * Append the specified value to the end of the list
	 *
	 * @param	InElement	the value to add to the list.
	 *
	 * @return	whether the node was successfully added into the list
	 */
	UBOOL AddTail( const ElementType& InElement )
	{
		TDoubleLinkedListNode* NewNode = new TDoubleLinkedListNode(InElement);
		if ( NewNode == NULL )
		{
			return FALSE;
		}

		if ( TailNode != NULL )
		{
			TailNode->NextNode = NewNode;
			NewNode->PrevNode = TailNode;
			TailNode = NewNode;
		}
		else
		{
			HeadNode = TailNode = NewNode;
		}

		SetListSize(ListSize + 1);
		return TRUE;
	}

	/**
	 * Insert the specified value into the list at an arbitrary point.
	 *
	 * @param	InElement			the value to insert into the list
	 * @param	NodeToInsertBefore	the new node will be inserted into the list at the current location of this node
	 *								if NULL, the new node will become the new head of the list
	 *
	 * @return	whether the node was successfully added into the list
	 */
	UBOOL InsertNode( const ElementType& InElement, TDoubleLinkedListNode* NodeToInsertBefore=NULL )
	{
		if ( NodeToInsertBefore == NULL || NodeToInsertBefore == HeadNode )
		{
			return AddHead(InElement);
		}

		TDoubleLinkedListNode* NewNode = new TDoubleLinkedListNode(InElement);
		if ( NewNode == NULL )
		{
			return FALSE;
		}

		NewNode->PrevNode = NodeToInsertBefore->PrevNode;
		NewNode->NextNode = NodeToInsertBefore;

		NodeToInsertBefore->PrevNode->NextNode = NewNode;
		NodeToInsertBefore->PrevNode = NewNode;

		SetListSize(ListSize + 1);
		return TRUE;
	}

	/**
	 * Remove the node corresponding to InElement
	 *
	 * @param	InElement	the value to remove from the list
	 */
	void RemoveNode( const ElementType& InElement )
	{
		TDoubleLinkedListNode* ExistingNode = FindNode(InElement);
		RemoveNode(ExistingNode);
	}

	/**
	 * Removes the node specified.
	 */
	void RemoveNode( TDoubleLinkedListNode* NodeToRemove )
	{
		if ( NodeToRemove != NULL )
		{
			// if we only have one node, just call Clear() so that we don't have to do lots of extra checks in the code below
			if ( Num() == 1 )
			{
				Clear();
				return;
			}

			if ( NodeToRemove == HeadNode )
			{
				HeadNode = HeadNode->NextNode;
				HeadNode->PrevNode = NULL;
			}

			else if ( NodeToRemove == TailNode )
			{
				TailNode = TailNode->PrevNode;
				TailNode->NextNode = NULL;
			}
			else
			{
				NodeToRemove->NextNode->PrevNode = NodeToRemove->PrevNode;
				NodeToRemove->PrevNode->NextNode = NodeToRemove->NextNode;
			}

			delete NodeToRemove;
			NodeToRemove = NULL;
			SetListSize(ListSize - 1);
		}
	}

	/**
	 * Removes all nodes from the list.
	 */
	void Clear()
	{
		TDoubleLinkedListNode* pNode;
		while ( HeadNode != NULL )
		{
			pNode = HeadNode->NextNode;
			delete HeadNode;
			HeadNode = pNode;
		}

		HeadNode = TailNode = NULL;
		SetListSize(0);
	}

	// Accessors.

	/**
	 * Returns the node at the head of the list
	 */
	TDoubleLinkedListNode* GetHead() const
	{
		return HeadNode;
	}

	/**
	 * Returns the node at the end of the list
	 */
	TDoubleLinkedListNode* GetTail() const
	{
		return TailNode;
	}

	/**
	 * Finds the node corresponding to the value specified
	 *
	 * @param	InElement	the value to find
	 *
	 * @return	a pointer to the node that contains the value specified, or NULL of the value couldn't be found
	 */
	TDoubleLinkedListNode* FindNode( const ElementType& InElement )
	{
		TDoubleLinkedListNode* pNode = HeadNode;
		while ( pNode != NULL )
		{
			if ( pNode->GetValue() == InElement )
			{
				break;
			}

			pNode = pNode->NextNode;
		}

		return pNode;
	}

	/**
	 * Returns the size of the list.
	 */
	INT Num()
	{
		return ListSize;
	}

protected:

	/**
	 * Updates the size reported by Num().  Child classes can use this function to conveniently
	 * hook into list additions/removals.
	 *
	 * @param	NewListSize		the new size for this list
	 */
	virtual void SetListSize( INT NewListSize )
	{
		ListSize = NewListSize;
	}

private:
	TDoubleLinkedListNode* HeadNode;
	TDoubleLinkedListNode* TailNode;
	INT ListSize;
};

/*----------------------------------------------------------------------------
	TList.
----------------------------------------------------------------------------*/

//
// Simple single-linked list template.
//
template <class ElementType> class TList
{
public:

	ElementType			Element;
	TList<ElementType>*	Next;

	// Constructor.

	TList(const ElementType &InElement, TList<ElementType>* InNext = NULL)
	{
		Element = InElement;
		Next = InNext;
	}
};


/**
 * A statically allocated array with a dynamic number of elements.
 */
template<typename Type,UINT MaxElements>
class TStaticArray
{
public:
	TStaticArray(): NumElements(0) {}

	UINT AddUniqueItem(const Type& Item)
	{
		for(UINT Index = 0;Index < Num();Index++)
		{
			if(Elements[Index] == Item)
			{
				return Index;
			}
		}
		return AddItem(Item);
	}

	UINT AddItem(const Type& Item)
	{
		//checkSlow(NumElements < MaxElements);
		Elements[NumElements] = Item;
		return NumElements++;
	}

	void Remove(UINT BaseIndex,UINT Count = 1)
	{
		checkSlow(Count >= 0);
		checkSlow(BaseIndex >= 0); 
		checkSlow(BaseIndex <= NumElements);
		checkSlow(BaseIndex + Count <= NumElements);

		// Destruct the elements being removed.
		if(TTypeInfo<Type>::NeedsDestructor)
		{
			for(UINT Index = BaseIndex;Index < BaseIndex + Count;Index++)
			{
				(&Elements[Index])->~Type();
			}
		}

		// Move the elements which have a greater index than the removed elements into the gap.
		appMemmove
		(
			(BYTE*)Elements + (BaseIndex      ) * sizeof(Type),
			(BYTE*)Elements + (BaseIndex+Count) * sizeof(Type),
			(NumElements - BaseIndex - Count ) * sizeof(Type)
		);
		NumElements -= Count;

		checkSlow(NumElements >= 0);
		checkSlow(MaxElements >= NumElements);
	}

	void Empty()
	{
		NumElements = 0;
	}

	Type& operator()(UINT Index)
	{
		//checkSlow(Index < NumElements);
		return Elements[Index];
	}

	const Type& operator()(UINT Index) const
	{
		//checkSlow(Index < NumElements);
		return Elements[Index];
	}

	UINT Num() const
	{
		return NumElements;
	}
	/** 
	* Helper function to return the amount of memory allocated by this container 
	*
	* @return number of bytes allocated by this container
	*/
	DWORD GetAllocatedSize( void ) const
	{
		return( MaxElements * sizeof( Type ) );
	}
private:
	Type Elements[MaxElements];
	UINT NumElements;
};


/**
 * FScriptInterface
 *
 * This utility class stores the UProperty data for a native interface property.  ObjectPointer and InterfacePointer point to different locations in the same UObject.
 */
class FScriptInterface
{
private:
	/**
	 * A pointer to a UObject that implements a native interface.
	 */
	UObject*	ObjectPointer;

	/**
	 * Pointer to the location of the interface object within the UObject referenced by ObjectPointer.
	 */
	void*		InterfacePointer;

public:
	/**
	 * Default constructor
	 */
	FScriptInterface( UObject* InObjectPointer=NULL, void* InInterfacePointer=NULL )
	: ObjectPointer(InObjectPointer), InterfacePointer(InInterfacePointer)
	{}

	/**
	 * Returns the ObjectPointer contained by this FScriptInterface
	 */
	FORCEINLINE UObject* GetObject() const
	{
		return ObjectPointer;
	}

	/**
	 * Returns the ObjectPointer contained by this FScriptInterface
	 */
	FORCEINLINE UObject*& GetObjectRef()
	{
		return ObjectPointer;
	}

	/**
	 * Returns the pointer to the interface
	 */
	FORCEINLINE void* GetInterface() const
	{
		// only allow access to InterfacePointer if we have a valid ObjectPointer.  This is necessary because the garbage collector will set ObjectPointer to NULL
		// without using the accessor methods
		return ObjectPointer != NULL ? InterfacePointer : NULL;
	}

	/**
	 * Sets the value of the ObjectPointer for this FScriptInterface
	 */
	FORCEINLINE void SetObject( UObject* InObjectPointer )
	{
		ObjectPointer = InObjectPointer;
		if ( ObjectPointer == NULL )
		{
			SetInterface(NULL);
		}
	}

	/**
	 * Sets the value of the InterfacePointer for this FScriptInterface
	 */
	FORCEINLINE void SetInterface( void* InInterfacePointer )
	{
		InterfacePointer = InInterfacePointer;
	}

	/**
	 * Comparison operator, taking a reference to another FScriptInterface
	 */
	FORCEINLINE UBOOL operator==( const FScriptInterface& Other ) const
	{
		return GetInterface() == Other.GetInterface() && ObjectPointer == Other.GetObject();
	}
	FORCEINLINE UBOOL operator!=( const FScriptInterface& Other ) const
	{
		return GetInterface() != Other.GetInterface() || ObjectPointer != Other.GetObject();
	}
};

/**
 * Templated version of FScriptInterface, which provides accessors and operators for referencing the interface portion of a UObject that implements a native interface.
 */
template< class InterfaceType > class TScriptInterface : public FScriptInterface
{
public:
	/**
	 * Default constructor
	 */
	TScriptInterface() {}
	/**
	 * Standard constructor.
	 *
	 * @param	SourceObject	a pointer to a UObject that implements the InterfaceType native interface class.
	 */
	template <class UObjectType> TScriptInterface( UObjectType* SourceObject )
	{
		(*this) = SourceObject;
	}
	/**
	 * Copy constructor
	 */
	TScriptInterface( const TScriptInterface& Other )
	{
		SetObject(Other.GetObject());
		SetInterface(Other.GetInterface());
	}

	/**
	 * Assignment operator.
	 *
	 * @param	SourceObject	a pointer to a UObject that implements the InterfaceType native interface class.
	 */
	template<class UObjectType> InterfaceType& operator=( UObjectType* SourceObject )
	{
		SetObject(SourceObject);
		
		InterfaceType* SourceInterface = SourceObject;
		SetInterface( SourceInterface );

		return *((InterfaceType*)GetInterface());
	}

	/**
	 * Comparison operator, taking a pointer to InterfaceType
	 */
	FORCEINLINE UBOOL operator==( const InterfaceType* Other ) const
	{
		return GetInterface() == Other;
	}
	FORCEINLINE UBOOL operator!=( const InterfaceType* Other ) const
	{
		return GetInterface() != Other;
	}

	/**
	 * Comparison operator, taking a reference to another TScriptInterface
	 */
	FORCEINLINE UBOOL operator==( const TScriptInterface& Other ) const
	{
		return GetInterface() == Other.GetInterface() && GetObject() == Other.GetObject();
	}
	FORCEINLINE UBOOL operator!=( const TScriptInterface& Other ) const
	{
		return GetInterface() != Other.GetInterface() || GetObject() != Other.GetObject();
	}

	/**
	 * Member access operator.  Provides transparent access to the interface pointer contained by this TScriptInterface
	 */
	FORCEINLINE InterfaceType* operator->() const
	{
		return (InterfaceType*)GetInterface();
	}

	/**
	 * Dereference operator.  Provides transparent access to the interface pointer contained by this TScriptInterface
	 *
	 * @return	a reference (of type InterfaceType) to the object pointed to by InterfacePointer
	 */
	FORCEINLINE InterfaceType& operator*() const
	{
		return *((InterfaceType*)GetInterface());
	}

	/**
	 * Boolean operator.  Provides transparent access to the interface pointer contained by this TScriptInterface.
	 *
	 * @return	TRUE if InterfacePointer is non-NULL.
	 */
	FORCEINLINE operator UBOOL() const
	{
		return GetInterface() != NULL;
	}

	friend FArchive& operator<<( FArchive& Ar, TScriptInterface& Interface )
	{
		UObject* ObjectValue = Interface.GetObject();
		Ar << ObjectValue;
		Interface.SetObject(ObjectValue);
		return Ar;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
