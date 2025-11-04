// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <itlib/atomic.hpp>
#include <omp.h>
#include <complex>
#include <thread>
#include <numeric>

#define PICOBENCH_IMPLEMENT
#include <picobench/picobench.hpp>

static constexpr uint32_t NUM_THREADS = 8;

int mandelbrot(double cx, double cy, int max_iter) {
    std::complex<double> c(cx, cy);
    std::complex<double> z = 0;
    int n = 0;
    while (std::abs(z) <= 2.0 && n < max_iter) {
        z = z * z + c;
        ++n;
    }
    return n;
}

void par_nest(picobench::state& s) {
    const auto size = s.iterations();
    std::vector<int> output(size * size);
    {
        picobench::scope scope(s);
        par::pfor({.max_par = NUM_THREADS}, 0, size, [&](int y) {
            par::pfor({.max_par = NUM_THREADS}, 0, size, [&](int x) {
                double cx = (x - size / 2.0) * 4.0 / size;
                double cy = (y - size / 2.0) * 4.0 / size;
                output[y * size + x] = mandelbrot(cx, cy, 1000);
            });
        });
    }
    s.set_result(std::accumulate(output.begin(), output.end(), 0));
}
PICOBENCH(par_nest);

void par_auto_collapse(picobench::state& s) {
    const auto size = s.iterations();
    std::vector<int> output(size * size);
    {
        picobench::scope scope(s);
        par::pfor({.max_par = NUM_THREADS}, 0, size * size, [&](int i) {
            auto x = i % size;
            auto y = i / size;
            double cx = (x - size / 2.0) * 4.0 / size;
            double cy = (y - size / 2.0) * 4.0 / size;
            output[y * size + x] = mandelbrot(cx, cy, 1000);
        });
    }
    s.set_result(std::accumulate(output.begin(), output.end(), 0));
}
PICOBENCH(par_auto_collapse);

void openmp(picobench::state& s) {
    const auto size = s.iterations();
    std::vector<int> output(size * size);
    {
        picobench::scope scope(s);
        #pragma omp parallel for num_threads(NUM_THREADS) schedule(dynamic) collapse(2)
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                double cx = (x - size / 2.0) * 4.0 / size;
                double cy = (y - size / 2.0) * 4.0 / size;
                output[y * size + x] = mandelbrot(cx, cy, 1000);
            }
        }
    }
    s.set_result(std::accumulate(output.begin(), output.end(), 0));
}
PICOBENCH(openmp);

int main(int argc, char* argv[]) {
    par::thread_pool::init_global(std::min(std::thread::hardware_concurrency(), NUM_THREADS + 2));

    picobench::runner r;
    r.set_compare_results_across_samples(true);
    r.set_compare_results_across_benchmarks(true);
    r.set_default_state_iterations({20, 80});
    r.parse_cmd_line(argc, argv);

    return r.run();
}