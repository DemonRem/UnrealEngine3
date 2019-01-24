/**********************************************************************

Filename    :   GStringHash.h
Content     :   String hash table used when optional case-insensitive
                lookup is required.
Created     :
Authors     :
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSTRINGHASH_H
#define INC_GSTRINGHASH_H

#include "GString.h"
#include "GHash.h"

// *** GStringHash

// This is a custom string hash table that supports case-insensitive
// searches through special functions such as GetCaseInsensitive, etc.
// This class is used for Flash labels, exports and other case-insensitive tables.

template<class U, class Allocator = GAllocatorGH<U> >
class GStringHash : public GHash<GString, U, GString::NoCaseHashFunctor, Allocator>
{
    typedef GStringHash<U, Allocator>                                 SelfType;
    typedef GHash<GString, U, GString::NoCaseHashFunctor, Allocator>  BaseType;
public:    

    void    operator = (const SelfType& src) { BaseType::operator = (src); }

    bool    GetCaseInsensitive(const GString& key, U* pvalue) const
    {
        GString::NoCaseKey ikey(key);
        return BaseType::GetAlt(ikey, pvalue);
    }
    // Pointer-returning get variety.
    const U* GetCaseInsensitive(const GString& key) const   
    {
        GString::NoCaseKey ikey(key);
        return BaseType::GetAlt(ikey);
    }
    U*  GetCaseInsensitive(const GString& key)
    {
        GString::NoCaseKey ikey(key);
        return BaseType::GetAlt(ikey);
    }

    
    typedef typename BaseType::Iterator base_iterator;

    base_iterator    FindCaseInsensitive(const GString& key)
    {
        GString::NoCaseKey ikey(key);
        return BaseType::FindAlt(ikey);
    }

    // Set just uses a find and assigns value if found. The key is not modified;
    // this behavior is identical to Flash string variable assignment.    
    void    SetCaseInsensitive(const GString& key, const U& value)
    {
        base_iterator it = FindCaseInsensitive(key);
        if (it != BaseType::End())
        {
            it->Second = value;
        }
        else
        {
            BaseType::Add(key, value);
        }
    } 
};


// Global version that assigns the id.
template<class U, int SID = GStat_Default_Mem>
class GStringHash_sid : public GStringHash<U, GAllocatorGH<U, SID> >
{
    typedef GStringHash_sid<U, SID>                 SelfType;
    typedef GStringHash<U, GAllocatorGH<U, SID> >   BaseType;
public:    

    void    operator = (const SelfType& src) { BaseType::operator = (src); }
};




// ***** GStringHashLH: Structure heap local String Hash table

// This hash table must live locally within a heap-allocated structure
// and is also allocates its string from the local heap by using GStringLcl.
// 
// This hash table needs to have custom implementation, since all strings
// are passed by GString reference, so the locally stored value is not the same.


template<class U, class HashF>
struct GStringLH_HashNode
{
    GStringLH  First;
    U          Second;

    // NodeRef is used to allow passing of elements into ghash_set
    // without using a temporary object.
    struct NodeRef
    {
        const GString*   pFirst;
        const U*         pSecond;

        NodeRef(const GString& f, const U& s) : pFirst(&f), pSecond(&s) { }
        NodeRef(const NodeRef& src)           : pFirst(src.pFirst), pSecond(src.pSecond) { }

        // Enable computation of ghash_node_hashf.
        inline UPInt GetHash() const        { return HashF()(*pFirst); } 
        // Necessary conversion to allow ghash_node::operator == to work.
        operator const GString& () const    { return *pFirst; }
    };

    // Note: No default constructor is necessary.
    GStringLH_HashNode(const GStringLH_HashNode& src) : First(src.First), Second(src.Second) { }
    GStringLH_HashNode(const NodeRef& src) : First(*src.pFirst), Second(*src.pSecond)  { }
    void operator = (const NodeRef& src)  { First  = *src.pFirst; Second = *src.pSecond; }

#ifdef GFC_CC_RENESAS
    bool operator == (const NodeRef& src) const { return (First == *src.pFirst); }
#endif

    template<class K>
    bool operator == (const K& src) const  { return (First == src); }

    template<class K>
    static UPInt GSTDCALL CalcHash(const K& data)   { return HashF()(data); }
    inline UPInt GetHash() const           { return HashF()(First); }

    // Hash functors used with this node. A separate functor is used for alternative
    // key lookup so that it does not need to access the '.First' element.    
    struct NodeHashF    
    {    
        template<class K>
        UPInt  operator()(const K& data) const { return data.GetHash(); } 
    };    
    struct NodeAltHashF    
    {
        template<class K>
        UPInt  operator()(const K& data) const { return GStringLH_HashNode<U,HashF>::CalcHash(data); }
    };
};


#undef new

template<class U, int SID = GStat_Default_Mem,
         class HashF = GString::NoCaseHashFunctor,         
         class HashNode = GStringLH_HashNode<U,HashF>,
         class Entry = GHashsetCachedNodeEntry<HashNode, typename HashNode::NodeHashF> >
class GStringHashLH
{
public:
    GFC_MEMORY_REDEFINE_NEW(GStringHashLH, SID)

    typedef GAllocatorLH<U, SID>                              Allocator;

    // Types used for hash_set.
    typedef GStringHashLH<U, SID, HashF, HashNode, Entry>     SelfType;
    typedef typename HashNode::NodeHashF                      HashNodeF;
    typedef GHashSet<HashNode, HashNodeF,
                     typename HashNode::NodeAltHashF, Allocator,
                     Entry >                                  Container;

    // Actual hash table itself, implemented as hash_set.
    Container   Hash;

public:    

    GStringHashLH()                                     { }
    GStringHashLH(int sizeHint) : Hash(sizeHint)        { }
    GStringHashLH(const SelfType& src) : Hash(src.Hash) { }
    ~GStringHashLH()                                    { }

    void    operator = (const SelfType& src)    { Hash = src.Hash; }

    // Remove all entries from the GHash table.
    inline void    Clear()                  { Hash.Clear(); }
    // Returns true if the GHash is empty.
    inline bool    IsEmpty() const          { return Hash.IsEmpty(); }

    // Access (set).
    inline void    Set(const GString& key, const U& value)
    {
        typename HashNode::NodeRef e(key, value);
        Hash.Set(e);
    }
    inline void    Add(const GString& key, const U& value)
    {
        typename HashNode::NodeRef e(key, value);
        Hash.Add(e);
    }

    // Removes an element by clearing its Entry.
    inline void     Remove(const GString& key)
    {   
        Hash.RemoveAlt(key);
    }
    template<class K>
    inline void     RemoveAlt(const K& key)
    {   
        Hash.RemoveAlt(key);
    }

    // Retrieve the value under the given key.    
    //  - If there's no value under the key, then return false and leave *pvalue alone.
    //  - If there is a value, return true, and set *Pvalue to the Entry's value.
    //  - If value == NULL, return true or false according to the presence of the key.    
    bool    Get(const GString& key, U* pvalue) const   
    {
        const HashNode* p = Hash.GetAlt(key);
        if (p)
        {
            if (pvalue)
                *pvalue = p->Second;
            return true;
        }
        return false;
    }

    template<class K>
    bool    GetAlt(const K& key, U* pvalue) const   
    {
        const HashNode* p = Hash.GetAlt(key);
        if (p)
        {
            if (pvalue)
                *pvalue = p->Second;
            return true;
        }
        return false;
    }

    // Retrieve the pointer to a value under the given key.    
    //  - If there's no value under the key, then return NULL.    
    //  - If there is a value, return the pointer.    
    inline U*  Get(const GString& key)
    {
        HashNode* p = Hash.GetAlt(key);
        return p ? &p->Second : 0;
    }
    inline const U* Get(const GString& key) const
    {
        const HashNode* p = Hash.GetAlt(key);
        return p ? &p->Second : 0;
    }

    template<class K>
    inline U*  GetAlt(const K& key)
    {
        HashNode* p = Hash.GetAlt(key);
        return p ? &p->Second : 0;
    }
    template<class K>
    inline const U* GetAlt(const K& key) const
    {
        const HashNode* p = Hash.GetAlt(key);
        return p ? &p->Second : 0;
    }

    // Sizing methods - delegate to GHash.
    inline UPInt   GetSize() const               { return Hash.GetSize(); }    
    inline void    Resize(UPInt n)               { Hash.Resize(n); }
    inline void    SetSapacity(UPInt newSize)    { Hash.RemoveAlt(newSize); }

    // Iterator API, like STL.
    typedef typename Container::ConstIterator   ConstIterator;
    typedef typename Container::Iterator        Iterator;

    inline Iterator        Begin()              { return Hash.Begin(); }
    inline Iterator        End()                { return Hash.End(); }
    inline ConstIterator   Begin() const        { return Hash.Begin(); }
    inline ConstIterator   End() const          { return Hash.End();   }

    Iterator        Find(const GString& key)          { return Hash.FindAlt(key);  }
    ConstIterator   Find(const GString& key) const    { return Hash.FindAlt(key);  }

    template<class K>
    Iterator        FindAlt(const K& key)       { return Hash.FindAlt(key);  }
    template<class K>
    ConstIterator   FindAlt(const K& key) const { return Hash.FindAlt(key);  }

    
    // **** Case-Insensitive Lookup

    bool    GetCaseInsensitive(const GString& key, U* pvalue) const
    {
        GString::NoCaseKey ikey(key);
        return GetAlt(ikey, pvalue);
    }
    // Pointer-returning get variety.
    const U* GetCaseInsensitive(const GString& key) const   
    {
        GString::NoCaseKey ikey(key);
        return GetAlt(ikey);
    }
    U*  GetCaseInsensitive(const GString& key)
    {
        GString::NoCaseKey ikey(key);
        return GetAlt(ikey);
    }


   // typedef typename BaseType::Iterator base_iterator;

    Iterator    FindCaseInsensitive(const GString& key)
    {
        GString::NoCaseKey ikey(key);
        return FindAlt(ikey);
    }

    // Set just uses a find and assigns value if found. The key is not modified;
    // this behavior is identical to Flash string variable assignment.    
    void    SetCaseInsensitive(const GString& key, const U& value)
    {
        Iterator it = FindCaseInsensitive(key);
        if (it != End())
        {
            it->Second = value;
        }
        else
        {
            Add(key, value);
        }
    } 

};


// Redefine operator 'new' if necessary.
#if defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif

#endif
