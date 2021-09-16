#define CATCH_CONFIG_MAIN
#include <catch2/catch_config.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <generator.hpp>

struct not_copyble {
    not_copyble(int v) : value{v} {}
    not_copyble(const not_copyble &) = delete;
    not_copyble(not_copyble &&) = default;
    not_copyble& operator=(const not_copyble &) = delete;
    not_copyble& operator=(not_copyble &&) = default;
    ~not_copyble() = default;
    int value;
    operator int() { return value; }
};

template <typename Type>
generator<Type> gen(int start, int count) {
    for (int i = 0; i < count; ++i) {
        co_yield start + i;
    }
}

TEMPLATE_TEST_CASE("create", "[simple]", int, not_copyble) {
    CHECK_NOTHROW(gen<TestType>(0,0));
}

template <generator_type Generator>
size_t counter(Generator &&generator) {
    size_t counter = 0;
    for (auto &value: generator) {
        ++counter;
    }
    return counter;
}

TEMPLATE_TEST_CASE("count", "[simple]", int, not_copyble) {
    CHECK(counter(gen<TestType>(0,0)) == 0);
    CHECK(counter(gen<TestType>(0,1)) == 1);
    CHECK(counter(gen<TestType>(0,100)) == 100);
}

TEMPLATE_TEST_CASE("filter", "[algorithm]", int, not_copyble) {
    auto odd = [](auto &value) { return int(value) % 2 == 0; };
    WHEN("Inplace") {
        CHECK(counter(gen<TestType>(0, 0) | odd) == 0);
        CHECK(counter(gen<TestType>(0, 1) | odd) == 1);
        CHECK(counter(gen<TestType>(0, 2) | odd) == 1);
        CHECK(counter(gen<TestType>(0, 100) | odd) == 50);
    } WHEN("copy") {
        auto gen1 = gen<TestType>(0, 100);
        CHECK(counter(gen1 | odd) == 50);

    }
}

TEMPLATE_TEST_CASE("concat", "[algorithm]", int, not_copyble) {
    WHEN("Inplace") {
        CHECK(counter(gen<TestType>(0, 0) + gen<TestType>(0, 0)) == 0);
        CHECK(counter(gen<TestType>(0, 1) + gen<TestType>(0, 0)) == 1);
        CHECK(counter(gen<TestType>(0, 1) + gen<TestType>(0, 1)) == 2);
    } WHEN("copy") {
        auto gen1 = gen<TestType>(0, 1);
        auto gen2 = gen<TestType>(0, 1);
        CHECK(counter(gen1 + gen2) == 2);
    } WHEN("mixed") {
        auto gen1 = gen<TestType>(0, 1);
        CHECK(counter(gen1 + gen<TestType>(0, 1)) == 2);
    }
}

TEST_CASE("make", "[adapter]") {
    std::vector values = {1,2,3,4,5};
    CHECK(counter(make_generator(values)) == 5);
}

TEST_CASE("zip", "[algorithm]") {
    WHEN("inplace") {
        CHECK(counter(zip(gen<int>(0, 0), gen<int>(0, 0))) == 0);
    } WHEN("reference") {
        auto gen1 = gen<int>(0, 0);
        auto gen2 = gen<int>(0, 0);
        CHECK(counter(zip(gen1, gen2)) == 0);
    }
}

TEST_CASE("zip3", "[algorithm]") {
    WHEN("inplace") {
        CHECK(counter(zip(gen<int>(0, 1), gen<int>(0, 1), gen<int>(0, 1))) == 1);
    } WHEN("reference") {
        auto gen1 = gen<int>(0, 1);
        auto gen2 = gen<int>(0, 1);
        auto gen3 = gen<int>(0, 1);
        CHECK(counter(zip(gen1, gen2, gen3)) == 1);
    }
}