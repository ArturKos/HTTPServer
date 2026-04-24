#include "http_server/server.h"

#include "http_server/cgi_handler.h"
#include "http_server/connection_io.h"
#include "http_server/file_handler.h"
#include "http_server/logger.h"
#include "http_server/path_utils.h"
#include "http_server/php_handler.h"
#include "http_server/request.h"
#include "http_server/response.h"
#include "http_server/socket_utils.h"
#include "http_server/status_codes.h"
#include "http_server/thread_pool.h"
#include "http_server/tls_context.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CONNECTION_BACKLOG            512
#define REQUEST_READ_BUFFER_SIZE      8192
#define RESOLVED_PATH_BUFFER_SIZE     1024
#define DEFAULT_WORKER_THREAD_COUNT   8
#define DEFAULT_TASK_QUEUE_CAPACITY   1024
#define EPOLL_EVENT_BATCH_SIZE        32
#define KEEP_ALIVE_IDLE_TIMEOUT_SEC   15
#define KEEP_ALIVE_MAX_REQUESTS       100

static volatile sig_atomic_t g_shutdown_requested = 0;
static int                   g_shutdown_eventfd   = -1;

typedef struct ConnectionTask {
    int                 client_socket;
    struct sockaddr_in  client_address;
    const ServerConfig* server_config;
    TlsContext*         tls_context;
} ConnectionTask;

static void handle_shutdown_signal(int signal_number)
{
    (void)signal_number;
    g_shutdown_requested = 1;
    if (g_shutdown_eventfd >= 0) {
        const uint64_t wake_value = 1;
        ssize_t ignored = write(g_shutdown_eventfd, &wake_value, sizeof(wake_value));
        (void)ignored;
    }
}

static bool install_signal_handlers(void)
{
    struct sigaction shutdown_action;
    memset(&shutdown_action, 0, sizeof(shutdown_action));
    shutdown_action.sa_handler = handle_shutdown_signal;
    sigemptyset(&shutdown_action.sa_mask);
    if (sigaction(SIGINT, &shutdown_action, NULL) < 0) return false;
    if (sigaction(SIGTERM, &shutdown_action, NULL) < 0) return false;

    signal(SIGPIPE, SIG_IGN);
    return true;
}

static int dispatch_request(ConnectionContext* connection,
                            const HttpRequest* request,
                            const ServerConfig* config,
                            const char* client_ip,
                            const char* raw_request_body,
                            size_t raw_request_body_size,
                            bool keep_alive_enabled)
{
    if (!is_request_path_safe(request->path)) {
        response_write_error(connection, HTTP_STATUS_FORBIDDEN);
        return HTTP_STATUS_FORBIDDEN;
    }

    char resolved_path_buffer[RESOLVED_PATH_BUFFER_SIZE];
    if (!resolve_document_path(config->document_root,
                               request->path,
                               resolved_path_buffer,
                               sizeof(resolved_path_buffer))) {
        response_write_error(connection, HTTP_STATUS_INTERNAL_SERVER_ERROR);
        return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }

    if (cgi_handler_is_cgi_request(request->path)) {
        return cgi_handler_execute(connection, request, resolved_path_buffer,
                                   client_ip, raw_request_body, raw_request_body_size);
    }

    if (php_handler_is_php_request(request->path)) {
        return php_handler_execute(connection, request, resolved_path_buffer,
                                   client_ip, raw_request_body, raw_request_body_size);
    }

    if (request->method != HTTP_METHOD_GET && request->method != HTTP_METHOD_HEAD) {
        response_write_error(connection, HTTP_STATUS_METHOD_NOT_ALLOWED);
        return HTTP_STATUS_METHOD_NOT_ALLOWED;
    }

    return file_handler_serve(connection, request, resolved_path_buffer,
                              keep_alive_enabled);
}

static void configure_client_socket_timeout(int client_socket)
{
    struct timeval idle_timeout;
    idle_timeout.tv_sec  = KEEP_ALIVE_IDLE_TIMEOUT_SEC;
    idle_timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,
               &idle_timeout, sizeof(idle_timeout));
}

