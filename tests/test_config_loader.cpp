#include <gtest/gtest.h>

#include "http_server/config_loader.h"
#include "http_server/server.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace {

std::filesystem::path make_temporary_config_file(const std::string& base_name,
                                                  const std::string& contents)
{
    auto temporary_directory = std::filesystem::temp_directory_path();
    auto config_path = temporary_directory
                       / (base_name + "_" + std::to_string(getpid()) + ".ini");
    std::ofstream output_stream(config_path);
    output_stream << contents;
    return config_path;
}

ServerConfig make_default_server_config()
{
    ServerConfig server_config{};
    server_config.log_max_file_size_bytes = 10u * 1024u * 1024u;
    server_config.log_max_rotated_files   = 5;
    return server_config;
}

}  // namespace

TEST(ConfigLoader, ParsesAllSupportedKeys)
{
    auto config_path = make_temporary_config_file("config_full",
        "# comment\n"
        "[server]\n"
        "port = 8081\n"
        "document_root = /srv/www\n"
        "log_target = /var/log/httpserver.log\n"
        "log_max_file_size = 1048576\n"
        "log_max_rotated = 7\n"
        "tls_certificate = /etc/ssl/cert.pem\n"
        "tls_private_key = /etc/ssl/key.pem\n");

    ServerConfig server_config = make_default_server_config();
    ASSERT_TRUE(config_load_from_file(config_path.string().c_str(), &server_config));

    EXPECT_EQ(server_config.listen_port, 8081u);
    EXPECT_STREQ(server_config.document_root, "/srv/www");
    EXPECT_FALSE(server_config.use_syslog_backend);
    EXPECT_STREQ(server_config.log_file_path, "/var/log/httpserver.log");
    EXPECT_EQ(server_config.log_max_file_size_bytes, 1048576u);
    EXPECT_EQ(server_config.log_max_rotated_files, 7);
    EXPECT_TRUE(server_config.use_tls);
    EXPECT_STREQ(server_config.tls_certificate_path, "/etc/ssl/cert.pem");
    EXPECT_STREQ(server_config.tls_private_key_path, "/etc/ssl/key.pem");

    std::filesystem::remove(config_path);
}

TEST(ConfigLoader, AcceptsSyslogTarget)
{
    auto config_path = make_temporary_config_file("config_syslog",
        "port = 80\n"
        "document_root = .\n"
        "log_target = syslog\n");

    ServerConfig server_config = make_default_server_config();
    ASSERT_TRUE(config_load_from_file(config_path.string().c_str(), &server_config));
    EXPECT_TRUE(server_config.use_syslog_backend);
    std::filesystem::remove(config_path);
}

TEST(ConfigLoader, RejectsInvalidPort)
{
    auto config_path = make_temporary_config_file("config_bad_port",
        "port = 99999\n"
        "document_root = .\n");

    ServerConfig server_config = make_default_server_config();
    EXPECT_FALSE(config_load_from_file(config_path.string().c_str(), &server_config));
    std::filesystem::remove(config_path);
}

TEST(ConfigLoader, ToleratesUnknownKeys)
{
    auto config_path = make_temporary_config_file("config_unknown",
        "port = 8080\n"
        "document_root = .\n"
        "unknown_key = whatever\n");

    ServerConfig server_config = make_default_server_config();
    EXPECT_TRUE(config_load_from_file(config_path.string().c_str(), &server_config));
    std::filesystem::remove(config_path);
}

TEST(ConfigLoader, ReportsMissingFile)
{
    ServerConfig server_config = make_default_server_config();
    EXPECT_FALSE(config_load_from_file("/nonexistent/path/that/does/not/exist.ini",
                                       &server_config));
}
