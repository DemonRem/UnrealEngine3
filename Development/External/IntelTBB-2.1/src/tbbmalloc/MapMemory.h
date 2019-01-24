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

#ifndef _itt_shared_malloc_MapMemory_H
#define _itt_shared_malloc_MapMemory_H

#if __linux__ || __APPLE__
#include <sys/mman.h>

#define MEMORY_MAPPING_USES_MALLOC 0
void* MapMemory (size_t bytes)
{
    void* result = 0;
#ifndef MAP_ANONYMOUS
// Mac OS* X defines MAP_ANON, which is deprecated in Linux.
#define MAP_ANONYMOUS MAP_ANON
#endif /* MAP_ANONYMOUS */
    result = mmap(result, bytes, (PROT_READ | PROT_WRITE), MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    return result==MAP_FAILED? 0: result;
}

int UnmapMemory(void *area, size_t bytes)
{
    return munmap(area, bytes);
}

#elif _WIN32 || _WIN64
#include <windows.h>

#define MEMORY_MAPPING_USES_MALLOC 0
void* MapMemory (size_t bytes)
{
    /* Is VirtualAlloc thread safe? */
    return VirtualAlloc(NULL, bytes, (MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN), PAGE_READWRITE);
}

int UnmapMemory(void *area, size_t bytes)
{
    BOOL result = VirtualFree(area, 0, MEM_RELEASE);
    return !result;
}

#else
#include <stdlib.h>

void* MapMemory (size_t bytes)
{
    return malloc( bytes );
}

int UnmapMemory(void *area, size_t bytes)
{
    free( area );
    return 0;
}

#endif /* OS dependent */

#endif /* _itt_shared_malloc_MapMemory_H */
