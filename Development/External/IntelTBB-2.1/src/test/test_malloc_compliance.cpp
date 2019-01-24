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

const int MByte = 1048576; //1MB

/* _WIN32_WINNT should be defined at the very beginning, 
   because other headers might include <windows.h>
*/
#if _WIN32 || _WIN64
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <stdio.h>

void limitMem( int limit )
{
    static HANDLE hJob = NULL;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo;

    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    jobInfo.ProcessMemoryLimit = limit? limit*MByte : 2*1024LL*MByte;
    if (NULL == hJob) {
        if (NULL == (hJob = CreateJobObject(NULL, NULL))) {
            printf("Can't assign create job object: %d\n", GetLastError());
            exit(1);
        }
        if (0 == AssignProcessToJobObject(hJob, GetCurrentProcess())) {
            printf("Can't assign process to job object: %d\n", GetLastError());
            exit(1);
        }
    }
    if (0 == SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, 
                                     &jobInfo, sizeof(jobInfo))) {
        printf("Can't set limits: %d\n", GetLastError());
        exit(1);
    }
}
#else
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void limitMem( int limit )
{
    rlimit rlim;
    rlim.rlim_cur = limit? limit*MByte : (rlim_t)RLIM_INFINITY;
    rlim.rlim_max = (rlim_t)RLIM_INFINITY;
    int ret = setrlimit(RLIMIT_AS,&rlim);
    if (0 != ret) {
        printf("Can't set limits: errno %d\n", errno);
        exit(1);
    }
}
#endif 

#include <time.h>
#include <errno.h>
#include <vector>
#define __TBB_NO_IMPLICIT_LINKAGE 1
#include "tbb/scalable_allocator.h"

#include "harness.h"
#if __linux__
#include <stdint.h> // uintptr_t
#endif
#if _WIN32 || _WIN64
#include <malloc.h> // _aligned_(malloc|free|realloc)
#endif

const size_t COUNT_ELEM_CALLOC = 2;
const int COUNT_TESTS = 1000;
const int COUNT_ELEM = 50000;
const size_t MAX_SIZE = 1000;
const int COUNTEXPERIMENT = 10000;

const char strError[]="failed";
const char strOk[]="done";

typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef unsigned long DWORD;
typedef unsigned char BYTE;


typedef void* TestMalloc(size_t size);
typedef void* TestCalloc(size_t num, size_t size);
typedef void* TestRealloc(void* memblock, size_t size);
typedef void  TestFree(void* memblock);
typedef int   TestPosixMemalign(void **memptr, size_t alignment, size_t size);
typedef void* TestAlignedMalloc(size_t size, size_t alignment);
typedef void* TestAlignedRealloc(void* memblock, size_t size, size_t alignment);
typedef void  TestAlignedFree(void* memblock);

TestMalloc*  Tmalloc;
TestCalloc*  Tcalloc;
TestRealloc* Trealloc;
TestFree*    Tfree;
TestAlignedFree* Taligned_free;
// call alignment-related function via pointer and check result's alignment
int   Tposix_memalign(void **memptr, size_t alignment, size_t size);
void* Taligned_malloc(size_t size, size_t alignment);
void* Taligned_realloc(void* memblock, size_t size, size_t alignment);

// pointers to alignment-related functions used while testing
TestPosixMemalign*  Rposix_memalign;
TestAlignedMalloc*  Raligned_malloc;
TestAlignedRealloc* Raligned_realloc;

bool error_occurred = false;

#if __APPLE__
// Tests that use the variable are skipped on Mac OS* X
#else
static bool perProcessLimits = true;
#endif

const size_t POWERS_OF_2 = 20;

struct MemStruct
{
    void* Pointer;
    UINT Size;

    MemStruct() : Pointer(NULL), Size(0) {}
    MemStruct(void* Pointer, UINT Size) : Pointer(Pointer), Size(Size) {}
};

