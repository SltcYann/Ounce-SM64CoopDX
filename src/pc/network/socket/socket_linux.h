#ifndef SOCKET_LINUX_H
#define SOCKET_LINUX_H

#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

#define SOCKET unsigned int
#define INVALID_SOCKET (unsigned int)(-1)
#define SOCKET_LAST_ERROR errno
#define NO_ERROR (0)
#define SOCKADDR struct sockaddr
#define SOCKET_ERROR (-1)
#define closesocket(fd) close(fd)
#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#define SOCKET_ECONNRESET ECONNRESET
#define RX_ADDR_SIZE_TYPE unsigned int

#ifdef TARGET_SWITCH
#include <stddef.h>
int socket_switch_bind(SOCKET sock, unsigned int port);
int socket_switch_sendto(SOCKET sock, const void *buf, size_t len, const struct sockaddr_in6 *to);
int socket_switch_recvfrom(SOCKET sock, void *buf, size_t len, struct sockaddr_in6 *from);
#endif

#endif
