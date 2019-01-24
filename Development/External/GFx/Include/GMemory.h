/**********************************************************************

Filename    :   GMemory.h
Content     :   General purpose memory allocation functions
Created     :   December 28, 1998 - 2009
Authors     :   Michael Antonov

Notes       :                

History     :   1/14/1999 MA    Revised file according to new format
                2/12/2002 MA    Major revision to utilize a buddy system
                2/05/2009 MA    Revised for pluginable memory heaps.

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GMemory_H
#define INC_GMemory_H

#include "GTypes.h"
#include "GMemoryHeap.h"

// Unified SysAlloc. New malloc-friendly allocator
#include "GSysAllocMalloc.h"
#define GSYSALLOC_DEFAULT_CLASS GSysAllocMalloc

// Include the right allocation header for the platform involved.
// This is ok even if users use custom allocators, as having the headers
// included here does NOT force the code to be linked.
/*
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX360) || defined(GFC_OS_WINCE)
    #include "GSysAllocWinAPI.h"
    #define GSYSALLOC_DEFAULT_CLASS GSysAllocWinAPI
#elif defined(GFC_OS_PS3)
    #include "GSysAllocPS3.h"
    #define GSYSALLOC_DEFAULT_CLASS GSysAllocPS3
#elif defined(GFC_OS_WII)
    #include "GSysAllocWii.h"
    #define GSYSALLOC_DEFAULT_CLASS GSysAllocWii
#else
    #include "GSysAllocPagedMalloc.h"
    #define GSYSALLOC_DEFAULT_CLASS GSysAllocPagedMalloc
#endif
*/


// Define this macro to enable memory tracking; since tracking is not free
// this variable probably should not be defined for a release build.
//#define   GFC_MEMORY_TRACKSIZES

// This file requires operator 'new' to NOT be defined; its definition
// is restored in the bottom of the header based on GFC_DEFINE_NEW.
#undef new


// **** Declared Classes
class GMemory;


// ***** Memory Allocation Macros

// These macros are used for allocation to ensure that the allocation location
// filename and line numbers are recorded in debug builds. The heap-based
// operator new versions are declared in a separate header file "GHeapNew.h".

#if !defined(GHEAP_DEBUG_INFO)

    // Allocate from global heap.
    #define GALLOC(s,id)                    GMemory::Alloc((s))
    #define GMEMALIGN(s,a,id)               GMemory::Alloc((s),(a))
    #define GREALLOC(p,s,id)                GMemory::Realloc((p),(s))
    #define GFREE(p)                        GMemory::Free((p))
    #define GFREE_ALIGN(s)                  GMemory::Free((s))
    // Heap versions.
    #define GHEAP_ALLOC(heap,s,id)          GMemory::AllocInHeap((heap),(s))
    #define GHEAP_MEMALIGN(heap,s,a,id)     GMemory::AllocInHeap((heap),(s),(a))
    #define GHEAP_AUTO_ALLOC(addr,s)        GMemory::AllocAutoHeap((addr),(s))
    #define GHEAP_AUTO_ALLOC_ID(addr,s,id)  GMemory::AllocAutoHeap((addr),(s))
    #define GHEAP_FREE(heap, p)             GMemory::Free((p))
#else
    #define GALLOC(s,id)                    GMemory::Alloc((s),     GAllocDebugInfo((id),__FILE__,__LINE__))
    #define GMEMALIGN(s,a,id)               GMemory::Alloc((s),(a), GAllocDebugInfo((id),__FILE__,__LINE__))
    #define GREALLOC(p,s,id)                GMemory::Realloc((p),(s))
    #define GFREE(p)                        GMemory::Free((p))
    #define GFREE_ALIGN(s)                  GMemory::Free((s))
    // Heap versions.
    #define GHEAP_ALLOC(heap,s,id)          GMemory::AllocInHeap((heap),(s),    GAllocDebugInfo((id),__FILE__,__LINE__))
    #define GHEAP_MEMALIGN(heap,s,a,id)     GMemory::AllocInHeap((heap),(s),(a),GAllocDebugInfo((id),__FILE__,__LINE__))
    #define GHEAP_AUTO_ALLOC(addr,s)        GMemory::AllocAutoHeap((addr),(s),  GAllocDebugInfo(GStat_Default_Mem,__FILE__,__LINE__))
    #define GHEAP_AUTO_ALLOC_ID(addr,s,id)  GMemory::AllocAutoHeap((addr),(s),  GAllocDebugInfo((id),__FILE__,__LINE__))
    #define GHEAP_FREE(heap, p)             GMemory::Free((p))

#endif //!defined(GFC_BUILD_DEBUG)



// ***** GMemory Class

// Maintains current heap and the global allocator, wrapping heap access functions.
// The main purpose of wrapping is to allow GAllocDebugInfo temporary to be converted
// from a constant reference to an optional pointer argument for the allocation
// functions in GMemoryHeap.

