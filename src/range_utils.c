#include "http_server/range_utils.h"

#include <stdlib.h>
#include <string.h>

static const char kRangeUnitPrefix[] = "bytes=";

static bool string_starts_with(const char* text, const char* prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static bool parse_nonnegative_integer(const char* text, size_t* out_value)
{
    if (*text == '\0') return false;
    char* parse_end_pointer = NULL;
    long long parsed = strtoll(text, &parse_end_pointer, 10);
    if (*parse_end_pointer != '\0') return false;
    if (parsed < 0) return false;
    *out_value = (size_t)parsed;
    return true;
}

RangeParseResult range_header_parse(const char* range_header_value,
                                    size_t total_resource_size,
                                    ByteRange* out_byte_range)
{
    if (out_byte_range == NULL) return RANGE_PARSE_UNSUPPORTED;
    if (range_header_value == NULL || range_header_value[0] == '\0') {
        return RANGE_PARSE_ABSENT;
    }
    if (!string_starts_with(range_header_value, kRangeUnitPrefix)) {
        return RANGE_PARSE_UNSUPPORTED;
    }

    const char* range_spec = range_header_value + sizeof(kRangeUnitPrefix) - 1;
    if (strchr(range_spec, ',') != NULL) {
        return RANGE_PARSE_UNSUPPORTED;
    }

    const char* dash_position = strchr(range_spec, '-');
    if (dash_position == NULL) {
        return RANGE_PARSE_UNSUPPORTED;
    }

    char first_part[32] = {0};
    char second_part[32] = {0};
    const size_t first_part_length = (size_t)(dash_position - range_spec);
    const size_t second_part_length = strlen(dash_position + 1);
    if (first_part_length >= sizeof(first_part)
        || second_part_length >= sizeof(second_part)) {
        return RANGE_PARSE_UNSUPPORTED;
    }
    memcpy(first_part, range_spec, first_part_length);
    memcpy(second_part, dash_position + 1, second_part_length);

    if (total_resource_size == 0) {
        return RANGE_PARSE_NOT_SATISFIABLE;
    }

    size_t first_offset = 0;
    size_t last_offset = 0;

    if (first_part[0] == '\0') {
        size_t suffix_length = 0;
        if (!parse_nonnegative_integer(second_part, &suffix_length)) {
            return RANGE_PARSE_UNSUPPORTED;
        }
        if (suffix_length == 0) {
            return RANGE_PARSE_NOT_SATISFIABLE;
        }
        if (suffix_length >= total_resource_size) {
            first_offset = 0;
        } else {
            first_offset = total_resource_size - suffix_length;
        }
        last_offset = total_resource_size - 1;
    } else {
        if (!parse_nonnegative_integer(first_part, &first_offset)) {
            return RANGE_PARSE_UNSUPPORTED;
        }
        if (first_offset >= total_resource_size) {
            return RANGE_PARSE_NOT_SATISFIABLE;
        }
        if (second_part[0] == '\0') {
            last_offset = total_resource_size - 1;
        } else {
            if (!parse_nonnegative_integer(second_part, &last_offset)) {
                return RANGE_PARSE_UNSUPPORTED;
            }
            if (last_offset < first_offset) {
                return RANGE_PARSE_UNSUPPORTED;
            }
            if (last_offset >= total_resource_size) {
                last_offset = total_resource_size - 1;
            }
        }
    }

    out_byte_range->first_byte_offset = first_offset;
    out_byte_range->last_byte_offset  = last_offset;
    return RANGE_PARSE_OK;
}
