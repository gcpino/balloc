#include <string.h>
#include "balloc.h"
#include "test.h"

int test_heap(void) {
    int failed = 0;

    fprintf(stderr, "[heap] create/destroy\n");
    ba_heap *h = ba_heap_create(4096);
    BA_TEST_ASSERT_NOT_NULL(h);
    BA_TEST_ASSERT_EQ_U(ba_heap_capacity(h), 4096);
    BA_TEST_ASSERT_EQ_U(ba_heap_used(h), 0);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] null destroy\n");
    ba_heap_destroy(NULL);

    fprintf(stderr, "[heap] minimum capacity\n");
    BA_TEST_ASSERT_NULL(ba_heap_create(1));

    fprintf(stderr, "[heap] basic alloc\n");
    h = ba_heap_create(4096);
    void *p1 = ba_heap_alloc(h, 128);
    BA_TEST_ASSERT_NOT_NULL(p1);
    BA_TEST_ASSERT(ba_heap_used(h) >= 128);
    memset(p1, 0xAB, 128);
    ba_heap_free(h, p1);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] free coalesces with next\n");
    h = ba_heap_create(4096);
    void *a = ba_heap_alloc(h, 100);
    void *b = ba_heap_alloc(h, 100);
    void *c = ba_heap_alloc(h, 100);
    BA_TEST_ASSERT_NOT_NULL(a);
    BA_TEST_ASSERT_NOT_NULL(b);
    BA_TEST_ASSERT_NOT_NULL(c);
    ba_heap_free(h, a);
    ba_heap_free(h, c);
    ba_heap_free(h, b);
    BA_TEST_ASSERT_EQ_U(ba_heap_used(h), 0);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] free coalesces with both neighbors\n");
    h = ba_heap_create(4096);
    void *q1 = ba_heap_alloc(h, 64);
    void *q2 = ba_heap_alloc(h, 64);
    void *q3 = ba_heap_alloc(h, 64);
    void *q4 = ba_heap_alloc(h, 64);
    (void)q1; (void)q3; (void)q4;
    ba_heap_free(h, q2);
    BA_TEST_ASSERT(ba_heap_used(h) > 0);
    ba_heap_free(h, q1);
    ba_heap_free(h, q3);
    ba_heap_free(h, q4);
    BA_TEST_ASSERT_EQ_U(ba_heap_used(h), 0);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] first-fit reuses space\n");
    h = ba_heap_create(2048);
    void *x = ba_heap_alloc(h, 200);
    void *y = ba_heap_alloc(h, 200);
    ba_heap_free(h, x);
    void *x2 = ba_heap_alloc(h, 200);
    BA_TEST_ASSERT_NOT_NULL(x2);
    BA_TEST_ASSERT(x2 == x);
    ba_heap_free(h, x2);
    ba_heap_free(h, y);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] split creates usable remainder\n");
    h = ba_heap_create(8192);
    void *big = ba_heap_alloc(h, 4000);
    (void)big;
    void *small = ba_heap_alloc(h, 64);
    BA_TEST_ASSERT_NOT_NULL(small);
    void *small2 = ba_heap_alloc(h, 64);
    BA_TEST_ASSERT_NOT_NULL(small2);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] exhaustion\n");
    h = ba_heap_create(1024);
    void *fits = ba_heap_alloc(h, 900);
    BA_TEST_ASSERT_NOT_NULL(fits);
    void *nope = ba_heap_alloc(h, 900);
    BA_TEST_ASSERT_NULL(nope);
    ba_heap_free(h, fits);
    void *fits_again = ba_heap_alloc(h, 900);
    BA_TEST_ASSERT_NOT_NULL(fits_again);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] calloc zeros\n");
    h = ba_heap_create(4096);
    char *z = (char *)ba_heap_calloc(h, 32, 4);
    BA_TEST_ASSERT_NOT_NULL(z);
    int all_zero = 1;
    for (int i = 0; i < 128; i++) {
        if (z[i] != 0) { all_zero = 0; break; }
    }
    BA_TEST_ASSERT(all_zero);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] realloc grow copies contents\n");
    h = ba_heap_create(8192);
    char *r = (char *)ba_heap_alloc(h, 32);
    for (int i = 0; i < 32; i++) r[i] = (char)i;
    r = (char *)ba_heap_realloc(h, r, 256);
    BA_TEST_ASSERT_NOT_NULL(r);
    for (int i = 0; i < 32; i++) {
        BA_TEST_ASSERT(r[i] == (char)i);
    }
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] realloc shrink keeps ptr\n");
    h = ba_heap_create(4096);
    void *r1 = ba_heap_alloc(h, 256);
    void *r2 = ba_heap_realloc(h, r1, 64);
    BA_TEST_ASSERT(r2 == r1);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] realloc(NULL, n) == alloc(n)\n");
    h = ba_heap_create(4096);
    void *r3 = ba_heap_realloc(h, NULL, 128);
    BA_TEST_ASSERT_NOT_NULL(r3);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] realloc(p, 0) frees\n");
    h = ba_heap_create(4096);
    void *r4 = ba_heap_alloc(h, 128);
    void *r5 = ba_heap_realloc(h, r4, 0);
    BA_TEST_ASSERT_NULL(r5);
    BA_TEST_ASSERT_EQ_U(ba_heap_used(h), 0);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] double free is a no-op\n");
    h = ba_heap_create(4096);
    void *df = ba_heap_alloc(h, 64);
    ba_heap_free(h, df);
    ba_heap_free(h, df);
    BA_TEST_ASSERT_EQ_U(ba_heap_used(h), 0);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] peak tracking\n");
    h = ba_heap_create(4096);
    void *peak1 = ba_heap_alloc(h, 200);
    (void)peak1;
    void *peak2 = ba_heap_alloc(h, 200);
    (void)peak2;
    size_t observed_peak = ba_heap_peak(h);
    BA_TEST_ASSERT(observed_peak >= 400);
    ba_heap_free(h, peak1);
    ba_heap_free(h, peak2);
    BA_TEST_ASSERT(ba_heap_peak(h) == observed_peak);
    ba_heap_destroy(h);

    fprintf(stderr, "[heap] null-safety\n");
    BA_TEST_ASSERT_NULL(ba_heap_alloc(NULL, 100));
    ba_heap_free(NULL, NULL);
    BA_TEST_ASSERT_EQ_U(ba_heap_used(NULL), 0);
    BA_TEST_ASSERT_EQ_U(ba_heap_capacity(NULL), 0);
    BA_TEST_ASSERT_EQ_U(ba_heap_peak(NULL), 0);

    (void)failed;
    return 0;
}
