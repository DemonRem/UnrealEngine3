//------------------------------------------------------------------------------
// Storage for a dynamic array.
//
// Owner: John Briggs
// 
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxArrayBase_H__
#define FxArrayBase_H__

#include "FxMemory.h"

namespace OC3Ent
{

namespace Face
{

/// A wrapper around a dynamically-allocated chunk of memory.
/// FxArrayBase is by FxArray and FxString to manage the memory for the data
/// storage in a convenient, exception-safe manner.  Licensees will generally
/// not need to use FxArrayBase directly.
template<typename T> class FxArrayBase
{
public:
	FxArrayBase(FxSize size=0)
		: _v( static_cast<T*>(size==0 ? NULL : FxAlloc(sizeof(T)*size, "FxArrayBase")) )
		, _allocatedCount(size)
		, _usedCount(0)
	{
	}

	~FxArrayBase()
	{
		FxDestroy(_v, _v + _usedCount);
		FxFree(_v, sizeof(T) * _allocatedCount);
		_v = NULL;
		_usedCount = 0;
		_allocatedCount = 0;
	}

	void Swap(FxArrayBase& other) throw()
	{
		FxSwap(_v, other._v);
		FxSwap(_allocatedCount, other._allocatedCount);
		FxSwap(_usedCount, other._usedCount);
	}

	T* _v;
	FxSize _allocatedCount;
	FxSize _usedCount;

private:
	// Undefined: no copying allowed.
	FxArrayBase(const FxArrayBase&);
	FxArrayBase& operator=(const FxArrayBase&);
};

} // namespace Face

} // namespace OC3Ent

#endif
