// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "bits/anchor.hpp"
#include <itlib/atomic.hpp>
#include <vector>
#include <atomic>
#include <string>

#include <splat/warnings.h>
PRAGMA_WARNING_PUSH
DISABLE_MSVC_WARNING(4324)

namespace par {

struct debug_stats {
    std::string pool_name;

    uint64_t total_lifetime_ns = 0;

    struct alignas(64) worker_stats {
        itlib::atomic_relaxed_counter<uint64_t> num_tasks_executed;
        itlib::atomic_relaxed_counter<uint64_t> num_tasks_stolen;
        itlib::atomic_relaxed_counter<uint64_t> total_task_time_ns;
    };

    worker_stats caller_stats;
    std::vector<anchor<worker_stats>> per_worker;
};

} // namespace par

PRAGMA_WARNING_POP
