cmake_minimum_required(VERSION 3.19)
project(coro_2021_09_15)

set(CMAKE_CXX_STANDARD 20)
include_directories(include)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

enable_testing()

add_subdirectory(test)

#add_executable(coro_2021_09_15 main.cpp)
