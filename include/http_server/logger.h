#ifndef HTTP_SERVER_LOGGER_H
#define HTTP_SERVER_LOGGER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file logger.h
 * @brief Append-only request logger with file (rotating) or syslog backends.
 */

/**
 * @brief Available logging backends.
 */
typedef enum LoggerBackend {
    LOGGER_BACKEND_FILE   = 0, /**< Log to a rotating local file. */
    LOGGER_BACKEND_SYSLOG = 1  /**< Log via the system syslog facility. */
} LoggerBackend;

/** Default rotation threshold (10 MiB) for the file backend. */
#define LOGGER_DEFAULT_MAX_FILE_SIZE_BYTES (10u * 1024u * 1024u)

/** Default number of rotated log files kept on disk (httpserver.log.1 .. .5). */
#define LOGGER_DEFAULT_MAX_ROTATED_FILES   5

/**
 * @brief Configure the file backend with size-based rotation.
 *
 * When the log file grows past @p max_file_size_bytes, it is rotated:
 * file.N is removed, file.{N-1} -> file.N, ..., file -> file.1.
 *
 * @param log_file_path        Path to the active log file.
 * @param max_file_size_bytes  Rotate once the file exceeds this size; 0 disables rotation.
 * @param max_rotated_files    Number of rotated files kept (must be >= 1).
 * @return true on success.
 */
bool logger_initialize_file(const char* log_file_path,
                            size_t max_file_size_bytes,
                            int max_rotated_files);

/**
 * @brief Configure the syslog backend.
 *
 * @param syslog_identifier Identifier passed to openlog(); must remain valid
 *                          for the process lifetime.
 * @return true on success.
 */
bool logger_initialize_syslog(const char* syslog_identifier);

/**
 * @brief Release any backend resources (closes syslog).
 */
void logger_shutdown(void);

/**
 * @brief Append a formatted access-log entry.
 *
 * @param client_ip    Dotted-quad client address string.
 * @param worker_id    Identifier of the worker that handled the request.
 * @param status_code  HTTP status code returned to the client.
 * @param request_path Requested resource path.
 */
void logger_log_request(const char* client_ip,
                        int worker_id,
                        int status_code,
                        const char* request_path);

/**
 * @brief Append a free-form diagnostic message.
 *
 * @param message NUL-terminated message.
 */
void logger_log_message(const char* message);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_LOGGER_H */
