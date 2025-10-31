// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/bits/anchor.hpp>
#include <doctest/doctest.h>
#include <mutex>
#include <vector>
#include <type_traits>

TEST_CASE("mutex") {
    static_assert(!std::is_move_constructible_v<std::mutex>);

    std::vector<par::anchor<std::mutex>> vec;
    vec.reserve(10);
    CHECK(vec.capacity() >= 10);
    for (size_t i = 0; i < vec.capacity(); ++i) {
        vec.emplace_back();
    }

    for (auto& m : vec) {
        std::lock_guard lock(*m);
    }

    for (auto& m : vec) {
        m->lock();
        m->unlock();
    }

    // check for a weird std::vector implementation
    // that would allocate space for 1000 elements when reserving for 10
    REQUIRE(vec.capacity() < 1000);

    CHECK_THROWS(vec.reserve(1000));
}
