#ifndef HTTP_SERVER_FILE_HANDLER_H
#define HTTP_SERVER_FILE_HANDLER_H

#include "http_server/connection_io.h"
#include "http_server/request.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file file_handler.h
 * @brief Static file serving.
 */

/**
 * @brief Serve a static file to a client connection.
 *
 * @param connection         Client connection context.
 * @param request            Parsed HTTP request.
 * @param resolved_path      Absolute filesystem path of the resource.
 * @param keep_alive_enabled Forward the keep-alive decision into the response headers.
 * @return HTTP status code reported to the client.
 */
int file_handler_serve(ConnectionContext* connection,
                       const HttpRequest* request,
                       const char* resolved_path,
                       bool keep_alive_enabled);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_FILE_HANDLER_H */
