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

#include "TypeDefinitions.h"

#if USE_PTHREAD
    // Some pthreads documentation says that <pthreads.h> must be first header.
    #include <pthread.h>
    #define TlsSetValue_func pthread_setspecific
    #define TlsGetValue_func pthread_getspecific
    typedef pthread_key_t tls_key_t;
    #include <sched.h>
    inline void do_yield() {sched_yield();}

#elif USE_WINTHREAD
    #define _WIN32_WINNT 0x0400
    #include <windows.h>
    #define TlsSetValue_func TlsSetValue
    #define TlsGetValue_func TlsGetValue
    typedef DWORD tls_key_t;
    inline void do_yield() {SwitchToThread();}

#else
    #error Must define USE_PTHREAD or USE_WINTHREAD

#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/********* Various compile-time options        **************/

#define MALLOC_TRACE 0

#if MALLOC_TRACE
#define TRACEF(x) printf x
#else
#define TRACEF(x) ((void)0)
#endif /* MALLOC_TRACE */

#define ASSERT_TEXT NULL

//! Define the main synchronization method
/** It should be specified before including LifoQueue.h */
#define FINE_GRAIN_LOCKS
#include "LifoQueue.h"

#define COLLECT_STATISTICS 0
#include "Statistics.h"

#define FREELIST_NONBLOCKING 1

// If USE_MALLOC_FOR_LARGE_OBJECT is nonzero, then large allocations are done via malloc.
// Otherwise large allocations are done using the scalable allocator's block allocator.
// As of 06.Jun.17, using malloc is about 10x faster on Linux.
#define USE_MALLOC_FOR_LARGE_OBJECT 1

extern "C" {
    void * scalable_malloc(size_t size);
    void   scalable_free(void *object);
}

/********* End compile-time options        **************/

#if MALLOC_LD_PRELOAD 
#include <new> /* for placement new */
#endif

namespace ThreadingSubstrate {

namespace Internal {

/******* A helper class to support overriding malloc with scalable_malloc *******/
#if MALLOC_LD_PRELOAD 
class recursive_malloc_call_protector {
    static MallocMutex rmc_mutex;

