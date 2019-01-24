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

// to avoid usage of #pragma comment
#define __TBB_NO_IMPLICIT_LINKAGE 1
#define __TBB_TASK_CPP_DIRECTLY_INCLUDED 1
#include "../tbb/task.cpp"

//------------------------------------------------------------------------
// Test that important assertions in class task fail as expected.
//------------------------------------------------------------------------

#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness.h"
#include "harness_bad_expr.h"

//! Task that will be abused.
tbb::task* volatile AbusedTask;

//! Number of times that AbuseOneTask
int AbuseOneTaskRan;

//! Body used to create task in thread 0 and abuse it in thread 1.
struct AbuseOneTask {
    void operator()( int ) const {
        tbb::task_scheduler_init init;
        // Thread 1 attempts to incorrectly use the task created by thread 0.
        TRY_BAD_EXPR(AbusedTask->spawn(*AbusedTask),"owne");
        TRY_BAD_EXPR(AbusedTask->spawn_and_wait_for_all(*AbusedTask),"owne");
        TRY_BAD_EXPR(tbb::task::spawn_root_and_wait(*AbusedTask),"owne");

        // Try variant that operate on a tbb::task_list
        tbb::task_list list;
        TRY_BAD_EXPR(AbusedTask->spawn(list),"owne");
        TRY_BAD_EXPR(AbusedTask->spawn_and_wait_for_all(list),"owne");
        // spawn_root_and_wait over empty list should vacuously succeed.
        tbb::task::spawn_root_and_wait(list);

        // Check that spawn_root_and_wait fails on non-empty list. 
        list.push_back(*AbusedTask);
        TRY_BAD_EXPR(tbb::task::spawn_root_and_wait(list),"owne");

        TRY_BAD_EXPR(AbusedTask->destroy(*AbusedTask),"owne");
        TRY_BAD_EXPR(AbusedTask->wait_for_all(),"owne");

        // Try abusing recycle_as_continuation
        TRY_BAD_EXPR(AbusedTask->recycle_as_continuation(), "execute" );
        TRY_BAD_EXPR(AbusedTask->recycle_as_safe_continuation(), "execute" );
        TRY_BAD_EXPR(AbusedTask->recycle_to_reexecute(), "execute" );

        // Check correct use of depth parameter
        tbb::task::depth_type depth = AbusedTask->depth();
        ASSERT( depth==0, NULL );
        for( int k=1; k<=81; k*=3 ) {
            AbusedTask->set_depth(depth+k);
            ASSERT( AbusedTask->depth()==depth+k, NULL );
            AbusedTask->add_to_depth(k+1);
            ASSERT( AbusedTask->depth()==depth+2*k+1, NULL );
        }
        AbusedTask->set_depth(0);

        // Try abusing the depth parameter
        TRY_BAD_EXPR(AbusedTask->set_depth(-1),"negative");
        TRY_BAD_EXPR(AbusedTask->add_to_depth(-1),"negative");

        ++AbuseOneTaskRan;
    }
};

//! Test various __TBB_ASSERT assertions related to class tbb::task.
void TestTaskAssertions() {
#if TBB_USE_ASSERT
    // Catch assertion failures
    tbb::set_assertion_handler( AssertionFailureHandler );
    tbb::task_scheduler_init init;
    // Create task to be abused
    AbusedTask = new( tbb::task::allocate_root() ) tbb::empty_task;
    NativeParallelFor( 1, AbuseOneTask() );
    ASSERT( AbuseOneTaskRan==1, NULL );
    AbusedTask->destroy(*AbusedTask);
    // Restore normal assertion handling
    tbb::set_assertion_handler( NULL );
#endif /* TBB_USE_ASSERT */
}

//------------------------------------------------------------------------
int main(int argc, char* argv[]) {
#if __GLIBC__==2&&__GLIBC_MINOR__==3
    printf("skip\n");
#else
    TestTaskAssertions();
    printf("done\n");
#endif
    return 0;
}
