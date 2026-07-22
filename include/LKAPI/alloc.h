#ifndef KRAW_ALLOC_H
#define KRAW_ALLOC_H

#include "types.h"

#define PAGE_SIZE 4096ULL

/* Memory protection flags */
#define KRAW_PROT_NONE  0x0
#define KRAW_PROT_READ  0x1
#define KRAW_PROT_WRITE 0x2
#define KRAW_PROT_EXEC  0x4

/* Memory mapping flags */
#define KRAW_MAP_SHARED    0x01
#define KRAW_MAP_PRIVATE   0x02
#define KRAW_MAP_ANON      0x20
#define KRAW_MAP_FAILED    ((void *)-1)

/* Bump allocator instance */
typedef struct {
    u8   *buffer;
    usize capacity;
    usize offset;
} kraw_bump_alloc_t;

/* Direct page allocation via Linux VMM */
void *kraw_mmap_pages(usize num_pages, u32 prot_flags);
i32   kraw_munmap_pages(void *ptr, usize num_pages);

/* Bump/Arena allocator interface */
void  kraw_bump_init(kraw_bump_alloc_t *alloc, void *memory_ptr, usize capacity);
void *kraw_bump_alloc(kraw_bump_alloc_t *alloc, usize size, usize align);
void  kraw_bump_reset(kraw_bump_alloc_t *alloc);

#endif /* KRAW_ALLOC_H */
