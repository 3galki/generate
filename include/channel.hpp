#pragma once
#include <experimental/coroutine>
#include <queue>
#include <variant>

using handle = std::experimental::coroutine_handle<>;

template <typename Type>
class channel {
public:
    using value_type = Type;

    class in {
    public:
        using value_type = channel::value_type;
        void close() {}

        bool await_ready() const { return m_self->m_capacity > m_self->m_queue.size(); }
        void await_suspend(handle coro) {
            m_self->m_in_queue.emplace(coro);
        }
        auto await_resume() {
            struct helper {
                channel *m_self;
                void operator << (value_type value) {
                    m_self->m_queue.template emplace(std::move(value));
                    // TODO: resume writer
                }
            };
            return helper{m_self};
        }
    private:
        explicit in(channel *self) noexcept : m_self{self} {}
        channel *m_self;
        friend class channel;
    };

    class out {
    public:
        using value_type = channel::value_type;
        bool await_ready() const { return !m_self->m_queue.empty(); }
        void await_suspend(handle coro) {
            m_self->m_out_queue.emplace(coro);
        }
        value_type await_resume() {
            auto value = std::move(m_self->m_queue.front());
            m_self->m_queue.pop();
            // TODO: resume reader
            return std::move(value);
        }
    private:
        explicit out(channel *self) noexcept : m_self{self} {}
        channel *m_self;
        friend class channel;
    };

    channel(size_t size) noexcept : m_capacity{size} {}

    operator in() noexcept { return in{this}; }
    operator out() noexcept { return out{this}; }
private:
    size_t m_capacity;
    std::queue<value_type> m_queue;
    std::queue<handle> m_in_queue;
    std::queue<handle> m_out_queue;
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