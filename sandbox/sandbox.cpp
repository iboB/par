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
#include <format>
#include <mutex>

//inline int mandelbrot(int x, int y, int size, int max_iter = 1000) {
//    double cx = (x - size / 2.0) * 2.0 / size;
//    double cy = (y - size / 2.0) * 2.0 / size;
//
//    std::complex<double> c(cx, cy);
//    std::complex<double> z = 0;
//    int n = 0;
//    while (std::abs(z) <= 2.0 && n < max_iter) {
//        z = z * z + c;
//        ++n;
//    }
//    return n;
//}

int main() {
    constexpr uint32_t num_jobs = 8;
    par::thread_pool::init_global(std::min(std::thread::hardware_concurrency(), num_jobs - 1));
    [[maybe_unused]] auto ret = par::thread_pool::global().warmup();
    assert(ret == num_jobs);

    const auto size = 20;
    // std::vector<int> output(size * size);

    std::mutex log_mutex;
    std::vector<std::string> log;
    log.reserve(2 * (size * size + size));

    auto doLog = [&](int y, int x = -1) {
        const auto tid = std::this_thread::get_id();
        const auto hash = std::hash<std::thread::id>{}(tid);
        std::string entry;
        if (x >= 0) {
            entry = std::format("{:x} enter y: {}, x: {}", hash, y, x);
        }
        else {
            entry = std::format("{:x} enter y: {}", hash, y);
        }
        {
            std::lock_guard lock(log_mutex);
            log.push_back(std::move(entry));
        }
    };

    par::pfor({.max_par = num_jobs}, 0, size, [&](int y) {
        doLog(y);
        par::pfor({.max_par = num_jobs}, 0, size, [&](int x) {
            doLog(y, x);
            // output[y * size + x] = mandelbrot(x, y, size);
        });
    });

    // std::cout << std::accumulate(output.begin(), output.end(), 0) << std::endl;

    for (auto& e : log) {
        std::cout << e << std::endl;
    }

    return 0;
}
