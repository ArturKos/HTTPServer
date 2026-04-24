#ifndef HTTP_SERVER_PATH_UTILS_H
#define HTTP_SERVER_PATH_UTILS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file path_utils.h
 * @brief Helpers for URL decoding, path safety checks and filesystem resolution.
 */

/**
 * @brief Decode a percent-encoded URL path into a plain string.
 *
 * Converts "%HH" sequences into bytes and '+' characters into spaces.
 * On malformed input (e.g. "%X") the function returns false and the contents
 * of @p decoded_buffer are unspecified.
 *
 * @param encoded_url         Input URL path (NUL-terminated).
 * @param decoded_buffer      Output buffer that receives the decoded path.
 * @param decoded_buffer_size Size of @p decoded_buffer in bytes.
 * @return true on success, false on malformed input or insufficient buffer.
 */
bool url_decode(const char* encoded_url,
                char* decoded_buffer,
                size_t decoded_buffer_size);

/**
 * @brief Verify that a request path cannot escape the document root.
 *
 * Rejects absolute paths, paths containing a "../" component (or ending with
 * "/..") and paths with embedded NUL bytes.
 *
 * @param request_path Decoded request path (NUL-terminated).
 * @return true when the path stays within the document root, false otherwise.
 */
bool is_request_path_safe(const char* request_path);

/**
 * @brief Split a path into its query-string component.
 *
 * The path portion is written into @p path_buffer, the query string (without the
 * leading '?') into @p query_buffer. Either component may end up empty.
 *
 * @param url                Full URL path (may contain '?').
 * @param path_buffer        Output buffer for the path part.
 * @param path_buffer_size   Size of @p path_buffer.
 * @param query_buffer       Output buffer for the query part.
 * @param query_buffer_size  Size of @p query_buffer.
 * @return true on success, false when any buffer is too small.
 */
bool split_path_and_query(const char* url,
                          char* path_buffer,
                          size_t path_buffer_size,
                          char* query_buffer,
                          size_t query_buffer_size);

/**
 * @brief Extract the lower-case file extension (without the dot) from a path.
 *
 * If the path has no extension the output buffer is set to an empty string.
 *
 * @param file_path               Path whose extension should be extracted.
 * @param extension_buffer        Output buffer for the extension.
 * @param extension_buffer_size   Size of @p extension_buffer.
 * @return true on success, false when the buffer is too small.
 */
bool extract_file_extension(const char* file_path,
                            char* extension_buffer,
                            size_t extension_buffer_size);

/**
 * @brief Join document root with a request path into an absolute filesystem path.
 *
 * When the request path ends with '/', the string "index.html" is appended so
 * that directory requests resolve to a default document.
 *
 * @param document_root            Path to the document root (must not be NULL).
 * @param request_path             Safe request path (must not be NULL).
 * @param resolved_path_buffer     Output buffer.
 * @param resolved_path_buffer_size Size of the output buffer.
 * @return true on success, false when the output buffer is too small.
 */
bool resolve_document_path(const char* document_root,
                           const char* request_path,
                           char* resolved_path_buffer,
                           size_t resolved_path_buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_PATH_UTILS_H */
