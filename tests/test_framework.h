/**
 * test_framework.h — Micro framework de test (aucune dépendance externe)
 *
 * Usage :
 *   TEST("description") { ASSERT(expr); ASSERT_EQ(a, b); }
 *   return test_summary();
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static int _tests_run    = 0;
static int _tests_failed = 0;
static const char *_current_test = "";

#define TEST(name) \
    do { \
        _current_test = (name); \
        _tests_run++; \
        int _test_failed = 0; \
        do

#define END_TEST \
        while(0); \
        if (!_test_failed) printf("  [PASS] %s\n", _current_test); \
    } while(0)

#define ASSERT(expr) \
    do { if (!(expr)) { \
        printf("  [FAIL] %s\n         %s:%d — assertion failed: %s\n", \
               _current_test, __FILE__, __LINE__, #expr); \
        _tests_failed++; _test_failed = 1; \
    } } while(0)

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) { \
        printf("  [FAIL] %s\n         %s:%d — expected %lld, got %lld  (%s == %s)\n", \
               _current_test, __FILE__, __LINE__, \
               (long long)(b), (long long)(a), #a, #b); \
        _tests_failed++; _test_failed = 1; \
    } } while(0)

#define ASSERT_NEQ(a, b) \
    do { if ((a) == (b)) { \
        printf("  [FAIL] %s\n         %s:%d — expected != %lld  (%s)\n", \
               _current_test, __FILE__, __LINE__, (long long)(b), #a); \
        _tests_failed++; _test_failed = 1; \
    } } while(0)

#define SECTION(name) printf("\n=== %s ===\n", name)

static inline int test_summary(void) {
    printf("\n─────────────────────────────────────\n");
    printf("  %d / %d tests passed", _tests_run - _tests_failed, _tests_run);
    if (_tests_failed == 0)
        printf("  ✓ ALL PASS\n");
    else
        printf("  ✗ %d FAILED\n", _tests_failed);
    printf("─────────────────────────────────────\n");
    return _tests_failed ? 1 : 0;
}

#endif /* TEST_FRAMEWORK_H */
