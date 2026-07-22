#include "../include/LKAPI/types.h"
#include "../include/LKAPI/syscall.h"
#include "../include/LKAPI/alloc.h"
#include "../include/LKAPI/io_uring.h"

#define PORT 8080
#define BACKLOG 128
#define BUF_SIZE 1024
#define RING_ENTRIES 64

/* Socket domain and options */
#define AF_INET        2
#define SOCK_STREAM    1
#define SOL_SOCKET     1
#define SO_REUSEADDR   2
#define SYS_SETSOCKOPT 54

/* Network Byte Order helper (htons) */
static ALWAYS_INLINE u16 htons(u16 hostshort) {
    return (u16)((hostshort >> 8) | (hostshort << 8));
}

/* IPv4 Socket address layout */
struct sockaddr_in {
    u16 sin_family;
    u16 sin_port;
    u32 sin_addr; /* INADDR_ANY = 0 */
    u8  sin_zero[8];
};

/* Event types for io_uring user_data context */
typedef enum {
    EVENT_ACCEPT = 1,
    EVENT_READ   = 2,
    EVENT_WRITE  = 3
} event_type_t;

/* Per-connection context state */
typedef struct {
    event_type_t type;
    i32          fd;
    u32          buf_len;
    u8           buf[BUF_SIZE];
} conn_ctx_t;

/* Global state pointers */
static kraw_uring_t      g_ring;
static kraw_bump_alloc_t g_bump;

/* System write helper to stdout */
static void print_msg(const char *msg, usize len) {
    sys_call3(SYS_WRITE, 1, (long)msg, (long)len);
}

/* Arm IORING_OP_ACCEPT for accepting new connections */
static void queue_accept(i32 server_fd, conn_ctx_t *accept_ctx) {
    accept_ctx->type = EVENT_ACCEPT;
    accept_ctx->fd   = server_fd;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(&g_ring);
    if (!sqe) return;

    sqe->opcode       = IORING_OP_ACCEPT;
    sqe->fd           = server_fd;
    sqe->addr         = 0; /* Client address buffer ignored for simplicity */
    sqe->off          = 0;
    sqe->accept_flags = 0;
    sqe->user_data    = (u64)accept_ctx;
}

/* Arm IORING_OP_READ for incoming connection data */
static void queue_read(conn_ctx_t *ctx) {
    ctx->type = EVENT_READ;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(&g_ring);
    if (!sqe) return;

    sqe->opcode    = IORING_OP_READ;
    sqe->fd        = ctx->fd;
    sqe->addr      = (u64)ctx->buf;
    sqe->len       = BUF_SIZE;
    sqe->off       = 0;
    sqe->user_data = (u64)ctx;
}

/* Arm IORING_OP_WRITE for echoing data back */
static void queue_write(conn_ctx_t *ctx, u32 len) {
    ctx->type    = EVENT_WRITE;
    ctx->buf_len = len;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(&g_ring);
    if (!sqe) return;

    sqe->opcode    = IORING_OP_WRITE;
    sqe->fd        = ctx->fd;
    sqe->addr      = (u64)ctx->buf;
    sqe->len       = len;
    sqe->off       = 0;
    sqe->user_data = (u64)ctx;
}

/* Entry point */
long kmain(int argc, char **argv) {
    (void)argc;
    (void)argv;

    print_msg("[KRAW] Starting bare-metal TCP Echo Server...\n", 45);

    /* 1. Allocate arena memory for bump allocator */
    void *arena_mem = kraw_mmap_pages(4, KRAW_PROT_READ | KRAW_PROT_WRITE); // 16 KB
    if (!arena_mem) {
        print_msg("[ERROR] Memory allocation failed\n", 33);
        return 1;
    }
    kraw_bump_init(&g_bump, arena_mem, 4 * PAGE_SIZE);

    /* 2. Initialize io_uring instance */
    if (kraw_uring_init(&g_ring, RING_ENTRIES, 0) < 0) {
        print_msg("[ERROR] io_uring setup failed\n", 30);
        return 1;
    }

    /* 3. Create, configure, bind & listen TCP socket */
    long sfd = sys_call3(SYS_SOCKET, AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        print_msg("[ERROR] sys_socket failed\n", 26);
        return 1;
    }
    i32 server_fd = (i32)sfd;

    i32 optval = 1;
    sys_call5(SYS_SETSOCKOPT, server_fd, SOL_SOCKET, SO_REUSEADDR, (long)&optval, sizeof(optval));

    struct sockaddr_in addr;
    for (u32 i = 0; i < sizeof(addr); i++) ((u8*)&addr)[i] = 0;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    addr.sin_addr   = 0; /* INADDR_ANY */

    if (sys_call3(SYS_BIND, server_fd, (long)&addr, sizeof(addr)) < 0) {
        print_msg("[ERROR] sys_bind failed\n", 24);
        return 1;
    }

    if (sys_call2(SYS_LISTEN, server_fd, BACKLOG) < 0) {
        print_msg("[ERROR] sys_listen failed\n", 26);
        return 1;
    }

    print_msg("[KRAW] Listening on port 8080\n", 30);

    /* 4. Prepare Accept Context and enqueue initial accept operation */
    conn_ctx_t *accept_ctx = (conn_ctx_t*)kraw_bump_alloc(&g_bump, sizeof(conn_ctx_t), 8);
    queue_accept(server_fd, accept_ctx);
    kraw_uring_submit(&g_ring, 1, 0);

    /* 5. Main Event Loop */
    while (1) {
        /* Submit queued SQEs and wait for at least 1 CQE completion */
        kraw_uring_submit(&g_ring, 0, 1);

        struct io_uring_cqe *cqe = NULL;
        while (kraw_uring_peek_cqe(&g_ring, &cqe) == 0) {
            conn_ctx_t *ctx = (conn_ctx_t *)cqe->user_data;
            i32 res = cqe->res;

            if (ctx) {
                switch (ctx->type) {
                    case EVENT_ACCEPT:
                        if (res >= 0) {
                            /* Allocate context for new client */
                            conn_ctx_t *client_ctx = (conn_ctx_t*)kraw_bump_alloc(&g_bump, sizeof(conn_ctx_t), 8);
                            if (client_ctx) {
                                client_ctx->fd = res;
                                queue_read(client_ctx);
                            }
                        }
                        /* Re-arm accept for new incoming connections */
                        queue_accept(server_fd, accept_ctx);
                        break;

                    case EVENT_READ:
                        if (res > 0) {
                            /* Echo received bytes back to client */
                            queue_write(ctx, (u32)res);
                        } else {
                            /* Client disconnected or error occurred */
                            sys_call1(SYS_CLOSE, ctx->fd);
                        }
                        break;

                    case EVENT_WRITE:
                        if (res > 0) {
                            /* Continue reading after write finishes */
                            queue_read(ctx);
                        } else {
                            sys_call1(SYS_CLOSE, ctx->fd);
                        }
                        break;
                }
            }

            kraw_uring_cqe_seen(&g_ring);
        }
    }

    return 0;
}
