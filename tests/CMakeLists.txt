include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(http_server_tests
    test_main.cpp
    test_mime.cpp
    test_path_utils.cpp
    test_request.cpp
    test_status_codes.cpp
    test_response.cpp
    test_thread_pool.cpp
)

target_link_libraries(http_server_tests
    PRIVATE
        http_server_lib
        GTest::gtest
)

target_compile_options(http_server_tests PRIVATE
    -Wall -Wextra -Wpedantic
)

gtest_discover_tests(http_server_tests)
