#define _POSIX_C_SOURCE 200809L

#include "balloc.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BA_ALIGN_MASK ((size_t)(BA_DEFAULT_ALIGN - 1))

static size_t align_up(size_t n, size_t align) {
    return (n + (align - 1)) & ~(align - 1);
}

static size_t align_up_default(size_t n) {
    return align_up(n, BA_DEFAULT_ALIGN);
}

static int alloc_aligned(void **out, size_t size, size_t align) {
    void *p = NULL;
    if (posix_memalign(&p, align, size) != 0) return -1;
    *out = p;
    return 0;
}

#define BA_BUFFER_ALIGN 64

/* ---------- Arena ---------- */

struct ba_arena {
    uint8_t *buffer;
    size_t capacity;
    size_t used;
};

ba_arena *ba_arena_create(size_t capacity) {
    if (capacity == 0) return NULL;
    ba_arena *a = (ba_arena *)malloc(sizeof(ba_arena));
    if (!a) return NULL;
    if (alloc_aligned((void **)&a->buffer, capacity, BA_BUFFER_ALIGN) != 0) {
        free(a);
        return NULL;
    }
    a->capacity = capacity;
    a->used = 0;
    return a;
}

void ba_arena_destroy(ba_arena *a) {
    if (!a) return;
    free(a->buffer);
    free(a);
}

void *ba_arena_alloc_aligned(ba_arena *a, size_t size, size_t align) {
    if (!a || size == 0 || !BA_IS_POW2(align)) return NULL;
    size_t aligned = align_up(a->used, align);
    if (aligned > a->capacity) return NULL;
    if (size > a->capacity - aligned) return NULL;
    void *ptr = a->buffer + aligned;
    a->used = aligned + size;
    return ptr;
}

void *ba_arena_alloc(ba_arena *a, size_t size) {
    return ba_arena_alloc_aligned(a, size, BA_DEFAULT_ALIGN);
}

