// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/pfor.hpp>
#include <iostream>
#include <utility>
#include <format>

int main() {
    const unsigned p = 4;
    const unsigned s = 10;

    par::balanced_chunks chunk(s, p);

    for (int i=0; i<p; ++i) {
        auto [lo, hi] = chunk(0, i);
        std::cout << std::format("{}-{}\n", lo, hi);
    }

    return 0;
}
