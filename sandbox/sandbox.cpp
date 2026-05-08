// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <thread>
#include <iostream>
#include <algorithm>
#include <functional>
#include <numeric>
#include <cassert>
#include <vector>
#include <complex>

inline int mandelbrot(int x, int y, int size, int max_iter = 1000) {
    double cx = (x - size / 2.0) * 2.0 / size;
    double cy = (y - size / 2.0) * 2.0 / size;

    std::complex<double> c(cx, cy);
    std::complex<double> z = 0;
    int n = 0;
    while (std::abs(z) <= 2.0 && n < max_iter) {
        z = z * z + c;
        ++n;
    }
    return n;
}

int main() {
    constexpr uint32_t num_jobs = 8;
    par::thread_pool::init_global(std::min(std::thread::hardware_concurrency(), num_jobs - 1));
    [[maybe_unused]] auto ret = par::thread_pool::global().warmup();
    assert(ret == num_jobs);

    const auto size = 20;
    std::vector<int> output(size * size);

    par::pfor({.max_par = 5}, 0, size, [&](int y) {
        par::pfor({.max_par = 5}, 0, size, [&](int x) {
            output[y * size + x] = mandelbrot(x, y, size);
        });
    });

    std::cout << std::accumulate(output.begin(), output.end(), 0) << std::endl;

    return 0;
}
