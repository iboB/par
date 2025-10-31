// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "thread_pool.hpp"
#include <concepts>

namespace par {

template <std::invocable<uint32_t> TaskFunc>
uint32_t prun(thread_pool& pool, run_opts opts, TaskFunc&& func) {
    if (opts.max_par == 1) {
        // only one worker, just call the function and skip the overhead below
        func(0);
        return 1;
    }
    return pool.run_task(opts, task_func_ptr<uint32_t>(func));
}

template <std::invocable<uint32_t, uint32_t> TaskFunc>
uint32_t prun(thread_pool& pool, run_opts opts, TaskFunc&& func) {
    const auto par = pool.get_par(opts);
    if (par == 1) {
        // only one worker, just call the function and skip the overhead below
        func(0, 1);
        return 1;
    }
    auto wfunc = [&](uint32_t i) {
        func(i, par);
    };
    return pool.run_task(opts, task_func_ptr<uint32_t>(wfunc));
}

template <typename TaskFunc>
uint32_t prun(run_opts opts, TaskFunc&& func) {
    return prun(thread_pool::global(), opts, std::forward<TaskFunc>(func));
}

} // namespace par
