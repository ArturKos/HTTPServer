#ifndef HTTP_SERVER_CGI_HANDLER_H
#define HTTP_SERVER_CGI_HANDLER_H

#include <stdbool.h>

#include "http_server/request.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cgi_handler.h
 * @brief CGI/1.1 script execution.
 */

/**
 * @brief Test whether a resolved path refers to an executable CGI script.
 *
 * A path is considered a CGI script when it lives under the "/cgi-bin/" URL
 * prefix. The caller is expected to check the executable bit separately via
 * @c access().
 *
 * @param request_path Decoded request path.
 * @return true if the path should be handled as CGI.
 */
bool cgi_handler_is_cgi_request(const char* request_path);

/**
 * @brief Execute a CGI script and relay its output to the client.
 *
 * Spawns the script via fork/exec, sets the CGI/1.1 environment variables and
 * streams the child's stdout back to the client. For POST requests, the request
 * body read from @p client_socket is piped to the child's stdin.
 *
 * @param client_socket  Connected client socket.
 * @param request        Parsed HTTP request (used for method, query, headers).
 * @param script_path    Absolute path to the CGI executable.
 * @param client_ip      Dotted-quad client IP for REMOTE_ADDR.
 * @param raw_request_body  Optional buffer with already-read POST body.
 * @param raw_request_body_size Size of @p raw_request_body in bytes.
 * @return HTTP status code reported to the client.
 */
int cgi_handler_execute(int client_socket,
                        const HttpRequest* request,
                        const char* script_path,
                        const char* client_ip,
                        const char* raw_request_body,
                        size_t raw_request_body_size);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_CGI_HANDLER_H */