static void handle_client_connection(ConnectionContext* connection,
                                     const struct sockaddr_in* client_address,
                                     const ServerConfig* config)
{
    char client_ip_string[INET_ADDRSTRLEN] = "0.0.0.0";
    inet_ntop(AF_INET, &client_address->sin_addr, client_ip_string, sizeof(client_ip_string));

    int requests_served_on_connection = 0;
    bool should_keep_connection_alive = true;

    while (should_keep_connection_alive
           && requests_served_on_connection < KEEP_ALIVE_MAX_REQUESTS) {
        char request_buffer[REQUEST_READ_BUFFER_SIZE];
        ssize_t bytes_received = connection_read(connection, request_buffer,
                                                 sizeof(request_buffer) - 1);
        if (bytes_received <= 0) {
            break;
        }
        request_buffer[bytes_received] = '\0';

        HttpRequest parsed_request;
        if (!http_request_parse(request_buffer, (size_t)bytes_received, &parsed_request)) {
            response_write_error(connection, HTTP_STATUS_BAD_REQUEST);
            logger_log_request(client_ip_string, (int)pthread_self(),
                               HTTP_STATUS_BAD_REQUEST, "-");
            break;
        }

        const bool keep_alive_for_this_request = http_request_is_keep_alive(&parsed_request);

        const char* headers_end = strstr(request_buffer, "\r\n\r\n");
        const char* body_start = (headers_end != NULL) ? headers_end + 4 : NULL;
        const size_t body_size = (body_start != NULL)
                                 ? (size_t)(bytes_received - (body_start - request_buffer))
                                 : 0;

        const int response_status = dispatch_request(connection, &parsed_request,
                                                     config, client_ip_string,
                                                     body_start, body_size,
                                                     keep_alive_for_this_request);
        logger_log_request(client_ip_string, (int)pthread_self(),
                           response_status, parsed_request.path);

        ++requests_served_on_connection;
        should_keep_connection_alive = keep_alive_for_this_request
                                       && response_status < 400;
    }
}

static void connection_task_runner(void* opaque_task_argument)
{
    ConnectionTask* connection_task = (ConnectionTask*)opaque_task_argument;
    configure_client_socket_timeout(connection_task->client_socket);

    ConnectionContext connection;
    connection.raw_socket_fd = connection_task->client_socket;
    connection.tls_session   = NULL;

    bool proceed_with_serving = true;
    if (connection_task->tls_context != NULL) {
        proceed_with_serving = tls_context_accept(connection_task->tls_context,
                                                  connection_task->client_socket,
                                                  &connection);
    }

    if (proceed_with_serving) {
        handle_client_connection(&connection,
                                 &connection_task->client_address,
                                 connection_task->server_config);
    }

    tls_connection_shutdown(&connection);
    close(connection_task->client_socket);
    free(connection_task);
}

static bool accept_pending_connections(int listening_socket,
                                       const ServerConfig* server_config,
                                       TlsContext* tls_context,
                                       ThreadPool* worker_pool)
{
    for (;;) {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        int client_socket = accept4(listening_socket,
                                    (struct sockaddr*)&client_address,
                                    &client_address_length,
                                    SOCK_CLOEXEC);
        if (client_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
            if (errno == EINTR) continue;
            return false;
        }

        ConnectionTask* connection_task = (ConnectionTask*)malloc(sizeof(ConnectionTask));
        if (connection_task == NULL) {
            close(client_socket);
            continue;
        }
        connection_task->client_socket  = client_socket;
        connection_task->client_address = client_address;
        connection_task->server_config  = server_config;
        connection_task->tls_context    = tls_context;

        if (!thread_pool_submit_task(worker_pool, connection_task_runner, connection_task)) {
            close(client_socket);
            free(connection_task);
        }
    }
}

