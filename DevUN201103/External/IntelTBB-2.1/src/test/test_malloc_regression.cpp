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

#define HARNESS_NO_PARSE_COMMAND_LINE 1

#include <stdio.h>
#include "tbb/scalable_allocator.h"

class minimalAllocFree {
public:
    void operator()(int size) const {
        tbb::scalable_allocator<char> a;
        char* str = a.allocate( size );
        a.deallocate( str, size );
    }
};

#include "harness.h"

template<typename Body, typename Arg>
void RunThread(const Body& body, const Arg& arg) {
    NativeParallelForTask<Arg,Body> job(arg, body);
    job.start();
    job.wait_to_finish();
}

#include "harness_memory.h"

// The regression test for bug #1518 where thread boot strap allocations "leaked"
bool test_bootstrap_leak(void) {
    // Check whether memory usage data can be obtained; if not, skip the test.
    if( !GetMemoryUsage() )
        return true;

    /* In the bug 1518, each thread leaked ~384 bytes.
       Initially, scalable allocator maps 1MB. Thus it is necessary to take out most of this space.
       1MB is chunked into 16K blocks; of those, one block is for thread boot strap, and one more 
       should be reserved for the test body. 62 blocks left, each can serve 15 objects of 1024 bytes.
    */
    const int alloc_size = 1024;
    const int take_out_count = 15*62;

    tbb::scalable_allocator<char> a;
    char* array[take_out_count];
    for( int i=0; i<take_out_count; ++i )
        array[i] = a.allocate( alloc_size );

    RunThread( minimalAllocFree(), alloc_size ); // for threading library to take some memory
    size_t memory_in_use = GetMemoryUsage();
    // Wait for memory usage data to "stabilize". The test number (1000) has nothing underneath.
    for( int i=0; i<1000; i++) {
        if( GetMemoryUsage()!=memory_in_use ) {
            memory_in_use = GetMemoryUsage();
            i = -1;
        }
    }

    // Notice that 16K boot strap memory block is enough to serve 42 threads.
    const int num_thread_runs = 200;
    for( int i=0; i<num_thread_runs; ++i )
        RunThread( minimalAllocFree(), alloc_size );

    ptrdiff_t memory_leak = GetMemoryUsage() - memory_in_use;
    if( memory_leak>0 ) { // possibly too strong?
        printf( "Error: memory leak of up to %ld bytes\n", static_cast<long>(memory_leak));
    }

    for( int i=0; i<take_out_count; ++i )
        a.deallocate( array[i], alloc_size );

    return memory_leak<=0;
}

int main( int /*argc*/, char* argv[] ) {
    bool passed = true;

    passed &= test_bootstrap_leak();

    if(passed) printf("done\n");
    else       printf("%s failed\n", argv[0]);

    return passed?0:1;
}
