// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <doctest/doctest.h>
#include <vector>
#include <thread>
#include <set>
#include <algorithm>
#include <mutex>

TEST_CASE("pfor dynamic") {
    static constexpr uint32_t num_threads = 4;
    par::thread_pool pool("test", 4);

    auto run_test_func = [&](uint32_t max_par) {
        std::atomic_int count = 0;
        par::pfor(pool, {.max_par = max_par}, 0, 1000, [&](int i) {
            count += i;
        });
        CHECK(count == 500 * 999);
    };

    run_test_func(1);
    run_test_func(3);
    run_test_func(num_threads);
    run_test_func(1000);
    run_test_func(0);
}

TEST_CASE("pfor static") {
    static constexpr uint32_t num_threads = 4;
    par::thread_pool pool("test", num_threads);

    static constexpr size_t size = 100;

    auto run_test = [&](uint32_t par) {
        std::vector<std::thread::id> thread_ids(size);
        par::pfor(pool, {.sched = par::schedule_static, .max_par = par}, size_t(0), size, [&](size_t i) {
            thread_ids[i] = std::this_thread::get_id();
        });

        std::set<std::thread::id> unique_ids(thread_ids.begin(), thread_ids.end());
        CHECK(unique_ids.size() == par); // 4 workers + caller

        std::atomic_size_t count = 0;
        par::pfor(pool, {.sched = par::schedule_static, .max_par = par}, size_t(0), size, [&](size_t i) {
            CHECK(thread_ids[i] == std::this_thread::get_id());
            count += i;
        });
        CHECK(count == 50 * 99);
    };

    run_test(1);
    run_test(3);
    run_test(num_threads);
    run_test(num_threads + 1);
}

TEST_CASE("pfor single-thread") {
    auto caller_tid = std::this_thread::get_id();

    par::pfor(par::default_run_opts, 0, 1, [&](int i) {
        CHECK(i == 0);
        CHECK(std::this_thread::get_id() == caller_tid);
    });

    auto run_on_one_thread = [&](par::schedule sched) {
        int count = 0;
        par::pfor({.sched = sched, .max_par = 1}, 0, 100, [&](int i) {
            count += i;
            CHECK(std::this_thread::get_id() == caller_tid);
        });
        CHECK(count == 50 * 99);
    };
    run_on_one_thread(par::schedule_dynamic);
    run_on_one_thread(par::schedule_static);
}

TEST_CASE("pfor 0") {
    par::pfor({}, 0, 0, [](int) {
        CHECK(false); // should not be called
    });
    par::pfor({}, 100, 50, [](int) {
        CHECK(false); // should not be called
    });
}


TEST_CASE("pfor with job data") {
    struct job_data : public par::job_info {
        job_data(const par::job_info& ji) : par::job_info(ji) {}
        int i = 0;
        std::vector<int> vec;
    };

    static constexpr uint32_t num_threads = 4;
    par::thread_pool pool("test", num_threads);

    std::atomic_int32_t sum = 0;
    pfor<job_data>(pool, {.sched = par::schedule_static}, 0, 3, [&](int index, job_data& arg) {
        CHECK(arg.job_index == index);
        CHECK(arg.num_jobs == 3);
        CHECK(arg.i == 0);
        CHECK(arg.vec.empty());
        ++arg.i;
        arg.vec.push_back(index);
        sum += index;
    });
    CHECK(sum.load() == 3);

    pfor<job_data>(pool, {}, 0, 10, [&](int index, job_data& arg) {
        static constexpr uint32_t num_jobs = num_threads + 1;
        CHECK(arg.job_index < num_jobs);
        CHECK(arg.num_jobs == num_jobs);
        ++arg.i;
        arg.vec.push_back(index);
        sum += index;
    });
    CHECK(sum.load() == 48);
}

TEST_CASE("pfor ranges") {
    static constexpr uint32_t num_threads = 4;
    par::thread_pool pool("test", num_threads);

    using vec = std::vector<int>;

    auto run_test = [&](int begin, int end, int step, vec expected) {
        for (int chunk_size = 1; chunk_size <= 10; ++chunk_size) {
            std::mutex mtx;
            vec result;
            par::pfor(pool, {}, par::range(begin, end).with_step(step).with_iterations_per_job(chunk_size),
                [&](int i) {
                    std::lock_guard lock(mtx);
                    result.push_back(i);
                }
            );
            std::sort(result.begin(), result.end());
            CHECK(result == expected);
        }
    };

    run_test(0, 0, 1, {});
    run_test(0, 1, 1, {0});
    run_test(0, 10, 1, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    run_test(0, 10, 2, {0, 2, 4, 6, 8});
    run_test(1, 10, 2, {1, 3, 5, 7, 9});
    run_test(2, 10, 2, {2, 4, 6, 8});
    run_test(0, 10, 3, {0, 3, 6, 9});
    run_test(-6, 8, 3, {-6, -3, 0, 3, 6});
    run_test(0, 10, 15, {0});

    // note that output is sorted
    run_test(5, 0, -1, {1, 2, 3, 4, 5});
    run_test(10, 1, -2, {2, 4, 6, 8, 10});
    run_test(5, -5, -2, {-3, -1, 1, 3, 5});
}


TEST_CASE("pfor chunks") {
    static constexpr uint32_t num_threads = 4;
    par::thread_pool pool("test", num_threads);

    using rv = std::vector<std::pair<int, int>>;

    auto run_test = [&](int size, int chunk_size, rv expected) {
        rv ranges(expected.size(), std::make_pair(-1, 0));

        par::pfor(pool, {}, par::range(size).with_iterations_per_job(chunk_size),
            [&](int i) {
                auto& range = ranges[i / chunk_size];
                if (range.first == -1) {
                    CHECK(i == (i / chunk_size) * chunk_size);
                    range.first = i;
                }
                if (range.second != 0) {
                    CHECK(range.second == i);
                }
                range.second = i + 1;
            }
        );

        std::sort(ranges.begin(), ranges.end());
        CHECK(ranges == expected);
    };

    run_test(0, 1, {});
    run_test(10, 0, {});
    run_test(1, 1, {{0, 1}});
    run_test(10, 3, {{0, 3}, {3, 6}, {6, 9}, {9, 10}});
    run_test(10, 10, {{0, 10}});
    run_test(10, 15, {{0, 10}});
    run_test(100, 10, {
        {0, 10}, {10, 20}, {20, 30}, {30, 40}, {40, 50},
        {50, 60}, {60, 70}, {70, 80}, {80, 90}, {90, 100}
    });
    run_test(53, 7, {
        {0, 7}, {7, 14}, {14, 21}, {21, 28}, {28, 35},
        {35, 42}, {42, 49}, {49, 53}
    });
}

