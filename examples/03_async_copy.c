#include "../include/LKAPI/types.h"
#include "../include/LKAPI/syscall.h"
#include "../include/LKAPI/alloc.h"
#include "../include/LKAPI/io_uring.h"
#include "../include/LKAPI/fs.h"

#define CHUNK_SIZE  (64 * 1024) /* 64 KB chunk size per request */
#define QUEUE_DEPTH 16          /* Maximum in-flight io_uring requests */
#define RING_SIZE   32

typedef enum {
    OP_READ  = 1,
    OP_WRITE = 2
} copy_op_t;

/* Per-chunk async state buffer */
typedef struct {
    copy_op_t op;
    u64       offset;
    u32       bytes_read;
    u8        buf[CHUNK_SIZE];
} chunk_req_t;

static kraw_uring_t g_ring;

static void print_str(const char *str) {
    usize len = 0;
    while (str[len]) len++;
    sys_call3(SYS_WRITE, 1, (long)str, (long)len);
}

static void print_num(u64 num) {
    char buf[32];
    int i = 30;
    buf[31] = '\n';
    if (num == 0) {
        buf[i--] = '0';
    } else {
        while (num > 0 && i >= 0) {
            buf[i--] = (char)('0' + (num % 10));
            num /= 10;
        }
    }
    sys_call3(SYS_WRITE, 1, (long)&buf[i + 1], (long)(31 - i));
}

long kmain(int argc, char **argv) {
    if (argc < 3) {
        print_str("Usage: ./03_async_copy <src_file> <dst_file>\n");
        return 1;
    }

    const char *src_path = argv[1];
    const char *dst_path = argv[2];

    print_str("[KRAW] Initializing async copy pipeline...\n");

    /* 1. Open Source File (Read-Only) */
    i32 src_fd = kraw_fs_open_sync(src_path, KRAW_O_RDONLY, 0);
    if (src_fd < 0) {
        print_str("[ERROR] Failed to open source file\n");
        return 1;
    }

    /* 2. Open Destination File (Write-Only, Create, Truncate, mode 0644) */
    u32 mode = KRAW_S_IRUSR | KRAW_S_IWUSR | KRAW_S_IRGRP | KRAW_S_IROTH;
    i32 dst_fd = kraw_fs_open_sync(dst_path, KRAW_O_WRONLY | KRAW_O_CREAT | KRAW_O_TRUNC, mode);
    if (dst_fd < 0) {
        print_str("[ERROR] Failed to open destination file\n");
        kraw_fs_close_sync(src_fd);
        return 1;
    }

    /* 3. Initialize io_uring engine */
    if (kraw_uring_init(&g_ring, RING_SIZE, 0) < 0) {
        print_str("[ERROR] Failed to setup io_uring\n");
        kraw_fs_close_sync(src_fd);
        kraw_fs_close_sync(dst_fd);
        return 1;
    }

    /* 4. Allocate request state descriptors via Virtual Memory */
    usize pages_needed = (sizeof(chunk_req_t) * QUEUE_DEPTH + PAGE_SIZE - 1) / PAGE_SIZE;
    chunk_req_t *reqs = (chunk_req_t *)kraw_mmap_pages(pages_needed, KRAW_PROT_READ | KRAW_PROT_WRITE);
    if (!reqs) {
        print_str("[ERROR] Memory allocation failed\n");
        return 1;
    }

    u64 read_offset      = 0;
    u64 total_bytes_copy = 0;
    u32 active_requests  = 0;
    u8  eof_reached      = 0;

    /* 5. Dispatch initial batch of async read SQEs */
    for (u32 i = 0; i < QUEUE_DEPTH; i++) {
        reqs[i].op     = OP_READ;
        reqs[i].offset = read_offset;
        
        if (kraw_fs_read(&g_ring, src_fd, reqs[i].buf, CHUNK_SIZE, read_offset, (u64)&reqs[i]) == 0) {
            read_offset += CHUNK_SIZE;
            active_requests++;
        }
    }

    /* 6. Event Loop (Process pipeline completions) */
    while (active_requests > 0) {
        kraw_uring_submit(&g_ring, 0, 1);

        struct io_uring_cqe *cqe = NULL;
        while (kraw_uring_peek_cqe(&g_ring, &cqe) == 0) {
            chunk_req_t *req = (chunk_req_t *)cqe->user_data;
            i32 res = cqe->res;

            if (req) {
                if (req->op == OP_READ) {
                    if (res > 0) {
                        /* Read succeeded -> Issue async Write SQE to destination */
                        req->op         = OP_WRITE;
                        req->bytes_read = (u32)res;
                        kraw_fs_write(&g_ring, dst_fd, req->buf, req->bytes_read, req->offset, (u64)req);
                    } else {
                        /* EOF or Read Error reached */
                        eof_reached = 1;
                        active_requests--;
                    }
                } else if (req->op == OP_WRITE) {
                    if (res > 0) {
                        total_bytes_copy += (u32)res;
                    }

                    /* Write finished -> Recycle buffer slot for next Read if EOF not reached */
                    if (!eof_reached) {
                        req->op     = OP_READ;
                        req->offset = read_offset;

                        if (kraw_fs_read(&g_ring, src_fd, req->buf, CHUNK_SIZE, read_offset, (u64)req) == 0) {
                            read_offset += CHUNK_SIZE;
                        } else {
                            active_requests--;
                        }
                    } else {
                        active_requests--;
                    }
                }
            }

            kraw_uring_cqe_seen(&g_ring);
        }
    }

    /* 7. Flush and close resources */
    kraw_fs_fsync(&g_ring, dst_fd, KRAW_FSYNC_DATASYNC, 0);
    kraw_uring_submit(&g_ring, 1, 1);

    kraw_fs_close_sync(src_fd);
    kraw_fs_close_sync(dst_fd);

    print_str("[KRAW] Copy completed successfully! Total bytes copied: ");
    print_num(total_bytes_copy);

    return 0;
}
