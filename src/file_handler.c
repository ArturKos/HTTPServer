#include "http_server/file_handler.h"

#include "http_server/mime.h"
#include "http_server/path_utils.h"
#include "http_server/range_utils.h"
#include "http_server/response.h"
#include "http_server/status_codes.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int send_file_contents(int client_socket, FILE* file_stream, size_t bytes_to_send)
{
    char file_chunk_buffer[8192];
    size_t remaining_bytes = bytes_to_send;
    while (remaining_bytes > 0) {
        const size_t requested_read = remaining_bytes < sizeof(file_chunk_buffer)
                                      ? remaining_bytes
                                      : sizeof(file_chunk_buffer);
        size_t bytes_read = fread(file_chunk_buffer, 1, requested_read, file_stream);
        if (bytes_read == 0) {
            if (ferror(file_stream)) return -1;
            break;
        }
        if (response_write_body(client_socket, file_chunk_buffer, bytes_read) < 0) {
            return -1;
        }
        remaining_bytes -= bytes_read;
    }
    return 0;
}

int file_handler_serve(int client_socket,
                       const HttpRequest* request,
                       const char* resolved_path,
                       bool keep_alive_enabled)
{
    struct stat file_stat_buffer;
    if (stat(resolved_path, &file_stat_buffer) != 0) {
        response_write_error(client_socket, HTTP_STATUS_NOT_FOUND);
        return HTTP_STATUS_NOT_FOUND;
    }

    if (!S_ISREG(file_stat_buffer.st_mode)) {
        response_write_error(client_socket, HTTP_STATUS_FORBIDDEN);
        return HTTP_STATUS_FORBIDDEN;
    }

    FILE* file_stream = fopen(resolved_path, "rb");
    if (file_stream == NULL) {
        const int status_code = (errno == EACCES)
                                ? HTTP_STATUS_FORBIDDEN
                                : HTTP_STATUS_NOT_FOUND;
        response_write_error(client_socket, status_code);
        return status_code;
    }

    char extension_buffer[16];
    extract_file_extension(resolved_path, extension_buffer, sizeof(extension_buffer));
    const char* content_type = mime_get_content_type(extension_buffer);
    const size_t total_file_size = (size_t)file_stat_buffer.st_size;

    ByteRange requested_byte_range;
    RangeParseResult range_parse_result = range_header_parse(request->range_header,
                                                             total_file_size,
                                                             &requested_byte_range);

    if (range_parse_result == RANGE_PARSE_NOT_SATISFIABLE) {
        fclose(file_stream);
        response_write_error(client_socket, HTTP_STATUS_RANGE_NOT_SATISFIABLE);
        return HTTP_STATUS_RANGE_NOT_SATISFIABLE;
    }

    if (range_parse_result == RANGE_PARSE_OK) {
        if (fseek(file_stream, (long)requested_byte_range.first_byte_offset, SEEK_SET) != 0) {
            fclose(file_stream);
            response_write_error(client_socket, HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return HTTP_STATUS_INTERNAL_SERVER_ERROR;
        }
        if (!response_write_partial_content_headers(client_socket,
                                                    content_type,
                                                    requested_byte_range.first_byte_offset,
                                                    requested_byte_range.last_byte_offset,
                                                    total_file_size,
                                                    keep_alive_enabled)) {
            fclose(file_stream);
            return HTTP_STATUS_INTERNAL_SERVER_ERROR;
        }
        if (request->method == HTTP_METHOD_HEAD) {
            fclose(file_stream);
            return HTTP_STATUS_PARTIAL_CONTENT;
        }
        const size_t slice_length = requested_byte_range.last_byte_offset
                                    - requested_byte_range.first_byte_offset + 1;
        const int transfer_result = send_file_contents(client_socket, file_stream, slice_length);
        fclose(file_stream);
        return (transfer_result == 0) ? HTTP_STATUS_PARTIAL_CONTENT
                                      : HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }

    if (!response_write_headers(client_socket,
                                HTTP_STATUS_OK,
                                content_type,
                                total_file_size,
                                keep_alive_enabled)) {
        fclose(file_stream);
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }

    if (request->method == HTTP_METHOD_HEAD) {
        fclose(file_stream);
        return HTTP_STATUS_OK;
    }

    const int transfer_result = send_file_contents(client_socket, file_stream, total_file_size);
    fclose(file_stream);
    return (transfer_result == 0) ? HTTP_STATUS_OK : HTTP_STATUS_INTERNAL_SERVER_ERROR;
}
