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

#include "tbb/parallel_scan.h"
#include "tbb/blocked_range.h"
#include "harness_assert.h"

typedef tbb::blocked_range<long> Range;

static volatile bool ScanIsRunning = false;

//! Sum of 0..i with wrap around on overflow.
inline int TriangularSum( int i ) {
    return i&1 ? ((i>>1)+1)*i : (i>>1)*(i+1); 
}

//! Verify that sum is sum of integers in closed interval [start_index..finish_index].
/** line should be the source line of the caller */
static void VerifySum( long start_index, long finish_index, int sum, int line );

const int MAXN = 2000;

enum AddendFlag {
    UNUSED=0,
    USED_NONFINAL=1,
    USED_FINAL=2
};

//! Array recording how each addend was used. 
/** 'unsigned char' instead of AddendFlag for sake of compactness. */
static unsigned char AddendHistory[MAXN];

//! Set to 1 for debugging output 
#define PRINT_DEBUG 0

#include "tbb/atomic.h"
#if PRINT_DEBUG
#include <stdio.h>
tbb::atomic<long> NextBodyId;
#endif /* PRINT_DEBUG */

struct BodyId {
#if PRINT_DEBUG
    const int id;
    BodyId() : id(NextBodyId++) {}
#endif /* PRINT_DEBUG */
};

tbb::atomic<long> NumberOfLiveAccumulator;

static void Snooze( bool scan_should_be_running ) {
    ASSERT( ScanIsRunning==scan_should_be_running, NULL );
}

template<typename T>
class Accumulator: BodyId {
    T my_total;
    const T* my_array;
    T* my_sum;
    Range my_range;
    //! Equals this while object is fully constructed, NULL otherwise.
    /** Used to detect premature destruction and accidental bitwise copy. */
    Accumulator* self;
    Accumulator( const T array[], T sum[] ) :
        my_total(), my_array(array), my_sum(sum), my_range(-1,-1,1)
    {
        ++NumberOfLiveAccumulator;
        // Set self as last action of constructor, to indicate that object is fully constructed.
        self = this;
    }
    friend void TestAccumulator( int mode, int nthread );
public:
#if PRINT_DEBUG
    void print() const {
        printf("%d [%ld..%ld)\n", id,my_range.begin(),my_range.end() );
    }
#endif /* PRINT_DEBUG */
    ~Accumulator() {
#if PRINT_DEBUG
        printf("%d [%ld..%ld) destroyed\n",id,my_range.begin(),my_range.end() ); 
#endif /* PRINT_DEBUG */
        // Clear self as first action of destructor, to indicate that object is not fully constructed.
        self = 0;
        --NumberOfLiveAccumulator;
    }
    Accumulator( Accumulator& a, tbb::split ) : 
        my_total(0), my_array(a.my_array), my_sum(a.my_sum), my_range(-1,-1,1)  
    {
        ++NumberOfLiveAccumulator;
#if PRINT_DEBUG
        printf("%d forked from %d\n",id,a.id);
#endif /* PRINT_DEBUG */
        Snooze(true);
        // Set self as last action of constructor, to indicate that object is fully constructed.
        self = this;
    }
    template<typename Tag> 
    void operator()( const Range& r, Tag /*tag*/ ) {
        Snooze(true);
#if PRINT_DEBUG
        if( my_range.empty() )
            printf("%d computing %s [%ld..%ld)\n",id,Tag::is_final_scan()?"final":"lookahead",r.begin(),r.end() );
        else
            printf("%d computing %s [%ld..%ld) [%ld..%ld)\n",id,Tag::is_final_scan()?"final":"lookahead",my_range.begin(),my_range.end(),r.begin(),r.end());
#endif /* PRINT_DEBUG */
        ASSERT( !Tag::is_final_scan() || (my_range.begin()==0 && my_range.end()==r.begin()) || (my_range.empty() && r.begin()==0), NULL );
        for( long i=r.begin(); i<r.end(); ++i ) {
            my_total += my_array[i];
            if( Tag::is_final_scan() ) {
                ASSERT( AddendHistory[i]<USED_FINAL, "addend used 'finally' twice?" );
                AddendHistory[i] |= USED_FINAL;
                my_sum[i] = my_total;
                VerifySum( 0L, i, int(my_sum[i]), __LINE__ );
            } else {
                ASSERT( AddendHistory[i]==UNUSED, "addend used too many times" );
                AddendHistory[i] |= USED_NONFINAL;
            }   
        }
        if( my_range.empty() )
            my_range = r;
        else
            my_range = Range(my_range.begin(), r.end(), 1 );
        Snooze(true);
        ASSERT( self==this, "this Accumulator corrupted or prematurely destroyed" );
    }
    void reverse_join( const Accumulator& left ) {
#if PRINT_DEBUG
        printf("reverse join %d [%ld..%ld) %d [%ld..%ld)\n",
               left.id,left.my_range.begin(),left.my_range.end(),
               id,my_range.begin(),my_range.end());
#endif /* PRINT_DEBUG */
        Snooze(true);
        ASSERT( ScanIsRunning, NULL );     
        ASSERT( left.my_range.end()==my_range.begin(), NULL );
        my_total += left.my_total;
        my_range = Range( left.my_range.begin(), my_range.end(), 1 );
        ASSERT( ScanIsRunning, NULL );     
        Snooze(true);
        ASSERT( ScanIsRunning, NULL );     
        ASSERT( self==this, NULL );
        ASSERT( left.self==&left, NULL );
    }
    void assign( const Accumulator& other ) {
        my_total = other.my_total;
        my_range = other.my_range;
        ASSERT( self==this, NULL );
        ASSERT( other.self==&other, "other Accumulator corrupted or prematurely destroyed" );
    }
};

