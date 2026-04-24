#ifndef HTTP_SERVER_LOGGER_H
#define HTTP_SERVER_LOGGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file logger.h
 * @brief Minimal append-only request logger.
 */

/**
 * @brief Initialise the logger with the target log file path.
 *
 * Safe to call multiple times; subsequent calls replace the previous path.
 *
 * @param log_file_path Path to the log file (must remain valid for the process lifetime).
 * @return true on success, false on allocation or validation error.
 */
bool logger_initialize(const char* log_file_path);

/**
 * @brief Append a formatted access-log entry.
 *
 * The entry is built as:
 *   [YYYY-MM-DD HH:MM:SS] [pid=N] client=IP status=CODE path="PATH"
 *
 * @param client_ip   Dotted-quad client address string.
 * @param process_id  PID that handled the request.
 * @param status_code HTTP status code returned to the client.
 * @param request_path Requested resource path.
 */
void logger_log_request(const char* client_ip,
                        int process_id,
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
