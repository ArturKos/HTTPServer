#include "http_server/path_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int hex_digit_to_int(char hex_character)
{
    if (hex_character >= '0' && hex_character <= '9') return hex_character - '0';
    if (hex_character >= 'a' && hex_character <= 'f') return 10 + (hex_character - 'a');
    if (hex_character >= 'A' && hex_character <= 'F') return 10 + (hex_character - 'A');
    return -1;
}

bool url_decode(const char* encoded_url,
                char* decoded_buffer,
                size_t decoded_buffer_size)
{
    if (encoded_url == NULL || decoded_buffer == NULL || decoded_buffer_size == 0) {
        return false;
    }

    size_t write_index = 0;
    for (size_t read_index = 0; encoded_url[read_index] != '\0'; ++read_index) {
        if (write_index + 1 >= decoded_buffer_size) {
            return false;
        }

        char current_character = encoded_url[read_index];
        if (current_character == '%') {
            int high_nibble = hex_digit_to_int(encoded_url[read_index + 1]);
            int low_nibble  = hex_digit_to_int(encoded_url[read_index + 2]);
            if (high_nibble < 0 || low_nibble < 0) {
                return false;
            }
            decoded_buffer[write_index++] = (char)((high_nibble << 4) | low_nibble);
            read_index += 2;
        } else if (current_character == '+') {
            decoded_buffer[write_index++] = ' ';
        } else {
            decoded_buffer[write_index++] = current_character;
        }
    }
    decoded_buffer[write_index] = '\0';
    return true;
}

bool is_request_path_safe(const char* request_path)
{
    if (request_path == NULL || request_path[0] != '/') {
        return false;
    }

    const size_t path_length = strlen(request_path);
    for (size_t character_index = 0; character_index < path_length; ++character_index) {
        if (request_path[character_index] == '\0') {
            return false;
        }
    }

    const char* traversal_cursor = request_path;
    while ((traversal_cursor = strstr(traversal_cursor, "..")) != NULL) {
        const bool starts_at_boundary = (traversal_cursor == request_path)
                                        || (*(traversal_cursor - 1) == '/');
        const char next_character = *(traversal_cursor + 2);
        const bool ends_at_boundary = (next_character == '\0') || (next_character == '/');
        if (starts_at_boundary && ends_at_boundary) {
            return false;
        }
        ++traversal_cursor;
    }
    return true;
}

bool split_path_and_query(const char* url,
                          char* path_buffer,
                          size_t path_buffer_size,
                          char* query_buffer,
                          size_t query_buffer_size)
{
    if (url == NULL || path_buffer == NULL || query_buffer == NULL
        || path_buffer_size == 0 || query_buffer_size == 0) {
        return false;
    }

    const char* question_mark_position = strchr(url, '?');
    size_t path_length = (question_mark_position != NULL)
                         ? (size_t)(question_mark_position - url)
                         : strlen(url);

    if (path_length + 1 > path_buffer_size) {
        return false;
    }
    memcpy(path_buffer, url, path_length);
    path_buffer[path_length] = '\0';

    if (question_mark_position == NULL) {
        query_buffer[0] = '\0';
        return true;
    }

    const size_t query_length = strlen(question_mark_position + 1);
    if (query_length + 1 > query_buffer_size) {
        return false;
    }
    memcpy(query_buffer, question_mark_position + 1, query_length);
    query_buffer[query_length] = '\0';
    return true;
}

bool extract_file_extension(const char* file_path,
                            char* extension_buffer,
                            size_t extension_buffer_size)
{
    if (file_path == NULL || extension_buffer == NULL || extension_buffer_size == 0) {
        return false;
    }

    const char* last_dot_position = strrchr(file_path, '.');
    const char* last_slash_position = strrchr(file_path, '/');
    if (last_dot_position == NULL || (last_slash_position != NULL
                                      && last_slash_position > last_dot_position)) {
        extension_buffer[0] = '\0';
        return true;
    }

    const char* extension_start = last_dot_position + 1;
    const size_t extension_length = strlen(extension_start);
    if (extension_length + 1 > extension_buffer_size) {
        return false;
    }

    for (size_t character_index = 0; character_index < extension_length; ++character_index) {
        extension_buffer[character_index] = (char)tolower((unsigned char)extension_start[character_index]);
    }
    extension_buffer[extension_length] = '\0';
    return true;
}

bool resolve_document_path(const char* document_root,
                           const char* request_path,
                           char* resolved_path_buffer,
                           size_t resolved_path_buffer_size)
{
    if (document_root == NULL || request_path == NULL || resolved_path_buffer == NULL
        || resolved_path_buffer_size == 0) {
        return false;
    }

    const size_t document_root_length = strlen(document_root);
    const bool root_has_trailing_slash = document_root_length > 0
        && document_root[document_root_length - 1] == '/';
    const bool path_has_leading_slash = request_path[0] == '/';

    const char* separator = (root_has_trailing_slash || !path_has_leading_slash) ? "" : "";
    (void)separator;

    int written_characters;
    if (root_has_trailing_slash && path_has_leading_slash) {
        written_characters = snprintf(resolved_path_buffer, resolved_path_buffer_size,
                                      "%s%s", document_root, request_path + 1);
    } else if (!root_has_trailing_slash && !path_has_leading_slash) {
        written_characters = snprintf(resolved_path_buffer, resolved_path_buffer_size,
                                      "%s/%s", document_root, request_path);
    } else {
        written_characters = snprintf(resolved_path_buffer, resolved_path_buffer_size,
                                      "%s%s", document_root, request_path);
    }

    if (written_characters < 0 || (size_t)written_characters >= resolved_path_buffer_size) {
        return false;
    }

    const size_t resolved_length = (size_t)written_characters;
    if (resolved_length > 0 && resolved_path_buffer[resolved_length - 1] == '/') {
        const char kDefaultIndexFile[] = "index.html";
        if (resolved_length + sizeof(kDefaultIndexFile) > resolved_path_buffer_size) {
            return false;
        }
        memcpy(resolved_path_buffer + resolved_length, kDefaultIndexFile, sizeof(kDefaultIndexFile));
    }

    return true;
}