    MallocMutex::scoped_lock* lock_acquired;
    char scoped_lock_space[sizeof(MallocMutex::scoped_lock)+1];
public:
    recursive_malloc_call_protector(bool condition) : lock_acquired(NULL) {
        if (condition) {
            lock_acquired = new (scoped_lock_space) MallocMutex::scoped_lock( rmc_mutex );
            lockRecursiveMallocFlag();
        }
    }
    ~recursive_malloc_call_protector() {
        if (lock_acquired) {
            unlockRecursiveMallocFlag();
            lock_acquired->~scoped_lock();
        }
    }
};

MallocMutex recursive_malloc_call_protector::rmc_mutex;

#define malloc_proxy __TBB_malloc_proxy

#else /* MALLOC_LD_PRELOAD */
class recursive_malloc_call_protector {
public:
    recursive_malloc_call_protector(bool) {}
    ~recursive_malloc_call_protector() {}
};
const bool malloc_proxy = false;
#endif /* MALLOC_LD_PRELOAD */

/*********** Code to provide thread ID and a thread-local void pointer **********/

typedef intptr_t ThreadId;

static ThreadId ThreadIdCount;

static tls_key_t TLS_pointer_key;
static tls_key_t Tid_key;

static inline ThreadId  getThreadId(void)
{
    ThreadId result;
    result = reinterpret_cast<ThreadId>(TlsGetValue_func(Tid_key));
    if( !result ) {
        recursive_malloc_call_protector scoped(/*condition=*/malloc_proxy);
        // Thread-local value is zero -> first call from this thread,
        // need to initialize with next ID value (IDs start from 1)
        result = AtomicIncrement(ThreadIdCount); // returned new value!
        TlsSetValue_func( Tid_key, reinterpret_cast<void*>(result) );
    }
    return result;
}

static inline void* getThreadMallocTLS() {
    void *result;
    result = TlsGetValue_func( TLS_pointer_key );
// The assert below is incorrect: with lazy initialization, it fails on the first call of the function.
//    MALLOC_ASSERT( result, "Memory allocator not initialized" );
    return result;
}

static inline void  setThreadMallocTLS( void * newvalue ) {
    recursive_malloc_call_protector scoped(/*condition=*/malloc_proxy);
    TlsSetValue_func( TLS_pointer_key, newvalue );
}

/*********** End code to provide thread ID and a TLS pointer **********/

/*
 * This number of bins in the TLS that leads to blocks that we can allocate in.
 */
const uint32_t numBlockBinLimit = 32;

/*
 * The identifier to make sure that memory is allocated by scalable_malloc.
 */
const uint64_t theMallocUniqueID=0xE3C7AF89A1E2D8C1ULL; 

/********* The data structures and global objects        **************/

struct FreeObject {
    FreeObject  *next;
};

/*
 * The following constant is used to define the size of struct Block, the block header.
 * The intent is to have the size of a Block multiple of the cache line size, this allows us to
 * get good alignment at the cost of some overhead equal to the amount of padding included in the Block.
 */

const int blockHeaderAlignment = 64; // a common size of a cache line

struct Block;

/* The 'next' field in the block header has to maintain some invariants:
 *   it needs to be on a 16K boundary and the first field in the block.
 *   Any value stored there needs to have the lower 14 bits set to 0
 *   so that various assert work. This means that if you want to smash this memory
 *   for debugging purposes you will need to obey this invariant.
 * The total size of the header needs to be a power of 2 to simplify
 * the alignment requirements. For now it is a 128 byte structure.
 * To avoid false sharing, the fields changed only locally are separated 
 * from the fields changed by foreign threads.
 * Changing the size of the block header would require to change
 * some bin allocation sizes, in particular "fitting" sizes (see above).
 */

struct LocalBlockFields {
    Block       *next;            /* This field needs to be on a 16K boundary and the first field in the block
                                     for LIFO lists to work. */
    uint64_t     mallocUniqueID;  /* The field to identify memory allocated by scalable_malloc */
    Block       *previous;        /* Use double linked list to speed up removal */
    unsigned int objectSize;
    unsigned int owner;
    FreeObject  *bumpPtr;         /* Bump pointer moves from the end to the beginning of a block */
    FreeObject  *freeList;
    unsigned int allocatedCount;  /* Number of objects allocated (obviously by the owning thread) */
    unsigned int isFull;
};

struct Block : public LocalBlockFields {
    size_t       __pad_local_fields[(blockHeaderAlignment-sizeof(LocalBlockFields))/sizeof(size_t)];
    FreeObject  *publicFreeList;
    Block       *nextPrivatizable;
    size_t       __pad_public_fields[(blockHeaderAlignment-2*sizeof(void*))/sizeof(size_t)];
};

struct Bin {
    Block      *activeBlk;
    Block      *mailbox;
    MallocMutex mailLock;
};

/*
 * The size of the TLS should be enough to hold twice as many as numBlockBinLimit pointers
 * the first sequence of pointers is for lists of blocks to allocate from
 * the second sequence is for lists of blocks that have non-empty publicFreeList
 */
const uint32_t tlsSize = numBlockBinLimit * sizeof(Bin);

/*
 * This is a LIFO linked list that one can init, push or pop from
 */
static LifoQueue freeBlockList;

/*
 * When a block that is not completely free is returned for reuse by other threads
 * this is where the block goes.
 *
 */

static char globalBinSpace[sizeof(LifoQueue)*numBlockBinLimit];
static LifoQueue* globalSizeBins = (LifoQueue*)globalBinSpace;

/********* End of the data structures                    **************/

/********** Various numeric parameters controlling allocations ********/

/*
 * blockSize - the size of a block, it must be larger than maxSegregatedObjectSize.
 *
 */
const uintptr_t blockSize = 16*1024;

/*
 * There are bins for all 8 byte aligned objects less than this segregated size; 8 bins in total
 */
const uint32_t minSmallObjectIndex = 0;
const uint32_t numSmallObjectBins = 8;
const uint32_t maxSmallObjectSize = 64;

/*
 * There are 4 bins between each couple of powers of 2 [64-128-256-...]
 * from maxSmallObjectSize till this size; 16 bins in total
 */
const uint32_t minSegregatedObjectIndex = minSmallObjectIndex+numSmallObjectBins;
const uint32_t numSegregatedObjectBins = 16;
const uint32_t maxSegregatedObjectSize = 1024;

/*
 * And there are 5 bins with the following allocation sizes: 1792, 2688, 3968, 5376, 8064.
 * They selected to fit 9, 6, 4, 3, and 2 sizes per a block, and also are multiples of 128.
 * If sizeof(Block) changes from 128, these sizes require close attention!
 */
const uint32_t minFittingIndex = minSegregatedObjectIndex+numSegregatedObjectBins;
const uint32_t numFittingBins = 5;

const uint32_t fittingAlignment = 128;

#define SET_FITTING_SIZE(N) ( (blockSize-sizeof(Block))/N ) & ~(fittingAlignment-1)
const uint32_t fittingSize1 = SET_FITTING_SIZE(9);
const uint32_t fittingSize2 = SET_FITTING_SIZE(6);
const uint32_t fittingSize3 = SET_FITTING_SIZE(4);
const uint32_t fittingSize4 = SET_FITTING_SIZE(3);
const uint32_t fittingSize5 = SET_FITTING_SIZE(2);
#undef SET_FITTING_SIZE

/*
 * Objects of this size and larger are considered large objects.
 */
const uint32_t minLargeObjectSize = fittingSize5 + 1;

const uint32_t numBlockBins = minFittingIndex+numFittingBins;

/*
 * Get virtual memory in pieces of this size: 0x0100000 is 1 megabyte decimal
 */
static size_t mmapRequestSize = 0x0100000;

/********** End of numeric parameters controlling allocations *********/

#if __INTEL_COMPILER || _MSC_VER
#define NOINLINE(decl) __declspec(noinline) decl
#elif __GNUC__
#define NOINLINE(decl) decl __attribute__ ((noinline))
#else
#define NOINLINE(decl) decl
#endif

static NOINLINE( Block* getPublicFreeListBlock(Bin* bin) );
static NOINLINE( void moveBlockToBinFront(Block *block) );
static NOINLINE( void processLessUsedBlock(Block *block) );

#undef NOINLINE

/*********** Code to acquire memory from the OS or other executive ****************/

#if USE_DEFAULT_MEMORY_MAPPING
#include "MapMemory.h"
#else
/* assume MapMemory and UnmapMemory are customized */
#endif

/*
 * Returns 0 if failure for any reason
 * otherwise returns a pointer to the newly available memory.
 */
static void* getMemory (size_t bytes)
{
    void *result = 0;
    // TODO: either make sure the assertion below is always valid, or remove it.
    /*MALLOC_ASSERT( bytes>=minLargeObjectSize, "request too small" );*/
#if MEMORY_MAPPING_USES_MALLOC
    recursive_malloc_call_protector scoped(/*condition=*/malloc_proxy);
#endif
    result = MapMemory(bytes);
#if MALLOC_TRACE
    if (!result) {
        TRACEF(( "[ScalableMalloc trace] getMemory unsuccess, can't get %d bytes from OS\n", bytes ));
    } else {
        TRACEF(( "[ScalableMalloc trace] - getMemory success returning %p\n", result ));
    }
#endif
    return result;
}

static void returnMemory(void *area, size_t bytes)
{
    int retcode = UnmapMemory(area, bytes);
#if MALLOC_TRACE
    if (retcode) {
        TRACEF(( "[ScalableMalloc trace] returnMemory unsuccess for %p; perhaps it has already been freed or was never allocated.\n", area ));
    }
#endif
    return;
}

/********* End memory acquisition code ********************************/

/********* Now some rough utility code to deal with indexing the size bins. **************/

/*
 * Given a number return the highest non-zero bit in it. It is intended to work with 32-bit values only.
 * Moreover, on IPF, for sake of simplicity and performance, it is narrowed to only serve for 64 to 1023.
 * This is enough for current algorithm of distribution of sizes among bins.
 */
#if _WIN64 && _MSC_VER>=1400 && !__INTEL_COMPILER
extern "C" unsigned char _BitScanReverse( unsigned long* i, unsigned long w );
#pragma intrinsic(_BitScanReverse)
#endif
static inline unsigned int highestBitPos(unsigned int n)
{
    unsigned int pos;
#if __ARCH_x86_32||__ARCH_x86_64

# if __linux__||__APPLE__||__FreeBSD__ || __sun
    __asm__ ("bsr %1,%0" : "=r"(pos) : "r"(n));
# elif (_WIN32 && (!_WIN64 || __INTEL_COMPILER))
    __asm
    {
        bsr eax, n
        mov pos, eax
    }
# elif _WIN64 && _MSC_VER>=1400
    _BitScanReverse((unsigned long*)&pos, (unsigned long)n);
# else
#   error highestBitPos() not implemented for this platform
# endif

#elif __ARCH_ipf || __ARCH_other
    static unsigned int bsr[16] = {0,6,7,7,8,8,8,8,9,9,9,9,9,9,9,9};
    MALLOC_ASSERT( n>=64 && n<1024, ASSERT_TEXT );
    pos = bsr[ n>>6 ];
#else
#   error highestBitPos() not implemented for this platform
#endif /* __ARCH_* */
    return pos;
}

/*
 * Depending on indexRequest, for a given size return either the index into the bin
 * for objects of this size, or the actual size of objects in this bin.
 */
template<bool indexRequest>
static unsigned int getIndexOrObjectSize (unsigned int size)
{
    if (size <= maxSmallObjectSize) { // selection from 4/8/16/24/32/40/48/56/64
         /* Index 0 holds up to 8 bytes, Index 1 16 and so forth */
        return indexRequest ? (size - 1) >> 3 : alignUp(size,8);
    }
    else if (size <= maxSegregatedObjectSize ) { // 80/96/112/128 / 160/192/224/256 / 320/384/448/512 / 640/768/896/1024
        unsigned int order = highestBitPos(size-1); // which group of bin sizes?
        MALLOC_ASSERT( 6<=order && order<=9, ASSERT_TEXT );
        if (indexRequest)
            return minSegregatedObjectIndex - (4*6) - 4 + (4*order) + ((size-1)>>(order-2));
        else {
            unsigned int alignment = 128 >> (9-order); // alignment in the group
            MALLOC_ASSERT( alignment==16 || alignment==32 || alignment==64 || alignment==128, ASSERT_TEXT );
            return alignUp(size,alignment);
        }
    }
    else {
        if( size <= fittingSize3 ) {
            if( size <= fittingSize2 ) {
                if( size <= fittingSize1 )
                    return indexRequest ? minFittingIndex : fittingSize1; 
                else
                    return indexRequest ? minFittingIndex+1 : fittingSize2;
            } else
                return indexRequest ? minFittingIndex+2 : fittingSize3;
        } else {
            if( size <= fittingSize5 ) {
                if( size <= fittingSize4 )
                    return indexRequest ? minFittingIndex+3 : fittingSize4;
                else
                    return indexRequest ? minFittingIndex+4 : fittingSize5;
            } else {
                MALLOC_ASSERT( 0,ASSERT_TEXT ); // this should not happen
                return ~0U;
            }
        }
    }
}

static unsigned int getIndex (unsigned int size)
{
    return getIndexOrObjectSize</*indexRequest*/true>(size);
}

static unsigned int getObjectSize (unsigned int size)
{
    return getIndexOrObjectSize</*indexRequest*/false>(size);
}

/*
 * Initialization code.
 *
 */

/*
 * Big Blocks are the blocks we get from the OS or some similar place using getMemory above.
 * They are placed on the freeBlockList once they are acquired.
 */

static inline void *alignBigBlock(void *unalignedBigBlock)
{
    void *alignedBigBlock;
    /* align the entireHeap so all blocks are aligned. */
    alignedBigBlock = alignUp(unalignedBigBlock, blockSize);
    return alignedBigBlock;
}

/* Divide the big block into smaller bigBlocks that hold this many blocks.
 * This is done since we really need a lot of blocks on the freeBlockList or there will be
 * contention problems.
 */
const unsigned int blocksPerBigBlock = 16;

/* Returns 0 if unsuccessful, otherwise 1. */
static int mallocBigBlock()
{
    void *unalignedBigBlock;
    void *alignedBigBlock;
    void *bigBlockCeiling;
    Block *splitBlock;
    void *splitEdge;
    size_t bigBlockSplitSize;

    unalignedBigBlock = getMemory(mmapRequestSize);

    if (!unalignedBigBlock) {
        TRACEF(( "[ScalableMalloc trace] in mallocBigBlock, getMemory returns 0\n" ));
        /* We can't get any more memory from the OS or executive so return 0 */
        return 0;
    }

    alignedBigBlock = alignBigBlock(unalignedBigBlock);
    bigBlockCeiling = (void*)((uintptr_t)unalignedBigBlock + mmapRequestSize);

    bigBlockSplitSize = blocksPerBigBlock * blockSize;

    splitBlock = (Block*)alignedBigBlock;

    while ( ((uintptr_t)splitBlock + blockSize) <= (uintptr_t)bigBlockCeiling ) {
        splitEdge = (void*)((uintptr_t)splitBlock + bigBlockSplitSize);
        if( splitEdge > bigBlockCeiling) {
            splitEdge = alignDown(bigBlockCeiling, blockSize);
        }
        splitBlock->bumpPtr = (FreeObject*)splitEdge;
        freeBlockList.push((void**) splitBlock);
        splitBlock = (Block*)splitEdge;
    }

    TRACEF(( "[ScalableMalloc trace] in mallocBigBlock returning 1\n" ));
    return 1;
}

/*
 * The malloc routines themselves need to be able to occasionally malloc some space,
 * in order to set up the structures used by the thread local structures. This
 * routine preforms that fuctions.
 */

/*
 * Forward Refs
 */
static void initEmptyBlock(Block *block, size_t size);
static Block *getEmptyBlock(size_t size);

static MallocMutex bootStrapLock;

static Block      *bootStrapBlock = NULL;
static Block      *bootStrapBlockUsed = NULL;
static FreeObject *bootStrapObjectList = NULL; 

static void *bootStrapMalloc(size_t size)
{
    FreeObject *result;

    MALLOC_ASSERT( size == tlsSize, ASSERT_TEXT );

    { // Lock with acquire
        MallocMutex::scoped_lock scoped_cs(bootStrapLock);

        if( bootStrapObjectList) {
            result = bootStrapObjectList;
            bootStrapObjectList = bootStrapObjectList->next;
        } else {
            if (!bootStrapBlock) {
                bootStrapBlock = getEmptyBlock(size);
            }
            result = bootStrapBlock->bumpPtr;
            bootStrapBlock->bumpPtr = (FreeObject *)((uintptr_t)bootStrapBlock->bumpPtr - bootStrapBlock->objectSize);
            if ((uintptr_t)bootStrapBlock->bumpPtr < (uintptr_t)bootStrapBlock+sizeof(Block)) {
                bootStrapBlock->bumpPtr = NULL;
                bootStrapBlock->next = bootStrapBlockUsed;
                bootStrapBlockUsed = bootStrapBlock;
                bootStrapBlock = NULL;
            }
        }
    } // Unlock with release

    memset (result, 0, size);
    return (void*)result;
}

static void bootStrapFree(void* ptr)
{
    MALLOC_ASSERT( ptr, ASSERT_TEXT );
    { // Lock with acquire
        MallocMutex::scoped_lock scoped_cs(bootStrapLock);
        ((FreeObject*)ptr)->next = bootStrapObjectList;
        bootStrapObjectList = (FreeObject*)ptr;
    } // Unlock with release
}

/********* End rough utility code  **************/

/********* Thread and block related code      *************/

#if MALLOC_DEBUG>1
/* The debug version verifies the TLSBin as needed */
static void verifyTLSBin (Bin* bin, size_t size)
{
    Block* temp;
    Bin*   tls;
    uint32_t index = getIndex(size);
    uint32_t objSize = getObjectSize(size);

    tls = (Bin*)getThreadMallocTLS();
    MALLOC_ASSERT( bin == tls+index, ASSERT_TEXT );

    if (tls[index].activeBlk) {
        MALLOC_ASSERT( tls[index].activeBlk->mallocUniqueID==theMallocUniqueID, ASSERT_TEXT );
        MALLOC_ASSERT( tls[index].activeBlk->owner == getThreadId(), ASSERT_TEXT );
        MALLOC_ASSERT( tls[index].activeBlk->objectSize == objSize, ASSERT_TEXT );

        for (temp = tls[index].activeBlk->next; temp; temp=temp->next) {
            MALLOC_ASSERT( temp!=tls[index].activeBlk, ASSERT_TEXT );
            MALLOC_ASSERT( temp->mallocUniqueID==theMallocUniqueID, ASSERT_TEXT );
            MALLOC_ASSERT( temp->owner == getThreadId(), ASSERT_TEXT );
            MALLOC_ASSERT( temp->objectSize == objSize, ASSERT_TEXT );
            MALLOC_ASSERT( temp->previous->next == temp, ASSERT_TEXT );
            if (temp->next) {
                MALLOC_ASSERT( temp->next->previous == temp, ASSERT_TEXT );
            }
        }
        for (temp = tls[index].activeBlk->previous; temp; temp=temp->previous) {
            MALLOC_ASSERT( temp!=tls[index].activeBlk, ASSERT_TEXT );
            MALLOC_ASSERT( temp->mallocUniqueID==theMallocUniqueID, ASSERT_TEXT );
            MALLOC_ASSERT( temp->owner == getThreadId(), ASSERT_TEXT );
            MALLOC_ASSERT( temp->objectSize == objSize, ASSERT_TEXT );
            MALLOC_ASSERT( temp->next->previous == temp, ASSERT_TEXT );
            if (temp->previous) {
                MALLOC_ASSERT( temp->previous->next == temp, ASSERT_TEXT );
            }
        }
    }
}
#else
inline static void verifyTLSBin (Bin*, size_t) {}
#endif /* MALLOC_DEBUG>1 */

/*
 * Add a block to the start of this tls bin list.
 */
static void pushTLSBin (Bin* bin, Block* block)
{
    /* The objectSize should be defined and not a parameter
       because the function is applied to partially filled blocks as well */
    unsigned int size = block->objectSize;
    Block* activeBlk;

    MALLOC_ASSERT( block->owner == getThreadId(), ASSERT_TEXT );
    MALLOC_ASSERT( block->objectSize != 0, ASSERT_TEXT );
    MALLOC_ASSERT( block->next == NULL, ASSERT_TEXT );
    MALLOC_ASSERT( block->previous == NULL, ASSERT_TEXT );

    MALLOC_ASSERT( bin, ASSERT_TEXT );
    verifyTLSBin(bin, size);
    activeBlk = bin->activeBlk;

    block->next = activeBlk;
    if( activeBlk ) {
        block->previous = activeBlk->previous;
        activeBlk->previous = block;
        if( block->previous )
            block->previous->next = block;
    } else {
        bin->activeBlk = block;
    }

    verifyTLSBin(bin, size);
}

/*
 * Take a block out of its tls bin (e.g. before removal).
 */
static void outofTLSBin (Bin* bin, Block* block)
{
    unsigned int size = block->objectSize;

    MALLOC_ASSERT( block->owner == getThreadId(), ASSERT_TEXT );
    MALLOC_ASSERT( block->objectSize != 0, ASSERT_TEXT );

    MALLOC_ASSERT( bin, ASSERT_TEXT );
    verifyTLSBin(bin, size);

    if (block == bin->activeBlk) {
        bin->activeBlk = block->previous? block->previous : block->next;
    }
    /* Delink the block */
    if (block->previous) {
        MALLOC_ASSERT( block->previous->next == block, ASSERT_TEXT );
        block->previous->next = block->next;
    }
    if (block->next) {
        MALLOC_ASSERT( block->next->previous == block, ASSERT_TEXT );
        block->next->previous = block->previous;
    }
    block->next = NULL;
    block->previous = NULL;

    verifyTLSBin(bin, size);
}

/*
 * Return the bin for the given size. If the TLS bin structure is absent, create it.
 */
static Bin* getAllocationBin(size_t size)
{
    Bin* tls = (Bin*)getThreadMallocTLS();
    if( !tls ) {
        MALLOC_ASSERT( tlsSize >= sizeof(Bin) * numBlockBins, ASSERT_TEXT );
        tls = (Bin*) bootStrapMalloc(tlsSize);
        /* the block contains zeroes after bootStrapMalloc, so bins are initialized */
#if MALLOC_DEBUG
        for (int i = 0; i < numBlockBinLimit; i++) {
            MALLOC_ASSERT( tls[i].activeBlk == 0, ASSERT_TEXT );
            MALLOC_ASSERT( tls[i].mailbox == 0, ASSERT_TEXT );
        }
#endif
        setThreadMallocTLS(tls);
    }
    MALLOC_ASSERT( tls, ASSERT_TEXT );
    return tls+getIndex(size);
}

const float emptyEnoughRatio = 1.0 / 4.0; /* "Reactivate" a block if this share of its objects is free. */

static unsigned int emptyEnoughToUse (Block *mallocBlock)
{
    const float threshold = (blockSize - sizeof(Block)) * (1-emptyEnoughRatio);

    if (mallocBlock->bumpPtr) {
        /* If we are still using a bump ptr for this block it is empty enough to use. */
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), examineEmptyEnough);
        mallocBlock->isFull = 0;
        return 1;
    }