class CMemTest: NoAssign
{
    UINT CountErrors;
    int total_threads;
    bool FullLog;
    static bool firstTime;

public:
    CMemTest(int total_threads,
             bool isVerbose=false) :
        CountErrors(0), total_threads(total_threads)
        {
            srand((UINT)time(NULL));
            FullLog=isVerbose;
            rand();
        }
    void InvariantDataRealloc(bool aligned); //realloc does not change data
    void NULLReturn(UINT MinSize, UINT MaxSize); // NULL pointer + check errno
    void UniquePointer(); // unique pointer - check with padding
    void AddrArifm(); // unique pointer - check with pointer arithmetic
    bool ShouldReportError();
    void Free_NULL(); // 
    void Zerofilling(); // check if arrays are zero-filled
    void TestAlignedParameters();
    void RunAllTests(int total_threads);
    ~CMemTest() {}
};

int argC;
char** argV;

struct RoundRobin: NoAssign {
    const long number_of_threads;
    mutable CMemTest test;

    RoundRobin( long p, bool verbose ) :
        number_of_threads(p), test(p, verbose) {}
    void operator()( int /*id*/ ) const 
        {
            test.RunAllTests(number_of_threads);
        }
};

bool CMemTest::firstTime = true;

static void setSystemAllocs()
{
    Tmalloc=malloc;
    Trealloc=realloc;
    Tcalloc=calloc;
    Tfree=free;
#if _WIN32 || _WIN64
    Raligned_malloc=_aligned_malloc;
    Raligned_realloc=_aligned_realloc;
    Taligned_free=_aligned_free;
    Rposix_memalign=0;
#elif  __APPLE__ || __sun //  Max OS X and Solaris don't have posix_memalign
    Raligned_malloc=0;
    Raligned_realloc=0;
    Taligned_free=0;
    Rposix_memalign=0;
#else 
    Raligned_malloc=0;
    Raligned_realloc=0;
    Taligned_free=0;
    Rposix_memalign=posix_memalign;
#endif
}

// check that realloc works as free and as malloc
void ReallocParam()
{
    const int ITERS = 1000;
    int i;
    void *bufs[ITERS];

    bufs[0] = Trealloc(NULL, 30*MByte);
    ASSERT(bufs[0], "Can't get memory to start the test.");
  
    for (i=1; i<ITERS; i++)
    {
        bufs[i] = Trealloc(NULL, 30*MByte);
        if (NULL == bufs[i])
            break;
    }
    ASSERT(i<ITERS, "Limits should be decreased for the test to work.");
  
    Trealloc(bufs[0], 0);
    /* There is a race for the free space between different threads at 
       this point. So, have to run the test sequentially.
    */
    bufs[0] = Trealloc(NULL, 30*MByte);
    ASSERT(bufs[0], NULL);
  
    for (int j=0; j<i; j++)
        Trealloc(bufs[j], 0);
}

struct TestStruct
{
    DWORD field1:2;
    DWORD field2:6;
    double field3;
    UCHAR field4[100];
    TestStruct* field5;
//  std::string field6;
    std::vector<int> field7;
    double field8;
    bool IzZero()
        {
            UCHAR *tmp;
            tmp=(UCHAR*)this;
            bool b=true;
            for (int i=0; i<(int)sizeof(TestStruct); i++)
                if (tmp[i]) b=false;
            return b;
        }
};

int Tposix_memalign(void **memptr, size_t alignment, size_t size)
{
    int ret = Rposix_memalign(memptr, alignment, size);
    if (0 == ret)
        ASSERT(0==((uintptr_t)*memptr & (alignment-1)),
               "allocation result should be aligned");
    return ret;
}
void* Taligned_malloc(size_t size, size_t alignment)
{
    void *ret = Raligned_malloc(size, alignment);
    if (0 != ret)
        ASSERT(0==((uintptr_t)ret & (alignment-1)),
               "allocation result should be aligned");
    return ret;
}
void* Taligned_realloc(void* memblock, size_t size, size_t alignment)
{
    void *ret = Raligned_realloc(memblock, size, alignment);
    if (0 != ret)
        ASSERT(0==((uintptr_t)ret & (alignment-1)),
               "allocation result should be aligned");
    return ret;
}

inline size_t choose_random_alignment() {
    return sizeof(void*)<<(rand() % POWERS_OF_2);
}

