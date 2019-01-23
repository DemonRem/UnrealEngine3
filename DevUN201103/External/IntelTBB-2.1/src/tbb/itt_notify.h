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

#ifndef _TBB_ITT_NOTIFY
#define _TBB_ITT_NOTIFY

#include "tbb/tbb_stddef.h"

// IMPORTANT: To use itt_notify.cpp/itt_notify.h in a library, exactly
// one of the files that #includes itt_notify.h should have
//     #define INSTANTIATE_ITT_NOTIFY 1
// just before the #include of "itt_notify.h".  
// That file is the one that will have the instances of the hooks.
//
// The intent is to allow precise placement of the hooks.  Ideally, they
// should reside on a hot mostly-read cache line.

namespace tbb {

//! Unicode support
#if _WIN32||_WIN64
    //! Unicode character type. Always wchar_t on Windows.
    /** We do not use typedefs from Windows TCHAR family to keep consistence of TBB coding style. **/
    typedef wchar_t tchar;
    //! Standard Windows macro to markup the string literals. 
    #define _T(string_literal) L ## string_literal
    #define tstrlen wcslen
#else /* !WIN */
    typedef char tchar;
    //! Standard Windows style macro to markup the string literals.
    #define _T(string_literal) string_literal
    #define tstrlen strlen
#endif /* !WIN */

} // namespace tbb

#if DO_ITT_NOTIFY

namespace tbb {

//! Identifies performance and correctness tools, which TBB sends special notifications to.
/** Enumerators must be ORable bit values.

    Initializing global tool indicator with TO_BE_INITIALIZED is required 
    to avoid bypassing early notification calls made through targeted macros until
    initialization is performed from somewhere else.

    Yet this entails another problem. The first targeted calls that happen to go
    into the proxy (dummy) handlers become promiscuous. **/
enum target_tool {
    NONE = 0ul,
    ITC = 1ul,
    ITP = 2ul,
    TO_BE_INITIALIZED = ~0ul
};

#if __TBB_NAMING_API_SUPPORT
    //! Display names of internal synchronization types
    extern const tchar 
            *SyncType_GlobalLock,
            *SyncType_Scheduler;
    //! Display names of internal synchronization components/scenarios
    extern const tchar 
            *SyncObj_LibraryInitialization,
            *SyncObj_SchedulerInitialization,
            *SyncObj_SchedulersList,
            *SyncObj_TaskStealingLoop,
            *SyncObj_WorkerTaskPool,
            *SyncObj_MasterTaskPool,
            *SyncObj_GateLock,
            *SyncObj_Gate,
            *SyncObj_SchedulerTermination,
            *SyncObj_ContextsList
            ;
#endif /* __TBB_NAMING_API_SUPPORT */

namespace internal {

//! Types of the tool notification functions (and corresponding proxy handlers). 
typedef void (*PointerToITT_Handler)(volatile void*);
typedef int  (*PointerToITT_thr_name_set)(const tchar*, int);
typedef void (*PointerToITT_thread_set_name)(const tchar*);

#if __TBB_NAMING_API_SUPPORT
typedef void (*PointerToITT_sync_create)(void* obj, const tchar* type, const tchar* name, int attribute);
typedef void (*PointerToITT_sync_rename)(void* obj, const tchar* new_name);

#if _WIN32||_WIN64
void itt_set_sync_name_v3( void* obj, const wchar_t* src ); 
#else
void itt_set_sync_name_v3( void* obj, const char* name); 
#endif
#endif /* __TBB_NAMING_API_SUPPORT */

extern PointerToITT_Handler ITT_Handler_sync_prepare;
extern PointerToITT_Handler ITT_Handler_sync_acquired;
extern PointerToITT_Handler ITT_Handler_sync_releasing;
extern PointerToITT_Handler ITT_Handler_sync_cancel;
extern PointerToITT_thr_name_set ITT_Handler_thr_name_set;
extern PointerToITT_thread_set_name ITT_Handler_thread_set_name;
#if __TBB_NAMING_API_SUPPORT
extern PointerToITT_sync_create ITT_Handler_sync_create;
extern PointerToITT_sync_rename ITT_Handler_sync_rename;
#endif /* __TBB_NAMING_API_SUPPORT */

extern target_tool current_tool;

} // namespace internal 

} // namespace tbb

//! Glues two tokens together.
#define ITT_HANDLER(name) tbb::internal::ITT_Handler_##name
#define CALL_ITT_HANDLER(name, arglist) ( ITT_HANDLER(name) ? (void)ITT_HANDLER(name)arglist : (void)0 )

//! Call routine itt_notify_(name) if corresponding handler is available.
/** For example, use ITT_NOTIFY(sync_releasing,x) to invoke __itt_notify_sync_releasing(x).
    Ordinarily, preprocessor token gluing games should be avoided.
    But here, it seemed to be the best way to handle the issue. */
#define ITT_NOTIFY(name,obj) CALL_ITT_HANDLER(name,(obj))
//! The same as ITT_NOTIFY but also checks if we are running under appropriate tool.
/** Parameter tools is an ORed set of target_tool enumerators. **/
#define ITT_NOTIFY_TOOL(tools,name,obj) ( ITT_HANDLER(name) && ((tools) & tbb::internal::current_tool) ? ITT_HANDLER(name)(obj) : (void)0 )

#define ITT_THREAD_SET_NAME(name) ( \
            ITT_HANDLER(thread_set_name) ? ITT_HANDLER(thread_set_name)(name)   \
                                         : CALL_ITT_HANDLER(thr_name_set,(name, tstrlen(name))) )

#if __TBB_NAMING_API_SUPPORT

/** 2 is the value of __itt_attr_mutex attribute. **/
#define ITT_SYNC_CREATE(obj, type, name) CALL_ITT_HANDLER(sync_create,(obj, type, name, 2))
#define ITT_SYNC_RENAME(obj, name) CALL_ITT_HANDLER(sync_rename,(obj, name))

#else /* !__TBB_NAMING_API_SUPPORT */

#define ITT_SYNC_CREATE(obj, type, name) ((void)0)
#define ITT_SYNC_RENAME(obj, name) ((void)0)

#endif /* !__TBB_NAMING_API_SUPPORT */

#else /* !DO_ITT_NOTIFY */

#define ITT_NOTIFY(name,obj) ((void)0)
#define ITT_NOTIFY_TOOL(tools,name,obj) ((void)0)
#define ITT_THREAD_SET_NAME(name) ((void)0)
#define ITT_SYNC_CREATE(obj, type, name) ((void)0)
#define ITT_SYNC_RENAME(obj, name) ((void)0)

#endif /* DO_ITT_NOTIFY */

#if DO_ITT_QUIET
#define ITT_QUIET(x) (__itt_thr_mode_set(__itt_thr_prop_quiet,(x)?__itt_thr_state_set:__itt_thr_state_clr))
#else
#define ITT_QUIET(x) ((void)0)
#endif /* DO_ITT_QUIET */

#endif /* _TBB_ITT_NOTIFY */
