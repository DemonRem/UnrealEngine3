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

// Please define version number in the file:
#include "tbb/tbb_stddef.h"

// And don't touch anything below
#ifndef ENDL
#define ENDL "\n"
#endif
#include "version_string.tmp"

#ifndef __TBB_VERSION_STRINGS
#pragma message("Warning: version_string.tmp isn't generated properly by version_info.sh script!")
// here is an example of macros value:
#define __TBB_VERSION_STRINGS \
"TBB: BUILD_HOST\tUnknown\n" \
"TBB: BUILD_ARCH\tUnknown\n" \
"TBB: BUILD_OS\t\tUnknown\n" \
"TBB: BUILD_CL\t\tUnknown\n" \
"TBB: BUILD_COMPILER\tUnknown\n" \
"TBB: BUILD_COMMAND\tUnknown\n"
#endif
#ifndef __TBB_DATETIME
#ifdef RC_INVOKED
#define __TBB_DATETIME "Unknown"
#else
#define __TBB_DATETIME __DATE__ __TIME__
#endif
#endif

#define __TBB_VERSION_NUMBER "TBB: VERSION\t\t" __TBB_STRING(TBB_VERSION_MAJOR.TBB_VERSION_MINOR) ENDL
#define __TBB_INTERFACE_VERSION_NUMBER "TBB: INTERFACE VERSION\t" __TBB_STRING(TBB_INTERFACE_VERSION) ENDL
#define __TBB_VERSION_DATETIME "TBB: BUILD_DATE\t\t" __TBB_DATETIME ENDL
#ifndef TBB_USE_DEBUG
    #define __TBB_VERSION_USE_DEBUG "TBB: TBB_USE_DEBUG\tundefined" ENDL
#elif TBB_USE_DEBUG==0
    #define __TBB_VERSION_USE_DEBUG "TBB: TBB_USE_DEBUG\t0" ENDL
#elif TBB_USE_DEBUG==1
    #define __TBB_VERSION_USE_DEBUG "TBB: TBB_USE_DEBUG\t1" ENDL
#elif TBB_USE_DEBUG==2
    #define __TBB_VERSION_USE_DEBUG "TBB: TBB_USE_DEBUG\t2" ENDL
#else
    #error Unexpected value for TBB_USE_DEBUG
#endif
#ifndef TBB_USE_ASSERT
    #define __TBB_VERSION_USE_ASSERT "TBB: TBB_USE_ASSERT\tundefined" ENDL
#elif TBB_USE_ASSERT==0
    #define __TBB_VERSION_USE_ASSERT "TBB: TBB_USE_ASSERT\t0" ENDL
#elif TBB_USE_ASSERT==1
    #define __TBB_VERSION_USE_ASSERT "TBB: TBB_USE_ASSERT\t1" ENDL
#elif TBB_USE_ASSERT==2
    #define __TBB_VERSION_USE_ASSERT "TBB: TBB_USE_ASSERT\t2" ENDL
#else
    #error Unexpected value for TBB_USE_ASSERT
#endif
#ifndef DO_ITT_NOTIFY
    #define __TBB_VERSION_DO_NOTIFY "TBB: DO_ITT_NOTIFY\tundefined" ENDL
#elif DO_ITT_NOTIFY==1
    #define __TBB_VERSION_DO_NOTIFY "TBB: DO_ITT_NOTIFY\t1" ENDL
#elif DO_ITT_NOTIFY==0
    #define __TBB_VERSION_DO_NOTIFY
#else
    #error Unexpected value for DO_ITT_NOTIFY
#endif

#define TBB_VERSION_STRINGS __TBB_VERSION_NUMBER __TBB_INTERFACE_VERSION_NUMBER __TBB_VERSION_DATETIME __TBB_VERSION_STRINGS __TBB_VERSION_USE_DEBUG __TBB_VERSION_USE_ASSERT __TBB_VERSION_DO_NOTIFY

// numbers
#ifndef __TBB_VERSION_YMD
#define __TBB_VERSION_YMD 0, 0
#endif

#define TBB_VERNUMBERS TBB_VERSION_MAJOR, TBB_VERSION_MINOR, __TBB_VERSION_YMD

#define TBB_VERSION __TBB_STRING(TBB_VERNUMBERS)