void CMemTest::InvariantDataRealloc(bool aligned)
{
    size_t size, sizeMin;
    CountErrors=0;
    if (FullLog) printf("\nInvariant data by realloc....");
    UCHAR* pchar;
    sizeMin=size=rand()%MAX_SIZE+10;
    pchar = aligned?
        (UCHAR*)Taligned_realloc(NULL,size,choose_random_alignment())
        : (UCHAR*)Trealloc(NULL,size);
    if (NULL == pchar)
        return;
    for (size_t k=0; k<size; k++)
        pchar[k]=(UCHAR)k%255+1;
    for (int i=0; i<COUNTEXPERIMENT; i++)
    {
        size=rand()%MAX_SIZE+10;
        UCHAR *pcharNew = aligned?
            (UCHAR*)Taligned_realloc(pchar,size, choose_random_alignment())
            : (UCHAR*)Trealloc(pchar,size);
        if (NULL == pcharNew)
            continue;
        pchar = pcharNew;
        sizeMin=size<sizeMin ? size : sizeMin;
        for (size_t k=0; k<sizeMin; k++)
            if (pchar[k] != (UCHAR)k%255+1)
            {
                CountErrors++;
                if (ShouldReportError())
                {
                    printf("stand '%c', must stand '%c'\n",pchar[k],(UCHAR)k%255+1);
                    printf("error: data changed (at %llu, SizeMin=%llu)\n",
                           (long long unsigned)k,(long long unsigned)sizeMin);
                }
            }
    }
    if (aligned)
        Taligned_realloc(pchar,0,choose_random_alignment());
    else
        Trealloc(pchar,0);
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
    //printf("end check\n");
}

void CMemTest::AddrArifm()
{
    int* MasPointer[COUNT_ELEM];
    size_t *MasCountElem = (size_t*)Tmalloc(COUNT_ELEM*sizeof(size_t));
    size_t count;
    int* tmpAddr;
    int j;
    UINT CountZero=0;
    CountErrors=0;
    if (FullLog) printf("\nUnique pointer using Address arithmetics\n");
    if (FullLog) printf("malloc....");
    ASSERT(MasCountElem, NULL);
    for (int i=0; i<COUNT_ELEM; i++)
    {
        count=rand()%MAX_SIZE;
        if (count == 0)
            CountZero++;
        tmpAddr=(int*)Tmalloc(count*sizeof(int));
        for (j=0; j<i; j++) // find a place for the new address
        {
            if (*(MasPointer+j)>tmpAddr) break;
        }
        for (int k=i; k>j; k--)
        {
            MasPointer[k]=MasPointer[k-1];
            MasCountElem[k]=MasCountElem[k-1];
        }
        MasPointer[j]=tmpAddr;
        MasCountElem[j]=count*sizeof(int);/**/
    }
    if (FullLog) printf("Count zero: %d\n",CountZero);
    for (int i=0; i<COUNT_ELEM-1; i++)
    {
        if (NULL!=MasPointer[i] && NULL!=MasPointer[i+1]
            && (uintptr_t)MasPointer[i]+MasCountElem[i] > (uintptr_t)MasPointer[i+1])
        {
            CountErrors++;
            if (ShouldReportError()) printf("malloc: intersection detect at 0x%p between %llu element(int) and 0x%p\n",
                                            (MasPointer+i),(long long unsigned)MasCountElem[i],(MasPointer+i+1));
        }
    }
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
    //----------------------------------------------------------------
    CountErrors=0;
    if (FullLog) printf("realloc....");
    for (int i=0; i<COUNT_ELEM; i++)
    {
        count=MasCountElem[i]*2;
        if (count == 0)
            CountZero++;
        tmpAddr=(int*)Trealloc(MasPointer[i],count);
        if (NULL==tmpAddr && 0!=count)
            Tfree(MasPointer[i]);
        for (j=0; j<i; j++) // find a place for the new address
        {
            if (*(MasPointer+j)>tmpAddr) break;
        }
        for (int k=i; k>j; k--)
        {
            MasPointer[k]=MasPointer[k-1];
            MasCountElem[k]=MasCountElem[k-1];
        }
        MasPointer[j]=tmpAddr;
        MasCountElem[j]=count;//*sizeof(int);/**/
    }
    if (FullLog) printf("Count zero: %d\n",CountZero);

    // now we have a sorted array of pointers
    for (int i=0; i<COUNT_ELEM-1; i++)
    {
        if (NULL!=MasPointer[i] && NULL!=MasPointer[i+1]
            && (uintptr_t)MasPointer[i]+MasCountElem[i] > (uintptr_t)MasPointer[i+1])
        {
            CountErrors++;
            if (ShouldReportError()) printf("realloc: intersection detect at 0x%p between %llu element(int) and 0x%p\n",
                                            (MasPointer+i),(long long unsigned)MasCountElem[i],(MasPointer+i+1));
        }
    }
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
    for (int i=0; i<COUNT_ELEM; i++)
    {
        Tfree(MasPointer[i]);
    }
    //-------------------------------------------
    CountErrors=0;
    if (FullLog) printf("calloc....");
    for (int i=0; i<COUNT_ELEM; i++)
    {
        count=rand()%MAX_SIZE;
        if (count == 0)
            CountZero++;
        tmpAddr=(int*)Tcalloc(count*sizeof(int),2);
        for (j=0; j<i; j++) // find a place for the new address
        {
            if (*(MasPointer+j)>tmpAddr) break;
        }
        for (int k=i; k>j; k--)
        {
            MasPointer[k]=MasPointer[k-1];
            MasCountElem[k]=MasCountElem[k-1];
        }
        MasPointer[j]=tmpAddr;
        MasCountElem[j]=count*sizeof(int)*2;/**/
    }
    if (FullLog) printf("Count zero: %d\n",CountZero);

    // now we have a sorted array of pointers
    for (int i=0; i<COUNT_ELEM-1; i++)
    {
        if (NULL!=MasPointer[i] && NULL!=MasPointer[i+1]
            && (uintptr_t)MasPointer[i]+MasCountElem[i] > (uintptr_t)MasPointer[i+1])
        {
            CountErrors++;
            if (ShouldReportError()) printf("calloc: intersection detect at 0x%p between %llu element(int) and 0x%p\n",
                                            (MasPointer+i),(long long unsigned)MasCountElem[i],(MasPointer+i+1));
        }
    }
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
    for (int i=0; i<COUNT_ELEM; i++)
    {
        Tfree(MasPointer[i]);
    }
    Tfree(MasCountElem);
}

