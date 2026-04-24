#include "http_server/response.h"

#include "http_server/status_codes.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static ssize_t write_all_bytes(int socket_fd, const void* buffer, size_t buffer_size)
{
    const unsigned char* byte_cursor = (const unsigned char*)buffer;
    size_t remaining_bytes = buffer_size;
    while (remaining_bytes > 0) {
        ssize_t bytes_written = send(socket_fd, byte_cursor, remaining_bytes, MSG_NOSIGNAL);
        if (bytes_written < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (bytes_written == 0) {
            return -1;
        }
        byte_cursor     += (size_t)bytes_written;
        remaining_bytes -= (size_t)bytes_written;
    }
    return (ssize_t)buffer_size;
}

bool response_write_headers(int client_socket,
                            int status_code,
                            const char* content_type,
                            size_t content_length)
{
    const char* reason_phrase = http_status_get_reason_phrase(status_code);
    const char* effective_content_type = (content_type != NULL)
                                         ? content_type
                                         : "application/octet-stream";

    char header_buffer[1024];
    int header_length;
    if (content_length == (size_t)-1) {
        header_length = snprintf(header_buffer, sizeof(header_buffer),
                                 "HTTP/1.1 %d %s\r\n"
                                 "Server: %s\r\n"
                                 "Content-Type: %s\r\n"
                                 "Connection: close\r\n"
                                 "\r\n",
                                 status_code, reason_phrase,
                                 HTTP_SERVER_NAME,
                                 effective_content_type);
    } else {
        header_length = snprintf(header_buffer, sizeof(header_buffer),
                                 "HTTP/1.1 %d %s\r\n"
                                 "Server: %s\r\n"
                                 "Content-Type: %s\r\n"
                                 "Content-Length: %zu\r\n"
                                 "Connection: close\r\n"
                                 "\r\n",
                                 status_code, reason_phrase,
                                 HTTP_SERVER_NAME,
                                 effective_content_type,
                                 content_length);
    }

    if (header_length < 0 || (size_t)header_length >= sizeof(header_buffer)) {
        return false;
    }
    return write_all_bytes(client_socket, header_buffer, (size_t)header_length) >= 0;
}

bool response_write_error(int client_socket, int status_code)
{
    const char* reason_phrase = http_status_get_reason_phrase(status_code);

    char body_buffer[512];
    int body_length = snprintf(body_buffer, sizeof(body_buffer),
                               "<!DOCTYPE html><html><head><title>%d %s</title></head>"
                               "<body><h1>%d %s</h1><p>%s.</p></body></html>",
                               status_code, reason_phrase,
                               status_code, reason_phrase,
                               reason_phrase);
    if (body_length < 0 || (size_t)body_length >= sizeof(body_buffer)) {
        return false;
    }

    if (!response_write_headers(client_socket, status_code,
                                "text/html; charset=utf-8", (size_t)body_length)) {
        return false;
    }
    return write_all_bytes(client_socket, body_buffer, (size_t)body_length) >= 0;
}

ssize_t response_write_body(int client_socket, const void* body_buffer, size_t body_size)
{
    return write_all_bytes(client_socket, body_buffer, body_size);
}
