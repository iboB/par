// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "thread_pool.hpp"
#include "job_info.hpp"
#include <splat/inline.h>
#include <type_traits>

namespace par {

namespace impl {
template <typename I, typename Func>
FORCE_INLINE void invoke_pchunk_func(I begin, I end, [[maybe_unused]] const job_info& ji, Func& func) {
    if constexpr (std::is_invocable_v<Func, I, I, const job_info&>) {
        func(begin, end, ji);
    }
    else {
        func(begin, end);
    }
}
} // namespace impl

template <typename I, typename Func>
uint32_t pchunk(thread_pool& pool, run_opts opts, const I size, Func&& func) {
    if (size == 0) return 0; // nothing to do
    const auto num_chunks = pool.adjust_par(size, opts);

    if (num_chunks == 1) {
        // only one chunk, just call the function and skip the overhead below
        impl::invoke_pchunk_func(I(0), size, job_info{0, 1}, func);
        return 1;
    }

    const auto chunk_size = (size + num_chunks - 1) / num_chunks;
    auto wfunc = [&](uint32_t ci) {
        const auto begin = I(ci * chunk_size);
        const auto end = I(ci + 1) < num_chunks ? begin + chunk_size : size;
        job_info ji{ci, uint32_t(num_chunks)};
        impl::invoke_pchunk_func(begin, end, ji, func);
    };
    return pool.run_task(opts, task_func(wfunc));
}

template <typename I, typename Func>
uint32_t pchunk(run_opts opts, const I size, Func&& func) {
    return pchunk(thread_pool::global(), opts, size, std::forward<Func>(func));
}

} // namespace par
