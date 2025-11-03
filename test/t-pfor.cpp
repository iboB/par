// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <doctest/doctest.h>
#include <vector>
#include <thread>
#include <set>

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
