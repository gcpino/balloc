#include "test.h"
#include <stdio.h>

int g_tests_run = 0;
int g_tests_failed = 0;

int main(void) {
    fprintf(stderr, "=== balloc test suite ===\n");
    int arena_failed = test_arena();
    int pool_failed = test_pool();
    int heap_failed = test_heap();
    g_tests_failed += arena_failed + pool_failed + heap_failed;
    fprintf(stderr, "=========================\n");
    if (g_tests_failed == 0) {
        fprintf(stderr, "All tests PASSED\n");
        return 0;
    }
    fprintf(stderr, "FAILED: %d test(s) reported failures\n", g_tests_failed);
    return 1;
}
