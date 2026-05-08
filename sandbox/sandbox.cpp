// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <iostream>

int main() {
    std::atomic<int64_t> sum = 0;
    par::pfor({.sched = par::schedule_static, .max_par = 4}, int16_t(0), int16_t(0x7FFF), [&](uint16_t i) {
        sum += i;
    });

    std::cout << sum.load() << std::endl;
    return 0;
}
