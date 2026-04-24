#include "http_server/server.h"
#include "http_server/socket_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char* program_name)
{
    fprintf(stderr,
            "Usage: %s <port> <document_root> [log_file]\n"
            "  port          TCP port to listen on (1-65535)\n"
            "  document_root Path to the directory with files to serve\n"
            "  log_file      Optional access log file path (default: httpserver.log)\n",
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

    const char* log_file_argument = (argument_count == 4)
                                    ? argument_values[3]
                                    : "httpserver.log";
    if (strlen(log_file_argument) + 1 > sizeof(server_config.log_file_path)) {
        fprintf(stderr, "Log file path is too long\n");
        return EXIT_FAILURE;
    }
    strcpy(server_config.log_file_path, log_file_argument);

    return server_run(&server_config);
}
