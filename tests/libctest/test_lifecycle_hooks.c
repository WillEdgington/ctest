#include "libctest/registry.h"
#include <clib/vector.h>
#include <ctest/ctest.h>
#include <stdio.h>

#define SUITE_NAME test_lifecycle_hooks

static int g_suite_setup_calls = 0;
static int g_suite_teardown_calls = 0;
static int g_test_setup_calls = 0;
static int g_test_teardown_calls = 0;

CTEST_SETUP_SUITE(hook_suite) { g_suite_setup_calls++; }
CTEST_TEARDOWN_SUITE(hook_suite) { g_suite_teardown_calls++; }

CTEST_SETUP(hook_suite) { g_test_setup_calls++; }
CTEST_TEARDOWN(hook_suite) { g_test_teardown_calls++; }

CTEST(hook_suite, dummy_case) {}

static void test_lifecycle_hooks(void) {
  CTestRegistry *reg = ctest_get_global_registry();
  TestSuite *suite = ctest_registry_get_or_add_suite(reg, "hook_suite");

  ASSERT_PTR_NOT_NULL(suite, "TestSuite should be present in global registry");
  if (suite == NULL)
    return;

  ASSERT_PTR_NOT_NULL(suite->setup_suite_fn, "setup_suite_fn should be set");
  ASSERT_PTR_NOT_NULL(suite->teardown_suite_fn,
                      "teardown_suite_fn should be set");
  ASSERT_PTR_NOT_NULL(suite->setup_fn, "setup_fn should be set");
  ASSERT_PTR_NOT_NULL(suite->teardown_fn, "teardown_fn should be set");

  suite->setup_suite_fn();
  ASSERT_INT_EQ(g_suite_setup_calls, 1, "suite setup hook function call");

  suite->setup_fn();
  ASSERT_INT_EQ(g_test_setup_calls, 1, "test setup hook function call");

  suite->teardown_fn();
  ASSERT_INT_EQ(g_test_teardown_calls, 1, "test teardown hook function call");

  suite->teardown_suite_fn();
  ASSERT_INT_EQ(g_suite_teardown_calls, 1, "suite teardown hook function call");

  ctest_registry_clear(reg);
}

int main(void) {
  printf("\nRunning: %s...\n", __FILE__);

  test_lifecycle_hooks();

  ctest_summary();
  return ctest_fail_count > 0 ? 1 : 0;
}