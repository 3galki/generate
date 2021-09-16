add_executable(test_all
        test_generate.cpp
        )
target_link_libraries(test_all ${CONAN_LIBS})

add_test(NAME test_all COMMAND test_all)