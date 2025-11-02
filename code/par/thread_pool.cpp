// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "thread_pool.hpp"
#include "bits/anchor.hpp"
#include "bits/cpu.hpp"
#include "bits/thread_name.hpp"
#include <itlib/qalgorithm.hpp>
#include <vector>
#include <atomic>
#include <latch>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <stdexcept>
#include <string>
#include <cassert>
#include <deque>
#include <optional>

#include <splat/warnings.h>
DISABLE_MSVC_WARNING(4324)

#if !defined(PAR_DEBUG_STATS)
#define PAR_DEBUG_STATS 1
#endif

#if PAR_DEBUG_STATS
#include "debug_stats.hpp"
#include <chrono>
using high_res_clock = std::chrono::high_resolution_clock;
#endif

namespace par {

namespace {

thread_local thread_pool::impl* current_pool = nullptr;

constexpr uint32_t max_threads = 128;

struct worker_task {
    uint32_t index;
    task_func func;
    std::latch* latch = nullptr; // have nullptr here when stopping

    void operator()() {
        func(index);
        latch->count_down();
    }
};

struct pending_dynamic_task {
    // index of last executed item (assigned is ignored)
    uint32_t index = 0;

    uint32_t size = 0; // nthreads, basically

    task_func func;

    std::latch& latch;

    pending_dynamic_task(uint32_t i, uint32_t n, const task_func& f, std::latch& l)
        : index(i)
        , size(n)
        , func(f)
        , latch(l)
    {}

    bool done() const {
        return index == size;
    }

    worker_task get_next_worker_task() {
        assert(index < size);
        worker_task wt;
        wt.index = ++index;
        wt.func = func;
        wt.latch = &latch;
        return wt;
    }
};

} // namespace

struct thread_pool::impl {
    std::string m_name;

    #if PAR_DEBUG_STATS
    debug_stats m_own_debug_stats;
    debug_stats& m_debug_stats;
    debug_stats::worker_stats& m_caller_stats;
    #endif

    std::atomic_flag m_have_dynamic_tasks = ATOMIC_FLAG_INIT;

    std::mutex m_task_mutex;
    std::deque<pending_dynamic_task> m_pending_dynamic_tasks;

    std::optional<worker_task> get_pending_dynamic_task() {
        if (!m_have_dynamic_tasks.test(std::memory_order_acquire)) {
            return std::nullopt;
        }

        std::lock_guard lock(m_task_mutex);

        while (true) {
            if (m_pending_dynamic_tasks.empty()) {
                m_have_dynamic_tasks.clear(std::memory_order_release);
                return std::nullopt;
            }

            auto& front = m_pending_dynamic_tasks.front();

            if (!front.done()) {
                return front.get_next_worker_task();
            }

            m_pending_dynamic_tasks.pop_front();
        }
    }

    #if PAR_DEBUG_STATS
    struct worker;
    static thread_local worker* current_worker;
    #endif

    struct alignas(cpu::alignment_to_avoid_false_sharing) worker {
        uint32_t m_index;
        impl& m_pool;

        #if PAR_DEBUG_STATS
        debug_stats::worker_stats& m_debug_stats;
        #endif

        std::thread m_thread;

        std::mutex m_mutex;
        std::condition_variable m_cv;

        std::vector<worker_task> m_pending_tasks;
        std::vector<worker_task> m_executing_tasks;

        std::atomic_flag m_busy = ATOMIC_FLAG_INIT;

        explicit worker(uint32_t i, impl& pool
            #if PAR_DEBUG_STATS
            , debug_stats::worker_stats& ds
            #endif
        )
            : m_index(i)
            , m_pool(pool)
            #if PAR_DEBUG_STATS
            , m_debug_stats(ds)
            #endif
        {
            m_thread = std::thread(&worker::run, this);
        }

        ~worker() {
            m_thread.join();
        }

        worker(const worker&) = delete;
        worker& operator=(const worker&) = delete;

        // should never be called
        worker(worker&&) noexcept = delete;
        worker& operator=(worker&&) noexcept = delete;

        // indscriminately add tasks guaranteed to be executed by this worker
        void add_task(worker_task task) {
            {
                std::unique_lock lock(m_mutex);
                m_busy.test_and_set(std::memory_order_acquire);
                m_pending_tasks.push_back(task);
            }
            m_cv.notify_one();
        }

