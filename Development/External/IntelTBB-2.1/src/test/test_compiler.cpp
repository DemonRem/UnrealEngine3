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

#include <stdio.h>

union char2bool {
    unsigned char c;
    volatile bool b;
} u;

// The function proves the compiler uses 0 or 1 to store a bool. It
// inspects what a compiler does when it loads a bool.  A compiler that
// uses a value other than 0 or 1 to represent a bool will have to normalize
// the value to 0 or 1 when the bool is cast to an unsigned char.
// Compilers that pass this test do not do the normalization, and thus must
// be assuming that a bool is a 0 or 1.
int test_bool_representation() {
    for( unsigned i=0; i<256; ++i ) {
        u.c = (unsigned char)i;
        unsigned char x = (unsigned char)u.b;
        if( x != i ) {
            printf("Test failed at iteration i=%d\n",i);
            return 1;
        }
    }
    return 0;
}

int main() {
    if( test_bool_representation()!=0 )
        printf("ERROR: bool representation test failed\n");
    else
        printf("done\n");
    return 0;
}