    /* allocatedCount shows how many objects in the block are in use; however it still counts
       blocks freed by other threads; so prior call to privatizePublicFreeList() is recommended */
    mallocBlock->isFull = (mallocBlock->allocatedCount*mallocBlock->objectSize > threshold)? 1: 0;
#if COLLECT_STATISTICS
    if (mallocBlock->isFull)
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), examineNotEmpty);
    else
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), examineEmptyEnough);
#endif
    return 1-mallocBlock->isFull;
}

/* Restore the bump pointer for an empty block that is planned to use */
static void restoreBumpPtr (Block *block)
{
    MALLOC_ASSERT( block->allocatedCount == 0, ASSERT_TEXT );
    MALLOC_ASSERT( block->publicFreeList == NULL, ASSERT_TEXT );
    STAT_increment(block->owner, getIndex(block->objectSize), freeRestoreBumpPtr);
    block->bumpPtr = (FreeObject *)((uintptr_t)block + blockSize - block->objectSize);
    block->freeList = NULL;
    block->isFull = 0;
}

#if !(FREELIST_NONBLOCKING)
static MallocMutex publicFreeListLock; // lock for changes of publicFreeList
#endif

const uintptr_t UNUSABLE = 0x1;
inline bool isSolidPtr( void* ptr )
{
    return (UNUSABLE|(uintptr_t)ptr)!=UNUSABLE;
}
inline bool isNotForUse( void* ptr )
{
    return (uintptr_t)ptr==UNUSABLE;
}

