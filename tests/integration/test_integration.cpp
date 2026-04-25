#include <gtest/gtest.h>

#include <curl/curl.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#ifndef HTTP_SERVER_BINARY_PATH
#define HTTP_SERVER_BINARY_PATH "./httpserver"
#endif

namespace {

size_t accumulate_response_body(char* buffer,
                                 size_t element_size,
                                 size_t element_count,
                                 void* user_data)
{
    std::string* accumulated_body = static_cast<std::string*>(user_data);
    accumulated_body->append(buffer, element_size * element_count);
    return element_size * element_count;
}

int find_free_tcp_port()
{
    int probe_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in probe_address{};
    probe_address.sin_family      = AF_INET;
    probe_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    probe_address.sin_port        = 0;
    bind(probe_socket, reinterpret_cast<sockaddr*>(&probe_address), sizeof(probe_address));
    socklen_t address_length = sizeof(probe_address);
    getsockname(probe_socket, reinterpret_cast<sockaddr*>(&probe_address), &address_length);
    int assigned_port = ntohs(probe_address.sin_port);
    close(probe_socket);
    return assigned_port;
}

class HttpServerFixture : public ::testing::Test {
protected:
    pid_t server_process_id = -1;
    int   server_port        = 0;
    std::filesystem::path document_root_path;

    void SetUp() override {
        server_port = find_free_tcp_port();

        document_root_path = std::filesystem::temp_directory_path()
                             / ("httpserver_it_" + std::to_string(getpid())
                                + "_" + std::to_string(server_port));
        std::filesystem::create_directories(document_root_path);

        std::ofstream(document_root_path / "index.html")
            << "<html>hello</html>";

        std::ofstream big_file_stream(document_root_path / "big.bin",
                                      std::ios::binary);
        for (int byte_index = 0; byte_index < 1024; ++byte_index) {
            big_file_stream.put(static_cast<char>(byte_index & 0xff));
        }
        big_file_stream.close();

        const std::string log_file_path = (document_root_path / "server.log").string();
        const std::string port_string   = std::to_string(server_port);

        server_process_id = fork();
        ASSERT_NE(server_process_id, -1);
        if (server_process_id == 0) {
            execl(HTTP_SERVER_BINARY_PATH,
                  HTTP_SERVER_BINARY_PATH,
                  port_string.c_str(),
                  document_root_path.c_str(),
                  log_file_path.c_str(),
                  static_cast<char*>(nullptr));
            _exit(127);
        }

        wait_until_server_accepts_connections();
    }

    void TearDown() override {
        if (server_process_id > 0) {
            kill(server_process_id, SIGTERM);
            int status_placeholder = 0;
            waitpid(server_process_id, &status_placeholder, 0);
        }
        std::error_code ignored_error_code;
        std::filesystem::remove_all(document_root_path, ignored_error_code);
    }

    void wait_until_server_accepts_connections() {
        for (int retry_index = 0; retry_index < 50; ++retry_index) {
            int probe_socket = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in server_address{};
            server_address.sin_family      = AF_INET;
            server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            server_address.sin_port        = htons(static_cast<uint16_t>(server_port));
            if (connect(probe_socket,
                        reinterpret_cast<sockaddr*>(&server_address),
                        sizeof(server_address)) == 0) {
                close(probe_socket);
                return;
            }
            close(probe_socket);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        FAIL() << "Server never became ready on port " << server_port;
    }

    std::string build_url(const std::string& path_component) const {
        return "http://127.0.0.1:" + std::to_string(server_port) + path_component;
    }
};

}  // namespace

TEST_F(HttpServerFixture, ServesStaticIndex)
{
    CURL* curl_handle = curl_easy_init();
    std::string response_body;
    long        response_status = 0;

    curl_easy_setopt(curl_handle, CURLOPT_URL, build_url("/").c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, accumulate_response_body);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response_body);

