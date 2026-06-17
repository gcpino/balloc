/*
 * balloc.h - Block Allocator
 *
 * A small, single-header allocator library offering three strategies:
 *   - ba_arena:  bump allocator with alignment and reset
 *   - ba_pool:   fixed-size object pool
 *   - ba_heap:   general-purpose first-fit free list with coalescing
 *
 * All allocators are single-threaded and assume size_t is at least
 * 32 bits. Memory is not zeroed on allocation unless explicitly
 * requested (ba_arena_calloc, ba_heap_calloc).
 *
 * All public functions return NULL on allocation failure. Callers
 * should always check the return value.
 */

#ifndef BALLOC_H
#define BALLOC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----- Arena (bump allocator) ---------------------------------------- */

typedef struct ba_arena ba_arena;

/* Create an arena with the given byte capacity. Returns NULL on failure. */
ba_arena *ba_arena_create(size_t capacity);

/* Destroy the arena and free its backing memory. */
void ba_arena_destroy(ba_arena *a);

/* Allocate `size` bytes from the arena with default alignment
 * (BA_DEFAULT_ALIGN, typically 16). Returns NULL if the arena is
 * exhausted. The returned memory is uninitialized. */
void *ba_arena_alloc(ba_arena *a, size_t size);

/* Allocate and zero-initialize `count * size` bytes. */
void *ba_arena_calloc(ba_arena *a, size_t count, size_t size);

/* Allocate `size` bytes aligned to `align` (must be a power of two). */
void *ba_arena_alloc_aligned(ba_arena *a, size_t size, size_t align);

/* Rewind the arena to empty. Existing allocations are invalidated. */
void ba_arena_reset(ba_arena *a);

size_t ba_arena_used(const ba_arena *a);
size_t ba_arena_capacity(const ba_arena *a);
size_t ba_arena_remaining(const ba_arena *a);

/* ----- Pool (fixed-size objects) -------------------------------------- */

typedef struct ba_pool ba_pool;

/* Create a pool that can hold `count` objects of `object_size` bytes.
 * The pool internally rounds object_size up to BA_DEFAULT_ALIGN. */
ba_pool *ba_pool_create(size_t object_size, size_t count);

void ba_pool_destroy(ba_pool *p);

void *ba_pool_alloc(ba_pool *p);
void ba_pool_free(ba_pool *p, void *obj);

size_t ba_pool_available(const ba_pool *p);
size_t ba_pool_capacity(const ba_pool *p);
size_t ba_pool_object_size(const ba_pool *p);

/* ----- Heap (general-purpose) ----------------------------------------- */

typedef struct ba_heap ba_heap;

/* Create a heap with the given byte capacity. All allocations come
 * from this pre-reserved region. If exhausted, ba_heap_alloc returns
 * NULL. The heap uses first-fit with immediate coalescing. */
ba_heap *ba_heap_create(size_t capacity);

void ba_heap_destroy(ba_heap *h);

void *ba_heap_alloc(ba_heap *h, size_t size);
void *ba_heap_calloc(ba_heap *h, size_t count, size_t size);
void *ba_heap_realloc(ba_heap *h, void *ptr, size_t size);
void ba_heap_free(ba_heap *h, void *ptr);

size_t ba_heap_used(const ba_heap *h);
size_t ba_heap_capacity(const ba_heap *h);
size_t ba_heap_peak(const ba_heap *h);

/* ----- Constants ------------------------------------------------------ */

#define BA_DEFAULT_ALIGN 16
#define BA_ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((size_t)((a) - 1)))
#define BA_IS_POW2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))

#ifdef __cplusplus
}
#endif

#endif /* BALLOC_H */
