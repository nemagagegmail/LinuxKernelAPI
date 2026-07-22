#include "../../include/LKAPI/timer.h"
#include "../../include/LKAPI/syscall.h"

/*
 * O(1) Get monotonic system time directly via Linux kernel
 */
i32 kraw_timer_now(struct kraw_timespec *ts) {
    if (!ts) return -1;
    long ret = sys_call2(SYS_CLOCK_GETTIME, KRAW_CLOCK_MONOTONIC, (long)ts);
    return (i32)ret;
}

/*
 * Arm asynchronous timeout in io_uring queue
 */
i32 kraw_timer_arm(kraw_uring_t *ring, struct kraw_timespec *ts, u64 user_data, u32 flags) {
    if (!ring || !ts) return -1;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(ring);
    if (!sqe) return -1;

    sqe->opcode        = IORING_OP_TIMEOUT;
    sqe->fd            = -1;
    sqe->addr          = (u64)ts;
    sqe->len           = 0; /* Wait count (0 = standalone timer) */
    sqe->off           = 0;
    sqe->timeout_flags = flags;
    sqe->user_data     = user_data;

    return 0;
}

/*
 * Cancel pending io_uring timeout by target user_data token
 */
i32 kraw_timer_cancel(kraw_uring_t *ring, u64 target_user_data, u64 user_data) {
    if (!ring) return -1;

    struct io_uring_sqe *sqe = kraw_uring_get_sqe(ring);
    if (!sqe) return -1;

    sqe->opcode    = IORING_OP_TIMEOUT_REMOVE;
    sqe->fd        = -1;
    sqe->addr      = target_user_data; /* Token of target SQE to cancel */
    sqe->user_data = user_data;

    return 0;
}
