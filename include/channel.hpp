#pragma once
#include <experimental/coroutine>
#include <variant>

using handle = std::experimental::coroutine_handle<>;

template <typename Type>
class channel {
public:
    class in {
    public:
        using value_type = Type;
        void close() {}

        bool await_ready() const { return false; }
        void await_suspend(handle coro) {}
        auto await_resume() {
            struct helper {
                void operator << (value_type value) {}
            };
            return helper{};
        }
    };

    class out {
    public:
        using value_type = Type;
        bool await_ready() const { return false; }
        void await_suspend(handle coro) {}
        value_type await_resume() {
            return {};
        }
    };

    channel(size_t size) {}

    operator in() { return {}; }
    operator out() { return {}; }
};

template <typename ...Channels>
auto select(Channels &&...channels) {
    struct [[nodiscard]] awaitable {
        bool await_ready() const { return false; }
        void await_suspend(handle coro) {}
        std::variant<std::monostate, typename std::decay_t<Channels>::value_type...> await_resume() {
            return {};
        }
    };
    return awaitable{};
}