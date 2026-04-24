#ifndef HTTP_SERVER_RESPONSE_H
#define HTTP_SERVER_RESPONSE_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file response.h
 * @brief Helpers for writing HTTP responses to a client socket.
 */

/** Default HTTP server identification string sent in the Server header. */
#define HTTP_SERVER_NAME "HTTPServer/1.0"

/**
 * @brief Write a full HTTP response header block to the client socket.
 *
 * The header block terminates with an empty line (CRLF CRLF). When @p content_type
 * is NULL, "application/octet-stream" is used. When @p content_length is SIZE_MAX
 * the Content-Length header is omitted (useful for chunked / streaming responses).
 *
 * @param client_socket Connected client socket.
 * @param status_code   HTTP status code (e.g. 200, 404).
 * @param content_type  Value of the Content-Type header or NULL.
 * @param content_length Size of the body that will follow the headers.
 * @return true on success, false when the socket write fails.
 */
bool response_write_headers(int client_socket,
                            int status_code,
                            const char* content_type,
                            size_t content_length);

/**
 * @brief Write an error response with a small HTML body.
 *
 * @param client_socket Connected client socket.
 * @param status_code   HTTP status code (>= 400 expected).
 * @return true on success, false on write failure.
 */
bool response_write_error(int client_socket, int status_code);

/**
 * @brief Write the full contents of @p body_buffer followed by no body framing.
 *
 * This helper retries on short writes and EINTR.
 *
 * @param client_socket Connected client socket.
 * @param body_buffer   Buffer containing the payload.
 * @param body_size     Number of bytes to write.
 * @return Number of bytes written or -1 on error.
 */
ssize_t response_write_body(int client_socket,
                            const void* body_buffer,
                            size_t body_size);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_RESPONSE_H */
