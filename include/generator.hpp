#pragma once
#include <experimental/coroutine>
#include <variant>

template <typename Value>
class generator {
public:
    using value_type = Value;
    using ref = value_type &;
    struct promise_type {
        using handle = std::experimental::coroutine_handle<promise_type>;
        using suspend_always = std::experimental::suspend_always;
        using suspend_never = std::experimental::suspend_never;
        std::variant<std::monostate, Value, std::exception_ptr> value;
        auto get_return_object() noexcept {
            return generator{handle::from_promise(*this)};
        }
        suspend_always initial_suspend() noexcept { return {}; }
        suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {
            value.template emplace<0>();
        }
        suspend_always yield_value(Value v) noexcept {
            value.template emplace<1>(std::move(v));
            return {};
        }
        void unhandled_exception() {
            value.template emplace<2>(std::current_exception());
        }
    };
    generator() noexcept = delete;
    generator(const generator &) = delete;
    generator(generator &&old) noexcept {
        std::swap(old.m_coro, m_coro);
    }
    ~generator() noexcept { if (m_coro) { m_coro.destroy();} }

    class iterator {
    public:
        [[nodiscard]] bool operator != (const iterator &second) const {
            if (m_self != nullptr) {
                return !m_self->eof();
            }
            return !second.m_self->eof();
        }
        iterator & operator++() {
            m_self->fetch();
            m_self->m_fetched = false;
            return *this;
        }
        [[nodiscard]] ref operator *() const {
            m_self->fetch();
            return m_self->value();
        }

    private:
        explicit iterator(generator *self) noexcept : m_self{self} {}
        generator *m_self;
        friend class generator;
    };
    iterator begin() noexcept { return iterator{this}; }
    iterator end() noexcept { return iterator{nullptr}; }
    ref operator ()() {
        next();
        return value();
    }

    void next() {
        m_fetched = false;
        fetch();
    }

    [[nodiscard]] bool eof() {
        fetch();
        return m_coro.promise().value.index() == 0;
    }

    ref value() {
        assert(!eof());
        if (auto err = std::get_if<2>(&m_coro.promise().value); err != nullptr) {
            std::rethrow_exception(*err);
        }
        return std::get<Value>(m_coro.promise().value);
    }

private:
    using handle = typename promise_type::handle;
    handle m_coro;
    bool m_fetched{false};

    explicit generator(handle coro) noexcept : m_coro{coro} {}

    void fetch() {
        if (!m_fetched) {
            m_coro.resume();
            m_fetched = true;
        }
    }
};

template <typename Value> concept generator_type = requires {
    typename std::decay_t<Value>::value_type;
    std::is_same_v<std::decay_t<Value>, generator<typename std::decay_t<Value>::value_type>>;
};

template <generator_type Generator, typename Predicate>
auto operator | (Generator &&source, Predicate predicate) -> std::decay_t<Generator> {
    for (auto &value: source) {
        if (predicate(value)) {
            co_yield std::move(value);
        }
    }
}

template <generator_type Generator1, generator_type Generator2,
        typename = std::enable_if_t<std::is_same_v<std::decay_t<Generator1>, std::decay_t<Generator2>>>>
auto operator + (Generator1 &&source1, Generator2 &&source2) -> std::decay_t<Generator1> {
    for (auto &value: source1) {
        co_yield std::move(value);
    }
    for (auto &value: source2) {
        co_yield std::move(value);
    }
}

template <typename Container>
auto make_generator(Container &&container) -> generator<typename std::decay_t<Container>::value_type> {
    for (auto &value: container) {
        co_yield value;
    }
}

template <generator_type Generator1, generator_type Generator2>
using pair_generator = generator<std::pair<
        typename std::decay_t<Generator1>::value_type,
        typename std::decay_t<Generator2>::value_type>>;

template <generator_type Generator1, generator_type Generator2>
auto zip(Generator1 &&source1, Generator2 &&source2) -> pair_generator<Generator1, Generator1> {
    using Value1 = typename std::decay_t<Generator1>::value_type;
    using Value2 = typename std::decay_t<Generator2>::value_type;
    while (!source1.eof() && !source2.eof()) {
        co_yield std::make_pair<Value1, Value2>(std::move(source1.value()), std::move(source2.value()));
        source1.next();
        source2.next();
    }
}

template <generator_type ...Generators>
using tuple_generator = generator<std::tuple<typename std::decay_t<Generators>::value_type...>>;

template <generator_type ...Generators>
auto zip(Generators && ...sources) -> tuple_generator<Generators...> {
    while ((!sources.eof() && ...)) {
        co_yield std::make_tuple<typename std::decay_t<Generators>::value_type...>(std::move(sources.value())...);
        (sources.next(),...);
    }
}
