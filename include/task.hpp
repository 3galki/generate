#pragma once
#include <experimental/coroutine>

namespace std {
using namespace experimental;
} // end of namespace std

class task {
public:
    struct promise_type {
        using suspend_never = std::suspend_never;
        task get_return_object() noexcept { return {}; }
        void return_void() noexcept { }
        suspend_never initial_suspend() noexcept { return {}; }
        suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }
    };

private:
    task() noexcept = default;
};