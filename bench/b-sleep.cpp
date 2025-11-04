// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <itlib/atomic.hpp>
#include <omp.h>
#include <thread>
#include <chrono>

#define PICOBENCH_IMPLEMENT
#include <picobench/picobench.hpp>

static constexpr uint32_t NUM_THREADS = 8;

void sleep() {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(10ms);
}

void bench_par(picobench::state& s) {
    itlib::atomic_relaxed_counter<uintptr_t> cnt(0);

    picobench::scope scope(s);
    par::pfor({.max_par = NUM_THREADS}, 0, s.iterations(), [&](int) {
        sleep();
        ++cnt;
    });

    s.set_result(cnt.load());
}
PICOBENCH(bench_par).label("par");

void openmp(picobench::state& s) {
    itlib::atomic_relaxed_counter<uintptr_t> cnt(0);

    picobench::scope scope(s);
    #pragma omp parallel for num_threads(NUM_THREADS) schedule(dynamic)
    for (int i = 0; i < s.iterations(); ++i) {
        sleep();
        ++cnt;
    }
    s.set_result(cnt.load());
}
PICOBENCH(openmp);

void linear(picobench::state& s) {
    uintptr_t cnt = 0;
    picobench::scope scope(s);
    for (int i = 0; i < s.iterations(); ++i) {
        sleep();
        ++cnt;
    }
    s.set_result(cnt);
}
PICOBENCH(linear);

int main(int argc, char* argv[]) {
    par::thread_pool::init_global(std::min(std::thread::hardware_concurrency(), NUM_THREADS + 2));

    picobench::runner r;
    r.set_default_state_iterations({10, 20});
    r.set_compare_results_across_samples(true);
    r.set_compare_results_across_benchmarks(true);
    r.parse_cmd_line(argc, argv);

    return r.run();
}
