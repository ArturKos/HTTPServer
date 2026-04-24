#ifndef HTTP_SERVER_SOCKET_UTILS_H
#define HTTP_SERVER_SOCKET_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file socket_utils.h
 * @brief Thin wrappers around the BSD socket API used by the server.
 */

/**
 * @brief Create an IPv4 TCP listening socket bound to the given port.
 *
 * The socket is created with SO_REUSEADDR so repeated restarts do not hit
 * TIME_WAIT on the listening port.
 *
 * @param listen_port TCP port to bind.
 * @param backlog     Maximum pending-connection queue length.
 * @return A non-negative file descriptor on success, -1 on error (errno is set).
 */
int create_listening_socket(uint16_t listen_port, int backlog);

/**
 * @brief Convert a numeric string into a valid uint16_t TCP port.
 *
 * @param port_string  Candidate port string.
 * @param out_port     Output port value.
 * @return true when parsing succeeds and the value is in [1, 65535].
 */
bool parse_tcp_port(const char* port_string, uint16_t* out_port);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_SOCKET_UTILS_H */
