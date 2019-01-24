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

#include "tbb/concurrent_hash_map.h"

namespace tbb {

namespace internal {

bool hash_map_segment_base::internal_grow_predicate() const {
    // Intel(R) Thread Checker considers the following reads to be races, so we hide them in the 
    // library so that Intel(R) Thread Checker will ignore them.  The reads are used in a double-check
    // context, so the program is nonetheless correct despite the race.
    return my_logical_size >= my_physical_size && my_physical_size < internal::hash_map_base::max_physical_size;
}

} // namespace internal

} // namespace tbb