class GMemory
{
public:
    static GMemoryHeap *pGlobalHeap;


    // May need to export this later and check allocation status.
    static void                 GSTDCALL SetGlobalHeap(GMemoryHeap *heap)
    {
        pGlobalHeap = heap;
    }

    static inline GMemoryHeap*  GSTDCALL GetGlobalHeap()                { return pGlobalHeap; }

    // *** Operations with memory arenas
    //
    //--------------------------------------------------------------------
    static void GSTDCALL CreateArena(UPInt arena, GSysAllocPaged* sysAlloc)  { pGlobalHeap->CreateArena(arena, sysAlloc); }
    static void GSTDCALL DestroyArena(UPInt arena)                      { pGlobalHeap->DestroyArena(arena); }
    static bool GSTDCALL ArenaIsEmpty(UPInt arena)                      { return pGlobalHeap->ArenaIsEmpty(arena); }

    // *** Memory Allocation
    //
    // GMemory::Alloc of size==0 will allocate a tiny block & return a valid pointer;
    // this makes it suitable for new operator.
    static void* GSTDCALL Alloc(UPInt size)                          { return pGlobalHeap->Alloc(size); }

    static void* GSTDCALL Alloc(UPInt size, UPInt align)             { return pGlobalHeap->Alloc(size, align); }

    static void* GSTDCALL Alloc(UPInt size, 
                       const GAllocDebugInfo& info)         { return pGlobalHeap->Alloc(size, &info); }

    static void* GSTDCALL Alloc(UPInt size, UPInt align,
                       const GAllocDebugInfo& info)         { return pGlobalHeap->Alloc(size, align, &info); }

    // Allocate while automatically identifying heap and allocation id based on the specified address.
    static void* GSTDCALL AllocAutoHeap(const void *p, UPInt size)   { return pGlobalHeap->AllocAutoHeap(p, size); }

    static void* GSTDCALL AllocAutoHeap(const void *p, UPInt size,
                               UPInt align)                 { return pGlobalHeap->AllocAutoHeap(p, size, align); }

    static void* GSTDCALL AllocAutoHeap(const void *p, UPInt size,
                               const GAllocDebugInfo& info) { return pGlobalHeap->AllocAutoHeap(p, size, &info); }

    static void* GSTDCALL AllocAutoHeap(const void *p, UPInt size,
                               UPInt align, 
                               const GAllocDebugInfo& info) { return pGlobalHeap->AllocAutoHeap(p, size, align, &info); }

    // Allocate in a specified heap. The later functions are provided to ensure that 
    // GAllocDebugInfo can be passed by reference with extended lifetime.
    static void* GSTDCALL AllocInHeap(GMemoryHeap* heap, UPInt size) { return heap->Alloc(size); }

    static void* GSTDCALL AllocInHeap(GMemoryHeap* heap, UPInt size,
                                 UPInt align)               { return heap->Alloc(size, align); }

    static void* GSTDCALL AllocInHeap(GMemoryHeap* heap, UPInt size,
                             const GAllocDebugInfo& info)   { return heap->Alloc(size, &info); }

    static void* GSTDCALL AllocInHeap(GMemoryHeap* heap, UPInt size,
                             UPInt align,
                             const GAllocDebugInfo& info)   { return heap->Alloc(size, align, &info); }

    
    // Reallocate to a new size, 0 return == can't reallocate
    //                           on fail, previous memory is still valid
    // Realloc to decrease size will never fail
    // Realloc of pointer == 0 is equivalent to GMemory::Alloc
    // Realloc of size == 0, is equivalent to GMemory::Free & will return 0
    // NOTE: Reallocation requests do not support custom alignment; if that is required an Alloc/Free call should be made.
    static void* GSTDCALL Realloc(void *p, UPInt newSize)            { return pGlobalHeap->Realloc(p, newSize); }
   
    // Free allocated/reallocated block
    static void  GSTDCALL Free(void *p)                              { return pGlobalHeap->Free(p); }

    static GMemoryHeap* GSTDCALL GetHeapByAddress(const void* p)     { return pGlobalHeap->GetAllocHeap(p); }

    static bool  GSTDCALL DetectMemoryLeaks()                        { return pGlobalHeap->DumpMemoryLeaks(); }
};


// ******* Global allocation overrides


// Calls constructor on own memory created with "new(ptr) type"
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE

#if defined(GFC_CC_MSVC) || defined(GFC_OS_SYMBIAN)
GINLINE void*   operator new        (UPInt n, void *ptr)   { return ptr; GUNUSED(n); }
GINLINE void    operator delete     (void *ptr, void *ptr2) { return; GUNUSED2(ptr,ptr2); }
#else
// Needed for placement on many platforms including PSP.
#include <new>
#endif

#endif // __PLACEMENT_NEW_INLINE


