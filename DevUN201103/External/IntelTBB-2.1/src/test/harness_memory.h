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

// Declarations for simple estimate of the memory being used by a program.
// Not yet implemented for Mac.
// This header is an optional part of the test harness.
// It assumes that "harness_assert.h" has already been included.

#if __linux__ || __sun
#include <sys/resource.h>
#include <unistd.h>

#elif __APPLE__
#include <unistd.h>
#include <mach/mach.h>
#include <mach/shared_memory_server.h>
#if SHARED_TEXT_REGION_SIZE || SHARED_DATA_REGION_SIZE
const size_t shared_size = SHARED_TEXT_REGION_SIZE+SHARED_DATA_REGION_SIZE;
#else
const size_t shared_size = 0;
#endif

#elif _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi")
#endif

//! Return estimate of number of bytes of memory that this program is currently using.
/* Returns 0 if not implemented on platform. */
static size_t GetMemoryUsage() { 
#if __linux__
    FILE* statsfile = fopen("/proc/self/statm","r");
    size_t pagesize = getpagesize();
    ASSERT(statsfile, NULL);
    long total_mem;
    int n = fscanf(statsfile,"%lu",&total_mem);
    if( n!=1 ) {
        printf("Warning: memory usage statistics wasn't obtained\n");
        return 0;
    }
    fclose(statsfile);
    return total_mem*pagesize;
#elif __APPLE__
    kern_return_t status;
    task_basic_info info;
    mach_msg_type_number_t msg_type = TASK_BASIC_INFO_COUNT;
    status = task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &msg_type);
    ASSERT(status==KERN_SUCCESS, NULL);
    return info.virtual_size - shared_size;
#elif _WIN32
    PROCESS_MEMORY_COUNTERS mem;
    bool status = GetProcessMemoryInfo(GetCurrentProcess(), &mem, sizeof(mem))!=0;
    ASSERT(status, NULL);
    return mem.PagefileUsage;
#else
    return 0;
#endif
}