void CMemTest::Zerofilling()
{
    TestStruct* TSMas;
    size_t CountElement;
    CountErrors=0;
    if (FullLog) printf("\nzeroings elements of array....");
    //test struct
    for (int i=0; i<COUNTEXPERIMENT; i++)
    {
        CountElement=rand()%MAX_SIZE;
        TSMas=(TestStruct*)Tcalloc(CountElement,sizeof(TestStruct));
        if (NULL == TSMas)
            continue;
        for (size_t j=0; j<CountElement; j++)
        {
            if (!TSMas->IzZero())
            {
                CountErrors++;
                if (ShouldReportError()) printf("detect nonzero element at TestStruct\n");
            }
        }
        Tfree(TSMas);
    }
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
}

// As several threads concurrently trying to push to memory limits, adding to 
// vectors may have intermittent failures.  
void reliablePushBack(std::vector<MemStruct> *vec, const MemStruct &mStruct)
{
    for (int i=0; i<10000; i++) {
        try {
            vec->push_back(mStruct);
        } catch(std::bad_alloc) {
            continue;
        }
        return;
    }
    ASSERT(0, "Unable to get free memory.");
}

void CMemTest::NULLReturn(UINT MinSize, UINT MaxSize)
{
    std::vector<MemStruct> PointerList;
    void *tmp;
    CountErrors=0;
    int CountNULL;
    if (FullLog) printf("\nNULL return & check errno:\n");
    UINT Size;
    do {
        Size=rand()%(MaxSize-MinSize)+MinSize;
        tmp=Tmalloc(Size);
        if (tmp != NULL)
        {
            memset(tmp, 0, Size);
            reliablePushBack(&PointerList, MemStruct(tmp, Size));
        }
    } while(tmp != NULL);
    if (FullLog) printf("\n");

    // preparation complete, now running tests
    // malloc
    if (FullLog) printf("malloc....");
    CountNULL = 0;
    while (CountNULL==0)
        for (int j=0; j<COUNT_TESTS; j++)
        {
            Size=rand()%(MaxSize-MinSize)+MinSize;
            errno = ENOMEM+j+1;
            tmp=Tmalloc(Size);
            if (tmp == NULL)
            {
                CountNULL++;
                if (errno != ENOMEM) {
                    CountErrors++;
                    if (ShouldReportError()) printf("NULL returned, error: errno (%d) != ENOMEM\n", errno);
                }
            }
            else
            {
                // Technically, if malloc returns a non-NULL pointer, it is allowed to set errno anyway.
                // However, on most systems it does not set errno.
                bool known_issue = false;
#if __linux__
                if( errno==ENOMEM ) known_issue = true;
#endif /* __linux__ */
                if (errno != ENOMEM+j+1 && !known_issue) {
                    CountErrors++;
                    if (ShouldReportError()) printf("error: errno changed to %d though valid pointer was returned\n", errno);
                }      
                memset(tmp, 0, Size);
                reliablePushBack(&PointerList, MemStruct(tmp, Size));
            }
        }
    if (FullLog) printf("end malloc\n");
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;

    CountErrors=0;
    //calloc
    if (FullLog) printf("calloc....");
    CountNULL = 0;
    while (CountNULL==0)
        for (int j=0; j<COUNT_TESTS; j++)
        {
            Size=rand()%(MaxSize-MinSize)+MinSize;
            errno = ENOMEM+j+1;
            tmp=Tcalloc(COUNT_ELEM_CALLOC,Size);  
            if (tmp == NULL)
            {
                CountNULL++;
                if (errno != ENOMEM) {
                    CountErrors++;
                    if (ShouldReportError()) printf("NULL returned, error: errno(%d) != ENOMEM\n", errno);
                }
            }
            else
            {
                // Technically, if calloc returns a non-NULL pointer, it is allowed to set errno anyway.
                // However, on most systems it does not set errno.
                bool known_issue = false;
#if __linux__
                if( errno==ENOMEM ) known_issue = true;
#endif /* __linux__ */
                if (errno != ENOMEM+j+1 && !known_issue) {
                    CountErrors++;
                    if (ShouldReportError()) printf("error: errno changed to %d though valid pointer was returned\n", errno);
                }      
                reliablePushBack(&PointerList, MemStruct(tmp, Size));
            }
        }
    if (FullLog) printf("end calloc\n");
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
    CountErrors=0;
    if (FullLog) printf("realloc....");
    CountNULL = 0;
    if (PointerList.size() > 0)
        while (CountNULL==0)
            for (size_t i=0; i<(size_t)COUNT_TESTS && i<PointerList.size(); i++)
            {
                errno = 0;
                tmp=Trealloc(PointerList[i].Pointer,PointerList[i].Size*2);
                if (PointerList[i].Pointer == tmp) // the same place
                {
                    bool known_issue = false;
#if __linux__
                    if( errno==ENOMEM ) known_issue = true;
#endif /* __linux__ */
                    if (errno != 0 && !known_issue) {
                        CountErrors++;
                        if (ShouldReportError()) printf("valid pointer returned, error: errno not kept\n");
                    }      
                    PointerList[i].Size *= 2;
                }
                else if (tmp != PointerList[i].Pointer && tmp != NULL) // another place
                {
                    bool known_issue = false;
#if __linux__
                    if( errno==ENOMEM ) known_issue = true;
#endif /* __linux__ */
                    if (errno != 0 && !known_issue) {
                        CountErrors++;
                        if (ShouldReportError()) printf("valid pointer returned, error: errno not kept\n");
                    }      
                    PointerList[i].Pointer = tmp;
                    PointerList[i].Size *= 2;
                }
                else if (tmp == NULL)
                {
                    CountNULL++;
                    if (errno != ENOMEM)
                    {
                        CountErrors++;
                        if (ShouldReportError()) printf("NULL returned, error: errno(%d) != ENOMEM\n", errno);
                    }
                    // check data integrity
                    BYTE *zer=(BYTE*)PointerList[i].Pointer;
                    for (UINT k=0; k<PointerList[i].Size; k++)
                        if (zer[k] != 0)
                        {
                            CountErrors++;
                            if (ShouldReportError()) printf("NULL returned, error: data changed\n");
                        }
                }
            }
    if (FullLog) printf("realloc end\n");
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
    for (UINT i=0; i<PointerList.size(); i++)
    {
        Tfree(PointerList[i].Pointer);
    }
}


