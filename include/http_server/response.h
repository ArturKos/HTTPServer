#ifndef HTTP_SERVER_RESPONSE_H
#define HTTP_SERVER_RESPONSE_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "http_server/connection_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file response.h
 * @brief Helpers for writing HTTP responses to a client connection.
 */

/** Default HTTP server identification string sent in the Server header. */
#define HTTP_SERVER_NAME "HTTPServer/1.0"

/**
 * @brief Write a full HTTP response header block.
 *
 * The header block terminates with an empty line (CRLF CRLF). When @p content_type
 * is NULL, "application/octet-stream" is used. When @p content_length is SIZE_MAX
 * the Content-Length header is omitted.
 *
 * @param connection        Connection context.
 * @param status_code       HTTP status code.
 * @param content_type      Value of the Content-Type header or NULL.
 * @param content_length    Size of the body that will follow.
 * @param keep_alive_enabled Whether to emit "Connection: keep-alive".
 * @return true on success, false when the write fails.
 */
bool response_write_headers(ConnectionContext* connection,
                            int status_code,
                            const char* content_type,
                            size_t content_length,
                            bool keep_alive_enabled);

/**
 * @brief Write a 206 Partial Content response header block.
 */
bool response_write_partial_content_headers(ConnectionContext* connection,
                                            const char* content_type,
                                            size_t first_byte_offset,
                                            size_t last_byte_offset,
                                            size_t total_resource_size,
                                            bool keep_alive_enabled);

/**
 * @brief Write an error response with a small HTML body.
 */
bool response_write_error(ConnectionContext* connection, int status_code);

/**
 * @brief Write arbitrary body bytes, retrying on short writes.
 */
ssize_t response_write_body(ConnectionContext* connection,
                            const void* body_buffer,
                            size_t body_size);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_RESPONSE_H */
