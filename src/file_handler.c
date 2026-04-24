#include "http_server/file_handler.h"

#include "http_server/mime.h"
#include "http_server/path_utils.h"
#include "http_server/response.h"
#include "http_server/status_codes.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int send_file_contents(int client_socket, FILE* file_stream)
{
    char file_chunk_buffer[8192];
    while (!feof(file_stream)) {
        size_t bytes_read = fread(file_chunk_buffer, 1, sizeof(file_chunk_buffer), file_stream);
        if (bytes_read == 0) {
            if (ferror(file_stream)) {
                return -1;
            }
            break;
        }
        if (response_write_body(client_socket, file_chunk_buffer, bytes_read) < 0) {
            return -1;
        }
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

    if (!response_write_headers(client_socket,
                                HTTP_STATUS_OK,
                                content_type,
                                (size_t)file_stat_buffer.st_size,
                                keep_alive_enabled)) {
        fclose(file_stream);
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }

    if (request->method == HTTP_METHOD_HEAD) {
        fclose(file_stream);
        return HTTP_STATUS_OK;
    }

    const int transfer_result = send_file_contents(client_socket, file_stream);
    fclose(file_stream);
    if (transfer_result != 0) {
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }
    return HTTP_STATUS_OK;
}
