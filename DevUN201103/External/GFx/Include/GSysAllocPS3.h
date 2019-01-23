/**********************************************************************

Filename    :   GSysAllocPS3.h
Content     :   PS3 system allocator
Created     :   2009
Authors     :   Vlad Merker

Notes       :   PS3 system allocator that uses sys_memory_* functions

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSysAllocPS3_H
#define INC_GSysAllocPS3_H

#include "GSysAlloc.h"

//------------------------------------------------------------------------
class GSysAllocPS3: public GSysAllocBase_SingletonSupport<GSysAllocPS3, GSysAllocPaged>
{
public:
    enum {
        MinPageSize = 0x10000U,  // 64K
        MaxPageSize = 0x100000U  // 1M
    };

    GSysAllocPS3(UPInt granularity = MinPageSize);

    virtual void*   Alloc(UPInt size, UPInt);
    virtual bool    Free(void* ptr, UPInt size, UPInt);
    virtual void    GetInfo(Info* i) const;

private:
    UPInt   Granularity;
    UPInt   SysAlignment;
    UPInt   Footprint;
};

#endif
