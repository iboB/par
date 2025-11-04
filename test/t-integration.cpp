// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <par/prun.hpp>
#include <par/debug_stats_print.hpp>
#include <doctest/doctest.h>
#include <vector>
#include <thread>
#include <iostream>

using ms_t = std::chrono::milliseconds;

constexpr uint32_t test_par = 7;

void test_a(par::thread_pool& pool) {
    static constexpr int64_t N = 100;
    std::vector<int64_t> data;
    data.resize(N, -1);
    par::pfor(pool, {.max_par = test_par}, int64_t(0), N, [&](int64_t i) {
        // simulate equal work
        std::this_thread::sleep_for(ms_t(1));
        data[i] = i;
    });

    for (int64_t i = 0; i < N; ++i) {
        CHECK(data[i] == i);
    }

    data.assign(N, -1);

    par::pfor(pool, {.max_par = test_par}, int64_t(0), N, [&](int64_t i) {
        // simulate unequal work
        std::this_thread::sleep_for(ms_t(1 + (i % 2)));
        data[i] = i;
    });

    for (int64_t i = 0; i < N; ++i) {
        CHECK(data[i] == i);
    }
}

void test_b(par::thread_pool& pool) {
    std::vector<int64_t> data;
    data.resize(test_par, -1);
    par::prun(pool, {.max_par = test_par}, [&](uint32_t i) {
        // simulate equal work
        std::this_thread::sleep_for(ms_t(100));
        data[i] = i;
    });

    for (int64_t i = 0; i < test_par; ++i) {
        CHECK(data[i] == i);
    }

    data.assign(test_par, -1);

    par::prun(pool, {.max_par = test_par}, [&](uint32_t i) {
        // simulate unequal work
        std::this_thread::sleep_for(ms_t(100 + (i % 3) * 100));
        data[i] = i;
    });

    for (int64_t i = 0; i < test_par; ++i) {
        CHECK(data[i] == i);
    }
}

TEST_CASE("integration") {
    par::debug_stats stats;
    {
        par::thread_pool pool("test", 10, &stats);

        // non concurrent
        test_a(pool);
        test_b(pool);

        // concurrent
        std::thread t1([&]() {
            test_a(pool);
        });
        std::thread t2([&]() {
            test_b(pool);
        });

        t1.join();
        t2.join();
    }
    print_debug_stats(std::cout, stats);
}