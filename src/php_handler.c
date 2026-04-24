#include "http_server/php_handler.h"

#include "http_server/response.h"
#include "http_server/status_codes.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static const char kPhpCgiBinaryName[] = "php-cgi";

#define PHP_ENVIRONMENT_SLOT_COUNT 16
#define PHP_ENVIRONMENT_VALUE_MAX  1024

bool php_handler_is_php_request(const char* request_path)
{
    if (request_path == NULL) return false;
    const size_t path_length = strlen(request_path);
    if (path_length < 4) return false;
    return strcmp(request_path + path_length - 4, ".php") == 0;
}

static ssize_t write_all_bytes_to_fd(int target_fd, const void* buffer, size_t buffer_size)
{
    const unsigned char* byte_cursor = (const unsigned char*)buffer;
    size_t remaining_bytes = buffer_size;
    while (remaining_bytes > 0) {
        ssize_t bytes_written = write(target_fd, byte_cursor, remaining_bytes);
        if (bytes_written < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        byte_cursor     += (size_t)bytes_written;
        remaining_bytes -= (size_t)bytes_written;
    }
    return (ssize_t)buffer_size;
}

static int relay_child_output_to_client(int child_stdout_fd,
                                        ConnectionContext* connection)
{
    char relay_buffer[8192];
    for (;;) {
        ssize_t bytes_read = read(child_stdout_fd, relay_buffer, sizeof(relay_buffer));
        if (bytes_read < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (bytes_read == 0) return 0;
        if (response_write_body(connection, relay_buffer, (size_t)bytes_read) < 0) {
            return -1;
        }
    }
}

typedef struct PhpEnvironment {
    char  storage[PHP_ENVIRONMENT_SLOT_COUNT][PHP_ENVIRONMENT_VALUE_MAX];
    char* envp[PHP_ENVIRONMENT_SLOT_COUNT + 1];
    size_t entry_count;
} PhpEnvironment;

static void php_environment_add(PhpEnvironment* environment,
                                const char* variable_name,
                                const char* variable_value)
{
    if (environment->entry_count >= PHP_ENVIRONMENT_SLOT_COUNT) return;
    char* destination_slot = environment->storage[environment->entry_count];
    const size_t name_length = strlen(variable_name);
    if (name_length + 2 >= PHP_ENVIRONMENT_VALUE_MAX) return;

    memcpy(destination_slot, variable_name, name_length);
    destination_slot[name_length] = '=';

    const char* effective_value = variable_value != NULL ? variable_value : "";
    const size_t available_for_value = PHP_ENVIRONMENT_VALUE_MAX - name_length - 2;
    const size_t value_length = strlen(effective_value);
    const size_t copy_length = value_length < available_for_value
                                ? value_length
                                : available_for_value;
    memcpy(destination_slot + name_length + 1, effective_value, copy_length);
    destination_slot[name_length + 1 + copy_length] = '\0';

    environment->envp[environment->entry_count] = destination_slot;
    ++environment->entry_count;
    environment->envp[environment->entry_count] = NULL;
}

static void build_php_cgi_environment(PhpEnvironment* environment,
                                      const HttpRequest* request,
                                      const char* script_path,
                                      const char* client_ip)
{
    char content_length_string[32];
    snprintf(content_length_string, sizeof(content_length_string),
             "%zu", request->content_length);

    php_environment_add(environment, "GATEWAY_INTERFACE", "CGI/1.1");
    php_environment_add(environment, "SERVER_PROTOCOL",   "HTTP/1.1");
    php_environment_add(environment, "SERVER_SOFTWARE",   "HTTPServer/1.0");
    php_environment_add(environment, "REQUEST_METHOD",    http_method_to_string(request->method));
    php_environment_add(environment, "QUERY_STRING",      request->query_string);
    php_environment_add(environment, "SCRIPT_FILENAME",   script_path);
    php_environment_add(environment, "SCRIPT_NAME",       request->path);
    php_environment_add(environment, "PATH_TRANSLATED",   script_path);
    php_environment_add(environment, "REMOTE_ADDR",       client_ip != NULL ? client_ip : "0.0.0.0");
    php_environment_add(environment, "REDIRECT_STATUS",   "200");
    php_environment_add(environment, "CONTENT_LENGTH",    content_length_string);
    if (request->content_type[0] != '\0') {
        php_environment_add(environment, "CONTENT_TYPE", request->content_type);
    }
}

int php_handler_execute(ConnectionContext* connection,
                        const HttpRequest* request,
                        const char* script_path,
                        const char* client_ip,
                        const char* raw_request_body,
                        size_t raw_request_body_size)
{
    int stdin_pipe_fds[2];
    int stdout_pipe_fds[2];
    if (pipe(stdin_pipe_fds) < 0 || pipe(stdout_pipe_fds) < 0) {
        response_write_error(connection, HTTP_STATUS_INTERNAL_SERVER_ERROR);
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }

    PhpEnvironment php_environment;
    memset(&php_environment, 0, sizeof(php_environment));
    build_php_cgi_environment(&php_environment, request, script_path, client_ip);

    pid_t child_process_id = fork();
    if (child_process_id < 0) {
        close(stdin_pipe_fds[0]);
        close(stdin_pipe_fds[1]);
        close(stdout_pipe_fds[0]);
        close(stdout_pipe_fds[1]);
        response_write_error(connection, HTTP_STATUS_INTERNAL_SERVER_ERROR);
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }

    if (child_process_id == 0) {
        dup2(stdin_pipe_fds[0], STDIN_FILENO);
        dup2(stdout_pipe_fds[1], STDOUT_FILENO);
        close(stdin_pipe_fds[0]);
        close(stdin_pipe_fds[1]);
        close(stdout_pipe_fds[0]);
        close(stdout_pipe_fds[1]);
        char* const child_argument_vector[] = {(char*)kPhpCgiBinaryName, NULL};
        execvpe(kPhpCgiBinaryName, child_argument_vector, php_environment.envp);
        _exit(127);
    }

    close(stdin_pipe_fds[0]);
    close(stdout_pipe_fds[1]);

    if (raw_request_body != NULL && raw_request_body_size > 0) {
        write_all_bytes_to_fd(stdin_pipe_fds[1], raw_request_body, raw_request_body_size);
    }
    close(stdin_pipe_fds[1]);

    char response_prelude[128];
    int prelude_length = snprintf(response_prelude, sizeof(response_prelude),
                                  "HTTP/1.1 200 OK\r\nServer: %s\r\n",
                                  HTTP_SERVER_NAME);
    response_write_body(connection, response_prelude, (size_t)prelude_length);

    relay_child_output_to_client(stdout_pipe_fds[0], connection);
    close(stdout_pipe_fds[0]);

    int child_exit_status = 0;
    waitpid(child_process_id, &child_exit_status, 0);
    if (WIFEXITED(child_exit_status) && WEXITSTATUS(child_exit_status) == 127) {
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }
    return HTTP_STATUS_OK;
}
