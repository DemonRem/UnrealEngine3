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

#ifndef _TBB_Gate_H
#define _TBB_Gate_H

#include "itt_notify.h"

namespace tbb {

namespace internal {

#if __TBB_USE_FUTEX

//! Implementation of Gate based on futex.
/** Use this futex-based implementation where possible, because it is the simplest and usually fastest. */
class Gate {
public:
    typedef intptr_t state_t;

    Gate() {
        ITT_SYNC_CREATE(&state, SyncType_Scheduler, SyncObj_Gate);
    }

    //! Get current state of gate
    state_t get_state() const {
        return state;
    }
    //! Update state=value if state==comparand (flip==false) or state!=comparand (flip==true)
    void try_update( intptr_t value, intptr_t comparand, bool flip=false ) {
        __TBB_ASSERT( comparand!=0 || value!=0, "either value or comparand must be non-zero" );
retry:
        state_t old_state = state;
        // First test for condition without using atomic operation
        if( flip ? old_state!=comparand : old_state==comparand ) {
            // Now atomically retest condition and set.
            state_t s = state.compare_and_swap( value, old_state );
            if( s==old_state ) {
                // compare_and_swap succeeded
                if( value!=0 )   
                    futex_wakeup_all( &state );  // Update was successful and new state is not SNAPSHOT_EMPTY
            } else {
                // compare_and_swap failed.  But for != case, failure may be spurious for our purposes if
                // the value there is nonetheless not equal to value.  This is a fairly rare event, so
                // there is no need for backoff.  In event of such a failure, we must retry.
                if( flip && s!=value ) 
                    goto retry;
            }
        }
    }
    //! Wait for state!=0.
    void wait() {
        if( state==0 )
            futex_wait( &state, 0 );
    }
private:
    atomic<state_t> state;
};

#elif USE_WINTHREAD

class Gate {
public:
    typedef intptr_t state_t;
private:
    //! If state==0, then thread executing wait() suspend until state becomes non-zero.
    state_t state;
    CRITICAL_SECTION critical_section;
    HANDLE event;
public:
    //! Initialize with count=0
    Gate() :   
    state(0) 
    {
        event = CreateEvent( NULL, true, false, NULL );
        InitializeCriticalSection( &critical_section );
        ITT_SYNC_CREATE(event, SyncType_Scheduler, SyncObj_Gate);
        ITT_SYNC_CREATE(&critical_section, SyncType_Scheduler, SyncObj_GateLock);
    }
    ~Gate() {
        CloseHandle( event );
        DeleteCriticalSection( &critical_section );
    }
    //! Get current state of gate
    state_t get_state() const {
        return state;
    }
    //! Update state=value if state==comparand (flip==false) or state!=comparand (flip==true)
    void try_update( intptr_t value, intptr_t comparand, bool flip=false ) {
        __TBB_ASSERT( comparand!=0 || value!=0, "either value or comparand must be non-zero" );
        EnterCriticalSection( &critical_section );
        state_t old = state;
        if( flip ? old!=comparand : old==comparand ) {
            state = value;
            if( !old )
                SetEvent( event );
            else if( !value )
                ResetEvent( event );
        }
        LeaveCriticalSection( &critical_section );
    }
    //! Wait for state!=0.
    void wait() {
        if( state==0 ) {
            WaitForSingleObject( event, INFINITE );
        }
    }
};

#elif USE_PTHREAD

class Gate {
public:
    typedef intptr_t state_t;
private:
    //! If state==0, then thread executing wait() suspend until state becomes non-zero.
    state_t state;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
public:
    //! Initialize with count=0
    Gate() : state(0)
    {
        pthread_mutex_init( &mutex, NULL );
        pthread_cond_init( &cond, NULL);
        ITT_SYNC_CREATE(&cond, SyncType_Scheduler, SyncObj_Gate);
        ITT_SYNC_CREATE(&mutex, SyncType_Scheduler, SyncObj_GateLock);
    }
    ~Gate() {
        pthread_cond_destroy( &cond );
        pthread_mutex_destroy( &mutex );
    }
    //! Get current state of gate
    state_t get_state() const {
        return state;
    }
    //! Update state=value if state==comparand (flip==false) or state!=comparand (flip==true)
    void try_update( intptr_t value, intptr_t comparand, bool flip=false ) {
        __TBB_ASSERT( comparand!=0 || value!=0, "either value or comparand must be non-zero" );
        pthread_mutex_lock( &mutex );
        state_t old = state;
        if( flip ? old!=comparand : old==comparand ) {
            state = value;
            if( !old )
                pthread_cond_broadcast( &cond );
        }
        pthread_mutex_unlock( &mutex );
    }
    //! Wait for state!=0.
    void wait() {
        if( state==0 ) {
            pthread_mutex_lock( &mutex );
            while( state==0 ) {
                pthread_cond_wait( &cond, &mutex );
            }
            pthread_mutex_unlock( &mutex );
        }
    }
};

#else
#error Must define USE_PTHREAD or USE_WINTHREAD
#endif  /* threading kind */

} // namespace Internal

} // namespace ThreadingBuildingBlocks

#endif /* _TBB_Gate_H */
