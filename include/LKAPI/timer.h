#ifndef KRAW_TIMER_H
#define KRAW_TIMER_H

#include "types.h"
#include "io_uring.h"

/* Linux Clock IDs */
#define KRAW_CLOCK_REALTIME  0
#define KRAW_CLOCK_MONOTONIC 1
#define KRAW_CLOCK_BOOTTIME  7

/* io_uring Timeout Flags */
#define KRAW_TIMEOUT_ABS       (1U << 0)
#define KRAW_TIMEOUT_UPDATE    (1U << 1)
#define KRAW_TIMEOUT_BOOTTIME  (1U << 2)
#define KRAW_TIMEOUT_REALTIME  (1U << 3)

/* Kernel 64-bit timespec structure for x86_64 */
struct kraw_timespec {
    i64 tv_sec;  /* Seconds */
    i64 tv_nsec; /* Nanoseconds */
};

/* Timer API */
i32 kraw_timer_now(struct kraw_timespec *ts);
i32 kraw_timer_arm(kraw_uring_t *ring, struct kraw_timespec *ts, u64 user_data, u32 flags);
i32 kraw_timer_cancel(kraw_uring_t *ring, u64 target_user_data, u64 user_data);

#endif /* KRAW_TIMER_H */
