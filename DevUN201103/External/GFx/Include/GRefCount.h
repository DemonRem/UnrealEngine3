/**********************************************************************

Filename    :   GRefCount.h
Content     :   Reference counting implementation headers
Created     :   January 14, 1999
Authors     :   Michael Antonov
Notes       :

History     :   

Copyright   :   (c) 1999-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GRefCount_H
#define INC_GRefCount_H

#include "GTypes.h"
#include "GMemory.h"


// ***** Reference Counting

// There are three types of reference counting base classes:
//
//  GRefCountBase     - Provides thread-safe reference counting (Default).
//  GRefCountBaseNTS  - Non Thread Safe version of reference counting.
//  GRefCountBaseWeakSupport - Non Thread Safe ref-counting with GWeakPtr support.


// ***** Declared classes

template<class C, int StatType>
class   GRefCountBase;
template<class C, int StatType>
class   GRefCountBaseNTS;
template<class C, int StatType>
class   GRefCountBaseWeakSupport;

class   GRefCountImpl;
class   GRefCountNTSImpl;
class   GRefCountWeakSupportImpl;
//class   GRefCountImpl;

template<class C>
class   GWeakPtr;
class   GWeakPtrProxy;



// ***** Implementation For Reference Counting

// GRefCountImplCore holds RefCount value and defines a few utility
// functions shared by all implementations.

class GRefCountImplCore
{
protected:
    volatile SInt   RefCount;

public:
    // GRefCountImpl constructor always initializes RefCount to 1 by default.
    GINLINE GRefCountImplCore() { RefCount = 1; }

    // Need virtual destructor
    // This:    1. Makes sure the right destructor's called.
    //          2. Makes us have VTable, necessary if we are going to have format needed by InitNewMem()
    GEXPORT virtual ~GRefCountImplCore();

    // Debug method only.
    SInt            GetRefCount() const { return RefCount;  }

    // This logic is used to detect invalid 'delete' calls of reference counted
    // objects. Direct delete calls are not allowed on them unless they come in
    // internally from Release.
#ifdef GFC_BUILD_DEBUG    
    static void   GCDECL    reportInvalidDelete(void *pmem);
    inline static void checkInvalidDelete(GRefCountImplCore *pmem)
    {
        if (pmem->RefCount != 0)
            reportInvalidDelete(pmem);
    }
#else
    inline static void checkInvalidDelete(GRefCountImplCore *) { }
#endif

    // Base class ref-count content should not be copied.
    void operator = (const GRefCountImplCore &) { }  
};



// GRefCountImpl provides Thread-Safe implementation of reference counting, so
// it should be used by default in most places.

class GRefCountImpl : public GRefCountImplCore
{
public:
    // Thread-Safe Ref-Count Implementation.
    GEXPORT void    AddRef();
    GEXPORT void    Release();   
};

// GRefCountImplNTS provides Non-Thread-Safe implementation of reference counting,
// which is slightly more efficient since it doesn't use atomics.

class GRefCountNTSImpl : public GRefCountImplCore
{
public:
    GINLINE void    AddRef() { RefCount++; }
    GEXPORT void    Release();   
};

// GRefCountWeakSupportImpl provides reference counting with WeakProxy support,
// allowing for GWeakPtr<> to be used on objects derived from this class. Weak pointer
// support is non-thread safe. Weak pointers are used for some ActionScript objects,
// especially when GC is disabled.

class GRefCountWeakSupportImpl : public GRefCountNTSImpl
{
protected:    
    mutable GWeakPtrProxy*  pWeakProxy;
public:
    GRefCountWeakSupportImpl() { pWeakProxy = 0; }
    GEXPORT virtual ~GRefCountWeakSupportImpl();

    // Create/return create proxy, users must release proxy when no longer needed.
    GWeakPtrProxy*  CreateWeakProxy() const;
};


// GRefCountBaseStatImpl<> is a common class that adds new/delete override with Stat tracking
// to the reference counting implemenation. Base must be one of the GRefCountImpl classes.

template<class Base, int StatType>
class GRefCountBaseStatImpl : public Base
{
public:
    GRefCountBaseStatImpl() { }
     
    // *** Override New and Delete

    // DOM-IGNORE-BEGIN
    // Undef new temporarily if it is being redefined
#ifdef GFC_BUILD_DEFINE_NEW
#ifdef GFC_DEFINE_NEW
#undef new
#endif
#endif

#ifdef GFC_BUILD_DEBUG
    // Custom check used to detect incorrect calls of 'delete' on ref-counted objects.
    #define GFC_REFCOUNTALLOC_CHECK_DELETE(class_name, p)   \
        do {if (p) ((class_name*)p)->checkInvalidDelete((class_name*)p); } while(0)
#else
    #define GFC_REFCOUNTALLOC_CHECK_DELETE(class_name, p)
#endif

    // Redefine all new & delete operators.
    GFC_MEMORY_REDEFINE_NEW_IMPL(Base, GFC_REFCOUNTALLOC_CHECK_DELETE, StatType)

#ifdef GFC_DEFINE_NEW
#define new GFC_DEFINE_NEW
#endif
        // GFC_BUILD_DEFINE_NEW
        // DOM-IGNORE-END
};




// *** End user GRefCountBase<> classes


// GRefCountBase is a base class for classes that require thread-safe reference
// counting; it also overrides the new and delete operators to use GMemoryHeap.
//
// Reference counted objects start out with RefCount value of 1. Further lifetame
// management is done through the AddRef() and Release() methods, typically
// hidden by GPtr<>.

template<class C, int Stat>
class GRefCountBase : public GRefCountBaseStatImpl<GRefCountImpl, Stat>
{
public:
    enum { StatType = Stat };   
    // Constructor.
    GINLINE GRefCountBase() : GRefCountBaseStatImpl<GRefCountImpl, Stat>() { }    
};


// GRefCountBaseNTS is a base class for classes that require Non-Thread-Safe reference
// counting; it also overrides the new and delete operators to use GMemoryHeap.
// This class should only be used if all pointers to it are known to be assigned,
// destroyed and manipulated within one thread.
//
// Reference counted objects start out with RefCount value of 1. Further lifetame
// management is done through the AddRef() and Release() methods, typically
// hidden by GPtr<>.

template<class C, int Stat>
class GRefCountBaseNTS : public GRefCountBaseStatImpl<GRefCountNTSImpl, Stat>
{
public:
    enum { StatType = Stat };   
    // Constructor.
    GINLINE GRefCountBaseNTS() : GRefCountBaseStatImpl<GRefCountNTSImpl, Stat>() { }    
};

// GRefCountBaseWeakSupport is a base class for classes that require Non-Thread-Safe
// reference counting and GWeakPtr<> support; it also overrides the new and delete
// operators to use GMemoryHeap. This class should only be used if all pointers to it
// are known to be assigned, destroyed and manipulated within one thread.
//
// Reference counted objects start out with RefCount value of 1. Further lifetame
// management is done through the AddRef() and Release() methods, typically
// hidden by GPtr<>.

template<class C, int Stat>
class GRefCountBaseWeakSupport : public GRefCountBaseStatImpl<GRefCountWeakSupportImpl, Stat>
{
public:
    enum { StatType = Stat };   
    // Constructor.
    GINLINE GRefCountBaseWeakSupport() : GRefCountBaseStatImpl<GRefCountWeakSupportImpl, Stat>() { }    
};




// ***** Ref-Counted template pointer

// Automatically AddRefs and Releases interfaces

template<class C>
class GPtr
{   
protected:
    C   *pObject;

public:

    // Constructors
    GINLINE GPtr() : pObject(0)
    { }
    GINLINE GPtr(C &robj)
    {
        pObject = &robj;
    }
    GINLINE GPtr(C *pobj)
    {
        if (pobj) pobj->AddRef();   
        pObject = pobj;
    }
    GINLINE GPtr(const GPtr<C> &src)
    {
        if (src.pObject) src.pObject->AddRef();     
        pObject = src.pObject;
    }

    template<class R>
    GINLINE GPtr(GPtr<R> &src)
    {
        if (src) src->AddRef();
        pObject = src;
    }

    // Destructor
    GINLINE ~GPtr()
    {
        if (pObject) pObject->Release();        
    }

    // Compares
    GINLINE bool operator == (const GPtr &other) const      { return pObject == other.pObject; }
    GINLINE bool operator != (const GPtr &other) const      { return pObject != other.pObject; }

    GINLINE bool operator == (const C *pother) const        { return pObject == pother; }
    GINLINE bool operator != (const C *pother) const        { return pObject != pother; }
    GINLINE bool operator == (C *pother) const              { return pObject == pother; }
    GINLINE bool operator != (C *pother) const              { return pObject != pother; }


    GINLINE bool operator < (const GPtr &other) const       { return pObject < other.pObject; }

    // Assignment
    template<class R>
    GINLINE const GPtr<C>& operator = (const GPtr<R> &src)
    {
        if (src) src->AddRef();
        if (pObject) pObject->Release();        
        pObject = src;
        return *this;
    }   
    // Specialization
    GINLINE const GPtr<C>& operator = (const GPtr<C> &src)
    {
        if (src) src->AddRef();
        if (pObject) pObject->Release();        
        pObject = src;
        return *this;
    }   
    
    GINLINE const GPtr<C>& operator = (C *psrc)
    {
        if (psrc) psrc->AddRef();
        if (pObject) pObject->Release();        
        pObject = psrc;
        return *this;
    }   
    GINLINE const GPtr<C>& operator = (C &src)
    {       
        if (pObject) pObject->Release();        
        pObject = &src;
        return *this;
    }
    
    // Set Assignment
    template<class R>
    GINLINE GPtr<C>& SetPtr(const GPtr<R> &src)
    {
        if (src) src->AddRef();
        if (pObject) pObject->Release();
        pObject = src;
        return *this;
    }
    // Specialization
    GINLINE GPtr<C>& SetPtr(const GPtr<C> &src)
    {
        if (src) src->AddRef();
        if (pObject) pObject->Release();
        pObject = src;
        return *this;
    }   
    
    GINLINE GPtr<C>& SetPtr(C *psrc)
    {
        if (psrc) psrc->AddRef();
        if (pObject) pObject->Release();
        pObject = psrc;
        return *this;
    }   
    GINLINE GPtr<C>& SetPtr(C &src)
    {       
        if (pObject) pObject->Release();
        pObject = &src;
        return *this;
    }

    // Nulls ref-counted pointer without decrement
    GINLINE void    NullWithoutRelease()    
    { 
        pObject = 0;    
    }

    // Clears the pointer to the object
    GINLINE void    Clear()
    {
        if (pObject) pObject->Release();
        pObject = 0;
    }

    // Obtain pointer reference directly, for D3D interfaces
    GINLINE C*& GetRawRef()                 { return pObject; }

    // Access Operators
    GINLINE C* GetPtr() const               { return pObject; }
    GINLINE C& operator * () const          { return *pObject; }
    GINLINE C* operator -> ()  const        { return pObject; }
    // Conversion                   
    GINLINE operator C* () const            { return pObject; }
};



// *** Weak Pointer Support

// Helper for making objects that can have weak_ptr's.
// TBD: Converted WeakPtr logic is not thread-safe, need to work this out

class GWeakPtrProxy : public GNewOverrideBase<GStat_Default_Mem>
{
public:
    GWeakPtrProxy(GRefCountWeakSupportImpl* pobject)
      : RefCount(1), pObject(pobject)
    { }

    // GWeakPtr calls this to determine if its pointer is valid or not
    GINLINE bool    IsAlive() const    { return (pObject != 0); }

    // Objects call this to inform of their death
    GINLINE void    NotifyObjectDied() { pObject = 0; }

    GRefCountWeakSupportImpl* GetObject() const { return pObject; }

    GINLINE void    AddRef()
    {   RefCount++; }
    GINLINE void    Release()
    {
        RefCount--;
        if (RefCount == 0)
            delete this;
    }    

private:
    // Hidden
    GWeakPtrProxy(const GWeakPtrProxy& w) { GUNUSED(w); }
    void    operator=(const GWeakPtrProxy& w) { GUNUSED(w); }

    SInt                        RefCount;
    GRefCountWeakSupportImpl*   pObject;
};


// A weak pointer points at an object, but the object may be deleted
// at any time, in which case the weak pointer automatically becomes
// NULL.  The only way to use a weak pointer is by converting it to a
// strong pointer (i.e. for temporary use).
//
// The class pointed to must have a "GWeakPtrProxy* GetWeakPtrProxy()" method.
//
// Usage idiom:
//
// if (GPtr<my_type> ptr = weak_ptr_to_my_type) { ... use ptr->whatever() safely in here ... }


template<class C>
class GWeakPtr
{
public:
    GWeakPtr()
    { }

    GINLINE GWeakPtr(C* ptr)
        : pProxy(*(ptr ? ptr->CreateWeakProxy() : (GWeakPtrProxy*)0))
    {  }
    GINLINE GWeakPtr(const GPtr<C>& ptr)
        : pProxy(*(ptr.GetPtr() ? ptr->CreateWeakProxy() : (GWeakPtrProxy*)0))
    {  }

    // Default constructor and assignment from GWeakPtr<C> are OK
    GINLINE void    operator = (C* ptr)
    {
        if (ptr)
        {
            pProxy = *ptr->CreateWeakProxy();
        }
        else
        {
            pProxy.Clear();
        }
    }

    GINLINE void    operator = (const GPtr<C>& ptr)
    { operator=(ptr.GetPtr()); }

    // Conversion to GPtr
    inline operator GPtr<C>()
    {        
        return GPtr<C>(GetObjectThroughProxy());
    }

    GINLINE bool    operator == (C* ptr)
    { return GetObjectThroughProxy() == ptr; }
    GINLINE bool    operator == (const GPtr<C>& ptr)
    { return GetObjectThroughProxy() == ptr.GetPtr(); }

private:
    
    // Set pObject to NULL if the object died
    GINLINE C* GetObjectThroughProxy()
    {
        C* pobject = 0;

        if (pProxy)
        {
            if (pProxy->IsAlive())
            {
                pobject = (C*)pProxy->GetObject();
            }
            else
            {
                // Release proxy if the underlying object went away.
                pProxy.Clear();
            }
        }
        return pobject;
    }

    GPtr<GWeakPtrProxy> pProxy;
};



#endif
