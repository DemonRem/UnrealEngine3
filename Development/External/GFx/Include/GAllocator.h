/**********************************************************************

Filename    :   GAllocator.h
Content     :   Template allocators, constructors and functions.
Created     :   2008
Authors     :   Michael Antonov, Maxim Shemanarev
Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GAllocator_H
#define INC_GAllocator_H

#include "GTypes.h"
#include "GDebug.h"
#include "GMemory.h"
#include "GMath.h"
#include <string.h>


// ***** Disable template-unfriendly MS VC++ warnings
#if defined(GFC_CC_MSVC)
// Pragma to prevent long name warnings in in VC++
#pragma warning(disable : 4503)
#pragma warning(disable : 4786)
// In MSVC 7.1, warning about placement new POD default initializer
#pragma warning(disable : 4345)
#endif


// ***** Add support for placement new

// Calls constructor on own memory created with "new(ptr) type"
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE

#if defined(GFC_CC_MWERKS) || defined(GFC_CC_BORLAND)
#include <new>
#else

#ifndef GFC_CC_GNU
GINLINE void*   operator new        (UPInt n, void *ptr)   { return ptr; }
GINLINE void    operator delete     (void *ptr, void *ptr2) { }
#else
#include <new>
#endif // GFC_CC_GNU

#endif // GFC_CC_MWERKS | GFC_CC_BORLAND

#endif // __PLACEMENT_NEW_INLINE


#undef new



// ***** Construct / Destruct

template <class T>
inline T*  G_Construct(void *p)
{
    return ::new(p) T;
}

template <class T>
inline T*  G_Construct(void *p, const T& source)
{
    return ::new(p) T(source);
}

// Same as above, but allows for a different type of constructor.
template <class T, class S>
inline T*  G_ConstructAlt(void *p, const S& source)
{
    return ::new(p) T(source);
}

template <class T>
GINLINE void G_ConstructArray(void *p, UPInt count)
{
    UByte *pdata = (UByte*)p;
    for (UPInt i=0; i< count; ++i, pdata += sizeof(T))
    {
        G_Construct<T>(pdata);
    }
}

template <class T>
GINLINE void G_ConstructArray(void *p, UPInt count, const T& source)
{
    UByte *pdata = (UByte*)p;
    for (UPInt i=0; i< count; ++i, pdata += sizeof(T))
    {
        G_Construct<T>(pdata, source);
    }
}

template <class T>
GINLINE void G_Destruct(T *pobj)
{
    pobj->~T();
}

template <class T>
GINLINE void G_DestructArray(T *pobj, UPInt count)
{   
    for (UPInt i=0; i<count; ++i, ++pobj)        
        pobj->~T();
}



// ***** Local Allocator

// Local allocator allocates memory for containers which are a part
// of other data structures. This allocator determines the allocation
// heap and StatId based on the argument address; due to heap limitation
// it can not be used for stack-allocated or global values.
//  - If SID is GStat_Default_Mem, the allocator also inherits the SID
//------------------------------------------------------------------------
template<int SID = GStat_Default_Mem>
class GAllocatorBaseLH
{
public:
    enum { StatId = SID };

#ifdef GFC_BUILD_DEBUG
    static void*   Alloc(const void* pheapAddr, UPInt size, const char* pfile, UInt line) 
    {         
        return GMemory::AllocAutoHeap(pheapAddr, size,
                                      GAllocDebugInfo(StatId, pfile, line));
    }
    static void*   Realloc(void* p, UPInt newSize, const char* pfile, UInt line) 
    { 
        GUNUSED2(pfile, line);
        return GMemory::Realloc(p, newSize);
    }

#else

    static void*   Alloc(const void* pheapAddr, UPInt size, const char*, UInt) 
    {         
        return GMemory::AllocAutoHeap(pheapAddr, size, GAllocDebugInfo(StatId, 0, 0));
    }
    static void*   Realloc(void* p, UPInt newSize, const char*, UInt) 
    { 
        return GMemory::Realloc(p, newSize);
    }

#endif

    static void Free(void *p) 
    { 
        GMemory::Free(p);
    }
};


// ***** Dynamic-Heap Allocator

// Dynamic-heap allocator allocates memory for containers created on 
// the stack, or anywhere else, where the heap pointer is explicitly
// specified. The difference is that GAllocatorBaseLH takes any 
// pointer inside the heap, while GAllocatorBaseDH requires a pointer
// to the heap itself.
//  - If SID is GStat_Default_Mem, the allocator also inherits the SID
//------------------------------------------------------------------------
template<int SID = GStat_Default_Mem>
class GAllocatorBaseDH
{
public:
    enum { StatId = SID };

#ifdef GFC_BUILD_DEBUG
    static void*   Alloc(const void* pheap, UPInt size, const char* pfile, UInt line) 
    {         
        return GMemory::AllocInHeap((GMemoryHeap*)pheap, size,
                                    GAllocDebugInfo(StatId, pfile, line));
    }
    static void*   Realloc(void* p, UPInt newSize, const char* pfile, UInt line) 
    { 
        GUNUSED2(pfile, line);
        return GMemory::Realloc(p, newSize);
    }

#else

    static void*   Alloc(const void* pheap, UPInt size, const char*, UInt) 
    {         
        return GMemory::AllocInHeap((GMemoryHeap*)pheap, 
                                    size, GAllocDebugInfo(StatId, 0, 0));
    }
    static void*   Realloc(void* p, UPInt newSize, const char*, UInt) 
    { 
        return GMemory::Realloc(p, newSize);
    }

#endif

    static void Free(void *p) 
    { 
        GMemory::Free(p);
    }
};


// ***** Global Allocator

// Allocator for items coming from GlobalHeap. StatId must be specified for useful
// memory tracking to be performed.
//------------------------------------------------------------------------
template<int SID = GStat_Default_Mem>
class GAllocatorBaseGH
{
public:
    enum { StatId = SID };

#ifdef GFC_BUILD_DEBUG
    static void*   Alloc(const void*, UPInt size, const char* pfile, UInt line) 
    {         
        return GMemory::Alloc(size, GAllocDebugInfo(StatId, pfile, line));
    }
    static void*   Realloc(void *p, UPInt newSize, const char* pfile, UInt line) 
    { 
        GUNUSED2(pfile, line);
        return GMemory::Realloc(p, newSize);
    }

#else

    static void*   Alloc(const void*, UPInt size, const char*, UInt) 
    {         
        return GMemory::Alloc(size, GAllocDebugInfo(StatId, 0, 0));
    }
    static void*   Realloc(void *p, UPInt newSize, const char*, UInt) 
    { 
        return GMemory::Realloc(p, newSize);
    }

#endif

    static void Free(void *p) 
    { 
        GMemory::Free(p);
    }
};


// ***** Constructors, Destructors, Copiers

// Plain Old Data
//------------------------------------------------------------------------
template<class T> 
class GConstructorPOD
{
public:
    static void Construct(void *) {}
    static void Construct(void *p, const T& source) 
    { 
        *(T*)p = source;
    }

    // Same as above, but allows for a different type of constructor.
    template <class S> 
    static void ConstructAlt(void *p, const S& source)
    {
        *(T*)p = source;
    }

    static void ConstructArray(void*, UPInt) {}

    static void ConstructArray(void* p, UPInt count, const T& source)
    {
        UByte *pdata = (UByte*)p;
        for (UPInt i=0; i< count; ++i, pdata += sizeof(T))
            *(T*)pdata = source;
    }

    static void ConstructArray(void* p, UPInt count, const T* psource)
    {
        memcpy(p, psource, sizeof(T) * count);
    }

    static void Destruct(T*) {}
    static void DestructArray(T*, UPInt) {}

    static void CopyArrayForward(T* dst, const T* src, UPInt count)
    {
        memmove(dst, src, count * sizeof(T));
    }

    static void CopyArrayBackward(T* dst, const T* src, UPInt count)
    {
        memmove(dst, src, count * sizeof(T));
    }

    static bool IsMovable() { return true; }
};


// ***** GConstructorMov
//
// Correct C++ construction and destruction for movable objects
//------------------------------------------------------------------------
template<class T> 
class GConstructorMov
{
public:
    static void Construct(void* p) 
    { 
        ::new(p) T;
    }

    static void Construct(void* p, const T& source) 
    { 
        ::new(p) T(source); 
    }

    // Same as above, but allows for a different type of constructor.
    template <class S> 
    static void ConstructAlt(void* p, const S& source)
    {
        ::new(p) T(source);
    }

    static void ConstructArray(void* p, UPInt count)
    {
        UByte* pdata = (UByte*)p;
        for (UPInt i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata);
    }

    static void ConstructArray(void* p, UPInt count, const T& source)
    {
        UByte* pdata = (UByte*)p;
        for (UPInt i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata, source);
    }

    static void ConstructArray(void* p, UPInt count, const T* psource)
    {
        UByte* pdata = (UByte*)p;
        for (UPInt i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata, *psource++);
    }

    static void Destruct(T* p)
    {
        p->~T();
        GUNUSED(p); // Suppress silly MSVC warning
    }

    static void DestructArray(T* p, UPInt count)
    {   
        p += count - 1;
        for (UPInt i=0; i<count; ++i, --p)
            p->~T();
    }

    static void CopyArrayForward(T* dst, const T* src, UPInt count)
    {
        memmove(dst, src, count * sizeof(T));
    }

    static void CopyArrayBackward(T* dst, const T* src, UPInt count)
    {
        memmove(dst, src, count * sizeof(T));
    }

    static bool IsMovable() { return true; }
};


// ***** GConstructorCPP
//
// Correct C++ construction and destruction for movable objects
//------------------------------------------------------------------------
template<class T> 
class GConstructorCPP
{
public:
    static void Construct(void* p) 
    { 
        ::new(p) T;
    }

    static void Construct(void* p, const T& source) 
    { 
        ::new(p) T(source); 
    }

    // Same as above, but allows for a different type of constructor.
    template <class S> 
    static void ConstructAlt(void* p, const S& source)
    {
        ::new(p) T(source);
    }

    static void ConstructArray(void* p, UPInt count)
    {
        UByte* pdata = (UByte*)p;
        for (UPInt i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata);
    }

    static void ConstructArray(void* p, UPInt count, const T& source)
    {
        UByte* pdata = (UByte*)p;
        for (UPInt i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata, source);
    }

    static void ConstructArray(void* p, UPInt count, const T* psource)
    {
        UByte* pdata = (UByte*)p;
        for (UPInt i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata, *psource++);
    }

    static void Destruct(T* p)
    {
        p->~T();
        GUNUSED(p); // Suppress silly MSVC warning
    }

    static void DestructArray(T* p, UPInt count)
    {   
        p += count - 1;
        for (UPInt i=0; i<count; ++i, --p)
            p->~T();
    }

    static void CopyArrayForward(T* dst, const T* src, UPInt count)
    {
        for(UPInt i = 0; i < count; ++i)
            dst[i] = src[i];
    }

    static void CopyArrayBackward(T* dst, const T* src, UPInt count)
    {
        for(UPInt i = count; i; --i)
            dst[i-1] = src[i-1];
    }

    static bool IsMovable() { return false; }
};



// ***** GAllocator*
//
// Simple wraps as specialized allocators
//------------------------------------------------------------------------
template<class T, int SID = GStat_Default_Mem> struct GAllocatorGH_POD : GAllocatorBaseGH<SID>, GConstructorPOD<T> {};
template<class T, int SID = GStat_Default_Mem> struct GAllocatorGH     : GAllocatorBaseGH<SID>, GConstructorMov<T> {};
template<class T, int SID = GStat_Default_Mem> struct GAllocatorGH_CPP : GAllocatorBaseGH<SID>, GConstructorCPP<T> {};

template<class T, int SID = GStat_Default_Mem> struct GAllocatorLH_POD : GAllocatorBaseLH<SID>, GConstructorPOD<T> {};
template<class T, int SID = GStat_Default_Mem> struct GAllocatorLH     : GAllocatorBaseLH<SID>, GConstructorMov<T> {};
template<class T, int SID = GStat_Default_Mem> struct GAllocatorLH_CPP : GAllocatorBaseLH<SID>, GConstructorCPP<T> {};

template<class T, int SID = GStat_Default_Mem> struct GAllocatorDH_POD : GAllocatorBaseDH<SID>, GConstructorPOD<T> {};
template<class T, int SID = GStat_Default_Mem> struct GAllocatorDH     : GAllocatorBaseDH<SID>, GConstructorMov<T> {};
template<class T, int SID = GStat_Default_Mem> struct GAllocatorDH_CPP : GAllocatorBaseDH<SID>, GConstructorCPP<T> {};

// Redefine operator 'new' if necessary.
#if defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif

#endif