// ***** Macros to redefine class new/delete operators

// Types specifically declared to allow disambiguation of address in
// class member operator new. Used in GHeapNew.h.
struct GMemAddressStub { };
typedef GMemAddressStub* GMemAddressPtr;


#define GFC_MEMORY_REDEFINE_NEW_IMPL(class_name, check_delete, StatType)                                        \
    void*   operator new(UPInt sz)                                                                              \
    { void *p = GALLOC(sz, StatType); return p; }                                                               \
    void*   operator new(UPInt sz, GMemoryHeap* heap)                                                           \
    { void *p = GHEAP_ALLOC(heap, sz, StatType); return p; }                                                    \
    void*   operator new(UPInt sz, GMemoryHeap* heap, int blocktype)                                            \
    { GUNUSED(blocktype); void *p = GHEAP_ALLOC(heap, sz, blocktype); return p; }                               \
    void*   operator new(UPInt sz, GMemAddressPtr adr)                                                          \
    { void *p = GMemory::AllocAutoHeap(adr, sz, GAllocDebugInfo(StatType,__FILE__,__LINE__)); return p; }       \
    void*   operator new(UPInt sz, const char* pfilename, int line)                                             \
    { void* p = GMemory::Alloc(sz, GAllocDebugInfo(StatType, pfilename, line)); return p; }                     \
    void*   operator new(UPInt sz, GMemoryHeap* heap, const char* pfilename, int line)                          \
    { void* p = GMemory::AllocInHeap(heap, sz, GAllocDebugInfo(StatType, pfilename, line)); return p; }         \
    void*   operator new(UPInt sz, GMemAddressPtr adr, const char* pfilename, int line)                         \
    { void* p = GMemory::AllocAutoHeap(adr, sz, GAllocDebugInfo(StatType, pfilename, line)); return p; }        \
    void*   operator new(UPInt sz, int blocktype, const char* pfilename, int line)                              \
    { void* p = GMemory::Alloc(sz, GAllocDebugInfo(blocktype, pfilename, line)); return p; }                    \
    void*   operator new(UPInt sz, GMemoryHeap* heap, int blocktype, const char* pfilename, int line)           \
    { void* p = GMemory::AllocInHeap(heap, sz, GAllocDebugInfo(blocktype, pfilename, line)); return p; }        \
    void*   operator new(UPInt sz, GMemAddressPtr adr, int blocktype, const char* pfilename, int line)          \
    { void* p = GMemory::AllocAutoHeap(adr, sz, GAllocDebugInfo(blocktype, pfilename, line)); return p; }       \
    void    operator delete(void *p)                                        \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, const char*, int)                      \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, int, const char*, int)                 \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, GMemoryHeap*)                          \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, GMemoryHeap*, int)                     \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, GMemoryHeap*, const char*, int)        \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, GMemoryHeap*, int,const char*,int)     \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, GMemAddressPtr)                        \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, GMemAddressPtr, int)                   \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, GMemAddressPtr, const char*, int)      \
    { check_delete(class_name, p); GFREE(p); }                              \
    void    operator delete(void *p, GMemAddressPtr,int,const char*,int)    \
    { check_delete(class_name, p); GFREE(p); }                              \


/*
    void*   operator new(UPInt sz, GMemoryHeap* heap, const char* pfilename, int line)                                \
    { void* p = heap->Alloc(sz, &GAllocDebugInfo(StatType, pfilename, line)); init_mem(class_name, p); return p; }   \
    void*   operator new(UPInt sz, GMemoryHeap* heap, int blocktype, const char* pfilename, int line)                 \
    { void* p = heap->(sz, &GAllocDebugInfo(blocktype, pfilename, line)); init_mem(class_name, p); return p; }   \
    */

#define GFC_MEMORY_CHECK_DELETE_NONE(class_name, p)

// Redefined all delete/new operators in a class without custom memory initialization
#define GFC_MEMORY_REDEFINE_NEW(class_name, StatType) GFC_MEMORY_REDEFINE_NEW_IMPL(class_name, GFC_MEMORY_CHECK_DELETE_NONE, StatType)


// Base class that overrides the new and delete operators.
// Deriving from this class, even as a multiple base, incurs no space overhead.
template<int Stat>
class GNewOverrideBase
{
public:
    enum { StatType = Stat };

    // Redefine all new & delete operators.
    GFC_MEMORY_REDEFINE_NEW(GNewOverrideBase, Stat)
};



// *** Restore operator 'new' Definition

// If users specified GFC_BUILD_DEFINE_NEW in preprocessor settings, use that definition.
#if defined(GFC_BUILD_DEFINE_NEW) && !defined(GFC_DEFINE_NEW)
#define GFC_DEFINE_NEW new(__FILE__,__LINE__)
#endif
// Redefine operator 'new' if necessary.
#if defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif


#endif // INC_GMEMORY_H
