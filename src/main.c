#include "http_server/config_loader.h"
#include "http_server/logger.h"
#include "http_server/server.h"
#include "http_server/socket_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char* program_name)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s --config <file>\n"
            "  %s <port> <document_root> [log_target] [tls_cert tls_key]\n"
            "\n"
            "Configuration file (INI) keys:\n"
            "  port, document_root, log_target, log_max_file_size,\n"
            "  log_max_rotated, tls_certificate, tls_private_key\n",
            program_name, program_name);
}

static bool copy_bounded(char* destination, size_t destination_size, const char* source)
{
    const size_t source_length = strlen(source);
    if (source_length + 1 > destination_size) return false;
    memcpy(destination, source, source_length + 1);
    return true;
}

static bool parse_positional_arguments(int argument_count,
                                       char* argument_values[],
                                       ServerConfig* server_config)
{
    if (argument_count < 3 || argument_count > 6 || argument_count == 5) {
        return false;
    }

    if (!parse_tcp_port(argument_values[1], &server_config->listen_port)) {
        fprintf(stderr, "Invalid port: %s\n", argument_values[1]);
        return false;
    }
    if (!copy_bounded(server_config->document_root,
                      sizeof(server_config->document_root),
                      argument_values[2])) {
        fprintf(stderr, "Document root path is too long\n");
        return false;
    }

    const char* log_target_argument = (argument_count >= 4)
                                      ? argument_values[3]
                                      : "httpserver.log";
    if (strcmp(log_target_argument, "syslog") == 0) {
        server_config->use_syslog_backend = true;
    } else {
        server_config->use_syslog_backend = false;
        if (!copy_bounded(server_config->log_file_path,
                          sizeof(server_config->log_file_path),
                          log_target_argument)) {
            fprintf(stderr, "Log file path is too long\n");
            return false;
        }
    }

    if (argument_count == 6) {
        server_config->use_tls = true;
        if (!copy_bounded(server_config->tls_certificate_path,
                          sizeof(server_config->tls_certificate_path),
                          argument_values[4])
            || !copy_bounded(server_config->tls_private_key_path,
                             sizeof(server_config->tls_private_key_path),
                             argument_values[5])) {
            fprintf(stderr, "TLS certificate or key path is too long\n");
            return false;
        }
    }
    return true;
}

int main(int argument_count, char* argument_values[])
{
    if (argument_count < 2) {
        print_usage(argument_values[0]);
        return EXIT_FAILURE;
    }

    ServerConfig server_config;
    memset(&server_config, 0, sizeof(server_config));
    server_config.log_max_file_size_bytes = LOGGER_DEFAULT_MAX_FILE_SIZE_BYTES;
    server_config.log_max_rotated_files   = LOGGER_DEFAULT_MAX_ROTATED_FILES;

    if (strcmp(argument_values[1], "--config") == 0) {
        if (argument_count != 3) {
            print_usage(argument_values[0]);
            return EXIT_FAILURE;
        }
        if (!copy_bounded(server_config.log_file_path,
                          sizeof(server_config.log_file_path),
                          "httpserver.log")) {
            return EXIT_FAILURE;
        }
        if (!config_load_from_file(argument_values[2], &server_config)) {
            return EXIT_FAILURE;
        }
        if (server_config.listen_port == 0) {
            fprintf(stderr, "config: 'port' is required\n");
            return EXIT_FAILURE;
        }
        if (server_config.document_root[0] == '\0') {
            fprintf(stderr, "config: 'document_root' is required\n");
            return EXIT_FAILURE;
        }
    } else {
        if (!parse_positional_arguments(argument_count, argument_values, &server_config)) {
            print_usage(argument_values[0]);
            return EXIT_FAILURE;
        }
    }

    return server_run(&server_config);
}
