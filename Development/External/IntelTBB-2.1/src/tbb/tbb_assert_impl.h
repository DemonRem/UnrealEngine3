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

// IMPORTANT: To use assertion handling in TBB, exactly one of the TBB source files
// should #include tbb_assert_impl.h thus instantiating assertion handling routines.
// The intent of putting it to a separate file is to allow some tests to use it
// as well in order to avoid dependency on the library.

// include headers for required function declarations
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#if _WIN32||_WIN64
#include <crtdbg.h>
#endif

#if _MSC_VER >= 1400
#define __TBB_EXPORTED_FUNC   __cdecl
#else
#define __TBB_EXPORTED_FUNC
#endif

using namespace std;

namespace tbb {
    //! Type for an assertion handler
    typedef void(*assertion_handler_type)( const char* filename, int line, const char* expression, const char * comment );

    static assertion_handler_type assertion_handler;

    assertion_handler_type __TBB_EXPORTED_FUNC set_assertion_handler( assertion_handler_type new_handler ) {
        assertion_handler_type old_handler = assertion_handler;
        assertion_handler = new_handler;
        return old_handler;
    }

    void __TBB_EXPORTED_FUNC assertion_failure( const char* filename, int line, const char* expression, const char* comment ) {
        if( assertion_handler_type a = assertion_handler ) {
            (*a)(filename,line,expression,comment);
        } else {
            static bool already_failed;
            if( !already_failed ) {
                already_failed = true;
                fprintf( stderr, "Assertion %s failed on line %d of file %s\n",
                         expression, line, filename );
                if( comment )
                    fprintf( stderr, "Detailed description: %s\n", comment );
#if (_WIN32||_WIN64) && defined(_DEBUG)
                if(1 == _CrtDbgReport(_CRT_ASSERT, filename, line, "tbb_debug.dll", "%s\r\n%s", expression, comment?comment:""))
                        _CrtDbgBreak();
#else
                abort();
#endif
            }
        }
    }

#if defined(_MSC_VER)&&_MSC_VER<1400
#   define vsnprintf _vsnprintf
#endif

    namespace internal {
        //! Report a runtime warning.
        void __TBB_EXPORTED_FUNC runtime_warning( const char* format, ... )
        {
            char str[1024]; memset(str, 0, 1024);
            va_list args; va_start(args, format);
            vsnprintf(str, 1024-1, format, args);
            fprintf(stderr, "TBB Warning: %s\n", str);
        }
    } // namespace internal

} /* namespace tbb */