        // try to add a task to this worker, return false if the worker is busy
        bool try_add_task(worker_task task) {
            if (m_busy.test(std::memory_order_acquire)) {
                return false;
            }
            {
                std::unique_lock lock(m_mutex);
                if (m_busy.test_and_set(std::memory_order_acquire)) {
                    return false;
                }
                m_pending_tasks.push_back(task);
            }
            m_cv.notify_one();
            return true;
        }

        bool try_wake_up_if_idle() {
            if (m_busy.test_and_set(std::memory_order_acquire)) {
                return false;
            }
            m_cv.notify_one();
            return true;
        }

        void run() {
            current_pool = &m_pool;
            #if PAR_DEBUG_STATS
            impl::current_worker = this;
            #endif
            {
                std::string name = m_pool.m_name + '-' + std::to_string(m_index);
                this_thread::set_name(name);
            }

            while (true) {
                std::unique_lock lock(m_mutex);
                while (true) {
                    if (!m_pending_tasks.empty()) {
                        std::swap(m_pending_tasks, m_executing_tasks);
                        lock.unlock();
                        break;
                    }
                    if (auto t = m_pool.get_pending_dynamic_task()) {
                        m_busy.test_and_set(std::memory_order_acquire);
                        m_executing_tasks.push_back(*t);
                        lock.unlock();
                        #if PAR_DEBUG_STATS
                        ++m_debug_stats.num_tasks_stolen;
                        #endif
                        break; // go check for dynamic tasks
                    }
                    m_busy.clear(std::memory_order_release);

                    m_cv.wait(lock);
                }
                #if PAR_DEBUG_STATS
                auto start = high_res_clock::now();
                #endif
                for (auto& task : m_executing_tasks) {
                    if (!task.latch) {
                        // stopping
                        return;
                    }
                    task();
                    #if PAR_DEBUG_STATS
                    ++m_debug_stats.num_tasks_executed;
                    #endif
                }
                #if PAR_DEBUG_STATS
                auto time = high_res_clock::now() - start;
                m_debug_stats.total_task_time_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
                #endif
                m_executing_tasks.clear();
            }
        }
    };

    std::vector<anchor<worker>> m_workers;

    impl(std::string name, uint32_t nthreads, [[maybe_unused]] debug_stats* ds)
        : m_name(std::move(name))
        #if PAR_DEBUG_STATS
        , m_own_debug_stats()
        , m_debug_stats(ds ? *ds : m_own_debug_stats)
        , m_caller_stats(m_debug_stats.caller_stats)
        #endif
    {
        if (nthreads >= max_threads) {
            throw std::runtime_error("par::thread_pool supports up to " + std::to_string(max_threads - 1) + " threads");
        }

        #if PAR_DEBUG_STATS
        m_debug_stats.pool_name = m_name;
        m_debug_stats.per_worker.resize(nthreads);
        m_debug_stats.total_lifetime_ns = high_res_clock::now().time_since_epoch().count();
        #endif

        m_workers.reserve(nthreads);
        for (uint32_t i = 0; i < nthreads; ++i) {
            m_workers.emplace_back(i, *this
                #if PAR_DEBUG_STATS
                , *m_debug_stats.per_worker[i]
                #endif
            );
        }
    }

    ~impl() {
        for (auto& worker : m_workers) {
            // notify workers to stop
            worker->add_task({});
        }
        #if PAR_DEBUG_STATS
        auto lifetime = high_res_clock::now().time_since_epoch().count() - m_debug_stats.total_lifetime_ns;
        m_debug_stats.total_lifetime_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            high_res_clock::duration(lifetime)
        ).count();
        #endif
    }

    bool current_thread_is_worker() const {
        return current_pool == this;
    }

    uint32_t get_par(const run_opts& opts) const {
        const auto num_workers = uint32_t(m_workers.size());
        if (num_workers == 0) return 1; // no workers, only caller thread

        if (current_thread_is_worker()) {
            switch (opts.sched) {
            // allow nesting, but don't oversubscribe
            case schedule_dynamic: return 1 + std::min(opts.max_par - 1, uint32_t(m_workers.size() - 1));

            // no extra workers
            case schedule_dynamic_no_nesting: return 1;

            // no nesting;
            case schedule_static: [[fallthrough]];
            default:
                return 0;
            }
        }
        else {
            return 1 + std::min(opts.max_par - 1, uint32_t(m_workers.size()));
        }
    }

