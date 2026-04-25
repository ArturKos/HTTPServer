#include "http_server/socket_utils.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int create_listening_socket(uint16_t listen_port, int backlog)
{
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        return -1;
    }

    int reuse_address_flag = 1;
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse_address_flag, sizeof(reuse_address_flag)) < 0) {
        const int saved_errno = errno;
        close(server_socket_fd);
        errno = saved_errno;
        return -1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family      = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port        = htons(listen_port);

    if (bind(server_socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        const int saved_errno = errno;
        close(server_socket_fd);
        errno = saved_errno;
        return -1;
    }

    if (listen(server_socket_fd, backlog) < 0) {
        const int saved_errno = errno;
        close(server_socket_fd);
        errno = saved_errno;
        return -1;
    }

    return server_socket_fd;
}

int try_take_systemd_listening_socket(void)
{
    const char* listen_pid_string = getenv("LISTEN_PID");
    const char* listen_fds_string = getenv("LISTEN_FDS");
    if (listen_pid_string == NULL || listen_fds_string == NULL) {
        return -1;
    }

    char* parse_end_pointer = NULL;
    long announced_pid_value = strtol(listen_pid_string, &parse_end_pointer, 10);
    if (parse_end_pointer == listen_pid_string || *parse_end_pointer != '\0') {
        return -1;
    }
    if (announced_pid_value != (long)getpid()) {
        return -1;
    }

    parse_end_pointer = NULL;
    long announced_fd_count = strtol(listen_fds_string, &parse_end_pointer, 10);
    if (parse_end_pointer == listen_fds_string || *parse_end_pointer != '\0') {
        return -1;
    }
    if (announced_fd_count < 1) {
        return -1;
    }

    /* sd_listen_fds(3) reserves fds 3..(3+LISTEN_FDS-1) for inherited sockets. */
    return 3;
}

bool parse_tcp_port(const char* port_string, uint16_t* out_port)
{
    if (port_string == NULL || out_port == NULL) {
        return false;
    }

    char* parse_end_pointer = NULL;
    errno = 0;
    long parsed_port_value = strtol(port_string, &parse_end_pointer, 10);
    if (errno != 0 || parse_end_pointer == port_string || *parse_end_pointer != '\0') {
        return false;
    }
    if (parsed_port_value < 1 || parsed_port_value > 65535) {
        return false;
    }
    *out_port = (uint16_t)parsed_port_value;
    return true;
}
