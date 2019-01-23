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

// Source file for miscellaneous entities that are infrequently referenced by 
// an executing program.

#include "tbb/tbb_stddef.h"
// Out-of-line TBB assertion handling routines are instantiated here.
#include "tbb_assert_impl.h"

#include "tbb_misc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(__EXCEPTIONS) || defined(_CPPUNWIND) || defined(__SUNPRO_CC)
    #include <string> // std::string is used to construct runtime_error
    #include <stdexcept>
#endif
#if __sun
    #include <alloca.h>     /* alloca */
#endif
#if _WIN32||_WIN64
    #include <malloc.h>     /* alloca */
#else
    #include <dlfcn.h>
#endif 

using namespace std;

#include "tbb/tbb_machine.h"

#include <iterator>

namespace tbb {

namespace internal {

#if defined(__EXCEPTIONS) || defined(_CPPUNWIND) || defined(__SUNPRO_CC)
// The above preprocessor symbols are defined by compilers when exception handling is enabled.
// However, in some cases it could be disabled for this file.

void handle_perror( int error_code, const char* what ) {
    char buf[128];
    sprintf(buf,"%s: ",what);
    char* end = strchr(buf,0);
    size_t n = buf+sizeof(buf)-end;
    strncpy( end, strerror( error_code ), n );
    // Ensure that buffer ends in terminator.
    buf[sizeof(buf)-1] = 0; 
    throw runtime_error(buf);
}
#endif //__EXCEPTIONS || _CPPUNWIND

bool GetBoolEnvironmentVariable( const char * name ) {
    if( const char* s = getenv(name) )
        return strcmp(s,"0") != 0;
    return false;
}

#if __TBB_WEAK_SYMBOLS

bool FillDynamicLinks( const char* /*library*/, const DynamicLinkDescriptor descriptors[], size_t n, size_t required )
{
    if ( required == ~(size_t)0 )
        required = n;
    __TBB_ASSERT( required<=n, "Number of required entry points exceeds their total number" );
    size_t k = 0;
    // Check if the first required entries are present in what was loaded into our process
    while ( k < required && descriptors[k].ptr )
        ++k;
    if ( k < required )
        return false;
    // Commit all the entry points.
    for ( k = 0; k < n; ++k )
        *descriptors[k].handler = (PointerToHandler) descriptors[k].ptr;
    return true;
}

#else /* !__TBB_WEAK_SYMBOLS */

bool FillDynamicLinks( void* module, const DynamicLinkDescriptor descriptors[], size_t n, size_t required )
{
    PointerToHandler *h = (PointerToHandler*)alloca(n * sizeof(PointerToHandler));
    if ( required == ~(size_t)0 )
        required = n;
    __TBB_ASSERT( required<=n, "Number of required entry points exceeds their total number" );
    size_t k = 0;
    for ( ; k < n; ++k ) {
#if _WIN32||_WIN64
        h[k] = (PointerToHandler) GetProcAddress( (HMODULE)module, descriptors[k].name );
#else
        h[k] = (PointerToHandler) dlsym( module, descriptors[k].name );
#endif /* _WIN32||_WIN64 */
        if ( !h[k] && k < required )
            return false;
    }
    __TBB_ASSERT( k == n, "if required entries are initialized, all entries are expected to be walked");
    // Commit the entry points.
    // Cannot use memset here, because the writes must be atomic.
    for( k = 0; k < n; ++k )
        *descriptors[k].handler = h[k];
    return true;
}

bool FillDynamicLinks( const char* library, const DynamicLinkDescriptor descriptors[], size_t n, size_t required )
{
#if _WIN32||_WIN64
    if ( FillDynamicLinks( GetModuleHandle(NULL), descriptors, n, required ) )
        // Target library was statically linked into this executable
        return true;
    // Prevent Windows from displaying silly message boxes if it fails to load library
    // (e.g. because of MS runtime problems - one of those crazy manifest related ones)
    UINT prev_mode = SetErrorMode (SEM_FAILCRITICALERRORS);
    void* module = LoadLibrary (library);
    SetErrorMode (prev_mode);
#else
    void* module = dlopen( library, RTLD_LAZY ); 
#endif /* _WIN32||_WIN64 */
    // Return true if the library is there and it contains all the expected entry points.
    return module != NULL  &&  FillDynamicLinks( module, descriptors, n, required );
}

#endif /* !__TBB_WEAK_SYMBOLS */


#include "tbb_version.h"

/** The leading "\0" is here so that applying "strings" to the binary delivers a clean result. */
static const char VersionString[] = "\0" TBB_VERSION_STRINGS;

static bool PrintVersionFlag = false;

void PrintVersion() {
    PrintVersionFlag = true;
    fputs(VersionString+1,stderr);
}

void PrintExtraVersionInfo( const char* category, const char* description ) {
    if( PrintVersionFlag ) 
        fprintf(stderr, "%s: %s\t%s\n", "TBB", category, description );
}

} // namespace internal
 
extern "C" int TBB_runtime_interface_version() {
    return TBB_INTERFACE_VERSION;
}

} // namespace tbb

#if __TBB_x86_32

#include "tbb/atomic.h"

//on Windows, int64_t defined in tbb::internal namespace only
#if _WIN32||_WIN64 
using tbb::internal::int64_t;
#endif

//! Warn about 8-byte store that crosses a cache line.
extern "C" void __TBB_machine_store8_slow_perf_warning( volatile void *ptr ) {
    // Report run-time warning unless we have already recently reported warning for that address.
    const unsigned n = 4;
    static tbb::atomic<void*> cache[n];
    static tbb::atomic<unsigned> k;
    for( unsigned i=0; i<n; ++i ) 
        if( ptr==cache[i] ) 
            goto done;
    cache[(k++)%n] = const_cast<void*>(ptr);
    tbb::internal::runtime_warning( "atomic store on misaligned 8-byte location %p is slow", ptr );
done:;
}

//! Handle 8-byte store that crosses a cache line.
extern "C" void __TBB_machine_store8_slow( volatile void *ptr, int64_t value ) {
    for( tbb::internal::AtomicBackoff b;; b.pause() ) {
        int64_t tmp = *(int64_t*)ptr;
        if( __TBB_machine_cmpswp8(ptr,value,tmp)==tmp ) 
            break;
    }
}

#endif /* __TBB_x86_32 */

#if __TBB_ipf
extern "C" intptr_t __TBB_machine_lockbyte( volatile unsigned char& flag ) {
    if ( !__TBB_TryLockByte(flag) ) {
        tbb::internal::AtomicBackoff b;
        do {
            b.pause();
        } while ( !__TBB_TryLockByte(flag) );
    }
    return 0;
}
#endif