static void freePublicObject (Block *block, FreeObject *objectToFree)
{
    Bin* theBin;
    FreeObject *publicFreeList;

#if FREELIST_NONBLOCKING
    FreeObject *temp = block->publicFreeList;
    MALLOC_ITT_SYNC_RELEASING(&block->publicFreeList);
    do {
        publicFreeList = objectToFree->next = temp;
        temp = (FreeObject*)AtomicCompareExchange(
                                (intptr_t&)block->publicFreeList,
                                (intptr_t)objectToFree, (intptr_t)publicFreeList );
        // no backoff necessary because trying to make change, not waiting for a change
    } while( temp != publicFreeList );
#else
    STAT_increment(getThreadId(), ThreadCommonCounters, lockPublicFreeList);
    {
        MallocMutex::scoped_lock scoped_cs(publicFreeListLock);
        publicFreeList = objectToFree->next = block->publicFreeList;
        block->publicFreeList = objectToFree;
    }
#endif

    if( publicFreeList==NULL ) {
        // if the block is abandoned, its nextPrivatizable pointer should be UNUSABLE
        // otherwise, it should point to the bin the block belongs to.
        // reading nextPrivatizable is thread-safe below, because:
        // 1) the executing thread atomically got publicFreeList==NULL and changed it to non-NULL;
        // 2) only owning thread can change it back to NULL,
        // 3) but it can not be done until the block is put to the mailbox
        // So the executing thread is now the only one that can change nextPrivatizable
        if( !isNotForUse(block->nextPrivatizable) ) {
            MALLOC_ASSERT( block->nextPrivatizable!=NULL, ASSERT_TEXT );
            MALLOC_ASSERT( block->owner!=0, ASSERT_TEXT );
            theBin = (Bin*) block->nextPrivatizable;
            MallocMutex::scoped_lock scoped_cs(theBin->mailLock);
            block->nextPrivatizable = theBin->mailbox;
            theBin->mailbox = block;
        } else {
            MALLOC_ASSERT( block->owner==0, ASSERT_TEXT );
        }
    }
    STAT_increment(getThreadId(), ThreadCommonCounters, freeToOtherThread);
    STAT_increment(block->owner, getIndex(block->objectSize), freeByOtherThread);
}

static void privatizePublicFreeList (Block *mallocBlock)
{
    FreeObject *temp, *publicFreeList;

    MALLOC_ASSERT( mallocBlock->owner == getThreadId(), ASSERT_TEXT );
#if FREELIST_NONBLOCKING
    temp = mallocBlock->publicFreeList;
    do {
        publicFreeList = temp;
        temp = (FreeObject*)AtomicCompareExchange(
                                (intptr_t&)mallocBlock->publicFreeList,
                                0, (intptr_t)publicFreeList);
        // no backoff necessary because trying to make change, not waiting for a change
    } while( temp != publicFreeList );
    MALLOC_ITT_SYNC_ACQUIRED(&mallocBlock->publicFreeList);
#else
    STAT_increment(mallocBlock->owner, ThreadCommonCounters, lockPublicFreeList);
    {
        MallocMutex::scoped_lock scoped_cs(publicFreeListLock);
        publicFreeList = mallocBlock->publicFreeList;
        mallocBlock->publicFreeList = NULL;
    }
    temp = publicFreeList;
#endif

    MALLOC_ASSERT( publicFreeList && publicFreeList==temp, ASSERT_TEXT ); // there should be something in publicFreeList!
    if( !isNotForUse(temp) ) { // return/getPartialBlock could set it to UNUSABLE
        MALLOC_ASSERT( mallocBlock->allocatedCount <= (blockSize-sizeof(Block))/mallocBlock->objectSize, ASSERT_TEXT );
        /* other threads did not change the counter freeing our blocks */
        mallocBlock->allocatedCount--;
        while( isSolidPtr(temp->next) ){ // the list will end with either NULL or UNUSABLE
            temp = temp->next;
            mallocBlock->allocatedCount--;
        }
        MALLOC_ASSERT( mallocBlock->allocatedCount < (blockSize-sizeof(Block))/mallocBlock->objectSize, ASSERT_TEXT );
        /* merge with local freeList */
        temp->next = mallocBlock->freeList;
        mallocBlock->freeList = publicFreeList;
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), allocPrivatized);
    }
}

static Block* getPublicFreeListBlock (Bin* bin)
{
    Block* block;
    MALLOC_ASSERT( bin, ASSERT_TEXT );
// the counter should be changed    STAT_increment(getThreadId(), ThreadCommonCounters, lockPublicFreeList);
    {
        MallocMutex::scoped_lock scoped_cs(bin->mailLock);
        block = bin->mailbox;
        if( block ) {
            MALLOC_ASSERT( block->owner == getThreadId(), ASSERT_TEXT );
            MALLOC_ASSERT( !isNotForUse(block->nextPrivatizable), ASSERT_TEXT );
            bin->mailbox = block->nextPrivatizable;
            block->nextPrivatizable = (Block*) bin;
        }
    }
    if( block ) {
        MALLOC_ASSERT( isSolidPtr(block->publicFreeList), ASSERT_TEXT );
        privatizePublicFreeList(block);
    }
    return block;
}

static Block *getPartialBlock(Bin* bin, unsigned int size)
{
    Block *result;
    MALLOC_ASSERT( bin, ASSERT_TEXT );
    unsigned int index = getIndex(size);
    result = (Block *) globalSizeBins[index].pop();
    if (result) {
        MALLOC_ASSERT( result->mallocUniqueID==theMallocUniqueID, ASSERT_TEXT );
        result->next = NULL;
        result->previous = NULL;
        MALLOC_ASSERT( result->publicFreeList!=NULL, ASSERT_TEXT );
        /* There is not a race here since no other thread owns this block */
        MALLOC_ASSERT( result->owner == 0, ASSERT_TEXT );
        result->owner = getThreadId();
        // It is safe to change nextPrivatizable, as publicFreeList is not null
        MALLOC_ASSERT( isNotForUse(result->nextPrivatizable), ASSERT_TEXT );
        result->nextPrivatizable = (Block*)bin;
        // the next call is required to change publicFreeList to 0
        privatizePublicFreeList(result);
        if( result->allocatedCount ) {
            // check its fullness and set result->isFull
            emptyEnoughToUse(result);
        } else {
            restoreBumpPtr(result);
        }
        MALLOC_ASSERT( !isNotForUse(result->publicFreeList), ASSERT_TEXT );
        STAT_increment(result->owner, index, allocBlockPublic);
    }
    return result;
}

