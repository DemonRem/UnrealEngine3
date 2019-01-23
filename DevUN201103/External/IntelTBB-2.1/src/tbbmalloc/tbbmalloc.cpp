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

#define INSTANTIATE_ITT_NOTIFY 1
#include "TypeDefinitions.h" // Customize.h and proxy.h get included

#define ITT_DoOneTimeInitialization __TBB_malloc_ITT_DoOneTimeInitialization_stub
#include "tbb/itt_notify.cpp"

#if MALLOC_LD_PRELOAD

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#if __sun
#include <string.h> /* for memset */
#include <errno.h>
#endif

extern "C" {

void   safer_scalable_free( void*, void (*)(void*) );
void * safer_scalable_realloc( void*, size_t, void* (*)(void*,size_t) );

bool __TBB_internal_find_original_malloc(int num, const char *names[], void *table[])  __attribute__ ((weak));

}

#define malloc_proxy __TBB_malloc_proxy

void* (*original_malloc_ptr)(size_t) = 0;
void  (*original_free_ptr)(void*) = 0;
static void* (*original_calloc_ptr)(size_t,size_t) = 0;
static void* (*original_realloc_ptr)(void*,size_t) = 0;

#endif /* MALLOC_LD_PRELOAD */

namespace tbb {
namespace internal {

void ITT_DoOneTimeInitialization() {} // required for itt_notify.cpp to work

#if DO_ITT_NOTIFY
/** Caller is responsible for ensuring this routine is called exactly once. */
void MallocInitializeITT() {
    bool success = false;
    // Check if we are running under control of VTune.
    if( GetBoolEnvironmentVariable("KMP_FOR_TCHECK") || GetBoolEnvironmentVariable("KMP_FOR_TPROFILE") ) {
        // Yes, we are under control of VTune.  Check for libittnotify library.
        success = FillDynamicLinks( LIBITTNOTIFY_NAME, ITT_HandlerTable, 5 );
    }
    if (!success){
        for (int i = 0; i < 5; i++)
            *ITT_HandlerTable[i].handler = NULL;
    }
}
#endif /* DO_ITT_NOTIFY */

void init_tbbmalloc() {
#if MALLOC_LD_PRELOAD
    if (malloc_proxy && __TBB_internal_find_original_malloc) {
        const char *alloc_names[] = { "malloc", "free", "realloc", "calloc"};
        void *orig_alloc_ptrs[4];

        if (__TBB_internal_find_original_malloc(4, alloc_names, orig_alloc_ptrs)) {
            (void *&)original_malloc_ptr  = orig_alloc_ptrs[0];
            (void *&)original_free_ptr    = orig_alloc_ptrs[1];
            (void *&)original_realloc_ptr = orig_alloc_ptrs[2];
            (void *&)original_calloc_ptr  = orig_alloc_ptrs[3];
            MALLOC_ASSERT( original_malloc_ptr!=malloc_proxy,
                           "standard malloc not found" );
/* It's workaround for a bug in GNU Libc 2.9 (as it shipped with Fedora 10).
   1st call to libc's malloc should be not from threaded code.
 */
            original_free_ptr(original_malloc_ptr(1024));
            original_malloc_found = 1;
        }
    }
#endif /* MALLOC_LD_PRELOAD */
#if DO_ITT_NOTIFY
    MallocInitializeITT();
#endif
}

#if !(_WIN32||_WIN64)
struct RegisterProcessShutdownNotification {
    ~RegisterProcessShutdownNotification() {
        mallocProcessShutdownNotification();
    }
};

static RegisterProcessShutdownNotification reg;
#endif

} } // namespaces

#ifdef _WIN32
#include <windows.h>

BOOL WINAPI DllMain( HINSTANCE hInst, DWORD callReason, LPVOID )
{

    if (callReason==DLL_THREAD_DETACH)
    {
        mallocThreadShutdownNotification(NULL);
    }
    else if (callReason==DLL_PROCESS_DETACH)
    {
        mallocProcessShutdownNotification();
    }
    return TRUE;
}

#endif //_WIN32

#if MALLOC_LD_PRELOAD

bool  original_malloc_found;

pthread_t   recursive_malloc_call_thread;
int         recursive_malloc_call_flag;

void lockRecursiveMallocFlag()
{
    recursive_malloc_call_thread = pthread_self();
    recursive_malloc_call_flag = 1;
}
void unlockRecursiveMallocFlag()
{
    recursive_malloc_call_flag = 0;
}
static inline bool underRecursiveMallocFlag()
{
    return recursive_malloc_call_flag
        && pthread_equal( recursive_malloc_call_thread, pthread_self());
}

extern "C" {

void * __TBB_internal_malloc(size_t size)
{
    if ( underRecursiveMallocFlag() ) {
        if ( original_malloc_found ){
            return original_malloc_ptr(size);
        }else{
            return NULL;
        }
    }else{
        return (void*)scalable_malloc(size);
    }
}

void * __TBB_internal_calloc(size_t num, size_t size)
{
    if ( underRecursiveMallocFlag() ) {
        if ( original_malloc_found ){
#if __sun
            /* There seem to be run time problems on Solaris if original_calloc_ptr is used. */
            size_t arraySize = num * size;
            void *result = original_malloc_ptr(arraySize);
            if (result)
                memset(result, 0, arraySize);
            return result;
#else
            return original_calloc_ptr(num, size);
#endif
        }else{
            return NULL;
        }
    }else{
        return scalable_calloc(num, size);
    }
}

int __TBB_internal_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    MALLOC_ASSERT( !underRecursiveMallocFlag(), 
                   "posix_memalign not expected to create a cyclic dependency" );
    return scalable_posix_memalign(memptr, alignment, size);
}

void* __TBB_internal_realloc(void* ptr, size_t sz)
{
    return safer_scalable_realloc(ptr, sz, original_realloc_ptr);
}

void __TBB_internal_free(void *object)
{
    safer_scalable_free(object, original_free_ptr);
}

} /* extern "C" */

#endif /* MALLOC_LD_PRELOAD */
