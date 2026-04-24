#ifndef HTTP_SERVER_SERVER_H
#define HTTP_SERVER_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file server.h
 * @brief Top-level server configuration and main loop.
 */

/** Maximum length of the document root path stored in the config. */
#define SERVER_CONFIG_PATH_MAX 256

/**
 * @brief Server runtime configuration.
 */
typedef struct ServerConfig {
    uint16_t listen_port;                                    /**< TCP port to listen on. */
    char     document_root[SERVER_CONFIG_PATH_MAX];          /**< Document root directory. */
    char     log_file_path[SERVER_CONFIG_PATH_MAX];          /**< File backend log path. */
    bool     use_syslog_backend;                             /**< Use syslog instead of a file. */
    size_t   log_max_file_size_bytes;                        /**< File rotation threshold. */
    int      log_max_rotated_files;                          /**< Number of rotated files kept. */

    bool     use_tls;                                        /**< Serve TLS on the listener. */
    char     tls_certificate_path[SERVER_CONFIG_PATH_MAX];   /**< PEM server certificate path. */
    char     tls_private_key_path[SERVER_CONFIG_PATH_MAX];   /**< PEM private key path. */
} ServerConfig;

/**
 * @brief Run the server main accept/fork loop until SIGINT/SIGTERM.
 *
 * Registers signal handlers for graceful shutdown and child reaping. The function
 * blocks for the lifetime of the server.
 *
 * @param server_config Validated server configuration (must not be NULL).
 * @return EXIT_SUCCESS on a graceful shutdown, EXIT_FAILURE on initialisation error.
 */
int server_run(const ServerConfig* server_config);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_SERVER_H */
