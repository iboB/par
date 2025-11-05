// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include "bu-init.hpp"
#include <par/pfor.hpp>
#include <itlib/atomic.hpp>
#include <omp.h>
#include <random>

#define PICOBENCH_IMPLEMENT
#include <picobench/picobench.hpp>

static constexpr uint32_t NUM_THREADS = 8;

bool is_in_sphere(double x, double y, double z) {
    return x * x + y * y + z * z <= 1;
}

void bench_par(picobench::state& s) {
    itlib::atomic_relaxed_counter<uintptr_t> accepted(0);

    picobench::scope scope(s);

    struct sampler {
        std::mt19937 rng;
        std::uniform_real_distribution<double> dist;

        sampler(const par::job_info& ji)
            : rng(ji.job_index)
            , dist(-1, 1)
        {}

        double operator()() {
            return dist(rng);
        }
    };

    par::run_opts opts = {.sched = par::schedule_static, .max_par = NUM_THREADS};
    par::pfor<sampler>(opts, 0, s.iterations(), [&](int, sampler& samp) {
        if (is_in_sphere(samp(), samp(), samp())) {
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
        std::uniform_real_distribution<double> dist(-1, 1);

        #pragma omp for schedule(static)
        for (int i = 0; i < s.iterations(); ++i) {
            if (is_in_sphere(dist(rng), dist(rng), dist(rng))) {
                ++accepted;
            }
        }
    }
    s.set_result(accepted.load());
}
PICOBENCH(openmp);

void linear(picobench::state& s) {
    uintptr_t accepted = 0;
    picobench::scope scope(s);
    std::mt19937 rng(0);
    std::uniform_real_distribution<double> dist(-1, 1);
    for (int i = 0; i < s.iterations(); ++i) {
        accepted += is_in_sphere(dist(rng), dist(rng), dist(rng));
    }
    s.set_result(accepted);
}
PICOBENCH(linear);

int main(int argc, char* argv[]) {
    init_benchmark(NUM_THREADS);

    picobench::runner r;
    r.set_default_state_iterations({10'000, 100'000});
    r.parse_cmd_line(argc, argv);

    return r.run();
}
