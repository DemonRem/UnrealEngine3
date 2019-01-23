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

// Declarations for rock-bottom simple test harness.
// Just include this file to use it.
// Every test is presumed to have a command line of the form "foo [-v] [nthread]"
// The default for nthread is 2.

#if __SUNPRO_CC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <cstdio>
#include <cstdlib>
#include <cstring>
#endif
#include <new>

#if _WIN32||_WIN64
    #include <windows.h>
    #include <process.h>
#else
    #include <pthread.h>
#endif
#if __linux__
#include <sys/utsname.h> /* for uname */
#include <errno.h>       /* for use in LinuxKernelVersion() */
#endif

#if !HARNESS_NO_ASSERT
#include "harness_assert.h"

static void ReportError( int line, const char* expression, const char * message, bool is_error ) {
    if ( is_error ) {
        printf("Line %d, assertion %s: %s\n", line, expression, message ? message : "failed" );
#if TBB_EXIT_ON_ASSERT
        exit(1);
#else
        abort();
#endif /* TBB_EXIT_ON_ASSERT */
    }
    else
        printf("Warning: at line %d, assertion %s: %s\n", line, expression, message ? message : "failed" );
}
#else
#define ASSERT(x,y) ((void)0)
#endif /* HARNESS_NO_ASSERT */

#if !HARNESS_NO_PARSE_COMMAND_LINE
//! Controls level of commentary.
/** If true, makes the test print commentary.  If false, test should print "done" and nothing more. */
static bool Verbose;

//! Minimum number of threads
/** The default is 0, which is typically interpreted by tests as "run without TBB". */
static int MinThread = 0;

//! Maximum number of threads
static int MaxThread = 2;

//! Parse command line of the form "name [-v] [nthread]"
/** Sets Verbose, MinThread, and MaxThread accordingly.
    The nthread argument can be a single number or a range of the form m:n.
    A single number m is interpreted as if written m:m. 
    The numbers must be non-negative.  
    Clients often treat the value 0 as "run sequentially." */
static void ParseCommandLine( int argc, char* argv[] ) {
    int i = 1;  
    if( i<argc ) {
        if( strcmp( argv[i], "-v" )==0 ) {
            Verbose = true;
            ++i;
        }
    }
    if( i<argc ) {
        char* endptr;
        MinThread = strtol( argv[i], &endptr, 0 );
        if( *endptr==':' )
            MaxThread = strtol( endptr+1, &endptr, 0 );
        else if( *endptr=='\0' ) 
            MaxThread = MinThread;
        if( *endptr!='\0' ) {
            printf("garbled nthread range\n");
            exit(1);
        }    
        if( MinThread<0 ) {
            printf("nthread must be nonnegative\n");
            exit(1);
        }
        if( MaxThread<MinThread ) {
            printf("nthread range is backwards\n");
            exit(1);
        }
        ++i;
    }
    if( i!=argc ) {
        printf("Usage: %s [-v] [nthread|minthread:maxthread]\n", argv[0] );
        exit(1);
    }
}
#endif /* HARNESS_NO_PARSE_COMMAND_LINE */

//! Base class for prohibiting compiler-generated operator=
class NoAssign {
    //! Assignment not allowed
    void operator=( const NoAssign& );
public:
#if __GNUC__
    //! Explicitly define default construction, because otherwise gcc issues gratuitous warning.
    NoAssign() {}
#endif /* __GNUC__ */
};

//! Base class for prohibiting compiler-generated copy constructor or operator=
class NoCopy: NoAssign {
    //! Copy construction not allowed  
    NoCopy( const NoCopy& );
public:
    NoCopy() {}
};

//! For internal use by template function NativeParallelFor
template<typename Index, typename Body>
class NativeParallelForTask: NoCopy {
public:
    NativeParallelForTask( Index index_, const Body& body_ ) :
        index(index_),
        body(body_)
    {}

