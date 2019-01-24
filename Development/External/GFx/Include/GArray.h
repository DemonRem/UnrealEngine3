/**********************************************************************

Filename    :   GArray.h
Content     :   Template implementation for GArray
Created     :   August 20, 2001
Authors     :   Michael Antonov, Maxim Shemanarev
Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GArray_H
#define INC_GArray_H

#include "GAllocator.h"

#undef new

// ***** GArrayDefaultPolicy
//
// Default resize behavior. No minimal capacity, Granularity=4, 
// Shrinking as needed. GArrayConstPolicy actually is the same as 
// GArrayDefaultPolicy, but parametrized with constants. 
// This struct is used only in order to reduce the template "matroska".
//------------------------------------------------------------------------
struct GArrayDefaultPolicy
{
    GArrayDefaultPolicy() : Capacity(0) {}
    GArrayDefaultPolicy(const GArrayDefaultPolicy&) : Capacity(0) {}

    UPInt GetMinCapacity() const { return 0; }
    UPInt GetGranularity() const { return 4; }
    bool  NeverShrinking() const { return 0; }

    UPInt GetCapacity()    const      { return Capacity; }
    void  SetCapacity(UPInt capacity) { Capacity = capacity; }
private:
    UPInt Capacity;
};


// ***** GArrayConstPolicy
//
// Statically parametrized resizing behavior:
// MinCapacity, Granularity, and Shrinking flag.
//------------------------------------------------------------------------
template<int MinCapacity=0, int Granularity=4, bool NeverShrink=false>
struct GArrayConstPolicy
{
    typedef GArrayConstPolicy<MinCapacity, Granularity, NeverShrink> SelfType;

    GArrayConstPolicy() : Capacity(0) {}
    GArrayConstPolicy(const SelfType&) : Capacity(0) {}

    UPInt GetMinCapacity() const { return MinCapacity; }
    UPInt GetGranularity() const { return Granularity; }
    bool  NeverShrinking() const { return NeverShrink; }

    UPInt GetCapacity()    const      { return Capacity; }
    void  SetCapacity(UPInt capacity) { Capacity = capacity; }
private:
    UPInt Capacity;
};


// ***** GArrayDataBase
//
// Basic operations with array data: Reserve, Resize, Free, GArrayPolicy.
// For internal use only: GArrayData, GArrayDataDH, GArrayDataCC and others.
//------------------------------------------------------------------------
template<class T, class Allocator, class SizePolicy> struct GArrayDataBase
{
    typedef T                                           ValueType;
    typedef Allocator                                   AllocatorType;
    typedef SizePolicy                                  SizePolicyType;
    typedef GArrayDataBase<T, Allocator, SizePolicy>    SelfType;

    GArrayDataBase()
        : Data(0), Size(0), Policy() {}

    GArrayDataBase(const SizePolicy& p)
        : Data(0), Size(0), Policy(p) {}

    ~GArrayDataBase() 
    {
        Allocator::DestructArray(Data, Size);
        Allocator::Free(Data);
    }

    UPInt GetCapacity() const 
    { 
        return Policy.GetCapacity(); 
    }

    void ClearAndRelease()
    {
        Allocator::DestructArray(Data, Size);
        Allocator::Free(Data);
        Data = 0;
        Size = 0;
        Policy.SetCapacity(0);
    }

    void Reserve(const void* pheapAddr, UPInt newCapacity)
    {
        if (Policy.NeverShrinking() && newCapacity < GetCapacity())
            return;

        if (newCapacity < Policy.GetMinCapacity())
            newCapacity = Policy.GetMinCapacity();

        // Resize the buffer.
        if (newCapacity == 0)
        {
            if (Data)
            {
                Allocator::Free(Data);
                Data = 0;
            }
            Policy.SetCapacity(0);
        }
        else
        {
            UPInt gran = Policy.GetGranularity();
            newCapacity = (newCapacity + gran - 1) / gran * gran;
            if (Data)
            {
                if (Allocator::IsMovable())
                {
                    Data = (T*)Allocator::Realloc(Data, sizeof(T) * newCapacity, __FILE__, __LINE__);
                }
                else
                {
                    T* newData = (T*)Allocator::Alloc(pheapAddr, sizeof(T) * newCapacity, __FILE__, __LINE__);
                    for (UPInt i = 0; i < Size; ++i)
                    {
                        Allocator::Construct(&newData[i], Data[i]);
                        Allocator::Destruct(&Data[i]);
                    }
                    Allocator::Free(Data);
                    Data = newData;
                }
            }
            else
            {
                Data = (T*)Allocator::Alloc(pheapAddr, sizeof(T) * newCapacity, __FILE__, __LINE__);
                //memset(Buffer, 0, (sizeof(ValueType) * newSize)); // Do we need this?
            }
            Policy.SetCapacity(newCapacity);
            // GASSERT(Data); // need to throw (or something) on alloc failure!
        }
    }

    // This version of Resize DOES NOT construct the elements.
    // It's done to optimize PushBack, which uses a copy constructor 
    // instead of the default constructor and assignment
    void ResizeNoConstruct(const void* pheapAddr, UPInt newSize)
    {
        UPInt oldSize = Size;

        if (newSize < oldSize)
        {
            Allocator::DestructArray(Data + newSize, oldSize - newSize);
            if (newSize < (Policy.GetCapacity() >> 1))
            {
                Reserve(pheapAddr, newSize);
            }
        }
        else if(newSize >= Policy.GetCapacity())
        {
            Reserve(pheapAddr, newSize + (newSize >> 2));
        }
        //! IMPORTANT to modify Size only after Reserve completes, because garbage collectable
        // array may use this array and may traverse it during Reserve (in the case, if 
        // collection occurs becuase of heap limit exceeded).
        Size = newSize;
    }

    ValueType*  Data;
    UPInt       Size;
    SizePolicy  Policy;
};




// ***** GArrayData
//
// General purpose array data.
// For internal use only in GArray, GArrayLH, GArrayPOD and so on.
//------------------------------------------------------------------------
template<class T, class Allocator, class SizePolicy> struct GArrayData : 
GArrayDataBase<T, Allocator, SizePolicy>
{
    typedef T ValueType;
    typedef Allocator                                   AllocatorType;
    typedef SizePolicy                                  SizePolicyType;
    typedef GArrayDataBase<T, Allocator, SizePolicy>    BaseType;
    typedef GArrayData    <T, Allocator, SizePolicy>    SelfType;

    GArrayData()
        : BaseType() { }

    GArrayData(int size)
        : BaseType() { Resize(size); }

    GArrayData(const SelfType& a)
        : BaseType(a.Policy) { Append(a.Data, a.Size); }

    void Reserve(UPInt newCapacity)
    {
        BaseType::Reserve(this, newCapacity);
    }

    void Resize(UPInt newSize)
    {
        UPInt oldSize = this->Size;
        BaseType::ResizeNoConstruct(this, newSize);
        if(newSize > oldSize)
            Allocator::ConstructArray(this->Data + oldSize, newSize - oldSize);
    }

    void PushBack(const ValueType& val)
    {
        BaseType::ResizeNoConstruct(this, this->Size + 1);
        Allocator::Construct(this->Data + this->Size - 1, val);
    }

    template<class S>
    void PushBackAlt(const S& val)
    {
        BaseType::ResizeNoConstruct(this, this->Size + 1);
        Allocator::ConstructAlt(this->Data + this->Size - 1, val);
    }

    // Append the given data to the array.
    void Append(const ValueType other[], UPInt count)
    {
        if (count)
        {
            UPInt oldSize = this->Size;
            BaseType::ResizeNoConstruct(this, this->Size + count);
            Allocator::ConstructArray(this->Data + oldSize, count, other);
        }
    }
};




// ***** GArrayDataDH
//
// General purpose array data with a heap pointer
// For internal use only in GArrayDH, GArrayDH_POD and so on.
//------------------------------------------------------------------------
template<class T, class Allocator, class SizePolicy> struct GArrayDataDH : 
GArrayDataBase<T, Allocator, SizePolicy>
{
    typedef T                                           ValueType;
    typedef Allocator                                   AllocatorType;
    typedef SizePolicy                                  SizePolicyType;
    typedef GArrayDataBase<T, Allocator, SizePolicy>    BaseType;
    typedef GArrayDataDH  <T, Allocator, SizePolicy>    SelfType;

    GArrayDataDH(GMemoryHeap* heap)
        : BaseType(), pHeap(heap) { }

    GArrayDataDH(GMemoryHeap* heap, int size)
        : BaseType(), pHeap(heap) { Resize(size); }

    GArrayDataDH(const SelfType& a)
        : BaseType(a.Policy), pHeap(a.pHeap) { Append(a.Data, a.Size); }

    void Reserve(UPInt newCapacity)
    {
        BaseType::Reserve(pHeap, newCapacity);
    }

    void Resize(UPInt newSize)
    {
        UPInt oldSize = this->Size;
        BaseType::ResizeNoConstruct(pHeap, newSize);
        if(newSize > oldSize)
            Allocator::ConstructArray(this->Data + oldSize, newSize - oldSize);
    }

    void PushBack(const ValueType& val)
    {
        BaseType::ResizeNoConstruct(pHeap, this->Size + 1);
        Allocator::Construct(this->Data + this->Size - 1, val);
    }

    template<class S>
    void PushBackAlt(const S& val)
    {
        BaseType::ResizeNoConstruct(pHeap, this->Size + 1);
        Allocator::ConstructAlt(this->Data + this->Size - 1, val);
    }

    // Append the given data to the array.
    void Append(const ValueType other[], UPInt count)
    {
        if (count > 0)
        {
            UPInt oldSize = this->Size;
            BaseType::ResizeNoConstruct(pHeap, this->Size + count);
            Allocator::ConstructArray(this->Data + oldSize, count, other);
        }
    }

    const GMemoryHeap* pHeap;
};




// ***** GArrayDataCC
//
// A modification of GArrayData that always copy-constructs new elements
// using a specified DefaultValue. For internal use only in GArrayCC.
//------------------------------------------------------------------------
template<class T, class Allocator, class SizePolicy> struct GArrayDataCC : 
GArrayDataBase<T, Allocator, SizePolicy>
{
    typedef T                                           ValueType;
    typedef Allocator                                   AllocatorType;
    typedef SizePolicy                                  SizePolicyType;
    typedef GArrayDataBase<T, Allocator, SizePolicy>    BaseType;
    typedef GArrayDataCC  <T, Allocator, SizePolicy>    SelfType;

    GArrayDataCC(const ValueType& defval)
        : BaseType(), DefaultValue(defval) { }

    GArrayDataCC(const ValueType& defval, int size)
        : BaseType(), DefaultValue(defval) { Resize(size); }

    GArrayDataCC(const SelfType& a)
        : BaseType(a.Policy), DefaultValue(a.DefaultValue) { Append(a.Data, a.Size); }

    void Reserve(UPInt newCapacity)
    {
        BaseType::Reserve(this, newCapacity);
    }

    void Resize(UPInt newSize)
    {
        UPInt oldSize = this->Size;
        BaseType::ResizeNoConstruct(this, newSize);
        if(newSize > oldSize)
            Allocator::ConstructArray(this->Data + oldSize, newSize - oldSize, DefaultValue);
    }

    void PushBack(const ValueType& val)
    {
        BaseType::ResizeNoConstruct(this, this->Size + 1);
        Allocator::Construct(this->Data + this->Size - 1, val);
    }

    template<class S>
    void PushBackAlt(const S& val)
    {
        BaseType::ResizeNoConstruct(this, this->Size + 1);
        Allocator::ConstructAlt(this->Data + this->Size - 1, val);
    }

    // Append the given data to the array.
    void Append(const ValueType other[], UPInt count)
    {
        if (count)
        {
            UPInt oldSize = this->Size;
            BaseType::ResizeNoConstruct(this, this->Size + count);
            Allocator::ConstructArray(this->Data + oldSize, count, other);
        }
    }

    ValueType   DefaultValue;
};











// ***** GArrayBase
//
// Resizable array. The behavior can be POD (suffix _POD) and 
// Movable (no suffix) depending on the allocator policy.
// In case of _POD the constructors and destructors are not called.
// 
// GArrays can't handle non-movable objects! Don't put anything in here 
// that can't be moved around by bitwise copy. 
// 
// The addresses of elements are not persistent! Don't keep the address 
// of an element; the array contents will move around as it gets resized.
//------------------------------------------------------------------------
template<class ArrayData> class GArrayBase
{
public:
    typedef typename ArrayData::ValueType       ValueType;
    typedef typename ArrayData::AllocatorType   AllocatorType;
    typedef typename ArrayData::SizePolicyType  SizePolicyType;
    typedef GArrayBase<ArrayData>               SelfType;

    GFC_MEMORY_REDEFINE_NEW(GArrayBase, AllocatorType::StatId)

    GArrayBase()
        : Data() {}
    GArrayBase(int size)
        : Data(size) {}
    GArrayBase(const SelfType& a)
        : Data(a.Data) {}

    GArrayBase(GMemoryHeap* heap)
        : Data(heap) {}
    GArrayBase(GMemoryHeap* heap, int size)
        : Data(heap, size) {}

    GArrayBase(const ValueType& defval)
        : Data(defval) {}
    GArrayBase(const ValueType& defval, int size)
        : Data(defval, size) {}
  
    SizePolicyType* GetSizePolicy() const                  { return Data.Policy; }
    void            SetSizePolicy(const SizePolicyType& p) { Data.Policy = p; }

    bool    NeverShrinking()const       { return Data.Policy.NeverShrinking(); }
    UPInt   GetSize()       const       { return Data.Size;  }
    UPInt   GetCapacity()   const       { return Data.GetCapacity(); }
    UPInt   GetNumBytes()   const       { return Data.GetCapacity() * sizeof(ValueType); }

    void    ClearAndRelease()           { Data.ClearAndRelease(); }
    void    Clear()                     { Data.Resize(0); }
    void    Resize(UPInt newSize)       { Data.Resize(newSize); }

    // Reserve can only increase the capacity
    void    Reserve(UPInt newCapacity)  
    { 
        if (newCapacity > Data.GetCapacity())
            Data.Reserve(newCapacity); 
    }

    // Basic access.
    ValueType& At(UPInt index)
    {
        GASSERT(index < Data.Size);
        return Data.Data[index]; 
    }
    const ValueType& At(UPInt index) const
    {
        GASSERT(index < Data.Size);
        return Data.Data[index]; 
    }

    ValueType ValueAt(UPInt index) const
    {
        GASSERT(index < Data.Size);
        return Data.Data[index]; 
    }

    // Basic access.
    ValueType& operator [] (UPInt index)
    {
        GASSERT(index < Data.Size);
        return Data.Data[index]; 
    }
    const ValueType& operator [] (UPInt index) const
    {
        GASSERT(index < Data.Size);
        return Data.Data[index]; 
    }

    // Raw pointer to the data. Use with caution!
    const ValueType* GetDataPtr() const { return Data.Data; }
          ValueType* GetDataPtr()       { return Data.Data; }

    // Insert the given element at the end of the array.
    void    PushBack(const ValueType& val)
    {
        // DO NOT pass elements of your own vector into
        // push_back()!  Since we're using references,
        // resize() may munge the element storage!
        // GASSERT(&val < &Buffer[0] || &val > &Buffer[BufferSize]);
        Data.PushBack(val);
    }

    template<class S>
    void PushBackAlt(const S& val)
    {
        Data.PushBackAlt(val);
    }

    // Remove the last element.
    void    PopBack()
    {
        GASSERT(Data.Size > 0);
        Data.Resize(Data.Size - 1);
    }

    // Access the first element.
          ValueType&    Front()         { return At(0); }
    const ValueType&    Front() const   { return At(0); }

    // Access the last element.
          ValueType&    Back()          { return At(Data.Size - 1); }
    const ValueType&    Back() const    { return At(Data.Size - 1); }

    // Array copy.  Copies the contents of a into this array.
    const SelfType& operator = (const SelfType& a)   
    {
        Resize(a.GetSize());
        for (UPInt i = 0; i < Data.Size; i++) {
            *(Data.Data + i) = a[i];
        }
        return *this;
    }

    // Removing multiple elements from the array.
    void    RemoveMultipleAt(UPInt index, UPInt num)
    {
        GASSERT(index + num <= Data.Size);
        if (Data.Size == num)
        {
            Clear();
        }
        else
        {
            AllocatorType::DestructArray(Data.Data + index, num);
            AllocatorType::CopyArrayForward(
                Data.Data + index, 
                Data.Data + index + num,
                Data.Size - num - index);
            Data.Size -= num;
        }
    }

    // Removing an element from the array is an expensive operation!
    // It compacts only after removing the last element.
    void    RemoveAt(UPInt index)
    {
        GASSERT(index < Data.Size);
        if (Data.Size == 1)
        {
            Clear();
        }
        else
        {
            AllocatorType::Destruct(Data.Data + index);
            AllocatorType::CopyArrayForward(
                Data.Data + index, 
                Data.Data + index + 1,
                Data.Size - 1 - index);
            --Data.Size;
        }
    }

    // Insert the given object at the given index shifting all the elements up.
    void    InsertAt(UPInt index, const ValueType& val = ValueType())
    {
        GASSERT(index <= Data.Size);

        Data.Resize(Data.Size + 1);
        if (index < Data.Size - 1)
        {
            AllocatorType::CopyArrayBackward(
                Data.Data + index + 1, 
                Data.Data + index, 
                Data.Size - 1 - index);
        }
        AllocatorType::Construct(Data.Data + index, val);
    }

    // Insert the given object at the given index shifting all the elements up.
    void    InsertMultipleAt(UPInt index, UPInt num, const ValueType& val = ValueType())
    {
        GASSERT(index <= Data.Size);

        Data.Resize(Data.Size + num);
        if (index < Data.Size - num)
        {
            AllocatorType::CopyArrayBackward(
                Data.Data + index + num,
                Data.Data + index,
                Data.Size - num - index);
        }
        for (UPInt i = 0; i < num; ++i)
            AllocatorType::Construct(Data.Data + index + i, val);
    }

    // Append the given data to the array.
    void    Append(const SelfType& other)
    {
        Append(other.Data.Data, other.GetSize());
    }

    // Append the given data to the array.
    void    Append(const ValueType other[], UPInt count)
    {
        Data.Append(other, count);
    }

    class Iterator
    {
        SelfType*       pArray;
        SPInt           CurIndex;

    public:
        Iterator() : pArray(0), CurIndex(-1) {}
        Iterator(SelfType* parr, SPInt idx = 0) : pArray(parr), CurIndex(idx) {}

        bool operator==(const Iterator& it) const { return pArray == it.pArray && CurIndex == it.CurIndex; }
        bool operator!=(const Iterator& it) const { return pArray != it.pArray || CurIndex != it.CurIndex; }

        Iterator& operator++()
        {
            if (pArray)
            {
                if (CurIndex < (SPInt)pArray->GetSize())
                    ++CurIndex;
            }
            return *this;
        }
        Iterator operator++(int)
        {
            Iterator it(*this);
            operator++();
            return it;
        }
        Iterator& operator--()
        {
            if (pArray)
            {
                if (CurIndex >= 0)
                    --CurIndex;
            }
            return *this;
        }
        Iterator operator--(int)
        {
            Iterator it(*this);
            operator--();
            return it;
        }
        Iterator operator+(int delta) const
        {
            return Iterator(pArray, CurIndex + delta);
        }
        Iterator operator-(int delta) const
        {
            return Iterator(pArray, CurIndex - delta);
        }
        SPInt operator-(const Iterator& right) const
        {
            GASSERT(pArray == right.pArray);
            return CurIndex - right.CurIndex;
        }
        ValueType& operator*() const    { GASSERT(pArray); return  (*pArray)[CurIndex]; }
        ValueType* operator->() const   { GASSERT(pArray); return &(*pArray)[CurIndex]; }
        ValueType* GetPtr() const       { GASSERT(pArray); return &(*pArray)[CurIndex]; }

        bool IsFinished() const { return !pArray || CurIndex < 0 || CurIndex >= (int)pArray->GetSize(); }

        void Remove()
        {
            if (!IsFinished())
                pArray->RemoveAt(CurIndex);
        }

        SPInt GetIndex() const { return CurIndex; }
    };

    Iterator Begin() { return Iterator(this); }
    Iterator End()   { return Iterator(this, (SPInt)GetSize()); }
    Iterator Last()  { return Iterator(this, (SPInt)GetSize() - 1); }

    class ConstIterator
    {
        const SelfType* pArray;
        SPInt           CurIndex;

    public:
        ConstIterator() : pArray(0), CurIndex(-1) {}
        ConstIterator(const SelfType* parr, SPInt idx = 0) : pArray(parr), CurIndex(idx) {}

        bool operator==(const ConstIterator& it) const { return pArray == it.pArray && CurIndex == it.CurIndex; }
        bool operator!=(const ConstIterator& it) const { return pArray != it.pArray || CurIndex != it.CurIndex; }

        ConstIterator& operator++()
        {
            if (pArray)
            {
                if (CurIndex < (int)pArray->GetSize())
                    ++CurIndex;
            }
            return *this;
        }
        ConstIterator operator++(int)
        {
            ConstIterator it(*this);
            operator++();
            return it;
        }
        ConstIterator& operator--()
        {
            if (pArray)
            {
                if (CurIndex >= 0)
                    --CurIndex;
            }
            return *this;
        }
        ConstIterator operator--(int)
        {
            ConstIterator it(*this);
            operator--();
            return it;
        }
        ConstIterator operator+(int delta) const
        {
            return ConstIterator(pArray, CurIndex + delta);
        }
        ConstIterator operator-(int delta) const
        {
            return ConstIterator(pArray, CurIndex - delta);
        }
        SPInt operator-(const ConstIterator& right) const
        {
            GASSERT(pArray == right.pArray);
            return CurIndex - right.CurIndex;
        }
        const ValueType& operator*() const  { GASSERT(pArray); return  (*pArray)[CurIndex]; }
        const ValueType* operator->() const { GASSERT(pArray); return &(*pArray)[CurIndex]; }
        const ValueType* GetPtr() const     { GASSERT(pArray); return &(*pArray)[CurIndex]; }

        bool IsFinished() const { return !pArray || CurIndex < 0 || CurIndex >= (int)pArray->GetSize(); }

        SPInt GetIndex()  const { return CurIndex; }
    };
    ConstIterator Begin() const { return ConstIterator(this); }
    ConstIterator End() const   { return ConstIterator(this, (SPInt)GetSize()); }
    ConstIterator Last() const  { return ConstIterator(this, (SPInt)GetSize() - 1); }

protected:
    ArrayData   Data;
};




// ***** GArray
//
// General purpose array for movable objects that require explicit 
// construction/destruction. Global heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem, class SizePolicy=GArrayDefaultPolicy>
class GArray : public GArrayBase<GArrayData<T, GAllocatorGH<T, SID>, SizePolicy> >
{
public:
    typedef T                                                               ValueType;
    typedef GAllocatorGH<T, SID>                                            AllocatorType;
    typedef SizePolicy                                                      SizePolicyType;
    typedef GArray<T, SID, SizePolicy>                                      SelfType;
    typedef GArrayBase<GArrayData<T, GAllocatorGH<T, SID>, SizePolicy> >    BaseType;

    GArray() : BaseType() {}
    GArray(int size) : BaseType(size) {}
    GArray(const SizePolicyType& p) : BaseType() { SetSizePolicy(p); }
    GArray(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** GArrayPOD
//
// General purpose array for movable objects that DOES NOT require  
// construction/destruction. Constructors and destructors are not called! 
// Global heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem, class SizePolicy=GArrayDefaultPolicy>
class GArrayPOD : public GArrayBase<GArrayData<T, GAllocatorGH_POD<T, SID>, SizePolicy> >
{
public:
    typedef T                                                                   ValueType;
    typedef GAllocatorGH<T, SID>                                                AllocatorType;
    typedef SizePolicy                                                          SizePolicyType;
    typedef GArrayPOD<T, SID, SizePolicy>                                       SelfType;
    typedef GArrayBase<GArrayData<T, GAllocatorGH_POD<T, SID>, SizePolicy> >    BaseType;

    GArrayPOD() : BaseType() {}
    GArrayPOD(int size) : BaseType(size) {}
    GArrayPOD(const SizePolicyType& p) : BaseType() { SetSizePolicy(p); }
    GArrayPOD(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** GArrayCPP
//
// General purpose, fully C++ compliant array. Can be used with non-movable data.
// Global heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem, class SizePolicy=GArrayDefaultPolicy>
class GArrayCPP : public GArrayBase<GArrayData<T, GAllocatorGH_CPP<T, SID>, SizePolicy> >
{
public:
    typedef T                                                                   ValueType;
    typedef GAllocatorGH<T, SID>                                                AllocatorType;
    typedef SizePolicy                                                          SizePolicyType;
    typedef GArrayCPP<T, SID, SizePolicy>                                       SelfType;
    typedef GArrayBase<GArrayData<T, GAllocatorGH_CPP<T, SID>, SizePolicy> >    BaseType;

    GArrayCPP() : BaseType() {}
    GArrayCPP(int size) : BaseType(size) {}
    GArrayCPP(const SizePolicyType& p) : BaseType() { SetSizePolicy(p); }
    GArrayCPP(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** GArrayLH
//
// General purpose array for movable objects that require explicit 
// construction/destruction. Local heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem, class SizePolicy=GArrayDefaultPolicy>
class GArrayLH : public GArrayBase<GArrayData<T, GAllocatorLH<T, SID>, SizePolicy> >
{
public:
    typedef T                                                               ValueType;
    typedef GAllocatorLH<T, SID>                                            AllocatorType;
    typedef SizePolicy                                                      SizePolicyType;
    typedef GArrayLH<T, SID, SizePolicy>                                    SelfType;
    typedef GArrayBase<GArrayData<T, GAllocatorLH<T, SID>, SizePolicy> >    BaseType;

    GArrayLH() : BaseType() {}
    GArrayLH(int size) : BaseType(size) {}
    GArrayLH(const SizePolicyType& p) : BaseType() { SetSizePolicy(p); }
    GArrayLH(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** GArrayLH_POD
//
// General purpose array for movable objects that DOES NOT require  
// construction/destruction. Constructors and destructors are not called! 
// Local heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem, class SizePolicy=GArrayDefaultPolicy>
class GArrayLH_POD : public GArrayBase<GArrayData<T, GAllocatorLH_POD<T, SID>, SizePolicy> >
{
public:
    typedef T                                                                   ValueType;
    typedef GAllocatorLH<T, SID>                                                AllocatorType;
    typedef SizePolicy                                                          SizePolicyType;
    typedef GArrayLH_POD<T, SID, SizePolicy>                                    SelfType;
    typedef GArrayBase<GArrayData<T, GAllocatorLH_POD<T, SID>, SizePolicy> >    BaseType;

    GArrayLH_POD() : BaseType() {}
    GArrayLH_POD(int size) : BaseType(size) {}
    GArrayLH_POD(const SizePolicyType& p) : BaseType() { SetSizePolicy(p); }
    GArrayLH_POD(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};

// ***** GArrayDH
//
// General purpose array for movable objects that require explicit 
// construction/destruction. Dynamic heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem, class SizePolicy=GArrayDefaultPolicy>
class GArrayDH : public GArrayBase<GArrayDataDH<T, GAllocatorDH<T, SID>, SizePolicy> >
{
public:
    typedef T                                                               ValueType;
    typedef GAllocatorDH<T, SID>                                            AllocatorType;
    typedef SizePolicy                                                      SizePolicyType;
    typedef GArrayDH<T, SID, SizePolicy>                                    SelfType;
    typedef GArrayBase<GArrayDataDH<T, GAllocatorDH<T, SID>, SizePolicy> >  BaseType;

    GArrayDH(GMemoryHeap* heap) : BaseType(heap) {}
    GArrayDH(GMemoryHeap* heap, int size) : BaseType(heap, size) {}
    GArrayDH(GMemoryHeap* heap, const SizePolicyType& p) : BaseType(heap) { SetSizePolicy(p); }
    GArrayDH(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** GArrayDH_POD
//
// General purpose array for movable objects that DOES NOT require  
// construction/destruction. Constructors and destructors are not called! 
// Dynamic heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem, class SizePolicy=GArrayDefaultPolicy>
class GArrayDH_POD : public GArrayBase<GArrayDataDH<T, GAllocatorDH_POD<T, SID>, SizePolicy> >
{
public:
    typedef T                                                                   ValueType;
    typedef GAllocatorDH<T, SID>                                                AllocatorType;
    typedef SizePolicy                                                          SizePolicyType;
    typedef GArrayDH_POD<T, SID, SizePolicy>                                    SelfType;
    typedef GArrayBase<GArrayDataDH<T, GAllocatorDH_POD<T, SID>, SizePolicy> >  BaseType;

    GArrayDH_POD(GMemoryHeap* heap) : BaseType(heap) {}
    GArrayDH_POD(GMemoryHeap* heap, int size) : BaseType(heap, size) {}
    GArrayDH_POD(GMemoryHeap* heap, const SizePolicyType& p) : BaseType(heap) { SetSizePolicy(p); }
    GArrayDH_POD(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** GArrayCC
//
// A modification of the array that uses the given default value to
// construct the elements. The constructors and destructors are 
// properly called, the objects must be movable.
// Local heap is in use.
//------------------------------------------------------------------------
template<class T, int SID=GStat_Default_Mem, class SizePolicy=GArrayDefaultPolicy>
class GArrayCC : public GArrayBase<GArrayDataCC<T, GAllocatorLH<T, SID>, SizePolicy> >
{
public:
    typedef T                                                               ValueType;
    typedef GAllocatorLH<T, SID>                                            AllocatorType;
    typedef SizePolicy                                                      SizePolicyType;
    typedef GArrayCC<T, SID, SizePolicy>                                    SelfType;
    typedef GArrayBase<GArrayDataCC<T, GAllocatorLH<T, SID>, SizePolicy> >  BaseType;

    GArrayCC(const ValueType& defval) : BaseType(defval) {}
    GArrayCC(const ValueType& defval, int size) : BaseType(defval, size) {}
    GArrayCC(const ValueType& defval, const SizePolicyType& p) : BaseType(defval) { SetSizePolicy(p); }
    GArrayCC(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};

// Redefine operator 'new' if necessary.
#if defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif

#endif
