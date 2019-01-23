/**********************************************************************

Access      :   Public
Filename    :   GSysAllocMalloc.h
Content     :   System Allocator Interface
Created     :   2009
Authors     :   Maxim Shemanarev, Vlad Merker

Notes       :   Interface to the system allocator.

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSysAllocMalloc_H
#define INC_GSysAllocMalloc_H

#include "GSysAlloc.h"

#if defined(GFC_OS_WIN32) || defined(GFC_OS_WINCE) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
#include <malloc.h>
#elif defined(GFC_OS_WII)
#include <revolution.h>
#include <revolution/os.h>
#include <string.h>
#include <new>
#else
#include <stdlib.h>
#include <string.h>
#endif

class GSysAllocMalloc : public GSysAllocBase_SingletonSupport<GSysAllocMalloc, GSysAlloc>
{
public:
#if defined(GFC_OS_WII)
    // Amount of memory to reserve in MEM2 arena; 0 is all available space
    GSysAllocMalloc(UPInt size = 0)
    {
        UByte *m2arenaLo = (UByte*) OSGetMEM2ArenaLo();
        UByte *m2arenaHi = (UByte*) OSGetMEM2ArenaHi();
        if (size > 0 && (m2arenaLo + size < m2arenaHi))
            m2arenaHi = m2arenaLo + size;
        OSSetMEM2ArenaLo(m2arenaHi);
        GFC_DEBUG_WARNING(m2arenaHi-m2arenaLo < 65536, "GSysAllocMalloc: Less than 64k memory available");

        ::new(PrivateData) GSysAllocStatic(m2arenaLo, (UByte*)m2arenaHi-(UByte*)m2arenaLo);
    }
#else
    GSysAllocMalloc() {}
#endif
    virtual ~GSysAllocMalloc() {}

#if defined(GFC_OS_WIN32) || defined(GFC_OS_WINCE) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
    virtual void* Alloc(UPInt size, UPInt align)
    {
        return _aligned_malloc(size, align);
    }

    virtual void  Free(void* ptr, UPInt size, UPInt align)
    {
        GUNUSED2(size, align);
        _aligned_free(ptr);
    }

    virtual void* Realloc(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align)
    {
        GUNUSED(oldSize);
        return _aligned_realloc(oldPtr, newSize, align);
    }

#elif defined(GFC_OS_PS3)
    virtual void* Alloc(UPInt size, UPInt align)
    {
        return memalign(align, size);
    }

    virtual void  Free(void* ptr, UPInt size, UPInt align)
    {
        GUNUSED2(size, align);
        free(ptr);
    }

    virtual void* Realloc(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align)
    {
        GUNUSED(oldSize);
        return reallocalign(oldPtr, newSize, align);
    }

#elif defined(GFC_OS_WII)
    virtual void* Alloc(UPInt size, UPInt align)
    {
        return GetAllocator()->Alloc(size, align);
    }

    virtual void  Free(void* ptr, UPInt size, UPInt align)
    {
        GetAllocator()->Free(ptr, size, align);
    }

    virtual void* Realloc(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align)
    {
        void* newPtr = NULL;
        if (GetAllocator()->ReallocInPlace(oldPtr, oldSize, newSize, align))
            newPtr = oldPtr;
        else {
            newPtr = GetAllocator()->Alloc(newSize, align);
            if (newPtr)
            {
                memcpy(newPtr, oldPtr, (newSize < oldSize) ? newSize : oldSize);
                GetAllocator()->Free(oldPtr, oldSize, align);
            }
        }
        return newPtr;
    }

private:
    UPInt PrivateData[(sizeof(GSysAllocStatic) + sizeof(UPInt)) / sizeof(UPInt)];
    GSysAllocStatic* GetAllocator() { return (GSysAllocStatic*) PrivateData; }

#else
    virtual void* Alloc(UPInt size, UPInt align)
    {
        UPInt ptr = (UPInt)malloc(size+align);
        UPInt aligned = 0;
        if (ptr)
        {
            aligned = (UPInt(ptr) + align-1) & ~(align-1);
            if (aligned == ptr) 
                aligned += align;
            *(((UPInt*)aligned)-1) = aligned-ptr;
        }
        return (void*)aligned;
    }

    virtual void  Free(void* ptr, UPInt size,  UPInt align)
    {
        UPInt src = UPInt(ptr) - *(((UPInt*)ptr)-1);
        free((void*)src);
    }

    virtual void* Realloc(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align)
    {
        void* newPtr = Alloc(newSize, align);
        if (newPtr)
        {
            memcpy(newPtr, oldPtr, (newSize < oldSize) ? newSize : oldSize);
            Free(oldPtr, oldSize, align);
        }
        return newPtr;
    }
#endif
};


#endif
