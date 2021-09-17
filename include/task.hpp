#pragma once
#include <experimental/coroutine>

template <typename Container>
auto pop(Container &container) -> typename Container::value_type {
    auto value = std::move(container.front());
    container.pop();
    return std::move(value);
}

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
        todo().emplace(std::move(work));
    }

    static void run() {
        while (!todo().empty()) {
            pop(todo())();
        }
    }

private:
    task() noexcept = default;
    static std::queue<std::function<void()>> &todo() {
        static std::queue<std::function<void()>> result;
        return result;
    }
};