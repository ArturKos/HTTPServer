#ifndef HTTP_SERVER_MIME_H
#define HTTP_SERVER_MIME_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file mime.h
 * @brief Mapping from file extensions to MIME content types.
 */

/**
 * @brief Resolve a MIME content type string from a file extension.
 *
 * The lookup is case-insensitive and does not accept a leading dot.
 * Unknown extensions fall back to "application/octet-stream".
 *
 * @param file_extension File extension without leading dot (e.g. "html", "png").
 *                       May be NULL.
 * @return Statically-allocated MIME type string (never NULL).
 */
const char* mime_get_content_type(const char* file_extension);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_MIME_H */
