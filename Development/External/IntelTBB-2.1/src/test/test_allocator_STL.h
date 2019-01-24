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

// Tests for compatibility with the host's STL.

#include "harness.h"

template<typename Container>
void TestSequence() {
    Container c;
    for( int i=0; i<1000; ++i )
        c.push_back(i*i);    
    typename Container::const_iterator p = c.begin();
    for( int i=0; i<1000; ++i ) {
        ASSERT( *p==i*i, NULL );
        ++p;
    }
}

template<typename Set>
void TestSet() {
    Set s;
    typedef typename Set::value_type value_type;
    for( int i=0; i<100; ++i ) 
        s.insert(value_type(3*i));
    for( int i=0; i<300; ++i ) {
        ASSERT( s.erase(i)==size_t(i%3==0), NULL );
    }
}

template<typename Map>
void TestMap() {
    Map m;
    typedef typename Map::value_type value_type;
    for( int i=0; i<100; ++i ) 
        m.insert(value_type(i,i*i));
    for( int i=0; i<100; ++i )
        ASSERT( m.find(i)->second==i*i, NULL );
}

#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>

template<template<typename T> class Allocator>
void TestAllocatorWithSTL() {
    // Sequenced containers
    TestSequence<std::deque <int,Allocator<int> > >();
    TestSequence<std::list  <int,Allocator<int> > >();
    TestSequence<std::vector<int,Allocator<int> > >();

    // Associative containers
    TestSet<std::set     <int, std::less<int>, Allocator<int> > >();
    TestSet<std::multiset<int, std::less<int>, Allocator<int> > >();
    TestMap<std::map     <int, int, std::less<int>, Allocator<std::pair<const int,int> > > >();
    TestMap<std::multimap<int, int, std::less<int>, Allocator<std::pair<const int,int> > > >();

#if _WIN32||_WIN64
    // Test compatibility with Microsoft's implementation of std::allocator for some cases that
    // are undefined according to the ISO standard but permitted by Microsoft.
    TestSequence<std::deque <const int,Allocator<const int> > >();
#if _CPPLIB_VER>=500
    TestSequence<std::list  <const int,Allocator<const int> > >();
#endif /* _CPPLIB_VER>=500 */
    TestSequence<std::vector<const int,Allocator<const int> > >();
    TestSet<std::set<const int, std::less<int>, Allocator<const int> > >();
    TestMap<std::map<int, int, std::less<int>, Allocator<std::pair<int,int> > > >();
    TestMap<std::map<const int, int, std::less<int>, Allocator<std::pair<const int,int> > > >();
    TestMap<std::multimap<int, int, std::less<int>, Allocator<std::pair<int,int> > > >();
    TestMap<std::multimap<const int, int, std::less<int>, Allocator<std::pair<const int,int> > > >();
#endif /* _WIN32||_WIN64 */
}