void *ba_arena_calloc(ba_arena *a, size_t count, size_t size) {
    if (!a || count == 0 || size == 0) return NULL;
    if (count > SIZE_MAX / size) return NULL;
    size_t total = count * size;
    void *ptr = ba_arena_alloc(a, total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void ba_arena_reset(ba_arena *a) {
    if (!a) return;
    a->used = 0;
}

size_t ba_arena_used(const ba_arena *a) { return a ? a->used : 0; }
size_t ba_arena_capacity(const ba_arena *a) { return a ? a->capacity : 0; }
size_t ba_arena_remaining(const ba_arena *a) {
    if (!a) return 0;
    return a->capacity - a->used;
}

/* ---------- Pool ---------- */

struct ba_pool {
    uint8_t *buffer;
    size_t object_size;
    size_t capacity;
    size_t available;
    void *free_list;
};

ba_pool *ba_pool_create(size_t object_size, size_t count) {
    if (object_size == 0 || count == 0) return NULL;
    size_t slot = align_up_default(object_size);
    if (slot < sizeof(void *)) slot = sizeof(void *);

    ba_pool *p = (ba_pool *)malloc(sizeof(ba_pool));
    if (!p) return NULL;
    p->buffer = NULL;
    if (alloc_aligned((void **)&p->buffer, slot * count, BA_BUFFER_ALIGN) != 0) {
        free(p);
        return NULL;
    }
    p->object_size = slot;
    p->capacity = count;
    p->available = count;
    p->free_list = NULL;

    for (size_t i = 0; i < count; i++) {
        void *slot_ptr = p->buffer + i * slot;
        *(void **)slot_ptr = p->free_list;
        p->free_list = slot_ptr;
    }
    return p;
}

void ba_pool_destroy(ba_pool *p) {
    if (!p) return;
    free(p->buffer);
    free(p);
}

void *ba_pool_alloc(ba_pool *p) {
    if (!p || !p->free_list) return NULL;
    void *obj = p->free_list;
    p->free_list = *(void **)obj;
    p->available--;
    return obj;
}

void ba_pool_free(ba_pool *p, void *obj) {
    if (!p || !obj) return;
    *(void **)obj = p->free_list;
    p->free_list = obj;
    p->available++;
}

size_t ba_pool_available(const ba_pool *p) { return p ? p->available : 0; }
size_t ba_pool_capacity(const ba_pool *p) { return p ? p->capacity : 0; }
size_t ba_pool_object_size(const ba_pool *p) { return p ? p->object_size : 0; }

/* ---------- Heap ---------- */

typedef struct {
    size_t size;
    int is_free;
} block_header_t;

typedef struct {
    block_header_t *prev;
    block_header_t *next;
} free_link_t;

#define HDR_SIZE sizeof(block_header_t)
#define LINK_SIZE sizeof(free_link_t)
#define MIN_FREE_BLOCK (HDR_SIZE + LINK_SIZE)

struct ba_heap {
    uint8_t *buffer;
    size_t capacity;
    size_t used;
    size_t peak;
    block_header_t *free_head;
};

static block_header_t *block_at(uint8_t *base, size_t offset) {
    return (block_header_t *)(base + offset);
}

static void *block_payload(block_header_t *b) {
    return (uint8_t *)b + HDR_SIZE;
}

static block_header_t *block_from_payload(void *p) {
    return (block_header_t *)((uint8_t *)p - HDR_SIZE);
}

static free_link_t *block_link(block_header_t *b) {
    return (free_link_t *)block_payload(b);
}

static void free_list_unlink(ba_heap *h, block_header_t *b) {
    free_link_t *link = block_link(b);
    if (link->prev) {
        block_link(link->prev)->next = link->next;
    } else {
        h->free_head = link->next;
    }
    if (link->next) {
        block_link(link->next)->prev = link->prev;
    }
    link->prev = NULL;
    link->next = NULL;
}

static void free_list_push(ba_heap *h, block_header_t *b) {
    free_link_t *link = block_link(b);
    link->prev = NULL;
    link->next = h->free_head;
    if (h->free_head) {
        block_link(h->free_head)->prev = b;
    }
    h->free_head = b;
}

ba_heap *ba_heap_create(size_t capacity) {
    if (capacity < MIN_FREE_BLOCK) return NULL;
    ba_heap *h = (ba_heap *)malloc(sizeof(ba_heap));
    if (!h) return NULL;
    h->buffer = NULL;
    if (alloc_aligned((void **)&h->buffer, capacity, BA_BUFFER_ALIGN) != 0) {
        free(h);
        return NULL;
    }
    h->capacity = capacity;
    h->used = 0;
    h->peak = 0;
    h->free_head = NULL;
    return h;
}

void ba_heap_destroy(ba_heap *h) {
    if (!h) return;
    free(h->buffer);
    free(h);
}

void *ba_heap_alloc(ba_heap *h, size_t size) {
    if (!h || size == 0) return NULL;
    size = align_up_default(size);
    if (size < LINK_SIZE) size = LINK_SIZE;

    block_header_t *curr = h->free_head;
    while (curr && curr->size < size) {
        curr = block_link(curr)->next;
    }

    if (curr) {
        if (curr->size >= size + MIN_FREE_BLOCK) {
            block_header_t *split =
                (block_header_t *)((uint8_t *)curr + HDR_SIZE + size);
            split->size = curr->size - size - HDR_SIZE;
            split->is_free = 1;
            curr->size = size;
            free_list_push(h, split);
            h->used += HDR_SIZE + size;
        } else {
            free_list_unlink(h, curr);
            h->used += HDR_SIZE + curr->size;
        }
        if (h->used > h->peak) h->peak = h->used;
        curr->is_free = 0;
        return block_payload(curr);
    }

    if (h->used + HDR_SIZE + size <= h->capacity) {
        block_header_t *b = (block_header_t *)(h->buffer + h->used);
        b->size = size;
        b->is_free = 0;
        h->used += HDR_SIZE + size;
        if (h->used > h->peak) h->peak = h->used;
        return block_payload(b);
    }

    return NULL;
}

void ba_heap_free(ba_heap *h, void *ptr) {
    if (!h || !ptr) return;
    block_header_t *b = block_from_payload(ptr);
    if (b->is_free) return;

    size_t original_size = b->size;
    size_t b_end = (size_t)((uint8_t *)b + HDR_SIZE + b->size - h->buffer);
    if (b_end < h->used) {
        block_header_t *next = block_at(h->buffer, b_end);
        if (next->is_free) {
            free_list_unlink(h, next);
            b->size += HDR_SIZE + next->size;
        }
    }

    b->is_free = 1;
    h->used -= HDR_SIZE + original_size;
    free_list_push(h, b);
}

void *ba_heap_calloc(ba_heap *h, size_t count, size_t size) {
    if (!h || count == 0 || size == 0) return NULL;
    if (count > SIZE_MAX / size) return NULL;
    size_t total = count * size;
    void *ptr = ba_heap_alloc(h, total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void *ba_heap_realloc(ba_heap *h, void *ptr, size_t size) {
    if (!h) return NULL;
    if (!ptr) return ba_heap_alloc(h, size);
    if (size == 0) {
        ba_heap_free(h, ptr);
        return NULL;
    }
    block_header_t *b = block_from_payload(ptr);
    size_t old_size = b->size;
    if (size <= old_size) return ptr;

    void *new_ptr = ba_heap_alloc(h, size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, old_size);
    ba_heap_free(h, ptr);
    return new_ptr;
}

size_t ba_heap_used(const ba_heap *h) { return h ? h->used : 0; }
size_t ba_heap_capacity(const ba_heap *h) { return h ? h->capacity : 0; }
size_t ba_heap_peak(const ba_heap *h) { return h ? h->peak : 0; }