void CMemTest::UniquePointer()
{
    CountErrors=0;
    int* MasPointer[COUNT_ELEM];
    size_t *MasCountElem = (size_t*)Tmalloc(sizeof(size_t)*COUNT_ELEM);
    if (FullLog) printf("\nUnique pointer using 0\n");
    ASSERT(MasCountElem, NULL);
    //
    //-------------------------------------------------------
    //malloc
    for (int i=0; i<COUNT_ELEM; i++)
    {
        MasCountElem[i]=rand()%MAX_SIZE;
        MasPointer[i]=(int*)Tmalloc(MasCountElem[i]*sizeof(int));
        if (NULL == MasPointer[i])
            MasCountElem[i]=0;
        for (UINT j=0; j<MasCountElem[i]; j++)
            *(MasPointer[i]+j)=0;
    }
    if (FullLog) printf("malloc....");
    for (UINT i=0; i<COUNT_ELEM-1; i++)
    {
        for (UINT j=0; j<MasCountElem[i]; j++)
        {
            if (*(*(MasPointer+i)+j)!=0)
            {
                CountErrors++;
                if (ShouldReportError()) printf("error, detect 1 with 0x%p\n",(*(MasPointer+i)+j));
            }
            *(*(MasPointer+i)+j)+=1;
        }
    }
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
    //----------------------------------------------------------
    //calloc
    for (int i=0; i<COUNT_ELEM; i++)
        Tfree(MasPointer[i]);
    CountErrors=0;
    for (long i=0; i<COUNT_ELEM; i++)
    {
        MasPointer[i]=(int*)Tcalloc(MasCountElem[i]*sizeof(int),2);
        if (NULL == MasPointer[i])
            MasCountElem[i]=0;
    }
    if (FullLog) printf("calloc....");
    for (int i=0; i<COUNT_ELEM-1; i++)
    {
        for (UINT j=0; j<*(MasCountElem+i); j++)
        {
            if (*(*(MasPointer+i)+j)!=0)
            {
                CountErrors++;
                if (ShouldReportError()) printf("error, detect 1 with 0x%p\n",(*(MasPointer+i)+j));
            }
            *(*(MasPointer+i)+j)+=1;
        }
    }
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
    //---------------------------------------------------------
    //realloc
    CountErrors=0;
    for (int i=0; i<COUNT_ELEM; i++)
    {
        MasCountElem[i]*=2;
        *(MasPointer+i)=
            (int*)Trealloc(*(MasPointer+i),MasCountElem[i]*sizeof(int));
        if (NULL == MasPointer[i])
            MasCountElem[i]=0;
        for (UINT j=0; j<MasCountElem[i]; j++)
            *(*(MasPointer+i)+j)=0;
    }
    if (FullLog) printf("realloc....");
    for (int i=0; i<COUNT_ELEM-1; i++)
    {
        for (UINT j=0; j<*(MasCountElem+i); j++)
        {
            if (*(*(MasPointer+i)+j)!=0)
            {
                CountErrors++;
            }
            *(*(MasPointer+i)+j)+=1;
        }
    }
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
    for (int i=0; i<COUNT_ELEM; i++)
        Tfree(MasPointer[i]);
    Tfree(MasCountElem);
}

