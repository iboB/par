// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <cstdint>

namespace par {

// this is not an enum class because `static` is a keyword and not usable as a symbol
enum schedule : uint32_t {
    // dynamic scheduling with work stealing, allows nested parallelism
    schedule_dynamic,

    // dynamic scheduling with work stealing, prevent nested parallelism
    // when no nesting is involved it's the same as schedule_dynamic
    // when used on a nested call, it will run on the caller thread only
    schedule_dynamic_no_nesting,

    // static scheduling, no work stealing, disallows nested parallelism
    // throw an exception when used on a nested call as nesting can cause deadlocks
    schedule_static,

    // REMOVED as it's was deemed not practical
    // dynamic scheduling with work stealing, allow nested parallelism
    // and also execute other jobs while waiting
    // schedule_dynamic_steal,

    // UNIMPLEMENTED, but could be
    // only execute parallel work
    // when the the caller thread is done, prevent any workers from starting this task, and only wait for the ones
    // that already started it
    // thus the number of threads which run the task depends on the current load of the pool,
    // it could only be one thread (the caller)
    // schedule_only_parallel,
};

struct run_opts {
    schedule sched = schedule_dynamic;

    // max number of task instances to spawn
    // ALWAYS clamped to the number of workers in the pool + 1 (the caller thread)
    // otherwise its the exact number of task instances
    // if you want exactly N instances of the task, use pfor or pchunk
    // 0 means use all available workers
    // 1 means only use the caller thread
    // dynamic: the task instances may eventually run on a single thread or few threads when there's other work,
    // static: each task instance will run on a separate thread (again, clamped to the number of workers + 1)
    uint32_t max_par = 0;
};

// optionally use this as an argument to make it explicit that default options are used
inline constexpr run_opts default_run_opts{};

} // namespace par