#ifndef KRAW_NET_H
#define KRAW_NET_H

#include "types.h"

/* Network Domains & Types */
#define KRAW_AF_INET      2
#define KRAW_AF_INET6     10
#define KRAW_SOCK_STREAM  1
#define KRAW_SOCK_DGRAM   2

/* Protocol levels & options */
#define KRAW_SOL_SOCKET   1
#define KRAW_IPPROTO_TCP  6

#define KRAW_SO_REUSEADDR 2
#define KRAW_SO_REUSEPORT 15
#define KRAW_TCP_NODELAY  1

/* Fcntl commands */
#define KRAW_F_GETFL      3
#define KRAW_F_SETFL      4
#define KRAW_O_NONBLOCK   04000

/* Socket addresses structures */
struct kraw_sockaddr_in {
    u16 sin_family;
    u16 sin_port;
    u32 sin_addr;
    u8  sin_zero[8];
};

struct kraw_sockaddr_in6 {
    u16 sin6_family;
    u16 sin6_port;
    u32 sin6_flowinfo;
    u8  sin6_addr[16];
    u32 sin6_scope_id;
};

/* Endianness / Byte Order translation */
static ALWAYS_INLINE u16 kraw_htons(u16 v) {
    return (u16)((v >> 8) | (v << 8));
}

static ALWAYS_INLINE u16 kraw_ntohs(u16 v) {
    return kraw_htons(v);
}

static ALWAYS_INLINE u32 kraw_htonl(u32 v) {
    return ((v & 0x000000FFU) << 24) |
           ((v & 0x0000FF00U) <<  8) |
           ((v & 0x00FF0000U) >>  8) |
           ((v & 0xFF000000U) >> 24);
}

static ALWAYS_INLINE u32 kraw_ntohl(u32 v) {
    return kraw_htonl(v);
}

/* High-level Network API */
i32 kraw_net_socket(i32 domain, i32 type, i32 protocol);
i32 kraw_net_listen_v4(u16 port, i32 backlog);
i32 kraw_net_set_nonblocking(i32 fd);
i32 kraw_net_set_nodelay(i32 fd);
i32 kraw_net_set_reuseaddr(i32 fd);
i32 kraw_net_close(i32 fd);
i32 kraw_net_parse_ip4(const char *ip_str, u32 *out_ip);

#endif /* KRAW_NET_H */
