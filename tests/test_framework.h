/**
 * test_framework.h — Minimal unit test framework (no external dependencies)
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>

static int test_passes = 0;
static int test_failures = 0;

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        printf("  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        test_failures++; \
    } else { \
        test_passes++; \
    } \
} while(0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

#define TEST_RUN(fn) do { \
    printf("  Running %s...\n", #fn); \
    fn(); \
} while(0)

#define TEST_SUMMARY() do { \
    printf("\n=== Results: %d passed, %d failed ===\n", test_passes, test_failures); \
    return test_failures > 0 ? 1 : 0; \
} while(0)

#endif /* TEST_FRAMEWORK_H */
