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

#include <cstdlib>
#include <math.h>
#include <vector>
#include <string>
#include "tbb/tbb_stddef.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/tick_count.h"
#include "tbb/blocked_range.h"
#include "../test/harness.h"
#include "harness_barrier.h"
#define STATISTICS_INLINE
#include "statistics.h"

class Timer {
    tbb::tick_count tick;
public:
    Timer() { tick = tbb::tick_count::now(); }
    double get_time()  { return (tbb::tick_count::now() - tick).seconds(); }
    double diff_time(const Timer &newer) { return (newer.tick - tick).seconds(); }
    double mark_time() { tbb::tick_count t1(tbb::tick_count::now()), t2(tick); tick = t1; return (t1 - t2).seconds(); }
    double mark_time(const Timer &newer) { tbb::tick_count t(tick); tick = newer.tick; return (tick - t).seconds(); }
};

class TesterBase /*: public tbb::internal::no_copy*/ {
protected:
    friend class TestProcessor;
    friend class TestRunner;

    //! it is barrier for synchronizing between threads
    spin_barrier *barrier;
    
    //! number of tests per this tester
    const int tests_count;
    
    //! number of threads to operate
    int threads_count;

    //! some value for tester
    int value;
    
public:
    //! init tester base. @arg ntests is number of embeded tests in this tester.
    TesterBase(int ntests)
        : barrier(NULL), tests_count(ntests)
    {}
    virtual ~TesterBase() {}

    //! internal function
    void base_init(int v, int t, spin_barrier &b) {
        threads_count = t;
        barrier = &b;
        value = v;
        init();
    }

    //! optionally override to init after value and threads count were set.
    virtual void init() { }

    //! Override to provide your names
    virtual std::string get_name(int testn) {
        return Format("test %d", testn);
    }

    //! Override to provide main test's entry function returns a value to record
    virtual value_t test(int testn, int threadn) = 0;
};

/*****
a user's tester concept:

class tester: public TesterBase {
public:
    //! init tester with known amount of work
    tester() : TesterBase(<user-specified tests count>) { ... }

    //! run a test with sequental number @arg test_number for @arg thread.
	/ *override* / value_t test(int test_number, int thread);
};

******/

template<typename Tester, bool longest_time = false>
class TimeTest : public Tester {
    /*override*/ value_t test(int testn, int threadn) {
        Timer timer;
        Tester::test(testn, threadn);
        if(longest_time) Tester::barrier->wait();
        return timer.get_time();
    }
};

template<typename Tester, bool longest_time = false>
class NanosecPerValue : public Tester {
    /*override*/ value_t test(int testn, int threadn) {
        Timer timer;
        Tester::test(testn, threadn);
        if(longest_time) Tester::barrier->wait();
        // return time (ns) per value
        return timer.get_time()*1000000.0/double(Tester::value);
    }
};

// operate with single tester
class TestRunner {
    friend class TestProcessor;
    friend struct RunArgsBody;
    TestRunner(const TestRunner &); // don't copy

    const char *tester_name;
    StatisticsCollector *stat;
    std::vector<std::vector<StatisticsCollector::TestCase> > keys;

public:
    TesterBase &tester;

    template<typename Test>
    TestRunner(const char *name, Test *test)
        : tester_name(name), tester(*dynamic_cast<TesterBase*>(test))
    {}
    
    ~TestRunner() { delete &tester; }

    void init(int value, int threads, spin_barrier &barrier, StatisticsCollector *s) {
        tester.base_init(value, threads, barrier);
        stat = s;
        keys.resize(tester.tests_count);
        for(int testn = 0; testn < tester.tests_count; testn++) {
            keys[testn].resize(threads);
            std::string test_name(tester.get_name(testn));
            for(int threadn = 0; threadn < threads; threadn++)
                keys[testn][threadn] = stat->SetTestCase(tester_name, test_name.c_str(), threadn);
        }
    }

    void test(int threadn) {
        for(int testn = 0; testn < tester.tests_count; testn++) {
            tester.barrier->wait();
            value_t result = tester.test(testn, threadn);
            stat->AddRoundResult(keys[testn][threadn], result);
        }
    }

    void post_process(StatisticsCollector &report) {
        const int threads = tester.threads_count;
        for(int testn = 0; testn < tester.tests_count; testn++) {
            size_t coln = keys[testn][0].getResults().size()-1;
            value_t result = keys[testn][0].getResults()[coln];
            for(int threadn = 1; threadn < threads; threadn++) {
                result += keys[testn][threadn].getResults()[coln]; // for SUM or AVG
                // can do also: MIN/MAX
            }
            result /= threads; // AVG
            report.SetTestCase(tester_name, tester.get_name(testn).c_str(), threads);
            report.AddRoundResult(result); // final result
        }
    }
};

struct RunArgsBody {
    const vector<TestRunner*> &run_list;
    RunArgsBody(const vector<TestRunner*> &a) : run_list(a) { }
    void operator()(const tbb::blocked_range<int>& threads) const {
        for(int thread = threads.begin(); thread < threads.end(); thread++)
            for(size_t i = 0; i < run_list.size(); i++)
                run_list[i]->test(thread);
    }
};

//! Main test processor.
/** Override or use like this:
 class MyTestCollection : public TestProcessor {
    void factory(int value, int threads) {
        process( value, threads,
            run("my1", new tester<my1>() ),
            run("my2", new tester<my2>() ),
        end );
        if(value == threads)
            stat->Print();
    }
};
*/

class TestProcessor {
    friend class TesterBase;

    // <threads, collector>
    typedef std::map<int, StatisticsCollector *> statistics_collection;
    statistics_collection stat_by_threads;

protected:
    // Members
    const char *collection_name;
    // current stat
    StatisticsCollector *stat;
    // token
    size_t end;

    // iterations parameters
    //int min_value, act_index, max_value;
    //int min_thread, act_thread, max_thread;

public:
    StatisticsCollector report;

    // token of tests list
    template<typename Test>
    TestRunner *run(const char *name, Test *test) {
        return new TestRunner(name, test);
    }

    // iteration processing
    void process(int value, int threads, ...) {
        // prepare items
        stat = stat_by_threads[threads];
        if(!stat) {
            stat_by_threads[threads] = stat = new StatisticsCollector((collection_name + Format("@%d", threads)).c_str(), StatisticsCollector::ByAlg);
            stat->SetTitle("Detailed log of %s running with %d threads. Thread #0 is AVG across all threads", collection_name, threads);
        }
        spin_barrier barrier(threads);
        // init args
        va_list args; va_start(args, threads);
        vector<TestRunner*> run_list; run_list.reserve(16);
        while(true) {
            TestRunner *item = va_arg(args, TestRunner*);
            if( !item ) break;
            item->init(value, threads, barrier, stat);
            run_list.push_back(item);
        }
        va_end(args);
        const size_t round_number = stat->GetRoundsCount();
        stat->SetRoundTitle(round_number, value);
        report.SetRoundTitle(round_number, value);
        // run them
        NativeParallelFor(tbb::blocked_range<int>(0, threads, 1), RunArgsBody(run_list));
        // destroy args
        for(size_t i = 0; i < run_list.size(); i++) {
            run_list[i]->post_process(report);
            delete run_list[i];
        }
    }

public:
    TestProcessor(const char *name)
        : collection_name(name), stat(NULL), end(0), report(collection_name, StatisticsCollector::ByAlg)
    { }

    ~TestProcessor() {
        for(statistics_collection::iterator i = stat_by_threads.begin(); i != stat_by_threads.end(); i++)
            delete i->second;
    }
};
