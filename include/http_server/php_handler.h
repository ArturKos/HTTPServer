#ifndef HTTP_SERVER_PHP_HANDLER_H
#define HTTP_SERVER_PHP_HANDLER_H

#include <stdbool.h>

#include "http_server/connection_io.h"
#include "http_server/request.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file php_handler.h
 * @brief PHP script execution via php-cgi.
 */

/** Return whether @p request_path ends with ".php". */
bool php_handler_is_php_request(const char* request_path);

/** Run a PHP script through php-cgi and relay its output to the client. */
int php_handler_execute(ConnectionContext* connection,
                        const HttpRequest* request,
                        const char* script_path,
                        const char* client_ip,
                        const char* raw_request_body,
                        size_t raw_request_body_size);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_PHP_HANDLER_H */
