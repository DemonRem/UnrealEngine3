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

#if !defined(INSTANTIATE_ITT_NOTIFY)
#define INSTANTIATE_ITT_NOTIFY 1
#endif
#include "itt_notify.h"
#include "tbb_misc.h"
#include "tbb/tbb_machine.h"
#include "tbb/cache_aligned_allocator.h" /* NFS_MaxLineSize */

#if _WIN32||_WIN64
    #include <windows.h>
#else /* !WIN */
    #include <dlfcn.h>
#if __TBB_WEAK_SYMBOLS
    #pragma weak __itt_notify_sync_prepare
    #pragma weak __itt_notify_sync_acquired
    #pragma weak __itt_notify_sync_releasing
    #pragma weak __itt_notify_sync_cancel
    #pragma weak __itt_thr_name_set
    #pragma weak __itt_thread_set_name
#if __TBB_NAMING_API_SUPPORT
    #pragma weak __itt_sync_create
    #pragma weak __itt_sync_rename
#endif /* __TBB_NAMING_API_SUPPORT */
    extern "C" {
        void __itt_notify_sync_prepare(void *p);
        void __itt_notify_sync_cancel(void *p);
        void __itt_notify_sync_acquired(void *p);
        void __itt_notify_sync_releasing(void *p);
        int __itt_thr_name_set (void* p, int len);
        void __itt_thread_set_name (const char* name);
#if __TBB_NAMING_API_SUPPORT
        void __itt_sync_create( void* obj, const char* name, const char* type, int attribute );
        void __itt_sync_rename( void* obj, const char* new_name );
#endif /* __TBB_NAMING_API_SUPPORT */
    }
#endif /* __TBB_WEAK_SYMBOLS */
#endif /* !WIN */

namespace tbb {
namespace internal {

#if DO_ITT_NOTIFY

//! Table describing the __itt_notify handlers.
static const DynamicLinkDescriptor ITT_HandlerTable[] = {
    DLD( __itt_notify_sync_prepare, ITT_Handler_sync_prepare),
    DLD( __itt_notify_sync_acquired, ITT_Handler_sync_acquired),
    DLD( __itt_notify_sync_releasing, ITT_Handler_sync_releasing),
    DLD( __itt_notify_sync_cancel, ITT_Handler_sync_cancel),
# if _WIN32||_WIN64
    DLD( __itt_thr_name_setW, ITT_Handler_thr_name_set),
    DLD( __itt_thread_set_nameW, ITT_Handler_thread_set_name),
# else
    DLD( __itt_thr_name_set, ITT_Handler_thr_name_set),
    DLD( __itt_thread_set_name, ITT_Handler_thread_set_name),
# endif /* _WIN32 || _WIN64 */

#if __TBB_NAMING_API_SUPPORT
# if _WIN32||_WIN64
    DLD( __itt_sync_createW, ITT_Handler_sync_create),
    DLD( __itt_sync_renameW, ITT_Handler_sync_rename)
# else
    DLD( __itt_sync_create, ITT_Handler_sync_create),
    DLD( __itt_sync_rename, ITT_Handler_sync_rename)
# endif
#endif /* __TBB_NAMING_API_SUPPORT */
};

static const int ITT_HandlerTable_size = 
    sizeof(ITT_HandlerTable)/sizeof(DynamicLinkDescriptor);

// LIBITTNOTIFY_NAME is the name of the ITT notification library 
# if _WIN32||_WIN64
#  define LIBITTNOTIFY_NAME "libittnotify.dll"
# elif __linux__
#  define LIBITTNOTIFY_NAME "libittnotify.so"
# else
#  error Intel(R) Threading Tools not provided for this OS
# endif

//! Performs tools support initialization.
/** Is called by DoOneTimeInitializations and ITT_DoOneTimeInitialization in 
    a protected (one-time) manner. Not to be invoked directly. **/
bool InitializeITT() {
    bool result = false;
    // Check if we are running under a performance or correctness tool
    bool t_checker = GetBoolEnvironmentVariable("KMP_FOR_TCHECK");
    bool t_profiler = GetBoolEnvironmentVariable("KMP_FOR_TPROFILE");
	__TBB_ASSERT(!(t_checker&&t_profiler), NULL);
    if ( t_checker || t_profiler ) {
        // Yes, we are in the tool mode. Try to load libittnotify library.
        result = FillDynamicLinks( LIBITTNOTIFY_NAME, ITT_HandlerTable, ITT_HandlerTable_size, 4 );
    }
    if (result){
        if ( t_checker ) {
            current_tool = ITC;
        } else if ( t_profiler ) {
            current_tool = ITP;
        }
    } else {
        // Clear away the proxy (dummy) handlers
        for (int i = 0; i < ITT_HandlerTable_size; i++)
            *ITT_HandlerTable[i].handler = NULL;
        current_tool = NONE;
    }
    PrintExtraVersionInfo( "ITT", result?"yes":"no" );
    return result;
}

#if !__TBB_NAMING_API_SUPPORT
    #define ITT_DoOneTimeInitialization DoOneTimeInitializations
#endif

//! Performs one-time initialization of tools interoperability mechanisms.
/** Defined in task.cpp. Makes a protected do-once call to InitializeITT(). **/
void ITT_DoOneTimeInitialization();

/** The following dummy_xxx functions are proxies that correspond to tool notification 
    APIs and are used to initialize corresponding pointers to the tool notifications
    (ITT_Handler_xxx). When the first call to ITT_Handler_xxx takes place before 
    the whole library initialization (done by DoOneTimeInitializations) happened,
    the proxy handler performs initialization of the tools support. After this
    ITT_Handler_xxx will be set to either tool notification pointer or NULL. **/
void dummy_sync_prepare( volatile void* ptr ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_prepare!=&dummy_sync_prepare, NULL );
    if (ITT_Handler_sync_prepare)
        (*ITT_Handler_sync_prepare) (ptr);
}

void dummy_sync_acquired( volatile void* ptr ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_acquired!=&dummy_sync_acquired, NULL );
    if (ITT_Handler_sync_acquired)
        (*ITT_Handler_sync_acquired) (ptr);
}

