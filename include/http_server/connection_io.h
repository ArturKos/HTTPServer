#ifndef HTTP_SERVER_CONNECTION_IO_H
#define HTTP_SERVER_CONNECTION_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file connection_io.h
 * @brief Abstracts the I/O operations on a client connection so the rest of
 *        the server does not know whether the socket carries plain TCP or TLS.
 *
 * @warning The tls_session field is stored as an opaque pointer to avoid a hard
 *          dependency on OpenSSL headers in public API. It points to SSL* when
 *          tls_session is non-NULL.
 */

/**
 * @brief Per-connection I/O context.
 */
typedef struct ConnectionContext {
    int   raw_socket_fd; /**< Underlying TCP socket file descriptor. */
    void* tls_session;   /**< Opaque TLS session pointer, or NULL for plain text. */
} ConnectionContext;

/**
 * @brief Read up to @p buffer_size bytes from the connection.
 *
 * @param connection   Connection context (must not be NULL).
 * @param buffer       Destination buffer.
 * @param buffer_size  Size of @p buffer in bytes.
 * @return Number of bytes read, 0 on orderly shutdown, -1 on error.
 */
ssize_t connection_read(ConnectionContext* connection,
                        void* buffer,
                        size_t buffer_size);

/**
 * @brief Write the full buffer, retrying on short writes and EINTR.
 *
 * @param connection   Connection context.
 * @param buffer       Source buffer.
 * @param buffer_size  Number of bytes to send.
 * @return @p buffer_size on success, -1 on error.
 */
ssize_t connection_write_all(ConnectionContext* connection,
                             const void* buffer,
                             size_t buffer_size);

/**
 * @brief Report whether the connection is carrying TLS.
 */
bool connection_is_tls(const ConnectionContext* connection);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_CONNECTION_IO_H */
