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

// Test for function template parallel_for.h

#include "tbb/parallel_for.h"
#include "tbb/atomic.h"
#include "harness_assert.h"
#include "harness.h"

static tbb::atomic<int> FooBodyCount;

//! An range object whose only public members are those required by the Range concept.
template<size_t Pad>
class FooRange {
    //! Start of range
    int start;

    //! Size of range
    int size;
    FooRange( int start_, int size_ ) : start(start_), size(size_) {
        zero_fill<char>(pad, Pad);
        pad[Pad-1] = 'x';
    }
    template<size_t Pad_> friend void Flog( int nthread );
    template<size_t Pad_> friend class FooBody;
    void operator&();

    char pad[Pad];
public:
    bool empty() const {return size==0;}
    bool is_divisible() const {return size>1;}
    FooRange( FooRange& original, tbb::split ) : size(original.size/2) {
        original.size -= size;
        start = original.start+original.size;
        ASSERT( original.pad[Pad-1]=='x', NULL );
        pad[Pad-1] = 'x';
    }
};

//! An range object whose only public members are those required by the parallel_for.h body concept.
template<size_t Pad>
class FooBody {
    static const int LIVE = 0x1234;
    tbb::atomic<int>* array;
    int state;
    friend class FooRange<Pad>;
    template<size_t Pad_> friend void Flog( int nthread );
    FooBody( tbb::atomic<int>* array_ ) : array(array_), state(LIVE) {}
public:
    ~FooBody() {
        --FooBodyCount;
        for( size_t i=0; i<sizeof(*this); ++i )
            reinterpret_cast<char*>(this)[i] = -1;
    }
    //! Copy constructor 
    FooBody( const FooBody& other ) : array(other.array), state(other.state) {
        ++FooBodyCount;
        ASSERT( state==LIVE, NULL );
    }
    void operator()( FooRange<Pad>& r ) const {
        for( int k=0; k<r.size; ++k )
            array[r.start+k]++;
    }
};

#include "tbb/tick_count.h"

static const int N = 1000;
static tbb::atomic<int> Array[N];

template<size_t Pad>
void Flog( int nthread ) {
    tbb::tick_count T0 = tbb::tick_count::now();
    for( int i=0; i<N; ++i ) {
        for ( int mode = 0; mode < 4; ++mode) 
        {
            FooRange<Pad> r( 0, i );
            const FooRange<Pad> rc = r;
            FooBody<Pad> f( Array );
            const FooBody<Pad> fc = f;
            memset( Array, 0, sizeof(Array) );
            FooBodyCount = 1;
            switch (mode) {
                case 0:
                    tbb::parallel_for( rc, fc );
                break;
                case 1:
                    tbb::parallel_for( rc, fc, tbb::simple_partitioner() );
                break;
                case 2:
                    tbb::parallel_for( rc, fc, tbb::auto_partitioner() );
                break;
                case 3: {
                    static tbb::affinity_partitioner affinity;
                    tbb::parallel_for( rc, fc, affinity );
                }
                break;
            }
            for( int j=0; j<i; ++j ) 
                ASSERT( Array[j]==1, NULL );
            for( int j=i; j<N; ++j ) 
                ASSERT( Array[j]==0, NULL );
            // Destruction of bodies might take a while, but there should be at most one body per thread
            // at this point.
            while( FooBodyCount>1 && FooBodyCount<=nthread )
                __TBB_Yield();
            ASSERT( FooBodyCount==1, NULL );
        }
    }
    tbb::tick_count T1 = tbb::tick_count::now();
    if( Verbose )
        printf("time=%g\tnthread=%d\tpad=%d\n",(T1-T0).seconds(),nthread,int(Pad));
}

#include <cstdio>
#include "tbb/task_scheduler_init.h"
#include "harness_cpu.h"

int main( int argc, char* argv[] ) {
    MinThread = 1;
    ParseCommandLine(argc,argv);
    if( MinThread<1 ) {
        printf("number of threads must be positive\n");
        exit(1);
    }
    for( int p=MinThread; p<=MaxThread; ++p ) {
        if( p>0 ) {
            tbb::task_scheduler_init init( p );
            Flog<1>(p);
            Flog<10>(p);
            Flog<100>(p);
            Flog<1000>(p);
            Flog<10000>(p);

           // Test that all workers sleep when no work
           TestCPUUserTime(p);
        }
    } 
    printf("done\n");
    return 0;
}
