/**********************************************************************

Filename    :   GHash.h
Content     :   Template hash-table/set implementation
Created     :   August 20, 2001
Authors     :   Michael Antonov
Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GHash_H
#define INC_GHash_H

#include "GAllocator.h"

#undef new


// ***** Hash Table Implementation

// GHastSet and GHash.
//
// Hash table, linear probing, internal chaining.  One  interesting/nice thing
// about this implementation is that the table itself is a flat chunk of memory
// containing no pointers, only relative indices. If the key and value types
// of the GHash contain no pointers, then the GHash can be serialized using raw IO.
//
// Never shrinks, unless you explicitly Clear() it.  Expands on
// demand, though.  For best results, if you know roughly how big your
// table will be, default it to that size when you create it.
//
// Key usability feature:
//
//   1. Allows node hash values to either be cached or not.
//
//   2. Allows for alternative keys with methods such as GetAlt(). Handy
//      if you need to search nodes by their components; no need to create
//      temporary nodes.
//


// *** Hash functors:
//
//  GIdentityHash  - use when the key is already a good hash
//  HFixedSizeHash - general hash based on object's in-memory representation.


// Hash is just the input value; can use this for integer-indexed hash tables.
template<class C>
class GIdentityHash
{
public:
    UPInt operator()(const C& data) const
    { return (UPInt) data; }
};

// Computes a hash of an object's representation.
template<class C>
class GFixedSizeHash
{
public:
    // Alternative: "sdbm" hash function, suggested at same web page
    // above, http::/www.cs.yorku.ca/~oz/hash.html
    // This is somewhat slower then Bernstein, but it works way better than the above
    // hash function for hashing large numbers of 32-bit ints.
    static GINLINE UPInt SDBM_Hash(const void* data_in, UPInt size, UPInt seed = 5381)     
    {
        const UByte* data = (const UByte*) data_in;
        UPInt        h = seed;
        while (size > 0)
        {
            size--;
            h = (h << 16) + (h << 6) - h + (UPInt)data[size];
        }   
        return h;
    }

    UPInt operator()(const C& data) const
    {
        unsigned char*  p = (unsigned char*) &data;
        int size = sizeof(C);

        return SDBM_Hash(p, size);
    }
};



// *** GHashsetEntry Entry types. 

// Compact hash table Entry type that re-computes hash keys during hash traversal.
// Good to use if the hash function is cheap or the hash value is already cached in C.
template<class C, class HashF>
class GHashsetEntry
{
public:
    // Internal chaining for collisions.
    SPInt       NextInChain;
    C           Value;

    GHashsetEntry()
        : NextInChain(-2) { }
    GHashsetEntry(const GHashsetEntry& e)
        : NextInChain(e.NextInChain), Value(e.Value) { }
    GHashsetEntry(const C& key, SPInt next)
        : NextInChain(next), Value(key) { }

    bool    IsEmpty() const          { return NextInChain == -2;  }
    bool    IsEndOfChain() const     { return NextInChain == -1;  }

    // Cached hash value access - can be optimized bu storing hash locally.
    // Mask value only needs to be used if SetCachedHash is not implemented.
    UPInt   GetCachedHash(UPInt maskValue) const  { return HashF()(Value) & maskValue; }
    void    SetCachedHash(UPInt)                  {}

    void    Clear()
    {        
        Value.~C(); // placement delete
        NextInChain = -2;
    }
    // Free is only used from dtor of hash; Clear is used during regular operations:
    // assignment, hash reallocations, value reassignments, so on.
    void    Free() { Clear(); }
};

// Hash table Entry type that caches the Entry hash value for nodes, so that it
// does not need to be re-computed during access.
template<class C, class HashF>
class GHashsetCachedEntry
{
public:
    // Internal chaining for collisions.
    SPInt       NextInChain;
    UPInt       HashValue;
    C           Value;

    GHashsetCachedEntry()
        : NextInChain(-2) { }
    GHashsetCachedEntry(const GHashsetCachedEntry& e)
        : NextInChain(e.NextInChain), HashValue(e.HashValue), Value(e.Value) { }
    GHashsetCachedEntry(const C& key, SPInt next)
        : NextInChain(next), Value(key) { }

    bool    IsEmpty() const          { return NextInChain == -2;  }
    bool    IsEndOfChain() const     { return NextInChain == -1;  }

    // Cached hash value access - can be optimized bu storing hash locally.
    // Mask value only needs to be used if SetCachedHash is not implemented.
    UPInt   GetCachedHash(UPInt maskValue) const  { GUNUSED(maskValue); return HashValue; }
    void    SetCachedHash(UPInt hashValue)        { HashValue = hashValue; }

    void    Clear()
    {
        Value.~C();
        NextInChain = -2;
    }
    // Free is only used from dtor of hash; Clear is used during regular operations:
    // assignment, hash reallocations, value reassignments, so on.
    void    Free() { Clear(); }
};



// *** GHashSet implementation - relies on either cached or regular entries.
//
// Use: Entry = GHashsetCachedEntry<C, HashF> if hashes are expensive to
//              compute and thus need caching in entries.
//      Entry = GHashsetEntry<C, HashF> if hashes are already externally cached.
//
template<class C, class HashF = GFixedSizeHash<C>,
         class AltHashF = HashF, 
         class Allocator = GAllocatorGH<C>,         
         class Entry = GHashsetCachedEntry<C, HashF> >
class GHashSetBase
{
    enum { HashMinSize = 8 };

public:
    GFC_MEMORY_REDEFINE_NEW(GHashSetBase, Allocator::StatId)

    typedef GHashSetBase<C, HashF, AltHashF, Allocator, Entry>    SelfType;

    GHashSetBase() : pTable(NULL)                       {   }
    GHashSetBase(int sizeHint) : pTable(NULL)           { SetCapacity(this, sizeHint);  }
    explicit GHashSetBase(void* pmemAddr) : pTable(NULL){ GUNUSED(pmemAddr);  }
    GHashSetBase(void* pmemAddr, int sizeHint) : pTable(NULL) { SetCapacity(pmemAddr, sizeHint);  }
    GHashSetBase(const SelfType& src) : pTable(NULL)    { Assign(this, src); }
    ~GHashSetBase()                                     
    { 
        if (pTable)
        {
            // Delete the entries.
            for (UPInt i = 0, n = pTable->SizeMask; i <= n; i++)
            {
                Entry*  e = &E(i);
                if (!e->IsEmpty())             
                    e->Free();
            }            

            Allocator::Free(pTable);
            pTable = NULL;
        }
    }


    void Assign(void* pmemAddr, const SelfType& src)
    {
        Clear();
        if (src.IsEmpty() == false)
        {
            SetCapacity(pmemAddr, src.GetSize());

            for (ConstIterator it = src.Begin(); it != src.End(); ++it)
            {
                Add(pmemAddr, *it);
            }
        }
    }


    // Remove all entries from the GHashSet table.
    void Clear() 
    {
        if (pTable)
        {
            // Delete the entries.
            for (UPInt i = 0, n = pTable->SizeMask; i <= n; i++)
            {
                Entry*  e = &E(i);
                if (!e->IsEmpty())             
                    e->Clear();
            }            
                
            Allocator::Free(pTable);
            pTable = NULL;
        }
    }

    // Returns true if the GHashSet is empty.
    bool IsEmpty() const
    {
        return pTable == NULL || pTable->EntryCount == 0;
    }


    // Set a new or existing value under the key, to the value.
    // Pass a different class of 'key' so that assignment reference object
    // can be passed instead of the actual object.
    template<class CRef>
    void Set(void* pmemAddr, const CRef& key)
    {
        UPInt  hashValue = HashF()(key);
        SPInt  index     = (SPInt)-1;

        if (pTable != NULL)
            index = findIndexCore(key, hashValue & pTable->SizeMask);

        if (index >= 0)
        {            
            E(index).Value = key;
        }
        else
        {
            // Entry under key doesn't exist.
            add(pmemAddr, key, hashValue);
        }
    }

    template<class CRef>
    inline void Add(void* pmemAddr, const CRef& key)
    {
        UPInt hashValue = HashF()(key);
        add(pmemAddr, key, hashValue);
    }

    // Remove by alternative key.
    template<class K>
    void RemoveAlt(const K& key)
    {   
        if (pTable == NULL)
            return;

        UPInt   hashValue = AltHashF()(key);
        SPInt   index     = hashValue & pTable->SizeMask;

        Entry*  e = &E(index);

        // If empty node or occupied by collider, we have nothing to remove.
        if (e->IsEmpty() || (e->GetCachedHash(pTable->SizeMask) != (UPInt)index))
            return;        

        // Save index
        SPInt   naturalIndex = index;
        SPInt   prevIndex    = -1;

        while ((e->GetCachedHash(pTable->SizeMask) != (UPInt)naturalIndex) || !(e->Value == key))
        {
            // Keep looking through the chain.
            prevIndex   = index;
            index       = e->NextInChain;
            if (index == -1)
                return; // End of chain, item not found
            e = &E(index);
        }

        // Found it - our item is at index
        if (naturalIndex == index)
        {
            // If we have a follower, move it to us
            if (!e->IsEndOfChain())
            {               
                Entry*  enext = &E(e->NextInChain);
                e->Clear();
                new (e) Entry(*enext);
                // Point us to the follower's cell that will be cleared
                e = enext;
            }
        }
        else
        {
            // We are not at natural index, so deal with the prev items next index
            E(prevIndex).NextInChain = e->NextInChain;
        }

        // Clear us, of the follower cell that was moved.
        e->Clear();
        pTable->EntryCount --;
        // Should we check the size to condense hash? ...
    }

    // Remove by main key.
    template<class CRef>
    void Remove(const CRef& key)
    {
        RemoveAlt(key);
    }

    // Retrieve the pointer to a value under the given key.
    //  - If there's no value under the key, then return NULL.    
    //  - If there is a value, return the pointer.    
    template<class K>
    C* Get(const K& key)
    {
        SPInt   index = findIndex(key);
        if (index >= 0)        
            return &E(index).Value;        
        return 0;
    }   

    template<class K>
    const C* Get(const K& key) const
    {
        SPInt   index = findIndex(key);
        if (index >= 0)        
            return &E(index).Value;        
        return 0;
    }

    // Alternative key versions of Get. Used by Hash.
    template<class K>
    const C* GetAlt(const K& key) const
    {
        SPInt   index = findIndexAlt(key);
        if (index >= 0)        
            return &E(index).Value;
        return 0;
    }

    template<class K>
    C* GetAlt(const K& key)
    {
        SPInt   index = findIndexAlt(key);
        if (index >= 0)        
            return &E(index).Value;
        return 0;
    }   

    template<class K>
    bool GetAlt(const K& key, C* pval) const
    {
        SPInt   index = findIndexAlt(key);
        if (index >= 0)
        {
            if (pval)
                *pval = E(index).Value;
            return true;
        }
        return false;
    }


    UPInt GetSize() const
    {
        return pTable == NULL ? 0 : (UPInt)pTable->EntryCount;
    }


    // Resize the GHashSet table to fit one more Entry.  Often this
    // doesn't involve any action.
    void CheckExpand(void* pmemAddr)  
    {
        if (pTable == NULL)
        {
            // Initial creation of table.  Make a minimum-sized table.
            setRawCapacity(pmemAddr, HashMinSize);
        }
        else if (pTable->EntryCount * 5 > (pTable->SizeMask + 1) * 4)
        {
            // pTable is more than 5/4 ths full.  Expand.
            setRawCapacity(pmemAddr, (pTable->SizeMask + 1) * 2);
        }
    }

    // Hint the bucket count to >= n.
    void Resize(void* pmemAddr, UPInt n)    
    {
        // Not really sure what this means in relation to
        // STLport's hash_map... they say they "increase the
        // bucket count to at least n" -- but does that mean
        // their real capacity after Resize(n) is more like
        // n*2 (since they do linked-list chaining within
        // buckets?).
        SetCapacity(pmemAddr, n);
    }

    // Size the GHashSet so that it can comfortably contain the given
    // number of elements.  If the GHashSet already contains more
    // elements than newSize, then this may be a no-op.
    void SetCapacity(void* pmemAddr, UPInt newSize)
    {
        UPInt newRawSize = (newSize * 5) / 4;
        if (newRawSize <= GetSize())
            return;
        setRawCapacity(pmemAddr, newRawSize);
    }

    // Disable inappropriate 'operator ->' warning on MSVC6.
#ifdef GFC_CC_MSVC
#if (GFC_CC_MSVC < 1300)
# pragma warning(disable : 4284)
#endif
#endif

    // Iterator API, like STL.
    struct ConstIterator
    {   
        const C&    operator * () const
        {            
            GASSERT(Index >= 0 && Index <= (SPInt)pHash->pTable->SizeMask);
            return pHash->E(Index).Value;
        }

        const C*    operator -> () const
        {
            GASSERT(Index >= 0 && Index <= (SPInt)pHash->pTable->SizeMask);
            return &pHash->E(Index).Value;
        }

        void    operator ++ ()
        {
            // Find next non-empty Entry.
            if (Index <= (SPInt)pHash->pTable->SizeMask)
            {
                Index++;
                while ((UPInt)Index <= pHash->pTable->SizeMask &&
                    pHash->E(Index).IsEmpty())
                {
                    Index++;
                }
            }
        }

        bool    operator == (const ConstIterator& it) const
        {
            if (IsEnd() && it.IsEnd())
            {
                return true;
            }
            else
            {
                return (pHash == it.pHash) && (Index == it.Index);
            }
        }

        bool    operator != (const ConstIterator& it) const
        {
            return ! (*this == it);
        }


        bool    IsEnd() const
        {
            return (pHash == NULL) || 
                (pHash->pTable == NULL) || 
                (Index > (SPInt)pHash->pTable->SizeMask);
        }

        ConstIterator()
            : pHash(NULL), Index(0)
        { }

    protected:
        friend class GHashSetBase<C, HashF, AltHashF, Allocator, Entry>;

        ConstIterator(const SelfType* h, SPInt index)
            : pHash(h), Index(index)
        { }

        const SelfType* pHash;
        SPInt           Index;
    };

    friend struct ConstIterator;


    // Non-const Iterator; Get most of it from ConstIterator.
    struct Iterator : public ConstIterator
    {      
        // Allow non-const access to entries.
        C&  operator*() const
        {            
            GASSERT(ConstIterator::Index >= 0 && ConstIterator::Index <= (SPInt)ConstIterator::pHash->pTable->SizeMask);
            return const_cast<SelfType*>(ConstIterator::pHash)->E(ConstIterator::Index).Value;
        }    

        C*  operator->() const 
        {
            return &(operator*());
        }

        Iterator()
            : ConstIterator(NULL, 0)
        { }

        // Removes current element from Hash
        void Remove()
        {
            SelfType*   phash = const_cast<SelfType*>(ConstIterator::pHash);
            Entry*      ee = &phash->E(ConstIterator::Index);
            const C&    key = ee->Value;

            UPInt       hashValue = AltHashF()(key);
            SPInt       index     = hashValue & phash->pTable->SizeMask;

            Entry*      e = &phash->E(index);

            // If empty node or occupied by collider, we have nothing to remove.
            if (e->IsEmpty() || (e->GetCachedHash(phash->pTable->SizeMask) != (UPInt)index))
                return;        

            // Save index
            SPInt   naturalIndex = index;
            SPInt   prevIndex    = -1;

            while ((e->GetCachedHash(phash->pTable->SizeMask) != (UPInt)naturalIndex) || !(e->Value == key))
            {
                // Keep looking through the chain.
                prevIndex   = index;
                index       = e->NextInChain;
                if (index == -1)
                    return; // End of chain, item not found
                e = &phash->E(index);
            }

            if (index == (SPInt)ConstIterator::Index)
            {
                // Found it - our item is at index
                if (naturalIndex == index)
                {
                    // If we have a follower, move it to us
                    if (!e->IsEndOfChain())
                    {               
                        Entry*  enext = &phash->E(e->NextInChain);
                        e->Clear();
                        new (e) Entry(*enext);
                        // Point us to the follower's cell that will be cleared
                        e = enext;
                        --ConstIterator::Index;
                    }
                }
                else
                {
                    // We are not at natural index, so deal with the prev items next index
                    phash->E(prevIndex).NextInChain = e->NextInChain;
                }

                // Clear us, of the follower cell that was moved.
                e->Clear();
                phash->pTable->EntryCount --;
            }
            else 
                GASSERT(0); //?
        }

    private:
        friend class GHashSetBase<C, HashF, AltHashF, Allocator, Entry>;

        Iterator(SelfType* h, SPInt i0)
            : ConstIterator(h, i0)
        { }
    };

    friend struct Iterator;

    Iterator    Begin()
    {
        if (pTable == 0)
            return Iterator(NULL, 0);

        // Scan till we hit the First valid Entry.
        UPInt  i0 = 0;
        while (i0 <= pTable->SizeMask && E(i0).IsEmpty())
        {
            i0++;
        }
        return Iterator(this, i0);
    }
    Iterator        End()           { return Iterator(NULL, 0); }

    ConstIterator   Begin() const   { return const_cast<SelfType*>(this)->Begin();     }
    ConstIterator   End() const     { return const_cast<SelfType*>(this)->End();   }

    template<class K>
    Iterator Find(const K& key)
    {
        SPInt index = findIndex(key);
        if (index >= 0)        
            return Iterator(this, index);        
        return Iterator(NULL, 0);
    }

    template<class K>
    Iterator FindAlt(const K& key)
    {
        SPInt index = findIndexAlt(key);
        if (index >= 0)        
            return Iterator(this, index);        
        return Iterator(NULL, 0);
    }

    template<class K>
    ConstIterator Find(const K& key) const       { return const_cast<SelfType*>(this)->Find(key); }

    template<class K>
    ConstIterator FindAlt(const K& key) const    { return const_cast<SelfType*>(this)->FindAlt(key); }

private:
    // Find the index of the matching Entry.  If no match, then return -1.
    template<class K>
    SPInt findIndex(const K& key) const
    {
        if (pTable == NULL)
            return -1;
        UPInt hashValue = HashF()(key) & pTable->SizeMask;
        return findIndexCore(key, hashValue);
    }

    template<class K>
    SPInt findIndexAlt(const K& key) const
    {
        if (pTable == NULL)
            return -1;
        UPInt hashValue = AltHashF()(key) & pTable->SizeMask;
        return findIndexCore(key, hashValue);
    }

    // Find the index of the matching Entry.  If no match, then return -1.
    template<class K>
    SPInt findIndexCore(const K& key, UPInt hashValue) const
    {
        // Table must exist.
        GASSERT(pTable != 0);
        // Hash key must be 'and-ed' by the caller.
        GASSERT((hashValue & ~pTable->SizeMask) == 0);

        UPInt           index = hashValue;
        const Entry*    e     = &E(index);

        // If empty or occupied by a collider, not found.
        if (e->IsEmpty() || (e->GetCachedHash(pTable->SizeMask) != index))
            return -1;

        while(1)
        {
            GASSERT(e->GetCachedHash(pTable->SizeMask) == hashValue);

            if (e->GetCachedHash(pTable->SizeMask) == hashValue && e->Value == key)
            {
                // Found it.
                return index;
            }
            // Values can not be equal at this point.
            // That would mean that the hash key for the same value differs.
            GASSERT(!(e->Value == key));

            // Keep looking through the chain.
            index = e->NextInChain;
            if (index == (UPInt)-1)
                break; // end of chain

            e = &E(index);
            GASSERT(!e->IsEmpty());
        }
        return -1;
    }


    // Add a new value to the GHashSet table, under the specified key.
    template<class CRef>
    void add(void* pmemAddr, const CRef& key, UPInt hashValue)
    {
        CheckExpand(pmemAddr);
        hashValue &= pTable->SizeMask;

        pTable->EntryCount++;

        SPInt   index        = hashValue;
        Entry*  naturalEntry = &(E(index));

        if (naturalEntry->IsEmpty())
        {
            // Put the new Entry in.
            new (naturalEntry) Entry(key, -1);
        }
        else
        {
            // Find a blank spot.
            SPInt blankIndex = index;
            do {
                blankIndex = (blankIndex + 1) & pTable->SizeMask;
            } while(!E(blankIndex).IsEmpty());

            Entry*  blankEntry = &E(blankIndex);

            if (naturalEntry->GetCachedHash(pTable->SizeMask) == (UPInt)index)
            {
                // Collision.  Link into this chain.

                // Move existing list head.
                new (blankEntry) Entry(*naturalEntry);    // placement new, copy ctor

                // Put the new info in the natural Entry.
                naturalEntry->Value       = key;
                naturalEntry->NextInChain = blankIndex;
            }
            else
            {
                // Existing Entry does not naturally
                // belong in this slot.  Existing
                // Entry must be moved.

                // Find natural location of collided element (i.e. root of chain)
                SPInt collidedIndex = naturalEntry->GetCachedHash(pTable->SizeMask);
                GASSERT(collidedIndex >= 0 && collidedIndex <= (SPInt)pTable->SizeMask);
                for (;;)
                {
                    Entry*  e = &E(collidedIndex);
                    if (e->NextInChain == index)
                    {
                        // Here's where we need to splice.
                        new (blankEntry) Entry(*naturalEntry);
                        e->NextInChain = blankIndex;
                        break;
                    }
                    collidedIndex = e->NextInChain;
                    GASSERT(collidedIndex >= 0 && collidedIndex <= (SPInt)pTable->SizeMask);
                }

                // Put the new data in the natural Entry.
                naturalEntry->Value       = key;
                naturalEntry->NextInChain = -1;                
            }            
        }

        // Record hash value: has effect only if cached node is used.
        naturalEntry->SetCachedHash(hashValue);
    }

    // Index access helpers.
    Entry& E(UPInt index)
    {
        // Must have pTable and access needs to be within bounds.
        GASSERT(index <= pTable->SizeMask);
        return *(((Entry*) (pTable + 1)) + index);
    }
    const Entry& E(UPInt index) const
    {        
        GASSERT(index <= pTable->SizeMask);
        return *(((Entry*) (pTable + 1)) + index);
    }


    // Resize the GHashSet table to the given size (Rehash the
    // contents of the current table).  The arg is the number of
    // GHashSet table entries, not the number of elements we should
    // actually contain (which will be less than this).
    void    setRawCapacity(void* pheapAddr, UPInt newSize)    
    {
        if (newSize == 0)
        {
            // Special case.
            Clear();
            return;
        }

        // Minimum size; don't incur rehashing cost when expanding
        // very small tables. Not that we perform this check before 
        // 'log2f' call to avoid fp exception with newSize == 1.
        if (newSize < HashMinSize)        
            newSize = HashMinSize;       
        else
        {
            // Force newSize to be a power of two.
            int bits = gfchop( glog2f((float)(newSize-1)) + 1);
            // GASSERT((1 << bits) >= newSize);
            newSize = UPInt(1) << bits;
        }

        SelfType  newHash;
        newHash.pTable = (TableType*)
            Allocator::Alloc(
                pheapAddr, 
                sizeof(TableType) + sizeof(Entry) * newSize,
                __FILE__, __LINE__);
        // Need to do something on alloc failure!
        GASSERT(newHash.pTable);

        newHash.pTable->EntryCount = 0;
        newHash.pTable->SizeMask = newSize - 1;
        UPInt i, n;

        // Mark all entries as empty.
        for (i = 0; i < newSize; i++)
            newHash.E(i).NextInChain = -2;

        // Copy stuff to newHash
        if (pTable)
        {            
            for (i = 0, n = pTable->SizeMask; i <= n; i++)
            {
                Entry*  e = &E(i);
                if (e->IsEmpty() == false)
                {
                    // Insert old Entry into new GHashSet.
                    newHash.Add(pheapAddr, e->Value);
                    // placement delete of old element
                    e->Clear();
                }
            }

            // Delete our old data buffer.
            Allocator::Free(pTable);
        }

        // Steal newHash's data.
        pTable = newHash.pTable;
        newHash.pTable = NULL;
    }

    struct TableType
    {
        UPInt EntryCount;
        UPInt SizeMask;
        // Entry array follows this structure
        // in memory.
    };
    TableType*  pTable;
};




template<class C, class HashF = GFixedSizeHash<C>,
         class AltHashF = HashF, 
         class Allocator = GAllocatorGH<C>,         
         class Entry = GHashsetCachedEntry<C, HashF> >
class GHashSet : public GHashSetBase<C, HashF, AltHashF, Allocator, Entry>
{
public:
    typedef GHashSetBase<C, HashF, AltHashF, Allocator, Entry> BaseType;
    typedef GHashSet<C, HashF, AltHashF, Allocator, Entry>     SelfType;

    GHashSet()                                      {   }
    GHashSet(int sizeHint) : BaseType(sizeHint)     {   }
    explicit GHashSet(void* pheap) : BaseType(pheap)                {   }
    GHashSet(void* pheap, int sizeHint) : BaseType(pheap, sizeHint) {   }
    GHashSet(const SelfType& src) : BaseType(src)   {   }
    ~GHashSet()                                     {   }

    void operator = (const SelfType& src)   { Assign(this, src); }

    // Set a new or existing value under the key, to the value.
    // Pass a different class of 'key' so that assignment reference object
    // can be passed instead of the actual object.
    template<class CRef>
    void Set(const CRef& key)
    {
        BaseType::Set(this, key);
    }

    template<class CRef>
    inline void Add(const CRef& key)
    {
        BaseType::Add(this, key);
    }

    // Resize the GHashSet table to fit one more Entry.  Often this
    // doesn't involve any action.
    void CheckExpand()  
    {
        BaseType::CheckExpand(this);
    }

    // Hint the bucket count to >= n.
    void Resize(UPInt n)    
    {
        BaseType::SetCapacity(this, n);
    }

    // Size the GHashSet so that it can comfortably contain the given
    // number of elements.  If the GHashSet already contains more
    // elements than newSize, then this may be a no-op.
    void SetCapacity(UPInt newSize)
    {
        BaseType::SetCapacity(this, newSize);
    }

};

// GHashSet for local member only allocation (auto-heap).
template<class C, class HashF = GFixedSizeHash<C>,
         class AltHashF = HashF, 
         int SID = GStat_Default_Mem,         
         class Entry = GHashsetCachedEntry<C, HashF> >
class GHashSetLH : public GHashSet<C, HashF, AltHashF, GAllocatorLH<C, SID>, Entry>
{
public:
    typedef GHashSetLH<C, HashF, AltHashF, SID, Entry>                  SelfType;
    typedef GHashSet<C, HashF, AltHashF, GAllocatorLH<C, SID>, Entry >  BaseType;

    // Delegated constructors.
    GHashSetLH()                                      { }
    GHashSetLH(int sizeHint) : BaseType(sizeHint)     { }
    GHashSetLH(const SelfType& src) : BaseType(src)   { }
    ~GHashSetLH()                                     { }
    
    void    operator = (const SelfType& src)
    {
        BaseType::operator = (src);
    }
};

template<class C, class HashF = GFixedSizeHash<C>,
         class AltHashF = HashF, 
         int SID = GStat_Default_Mem,         
         class Entry = GHashsetCachedEntry<C, HashF> >
class GHashSetDH : public GHashSetBase<C, HashF, AltHashF, GAllocatorDH<C, SID>, Entry>
{
    void* pHeap;
public:
    typedef GHashSetDH<C, HashF, AltHashF, SID, Entry>                      SelfType;
    typedef GHashSetBase<C, HashF, AltHashF, GAllocatorDH<C, SID>, Entry>   BaseType;

    explicit GHashSetDH(void* pheap) : pHeap(pheap)                                 {   }
    GHashSetDH(void* pheap, int sizeHint) : BaseType(pheap, sizeHint), pHeap(pheap) {   }
    GHashSetDH(const SelfType& src) : BaseType(src), pHeap(src.pHeap)               {   }
    ~GHashSetDH()                                     {   }

    void operator = (const SelfType& src)
    { 
        Assign(src.pHeap, src); 
        pHeap = src.pHeap;
    }

    // Set a new or existing value under the key, to the value.
    // Pass a different class of 'key' so that assignment reference object
    // can be passed instead of the actual object.
    template<class CRef>
    void Set(const CRef& key)
    {
        BaseType::Set(pHeap, key);
    }

    template<class CRef>
    inline void Add(const CRef& key)
    {
        BaseType::Add(pHeap, key);
    }

    // Resize the GHashSet table to fit one more Entry.  Often this
    // doesn't involve any action.
    void CheckExpand()  
    {
        BaseType::CheckExpand(this);
    }

    // Hint the bucket count to >= n.
    void Resize(UPInt n)    
    {
        BaseType::SetCapacity(pHeap, n);
    }

    // Size the GHashSet so that it can comfortably contain the given
    // number of elements.  If the GHashSet already contains more
    // elements than newSize, then this may be a no-op.
    void SetCapacity(UPInt newSize)
    {
        BaseType::SetCapacity(pHeap, newSize);
    }

};

// GHashSet with uncached hash code; declared for convenience.
template<class C, class HashF = GFixedSizeHash<C>,
                  class AltHashF = HashF,
                  class Allocator = GAllocatorGH<C> >
class GHashSetUncached : public GHashSet<C, HashF, AltHashF, Allocator, GHashsetEntry<C, HashF> >
{
public:
    
    typedef GHashSetUncached<C, HashF, AltHashF, Allocator>                    SelfType;
    typedef GHashSet<C, HashF, AltHashF, Allocator, GHashsetEntry<C, HashF> >  BaseType;

    // Delegated constructors.
    GHashSetUncached()                                        { }
    GHashSetUncached(int sizeHint) : BaseType(sizeHint)       { }
    GHashSetUncached(const SelfType& src) : BaseType(src)     { }
    ~GHashSetUncached()                                       { }
    
    void    operator = (const SelfType& src)
    {
        BaseType::operator = (src);
    }
};

// GHashSet with uncached hash code, for local-only use.
template<class C, class HashF = GFixedSizeHash<C>,
                  class AltHashF = HashF,
                  int SID = GStat_Default_Mem >
class GHashSetUncachedLH : public GHashSet<C, HashF, AltHashF, GAllocatorLH<C, SID>, GHashsetEntry<C, HashF> >
{
public:
    
    typedef GHashSetUncachedLH<C, HashF, AltHashF, SID>                                   SelfType;
    typedef GHashSet<C, HashF, AltHashF, GAllocatorLH<C, SID>, GHashsetEntry<C, HashF> >  BaseType;

    // Delegated constructors.
    GHashSetUncachedLH()                                    { }
    GHashSetUncachedLH(int sizeHint) : BaseType(sizeHint)   { }
    GHashSetUncachedLH(const SelfType& src) : BaseType(src) { }
    ~GHashSetUncachedLH()                                   { }
    
    void    operator = (const SelfType& src)
    {
        BaseType::operator = (src);
    }
};

// GHashSet with uncached hash code, for using with custom heap.
template<class C, 
         class HashF = GFixedSizeHash<C>,
         class AltHashF = HashF,
         int SID = GStat_Default_Mem >
class GHashSetUncachedDH : public GHashSetDH<C, HashF, AltHashF, SID, GHashsetEntry<C, HashF> >
{
public:

    typedef GHashSetUncachedDH<C, HashF, AltHashF, SID>                     SelfType;
    typedef GHashSetDH<C, HashF, AltHashF, SID, GHashsetEntry<C, HashF> >   BaseType;

    // Delegated constructors.
    explicit GHashSetUncachedDH(void* pheap) : BaseType(pheap)                  { }
    GHashSetUncachedDH(void* pheap, int sizeHint) : BaseType(pheap, sizeHint)   { }
    GHashSetUncachedDH(const SelfType& src) : BaseType(src) { }
    ~GHashSetUncachedDH()                                   { }

    void    operator = (const SelfType& src)
    {
        BaseType::operator = (src);
    }
};



// ***** GHash hash table implementation

// Node for GHash - necessary so that GHash can delegate its implementation
// to GHashSet.
template<class C, class U, class HashF>
struct GHashNode
{
    typedef GHashNode<C, U, HashF>      SelfType;
    typedef C                           FirstType;
    typedef U                           SecondType;

    C   First;
    U   Second;

    // NodeRef is used to allow passing of elements into GHashSet
    // without using a temporary object.
    struct NodeRef
    {
        const C*   pFirst;
        const U*   pSecond;

        NodeRef(const C& f, const U& s) : pFirst(&f), pSecond(&s) { }
        NodeRef(const NodeRef& src)     : pFirst(src.pFirst), pSecond(src.pSecond) { }

        // Enable computation of ghash_node_hashf.
        inline UPInt GetHash() const            { return HashF()(*pFirst); } 
        // Necessary conversion to allow GHashNode::operator == to work.
        operator const C& () const              { return *pFirst; }
    };

    // Note: No default constructor is necessary.
     GHashNode(const GHashNode& src) : First(src.First), Second(src.Second)    { }
     GHashNode(const NodeRef& src) : First(*src.pFirst), Second(*src.pSecond)  { }
    void operator = (const NodeRef& src)  { First  = *src.pFirst; Second = *src.pSecond; }

#ifdef GFC_CC_RENESAS
    bool operator == (const NodeRef& src) const { return (First == *src.pFirst); }
#endif

    template<class K>
    bool operator == (const K& src) const   { return (First == src); }

    template<class K>
    static UPInt CalcHash(const K& data)   { return HashF()(data); }
    inline UPInt GetHash() const           { return HashF()(First); }

    // Hash functors used with this node. A separate functor is used for alternative
    // key lookup so that it does not need to access the '.First' element.    
    struct NodeHashF
    {    
        template<class K>
        UPInt operator()(const K& data) const { return data.GetHash(); } 
    };    
    struct NodeAltHashF
    {
        template<class K>
        UPInt operator()(const K& data) const { return GHashNode<C,U,HashF>::CalcHash(data); }
    };
};



// **** Extra hashset_entry types to allow NodeRef construction.

// The big difference between the below types and the ones used in hash_set is that
// these allow initializing the node with 'typename C::NodeRef& keyRef', which
// is critical to avoid temporary node allocation on stack when using placement new.

// Compact hash table Entry type that re-computes hash keys during hash traversal.
// Good to use if the hash function is cheap or the hash value is already cached in C.
template<class C, class HashF>
class GHashsetNodeEntry
{
public:
    // Internal chaining for collisions.
    SPInt NextInChain;
    C     Value;

    GHashsetNodeEntry()
        : NextInChain(-2) { }
    GHashsetNodeEntry(const GHashsetNodeEntry& e)
        : NextInChain(e.NextInChain), Value(e.Value) { }
    GHashsetNodeEntry(const C& key, SPInt next)
        : NextInChain(next), Value(key) { }    
    GHashsetNodeEntry(const typename C::NodeRef& keyRef, SPInt next)
        : NextInChain(next), Value(keyRef) { }

    bool    IsEmpty() const             { return NextInChain == -2;  }
    bool    IsEndOfChain() const        { return NextInChain == -1;  }
    UPInt   GetCachedHash(UPInt maskValue) const  { return HashF()(Value) & maskValue; }
    void    SetCachedHash(UPInt hashValue)        { GUNUSED(hashValue); }

    void    Clear()
    {        
        Value.~C(); // placement delete
        NextInChain = -2;
    }
    // Free is only used from dtor of hash; Clear is used during regular operations:
    // assignment, hash reallocations, value reassignments, so on.
    void    Free() { Clear(); }
};

// Hash table Entry type that caches the Entry hash value for nodes, so that it
// does not need to be re-computed during access.
template<class C, class HashF>
class GHashsetCachedNodeEntry
{
public:
    // Internal chaining for collisions.
    SPInt NextInChain;
    UPInt HashValue;
    C     Value;

    GHashsetCachedNodeEntry()
        : NextInChain(-2) { }
    GHashsetCachedNodeEntry(const GHashsetCachedNodeEntry& e)
        : NextInChain(e.NextInChain), HashValue(e.HashValue), Value(e.Value) { }
    GHashsetCachedNodeEntry(const C& key, SPInt next)
        : NextInChain(next), Value(key) { }
    GHashsetCachedNodeEntry(const typename C::NodeRef& keyRef, SPInt next)
        : NextInChain(next), Value(keyRef) { }

    bool    IsEmpty() const            { return NextInChain == -2;  }
    bool    IsEndOfChain() const       { return NextInChain == -1;  }
    UPInt   GetCachedHash(UPInt maskValue) const  { GUNUSED(maskValue); return HashValue; }
    void    SetCachedHash(UPInt hashValue)        { HashValue = hashValue; }

    void    Clear()
    {
        Value.~C();
        NextInChain = -2;
    }
    // Free is only used from dtor of hash; Clear is used during regular operations:
    // assignment, hash reallocations, value reassignments, so on.
    void    Free() { Clear(); }
};


template<class C, class U,
         class HashF = GFixedSizeHash<C>,
         class Allocator = GAllocatorGH<C>,
         class HashNode = GHashNode<C,U,HashF>,
         class Entry = GHashsetCachedNodeEntry<HashNode, typename HashNode::NodeHashF>,
         class Container =  GHashSet<HashNode, typename HashNode::NodeHashF,
             typename HashNode::NodeAltHashF, Allocator,
             Entry> >
class GHash
{
public:
    GFC_MEMORY_REDEFINE_NEW(GHash, Allocator::StatId)

    // Types used for hash_set.
    typedef GHash<C, U, HashF, Allocator, HashNode, Entry, Container>    SelfType;

    // Actual hash table itself, implemented as hash_set.
    Container   Hash;

public:
    GHash()     {  }
    GHash(int sizeHint) : Hash(sizeHint)                        { }
    explicit GHash(void* pheap) : Hash(pheap)                   { }
    GHash(void* pheap, int sizeHint) : Hash(pheap, sizeHint)    { }
    GHash(const SelfType& src) : Hash(src.Hash)                 { }
    ~GHash()                                                    { }

    void    operator = (const SelfType& src)    { Hash = src.Hash; }

    // Remove all entries from the GHash table.
    inline void    Clear() { Hash.Clear(); }
    // Returns true if the GHash is empty.
    inline bool    IsEmpty() const { return Hash.IsEmpty(); }

    // Access (set).
    inline void    Set(const C& key, const U& value)
    {
        typename HashNode::NodeRef e(key, value);
        Hash.Set(e);
    }
    inline void    Add(const C& key, const U& value)
    {
        typename HashNode::NodeRef e(key, value);
        Hash.Add(e);
    }

    // Removes an element by clearing its Entry.
    inline void     Remove(const C& key)
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
    //  - If there is a value, return true, and Set *Pvalue to the Entry's value.
    //  - If value == NULL, return true or false according to the presence of the key.    
    bool    Get(const C& key, U* pvalue) const   
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
    inline U*  Get(const C& key)
    {
        HashNode* p = Hash.GetAlt(key);
        return p ? &p->Second : 0;
    }
    inline const U* Get(const C& key) const
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
    inline UPInt   GetSize() const              { return Hash.GetSize(); }    
    inline void    Resize(UPInt n)              { Hash.Resize(n); }
    inline void    SetCapacity(UPInt newSize)   { Hash.SetCapacity(newSize); }

    // Iterator API, like STL.
    typedef typename Container::ConstIterator   ConstIterator;
    typedef typename Container::Iterator        Iterator;

    inline Iterator        Begin()              { return Hash.Begin(); }
    inline Iterator        End()                { return Hash.End(); }
    inline ConstIterator   Begin() const        { return Hash.Begin(); }
    inline ConstIterator   End() const          { return Hash.End();   }

    Iterator        Find(const C& key)          { return Hash.FindAlt(key);  }
    ConstIterator   Find(const C& key) const    { return Hash.FindAlt(key);  }

    template<class K>
    Iterator        FindAlt(const K& key)       { return Hash.FindAlt(key);  }
    template<class K>
    ConstIterator   FindAlt(const K& key) const { return Hash.FindAlt(key);  }
};



// Local-only version of GHash
template<class C, class U,         
         class HashF = GFixedSizeHash<C>,
         int SID = GStat_Default_Mem,        
         class HashNode = GHashNode<C,U,HashF>,
         class Entry = GHashsetCachedNodeEntry<HashNode, typename HashNode::NodeHashF> >
class GHashLH
    : public GHash<C, U, HashF, GAllocatorLH<C, SID>, HashNode, Entry>
{
public:
    typedef GHashLH<C, U, HashF, SID, HashNode, Entry>                   SelfType;
    typedef GHash<C, U, HashF, GAllocatorLH<C, SID>, HashNode, Entry>    BaseType;

    // Delegated constructors.
    GHashLH()                                        { }
    GHashLH(int sizeHint) : BaseType(sizeHint)       { }
    GHashLH(const SelfType& src) : BaseType(src)     { }
    ~GHashLH()                                       { }
    void operator = (const SelfType& src)            { BaseType::operator = (src); }
};


// Custom-heap version of GHash
template<class C, class U,         
         class HashF = GFixedSizeHash<C>,
         int SID = GStat_Default_Mem,        
         class HashNode = GHashNode<C,U,HashF>,
         class Entry = GHashsetCachedNodeEntry<HashNode, typename HashNode::NodeHashF> >
class GHashDH
    : public GHash<C, U, HashF, GAllocatorDH<C, SID>, HashNode, Entry, 
                GHashSetDH<HashNode, typename HashNode::NodeHashF,
                    typename HashNode::NodeAltHashF, SID, Entry> >
{
public:
    typedef GHashDH<C, U, HashF, SID, HashNode, Entry>                  SelfType;
    typedef GHash<C, U, HashF, GAllocatorDH<C, SID>, HashNode, Entry, 
        GHashSetDH<HashNode, typename HashNode::NodeHashF,
            typename HashNode::NodeAltHashF, SID, Entry> >              BaseType;

    // Delegated constructors.
    explicit GHashDH(void* pheap) : BaseType(pheap)                        { }
    GHashDH(void* pheap, int sizeHint) : BaseType(pheap, sizeHint)         { }
    GHashDH(const SelfType& src) : BaseType(src)     { }
    ~GHashDH()                                       { }
    void operator = (const SelfType& src) { BaseType::operator = (src); }
};


// GHash with uncached hash code; declared for convenience.
template<class C, class U, class HashF = GFixedSizeHash<C>, class Allocator = GAllocatorGH<C> >
class GHashUncached
    : public GHash<C, U, HashF, Allocator, GHashNode<C,U,HashF>,
                   GHashsetNodeEntry<GHashNode<C,U,HashF>, typename GHashNode<C,U,HashF>::NodeHashF> >
{
public:
    typedef GHashUncached<C, U, HashF, Allocator>                                                     SelfType;
    typedef GHash<C, U, HashF, Allocator, GHashNode<C,U,HashF>,
                  GHashsetNodeEntry<GHashNode<C,U,HashF>, typename GHashNode<C,U,HashF>::NodeHashF> > BaseType;

    // Delegated constructors.
    GHashUncached()                                        { }
    GHashUncached(int sizeHint) : BaseType(sizeHint)       { }
    GHashUncached(const SelfType& src) : BaseType(src)     { }
    ~GHashUncached()                                       { }
    void operator = (const SelfType& src)                  { BaseType::operator = (src); }
};


// Local heap member-only hash
template<class C, class U, class HashF = GFixedSizeHash<C>, int StatId = GStat_Default_Mem>
class GHashUncachedLH
    : public GHashLH<C, U, HashF, StatId, GHashNode<C,U,HashF>,
                   GHashsetNodeEntry<GHashNode<C,U,HashF>, typename GHashNode<C,U,HashF>::NodeHashF> >
{
public:
    typedef GHashUncachedLH<C, U, HashF, StatId>                                            SelfType;
    typedef GHashLH<C, U, HashF, StatId, GHashNode<C,U,HashF>,
        GHashsetNodeEntry<GHashNode<C,U,HashF>, typename GHashNode<C,U,HashF>::NodeHashF> > BaseType;

    // Delegated constructors.
    GHashUncachedLH()                                        { }
    GHashUncachedLH(int sizeHint) : BaseType(sizeHint)       { }
    GHashUncachedLH(const SelfType& src) : BaseType(src)     { }
    ~GHashUncachedLH()                                       { }
    void operator = (const SelfType& src)                    { BaseType::operator = (src); }
};

// Custom-heap hash
template<class C, class U, class HashF = GFixedSizeHash<C>, int StatId = GStat_Default_Mem>
class GHashUncachedDH
    : public GHashDH<C, U, HashF, StatId, GHashNode<C,U,HashF>,
    GHashsetNodeEntry<GHashNode<C,U,HashF>, typename GHashNode<C,U,HashF>::NodeHashF> >
{
public:
    typedef GHashUncachedDH<C, U, HashF, StatId>                                            SelfType;
    typedef GHashDH<C, U, HashF, StatId, GHashNode<C,U,HashF>,
        GHashsetNodeEntry<GHashNode<C,U,HashF>, typename GHashNode<C,U,HashF>::NodeHashF> > BaseType;

    // Delegated constructors.
    explicit GHashUncachedDH(void* pheap) : BaseType(pheap)  { }
    GHashUncachedDH(void* pheap, int sizeHint) : BaseType(pheap, sizeHint) { }
    GHashUncachedDH(const SelfType& src) : BaseType(src)     { }
    ~GHashUncachedDH()                                       { }
    void operator = (const SelfType& src)                    { BaseType::operator = (src); }
};

// And identity hash in which keys serve as hash value. Can be uncached,
// since hash computation is assumed cheap.
template<class C, class U, class Allocator = GAllocatorGH<C>, class HashF = GIdentityHash<C> >
class GHashIdentity
    : public GHashUncached<C, U, HashF, Allocator>
{
public:
    typedef GHashIdentity<C, U, Allocator, HashF> SelfType;
    typedef GHashUncached<C, U, HashF, Allocator> BaseType;

    // Delegated constructors.
    GHashIdentity()                                        { }
    GHashIdentity(int sizeHint) : BaseType(sizeHint)       { }
    GHashIdentity(const SelfType& src) : BaseType(src)     { }
    ~GHashIdentity()                                       { }
    void operator = (const SelfType& src)                  { BaseType::operator = (src); }
};



// Local member-only heap hash identity
template<class C, class U, int SID = GStat_Default_Mem, class HashF = GIdentityHash<C> >
class GHashIdentityLH
    : public GHashUncachedLH<C, U, HashF, SID>
{
public:
    typedef GHashIdentityLH<C, U, SID, HashF>               SelfType;
    typedef GHashUncachedLH<C, U, HashF, SID>               BaseType;

    // Delegated constructors.
    GHashIdentityLH()                                       { }
    GHashIdentityLH(int sizeHint) : BaseType(sizeHint)      { }
    GHashIdentityLH(const SelfType& src) : BaseType(src)    { }
    ~GHashIdentityLH()                                      { }
    void operator = (const SelfType& src)                   { BaseType::operator = (src); }
};

// Hash identity with specified heap
template<class C, class U, int SID = GStat_Default_Mem, class HashF = GIdentityHash<C> >
class GHashIdentityDH
    : public GHashUncachedDH<C, U, HashF, SID>
{
public:
    typedef GHashIdentityDH<C, U, SID, HashF>               SelfType;
    typedef GHashUncachedDH<C, U, HashF, SID>               BaseType;

    // Delegated constructors.
    explicit GHashIdentityDH(void* pheap) : BaseType(pheap) { }
    GHashIdentityDH(void* pheap, int sizeHint) : BaseType(pheap, sizeHint)      { }
    GHashIdentityDH(const SelfType& src) : BaseType(src)    { }
    ~GHashIdentityDH()                                      { }
    void operator = (const SelfType& src)                   { BaseType::operator = (src); }
};


// Redefine operator 'new' if necessary.
#if defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif


#endif
