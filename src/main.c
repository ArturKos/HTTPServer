#include "http_server/logger.h"
#include "http_server/server.h"
#include "http_server/socket_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char* program_name)
{
    fprintf(stderr,
            "Usage: %s <port> <document_root> [log_target]\n"
            "  port          TCP port to listen on (1-65535)\n"
            "  document_root Path to the directory with files to serve\n"
            "  log_target    Optional: file path, or \"syslog\" to use syslog(3)\n"
            "                (default: httpserver.log)\n",
            program_name);
}

int main(int argument_count, char* argument_values[])
{
    if (argument_count < 3 || argument_count > 4) {
        print_usage(argument_values[0]);
        return EXIT_FAILURE;
    }

    ServerConfig server_config;
    memset(&server_config, 0, sizeof(server_config));
    server_config.log_max_file_size_bytes = LOGGER_DEFAULT_MAX_FILE_SIZE_BYTES;
    server_config.log_max_rotated_files   = LOGGER_DEFAULT_MAX_ROTATED_FILES;

    if (!parse_tcp_port(argument_values[1], &server_config.listen_port)) {
        fprintf(stderr, "Invalid port: %s\n", argument_values[1]);
        return EXIT_FAILURE;
    }

    const char* document_root_argument = argument_values[2];
    if (strlen(document_root_argument) + 1 > sizeof(server_config.document_root)) {
        fprintf(stderr, "Document root path is too long\n");
        return EXIT_FAILURE;
    }
    strcpy(server_config.document_root, document_root_argument);

    const char* log_target_argument = (argument_count == 4)
                                      ? argument_values[3]
                                      : "httpserver.log";

    if (strcmp(log_target_argument, "syslog") == 0) {
        server_config.use_syslog_backend = true;
    } else {
        server_config.use_syslog_backend = false;
        if (strlen(log_target_argument) + 1 > sizeof(server_config.log_file_path)) {
            fprintf(stderr, "Log file path is too long\n");
            return EXIT_FAILURE;
        }
        strcpy(server_config.log_file_path, log_target_argument);
    }

    return server_run(&server_config);
}