    ASSERT_EQ(curl_easy_perform(curl_handle), CURLE_OK);
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_status);
    EXPECT_EQ(response_status, 200);
    EXPECT_NE(response_body.find("hello"), std::string::npos);
    curl_easy_cleanup(curl_handle);
}

TEST_F(HttpServerFixture, HeadRequestReturnsNoBody)
{
    CURL* curl_handle = curl_easy_init();
    std::string response_body;
    long        response_status = 0;

    curl_easy_setopt(curl_handle, CURLOPT_URL, build_url("/").c_str());
    curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, accumulate_response_body);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response_body);

    ASSERT_EQ(curl_easy_perform(curl_handle), CURLE_OK);
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_status);
    EXPECT_EQ(response_status, 200);
    EXPECT_TRUE(response_body.empty());
    curl_easy_cleanup(curl_handle);
}

TEST_F(HttpServerFixture, RangeRequestReturnsPartialContent)
{
    CURL* curl_handle = curl_easy_init();
    std::string response_body;
    long        response_status = 0;

    curl_easy_setopt(curl_handle, CURLOPT_URL, build_url("/big.bin").c_str());
    curl_easy_setopt(curl_handle, CURLOPT_RANGE, "100-199");
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, accumulate_response_body);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response_body);

    ASSERT_EQ(curl_easy_perform(curl_handle), CURLE_OK);
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_status);
    EXPECT_EQ(response_status, 206);
    EXPECT_EQ(response_body.size(), 100u);
    EXPECT_EQ(static_cast<unsigned char>(response_body[0]), 100u);
    EXPECT_EQ(static_cast<unsigned char>(response_body[99]), 199u);
    curl_easy_cleanup(curl_handle);
}

TEST_F(HttpServerFixture, MissingResourceReturns404)
{
    CURL* curl_handle = curl_easy_init();
    long  response_status = 0;

    curl_easy_setopt(curl_handle, CURLOPT_URL, build_url("/no-such-file").c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, accumulate_response_body);
    std::string unused_body;
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &unused_body);

    ASSERT_EQ(curl_easy_perform(curl_handle), CURLE_OK);
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_status);
    EXPECT_EQ(response_status, 404);
    curl_easy_cleanup(curl_handle);
}

TEST_F(HttpServerFixture, KeepAliveReusesTcpConnection)
{
    CURL* curl_handle = curl_easy_init();
    std::string response_body;
    long        response_status   = 0;
    long        new_connection_total = 0;

    for (int request_index = 0; request_index < 3; ++request_index) {
        response_body.clear();
        curl_easy_setopt(curl_handle, CURLOPT_URL, build_url("/").c_str());
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, accumulate_response_body);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response_body);
        ASSERT_EQ(curl_easy_perform(curl_handle), CURLE_OK);
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_status);
        EXPECT_EQ(response_status, 200);

        long connections_opened_for_this_request = 0;
        curl_easy_getinfo(curl_handle, CURLINFO_NUM_CONNECTS,
                          &connections_opened_for_this_request);
        new_connection_total += connections_opened_for_this_request;
    }

    EXPECT_EQ(new_connection_total, 1);
    curl_easy_cleanup(curl_handle);
}

TEST_F(HttpServerFixture, PathTraversalIsRejected)
{
    CURL* curl_handle = curl_easy_init();
    long  response_status = 0;
    std::string unused_body;

    curl_easy_setopt(curl_handle, CURLOPT_URL, build_url("/../etc/passwd").c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PATH_AS_IS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, accumulate_response_body);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &unused_body);

    ASSERT_EQ(curl_easy_perform(curl_handle), CURLE_OK);
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_status);
    EXPECT_EQ(response_status, 403);
    curl_easy_cleanup(curl_handle);
}

int main(int argument_count, char** argument_values)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    ::testing::InitGoogleTest(&argument_count, argument_values);
    const int test_exit_code = RUN_ALL_TESTS();
    curl_global_cleanup();
    return test_exit_code;
}
