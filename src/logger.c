#include "http_server/logger.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define LOGGER_PATH_MAX 512

static LoggerBackend   g_active_backend                       = LOGGER_BACKEND_FILE;
static char            g_log_file_path[LOGGER_PATH_MAX]       = "httpserver.log";
static size_t          g_max_file_size_bytes                  = LOGGER_DEFAULT_MAX_FILE_SIZE_BYTES;
static int             g_max_rotated_files                    = LOGGER_DEFAULT_MAX_ROTATED_FILES;
static bool            g_syslog_opened                        = false;
static pthread_mutex_t g_logger_mutex                         = PTHREAD_MUTEX_INITIALIZER;

static void format_timestamp(char* timestamp_buffer, size_t timestamp_buffer_size)
{
    time_t current_time = time(NULL);
    struct tm broken_down_time;
    if (localtime_r(&current_time, &broken_down_time) == NULL) {
        snprintf(timestamp_buffer, timestamp_buffer_size, "0000-00-00 00:00:00");
        return;
    }
    strftime(timestamp_buffer, timestamp_buffer_size,
             "%Y-%m-%d %H:%M:%S", &broken_down_time);
}

bool logger_initialize_file(const char* log_file_path,
                            size_t max_file_size_bytes,
                            int max_rotated_files)
{
    if (log_file_path == NULL || log_file_path[0] == '\0') return false;
    if (max_rotated_files < 1) return false;

    const size_t path_length = strlen(log_file_path);
    if (path_length + 1 > LOGGER_PATH_MAX) return false;

    pthread_mutex_lock(&g_logger_mutex);
    if (g_syslog_opened) {
        closelog();
        g_syslog_opened = false;
    }
    memcpy(g_log_file_path, log_file_path, path_length + 1);
    g_max_file_size_bytes = max_file_size_bytes;
    g_max_rotated_files   = max_rotated_files;
    g_active_backend      = LOGGER_BACKEND_FILE;
    pthread_mutex_unlock(&g_logger_mutex);
    return true;
}

bool logger_initialize_syslog(const char* syslog_identifier)
{
    if (syslog_identifier == NULL || syslog_identifier[0] == '\0') return false;

    pthread_mutex_lock(&g_logger_mutex);
    if (g_syslog_opened) {
        closelog();
    }
    openlog(syslog_identifier, LOG_PID | LOG_CONS, LOG_DAEMON);
    g_syslog_opened  = true;
    g_active_backend = LOGGER_BACKEND_SYSLOG;
    pthread_mutex_unlock(&g_logger_mutex);
    return true;
}

void logger_shutdown(void)
{
    pthread_mutex_lock(&g_logger_mutex);
    if (g_syslog_opened) {
        closelog();
        g_syslog_opened = false;
    }
    pthread_mutex_unlock(&g_logger_mutex);
}

static void rotate_log_files_locked(void)
{
    char source_path_buffer[LOGGER_PATH_MAX + 16];
    char destination_path_buffer[LOGGER_PATH_MAX + 16];

    snprintf(destination_path_buffer, sizeof(destination_path_buffer),
             "%s.%d", g_log_file_path, g_max_rotated_files);
    unlink(destination_path_buffer);

    for (int rotation_index = g_max_rotated_files - 1; rotation_index >= 1; --rotation_index) {
        snprintf(source_path_buffer, sizeof(source_path_buffer),
                 "%s.%d", g_log_file_path, rotation_index);
        snprintf(destination_path_buffer, sizeof(destination_path_buffer),
                 "%s.%d", g_log_file_path, rotation_index + 1);
        rename(source_path_buffer, destination_path_buffer);
    }

    snprintf(destination_path_buffer, sizeof(destination_path_buffer),
             "%s.1", g_log_file_path);
    rename(g_log_file_path, destination_path_buffer);
}

static void append_line_to_file_backend_locked(const char* log_line)
{
    if (g_max_file_size_bytes > 0) {
        struct stat file_stat_buffer;
        if (stat(g_log_file_path, &file_stat_buffer) == 0
            && (size_t)file_stat_buffer.st_size >= g_max_file_size_bytes) {
            rotate_log_files_locked();
        }
    }

    FILE* log_file_stream = fopen(g_log_file_path, "a");
    if (log_file_stream != NULL) {
        fputs(log_line, log_file_stream);
        fputc('\n', log_file_stream);
        fclose(log_file_stream);
    }
}

static void emit_log_line(const char* log_line)
{
    pthread_mutex_lock(&g_logger_mutex);
    if (g_active_backend == LOGGER_BACKEND_SYSLOG) {
        syslog(LOG_INFO, "%s", log_line);
    } else {
        append_line_to_file_backend_locked(log_line);
    }
    pthread_mutex_unlock(&g_logger_mutex);
}

void logger_log_request(const char* client_ip,
                        int worker_id,
                        int status_code,
                        const char* request_path)
{
    char timestamp_buffer[32];
    format_timestamp(timestamp_buffer, sizeof(timestamp_buffer));

    char log_line_buffer[1024];
    snprintf(log_line_buffer, sizeof(log_line_buffer),
             "[%s] [worker=%d] client=%s status=%d path=\"%s\"",
             timestamp_buffer,
             worker_id,
             client_ip != NULL ? client_ip : "-",
             status_code,
             request_path != NULL ? request_path : "-");
    emit_log_line(log_line_buffer);
}

void logger_log_message(const char* message)
{
    if (message == NULL) return;
    char timestamp_buffer[32];
    format_timestamp(timestamp_buffer, sizeof(timestamp_buffer));

    char log_line_buffer[1024];
    snprintf(log_line_buffer, sizeof(log_line_buffer),
             "[%s] %s", timestamp_buffer, message);
    emit_log_line(log_line_buffer);
}
