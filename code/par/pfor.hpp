// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "thread_pool.hpp"
#include "bits/imath.hpp"
#include <atomic>
#include <type_traits>

namespace par {

template <typename I, typename LoopFunc>
void pfor(thread_pool& pool, run_opts opts, const I begin, const I end, LoopFunc&& func) {
    if (begin >= end) return; // nothing to do
    using U = std::make_unsigned_t<I>;
    const U size = U(end) - U(begin);

    const auto num_jobs = pool.adjust_par(size, opts);

    if (num_jobs == 1) {
        // only one worker, just call the function and skip the overhead below
        for (I i = begin; i < end; ++i) {
            func(i);
        }
        return;
    }

    if (opts.sched == schedule_static) {
        auto worker_part = divide_round_up(size, num_jobs);

        auto wfunc = [&](uint32_t ji) {
            const auto wbegin = U(ji * worker_part);
            const auto wend = U(ji + 1) < num_jobs ? begin + worker_part : size;
            for (U i = wbegin; i < wend; ++i) {
                func(I(U(begin) + i));
            }
        };

        pool.run_task(opts, task_func(wfunc));
    }
    else {
        std::atomic<U> slot = 0;

        auto wfunc = [&](uint32_t) {
            while (true) {
                const U i = slot.fetch_add(1, std::memory_order_relaxed);
                if (i >= size) return; // all done
                func(I(U(begin) + i));
            }
        };

        pool.run_task(opts, task_func(wfunc));
    }
}

template <typename I, typename LoopFunc>
void pfor(run_opts opts, I begin, I end, LoopFunc&& func) {
    pfor(thread_pool::global(), opts, begin, end, std::forward<LoopFunc>(func));
}

template <typename I>
struct pfor_def {
    I begin, end;
    I step = 1;
    I iterations_per_job = 1;
};

template <typename I, typename LoopFunc>
void pfor(thread_pool& pool, run_opts opts, const pfor_def<I>& def, LoopFunc&& func) {
    if (def.step == 1 && def.iterations_per_job == 1) {
        // most straightforward case
        pfor(pool, opts, def.begin, def.end, std::forward<LoopFunc>(func));
        return;
    }

    using U = std::make_unsigned_t<I>;
    const U begin = U(def.begin);
    const U end = U(def.end);

    const U range_size = [&]() {
        def.end >= def.begin ? end - begin : begin - end;
    }();

    const U total_iterations = range_size / std::abs(def.step);
    const U num_chunks = divide_round_up(total_iterations, U(def.iterations_per_job));
    const U chunk_size = U(def.iterations_per_job);

    const auto num_jobs = pool.adjust_par(num_chunks, opts);

    if (num_jobs == 1) {
        // only one worker, just call the function and skip the overhead below
        I i = def.begin;
        for (U u = 0; u < total_iterations; ++u, i += def.step) {
            func(i);
        }
        return;
    }

    if (def.step == 1) {
        // avoid multiplication in the inner loop
        pfor(pool, opts, I(0), num_chunks, [&](U ichunk) {
            const U chunk_begin = ichunk * chunk_size;
            const U chunk_end = ichunk + 1 < num_chunks ? begin + chunk_size : range_size;

            for (U u = chunk_begin; u < chunk_end; ++u) {
                func(I(begin + u));
            }
        });
        return;
    }

    pfor(pool, opts, I(0), num_chunks, [&](U chunk_index) {
        const U chunk_begin = chunk_index * chunk_size;
        const U chunk_end = (chunk_index + 1) < num_chunks ? chunk_begin + chunk_size : total_iterations;
        I i = I(begin + chunk_begin * U(def.step));
        for (U u = chunk_begin; u < chunk_end; ++u, i += def.step) {
            func(i);
        }
    });
}

} // namespace par
