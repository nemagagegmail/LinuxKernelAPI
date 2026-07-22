#ifndef KRAW_FS_H
#define KRAW_FS_H

#include "types.h"
#include "io_uring.h"

/* Linux File Open Flags */
#define KRAW_O_RDONLY    00
#define KRAW_O_WRONLY    01
#define KRAW_O_RDWR      02
#define KRAW_O_CREAT     0100
#define KRAW_O_EXCL      0200
#define KRAW_O_TRUNC     01000
#define KRAW_O_APPEND    02000
#define KRAW_O_NONBLOCK  04000
#define KRAW_O_DIRECT    040000
#define KRAW_O_DIRECTORY 0200000

/* Special directory file descriptor */
#define KRAW_AT_FDCWD    -100

/* Standard File Permissions (mode) */
#define KRAW_S_IRUSR     0400
#define KRAW_S_IWUSR     0200
#define KRAW_S_IXUSR     0100
#define KRAW_S_IRGRP     0040
#define KRAW_S_IROTH     0004

/* Fsync Flags */
#define KRAW_FSYNC_DATASYNC (1U << 0)

/* Async io_uring File API */
i32 kraw_fs_openat(kraw_uring_t *ring, i32 dfd, const char *pathname, i32 flags, u32 mode, u64 user_data);
i32 kraw_fs_read(kraw_uring_t *ring, i32 fd, void *buf, u32 len, u64 offset, u64 user_data);
i32 kraw_fs_write(kraw_uring_t *ring, i32 fd, const void *buf, u32 len, u64 offset, u64 user_data);
i32 kraw_fs_close(kraw_uring_t *ring, i32 fd, u64 user_data);
i32 kraw_fs_fsync(kraw_uring_t *ring, i32 fd, u32 fsync_flags, u64 user_data);

/* Direct Synchronous Fallback API */
i32   kraw_fs_open_sync(const char *pathname, i32 flags, u32 mode);
isize kraw_fs_read_sync(i32 fd, void *buf, usize count);
isize kraw_fs_write_sync(i32 fd, const void *buf, usize count);
i32   kraw_fs_close_sync(i32 fd);

#endif /* KRAW_FS_H */
