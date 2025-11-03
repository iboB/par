// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <cstddef>
#include <utility>

// type-erased function pointer

namespace par {

template <typename Func>
class te_func_ptr;

template <typename Ret, typename... Args>
class te_func_ptr<Ret(Args...)> {
    void* m_callable_payload;
    Ret(*m_invoke)(void*, Args...);
public:
    te_func_ptr() {
        reset();
    }
    te_func_ptr(std::nullptr_t) {
        reset(nullptr);
    }

    template <typename Func>
    explicit te_func_ptr(Func* func) {
        reset(func);
    }

    template <typename Func>
    explicit te_func_ptr(Func& func) {
        reset(func);
    }

    template <typename Func>
    void reset(Func* func) {
        static_assert(sizeof(Func*) == sizeof(void*));
        m_callable_payload = reinterpret_cast<void*>(func);
        m_invoke = [](void* payload, Args... args) -> Ret {
            Func* f = reinterpret_cast<Func*>(payload);
            return (*f)(std::forward<Args>(args)...);
        };
    }

    template <typename Func>
    void reset(Func& func) {
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
    Ret operator()(CallArgs&&... args) const {
        return m_invoke(m_callable_payload, std::forward<CallArgs>(args)...);
    }
};

} // namespace par
