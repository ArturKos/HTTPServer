cmake_minimum_required(VERSION 3.14)
project(HTTPServer
    VERSION 1.0.0
    DESCRIPTION "Multi-process HTTP/1.1 server with CGI and PHP support"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(BUILD_TESTING      "Build unit tests"       ON)
option(BUILD_DOCUMENTATION "Build Doxygen docs"    OFF)
option(ENABLE_TLS          "Compile TLS support via OpenSSL" ON)

set(HTTP_SERVER_COMPILE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wformat=2
    -Wconversion
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wunused
    -Wdouble-promotion
    -Wnull-dereference
    -Wwrite-strings
    -D_GNU_SOURCE
)

find_package(Threads REQUIRED)

add_library(http_server_lib STATIC
    src/status_codes.c
    src/mime.c
    src/path_utils.c
    src/range_utils.c
    src/logger.c
    src/socket_utils.c
    src/request.c
    src/response.c
    src/connection_io.c
    src/tls_context.c
    src/config_loader.c
    src/file_handler.c
    src/cgi_handler.c
    src/php_handler.c
    src/thread_pool.c
    src/server.c
)

target_include_directories(http_server_lib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(http_server_lib PRIVATE ${HTTP_SERVER_COMPILE_OPTIONS})
target_link_libraries(http_server_lib PUBLIC Threads::Threads)

if(ENABLE_TLS)
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(http_server_lib PUBLIC HTTP_SERVER_TLS_ENABLED=1)
    target_link_libraries(http_server_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
else()
    target_compile_definitions(http_server_lib PUBLIC HTTP_SERVER_TLS_ENABLED=0)
endif()

add_executable(httpserver src/main.c)
target_link_libraries(httpserver PRIVATE http_server_lib)
target_compile_options(httpserver PRIVATE ${HTTP_SERVER_COMPILE_OPTIONS})

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_DOCUMENTATION)
    find_package(Doxygen REQUIRED)
    set(DOXYGEN_INPUT_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src")
    set(DOXYGEN_OUTPUT_DIR  "${CMAKE_CURRENT_BINARY_DIR}/docs")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        @ONLY
    )
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

install(TARGETS httpserver RUNTIME DESTINATION bin)
