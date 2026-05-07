// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <par/prun.hpp>
#include <omp.h>
#include <atomic>
#include <thread>
#include <cassert>

inline void init_benchmark(uint32_t num_jobs) {
    par::thread_pool::init_global(std::min(std::thread::hardware_concurrency(), num_jobs - 1));

    num_jobs = par::thread_pool::global().num_threads() + 1;

    std::atomic_uint32_t counter{0};

    // warm up par
    [[maybe_unused]] auto ret = par::thread_pool::global().warmup();

    // warm up OpenMP
    #pragma omp parallel for num_threads(num_jobs) schedule(static)
    for (int i = 0; i < int(num_jobs); ++i) {
        ++counter;
    }

    // sanity
    assert(ret == counter);
}
