#ifndef HTTP_SERVER_CONFIG_LOADER_H
#define HTTP_SERVER_CONFIG_LOADER_H

#include <stdbool.h>

#include "http_server/server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file config_loader.h
 * @brief Loads server configuration from a simple INI-style key=value file.
 *
 * Supported syntax:
 * @code
 *   # comment line (also ; or //)
 *   [server]                  ; sections are accepted but ignored
 *   port               = 8080
 *   document_root      = ./www
 *   log_target         = httpserver.log    ; or "syslog"
 *   log_max_file_size  = 10485760
 *   log_max_rotated    = 5
 *   tls_certificate    = /etc/ssl/cert.pem
 *   tls_private_key    = /etc/ssl/key.pem
 * @endcode
 *
 * Whitespace around keys, equals signs and values is trimmed. Quotes around
 * values are not required and not stripped.
 */

/**
 * @brief Populate @p out_config from the contents of @p config_file_path.
 *
 * Defaults already present in @p out_config are preserved when the file does
 * not override them. Unknown keys are reported on stderr but do not abort the
 * load.
 *
 * @param config_file_path Path to the configuration file.
 * @param out_config       Configuration to populate (must not be NULL).
 * @return true on success, false on I/O failure or invalid value.
 */
bool config_load_from_file(const char* config_file_path,
                           ServerConfig* out_config);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_CONFIG_LOADER_H */
