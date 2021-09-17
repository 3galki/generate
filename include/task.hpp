#pragma once
#include <experimental/coroutine>

class task {
public:
    struct promise_type {
        using handle = std::experimental::coroutine_handle<promise_type>;
        using suspend_always = std::experimental::suspend_always;
        using suspend_never = std::experimental::suspend_never;

        task get_return_object() noexcept { return {}; }
        void return_void() noexcept { }
        suspend_never initial_suspend() noexcept { return {}; }
        suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }
    };

    static void run() {}
private:
    task() noexcept = default;
};