void dummy_sync_releasing( volatile void* ptr ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_releasing!=&dummy_sync_releasing, NULL );
    if (ITT_Handler_sync_releasing)
        (*ITT_Handler_sync_releasing) (ptr);
}

void dummy_sync_cancel( volatile void* ptr ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_cancel!=&dummy_sync_cancel, NULL );
    if (ITT_Handler_sync_cancel)
        (*ITT_Handler_sync_cancel) (ptr);
}

int dummy_thr_name_set( const tchar* str, int number ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_thr_name_set!=&dummy_thr_name_set, NULL );
    if (ITT_Handler_thr_name_set)
        return (*ITT_Handler_thr_name_set) (str, number);
    return -1;
}

void dummy_thread_set_name( const tchar* name ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_thread_set_name!=&dummy_thread_set_name, NULL );
    if (ITT_Handler_thread_set_name)
        (*ITT_Handler_thread_set_name)( name );
}

#if __TBB_NAMING_API_SUPPORT
void dummy_sync_create( void* obj, const tchar* objname, const tchar* objtype, int /*attribute*/ ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_create!=&dummy_sync_create, NULL );
    ITT_SYNC_CREATE( obj, objtype, objname );
}

void dummy_sync_rename( void* obj, const tchar* new_name ) {
    ITT_DoOneTimeInitialization();
    __TBB_ASSERT( ITT_Handler_sync_rename!=&dummy_sync_rename, NULL );
    ITT_SYNC_RENAME(obj, new_name);
}

#endif /* __TBB_NAMING_API_SUPPORT */

//! Leading padding before the area where tool notification handlers are placed.
/** Prevents cache lines where the handler pointers are stored from thrashing.
    Defined as extern to prevent compiler from placing the padding arrays separately
    from the handler pointers (which are declared as extern).
    Declared separately from definition to get rid of compiler warnings. **/
extern char __ITT_Handler_leading_padding[NFS_MaxLineSize];

//! Trailing padding after the area where tool notification handlers are placed.
extern char __ITT_Handler_trailing_padding[NFS_MaxLineSize];

char __ITT_Handler_leading_padding[NFS_MaxLineSize] = {0};
PointerToITT_Handler ITT_Handler_sync_prepare = &dummy_sync_prepare;
PointerToITT_Handler ITT_Handler_sync_acquired = &dummy_sync_acquired;
PointerToITT_Handler ITT_Handler_sync_releasing = &dummy_sync_releasing;
PointerToITT_Handler ITT_Handler_sync_cancel = &dummy_sync_cancel;
PointerToITT_thr_name_set ITT_Handler_thr_name_set = &dummy_thr_name_set;
PointerToITT_thread_set_name ITT_Handler_thread_set_name = &dummy_thread_set_name;
#if __TBB_NAMING_API_SUPPORT
PointerToITT_sync_create ITT_Handler_sync_create = &dummy_sync_create;
PointerToITT_sync_rename ITT_Handler_sync_rename = &dummy_sync_rename;
#endif /* __TBB_NAMING_API_SUPPORT */
char __ITT_Handler_trailing_padding[NFS_MaxLineSize] = {0};

target_tool current_tool = TO_BE_INITIALIZED;

#endif /* DO_ITT_NOTIFY */

void itt_store_pointer_with_release_v3( void* dst, void* src ) {
    ITT_NOTIFY(sync_releasing, dst);
    __TBB_store_with_release(*static_cast<void**>(dst),src);
}

void* itt_load_pointer_with_acquire_v3( const void* src ) {
    void* result = __TBB_load_with_acquire(*static_cast<void*const*>(src));
    ITT_NOTIFY(sync_acquired, const_cast<void*>(src));
    return result;
}

void* itt_load_pointer_v3( const void* src ) {
    void* result = *static_cast<void*const*>(src);
    return result;
}

#if __TBB_NAMING_API_SUPPORT
void itt_set_sync_name_v3( void *obj, const tchar* name) {
#if DO_ITT_NOTIFY
    __TBB_ASSERT( ITT_Handler_sync_rename!=&dummy_sync_rename, NULL );
#endif
    ITT_SYNC_RENAME(obj, name);
}
#endif /* __TBB_NAMING_API_SUPPORT */

} // namespace internal 

} // namespace tbb
