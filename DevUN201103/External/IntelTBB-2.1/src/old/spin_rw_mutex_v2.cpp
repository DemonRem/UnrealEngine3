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

#include "spin_rw_mutex_v2.h"
#include "tbb/tbb_machine.h"
#include "../tbb/tbb_misc.h"
#include "../tbb/itt_notify.h"

namespace tbb {

using namespace internal;

static inline bool CAS(volatile uintptr &addr, uintptr newv, uintptr oldv) {
    return __TBB_CompareAndSwapW((volatile void *)&addr, (intptr)newv, (intptr)oldv) == (intptr)oldv;
}

//! Signal that write lock is released
void spin_rw_mutex::internal_itt_releasing(spin_rw_mutex *mutex) {
    ITT_NOTIFY(sync_releasing, mutex);
}

bool spin_rw_mutex::internal_acquire_writer(spin_rw_mutex *mutex)
{
    ITT_NOTIFY(sync_prepare, mutex);
    ExponentialBackoff backoff;
    for(;;) {
        state_t s = mutex->state;
        if( !(s & BUSY) ) { // no readers, no writers
            if( CAS(mutex->state, WRITER, s) )
                break; // successfully stored writer flag
            backoff.reset(); // we could be very close to complete op.
        } else if( !(s & WRITER_PENDING) ) { // no pending writers
            __TBB_AtomicOR(&mutex->state, WRITER_PENDING);
        }
        backoff.pause();
    }
    ITT_NOTIFY(sync_acquired, mutex);
    __TBB_ASSERT( (mutex->state & BUSY)==WRITER, "invalid state of a write lock" );
    return false;
}

//! Signal that write lock is released
void spin_rw_mutex::internal_release_writer(spin_rw_mutex *mutex) {
    __TBB_ASSERT( (mutex->state & BUSY)==WRITER, "invalid state of a write lock" );
    ITT_NOTIFY(sync_releasing, mutex);
    mutex->state = 0; 
}

//! Acquire lock on given mutex.
void spin_rw_mutex::internal_acquire_reader(spin_rw_mutex *mutex) {
    ITT_NOTIFY(sync_prepare, mutex);
    ExponentialBackoff backoff;
    for(;;) {
        state_t s = mutex->state;
        if( !(s & (WRITER|WRITER_PENDING)) ) { // no writer or write requests
            if( CAS(mutex->state, s+ONE_READER, s) )
                break; // successfully stored increased number of readers
            backoff.reset(); // we could be very close to complete op.
        }
        backoff.pause();
    }
    ITT_NOTIFY(sync_acquired, mutex);
    __TBB_ASSERT( mutex->state & READERS, "invalid state of a read lock: no readers" );
    __TBB_ASSERT( !(mutex->state & WRITER), "invalid state of a read lock: active writer" );
}

//! Upgrade reader to become a writer.
/** Returns true if the upgrade happened without re-acquiring the lock and false if opposite */
bool spin_rw_mutex::internal_upgrade(spin_rw_mutex *mutex) {
    state_t s = mutex->state;
    __TBB_ASSERT( s & READERS, "invalid state before upgrade: no readers " );
    __TBB_ASSERT( !(s & WRITER), "invalid state before upgrade: active writer " );
    // check and set writer-pending flag
    // required conditions: either no pending writers, or we are the only reader
    // (with multiple readers and pending writer, another upgrade could have been requested)
    while( (s & READERS)==ONE_READER || !(s & WRITER_PENDING) ) {
        if( CAS(mutex->state, s | WRITER_PENDING, s) )
        {
            ExponentialBackoff backoff;
            ITT_NOTIFY(sync_prepare, mutex);
            while( (mutex->state & READERS) != ONE_READER ) // more than 1 reader
                backoff.pause();
            // the state should be 0...0110, i.e. 1 reader and waiting writer;
            // both new readers and writers are blocked
            __TBB_ASSERT(mutex->state == (ONE_READER | WRITER_PENDING),"invalid state when upgrading to writer");
            mutex->state = WRITER;
            ITT_NOTIFY(sync_acquired, mutex);
            __TBB_ASSERT( (mutex->state & BUSY) == WRITER, "invalid state after upgrade" );
            return true; // successfully upgraded
        } else {
            s = mutex->state; // re-read
        }
    }
    // slow reacquire
    internal_release_reader(mutex);
    return internal_acquire_writer(mutex); // always returns false
}

void spin_rw_mutex::internal_downgrade(spin_rw_mutex *mutex) {
    __TBB_ASSERT( (mutex->state & BUSY) == WRITER, "invalid state before downgrade" );
    ITT_NOTIFY(sync_releasing, mutex);
    mutex->state = ONE_READER;
    __TBB_ASSERT( mutex->state & READERS, "invalid state after downgrade: no readers" );
    __TBB_ASSERT( !(mutex->state & WRITER), "invalid state after downgrade: active writer" );
}

void spin_rw_mutex::internal_release_reader(spin_rw_mutex *mutex)
{
    __TBB_ASSERT( mutex->state & READERS, "invalid state of a read lock: no readers" );
    __TBB_ASSERT( !(mutex->state & WRITER), "invalid state of a read lock: active writer" );
    ITT_NOTIFY(sync_releasing, mutex); // release reader
    __TBB_FetchAndAddWrelease((volatile void *)&(mutex->state),-(intptr)ONE_READER);
}

bool spin_rw_mutex::internal_try_acquire_writer( spin_rw_mutex * mutex )
{
// for a writer: only possible to acquire if no active readers or writers
    state_t s = mutex->state; // on Itanium, this volatile load has acquire semantic
    if( !(s & BUSY) ) // no readers, no writers; mask is 1..1101
        if( CAS(mutex->state, WRITER, s) ) {
            ITT_NOTIFY(sync_acquired, mutex);
            return true; // successfully stored writer flag
        }
    return false;
}

bool spin_rw_mutex::internal_try_acquire_reader( spin_rw_mutex * mutex )
{
// for a reader: acquire if no active or waiting writers
    state_t s = mutex->state;    // on Itanium, a load of volatile variable has acquire semantic
    while( !(s & (WRITER|WRITER_PENDING)) ) // no writers
        if( CAS(mutex->state, s+ONE_READER, s) ) {
            ITT_NOTIFY(sync_acquired, mutex);
            return true; // successfully stored increased number of readers
        }
    return false;
}

} // namespace tbb
