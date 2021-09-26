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

namespace {
/* Аналог на variant */
task coroutine(channel<int>::out out1, channel<int>::out out2, channel<std::string>::out out3) {
    while (true) {
        auto result = co_await s(out1, out2, out3);
        if (auto *value = result.get_if(out1); value != nullptr) {
            std::cout << "got int    " << *value << std::endl;
        } else if (auto *value = result.get_if(out2); value != nullptr) {
            std::cout << "got int2 " << *value << std::endl;
        } else if (auto *value = result.get_if(out3); value != nullptr) {
            std::cout << "got string " << *value << std::endl;
        }
    }
}

task writer_int(channel<int>::in in) {
    for (int i = 1; i < 3; ++i) {
        co_await in << i;
        std::cout << "    push int    " << i << std::endl;
    }
    in.close();
}

task writer_string(channel<std::string>::in in) {
    for (int i = 1; i < 3; ++i) {
        co_await in << std::to_string(i);
        std::cout << "    push string " << i << std::endl;
    }
    in.close();
}
} // end of namespace

TEST_CASE("complex", "[simple]") {
    channel<int> channel_int1{1};
    channel<int> channel_int2{1};
    channel<std::string> channel_string{1};
    coroutine(channel_int1, channel_int2, channel_string);
    writer_int(channel_int1);
    writer_int(channel_int2);
    writer_string(channel_string);
}