#include "http_server/connection_io.h"

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#if HTTP_SERVER_TLS_ENABLED
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

bool connection_is_tls(const ConnectionContext* connection)
{
    return connection != NULL && connection->tls_session != NULL;
}

ssize_t connection_read(ConnectionContext* connection,
                        void* buffer,
                        size_t buffer_size)
{
    if (connection == NULL || buffer == NULL || buffer_size == 0) {
        errno = EINVAL;
        return -1;
    }

#if HTTP_SERVER_TLS_ENABLED
    if (connection->tls_session != NULL) {
        SSL* ssl_session = (SSL*)connection->tls_session;
        int  bytes_read  = SSL_read(ssl_session, buffer, (int)buffer_size);
        if (bytes_read <= 0) {
            int ssl_error = SSL_get_error(ssl_session, bytes_read);
            if (ssl_error == SSL_ERROR_ZERO_RETURN) return 0;
            return -1;
        }
        return bytes_read;
    }
#endif

    for (;;) {
        ssize_t bytes_read = recv(connection->raw_socket_fd, buffer, buffer_size, 0);
        if (bytes_read < 0 && errno == EINTR) continue;
        return bytes_read;
    }
}

ssize_t connection_write_all(ConnectionContext* connection,
                             const void* buffer,
                             size_t buffer_size)
{
    if (connection == NULL || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    const unsigned char* byte_cursor = (const unsigned char*)buffer;
    size_t remaining_bytes = buffer_size;

#if HTTP_SERVER_TLS_ENABLED
    if (connection->tls_session != NULL) {
        SSL* ssl_session = (SSL*)connection->tls_session;
        while (remaining_bytes > 0) {
            int bytes_written = SSL_write(ssl_session, byte_cursor, (int)remaining_bytes);
            if (bytes_written <= 0) return -1;
            byte_cursor     += (size_t)bytes_written;
            remaining_bytes -= (size_t)bytes_written;
        }
        return (ssize_t)buffer_size;
    }
#endif

    while (remaining_bytes > 0) {
        ssize_t bytes_written = send(connection->raw_socket_fd,
                                     byte_cursor, remaining_bytes, MSG_NOSIGNAL);
        if (bytes_written < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (bytes_written == 0) return -1;
        byte_cursor     += (size_t)bytes_written;
        remaining_bytes -= (size_t)bytes_written;
    }
    return (ssize_t)buffer_size;
}
