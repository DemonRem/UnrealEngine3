//------------------------------------------------------------------------------
// A templated dynamic array.
//
// Owner: John Briggs
// 
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxArray_H__
#define FxArray_H__

#include "FxMemory.h"
#include "FxTypeTraits.h"
#include "FxArchive.h"

namespace OC3Ent
{

namespace Face
{

// Forward declare the array base.
template<typename Elem> class FxArrayBase;

#define kCurrentFxArrayVersion 0

/// A templated dynamic array.
///
/// The array uses the array base as the underlying storage mechanism to help
/// provide exception safety and ensure proper cleanup.
///
/// No objects are ever constructed in the array until explicitly required.
/// \ingroup support
template<typename Elem> class FxArray
{
	typedef FxArrayBase<Elem> _container_type;
	typedef FxArray<Elem> _array_type;

public:
	typedef Elem value_type;
	typedef Elem* pointer;
	typedef const Elem* const_pointer;
	typedef pointer iterator;
	typedef const_pointer const_iterator;

	/// Construct an array with room for numReserved elements.
	/// \param numReserved The maximum length the array could attain without a
	/// reallocation.
	explicit FxArray(FxSize numReserved=0);
	/// Construct an array from a native array.
	/// \param pArray A pointer to an array of elements.
	/// \param numElements The number of elements in \a pArray.
	FxArray(const value_type* pArray, FxSize numElements);
	/// Copy constructor.
	/// \param other The array to copy.
	FxArray(const FxArray& other);
	/// Assignment operator.
	/// \param other The array to copy.
	FxArray& operator=(const FxArray& other);

	/// Clears the array and deallocates its memory.
	void Clear();

	/// Returns the number of elements in the array.
	FxSize Length() const;
	/// Returns the number of elements allocated.
	FxSize Allocated() const;
	/// Returns true if the array has no elements.
	FxBool IsEmpty() const;

	/// Element access.
	/// \param index The index of the element to access.
	/// \return A reference to the element at \a index.
	value_type& operator[](FxSize index);
	/// Const element access.
	/// \param index The index of the element to access.
	/// \return A const reference to the element at \a index.
	const value_type& operator[](FxSize index) const;

	/// Returns an iterator to the beginning of the array.
	iterator Begin();
	/// Returns a const iterator to the beginning of the array.
	const_iterator Begin() const;
	// Returns an iterator to the end of the array.
	iterator End();
	/// Returns a const iterator to the end of the array.
	const_iterator End() const;

	/// Ensures a minimum size for the array.
	/// \param n The maximum number of elements the array could hold without a 
	/// reallocation.
	void Reserve(FxSize n);

	/// Adds an element to the end of the array.
	/// \param element The element to append to the end of the array.
	void PushBack(const value_type& element);
	/// Removes the last element in the array.
	/// Does not return the last element.  Use Back() to read the last element
	/// before calling PopBack().
	void PopBack();

	/// Returns a reference to the last element in the array.
	value_type& Back();
	/// Returns a const reference to the last element in the array.
	const value_type& Back() const;
	/// Returns a reference to the first element in the array.
	value_type& Front();
	/// Returns a const reference to the first element in the array.
	const value_type& Front() const;

	/// Inserts an element at index.
	/// \param element The element to insert.
	/// \param index The desired index of \a element.
	void Insert(const value_type& element, FxSize index);
	/// Removes the element at index.
	/// \param index The index of the element to remove.
	void Remove(FxSize index);

	/// Finds the index of \a element, starting the search at \a startIndex.
	/// \param element The element for which to search.
	/// \param startIndex The index at which to start the search.
	/// \return The index of the element in the array equal to \a element.
	/// \note The type of element must define operator==.
	FxSize Find(const value_type& element, FxSize startIndex=0) const;

	/// Swaps the contents with an other array.
	void Swap(FxArray& other);

	/// Returns a pointer to the underlying C-style array.
	/// For compatibility with systems requiring a C-style array.  Returns a
	/// pointer to the first element in the array.  
	/// \note The user \em must \em not add or remove items in the array returned 
	/// by GetData() or the container will be in an invalid state.
	value_type* GetData() const;

	/// Swaps out the underlying array base.
	/// \note For internal use only.
	void SwapArrayBase(_container_type& other);

	/// Serializes the array to an archive.
	FxArchive& Serialize( FxArchive& arc )
	{
		FxUInt16 version = kCurrentFxArrayVersion;
		arc << version;

		FxSize length = Length();
		arc << length;

		if( arc.IsSaving() )
		{
			if( FxTypeTraits::is_arithmetic<Elem>::value )
			{
				// Shortcut saving for arithmetic types.
				arc.SerializePODArray(_container._v, length);
			}
			else
			{
				for( FxSize i = 0; i < length; ++i )
				{
					arc << operator[](i);
				}
			}
		}
		else
		{
			_container_type temp(length);
			if( FxTypeTraits::is_arithmetic<Elem>::value )
			{
				// Shortcut loading for arithmetic types
				arc.SerializePODArray(temp._v, length);
			}
			else
			{
				for( FxSize i = 0; i < length; ++i )
				{
					if( !FxTypeTraits::has_trivial_constructor<value_type>::value )
					{
						// We must create a valid object before we can serialize
						// into it.  Default-constructed FaceFX classes never
						// allocate memory, so this has little overhead.
						FxDefaultConstruct(temp._v + i);
					}
					arc << temp._v[i];
				}
			}
			temp._usedCount = length;
			_container.Swap(temp);
		}

		return arc;
	}

protected:
	// The actual container for the data in the array.
	_container_type _container;
};

/// Array equality.
/// Arrays are equal if their lengths are equal and each element is equal to 
/// the corresponding element in the other array.
/// \relates FxArray
template<typename Elem> FxBool operator==(const FxArray<Elem>& lhs, const FxArray<Elem>& rhs);
/// Array inequality.
/// \relates FxArray
template<typename Elem> FxBool operator!=(const FxArray<Elem>& lhs, const FxArray<Elem>& rhs);
/// Array serialization.
template<typename Elem> FxArchive& operator<<( FxArchive& arc, FxArray<Elem>& array );
} // namespace Face

} // namespace OC3Ent

// Include the implementation
#include "FxArrayImpl.h"

#endif
