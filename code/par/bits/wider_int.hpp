// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <cstdint>
#include <type_traits>

namespace par {
namespace impl {
template <bool Signed, size_t Size>
struct wider_int;

template <> struct wider_int<true, 1> { using type = int16_t; };
template <> struct wider_int<true, 2> { using type = int32_t; };
template <> struct wider_int<true, 4> { using type = int64_t; };
template <> struct wider_int<false, 1> { using type = uint16_t; };
template <> struct wider_int<false, 2> { using type = uint32_t; };
template <> struct wider_int<false, 4> { using type = uint64_t; };

// 8 bytes is the largest we support, so we don't need to go beyond that
template <> struct wider_int<true, 8> { using type = int64_t; };
template <> struct wider_int<false, 8> { using type = uint64_t; };
} // namespace impl

template <typename T>
using wider_int_t = typename impl::wider_int<std::is_signed<T>::value, sizeof(T)>::type;

} // namespace par
