// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <par/prun.hpp>
#include <omp.h>
#include <atomic>
#include <thread>

inline void init_benchmark(uint32_t num_threads) {
    par::thread_pool::init_global(std::min(std::thread::hardware_concurrency(), num_threads));

    num_threads = par::thread_pool::global().num_threads();

    std::atomic_uint32_t counter{0};

    // warm up par
    par::prun({.sched = par::schedule_static}, [&](uint32_t) {
        ++counter;
    });

    // warm up OpenMP
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int i = 0; i < int(num_threads); ++i) {
        ++counter;
    }
}
