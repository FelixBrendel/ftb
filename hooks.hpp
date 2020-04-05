#pragma once
#include "./arraylist.hpp"

// For std::decay
#include <type_traits>

template<typename>
struct Lambda; // intentionally not defined

template<typename R, typename ...Args>
struct Lambda<R(Args...)>
{
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
        {
        }

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


struct Hook : Array_List<Lambda<void()>> {
    Hook() {
        alloc();
    }
    ~Hook () {
        dealloc();
    }
    void operator<<(Lambda<void()> f) {
        // FIXME(Felix): Why can I not call Array_List::append here??? Hallo?
        if (next_index == length) {
            length *= 2;
            data = (Lambda<void()>*)realloc(data, length * sizeof(Lambda<void()>));
        }
        data[next_index] = f;
        next_index++;
    }
    void operator()() {
        while(next_index --> 0) {
            fflush(stdout);
            data[next_index]();
        }
        next_index = 0;
    }
};

// struct __System_Shutdown_Hook : Hook {
//     void operator()() = delete;
//     ~__System_Shutdown_Hook() {
//         Hook::operator()();
//         dealloc();
//     }
// } system_shutdown_hook;
