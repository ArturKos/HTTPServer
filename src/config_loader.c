#include "http_server/config_loader.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_LINE_MAX 1024

static char* trim_in_place(char* text)
{
    while (*text != '\0' && isspace((unsigned char)*text)) ++text;
    if (*text == '\0') return text;
    char* trailing_cursor = text + strlen(text) - 1;
    while (trailing_cursor > text && isspace((unsigned char)*trailing_cursor)) {
        *trailing_cursor-- = '\0';
    }
    return text;
}

static bool is_comment_or_blank(const char* line)
{
    if (line[0] == '\0') return true;
    if (line[0] == '#' || line[0] == ';') return true;
    if (line[0] == '/' && line[1] == '/') return true;
    return false;
}

static bool parse_unsigned_long(const char* text, unsigned long* out_value)
{
    char* parse_end_pointer = NULL;
    errno = 0;
    unsigned long parsed_value = strtoul(text, &parse_end_pointer, 10);
    if (errno != 0 || parse_end_pointer == text || *parse_end_pointer != '\0') {
        return false;
    }
    *out_value = parsed_value;
    return true;
}

static bool copy_string_field(char* destination,
                              size_t destination_size,
                              const char* source_value)
{
    const size_t source_length = strlen(source_value);
    if (source_length + 1 > destination_size) return false;
    memcpy(destination, source_value, source_length + 1);
    return true;
}

static bool apply_key_value_pair(const char* configuration_key,
                                 const char* configuration_value,
                                 ServerConfig* out_config)
{
    if (strcmp(configuration_key, "port") == 0) {
        unsigned long parsed_port_value = 0;
        if (!parse_unsigned_long(configuration_value, &parsed_port_value)
            || parsed_port_value == 0 || parsed_port_value > 65535) {
            fprintf(stderr, "config: invalid port \"%s\"\n", configuration_value);
            return false;
        }
        out_config->listen_port = (uint16_t)parsed_port_value;
        return true;
    }
    if (strcmp(configuration_key, "document_root") == 0) {
        return copy_string_field(out_config->document_root,
                                 sizeof(out_config->document_root),
                                 configuration_value);
    }
    if (strcmp(configuration_key, "log_target") == 0) {
        if (strcmp(configuration_value, "syslog") == 0) {
            out_config->use_syslog_backend = true;
            return true;
        }
        out_config->use_syslog_backend = false;
        return copy_string_field(out_config->log_file_path,
                                 sizeof(out_config->log_file_path),
                                 configuration_value);
    }
    if (strcmp(configuration_key, "log_max_file_size") == 0) {
        unsigned long parsed_size_value = 0;
        if (!parse_unsigned_long(configuration_value, &parsed_size_value)) {
            fprintf(stderr, "config: invalid log_max_file_size \"%s\"\n", configuration_value);
            return false;
        }
        out_config->log_max_file_size_bytes = (size_t)parsed_size_value;
        return true;
    }
    if (strcmp(configuration_key, "log_max_rotated") == 0) {
        unsigned long parsed_count_value = 0;
        if (!parse_unsigned_long(configuration_value, &parsed_count_value)
            || parsed_count_value == 0 || parsed_count_value > 1000) {
            fprintf(stderr, "config: invalid log_max_rotated \"%s\"\n", configuration_value);
            return false;
        }
        out_config->log_max_rotated_files = (int)parsed_count_value;
        return true;
    }
    if (strcmp(configuration_key, "tls_certificate") == 0) {
        out_config->use_tls = true;
        return copy_string_field(out_config->tls_certificate_path,
                                 sizeof(out_config->tls_certificate_path),
                                 configuration_value);
    }
    if (strcmp(configuration_key, "tls_private_key") == 0) {
        out_config->use_tls = true;
        return copy_string_field(out_config->tls_private_key_path,
                                 sizeof(out_config->tls_private_key_path),
                                 configuration_value);
    }

    fprintf(stderr, "config: unknown key \"%s\" (ignored)\n", configuration_key);
    return true;
}

bool config_load_from_file(const char* config_file_path,
                           ServerConfig* out_config)
{
    if (config_file_path == NULL || out_config == NULL) return false;

    FILE* config_file_stream = fopen(config_file_path, "r");
    if (config_file_stream == NULL) {
        fprintf(stderr, "config: cannot open %s: %s\n",
                config_file_path, strerror(errno));
        return false;
    }

    char raw_line_buffer[CONFIG_LINE_MAX];
    int  current_line_number = 0;
    bool overall_success = true;

    while (fgets(raw_line_buffer, sizeof(raw_line_buffer), config_file_stream) != NULL) {
        ++current_line_number;
        char* trimmed_line = trim_in_place(raw_line_buffer);
        if (is_comment_or_blank(trimmed_line)) continue;
        if (trimmed_line[0] == '[') continue;

        char* equals_position = strchr(trimmed_line, '=');
        if (equals_position == NULL) {
            fprintf(stderr, "config: line %d missing '='\n", current_line_number);
            overall_success = false;
            continue;
        }
        *equals_position = '\0';
        char* configuration_key   = trim_in_place(trimmed_line);
        char* configuration_value = trim_in_place(equals_position + 1);

        if (!apply_key_value_pair(configuration_key, configuration_value, out_config)) {
            fprintf(stderr, "config: error at line %d\n", current_line_number);
            overall_success = false;
        }
    }

    fclose(config_file_stream);
    return overall_success;
}
