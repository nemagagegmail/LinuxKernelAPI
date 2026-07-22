#ifndef KRAW_IO_URING_H
#define KRAW_IO_URING_H

#include "types.h"

/*
 * Magic offset values for mmap() on io_uring fd
 */
#define IORING_OFF_SQ_RING 0ULL
#define IORING_OFF_CQ_RING 0x20000000ULL
#define IORING_OFF_SQES    0x10000000ULL

/*
 * io_uring_setup() flags
 */
#define IORING_SETUP_IOPOLL     (1U << 0) /* Busy-wait for I/O completion */
#define IORING_SETUP_SQPOLL     (1U << 1) /* Dedicated kernel SQ poll thread */
#define IORING_SETUP_SQ_AFF     (1U << 2) /* Bound SQ poll thread to CPU */
#define IORING_SETUP_CQSIZE     (1U << 3) /* Custom CQ ring size */
#define IORING_SETUP_CLAMP      (1U << 4) /* Clamp ring sizes instead of failing */
#define IORING_SETUP_ATTACH_WQ  (1U << 5) /* Attach to existing wq */
#define IORING_SETUP_R_DISABLED (1U << 6) /* Start ring disabled */
#define IORING_SETUP_SUBMIT_ALL (1U << 7) /* Continue submitting on error */

/*
 * io_uring_enter() flags
 */
#define IORING_ENTER_GETEVENTS       (1U << 0)
#define IORING_ENTER_SQ_WAKEUP       (1U << 1)
#define IORING_ENTER_SQ_WAIT         (1U << 2)
#define IORING_ENTER_EXT_ARG         (1U << 3)
#define IORING_ENTER_REGISTER_RING_FD (1U << 4)

/*
 * SQE Command Opcodes
 */
#define IORING_OP_NOP             0
#define IORING_OP_READV           1
#define IORING_OP_WRITEV          2
#define IORING_OP_FSYNC           3
#define IORING_OP_READ_FIXED      4
#define IORING_OP_WRITE_FIXED     5
#define IORING_OP_POLL_ADD        6
#define IORING_OP_POLL_REMOVE     7
#define IORING_OP_SYNC_FILE_RANGE 8
#define IORING_OP_SENDMSG         9
#define IORING_OP_RECVMSG         10
#define IORING_OP_TIMEOUT         11
#define IORING_OP_TIMEOUT_REMOVE  12
#define IORING_OP_ACCEPT          13
#define IORING_OP_ASYNC_CANCEL    14
#define IORING_OP_LINK_TIMEOUT    15
#define IORING_OP_CONNECT         16
#define IORING_OP_FALLOCATE       17
#define IORING_OP_OPENAT          18
#define IORING_OP_CLOSE           19
#define IORING_OP_FILES_UPDATE    20
#define IORING_OP_STATX           21
#define IORING_OP_READ            22
#define IORING_OP_WRITE           23
#define IORING_OP_SOCKET          45

/*
 * SQE Flags (io_uring_sqe.flags)
 */
#define IOSQE_FIXED_FILE    (1U << 0) /* Use registered file table */
#define IOSQE_IO_DRAIN      (1U << 1) /* Issue after previous SQEs complete */
#define IOSQE_IO_LINK       (1U << 2) /* Next SQE depends on this SQE success */
#define IOSQE_IO_HARDLINK   (1U << 3) /* Next SQE runs regardless of result */
#define IOSQE_ASYNC         (1U << 4) /* Force async execution in worker thread */
#define IOSQE_BUFFER_SELECT (1U << 5) /* Select buffer from kernel pool */

/*
 * Kernel SQ Ring offset descriptor
 */
struct io_sqring_offsets {
    u32 head;
    u32 tail;
    u32 ring_mask;
    u32 ring_entries;
    u32 flags;
    u32 dropped;
    u32 array;
    u32 resv1;
    u64 user_addr;
};

/*
 * Kernel CQ Ring offset descriptor
 */
struct io_cqring_offsets {
    u32 head;
    u32 tail;
    u32 ring_mask;
    u32 ring_entries;
    u32 overflow;
    u32 cqes;
    u32 flags;
    u32 resv1;
    u64 user_addr;
};

/*
 * Kernel parameter structure passed to sys_io_uring_setup()
 */
struct io_uring_params {
    u32 sq_entries;
    u32 cq_entries;
    u32 flags;
    u32 sq_thread_cpu;
    u32 sq_thread_idle;
    u32 features;
    u32 wq_fd;
    u32 resv[3];
    struct io_sqring_offsets sq_off;
    struct io_cqring_offsets cq_off;
};

/*
 * Submission Queue Entry (Must be strictly 64 bytes)
 */
struct io_uring_sqe {
    u8  opcode;
    u8  flags;
    u16 ioprio;
    i32 fd;
    union {
        u64 off;
        u64 addr2;
    };
    union {
        u64 addr;
        u64 splice_off_in;
    };
    u32 len;
    union {
        u32 rw_flags;
        u32 fsync_flags;
        u32 poll_events;
        u32 poll32_events;
        u32 sync_range_flags;
        u32 msg_flags;
        u32 timeout_flags;
        u32 accept_flags;
        u32 cancel_flags;
        u32 open_flags;
        u32 statx_flags;
        u32 fadvise_advice;
        u32 splice_flags;
        u32 rename_flags;
        u32 unlink_flags;
        u32 hardlink_flags;
        u32 xattr_flags;
        u32 msg_ring_flags;
        u32 urING_cmd_flags;
    };
    u64 user_data;
    union {
        u16 buf_index;
        u16 buf_group;
    };
    u16 personality;
    union {
        i32 splice_fd_in;
        u32 file_index;
        struct {
            u16 addr_len;
            u16 __pad3[1];
        };
    };
    union {
        struct {
            u64 addr3;
            u64 __pad2[1];
        };
        u64 optval;
        u64 __pad2[2];
    };
};

/*
 * Completion Queue Entry (16 bytes)
 */
struct io_uring_cqe {
    u64 user_data; /* User-provided token from SQE */
    i32 res;       /* Result code (bytes read/written or -errno) */
    u32 flags;     /* Completion flags */
};

/*
 * Userspace abstraction for managing mapped ring state
 */
typedef struct {
    i32 fd;
    
    /* Mapped SQ Ring pointers */
    u8  *sq_ring_ptr;
    u32 *sq_khead;
    u32 *sq_ktail;
    u32 *sq_kmask;
    u32 *sq_karray;
    struct io_uring_sqe *sqes;
    
    /* Mapped CQ Ring pointers */
    u8  *cq_ring_ptr;
    u32 *cq_khead;
    u32 *cq_ktail;
    u32 *cq_kmask;
    struct io_uring_cqe *cqes;

    u32 sq_entries;
    u32 cq_entries;
    
    usize sq_ring_sz;
    usize cq_ring_sz;
    usize sqes_sz;
} kraw_uring_t;

#endif /* KRAW_IO_URING_H */