static void returnPartialBlock(Bin* bin, Block *block)
{
    unsigned int index = getIndex(block->objectSize);
    MALLOC_ASSERT( bin, ASSERT_TEXT );
    MALLOC_ASSERT( block->owner==getThreadId(), ASSERT_TEXT );
    STAT_increment(block->owner, index, freeBlockPublic);
    // need to set publicFreeList to non-zero, so other threads
    // will not change nextPrivatizable and it can be zeroed.
    if ((intptr_t)block->nextPrivatizable==(intptr_t)bin) {
        void* oldval;
#if FREELIST_NONBLOCKING
        oldval = (void*)AtomicCompareExchange((intptr_t&)block->publicFreeList, (intptr_t)UNUSABLE, 0);
#else
        STAT_increment(block->owner, ThreadCommonCounters, lockPublicFreeList);
        {
            MallocMutex::scoped_lock scoped_cs(publicFreeListLock);
            if ( (oldval=block->publicFreeList)==NULL )
                (uintptr_t&)(block->publicFreeList) = UNUSABLE;
        }
#endif
        if ( oldval!=NULL ) {
            // another thread freed an object; we need to wait until it finishes.
            // I believe there is no need for exponential backoff, as the wait here is not for a lock;
            // but need to yield, so the thread we wait has a chance to run.
            int count = 256;
            while( (intptr_t)const_cast<Block* volatile &>(block->nextPrivatizable)==(intptr_t)bin ) {
                if (--count==0) {
                    do_yield();
                    count = 256;
                }
            }
        }
    } else {
        MALLOC_ASSERT( isSolidPtr(block->publicFreeList), ASSERT_TEXT );
    }
    MALLOC_ASSERT( block->publicFreeList!=NULL, ASSERT_TEXT );
    // now it is safe to change our data
    block->previous = NULL;
    block->owner = 0;
    // it is caller responsibility to ensure that the list of blocks
    // formed by nextPrivatizable pointers is kept consistent if required.
    // if only called from thread shutdown code, it does not matter.
    (uintptr_t&)(block->nextPrivatizable) = UNUSABLE;
    globalSizeBins[index].push((void **)block);
}

static void initEmptyBlock(Block *block, size_t size)
{
    // Having getIndex and getObjectSize called next to each other
    // allows better compiler optimization as they basically share the code.
    unsigned int index      = getIndex(size);
    unsigned int objectSize = getObjectSize(size); 
    Bin* tls = (Bin*)getThreadMallocTLS();
#if MALLOC_DEBUG
    memset (block, 0x0e5, blockSize);
#endif
    block->mallocUniqueID = theMallocUniqueID;
    block->next = NULL;
    block->previous = NULL;
    block->objectSize = objectSize;
    block->owner = getThreadId();
    // bump pointer should be prepared for first allocation - thus mode it down to objectSize
    block->bumpPtr = (FreeObject *)((uintptr_t)block + blockSize - objectSize);
    block->freeList = NULL;
    block->allocatedCount = 0;
    block->isFull = 0;

    block->publicFreeList = NULL;
    // each block should have the address where the head of the list of "privatizable" blocks is kept
    // the only exception is a block for boot strap which is initialized when TLS is yet NULL
    block->nextPrivatizable = tls? (Block*)(tls + index) : NULL;
    TRACEF(( "[ScalableMalloc trace] Empty block %p is initialized, owner is %d, objectSize is %d, bumpPtr is %p\n",
             block, block->owner, block->objectSize, block->bumpPtr ));
  }

/* Return an empty uninitialized block in a non-blocking fashion. */
static Block *getEmptyBlock(size_t size)
{
    Block *result;
    Block *bigBlock;

    result = NULL;

    bigBlock = (Block *) freeBlockList.pop();

    while (!bigBlock) {
        /* We are out of blocks so go to the OS and get another one */
        if (!mallocBigBlock()) {
            return NULL;
        }
        bigBlock = (Block *) freeBlockList.pop();
    }

    // check alignment
    MALLOC_ASSERT( isAligned( bigBlock, blockSize ), ASSERT_TEXT );
    MALLOC_ASSERT( isAligned( bigBlock->bumpPtr, blockSize ), ASSERT_TEXT );
    // block should be at least as big as blockSize; otherwise the previous block can be damaged.
    MALLOC_ASSERT( (uintptr_t)bigBlock->bumpPtr >= (uintptr_t)bigBlock + blockSize, ASSERT_TEXT );
    bigBlock->bumpPtr = (FreeObject *)((uintptr_t)bigBlock->bumpPtr - blockSize);
    result = (Block *)bigBlock->bumpPtr;
    if ( result!=bigBlock ) {
        TRACEF(( "[ScalableMalloc trace] Pushing partial rest of block back on.\n" ));
        freeBlockList.push((void **)bigBlock);
    }
    initEmptyBlock(result, size);
    STAT_increment(result->owner, getIndex(result->objectSize), allocBlockNew);

    return result;
}

/* We have a block give it back to the malloc block manager */
static void returnEmptyBlock (Block *block, bool keepTheBin = true)
{
    // it is caller's responsibility to ensure no data is lost before calling this
    MALLOC_ASSERT( block->allocatedCount==0, ASSERT_TEXT );
    MALLOC_ASSERT( block->publicFreeList==NULL, ASSERT_TEXT );
    if (keepTheBin) {
        /* We should keep the TLS bin structure */
        MALLOC_ASSERT( block->next == NULL, ASSERT_TEXT );
        MALLOC_ASSERT( block->previous == NULL, ASSERT_TEXT );
    }
    STAT_increment(block->owner, getIndex(block->objectSize), freeBlockBack);

    block->publicFreeList = NULL;
    block->nextPrivatizable = NULL;

    block->mallocUniqueID=0;
    block->next = NULL;
    block->previous = NULL;
    block->objectSize = 0;
    block->owner = (unsigned)-1;
    // for an empty block, bump pointer should point right after the end of the block
    block->bumpPtr = (FreeObject *)((uintptr_t)block + blockSize);
    block->freeList = NULL;
    block->allocatedCount = 0;
    block->isFull = 0;
    freeBlockList.push((void **)block);
}

inline static Block* getActiveBlock( Bin* bin )
{
    MALLOC_ASSERT( bin, ASSERT_TEXT );
    return bin->activeBlk;
}

inline static void setActiveBlock (Bin* bin, Block *block)
{
    MALLOC_ASSERT( bin, ASSERT_TEXT );
    MALLOC_ASSERT( block->owner == getThreadId(), ASSERT_TEXT );
    // it is the caller responsibility to keep bin consistence (i.e. ensure this block is in the bin list)
    bin->activeBlk = block;
}

inline static Block* setPreviousBlockActive( Bin* bin )
{
    MALLOC_ASSERT( bin && bin->activeBlk, ASSERT_TEXT );
    Block* temp = bin->activeBlk->previous;
    if( temp ) {
        MALLOC_ASSERT( temp->isFull == 0, ASSERT_TEXT );
        bin->activeBlk = temp;
    }
    return temp;
}

/********* End thread related code  *************/

/********* Library initialization *************/

//! Value indicating the state of initialization.
/* 0 = initialization not started.
 * 1 = initialization started but not finished.
 * 2 = initialization finished.
 * In theory, we only need values 0 and 2. But value 1 is nonetheless
 * useful for detecting errors in the double-check pattern.
 */
static int mallocInitialized;   // implicitly initialized to 0
static MallocMutex initAndShutMutex;

extern "C" void mallocThreadShutdownNotification(void*);

/*
 * Allocator initialization routine;
 * it is called lazily on the very first scalable_malloc call.
 */
static void initMemoryManager()
{
    TRACEF(( "[ScalableMalloc trace] sizeof(Block) is %d (expected 128); sizeof(uintptr_t) is %d\n",
             sizeof(Block), sizeof(uintptr_t) ));
    MALLOC_ASSERT( 2*blockHeaderAlignment == sizeof(Block), ASSERT_TEXT );

// Create keys for thread-local storage and for thread id
// TODO: add error handling
#if USE_WINTHREAD
    TLS_pointer_key = TlsAlloc();
    Tid_key = TlsAlloc();
#else
    int status1 = pthread_key_create( &TLS_pointer_key, mallocThreadShutdownNotification );
    int status2 = pthread_key_create( &Tid_key, NULL );
    if ( status1 || status2 ) {
        fprintf (stderr, "The memory manager cannot create tls key during initialisation; exiting \n");
        exit(1);
    }
#endif /* USE_WINTHREAD */
    TRACEF(( "[ScalableMalloc trace] Asking for a mallocBigBlock\n" ));
    if (!mallocBigBlock()) {
        fprintf (stderr, "The memory manager cannot access sufficient memory to initialize; exiting \n");
        exit(1);
    }
}

