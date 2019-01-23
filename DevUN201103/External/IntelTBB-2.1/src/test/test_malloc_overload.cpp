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


#if __linux__
#define MALLOC_REPLACEMENT_AVAILABLE 1
#endif

#if MALLOC_REPLACEMENT_AVAILABLE

#include "harness_assert.h"
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <new>

#include <dlfcn.h>
#include <unistd.h> // for sysconf
#include <stdint.h> // for uintptr_t


template<typename T>
static inline T alignDown(T arg, uintptr_t alignment) {
    return T( (uintptr_t)arg  & ~(alignment-1));
}
template<typename T>
static inline bool isAligned(T arg, uintptr_t alignment) {
    return 0==((uintptr_t)arg &  (alignment-1));
}

/* Below is part of MemoryAllocator.cpp. */

/*
 * The identifier to make sure that memory is allocated by scalable_malloc.
 */
const uint64_t theMallocUniqueID=0xE3C7AF89A1E2D8C1ULL; 

struct LargeObjectHeader {
    void        *unalignedResult;   /* The base of the memory returned from getMemory, this is what is used to return this to the OS */
    size_t       unalignedSize;     /* The size that was requested from getMemory */
    uint64_t     mallocUniqueID;    /* The field to check whether the memory was allocated by scalable_malloc */
    size_t       objectSize;        /* The size originally requested by a client */
};

/*
 * Objects of this size and larger are considered large objects.
 */
const uint32_t minLargeObjectSize = 8065;

/* end of inclusion from MemoryAllocator.cpp */

/* Correct only for arge blocks, i.e. not smaller then minLargeObjectSize */
static bool scalableMallocLargeBlock(void *object, size_t size)
{
    ASSERT(size >= minLargeObjectSize, NULL);

    LargeObjectHeader *h = (LargeObjectHeader*)((uintptr_t)object-sizeof(LargeObjectHeader));
    return h->mallocUniqueID==theMallocUniqueID && h->objectSize==size;
}

struct BigStruct {
    char f[minLargeObjectSize];
};

int main(int , char *[])
{
    void *ptr, *ptr1;

    if (NULL == dlsym(RTLD_DEFAULT, "scalable_malloc")) {
        printf("libtbbmalloc not found\nfail\n");
        return 1;
    }

    ptr = malloc(minLargeObjectSize);
    ASSERT(ptr!=NULL && scalableMallocLargeBlock(ptr, minLargeObjectSize), NULL);
    free(ptr);

    ptr = calloc(minLargeObjectSize, 2);
    ASSERT(ptr!=NULL && scalableMallocLargeBlock(ptr, minLargeObjectSize*2), NULL);
    ptr1 = realloc(ptr, minLargeObjectSize*10);
    ASSERT(ptr1!=NULL && scalableMallocLargeBlock(ptr1, minLargeObjectSize*10), NULL);
    free(ptr1);

    int ret = posix_memalign(&ptr, 1024, 3*minLargeObjectSize);
    ASSERT(0==ret && ptr!=NULL && scalableMallocLargeBlock(ptr, 3*minLargeObjectSize), NULL);
    free(ptr);

    ptr = memalign(128, 4*minLargeObjectSize);
    ASSERT(ptr!=NULL && scalableMallocLargeBlock(ptr, 4*minLargeObjectSize), NULL);
    free(ptr);

    ptr = valloc(minLargeObjectSize);
    ASSERT(ptr!=NULL && scalableMallocLargeBlock(ptr, minLargeObjectSize), NULL);
    free(ptr);

    long memoryPageSize = sysconf(_SC_PAGESIZE);
    int sz = 1024*minLargeObjectSize;
    ptr = pvalloc(sz);
    ASSERT(ptr!=NULL &&                // align size up to the page size
           scalableMallocLargeBlock(ptr, ((sz-1) | (memoryPageSize-1)) + 1), NULL);
    free(ptr);

    struct mallinfo info = mallinfo();
    // right now mallinfo initialized by zero
    ASSERT(!info.arena && !info.ordblks && !info.smblks && !info.hblks 
           && !info.hblkhd && !info.usmblks && !info.fsmblks 
           && !info.uordblks && !info.fordblks && !info.keepcost, NULL);

    BigStruct *f = new BigStruct;
    ASSERT(f!=NULL && scalableMallocLargeBlock(f, sizeof(BigStruct)), NULL);
    delete f;

    f = new BigStruct[10];
    ASSERT(f!=NULL && scalableMallocLargeBlock(f, 10*sizeof(BigStruct)), NULL);
    delete []f;

    f = new(std::nothrow) BigStruct;
    ASSERT(f!=NULL && scalableMallocLargeBlock(f, sizeof(BigStruct)), NULL);
    delete f;

    f = new(std::nothrow) BigStruct[2];
    ASSERT(f!=NULL && scalableMallocLargeBlock(f, 2*sizeof(BigStruct)), NULL);
    delete []f;

    printf("done\n");
    return 0;
}

#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness.h"

#else  /* MALLOC_REPLACEMENT_AVAILABLE */
#include <stdio.h>

int main(int , char *[])
{
    printf("skip\n");
}
#endif /* MALLOC_REPLACEMENT_AVAILABLE */
