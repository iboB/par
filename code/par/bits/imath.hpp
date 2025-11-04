// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <concepts>

namespace par {

template <std::integral T>
constexpr T divide_round_up(T dividend, T divisor) {
    return (dividend + divisor - 1) / divisor;
}

} // namespace par
