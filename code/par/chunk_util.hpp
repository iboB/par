// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "bits/imath.hpp"
#include <type_traits>
#include <utility>

namespace par {

template <typename U>
struct balanced_chunks {
    static_assert(std::is_unsigned_v<U>, "balanced_chunks requires an unsigned integer type");

    balanced_chunks(U size, U num_chunks)
        : r(size % num_chunks)
        , q(size / num_chunks)
    {}

    template <typename I>
    std::pair<I, I> operator()(I offset, U chunk_index) const {
        U cbegin, cend;
        if (chunk_index < r) {
            cbegin = U(offset) + chunk_index * (q + 1);
            cend = cbegin + q + 1;
        }
        else {
            cbegin = U(offset) + chunk_index * q + r;
            cend = cbegin + q;
        }
        return {I(cbegin), I(cend)};
    }

    U r, q;
};

template <typename U>
struct fixed_chunks {
    static_assert(std::is_unsigned_v<U>, "fixed_chunks requires an unsigned integer type");
    fixed_chunks(U size, U chunk_size)
        : size(size)
        , chunk_size(chunk_size)
        , num_chunks(divide_round_up(size, chunk_size))
    {}
    template <typename I>
    std::pair<I, I> operator()(I offset, U chunk_index) const {
        U cbegin = U(offset) + chunk_index * chunk_size;
        U cend = chunk_index + 1 < num_chunks ? cbegin + chunk_size : U(offset) + size;
        return {I(cbegin), I(cend)};
    }
    U size;
    U chunk_size;
    U num_chunks;
};

} // namespace par
