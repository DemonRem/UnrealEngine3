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

#ifndef _TBB_malloc_Customize_H_
#define _TBB_malloc_Customize_H_

/* Thread shutdown notification callback */
/* redefine the name of the callback to meet TBB requirements
   for externally visible names of service functions */
#define mallocThreadShutdownNotification __TBB_mallocThreadShutdownNotification
#define mallocProcessShutdownNotification __TBB_mallocProcessShutdownNotification

extern "C" void mallocThreadShutdownNotification(void *);
extern "C" void mallocProcessShutdownNotification(void);

// customizing MALLOC_ASSERT macro
#include "tbb/tbb_stddef.h"
#define MALLOC_ASSERT(assertion, message) __TBB_ASSERT(assertion, message)

#ifndef MALLOC_DEBUG
#define MALLOC_DEBUG TBB_USE_DEBUG
#endif

#include "tbb/tbb_machine.h"

#if DO_ITT_NOTIFY
#include "tbb/itt_notify.h"
#define MALLOC_ITT_SYNC_PREPARE(pointer) ITT_NOTIFY(sync_prepare, (pointer))
#define MALLOC_ITT_SYNC_ACQUIRED(pointer) ITT_NOTIFY(sync_acquired, (pointer))
#define MALLOC_ITT_SYNC_RELEASING(pointer) ITT_NOTIFY(sync_releasing, (pointer))
#define MALLOC_ITT_SYNC_CANCEL(pointer) ITT_NOTIFY(sync_cancel, (pointer))
#else
#define MALLOC_ITT_SYNC_PREPARE(pointer) ((void)0)
#define MALLOC_ITT_SYNC_ACQUIRED(pointer) ((void)0)
#define MALLOC_ITT_SYNC_RELEASING(pointer) ((void)0)
#define MALLOC_ITT_SYNC_CANCEL(pointer) ((void)0)
#endif

//! Stripped down version of spin_mutex.
/** Instances of MallocMutex must be declared in memory that is zero-initialized.
    There are no constructors.  This is a feature that lets it be
    used in situations where the mutex might be used while file-scope constructors
    are running.

    There are no methods "acquire" or "release".  The scoped_lock must be used
    in a strict block-scoped locking pattern.  Omitting these methods permitted
    further simplication. */
class MallocMutex {
    unsigned char value;

    //! Deny assignment
    void operator=( MallocMutex& MallocMutex );
public:
    class scoped_lock {
        const unsigned char value;
        MallocMutex& mutex;
    public:
        scoped_lock( MallocMutex& m ) : value( __TBB_LockByte(m.value)), mutex(m) {}
        ~scoped_lock() { __TBB_store_with_release(mutex.value, value); }
    };
    friend class scoped_lock;
};

inline intptr_t AtomicIncrement( volatile intptr_t& counter ) {
    return __TBB_FetchAndAddW( &counter, 1 )+1;
}

inline intptr_t AtomicCompareExchange( volatile intptr_t& location, intptr_t new_value, intptr_t comparand) {
    return __TBB_CompareAndSwapW( &location, new_value, comparand );
}

#define USE_DEFAULT_MEMORY_MAPPING 1

namespace tbb {
namespace internal {
    void init_tbbmalloc();
} } // namespaces

#define MALLOC_EXTRA_INITIALIZATION tbb::internal::init_tbbmalloc()

// To support malloc replacement with LD_PRELOAD
#include "proxy.h"

#if MALLOC_LD_PRELOAD
extern "C" void * __TBB_malloc_proxy(size_t)  __attribute__ ((weak));
#endif

#endif /* _TBB_malloc_Customize_H_ */
