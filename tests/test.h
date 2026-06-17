#ifndef BALLOC_TEST_H
#define BALLOC_TEST_H

#include <stdio.h>

extern int g_tests_run;
extern int g_tests_failed;

#define BA_TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        g_tests_failed++; \
        return 1; \
    } \
} while (0)

#define BA_TEST_ASSERT_EQ_U(a, b) do { \
    size_t _a = (size_t)(a); \
    size_t _b = (size_t)(b); \
    if (_a != _b) { \
        fprintf(stderr, "  FAIL: %s:%d: expected %zu, got %zu\n", \
                __FILE__, __LINE__, _b, _a); \
        g_tests_failed++; \
        return 1; \
    } \
} while (0)

#define BA_TEST_ASSERT_NOT_NULL(p) BA_TEST_ASSERT((p) != NULL)
#define BA_TEST_ASSERT_NULL(p) BA_TEST_ASSERT((p) == NULL)

int test_arena(void);
int test_pool(void);
int test_heap(void);

#endif