static int set_socket_nonblocking(int socket_fd)
{
    int current_flags = fcntl(socket_fd, F_GETFL, 0);
    if (current_flags < 0) return -1;
    return fcntl(socket_fd, F_SETFL, current_flags | O_NONBLOCK);
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

    const bool logger_initialised = server_config->use_syslog_backend
        ? logger_initialize_syslog("httpserver")
        : logger_initialize_file(server_config->log_file_path,
                                 server_config->log_max_file_size_bytes,
                                 server_config->log_max_rotated_files);
    if (!logger_initialised) {
        fprintf(stderr, "Failed to initialise logger\n");
        return EXIT_FAILURE;
    }

    TlsContext* tls_context = NULL;
    if (server_config->use_tls) {
        tls_context = tls_context_create(server_config->tls_certificate_path,
                                         server_config->tls_private_key_path);
        if (tls_context == NULL) {
            fprintf(stderr, "Failed to initialise TLS context\n");
            logger_shutdown();
            return EXIT_FAILURE;
        }
    }

    int listening_socket = create_listening_socket(server_config->listen_port,
                                                   CONNECTION_BACKLOG);
    if (listening_socket < 0) {
        fprintf(stderr, "Failed to create listening socket on port %u: %s\n",
                server_config->listen_port, strerror(errno));
        tls_context_destroy(tls_context);
        logger_shutdown();
        return EXIT_FAILURE;
    }
    if (set_socket_nonblocking(listening_socket) < 0) {
        fprintf(stderr, "Failed to set listening socket non-blocking: %s\n", strerror(errno));
        close(listening_socket);
        tls_context_destroy(tls_context);
        logger_shutdown();
        return EXIT_FAILURE;
    }

    g_shutdown_eventfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (g_shutdown_eventfd < 0) {
        fprintf(stderr, "Failed to create shutdown eventfd: %s\n", strerror(errno));
        close(listening_socket);
        tls_context_destroy(tls_context);
        logger_shutdown();
        return EXIT_FAILURE;
    }

    int epoll_descriptor = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_descriptor < 0) {
        fprintf(stderr, "epoll_create1 failed: %s\n", strerror(errno));
        close(listening_socket);
        close(g_shutdown_eventfd);
        tls_context_destroy(tls_context);
        logger_shutdown();
        return EXIT_FAILURE;
    }

    struct epoll_event listening_event;
    listening_event.events  = EPOLLIN;
    listening_event.data.fd = listening_socket;
    epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, listening_socket, &listening_event);

    struct epoll_event shutdown_event;
    shutdown_event.events  = EPOLLIN;
    shutdown_event.data.fd = g_shutdown_eventfd;
    epoll_ctl(epoll_descriptor, EPOLL_CTL_ADD, g_shutdown_eventfd, &shutdown_event);

    ThreadPool* worker_pool = thread_pool_create(DEFAULT_WORKER_THREAD_COUNT,
                                                 DEFAULT_TASK_QUEUE_CAPACITY);
    if (worker_pool == NULL) {
        fprintf(stderr, "Failed to create thread pool\n");
        close(epoll_descriptor);
        close(listening_socket);
        close(g_shutdown_eventfd);
        tls_context_destroy(tls_context);
        logger_shutdown();
        return EXIT_FAILURE;
    }

    fprintf(stdout,
            "Server listening on port %u (%s), document root: %s, workers: %d\n",
            server_config->listen_port,
            server_config->use_tls ? "TLS" : "plain",
            server_config->document_root,
            DEFAULT_WORKER_THREAD_COUNT);

    struct epoll_event ready_events[EPOLL_EVENT_BATCH_SIZE];
    while (!g_shutdown_requested) {
        int ready_event_count = epoll_wait(epoll_descriptor, ready_events,
                                           EPOLL_EVENT_BATCH_SIZE, -1);
        if (ready_event_count < 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "epoll_wait failed: %s\n", strerror(errno));
            break;
        }

        for (int event_index = 0; event_index < ready_event_count; ++event_index) {
            const int triggered_fd = ready_events[event_index].data.fd;
            if (triggered_fd == listening_socket) {
                accept_pending_connections(listening_socket, server_config,
                                           tls_context, worker_pool);
            } else if (triggered_fd == g_shutdown_eventfd) {
                g_shutdown_requested = 1;
            }
        }
    }

    thread_pool_shutdown_and_destroy(worker_pool);
    close(epoll_descriptor);
    close(listening_socket);
    close(g_shutdown_eventfd);
    tls_context_destroy(tls_context);
    logger_shutdown();
    fprintf(stdout, "Server shut down gracefully\n");
    return EXIT_SUCCESS;
}