bool CMemTest::ShouldReportError()
{
    if (FullLog)
        return true;
    else
        if (firstTime) {
            firstTime = false;
            return true;
        } else
            return false;
}

void CMemTest::Free_NULL()
{
    CountErrors=0;
    if (FullLog) printf("\ncall free with parameter NULL....");
    errno = 0;
    for (int i=0; i<COUNTEXPERIMENT; i++)
    {
        Tfree(NULL);
        if (errno != 0)
        {
            CountErrors++;
            if (ShouldReportError()) printf("error is found by a call free with parameter NULL\n");
        }
    }
    if (CountErrors) printf("%s\n",strError);
    else if (FullLog) printf("%s\n",strOk);
    error_occurred |= ( CountErrors>0 ) ;
}

void CMemTest::TestAlignedParameters()
{
    void *memptr;
    int ret;

    if (Rposix_memalign) {
        // alignment isn't power of 2
        for (int bad_align=3; bad_align<16; bad_align++)
            if (bad_align&(bad_align-1)) {
                ret = Tposix_memalign(NULL, bad_align, 100);
                ASSERT(EINVAL==ret, NULL);
            }
    
        memptr = &ret;
        ret = Tposix_memalign(&memptr, 5*sizeof(void*), 100);
        ASSERT(memptr == &ret,
               "memptr should not be changed after unsuccesful call");
        ASSERT(EINVAL==ret, NULL);
    
        // alignment is power of 2, but not a multiple of sizeof(void *),
        // we expect that sizeof(void*) > 2
        ret = Tposix_memalign(NULL, 2, 100);
        ASSERT(EINVAL==ret, NULL);
    }
    if (Raligned_malloc) {
        // alignment isn't power of 2
        for (int bad_align=3; bad_align<16; bad_align++)
            if (bad_align&(bad_align-1)) {
                memptr = Taligned_malloc(100, bad_align);
                ASSERT(NULL==memptr, NULL);
                ASSERT(EINVAL==errno, NULL);
            }
    
        // size is zero
        memptr = Taligned_malloc(0, 16);
        ASSERT(NULL==memptr, "size is zero, so must return NULL");
        ASSERT(EINVAL==errno, NULL);
    }
    if (Taligned_free) {
        // NULL pointer is OK to free
        errno = 0;
        Taligned_free(NULL);
        /* As there is no return value for free, strictly speaking we can't 
           check errno here. But checked implementations obey the assertion.
        */
        ASSERT(0==errno, NULL);
    }
    if (Raligned_realloc) {
        for (int i=1; i<20; i++) {
            // checks that calls work correctly in presence of non-zero errno
            errno = i;
            void *ptr = Taligned_malloc(i*10, 128);
            ASSERT(NULL!=ptr, NULL);
            ASSERT(0!=errno, NULL);
            // if size is zero and pointer is not NULL, works like free
            memptr = Taligned_realloc(ptr, 0, 64);
            ASSERT(NULL==memptr, NULL);
            ASSERT(0!=errno, NULL);
        }
        // alignment isn't power of 2
        for (int bad_align=3; bad_align<16; bad_align++)
            if (bad_align&(bad_align-1)) {
                void *ptr = &bad_align;
                memptr = Taligned_realloc(&ptr, 100, bad_align);
                ASSERT(NULL==memptr, NULL);
                ASSERT(&bad_align==ptr, NULL);
                ASSERT(EINVAL==errno, NULL);
            }
    }
}

