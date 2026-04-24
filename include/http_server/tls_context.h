#ifndef HTTP_SERVER_TLS_CONTEXT_H
#define HTTP_SERVER_TLS_CONTEXT_H

#include <stdbool.h>

#include "http_server/connection_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file tls_context.h
 * @brief Optional OpenSSL-backed TLS listener support.
 *
 * The entire module is a no-op when the project is built without OpenSSL. The
 * @c HTTP_SERVER_TLS_ENABLED macro is 1 when TLS support was compiled in.
 */

#ifndef HTTP_SERVER_TLS_ENABLED
#define HTTP_SERVER_TLS_ENABLED 0
#endif

/**
 * @brief Opaque TLS server context (wraps SSL_CTX).
 */
typedef struct TlsContext TlsContext;

/**
 * @brief Create a server-side TLS context from certificate and private key files.
 *
 * @param certificate_path Path to the PEM-encoded server certificate.
 * @param private_key_path Path to the PEM-encoded private key.
 * @return Heap-allocated context or NULL on error. Returns NULL when TLS is disabled.
 */
TlsContext* tls_context_create(const char* certificate_path,
                               const char* private_key_path);

/**
 * @brief Destroy a TLS context and all cached OpenSSL state.
 *
 * @param tls_context Context previously returned by @ref tls_context_create (may be NULL).
 */
void tls_context_destroy(TlsContext* tls_context);

/**
 * @brief Upgrade an accepted plain socket to TLS and fill @p out_connection.
 *
 * @param tls_context    Server context.
 * @param accepted_socket Already-accepted TCP socket.
 * @param out_connection Connection context to populate; @c tls_session is set.
 * @return true on successful TLS handshake.
 */
bool tls_context_accept(TlsContext* tls_context,
                        int accepted_socket,
                        ConnectionContext* out_connection);

/**
 * @brief Tear down the TLS session on the connection and clear the pointer.
 */
void tls_connection_shutdown(ConnectionContext* connection);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_TLS_CONTEXT_H */
