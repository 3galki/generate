#define CATCH_CONFIG_MAIN
#include <catch2/catch_config.hpp>
#include <catch2/catch_test_macros.hpp>
#include <channel.hpp>
#include <generator.hpp>
#include <iostream>
#include <task.hpp>
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
task coroutine(channel<int>::out out1, channel<std::string>::out out2) {
    auto data = co_await out1;
    while (true) {
        auto result = co_await select(out1, out2);
        if (auto *value = std::get_if<int>(&result); value != nullptr) {
            std::cout << "got int " << *value << std::endl;
        } else if (auto *value = std::get_if<std::string>(&result); value != nullptr) {
            std::cout << "got string " << *value << std::endl;
        } else {
            break;
        }
    }
}

task writer_int(channel<int>::in in) {
    for (int i = 1; i<10; ++i) {
        std::cout << "\tpush int " << i << std::endl;
        co_await in << i;
    }
    in.close();
}

task writer_string(channel<std::string>::in in) {
    for (int i = 1; i<10; ++i) {
        std::cout << "\tpush string " << i << std::endl;
        co_await in << std::to_string(i);
    }
    in.close();
}

TEST_CASE("create", "[simple]") {
    channel<int> channel_int{3};
    channel<std::string> channel_string{3};
    writer_int(channel_int);
    writer_string(channel_string);
    coroutine(channel_int, channel_string);
    task::run();
}