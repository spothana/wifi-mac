/* test_util.h - minimal header-only test harness (no external deps). */
#ifndef TEST_UTIL_H
#define TEST_UTIL_H
#include <stdio.h>

static int g_tests = 0, g_fails = 0;

#define CHECK(cond, msg) do {                                        \
    g_tests++;                                                       \
    if (cond) {                                                      \
        printf("  PASS: %s\n", msg);                                 \
    } else {                                                         \
        g_fails++;                                                   \
        printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__);    \
    }                                                                \
} while (0)

#define TEST_SUMMARY() do {                                          \
    printf("\n%d checks, %d failed\n", g_tests, g_fails);            \
    return g_fails == 0 ? 0 : 1;                                     \
} while (0)

#endif /* TEST_UTIL_H */
