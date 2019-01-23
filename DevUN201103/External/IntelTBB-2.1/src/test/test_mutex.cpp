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

//------------------------------------------------------------------------
// Test TBB mutexes when used with parallel_for.h
//
// Usage: test_Mutex.exe [-v] nthread
//
// The -v option causes timing information to be printed.
//
// Compile with _OPENMP and -openmp
//------------------------------------------------------------------------
#include "tbb/spin_mutex.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/queuing_rw_mutex.h"
#include "tbb/queuing_mutex.h"
#include "tbb/mutex.h"
#include "tbb/recursive_mutex.h"
#include "tbb/null_mutex.h"
#include "tbb/null_rw_mutex.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"
#include "tbb/atomic.h"
#include "harness.h"
#include "harness_trace.h"
#include <cstdlib>
#include <cstdio>
#if _OPENMP
#include "test/OpenMP_Mutex.h"
#endif /* _OPENMP */
#include "tbb/tbb_profiling.h"

#ifndef TBBTEST_LOW_WORKLOAD
    #define TBBTEST_LOW_WORKLOAD TBB_USE_THREADING_TOOLS
#endif

// This test deliberately avoids a "using tbb" statement,
// so that the error of putting types in the wrong namespace will be caught.

template<typename M>
struct Counter {
    typedef M mutex_type;
    M mutex;
    volatile long value;
};

//! Function object for use with parallel_for.h.
template<typename C>
struct AddOne: NoAssign {
    C& counter;
    /** Increments counter once for each iteration in the iteration space. */
    void operator()( tbb::blocked_range<size_t>& range ) const {
        for( size_t i=range.begin(); i!=range.end(); ++i ) {
            if( i&1 ) {
                // Try implicit acquire and explicit release
                typename C::mutex_type::scoped_lock lock(counter.mutex);
                counter.value = counter.value+1;
                lock.release();
            } else {
                // Try explicit acquire and implicit release
                typename C::mutex_type::scoped_lock lock;
                lock.acquire(counter.mutex);
                counter.value = counter.value+1;
            }
        }
    }
    AddOne( C& counter_ ) : counter(counter_) {}
};

//! Generic test of a TBB mutex type M.
/** Does not test features specific to reader-writer locks. */
template<typename M>
void Test( const char * name ) {
    TRACE("%s time = ",name);
    Counter<M> counter;
    counter.value = 0;
#if __TBB_NAMING_API_SUPPORT 
    tbb::profiling::set_name(counter.mutex, name);
#endif /* __TBB_NAMING_API_SUPPORT */
#if TBBTEST_LOW_WORKLOAD
    const int n = 10000;
#else
    const int n = 100000;
#endif /* TBBTEST_LOW_WORKLOAD */
    tbb::tick_count t0 = tbb::tick_count::now();
    tbb::parallel_for(tbb::blocked_range<size_t>(0,n,n/10),AddOne<Counter<M> >(counter));
    tbb::tick_count t1 = tbb::tick_count::now();
    TRACE("%g usec\n",(t1-t0).seconds());
    if( counter.value!=n )
        std::printf("ERROR for %s: counter.value=%ld\n",name,counter.value);
}

template<typename M, size_t N>
struct Invariant {
    typedef M mutex_type;
    M mutex;
    const char* mutex_name;
    volatile long value[N];
    volatile long single_value;
    Invariant( const char* mutex_name_ ) :
        mutex_name(mutex_name_)
    {
        single_value = 0;
        for( size_t k=0; k<N; ++k )
            value[k] = 0;
#if __TBB_NAMING_API_SUPPORT 
        tbb::profiling::set_name(mutex, mutex_name_);
#endif /* __TBB_NAMING_API_SUPPORT */
    }
    void update() {
        for( size_t k=0; k<N; ++k )
            ++value[k];
    }
    bool value_is( long expected_value ) const {
        long tmp;
        for( size_t k=0; k<N; ++k )
            if( (tmp=value[k])!=expected_value ) {
                printf("ERROR: %ld!=%ld\n", tmp, expected_value);
                return false;
            }
        return true;
    }
    bool is_okay() {
        return value_is( value[0] );
    }
};

