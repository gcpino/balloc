#include "balloc.h"
#include "test.h"

int test_pool(void) {
    int failed = 0;

    fprintf(stderr, "[pool] create/destroy\n");
    ba_pool *p = ba_pool_create(64, 16);
    BA_TEST_ASSERT_NOT_NULL(p);
    BA_TEST_ASSERT_EQ_U(ba_pool_capacity(p), 16);
    BA_TEST_ASSERT_EQ_U(ba_pool_available(p), 16);
    ba_pool_destroy(p);

    fprintf(stderr, "[pool] null destroy\n");
    ba_pool_destroy(NULL);

    fprintf(stderr, "[pool] invalid args\n");
    BA_TEST_ASSERT_NULL(ba_pool_create(0, 16));
    BA_TEST_ASSERT_NULL(ba_pool_create(16, 0));

    fprintf(stderr, "[pool] alloc drains available\n");
    p = ba_pool_create(32, 8);
    void *slots[8];
    for (int i = 0; i < 8; i++) {
        slots[i] = ba_pool_alloc(p);
        BA_TEST_ASSERT_NOT_NULL(slots[i]);
    }
    BA_TEST_ASSERT_EQ_U(ba_pool_available(p), 0);
    BA_TEST_ASSERT_NULL(ba_pool_alloc(p));
    ba_pool_destroy(p);

    fprintf(stderr, "[pool] free refills available\n");
    p = ba_pool_create(32, 4);
    void *a = ba_pool_alloc(p);
    void *b = ba_pool_alloc(p);
    BA_TEST_ASSERT_EQ_U(ba_pool_available(p), 2);
    ba_pool_free(p, a);
    BA_TEST_ASSERT_EQ_U(ba_pool_available(p), 3);
    ba_pool_free(p, b);
    BA_TEST_ASSERT_EQ_U(ba_pool_available(p), 4);
    ba_pool_destroy(p);

    fprintf(stderr, "[pool] distinct addresses\n");
    p = ba_pool_create(64, 32);
    void *seen[32];
    for (int i = 0; i < 32; i++) {
        seen[i] = ba_pool_alloc(p);
        BA_TEST_ASSERT_NOT_NULL(seen[i]);
        for (int j = 0; j < i; j++) {
            BA_TEST_ASSERT(seen[i] != seen[j]);
        }
    }
    ba_pool_destroy(p);

    fprintf(stderr, "[pool] lIFO reuse\n");
    p = ba_pool_create(32, 4);
    void *x = ba_pool_alloc(p);
    ba_pool_free(p, x);
    void *y = ba_pool_alloc(p);
    BA_TEST_ASSERT(y == x);
    ba_pool_destroy(p);

    fprintf(stderr, "[pool] object size rounded up to alignment\n");
    p = ba_pool_create(7, 4);
    BA_TEST_ASSERT(ba_pool_object_size(p) >= BA_DEFAULT_ALIGN);
    ba_pool_destroy(p);

    fprintf(stderr, "[pool] small slot still holds next pointer\n");
    p = ba_pool_create(1, 8);
    BA_TEST_ASSERT(ba_pool_object_size(p) >= sizeof(void *));
    void *o[8];
    for (int i = 0; i < 8; i++) o[i] = ba_pool_alloc(p);
    for (int i = 0; i < 8; i++) ba_pool_free(p, o[i]);
    for (int i = 0; i < 8; i++) {
        void *n = ba_pool_alloc(p);
        BA_TEST_ASSERT_NOT_NULL(n);
    }
    ba_pool_destroy(p);

    fprintf(stderr, "[pool] null-safety\n");
    BA_TEST_ASSERT_NULL(ba_pool_alloc(NULL));
    ba_pool_free(NULL, NULL);
    BA_TEST_ASSERT_EQ_U(ba_pool_available(NULL), 0);
    BA_TEST_ASSERT_EQ_U(ba_pool_capacity(NULL), 0);
    BA_TEST_ASSERT_EQ_U(ba_pool_object_size(NULL), 0);

    (void)failed;
    return 0;
}