//! Ensures that initMemoryManager() is called once and only once.
/** Does not return until initMemoryManager() has been completed by a thread.
    There is no need to call this routine if mallocInitialized==2 . */
static inline void checkInitialization()
{
    if (mallocInitialized==2) return;
    MallocMutex::scoped_lock lock( initAndShutMutex );
    if (mallocInitialized!=2) {
        MALLOC_ASSERT( mallocInitialized==0, ASSERT_TEXT );
        mallocInitialized = 1;
        initMemoryManager();
#ifdef  MALLOC_EXTRA_INITIALIZATION
        recursive_malloc_call_protector scoped(/*condition=*/malloc_proxy);
        MALLOC_EXTRA_INITIALIZATION;
#endif
        MALLOC_ASSERT( mallocInitialized==1, ASSERT_TEXT );
        mallocInitialized = 2;
    }
    MALLOC_ASSERT( mallocInitialized==2, ASSERT_TEXT ); /* It can't be 0 or I would have initialized it */
}

/********* End library initialization *************/

/********* The malloc show          *************/

/*
 * The program wants a large object that we are not prepared to deal with.
 * so we pass the problem on to the OS. Large Objects are the only objects in
 * the system that begin on a 16K byte boundary since the blocks used for smaller
 * objects have the Block structure at each 16K boundary.
 *
 */

struct LargeObjectHeader {
    void        *unalignedResult;   /* The base of the memory returned from getMemory, this is what is used to return this to the OS */
    size_t       unalignedSize;     /* The size that was requested from getMemory */
    uint64_t     mallocUniqueID;    /* The field to check whether the memory was allocated by scalable_malloc */
    size_t       objectSize;        /* The size originally requested by a client */
};

/* A predicate checks whether an object starts on blockSize boundary */
static inline unsigned int isLargeObject(void *object)
{
    return isAligned(object, blockSize);
}

static inline void *mallocLargeObject (size_t size, size_t alignment)
{
    void *alignedArea;
    void *unalignedArea;
    LargeObjectHeader *header;

    // TODO: can the requestedSize be optimized somehow?
    size_t requestedSize = size + sizeof(LargeObjectHeader) + alignment;
#if USE_MALLOC_FOR_LARGE_OBJECT
#if MALLOC_LD_PRELOAD
    if (malloc_proxy) {
        if ( original_malloc_found ){
            unalignedArea = (*original_malloc_ptr)(requestedSize);
        }else{
            MALLOC_ASSERT( 0, ASSERT_TEXT );
            return NULL;
        }
    } else
#endif /* MALLOC_LD_PRELOAD */
        unalignedArea = malloc(requestedSize);
#else
    unalignedArea = getMemory(requestedSize);
#endif /* USE_MALLOC_FOR_LARGE_OBJECT */
    if (!unalignedArea) {
        /* We can't get any more memory from the OS or executive so return 0 */
        return 0;
    }
    alignedArea = (void*) alignUp((uintptr_t)unalignedArea+sizeof(LargeObjectHeader),
                                   alignment);
    header = (LargeObjectHeader*) ((uintptr_t)alignedArea-sizeof(LargeObjectHeader));
    header->unalignedResult = unalignedArea;
    header->mallocUniqueID=theMallocUniqueID;
    header->unalignedSize = requestedSize;
    header->objectSize = size;
    MALLOC_ASSERT( isLargeObject(alignedArea), ASSERT_TEXT );
    STAT_increment(getThreadId(), ThreadCommonCounters, allocLargeSize);
    return alignedArea;
}

static inline void freeLargeObject(void *object)
{
    LargeObjectHeader *header;
    STAT_increment(getThreadId(), ThreadCommonCounters, freeLargeSize);
    header = (LargeObjectHeader *)((uintptr_t)object - sizeof(LargeObjectHeader));
    header->mallocUniqueID = 0;
#if USE_MALLOC_FOR_LARGE_OBJECT
#if MALLOC_LD_PRELOAD
    if (malloc_proxy) {
        if ( original_malloc_found ){
            original_free_ptr(header->unalignedResult);
        }else{
            MALLOC_ASSERT( 0, ASSERT_TEXT );
        }
    } else
#endif /* MALLOC_LD_PRELOAD */
        free(header->unalignedResult);
#else
    returnMemory(header->unalignedResult, header->unalignedSize);
#endif /* USE_MALLOC_FOR_LARGE_OBJECT */
}

static FreeObject *allocateFromFreeList(Block *mallocBlock)
{
    FreeObject *result;

    if (!mallocBlock->freeList) {
        return NULL;
    }

    result = mallocBlock->freeList;
    MALLOC_ASSERT( result, ASSERT_TEXT );

    mallocBlock->freeList = result->next;
    MALLOC_ASSERT( mallocBlock->allocatedCount < (blockSize-sizeof(Block))/mallocBlock->objectSize, ASSERT_TEXT );
    mallocBlock->allocatedCount++;
    STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), allocFreeListUsed);

    return result;
}

static FreeObject *allocateFromBumpPtr(Block *mallocBlock)
{
    FreeObject *result = mallocBlock->bumpPtr;
    if (result) {
        mallocBlock->bumpPtr =
            (FreeObject *) ((uintptr_t) mallocBlock->bumpPtr - mallocBlock->objectSize);
        if ( (uintptr_t)mallocBlock->bumpPtr < (uintptr_t)mallocBlock+sizeof(Block) ) {
            mallocBlock->bumpPtr = NULL;
        }
        MALLOC_ASSERT( mallocBlock->allocatedCount < (blockSize-sizeof(Block))/mallocBlock->objectSize, ASSERT_TEXT );
        mallocBlock->allocatedCount++;
        STAT_increment(mallocBlock->owner, getIndex(mallocBlock->objectSize), allocBumpPtrUsed);
    }
    return result;
}

inline static FreeObject* allocateFromBlock( Block *mallocBlock )
{
    FreeObject *result;

    MALLOC_ASSERT( mallocBlock->owner == getThreadId(), ASSERT_TEXT );

    /* for better cache locality, first looking in the free list. */
    if ( (result = allocateFromFreeList(mallocBlock)) ) {
        return result;
    }
    MALLOC_ASSERT( !mallocBlock->freeList, ASSERT_TEXT );

    /* if free list is empty, try thread local bump pointer allocation. */
    if ( (result = allocateFromBumpPtr(mallocBlock)) ) {
        return result;
    }
    MALLOC_ASSERT( !mallocBlock->bumpPtr, ASSERT_TEXT );

    /* the block is considered full. */
    mallocBlock->isFull = 1;
    return NULL;
}

static void moveBlockToBinFront(Block *block)
{
    Bin* bin = getAllocationBin(block->objectSize);
    /* move the block to the front of the bin */
    outofTLSBin(bin, block);
    pushTLSBin(bin, block);
}

static void processLessUsedBlock(Block *block)
{
    Bin* bin = getAllocationBin(block->objectSize);
    if (block != getActiveBlock(bin) && block != getActiveBlock(bin)->previous ) {
        /* We are not actively using this block; return it to the general block pool */
        outofTLSBin(bin, block);
        returnEmptyBlock(block);
    } else {
        /* all objects are free - let's restore the bump pointer */
        restoreBumpPtr(block);
    }
}

/*
 * All aligned allocations fall into one of the following categories:
 *  1. if both request size and alignment are <= maxSegregatedObjectSize,
 *       we just align the size up, and request this amount, because for every size
 *       aligned to some power of 2, the allocated object is at least that aligned.
 * 2. for bigger size, check if already guaranteed fittingAlignment is enough.
 * 3. if size+alignment<minLargeObjectSize, we take an object of fittingSizeN and align
 *       its address up; given such pointer, scalable_free could find the real object.
 * 4. otherwise, aligned large object is allocated.
 */
