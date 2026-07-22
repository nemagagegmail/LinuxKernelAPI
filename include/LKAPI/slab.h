#ifndef KRAW_SLAB_H
#define KRAW_SLAB_H

#include "types.h"
#include "alloc.h"

/* Intrusive node placed inside unused slot memory */
typedef struct kraw_slab_node {
    struct kraw_slab_node *next;
} kraw_slab_node_t;

/* Slab Cache for fixed-size objects */
typedef struct {
    usize             obj_size;
    kraw_slab_node_t *free_list;
    usize             total_allocated;
    usize             free_count;
} kraw_slab_cache_t;

/* Low-level Slab API */
void  kraw_slab_cache_init(kraw_slab_cache_t *cache, usize obj_size);
void *kraw_slab_alloc(kraw_slab_cache_t *cache);
void  kraw_slab_free(kraw_slab_cache_t *cache, void *ptr);

/* General Purpose malloc/free wrapper API */
void  kraw_mem_init(void);
void *kraw_malloc(usize size);
void  kraw_free(void *ptr, usize size);

#endif /* KRAW_SLAB_H */
