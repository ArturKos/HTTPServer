#include <gtest/gtest.h>

#include "http_server/socket_utils.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

namespace {

void clear_systemd_socket_activation_environment()
{
    unsetenv("LISTEN_PID");
    unsetenv("LISTEN_FDS");
}

}  // namespace

TEST(ParseTcpPort, AcceptsValidPort)
{
    uint16_t parsed_port = 0;
    ASSERT_TRUE(parse_tcp_port("8080", &parsed_port));
    EXPECT_EQ(parsed_port, 8080);
}

TEST(ParseTcpPort, RejectsNonNumeric)
{
    uint16_t parsed_port = 0;
    EXPECT_FALSE(parse_tcp_port("eighty", &parsed_port));
    EXPECT_FALSE(parse_tcp_port("80x",    &parsed_port));
    EXPECT_FALSE(parse_tcp_port("",       &parsed_port));
}

TEST(ParseTcpPort, RejectsOutOfRange)
{
    uint16_t parsed_port = 0;
    EXPECT_FALSE(parse_tcp_port("0",     &parsed_port));
    EXPECT_FALSE(parse_tcp_port("65536", &parsed_port));
    EXPECT_FALSE(parse_tcp_port("-1",    &parsed_port));
}

TEST(SystemdSocketActivation, ReturnsMinusOneWhenEnvironmentMissing)
{
    clear_systemd_socket_activation_environment();
    EXPECT_EQ(try_take_systemd_listening_socket(), -1);
}

TEST(SystemdSocketActivation, ReturnsMinusOneWhenPidMismatches)
{
    clear_systemd_socket_activation_environment();
    setenv("LISTEN_PID", "999999", 1);
    setenv("LISTEN_FDS", "1", 1);
    EXPECT_EQ(try_take_systemd_listening_socket(), -1);
    clear_systemd_socket_activation_environment();
}

TEST(SystemdSocketActivation, ReturnsFirstFdWhenEnvironmentMatches)
{
    clear_systemd_socket_activation_environment();
    const std::string current_pid_string = std::to_string(getpid());
    setenv("LISTEN_PID", current_pid_string.c_str(), 1);
    setenv("LISTEN_FDS", "2", 1);
    EXPECT_EQ(try_take_systemd_listening_socket(), 3);
    clear_systemd_socket_activation_environment();
}

TEST(SystemdSocketActivation, ReturnsMinusOneWhenFdCountIsZero)
{
    clear_systemd_socket_activation_environment();
    const std::string current_pid_string = std::to_string(getpid());
    setenv("LISTEN_PID", current_pid_string.c_str(), 1);
    setenv("LISTEN_FDS", "0", 1);
    EXPECT_EQ(try_take_systemd_listening_socket(), -1);
    clear_systemd_socket_activation_environment();
}
