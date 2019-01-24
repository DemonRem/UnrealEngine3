/**********************************************************************

Filename    :   GSystem.h
Content     :   General kernel initalization/cleanup, including that
                of the memory allocator.
Created     :   Ferbruary 5, 2009
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2009 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSystem_H
#define INC_GSystem_H

#include "GMemory.h"


// ***** GSystem Core Initialization class

// GSystem initialization must take place before any other GKernel objects are used;
// this is done my calling GSystem::Init(). Among other things, this is necessary to
// initialize the memory allocator. Similarly, GSystem::Destroy must be
// called before program exist for proper clenup. Both of these tasks can be achieved by
// simply creating GSystem object first, allowing its constructor/destructor do the work.

// Note that for GFx use this class super-seeded by the GFxSystem class, which
// should be used instead.

class GSystem
{
public:

    // Two default argument constructors, to allow specifying GSysAllocPaged with and without
    // HeapDesc for the root heap.
    GEXPORT GSystem(GSysAllocBase* psysAlloc = GSYSALLOC_DEFAULT_CLASS::InitSystemSingleton())
    {
        Init(psysAlloc);
    }
    GEXPORT GSystem(const GMemoryHeap::HeapDesc& rootHeapDesc,
            GSysAllocBase* psysAlloc = GSYSALLOC_DEFAULT_CLASS::InitSystemSingleton())
    {
        Init(rootHeapDesc, psysAlloc);
    }

    GEXPORT ~GSystem()
    {
        Destroy();
    }

    // Initializes GSystem core, setting the global heap that is needed for GFx
    // memory allocations. Users can override memory heap implementation by passing
    // a different memory heap here.   
    GEXPORT static void GCDECL Init(const GMemoryHeap::HeapDesc& rootHeapDesc,
                     GSysAllocBase* psysAlloc = GSYSALLOC_DEFAULT_CLASS::InitSystemSingleton());
    
    GEXPORT static void GCDECL Init(
                     GSysAllocBase* psysAlloc = GSYSALLOC_DEFAULT_CLASS::InitSystemSingleton())
    {
        Init(GMemoryHeap::RootHeapDesc(), psysAlloc);
    }

    // De-initializes GSystem more, finalizing the threading system and destroying
    // the global memory allocator.
    GEXPORT static void GCDECL Destroy();
};

#endif