//! Function object for use with parallel_for.h.
template<typename I>
struct TwiddleInvariant: NoAssign {
    I& invariant;
    /** Increments counter once for each iteration in the iteration space. */
    void operator()( tbb::blocked_range<size_t>& range ) const {
        for( size_t i=range.begin(); i!=range.end(); ++i ) {
            //! Every 8th access is a write access
            bool write = (i%8)==7;
            bool okay = true;
            bool lock_kept = true;
            if( (i/8)&1 ) {
                // Try implicit acquire and explicit release
                typename I::mutex_type::scoped_lock lock(invariant.mutex,write);
                if( write ) {
                    long my_value = invariant.value[0];
                    invariant.update();
                    if( i%16==7 ) {
                        lock_kept = lock.downgrade_to_reader();
                        if( !lock_kept )
                            my_value = invariant.value[0] - 1;
                        okay = invariant.value_is(my_value+1);
                    }
                } else {
                    okay = invariant.is_okay();
                    if( i%8==3 ) {
                        long my_value = invariant.value[0];
                        lock_kept = lock.upgrade_to_writer();
                        if( !lock_kept )
                            my_value = invariant.value[0];
                        invariant.update();
                        okay = invariant.value_is(my_value+1);
                    }
                }
                lock.release();
            } else {
                // Try explicit acquire and implicit release
                typename I::mutex_type::scoped_lock lock;
                lock.acquire(invariant.mutex,write);
                if( write ) {
                    long my_value = invariant.value[0];
                    invariant.update();
                    if( i%16==7 ) {
                        lock_kept = lock.downgrade_to_reader();
                        if( !lock_kept )
                            my_value = invariant.value[0] - 1;
                        okay = invariant.value_is(my_value+1);
                    }
                } else {
                    okay = invariant.is_okay();
                    if( i%8==3 ) {
                        long my_value = invariant.value[0];
                        lock_kept = lock.upgrade_to_writer();
                        if( !lock_kept )
                            my_value = invariant.value[0];
                        invariant.update();
                        okay = invariant.value_is(my_value+1);
                    }
                }
            }
            if( !okay ) {
                std::printf( "ERROR for %s at %ld: %s %s %s %s\n",invariant.mutex_name, long(i),
                             write?"write,":"read,", write?(i%16==7?"downgrade,":""):(i%8==3?"upgrade,":""),
                             lock_kept?"lock kept,":"lock not kept,", (i/8)&1?"imp/exp":"exp/imp" );
            }
        }
    }
    TwiddleInvariant( I& invariant_ ) : invariant(invariant_) {}
};

/** This test is generic so that we can test any other kinds of ReaderWriter locks we write later. */
template<typename M>
void TestReaderWriterLock( const char * mutex_name ) {
    TRACE( "%s readers & writers time = ", mutex_name );
    Invariant<M,8> invariant(mutex_name);
#if TBBTEST_LOW_WORKLOAD
    const size_t n = 10000;
#else
    const size_t n = 500000;
#endif /* TBBTEST_LOW_WORKLOAD */
    tbb::tick_count t0 = tbb::tick_count::now();
    tbb::parallel_for(tbb::blocked_range<size_t>(0,n,n/100),TwiddleInvariant<Invariant<M,8> >(invariant));
    tbb::tick_count t1 = tbb::tick_count::now();
    // There is either a writer or a reader upgraded to a writer for each 4th iteration
    long expected_value = n/4;
    if( !invariant.value_is(expected_value) )
        std::printf("ERROR for %s: final invariant value is wrong\n",mutex_name);
    TRACE( "%g usec\n", (t1-t0).seconds() );
}

#if _MSC_VER && !defined(__INTEL_COMPILER)
    // Suppress "conditional expression is constant" warning.
    #pragma warning( push )
    #pragma warning( disable: 4127 )
#endif

/** Test try_acquire_reader functionality of a non-reenterable reader-writer mutex */
template<typename M>
void TestTryAcquireReader_OneThread( const char * mutex_name ) {
    M tested_mutex;
    typename M::scoped_lock lock1;
    if( M::is_rw_mutex ) {
        if( lock1.try_acquire(tested_mutex, false) )
            lock1.release();
        else
            std::printf("ERROR for %s: try_acquire failed though it should not\n", mutex_name);
        {
            typename M::scoped_lock lock2(tested_mutex, false);
            if( lock1.try_acquire(tested_mutex) )
                std::printf("ERROR for %s: try_acquire succeeded though it should not\n", mutex_name);
            lock2.release();
            lock2.acquire(tested_mutex, true);
            if( lock1.try_acquire(tested_mutex, false) )
                std::printf("ERROR for %s: try_acquire succeeded though it should not\n", mutex_name);
        }
        if( lock1.try_acquire(tested_mutex, false) )
            lock1.release();
        else
            std::printf("ERROR for %s: try_acquire failed though it should not\n", mutex_name);
    }
}

