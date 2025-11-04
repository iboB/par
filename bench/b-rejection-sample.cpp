// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <itlib/atomic.hpp>
#include <omp.h>
#include <random>
#include <thread>

#define PICOBENCH_IMPLEMENT
#include <picobench/picobench.hpp>

static constexpr uint32_t NUM_THREADS = 8;

void bench_par(picobench::state& s) {
    itlib::atomic_relaxed_counter<uintptr_t> accepted(0);

    picobench::scope scope(s);

    struct sampler {
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist;

        sampler(const par::job_info& ji)
            : rng(ji.job_index)
            , dist(-1, 1)
        {}

        float operator()() {
            return dist(rng);
        }
    };

    par::run_opts opts = {.sched = par::schedule_static, .max_par = NUM_THREADS};
    par::pfor<sampler>(opts, 0, s.iterations(), [&](int, sampler& samp) {
        float x = samp();
        float y = samp();
        if (x * x + y * y <= 1.0f) {
            ++accepted;
        }
    });

    s.set_result(accepted.load());
}
PICOBENCH(bench_par).label("par");

void openmp(picobench::state& s) {
    itlib::atomic_relaxed_counter<uintptr_t> accepted(0);

    picobench::scope scope(s);
    #pragma omp parallel num_threads(NUM_THREADS)
    {
        std::mt19937 rng(omp_get_thread_num());
        std::uniform_real_distribution<float> dist(-1, 1);

        #pragma omp for schedule(static)
        for (int i = 0; i < s.iterations(); ++i) {
            float x = dist(rng);
            float y = dist(rng);
            if (x * x + y * y <= 1.0f) {
                ++accepted;
            }
        }
    }
    s.set_result(accepted.load());
}
PICOBENCH(openmp);

int main(int argc, char* argv[]) {
    par::thread_pool::init_global(std::min(std::thread::hardware_concurrency(), NUM_THREADS + 2));

    picobench::runner r;
    r.set_default_state_iterations({10'000, 100'000});
    r.parse_cmd_line(argc, argv);

    return r.run();
}