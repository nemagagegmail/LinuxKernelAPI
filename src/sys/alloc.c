#include "../../include/LKAPI/alloc.h"
#include "../../include/LKAPI/syscall.h"

/*
 * Allocate virtual pages directly from Linux kernel
 */
void *kraw_mmap_pages(usize num_pages, u32 prot_flags) {
    if (num_pages == 0) return NULL;

    usize length = num_pages * PAGE_SIZE;
    
    // MAP_PRIVATE | MAP_ANONYMOUS
    long ret = sys_call6(
        SYS_MMAP,
        0,                             // Kernel selects virtual address
        (long)length,
        (long)prot_flags,
        KRAW_MAP_PRIVATE | KRAW_MAP_ANON,
        -1,                            // Anonymous mapping (no fd)
        0                              // Offset
    );

    // Linux syscall errors are returned in [-4095, -1] range
    if (ret < 0 && ret >= -4095) {
        return NULL;
    }

    return (void *)ret;
}

/*
 * Release virtual pages back to Linux kernel
 */
i32 kraw_munmap_pages(void *ptr, usize num_pages) {
    if (!ptr || num_pages == 0) return -1;
    
    usize length = num_pages * PAGE_SIZE;
    long ret = sys_call2(SYS_MUNMAP, (long)ptr, (long)length);

    return (i32)ret;
}

/*
 * Initialize bump allocator on pre-allocated region
 */
void kraw_bump_init(kraw_bump_alloc_t *alloc, void *memory_ptr, usize capacity) {
    if (!alloc) return;
    alloc->buffer   = (u8 *)memory_ptr;
    alloc->capacity = capacity;
    alloc->offset   = 0;
}

/*
 * Fast O(1) allocation with custom alignment
 */
void *kraw_bump_alloc(kraw_bump_alloc_t *alloc, usize size, usize align) {
    if (!alloc || !alloc->buffer || size == 0) return NULL;

    // Default alignment to 8 bytes if 0 or non power-of-two provided
    if (align == 0 || (align & (align - 1)) != 0) {
        align = 8;
    }

    uintptr_t current_addr = (uintptr_t)(alloc->buffer + alloc->offset);
    uintptr_t aligned_addr = (current_addr + (align - 1)) & ~(align - 1);
    usize new_offset       = (aligned_addr - (uintptr_t)alloc->buffer) + size;

    if (new_offset > alloc->capacity) {
        return NULL; // Out of memory in this arena
    }

    alloc->offset = new_offset;
    return (void *)aligned_addr;
}

/*
 * Reset bump pointer (instant free for entire arena)
 */
void kraw_bump_reset(kraw_bump_alloc_t *alloc) {
    if (alloc) {
        alloc->offset = 0;
    }
}
