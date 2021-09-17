#define CATCH_CONFIG_MAIN
#include <catch2/catch_config.hpp>
#include <catch2/catch_test_macros.hpp>
#include <generator.hpp>
#include <iostream>

/* Оригинал на GoLang */
/*
func test(out1 <-chan int, out2 <-chan string) {
	select {
	case value := <-out1:
		fmt.Println("got int ", value)
	case value := <-out2:
		fmt.Println("got string ", value)
	}
}
*/

/* Аналог на variant */
/*
task coroutine(channel<int>::out out1, channel<std::string>::out out2) {
    auto result = co_await select(out1, out2);
    if (auto value = std::get_if<int>(result); value != nullptr) {
        std::cout << "got int " << *value << std::endl;
    } else if (auto value = std::get_if<std::string>(result); value != nullptr) {
        std::cout << "got string " << *value << std::endl;
    }
}
*/

/* Аналог на лямбдах */
/*
task coroutine(channel<int>::out out1, channel<std::string>::out out2) {
    co_await select({
        out1 >> [](auto value) {
            std::cout << "got int " << value << std::endl;
        },
        out2 >> [](auto value) {
            std::cout << "got string " << value << std::endl;
        },
    });
}
*/

/* "Сложный вариант" */
/*
task coroutine(channel<int>::out out1, channel<std::string>::out out2) {
    auto result = co_await select(out1, out2);
    if (auto *value = result.get(out1); value != nullptr) {
        std::cout << "got int " << *value << std::endl;
    } else if (auto *value = result.get(out2); value != nullptr) {
        std::cout << "got string " << *value << std::endl;
    }
}
*/

TEST_CASE("create", "[simple]") {
    coroutine();
}