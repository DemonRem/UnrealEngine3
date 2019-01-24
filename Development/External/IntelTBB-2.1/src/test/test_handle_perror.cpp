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

// Program for basic correctness of handle_perror, which is internal
// to the TBB shared library.

#include <cerrno>
#include <stdexcept>
#include "tbb/tbb_misc.h"
#include "harness.h"

static void TestHandlePerror() {
    bool caught = false;
    try {
	tbb::internal::handle_perror( EAGAIN, "apple" );
    } catch( std::runtime_error& e ) {
	if( Verbose )
	    printf("caught runtime_exception('%s')\n",e.what());
 	ASSERT( memcmp(e.what(),"apple: ",7)==0, NULL );
        ASSERT( strstr(e.what(),"unavailable")!=NULL, "bad error message?" ); 
	caught = true;
    }
    ASSERT(caught,NULL);
}

int main( int argc, char* argv[] ) {
    ParseCommandLine( argc, argv );
    TestHandlePerror();
    printf("done\n");
    return 0;
}
