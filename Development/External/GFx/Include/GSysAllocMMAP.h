/**********************************************************************

Filename    :   GSysAllocMMAP.h
Content     :   MMAP based System Allocator
Created     :   2009
Authors     :   Maxim Shemanarev, Boris Rayskiy

Notes       :   System Allocator that uses regular malloc/free

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSysAllocMMAP_H
#define INC_GSysAllocMMAP_H

#include "GSysAlloc.h"


// ***** GSysAllocMMAP
//
//------------------------------------------------------------------------
class GSysAllocMMAP : public GSysAllocBase_SingletonSupport<GSysAllocMMAP, GSysAllocPaged>
{
public:
    enum { MinGranularity = 4*1024 };

    GSysAllocMMAP(UPInt granularity = MinGranularity);


    virtual void    GetInfo(Info* i) const;
    virtual void*   Alloc(UPInt size, UPInt align);
    virtual bool    ReallocInPlace(void* oldPtr, UPInt oldSize,
                                   UPInt newSize, UPInt align);
    virtual bool    Free(void* ptr, UPInt size, UPInt align);


private:

    UPInt   Alignment;
    UPInt   Granularity;
    UPInt   Footprint;

};

#endif // INC_GSysAllocMMAP_H


