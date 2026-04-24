#include "http_server/logger.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static char            g_log_file_path[512] = "httpserver.log";
static pthread_mutex_t g_log_file_mutex     = PTHREAD_MUTEX_INITIALIZER;

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

bool logger_initialize(const char* log_file_path)
{
    if (log_file_path == NULL || log_file_path[0] == '\0') {
        return false;
    }
    const size_t path_length = strlen(log_file_path);
    if (path_length + 1 > sizeof(g_log_file_path)) {
        return false;
    }
    pthread_mutex_lock(&g_log_file_mutex);
    memcpy(g_log_file_path, log_file_path, path_length + 1);
    pthread_mutex_unlock(&g_log_file_mutex);
    return true;
}

static void append_line_to_log_file(const char* log_line)
{
    pthread_mutex_lock(&g_log_file_mutex);
    FILE* log_file_stream = fopen(g_log_file_path, "a");
    if (log_file_stream != NULL) {
        fputs(log_line, log_file_stream);
        fputc('\n', log_file_stream);
        fclose(log_file_stream);
    }
    pthread_mutex_unlock(&g_log_file_mutex);
}

void logger_log_request(const char* client_ip,
                        int process_id,
                        int status_code,
                        const char* request_path)
{
    char timestamp_buffer[32];
    format_timestamp(timestamp_buffer, sizeof(timestamp_buffer));

    char log_line_buffer[1024];
    snprintf(log_line_buffer, sizeof(log_line_buffer),
             "[%s] [pid=%d] client=%s status=%d path=\"%s\"",
             timestamp_buffer,
             process_id,
             client_ip != NULL ? client_ip : "-",
             status_code,
             request_path != NULL ? request_path : "-");
    append_line_to_log_file(log_line_buffer);
}

void logger_log_message(const char* message)
{
    if (message == NULL) {
        return;
    }
    char timestamp_buffer[32];
    format_timestamp(timestamp_buffer, sizeof(timestamp_buffer));

    char log_line_buffer[1024];
    snprintf(log_line_buffer, sizeof(log_line_buffer),
             "[%s] [pid=%d] %s",
             timestamp_buffer, (int)getpid(), message);
    append_line_to_log_file(log_line_buffer);
}
