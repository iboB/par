// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "thread_pool.hpp"
#include "job_info.hpp"
#include <concepts>

namespace par {

template <std::invocable<uint32_t> TaskFunc>
uint32_t prun(thread_pool& pool, run_opts opts, TaskFunc&& func) {
    if (opts.max_par == 1) {
        // only one worker, just call the function and skip the overhead below
        func(0);
        return 1;
    }
    return pool.run_task(opts, thread_pool::task_func(func));
}

template <std::invocable<const job_info&> TaskFunc>
uint32_t prun(thread_pool& pool, run_opts opts, TaskFunc&& func) {
    const auto par = pool.get_par(opts);
    if (par == 1) {
        // only one worker, just call the function and skip the overhead below
        func(job_info{0, 1});
        return 1;
    }
    auto wfunc = [&](uint32_t i) {
        job_info arg{i, par};
        func(arg);
    };
    return pool.run_task(opts, thread_pool::task_func(wfunc));
}

template <typename TaskFunc>
uint32_t prun(run_opts opts, TaskFunc&& func) {
    return prun(thread_pool::global(), opts, std::forward<TaskFunc>(func));
}

} // namespace par
