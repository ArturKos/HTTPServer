#ifndef HTTP_SERVER_SERVER_H
#define HTTP_SERVER_SERVER_H

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
    uint16_t listen_port;                          /**< TCP port to listen on. */
    char     document_root[SERVER_CONFIG_PATH_MAX];/**< Absolute or relative document root. */
    char     log_file_path[SERVER_CONFIG_PATH_MAX];/**< Path of the access log file. */
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
