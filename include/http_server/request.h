#ifndef HTTP_SERVER_REQUEST_H
#define HTTP_SERVER_REQUEST_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file request.h
 * @brief HTTP request parsing.
 */

/** Maximum length of a parsed request path (including terminator). */
#define HTTP_REQUEST_PATH_MAX   1024
/** Maximum length of a parsed query string (including terminator). */
#define HTTP_REQUEST_QUERY_MAX  1024
/** Maximum length of the HTTP version string (e.g. "HTTP/1.1"). */
#define HTTP_VERSION_MAX        16
/** Maximum length of a request header value stored by the parser. */
#define HTTP_HEADER_VALUE_MAX   256

/**
 * @brief HTTP methods explicitly recognised by the server.
 */
typedef enum HttpMethod {
    HTTP_METHOD_UNKNOWN = 0, /**< Method could not be identified. */
    HTTP_METHOD_GET,         /**< GET. */
    HTTP_METHOD_HEAD,        /**< HEAD. */
    HTTP_METHOD_POST         /**< POST. */
} HttpMethod;

/**
 * @brief Parsed HTTP request line and commonly-used headers.
 */
typedef struct HttpRequest {
    HttpMethod method;                                /**< Parsed HTTP method. */
    char       path[HTTP_REQUEST_PATH_MAX];           /**< Decoded request path. */
    char       query_string[HTTP_REQUEST_QUERY_MAX];  /**< Query string (no leading '?'). */
    char       version[HTTP_VERSION_MAX];             /**< HTTP version string. */
    char       content_type[HTTP_HEADER_VALUE_MAX];   /**< Content-Type header (may be empty). */
    size_t     content_length;                        /**< Parsed Content-Length or 0. */
} HttpRequest;

/**
 * @brief Convert an @ref HttpMethod enum value into its canonical string form.
 *
 * @param method Method value.
 * @return Upper-case ASCII method name or "UNKNOWN".
 */
const char* http_method_to_string(HttpMethod method);

/**
 * @brief Parse the request line and a small set of headers from a raw request buffer.
 *
 * The parser is deliberately conservative: it rejects requests with embedded NUL
 * bytes, unreasonably long lines and unsupported versions. On success, @p out_request
 * has been zero-initialised and filled with the decoded path, query string,
 * content-type and content-length.
 *
 * @param raw_request      Raw request bytes (NUL-terminated).
 * @param raw_request_size Number of bytes in @p raw_request (excluding the terminator).
 * @param out_request      Output request structure.
 * @return true on successful parse, false otherwise.
 */
bool http_request_parse(const char* raw_request,
                        size_t raw_request_size,
                        HttpRequest* out_request);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_REQUEST_H */
