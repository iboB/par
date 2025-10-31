// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <cstddef>

// Why not use std::hardware_*structive_interference_size?
// The reason is that gcc and clang detect the use of them in headers and emit warnings in this case
// and rightfully so. It's theoretically dangerous to use this in a public ABI, especially in the light of
// -march or -mtune options.
// If these values find themselves in a public ABI, ODR violations may ensue.
// We however are not as conservative as the C++ standardization committee. We don't care to support
// every conceivable platform. The ones we do are pretty straight-forward.

namespace par::cpu {

inline constexpr size_t cache_line_size =
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
    // The cache line size *is* indeed 128 bytes, but this header is not meant for device code.
    // If used in both host and device code the ODR violation risk mentioned above is too high.
    // Err on the side of caution.
    #error Dont use this header in CUDA or HIP device code
#elif defined(_M_X64) || defined(__x86_64__) || defined(__amd64__) || defined(_M_IX86) || defined(__i386__)
    // for x86 and x64 safely assume 64 byte cache lines
    // 32 bytes or less were last seen in the Pentium III era (pre 2000)
    64
#elif defined(__APPLE__)
    // non-x86 Apple hardware means Apple silicon, which has 128 byte cache lines
    128
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64__)
    // ARM64 has 64 byte cache lines
    64
#else
    // we could get creative on 32-bit ARM, but there's more variability there
    // instead of guessing, be conservative and error out
    // it's doubtful that this software will be compiled for 32-bit ARM, anyway
    #error Unknown par::cpu platform
#endif
    ;

// For future proof-ness and clarity we still use different constants here.
// They would be different only on the most exotic of platforms (like some IBM z/Architecture CPUs)
inline constexpr size_t alignment_to_avoid_false_sharing = cache_line_size;
inline constexpr size_t min_cache_line_size_for_true_sharing = cache_line_size;

} // namespace par::cpu
