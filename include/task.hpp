#pragma once
#include <experimental/coroutine>

class task {
public:
    struct promise_type {
        using suspend_never = std::experimental::suspend_never;
        task get_return_object() noexcept { return {}; }
        void return_void() noexcept { }
        suspend_never initial_suspend() noexcept { return {}; }
        suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }
    };

    static void post(std::function<void()> work) {
        todo().emplace_back(std::move(work));
    }

    static void run() {
        while (!todo().empty()) {
            auto last = std::move(todo().back());
            todo().pop_back();
            last();
        }
    }

private:
    task() noexcept = default;
    static std::vector<std::function<void()>> &todo() {
        static std::vector<std::function<void()>> result;
        return result;
    }
};