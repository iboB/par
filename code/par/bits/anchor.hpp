// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <cstddef>
#include <utility>
#include <stdexcept>

// anchor:
//   for non-movable types that you want in a context which needs to compile BUT NOT INVOKE move operations
//   like std::vector which has reserved enough memory to avoid reallocation (and moving)
// usage:
//   std::vector<par::anchor<std::mutex>> vec;
//   vec.reserve(10); // !! this will guarantee no reallocation and no moves
//   for (int i = 0; i < 10; ++i)
//      vec.emplace_back(args...); // construct in preallocated space

namespace par {

template <typename T>
struct anchor {
    union {
        T value;
    };

    template <typename... Args>
    anchor(Args&&... args)
        : value(std::forward<Args>(args)...)
    {}

    ~anchor() {
        value.~T();
    }

    anchor(const anchor&) = delete;
    anchor& operator=(const anchor&) = delete;

    [[noreturn]] anchor(anchor&&) {
        throw std::runtime_error("attempt to move-construct anchor");
    }
    [[noreturn]] anchor& operator=(anchor&&) {
        throw std::runtime_error("attempt to move-assign anchor");
    }

    T& operator*() & { return value;  }
    const T& operator*() const & { return value; }
    T operator*() && = delete;

    T* operator->() & { return &value; }
    const T* operator->() const & { return &value; }
    T* operator->() && = delete;
};

} // namespace par
