/**********************************************************************

Filename    :   GHeapNew.h
Content     :   Defines heap allocation macros such as GHEAP_NEW, etc.
Created     :   September 9, 2008
Authors     :   Michael Antonov

Notes       :   This file should be included in internal headers and
                source files but NOT the public headers because it
                #undefines operator new. It is separated from GMemory
                for this reason.

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GMemory.h"

// Operator 'new' must NOT be defined, or the macros below will not compile
// correctly (compiler would complain about the number of macro arguments).
#undef new

#ifndef INC_GHEAPNEW_H
#define INC_GHEAPNEW_H


// Define heap macros
#if defined(GHEAP_DEBUG_INFO)

#define GHEAP_NEW(pheap)        new(pheap,__FILE__,__LINE__)
#define GHEAP_NEW_ID(pheap,id)  new(pheap, id, __FILE__,__LINE__)
#define GHEAP_AUTO_NEW(paddr)   new((GMemAddressPtr)((void*)paddr),__FILE__,__LINE__)
// Global new
#define GNEW                    new(__FILE__,__LINE__)

#else

#define GHEAP_NEW(pheap)        new(pheap)
#define GHEAP_NEW_ID(pheap,id)  new(pheap,id)
#define GHEAP_AUTO_NEW(paddr)   new((GMemAddressPtr)((void*)paddr))
#define GNEW                    new
                                
#endif 


#endif // INC_GHEAPNEW_H