static void *allocateAligned(size_t size, size_t alignment)
{
    MALLOC_ASSERT( isPowerOfTwo(alignment), ASSERT_TEXT );

    void *result;
    if (size<=maxSegregatedObjectSize && alignment<=maxSegregatedObjectSize)
        result = scalable_malloc(alignUp(size? size: sizeof(size_t), alignment));
    else if (alignment<=fittingAlignment)
        result = scalable_malloc(size);
    else if (size+alignment < minLargeObjectSize) {
        void *unaligned = scalable_malloc(size+alignment);
        if (!unaligned) return NULL;
        result = alignUp(unaligned, alignment);
    } else {
        /* This can be the first allocation call. */
        checkInitialization();
        /* To correctly detect kind of allocation in scalable_free we need 
           to distinguish memory allocated as large object.
           This is done via alignment that is greater than can be found in Block.
        */ 
        result = mallocLargeObject(size, blockSize>alignment? blockSize: alignment);
    }

    MALLOC_ASSERT( isAligned(result, alignment), ASSERT_TEXT );
    return result;
}

static void *reallocAligned(void *ptr, size_t size, size_t alignment = 0)
{
    void *result;
    size_t copySize;

    if (isLargeObject(ptr)) {
        LargeObjectHeader* loh = (LargeObjectHeader *)((uintptr_t)ptr - sizeof(LargeObjectHeader));
        MALLOC_ASSERT( loh->mallocUniqueID==theMallocUniqueID, ASSERT_TEXT );
        copySize = loh->unalignedSize-((uintptr_t)ptr-(uintptr_t)loh->unalignedResult);
        if (size <= copySize && (0==alignment || isAligned(ptr, alignment))) {
            loh->objectSize = size;
            return ptr;
        } else {
           copySize = loh->objectSize;
           result = alignment ? allocateAligned(size, alignment) : scalable_malloc(size);
        }
    } else {
        Block* block = (Block *)alignDown(ptr, blockSize);
        MALLOC_ASSERT( block->mallocUniqueID==theMallocUniqueID, ASSERT_TEXT );
        copySize = block->objectSize;
        if (size <= copySize && (0==alignment || isAligned(ptr, alignment))) {
            return ptr;
        } else {
            result = alignment ? allocateAligned(size, alignment) : scalable_malloc(size);
        }
    }
    if (result) {
        memcpy(result, ptr, copySize<size? copySize: size);
        scalable_free(ptr);
    }
    return result;
}

/* A predicate checks if an object is properly placed inside its block */
static inline bool isProperlyPlaced(const void *object, const Block *block)
{
    return 0 == ((uintptr_t)block + blockSize - (uintptr_t)object) % block->objectSize;
}

/* Finds the real object inside the block */
static inline FreeObject *findAllocatedObject(const void *address, const Block *block)
{
    // calculate offset from the end of the block space
    uintptr_t offset = (uintptr_t)block + blockSize - (uintptr_t)address;
    MALLOC_ASSERT( offset<blockSize-sizeof(Block), ASSERT_TEXT );
    // find offset difference from a multiple of allocation size
    offset %= block->objectSize;
    // and move the address down to where the real object starts.
    return (FreeObject*)((uintptr_t)address - (offset? block->objectSize-offset: 0));
}

} // namespace Internal
} // namespace ThreadingSubstrate

using namespace ThreadingSubstrate;
using namespace ThreadingSubstrate::Internal;

/*
 * When a thread is shutting down this routine should be called to remove all the thread ids
 * from the malloc blocks and replace them with a NULL thread id.
 *
 */
#if MALLOC_TRACE
static unsigned int threadGoingDownCount = 0;
#endif

/*
 * for pthreads, the function is set as a callback in pthread_key_create for TLS bin.
 * it will be automatically called at thread exit with the key value as the argument.
 *
 * for Windows, it should be called directly e.g. from DllMain; the argument can be NULL
 * one should include "TypeDefinitions.h" for the declaration of this function.
*/
extern "C" void mallocThreadShutdownNotification(void* arg)
{
    Bin   *tls;
    Block *threadBlock;
    Block *threadlessBlock;
    unsigned int index;

    {
        MallocMutex::scoped_lock lock( initAndShutMutex );
        if ( mallocInitialized == 0 ) return;
    }

    TRACEF(( "[ScalableMalloc trace] Thread id %d blocks return start %d\n",
             getThreadId(),  threadGoingDownCount++ ));
#ifdef USE_WINTHREAD
    tls = (Bin*)getThreadMallocTLS();
#else
    tls = (Bin*)arg;
#endif
    if (tls) {
        for (index = 0; index < numBlockBins; index++) {
            if (tls[index].activeBlk==NULL)
                continue;
            threadlessBlock = tls[index].activeBlk->previous;
            while (threadlessBlock) {
                threadBlock = threadlessBlock->previous;
                if (threadlessBlock->allocatedCount==0 && threadlessBlock->publicFreeList==NULL) {
                    /* we destroy the thread, no need to keep its TLS bin -> the second param is false */
                    returnEmptyBlock(threadlessBlock, false);
                } else {
                    returnPartialBlock(tls+index, threadlessBlock);
                }
                threadlessBlock = threadBlock;
            }
            threadlessBlock = tls[index].activeBlk;
            while (threadlessBlock) {
                threadBlock = threadlessBlock->next;
                if (threadlessBlock->allocatedCount==0 && threadlessBlock->publicFreeList==NULL) {
                    /* we destroy the thread, no need to keep its TLS bin -> the second param is false */
                    returnEmptyBlock(threadlessBlock, false);
                } else {
                    returnPartialBlock(tls+index, threadlessBlock);
                }
                threadlessBlock = threadBlock;
            }
            tls[index].activeBlk = 0;
        }
        bootStrapFree((void*)tls);
        setThreadMallocTLS(NULL);
    }

    TRACEF(( "[ScalableMalloc trace] Thread id %d blocks return end\n", getThreadId() ));
}

extern "C" void mallocProcessShutdownNotification(void)
{
    // for now, this function is only necessary for dumping statistics
#if COLLECT_STATISTICS
    ThreadId nThreads = ThreadIdCount;
    for( int i=1; i<=nThreads && i<MAX_THREADS; ++i )
        STAT_print(i);
#endif
}

/********* The malloc code          *************/

extern "C" void * scalable_malloc(size_t size)
{
    Bin* bin;
    Block * mallocBlock;
    FreeObject *result;

    if (!size) size = sizeof(size_t);

    /* This returns only after malloc is initialized */
    checkInitialization();

    /*
     * Use Large Object Allocation
     */
    if (size >= minLargeObjectSize) {
        result = (FreeObject*)mallocLargeObject(size, blockSize);
        if (!result) errno = ENOMEM;
        return result;
    }

    /*
     * Get an element in thread-local array corresponding to the given size;
     * It keeps ptr to the active block for allocations of this size
     */
    bin = getAllocationBin(size);

    /* Get the block of you want to try to allocate in. */
    mallocBlock = getActiveBlock(bin);

    if (mallocBlock) {
        do {
            if( (result = allocateFromBlock(mallocBlock)) ) {
                return result;
            }
            // the previous block, if any, should be empty enough
        } while( (mallocBlock = setPreviousBlockActive(bin)) );
    }
    MALLOC_ASSERT( !(bin->activeBlk) || bin->activeBlk->isFull==1, ASSERT_TEXT );

    /*
     * else privatize publicly freed objects in some block and allocate from it
     */
    mallocBlock = getPublicFreeListBlock( bin );
    if (mallocBlock) {
        if (emptyEnoughToUse(mallocBlock)) {
            /* move the block to the front of the bin */
            outofTLSBin(bin, mallocBlock);
            pushTLSBin(bin, mallocBlock);
        }
        MALLOC_ASSERT( mallocBlock->freeList, ASSERT_TEXT );
        if ( (result = allocateFromFreeList(mallocBlock)) ) {
            return result;
        }
        /* Else something strange happened, need to retry from the beginning; */
        TRACEF(( "[ScalableMalloc trace] Something is wrong: no objects in public free list; reentering.\n" ));
        return scalable_malloc(size);
    }

    /*
     * no suitable own blocks, try to get a partial block that some other thread has discarded.
     */
    mallocBlock = getPartialBlock(bin, size);
    while (mallocBlock) {
        pushTLSBin(bin, mallocBlock);
// guaranteed by pushTLSBin: MALLOC_ASSERT( *bin==mallocBlock || (*bin)->previous==mallocBlock, ASSERT_TEXT );
        setActiveBlock(bin, mallocBlock);
        if( (result = allocateFromBlock(mallocBlock)) ) {
            return result;
        }
        mallocBlock = getPartialBlock(bin, size);
    }

    /*
     * else try to get a new empty block
     */
    mallocBlock = getEmptyBlock(size);
    if (mallocBlock) {
        pushTLSBin(bin, mallocBlock);
// guaranteed by pushTLSBin: MALLOC_ASSERT( *bin==mallocBlock || (*bin)->previous==mallocBlock, ASSERT_TEXT );
        setActiveBlock(bin, mallocBlock);
        if( (result = allocateFromBlock(mallocBlock)) ) {
            return result;
        }
        /* Else something strange happened, need to retry from the beginning; */
        TRACEF(( "[ScalableMalloc trace] Something is wrong: no objects in empty block; reentering.\n" ));
        return scalable_malloc(size);
    }
    /*
     * else nothing works so return NULL
     */
    TRACEF(( "[ScalableMalloc trace] No memory found, returning NULL.\n" ));
    errno = ENOMEM;
    return NULL;
}

