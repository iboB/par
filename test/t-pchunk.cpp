// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pchunk.hpp>
#include <doctest/doctest.h>
#include <mutex>
#include <vector>
#include <utility>
#include <algorithm>

using range_vec = std::vector<std::pair<int, int>>;

TEST_CASE("pchunk") {
    static constexpr uint32_t num_threads = 4;
    par::thread_pool pool("test", num_threads);

    auto run_test = [&](int size, range_vec expected, uint32_t max_par = 0) {
        std::mutex mtx;
        range_vec ranges;

        auto ret = par::pchunk(pool, {.max_par = max_par}, size, [&](int begin, int end) {
            std::lock_guard lock(mtx);
            ranges.emplace_back(begin, end);
        });

        CHECK(ret == uint32_t(ranges.size()));

        std::sort(ranges.begin(), ranges.end());
        CHECK(ranges == expected);
    };

    run_test(0, {});
    run_test(1, {{0, 1}});
    run_test(2, {{0, 1}, {1, 2}});
    run_test(10, {{0, 2}, {2, 4}, {4, 6}, {6, 8}, {8, 10}});
    run_test(10, {{0, 3}, {3, 6}, {6, 8}, {8, 10}}, 4);
    run_test(10, {{0, 10}}, 1);
    run_test(10, {{0, 5}, {5, 10}}, 2);
    run_test(10, {{0, 4}, {4, 7}, {7, 10}}, 3);
    run_test(97, {{0, 20}, {20, 40}, {40, 59}, {59, 78}, {78, 97}});
    run_test(97, {{0, 20}, {20, 40}, {40, 59}, {59, 78}, {78, 97}}, 1000);
}

TEST_CASE("pchunk with job info") {
    static constexpr uint32_t num_threads = 4;
    par::thread_pool pool("test", num_threads);

    std::mutex mtx;
    range_vec ranges;
    std::vector<uint32_t> job_indices;
    auto ret = par::pchunk(pool, {}, 23, [&](int begin, int end, par::job_info ji) {
        CHECK(ji.num_jobs == 5);
        std::lock_guard lock(mtx);
        ranges.emplace_back(begin, end);
        job_indices.push_back(ji.job_index);
    });

    CHECK(ret == 5);
    std::sort(ranges.begin(), ranges.end());
    std::sort(job_indices.begin(), job_indices.end());
    CHECK(ranges == range_vec{{0, 5}, {5, 10}, {10, 15}, {15, 19}, {19, 23}});
    CHECK(job_indices == std::vector<uint32_t>{0, 1, 2, 3, 4});
}
