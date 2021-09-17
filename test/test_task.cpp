#define CATCH_CONFIG_MAIN
#include <catch2/catch_config.hpp>
#include <catch2/catch_test_macros.hpp>
#include <task.hpp>

TEST_CASE("task", "[simple]") {
    auto testing = false;
    task::post([&] () {testing = true; });
    CHECK_FALSE(testing);
    task::run();
    CHECK(testing);
}