/** Test try_acquire functionality of a non-reenterable mutex */
template<typename M>
void TestTryAcquire_OneThread( const char * mutex_name ) {
    M tested_mutex;
    typename M::scoped_lock lock1;
    if( lock1.try_acquire(tested_mutex) )
        lock1.release();
    else
        std::printf("ERROR for %s: try_acquire failed though it should not\n", mutex_name);
    {
        if( M::is_recursive_mutex ) {
            typename M::scoped_lock lock2(tested_mutex);
            if( lock1.try_acquire(tested_mutex) )
                lock1.release();
            else
                std::printf("ERROR for %s: try_acquire on recursive lock failed though it should not\n", mutex_name);
            //windows.. -- both are recursive
        } else {
            typename M::scoped_lock lock2(tested_mutex);
            if( lock1.try_acquire(tested_mutex) )
                std::printf("ERROR for %s: try_acquire succeeded though it should not\n", mutex_name);
        }
    }
    if( lock1.try_acquire(tested_mutex) )
        lock1.release();
    else
        std::printf("ERROR for %s: try_acquire failed though it should not\n", mutex_name);
} 

#if _MSC_VER && !defined(__INTEL_COMPILER)
    #pragma warning( pop )
#endif

const int RecurN = 4;
int RecurArray[ RecurN ];
tbb::recursive_mutex RecurMutex[ RecurN ];

struct RecursiveAcquisition {
    /** x = number being decoded in base N
        max_lock = index of highest lock acquired so far
        mask = bit mask; ith bit set if lock i has been acquired. */
    void Body( size_t x, int max_lock=-1, unsigned int mask=0 ) const
    {
        int i = (int) (x % RecurN);
        bool first = (mask&1U<<i)==0;
        if( first ) {
            // first time to acquire lock
            if( i<max_lock ) 
                // out of order acquisition might lead to deadlock, so stop
                return;
            max_lock = i;
        }

        if( (i&1)!=0 ) {
            // acquire lock on location RecurArray[i] using explict acquire
            tbb::recursive_mutex::scoped_lock r_lock;
            r_lock.acquire( RecurMutex[i] );
            int a = RecurArray[i];
            ASSERT( (a==0)==first, "should be either a==0 if it is the first time to acquire the lock or a!=0 otherwise" );
            ++RecurArray[i];
            if( x ) 
                Body( x/RecurN, max_lock, mask|1U<<i );
            --RecurArray[i];
            ASSERT( a==RecurArray[i], "a is not equal to RecurArray[i]" );                        

            // release lock on location RecurArray[i] using explicit release; otherwise, use implicit one
            if( (i&2)!=0 ) r_lock.release();
        } else {
            // acquire lock on location RecurArray[i] using implicit acquire
            tbb::recursive_mutex::scoped_lock r_lock( RecurMutex[i] );
            int a = RecurArray[i];

            ASSERT( (a==0)==first, "should be either a==0 if it is the first time to acquire the lock or a!=0 otherwise" );

            ++RecurArray[i];
            if( x ) 
                Body( x/RecurN, max_lock, mask|1U<<i );
            --RecurArray[i];

            ASSERT( a==RecurArray[i], "a is not equal to RecurArray[i]" );                        

            // release lock on location RecurArray[i] using explicit release; otherwise, use implicit one
            if( (i&2)!=0 ) r_lock.release();
        }
    }

    void operator()( const tbb::blocked_range<size_t> &r ) const
    {   
        for( size_t x=r.begin(); x<r.end(); x++ ) {
            Body( x );
        }
    }
};

/** This test is generic so that we may test other kinds of recursive mutexes.*/
template<typename M>
void TestRecursiveMutex( const char * mutex_name )
{
#if __TBB_NAMING_API_SUPPORT
    for ( int i = 0; i < RecurN; ++i ) {
        tbb::profiling::set_name(RecurMutex[i], mutex_name);
    }
#endif /* __TBB_NAMING_API_SUPPORT */
    tbb::tick_count t0 = tbb::tick_count::now();
    tbb::parallel_for(tbb::blocked_range<size_t>(0,10000,500), RecursiveAcquisition());
    tbb::tick_count t1 = tbb::tick_count::now();
    TRACE( "%s recursive mutex time = %g usec\n", mutex_name, (t1-t0).seconds() );
}

template<typename C>
struct NullRecursive: NoAssign {
    void recurse_till( size_t i, size_t till ) const {
        if( i==till ) {
            counter.value = counter.value+1;
            return;
        }
        if( i&1 ) {
            typename C::mutex_type::scoped_lock lock2(counter.mutex);
            recurse_till( i+1, till );
            lock2.release();
        } else {
            typename C::mutex_type::scoped_lock lock2;
            lock2.acquire(counter.mutex);
            recurse_till( i+1, till );
        }
    }

    void operator()( tbb::blocked_range<size_t>& range ) const {
        typename C::mutex_type::scoped_lock lock(counter.mutex);
        recurse_till( range.begin(), range.end() );
    }
    NullRecursive( C& counter_ ) : counter(counter_) {
        ASSERT( C::mutex_type::is_recursive_mutex, "Null mutex should be a recursive mutex." );
    }
    C& counter;
};

