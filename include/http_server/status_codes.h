#ifndef HTTP_SERVER_STATUS_CODES_H
#define HTTP_SERVER_STATUS_CODES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file status_codes.h
 * @brief HTTP status codes supported by the server and helpers to stringify them.
 */

/**
 * @brief Subset of HTTP status codes used by the server.
 */
typedef enum HttpStatusCode {
    HTTP_STATUS_OK                     = 200, /**< Successful response. */
    HTTP_STATUS_PARTIAL_CONTENT        = 206, /**< Partial content for a Range request. */
    HTTP_STATUS_BAD_REQUEST            = 400, /**< Malformed request. */
    HTTP_STATUS_FORBIDDEN              = 403, /**< Client is not allowed to access the resource. */
    HTTP_STATUS_NOT_FOUND              = 404, /**< Requested resource was not found. */
    HTTP_STATUS_METHOD_NOT_ALLOWED     = 405, /**< HTTP method not supported for this resource. */
    HTTP_STATUS_RANGE_NOT_SATISFIABLE  = 416, /**< Range header cannot be satisfied. */
    HTTP_STATUS_INTERNAL_SERVER_ERROR  = 500, /**< Unexpected server error. */
    HTTP_STATUS_NOT_IMPLEMENTED        = 501  /**< Feature (e.g. method) is not implemented. */
} HttpStatusCode;

/**
 * @brief Return the reason-phrase associated with a status code.
 *
 * @param status_code HTTP status code.
 * @return Human-readable reason phrase (never NULL; falls back to "Unknown").
 */
const char* http_status_get_reason_phrase(int status_code);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_STATUS_CODES_H */
