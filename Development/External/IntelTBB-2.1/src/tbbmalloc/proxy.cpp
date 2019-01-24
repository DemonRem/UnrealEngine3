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

#include "proxy.h"

#if MALLOC_LD_PRELOAD

/*** service functions and variables ***/

#include <unistd.h> // for sysconf
#include <dlfcn.h>

static long memoryPageSize;

static inline void initPageSize()
{
    memoryPageSize = sysconf(_SC_PAGESIZE);
}

/* For the expected behaviour (i.e., finding malloc/free/etc from libc.so, 
   not from ld-linux.so) dlsym(RTLD_NEXT) should be called from 
   a LD_PRELOADed library, not another dynamic library.
   So we have to put find_original_malloc here.
 */
extern "C" bool __TBB_internal_find_original_malloc(int num, const char *names[],
                                                        void *ptrs[])
{
    for (int i=0; i<num; i++)
        if (NULL == (ptrs[i] = dlsym (RTLD_NEXT, names[i])))
            return false;

    return true;
}

/* __TBB_malloc_proxy used as a weak symbol by libtbbmalloc for: 
   1) detection that the proxy library is loaded
   2) check that dlsym("malloc") found something different from our replacement malloc
*/
extern "C" void *__TBB_malloc_proxy() __attribute__ ((alias ("malloc")));

#ifndef __THROW
#define __THROW
#endif

/*** replacements for malloc and the family ***/

extern "C" {

void *malloc(size_t size) __THROW
{
    return __TBB_internal_malloc(size);
}

void * calloc(size_t num, size_t size) __THROW
{
    return __TBB_internal_calloc(num, size);
}

void free(void *object) __THROW
{
    __TBB_internal_free(object);
}

void * realloc(void* ptr, size_t sz) __THROW
{
    return __TBB_internal_realloc(ptr, sz);
}

int posix_memalign(void **memptr, size_t alignment, size_t size) __THROW
{
    return __TBB_internal_posix_memalign(memptr, alignment, size);
}

/* The older *NIX interface for aligned allocations;
   it's formally substituted by posix_memalign and deprecated,
   so we do not expect it to cause cyclic dependency with C RTL. */
void * memalign(size_t alignment, size_t size)  __THROW
{
    return scalable_aligned_malloc(size, alignment);
}

/* valloc allocates memory aligned on a page boundary */
void * valloc(size_t size) __THROW
{
    if (! memoryPageSize) initPageSize();

    return scalable_aligned_malloc(size, memoryPageSize);
}

/* pvalloc allocates smallest set of complete pages which can hold 
   the requested number of bytes. Result is aligned on page boundary. */
void * pvalloc(size_t size) __THROW
{
    if (! memoryPageSize) initPageSize();
	// align size up to the page size
	size = ((size-1) | (memoryPageSize-1)) + 1;

    return scalable_aligned_malloc(size, memoryPageSize);
}

int mallopt(int /*param*/, int /*value*/) __THROW
{
    return 1;
}

} /* extern "C" */

#if __linux__
#include <malloc.h>
#include <string.h> // for memset

extern "C" struct mallinfo mallinfo() __THROW
{
    struct mallinfo m;
    memset(&m, 0, sizeof(struct mallinfo));

    return m;
}
#endif /* __linux__ */

/*** replacements for global operators new and delete ***/

#include <new>

void * operator new(size_t sz) throw (std::bad_alloc) {
    void *res = scalable_malloc(sz);
    if (NULL == res) throw std::bad_alloc();
    return res;
}
void* operator new[](size_t sz) throw (std::bad_alloc) {
    void *res = scalable_malloc(sz);
    if (NULL == res) throw std::bad_alloc();
    return res;
}
void operator delete(void* ptr) throw() {
    scalable_free(ptr);
}
void operator delete[](void* ptr) throw() {
    scalable_free(ptr);
}
void* operator new(size_t sz, const std::nothrow_t&) throw() {
    return scalable_malloc(sz);
}
void* operator new[](std::size_t sz, const std::nothrow_t&) throw() {
    return scalable_malloc(sz);
}
void operator delete(void* ptr, const std::nothrow_t&) throw() {
    scalable_free(ptr);
}
void operator delete[](void* ptr, const std::nothrow_t&) throw() {
    scalable_free(ptr);
}

#endif /* MALLOC_LD_PRELOAD */