    //! Start task
    void start() {
#if _WIN32||_WIN64
        unsigned thread_id;
        thread_handle = (HANDLE)_beginthreadex( NULL, 0, thread_function, this, 0, &thread_id );
        ASSERT( thread_handle!=0, "NativeParallelFor: _beginthreadex failed" );
#else
#if __ICC==1100
    #pragma warning (push)
    #pragma warning (disable: 2193)
#endif /* __ICC==1100 */
        int status = pthread_create(&thread_id, NULL, thread_function, this);
        ASSERT(0==status, "NativeParallelFor: pthread_create failed");
#if __ICC==1100
    #pragma warning (pop)
#endif
#endif /* _WIN32||_WIN64 */
    }

    //! Wait for task to finish
    void wait_to_finish() {
#if _WIN32||_WIN64
        DWORD status = WaitForSingleObject( thread_handle, INFINITE );
        ASSERT( status!=WAIT_FAILED, "WaitForSingleObject failed" );
        CloseHandle( thread_handle );
#else
        int status = pthread_join( thread_id, NULL );
        ASSERT( !status, "pthread_join failed" );
#endif 
    }

private:
#if _WIN32||_WIN64
    HANDLE thread_handle;
#else
    pthread_t thread_id;
#endif

    //! Range over which task will invoke the body.
    const Index index;

    //! Body to invoke over the range.
    const Body body;

#if _WIN32||_WIN64
    static unsigned __stdcall thread_function( void* object )
#else
    static void* thread_function(void* object)
#endif
    {
        NativeParallelForTask& self = *static_cast<NativeParallelForTask*>(object);
        (self.body)(self.index);
        return 0;
    }
};

//! Execute body(i) in parallel for i in the interval [0,n).
/** Each iteartion is performed by a separate thread. */
template<typename Index, typename Body>
void NativeParallelFor( Index n, const Body& body ) {
    typedef NativeParallelForTask<Index,Body> task;

    if( n>0 ) {
        // Allocate array to hold the tasks
        task* array = static_cast<task*>(operator new( n*sizeof(task) ));

        // Construct the tasks
        for( Index i=0; i!=n; ++i ) 
            new( &array[i] ) task(i,body);

        // Start the tasks
        for( Index i=0; i!=n; ++i )
            array[i].start();

        // Wait for the tasks to finish and destroy each one.
        for( Index i=n; i; --i ) {
            array[i-1].wait_to_finish();
            array[i-1].~task();
        }

        // Deallocate the task array
        operator delete(array);
    }
}

//! The function to zero-initialize arrays; useful to avoid warnings
template <typename T>
void zero_fill(void* array, size_t N) {
    memset(array, 0, sizeof(T)*N);
}

#ifndef min
    //! Utility template function returning lesser of the two values.
    /** Provided here to avoid including not strict safe <algorithm>.\n
        In case operands cause signed/unsigned or size mismatch warnings it is caller's
        responsibility to do the appropriate cast before calling the function. **/
    template<typename T1, typename T2>
    const T1& min ( const T1& val1, const T2& val2 ) {
        return val1 < val2 ? val1 : val2;
    }
#endif /* !min */

#ifndef max
    //! Utility template function returning greater of the two values. Provided here to avoid including not strict safe <algorithm>.
    /** Provided here to avoid including not strict safe <algorithm>.\n
        In case operands cause signed/unsigned or size mismatch warnings it is caller's
        responsibility to do the appropriate cast before calling the function. **/
    template<typename T1, typename T2>
    const T1& max ( const T1& val1, const T2& val2 ) {
        return val1 < val2 ? val2 : val1;
    }
#endif /* !max */

#if __linux__
inline unsigned LinuxKernelVersion()
{
    unsigned a, b, c;
    struct utsname utsnameBuf;

    if (-1 == uname(&utsnameBuf)) {
        printf("Can't call uname: errno %d\n", errno);
        exit(1);
    }
    if (3 != sscanf(utsnameBuf.release, "%u.%u.%u", &a, &b, &c)) {
        printf("Unable to parse OS release '%s'\n", utsnameBuf.release);
        exit(1);
    }
    return 1000000*a+1000*b+c;
}
#endif
