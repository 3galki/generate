#pragma once
#include <experimental/coroutine>
#include <queue>
#include <task.hpp>
#include <variant>

using handle = std::experimental::coroutine_handle<>;

template <typename Type, typename Tasks = task>
class channel {
public:
    using value_type = Type;

    class in {
    public:
        using value_type = channel::value_type;
        void close() {}

        bool await_ready() const { return m_self->m_capacity > m_self->m_data.size(); }
        void await_suspend(handle coro) { m_self->m_in_queue.emplace(coro); }
        auto await_resume() {
            struct helper {
                channel *m_self;
                void operator << (value_type value) {
                    m_self->m_data.template emplace(std::move(value));
                    m_self->resume_out();
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
        std::shared_ptr<handle> coro;
        using value_type = channel::value_type;
        bool await_ready() const { return !m_self->m_data.empty(); }
        void await_suspend(handle c) {
            coro = std::make_shared<handle>(c);
            _suspend(coro);
        }
        void _suspend(std::weak_ptr<handle> c) { m_self->m_out_queue.emplace(c); }
        value_type await_resume() {
            auto value = pop(m_self->m_data);
            m_self->resume_in();
            return std::move(value);
        }

        template <size_t Index, typename Variant>
        void extract(Variant &v) {
            if (v.index() == 0 && await_ready()) {
                v.template emplace<Index + 1>(await_resume());
            }
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
    std::queue<std::weak_ptr<handle>> m_out_queue;

    void resume_in() {
        if (!m_in_queue.empty()) {
            task::post([coro = pop(m_in_queue)] () mutable { coro.resume(); });
        }
    }

    void resume_out() {
        while (!m_out_queue.empty()) {
            auto ptr = pop(m_out_queue);
            if (auto coro = ptr.lock(); coro != nullptr && *coro != nullptr) {
                task::post([coro = *coro] () mutable {
                    coro.resume();
                });
                *coro = nullptr;
                break;
            }
        }
    }
};

template<typename Variant, class Tuple, std::size_t... Is>
void tuple2variant(Variant &v, Tuple& t, std::index_sequence<Is...>) {
    (std::get<Is>(t).template extract<Is>(v), ...);
}

template <typename ...InChannels>
auto select(InChannels ...channels) {
    struct [[nodiscard]] awaitable {
        std::tuple<InChannels...> channels;
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
            std::variant<std::monostate, typename std::decay_t<InChannels>::value_type...> result;
            tuple2variant(result, channels, std::index_sequence_for<InChannels...>{});
            return std::move(result);
        }
    };

    return awaitable{std::make_tuple(channels...)};
}