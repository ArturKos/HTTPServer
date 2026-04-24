#ifndef HTTP_SERVER_CGI_HANDLER_H
#define HTTP_SERVER_CGI_HANDLER_H

#include <stdbool.h>

#include "http_server/connection_io.h"
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
 */
bool cgi_handler_is_cgi_request(const char* request_path);

/**
 * @brief Execute a CGI script and relay its output to the client.
 */
int cgi_handler_execute(ConnectionContext* connection,
                        const HttpRequest* request,
                        const char* script_path,
                        const char* client_ip,
                        const char* raw_request_body,
                        size_t raw_request_body_size);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_CGI_HANDLER_H */
