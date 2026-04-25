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

/**
 * @brief Attempt to inherit a listening socket from systemd socket activation.
 *
 * Reads the LISTEN_PID and LISTEN_FDS environment variables according to the
 * sd_listen_fds(3) protocol. When the values are present, point to the current
 * process and announce at least one fd, the function returns the first inherited
 * descriptor (file descriptor 3 by convention). On any mismatch -1 is returned
 * and the caller should fall back to @ref create_listening_socket.
 *
 * @return Inherited file descriptor on success, -1 otherwise.
 */
int try_take_systemd_listening_socket(void);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_SOCKET_UTILS_H */
