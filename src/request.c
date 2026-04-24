#include "http_server/request.h"

#include "http_server/path_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

const char* http_method_to_string(HttpMethod method)
{
    switch (method) {
        case HTTP_METHOD_GET:  return "GET";
        case HTTP_METHOD_HEAD: return "HEAD";
        case HTTP_METHOD_POST: return "POST";
        default:               return "UNKNOWN";
    }
}

static HttpMethod parse_method_token(const char* method_token, size_t method_token_length)
{
    if (method_token_length == 3 && strncmp(method_token, "GET", 3) == 0) {
        return HTTP_METHOD_GET;
    }
    if (method_token_length == 4 && strncmp(method_token, "HEAD", 4) == 0) {
        return HTTP_METHOD_HEAD;
    }
    if (method_token_length == 4 && strncmp(method_token, "POST", 4) == 0) {
        return HTTP_METHOD_POST;
    }
    return HTTP_METHOD_UNKNOWN;
}

static bool copy_bounded_string(char* destination,
                                size_t destination_size,
                                const char* source,
                                size_t source_length)
{
    if (source_length + 1 > destination_size) {
        return false;
    }
    memcpy(destination, source, source_length);
    destination[source_length] = '\0';
    return true;
}

static void parse_header_value(const char* header_line,
                               size_t header_line_length,
                               const char* header_name,
                               char* value_buffer,
                               size_t value_buffer_size)
{
    const size_t header_name_length = strlen(header_name);
    if (header_line_length <= header_name_length + 1) {
        return;
    }
    if (strncasecmp(header_line, header_name, header_name_length) != 0) {
        return;
    }
    if (header_line[header_name_length] != ':') {
        return;
    }

    const char* value_start = header_line + header_name_length + 1;
    while (*value_start == ' ' || *value_start == '\t') {
        ++value_start;
    }
    size_t value_length = (size_t)((header_line + header_line_length) - value_start);
    while (value_length > 0 && (value_start[value_length - 1] == ' '
                                 || value_start[value_length - 1] == '\t'
                                 || value_start[value_length - 1] == '\r')) {
        --value_length;
    }
    if (value_length + 1 > value_buffer_size) {
        value_length = value_buffer_size - 1;
    }
    memcpy(value_buffer, value_start, value_length);
    value_buffer[value_length] = '\0';
}

bool http_request_parse(const char* raw_request,
                        size_t raw_request_size,
                        HttpRequest* out_request)
{
    if (raw_request == NULL || out_request == NULL || raw_request_size == 0) {
        return false;
    }

    memset(out_request, 0, sizeof(*out_request));

    const char* line_end_position = memchr(raw_request, '\n', raw_request_size);
    if (line_end_position == NULL) {
        return false;
    }
    size_t request_line_length = (size_t)(line_end_position - raw_request);
    if (request_line_length > 0 && raw_request[request_line_length - 1] == '\r') {
        --request_line_length;
    }

    const char* first_space = memchr(raw_request, ' ', request_line_length);
    if (first_space == NULL) {
        return false;
    }
    const size_t method_token_length = (size_t)(first_space - raw_request);
    out_request->method = parse_method_token(raw_request, method_token_length);
    if (out_request->method == HTTP_METHOD_UNKNOWN) {
        return false;
    }

    const char* url_start = first_space + 1;
    const char* second_space = memchr(url_start, ' ',
                                      request_line_length - method_token_length - 1);
    if (second_space == NULL) {
        return false;
    }
    const size_t url_length = (size_t)(second_space - url_start);

    char raw_url_buffer[HTTP_REQUEST_PATH_MAX + HTTP_REQUEST_QUERY_MAX];
    if (!copy_bounded_string(raw_url_buffer, sizeof(raw_url_buffer), url_start, url_length)) {
        return false;
    }

    char encoded_path_buffer[HTTP_REQUEST_PATH_MAX];
    if (!split_path_and_query(raw_url_buffer,
                              encoded_path_buffer, sizeof(encoded_path_buffer),
                              out_request->query_string, sizeof(out_request->query_string))) {
        return false;
    }

    if (!url_decode(encoded_path_buffer, out_request->path, sizeof(out_request->path))) {
        return false;
    }

    const char* version_start = second_space + 1;
    const size_t version_length = request_line_length - method_token_length - 1 - url_length - 1;
    if (!copy_bounded_string(out_request->version, sizeof(out_request->version),
                             version_start, version_length)) {
        return false;
    }

    const char* header_cursor = line_end_position + 1;
    const char* request_end = raw_request + raw_request_size;
    while (header_cursor < request_end) {
        const char* header_line_end = memchr(header_cursor, '\n',
                                             (size_t)(request_end - header_cursor));
        if (header_line_end == NULL) {
            break;
        }
        size_t header_line_length = (size_t)(header_line_end - header_cursor);
        if (header_line_length > 0 && header_cursor[header_line_length - 1] == '\r') {
            --header_line_length;
        }
        if (header_line_length == 0) {
            break;
        }

        char content_length_buffer[32] = {0};
        parse_header_value(header_cursor, header_line_length,
                           "Content-Type",
                           out_request->content_type, sizeof(out_request->content_type));
        parse_header_value(header_cursor, header_line_length,
                           "Content-Length",
                           content_length_buffer, sizeof(content_length_buffer));
        if (content_length_buffer[0] != '\0') {
            char* parse_end_pointer = NULL;
            long parsed_length_value = strtol(content_length_buffer, &parse_end_pointer, 10);
            if (parse_end_pointer != content_length_buffer && parsed_length_value >= 0) {
                out_request->content_length = (size_t)parsed_length_value;
            }
        }

        header_cursor = header_line_end + 1;
    }

    return true;
}
