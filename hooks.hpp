/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Felix Brendel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include "./arraylist.hpp"

#include <type_traits> // For std::decay

template<typename>
struct Lambda; // intentionally not defined

template<typename R, typename ...Args>
struct Lambda<R(Args...)> {
    using Dispatcher = R(*)(void*, Args...);

    Dispatcher m_Dispatcher; // A pointer to the static function that will call the
    // wrapped invokable object
    void* m_Target;          // A pointer to the invokable object

    // Dispatch() is instantiated by the Lambda constructor,
    // which will store a pointer to the function in m_Dispatcher.
    template<typename S>
    static R Dispatch(void* target, Args... args)
    {
        return (*(S*)target)(args...);
    }

    template<typename T>
    Lambda(T&& target)
        : m_Dispatcher(&Dispatch<typename std::decay<T>::type>)
        , m_Target(&target)
    {}

    // Specialize for reference-to-function, to ensure that a valid pointer is
    // stored.
    using TargetFunctionRef = R(Args...);
    Lambda(TargetFunctionRef target)
        : m_Dispatcher(Dispatch<TargetFunctionRef>)
    {
        static_assert(sizeof(void*) == sizeof target,
                      "It will not be possible to pass functions by reference on this platform. "
                      "Please use explicit function pointers i.e. foo(target) -> foo(&target)");
        m_Target = (void*)target;
    }

    R operator()(Args... args) const
    {
        return m_Dispatcher(m_Target, args...);
    }
};

struct Hook {
    Array_List<Lambda<void()>> lambdas;
    Hook() {
        lambdas.init();
    }
    ~Hook () {
        lambdas.deinit();
    }
    void operator<<(Lambda<void()>&& f) {
        lambdas.append(f);
    }
    void operator()() {
        for (auto l : lambdas) {
            l();
        }
        lambdas.clear();
    }
};

#ifdef FTB_GLOBAL_SHUTDOWN_HOOK
struct __System_Shutdown_Hook : Hook {
    void operator()() = delete;
    ~__System_Shutdown_Hook() {
        Hook::operator()();
        lambdas.deinit();
    }
} system_shutdown_hook;
#endif
