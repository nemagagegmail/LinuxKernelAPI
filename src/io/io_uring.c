#include "../../include/LKAPI/io_uring.h"
#include "../../include/LKAPI/syscall.h"
#include "../../include/LKAPI/alloc.h"

/* Private helper for memory zeroing without libc memset */
static void kraw_memzero(void *ptr, usize size) {
    u8 *p = (u8 *)ptr;
    for (usize i = 0; i < size; i++) {
        p[i] = 0;
    }
}

/*
 * Initialize io_uring instance and map kernel SQ/CQ rings to userspace
 */
i32 kraw_uring_init(kraw_uring_t *ring, u32 entries, u32 flags) {
    if (!ring || entries == 0) return -1;

    struct io_uring_params p;
    kraw_memzero(&p, sizeof(p));
    p.flags = flags;

    long fd = sys_call2(SYS_IO_URING_SETUP, entries, (long)&p);
    if (fd < 0 && fd >= -4095) return (i32)fd;

    ring->fd = (i32)fd;
    ring->sq_entries = p.sq_entries;
    ring->cq_entries = p.cq_entries;

    /* 1. Calculate and mmap SQ Ring */
    ring->sq_ring_sz = p.sq_off.array + p.sq_entries * sizeof(u32);
    long sq_ptr = sys_call6(
        SYS_MMAP, 0, (long)ring->sq_ring_sz,
        KRAW_PROT_READ | KRAW_PROT_WRITE,
        KRAW_MAP_SHARED, ring->fd, IORING_OFF_SQ_RING
    );
    if (sq_ptr < 0 && sq_ptr >= -4095) goto err_close;

    ring->sq_ring_ptr = (u8 *)sq_ptr;
    ring->sq_khead    = (u32 *)(ring->sq_ring_ptr + p.sq_off.head);
    ring->sq_ktail    = (u32 *)(ring->sq_ring_ptr + p.sq_off.tail);
    ring->sq_kmask    = (u32 *)(ring->sq_ring_ptr + p.sq_off.ring_mask);
    ring->sq_karray   = (u32 *)(ring->sq_ring_ptr + p.sq_off.array);

    /* 2. Calculate and mmap SQEs array */
    ring->sqes_sz = p.sq_entries * sizeof(struct io_uring_sqe);
    long sqes_ptr = sys_call6(
        SYS_MMAP, 0, (long)ring->sqes_sz,
        KRAW_PROT_READ | KRAW_PROT_WRITE,
        KRAW_MAP_SHARED, ring->fd, IORING_OFF_SQES
    );
    if (sqes_ptr < 0 && sqes_ptr >= -4095) goto err_unmap_sq;

    ring->sqes = (struct io_uring_sqe *)sqes_ptr;

    /* 3. Calculate and mmap CQ Ring */
    ring->cq_ring_sz = p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe);
    long cq_ptr = sys_call6(
        SYS_MMAP, 0, (long)ring->cq_ring_sz,
        KRAW_PROT_READ | KRAW_PROT_WRITE,
        KRAW_MAP_SHARED, ring->fd, IORING_OFF_CQ_RING
    );
    if (cq_ptr < 0 && cq_ptr >= -4095) goto err_unmap_sqes;

    ring->cq_ring_ptr = (u8 *)cq_ptr;
    ring->cq_khead    = (u32 *)(ring->cq_ring_ptr + p.cq_off.head);
    ring->cq_ktail    = (u32 *)(ring->cq_ring_ptr + p.cq_off.tail);
    ring->cq_kmask    = (u32 *)(ring->cq_ring_ptr + p.cq_off.ring_mask);
    ring->cqes        = (struct io_uring_cqe *)(ring->cq_ring_ptr + p.cq_off.cqes);

    return 0;

err_unmap_sqes:
    sys_call2(SYS_MUNMAP, (long)ring->sqes, (long)ring->sqes_sz);
err_unmap_sq:
    sys_call2(SYS_MUNMAP, (long)ring->sq_ring_ptr, (long)ring->sq_ring_sz);
err_close:
    sys_call1(SYS_CLOSE, ring->fd);
    return -1;
}

/*
 * Acquire a clean SQE slot from the submission ring
 */
struct io_uring_sqe *kraw_uring_get_sqe(kraw_uring_t *ring) {
    if (!ring) return NULL;

    u32 head = *ring->sq_khead;
    u32 tail = *ring->sq_ktail;

    if (tail - head >= ring->sq_entries) {
        return NULL; /* SQ ring is full */
    }

    u32 index = tail & *ring->sq_kmask;
    struct io_uring_sqe *sqe = &ring->sqes[index];

    kraw_memzero(sqe, sizeof(*sqe));
    ring->sq_karray[index] = index;
    *ring->sq_ktail = tail + 1;

    return sqe;
}

/*
 * Submit pending SQEs to kernel via sys_io_uring_enter
 */
i32 kraw_uring_submit(kraw_uring_t *ring, u32 to_submit, u32 min_complete) {
    if (!ring) return -1;

    u32 enter_flags = 0;
    if (min_complete > 0) {
        enter_flags |= IORING_ENTER_GETEVENTS;
    }

    /* Enforce memory ordering before informing kernel */
    __asm__ volatile("" ::: "memory");

    long ret = sys_call6(
        SYS_IO_URING_ENTER,
        ring->fd,
        (long)to_submit,
        (long)min_complete,
        (long)enter_flags,
        0, 0
    );

    return (i32)ret;
}

/*
 * Peek at completed CQE without advancing head
 */
i32 kraw_uring_peek_cqe(kraw_uring_t *ring, struct io_uring_cqe **cqe_ptr) {
    if (!ring || !cqe_ptr) return -1;

    u32 head = *ring->cq_khead;
    u32 tail = *ring->cq_ktail;

    if (head == tail) {
        *cqe_ptr = NULL;
        return -1; /* CQ ring is empty */
    }

    *cqe_ptr = &ring->cqes[head & *ring->cq_kmask];
    return 0;
}

/*
 * Mark CQE as consumed and advance completion head
 */
void kraw_uring_cqe_seen(kraw_uring_t *ring) {
    if (!ring) return;

    u32 head = *ring->cq_khead;
    __asm__ volatile("" ::: "memory");
    *ring->cq_khead = head + 1;
}
