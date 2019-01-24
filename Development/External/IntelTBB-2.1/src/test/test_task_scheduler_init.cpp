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

#include "tbb/task_scheduler_init.h"
#include <cstdlib>
#include "harness_assert.h"

//! Test that task::initialize and task::terminate work when doing nothing else.
/** maxthread is treated as the "maximum" number of worker threads. */
void InitializeAndTerminate( int maxthread ) {
    for( int i=0; i<200; ++i ) {
        switch( i&3 ) {
            default: {
                tbb::task_scheduler_init init( std::rand() % maxthread + 1 );
                ASSERT(init.is_active(), NULL);
                break;
            }
            case 0: {   
                tbb::task_scheduler_init init;
                ASSERT(init.is_active(), NULL);
                break;
            }
            case 1: {
                tbb::task_scheduler_init init( tbb::task_scheduler_init::automatic );
                ASSERT(init.is_active(), NULL);
                break;
            }
            case 2: {
                tbb::task_scheduler_init init( tbb::task_scheduler_init::deferred );
                ASSERT(!init.is_active(), "init should not be active; initialization was deferred");
                init.initialize( std::rand() % maxthread + 1 );
                ASSERT(init.is_active(), NULL);
                init.terminate();
                ASSERT(!init.is_active(), "init should not be active; it was terminated");
                break;
            }
        }
    }
}

#include <cstdio>
#include <stdexcept>
#include "harness.h"

#if _WIN64
namespace std {      // 64-bit Windows compilers have not caught up with 1998 ISO C++ standard
    using ::srand;
    using ::printf;
}
#endif /* _WIN64 */

struct ThreadedInit {
    void operator()( int ) const {
        try {
            InitializeAndTerminate(MaxThread);
        } catch( std::runtime_error& error ) {
            std::printf("ERROR: %s\n", error.what() );
        }
    }
};

#if _MSC_VER
#include <windows.h>
#include <tchar.h>
#endif /* _MSC_VER */

//! Test driver
int main(int argc, char* argv[]) {
#if _MSC_VER && !__TBB_NO_IMPLICIT_LINKAGE
    #ifdef _DEBUG
        ASSERT(!GetModuleHandle(_T("tbb.dll")) && GetModuleHandle(_T("tbb_debug.dll")),
            "debug application links with non-debug tbb library");
    #else
        ASSERT(!GetModuleHandle(_T("tbb_debug.dll")) && GetModuleHandle(_T("tbb.dll")),
            "non-debug application links with debug tbb library");
    #endif
#endif /* _MSC_VER && !__TBB_NO_IMPLICIT_LINKAGE */
    std::srand(2);
    // Set defaults
    MaxThread = MinThread = 2;
    ParseCommandLine( argc, argv );
    try {
        InitializeAndTerminate(MaxThread);
    } catch( std::runtime_error& error ) {
        std::printf("ERROR: %s\n", error.what() );
    }
    for( int p=MinThread; p<=MaxThread; ++p ) {
        if( Verbose ) printf("testing with %d threads\n", p );
        NativeParallelFor( p, ThreadedInit() );
    }
    std::printf("done\n");
    return 0;
}
