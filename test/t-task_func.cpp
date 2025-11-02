// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/task_func.hpp>
#include <doctest/doctest.h>

TEST_CASE("empty") {
    par::task_func f;
    CHECK(!f);
}

TEST_CASE("lambda") {
    uint32_t x = 0;
    auto lambda = [&](uint32_t v) { x += v; };
    auto lambda2 = [&](uint32_t v) { x = v * 2; };
    par::task_func lptr(lambda);
    CHECK(!!lptr);
    lptr(3);
    CHECK(x == 3);
    lptr(4);
    CHECK(x == 7);

    lptr.reset();
    CHECK(!lptr);

    lptr.reset(lambda2);
    CHECK(!!lptr);
    lptr(5);
    CHECK(x == 10);
}
