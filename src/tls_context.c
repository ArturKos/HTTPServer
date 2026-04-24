#include "http_server/tls_context.h"

#include <stdio.h>
#include <stdlib.h>

#if HTTP_SERVER_TLS_ENABLED
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>

struct TlsContext {
    SSL_CTX* openssl_server_context;
};

static pthread_once_t g_openssl_init_once = PTHREAD_ONCE_INIT;

static void perform_openssl_library_initialisation(void)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

TlsContext* tls_context_create(const char* certificate_path,
                               const char* private_key_path)
{
    if (certificate_path == NULL || private_key_path == NULL) return NULL;
    pthread_once(&g_openssl_init_once, perform_openssl_library_initialisation);

    TlsContext* tls_context = (TlsContext*)calloc(1, sizeof(TlsContext));
    if (tls_context == NULL) return NULL;

    const SSL_METHOD* server_method = TLS_server_method();
    tls_context->openssl_server_context = SSL_CTX_new(server_method);
    if (tls_context->openssl_server_context == NULL) {
        free(tls_context);
        return NULL;
    }

    SSL_CTX_set_min_proto_version(tls_context->openssl_server_context, TLS1_2_VERSION);
    SSL_CTX_set_mode(tls_context->openssl_server_context, SSL_MODE_AUTO_RETRY);

    if (SSL_CTX_use_certificate_chain_file(tls_context->openssl_server_context,
                                           certificate_path) <= 0) {
        fprintf(stderr, "TLS: failed to load certificate %s\n", certificate_path);
        SSL_CTX_free(tls_context->openssl_server_context);
        free(tls_context);
        return NULL;
    }
    if (SSL_CTX_use_PrivateKey_file(tls_context->openssl_server_context,
                                    private_key_path, SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "TLS: failed to load private key %s\n", private_key_path);
        SSL_CTX_free(tls_context->openssl_server_context);
        free(tls_context);
        return NULL;
    }
    if (SSL_CTX_check_private_key(tls_context->openssl_server_context) != 1) {
        fprintf(stderr, "TLS: private key does not match certificate\n");
        SSL_CTX_free(tls_context->openssl_server_context);
        free(tls_context);
        return NULL;
    }
    return tls_context;
}

void tls_context_destroy(TlsContext* tls_context)
{
    if (tls_context == NULL) return;
    if (tls_context->openssl_server_context != NULL) {
        SSL_CTX_free(tls_context->openssl_server_context);
    }
    free(tls_context);
}

bool tls_context_accept(TlsContext* tls_context,
                        int accepted_socket,
                        ConnectionContext* out_connection)
{
    if (tls_context == NULL || out_connection == NULL) return false;

    SSL* ssl_session = SSL_new(tls_context->openssl_server_context);
    if (ssl_session == NULL) return false;
    if (SSL_set_fd(ssl_session, accepted_socket) != 1) {
        SSL_free(ssl_session);
        return false;
    }
    if (SSL_accept(ssl_session) <= 0) {
        SSL_free(ssl_session);
        return false;
    }
    out_connection->raw_socket_fd = accepted_socket;
    out_connection->tls_session   = ssl_session;
    return true;
}

void tls_connection_shutdown(ConnectionContext* connection)
{
    if (connection == NULL || connection->tls_session == NULL) return;
    SSL* ssl_session = (SSL*)connection->tls_session;
    SSL_shutdown(ssl_session);
    SSL_free(ssl_session);
    connection->tls_session = NULL;
}

#else  /* HTTP_SERVER_TLS_ENABLED */

struct TlsContext { int unused_placeholder; };

TlsContext* tls_context_create(const char* certificate_path,
                               const char* private_key_path)
{
    (void)certificate_path;
    (void)private_key_path;
    fprintf(stderr, "TLS support not compiled in (re-build with -DENABLE_TLS=ON)\n");
    return NULL;
}

void tls_context_destroy(TlsContext* tls_context) { (void)tls_context; }

bool tls_context_accept(TlsContext* tls_context,
                        int accepted_socket,
                        ConnectionContext* out_connection)
{
    (void)tls_context;
    (void)accepted_socket;
    (void)out_connection;
    return false;
}

void tls_connection_shutdown(ConnectionContext* connection) { (void)connection; }

#endif
