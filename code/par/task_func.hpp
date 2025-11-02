// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <cstddef>
#include <cstdint>

// type-erased function pointer

namespace par {

class task_func {
    const void* m_callable_payload;
    void(*m_trampoline)(const void*, uint32_t);
public:
    task_func() {
        reset();
    }

    template <typename Func>
    explicit task_func(Func* func) {
        reset(func);
    }

    template <typename Func>
    explicit task_func(Func& func) {
        reset(func);
    }

    template <typename Func>
    void reset(Func* func) {
        static_assert(sizeof(Func*) == sizeof(void*));
        m_callable_payload = reinterpret_cast<const void*>(func);
        m_trampoline = [](const void* payload, uint32_t arg) {
            Func* f = reinterpret_cast<Func*>(payload);
            (*f)(arg);
        };
    }

    template <typename Func>
    void reset(const Func& func) {
        reset(&func);
    }

    void reset() {
        m_callable_payload = nullptr;
        m_trampoline = nullptr;
    }

    explicit operator bool() const {
        return !!m_callable_payload;
    }

    void operator()(uint32_t arg) const {
        return m_trampoline(m_callable_payload, arg);
    }
};

} // namespace par