    uint32_t run_task(const run_opts& opts, task_func func) {
        auto num_worker_jobs = get_par(opts);

        if (num_worker_jobs == 1) {
            // only run in the caller thread
            func(0);
            return 1;
        }

        #if PAR_DEBUG_STATS
        debug_stats::worker_stats& dstats = current_thread_is_worker() ? current_worker->m_debug_stats : m_caller_stats;
        #endif

        if (num_worker_jobs == 0) {
            throw std::runtime_error("unsupported nested par call");
        }

        // the caller will do at least one unit of work, so exclude it
        --num_worker_jobs;

        std::latch latch(num_worker_jobs);

        bool task_added_to_dynamic_tasks = false;
        if (opts.sched == schedule_static) {
            // static scheduling, no work stealing
            // just add task to corresponding workers
            for (uint32_t i = 0; i < num_worker_jobs; ++i) {
                m_workers[i]->add_task({i + 1, func, &latch});
            }
        }
        else {
            uint32_t index = 0;
            for (auto& w : m_workers) {
                if (w->try_add_task({index + 1, func, &latch})) {
                    ++index;
                    if (index == num_worker_jobs) {
                        break;
                    }
                }
            }
            if (index < num_worker_jobs) {
                task_added_to_dynamic_tasks = true;
                m_have_dynamic_tasks.test_and_set(std::memory_order_acquire);
                {
                    // not enough idle workers, add the rest to the pending dynamic tasks
                    std::lock_guard lock(m_task_mutex);
                    m_pending_dynamic_tasks.emplace_back(index, num_worker_jobs, func, latch);
                }
                for (auto& w : m_workers) {
                    // try to wake up workers which have gone idle while we were adding the pending task
                    if (w->try_wake_up_if_idle()) {
                        ++index;
                        if (index == num_worker_jobs) {
                            // we woke up enough workers to do the remote task
                            break;
                        }
                    }
                }
            }
        }

        func(0);
        #if PAR_DEBUG_STATS
        ++dstats.num_tasks_executed;
        #endif

        if (opts.sched != schedule_static && task_added_to_dynamic_tasks) {
            // try stealing work while possible
            while (true) {
                worker_task task;

                {
                    std::lock_guard lock(m_task_mutex);

                    // find our task so that the caller only works on its own task
                    auto f = itlib::pfind_if(m_pending_dynamic_tasks, [&](const pending_dynamic_task& t) {
                        return &t.latch == &latch;
                    });
                    if (!f || f->done()) break; // no more work to steal
                    task = f->get_next_worker_task();

                    // note that we don't remove the task from m_pending_dynamic_tasks here
                    // we leave this job to the workers
                }

                task();
                #if PAR_DEBUG_STATS
                ++dstats.num_tasks_stolen;
                ++dstats.num_tasks_executed;
                #endif
            }
        }

        latch.wait(); // wait for all tasks to finish
        return num_worker_jobs + 1;
    }
};

#if PAR_DEBUG_STATS
thread_local thread_pool::impl::worker* thread_pool::impl::current_worker = nullptr;
#endif

thread_pool::thread_pool(std::string name, uint32_t nthreads, debug_stats* ds)
    : m_impl(std::make_unique<impl>(std::move(name), nthreads, ds))
{}

thread_pool::~thread_pool() = default;

bool thread_pool::current_thread_is_worker() const {
    return m_impl->current_thread_is_worker();
}

uint32_t thread_pool::run_task(run_opts opts, task_func task) {
    return m_impl->run_task(opts, std::move(task));
}

uint32_t thread_pool::num_threads() const {
    return uint32_t(m_impl->m_workers.size());
}

uint32_t thread_pool::get_par(run_opts opts) const {
    return m_impl->get_par(opts);
}

// global thread pool management

namespace {

std::atomic_flag global_initialized = ATOMIC_FLAG_INIT;

std::unique_ptr<thread_pool> global_thread_pool;

thread_pool& do_init_global(uint32_t nthreads) {
    global_thread_pool = std::make_unique<thread_pool>("gpar", nthreads);
    return *global_thread_pool;
}

} // namespace

thread_pool& thread_pool::global() {
    if (!global_initialized.test_and_set(std::memory_order_acquire)) {
        constexpr uint32_t other_threads = 2;
        const auto hwc = std::thread::hardware_concurrency();
        if (hwc <= other_threads) {
            // no wokers, only caller thread
            return do_init_global(0);
        }
        else {
            return do_init_global(hwc - other_threads);
        }
    }
    return *global_thread_pool;
}

thread_pool& thread_pool::init_global(uint32_t nthreads) {
    if (global_initialized.test_and_set(std::memory_order_acquire)) {
        throw std::runtime_error("global par::thread_pool already initialized");
    }
    return do_init_global(nthreads);
}

} // namespace par
