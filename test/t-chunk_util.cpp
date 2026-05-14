// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/chunk_util.hpp>
#include <doctest/doctest.h>
#include <vector>

TEST_CASE("balanced_chunks") {
    using vec = std::vector<std::pair<int, int>>;

    auto test = [](int begin, unsigned size, unsigned num_chunks, vec expected) {
        par::balanced_chunks bc(size, num_chunks);
        vec chunks;
        for (unsigned i = 0; i < num_chunks; ++i) {
            chunks.push_back(bc(begin, i));
        }
        CHECK(chunks == expected);
    };

    test(0, 10, 1, {{0, 10}});
    test(0, 10, 2, {{0, 5}, {5, 10}});
    test(0, 10, 3, {{0, 4}, {4, 7}, {7, 10}});
    test(0, 10, 4, {{0, 3}, {3, 6}, {6, 8}, {8, 10}});

    test(100, 10, 1, {{100, 110}});
    test(100, 10, 2, {{100, 105}, {105, 110}});
    test(100, 10, 3, {{100, 104}, {104, 107}, {107, 110}});
    test(100, 10, 4, {{100, 103}, {103, 106}, {106, 108}, {108, 110}});

    test(-20, 10, 1, {{-20, -10}});
    test(-20, 10, 2, {{-20, -15}, {-15, -10}});
    test(-20, 10, 3, {{-20, -16}, {-16, -13}, {-13, -10}});
    test(-20, 10, 4, {{-20, -17}, {-17, -14}, {-14, -12}, {-12, -10}});
}