void CMemTest::RunAllTests(int /*total_threads*/)
{
    Zerofilling();
    Free_NULL();
    InvariantDataRealloc(/*aligned=*/false);
    if (Raligned_realloc)
        InvariantDataRealloc(/*aligned=*/true);
    TestAlignedParameters();
    if (FullLog) printf("All tests ended\nclearing memory....");
}

int main(int argc, char* argv[])
{
    argC=argc;
    argV=argv;
    MaxThread = MinThread = 1;
    Tmalloc=scalable_malloc;
    Trealloc=scalable_realloc;
    Tcalloc=scalable_calloc;
    Tfree=scalable_free;
    Rposix_memalign=scalable_posix_memalign;
    Raligned_malloc=scalable_aligned_malloc;
    Raligned_realloc=scalable_aligned_realloc;
    Taligned_free=scalable_aligned_free;

    // check if we were called to test standard behavior
    for (int i=1; i< argc; i++) {
        if (strcmp((char*)*(argv+i),"-s")==0)
        {
            setSystemAllocs();
            argC--;
            break;
        }
    }

    ParseCommandLine( argC, argV );

    CMemTest test(1, Verbose);
    // Tests with memory limit also run in serial only
#if __APPLE__
    /* Skip due to lack of memory limit enforcing under Mac OS X. */
#else
    limitMem(200);
    ReallocParam(); /* this test should be first to have enough unfragmented memory */
    test.NULLReturn(1*MByte,100*MByte);
    limitMem(0);
#endif

    // Long tests, so running in serial only. TODO - rework to make shorter
#if __APPLE__
    /* TODO:  check why skipped on Mac OS X */
#else
    test.UniquePointer();
    test.AddrArifm();
#endif

#if __linux__
    /* According to man pthreads 
       "NPTL threads do not share resource limits (fixed in kernel 2.6.10)".
       Use per-threads limits for affected systems.
     */
    if ( LinuxKernelVersion() < 2*1000000 + 6*1000 + 10)
        perProcessLimits = false;
#endif    

    // Tests running in parallel
    for( int p=MaxThread; p>=MinThread; --p ) {
        if( Verbose )
            printf("testing with %d threads\n", p );
        NativeParallelFor( p, RoundRobin(p, Verbose) );
    }

    if( !error_occurred ) printf("done\n");
    return 0;
}
