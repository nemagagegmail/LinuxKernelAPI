#include "../../include/LKAPI/fs.h"
#include "../../include/LKAPI/syscall.h"

/*
 * Async Queue OPENAT operation into io_uring
 */
i32 kraw_fs_openat(kraw_uring_t *ring, i32 dfd, const char *pathname, i32 flags, u32 mode, u64 user_data) {
    if (!ring || !pathname) return -1;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(ring);
    if (!sqe) return -1;

    sqe->opcode     = IORING_OP_OPENAT;
    sqe->fd         = dfd;
    sqe->addr       = (u64)pathname;
    sqe->len        = mode;
    sqe->open_flags = (u32)flags;
    sqe->user_data  = user_data;

    return 0;
}

/*
 * Async Queue READ operation with offset into io_uring
 */
i32 kraw_fs_read(kraw_uring_t *ring, i32 fd, void *buf, u32 len, u64 offset, u64 user_data) {
    if (!ring || !buf) return -1;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(ring);
    if (!sqe) return -1;

    sqe->opcode    = IORING_OP_READ;
    sqe->fd        = fd;
    sqe->addr      = (u64)buf;
    sqe->len       = len;
    sqe->off       = offset;
    sqe->user_data = user_data;

    return 0;
}

/*
 * Async Queue WRITE operation with offset into io_uring
 */
i32 kraw_fs_write(kraw_uring_t *ring, i32 fd, const void *buf, u32 len, u64 offset, u64 user_data) {
    if (!ring || !buf) return -1;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(ring);
    if (!sqe) return -1;

    sqe->opcode    = IORING_OP_WRITE;
    sqe->fd        = fd;
    sqe->addr      = (u64)buf;
    sqe->len       = len;
    sqe->off       = offset;
    sqe->user_data = user_data;

    return 0;
}

/*
 * Async Queue CLOSE operation into io_uring
 */
i32 kraw_fs_close(kraw_uring_t *ring, i32 fd, u64 user_data) {
    if (!ring) return -1;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(ring);
    if (!sqe) return -1;

    sqe->opcode    = IORING_OP_CLOSE;
    sqe->fd        = fd;
    sqe->user_data = user_data;

    return 0;
}

/*
 * Async Queue FSYNC operation into io_uring
 */
i32 kraw_fs_fsync(kraw_uring_t *ring, i32 fd, u32 fsync_flags, u64 user_data) {
    if (!ring) return -1;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(ring);
    if (!sqe) return -1;

    sqe->opcode      = IORING_OP_FSYNC;
    sqe->fd          = fd;
    sqe->fsync_flags = fsync_flags;
    sqe->user_data   = user_data;

    return 0;
}

/*
 * Synchronous direct open syscall
 */
i32 kraw_fs_open_sync(const char *pathname, i32 flags, u32 mode) {
    long ret = sys_call4(SYS_OPENAT, KRAW_AT_FDCWD, (long)pathname, flags, mode);
    return (i32)ret;
}

/*
 * Synchronous direct read syscall
 */
isize kraw_fs_read_sync(i32 fd, void *buf, usize count) {
    long ret = sys_call3(SYS_READ, fd, (long)buf, (long)count);
    return (isize)ret;
}

/*
 * Synchronous direct write syscall
 */
isize kraw_fs_write_sync(i32 fd, const void *buf, usize count) {
    long ret = sys_call3(SYS_WRITE, fd, (long)buf, (long)count);
    return (isize)ret;
}

/*
 * Synchronous direct close syscall
 */
i32 kraw_fs_close_sync(i32 fd) {
    long ret = sys_call1(SYS_CLOSE, fd);
    return (i32)ret;
}
