#pragma once
#include <concepts>
#include <experimental/coroutine>
#include <queue>
#include <variant>

template <typename Container>
auto pop(Container &container) -> typename Container::value_type {
    auto value = std::move(container.front());
    container.pop();
    return std::move(value);
}

struct default_scheduler {
    static void post(std::function<void()> callback) {
        callback();
    }
};

template <typename Type, typename Scheduler = default_scheduler>
class channel {
public:
    using handle = std::experimental::coroutine_handle<>;
    using value_type = Type;

    class in {
    public:
        using value_type = channel::value_type;
        void close() {}

        bool await_ready() const { return m_self->m_capacity > m_self->m_data.size(); }
        void await_suspend(handle coro) { m_self->m_in_queue.emplace(coro); }
        auto await_resume() {
            class helper {
            public:
                ~helper() { ++m_self->m_capacity; }
                void operator << (value_type value) {
                    m_self->m_data.template emplace(std::move(value));
                    m_self->resume_out();
                }
            private:
                channel *m_self;
                helper(channel *self) : m_self{self} { --m_self->m_capacity; };
                helper(const helper &) = delete;
                helper & operator = (const helper &) = delete;
                helper(helper &&) = delete;
                helper & operator = (helper &&) = delete;
                friend class in;
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
        bool await_ready() const { return !m_self->m_data.empty(); }
        void await_suspend(handle c) { m_self->m_out_queue.emplace(c); }
        void _suspend(std::weak_ptr<handle> c) { m_self->m_out_queue.emplace(c); }
        value_type await_resume() {
            auto value = pop(m_self->m_data);
            m_self->resume_in();
            return std::move(value);
        }

        template <size_t Index, typename Variant>
        bool extract(Variant &v) {
            if (await_ready()) {
                v.template emplace<Index>(await_resume());
                return true;
            }
            return false;
        }

        bool callback(std::function<void(value_type)> &callback) {
            if (await_ready()) {
                callback(await_resume());
                return true;
            }
            return false;
        }

        auto operator >> (std::function<void(value_type)> callback) {
            struct helper {
                using value_type = channel::value_type;
                std::function<void(value_type)> callback;
                typename channel<Type>::out self;
            };
            return helper{std::move(callback), *this};
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
    std::queue<value_type> m_data;
    std::queue<handle> m_in_queue;
    std::queue<std::variant<handle, std::weak_ptr<handle>>> m_out_queue;

    void resume_in() {
        if (!m_in_queue.empty()) {
            Scheduler::post([coro = pop(m_in_queue)] () mutable { coro.resume(); });
        }
    }

    void resume_out() {
        while (!m_out_queue.empty()) {
            auto ptr = pop(m_out_queue);
            if (auto coro = std::get_if<handle>(&ptr); coro != nullptr) {
                Scheduler::post([coro = *coro] () mutable { coro.resume(); });
                break;
            } else if (auto coro = std::get<std::weak_ptr<handle>>(ptr).lock(); coro != nullptr && *coro != nullptr) {
                auto c = *coro;
                *coro = nullptr;
                Scheduler::post([coro = c] () mutable { coro.resume(); });
                break;
            }
        }
    }
};

template<typename Variant, class Tuple, std::size_t... Is>
void tuple2variant(Variant &v, Tuple& t, std::index_sequence<Is...>) {
    (std::get<Is>(t).template extract<Is>(v) || ...);
}

template <typename Type>
concept channel_out = requires(Type T) {
    typename Type::value_type;
    std::is_same_v<Type, typename channel<typename Type::value_type>::out>;
    { T.await_ready() };
};

template <channel_out ...Channels>
auto select(Channels ...channels) {
    struct [[nodiscard]] awaitable {
        using handle = std::experimental::coroutine_handle<>;
        std::tuple<Channels...> channels;
        std::shared_ptr<handle> coro;
        bool await_ready() const {
            return std::apply([](auto & ...values) {
                return (values.await_ready() || ...);
            }, channels);
        }
        void await_suspend(handle c) {
            coro = std::make_shared<handle>(c);
            std::apply([this](auto & ...values) {
                (values._suspend(coro), ...);
            }, channels);
        }
        auto await_resume() {
            std::variant<typename Channels::value_type...> result;
            tuple2variant(result, channels, std::index_sequence_for<Channels...>{});
            return std::move(result);
        }
    };

    return awaitable{std::make_tuple(channels...)};
}

template <typename ...Callbacks>
auto select(Callbacks ...callbacks) {
    struct [[nodiscard]] awaitable {
        using handle = std::experimental::coroutine_handle<>;
        std::tuple<Callbacks...> callbacks;
        std::shared_ptr<handle> coro;
        bool await_ready() const {
            return std::apply([](auto & ...values) {
                return (values.self.await_ready() || ...);
            }, callbacks);
        }
        void await_suspend(handle c) {
            coro = std::make_shared<handle>(c);
            std::apply([this](auto & ...values) {
                (values.self._suspend(coro), ...);
            }, callbacks);
        }
        void await_resume() {
            std::apply([](auto & ...values) {
                return (values.self.callback(values.callback) || ...);
            }, callbacks);
        }
    };
    return awaitable{std::make_tuple(std::move(callbacks)...)};
}