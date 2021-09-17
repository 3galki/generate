add_executable(test_generate
        test_generate.cpp
        )
target_link_libraries(test_generate ${CONAN_LIBS})

add_test(NAME test_generate COMMAND test_generate)

add_executable(test_channel
        test_channel_variant.cpp
        )
target_link_libraries(test_channel ${CONAN_LIBS})

add_test(NAME test_channel COMMAND test_channel)