#include "../../include/LKAPI/net.h"
#include "../../include/LKAPI/syscall.h"

/*
 * Create socket file descriptor
 */
i32 kraw_net_socket(i32 domain, i32 type, i32 protocol) {
    long ret = sys_call3(SYS_SOCKET, domain, type, protocol);
    return (i32)ret;
}

/*
 * Set socket non-blocking mode via fcntl
 */
i32 kraw_net_set_nonblocking(i32 fd) {
    long flags = sys_call3(SYS_FCNTL, fd, KRAW_F_GETFL, 0);
    if (flags < 0) return (i32)flags;

    long ret = sys_call3(SYS_FCNTL, fd, KRAW_F_SETFL, flags | KRAW_O_NONBLOCK);
    return (i32)ret;
}

/*
 * Enable/Disable TCP_NODELAY (Nagle algorithm)
 */
i32 kraw_net_set_nodelay(i32 fd) {
    i32 optval = 1;
    long ret = sys_call5(
        SYS_SETSOCKOPT,
        fd,
        KRAW_IPPROTO_TCP,
        KRAW_TCP_NODELAY,
        (long)&optval,
        sizeof(optval)
    );
    return (i32)ret;
}

/*
 * Allow immediate reuse of local address
 */
i32 kraw_net_set_reuseaddr(i32 fd) {
    i32 optval = 1;
    long ret = sys_call5(
        SYS_SETSOCKOPT,
        fd,
        KRAW_SOL_SOCKET,
        KRAW_SO_REUSEADDR,
        (long)&optval,
        sizeof(optval)
    );
    return (i32)ret;
}

/*
 * High-level helper: Create, configure, bind & listen TCP IPv4 socket
 */
i32 kraw_net_listen_v4(u16 port, i32 backlog) {
    i32 sfd = kraw_net_socket(KRAW_AF_INET, KRAW_SOCK_STREAM, 0);
    if (sfd < 0) return sfd;

    if (kraw_net_set_reuseaddr(sfd) < 0) {
        kraw_net_close(sfd);
        return -1;
    }

    struct kraw_sockaddr_in addr;
    u8 *ptr = (u8 *)&addr;
    for (usize i = 0; i < sizeof(addr); i++) ptr[i] = 0;

    addr.sin_family = KRAW_AF_INET;
    addr.sin_port   = kraw_htons(port);
    addr.sin_addr   = 0; /* INADDR_ANY */

    long ret = sys_call3(SYS_BIND, sfd, (long)&addr, sizeof(addr));
    if (ret < 0) {
        kraw_net_close(sfd);
        return (i32)ret;
    }

    ret = sys_call2(SYS_LISTEN, sfd, backlog);
    if (ret < 0) {
        kraw_net_close(sfd);
        return (i32)ret;
    }

    return sfd;
}

/*
 * Close network socket
 */
i32 kraw_net_close(i32 fd) {
    long ret = sys_call1(SYS_CLOSE, fd);
    return (i32)ret;
}

/*
 * Zero-libc IPv4 string parser ("127.0.0.1" -> u32)
 */
i32 kraw_net_parse_ip4(const char *ip_str, u32 *out_ip) {
    if (!ip_str || !out_ip) return -1;

    u32 octets[4] = {0};
    u32 octet_idx = 0;

    while (*ip_str) {
        char c = *ip_str;
        if (c >= '0' && c <= '9') {
            octets[octet_idx] = octets[octet_idx] * 10 + (u32)(c - '0');
            if (octets[octet_idx] > 255) return -1;
        } else if (c == '.') {
            octet_idx++;
            if (octet_idx > 3) return -1;
        } else {
            return -1;
        }
        ip_str++;
    }

    if (octet_idx != 3) return -1;

    /* Pack into network byte order */
    *out_ip = (octets[0]) | (octets[1] << 8) | (octets[2] << 16) | (octets[3] << 24);
    return 0;
}
