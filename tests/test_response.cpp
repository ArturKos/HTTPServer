#include <gtest/gtest.h>

#include "http_server/response.h"

#include <array>
#include <string>
#include <unistd.h>
#include <sys/socket.h>

namespace {

int create_connected_socket_pair(int pair_fds[2])
{
    return socketpair(AF_UNIX, SOCK_STREAM, 0, pair_fds);
}

std::string drain_socket(int socket_fd)
{
    std::string accumulated_output;
    std::array<char, 1024> read_buffer{};
    for (;;) {
        ssize_t bytes_read = read(socket_fd, read_buffer.data(), read_buffer.size());
        if (bytes_read <= 0) break;
        accumulated_output.append(read_buffer.data(), static_cast<size_t>(bytes_read));
    }
    return accumulated_output;
}

}  // namespace

TEST(ResponseHeaders, WritesExpectedStatusLineAndHeaders)
{
    int socket_pair_fds[2];
    ASSERT_EQ(create_connected_socket_pair(socket_pair_fds), 0);

    ASSERT_TRUE(response_write_headers(socket_pair_fds[1], 200,
                                       "text/plain; charset=utf-8", 42, false));
    close(socket_pair_fds[1]);

    const std::string received_output = drain_socket(socket_pair_fds[0]);
    close(socket_pair_fds[0]);

    EXPECT_NE(received_output.find("HTTP/1.1 200 OK\r\n"), std::string::npos);
    EXPECT_NE(received_output.find("Content-Type: text/plain; charset=utf-8\r\n"), std::string::npos);
    EXPECT_NE(received_output.find("Content-Length: 42\r\n"), std::string::npos);
    EXPECT_NE(received_output.find("Connection: close\r\n"), std::string::npos);
    EXPECT_NE(received_output.find("\r\n\r\n"), std::string::npos);
}

TEST(ResponseHeaders, EmitsKeepAliveWhenRequested)
{
    int socket_pair_fds[2];
    ASSERT_EQ(create_connected_socket_pair(socket_pair_fds), 0);

    ASSERT_TRUE(response_write_headers(socket_pair_fds[1], 200,
                                       "text/html", 7, true));
    close(socket_pair_fds[1]);

    const std::string received_output = drain_socket(socket_pair_fds[0]);
    close(socket_pair_fds[0]);

    EXPECT_NE(received_output.find("Connection: keep-alive\r\n"), std::string::npos);
}

TEST(ResponseError, ContainsStatusCodeInBody)
{
    int socket_pair_fds[2];
    ASSERT_EQ(create_connected_socket_pair(socket_pair_fds), 0);

    ASSERT_TRUE(response_write_error(socket_pair_fds[1], 404));
    close(socket_pair_fds[1]);

    const std::string received_output = drain_socket(socket_pair_fds[0]);
    close(socket_pair_fds[0]);

    EXPECT_NE(received_output.find("HTTP/1.1 404 Not Found\r\n"), std::string::npos);
    EXPECT_NE(received_output.find("<h1>404 Not Found</h1>"), std::string::npos);
}
