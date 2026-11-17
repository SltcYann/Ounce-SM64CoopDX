#ifndef WINSOCK
#include "socket_linux.h"
#include "../network.h"
#include "pc/debuglog.h"

SOCKET socket_initialize(void) {
    // initialize socket
#ifdef TARGET_SWITCH
    // Horizon's BSD sockets are IPv4 only; socket_switch_* translate the game's v4-mapped IPv6 addresses
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
    SOCKET sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
#endif
    if (sock == INVALID_SOCKET) {
        LOG_ERROR("socket failed with error %d", SOCKET_LAST_ERROR);
        return INVALID_SOCKET;
    }

    // set non-blocking mode
    int rc = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
    if (rc == (int)INVALID_SOCKET) {
        LOG_ERROR("fcntl failed with error: %d", rc);
        return INVALID_SOCKET;
    }

#ifndef TARGET_SWITCH
    // Set socket to dual-stack
    int v6only = 0;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&v6only, sizeof(v6only)) < 0) {
        LOG_ERROR("setsockopt(IPV6_V6ONLY) failed.");
        return INVALID_SOCKET;
    };
#endif

    LOG_INFO("socket initialized.");

    return sock;
}

#ifdef TARGET_SWITCH
#include <string.h>

static void v6_to_v4(const struct sockaddr_in6 *in, struct sockaddr_in *out) {
    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;
    out->sin_port = in->sin6_port;
    // ::ffff:a.b.c.d -> a.b.c.d (anything else is not reachable over IPv4)
    memcpy(&out->sin_addr, &in->sin6_addr.s6_addr[12], 4);
}

static void v4_to_v6(const struct sockaddr_in *in, struct sockaddr_in6 *out) {
    // same layout as resolve_domain() builds, so memcmp() against sAddr[] matches
    memset(out, 0, sizeof(*out));
    out->sin6_family = AF_INET6;
    out->sin6_port = in->sin_port;
    out->sin6_addr.s6_addr[10] = 0xff;
    out->sin6_addr.s6_addr[11] = 0xff;
    memcpy(&out->sin6_addr.s6_addr[12], &in->sin_addr, 4);
}

int socket_switch_bind(SOCKET sock, unsigned int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    return bind(sock, (struct sockaddr *)&addr, sizeof(addr));
}

int socket_switch_sendto(SOCKET sock, const void *buf, size_t len, const struct sockaddr_in6 *to) {
    struct sockaddr_in addr;
    v6_to_v4(to, &addr);
    return sendto(sock, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
}

int socket_switch_recvfrom(SOCKET sock, void *buf, size_t len, struct sockaddr_in6 *from) {
    struct sockaddr_in addr;
    socklen_t addrSize = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    int rc = recvfrom(sock, buf, len, 0, (struct sockaddr *)&addr, &addrSize);
    v4_to_v6(&addr, from);
    return rc;
}
#endif

void socket_shutdown(SOCKET socket) {
    if (socket == INVALID_SOCKET) { return; }
    int rc = closesocket(socket);
    if (rc == (int)SOCKET_ERROR) {
        LOG_ERROR("closesocket failed with error %d\n", SOCKET_LAST_ERROR);
    }
}

#endif
