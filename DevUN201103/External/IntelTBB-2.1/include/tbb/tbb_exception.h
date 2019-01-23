/*
    Copyright 2005-2009 Intel Corporation.  All Rights Reserved.

    The source code contained or described herein and all documents related
    to the source code ("Material") are owned by Intel Corporation or its
    suppliers or licensors.  Title to the Material remains with Intel
    Corporation or its suppliers and licensors.  The Material is protected
    by worldwide copyright laws and treaty provisions.  No part of the
    Material may be used, copied, reproduced, modified, published, uploaded,
    posted, transmitted, distributed, or disclosed in any way without
    Intel's prior express written permission.

    No license under any patent, copyright, trade secret or other
    intellectual property right is granted to or conferred upon you by
    disclosure or delivery of the Materials, either expressly, by
    implication, inducement, estoppel or otherwise.  Any license under such
    intellectual property rights must be express and approved by Intel in
    writing.
*/

#ifndef __TBB_exception_H
#define __TBB_exception_H

#include "tbb_stddef.h"
#include <stdexcept>

namespace tbb {

//! Exception for concurrent containers
class bad_last_alloc : public std::bad_alloc {
public:
    virtual const char* what() const throw() { return "bad allocation in previous or concurrent attempt"; }
    virtual ~bad_last_alloc() throw() {}
};

} // namespace tbb

#if __TBB_EXCEPTIONS
#include "tbb_allocator.h"
#include <exception>
#include <typeinfo>
#include <new>

namespace tbb {

//! Interface to be implemented by all exceptions TBB recognizes and propagates across the threads.
/** If an unhandled exception of the type derived from tbb::tbb_exception is intercepted
    by the TBB scheduler in one of the worker threads, it is delivered to and re-thrown in
    the root thread. The root thread is the thread that has started the outermost algorithm 
    or root task sharing the same task_group_context with the guilty algorithm/task (the one
    that threw the exception first).
    
    Note: when documentation mentions workers with respect to exception handling, 
    masters are implied as well, because they are completely equivalent in this context.
    Consequently a root thread can be master or worker thread. 

    NOTE: In case of nested algorithms or complex task hierarchies when the nested 
    levels share (explicitly or by means of implicit inheritance) the task group 
    context of the outermost level, the exception may be (re-)thrown multiple times 
    (ultimately - in each worker on each nesting level) before reaching the root 
    thread at the outermost level. IMPORTANT: if you intercept an exception derived 
    from this class on a nested level, you must re-throw it in the catch block by means
    of the "throw;" operator. 
    
    TBB provides two implementations of this interface: tbb::captured_exception and 
    template class tbb::movable_exception. See their declarations for more info. **/
class tbb_exception : public std::exception {
public:
    //! Creates and returns pointer to the deep copy of this exception object. 
    /** Move semantics is allowed. **/
    virtual tbb_exception* move () throw() = 0;
    
    //! Destroys objects created by the move() method.
    /** Frees memory and calls destructor for this exception object. 
        Can and must be used only on objects created by the move method. **/
    virtual void destroy () throw() = 0;

    //! Throws this exception object.
    /** Make sure that if you have several levels of derivation from this interface
        you implement or override this method on the most derived level. The implementation 
        is as simple as "throw *this;". Failure to do this will result in exception 
        of a base class type being thrown. **/
    virtual void throw_self () = 0;

    //! Returns RTTI name of the originally intercepted exception
    virtual const char* name() const throw() = 0;

    //! Returns the result of originally intercepted exception's what() method.
    virtual const char* what() const throw() = 0;
};

//! This class is used by TBB to propagate information about unhandled exceptions into the root thread.
/** Exception of this type is thrown by TBB in the root thread (thread that started a parallel 
    algorithm ) if an unhandled exception was intercepted during the algorithm execution in one 
    of the workers.
    \sa tbb::tbb_exception **/
class captured_exception : public tbb_exception
{
public:
    captured_exception ( const captured_exception& src )
        : my_dynamic(false)
    {
        set(src.my_exception_name, src.my_exception_info);
    }

    captured_exception ( const char* name, const char* info )
        : my_dynamic(false)
    {
        set(name, info);
    }

    __TBB_EXPORTED_METHOD ~captured_exception () throw() {
        clear();
    }

    captured_exception& operator= ( const captured_exception& src ) {
        if ( this != &src ) {
            clear();
            set(src.my_exception_name, src.my_exception_info);
        }
        return *this;
    }

    /*override*/ 
    captured_exception* move () throw();

    /*override*/ 
    void destroy () throw();

    /*override*/ 
    void throw_self () { throw *this; }

    /*override*/ 
    const char* __TBB_EXPORTED_METHOD name() const throw();

    /*override*/ 
    const char* __TBB_EXPORTED_METHOD what() const throw();

private:
    //! Used only by method clone().  
    captured_exception() {}

    //! Functionally equivalent to {captured_exception e(name,info); return c.clone();}
    static captured_exception* allocate ( const char* name, const char* info );

    void set ( const char* name, const char* info ) throw();
    void clear () throw();

    bool my_dynamic;
    const char* my_exception_name;
    const char* my_exception_info;
};

//! Template that can be used to implement exception that transfers arbitrary ExceptionData to the root thread
/** Code using TBB can instantiate this template with an arbitrary ExceptionData type 
    and throw this exception object. Such exceptions are intercepted by the TBB scheduler
    and delivered to the root thread (). 
    \sa tbb::tbb_exception **/
template<typename ExceptionData>
class movable_exception : public tbb_exception
{
    typedef movable_exception<ExceptionData> self_type;

public:
    movable_exception ( const ExceptionData& data ) 
        : my_exception_data(data)
        , my_dynamic(false)
        , my_exception_name(typeid(self_type).name())
    {}

    movable_exception ( const movable_exception& src ) throw () 
        : my_exception_data(src.my_exception_data)
        , my_dynamic(false)
        , my_exception_name(src.my_exception_name)
    {}

    ~movable_exception () throw() {}

    const movable_exception& operator= ( const movable_exception& src ) {
        if ( this != &src ) {
            my_exception_data = src.my_exception_data;
            my_exception_name = src.my_exception_name;
        }
        return *this;
    }

    ExceptionData& data () throw() { return my_exception_data; }

    const ExceptionData& data () const throw() { return my_exception_data; }

    /*override*/ const char* name () const throw() { return my_exception_name; }

    /*override*/ const char* what () const throw() { return "tbb::movable_exception"; }

    /*override*/ 
    movable_exception* move () throw() {
        void* e = internal::allocate_via_handler_v3(sizeof(movable_exception));
        if ( e ) {
            new (e) movable_exception(*this);
            ((movable_exception*)e)->my_dynamic = true;
        }
        return (movable_exception*)e;
    }
    /*override*/ 
    void destroy () throw() {
        __TBB_ASSERT ( my_dynamic, "Method destroy can be called only on dynamically allocated movable_exceptions" );
        if ( my_dynamic ) {
            this->~movable_exception();
            internal::deallocate_via_handler_v3(this);
        }
    }
    /*override*/ 
    void throw_self () {
        throw *this;
    }

protected:
    //! User data
    ExceptionData  my_exception_data;

private:
    //! Flag specifying whether this object has been dynamically allocated (by the move method)
    bool my_dynamic;

    //! RTTI name of this class
    /** We rely on the fact that RTTI names are static string constants. **/
    const char* my_exception_name;
};

} // namespace tbb

#endif /* __TBB_EXCEPTIONS */

#endif /* __TBB_exception_H */
