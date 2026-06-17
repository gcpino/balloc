#include "balloc.h"
#include "test.h"

int test_arena(void) {
    int failed = 0;

    fprintf(stderr, "[arena] create/destroy\n");
    ba_arena *a = ba_arena_create(1024);
    BA_TEST_ASSERT_NOT_NULL(a);
    BA_TEST_ASSERT_EQ_U(ba_arena_capacity(a), 1024);
    BA_TEST_ASSERT_EQ_U(ba_arena_used(a), 0);
    ba_arena_destroy(a);

    fprintf(stderr, "[arena] null destroy\n");
    ba_arena_destroy(NULL);

    fprintf(stderr, "[arena] zero capacity\n");
    BA_TEST_ASSERT_NULL(ba_arena_create(0));

    fprintf(stderr, "[arena] basic alloc\n");
    a = ba_arena_create(1024);
    void *p1 = ba_arena_alloc(a, 64);
    BA_TEST_ASSERT_NOT_NULL(p1);
    BA_TEST_ASSERT_EQ_U(ba_arena_used(a), 64);
    BA_TEST_ASSERT_EQ_U(ba_arena_remaining(a), 1024 - 64);

    fprintf(stderr, "[arena] alignment\n");
    a = ba_arena_create(1024);
    void *x = ba_arena_alloc(a, 1);
    BA_TEST_ASSERT_NOT_NULL(x);
    BA_TEST_ASSERT_EQ_U(((size_t)x) & (BA_DEFAULT_ALIGN - 1), 0);
    void *y = ba_arena_alloc(a, 1);
    BA_TEST_ASSERT_NOT_NULL(y);
    BA_TEST_ASSERT_EQ_U(((size_t)y) & (BA_DEFAULT_ALIGN - 1), 0);
    BA_TEST_ASSERT(y > x);
    ba_arena_destroy(a);

    fprintf(stderr, "[arena] aligned alloc\n");
    a = ba_arena_create(1024);
    void *a32 = ba_arena_alloc_aligned(a, 8, 64);
    BA_TEST_ASSERT_NOT_NULL(a32);
    BA_TEST_ASSERT_EQ_U(((size_t)a32) & 63, 0);

    fprintf(stderr, "[arena] bad alignment rejected\n");
    void *bad = ba_arena_alloc_aligned(a, 8, 7);
    BA_TEST_ASSERT_NULL(bad);
    ba_arena_destroy(a);

    fprintf(stderr, "[arena] exhaustion\n");
    a = ba_arena_create(128);
    void *big = ba_arena_alloc(a, 1024);
    BA_TEST_ASSERT_NULL(big);
    void *small = ba_arena_alloc(a, 100);
    BA_TEST_ASSERT_NOT_NULL(small);
    void *tiny = ba_arena_alloc(a, 100);
    BA_TEST_ASSERT_NULL(tiny);
    ba_arena_destroy(a);

    fprintf(stderr, "[arena] calloc zeros\n");
    a = ba_arena_create(256);
    char *z = (char *)ba_arena_calloc(a, 32, 4);
    BA_TEST_ASSERT_NOT_NULL(z);
    int all_zero = 1;
    for (int i = 0; i < 128; i++) {
        if (z[i] != 0) { all_zero = 0; break; }
    }
    BA_TEST_ASSERT(all_zero);
    ba_arena_destroy(a);

    fprintf(stderr, "[arena] reset reclaims space\n");
    a = ba_arena_create(256);
    (void)ba_arena_alloc(a, 200);
    BA_TEST_ASSERT(ba_arena_remaining(a) < 256);
    ba_arena_reset(a);
    BA_TEST_ASSERT_EQ_U(ba_arena_used(a), 0);
    void *reused = ba_arena_alloc(a, 200);
    BA_TEST_ASSERT_NOT_NULL(reused);
    ba_arena_destroy(a);

    fprintf(stderr, "[arena] null-safety on queries\n");
    BA_TEST_ASSERT_EQ_U(ba_arena_used(NULL), 0);
    BA_TEST_ASSERT_EQ_U(ba_arena_capacity(NULL), 0);
    BA_TEST_ASSERT_EQ_U(ba_arena_remaining(NULL), 0);
    ba_arena_reset(NULL);

    (void)failed;
    return 0;
}
