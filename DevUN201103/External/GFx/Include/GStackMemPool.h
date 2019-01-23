/**********************************************************************

Filename    :   GStackMemPool.h
Content     :   Policy-based memory pool, which tries to allocate memory
                on stack first.
Created     :   January 22, 2009
Authors     :   Sergey Sikorskiy

Notes       :   

History     :   

Copyright   :   (c) 2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GMEMSTACKPOOL_H
#define INC_GMEMSTACKPOOL_H

#include "GMemory.h"

////////////////////////////////////////////////////////////////////////////////
// GStackMemPool Alloc/Free policies.

// Regular Alloc/Free logic.
struct GMemPoolImmediateFree
{
    GMemPoolImmediateFree(GMemoryHeap* pheap) : pHeap(pheap) {}

    void* Alloc(UPInt nbytes, UPInt align)
    {
        return pHeap ? GMemory::AllocInHeap(pHeap, nbytes, align) : GMemory::Alloc(nbytes, align);
    }
    void Free(void* p)
    {
        GMemory::Free(p);
    }

private:
    GMemoryHeap*    pHeap;
};

// Postpone freeing of memory till the very end.
struct GMemPoolPostponeFree
{
    GMemPoolPostponeFree(GMemoryHeap* pheap) : pHeap(pheap) {}
    ~GMemPoolPostponeFree()
    {
        for (UPInt i = 0; i < AllocatedMem.GetSize(); ++i)
        {
            GMemory::Free(AllocatedMem[i]);
        }
    }

    void* Alloc(UPInt nbytes, UPInt align)
    {
        void* mem = pHeap ? GMemory::AllocInHeap(pHeap, nbytes, align) : GMemory::Alloc(nbytes, align);

        if (mem)
        {
            AllocatedMem.PushBack(mem);
        }

        return mem;
    }
    static void Free(void* /*p*/)
    {
        ; // Do nothing.
    }

private:
    GMemoryHeap*    pHeap;
    GArray<void*>   AllocatedMem;
};

////////////////////////////////////////////////////////////////////////////////
// Memory pool, which tries to allocate memory on stack.
// It can be used as a more flexible version of a stack memory buffer, or as 
// a faster memory allocator for in-place constructors.
template <UPInt N = 512, UPInt A = sizeof(void*), class AFP = GMemPoolImmediateFree>
class GStackMemPool : AFP
{
public:
    GStackMemPool(GMemoryHeap* pheap = NULL)
    : AFP(pheap), BuffPtr(AlignMem(Buff)), BuffSize(buff_max_size - (BuffPtr - Buff))
    {
    }

public:
    void* Alloc(UPInt nbytes)
    {
        void* tmpBuffPtr = NULL;

        if (nbytes <= BuffSize)
        {
            // Allocate in Buffer.
            tmpBuffPtr = BuffPtr;

            BuffPtr = AlignMem(BuffPtr + nbytes);
            const UPInt curSize = BuffPtr - Buff;
            BuffSize = buff_max_size > curSize ? buff_max_size - curSize : 0;
        } else
        {
            // Allocate in heap.
            tmpBuffPtr = AFP::Alloc(nbytes, align);
        }

        return tmpBuffPtr;
    }
    void Free(void* p)
    {
        if (!(p >= Buff && p < Buff + buff_max_size))
        {
            // Pointer is not inside of the Buffer.
            AFP::Free(p);
        }
    }

private:
    // Return new aligned offset. Align must be power of two.
    static
    char* AlignMem(char* offset)
    {
        return reinterpret_cast<char*>(A + ((reinterpret_cast<UPInt>(offset) - 1) & ~(A - 1)));
    }

private:
    // align must be power of two.
    enum { buff_max_size = N, align = A };

    char  Buff[buff_max_size];
    char* BuffPtr;
    UPInt BuffSize;
};

#undef new

template <UPInt N, UPInt A>
inline
void* operator new (UPInt nbytes, GStackMemPool<N, A>& pool)
{
    return pool.Alloc(nbytes);
}

template <UPInt N, UPInt A>
inline
void operator delete (void* p, GStackMemPool<N, A>& pool)
{
    pool.Free(p);
}

// Redefine operator 'new' if necessary.
#if defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif

// This header has to follow "#define new".
#include "GHeapNew.h"

#endif // INC_GMEMSTACKPOOL_H
