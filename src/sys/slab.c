#include "../../include/LKAPI/slab.h"

/* Power-of-two general purpose slab caches */
static kraw_slab_cache_t g_slab_32;
static kraw_slab_cache_t g_slab_64;
static kraw_slab_cache_t g_slab_128;
static kraw_slab_cache_t g_slab_256;
static kraw_slab_cache_t g_slab_512;
static kraw_slab_cache_t g_slab_1024;
static kraw_slab_cache_t g_slab_2048;

static u8 g_mem_initialized = 0;

/*
 * Initialize Slab Cache for specific object size
 */
void kraw_slab_cache_init(kraw_slab_cache_t *cache, usize obj_size) {
    if (!cache) return;

    /* Minimum size must hold intrusive pointer (8 bytes) */
    if (obj_size < sizeof(kraw_slab_node_t)) {
        obj_size = sizeof(kraw_slab_node_t);
    }

    /* Align size to 8 bytes */
    obj_size = (obj_size + 7) & ~7ULL;

    cache->obj_size        = obj_size;
    cache->free_list       = NULL;
    cache->total_allocated = 0;
    cache->free_count      = 0;
}

/*
 * Allocate O(1) slot from Slab Cache. Grows memory via mmap if needed.
 */
void *kraw_slab_alloc(kraw_slab_cache_t *cache) {
    if (!cache) return NULL;

    /* Refill cache if empty */
    if (!cache->free_list) {
        usize num_pages = 1;
        if (cache->obj_size > PAGE_SIZE) {
            num_pages = (cache->obj_size + PAGE_SIZE - 1) / PAGE_SIZE;
        }

        u8 *page_mem = (u8 *)kraw_mmap_pages(num_pages, KRAW_PROT_READ | KRAW_PROT_WRITE);
        if (!page_mem) return NULL;

        usize total_bytes = num_pages * PAGE_SIZE;
        usize slots_count = total_bytes / cache->obj_size;

        /* Partition page into slots and build free list */
        for (usize i = 0; i < slots_count; i++) {
            kraw_slab_node_t *node = (kraw_slab_node_t *)(page_mem + (i * cache->obj_size));
            node->next = cache->free_list;
            cache->free_list = node;
        }

        cache->total_allocated += slots_count;
        cache->free_count      += slots_count;
    }

    /* Pop slot from head */
    kraw_slab_node_t *allocated = cache->free_list;
    cache->free_list = allocated->next;
    cache->free_count--;

    return (void *)allocated;
}

/*
 * Return slot back to Slab Cache in O(1)
 */
void kraw_slab_free(kraw_slab_cache_t *cache, void *ptr) {
    if (!cache || !ptr) return;

    kraw_slab_node_t *node = (kraw_slab_node_t *)ptr;
    node->next = cache->free_list;
    cache->free_list = node;
    cache->free_count++;
}

/*
 * Global memory sub-system initialization
 */
void kraw_mem_init(void) {
    if (g_mem_initialized) return;

    kraw_slab_cache_init(&g_slab_32,   32);
    kraw_slab_cache_init(&g_slab_64,   64);
    kraw_slab_cache_init(&g_slab_128,  128);
    kraw_slab_cache_init(&g_slab_256,  256);
    kraw_slab_cache_init(&g_slab_512,  512);
    kraw_slab_cache_init(&g_slab_1024, 1024);
    kraw_slab_cache_init(&g_slab_2048, 2048);

    g_mem_initialized = 1;
}

/*
 * General purpose allocator (size-routed)
 */
void *kraw_malloc(usize size) {
    if (!g_mem_initialized) kraw_mem_init();
    if (size == 0) return NULL;

    if (size <= 32)   return kraw_slab_alloc(&g_slab_32);
    if (size <= 64)   return kraw_slab_alloc(&g_slab_64);
    if (size <= 128)  return kraw_slab_alloc(&g_slab_128);
    if (size <= 256)  return kraw_slab_alloc(&g_slab_256);
    if (size <= 512)  return kraw_slab_alloc(&g_slab_512);
    if (size <= 1024) return kraw_slab_alloc(&g_slab_1024);
    if (size <= 2048) return kraw_slab_alloc(&g_slab_2048);

    /* Allocate directly via Virtual Memory Manager for large objects */
    usize num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    return kraw_mmap_pages(num_pages, KRAW_PROT_READ | KRAW_PROT_WRITE);
}

/*
 * General purpose free
 */
void kraw_free(void *ptr, usize size) {
    if (!ptr || size == 0) return;

    if (size <= 32)   { kraw_slab_free(&g_slab_32, ptr);   return; }
    if (size <= 64)   { kraw_slab_free(&g_slab_64, ptr);   return; }
    if (size <= 128)  { kraw_slab_free(&g_slab_128, ptr);  return; }
    if (size <= 256)  { kraw_slab_free(&g_slab_256, ptr);  return; }
    if (size <= 512)  { kraw_slab_free(&g_slab_512, ptr);  return; }
    if (size <= 1024) { kraw_slab_free(&g_slab_1024, ptr); return; }
    if (size <= 2048) { kraw_slab_free(&g_slab_2048, ptr); return; }

    /* Return large objects directly to Linux kernel */
    usize num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    kraw_munmap_pages(ptr, num_pages);
}
