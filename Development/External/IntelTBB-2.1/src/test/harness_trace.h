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

// Just the tracing portion of the harness.
//
// This header defines TRACE and TRCAENL macros, which use printf like syntax and 
// are useful for duplicating trace output to the standard debug output on Windows.
// It is possible to add the ability of automatic extending messages with additional
// info (file, line, function, time, thread ID, ...).
//
// Macros output nothing when test app runs in non-verbose mode (default).
//
// The full "harness.h" must be included before this header.

#ifndef tbb_tests_harness_trace_H
#define tbb_tests_harness_trace_H

#if defined(MAX_TRACE_SIZE) && MAX_TRACE_SIZE < 1024
    #undef MAX_TRACE_SIZE
#endif
#ifndef MAX_TRACE_SIZE
    #define MAX_TRACE_SIZE  1024
#endif

#include <cstdarg>

#if _WIN32||_WIN64
    #define snprintf _snprintf
#endif
#if defined(_MSC_VER) && (_MSC_VER<=1400)
    #define vsnprintf _vsnprintf
#endif

namespace harness_internal {

    struct tracer_t
    {
        int         my_flags;
        const char  *my_file;
        const char  *my_func;
        size_t      my_line;

        enum  { 
            prefix = 1,
            need_lf = 2
        };

	    tracer_t*  set_trace_info ( int flags, const char *file, size_t line, const char *func )
	    {
		    my_flags = flags;
		    my_line = line;
            my_file = file;
            my_func = func;
		    return  this;
	    }

        void  trace ( const char* fmt, ... )
        {
            if ( !Verbose )
                return;
            char    msg[MAX_TRACE_SIZE];
            char    msg_fmt_buf[MAX_TRACE_SIZE];
            const char  *msg_fmt = fmt;
            if ( my_flags & prefix ) {
                snprintf (msg_fmt_buf, MAX_TRACE_SIZE, "[%s] %s", my_func, fmt);
                msg_fmt = msg_fmt_buf;
            }
            std::va_list argptr;
            va_start (argptr, fmt);
            int len = vsnprintf (msg, MAX_TRACE_SIZE, msg_fmt, argptr);
            va_end (argptr);
            if ( my_flags & need_lf &&  
                 len < MAX_TRACE_SIZE - 1  &&  msg_fmt[len-1] != '\n' )
            {
                msg[len] = '\n';
                msg[len + 1] = 0;
            }
            printf ("%s",msg);
#if _WIN32 || _WIN64
            OutputDebugStringA(msg);
#endif
        }
    }; // class tracer_t

    static tracer_t tracer;

} // namespace harness_internal

#if defined(_MSC_VER)  &&  _MSC_VER >= 1300  ||  defined(__GNUC__)  ||  defined(__GNUG__)
	#define HARNESS_TRACE_ORIG_INFO __FILE__, __LINE__, __FUNCTION__
#else
	#define HARNESS_TRACE_ORIG_INFO __FILE__, __LINE__, ""
#endif


//! printf style tracing macro
/** This variant of TRACE adds trailing line-feed (new line) character, if it is absent. **/
#define TRACE  harness_internal::tracer.set_trace_info(harness_internal::tracer_t::need_lf, HARNESS_TRACE_ORIG_INFO)->trace

//! printf style tracing macro without automatic new line character adding
#define TRACENL harness_internal::tracer.set_trace_info(0, HARNESS_TRACE_ORIG_INFO)->trace

//! printf style tracing macro automatically prepending additional information
#define TRACEP harness_internal::tracer.set_trace_info(harness_internal::tracer_t::prefix | \
                                    harness_internal::tracer_t::need_lf, HARNESS_TRACE_ORIG_INFO)->trace

#endif /* tbb_tests_harness_trace_H */
