// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "thread_pool.hpp"
#include "job_info.hpp"
#include "bits/imath.hpp"
#include <splat/inline.h>
#include <atomic>
#include <type_traits>

namespace par {

template <typename I>
struct pfor_range {
    I begin, end;
    I step = 1;
    I iterations_per_job = 1;

    pfor_range<I>& with_step(I s) {
        step = s;
        return *this;
    }
    pfor_range<I>& with_iterations_per_job(I ipj) {
        iterations_per_job = ipj;
        return *this;
    }
};

template <typename I>
pfor_range<I> range(I begin, I end) {
    return {begin, end, 1, 1};
}

template <typename I>
pfor_range<I> range(I size) {
    return {0, size, 1, 1};
}

namespace impl {

template <typename I, typename JobData, typename Func>
FORCE_INLINE void invoke_pfor_func(I index, JobData& data, Func& func) {
    if constexpr (std::is_invocable_v<Func, I, JobData&>) {
        func(index, data);
    }
    else {
        func(index);
    }
}

template <typename JobData>
JobData default_job_data_init([[maybe_unused]] const job_info& info) {
    if constexpr (std::is_constructible_v<JobData, const job_info&>) {
        return JobData{info};
    }
    else {
        return JobData{};
    }
}

template <typename JobData, typename I, typename JobDataInitFunc, typename LoopFunc>
void simple_pfor(
    thread_pool& pool,
    run_opts opts,
    JobDataInitFunc&& init_job_data,
    const I begin, const I end,
    LoopFunc&& func
) {
    if (begin >= end) return; // nothing to do
    using U = std::make_unsigned_t<I>;
    const U size = U(end) - U(begin);

    const auto num_jobs = pool.adjust_par(size, opts);

    if (num_jobs == 1) {
        // only one worker, just call the function and skip the overhead below
        JobData data = init_job_data(job_info{0, 1});
        for (I i = begin; i < end; ++i) {
            invoke_pfor_func(i, data, func);
        }
        return;
    }

    if (opts.sched == schedule_static) {
        auto worker_part = divide_round_up(size, num_jobs);

        auto wfunc = [&](uint32_t ji) {
            JobData data = init_job_data(job_info{ji, uint32_t(num_jobs)});
            const auto wbegin = U(ji * worker_part);
            const auto wend = U(ji + 1) < num_jobs ? wbegin + worker_part : size;
            for (U i = wbegin; i < wend; ++i) {
                invoke_pfor_func(I(U(begin) + i), data, func);
            }
        };

        pool.run_task(opts, thread_pool::task_func(wfunc));
    }
    else {
        std::atomic<U> slot = 0;

        auto wfunc = [&](uint32_t ji) {
            JobData data = init_job_data(job_info{ji, uint32_t(num_jobs)});
            while (true) {
                const U i = slot.fetch_add(1, std::memory_order_relaxed);
                if (i >= size) return; // all done
                invoke_pfor_func(I(U(begin) + i), data, func);
            }
        };

        pool.run_task(opts, thread_pool::task_func(wfunc));
    }
}


template <typename JobData, typename I, typename JobDataInitFunc, typename LoopFunc>
void range_pfor(
    thread_pool& pool,
    run_opts opts,
    JobDataInitFunc&& init_job_data,
    const pfor_range<I>& range,
    LoopFunc&& func
) {
    if (range.iterations_per_job <= 0) {
        return; // nothing to do
    }

    if (range.step == 1 && range.iterations_per_job == 1) {
        // most straightforward case
        simple_pfor<JobData>(
            pool, opts,
            std::forward<JobDataInitFunc>(init_job_data),
            range.begin, range.end,
            std::forward<LoopFunc>(func)
        );
        return;
    }

    using U = std::make_unsigned_t<I>;
    const U begin = U(range.begin);
    const U end = U(range.end);

    const U range_size = [&]() {
        return range.end >= range.begin ? end - begin : begin - end;
    }();

    const U total_iterations = divide_round_up(range_size, U(std::abs(range.step)));
    const U num_chunks = divide_round_up(total_iterations, U(range.iterations_per_job));
    const U chunk_size = U(range.iterations_per_job);

    const auto num_jobs = pool.adjust_par(num_chunks, opts);

    if (num_jobs == 1) {
        // only one worker, just call the function and skip the overhead below
        JobData data = init_job_data(job_info{0, 1});
        I i = range.begin;
        for (U u = 0; u < total_iterations; ++u, i += range.step) {
            invoke_pfor_func(i, data, func);
        }
        return;
    }

    if (range.step == 1) {
        // avoid multiplication in the inner loop
        simple_pfor<JobData>(pool, opts, std::forward<JobDataInitFunc>(init_job_data),
            U(0), num_chunks, [&](U chunk_index, JobData& data) {
                const U chunk_begin = chunk_index * chunk_size;
                const U chunk_end = chunk_index + 1 < num_chunks ? chunk_begin + chunk_size : range_size;
                for (U u = chunk_begin; u < chunk_end; ++u) {
                    invoke_pfor_func(I(begin + u), data, func);
                }
            }
        );
        return;
    }

    simple_pfor<JobData>(pool, opts, std::forward<JobDataInitFunc>(init_job_data),
        U(0), num_chunks, [&](U chunk_index, JobData& data) {
            const U chunk_begin = chunk_index * chunk_size;
            const U chunk_end = (chunk_index + 1) < num_chunks ? chunk_begin + chunk_size : total_iterations;
            I i = I(begin + chunk_begin * U(range.step));
            for (U u = chunk_begin; u < chunk_end; ++u, i += range.step) {
                invoke_pfor_func(i, data, func);
            }
        }
    );
}

} // namespace impl


template <typename JobData = job_info, typename I, typename LoopFunc>
void pfor(thread_pool& pool, run_opts opts, const I begin, const I end, LoopFunc&& func) {
    impl::simple_pfor<JobData>(
        pool, opts,
        impl::default_job_data_init<JobData>,
        begin, end, std::forward<LoopFunc>(func)
    );
}

template <typename JobData = job_info, typename I, typename LoopFunc>
void pfor(run_opts opts, I begin, I end, LoopFunc&& func) {
    pfor<JobData>(thread_pool::global(), opts, begin, end, std::forward<LoopFunc>(func));
}

template <typename I, typename JobDataInitFunc, typename LoopFunc>
void pfor(
    thread_pool& pool,
    run_opts opts,
    JobDataInitFunc&& init_job_data,
    const I begin, const I end,
    LoopFunc&& func
) {
    impl::simple_pfor<decltype(init_job_data(job_info{}))> (
        pool, opts,
        std::forward<JobDataInitFunc>(init_job_data),
        begin, end, std::forward<LoopFunc>(func)
    );
}

template <typename I, typename JobDataInitFunc, typename LoopFunc>
void pfor(
    run_opts opts,
    JobDataInitFunc&& init_job_data,
    const I begin, const I end,
    LoopFunc&& func
) {
    pfor(
        thread_pool::global(), opts,
        std::forward<JobDataInitFunc>(init_job_data),
        begin, end, std::forward<LoopFunc>(func)
    );
}

template <typename JobData = job_info, typename I, typename LoopFunc>
void pfor(thread_pool& pool, run_opts opts, const pfor_range<I>& range, LoopFunc&& func) {
    impl::range_pfor<JobData>(
        pool, opts,
        impl::default_job_data_init<JobData>,
        range, std::forward<LoopFunc>(func)
    );
};

template <typename JobData = job_info, typename I, typename LoopFunc>
void pfor(run_opts opts, const pfor_range<I>& range, LoopFunc&& func) {
    pfor<JobData>(thread_pool::global(), opts, range, std::forward<LoopFunc>(func));
}

template <typename I, typename JobDataInitFunc, typename LoopFunc>
void pfor(
    thread_pool& pool,
    run_opts opts,
    JobDataInitFunc&& init_job_data,
    const pfor_range<I>& range,
    LoopFunc&& func
) {
    impl::range_pfor<decltype(init_job_data(job_info{}))>(
        pool, opts,
        std::forward<JobDataInitFunc>(init_job_data),
        range, std::forward<LoopFunc>(func)
    );
}

template <typename I, typename JobDataInitFunc, typename LoopFunc>
void pfor(
    run_opts opts,
    JobDataInitFunc&& init_job_data,
    const pfor_range<I>& range,
    LoopFunc&& func
) {
    pfor(
        thread_pool::global(), opts,
        std::forward<JobDataInitFunc>(init_job_data),
        range, std::forward<LoopFunc>(func)
    );
}

} // namespace par
