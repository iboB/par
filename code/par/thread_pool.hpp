// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "api.h"
#include "run_opts.hpp"
#include "task_func_ptr.hpp"
#include <memory>
#include <cstdint>
#include <string>
#include <type_traits>

namespace par {

struct debug_stats;

// for non-movable types that you want in a std::vector

class PAR_API thread_pool {
public:
    static thread_pool& global();
    static thread_pool& init_global(uint32_t nthreads);

    // debug stats are conditionally compiled in
    // if PAR_DEBUG_STATS is not defined to a truthy value, this parameter is ignored
    // if debug stats are available, the data is only reliable after the thread_pool is destroyed
    thread_pool(std::string name, uint32_t nthreads, debug_stats* ds = nullptr);
    ~thread_pool();

    // utility function to check if debug stats are available
    bool have_debug_stats() const;

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    const std::string& name() const;

    // return the number of threads used to run the task, including the caller thread
    uint32_t run_task(run_opts opts, task_func_ptr<uint32_t> task);
    uint32_t run_task(task_func_ptr<uint32_t> task, run_opts opts = {}) {
        return run_task(opts, std::move(task));
    }


    // note that this does not include the caller thread
    uint32_t num_threads() const;

    uint32_t max_parallel_jobs() const {
        return num_threads() + 1;
    }

    // get the actual number of threads that will be used to run a task with the given options
    // from the point of view of the caller thread
    uint32_t get_par(run_opts opts = {}) const;

    template <typename I>
    I adjust_par(I size, run_opts& opts) const {
        static_assert(std::is_integral_v<I>, "I must be an integral type");
        if constexpr (std::is_signed_v<I>) {
            if (size <= 0) return 0;
        }
        auto opar = get_par(opts);

        // avoid overflow when converting between different integer types
        if constexpr (sizeof(I) <= sizeof(uint32_t)) {
            auto ret = std::min(uint32_t(size), opar);
            opts.max_par = ret;
            return I(ret);
        }
        else {
            auto ret = std::min(size, I(opar));
            opts.max_par = uint32_t(ret);
            return ret;
        }
    }

    template <typename I>
    I get_par(I size, run_opts opts = {}) const {
        return adjust_par(size, opts);
    }


    // check if the current thread is one of the worker threads of this pool
    bool current_thread_is_worker() const;

    struct impl;
private:
    std::unique_ptr<impl> m_impl;
};


} // namespace par
