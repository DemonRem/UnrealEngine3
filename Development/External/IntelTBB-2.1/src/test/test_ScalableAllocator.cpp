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

// Test whether scalable_allocator complies with the requirements in 20.1.5 of ISO C++ Standard (1998).

#define HARNESS_NO_PARSE_COMMAND_LINE 1

#include "tbb/scalable_allocator.h"

// the actual body of the test is there:
#include "test_allocator.h"

#if _MSC_VER
#include <windows.h>
#endif /* _MSC_VER */

int main(void)
{
#if _MSC_VER && !__TBBMALLOC_NO_IMPLICIT_LINKAGE
    #ifdef _DEBUG
        ASSERT(!GetModuleHandle("tbbmalloc.dll") && GetModuleHandle("tbbmalloc_debug.dll"),
            "debug application links with non-debug tbbmalloc library");
    #else
        ASSERT(!GetModuleHandle("tbbmalloc_debug.dll") && GetModuleHandle("tbbmalloc.dll"),
            "non-debug application links with debug tbbmalloc library");
    #endif
#endif /* _MSC_VER && !__TBBMALLOC_NO_IMPLICIT_LINKAGE */
    int result = TestMain<tbb::scalable_allocator<void> >();
    printf("done\n");
    return result;
}
