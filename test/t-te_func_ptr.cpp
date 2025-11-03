// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/bits/te_func_ptr.hpp>
#include <doctest/doctest.h>

TEST_CASE("empty") {
    par::te_func_ptr<void()> f;
    CHECK(!f);
}

TEST_CASE("lambda") {
    int x = 0;
    auto lambda = [&](int v) { x += v; };
    auto lambda2 = [&](int v) { x = v * 2; };
    par::te_func_ptr<void(int)> lptr(lambda);
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

TEST_CASE("function pointer") {
    auto func = +[](int v) { return v * 3; };
    par::te_func_ptr<int(int)> fptr(func);
    CHECK(!!fptr);
    int r = fptr(4);
    CHECK(r == 12);
    fptr.reset();
    CHECK(!fptr);
    fptr.reset(func);
    CHECK(!!fptr);
    r = fptr(5);
    CHECK(r == 15);
}
