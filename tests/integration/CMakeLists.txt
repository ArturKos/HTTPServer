find_package(CURL REQUIRED)

add_executable(http_server_integration_tests
    test_integration.cpp
)

target_link_libraries(http_server_integration_tests
    PRIVATE
        GTest::gtest
        CURL::libcurl
)

target_compile_options(http_server_integration_tests PRIVATE
    -Wall -Wextra -Wpedantic
)

target_compile_definitions(http_server_integration_tests PRIVATE
    HTTP_SERVER_BINARY_PATH=\"$<TARGET_FILE:httpserver>\"
)

add_dependencies(http_server_integration_tests httpserver)

gtest_discover_tests(http_server_integration_tests
    PROPERTIES LABELS "integration"
)
