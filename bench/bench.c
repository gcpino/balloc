#define _POSIX_C_SOURCE 199309L

#include "balloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ITERATIONS 200000
#define SMALL_SIZE 16
#define LARGE_SIZE 512
#define MIX_COUNT 1024

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void shuffle_indices(int *idx, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = idx[i]; idx[i] = idx[j]; idx[j] = t;
    }
}

static void bench_small_alloc_free(const char *label, int use_balloc) {
    void **ptrs = malloc(sizeof(void *) * ITERATIONS);
    uint64_t start = now_ns();
    if (use_balloc) {
        ba_heap *h = ba_heap_create(64UL * 1024 * 1024);
        for (int i = 0; i < ITERATIONS; i++) ptrs[i] = ba_heap_alloc(h, SMALL_SIZE);
        for (int i = 0; i < ITERATIONS; i++) ba_heap_free(h, ptrs[i]);
        ba_heap_destroy(h);
    } else {
        for (int i = 0; i < ITERATIONS; i++) ptrs[i] = malloc(SMALL_SIZE);
        for (int i = 0; i < ITERATIONS; i++) free(ptrs[i]);
    }
    uint64_t elapsed = now_ns() - start;
    double ns_per_op = (double)elapsed / (2.0 * ITERATIONS);
    printf("  %-30s %8.2f ns/op  (%llu total)\n",
           label, ns_per_op, (unsigned long long)elapsed);
    free(ptrs);
}

static void bench_large_alloc_free(const char *label, int use_balloc) {
    void **ptrs = malloc(sizeof(void *) * ITERATIONS);
    uint64_t start = now_ns();
    if (use_balloc) {
        ba_heap *h = ba_heap_create(256UL * 1024 * 1024);
        for (int i = 0; i < ITERATIONS; i++) ptrs[i] = ba_heap_alloc(h, LARGE_SIZE);
        for (int i = 0; i < ITERATIONS; i++) ba_heap_free(h, ptrs[i]);
        ba_heap_destroy(h);
    } else {
        for (int i = 0; i < ITERATIONS; i++) ptrs[i] = malloc(LARGE_SIZE);
        for (int i = 0; i < ITERATIONS; i++) free(ptrs[i]);
    }
    uint64_t elapsed = now_ns() - start;
    double ns_per_op = (double)elapsed / (2.0 * ITERATIONS);
    printf("  %-30s %8.2f ns/op  (%llu total)\n",
           label, ns_per_op, (unsigned long long)elapsed);
    free(ptrs);
}

static void bench_mixed_random(const char *label, int use_balloc) {
    int *idx = malloc(sizeof(int) * ITERATIONS);
    void **ptrs = malloc(sizeof(void *) * ITERATIONS);
    for (int i = 0; i < ITERATIONS; i++) idx[i] = i;
    srand(42);

    uint64_t start = now_ns();
    if (use_balloc) {
        ba_heap *h = ba_heap_create(64UL * 1024 * 1024);
        for (int i = 0; i < ITERATIONS; i++) {
            size_t s = 8 + (rand() % 256);
            ptrs[i] = ba_heap_alloc(h, s);
        }
        shuffle_indices(idx, ITERATIONS);
        for (int i = 0; i < ITERATIONS; i++) ba_heap_free(h, ptrs[idx[i]]);
        ba_heap_destroy(h);
    } else {
        for (int i = 0; i < ITERATIONS; i++) {
            size_t s = 8 + (rand() % 256);
            ptrs[i] = malloc(s);
        }
        shuffle_indices(idx, ITERATIONS);
        for (int i = 0; i < ITERATIONS; i++) free(ptrs[idx[i]]);
    }
    uint64_t elapsed = now_ns() - start;
    double ns_per_op = (double)elapsed / (2.0 * ITERATIONS);
    printf("  %-30s %8.2f ns/op  (%llu total)\n",
           label, ns_per_op, (unsigned long long)elapsed);
    free(ptrs);
    free(idx);
}

static void bench_pool_fixed(const char *label, int use_balloc) {
    int cap = MIX_COUNT;
    void **ptrs = malloc(sizeof(void *) * cap);
    srand(42);
    uint64_t start = now_ns();
    if (use_balloc) {
        ba_pool *p = ba_pool_create(64, cap);
        for (int i = 0; i < cap; i++) ptrs[i] = ba_pool_alloc(p);
        for (int i = 0; i < cap; i++) ba_pool_free(p, ptrs[i]);
        ba_pool_destroy(p);
    } else {
        for (int i = 0; i < cap; i++) ptrs[i] = malloc(64);
        for (int i = 0; i < cap; i++) free(ptrs[i]);
    }
    uint64_t elapsed = now_ns() - start;
    double ns_per_op = (double)elapsed / (2.0 * cap);
    printf("  %-30s %8.2f ns/op  (%llu total)\n",
           label, ns_per_op, (unsigned long long)elapsed);
    free(ptrs);
}

static void bench_arena_bulk(const char *label) {
    int n = MIX_COUNT;
    uint64_t start = now_ns();
    ba_arena *a = ba_arena_create(8UL * 1024 * 1024);
    for (int i = 0; i < n; i++) {
        ba_arena_alloc(a, 64);
    }
    uint64_t elapsed = now_ns() - start;
    double ns_per_op = (double)elapsed / n;
    printf("  %-30s %8.2f ns/op  (%llu total)\n",
           label, ns_per_op, (unsigned long long)elapsed);
    ba_arena_destroy(a);
}

int main(void) {
    printf("balloc benchmark (lower is better)\n");
    printf("==================================\n");

    printf("\nsmall alloc/free (16 B, %d iters):\n", ITERATIONS);
    bench_small_alloc_free("balloc heap", 1);
    bench_small_alloc_free("glibc malloc", 0);

    printf("\nlarge alloc/free (512 B, %d iters):\n", ITERATIONS);
    bench_large_alloc_free("balloc heap", 1);
    bench_large_alloc_free("glibc malloc", 0);

    printf("\nmixed sizes, random free order:\n");
    bench_mixed_random("balloc heap", 1);
    bench_mixed_random("glibc malloc", 0);

    printf("\nfixed-size object pool (64 B x %d):\n", MIX_COUNT);
    bench_pool_fixed("balloc pool", 1);
    bench_pool_fixed("glibc malloc", 0);

    printf("\narena bulk alloc (64 B x %d):\n", MIX_COUNT);
    bench_arena_bulk("balloc arena");

    printf("\n");
    return 0;
}
