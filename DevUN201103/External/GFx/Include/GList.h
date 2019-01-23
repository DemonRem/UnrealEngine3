/**********************************************************************

Filename    :   GList.h
Content     :   Template implementation for doubly-connected linked GList
Created     :   2008
Authors     :   MaximShemanarev
Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GList_H
#define INC_GList_H

#include "GTypes.h"

// ***** GListNode
//
// Base class for the elements of the intrusive linked list.
// To store elements in the GList do:
//
// struct MyData : GListNode<MyData>
// {
//     . . .
// };
//------------------------------------------------------------------------
template<class T> struct GListNode
{
    T* pPrev;
    T* pNext;

    void Remove()
    {
        pPrev->pNext = pNext;
        pNext->pPrev = pPrev;
    }
};



// ***** GList
//
// Doubly linked intrusive list. 
// The data type must be derived from GListNode.
// 
// Adding:   PushFront(), PushBack().
// Removing: Remove() - the element must be in the list!
// Moving:   BringToFront(), SendToBack() - the element must be in the list!
//
// Iterating:
//    MyData* data = MyList.GetFirst();
//    while (!MyList.IsNull(data))
//    {
//        . . .
//        data = MyList.GetNext(data);
//    }
//
// Removing:
//    MyData* data = MyList.GetFirst();
//    while (!MyList.IsNull(data))
//    {
//        MyData* next = MyList.GetNext(data);
//        if (ToBeRemoved(data))
//             MyList.Remove(data);
//        data = next;
//    }
//
//------------------------------------------------------------------------
template<class T> class GList
{
public:
    typedef T ValueType;

    GList()
    {
        Root.pNext = Root.pPrev = (ValueType*)&Root;
    }

    void Clear()
    {
        Root.pNext = Root.pPrev = (ValueType*)&Root;
    }

    const ValueType* GetFirst() const { return Root.pNext; }
    const ValueType* GetLast () const { return Root.pPrev; }
          ValueType* GetFirst()       { return Root.pNext; }
          ValueType* GetLast ()       { return Root.pPrev; }

    bool IsEmpty()                   const { return Root.pNext == (ValueType*)&Root; }
    bool IsFirst(const ValueType* p) const { return p == Root.pNext; }
    bool IsLast (const ValueType* p) const { return p == Root.pPrev; }
    bool IsNull (const ValueType* p) const { return p == (const ValueType*)&Root; }

    inline static const ValueType* GetPrev(const ValueType* p) { return p->pPrev; }
    inline static const ValueType* GetNext(const ValueType* p) { return p->pNext; }
    inline static       ValueType* GetPrev(      ValueType* p) { return p->pPrev; }
    inline static       ValueType* GetNext(      ValueType* p) { return p->pNext; }

    void PushFront(ValueType* p)
    {
        p->pNext          =  Root.pNext;
        p->pPrev          = (ValueType*)&Root;
        Root.pNext->pPrev =  p;
        Root.pNext        =  p;
    }

    void PushBack(ValueType* p)
    {
        p->pPrev          =  Root.pPrev;
        p->pNext          = (ValueType*)&Root;
        Root.pPrev->pNext =  p;
        Root.pPrev        =  p;
    }

    static void Remove(ValueType* p)
    {
        p->pPrev->pNext = p->pNext;
        p->pNext->pPrev = p->pPrev;
    }

    void BringToFront(ValueType* p)
    {
        Remove(p);
        PushFront(p);
    }

    void SendToBack(ValueType* p)
    {
        Remove(p);
        PushBack(p);
    }

    // Appends the contents of the argument list to the front of this list;
    // items are removed from the argument list.
    void PushListToFront(GList<T>& src)
    {
        if (!src.IsEmpty())
        {
            ValueType* pfirst = src.GetFirst();
            ValueType* plast  = src.GetLast();
            src.Clear();
            plast->pNext   = Root.pNext;
            pfirst->pPrev  = (ValueType*)&Root;
            Root.pNext->pPrev = plast;
            Root.pNext        = pfirst;
        }
    }


private:
    // Copying is prohibited
    GList(const GList<T>&);
    const GList<T>& operator = (const GList<T>&);

    GListNode<ValueType> Root;
};


// ***** GList2
//
// Doubly linked intrusive list with data accessor.
// Used when the same data must be in two or more different linked lists. 
// GList2, unlike GList keeps the whole ValueType as the root element 
// (while GList keeps only GListNode<T>).
// 
//  struct MyData
//  {
//      MyData* pPrev1;
//      MyData* pNext1;
//      MyData* pPrev2;
//      MyData* pNext2;
//      . . .
//  };
//
//  struct MyData_Accessor1
//  {
//      static void SetPrev(MyData* self, MyData* what)  { self->pPrev1 = what; }
//      static void SetNext(MyData* self, MyData* what)  { self->pNext1 = what; }
//      static const MyData* GetPrev(const MyData* self) { return self->pPrev1; }
//      static const MyData* GetNext(const MyData* self) { return self->pNext1; }
//      static       MyData* GetPrev(MyData* self)       { return self->pPrev1; }
//      static       MyData* GetNext(MyData* self)       { return self->pNext1; }
//  };
//
//  struct MyData_Accessor2
//  {
//      static void SetPrev(MyData* self, MyData* what)  { self->pPrev2 = what; }
//      static void SetNext(MyData* self, MyData* what)  { self->pNext2 = what; }
//      static const MyData* GetPrev(const MyData* self) { return self->pPrev2; }
//      static const MyData* GetNext(const MyData* self) { return self->pNext2; }
//      static       MyData* GetPrev(MyData* self)       { return self->pPrev2; }
//      static       MyData* GetNext(MyData* self)       { return self->pNext2; }
//  };
//
//  GList2<MyData, MyData_Accessor1> List1;
//  GList2<MyData, MyData_Accessor2> List2;
//
//------------------------------------------------------------------------
template<class T, class Accessor> class GList2
{
public:
    typedef T ValueType;

    inline static void SetPrev(ValueType* self, ValueType* what)  { Accessor::SetPrev(self, what);  }
    inline static void SetNext(ValueType* self, ValueType* what)  { Accessor::SetNext(self, what);  }
    inline static const ValueType* GetPrev(const ValueType* self) { return Accessor::GetPrev(self); }
    inline static const ValueType* GetNext(const ValueType* self) { return Accessor::GetNext(self); }
    inline static       ValueType* GetPrev(ValueType* self)       { return Accessor::GetPrev(self); }
    inline static       ValueType* GetNext(ValueType* self)       { return Accessor::GetNext(self); }

    GList2()
    {
        SetPrev(&Root, &Root);
        SetNext(&Root, &Root);
    }

    void Clear()
    {
        SetPrev(&Root, &Root);
        SetNext(&Root, &Root);
    }

    const ValueType* GetFirst() const { return GetNext(&Root); }
    const ValueType* GetLast () const { return GetPrev(&Root); }
          ValueType* GetFirst()       { return GetNext(&Root); }
          ValueType* GetLast ()       { return GetPrev(&Root); }

    bool IsEmpty()                   const { return GetNext(&Root) == &Root; }
    bool IsFirst(const ValueType* p) const { return p == GetNext(&Root); }
    bool IsLast (const ValueType* p) const { return p == GetPrev(&Root); }
    bool IsNull (const ValueType* p) const { return p == &Root; }

    void PushFront(ValueType* p)
    {
        SetNext(p, GetNext(&Root));
        SetPrev(p, &Root);
        SetPrev(GetNext(&Root), p);
        SetNext(&Root, p);
    }

    void PushBack(ValueType* p)
    {
        SetPrev(p, GetPrev(&Root));
        SetNext(p, &Root);
        SetNext(GetPrev(&Root), p);
        SetPrev(&Root, p);
    }

    void InsertBefore(ValueType* existing, ValueType* newOne)
    {
        ValueType* prev = GetPrev(existing);
        SetNext(newOne,   existing);
        SetPrev(newOne,   prev);
        SetNext(prev,     newOne);
        SetPrev(existing, newOne);
    }

    void InsertAfter(ValueType* existing, ValueType* newOne)
    {
        ValueType* next = GetNext(existing);
        SetPrev(newOne,   existing);
        SetNext(newOne,   next);
        SetPrev(next,     newOne);
        SetNext(existing, newOne);
    }

    static void Remove(ValueType* p)
    {
        SetNext(GetPrev(p), GetNext(p));
        SetPrev(GetNext(p), GetPrev(p));
    }

    void BringToFront(ValueType* p)
    {
        Remove(p);
        PushFront(p);
    }

    void SendToBack(ValueType* p)
    {
        Remove(p);
        PushBack(p);
    }

private:
    // Copying is prohibited
    GList2(const GList2<T,Accessor>&);
    const GList2<T,Accessor>& operator = (const GList2<T,Accessor>&);

    ValueType Root;
};


// ***** G_FreeListElements
//
// Remove all elements in the list and free them in the allocator
//------------------------------------------------------------------------
template<class List, class Allocator>
void G_FreeListElements(List& list, Allocator& allocator)
{
    typename List::ValueType* self = list.GetFirst();
    while(!list.IsNull(self))
    {
        typename List::ValueType* next = list.GetNext(self);
        allocator.Free(self);
        self = next;
    }
    list.Clear();
}


#endif
