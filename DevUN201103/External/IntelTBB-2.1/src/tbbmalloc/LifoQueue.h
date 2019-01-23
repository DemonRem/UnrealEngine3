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

#ifndef _itt_common_malloc_LifoQueue_H_
#define _itt_common_malloc_LifoQueue_H_

#include "TypeDefinitions.h"
#include <string.h> // for memset()

//! Checking the synchronization method
/** FINE_GRAIN_LOCKS is the only variant for now; should be defined for LifoQueue */
#ifndef FINE_GRAIN_LOCKS
#define FINE_GRAIN_LOCKS
#endif

namespace ThreadingSubstrate {

namespace Internal {

class LifoQueue {
public:
    inline LifoQueue();
    inline void push(void** ptr);
    inline void* pop(void);

private:
    void * top;
#ifdef FINE_GRAIN_LOCKS
    MallocMutex lock;
#endif /* FINE_GRAIN_LOCKS     */
};

#ifdef FINE_GRAIN_LOCKS
/* LifoQueue assumes zero initialization so a vector of it can be created
 * by just allocating some space with no call to constructor.
 * On Linux, it seems to be necessary to avoid linking with C++ libraries.
 *
 * By usage convention there is no race on the initialization. */
LifoQueue::LifoQueue( ) : top(NULL)
{
    // MallocMutex assumes zero initialization
    memset(&lock, 0, sizeof(MallocMutex));
}

void LifoQueue::push( void **ptr )
{   
    MallocMutex::scoped_lock scoped_cs(lock);
    *ptr = top;
    top = ptr;
}

void * LifoQueue::pop( )
{   
    void **result=NULL;
    {
        MallocMutex::scoped_lock scoped_cs(lock);
        if (!top) goto done;
        result = (void **) top;
        top = *result;
    } 
    *result = NULL;
done:
    return result;
}

#endif /* FINE_GRAIN_LOCKS     */

} // namespace Internal
} // namespace ThreadingSubstrate

#endif /* _itt_common_malloc_LifoQueue_H_ */

