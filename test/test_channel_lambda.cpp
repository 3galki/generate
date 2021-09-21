#define CATCH_CONFIG_MAIN
#include <catch2/catch_config.hpp>
#include <catch2/catch_test_macros.hpp>
#include <channel.hpp>
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

/* Аналог на лямбдах */
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

task writer_int(channel<int>::in in) {
    for (int i = 1; i<3; ++i) {
        co_await in << i;
        std::cout << "    push int    " << i << std::endl;
    }
    in.close();
}

task writer_string(channel<std::string>::in in) {
    for (int i = 1; i<3; ++i) {
        co_await in << std::to_string(i);
        std::cout << "    push string " << i << std::endl;
    }
    in.close();
}

TEST_CASE("lambda", "[simple]") {
    channel<int> channel_int{1};
    channel<std::string> channel_string{1};
    coroutine(channel_int, channel_string);
    writer_int(channel_int);
    writer_string(channel_string);
}