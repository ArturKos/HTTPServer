#include <gtest/gtest.h>

#include "http_server/logger.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace {

std::filesystem::path make_temporary_log_path(const std::string& base_name)
{
    auto temp_directory = std::filesystem::temp_directory_path();
    return temp_directory / (base_name + "_" + std::to_string(getpid()) + ".log");
}

size_t file_size_or_zero(const std::filesystem::path& file_path)
{
    std::error_code ignored_error_code;
    auto file_size = std::filesystem::file_size(file_path, ignored_error_code);
    if (ignored_error_code) return 0;
    return static_cast<size_t>(file_size);
}

void remove_log_family(const std::filesystem::path& base_path, int max_rotations)
{
    std::error_code ignored_error_code;
    std::filesystem::remove(base_path, ignored_error_code);
    for (int rotation_index = 1; rotation_index <= max_rotations; ++rotation_index) {
        auto rotated_path = base_path;
        rotated_path += "." + std::to_string(rotation_index);
        std::filesystem::remove(rotated_path, ignored_error_code);
    }
}

}  // namespace

TEST(LoggerFileBackend, WritesRequestLineToFile)
{
    auto log_path = make_temporary_log_path("logger_write");
    remove_log_family(log_path, 3);

    ASSERT_TRUE(logger_initialize_file(log_path.c_str(), 0, 3));
    logger_log_request("10.0.0.1", 7, 200, "/index.html");
    logger_shutdown();

    std::ifstream log_file_stream(log_path);
    std::string log_line;
    std::getline(log_file_stream, log_line);
    EXPECT_NE(log_line.find("client=10.0.0.1"), std::string::npos);
    EXPECT_NE(log_line.find("status=200"),      std::string::npos);
    EXPECT_NE(log_line.find("path=\"/index.html\""), std::string::npos);

    remove_log_family(log_path, 3);
}

TEST(LoggerFileBackend, RotatesAtSizeLimit)
{
    auto log_path = make_temporary_log_path("logger_rotate");
    remove_log_family(log_path, 3);

    const size_t rotation_threshold_bytes = 256;
    ASSERT_TRUE(logger_initialize_file(log_path.c_str(),
                                       rotation_threshold_bytes, 3));

    for (int request_index = 0; request_index < 20; ++request_index) {
        logger_log_request("127.0.0.1", request_index, 200, "/index.html");
    }
    logger_shutdown();

    auto rotated_path_1 = log_path;
    rotated_path_1 += ".1";
    EXPECT_GT(file_size_or_zero(rotated_path_1), 0u);
    EXPECT_LE(file_size_or_zero(log_path), rotation_threshold_bytes + 256);

    remove_log_family(log_path, 3);
}