template<typename M>
struct NullUpgradeDowngrade: NoAssign {
    void operator()( tbb::blocked_range<size_t>& range ) const {
        typename M::scoped_lock lock2;
        for( size_t i=range.begin(); i!=range.end(); ++i ) {
            if( i&1 ) {
                typename M::scoped_lock lock1(my_mutex, true) ;
                if( lock1.downgrade_to_reader()==false )
                    std::printf("ERROR for %s: downgrade should always succeed\n", name);
            } else {
                lock2.acquire( my_mutex, false );
                if( lock2.upgrade_to_writer()==false )
                    std::printf("ERROR for %s: upgrade should always succeed\n", name);
                lock2.release();
            }
        }
    }

    NullUpgradeDowngrade( M& m_, const char* n_ ) : my_mutex(m_), name(n_) {}
    M& my_mutex;
    const char* name;
} ;

template<typename M>
void TestNullMutex( const char * name ) {
    Counter<M> counter;
    counter.value = 0;
    const int n = 100;
    if( Verbose ) printf("%s ",name);
    {
        tbb::parallel_for(tbb::blocked_range<size_t>(0,n,10),AddOne<Counter<M> >(counter));
    }
    counter.value = 0;
    {
        tbb::parallel_for(tbb::blocked_range<size_t>(0,n,10),NullRecursive<Counter<M> >(counter));
    }

}

template<typename M>
void TestNullRWMutex( const char * name ) {
    if( Verbose ) printf("%s ",name);
    const int n = 100;
    M m;
    tbb::parallel_for(tbb::blocked_range<size_t>(0,n,10),NullUpgradeDowngrade<M>(m, name));
}

#include "tbb/task_scheduler_init.h"

int main( int argc, char * argv[] ) {
    // Default is to run on two threads
    MinThread = MaxThread = 2;
    ParseCommandLine( argc, argv );
    for( int p=MinThread; p<=MaxThread; ++p ) {
        tbb::task_scheduler_init init( p );
        TRACE( "testing with %d workers\n", static_cast<int>(p) );
#if TBBTEST_LOW_WORKLOAD
        // The amount of work is decreased in this mode to bring the length 
        // of the runs under tools into the tolerable limits.
        const int n = 1;
#else
        const int n = 3;
#endif
        // Run each test several times.
        for( int i=0; i<n; ++i ) {
            TestNullMutex<tbb::null_mutex>( "Null Mutex" );
            TestNullMutex<tbb::null_rw_mutex>( "Null RW Mutex" );
            TestNullRWMutex<tbb::null_rw_mutex>( "Null RW Mutex" );
            Test<tbb::spin_mutex>( "Spin Mutex" );
#if _OPENMP
            Test<OpenMP_Mutex>( "OpenMP_Mutex" );
#endif /* _OPENMP */
            Test<tbb::queuing_mutex>( "Queuing Mutex" );
            Test<tbb::mutex>( "Wrapper Mutex" );
            Test<tbb::recursive_mutex>( "Recursive Mutex" );
            Test<tbb::queuing_rw_mutex>( "Queuing RW Mutex" );
            Test<tbb::spin_rw_mutex>( "Spin RW Mutex" );

            TestTryAcquire_OneThread<tbb::spin_mutex>("Spin Mutex");
            TestTryAcquire_OneThread<tbb::queuing_mutex>("Queuing Mutex");
#if USE_PTHREAD 
            // under ifdef because on Windows tbb::mutex is reenterable and the test will fail
            TestTryAcquire_OneThread<tbb::mutex>("Wrapper Mutex");
#endif /* USE_PTHREAD */
            TestTryAcquire_OneThread<tbb::recursive_mutex>( "Recursive Mutex" );
            TestTryAcquire_OneThread<tbb::spin_rw_mutex>("Spin RW Mutex"); // only tests try_acquire for writers
            TestTryAcquire_OneThread<tbb::queuing_rw_mutex>("Queuing RW Mutex"); // only tests try_acquire for writers
            TestTryAcquireReader_OneThread<tbb::spin_rw_mutex>("Spin RW Mutex"); 
            TestTryAcquireReader_OneThread<tbb::queuing_rw_mutex>("Queuing RW Mutex"); 

            TestReaderWriterLock<tbb::queuing_rw_mutex>( "Queuing RW Mutex" );
            TestReaderWriterLock<tbb::spin_rw_mutex>( "Spin RW Mutex" );

            TestRecursiveMutex<tbb::recursive_mutex>( "Recursive Mutex" );
        }
        TRACE( "calling destructor for task_scheduler_init\n" );
    }
    std::printf("done\n");
    return 0;
}