/**********************************************************************

Filename    :   GFxStringHash.h
Content     :   String hash table used when optional case-insensitive
                lookup is required.
Created     :
Authors     :
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxStringHash_H
#define INC_GFxStringHash_H

#include "GStringHash.h"

// GFxStringHash... classes have been renamed to GStringHash... classes and moved to GKernel
// These classes are here only for backward compatibility


template<class U, class Allocator = GAllocatorGH<U> >
class GFxStringHash : public GStringHash<U, Allocator>
{
    typedef GFxStringHash<U, Allocator>                                 SelfType;
    typedef GHash<GString, U, GString::NoCaseHashFunctor, Allocator>    BaseType;
public:    

    void    operator = (const SelfType& src) { BaseType::operator = (src); }
};

template<class U>
class GFxStringHash_sid : public GStringHash_sid<U>
{
    typedef GFxStringHash_sid<U>      SelfType;
    typedef GStringHash<U>            BaseType;
public:    

    void    operator = (const SelfType& src) { BaseType::operator = (src); }
};


template<class U, int SID = GStat_Default_Mem,
         class HashF = GString::NoCaseHashFunctor,         
         class HashNode = GStringLH_HashNode<U,HashF>,
         class Entry = GHashsetCachedNodeEntry<HashNode, typename HashNode::NodeHashF> >
class GFxStringHashLH : public GStringHashLH<U, SID, HashF, HashNode, Entry>
{
public:
    typedef GFxStringHashLH<U, SID, HashF, HashNode, Entry>   SelfType;

public:    

GFxStringHashLH() : GStringHashLH<U,SID,HashF,HashNode,Entry>()  {  }
    GFxStringHashLH(int sizeHint) : GStringHashLH<U,SID,HashF,HashNode,Entry>(sizeHint)  { }
    GFxStringHashLH(const SelfType& src) : GStringHashLH<U,SID,HashF,HashNode,Entry>(src) { }
    ~GFxStringHashLH()    { }

    void    operator = (const SelfType& src) { GStringHashLH<U,SID,HashF,HashNode,Entry>::operator = (src); }
};

#endif
