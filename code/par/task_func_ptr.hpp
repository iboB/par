// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <cstddef>
#include <utility>

// type-erased function pointer

namespace par {

template <typename... Args>
class task_func_ptr {
    const void* m_callable_payload;
    void(*m_invoke)(const void*, Args...);
public:
    task_func_ptr() {
        reset();
    }
    task_func_ptr(std::nullptr_t) {
        reset(nullptr);
    }

    template <typename Func>
    explicit task_func_ptr(Func* func) {
        reset(func);
    }

    template <typename Func>
    explicit task_func_ptr(Func& func) {
        reset(func);
    }

    template <typename Func>
    void reset(Func* func) {
        static_assert(sizeof(Func*) == sizeof(void*));
        m_callable_payload = reinterpret_cast<const void*>(func);
        m_invoke = [](const void* payload, Args... args) {
            Func* f = reinterpret_cast<Func*>(payload);
            (*f)(std::forward<Args>(args)...);
        };
    }

    template <typename Func>
    void reset(const Func& func) {
        reset(&func);
    }

    void reset() {
        m_callable_payload = nullptr;
        m_invoke = nullptr;
    }

    void reset(std::nullptr_t) {
        reset();
    }

    explicit operator bool() const {
        return !!m_callable_payload;
    }

    template <typename... CallArgs>
    void operator()(CallArgs&&... args) const {
        return m_invoke(m_callable_payload, std::forward<CallArgs>(args)...);
    }
};

} // namespace par
