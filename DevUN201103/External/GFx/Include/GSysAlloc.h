/**********************************************************************

Filename    :   GSysAlloc.h
Content     :   System Allocator Interface
Created     :   2009
Authors     :   Maxim Shemanarev, Michael Antonov

Notes       :   Interface to the system allocator.

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSysAlloc_H
#define INC_GSysAlloc_H

#include "GTypes.h"


// ***** GSysAllocBase

// GSysAllocBase is a base class for GSysAllocPaged and GSysAllocBase.
// This class is used to provide link-independent initialization for 
// different memory heap engines in GSystem class and has no public APIs.
class GSysAllocBase
{
    friend class GSystem;
protected:
    virtual ~GSysAllocBase() { }

    // Initializes heap system, creating and initializing GlobalHeap.
    virtual bool initHeapEngine(const void* heapDesc) { GUNUSED(heapDesc); return false; }
    // Shuts down heap system, clearing out global heap.
    virtual void shutdownHeapEngine() { }
};


//------------------------------------------------------------------------
// ***** GSysAlloc

// GSysAlloc defines a memory allocation interface that developers can override
// to to provide memory for GFx; an instance of this class is typically created on
// application startup and passed into GSystem or GFxSystem constructor.
// 
// This new "malloc-friendly" interface is introduced with GFx 3.3 and replaces
// older GSysAlloc implementation (now renamed to GSysAllocPaged). GSysAlloc
// is more memory efficient when delegating to malloc/dlmalloc based implementation,
// as it doesn't require large alignment and will return more memory blocks
// to the application once content is unloaded.
//
// Users implementing this interface must provide three functions: Alloc, Free,
// and Realloc. Implementations of these functions must honor the requested alignment.
// Although arbitrary alignment requests are possible, requested alignment will
// typically be small, such as 16 bytes or less.
//
// GSysAlloc links to the GMemoryHeapMH implementation, which groups small
// allocations into 4K-sized blocks belonging to the heap. Allocations larger
// then 512 bytes will delegated directly to GSysAlloc and released immediately
// once no longer used, making more memory available to the application.

class GSysAlloc : public GSysAllocBase
{
public:
    GSysAlloc() {}
    virtual ~GSysAlloc() {}

    virtual void* Alloc(UPInt size, UPInt align) = 0;
    virtual void  Free(void* ptr, UPInt size, UPInt align) = 0;
    virtual void* Realloc(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align) = 0;

protected:
    // Implementation of GSysAllocBase based on MemoryHeapMH.
    virtual bool initHeapEngine(const void* heapDesc);
    virtual void shutdownHeapEngine();
};



//------------------------------------------------------------------------
// ***** GSysAllocPaged

// GSysAllocPaged is an abstract interface used to provide allocations to page
// based GMemoryHeapPT memory system, first introduced with GFx 3.0. Unlike
// GSysAlloc, this system will requires 4K page alignment on allocations and
// will in general keep larger blocks allocated. This system is best for OS-direct
// allocations, being able to take advantage of HW paging, and is also used
// to implement GSysAllocStatic. If you plan to implement this interface
// by delegating to malloc/dlmalloc based API, GSysAlloc is a better choice.
// 
// users can implement it to delegate allocations to their own memory allocator.
// Developers implementing GSysAllocPaged need to define at least three
// functions: GetInfo, Alloc and Free.
//
// When running, GFx will allocate and free large blocks through this interface,
// managing all of the smaller allocations internally. As GFx uses 4K alignment
// for large blocks internally, it is best if this interface maps directly to OS
// implementation. When that is not the case, it is best to use 64K+ granularity
// to make sure that minimal memory is wasted on alignment.
//
// Scaleform provides several default GSysAllocPaged implementations:
//  1. Memory Range Based:  GSysAllocStatic.
//      - Accepts a memory range in constructor, forcing GFx to work within
//        a block of memory.                    
//  2. System Specific:     GSysAllocWin32, etc.
//     - Uses OS specific implementation such as VirtualAlloc, mmap, etc
//       to allocate memory pages. This implementation is most efficient for
//       the target platform. This heap is used by default in GFxSystem if
//       you don't specify a different allocator.
//  3. Malloc version:      GSysAllocPagedMalloc.
//     - Uses standard library functions such as memalign() and free(). You can
//       use the source code to these as a reference for your own implementation.
//     - It is better to use GSysAlloc derived implementation insead.

class GSysAllocPaged : public GSysAllocBase
{
public:
    struct Info
    {
        // MinAlign is the minimum alignment that system allocator will always 
        // to apply all of the allocations, independent of whether it is requested
        // or not. For OS allocators, this will often be equal to the page size
        // of the system.
        UPInt   MinAlign;

        // MaxAlign is the maximum supported alignment that will be honored
        // by the allocator. For larger alignment requests, external granulator
        // wrapper will fake alignment by making larger allocations.
        // Set this value to 0 if you allocator supports any alignment request;
        // set this to 1 byte if it doesn't support any alignment.
        UPInt   MaxAlign;

        // Granularity is the allocation granularity that the system can handle 
        // efficiently. On Win32 it's at least 64K and must be a multiple of 64K.
        UPInt   Granularity;

        // When not null SysDirectThreshold defines the global size threshold. 
        // If the allocation size is greater or equal this threshold it's being 
        // redirected to the system, ignoring the granulator layer.
        UPInt   SysDirectThreshold;

        // If not null it MaxHeapGranularity restricts the maximal possible 
        // heap granularity. In most cases it can reduce the system memory 
        // footprint for the price of more frequent segment alloc/free operations, 
        // which slows down the allocator.
        // MaxHeapGranularity must be at least 4096 and a multiple of 4096.
        UPInt   MaxHeapGranularity;

        // HasRealloc flag tells the allocator whether or not ReallocInPlace is 
        // implemented. This is just an optimization that allows the allocator to 
        // eliminate some unnecessary operations. If ReallocInPlace is not 
        // implemented the allocator is still capable to reallocate memory but
        // moving of the data will occur more frequently.
        bool    HasRealloc;
    };
    GSysAllocPaged() {}

    // Fills in information about capabilities of this GSysAllocPaged implementation.
    // GMemoryHeap implementation will take these values into account when making
    // allocation calls.
    virtual void    GetInfo(Info* i) const = 0;
    virtual void*   Alloc(UPInt size, UPInt align) = 0;    
    virtual bool    Free(void* ptr, UPInt size, UPInt align) = 0;

    // ReallocInPlace attempts to reallocate memory to a new size without moving it.
    // If such reallocation succeeds 'true' is returned, otherwise 'false' is returned and the
    // previous allocation remains unchanged. This function is provided as an optimization
    // for internal Realloc implementation for large blocks; it does not need to be implemented.
    virtual bool    ReallocInPlace(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align)
    {
        GUNUSED4(oldPtr, oldSize, newSize, align);
        return false;
    }

    // Not mandatory for overriding. May do nothing.
    virtual void*   AllocSysDirect(UPInt, UPInt, UPInt*, UPInt*) { return 0; }
    virtual bool    FreeSysDirect(void*, UPInt, UPInt)           { return false; }

    virtual UPInt   GetBase() const { return 0; } // DBG
    virtual UPInt   GetSize() const { return 0; } // DBG

    virtual UPInt   GetFootprint() const { return 0; }
    virtual UPInt   GetUsedSpace() const { return 0; }
    virtual void    VisitMem(class GHeapMemVisitor*) const {}
    virtual void    VisitSegments(class GHeapSegVisitor*, UPInt, UPInt) const {}

    virtual ~GSysAllocPaged() { }


protected:
    // Implementation of GSysAllocBase based on MemoryHeapPT.
    virtual bool initHeapEngine(const void* heapDesc);
    virtual void shutdownHeapEngine();
};


//------------------------------------------------------------------------
// ***** GSysAllocStatic
//
//  System allocator that works entirely in a single block of memory.
class  GHeapAllocLite;
class  GHeapMemVisitor;
class GSysAllocStatic : public GSysAllocPaged
{
public:
    enum { MaxSegments = 4 };

    GEXPORT GSysAllocStatic(void* mem1=0, UPInt size1=0,
                    void* mem2=0, UPInt size2=0,
                    void* mem3=0, UPInt size3=0,
                    void* mem4=0, UPInt size4=0);

    GEXPORT GSysAllocStatic(UPInt minSize);

    GEXPORT void            AddMemSegment(void* mem, UPInt size);

    virtual void    GetInfo(Info* i) const;
    virtual void*   Alloc(UPInt size, UPInt align);
    virtual bool    ReallocInPlace(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align);
    virtual bool    Free(void* ptr, UPInt size, UPInt align);

    virtual void    VisitMem(GHeapMemVisitor* visitor) const;

    virtual UPInt   GetFootprint() const;
    virtual UPInt   GetUsedSpace() const;

    virtual UPInt   GetBase() const; // DBG
    virtual UPInt   GetSize() const; // DBG

private:
    UPInt           MinSize;
    UPInt           NumSegments;
    GHeapAllocLite* pAllocator;
    UPInt           PrivateData[8];
    UPInt           Segments[MaxSegments][8];
    UPInt           TotalSpace;
};



//------------------------------------------------------------------------
// ***** GSysAllocBase_SingletonSupport

// GSysAllocBase_SingletonSupport is a GSysAllocBase class wrapper that implements
// the InitSystemSingleton static function, used to create a global singleton
// used for the GFxSystem default argument initialization.
//
// End users implementing custom GSysAlloc/Paged interface don't need to make use of this base
// class; they can just create an instance of their own class on stack and pass it to GSystem.

template<class A, class B>
class GSysAllocBase_SingletonSupport : public B
{
    struct SysAllocContainer
    {        
        UPInt Data[(sizeof(A) + sizeof(UPInt)-1) / sizeof(UPInt)];
        bool  Initialized;
        SysAllocContainer() : Initialized(0) { }
    };

    SysAllocContainer* pContainer;

public:
    GSysAllocBase_SingletonSupport() : pContainer(0) { }

    // Creates a singleton instance of this GSysAllocPaged class used
    // on GSYSALLOC_DEFAULT_CLASS during GFxSystem initialization.
    static  B*  InitSystemSingleton()
    {
        static SysAllocContainer Container;
        GASSERT(Container.Initialized == false);

    #undef new

        GSysAllocBase_SingletonSupport<A,B> *presult = ::new((void*)Container.Data) A;

    // Redefine operator 'new' if necessary.
    #if defined(GFC_DEFINE_NEW)
    #define new GFC_DEFINE_NEW
    #endif

        presult->pContainer = &Container;
        Container.Initialized = true;
        return presult;
    }

protected:
    virtual void shutdownHeapEngine()
    {
        B::shutdownHeapEngine();
        if (pContainer)
        {
            pContainer->Initialized = false;
            ((B*)this)->~B();
            pContainer = 0;
        }
    }
};


//------------------------------------------------------------------------
// ***** GSysMemoryMap
//
// GSysMemMap is an abstract interface to the system memory mapping such as
// WinAPI VirtualAlloc/VirtualFree or posix mmap/munmap. It's used with
// GSysAllocMemoryMap that takes a full advantage of the system memory mapping
// interface.
class GSysMemoryMap
{
public:
    virtual UPInt   GetPageSize() const = 0;

    // Reserve and release address space. The functions must not allocate
    // any actual memory. The size is typically very large, such as 128 
    // megabytes. ReserveAddrSpace() returns a pointer to the reserved
    // space; there is no usable memory is allocated.
    //--------------------------------------------------------------------
    virtual void*   ReserveAddrSpace(UPInt size) = 0;
    virtual bool    ReleaseAddrSpace(void* ptr, UPInt size) = 0;

    // Map and unmap memory pages to allocate actual memory. The caller 
    // guarantees the integrity of the calls. That is, the the size is 
    // always a multiple of GetPageSize() and the ptr...ptr+size is always 
    // within the reserved address space. Also it's guaranteed that there 
    // will be no MapPages call for already mapped memory and no UnmapPages 
    // for not mapped memory.
    //--------------------------------------------------------------------
    virtual void*   MapPages(void* ptr, UPInt size) = 0;
    virtual bool    UnmapPages(void* ptr, UPInt size) = 0;

    virtual ~GSysMemoryMap() {}
};


#endif
