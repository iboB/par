// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/task_func_ptr.hpp>
#include <doctest/doctest.h>

TEST_CASE("empty") {
    par::task_func_ptr<> f;
    CHECK(!f);
}

TEST_CASE("lambda") {
    int x = 0;
    auto lambda = [&](int v) { x += v; };
    auto lambda2 = [&](int v) { x = v * 2; };
    par::task_func_ptr<int> lptr(lambda);
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
    auto func = +[](int& r, int v) { r = v * 3; };
    par::task_func_ptr<int&, int> fptr(func);
    CHECK(!!fptr);
    int r;
    fptr(r, 4);
    CHECK(r == 12);
    fptr.reset();
    CHECK(!fptr);
    fptr.reset(func);
    CHECK(!!fptr);
    fptr(r, 5);
    CHECK(r == 15);
}
