#ifndef HTTP_SERVER_PHP_HANDLER_H
#define HTTP_SERVER_PHP_HANDLER_H

#include <stdbool.h>

#include "http_server/request.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file php_handler.h
 * @brief PHP script execution via php-cgi.
 *
 * The server invokes the system-installed @c php-cgi interpreter in CGI/1.1 mode.
 * The interpreter binary is resolved at runtime from PATH; a missing binary
 * results in a 500 response.
 */

/**
 * @brief Test whether a resolved path refers to a PHP script.
 *
 * @param request_path Decoded request path.
 * @return true when the path ends with ".php".
 */
bool php_handler_is_php_request(const char* request_path);

/**
 * @brief Run a PHP script through php-cgi and relay its output to the client.
 *
 * @param client_socket  Connected client socket.
 * @param request        Parsed HTTP request.
 * @param script_path    Absolute path to the PHP script.
 * @param client_ip      Dotted-quad client IP for REMOTE_ADDR.
 * @param raw_request_body  Optional POST body.
 * @param raw_request_body_size Size of @p raw_request_body.
 * @return HTTP status code reported to the client.
 */
int php_handler_execute(int client_socket,
                        const HttpRequest* request,
                        const char* script_path,
                        const char* client_ip,
                        const char* raw_request_body,
                        size_t raw_request_body_size);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_PHP_HANDLER_H */
