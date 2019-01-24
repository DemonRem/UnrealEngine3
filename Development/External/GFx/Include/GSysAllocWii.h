/**********************************************************************

Filename    :   GSysAllocWii.h
Content     :   Wii System Allocator
Created     :   2009
Authors     :   Maxim Shemanarev

Notes       :   

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSysAllocWii_H
#define INC_GSysAllocWii_H

#include "GSysAlloc.h"

class GSysAllocWii : public GSysAllocBase_SingletonSupport<GSysAllocWii, GSysAllocPaged>
{
public:
    // amount of memory to reserve in MEM2 arena; 0 is all available space
    GSysAllocWii(UPInt size = 0);

    virtual void    GetInfo(Info* i) const;
    virtual void*   Alloc(UPInt size, UPInt align);
    virtual bool    ReallocInPlace(void* oldPtr, UPInt oldSize, UPInt newSize, UPInt align);
    virtual bool    Free(void* ptr, UPInt size, UPInt align);

private:
    UPInt PrivateData[(sizeof(GSysAllocStatic) + sizeof(UPInt)) / sizeof(UPInt)];
    GSysAllocStatic* GetAllocator() { return (GSysAllocStatic*) PrivateData; }
};


#endif
