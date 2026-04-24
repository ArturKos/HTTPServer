#ifndef HTTP_SERVER_RANGE_UTILS_H
#define HTTP_SERVER_RANGE_UTILS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file range_utils.h
 * @brief Parsing of a single HTTP Range header (RFC 7233).
 *
 * Only single-range requests are supported; multi-range "bytes=0-10,20-30"
 * syntax is rejected (and should be served as 200 OK with the full body by
 * the caller).
 */

/**
 * @brief Inclusive byte range within a resource.
 */
typedef struct ByteRange {
    size_t first_byte_offset; /**< Offset of the first byte to send. */
    size_t last_byte_offset;  /**< Offset of the last byte to send (inclusive). */
} ByteRange;

/**
 * @brief Outcome of parsing a Range header.
 */
typedef enum RangeParseResult {
    RANGE_PARSE_ABSENT           = 0, /**< Header missing or empty. */
    RANGE_PARSE_OK               = 1, /**< Range parsed and satisfiable. */
    RANGE_PARSE_UNSUPPORTED      = 2, /**< Syntactically valid but unsupported (e.g. multi-range). */
    RANGE_PARSE_NOT_SATISFIABLE  = 3  /**< Range out of bounds for the resource. */
} RangeParseResult;

/**
 * @brief Parse a Range header against a known total resource size.
 *
 * Supported forms (units MUST be "bytes"):
 *   - "bytes=A-B"   (A <= B, both inclusive)
 *   - "bytes=A-"    (A..end)
 *   - "bytes=-N"    (last N bytes)
 *
 * @param range_header_value Raw header value without the "Range:" prefix.
 * @param total_resource_size Full size of the underlying resource.
 * @param out_byte_range     Populated when the result is RANGE_PARSE_OK.
 * @return RangeParseResult describing how the header should be treated.
 */
RangeParseResult range_header_parse(const char* range_header_value,
                                    size_t total_resource_size,
                                    ByteRange* out_byte_range);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_RANGE_UTILS_H */