#include "tbb/tick_count.h"
#include "harness.h"

static void VerifySum( long start_index, long finish_index, int sum, int line ) {
    int expected = TriangularSum( finish_index ) - TriangularSum( start_index );
    if( expected!=sum ) {
        printf( "line %d: sum[%ld..%ld] should be = %d, but was computed as %d\n",
                line, start_index, finish_index, expected, sum );
        abort();
    }
}

void TestAccumulator( int mode, int nthread ) {
    typedef int T;
    T* addend = new T[MAXN];
    T* sum = new T[MAXN];
    for( long n=0; n<=MAXN; ++n ) {
        for( long i=0; i<MAXN; ++i ) {
            addend[i] = -1;
            sum[i] = -2;
            AddendHistory[i] = UNUSED;
        }
        for( long i=0; i<n; ++i )
            addend[i] = i;
        Accumulator<T> acc( addend, sum );
        tbb::tick_count t0 = tbb::tick_count::now();
#if PRINT_DEBUG
        printf("--------- mode=%d range=[0..%ld)\n",mode,n);
#endif /* PRINT_DEBUG */
        ScanIsRunning = true;

        switch (mode) {
            case 0:
                tbb::parallel_scan( Range( 0, n, 1 ), acc );
            break;
            case 1:
                tbb::parallel_scan( Range( 0, n, 1 ), acc, tbb::simple_partitioner() );
            break;
            case 2:
                tbb::parallel_scan( Range( 0, n, 1 ), acc, tbb::auto_partitioner() );
            break;
        }

        ScanIsRunning = false;
#if PRINT_DEBUG
        printf("=========\n");
#endif /* PRINT_DEBUG */
        Snooze(false);
        tbb::tick_count t1 = tbb::tick_count::now();
        long used_once_count = 0;
        for( long i=0; i<n; ++i ) 
            if( !(AddendHistory[i]&USED_FINAL) ) {
                printf("failed to use addend[%ld] %s\n",i,AddendHistory[i]&USED_NONFINAL?"(but used nonfinal)":"");
            }
        for( long i=0; i<n; ++i ) {
            VerifySum( 0, i, sum[i], __LINE__ );
            used_once_count += AddendHistory[i]==USED_FINAL;
        }
        if( n )
            ASSERT( acc.my_total==sum[n-1], NULL );
        else
            ASSERT( acc.my_total==0, NULL );
        if( Verbose ) 
            printf("time [n=%ld] = %g\tused_once%% = %g\tnthread=%d\n",n,(t1-t0).seconds(), n==0 ? 0 : 100.0*used_once_count/n,nthread);
    }
    delete[] addend;
    delete[] sum;
}

static void TestScanTags() {
    ASSERT( tbb::pre_scan_tag::is_final_scan()==false, NULL );
    ASSERT( tbb::final_scan_tag::is_final_scan()==true, NULL );
}

#include "tbb/task_scheduler_init.h"
#include "harness_cpu.h"

int main(int argc, char* argv[]) {
    // Default is to run on two threads.
    MinThread = MaxThread = 2;
    ParseCommandLine(argc,argv);
    TestScanTags();
    for( int p=MinThread; p<=MaxThread; ++p ) {
        for (int mode = 0; mode < 3; mode++) {
            tbb::task_scheduler_init init(p);
            NumberOfLiveAccumulator = 0;
            TestAccumulator(mode, p);

            // Test that all workers sleep when no work
            TestCPUUserTime(p);

            // Checking has to be done late, because when parallel_scan makes copies of
            // the user's "Body", the copies might be destroyed slightly after parallel_scan 
            // returns.
            ASSERT( NumberOfLiveAccumulator==0, NULL );
        }
    }
    printf("done\n");
    return 0;
}