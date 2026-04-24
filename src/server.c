#include "http_server/server.h"

#include "http_server/cgi_handler.h"
#include "http_server/file_handler.h"
#include "http_server/logger.h"
#include "http_server/path_utils.h"
#include "http_server/php_handler.h"
#include "http_server/request.h"
#include "http_server/response.h"
#include "http_server/socket_utils.h"
#include "http_server/status_codes.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CONNECTION_BACKLOG         64
#define REQUEST_READ_BUFFER_SIZE   8192
#define RESOLVED_PATH_BUFFER_SIZE  1024

static volatile sig_atomic_t g_shutdown_requested = 0;

static void handle_shutdown_signal(int signal_number)
{
    (void)signal_number;
    g_shutdown_requested = 1;
}

static void reap_child_processes(int signal_number)
{
    (void)signal_number;
    const int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        /* reap */
    }
    errno = saved_errno;
}

static bool install_signal_handlers(void)
{
    struct sigaction shutdown_action;
    memset(&shutdown_action, 0, sizeof(shutdown_action));
    shutdown_action.sa_handler = handle_shutdown_signal;
    sigemptyset(&shutdown_action.sa_mask);
    if (sigaction(SIGINT, &shutdown_action, NULL) < 0) return false;
    if (sigaction(SIGTERM, &shutdown_action, NULL) < 0) return false;

    struct sigaction child_action;
    memset(&child_action, 0, sizeof(child_action));
    child_action.sa_handler = reap_child_processes;
    sigemptyset(&child_action.sa_mask);
    child_action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &child_action, NULL) < 0) return false;

    signal(SIGPIPE, SIG_IGN);
    return true;
}

static int dispatch_request(int client_socket,
                            const HttpRequest* request,
                            const ServerConfig* config,
                            const char* client_ip,
                            const char* raw_request_body,
                            size_t raw_request_body_size)
{
    if (!is_request_path_safe(request->path)) {
        response_write_error(client_socket, HTTP_STATUS_FORBIDDEN);
        return HTTP_STATUS_FORBIDDEN;
    }

    char resolved_path_buffer[RESOLVED_PATH_BUFFER_SIZE];
    if (!resolve_document_path(config->document_root,
                               request->path,
                               resolved_path_buffer,
                               sizeof(resolved_path_buffer))) {
        response_write_error(client_socket, HTTP_STATUS_INTERNAL_SERVER_ERROR);
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }

    if (cgi_handler_is_cgi_request(request->path)) {
        return cgi_handler_execute(client_socket, request, resolved_path_buffer,
                                   client_ip, raw_request_body, raw_request_body_size);
    }

    if (php_handler_is_php_request(request->path)) {
        return php_handler_execute(client_socket, request, resolved_path_buffer,
                                   client_ip, raw_request_body, raw_request_body_size);
    }

    if (request->method != HTTP_METHOD_GET && request->method != HTTP_METHOD_HEAD) {
        response_write_error(client_socket, HTTP_STATUS_METHOD_NOT_ALLOWED);
        return HTTP_STATUS_METHOD_NOT_ALLOWED;
    }

    return file_handler_serve(client_socket, request, resolved_path_buffer);
}

static void handle_client_connection(int client_socket,
                                     const struct sockaddr_in* client_address,
                                     const ServerConfig* config)
{
    char client_ip_string[INET_ADDRSTRLEN] = "0.0.0.0";
    inet_ntop(AF_INET, &client_address->sin_addr, client_ip_string, sizeof(client_ip_string));

    char request_buffer[REQUEST_READ_BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, request_buffer,
                                  sizeof(request_buffer) - 1, 0);
    if (bytes_received <= 0) {
        return;
    }
    request_buffer[bytes_received] = '\0';

    HttpRequest parsed_request;
    if (!http_request_parse(request_buffer, (size_t)bytes_received, &parsed_request)) {
        response_write_error(client_socket, HTTP_STATUS_BAD_REQUEST);
        logger_log_request(client_ip_string, (int)getpid(),
                           HTTP_STATUS_BAD_REQUEST, "-");
        return;
    }

    const char* headers_end = strstr(request_buffer, "\r\n\r\n");
    const char* body_start = (headers_end != NULL) ? headers_end + 4 : NULL;
    const size_t body_size = (body_start != NULL)
                             ? (size_t)(bytes_received - (body_start - request_buffer))
                             : 0;

    const int response_status = dispatch_request(client_socket, &parsed_request,
                                                 config, client_ip_string,
                                                 body_start, body_size);
    logger_log_request(client_ip_string, (int)getpid(),
                       response_status, parsed_request.path);
}

int server_run(const ServerConfig* server_config)
{
    if (server_config == NULL) {
        return EXIT_FAILURE;
    }

    if (!install_signal_handlers()) {
        fprintf(stderr, "Failed to install signal handlers: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (!logger_initialize(server_config->log_file_path)) {
        fprintf(stderr, "Failed to initialise logger\n");
        return EXIT_FAILURE;
    }

    int listening_socket = create_listening_socket(server_config->listen_port,
                                                   CONNECTION_BACKLOG);
    if (listening_socket < 0) {
        fprintf(stderr, "Failed to create listening socket on port %u: %s\n",
                server_config->listen_port, strerror(errno));
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Server listening on port %u, document root: %s\n",
            server_config->listen_port, server_config->document_root);

    while (!g_shutdown_requested) {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        int client_socket = accept(listening_socket,
                                   (struct sockaddr*)&client_address,
                                   &client_address_length);
        if (client_socket < 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "accept() failed: %s\n", strerror(errno));
            continue;
        }

        pid_t child_process_id = fork();
        if (child_process_id < 0) {
            fprintf(stderr, "fork() failed: %s\n", strerror(errno));
            close(client_socket);
            continue;
        }

        if (child_process_id == 0) {
            close(listening_socket);
            handle_client_connection(client_socket, &client_address, server_config);
            close(client_socket);
            _exit(EXIT_SUCCESS);
        }

        close(client_socket);
    }

    close(listening_socket);
    fprintf(stdout, "Server shut down gracefully\n");
    return EXIT_SUCCESS;
}