/********* End the malloc code      *************/

/********* The free code            *************/

extern "C" void scalable_free (void *object) {
    Block *block;
    ThreadId myTid;
    FreeObject *objectToFree;

    if (!object) {
        return;
    }

    if (isLargeObject(object)) {
        freeLargeObject(object);
        return;
    }

    myTid = getThreadId();

    block = (Block *)alignDown(object, blockSize);/* mask low bits to get the block */
    MALLOC_ASSERT( block->mallocUniqueID == theMallocUniqueID, ASSERT_TEXT );
    MALLOC_ASSERT( block->allocatedCount, ASSERT_TEXT );

    // Due to aligned allocations, a pointer passed to scalable_free
    // might differ from the address of internally allocated object.
    // Small objects however should always be fine.    
    if (block->objectSize <= maxSegregatedObjectSize)
        objectToFree = (FreeObject*)object;
    // "Fitting size" allocations are suspicious if aligned higher than naturally
    else {
        if ( ! isAligned(object,2*fittingAlignment) )
        // TODO: the above check is questionable - it gives false negatives in ~50% cases,
        //       so might even be slower in average than unconditional use of findAllocatedObject.
        // here it should be a "real" object
            objectToFree = (FreeObject*)object;
        else
        // here object can be an aligned address, so applying additional checks
            objectToFree = findAllocatedObject(object, block);
        MALLOC_ASSERT( isAligned(objectToFree,fittingAlignment), ASSERT_TEXT );
    }
    MALLOC_ASSERT( isProperlyPlaced(objectToFree, block), ASSERT_TEXT );

    if (myTid == block->owner) {
        objectToFree->next = block->freeList;
        block->freeList = objectToFree;
        block->allocatedCount--;
        MALLOC_ASSERT( block->allocatedCount < (blockSize-sizeof(Block))/block->objectSize, ASSERT_TEXT );
#if COLLECT_STATISTICS
        if (getActiveBlock(getAllocationBin(block->objectSize)) != block)
            STAT_increment(myTid, getIndex(block->objectSize), freeToInactiveBlock);
        else
            STAT_increment(myTid, getIndex(block->objectSize), freeToActiveBlock);
#endif
        if (block->isFull) {
            if (emptyEnoughToUse(block))
                moveBlockToBinFront(block);
        } else {
            if (block->allocatedCount==0 && block->publicFreeList==NULL)
                processLessUsedBlock(block);
        }
    } else { /* Slower path to add to the shared list, the allocatedCount is updated by the owner thread in malloc. */
        freePublicObject (block, objectToFree);
    }
}

/*
 * A variant that provides additional memory safety, by checking whether the given address
 * was obtained with this allocator, and if not redirecting to the provided alternative call.
 */
extern "C" void safer_scalable_free (void *object, void (*original_free)(void*)) 
{
    if (!object)
        return;

    // Check if the memory was allocated by scalable_malloc
    uint64_t id = isLargeObject(object)?
                    ((LargeObjectHeader*)((uintptr_t)object-sizeof(LargeObjectHeader)))->mallocUniqueID:
                    ((Block *)alignDown(object, blockSize))->mallocUniqueID;
    if (id==theMallocUniqueID)
        scalable_free(object);
    else if (original_free)
        original_free(object);
}

/********* End the free code        *************/

/********* Code for scalable_realloc       ***********/

/*
 * From K&R
 * "realloc changes the size of the object pointed to by p to size. The contents will
 * be unchanged up to the minimum of the old and the new sizes. If the new size is larger,
 * the new space is uninitialized. realloc returns a pointer to the new space, or
 * NULL if the request cannot be satisfied, in which case *p is unchanged."
 *
 */
extern "C" void* scalable_realloc(void* ptr, size_t size)
{
    /* corner cases left out of reallocAligned to not deal with errno there */
    if (!ptr) {
        return scalable_malloc(size);
    }
    if (!size) {
        scalable_free(ptr);
        return NULL;
    }
    void* tmp = reallocAligned(ptr, size, 0);
    if (!tmp) errno = ENOMEM;
    return tmp;
}

/*
 * A variant that provides additional memory safety, by checking whether the given address
 * was obtained with this allocator, and if not redirecting to the provided alternative call.
 */
extern "C" void* safer_scalable_realloc (void* ptr, size_t sz, void* (*original_realloc)(void*,size_t)) 
{
    if (!ptr) {
        return scalable_malloc(sz);
    }
    // Check if the memory was allocated by scalable_malloc
    uint64_t id = isLargeObject(ptr)?
                    ((LargeObjectHeader*)((uintptr_t)ptr-sizeof(LargeObjectHeader)))->mallocUniqueID:
                    ((Block *)alignDown(ptr, blockSize))->mallocUniqueID;
    if (id==theMallocUniqueID) {
        if (!sz) {
            scalable_free(ptr);
            return NULL;
        }
        void* tmp = reallocAligned(ptr, sz, 0);
        if (!tmp) errno = ENOMEM;
        return tmp;
    }
    else if (original_realloc)
        return original_realloc(ptr,sz);
    else
        return NULL;
}

/********* End code for scalable_realloc   ***********/

/********* Code for scalable_calloc   ***********/

/*
 * From K&R
 * calloc returns a pointer to space for an array of nobj objects, 
 * each of size size, or NULL if the request cannot be satisfied. 
 * The space is initialized to zero bytes.
 *
 */

extern "C" void * scalable_calloc(size_t nobj, size_t size)
{
    size_t arraySize = nobj * size;
    void* result = scalable_malloc(arraySize);
    if (result)
        memset(result, 0, arraySize);
    return result;
}

/********* End code for scalable_calloc   ***********/

/********* Code for aligned allocation API **********/

extern "C" int scalable_posix_memalign(void **memptr, size_t alignment, size_t size)
{
    if ( !isPowerOfTwoMultiple(alignment, sizeof(void*)) )
        return EINVAL;
    void *result = allocateAligned(size, alignment);
    if (!result)
        return ENOMEM;
    *memptr = result;
    return 0;
}

extern "C" void * scalable_aligned_malloc(size_t size, size_t alignment)
{
    if (!isPowerOfTwo(alignment) || 0==size) {
        errno = EINVAL;
        return NULL;
    }
    void* tmp = allocateAligned(size, alignment);
    if (!tmp) errno = ENOMEM;
    return tmp;
}

extern "C" void * scalable_aligned_realloc(void *ptr, size_t size, size_t alignment)
{
    /* corner cases left out of reallocAligned to not deal with errno there */
    if (!isPowerOfTwo(alignment)) {
        errno = EINVAL;
        return NULL;
    }
    if (!ptr) {
        return allocateAligned(size, alignment);
    }
    // Check if the memory was allocated by scalable_malloc
    uint64_t id = isLargeObject(ptr)?
                    ((LargeObjectHeader*)((uintptr_t)ptr-sizeof(LargeObjectHeader)))->mallocUniqueID:
                    ((Block *)alignDown(ptr, blockSize))->mallocUniqueID;
    if (id!=theMallocUniqueID)
        return NULL;

    if (!size) {
        scalable_free(ptr);
        return NULL;
    }
    void* tmp = reallocAligned(ptr, size, alignment);
    if (!tmp) errno = ENOMEM;
    return tmp;
}

extern "C" void scalable_aligned_free(void *ptr)
{
    scalable_free(ptr);
}

/********* end code for aligned allocation API **********/
